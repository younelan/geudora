/*
 * idle_scheduler.c — Centralized background task handler
 *
 * Runs periodically on the GTK main thread via g_timeout_add.
 * Dispatches to macmbx_mailer for mail pipeline work,
 * handles UI notifications, auto-check timer, etc.
 *
 * Does NOT touch TOCs, Delivery Folder, or Spool Folder directly —
 * all mail storage is managed by macmbx.
 */

#include "idle_scheduler.h"

#include "Globals.h"
#include "StringDefs.h"
#include "threading.h"
#include "toc.h"
#include "macmbx.h"
#include "macmbx_mailer.h"
#include "macmbx_conf.h"
#include "gtk_mailbox.h"
#include "taskProgress.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

/* ── External mailer instance (set up by main_eudora.c) ── */

static MacmbxMailer *g_mailer = NULL;

void idle_scheduler_set_mailer(MacmbxMailer *m) { g_mailer = m; }
MacmbxMailer *idle_scheduler_get_mailer(void) { return g_mailer; }

/* ── Timer state ── */

/* Status bar API (in main_eudora.c) */
extern void eudora_status_set(const char *title, const char *subtitle,
                               const char *message, double progress);
extern void eudora_status_clear(void);

static guint idle_timer_id = 0;
static bool scheduler_running = false;
static bool need_notify = false;
static bool need_send = false;
static bool send_in_progress = false;
static bool need_check = false;
static bool check_in_progress = false;

/* ── Incoming mail delivery ──
 *
 * When the background check thread finishes (NeedToFilterIn > 0),
 * we ask macmbx_mailer to process the delivery pipeline:
 * deliver → filter → junk score → move to In.
 * macmbx handles all of this internally.
 */
static void process_incoming(void) {
  if (!NeedToFilterIn || !g_mailer)
    return;

  /* macmbx_mailer_check already handled delivery during the check.
   * NeedToFilterIn is set by the legacy pop.c thread — just clear it
   * and mark that we need to notify the UI. */
  NeedToFilterIn = 0;
  need_notify = true;
}

/* ── New mail notification ── */

static void notify_new_mail(void) {
  if (!need_notify)
    return;
  if (CheckThreadRunning || IMAPCheckThreadRunning)
    return;

  need_notify = false;
  eudora_status_clear();

  /* Refresh open mailbox tabs to show newly delivered messages */
  extern void eudora_refresh_open_mailboxes(void);
  eudora_refresh_open_mailboxes();

  /* Refresh sidebar unread counts */
  MacmbxStore *store = gtk_mailbox_get_store();
  if (store) {
    macmbx_store_update_counts(store);
    extern GtkWidget *app_get_mailbox_tree(void);
    GtkWidget *tree = app_get_mailbox_tree();
    if (tree) gtk_mailbox_tree_refresh(tree);
  }
}

/* ── Send queued messages (async via background thread) ── */

void idle_scheduler_request_send(void) {
  if (Offline) {
    eudora_status_set("Offline", "", "Message queued — will send when online", -1);
    return;
  }
  need_send = true;
}
void idle_scheduler_request_check(void) {
  if (Offline) {
    eudora_status_set("Offline", "", "Cannot check mail while offline", -1);
    return;
  }
  need_check = true;
}

/* ── Check mail (async via background thread) ── */

static int g_check_result = 0;

/* Task IDs for check progress (one per account) */
#define MAX_CHECK_TASKS 16
static int g_check_task_ids[MAX_CHECK_TASKS];
static int g_check_task_count = 0;

typedef struct { char text[256]; int task_id; bool is_error; } TaskUpdate;

static gboolean check_task_update_cb(gpointer data) {
  TaskUpdate *u = (TaskUpdate *)data;
  if (u->is_error)
    tp_add_error("dialog-error-symbolic", u->text);
  else
    tp_update_task(u->task_id, u->text);
  g_free(u);
  return G_SOURCE_REMOVE;
}

static gboolean check_done_cb(gpointer data) {
  (void)data;
  /* Remove all check task lines */
  for (int i = 0; i < g_check_task_count; i++)
    tp_remove_task(g_check_task_ids[i]);
  g_check_task_count = 0;

  if (g_check_result > 0) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d new message(s)", g_check_result);
    eudora_status_set("Ready", "", buf, -1);

    /* Open In mailbox and refresh */
    extern void eudora_open_mailbox_by_name(const char *name);
    extern void eudora_refresh_open_mailboxes(void);
    eudora_open_mailbox_by_name("In");
    eudora_refresh_open_mailboxes();
  } else {
    eudora_status_set("Ready", "", "No new mail", -1);
  }
  return G_SOURCE_REMOVE;
}

static gpointer check_thread_func(gpointer data) {
  MacmbxMailer *mailer = (MacmbxMailer *)data;

  /* Check each account individually so we can report per-account errors.
   * Skip IMAP accounts that have IDLE active — they get push updates. */
  MacmbxConf *conf = macmbx_mailer_get_conf(mailer);
  int acct_count = macmbx_conf_count_accounts(conf);
  int total = 0;
  int acct_idx = 0;

  /* Dominant */
  {
    if (macmbx_mailer_idle_active(mailer, 0)) {
      /* IDLE handles this account — skip */
      acct_idx++;
    } else {
      int n = macmbx_mailer_check_account(mailer, 0);
      if (n >= 0) {
        total += n;
        if (acct_idx < g_check_task_count) {
          TaskUpdate *u = g_new0(TaskUpdate, 1);
          snprintf(u->text, sizeof(u->text), "Done — %d message(s)", n);
          u->task_id = g_check_task_ids[acct_idx];
          g_idle_add(check_task_update_cb, u);
        }
      } else {
        MacmbxAccount acct;
        macmbx_conf_get_dominant(conf, &acct);
        TaskUpdate *u = g_new0(TaskUpdate, 1);
        snprintf(u->text, sizeof(u->text), "Check failed: %s (%s)",
                 acct.name[0] ? acct.name : acct.email, acct.server);
        u->is_error = true;
        g_idle_add(check_task_update_cb, u);
      }
      acct_idx++;
    }
  }

  /* Personalities */
  for (int i = 1; i <= acct_count; i++) {
    MacmbxAccount acct;
    if (macmbx_conf_get_account(conf, i, &acct) != 0) continue;
    if (!acct.enabled || !acct.server[0]) continue;

    if (macmbx_mailer_idle_active(mailer, i)) {
      acct_idx++;
      continue; /* IDLE handles this account */
    }

    int n = macmbx_mailer_check_account(mailer, i);
    if (n >= 0) {
      total += n;
      if (acct_idx < g_check_task_count) {
        TaskUpdate *u = g_new0(TaskUpdate, 1);
        snprintf(u->text, sizeof(u->text), "%s — %d message(s)", acct.name, n);
        u->task_id = g_check_task_ids[acct_idx];
        g_idle_add(check_task_update_cb, u);
      }
    } else {
      TaskUpdate *u = g_new0(TaskUpdate, 1);
      snprintf(u->text, sizeof(u->text), "Check failed: %s (%s)",
               acct.name[0] ? acct.name : acct.email, acct.server);
      u->is_error = true;
      g_idle_add(check_task_update_cb, u);
    }
    acct_idx++;
  }

  g_check_result = total;
  g_print("Check thread: total %d new messages\n", total);
  check_in_progress = false;
  need_notify = true;
  g_idle_add(check_done_cb, NULL);
  return NULL;
}

/* Add task lines for each personality before launching check */
static gboolean add_check_tasks_cb(gpointer data) {
  (void)data;
  MacmbxConf *conf = g_mailer ? macmbx_mailer_get_conf(g_mailer) : NULL;
  if (!conf) return G_SOURCE_REMOVE;

  g_check_task_count = 0;

  /* Dominant account */
  MacmbxAccount acct;
  if (macmbx_conf_get_dominant(conf, &acct) == 0 && acct.server[0]) {
    char desc[256];
    snprintf(desc, sizeof(desc), "Checking %s (%s)...",
             acct.name[0] ? acct.name : acct.email, acct.server);
    g_check_task_ids[g_check_task_count++] =
        tp_add_task("mail-inbox-symbolic", desc);
  }

  /* Personalities */
  int n = macmbx_conf_count_accounts(conf);
  for (int i = 1; i <= n && g_check_task_count < MAX_CHECK_TASKS; i++) {
    if (macmbx_conf_get_account(conf, i, &acct) == 0 && acct.enabled && acct.server[0]) {
      char desc[256];
      snprintf(desc, sizeof(desc), "Checking %s (%s)...",
               acct.name[0] ? acct.name : acct.email, acct.server);
      g_check_task_ids[g_check_task_count++] =
          tp_add_task("mail-inbox-symbolic", desc);
    }
  }
  return G_SOURCE_REMOVE;
}

/* Preload passwords from keychain into macmbx config (main thread only).
 * After this, get_password reads from config and never hits keychain again.
 * This avoids keychain prompts from background threads. */
#include "keychain.h"
static bool passwords_preloaded = false;
static void preload_passwords(void) {
  if (passwords_preloaded) return;
  passwords_preloaded = true;

  MacmbxConf *conf = macmbx_mailer_get_conf(g_mailer);
  if (!conf) return;
  char pw[256];
  MacmbxAccount acct;

  /* Dominant account */
  if (macmbx_conf_get_dominant(conf, &acct) == 0 && acct.server[0]) {
    char key[256];
    snprintf(key, sizeof(key), "%s@%s", acct.username, acct.server);
    if (keychain_find("gEudora", key, pw, sizeof(pw)) == 0) {
      macmbx_conf_set(conf, "checking_mail", "saved_password", pw);
    }
  }

  /* Personality accounts */
  int n = macmbx_conf_count_accounts(conf);
  for (int i = 1; i <= n; i++) {
    if (macmbx_conf_get_account(conf, i, &acct) == 0 && acct.enabled && acct.server[0]) {
      char key[256];
      snprintf(key, sizeof(key), "%s@%s", acct.username, acct.server);
      if (keychain_find("gEudora", key, pw, sizeof(pw)) == 0) {
        char sec[32]; snprintf(sec, sizeof(sec), "account_%d", i);
        macmbx_conf_set(conf, sec, "saved_password", pw);
      }
    }
  }
}

static void process_check(void) {
  if (!need_check || check_in_progress || !g_mailer) return;
  need_check = false;
  check_in_progress = true;
  eudora_status_set("Checking mail...", "", "Connecting...", -1);
  /* Preload passwords on main thread so background thread hits cache */
  preload_passwords();
  /* Add task lines on main thread, then start check thread */
  add_check_tasks_cb(NULL);
  g_thread_new("check-mail", check_thread_func, g_mailer);
}

static int g_send_result = 0;

static gboolean send_done_cb(gpointer data) {
  (void)data;
  char buf[64];
  snprintf(buf, sizeof(buf), "%d message(s) sent", g_send_result);
  eudora_status_set("Ready", "", g_send_result > 0 ? buf : "Send complete", -1);
  return G_SOURCE_REMOVE;
}

static gpointer send_thread_func(gpointer data) {
  MacmbxMailer *mailer = (MacmbxMailer *)data;
  g_send_result = macmbx_mailer_send(mailer);
  g_print("Send thread: macmbx_mailer_send returned %d\n", g_send_result);
  send_in_progress = false;
  need_notify = true;
  g_idle_add(send_done_cb, NULL);
  return NULL;
}

static void process_send(void) {
  if (!need_send || send_in_progress || !g_mailer) return;
  need_send = false;
  send_in_progress = true;
  eudora_status_set("Sending mail...", "", "Connecting...", -1);
  g_thread_new("send-queue", send_thread_func, g_mailer);
}

/* ── Main tick ── */

static gboolean idle_scheduler_tick(gpointer user_data) {
  (void)user_data;

  /* Priority order:
     1. Check mail (async)
     2. Send queued messages (async)
     3. Process incoming mail delivery
     3. Notify user of new mail */

  if (need_check && !check_in_progress)
    process_check();

  if (need_send && !send_in_progress)
    process_send();

  if (NeedToFilterIn)
    process_incoming();
  else if (need_notify || NeedToNotify) {
    if (NeedToNotify) { need_notify = true; NeedToNotify = false; }
    notify_new_mail();
  }

  return G_SOURCE_CONTINUE;
}

/* ── IMAP IDLE callback ──
 *
 * Fired from macmbx_mailer's IDLE thread when the server pushes
 * an update. We dispatch to GTK main thread via g_idle_add. */

typedef struct {
  int account_index;
  int new_msgs;
} IdleNotify;

static gboolean idle_notify_cb(gpointer data) {
  IdleNotify *n = (IdleNotify *)data;

  if (n->new_msgs > 0) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%d new message(s)", n->new_msgs);
    eudora_status_set("Ready", "", buf, -1);

    extern void eudora_open_mailbox_by_name(const char *name);
    extern void eudora_refresh_open_mailboxes(void);
    eudora_open_mailbox_by_name("In");
    eudora_refresh_open_mailboxes();
  }

  /* Refresh sidebar */
  MacmbxStore *store = gtk_mailbox_get_store();
  if (store) {
    macmbx_store_update_counts(store);
    extern GtkWidget *app_get_mailbox_tree(void);
    GtkWidget *tree = app_get_mailbox_tree();
    if (tree) gtk_mailbox_tree_refresh(tree);
  }

  g_free(n);
  return G_SOURCE_REMOVE;
}

static void on_imap_idle(int account_index, int new_msgs,
                          int deleted, int flag_updated, void *ctx) {
  (void)deleted; (void)flag_updated; (void)ctx;
  IdleNotify *n = g_new0(IdleNotify, 1);
  n->account_index = account_index;
  n->new_msgs = new_msgs;
  g_idle_add(idle_notify_cb, n);
}

static void start_imap_idle(void); /* forward decl */

/* Prompt when going online with queued messages.
 * Three choices: Send All Queue, Go Online (don't send), Stay Offline. */
enum { RESP_SEND_ALL = 1, RESP_GO_ONLINE = 2, RESP_STAY_OFFLINE = 3 };

static void on_queue_prompt_response(GObject *source, GAsyncResult *res, gpointer ud) {
  GtkAlertDialog *dlg = GTK_ALERT_DIALOG(source);
  int btn = gtk_alert_dialog_choose_finish(dlg, res, NULL);

  if (btn == 0) {
    /* Send All Queue */
    need_send = true;
  } else if (btn == 1) {
    /* Go Online — already online, do nothing */
  } else {
    /* Stay Offline (btn == 2 or cancelled) */
    Offline = true;
    if (g_mailer) {
      macmbx_mailer_idle_stop(g_mailer, -1);
      macmbx_mailer_disconnect(g_mailer);
    }
    eudora_status_set("Offline", "", "No connections", -1);
    /* Notify UI to update the button */
    extern void update_offline_button(void);
    update_offline_button();
    extern void prefs_set_bool(const char *, const char *, gboolean);
    prefs_set_bool("offline", "offline_mode", TRUE);
  }
}

static void prompt_queued_on_online(void) {
  int queued = macmbx_mailer_queued_count(g_mailer);
  char msg[128];
  snprintf(msg, sizeof(msg), "%d message(s) waiting in the queue.", queued);

  const char *buttons[] = { "Send All Queue", "Go Online", "Stay Offline", NULL };
  GtkAlertDialog *dlg = gtk_alert_dialog_new("%s", msg);
  gtk_alert_dialog_set_detail(dlg, "What would you like to do?");
  gtk_alert_dialog_set_buttons(dlg, buttons);
  gtk_alert_dialog_set_cancel_button(dlg, 2);
  gtk_alert_dialog_set_default_button(dlg, 0);

  extern GtkWidget *get_main_window(void);
  gtk_alert_dialog_choose(dlg, GTK_WINDOW(get_main_window()),
                           NULL, on_queue_prompt_response, NULL);
  g_object_unref(dlg);
}

void idle_scheduler_set_offline(bool offline) {
  Offline = offline;
  if (offline) {
    /* Stop all IDLE sessions and disconnect */
    if (g_mailer) {
      macmbx_mailer_idle_stop(g_mailer, -1);
      macmbx_mailer_disconnect(g_mailer);
    }
    eudora_status_set("Offline", "", "No connections", -1);
  } else {
    /* Go back online — restart IDLE for IMAP accounts */
    start_imap_idle();
    eudora_status_set("Ready", "", "Online", -1);

    /* If there are queued messages, prompt the user */
    if (g_mailer && macmbx_mailer_queued_count(g_mailer) > 0)
      prompt_queued_on_online();
  }
}

bool idle_scheduler_is_offline(void) { return Offline; }

/* Start IDLE for all IMAP accounts. Called once at startup. */
static void start_imap_idle(void) {
  if (!g_mailer) return;
  MacmbxConf *conf = macmbx_mailer_get_conf(g_mailer);
  if (!conf) return;

  preload_passwords();

  /* Dominant account */
  MacmbxAccount acct;
  if (macmbx_conf_get_dominant(conf, &acct) == 0 &&
      acct.server[0] && strcasecmp(acct.type, "IMAP") == 0) {
    macmbx_mailer_idle_start(g_mailer, 0, on_imap_idle, NULL);
  }

  /* Personalities */
  int n = macmbx_conf_count_accounts(conf);
  for (int i = 1; i <= n; i++) {
    if (macmbx_conf_get_account(conf, i, &acct) == 0 &&
        acct.enabled && acct.server[0] &&
        strcasecmp(acct.type, "IMAP") == 0) {
      macmbx_mailer_idle_start(g_mailer, i, on_imap_idle, NULL);
    }
  }
}

/* ── Public API ── */

void IdleSchedulerStart(void) {
  if (scheduler_running)
    return;

  idle_timer_id = g_timeout_add(500, idle_scheduler_tick, NULL);
  scheduler_running = true;

  /* Start IMAP IDLE for accounts that support it.
   * POP accounts still use the timer-based check. */
  start_imap_idle();
}

void IdleSchedulerStop(void) {
  if (!scheduler_running)
    return;

  /* Stop all IDLE sessions */
  if (g_mailer)
    macmbx_mailer_idle_stop(g_mailer, -1);

  g_source_remove(idle_timer_id);
  idle_timer_id = 0;
  scheduler_running = false;
}
