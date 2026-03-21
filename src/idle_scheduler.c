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

void idle_scheduler_request_send(void) { need_send = true; }
void idle_scheduler_request_check(void) { need_check = true; }

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
  } else {
    eudora_status_set("Ready", "", "No new mail", -1);
  }
  return G_SOURCE_REMOVE;
}

static gpointer check_thread_func(gpointer data) {
  MacmbxMailer *mailer = (MacmbxMailer *)data;

  /* Check each account individually so we can report per-account errors */
  MacmbxConf *conf = macmbx_mailer_get_conf(mailer);
  int acct_count = macmbx_conf_count_accounts(conf);
  int total = 0;
  int acct_idx = 0;

  /* Dominant */
  {
    int n = macmbx_mailer_check_account(mailer, 0);
    if (n >= 0) {
      total += n;
      /* Update task line: done */
      if (acct_idx < g_check_task_count) {
        TaskUpdate *u = g_new0(TaskUpdate, 1);
        snprintf(u->text, sizeof(u->text), "Done — %d message(s)", n);
        u->task_id = g_check_task_ids[acct_idx];
        g_idle_add(check_task_update_cb, u);
      }
    } else {
      /* Error — add error line */
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

  /* Personalities */
  for (int i = 1; i <= acct_count; i++) {
    MacmbxAccount acct;
    if (macmbx_conf_get_account(conf, i, &acct) != 0) continue;
    if (!acct.enabled || !acct.server[0]) continue;

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

static void process_check(void) {
  if (!need_check || check_in_progress || !g_mailer) return;
  need_check = false;
  check_in_progress = true;
  eudora_status_set("Checking mail...", "", "Connecting...", -1);
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

/* ── Public API ── */

void IdleSchedulerStart(void) {
  if (scheduler_running)
    return;

  idle_timer_id = g_timeout_add(500, idle_scheduler_tick, NULL);
  scheduler_running = true;
}

void IdleSchedulerStop(void) {
  if (!scheduler_running)
    return;
  g_source_remove(idle_timer_id);
  idle_timer_id = 0;
  scheduler_running = false;
}
