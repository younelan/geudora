/*
 * idle_scheduler.c — Centralized background task handler
 *
 * Replaces Mac's FilterXferMessages() from filtthread.c.
 * Called every 500ms via g_timeout_add from the GTK main loop.
 * All work runs on the main thread — thread-safe coordination
 * via global flags set by background threads.
 */

#include "idle_scheduler.h"

#include "FiltDefs.h"
#include "Globals.h"
#include "StringDefs.h"
#include "fileutil.h"
#include "filtrun.h"
#include "filters.h"
#include "mailxfer.h"
#include "progress.h"
#include "threading.h"
#include "toc.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* GetTOCByFSS is in toc.c but not in a public header */
extern short GetTOCByFSS(char * specPtr, TOCType **tocH);

static guint idle_timer_id = 0;
static bool scheduler_running = false;

/* Saved filter state across ticks (mirrors Mac's static fpb in FilterDeliveryTOCs) */
static FilterPB idle_fpb;
static bool idle_fpb_inited = false;
static bool need_to_open_in = false;

/*--- GetNextDeliveryTOC (port of filtthread.c) ---
 * Scan the delivery folder for the lowest-numbered mailbox file.
 * These files are created by RenameInTemp() in pop.c during threaded download.
 */
static TOCType *GetNextDeliveryTOC(void) {
  FSSpec deliverFolder;
  FSSpec deliverSpec;
  DIR *dp;
  struct dirent *entry;
  long minFileNum = 0x7fffffff;
  long fileNum;
  TOCType *tocH = NULL;
  char bestName[64] = {0};

  if (SubFolderSpec(DELIVERY_FOLDER, &deliverFolder)) {
    g_print("GetNextDeliveryTOC: SubFolderSpec failed\n");
    return NULL;
  }
  g_print("GetNextDeliveryTOC: folder='%s'\n", deliverFolder);

  dp = opendir(deliverFolder);
  if (!dp)
    return NULL;

  /* Find lowest-numbered file in delivery folder */
  while ((entry = readdir(dp)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    /* Skip .toc files — we only want the mailbox files */
    const char *dot = strrchr(entry->d_name, '.');
    if (dot && strcmp(dot, ".toc") == 0)
      continue;

    char *endp;
    fileNum = strtol(entry->d_name, &endp, 10);
    if (endp == entry->d_name || fileNum <= 0)
      continue; /* not a numbered file */

    if (fileNum < minFileNum) {
      minFileNum = fileNum;
      g_strlcpy(bestName, entry->d_name, sizeof(bestName));
    }
  }
  closedir(dp);

  if (!bestName[0])
    return NULL;

  /* Build FSSpec for this delivery mailbox */
  spec_for(deliverFolder, bestName, &deliverSpec);

  /* Open the TOC for this mailbox */
  if (GetTOCByFSS(&deliverSpec, &tocH) != 0)
    return NULL;

  if (tocH)
    tocH->which = DELIVERY_BATCH;

  g_print("IdleScheduler: found delivery TOC '%s' with %d messages\n",
          bestName, tocH ? tocH->count : 0);

  return tocH;
}

/*--- DeleteDeliveryTOC ---
 * Remove the numbered mailbox and its .toc file from the delivery folder
 * after all messages have been filtered/moved.
 */
static void DeleteDeliveryTOC(TOCType *tocH) {
  FSSpec spec;
  char tocPath[1024];

  if (!tocH)
    return;

  GetMailboxSpec(tocH, -1, spec);

  /* Close the TOC window if open */
  if (tocH->win)
    CloseMyWindow(GetMyWindowWindowPtr(tocH->win));

  /* Delete the mailbox file */
  if (spec[0]) {
    unlink(spec);

    /* Delete the corresponding .toc file */
    snprintf(tocPath, sizeof(tocPath), "%s.toc", spec);
    unlink(tocPath);

    g_print("IdleScheduler: deleted delivery files: %s\n", spec);
  }
}

/*--- Delivery TOC Processing (port of FilterDeliveryTOCs) ---*/

static void process_delivery_tocs(void) {
  TOCType *tocH;

  if (!NeedToFilterIn)
    return;

  /* Find and process delivery mailboxes one at a time.
     The 500ms tick interval ensures GTK stays responsive. */
  tocH = GetNextDeliveryTOC();
  if (tocH) {
    if (!idle_fpb_inited) {
      InitFPB(&idle_fpb, true, true);
      idle_fpb_inited = true;
    }

    if (tocH->count) {
      int err = FilterMessagesFrom(flkDelivery, tocH, 0, &idle_fpb, NoXfer);
      if (!err)
        NeedToNotify = true;

      /* If delivery TOC is now empty, delete it */
      if (!err && !tocH->count) {
        need_to_open_in = true;
        DeleteDeliveryTOC(tocH);
      }
    } else {
      /* Empty delivery TOC — just delete it */
      DeleteDeliveryTOC(tocH);
    }
  } else {
    /* No more delivery TOCs to process */
    NeedToFilterIn = 0;
  }
}

/*--- New Mail Notification ---*/

static void notify_new_mail_idle(void) {
  if (!NeedToNotify)
    return;
  if (CheckThreadRunning || IMAPCheckThreadRunning)
    return;

  NeedToNotify = false;

  if (idle_fpb_inited) {
    NotifyNewMailLo(
        idle_fpb.doNotifyThing || idle_fpb.openMessage || idle_fpb.openMailbox,
        NoXfer, GetRealInTOC(), &idle_fpb, need_to_open_in);
  } else {
    /* No fpb context — just do basic notification */
    NotifyNewMail(1, NoXfer, GetRealInTOC(), NULL);
  }
  need_to_open_in = false;

  /* Reset fpb for next cycle */
  idle_fpb_inited = false;
}

/*--- Main Tick ---*/

static gboolean idle_scheduler_tick(gpointer user_data) {
  (void)user_data;

  /* Priority order matches Mac's FilterXferMessages:
     1. Process incoming delivery TOCs (most urgent)
     2. Notify user of new mail
     Future: NeedToFilterOut, auto-check timer, IMAP filtering */

  if (NeedToFilterIn)
    process_delivery_tocs();
  else if (NeedToNotify)
    notify_new_mail_idle();

  return G_SOURCE_CONTINUE; /* keep the timer running */
}

/*--- Startup Recovery ---
 * Check for orphaned delivery files from a previous session that crashed
 * or exited before filtering completed.
 */
static void check_orphaned_deliveries(void) {
  FSSpec deliverFolder;
  DIR *dp;
  struct dirent *entry;
  int count = 0;

  if (SubFolderSpec(DELIVERY_FOLDER, &deliverFolder)) {
    g_print("IdleScheduler: check_orphaned SubFolderSpec failed\n");
    return;
  }
  g_print("IdleScheduler: check_orphaned folder='%s'\n", deliverFolder);

  dp = opendir(deliverFolder);
  if (!dp)
    return;

  while ((entry = readdir(dp)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;
    /* Skip .toc files */
    const char *dot = strrchr(entry->d_name, '.');
    if (dot && strcmp(dot, ".toc") == 0)
      continue;
    char *endp;
    long num = strtol(entry->d_name, &endp, 10);
    if (endp != entry->d_name && num > 0)
      count++;
  }
  closedir(dp);

  if (count > 0) {
    g_print("IdleScheduler: found %d orphaned delivery file(s) from previous session\n", count);
    NeedToFilterIn += count;
  }
}

/*--- Public API ---*/

void IdleSchedulerStart(void) {
  if (scheduler_running)
    return;

  /* Check for leftover delivery files from previous session */
  check_orphaned_deliveries();

  idle_timer_id = g_timeout_add(500, idle_scheduler_tick, NULL);
  scheduler_running = true;
  g_print("IdleScheduler: started (500ms interval)\n");
}

void IdleSchedulerStop(void) {
  if (!scheduler_running)
    return;
  g_source_remove(idle_timer_id);
  idle_timer_id = 0;
  scheduler_running = false;
  g_print("IdleScheduler: stopped\n");
}
