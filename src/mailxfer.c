/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "mailxfer.h"
#include "../gEditCtrl/geditctrl.h"
#include "keychain.h"
#include <glib/gstdio.h>
#include "compact.h"
#include "ends.h"
/* filtrun.h removed — macmbx_filter handles filters */
#include "gtk_dialogs.h"
#include "gtk_nag.h"
#include "gtk_prefs.h"
/* imapmailboxes.h removed — crispy_imap handles IMAP */
/* junk.h removed — macmbx_junk handles junk */
#include "log.h"
#include "mime.h"
/* myssl.h removed — crispy handles TLS */
#include "MyRes.h"
#include "prefdefs.h"
#include "progress.h"
/* sendmail.h removed — crispy handles SMTP */
#include "signaturewin.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "threading.h"
/* trans.h removed — crispy handles network */
#include "toc.h"
#include "auditdefs.h"
#include "fileutil.h"
#include "Globals.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>

/* Crispy mail library */
#include "buildtoc.h"
#include "lineio.h"
/* pop.h removed — crispy_pop3 handles POP */
#include "crispy_smtp.h"
#include "crispy_pop3.h"
#include "crispy_transport.h"
#include "crispy_headparse.h"
#include "crispy_msg.h"

/* macmbx mailer — all mail pipeline goes through here */
#include "macmbx.h"
#include "macmbx_mailer.h"
#include "gtk_mailbox.h"

#define FILE_NUM 52

/* Globals come from Globals.h (included above) */
extern long CountFlaggedMessages(MacmbxTOC *toc);

/* IMAP functions — declared in imapdownload.h */
extern int DoIMAPFilterProgress(void);


/* Mail transfer functions */
extern int GetUUPCMail(bool a, short *b);
extern int NewTransStream(TransStream *stream);
extern long ReportStreamAudit(TransStream stream);
extern void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what);
extern short EffectiveTID(short id);
extern bool SaveMessageSum(void *vsum, MacmbxTOC **tocH);
extern short TransOutTablID(void);
extern char *GetFlatten(void);

/* Personality functions */
extern void GetPOPInfo(void *a, void *b);
extern char *GetPOPPref(char *dest);
extern void PushPers(PersHandle pers);
extern void PopPers(void);

/* UI / window functions */
extern void *Get1Resource(uint32_t type, short id);
extern void SelectBoxRange(MacmbxTOC *toc, short a, short b, bool c, short d,
                           short e);
extern void ScrollIt(GtkWidget * w, short a, long b);
extern bool SortedDescending(MacmbxTOC *toc);
extern void ShowMyWindowBehind(GtkWidget *a, GtkWidget *b);
extern void eudora_open_mailbox_by_name(const char *name);
extern MyWindowPtr FindText(char * spec);
extern void MySelectWindow(GtkWidget * w);
extern MyWindowPtr OpenText(char * spec, void *a, void *b, void *c, bool d,
                            void *e, bool f, bool g);
extern int FindOpenWazoo(int win);
extern void OpenTasksWinBehind(void *win);

/* TOC / message operations */
extern void SetState(MacmbxTOC *toc, int sum, int state);
extern int macmbx_toc_save(MacmbxTOC *toc);
extern void DeleteMessage(MacmbxTOC *toc, int sum, bool nuke);

extern int MoveMessageLo(MacmbxTOC *tocH, int sumNum, char * dest, bool copy,
                         bool queue, bool open);
extern void TOCSetDirty(MacmbxTOC *toc, bool dirty);
extern void UpdateNumStat(int type, int val);

/* IMAP mailbox functions — declared in imapdownload.h */
extern MailboxNodeHandle LocateInboxForPers(PersHandle pers);
extern MacmbxTOC *TOCBySpec(char *spec);

/* Debug */
void Dprintf(const char *fmt, ...);
extern void CheckNagging(int userState);

/* Constants not in any ported header yet */
#define TASKS_WIN 100
#define BUG15 0
#define TRANS_IN_TABL 1003
#define charCodeMask 0x000000FF
#define mouseDown 1
#define kAlertStdAlertOKButton 1
#define kAlertStdAlertCancelButton 2
#define kStatSentMail 1
#define kStatForwardMsg 1
#define kStatReplyMsg 2
#define kStatRedirectMsg 3
#define CHECK_EXPIRE
#define CHECK_DEMO
#define RunType 0
#define EMSFIDLE_PRE_SEND 0x0008L

/* Helper app notification events (from original appleevent.h) */
#define eMailArrive 1L
#define eMailSent 2L
#define eWillConnect 3L
#define eHasConnected 4L


/* Module globals */
bool UUPCIn = false;
bool UUPCOut = false;
GArray *OutgoingMIDList = NULL;
bool OutgoingMIDListDirty = false;

/* No-op stubs for features not applicable in GTK port */
bool IsAdwareMode(void) { return false; }
void NotifyHelpers(int a, int b, void *c) {}
bool MonitorGrow(bool a) { return false; }
bool ShouldSMTPAuth(void) { return false; }
void FiltersDecRef(void) {}
void ETLIdle(long flags) {}
/* ResyncCurrentIMAPMailbox is in imapdownload.c */
static void AdCheckingMail(void) {}
static void PhKill(void) {}
static int CountResources(int type) { return 0; }
static int UUPCSendMessage(MacmbxTOC *toc, int sum, CSpecHandle list) { return 0; }
static void RegisterSuccess(int val) {}
short ETLSendMessage(TransStream stream, MessHandle messH, bool chatter,
                     bool sendDataCmd) { return 0; }
static void StartAuthenticatedSMTP(TransStream stream, char *server,
                                   long port) {}
/* Type2Select removed — dead Mac code */
static long GetDblTime(void) { return 0; }
static void ResetAlertStage(void) {}
static void TaskProgressRefresh(void) {}
static void FlushTOCs(bool a, bool b) {}
static bool SelectXferMailPers(bool check, bool send, bool manual) {
  return false;
}

/* Real implementations */
bool IsQueued(MacmbxTOC *toc, int sum) {
  if (!toc || sum < 0 || sum >= toc->count)
    return false;
  int s = toc->msgs[sum].state;
  return (s == QUEUED || s == TIMED);
}

long GetSMTPPort(void) {
  bool submission = prefs_get_bool(PREFS_GROUP_SENDING_MAIL, "use_submission_port", FALSE);
  long defaultPort = submission ? 587 : 25;
  long p = prefs_get_int(PREFS_GROUP_SENDING_MAIL, "smtp_port", defaultPort);
  return p ? p : defaultPort;
}

int GetSMTPInfoLo(char *server, long *port) {
  /* Try per-personality account first */
  PrefsAccount acct;
  extern bool GetCurPersAccount(PrefsAccount *acct);
  if (GetCurPersAccount(&acct) && acct.smtp_server[0]) {
    if (server) { strncpy(server, acct.smtp_server, 255); server[255] = '\0'; }
    return 0;
  }

  /* Fallback to global */
  gchar *smtp = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "smtp_server", "");
  if (server) {
    strncpy(server, smtp, 255);
    server[255] = '\0';
  }
  g_free(smtp);
  if (!server || !server[0])
    return 1;
  return 0;
}

int GetPOPInfoLo(char *user, char *host, long *port) {
  GetPOPInfo(user, host);
  if (!host || !host[0])
    return 1;
  if (port && *port == 0)
    *port = 110;
  return 0;
}

bool IsIMAPPers(PersHandle pers) {
  return (bool)prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_imap", FALSE);
}

/* Forward declarations */
void NewMailSound(void);
short CheckForMail(TransStream stream, short *gotSome, XferFlags *flags);
short SendTheQueue(TransStream stream, XferFlags flags);
void ResetCheckTime(bool force);
int SpecialXfer(struct XferFlags *flags);
int POPHostLimit(void);
short XferMailLo(bool check, bool send, bool manual, XferFlags flags,
                 long *totalGot, int *dialErrPtr);
bool NeedPassword(bool check, bool send);
void ResetPersCheckTime(bool force);
bool OKToThread(bool check, bool send, bool manual, bool scripted);
long FindTotalQueuedSize(MacmbxTOC * tocH, long gmtSecs);
bool AddSigIntro(GtkWidget *pte, void **text);
bool RemoveSigIntro(GtkWidget *pte, void **text);
bool SpecialXferFilter(void *dgPtr, void *event, short *item);
PersHandle SMTPRelayPers(void);
int RememberMID(uint32_t midHash);
long GlobalOpenUnreadCount();
long GlobalInUnreadCount();

/************************************************************************
 * OKToThread - returns false if threading should be used because of
 *	transport limitations
 *
 *	(a) Do not thread if the connection method is the CTB.  The CTB
 *		supports one and only one connection at a time.
 *
 *	(b) Do not thread if any personality is set to uucpOut and is allowed to
 *		send mail.
 *
 *	(c) Do not thread if any personality is set to uucpIn and is allowed to
 *		check mail manually or automatically.
 *
 ************************************************************************/
bool OKToThread(bool check, bool send, bool manual, bool scripted) {
  bool threadOK = true;
//	char pass;

  return (threadOK);
}

/************************************************************************
 * XferMail - do the right thing
 * 						if thread is true, try to do it
 *
 *  To support separate send and check threads:
 *
 *  - don't allow more than one check thread at a time
 *  - don't allow more than one send thread at a time
 *  - if user tries to send while a send thread is running, queue the request
 *up (via SendImmediately flag) and let the check go
 *  - before sending, set the SendQueue. if it's zero, don't send and reset
 *SendImmediately flag
 *
 ************************************************************************/

/* ── Get the global macmbx_mailer instance ── */
static MacmbxMailer *get_mailer(void) {
  extern MacmbxMailer *idle_scheduler_get_mailer(void);
  /* idle_scheduler owns the mailer instance; we borrow it */
  return idle_scheduler_get_mailer();
}

short XferMail(bool check, bool send, bool manual, bool scripted, bool thread,
               short modifiers) {
  (void)scripted; (void)thread; (void)modifiers;
  int err = 0;

  /* Don't allow concurrent check/send */
  if (CheckThreadRunning && check) return 0;
  if (SendThreadRunning && send) return 0;

  MacmbxMailer *mailer = get_mailer();
  g_print("XferMail: check=%d send=%d mailer=%p\n", check, send, (void*)mailer);
  if (!mailer) {
    g_warning("XferMail: no mailer instance — macmbx_mailer not initialized");
    return -1;
  }

  /* Check mail — macmbx_mailer handles the entire pipeline:
   * connect → download → deliver to In → filter → junk score */
  if (check) {
    g_print("XferMail: calling macmbx_mailer_check...\n");
    err = macmbx_mailer_check(mailer);
    g_print("XferMail: macmbx_mailer_check returned %d\n", err);
    if (err >= 0) {
      NeedToNotify = true;
      gNewMessages += err;
      err = 0;
    }
  }

  /* Send queued — async via idle scheduler background thread */
  if (send) {
    extern void idle_scheduler_request_send(void);
    idle_scheduler_request_send();
  }

  return err;
}

/* Legacy pipeline (XferMailSetup/Run/Lo, SendTheQueue, CheckForMail) removed.
 * All mail transfer goes through macmbx_mailer now. */

/**********************************************************************
 * ResetCheckTime - reset the mail check interval
 **********************************************************************/
void ResetCheckTime(bool force) {
  PushPers(CurPers);
  for (CurPers = PersList; CurPers; CurPers = CurPers->next)
    ResetPersCheckTime(force);
  PopPers();
}

/************************************************************************
 * ResetPersCheckTime - reset the mail check interval for a personality
 ************************************************************************/
void ResetPersCheckTime(bool force) {
  long interval =
      PrefIsSet(PREF_AUTO_CHECK) ? TICKS2MINS * GetPrefLong(PREF_INTERVAL) : 0;

  if (interval) {
    /*
     * setup the initial check interval
     */
    if (!CurPers->checkTicks)
      CurPers->checkTicks = TickCount();

    /*
     * manage the mail check interval
     */
    if (force && CurPers->doMeNow || CurPers->checked) {
      if (CurPers->checkTicks + interval < TickCount() + 45 * 60) {
        CurPers->checkTicks +=
            interval * ((TickCount() - CurPers->checkTicks + 1) / interval);
        if (CurPers->checkTicks + interval < TickCount() + 45 * 60)
          CurPers->checkTicks += interval;
      }
    }
  } else
    CurPers->checkTicks = 0;
}

/************************************************************************
 * NotifyNewMail - notify the user that new mail has arrived, via the
 * notification manager.
 ************************************************************************/

// DO NOT FIX THIS ROUTINE
// IF IT MUST BE FIXED, THROW IT AWAY AND REWRITE IT & ALL RELATED TO IT
// SD 8/20/02

void NotifyNewMail(short gotSome, bool noXfer, MacmbxTOC * tocH,
                   FilterPB *fpbDelivery) {
  NotifyNewMailLo(gotSome, noXfer, tocH, fpbDelivery, true);
}

void NotifyNewMailLo(short gotSome, bool noXfer, MacmbxTOC * tocH,
                     FilterPB *fpbDelivery, bool OpenIn) {
  (void)noXfer; (void)fpbDelivery;

  /* Filtering and junk scoring are handled by macmbx_mailer internally.
   * This function now only handles UI notification. */

  if (!gotSome && !gNewMessages) {
    CloseProgress();
    return;
  }

  gNewMessages += gotSome;

  /* Refresh open mailbox tabs */
  extern void eudora_refresh_open_mailboxes(void);
  eudora_refresh_open_mailboxes();

  /* Open In mailbox if configured */
  if (gotSome && OpenIn && !PrefIsSet(PREF_NO_OPEN_IN))
    eudora_open_mailbox_by_name("In");

  /* Reset counter after notification */
  if (!CheckThreadRunning && !IMAPCheckThreadRunning)
    gNewMessages = 0;
}
GtkWidget * OpenBehindMePlease(void) {
  MyWindowPtr win;
  GtkWidget *winWP, *frontWP, *returnWinWP = NULL;

  frontWP = NULL;

  switch (GetPrefLong(PREF_OPEN_WHERE)) {
  case 0: // last comp on top
    for (winWP = frontWP; winWP; winWP = GetNextWindow(winWP))
      if (IsWindowVisible(winWP))
        if (GetWindowKind(winWP) == COMP_WIN)
          returnWinWP = winWP;
        else
          break;
    break;

  case 1: // on top
    returnWinWP = NULL;
    break;

  case 2: // behind In
    for (winWP = frontWP; winWP; winWP = GetNextWindow(winWP))
      if (IsWindowVisible(winWP))
        if (GetWindowKind(winWP) == MBOX_WIN)
          if (((MacmbxTOC *)NULL)->which == IN) {
            returnWinWP = winWP;
            break;
          }
    if (returnWinWP)
      break;
    // if no visible In box, fall through to behind current mailbox

  case 3: // behind current mailbox
    for (winWP = frontWP; winWP; winWP = GetNextWindow(winWP))
      if (IsWindowVisible(winWP))
        break;
    win = GetWindowMyWindowPtr(winWP);
    if (GetWindowKind(winWP) == MBOX_WIN || GetWindowKind(winWP) == CBOX_WIN) {
      returnWinWP = winWP;
      break;
    } else if (GetWindowKind(winWP) == MESS_WIN ||
               GetWindowKind(winWP) == COMP_WIN) {
      returnWinWP = winWP; // just in case the toc's not visible
      win = Win2MessH(win)->tocH->win;
      winWP = GetMyWindowWindowPtr(win);
      if (IsWindowVisible(winWP)) {
        returnWinWP = winWP;
        break;
      }
    }
    break;

  case 4: // in the back
    for (winWP = frontWP; winWP; winWP = GetNextWindow(winWP))
      if (IsWindowVisible(winWP))
        returnWinWP = winWP;
    break;

  case 5: // behind frontmost window
    for (winWP = frontWP; winWP; winWP = GetNextWindow(winWP))
      if (IsWindowVisible(winWP)) {
        returnWinWP = winWP;
        break;
      }
    break;
  }

  // If there's a modal window open, make sure the next window isn't opened on
  // top of it.
  if ((returnWinWP == NULL) && ModalWindow)
    returnWinWP = ModalWindow;

  win = GetWindowMyWindowPtr(frontWP);
  if (win /* && win->isNag */)
    returnWinWP = frontWP;

  return (returnWinWP);
}

/************************************************************************
 * ShowBoxSel - show the mailbox with a selection
 ************************************************************************/
void ShowBoxAt(MacmbxTOC * tocH, short selectMe, GtkWidget * behindWin) {
  GtkWidget * tocWinWP = GetMyWindowWindowPtr(tocH->win);

  if (selectMe >= 0)
    SelectBoxRange(tocH, selectMe, selectMe, false, -1, -1);
  /* RedoTOC removed — UI refreshes on demand */
  ScrollIt(tocH->win, 0, SortedDescending(tocH) ? REAL_BIG : -REAL_BIG);
  if (IsWindowVisible(tocWinWP)) {
    if (behindWin) {
      if (behindWin != tocWinWP)
        SendBehind(tocWinWP, behindWin);
    } else
      gtk_window_present(GTK_WINDOW(tocWinWP));
  } else
    ShowMyWindowBehind(tocWinWP, behindWin);
}

/************************************************************************
 * FumLub - find the FumLub
 ************************************************************************/
short FumLub(MacmbxTOC * tocH) {
  short i;
  if (!tocH)
    return (-1);

  /* RedoTOC removed — UI refreshes on demand */

  if (SortedDescending(tocH))
    return (0);

  for (i = tocH->count - 1; i >= 0; i--)
    if (tocH->msgs[i].state != UNREAD) {
      i++;
      break;
    }
  return (i < tocH->count ? MAX(i, 0) : tocH->count - 1);
}

/************************************************************************
 * NewMailSound - play the sound for new mail
 *
 * Original played a Mac 'snd ' resource via PlayNamedSound. GTK port
 * uses GLib's notification system or a simple system bell.
 ************************************************************************/
void NewMailSound(void) {
  /* TODO: implement via g_notification or GStreamer if desired */
  gdk_display_beep(gdk_display_get_default());
}

/************************************************************************
 * GrabSignature - read the signature file into the global eSignature.
 *
 * Original opened a text window and extracted PTE text + built
 * enriched/HTML variants via Accumulators. This port reads the file
 * directly into a malloc'd buffer (eSignature). Enriched/HTML
 * signature variants are not built yet (they require the styled text
 * pipeline).
 ************************************************************************/
void GrabSignature(uint32_t fid) {
  char *sigText = NULL;
  gsize sigLen = 0;

  /* Free previous signatures */
  free(eSignature);
  eSignature = NULL;
  free(RichSignature);
  RichSignature = NULL;
  free(HTMLSignature);
  HTMLSignature = NULL;
  SigStyled = false;

  if (fid == SIG_NONE)
    return;

  char *path = SigPath(fid);
  if (!path) {
    FileSystemError(CANT_READ_SIG, "", ENOENT);
    return;
  }

  if (!g_file_get_contents(path, &sigText, &sigLen, NULL)) {
    FileSystemError(CANT_READ_SIG, path, EIO);
    g_free(path);
    return;
  }
  g_free(path);

  /* Store as eSignature (plain text) — allocated with g_malloc via GLib,
     but eSignature is typed as unsigned char * (void**) in Globals.h for legacy
     compatibility. For now, store the raw text pointer. Callers that
     use eSignature will need to treat it as a plain char* buffer. */
  eSignature = sigText;
}

/************************************************************************
 * AddSigIntro - add the sig introducer to a petehandle or a text handle
 * The sig intro is typically "-- \r" — prepended before the signature.
 ************************************************************************/
bool AddSigIntro(GtkWidget *pte, void **text) {
  char sigIntro[32];
  bool didIt = false;
  long len;

  if (!*GetRString(sigIntro, SIG_INTRO))
    return false;

  long introLen = strlen(sigIntro);
  if (text && *text) {
    len = strlen((char *)*text);
    if (len > 0) {
      char *ptr = (char *)*text;
      if (len < introLen || memcmp(ptr, sigIntro, introLen) != 0) {
        { void *_r = realloc(*text, len + introLen); if (_r) *text = _r; }
        {
          ptr = (char *)*text;
          memmove(ptr + introLen, ptr, len);
          memmove(ptr, sigIntro, introLen);
          didIt = true;
        }
      }
    }
  }

  /* void *the pte (GtkTextView) */
  if (pte) {
    long textLen = geditctrl_get_length(pte);
    if (textLen > 0) {
      gchar *intro = g_strndup((const char *)(sigIntro + 1), *sigIntro);
      geditctrl_insert_text(pte, 0, intro, -1);
      g_free(intro);
      didIt = true;
    }
  }

  return didIt;
}

/************************************************************************
 * RemoveSigIntro - remove the sig introducer from text or pte
 ************************************************************************/
bool RemoveSigIntro(GtkWidget *pte, void **text) {
  char sigIntro[32];
  long len;
  bool didIt = false;

  if (!*GetRString(sigIntro, SIG_INTRO))
    return false;

  long introLen = strlen(sigIntro);
  if (text && *text) {
    len = strlen((char *)*text);
    if (len >= introLen) {
      char *ptr = (char *)*text;
      if (memcmp(ptr, sigIntro, MIN(introLen, 4)) == 0) {
        memmove(ptr, ptr + introLen, len - introLen);
        { void *_r = realloc(text, len - introLen); if (_r) text = _r; }
        didIt = true;
      }
    }
  }

  /* void *the pte */
  if (pte) {
    gchar *h = geditctrl_get_text(pte);
    if (h) {
      len = strlen((char *)h);
      if (len >= *sigIntro) {
        char *ptr = (char *)h;
        if (memcmp(ptr, sigIntro + 1, MIN(*sigIntro, 4)) == 0) {
          geditctrl_delete_range(pte, 0, *sigIntro);
          didIt = true;
        }
      }
    }
  }

  return didIt;
}


/* Dead legacy send/IMAP code removed — macmbx_mailer handles everything. */
