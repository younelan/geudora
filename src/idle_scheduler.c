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

static guint idle_timer_id = 0;
static bool scheduler_running = false;
static bool need_notify = false;
static bool need_send = false;
static bool send_in_progress = false;

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

static gpointer send_thread_func(gpointer data) {
  MacmbxMailer *mailer = (MacmbxMailer *)data;
  int sent = macmbx_mailer_send(mailer);
  g_print("Send thread: macmbx_mailer_send returned %d\n", sent);
  send_in_progress = false;
  need_notify = true; /* trigger UI refresh on next tick */
  return NULL;
}

static void process_send(void) {
  if (!need_send || send_in_progress || !g_mailer) return;
  need_send = false;
  send_in_progress = true;
  g_thread_new("send-queue", send_thread_func, g_mailer);
}

/* ── Main tick ── */

static gboolean idle_scheduler_tick(gpointer user_data) {
  (void)user_data;

  /* Priority order:
     1. Send queued messages (async)
     2. Process incoming mail delivery
     3. Notify user of new mail */

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
