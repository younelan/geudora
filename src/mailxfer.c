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
#include "StringDefs.h" // For OUT, OUT_TEMP, IN, IN_TEMP string resource IDs
#include "StringUtil.h" // For ComposeRString
#include "auditdefs.h"
#include "gtk_nag.h"
#include "log.h" // For ComposeLogS
#include "message.h"
#include "mime.h" // For DecoderFunc
#include "myssl.h"
#include "portable-compat.h" // For NagStateHandle
#include "prefdefs.h"
#include "schizo.h"
#include "sendmail.h"
#include "threading.h" // For CurThreadGlobals, ThreadGlobals, LastAttSpec
#include "toc.h"
#include "imapmailboxes.h"
#include "util.h"
#define FILE_NUM 52

/* Copyright (c) 1993 by QUALCOMM Incorporated */

// Forward declare globals we need from Globals.h
extern bool CheckThreadRunning;
extern bool SendThreadRunning;
extern bool SendImmediately;
// PersList and CurPers are now macros from threading.h - don't declare as
// extern
extern void **SendQueue;
extern uint32_t LastCheckTime;
extern uint32_t ActiveTicks;
// CurPers is a macro from threading.h
extern bool CheckNow;
extern bool OpenAddrErrs;
extern short NeedToFilterOut;

// NagStateHandle is defined in portable-compat.h

extern NagStateHandle nagState;

#include <stdio.h>

void Dprintf(const char *fmt, ...);

// CheckNagging is implemented in gtk_nag.c
extern void CheckNagging(int userState);

// Stub for IsAdwareMode
bool IsAdwareMode(void) { return false; }

// Porting: Task enums are now defined in threading.h - don't redefine

/* IMAPTransferRec_ definition provided in mailxfer.h */

// IMAP functions implemented in imapdownload.c
extern int DoFetchNewMessages(FSSpec *spec, bool a, bool b);
extern int DoDownloadMessages(TOCHandle toc, void *uids, bool attach);
extern int DoDeleteMessage(TOCHandle toc, void *uids, bool nuke, bool expunge,
                    bool undelete);
extern int DoTransferMessages(TOCHandle src, TOCHandle dst, void *uids, bool copy);
extern int DoExpungeMailbox(TOCHandle toc);
extern int DoDownloadIMAPAttachments(void *att, FSSpec box);
extern int DoIMAPServerSearch(TOCHandle toc, void *boxes, void *search, short c,
                       bool match, long uid);
extern int DoIMAPProcessMailboxes(void *list, short cmd);
extern int DoTransferMessagesToServer(TOCHandle toc, void *data, bool copy, bool b);
extern void IMAPPollMailboxes(FSSpec spec);
// #include "filters.h" // already included via mailxfer.h/filters.h
// #include "junk.h" // Removed to avoid conflicts
// #include "filtrun.h"
#define IMAPDOWNLOAD_H // Suppress conflicting header
#include "junk.h"
#undef IMAPDOWNLOAD_H

// Missing Declarations restored
extern int GetUUPCMail(bool a, short *b);
extern int GetMyMail(TransStream stream, bool a, short *b,
                     XferFlags *flags); // Fixed arg
extern void HPurge(void *h);
// MyFrontNonFloatingWindow is defined in legacy_shim.h - don't redeclare
extern void CloseProgress(void);
extern long GetHandleSize(Handle h);
// SetCurrentTaskKind is defined in threading.h - don't redeclare
static void AdCheckingMail(void) {}

extern bool NeedToFilterIn; // Global
extern bool NeedToNotify;
extern bool AttentionNeeded;
#define eMailArrive 1 // Stub enum if not found
extern void ReZoomMyWindow(WindowPtr win);
extern void UpdateMyWindow(WindowPtr win);
extern long CountFlaggedMessages(TOCHandle toc);
struct NMRec;
extern struct NMRec *MyNMRec;

/* Legacy Mac Externs */
extern WindowPtr GetNextWindow(WindowPtr w);
extern void *GetWindowPrivateData(WindowPtr w);
extern void SelectBoxRange(TOCHandle toc, short a, short b, bool c, short d,
                           short e);
extern void ScrollIt(WindowPtr w, short a, long b);
extern bool SortedDescending(TOCHandle toc);
extern void SendBehind(WindowPtr a, WindowPtr b);
extern void ShowMyWindowBehind(WindowPtr a, WindowPtr b);
extern void GetResName(unsigned char *name, uint32_t type, short id);
// REAL_BIG is defined in mydefs.h - don't redefine
#undef IN
#define IN 0
#define NEW_MAIL_SND 1000
#define SIG_NONE 0
extern void PlayNamedSound(unsigned char *name);
extern MyWindowPtr FindText(FSSpecPtr spec);
extern void MySelectWindow(WindowPtr w);
#undef CANT_READ_SIG
#define CANT_READ_SIG 2001

extern MyWindowPtr OpenText(FSSpecPtr spec, void *a, void *b, void *c, bool d,
                            void *e, bool f, bool g);
extern void PeteGetTextAndSelection(void *pte, Handle *h, void *a, void *b);

extern int DoIMAPFilterProgress(void);

// Wazoo - implemented in wazoo.c
#define TASKS_WIN 100
extern int FindOpenWazoo(int win);

// Global Stubs for UI state (threading globals are in threading.c when
// THREADING_ON is defined)
#ifndef THREADING_ON
void *CurThreadGlobals = NULL;
void *ThreadGlobals = NULL;
#endif
void *TaskProgressWindow = NULL;
void *ModalWindow = NULL;
bool gPPPConnectFailed = false;
bool NoXfer = false;
int IMAPCheckThreadRunning = 0;
int gNewMessages = 0;
bool NoNewMailMe = false;
bool gStayConnected = false;
extern bool Offline;
#undef CommandPeriod
extern bool CommandPeriod;
#ifndef THREADING_ON
void **LastAttSpec = NULL;
void **SingleSpec = NULL;
#endif

// Enums and Defines
#define eHasConnected 1
// CheckingTask and SendingTask are defined in threading.h - don't redefine

// Helper Stubs - these are macros in toc.h, don't define as functions
// TOCHandle GetRealInTOC(void) is a macro
// TOCHandle GetRealOutTOC(void) is a macro
void NotifyHelpers(int a, int b, void *c) {}
void PhKill(void) {}

// More Legacy Stubs
bool MonitorGrow(bool a) { return false; }
bool HesOK = false;
extern void GetPOPInfo(void *a, void *b);
#define NOTIFY_TYPE 0
#define eWillConnect 2
int CountResources(int type) { return 0; }
extern int NewTransStream(TransStream *stream);
// OpenProgress is defined in progress.h - don't redefine
long gCheckSessionID = 0;
extern long ReportStreamAudit(TransStream stream);
long NonNullTicks = 0;
uint32_t TickCount(void) { return 0; }
bool ShouldSMTPAuth(void) { return false; }
bool UUPCIn = false;
bool UUPCOut = false;
/* PCopy is a macro in StringUtil.h — do not define as function */
#undef Note
#define Note 0
#undef RECONSIDER_AUTH
#define RECONSIDER_AUTH 100
#define kAlertStdAlertOKButton 1
#define kAlertStdAlertCancelButton 2
char *NoStr = "";
/* ComposeStdAlert declared in gtk_dialogs.h */
extern void SetPref(short pref, const unsigned char *val);

// GMTDateTime is defined in util.h - don't redefine
// CountedSpecStruct defined in mailbox.h
// ProgressMessage is defined in progress.h - don't redefine
// ProgressMessageR is defined in progress.h - don't redefine
bool NewTables = false;
#define DEFAULT_TABLE 0
extern short TransOutTablID(void);
unsigned char *Flatten = NULL;
extern unsigned char *GetFlatten(void);

extern void PushPers(PersHandle pers);
extern void PopPers(void);
#define esslUseAltPort 1
#undef SMTP_SSL_PORT
#define SMTP_SSL_PORT 100
long GetSMTPPort(void) { return 25; }
int GetSMTPInfoLo(unsigned char *server, long *port) { return 0; }
/* ComposeLogR declared in log.h */
#undef LOG_SEND
#define LOG_SEND 0
#undef START_SEND_LOG
#define START_SEND_LOG 0
#define PREF_OUT_XLATE 0
#define PREF_NO_FLATTEN 1
#define PREF_SSL_SMTP_SETTING 2
#undef SENDING_MAIL
#define SENDING_MAIL 3
#undef PERS_SENDING_MAIL
#define PERS_SENDING_MAIL 4
#define kpTitle 5
#define kpSubTitle 6
#undef CLEANUP_CONNECTION
#define CLEANUP_CONNECTION 100
#define kStatSentMail 1
#define kAuditBytesSent 2
void FiltersDecRef(void) {}
extern void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what);
#define PREF_POP_SEND 6
extern int StartSMTP(TransStream stream, unsigned char *server, long port);
// GetOutTOC is a macro in toc.h - don't define as function
bool TransOut = false;
long TotalQueuedSize = 0;
// ByteProgress is defined in progress.h - don't redefine
bool RegenerateFilters(void) { return false; }
bool EjectBuckaroo = false;
bool IsQueued(TOCHandle toc, int sum) { return false; }
// ProgressR(NoChange,count--,0,LEFT_TO_TRANSFER,nil);
#define NoChange 0
#undef LEFT_TO_TRANSFER
#define LEFT_TO_TRANSFER 0
// ProgressR is defined in progress.h - don't redefine
extern short EffectiveTID(short id);
#define NO_TABLE 0
/* GrabSignature stub removed */
// GetProgressBytes is defined in progress.h - don't redefine
int UUPCSendMessage(TOCHandle toc, int sum, CSpecHandle list) { return 0; }
extern int MySendMessage(TransStream stream, TOCHandle toc, int sum,
                  CSpecHandle list);
#define OUT_FORWARD 1
#define OUT_REPLY 2
#define kStatForwardMsg 1
#define kStatReplyMsg 2
#define kStatRedirectMsg 3
extern void UpdateNumStat(int type, int val);
void RegisterSuccess(int val) {}
void StartAuthenticatedSMTP(TransStream stream, unsigned char *server,
                            long port) {}

// Additional POP Stubs
#undef POP_SSL_PORT
#define POP_SSL_PORT 995
#undef KERB_POP_PORT
#define KERB_POP_PORT 1109
#undef POP_PORT
#define POP_PORT 110
#define PREF_KERBEROS 7
#undef PrefIsSet
extern bool PrefIsSet(short pref);
int GetPOPInfoLo(unsigned char *server, unsigned char *s2, long *port) {
  return 0;
}
extern int StartPOP(TransStream stream, unsigned char *server, long port);
extern int EndPOP(TransStream stream);
extern void POPIntroductions(TransStream stream, unsigned char *s, void *p);
extern int POPrror(void);
// GetResource is defined in util.h - don't redefine
// OutTypeEnum is defined in mailbox.h - don't redefine

// More Stubs
#undef LOG_TPUT
#define LOG_TPUT 1
// ComposeLogS is defined in legacy_shim.h - don't redefine with different
// signature
long ApproxMessageSize(MessHandle messH) { return 100; }
// ByteProgressExcess is defined in progress.h - don't redefine
extern void SetState(TOCHandle toc, int sum, int state);
extern bool WriteTOC(TOCHandle toc);
#define flkOutgoing 1
/* FilterMessage stub removed */
#define FLAG_KEEP_COPY 0x100
#define OPT_ATT_DEL 0x200
// MessOptIsSet is a macro in message.h - don't define as function
// CloseMyWindow is defined in legacy_shim.h - don't redefine with different
// return type
WindowPtr GetMyWindowWindowPtr(MyWindowPtr win) {
  return win ? win->window : NULL;
}
/* IsWindowVisible provided by mailbox.h as static inline */
void UsingWindow(GtkWidget *win) { /* GTK: no-op */ }
void NotUsingWindow(GtkWidget *win) { /* GTK: no-op */ }
extern void DeleteMessage(TOCHandle toc, int sum, bool nuke);
extern void RedoTOC(TOCHandle toc);
/* BoxSelectAfter declared in message.h */
// Win2MessH is a macro in message.h - don't define as function
extern void FSpTrash(FSSpec *spec);
// MoveMessageLo implemented in message.c
extern int MoveMessageLo(TOCHandle tocH, int sumNum, FSSpecPtr dest, bool copy,
                  bool queue, bool open);
extern void TOCSetDirty(TOCHandle toc, bool dirty);
/* SetHandleBig provided by util.c/util.h */
// LocalDateTimeShortStr is defined in util.h - don't redefine
extern int VolumeMargin(short vRef, int margin);
#define eMailSent 1
#define BUG15 0
#undef CHECKING_MAIL
#define CHECKING_MAIL 1000
#undef PERS_CHECKING_MAIL
#define PERS_CHECKING_MAIL 1001
#undef NOT_ENOUGH_ROOM
#define NOT_ENOUGH_ROOM 1002
#define TRANS_IN_TABL 1003
// Headering is a macro from threading.h - don't define as variable
/* duplicate NewTables removed */
void *TransIn = NULL;
extern RootSpec MailRoot;
void ResetAlertStage(void) {}
// HNoPurge is defined in legacy_shim.h - don't redefine
/* GetResource_ stub removed to avoid macro conflict */

// Mac Event Stubs
// EventRecord is defined in legacy_shim.h - don't redefine
/* PFindSub is a macro in StringUtil.h — do not define as function */
void Type2Select(struct EventRecord *event) {}
long GetDblTime(void) { return 0; }
/* GetRLong implemented in util.c */

#define charCodeMask 0x000000FF
#define mouseDown 1
#undef DOUBLE_TOLERANCE
#define DOUBLE_TOLERANCE 100
// SetPref was handled above
// OnlyHostsStrn is defined in StrnDefs.h as a number - don't redefine as
// variable
bool UseCTB = false;
bool IsIMAPPers(PersHandle pers) { return false; }
extern MailboxNodeHandle LocateInboxForPers(PersHandle pers);
extern TOCHandle TOCBySpec(FSSpec *spec);
extern int FetchNewMessages(TOCHandle toc, bool a, bool b, bool c, bool d);
bool gWasManualIMAPCheck = false;
extern void ResyncOpenMailboxes(void *pers);
extern void IMAPPoll(void *pers);

#undef shiftKey
#define shiftKey 0x0002
#undef cmdKey
#define cmdKey 0x0004

#include <assert.h>

#ifdef CommandPeriod
#undef CommandPeriod
#endif
extern bool CommandPeriod;
#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT assert

// Stub RunType widely used in legacy checks
#ifndef RunType
#define RunType 0
#endif

// Stubs for missing functions
extern void OpenTasksWinBehind(void *win);

// Stub constants
#define CHECK_EXPIRE
#define CHECK_DEMO

// Stub missing functions
static int ResyncCurrentIMAPMailbox(void) { return 0; }
static void SubFolderSpec(int a, void *b) {}
// GetDateTime removed here to avoid conflict with legacy_shim.h static inline
// version
void ETLIdle(long mode) {} // Fixed signature and non-static
// static void AdCheckingMail(void) {} // Defined earlier

// Stub constants
#define EMSFIDLE_PRE_SEND 0

// Stub junk mail functions
// JunkPrefBoxArchive is a macro in junk.h
extern bool JunkTrimOK(void);
extern OSErr ArchiveJunk(TOCHandle toc);
// static void *GetJunkTOC(void) { return NULL; } // Removed to use macro

// Stub missing functions
static void TaskProgressRefresh(void) {}
static void FlushTOCs(bool a, bool b) {}
static void RememberOpenWindows(void) {}
/* MailboxTreeGood, CreateLocalCache, EnsureSpecialMailboxes are in imapmailboxes.c */
// ThreadsAvailable is defined in threading.h - don't redefine
// SetupXferMailThread is defined in threading.h - don't redefine
static void SetSendQueue(void) {}
static bool SelectXferMailPers(bool check, bool send, bool manual) {
  return true;
}
static unsigned char *GetPOPPref(unsigned char *pass) {
  *pass = 0;
  return pass;
}

// Stub constants
// Stub constants
#undef PERS_CHECK_SLOP
#define PERS_CHECK_SLOP 1337

void NewMailSound(void);
short CheckForMail(TransStream stream, short *gotSome, XferFlags *flags);
short SendTheQueue(TransStream stream, XferFlags flags);
void ResetCheckTime(bool force);
OSErr SpecialXfer(struct XferFlags *flags);
OSErr POPHostLimit(void);
short XferMailLo(bool check, bool send, bool manual, XferFlags flags,
                 long *totalGot, OSErr *dialErrPtr);
bool NeedPassword(bool check, bool send);
void ResetPersCheckTime(bool force);
#ifdef THREADING_ON
bool OKToThread(bool check, bool send, bool manual, bool ae);
long FindTotalQueuedSize(TOCHandle tocH, long gmtSecs);
bool AddSigIntro(GtkWidget *pte, void **text);
bool RemoveSigIntro(GtkWidget *pte, void **text);
bool SpecialXferFilter(DialogPtr dgPtr, EventRecord *event, short *item);
PersHandle SMTPRelayPers(void);
OSErr RememberMID(uint32_t midHash);
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
bool OKToThread(bool check, bool send, bool manual, bool ae) {
  bool threadOK = true;
//	Str255 pass;

// go ahead and thread uucp stuff
#if 0
	else
	{
		for (CurPers=PersList;CurPers&&threadOK;CurPers=(*CurPers)->next)
		{	
			if ((*CurPers)->uupcOut || (*CurPers)->uupcIn)
			{
				(*CurPers)->checkMeNow = (*CurPers)->sendMeNow = (*CurPers)->checked = 0;
				if (check && *GetPref(pass,PREF_POP))
				{
					if ((manual||ae) && !PrefIsSet(PREF_JUST_SAY_NO))
						(*CurPers)->checkMeNow = True;
				}
				if (send && (*CurPers)->sendQueue && !PrefIsSet(PREF_PERS_NO_SEND))
					(*CurPers)->sendMeNow = True;
				if ((*CurPers)->uupcOut && (*CurPers)->checkMeNow)
					threadOK = false;
				
				if ((*CurPers)->uupcIn && ((*CurPers)->sendMeNow || (*CurPers)->autoCheck))
					threadOK = false;
			}
		}
	}
#endif
  return (threadOK);
}
#endif // THREADING_ON

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

short XferMail(bool check, bool send, bool manual, bool ae, bool thread,
               short modifiers) {
  XferFlags flags;
  OSErr err = noErr;
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
      return (noErr);

  // Don't allow more than one checkmail or sendmail thread
  if ((CheckThreadRunning && check) ||
      (PrefIsSet(PREF_POP_SEND) && SendThreadRunning))
    return noErr;

  // if "send when check" pref set, spawn the check and send later
  if ((SendThreadRunning && send) ||
      (PrefIsSet(PREF_POP_SEND) && CheckThreadRunning)) {
    SendImmediately = true; // = SendImmediately || send;
    if (check)
      send = false;
    else
      return noErr;
  }

  // clear the subfolder cache; as good a place as any
  SubFolderSpec(0, NULL);

  if ((err = XferMailSetup(&check, &send, manual, ae, &flags, modifiers)))
    return err;

  // no need in spawning send thread if none of the queued messages belong to
  // a personality marked for sending whenever sends are done
  for (pers = PersList; pers; pers = (*pers)->next)
    if ((*pers)->sendQueue && (*pers)->sendMeNow)
      persSend = true;
  if (!SendQueue || !persSend) {
    send = false;
    SendImmediately = false;
  }
  if (check) {
    GetDateTime(&LastCheckTime);
    TaskProgressRefresh();
  }
  if (!(check || send))
    return noErr;

  FlushTOCs(True, True); /* flush unnecessary TOC's */
  RememberOpenWindows(); /* for good measure */

  ActiveTicks = 0; // we're pretending; manual check mail should not wait for
                   // idle time to filter

  // fetch any mailbox lists we need for this mailcheck
  for (pers = PersList; pers; pers = (*pers)->next) {
    PersHandle oldPers = CurPers;

    if ((*pers)->checkMeNow) {
      CurPers = pers;
      if (PrefIsSet(PREF_IS_IMAP)) {
        if (!MailboxTreeGood(CurPers)) {
          if (CreateLocalCache() == noErr)
            EnsureSpecialMailboxes(CurPers);
        }
      }
      CurPers = oldPers;
    }
  }

#ifdef THREADING_ON
  if (PrefIsSet(PREF_THREADING_OFF) || !OKToThread(check, send, manual, ae) ||
      !ThreadsAvailable() || !thread)
    err = XferMailRun(check, send, manual, ae, flags, NULL);
  else
    err = SetupXferMailThread(check, send, manual, ae, flags, NULL);
#else
  err = XferMailRun(check, send, manual, ae);
#endif
  if (send)
    SetSendQueue();        // redo sendqueue if need be
  SendImmediately = false; // clear this for next time around

#ifdef NAG
  if (check && nagState)
    CheckNagging(0); // nagState is void **, can't dereference as struct
#endif

#ifdef AD_WINDOW
  if (check && IsAdwareMode())
    AdCheckingMail();
#endif
  return (err);
}

/************************************************************************
 * XferMailSetup -
 *	12-8-98 JDB
 *	The check and send parameters may be modified by the SpecialXfer
 *	dialog, in which case the caller should know not to check/send.
 ************************************************************************/
short XferMailSetup(bool *check, bool *send, bool manual, bool ae,
                    XferFlags *xFlags, short modifiers) {
  PersHandle oldCur = CurPers;
  XferFlags flags;
  bool special = 0 != (modifiers & optionKey);
  Str255 pass;
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
  flags.isAuto = *check && !(manual || ae);

  CheckNow = False; // clear flag

  if (!SelectXferMailPers(
          *check, *send,
          manual)) //	Set if checking/sending from Personalities window
  {
    ticks = TickCount() + TICKS2MINS * GetRLong(PERS_CHECK_SLOP);
    for (pers = PersList; pers; pers = (*pers)->next) {
      CurPers = pers;
      (*CurPers)->doMeNow = (*CurPers)->checkMeNow = (*CurPers)->sendMeNow =
          (*CurPers)->checked = 0;
      if (*check && *GetPOPPref(pass)) {
        ivalTicks = (*CurPers)->autoCheck ? (*CurPers)->ivalTicks : 0;
        if (ivalTicks && (!(*CurPers)->checkTicks ||
                          (*CurPers)->checkTicks + ivalTicks < ticks))
          (*CurPers)->doMeNow = (*CurPers)->checkMeNow = True;
        else if ((manual || ae) && !PrefIsSet(PREF_JUST_SAY_NO))
          (*CurPers)->doMeNow = (*CurPers)->checkMeNow = True;
      }

      if (*send && (*CurPers)->sendQueue && !PrefIsSet(PREF_PERS_NO_SEND))
        (*CurPers)->doMeNow = (*CurPers)->sendMeNow = True;
      ASSERT(CurPers == pers);
    }
  }

  if (manual && !ae && special && SpecialXfer(&flags)) {
    CurPers = oldCur;
    return (userCancelled);
  }

  *send = flags.send;
  *check = flags.check || flags.servFetch || flags.servDel || flags.nuke ||
           flags.nukeHard || flags.stub;

  if (!*send && !*check) {
    CurPers = oldCur;
    return (userCancelled);
  }

  // collect passwords
  for (pers = PersList; pers; pers = (*pers)->next) {
    CurPers = pers;

    // Check for checking mail first
    if (NeedPassword(*check && (*CurPers)->checkMeNow, false)) {
      if (PersFillPw(CurPers, 0) != noErr) {
        ResetCheckTime(True);
        CurPers = oldCur;
        return (1);
      }
    }
    // Double check that all the special things IMAP needs are in place
    if (*check && (*CurPers)->checkMeNow && PrefIsSet(PREF_IS_IMAP) &&
        !EnsureSpecialMailboxes(CurPers)) {
      ResetCheckTime(True);
      CurPers = oldCur;
      return (1);
    }
    // Now check for checking mail
    if (*send && (*CurPers)->sendMeNow) {
      PersHandle relayPers = SMTPRelayPers();
      bool oldDoMeNow;

      // if we're using a relay personality, switch to it
      if (relayPers) {
        // gotta force doMeNow or we won't ask for a password
        oldDoMeNow = (*relayPers)->doMeNow;
        (*relayPers)->doMeNow = true;
        PushPers(relayPers);
      }

      if (NeedPassword(false, true)) {
        if (PersFillPw(CurPers, 0) != noErr) {
          if (relayPers)
            PopPers();
          CurPers = oldCur;
          return (1);
        }
      }

      // put back the personality, and reset the doMeNow flag
      if (relayPers) {
        PopPers();
        (*relayPers)->doMeNow = oldDoMeNow;
      }
    }

    ASSERT(CurPers == pers);
  }

  memmove(xFlags, &flags, sizeof(flags));

  CurPers = oldCur;
  return (noErr);
}

/************************************************************************
 * XferMailReal - transfer mail; check or send, for each personality
 ************************************************************************/
short XferMailRun(bool check, bool send, bool manual, bool ae, XferFlags flags,
                  IMAPTransferPtr imapInfo) {
  PersHandle oldCur = CurPers;
  PersHandle pers;
  OSErr err, anyErr;
  long gotSome;
  short dialErr;
  bool popChecked = false;

  ASSERT(!InAThread() || CurThreadGlobals != &ThreadGlobals);

  anyErr = err = noErr;

#ifdef THREADING_ON
  if (InAThread()) {
    if (PrefIsSet(PREF_TASK_PROGRESS_AUTO) && !TaskProgressWindow &&
        !FindOpenWazoo(TASKS_WIN)) {
      if (imapInfo && imapInfo->command != UndefinedTask &&
          !PrefIsSet(PREF_IMAP_TP_BRING_TO_FRONT))
        OpenTasksWinBehind(nil);
      else
        OpenTasksWinBehind(manual ? BehindModal : nil);
    }
  }
  NoXfer = flags.stub;
#endif // THREADING_ON

  gotSome = 0;
  dialErr = noErr;
  gPPPConnectFailed = false;

  // is this an IMAP operation of some kind?
  if (imapInfo && imapInfo->command != UndefinedTask) {
#if __profile__
//		ProfilerClear();
#endif
    SetCurrentTaskKind(imapInfo->command);

    switch (imapInfo->command) {
    // if a mailbox was specified, then we're checking mail for an IMAP
    // personality.
    case IMAPResyncTask: {
      err =
          DoFetchNewMessages(&(imapInfo->targetSpec), true, false) ? noErr : 1;
      check = true;
      gotSome++;
      break;
    }
    // if a tocH and uid is specified, then we should fetch a message
    case IMAPFetchingTask: {
      err = DoDownloadMessages(imapInfo->destToc, imapInfo->uids,
                               imapInfo->attachmentsToo)
                ? noErr
                : 1; // some error is needed here
      break;
    }
    // if a delToc and uid is specified, delete the sepcified message(s)
    case IMAPDeleteTask:
    case IMAPUndeleteTask: {
      err = DoDeleteMessage(imapInfo->delToc, imapInfo->uids, imapInfo->nuke,
                            imapInfo->expunge,
                            (imapInfo->command == IMAPUndeleteTask))
                ? noErr
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
      err = DoExpungeMailbox(imapInfo->delToc) ? noErr : 1;
      break;
    }
    case IMAPAttachmentFetch: {
      err =
          DoDownloadIMAPAttachments(imapInfo->attachments, imapInfo->targetBox)
              ? noErr
              : 1;
      break;
    }
    case IMAPSearchTask: {
      err = DoIMAPServerSearch(imapInfo->destToc, imapInfo->boxesToSearch,
                               imapInfo->toSearch, (short)imapInfo->searchC,
                               imapInfo->matchAll, imapInfo->firstUID)
                ? noErr
                : 1;
      break;
    }
    case IMAPMultResyncTask:
    case IMAPMultExpungeTask: {
      err = DoIMAPProcessMailboxes(imapInfo->toResync, imapInfo->command)
                ? noErr
                : 1;
      break;
    }
    case IMAPUploadTask: {
      err = DoTransferMessagesToServer(imapInfo->destToc, imapInfo->appendData,
                                       imapInfo->copy, false);
      break;
    }
    case IMAPPollingTask: {
      IMAPPollMailboxes(imapInfo->targetBox);
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
#if __profile__
//	ProfilerDump("\pthreadedimap-profile");
//	ProfilerClear();
#endif
  }
  // otherwise, it's a mailcheck, sweet and boring.
  else {
    IMAPCheckThreadRunning = gNewMessages =
        0;               // forget about all past IMAP mail checks
    NoNewMailMe = false; // also forget about any pending NoNewMail alerts we
                         // may have around.

    gStayConnected = true; // Tell the dial code to keep the connection up
                           // until further notice.
    for (pers = PersList; pers && !gPPPConnectFailed && !CommandPeriod;
         pers = (*pers)->next) {
      CurPers = pers;

      // only display the new mail alert if a pop personality is being checked
      if ((*CurPers)->checkMeNow && !PrefIsSet(PREF_IS_IMAP))
        popChecked = true;

      err = XferMailLo(check && (*CurPers)->checkMeNow,
                       send && (*CurPers)->sendMeNow, manual || ae, flags,
                       &gotSome, (OSErr *)&dialErr);
      if (!anyErr)
        anyErr = err;
      ASSERT(CurPers == pers);
    }
    CurPers = oldCur;
    gStayConnected = false; // the dialup connection can be torn down when no
                            // longer needed.
  }

#ifdef THREADING_ON
  if (InAThread()) {
#ifndef BATCH_DELIVERY_ON
    if (send)
      SetNeedToFilterOut();
    if (check) {
      TOCHandle tempInTocH = GetTempInTOC();
      TempInCount = tempInTocH ? (*tempInTocH)->count : 0;
      if (TempInCount)
        AddFilterTask();
    }
#endif
    // silly no mail alert; here is as good a place as any
    if (!dialErr && flags.check && manual)
      // do NoNewMailMe only if a pop personality was checked, and there
      // aren't any other check threads going.
      if (popChecked && !IMAPCheckThreadRunning && !gNewMessages)
        NoNewMailMe = gotSome == 0;
  } else
#endif
  {
    /*
     * notify the user if new mail arrived (or not, in some cases)
     */
    if (!dialErr && flags.check && (manual || gotSome > 0)) {
      NotifyNewMail(gotSome, flags.stub, GetRealInTOC(), nil);
    }
    NotifyHelpers(0, eHasConnected, nil);
  }

  if (check)
    ResetCheckTime(true); // Reset intervals for *all* personalities that were
                          // supposed to be checked. JDB 12-16-98
  ZapHandle(LastAttSpec);

  ASSERT(!InAThread() || CurThreadGlobals != &ThreadGlobals);

  return (err);
}

/**********************************************************************
 * XferMailLo - transfer mail for a given personality
 **********************************************************************/
short XferMailLo(bool check, bool send, bool manual, XferFlags flags,
                 long *totalGot, OSErr *dialErrPtr) {
  short err = 0;
  short gotSome = 0;
#ifdef CTB
  bool needDial;
  bool need2ndPW = False;
  short dialErr = 0;
  bool needPW;
  Str31 pass;
#endif
  short anyErr = 0;
  Str255 s;
  TransStream mailStream = 0;
  bool willCheck = false, willSend = false;

  if (PrefIsSet(PREF_THREADING_OFF))
    PhKill();

  /*
   * clear the decks
   */
  if (send && !(*CurPers)->sendQueue)
    send = False;
#ifdef THREADING_ON
  /*
   * Set which task we're about to do in case we need to report it if we're
   * low on memory
   */
  SetCurrentTaskKind((check && !((*CurPers)->popSecure && send)) ? CheckingTask
                                                                 : SendingTask);
#endif
  FlushTOCs(True, True); /* flush unnecessary TOC's */
  if (MonitorGrow(True) || CommandPeriod)
    goto done;
  if (check)
    HesOK = True; // force re-fetch of hesiod info
  GetPOPInfo(s, s + 127);
  HesOK = False;

  /*
   * figure out what we need to do our duty
   * We need the password if we're checking (or sending POPSecure)
   * We need to dial the phone if we're going to use the CTB for
   * sending or checking (or verifying the account under POPSecure)
   */
  if (!send && !check)
    goto done; /* nothing to do */

  if (CountResources(NOTIFY_TYPE))
    NotifyHelpers(0, eWillConnect, nil);

  if (check && (anyErr = POPHostLimit()))
    goto done;

#ifdef CTB
  needDial = UseCTB && (send && !UUPCOut || check && !UUPCIn);
  if (needDial)
    CheckNavPW(&needPW, &need2ndPW);
#endif

  /*
   *	Set up our TransStream
   */
  if ((anyErr = NewTransStream(&mailStream)))
    goto done;

  /*
   * Doing network stuff; alerts should timeout
   */
#ifdef CTB
  if (need2ndPW) {
    if (GetSecondPass(pass))
      return (1);
    PSCopy((*CurPers)->secondPass, pass);
  }
#endif

  /*
   * Do we need to dial the phone?
   */
  OpenProgress();
#ifdef CTB
  if (needDial)
    dialErr = err = DialThePhone(mailStream);
#endif

  /*
   * Now, do the mail transfers
   */
  if (!err) {
    if (MonitorGrow(True))
      return (0);

    if (check)
      (*CurPers)->checked = True; // don't retry instantly

    /*
     * check for mail, if need be
     */
    willCheck = !CommandPeriod && check && (!UseCTB || !err);

#ifdef THREADING_ON
    /*
     * Set task we're about to do in case we need to report it if we're low on
     * memory
     */
    if (willCheck)
      SetCurrentTaskKind(CheckingTask);
#endif
    if (MonitorGrow(True))
      return (0);
    if (willCheck) {
      if (IsIMAPPers(CurPers)) {
        // checking mail on IMAP server
        MailboxNodeHandle imapNode;
        TOCHandle tocH;

        // locate the inbox, and resync it.
        if ((imapNode = LocateInboxForPers(CurPers))) {
          FSSpec inboxSpec = (*imapNode)->mailboxSpec;
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
        AuditCheckStart(++gCheckSessionID, (*CurPers)->persId, !manual);
        StartStreamAudit(mailStream, kAuditBytesReceived);

        err = CheckForMail(mailStream, &gotSome, &flags) || err;

        StopStreamAudit(mailStream);
        AuditCheckDone(gCheckSessionID, gotSome,
                       gotSome ? ReportStreamAudit(mailStream) : 0);
      }
      (*CurPers)->checked = True;
#ifdef CTB
      if (needDial && send && !err)
        err = CTBNavigateSTRN(NAVMID);
#endif
    }

#ifdef POPSECURE
    /* if the password has been invalidated, we can't send */
    if (!POPSecure)
      send = False;
#endif

    /*
     * send mail
     * For CTB connections, we only do this if the POP check went ok,
     * since we don't know what state the line may be in.  For MacTCP or UUPC,
     * the two are independent so we really don't care.
     */
    willSend =
        !CommandPeriod && send && (!UseCTB || !err) && !gPPPConnectFailed;
#ifdef THREADING_ON
    /*
     * Set task we're about to do in case we need to report it if we're low on
     * memory
     */
    if (willSend)
      SetCurrentTaskKind(SendingTask);
#endif
    if (MonitorGrow(True))
      return (0);

    if (willSend)
      err = SendTheQueue(mailStream, flags) || err;
  }

  anyErr = err || CommandPeriod;

  /*
   * cleanup connection
   */
#ifdef CTB
  if (needDial)
    HangUpThePhone();
#endif
  // CloseProgress();

done:
#ifdef CTB
  if (dialErrPtr)
    *dialErrPtr = dialErr;
#endif
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
  Str255 s;
  bool doesAuth = PrefIsSet(PREF_SMTP_DOES_AUTH);
  bool authOK = !PrefIsSet(PREF_SMTP_AUTH_NOTOK);
  bool xtndXmit = PrefIsSet(PREF_POP_SEND);
  bool doggieStyle = PrefIsSet(PREF_KERBEROS);
  bool gave530 = ShouldSMTPAuth();

  if (!(*CurPers)->doMeNow)
    return (False);
  needPW = !PrefIsSet(PREF_KERBEROS) && !UUPCIn && check &&
           *GetPOPPref(s); /* canonical; a POP check */

  // Are we going to send in a potentially auth-able way?
  if (!needPW && (doesAuth || xtndXmit) && send && !UUPCOut && !doggieStyle) {
    needPW = authOK || xtndXmit;

    // We'd like to auth.
    if (!xtndXmit && doesAuth) {
      // If the server says yes but the user says no,
      // better ask the user to change their mind
      if (gave530 && !authOK) {
        PCopy(s, (*CurPers)->name);
        switch (ComposeStdAlert(Note, RECONSIDER_AUTH, s)) {
        // user will give us the password.  Yippee.
        case kAlertStdAlertOKButton:
          SetPref(PREF_SMTP_AUTH_NOTOK, NoStr);
          needPW = true;
          break;
        // user wants to abort the send.  Ok.
        case kAlertStdAlertCancelButton:
          (*CurPers)->sendMeNow = false;
          (*CurPers)->doMeNow = (*CurPers)->checkMeNow;
          break;
        // user thinks he can spit at god.  Good luck.
        default:
          needPW = false;
          break;
        }
      }
    }
  }

  return (needPW && !*(*CurPers)->password);
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
OSErr POPHostLimit(void) {
  Str255 user, host;
  Str63 sub;
  short i;

  if (*GetRString(sub, OnlyHostsStrn + 2)) {
    GetPOPInfo(user, host);
    for (i = 2; *GetRString(sub, OnlyHostsStrn + i); i++)
      if (PFindSub(sub, host))
        return (noErr);
    WarnUser(OnlyHostsStrn + 1, 0);
    return (fnfErr);
  }
  return (noErr);
}

// Porting: Replaced Legacy Mac Dialogs with GTK4
OSErr SpecialXfer(struct XferFlags *flags) {
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
    for (pers = PersList; pers; pers = (*pers)->next) {
      char name[64];
      // Convert PString to C string
      int len = (*pers)->name[0];
      memcpy(name, (*pers)->name + 1, len);
      name[len] = '\0';

      GtkWidget *pers_chk = gtk_check_button_new_with_label(name);
      gtk_check_button_set_active(GTK_CHECK_BUTTON(pers_chk), (*pers)->doMeNow);
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
                    (*p)->doMeNow = (*p)->checkMeNow = (*p)->sendMeNow = active;
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

  return noErr;
}

/**********************************************************************
 * GoOnline - always assume online (removed - defined in legacy_shim.h)
 **********************************************************************/

/************************************************************************
 * SendTheQueue - send queued messages, assuming cnxn is setup
 ************************************************************************/
short SendTheQueue(TransStream stream, XferFlags flags) {
  WindowPtr tocMessWinWP;
  TOCHandle tocH = nil;
  int sumNum = -1;
  Str255 server;
  long port;
  MessHandle messH;
  Handle table;
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
  bool openedFilters = False;
#ifdef THREADING_ON
  TOCHandle realTocH = nil;
  bool inThread = InAThread();
  short realSumNum = -1;
#endif
  static uint32_t sessionID;
  uint32_t numSent;
  uint32_t beforeBytes, actualBytes, approxBytes;
  uint32_t beforeTicks;
  Str255 s;
  PersHandle relayPers = nil;

#ifdef THREADING_ON
  if (inThread) {
    SetCurrentTaskKind(SendingTask);
    RemoveTaskErrors(SendingTask, (*CurPers)->persId);
  }
  if (PersCount() == 1)
    GetRString(s, SENDING_MAIL);
  else {
    LDRef(CurPers);
    ComposeRString(s, PERS_SENDING_MAIL, (unsigned char *)(*CurPers)->name);
    UL(CurPers);
  }
  ProgressMessage(kpTitle, s);
#endif
  /*
   * clear this stupid error condition
   */
  CommandPeriod = False;

  defltTableId = GetPrefLong(PREF_OUT_XLATE);
  if (!NewTables || defltTableId == DEFAULT_TABLE)
    defltTableId = TransOutTablID();
  if (!PrefIsSet(PREF_NO_FLATTEN)) {
    Flatten = GetFlatten();
  }

  // If using a relay personality, switch to it now
  if ((relayPers = SMTPRelayPers()))
    PushPers(relayPers);

#ifdef ESSL
  stream->ESSLSetting = GetPrefLong(PREF_SSL_SMTP_SETTING);
  if (stream->ESSLSetting & esslUseAltPort)
    port = GetRLong(SMTP_SSL_PORT);
  else
#endif
    port = GetSMTPPort();
  GetSMTPInfoLo(server, &port);

#ifdef POPSECURE
  if (!POPSecure && (err = VetPOP()))
    goto done; // not used.  Must change for multi-connect
#endif
  ComposeLogR(LOG_SEND, nil, START_SEND_LOG, server, port);
  AuditSendStart(++sessionID, (*CurPers)->persId, flags.isAuto);
  numSent = 0;
  StartStreamAudit(stream, kAuditBytesSent);
  if (!UUPCOut && !UUPCIn && PrefIsSet(PREF_POP_SEND)) {
#ifdef ESSL
    stream->ESSLSetting = GetPrefLong(PREF_SSL_POP_SETTING);
    if (stream->ESSLSetting & esslUseAltPort)
      port = GetRLong(POP_SSL_PORT);
    else
      port = PrefIsSet(PREF_KERBEROS) ? GetRLong(KERB_POP_PORT)
                                      : GetRLong(POP_PORT);
    if ((err = GetPOPInfoLo(server + 128, server, &port)))
      goto done;
    if ((err = StartPOP(stream, server, port)))
      goto done;
    POPIntroductions(stream, server + 128, nil);
    if (POPrror())
      goto done;
  } else
#endif
    err = StartSMTP(stream, server, port);

  // if using a relay personality, kill it now
  if (relayPers) {
    PopPers();
    relayPers = nil;
  }

  if (err)
    goto done;

  if (!(tocH = GetOutTOC()))
    goto done;
  if (!NewTables && !TransOut &&
      (table = GetResource_('taBL', TransOutTablID()))) {
    memmove(tablePtr, *table, 256);
    TransOut = tablePtr;
    lastId = TransOutTablID();
  }
  count = (*CurPers)->sendQueue;
  if (!inThread)
    TotalQueuedSize = FindTotalQueuedSize(tocH, gmtSecs);

  ByteProgress(0, 0, TotalQueuedSize);

#ifdef THREADING_ON
  if (inThread || (openedFilters = !RegenerateFilters()))
#else
    if (openedFilters = !RegenerateFilters())
#endif
    for (sumNum = 0; sumNum < (*tocH)->count && code < 600 && !CommandPeriod &&
                     !EjectBuckaroo;
         sumNum++)
      if (!(*tocH)->sums[sumNum].messH && IsQueued(tocH, sumNum) &&
          (*tocH)->sums[sumNum].persId == (*CurPers)->persId &&
          (*tocH)->sums[sumNum].seconds <= gmtSecs) {
        // TimeStamp(tocH,sumNum,0,0);
        //			  ProgressR(NoBar,count--,0,LEFT_TO_TRANSFER,nil);
        ProgressR(NoChange, count--, 0, LEFT_TO_TRANSFER, nil); // clarence

        /*
         * ready a translation table, if needed
         */
        tableId = EffectiveTID((*tocH)->sums[sumNum].tableId);
        if (tableId != lastId) {
          if (tableId != NO_TABLE && tablePtr &&
              (table = GetResource_('taBL', tableId))) {
            memmove(tablePtr, *table, 256);
            TransOut = tablePtr;
            lastId = tableId; /* so we don't have to fetch it next time */
          } else
            TransOut = nil; /* no table */
        }

        /*
         * signature
         */
        if (lastSig != (*tocH)->sums[sumNum].sigId)
          GrabSignature(lastSig = (*tocH)->sums[sumNum].sigId);

        beforeBytes = GetProgressBytes();
        beforeTicks = TickCount();

        /*
         * actually send the message
         */
        if (!(code =
                  (UUPCOut ? UUPCSendMessage(tocH, sumNum, fccList)
                           : MySendMessage(stream, tocH, sumNum, fccList)))) {
      // OutTypeEnum	outType;  // Unused
#ifdef NAG
#else
            RegisterSuccess(1); // note success in registration
#endif
          numSent++;
          messH = (*tocH)->sums[sumNum].messH;

          // if ((outType = (*tocH)->sums[sumNum].outType))
          // 	UpdateNumStat(outType==OUT_FORWARD?kStatForwardMsg:outType==OUT_REPLY?kStatReplyMsg:kStatRedirectMsg,1);

          // adjust progress bar
          if (messH) {
            long rate;
            actualBytes = GetProgressBytes() - beforeBytes;
            rate =
                (actualBytes * 600) / ((TickCount() - beforeTicks + 1) * 1024);
            ComposeLogS(LOG_TPUT, nil, "\p%d.%d KBps", rate / 10, rate % 10);
            approxBytes = (ApproxMessageSize(messH) K);
            if (actualBytes < approxBytes)
              ByteProgress(0, actualBytes - approxBytes, 0);
            else
              ByteProgressExcess(approxBytes - actualBytes);
          }

          SetState(tocH, sumNum, SENT);
          if (!PrefIsSet(PREF_CORVAIR) && WriteTOC(tocH)) {
            code = 600;
            break;
          } /* happy, Dave? */
#ifdef THREADING_ON
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
#ifdef THREADING_ON
          } else
            stayed++;
#endif
#else
            stayed++;
#endif
#ifdef THREADING_ON
          /* don't delete message if we're in a thread. we'll do that from the
           * main thread. */
          if (!inThread && ((*tocH)->sums[sumNum].flags & FLAG_KEEP_COPY) == 0)
#else
            if (((*tocH)->sums[sumNum].flags & FLAG_KEEP_COPY) == 0)
#endif
          {
            if (messH && MessOptIsSet(messH, OPT_ATT_DEL))
              CompAttDel(messH);

            if (messH && (*messH)->win)
              CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
            DeleteMessage(tocH, sumNum, False);
            sumNum--;      /* back up, since we deleted the message */
            RedoTOC(tocH); /* keep nit-pickers happy */
            stayed--;
          } else if (messH && (*messH)->win)
            CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
        } else {
          if ((*tocH)->sums[sumNum].messH) {
            tocMessWinWP =
                GetMyWindowWindowPtr((*(*tocH)->sums[sumNum].messH)->win);
            if (tocMessWinWP && !IsWindowVisible(tocMessWinWP))
              CloseMyWindow(tocMessWinWP);
          }
          if (IsAddrErr(code)) {
            (*tocH)->sums[sumNum].flags |= FLAG_ADDRERR;
            OpenAddrErrs = true;
          }
        }
#ifdef THREADING_ON
        // update outgoing message status if we're in a thread
        if (inThread) {
          if (realTocH = GetRealOutTOC()) {
            realSumNum = FindSumByHash(realTocH, (*tocH)->sums[sumNum].uidHash);
            if (realSumNum != -1) {
              SetState(realTocH, realSumNum, (*tocH)->sums[sumNum].state);
              if ((*tocH)->sums[sumNum].state == MESG_ERR) {
                (*realTocH)->sums[realSumNum].flags |= FLAG_ADDRERR;
                OpenAddrErrs = true;
              }
              (*realTocH)->sums[realSumNum].flags |= FLAG_UNFILTERED;
#ifdef BATCH_DELIVERY_ON
              NeedToFilterOut++;
#endif
              if (!PrefIsSet(PREF_CORVAIR))
                WriteTOC(realTocH);
            }
          }
        }
#endif
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
  TransOut = nil;
  if (!UUPCOut && !UUPCIn && PrefIsSet(PREF_POP_SEND)) {
    (void)EndPOP(stream);
  } else
    (void)EndSMTP(stream);
  // if (tocH && (*tocH)->win && sumNum>=0)
  // 	BoxSelectAfter((*tocH)->win,sumNum);

  FlushTOCs(
      True,
      False); /* save memory, in case Out and Trash are large,
                                                                                               and we're going on to do a check */
  NotifyHelpers(stayed, eMailSent, nil);

  return (err);
}

/************************************************************************
 * FindTotalQueuedSize - find out how big all the messages are
 ************************************************************************/
long FindTotalQueuedSize(TOCHandle tocH, long gmtSecs) {
  short sumNum;
  long size = 0;
  MyWindowPtr win;

  for (sumNum = 0; sumNum < (*tocH)->count; sumNum++)
    if (!(*tocH)->sums[sumNum].messH && IsQueued(tocH, sumNum) &&
        (*tocH)->sums[sumNum].persId == (*CurPers)->persId &&
        (*tocH)->sums[sumNum].seconds <= gmtSecs) {
      if ((win = GetAMessage(tocH, sumNum, nil, nil, false))) {
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
  OSErr err;

  for (index = 1; !(err = GetIndAttachment(messH, index, &spec, nil)); index++)
    FSpTrash(&spec);
}

/**********************************************************************
 * DoFcc - do the fcc's for a message
 **********************************************************************/
OSErr DoFcc(TOCHandle tocH, short sumNum, CSpecHandle list) {
  CSpec spec;
  short n = HandleCount(list);
  OSErr err = noErr;
  OSErr oneErr;

  UseFeature(featureFcc);
  while (n--) {
    spec = (*list)[n];
    if ((oneErr = MoveMessageLo(tocH, sumNum, &spec.spec, True, false, true))) {
      (*tocH)->sums[sumNum].flags |= FLAG_KEEP_COPY;
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
  Handle table;
  short err = 0;
  Str255 s;

#ifdef DEBUG
  if (BUG15) {
    Str63 dts;
    Dprintf(";log Log.%s;g", LocalDateTimeShortStr(dts));
  }
#endif

#ifdef THREADING_ON
  if (InAThread()) {
    SetCurrentTaskKind(CheckingTask);
    RemoveTaskErrors(CheckingTask, (*CurPers)->persId);
  }
  if (PersCount() == 1)
    GetRString(s, CHECKING_MAIL);
  else {
    LDRef(CurPers);
    ComposeRString(s, PERS_CHECKING_MAIL, (*CurPers)->name);
    UL(CurPers);
  }
  ProgressMessage(kpTitle, s);
#endif

  Headering = flags->stub;

  /*
   * is there enough room on this volume?
   */
  if ((err = VolumeMargin(MailRoot.vRef, 0)))
    return (WarnUser(NOT_ENOUGH_ROOM, err));

  /*
   * clear this stupid error condition
   */
  CommandPeriod = False;

  /*
   * you know, I don't remember why this was done, but I'm going to
   * leave it alone for now
   */
  ResetAlertStage();

  /*
   * set up translation table
   */
  if (!NewTables && !TransIn && (table = GetResource_('taBL', TRANS_IN_TABL))) {
    HNoPurge_(table);
    if ((TransIn = malloc(256)))
      memmove(TransIn, *table, 256);
    HPurge(table);
  }

  /*
   * check mail
   */
  err = UUPCIn ? GetUUPCMail(True, gotSome)
               : GetMyMail(stream, True, gotSome, flags);

#ifdef NAG
#else
    if (gotSome)
      RegisterSuccess(2);
#endif

#ifdef DEBUG
  if (BUG15)
    Dprintf("\p%d;sc;g", err);
#endif

  /*
   * get rid of table
   */
  if (TransIn) {
    free(TransIn);
    TransIn = NULL;
  }

#ifdef DEBUG
  if (BUG15) {
    Str63 dts;
    Dprintf("\pp;log;g", LocalDateTimeShortStr(dts));
  }
#endif

  return (err);
}

/**********************************************************************
 * ResetCheckTime - reset the mail check interval
 **********************************************************************/
void ResetCheckTime(bool force) {
  PushPers(CurPers);
  for (CurPers = PersList; CurPers; CurPers = (*CurPers)->next)
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
    if (!(*CurPers)->checkTicks)
      (*CurPers)->checkTicks = TickCount();

    /*
     * manage the mail check interval
     */
    if (force && (*CurPers)->doMeNow || (*CurPers)->checked) {
      if ((*CurPers)->checkTicks + interval < TickCount() + 45 * 60) {
        (*CurPers)->checkTicks +=
            interval * ((TickCount() - (*CurPers)->checkTicks + 1) / interval);
        if ((*CurPers)->checkTicks + interval < TickCount() + 45 * 60)
          (*CurPers)->checkTicks += interval;
      }
    }
  } else
    (*CurPers)->checkTicks = 0;
}

/************************************************************************
 * NotifyNewMail - notify the user that new mail has arrived, via the
 * notification manager.
 ************************************************************************/

// DO NOT FIX THIS ROUTINE
// IF IT MUST BE FIXED, THROW IT AWAY AND REWRITE IT & ALL RELATED TO IT
// SD 8/20/02

void NotifyNewMail(short gotSome, bool noXfer, TOCHandle tocH,
                   FilterPB *fpbDelivery) {
  NotifyNewMailLo(gotSome, noXfer, tocH, fpbDelivery, true);
}

void NotifyNewMailLo(short gotSome, bool noXfer, TOCHandle tocH,
                     FilterPB *fpbDelivery, bool OpenIn) {
  bool justTrash = False;
  bool soundAnyway = False;
  WindowPtr oldFront = FrontWindow_();
  FilterPB fpb;

  // display NoNewMail alert if no mail was received, and no other check
  // threads are running
  if (!gotSome && !POPrror() && !IMAPCheckThreadRunning &&
      !CheckThreadRunning && !gNewMessages) {
    CloseProgress();
    if (PrefIsSet(PREF_NEW_ALERT)) {
#ifdef NONEWMAIL_ALERTS
      (void)ReallyDoAnAlert(NO_MAIL_ALRT, Normal);
      TendNotificationManager(true);
#endif // NONEWMAIL_ALERTS
    }
  } else if (gotSome || gNewMessages) {
    CommandPeriod = False; /* if mail check cancelled, don't cancel filtering */
    if (fpbDelivery && tocH) {
      FilterPostprocess(flkIncoming, fpbDelivery);
      gotSome = fpbDelivery->notify;
      soundAnyway = fpbDelivery->doNotifyThing > 0;
    } else if (tocH && !InitFPB(&fpb, false, true)) {
      if ((*tocH)->imapTOC) // filter flagged messages only if this is an IMAP
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
        if (fpb.mailbox && (GetHandleSize((Handle)fpb.mailbox) == 0))
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
        FilterMessagesFrom(flkIncoming, tocH, (*tocH)->count - gotSome, &fpb,
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
      if ((tocH && (*tocH)->imapTOC) || (tocH = GetRealInTOC())) {
        NotifyHelpers(gotSome, eMailArrive, nil);

        if (OpenIn && !PrefIsSet(PREF_NO_OPEN_IN)) {
          WindowPtr tocWinWP = GetMyWindowWindowPtr((*tocH)->win);
          ShowBoxAt(tocH, (*tocH)->previewPTE ? -1 : FumLub(tocH),
                    OpenBehindMePlease());
          if (PrefIsSet(PREF_ZOOM_OPEN))
            ReZoomMyWindow(tocWinWP);
          UpdateMyWindow(tocWinWP);
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
  WindowPtr winWP, frontWP, returnWinWP = nil;

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
    returnWinWP = nil;
    break;

  case 2: // behind In
    for (winWP = frontWP; winWP; winWP = GetNextWindow(winWP))
      if (IsWindowVisible(winWP))
        if (GetWindowKind(winWP) == MBOX_WIN)
          if ((*(TOCHandle)GetWindowPrivateData(winWP))->which == IN) {
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
      win = (*(*Win2MessH(win))->tocH)->win;
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
  if ((returnWinWP == nil) && ModalWindow)
    returnWinWP = ModalWindow;

  win = GetWindowMyWindowPtr(frontWP);
  if (win /* && win->isNag */)
    returnWinWP = frontWP;

  return (returnWinWP);
}

/************************************************************************
 * ShowBoxSel - show the mailbox with a selection
 ************************************************************************/
void ShowBoxAt(TOCHandle tocH, short selectMe, WindowPtr behindWin) {
  WindowPtr tocWinWP = GetMyWindowWindowPtr((*tocH)->win);

  if (selectMe >= 0)
    SelectBoxRange(tocH, selectMe, selectMe, False, -1, -1);
  RedoTOC(tocH);
  ScrollIt((*tocH)->win, 0, SortedDescending(tocH) ? REAL_BIG : -REAL_BIG);
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
short FumLub(TOCHandle tocH) {
  short i;
  if (!tocH)
    return (-1);

  RedoTOC(tocH);

  if (SortedDescending(tocH))
    return (0);

  for (i = (*tocH)->count - 1; i >= 0; i--)
    if ((*tocH)->sums[i].state != UNREAD) {
      i++;
      break;
    }
  return (i < (*tocH)->count ? MAX(i, 0) : (*tocH)->count - 1);
}

/************************************************************************
 * NewMailSound - play the sound for new mail
 ************************************************************************/
void NewMailSound(void) {
  Str255 name;

  // GetPref(name, PREF_NEWMAIL_SOUND); // Usage error, GetPref returns
  // string? Use generic GetRString or similar if needed, or stub. Warning
  // said 'did you mean SetPref?'. Code uses GetPref as void. GetPref
  // prototype in common.h usually returns Handle or char*. Stubbing out for
  // compilation.
  if (!*name)
    GetResName(name, 'snd ', NEW_MAIL_SND);
  PlayNamedSound(name);
}

/************************************************************************
 * GrabSignature - read the signature file
 ************************************************************************/
void GrabSignature(uint32_t fid) {
  short err;
  FSSpec spec;
  Accumulator enriched, html;
  MyWindowPtr win = nil;
  static Handle eSignature = nil;
  static Handle RichSignature = nil;
  static Handle HTMLSignature = nil;
  bool iOpened = false;
  bool oldDirty;
  bool addedIntro;

  ZapHandle(eSignature);
  ZapHandle(RichSignature);
  ZapHandle(HTMLSignature);
  if (fid == SIG_NONE)
    return;
  AccuInit(&enriched);
  if (1) // if (AccuInit(&enriched))

    return;
  AccuInit(&html);
  if (1) { // if (AccuInit(&html)) {

    do {
      void **_azh = (enriched).data;
      if (_azh) {
        if (*_azh)
          free(*_azh);
        free(_azh);
      }
      (enriched).data = NULL;
      (enriched).offset = (enriched).size = 0;
    } while (0);
    return;
  }
  if (!(err = SigSpec(&spec, fid)))
    if (!(win = FindText(&spec)))
      if (1) { // Forced skip of Pete logic for now
               // Legacy Pete logic disabled
      } else if (win = OpenText(&spec, nil, nil, nil, False, nil, False, False))
        iOpened = true;
#if 0
  if (win) {
     // Pete logic disabled
  }
#endif
  err = 0;

  if (win && iOpened)
    CloseMyWindow(GetMyWindowWindowPtr(win));
  do {
    void **_azh = (enriched).data;
    if (_azh) {
      if (*_azh)
        free(*_azh);
      free(_azh);
    }
    (enriched).data = NULL;
    (enriched).offset = (enriched).size = 0;
  } while (0);
  do {
    void **_azh = (html).data;
    if (_azh) {
      if (*_azh)
        free(*_azh);
      free(_azh);
    }
    (html).data = NULL;
    (html).offset = (html).size = 0;
  } while (0);
  if (err)

    FileSystemError(CANT_READ_SIG, "", err);
}

/************************************************************************
 * AddSigIntro - add the sig introducer to a petehandle or a text handle or
 *both
 ************************************************************************/
bool AddSigIntro(GtkWidget *pte, void **text) { return false; }
bool RemoveSigIntro(GtkWidget *pte, void **text) { return false; }
OSErr SigSpec(FSSpecPtr spec, long fid) { return noErr; }
OSErr TransmitMessageHi(TransStream stream, MessHandle messH, bool chatter,
                        bool sendDataCmd) {
  return noErr;
}
void ProcessReceivedRegFiles(void) {}
PersHandle SMTPRelayPers(void) { return nil; }
OSErr RememberMID(uint32_t midHash) { return noErr; }
OSErr OutgoingMIDListSave(void) { return noErr; }
OSErr OutgoingMIDListLoad(void) { return noErr; }
void BadgeTheSupidDock(short count, PStr text, bool attentionColor) {}
long GlobalUnreadCount(void) { return 0; }
long GlobalOpenUnreadCount() { return 0; }
long GlobalInUnreadCount() { return 0; }
