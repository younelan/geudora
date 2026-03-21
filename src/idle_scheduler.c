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
#include "gtk_mailbox.h"

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

static gboolean check_done_cb(gpointer data) {
  (void)data;
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
  g_check_result = macmbx_mailer_check(mailer);
  g_print("Check thread: macmbx_mailer_check returned %d\n", g_check_result);
  check_in_progress = false;
  need_notify = true;
  g_idle_add(check_done_cb, NULL);
  return NULL;
}

static void process_check(void) {
  if (!need_check || check_in_progress || !g_mailer) return;
  need_check = false;
  check_in_progress = true;
  eudora_status_set("Checking mail...", "", "Connecting...", -1);
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
