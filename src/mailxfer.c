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
#include "compact.h"
#include "ends.h"
#include "filtrun.h"
#include "gtk_dialogs.h"
#include "gtk_nag.h"
#include "gtk_prefs.h"
#include "imapmailboxes.h"
#include "junk.h"
#include "log.h"
#include "mime.h"
#include "myssl.h"
#include "MyRes.h"
#include "portable-compat.h"
#include "prefdefs.h"
#include "progress.h"
#include "sendmail.h"
#include "signaturewin.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "threading.h"
#include "trans.h"
#include "toc.h"
#include "auditdefs.h"
#include "fileutil.h"
#include "Globals.h"
#include "peteglue.h"
#include "util.h"
#include <assert.h>
#include <stdio.h>

#define FILE_NUM 52

/* Globals come from Globals.h (included above) */
extern long CountFlaggedMessages(TOCType *toc);

/* IMAP functions — declared in imapdownload.h */
extern int DoIMAPFilterProgress(void);


/* Mail transfer functions */
extern int GetUUPCMail(bool a, short *b);
extern int GetMyMail(TransStream stream, bool a, short *b, XferFlags *flags);
extern int NewTransStream(TransStream *stream);
extern long ReportStreamAudit(TransStream stream);
extern void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what);
extern int StartSMTP(TransStream stream, unsigned char *server, long port);
extern int StartPOP(TransStream stream, unsigned char *server, long port);
extern int EndPOP(TransStream stream);
extern void POPIntroductions(TransStream stream, unsigned char *s, void *p);
extern int POPrror(void);
extern int MySendMessage(TransStream stream, TOCType *toc, int sum,
                         CSpecHandle list);
extern short EffectiveTID(short id);
extern short TransOutTablID(void);
extern unsigned char *GetFlatten(void);

/* Personality functions */
extern void GetPOPInfo(void *a, void *b);
extern PStr GetPOPPref(PStr dest);
extern void PushPers(PersHandle pers);
extern void PopPers(void);

/* UI / window functions */
extern void *GetWindowPrivateData(WindowPtr w);
extern void SelectBoxRange(TOCType *toc, short a, short b, bool c, short d,
                           short e);
extern void ScrollIt(WindowPtr w, short a, long b);
extern bool SortedDescending(TOCType *toc);
extern void ShowMyWindowBehind(WindowPtr a, WindowPtr b);
extern void eudora_open_mailbox_by_name(const char *name);
extern MyWindowPtr FindText(FSSpecPtr spec);
extern void MySelectWindow(WindowPtr w);
extern MyWindowPtr OpenText(FSSpecPtr spec, void *a, void *b, void *c, bool d,
                            void *e, bool f, bool g);
extern int FindOpenWazoo(int win);
extern void OpenTasksWinBehind(void *win);

/* TOC / message operations */
extern void SetState(TOCType *toc, int sum, int state);
extern int WriteTOC(TOCType *toc);
extern void DeleteMessage(TOCType *toc, int sum, bool nuke);
extern void RedoTOC(TOCType *toc);
extern int MoveMessageLo(TOCType *tocH, int sumNum, FSSpecPtr dest, bool copy,
                         bool queue, bool open);
extern void TOCSetDirty(TOCType *toc, bool dirty);
extern void UpdateNumStat(int type, int val);

/* IMAP mailbox functions — declared in imapdownload.h */
extern MailboxNodeHandle LocateInboxForPers(PersHandle pers);
extern TOCType *TOCBySpec(FSSpec *spec);

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
static int UUPCSendMessage(TOCType *toc, int sum, CSpecHandle list) { return 0; }
static void RegisterSuccess(int val) {}
short ETLSendMessage(TransStream stream, MessHandle messH, bool chatter,
                     bool sendDataCmd) { return 0; }
static void StartAuthenticatedSMTP(TransStream stream, unsigned char *server,
                                   long port) {}
static void Type2Select(struct EventRecord *event) {}
static long GetDblTime(void) { return 0; }
static void ResetAlertStage(void) {}
static void TaskProgressRefresh(void) {}
static void FlushTOCs(bool a, bool b) {}
static bool SelectXferMailPers(bool check, bool send, bool manual) {
  return false;
}

/* Real implementations */
bool IsQueued(TOCType *toc, int sum) {
  if (!toc || sum < 0 || sum >= toc->count)
    return false;
  int s = toc->sums[sum].state;
  return (s == QUEUED || s == TIMED);
}

long GetSMTPPort(void) {
  bool submission = prefs_get_bool(PREFS_GROUP_SENDING_MAIL, "use_submission_port", FALSE);
  long defaultPort = submission ? 587 : 25;
  long p = prefs_get_int(PREFS_GROUP_SENDING_MAIL, "smtp_port", defaultPort);
  return p ? p : defaultPort;
}

int GetSMTPInfoLo(unsigned char *server, long *port) {
  gchar *smtp = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "smtp_server", "");
  if (server) {
    strncpy((char *)server, smtp, 255);
    ((char *)server)[255] = '\0';
  }
  g_free(smtp);
  if (!server || !server[0])
    return 1;
  return 0;
}

int GetPOPInfoLo(unsigned char *user, unsigned char *host, long *port) {
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
long FindTotalQueuedSize(TOCType * tocH, long gmtSecs);
bool AddSigIntro(GtkWidget *pte, void **text);
bool RemoveSigIntro(GtkWidget *pte, void **text);
bool SpecialXferFilter(DialogPtr dgPtr, EventRecord *event, short *item);
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

short XferMail(bool check, bool send, bool manual, bool scripted, bool thread,
               short modifiers) {
  XferFlags flags;
  int err = 0;
  PersHandle pers;
  bool persSend = false;

  // archive the junk mailbox?  Only if we're desperate
  if (JunkPrefBoxArchive() && JunkTrimOK())
#ifndef JUNK
#define JUNK 4
#endif
    ArchiveJunk(GetJunkTOC());

  // Cmd-Shift-M resyncs frontmost IMAP mailbox.
  if (!PrefIsSet(PREF_ALTERNATE_CHECK_MAIL_CMD))
    if (!(modifiers & optionKey) && (modifiers & shiftKey) &&
        ResyncCurrentIMAPMailbox())
      return (0);

  // Don't allow more than one checkmail or sendmail thread
  if ((CheckThreadRunning && check) ||
      (PrefIsSet(PREF_POP_SEND) && SendThreadRunning))
    return 0;

  // if "send when check" pref set, spawn the check and send later
  if ((SendThreadRunning && send) ||
      (PrefIsSet(PREF_POP_SEND) && CheckThreadRunning)) {
    SendImmediately = true; // = SendImmediately || send;
    if (check)
      send = false;
    else
      return 0;
  }

  // clear the subfolder cache; as good a place as any
  SubFolderSpec(0, NULL);

  if ((err = XferMailSetup(&check, &send, manual, scripted, &flags, modifiers))) {
    g_print("XferMail: XferMailSetup returned err=%d\n", err);
    return err;
  }
  g_print("XferMail: after setup check=%d send=%d SendQueue=%d\n", check, send, SendQueue);

  // no need in spawning send thread if none of the queued messages belong to
  // a personality marked for sending whenever sends are done
  for (pers = PersList; pers; pers = pers->next) {
    g_print("XferMail: pers=%p sendQueue=%ld sendMeNow=%d\n", (void*)pers, pers->sendQueue, pers->sendMeNow);
    if (pers->sendQueue && pers->sendMeNow)
      persSend = true;
  }
  if (!SendQueue || !persSend) {
    g_print("XferMail: send disabled: SendQueue=%d persSend=%d\n", SendQueue, persSend);
    send = false;
    SendImmediately = false;
  }
  if (check) {
    GetDateTime(&LastCheckTime);
    TaskProgressRefresh();
  }
  if (!(check || send))
    return 0;

  FlushTOCs(true, true); /* flush unnecessary TOC's */
  RememberOpenWindows(); /* for good measure */

  ActiveTicks = 0; // we're pretending; manual check mail should not wait for
                   // idle time to filter

  // fetch any mailbox lists we need for this mailcheck
  for (pers = PersList; pers; pers = pers->next) {
    PersHandle oldPers = CurPers;

    if (pers->checkMeNow) {
      CurPers = pers;
      if (PrefIsSet(PREF_IS_IMAP)) {
        if (!MailboxTreeGood(CurPers)) {
          if (CreateLocalCache() == 0)
            EnsureSpecialMailboxes(CurPers);
        }
      }
      CurPers = oldPers;
    }
  }

  g_print("XferMail: about to run/thread check=%d send=%d threading_off=%d threads_avail=%d thread=%d\n",
          check, send, PrefIsSet(PREF_THREADING_OFF), ThreadsAvailable(), thread);
  fflush(stdout);
  if (PrefIsSet(PREF_THREADING_OFF) || !OKToThread(check, send, manual, scripted) ||
      !ThreadsAvailable() || !thread)
    err = XferMailRun(check, send, manual, scripted, flags, NULL);
  else
    err = SetupXferMailThread(check, send, manual, scripted, flags, NULL);
  fflush(stdout);
  if (send)
    SetSendQueue();        // redo sendqueue if need be
  SendImmediately = false; // clear this for next time around

  if (check && IsAdwareMode())
    AdCheckingMail();

  fflush(stdout);
  return (err);
}

/************************************************************************
 * XferMailSetup -
 *	12-8-98 JDB
 *	The check and send parameters may be modified by the SpecialXfer
 *	dialog, in which case the caller should know not to check/send.
 ************************************************************************/
short XferMailSetup(bool *check, bool *send, bool manual, bool scripted,
                    XferFlags *xFlags, short modifiers) {
  PersHandle oldCur = CurPers;
  XferFlags flags;
  bool special = 0 != (modifiers & optionKey);
  char pass[256];
  Zero(pass);
  uint32_t ticks, ivalTicks;
  PersHandle pers;

  CHECK_EXPIRE;

  CHECK_DEMO;

  if (*send) // 5/15/97 ccw -- fixes "Send queued messages" deactive menu and
             // timed mesg bugs
  {
    SetSendQueue();
    ETLIdle(EMSFIDLE_PRE_SEND);
  }

  // PeteSetRudeColor();

  Zero(flags);
  flags.check = flags.servFetch = flags.servDel = *check;
  flags.send = *send;
  // flags.nuke = check && !PrefIsSet(PREF_LMOS);
  flags.isAuto = *check && !(manual || scripted);

  CheckNow = false; // clear flag

  if (!SelectXferMailPers(
          *check, *send,
          manual)) //	Set if checking/sending from Personalities window
  {
    ticks = TickCount() + TICKS2MINS * GetRLong(PERS_CHECK_SLOP);
    for (pers = PersList; pers; pers = pers->next) {
      CurPers = pers;
      CurPers->doMeNow = CurPers->checkMeNow = CurPers->sendMeNow =
          CurPers->checked = 0;
      if (*check && *GetPOPPref(pass)) {
        ivalTicks = CurPers->autoCheck ? CurPers->ivalTicks : 0;
        if (ivalTicks && (!CurPers->checkTicks ||
                          CurPers->checkTicks + ivalTicks < ticks))
          CurPers->doMeNow = CurPers->checkMeNow = true;
        else if ((manual || scripted) && !PrefIsSet(PREF_JUST_SAY_NO))
          CurPers->doMeNow = CurPers->checkMeNow = true;
      }

      g_print("XferMailSetup: pers=%p sendQueue=%ld send=%d noSend=%d checkMeNow=%d sendMeNow=%d doMeNow=%d popPref='%s'\n",
              (void*)CurPers, CurPers->sendQueue, *send, PrefIsSet(PREF_PERS_NO_SEND),
              CurPers->checkMeNow, CurPers->sendMeNow, CurPers->doMeNow, (char*)pass);
      if (*send && CurPers->sendQueue && !PrefIsSet(PREF_PERS_NO_SEND))
        CurPers->doMeNow = CurPers->sendMeNow = true;
      assert(CurPers == pers);
    }
  }

  if (manual && !scripted && special && SpecialXfer(&flags)) {
    CurPers = oldCur;
    return (userCancelled);
  }

  *send = flags.send;
  *check = flags.check || flags.servFetch || flags.servDel || flags.nuke ||
           flags.nukeHard || flags.stub;

  g_print("XferMailSetup: final send=%d check=%d\n", *send, *check);
  if (!*send && !*check) {
    g_print("XferMailSetup: both false, returning userCancelled\n");
    CurPers = oldCur;
    return (userCancelled);
  }

  // collect passwords
  for (pers = PersList; pers; pers = pers->next) {
    CurPers = pers;

    // Check for checking mail first
    if (NeedPassword(*check && CurPers->checkMeNow, false)) {
      if (PersFillPw(CurPers, 0) != 0) {
        ResetCheckTime(true);
        CurPers = oldCur;
        return (1);
      }
    }
    // Double check that all the special things IMAP needs are in place
    if (*check && CurPers->checkMeNow && PrefIsSet(PREF_IS_IMAP) &&
        !EnsureSpecialMailboxes(CurPers)) {
      ResetCheckTime(true);
      CurPers = oldCur;
      return (1);
    }
    // Now check for checking mail
    if (*send && CurPers->sendMeNow) {
      PersHandle relayPers = SMTPRelayPers();
      bool oldDoMeNow;

      // if we're using a relay personality, switch to it
      if (relayPers) {
        // gotta force doMeNow or we won't ask for a password
        oldDoMeNow = relayPers->doMeNow;
        relayPers->doMeNow = true;
        PushPers(relayPers);
      }

      if (NeedPassword(false, true)) {
        if (PersFillPw(CurPers, 0) != 0) {
          if (relayPers)
            PopPers();
          CurPers = oldCur;
          return (1);
        }
      }

      // put back the personality, and reset the doMeNow flag
      if (relayPers) {
        PopPers();
        relayPers->doMeNow = oldDoMeNow;
      }
    }

    assert(CurPers == pers);
  }

  memmove(xFlags, &flags, sizeof(flags));

  CurPers = oldCur;
  return (0);
}

/************************************************************************
 * XferMailReal - transfer mail; check or send, for each personality
 ************************************************************************/
short XferMailRun(bool check, bool send, bool manual, bool scripted, XferFlags flags,
                  IMAPTransferPtr imapInfo) {
  PersHandle oldCur = CurPers;
  PersHandle pers;
  int err, anyErr;
  long gotSome;
  short dialErr;
  bool popChecked = false;

  assert(!InAThread() || CurThreadGlobals != &ThreadGlobals);

  anyErr = err = 0;

  if (InAThread()) {
    if (PrefIsSet(PREF_TASK_PROGRESS_AUTO) && !TaskProgressWindow &&
        !FindOpenWazoo(TASKS_WIN)) {
      if (imapInfo && imapInfo->command != UndefinedTask &&
          !PrefIsSet(PREF_IMAP_TP_BRING_TO_FRONT))
        OpenTasksWinBehind(NULL);
      else
        OpenTasksWinBehind(manual ? BehindModal : NULL);
    }
  }
  NoXfer = flags.stub;

  gotSome = 0;
  dialErr = 0;
  gPPPConnectFailed = false;

  // is this an IMAP operation of some kind?
  if (imapInfo && imapInfo->command != UndefinedTask) {
    SetCurrentTaskKind(imapInfo->command);

    switch (imapInfo->command) {
    // if a mailbox was specified, then we're checking mail for an IMAP
    // personality.
    case IMAPResyncTask: {
      err =
          DoFetchNewMessages(&(imapInfo->targetSpec), true, false) ? 0 : 1;
      check = true;
      gotSome++;
      break;
    }
    // if a tocH and uid is specified, then we should fetch a message
    case IMAPFetchingTask: {
      err = DoDownloadMessages(imapInfo->destToc, imapInfo->uids,
                               imapInfo->attachmentsToo)
                ? 0
                : 1; // some error is needed here
      break;
    }
    // if a delToc and uid is specified, delete the sepcified message(s)
    case IMAPDeleteTask:
    case IMAPUndeleteTask: {
      err = DoDeleteMessage(imapInfo->delToc, imapInfo->uids, imapInfo->nuke,
                            imapInfo->expunge,
                            (imapInfo->command == IMAPUndeleteTask))
                ? 0
                : 1;
      break;
    }
    // if a source, destination, and uid list is specified, then do a transfer
    case IMAPTransferTask: {
      err = DoTransferMessages(imapInfo->sourceToc, imapInfo->destToc,
                               imapInfo->uids, imapInfo->copy);
      break;
    }
    case IMAPExpungeTask: {
      err = DoExpungeMailbox(imapInfo->delToc) ? 0 : 1;
      break;
    }
    case IMAPAttachmentFetch: {
      err =
          DoDownloadIMAPAttachments((FSSpecHandle)imapInfo->attachments,
                                   (MailboxNodeHandle)imapInfo->boxesToSearch)
              ? 0
              : 1;
      break;
    }
    case IMAPSearchTask: {
      err = DoIMAPServerSearch(imapInfo->destToc, (BoxCountHandle)imapInfo->boxesToSearch,
                               (void *)imapInfo->toSearch, (IMAPSCHandle)imapInfo->searchC,
                               imapInfo->matchAll, imapInfo->firstUID)
                ? 0
                : 1;
      break;
    }
    case IMAPMultResyncTask:
    case IMAPMultExpungeTask: {
      err = DoIMAPProcessMailboxes(imapInfo->toResync, imapInfo->command)
                ? 0
                : 1;
      break;
    }
    case IMAPUploadTask: {
      err = DoTransferMessagesToServer(imapInfo->destToc, imapInfo->appendData,
                                       imapInfo->copy, false);
      break;
    }
    case IMAPPollingTask: {
      IMAPPollMailboxes((MailboxNodeHandle)imapInfo->boxesToSearch);
      break;
    }
    case IMAPFilterTask: {
      err = DoIMAPFilterProgress();
      break;
    }
    default: {
      // err = something went horribly wrong to get here
      break;
    }
    }
  }
  // otherwise, it's a mailcheck, sweet and boring.
  else {
    IMAPCheckThreadRunning = gNewMessages =
        0;               // forget about all past IMAP mail checks
    NoNewMailMe = false; // also forget about any pending NoNewMail alerts we
                         // may have around.

    gStayConnected = true; // Tell the dial code to keep the connection up
                           // until further notice.
    g_print("XferMailRun: PersList=%p check=%d send=%d inThread=%d\n",
            (void*)PersList, check, send, InAThread());
    for (pers = PersList; pers && !gPPPConnectFailed && !CommandPeriod;
         pers = pers->next) {
      CurPers = pers;
      g_print("XferMailRun: pers=%p checkMeNow=%d sendMeNow=%d pw='%.4s...'\n",
              (void*)pers, pers->checkMeNow, pers->sendMeNow, (char*)pers->password);

      // only display the new mail alert if a pop personality is being checked
      if (CurPers->checkMeNow && !PrefIsSet(PREF_IS_IMAP))
        popChecked = true;

      err = XferMailLo(check && CurPers->checkMeNow,
                       send && CurPers->sendMeNow, manual || scripted, flags,
                       &gotSome, (int *)&dialErr);
      if (!anyErr)
        anyErr = err;
      assert(CurPers == pers);
    }
    CurPers = oldCur;
    gStayConnected = false; // the dialup connection can be torn down when no
                            // longer needed.
  }

  if (InAThread()) {
    // silly no mail alert; here is as good a place as any
    if (!dialErr && flags.check && manual)
      // do NoNewMailMe only if a pop personality was checked, and there
      // aren't any other check threads going.
      if (popChecked && !IMAPCheckThreadRunning && !gNewMessages)
        NoNewMailMe = gotSome == 0;

    /* NeedToFilterIn was already incremented by POP thread (pop.c).
       The idle scheduler will pick up delivery TOCs, filter, and notify. */
    if (!dialErr && flags.check && (manual || gotSome > 0))
      NeedToNotify = true;
  } else
  {
    /*
     * notify the user if new mail arrived (or not, in some cases)
     */
    if (!dialErr && flags.check && (manual || gotSome > 0)) {
      NotifyNewMail(gotSome, flags.stub, GetRealInTOC(), NULL);
    }
    NotifyHelpers(0, eHasConnected, NULL);
  }

  if (check)
    ResetCheckTime(true); // Reset intervals for *all* personalities that were
                          // supposed to be checked. JDB 12-16-98
  if (LastAttPath) { free(LastAttPath); LastAttPath = NULL; }

  assert(!InAThread() || CurThreadGlobals != &ThreadGlobals);

  return (err);
}

/**********************************************************************
 * XferMailLo - transfer mail for a given personality
 **********************************************************************/
short XferMailLo(bool check, bool send, bool manual, XferFlags flags,
                 long *totalGot, int *dialErrPtr) {
  short err = 0;
  short gotSome = 0;
  short anyErr = 0;
  char s[256];
  char popUser[256], popHost[256];
  TransStream mailStream = 0;
  bool willCheck = false, willSend = false;

  g_print("XferMailLo: check=%d send=%d pers=%p checkMeNow=%d sendMeNow=%d\n",
          check, send, (void*)CurPers, CurPers->checkMeNow, CurPers->sendMeNow);

  if (PrefIsSet(PREF_THREADING_OFF))
    PhKill();

  /*
   * clear the decks
   */
  if (send && !CurPers->sendQueue)
    send = false;
  g_print("XferMailLo: [1] after sendQueue check, send=%d\n", send);
  /*
   * Set which task we're about to do in case we need to report it if we're
   * low on memory
   */
  SetCurrentTaskKind((check && !(CurPers->popSecure && send)) ? CheckingTask
                                                                 : SendingTask);
  g_print("XferMailLo: [2] after SetCurrentTaskKind\n");
  FlushTOCs(true, true); /* flush unnecessary TOC's */
  g_print("XferMailLo: [3] after FlushTOCs\n");
  if (MonitorGrow(true) || CommandPeriod)
    goto done;
  g_print("XferMailLo: [4] after MonitorGrow\n");
  if (check)
    HesOK = true; // force re-fetch of hesiod info
  GetPOPInfo(popUser, popHost);
  g_print("XferMailLo: [5] after GetPOPInfo user='%s' host='%s'\n", popUser, popHost);
  HesOK = false;

  /*
   * figure out what we need to do our duty
   * We need the password if we're checking (or sending POPSecure)
   * We need to dial the phone if we're going to use the CTB for
   * sending or checking (or verifying the account under POPSecure)
   */
  if (!send && !check)
    goto done; /* nothing to do */

  g_print("XferMailLo: [6] before CountResources\n");
  if (CountResources(NOTIFY_TYPE))
    NotifyHelpers(0, eWillConnect, NULL);

  g_print("XferMailLo: [7] before POPHostLimit\n");
  if (check && (anyErr = POPHostLimit()))
    goto done;

  g_print("XferMailLo: [8] before NewTransStream\n");
  /*
   *	Set up our TransStream
   */
  if ((anyErr = NewTransStream(&mailStream)))
    goto done;

  g_print("XferMailLo: [9] before OpenProgress\n");
  /*
   * Doing network stuff; alerts should timeout
   */

  /*
   * Do we need to dial the phone?
   */
  OpenProgress();
  g_print("XferMailLo: [10] after OpenProgress\n");

  /*
   * Now, do the mail transfers
   */
  if (!err) {
    if (MonitorGrow(true))
      return (0);

    if (check)
      CurPers->checked = true; // don't retry instantly

    /*
     * check for mail, if need be
     */
    willCheck = !CommandPeriod && check && (!UseCTB || !err);
    g_print("XferMailLo: [11] willCheck=%d willSend=pending check=%d send=%d\n", willCheck, check, send);

    /*
     * Set task we're about to do in case we need to report it if we're low on
     * memory
     */
    if (willCheck)
      SetCurrentTaskKind(CheckingTask);
    if (MonitorGrow(true))
      return (0);
    if (willCheck) {
      if (IsIMAPPers(CurPers)) {
        // checking mail on IMAP server
        MailboxNodeHandle imapNode;
        TOCType * tocH;

        // locate the inbox, and resync it.
        if ((imapNode = LocateInboxForPers(CurPers))) {
          FSSpec inboxSpec = imapNode->mailboxSpec;
          tocH = TOCBySpec(&inboxSpec);
          IMAPCheckThreadRunning++;
          if (FetchNewMessages(tocH, true, true, true, !manual)) {
            // remember if this was a manual mail check.  the No New mail
            // dialog might have to be shown.
            gWasManualIMAPCheck = manual;

            // resync all open IMAP mailboxes as well, if we should
            if (PrefIsSet(IMAP_RESYNC_OPEN_MAILBOXES))
              ResyncOpenMailboxes(CurPers);

            // poll mailboxes if we ought to
            IMAPPoll(CurPers);
          }
          IMAPCheckThreadRunning--;
        }
      } else {
        AuditCheckStart(++gCheckSessionID, CurPers->persId, !manual);
        StartStreamAudit(mailStream, kAuditBytesReceived);

        g_print("XferMailLo: [11b] calling CheckForMail\n"); fflush(stdout);
        err = CheckForMail(mailStream, &gotSome, &flags) || err;
        g_print("XferMailLo: [11c] CheckForMail returned err=%d gotSome=%d\n", err, gotSome); fflush(stdout);

        StopStreamAudit(mailStream);
        AuditCheckDone(gCheckSessionID, gotSome,
                       gotSome ? ReportStreamAudit(mailStream) : 0);
      }
      CurPers->checked = true;
    }


    /*
     * send mail
     * For CTB connections, we only do this if the POP check went ok,
     * since we don't know what state the line may be in.  For MacTCP or UUPC,
     * the two are independent so we really don't care.
     */
    willSend =
        !CommandPeriod && send && (!UseCTB || !err) && !gPPPConnectFailed;
    g_print("XferMailLo: [12] willSend=%d\n", willSend);
    /*
     * Set task we're about to do in case we need to report it if we're low on
     * memory
     */
    if (willSend)
      SetCurrentTaskKind(SendingTask);
    if (MonitorGrow(true))
      return (0);

    if (willSend) {
      g_print("XferMailLo: [13] calling SendTheQueue\n");
      err = SendTheQueue(mailStream, flags) || err;
      g_print("XferMailLo: [14] SendTheQueue returned err=%d\n", err);
    }
  }

  anyErr = err || CommandPeriod;

  /*
   * cleanup connection
   */
  // CloseProgress();

done:
  if (totalGot)
    *totalGot += gotSome;
  if (mailStream)
    ZapTransStream(&mailStream);
  NonNullTicks = TickCount(); // We pretend that finishing a mailcheck is a
                              // meaningful "event"
  return (anyErr);
}

/**********************************************************************
 *
 **********************************************************************/
bool NeedPassword(bool check, bool send) {
  bool needPW;
  char s[256];
  bool doesAuth = PrefIsSet(PREF_SMTP_DOES_AUTH);
  bool authOK = !PrefIsSet(PREF_SMTP_AUTH_NOTOK);
  bool xtndXmit = PrefIsSet(PREF_POP_SEND);
  bool doggieStyle = PrefIsSet(PREF_KERBEROS);
  bool gave530 = ShouldSMTPAuth();

  if (!CurPers->doMeNow)
    return (false);

  /* Need password for POP or IMAP check */
  needPW = !PrefIsSet(PREF_KERBEROS) && check &&
           *GetPOPPref((unsigned char *)s);

  // Are we going to send in a potentially auth-able way?
  if (!needPW && (doesAuth || xtndXmit) && send && !UUPCOut && !doggieStyle) {
    needPW = authOK || xtndXmit;

    // We'd like to auth.
    if (!xtndXmit && doesAuth) {
      // If the server says yes but the user says no,
      // better ask the user to change their mind
      if (gave530 && !authOK) {
        PCopy(s, CurPers->name);
        switch (ComposeStdAlert(Note, RECONSIDER_AUTH, s)) {
        // user will give us the password.  Yippee.
        case kAlertStdAlertOKButton:
          SetPref(PREF_SMTP_AUTH_NOTOK, NoStr);
          needPW = true;
          break;
        // user wants to abort the send.  Ok.
        case kAlertStdAlertCancelButton:
          CurPers->sendMeNow = false;
          CurPers->doMeNow = CurPers->checkMeNow;
          break;
        // user thinks he can spit at god.  Good luck.
        default:
          needPW = false;
          break;
        }
      }
    }
  }

  return (needPW && !*CurPers->password);
}

typedef enum {
  sxfOK = 1,
  sxfCancel,
  sxfCheck,
  sxfSend,
  sxfServDel,
  sxfServFetch,
  sxfNuke,
  sxfNukeHard,
  sxfStub,
  sxfIcon = 11,
  sxfLabel = 14,
  sxfVertical,
  sxfList = 17
} SXFDialogEnum;

// Helper for SpecialXfer GTK4 dialog
static void special_xfer_ok_clicked(GtkButton *btn, gpointer data) {
  GtkWidget *dlg = GTK_WIDGET(data);
  g_object_set_data(G_OBJECT(dlg), "response",
                    GINT_TO_POINTER(GTK_RESPONSE_OK));
  gtk_window_close(GTK_WINDOW(dlg));
}

/**********************************************************************
 * POPHostLimit - limit the domain of hosts the user is allowed to connect to
 **********************************************************************/
int POPHostLimit(void) {
  char user[256], host[256];
  char sub[64];
  short i;

  if (*GetRString(sub, OnlyHostsStrn + 2)) {
    GetPOPInfo(user, host);
    for (i = 2; *GetRString(sub, OnlyHostsStrn + i); i++)
      if (PFindSub(sub, host))
        return (0);
    WarnUser(OnlyHostsStrn + 1, 0);
    return (fnfErr);
  }
  return (0);
}

// Porting: Replaced Legacy Mac Dialogs with GTK4
int SpecialXfer(struct XferFlags *flags) {
  GtkWidget *dialog, *content_box, *grid, *button_box;
  GtkWidget *check_chk, *send_chk, *del_chk, *fetch_chk, *nuke_chk,
      *nuke_hard_chk, *stub_chk;
  GtkWidget *cancel_btn, *ok_btn;
  int response = GTK_RESPONSE_NONE;
  PersHandle pers;

  // Create GTK4 Window (not deprecated GtkDialog)
  dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Check Mail");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);

  // Main content box
  content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(content_box, 12);
  gtk_widget_set_margin_end(content_box, 12);
  gtk_widget_set_margin_top(content_box, 12);
  gtk_widget_set_margin_bottom(content_box, 12);
  gtk_window_set_child(GTK_WINDOW(dialog), content_box);

  // Grid for checkboxes
  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
  gtk_box_append(GTK_BOX(content_box), grid);

  // Create Checkboxes
  check_chk = gtk_check_button_new_with_label("Check for Mail");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(check_chk), flags->check);
  gtk_grid_attach(GTK_GRID(grid), check_chk, 0, 0, 1, 1);

  send_chk = gtk_check_button_new_with_label("Send Queued Mail");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(send_chk), flags->send);
  gtk_grid_attach(GTK_GRID(grid), send_chk, 0, 1, 1, 1);

  fetch_chk = gtk_check_button_new_with_label("Fetch (Don't Delete)");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(fetch_chk), flags->servFetch);
  gtk_grid_attach(GTK_GRID(grid), fetch_chk, 1, 0, 1, 1);

  del_chk = gtk_check_button_new_with_label("Delete from Server");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(del_chk), flags->servDel);
  gtk_grid_attach(GTK_GRID(grid), del_chk, 1, 1, 1, 1);

  nuke_chk = gtk_check_button_new_with_label("Nuke Messages");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(nuke_chk), flags->nuke);
  gtk_grid_attach(GTK_GRID(grid), nuke_chk, 0, 2, 1, 1);

  nuke_hard_chk = gtk_check_button_new_with_label("Hard Nuke");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(nuke_hard_chk), flags->nukeHard);
  gtk_grid_attach(GTK_GRID(grid), nuke_hard_chk, 1, 2, 1, 1);

  stub_chk = gtk_check_button_new_with_label("Stub Mode");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(stub_chk), flags->stub);
  gtk_grid_attach(GTK_GRID(grid), stub_chk, 0, 3, 2, 1);

  // Personalities List (if more than 1) - Use GTK4 checkboxes instead of
  // deprecated tree view
  if (PersCount() > 1) {
    GtkWidget *pers_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *label = gtk_label_new("Select Personalities:");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(pers_box), label);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_size_request(scrolled_window, -1, 150);

    GtkWidget *pers_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
                                  pers_list_box);

    // Fill with personality checkboxes
    for (pers = PersList; pers; pers = pers->next) {
      char name[64];
      // Copy personality name (C string)
      strncpy(name, (const char *)pers->name, 63);
      name[63] = '\0';

      GtkWidget *pers_chk = gtk_check_button_new_with_label(name);
      gtk_check_button_set_active(GTK_CHECK_BUTTON(pers_chk), pers->doMeNow);
      g_object_set_data(G_OBJECT(pers_chk), "pers_handle", pers);
      gtk_box_append(GTK_BOX(pers_list_box), pers_chk);
    }

    gtk_box_append(GTK_BOX(pers_box), scrolled_window);
    gtk_box_append(GTK_BOX(content_box), pers_box);
  }

  // Button box at bottom
  button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign(button_box, GTK_ALIGN_END);

  cancel_btn = gtk_button_new_with_label("Cancel");
  g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close),
                           dialog);
  gtk_box_append(GTK_BOX(button_box), cancel_btn);

  ok_btn = gtk_button_new_with_label("OK");
  g_object_set_data(G_OBJECT(ok_btn), "response",
                    GINT_TO_POINTER(GTK_RESPONSE_OK));
  g_object_set_data(G_OBJECT(ok_btn), "dialog", dialog);
  gtk_box_append(GTK_BOX(button_box), ok_btn);

  gtk_box_append(GTK_BOX(content_box), button_box);

  // Show and run modal loop
  gtk_widget_set_visible(dialog, TRUE);

  // GTK4 modal implementation: store response via object data
  g_object_set_data(G_OBJECT(dialog), "response",
                    GINT_TO_POINTER(GTK_RESPONSE_CANCEL));
  g_signal_connect(ok_btn, "clicked", G_CALLBACK(special_xfer_ok_clicked),
                   dialog);

  // Simple blocking loop for modal behavior
  GMainContext *context = g_main_context_default();
  while (gtk_window_is_active(GTK_WINDOW(dialog))) {
    g_main_context_iteration(context, TRUE);
  }
  response = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "response"));

  if (response == GTK_RESPONSE_OK) {
    flags->check = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_chk));
    flags->send = gtk_check_button_get_active(GTK_CHECK_BUTTON(send_chk));
    flags->servFetch = gtk_check_button_get_active(GTK_CHECK_BUTTON(fetch_chk));
    flags->servDel = gtk_check_button_get_active(GTK_CHECK_BUTTON(del_chk));
    flags->nuke = gtk_check_button_get_active(GTK_CHECK_BUTTON(nuke_chk));
    flags->nukeHard =
        gtk_check_button_get_active(GTK_CHECK_BUTTON(nuke_hard_chk));
    flags->stub = gtk_check_button_get_active(GTK_CHECK_BUTTON(stub_chk));

    // Update personalities from checkboxes if multiple personalities
    if (PersCount() > 1) {
      // Find the pers_list_box and iterate its children
      GtkWidget *w = gtk_widget_get_first_child(content_box);
      while (w) {
        if (GTK_IS_BOX(w)) {
          GtkWidget *scroll = gtk_widget_get_first_child(w);
          if (scroll && GTK_IS_SCROLLED_WINDOW(scroll)) {
            GtkWidget *list_box =
                gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scroll));
            if (list_box) {
              GtkWidget *chk = gtk_widget_get_first_child(list_box);
              while (chk) {
                if (GTK_IS_CHECK_BUTTON(chk)) {
                  PersHandle p =
                      g_object_get_data(G_OBJECT(chk), "pers_handle");
                  if (p) {
                    bool active =
                        gtk_check_button_get_active(GTK_CHECK_BUTTON(chk));
                    p->doMeNow = p->checkMeNow = p->sendMeNow = active;
                  }
                }
                chk = gtk_widget_get_next_sibling(chk);
              }
            }
          }
        }
        w = gtk_widget_get_next_sibling(w);
      }
    }
  }

  return 0;
}

/**********************************************************************
 * GoOnline - always assume online (removed - defined in legacy_shim.h)
 **********************************************************************/

/************************************************************************
 * SendTheQueue - send queued messages, assuming cnxn is setup
 ************************************************************************/
short SendTheQueue(TransStream stream, XferFlags flags) {
  WindowPtr tocMessWinWP;
  TOCType * tocH = NULL;
  int sumNum = -1;
  char server[256];
  long port;
  MessHandle messH;
  void * table;
  short err, code = 0;
  short tableId;
  uint32_t gmtSecs = GMTDateTime();
  short defltTableId;
  unsigned char *tablePtr = malloc(256);
  short lastId = 0;
  uint32_t lastSig = 0xffffffff;
  short stayed = 0;
  long count;
  CSpecHandle fccList = (CSpecHandle)NuHandle(0);
  bool openedFilters = false;
  TOCType * realTocH = NULL;
  bool inThread = InAThread();
  short realSumNum = -1;
  static uint32_t sessionID;
  uint32_t numSent;
  uint32_t beforeBytes, actualBytes, approxBytes;
  uint32_t beforeTicks;
  char s[256];
  PersHandle relayPers = NULL;

  g_print("SendTheQueue: [A] entering, inThread=%d\n", inThread);
  if (inThread) {
    SetCurrentTaskKind(SendingTask);
    RemoveTaskErrors(SendingTask, CurPers->persId);
  }
  g_print("SendTheQueue: [B] after RemoveTaskErrors\n");
  if (PersCount() == 1)
    GetRString(s, SENDING_MAIL);
  else {
    ComposeRString(s, PERS_SENDING_MAIL, (unsigned char *)CurPers->name);
  }
  g_print("SendTheQueue: [B2] calling ProgressMessage s='%s'\n", s);
  ProgressMessage(kpTitle, s);
  g_print("SendTheQueue: [C] after ProgressMessage\n");
  /*
   * clear this stupid error condition
   */
  CommandPeriod = false;

  defltTableId = GetPrefLong(PREF_OUT_XLATE);
  if (!NewTables || defltTableId == DEFAULT_TABLE)
    defltTableId = TransOutTablID();
  if (!PrefIsSet(PREF_NO_FLATTEN)) {
    Flatten = GetFlatten();
  }

  // If using a relay personality, switch to it now
  if ((relayPers = SMTPRelayPers()))
    PushPers(relayPers);

  port = GetSMTPPort();
  GetSMTPInfoLo(server, &port);
  g_print("SendTheQueue: [D] server='%s' port=%ld\n", server, port);

  ComposeLogR(LOG_SEND, NULL, START_SEND_LOG, server, port);
  AuditSendStart(++sessionID, CurPers->persId, flags.isAuto);
  numSent = 0;
  StartStreamAudit(stream, kAuditBytesSent);
  if (!UUPCOut && !UUPCIn && PrefIsSet(PREF_POP_SEND)) {
    /* POP-before-SMTP auth path removed (was ESSL-only) */
  }
  g_print("SendTheQueue: [E] calling StartSMTP\n");
  err = StartSMTP(stream, server, port);
  g_print("SendTheQueue: [F] StartSMTP returned err=%d\n", err);

  // if using a relay personality, kill it now
  if (relayPers) {
    PopPers();
    relayPers = NULL;
  }

  if (err)
    goto done;

  if (!(tocH = GetOutTOC()))
    goto done;
  if (!NewTables && !TransOut &&
      (table = GetResource_('taBL', TransOutTablID()))) {
    memmove(tablePtr, table, 256);
    TransOut = tablePtr;
    lastId = TransOutTablID();
  }
  count = CurPers->sendQueue;
  if (!inThread)
    TotalQueuedSize = FindTotalQueuedSize(tocH, gmtSecs);

  ByteProgress(NULL, 0, TotalQueuedSize);

  if (inThread || (openedFilters = !RegenerateFilters()))
    for (sumNum = 0; sumNum < tocH->count && code < 600 && !CommandPeriod &&
                     !EjectBuckaroo;
         sumNum++)
      if (!tocH->sums[sumNum].messH && IsQueued(tocH, sumNum) &&
          tocH->sums[sumNum].persId == CurPers->persId &&
          tocH->sums[sumNum].seconds <= gmtSecs) {
        // TimeStamp(tocH,sumNum,0,0);
        //			  ProgressR(NoBar,count--,0,LEFT_TO_TRANSFER,NULL);
        ProgressR(NoChange, count--, 0, LEFT_TO_TRANSFER, NULL); // clarence

        /*
         * ready a translation table, if needed
         */
        tableId = EffectiveTID(tocH->sums[sumNum].tableId);
        if (tableId != lastId) {
          if (tableId != NO_TABLE && tablePtr &&
              (table = GetResource_('taBL', tableId))) {
            memmove(tablePtr, table, 256);
            TransOut = tablePtr;
            lastId = tableId; /* so we don't have to fetch it next time */
          } else
            TransOut = NULL; /* no table */
        }

        /*
         * signature
         */
        if (lastSig != tocH->sums[sumNum].sigId)
          GrabSignature(lastSig = tocH->sums[sumNum].sigId);

        beforeBytes = GetProgressBytes();
        beforeTicks = TickCount();

        /*
         * actually send the message
         */
        if (!(code =
                  (UUPCOut ? UUPCSendMessage(tocH, sumNum, fccList)
                           : MySendMessage(stream, tocH, sumNum, fccList)))) {
      // OutTypeEnum	outType;  // Unused
            RegisterSuccess(1); // note success in registration
          numSent++;
          messH = tocH->sums[sumNum].messH;

          // if ((outType = tocH->sums[sumNum].outType))
          // 	UpdateNumStat(outType==OUT_FORWARD?kStatForwardMsg:outType==OUT_REPLY?kStatReplyMsg:kStatRedirectMsg,1);

          // adjust progress bar
          if (messH) {
            long rate;
            actualBytes = GetProgressBytes() - beforeBytes;
            rate =
                (actualBytes * 600) / ((TickCount() - beforeTicks + 1) * 1024);
            ComposeLogS(LOG_TPUT, NULL, (unsigned char *)"%d.%d KBps", rate / 10, rate % 10);
            approxBytes = (ApproxMessageSize(messH) K);
            if (actualBytes < approxBytes)
              ByteProgress(NULL, actualBytes - approxBytes, 0);
            else
              ByteProgressExcess(approxBytes - actualBytes);
          }

          SetState(tocH, sumNum, SENT);
          if (!PrefIsSet(PREF_CORVAIR) && WriteTOC(tocH)) {
            code = 600;
            break;
          } /* happy, Dave? */
          /* fcc and filtering of outgoing messages should be done after all
           * messages sent in the main thread */
          if (!inThread) {
            if (fccList && GetHandleSize_(fccList))
              DoFcc(tocH, sumNum, fccList);
            if (messH) {
              err = FilterMessage(flkOutgoing, tocH, sumNum);
              if (err == euFilterXfered) {
                sumNum--;
                continue;
              } else
                stayed++;
            }
          } else
            stayed++;
          /* don't delete message if we're in a thread. we'll do that from the
           * main thread. */
          if (!inThread && (tocH->sums[sumNum].flags & FLAG_KEEP_COPY) == 0)
          {
            if (messH && MessOptIsSet(messH, OPT_ATT_DEL))
              CompAttDel(messH);

            if (messH && messH->win)
              CloseMyWindow(GetMyWindowWindowPtr(messH->win));
            DeleteMessage(tocH, sumNum, false);
            sumNum--;      /* back up, since we deleted the message */
            RedoTOC(tocH); /* keep nit-pickers happy */
            stayed--;
          } else if (messH && messH->win)
            CloseMyWindow(GetMyWindowWindowPtr(messH->win));
        } else {
          if (tocH->sums[sumNum].messH) {
            tocMessWinWP =
                GetMyWindowWindowPtr(tocH->sums[sumNum].messH->win);
            if (tocMessWinWP && !IsWindowVisible(tocMessWinWP))
              CloseMyWindow(tocMessWinWP);
          }
          if (IsAddrErr(code)) {
            tocH->sums[sumNum].flags |= FLAG_ADDRERR;
            OpenAddrErrs = true;
          }
        }
        // update outgoing message status if we're in a thread
        if (inThread) {
          if (realTocH = GetRealOutTOC()) {
            realSumNum = FindSumByHash(realTocH, tocH->sums[sumNum].uidHash);
            if (realSumNum != -1) {
              SetState(realTocH, realSumNum, tocH->sums[sumNum].state);
              if (tocH->sums[sumNum].state == MESG_ERR) {
                realTocH->sums[realSumNum].flags |= FLAG_ADDRERR;
                OpenAddrErrs = true;
              }
              realTocH->sums[realSumNum].flags |= FLAG_UNFILTERED;
              NeedToFilterOut++;
              if (!PrefIsSet(PREF_CORVAIR))
                WriteTOC(realTocH);
            }
          }
        }
      }
done:
  if (relayPers)
    PopPers();
  StopStreamAudit(stream);
  AuditSendDone(sessionID, numSent, ReportStreamAudit(stream));
  UpdateNumStat(kStatSentMail, numSent);
  if (openedFilters)
    FiltersDecRef();
  ZapHandle(fccList);
  ProgressMessageR(kpSubTitle, CLEANUP_CONNECTION);
  if (tablePtr) {
    free(tablePtr);
    tablePtr = NULL;
  }
  if (Flatten) {
    free(Flatten);
    Flatten = NULL;
  }
  TransOut = NULL;
  if (!UUPCOut && !UUPCIn && PrefIsSet(PREF_POP_SEND)) {
    (void)EndPOP(stream);
  } else
    (void)EndSMTP(stream);
  // if (tocH && tocH->win && sumNum>=0)
  // 	BoxSelectAfter(tocH->win,sumNum);

  FlushTOCs(
      true,
      false); /* save memory, in case Out and Trash are large,
                                                                                               and we're going on to do a check */
  NotifyHelpers(stayed, eMailSent, NULL);

  return (err);
}

/************************************************************************
 * FindTotalQueuedSize - find out how big all the messages are
 ************************************************************************/
long FindTotalQueuedSize(TOCType * tocH, long gmtSecs) {
  short sumNum;
  long size = 0;
  MyWindowPtr win;

  for (sumNum = 0; sumNum < tocH->count; sumNum++)
    if (!tocH->sums[sumNum].messH && IsQueued(tocH, sumNum) &&
        tocH->sums[sumNum].persId == CurPers->persId &&
        tocH->sums[sumNum].seconds <= gmtSecs) {
      if ((win = GetAMessage(tocH, sumNum, NULL, NULL, false))) {
        size += ApproxMessageSize(Win2MessH(win)) K;
        CloseMyWindow(GetMyWindowWindowPtr(win));
      }
    }
  return (size);
}

/**********************************************************************
 * CompAttDel - delete attachments with an outgoing message
 **********************************************************************/
void CompAttDel(MessHandle messH) {
  short index;
  FSSpec spec;
  int err;

  for (index = 1; !(err = GetIndAttachment(messH, index, &spec, NULL)); index++)
    FSpTrash(&spec);
}

/**********************************************************************
 * DoFcc - do the fcc's for a message
 **********************************************************************/
int DoFcc(TOCType * tocH, short sumNum, CSpecHandle list) {
  CSpec spec;
  short n = HandleCount(list);
  int err = 0;
  int oneErr;

  UseFeature(featureFcc);
  while (n--) {
    spec = (*list)[n];
    if ((oneErr = MoveMessageLo(tocH, sumNum, &spec.spec, true, false, true))) {
      tocH->sums[sumNum].flags |= FLAG_KEEP_COPY;
      TOCSetDirty(tocH, true);
      if (!err)
        err = oneErr;
    }
  }

  SetHandleBig_(list, 0);
  return (err);
}

/************************************************************************
 * CheckForMail - need I say more?
 ************************************************************************/
short CheckForMail(TransStream stream, short *gotSome, XferFlags *flags) {
  long interval = TICKS2MINS * GetPrefLong(PREF_INTERVAL);
  void * table;
  short err = 0;
  char s[256];

  if (InAThread()) {
    SetCurrentTaskKind(CheckingTask);
    RemoveTaskErrors(CheckingTask, CurPers->persId);
  }
  if (PersCount() == 1)
    GetRString(s, CHECKING_MAIL);
  else {
    ComposeRString(s, PERS_CHECKING_MAIL, CurPers->name);
  }
  ProgressMessage(kpTitle, s);

  Headering = flags->stub;

  /*
   * is there enough room on this volume?
   */
  if ((err = VolumeMargin(MailRoot.vRef, 0)))
    return (WarnUser(NOT_ENOUGH_ROOM, err));

  /*
   * clear this stupid error condition
   */
  CommandPeriod = false;

  /*
   * you know, I don't remember why this was done, but I'm going to
   * leave it alone for now
   */
  ResetAlertStage();

  /*
   * set up translation table
   */
  if (!NewTables && !TransIn && (table = GetResource_('taBL', TRANS_IN_TABL))) {
    if ((TransIn = malloc(256)))
      memmove(TransIn, table, 256);
  }

  /*
   * check mail
   */
  err = UUPCIn ? GetUUPCMail(true, gotSome)
               : GetMyMail(stream, true, gotSome, flags);

  if (gotSome)
    RegisterSuccess(2);

  /*
   * get rid of table
   */
  if (TransIn) {
    free(TransIn);
    TransIn = NULL;
  }


  return (err);
}

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

void NotifyNewMail(short gotSome, bool noXfer, TOCType * tocH,
                   FilterPB *fpbDelivery) {
  NotifyNewMailLo(gotSome, noXfer, tocH, fpbDelivery, true);
}

void NotifyNewMailLo(short gotSome, bool noXfer, TOCType * tocH,
                     FilterPB *fpbDelivery, bool OpenIn) {
  bool justTrash = false;
  bool soundAnyway = false;
  WindowPtr oldFront = FrontWindow_();
  FilterPB fpb;

  // display NoNewMail alert if no mail was received, and no other check
  // threads are running
  if (!gotSome && !POPrror() && !IMAPCheckThreadRunning &&
      !CheckThreadRunning && !gNewMessages) {
    CloseProgress();
    if (PrefIsSet(PREF_NEW_ALERT)) {
      /* NONEWMAIL_ALERTS disabled in GTK port */
    }
  } else if (gotSome || gNewMessages) {
    CommandPeriod = false; /* if mail check cancelled, don't cancel filtering */
    if (fpbDelivery && tocH) {
      FilterPostprocess(flkIncoming, fpbDelivery);
      gotSome = fpbDelivery->notify;
      soundAnyway = fpbDelivery->doNotifyThing > 0;
    } else if (tocH && !InitFPB(&fpb, false, true)) {
      if (tocH->imapTOC) // filter flagged messages only if this is an IMAP
                            // mailbox
      {
        // Score the incoming mail all at once.
        // This is ok, we're in the foreground anyway.
        if (HasFeature(featureJunk) && JunkPrefBoxHold() && CanScoreJunk()) {
          JunkScoreIMAPBox(tocH, -1, -1, true);
          MoveToIMAPJunk(tocH, -1, GetRLong(JUNK_MAILBOX_THRESHHOLD), &fpb);
        }

        // filter what's left over
        FilterFlaggedMessages(flkIncoming, tocH, &fpb);

        //
        // clean up fpb
        //
        // after an IMAP mailcheck, if messages aren't to be filtered
        // anywhere, the list of mailboxes to be opened is allocated, but
        // zero. FilterPostprocess will do a GetSpecialTOC(IN), which causes
        // the In mailbox to be opened even though it doesn't need to be. This
        // causes some window layering confusion in the Carbon version.
        // -jdboyd
        //
        if (fpb.mailbox && (GetHandleSize((void *)fpb.mailbox) == 0))
          ZapHandle(fpb.mailbox);

        // Show NoNewMail if no mail arrived, and there's no other check
        // threads running only do this if a manual IMAP check happened
        // recently
        if (gWasManualIMAPCheck && !IMAPCheckThreadRunning &&
            !CheckThreadRunning && !gNewMessages && !NeedToFilterIn &&
            !NeedToNotify && !CountFlaggedMessages(tocH)) {
          gWasManualIMAPCheck = false;
          NoNewMailMe = true;
        }
      } else
        FilterMessagesFrom(flkIncoming, tocH, tocH->count - gotSome, &fpb,
                           noXfer);

      FilterPostprocess(flkIncoming, &fpb);
      gotSome = fpb.notify;
      soundAnyway = fpb.doNotifyThing > 0;

      // keep track of all new messages we're receiving
      gNewMessages += gotSome;

      // make sure notification happens if soundAnyway
      if (soundAnyway)
        gNewMessages++;
    }

    // process any reg files we received ...
    ProcessReceivedRegFiles();

    if (!gotSome && !soundAnyway && !gNewMessages)
      return;

    /*
     * let the helper apps know
     */
    if (gotSome) {
      if ((tocH && tocH->imapTOC) || (tocH = GetRealInTOC())) {
        NotifyHelpers(gotSome, eMailArrive, NULL);

        if (OpenIn && !PrefIsSet(PREF_NO_OPEN_IN)) {
          eudora_open_mailbox_by_name("In");
        }
      }
    }

    // display the new mail alert if there are no check threads running, and
    // we received some new messages.
    if (CheckThreadRunning || IMAPCheckThreadRunning || !gNewMessages)
      return;
    else
      gNewMessages = 0;

#if 0 // Legacy Mac Notifications disabled
    AttentionNeeded = true;
    if (PrefIsSet(PREF_NEW_SOUND))
      NewMailSound();
    
    // NMRec and legacy alerts disabled
#endif
  } // Close NotifyNewMailLo
}
WindowPtr OpenBehindMePlease(void) {
  MyWindowPtr win;
  WindowPtr winWP, frontWP, returnWinWP = NULL;

  frontWP = MyFrontNonFloatingWindow();

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
          if (((TOCType *)GetWindowPrivateData(winWP))->which == IN) {
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
void ShowBoxAt(TOCType * tocH, short selectMe, WindowPtr behindWin) {
  WindowPtr tocWinWP = GetMyWindowWindowPtr(tocH->win);

  if (selectMe >= 0)
    SelectBoxRange(tocH, selectMe, selectMe, false, -1, -1);
  RedoTOC(tocH);
  ScrollIt(tocH->win, 0, SortedDescending(tocH) ? REAL_BIG : -REAL_BIG);
  if (IsWindowVisible(tocWinWP)) {
    if (behindWin) {
      if (behindWin != tocWinWP)
        SendBehind(tocWinWP, behindWin);
    } else
      SelectWindow_(tocWinWP);
  } else
    ShowMyWindowBehind(tocWinWP, behindWin);
}

/************************************************************************
 * FumLub - find the FumLub
 ************************************************************************/
short FumLub(TOCType * tocH) {
  short i;
  if (!tocH)
    return (-1);

  RedoTOC(tocH);

  if (SortedDescending(tocH))
    return (0);

  for (i = tocH->count - 1; i >= 0; i--)
    if (tocH->sums[i].state != UNREAD) {
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
    FileSystemError(CANT_READ_SIG, "", fnfErr);
    return;
  }

  if (!g_file_get_contents(path, &sigText, &sigLen, NULL)) {
    FileSystemError(CANT_READ_SIG, path, ioErr);
    g_free(path);
    return;
  }
  g_free(path);

  /* Store as eSignature (plain text) — allocated with g_malloc via GLib,
     but eSignature is typed as UHandle (void**) in Globals.h for legacy
     compatibility. For now, store the raw text pointer. Callers that
     use eSignature will need to treat it as a plain char* buffer. */
  eSignature = (void *)sigText;
}

/************************************************************************
 * AddSigIntro - add the sig introducer to a petehandle or a text handle
 * The sig intro is typically "-- \r" — prepended before the signature.
 ************************************************************************/
bool AddSigIntro(GtkWidget *pte, void **text) {
  unsigned char sigIntro[32];
  bool didIt = false;
  long len;

  if (!*GetRString(sigIntro, SIG_INTRO))
    return false;

  /* Handle the text block */
  if (text && *text) {
    len = GetHandleSize((void *)text);
    if (len > 0) {
      /* Check if intro is already present */
      char *ptr = *(char **)text;
      if (len < *sigIntro || memcmp(ptr, sigIntro + 1, *sigIntro) != 0) {
        /* Prepend the sig intro */
        SetHandleBig_((void *)text, len + *sigIntro);
        if (!MemError()) {
          ptr = *(char **)text;
          memmove(ptr + *sigIntro, ptr, len);
          memmove(ptr, sigIntro + 1, *sigIntro);
          didIt = true;
        }
      }
    }
  }

  /* Handle the pte (GtkTextView) */
  if (pte) {
    long textLen = PETEGetTextLen(NULL, pte);
    if (textLen > 0) {
      PETEInsertTextPtr(NULL, pte, 0, (char *)(sigIntro + 1), *sigIntro, NULL);
      didIt = true;
    }
  }

  return didIt;
}

/************************************************************************
 * RemoveSigIntro - remove the sig introducer from text or pte
 ************************************************************************/
bool RemoveSigIntro(GtkWidget *pte, void **text) {
  unsigned char sigIntro[32];
  long len;
  bool didIt = false;

  if (!*GetRString(sigIntro, SIG_INTRO))
    return false;

  /* Handle the text block */
  if (text && *text) {
    len = GetHandleSize((void *)text);
    if (len >= *sigIntro) {
      char *ptr = *(char **)text;
      if (memcmp(ptr, sigIntro + 1, MIN(*sigIntro, 4)) == 0) {
        memmove(ptr, ptr + *sigIntro, len - *sigIntro);
        SetHandleBig_((void *)text, len - *sigIntro);
        didIt = true;
      }
    }
  }

  /* Handle the pte */
  if (pte) {
    void * h = NULL;
    PeteGetTextAndSelection(pte, &h, NULL, NULL);
    if (h) {
      len = GetHandleSize(h);
      if (len >= *sigIntro) {
        char *ptr = (char *)h;
        if (memcmp(ptr, sigIntro + 1, MIN(*sigIntro, 4)) == 0) {
          PeteDelete(pte, 0, *sigIntro);
          didIt = true;
        }
      }
    }
  }

  return didIt;
}

/* SigSpec removed — use SigPath() from signaturewin.h instead */

/************************************************************************
 * TransmitMessageHi - transmit a message, with ETL plugin support
 ************************************************************************/
int TransmitMessageHi(TransStream stream, MessHandle messH, bool chatter,
                        bool sendDataCmd) {
  int err;

  if (messH->hTranslators)
    err = ETLSendMessage(stream, messH, chatter, sendDataCmd);
  else
    err = TransmitMessage(stream, messH, chatter, true, true, NULL, sendDataCmd);

  if (!err)
    RememberMID(SumOf(messH)->msgIdHash);

  return err;
}

/************************************************************************
 * ProcessReceivedRegFiles - process registration files (not applicable)
 ************************************************************************/
void ProcessReceivedRegFiles(void) {
  /* Registration file processing is Mac-specific and not needed in GTK port */
}

/************************************************************************
 * SMTPRelayPers - find the SMTP relay personality
 ************************************************************************/
PersHandle SMTPRelayPers(void) {
  char persName[64];

  if (PrefIsSet(PREF_NO_RELAY_PARTICIPATE))
    return NULL;

  return FindPersByName(GetPref((unsigned char *)persName, PREF_RELAY_PERSONALITY));
}

/************************************************************************
 * RememberMID - remember a message ID hash for duplicate detection
 ************************************************************************/
int RememberMID(uint32_t midHash) {
  OutgoingMIDListDirty = true;
  if (!OutgoingMIDList)
    OutgoingMIDList = g_array_new(FALSE, FALSE, sizeof(uint32_t));
  g_array_append_val(OutgoingMIDList, midHash);
  return 0;
}

/************************************************************************
 * OutgoingMIDListSave - save the outgoing message ID list to a file
 ************************************************************************/
int OutgoingMIDListSave(void) {
  int err = 0;
  FSSpec spec;

  if (!OutgoingMIDListDirty)
    return 0;

  if (OutgoingMIDList && OutgoingMIDList->len > 0) {
    long limit = GetRLong(OUTGOING_MID_LIST_SIZE);

    /* Trim to limit */
    if ((long)OutgoingMIDList->len > limit)
      g_array_remove_range(OutgoingMIDList, 0, OutgoingMIDList->len - limit);

    /* Write to file in mail root */
    if (!SubFolderSpec(0, &spec)) {
      char path[512];
      snprintf(path, sizeof(path), "%s/outgoing_mids.dat", spec.name);
      FILE *f = fopen(path, "wb");
      if (f) {
        fwrite(OutgoingMIDList->data, sizeof(uint32_t), OutgoingMIDList->len, f);
        fclose(f);
        OutgoingMIDListDirty = false;
      } else {
        err = fnfErr;
      }
    }
  }
  return err;
}

/************************************************************************
 * OutgoingMIDListLoad - load the outgoing message ID list from file
 ************************************************************************/
int OutgoingMIDListLoad(void) {
  int err = 0;
  FSSpec spec;

  if (OutgoingMIDList) {
    g_array_free(OutgoingMIDList, TRUE);
    OutgoingMIDList = NULL;
  }
  OutgoingMIDListDirty = false;

  if (!SubFolderSpec(0, &spec)) {
    char path[512];
    struct stat st;
    snprintf(path, sizeof(path), "%s/outgoing_mids.dat", spec.name);
    if (stat(path, &st) == 0 && st.st_size > 0) {
      long count = st.st_size / sizeof(uint32_t);
      FILE *f = fopen(path, "rb");
      if (f) {
        OutgoingMIDList = g_array_sized_new(FALSE, FALSE, sizeof(uint32_t), count);
        g_array_set_size(OutgoingMIDList, count);
        fread(OutgoingMIDList->data, sizeof(uint32_t), count, f);
        fclose(f);
      }
    }
  }
  return err;
}

/************************************************************************
 * BadgeTheSupidDock - update the dock badge with unread count
 * In GTK, this is a no-op since Linux/GTK doesn't have a dock badge
 * the same way macOS does. Could be extended with libunity or similar.
 ************************************************************************/
void BadgeTheSupidDock(short count, char *text, bool attentionColor) {
  /* No dock badge equivalent in GTK — no-op */
}

/************************************************************************
 * GlobalUnreadCount - count unread messages globally
 ************************************************************************/
long GlobalUnreadCount(void) {
  return PrefBadgeOpenBoxes() ? GlobalOpenUnreadCount() : GlobalInUnreadCount();
}

/************************************************************************
 * GlobalOpenUnreadCount - count unread in all open mailbox windows
 ************************************************************************/
long GlobalOpenUnreadCount(void) {
  long count = 0;
  TOCType *tocH;

  for (tocH = TOCList; tocH; tocH = tocH->next)
    if (!tocH->virtualTOC && tocH->win &&
        IsWindowVisible(GetMyWindowWindowPtr(tocH->win)))
      count += TOCUnreadCount(tocH, PrefBadgeRecent());

  return count;
}

/************************************************************************
 * GlobalInUnreadCount - count unread in In mailbox + IMAP inboxes
 ************************************************************************/
long GlobalInUnreadCount(void) {
  long count = TOCUnreadCount(GetInTOC(), PrefBadgeRecent());
  PersHandle pers;

  for (pers = PersList; pers; pers = pers->next) {
    MailboxNodeHandle node;

    if (!IsIMAPPers(pers))
      continue;

    if ((node = LocateInboxForPers(pers))) {
      FSSpec inboxSpec = node->mailboxSpec;
      count += TOCUnreadCount(TOCBySpec(&inboxSpec), PrefBadgeRecent());
    }
  }

  return count;
}
