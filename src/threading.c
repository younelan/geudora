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

/*
        File: Threading.c
        Author: Clarence Wong <cwong@qualcomm.com>
        Date: February 1997 - ...
        Copyright (c) 1997 by QUALCOMM Incorporated

        Comments:

        - To switch threading off, #undef THREADING_ON in PreCompTwoPPC.pch
        - Threads can access all global variables (globals have inter-thread
   global scope by default)
        - Threads can also maintain "intra-thread global" variables via context
   switch procs. (the scope of the variable is global to one specific thread.)
                Check out the threadContextData and threadGlobals structs
   to see the intra-thread globals
        - Do not unload code segments that are referenced in the calling chain
   of any running or stopped thread
        - Out-Context switch procs must be manually called from the thread
   termination proc
*/

#define FILE_NUM 90


#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "StringUtil.h"
#include "log.h"
#include "mailbox.h"
#include "mydefs.h"

#include "Globals.h"
#include "StringDefs.h"
#include "fileutil.h"
#include "message.h" /* IsQueued */
#include "sendmail.h"
#include "threading.h"
#include "toc.h" /* WriteTOC, SetState */

#define THREAD_STACK_SIZE 0 // Let pthread use default stack size

#ifndef MINI_MASK
#define MINI_MASK 0
#endif

// External declarations for missing headers
extern bool IsQueued(TOCType * tocH, short sumNum);
extern long ApproxMessageSize(MessHandle messH);
extern void SetState(TOCType * tocH, short sumNum, short state);
extern int WriteTOC(TOCType * tocH);

// Ensure threadTooManyReqsErr is defined (Mac specific error code)
#ifndef threadTooManyReqsErr
#define threadTooManyReqsErr (-32766)
#endif

extern void TaskProgressRefresh(void);
extern void AddProgressTask(void *threadData);
extern void SetSendQueue(void);
extern bool GetPref(int pref);
extern void GetPrefString(int pref, char *val);
extern void SetPref(int pref, const char *val);
extern bool ShouldSMTPAuth(void);
extern bool DeleteSum(TOCType * tocH, int sumNum);

#define threadNotFoundErr (-32767)

#define kNoThreadID ((pthread_t)0)

/*
        private data structures
*/

typedef struct prefChange_ {
  short pref;
  uLong persId;
} prefChangeRec, *prefChangePtr;

static atomic_int gNumBackgroundThreads = 0;
static pthread_t gMainThreadID = 0;
static atomic_int gCriticalSection = 0;
static pthread_mutex_t gCriticalMutex;
static pthread_once_t gCriticalMutexOnce = PTHREAD_ONCE_INIT;
static void init_critical_mutex(void) {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&gCriticalMutex, &attr);
  pthread_mutexattr_destroy(&attr);
}

// Thread-local storage for globals
threadGlobalsRec ThreadGlobals;
/* Initialized to NULL; main() must call CurThreadGlobals = &ThreadGlobals
   before any code accesses thread globals. Cannot use &ThreadGlobals as
   a TLS initializer — macOS ARM64 doesn't support address fixups in TLS. */
_Thread_local threadGlobalsPtr CurThreadGlobals = NULL;

/*
         private functions
*/
static int CopySettingsForThread(short sourceRefN, PersHandle sourcePerslist,
                                   short *destRefN, PersHandle *destPersList,
                                   PersHandle *destCurPers);
static int DeleteSettingsForThread(short *settingsRefN);
static int SaveSettingsToMainThread(threadDataHandle threadData);
static int InitThreadGlobals(threadGlobalsPtr *newThreadGlobals);
static int DisposeThreadGlobals(threadGlobalsPtr threadGlobals);

int GetFCCs(MessHandle messH, CSpecHandle fccSpecs); // move to sendmail.c?

static int NewXferMail(threadDataHandle *tData, bool check, bool send,
                         bool manual, bool scripted, XferFlags flags,
                         IMAPTransferPtr imapInfo);
void *XferMailThread(void *threadParameter);
void ThreadSwitchProcIn(pthread_t threadBeingSwitched, void *switchProcParam);
void ThreadSwitchProcOut(pthread_t threadBeingSwitched, void *switchProcParam);
void ThreadTermination(pthread_t threadTerminated, void *terminationProcParam);

#ifdef TASK_PROGRESS_ON
#ifdef DEBUG
void InvalTaskProgressBeat(ProgressBHandle prbl);
#endif
#endif

void CheckSelectedGlobals();

/* Accessor for CommandPeriod field — used by files that can't include
   the full threading.h (e.g. fileutil.c) */
short *_CommandPeriodPtr(void) {
  return &CurThreadGlobals->tCommandPeriod;
}

#ifdef DEBUG_THREAD
static void MyDebuggerNewThread(pthread_t threadCreated) {
  fprintf(stderr, "Thread started: %p\n", (void *)threadCreated);
}

static void MyDebuggerDisposeThread(pthread_t threadDeleted) {
  fprintf(stderr, "Thread ended: %p\n", (void *)threadDeleted);
}
#endif

/************************************************************************
 * GetThreadData -
 ************************************************************************/
void GetThreadData(pthread_t threadID, threadDataHandle *threadData) {
  threadDataHandle index;

  *threadData = nil;
  for (index = gThreadData; index; index = index->next)
    if (pthread_equal(index->threadID, threadID)) {
      *threadData = index;
      return;
    }
}

/************************************************************************
 * GetCurrentThreadData -
 ************************************************************************/
void GetCurrentThreadData(threadDataHandle *threadData) {
  pthread_t threadID = pthread_self();
  GetThreadData(threadID, threadData);
}

/************************************************************************
 * GetCurrentThreadPrbl -
 ************************************************************************/
ProgressBlock **GetCurrentThreadPrbl(void) {
  threadDataHandle threadData = nil;

  GetCurrentThreadData(&threadData);
  return (threadData ? threadData->prbl : nil);
}

/************************************************************************
 * InitThreadGlobals - create thread and allocate data for it
 ************************************************************************/
static int InitThreadGlobals(threadGlobalsPtr *newThreadGlobals) {
  if (!(*newThreadGlobals =
            (threadGlobalsPtr)calloc(1, sizeof(**newThreadGlobals))))
    return -108;
  (*newThreadGlobals)->tSettingsRefN = -1;
  (*newThreadGlobals)->tResRefN = -1;
  /* Bypass _Thread_local macro — use memcpy from ThreadGlobals directly.
     Struct copies through _Thread_local pointers miscompile on ARM64 macOS. */
  memcpy(&(*newThreadGlobals)->tCurTrans, &ThreadGlobals.tCurTrans,
         sizeof(TransVector));
  return (0);
}

/************************************************************************
 * CopyOutToTemp - copy queued messages in Out mailbox to temporary Out mailbox
 ************************************************************************/
static int CopyOutToTemp(void) {
  int ii;
  TOCType *tocH, *tempTocH;
  StateEnum state;
  MessHandle messH;
  int err = 0;
  uLong gmtSecs = GMTDateTime();
  PersHandle pers;

  MyThreadBeginCritical();
  tocH = GetRealOutTOC();
  tempTocH = GetTempOutTOC();

  TotalQueuedSize = 0;
  g_print("CopyOutToTemp: tocH=%p tempTocH=%p\n", (void*)tocH, (void*)tempTocH);
  if (tocH && tempTocH) {
    g_print("CopyOutToTemp: real count=%d gmtSecs=%lu\n", tocH->count, gmtSecs);
    // only copy messages ready to be sent now
    for (ii = 0; ii < tocH->count; ii++) {
      g_print("CopyOutToTemp: msg %d state=%d messH=%p persId=%ld seconds=%lu\n",
              ii, tocH->sums[ii].state, (void*)tocH->sums[ii].messH,
              tocH->sums[ii].persId, tocH->sums[ii].seconds);
      if (!tocH->sums[ii].messH && IsQueued(tocH, ii)) {
        pers = FindPersById(tocH->sums[ii].persId);
        g_print("CopyOutToTemp: msg %d IsQueued=YES pers=%p sendMeNow=%d\n",
                ii, (void*)pers, pers ? pers->sendMeNow : -1);
        ASSERT(pers);
        if (pers && pers->sendMeNow &&
            tocH->sums[ii].seconds <= gmtSecs) {
          /*
           * handle open, dirty windows
           */
          {
            /* SaveB4Send opens the message to get a messH.
               On the GTK port OpenComp creates a visible window which
               is wrong for background sending.  We only need messH
               for ApproxMessageSize, so skip if not already open. */
            messH = (MessHandle)tocH->sums[ii].messH;
            MiniEvents();
            state = tocH->sums[ii].state;
            /* Copy message directly to temp TOC — bypass MoveMessageLo
               which re-looks up the TOC via TOCBySpec and fails */
            err = AppendMessage(tocH, ii, &tempTocH, true, true, false);
            g_print("CopyOutToTemp: msg %d AppendMessage err=%d tempCount=%d\n",
                    ii, err, tempTocH->count);
            if (err)
              break;
            SetState(tocH, ii, BUSY_SENDING);
            SetState(tempTocH, tempTocH->count - 1, state);
          }
        }
      }
    }
    g_print("CopyOutToTemp: done, tempTocH->count=%d\n", tempTocH->count);
    BoxFClose(tempTocH, true);
    WriteTOC(tempTocH);
  }
  MyThreadEndCritical();
  return err;
}

/************************************************************************
 * NewXferMail - create thread and allocate data for it
 ************************************************************************/
static int NewXferMail(threadDataHandle *tData, bool check, bool send,
                         bool manual, bool scripted, XferFlags flags,
                         IMAPTransferPtr imapInfo)
{
  threadDataHandle threadData = nil;
  threadContextDataPtr threadContext = nil;
  pthread_t threadID;
  int theError = 0;
  threadGlobalsPtr newThreadGlobals = nil;

  // pthreads don't need UPPs or A5 world setup

  ASSERT(ThreadsAvailable());

  // Don't allow more than one checkmail or sendmail thread
  if ((CheckThreadRunning && check) || (SendThreadRunning && send)) {
    ASSERT(0);
    *tData = nil;
    return threadTooManyReqsErr;
  }

  SendThreadRunning = SendThreadRunning || send;
  CheckThreadRunning = CheckThreadRunning || check;
  if (CheckThreadRunning)
    TaskProgressRefresh();

  IncrementNumBackgroundThreads();

  // set up threadContext
  threadData = (threadDataHandle)calloc(1, sizeof(threadDataRec));
  if (!threadData)
    theError = -108;
  if (threadData)
    threadData->next = gThreadData;
  gThreadData = threadData;
  if (!theError)
    theError = InitThreadGlobals(&newThreadGlobals);
  if (!theError) {
    threadContext = &threadData->threadContext;
    // pthreads don't need A5 world
    threadContext->newThreadGlobals = newThreadGlobals;

    theError = StackInit(sizeof(prefChangeRec), &threadContext->prefStack);
  }
  if (!theError) {
#ifdef TASK_PROGRESS_ON
    if (!(threadData->prbl = NewZH(ProgressBlock)))
      theError = 0;
#endif
    threadData->xferMailParams.send = send;
    threadData->xferMailParams.check = check;
    threadData->xferMailParams.manual = manual;
    threadData->xferMailParams.scripted = scripted;
    memcpy(&threadData->xferMailParams.flags, &flags,
           sizeof(threadData->xferMailParams.flags));
    threadData->imapInfo.command = UndefinedTask;
    if (imapInfo)
      memcpy(&threadData->imapInfo, imapInfo, sizeof(IMAPTransferRec));
  }
  if (!theError)
    theError = CopySettingsForThread(
        ThreadGlobals.tSettingsRefN, ThreadGlobals.tPersList,
        &newThreadGlobals->tSettingsRefN, &newThreadGlobals->tPersList,
        &newThreadGlobals->tCurPers);

  // if send only, use smaller stack size????
  MemCanFail = true;
  // Create pthread - no UPPs needed for pthreads
  if (!theError) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    int result =
        pthread_create(&threadID, &attr, XferMailThread, (void *)threadData);
    pthread_attr_destroy(&attr);

    if (result != 0) {
      theError = result;
    }
  }
  MemCanFail = false;

  // pthreads don't have thread switchers or terminators - handle cleanup in
  // thread function

  if (!theError)
    threadData->threadID = threadID;
#if __profile__
  if (!theError) {
    Size stackSize;
    ProfilerThreadRef threadRef;

    GetDefaultThreadStackSize(kCooperativeThread, &stackSize);
    theError = ProfilerCreateThread(100, stackSize, &threadRef);
    threadData->threadRef = threadRef;
    ASSERT(!theError);
  }
#endif
  // pthreads start immediately - no SetThreadState needed

  if (!theError)
    threadData->threadID = threadID;
#ifdef TASK_PROGRESS_ON
  if (TaskProgressWindow && gTaskProgressInitied) {
    if (!theError && threadData)
      AddProgressTask(threadData);
    else
      InvalContent(TaskProgressWindow);
  }
#endif
  if (theError) {
    if (send)
      SendThreadRunning = false;
    if (check)
      CheckThreadRunning = false;
    DecrementNumBackgroundThreads();
    if (threadData) {
#ifdef TASK_PROGRESS_ON
      ASSERT(threadData->prbl);
      DisposProgress(threadData->prbl);
#endif
      LL_Remove(gThreadData, threadData, (threadDataHandle));
      DisposeThreadGlobals(newThreadGlobals);
      if (threadData) {
        free(threadData);
      }
      threadData = nil;
    }
    if (!manual)
      ResetCheckTime(true);
    CheckOnIdle = false;
  }
  *tData = threadData;
  return (CommandPeriod ? ECANCELED : theError);
}

/************************************************************************
 * SetupXferMailThread - create thread and allocate data for it
 ************************************************************************/
int SetupXferMailThread(bool check, bool send, bool manual, bool scripted,
                          XferFlags flags, IMAPTransferPtr imapInfo)
{
  threadDataHandle sendData = nil, checkData = nil;
  int err = 0;

  // we tried to send it
  if (send)
    SendImmediately = false;
  if (PrefIsSet(PREF_THREADING_SEND_OFF) || PrefIsSet(PREF_POP_SEND) ||
      !(check && send)) {
    err = NewXferMail(&checkData, check, send, manual, scripted, flags, imapInfo);
    sendData = checkData;
  } else {
    if (!(err = NewXferMail(&checkData, check, false, manual, scripted, flags,
                            imapInfo))) {
      if (NewXferMail(&sendData, false, send, manual, scripted, flags, imapInfo))
      {
        if (!SendThreadRunning) {
          ASSERT(checkData);
          if (checkData) {
            checkData->xferMailParams.send = send;
            sendData = checkData;
            SendThreadRunning = send;
          }
        }
      }
    }
  }
#ifdef TASK_PROGRESS_ON
#endif
  if (err != ECANCELED) {
    if (check && !checkData)
      WarnUser(THREAD_CANT_CHECK, err);
    else if (send && !sendData)
      WarnUser(THREAD_CANT_SEND, err);
    if (!err && send && sendData) {
      if ((err = CopyOutToTemp()))
        WarnUser(THREAD_CANT_SEND_ALL, err);
    }
  }
  return err;
}

/************************************************************************
 * DisposeThreadGlobals -
 ************************************************************************/
static int DisposeThreadGlobals(threadGlobalsPtr threadGlobals) {
  if (!threadGlobals)
    return EINVAL;

  CurThreadGlobals = threadGlobals;
  // kill string cache
  if (StringCache) {
    free(StringCache);
    StringCache = nil;
  }

  /*	Kill personalities	*/
  DisposePersonalities();
  PersList = nil;
  CurPers = nil;
  if (PersStack) {
    free(PersStack);
    PersStack = nil;
  }

  // MIME Maps
  if (MMIn) {
    free(MMIn);
    MMIn = nil;
  }
  if (MMOut) {
    free(MMOut);
    MMOut = nil;
  }

  if (GWStack) {
    free(GWStack);
    GWStack = nil;
  }
  if (threadGlobals)
    free(threadGlobals);
  threadGlobals = nil;
  CurThreadGlobals = &ThreadGlobals;
  return (0);
}

/************************************************************************
 * CleanTempOutTOC - now that we update mesg status immediately and
 *									user can
 *move these unlocked messages, we can just clear temp out box
 ************************************************************************/
void CleanTempOutTOC(void) {
  TOCType * tempTocH;
  short ii;
  uLong oldForceSend = ForceSend;

  tempTocH = GetTempOutTOC();
  ASSERT(tempTocH);
  // maybe there's a quicker way???
  if (tempTocH)
    for (ii = 0; ii < tempTocH->count; ii++)
      if (!DeleteSum(tempTocH, ii))
        ii--;
  ForceSend = oldForceSend;
}

/************************************************************************
 * CleanRealOutTOC -
 ************************************************************************/
void CleanRealOutTOC(void) {
  TOCType *tocH = GetRealOutTOC(), *tempTocH = GetTempOutTOC();
  int ii;
  short sum;

  g_print("CleanRealOutTOC: tocH=%p tempTocH=%p\n", (void*)tocH, (void*)tempTocH);
  if (!(tocH && tempTocH)) {
    g_print("CleanRealOutTOC: NULL toc, skipping\n");
    return;
  }

  g_print("CleanRealOutTOC: real count=%d temp count=%d\n", tocH->count, tempTocH->count);
  for (ii = tocH->count - 1; ii >= 0; ii--) {
    g_print("CleanRealOutTOC: msg %d state=%d hash=%u\n", ii, tocH->sums[ii].state, tocH->sums[ii].uidHash);
    if (tocH->sums[ii].state == BUSY_SENDING) {
      sum = FindSumByHash(tempTocH, tocH->sums[ii].uidHash);
      g_print("CleanRealOutTOC: BUSY_SENDING -> found in temp at %d, new state=%d\n",
              sum, (sum != -1) ? tempTocH->sums[sum].state : SENDABLE);
      SetState(tocH, ii, (sum != -1) ? tempTocH->sums[sum].state : SENDABLE);
    }
  }
  fflush(stdout);
}

/************************************************************************
 * GetCurrentTaskKind -
 ************************************************************************/
TaskKindEnum GetCurrentTaskKind(void) {
  threadDataHandle threadData = nil;

  GetCurrentThreadData(&threadData);
  return (threadData ? threadData->currentTask : UndefinedTask);
}

/************************************************************************
 * SetCurrentTaskKind -
 ************************************************************************/
void SetCurrentTaskKind(TaskKindEnum taskKind) {
  threadDataHandle threadData = nil;

  GetCurrentThreadData(&threadData);
  if (threadData)
    threadData->currentTask = taskKind;
}

/************************************************************************
 * killThreads - cancel each thread and wait until they're all dead
 ************************************************************************/
void KillThreads(void) {
  threadDataHandle threadDataIndex;
  // Set cancel flag for each thread
  for (threadDataIndex = gThreadData; threadDataIndex;
       threadDataIndex = threadDataIndex->next)
    SetThreadGlobalCommandPeriod(threadDataIndex->threadID, true);

  // Wait for each thread to finish
  for (threadDataIndex = gThreadData; threadDataIndex;
       threadDataIndex = threadDataIndex->next) {
    if (threadDataIndex->threadID)
      pthread_join(threadDataIndex->threadID, NULL);
  }
}

/************************************************************************
 * XferMailThread - open threaded progress, xfermail, save
 * and delete settings
 ************************************************************************/
void *XferMailThread(void *threadParameter) {
  threadDataHandle threadData = nil;
  int theError = 0;
  xferMailParamsRec xferMailParams;
  IMAPTransferRec imapInfo;

  ASSERT(threadParameter);
  if (!threadParameter)
    return nil;

  threadData = (threadDataHandle)threadParameter;

  /* Switch to thread-local globals — on Mac the Thread Manager did this
     automatically via switch-in procs; with pthreads we do it explicitly */
  ThreadSwitchProcIn(pthread_self(), threadData);

#ifdef DEBUG
  threadData->startTime = TickCount();
#endif
  memcpy(&xferMailParams, &threadData->xferMailParams, sizeof(xferMailParams));
  memcpy(&imapInfo, &threadData->imapInfo, sizeof(IMAPTransferRec));
#ifdef DEBUG_THREAD
  MyDebuggerNewThread(threadData->threadContext.threadID);
#endif
  if (!theError) {
    /* batch delivery is always on — idle tick adjustment not needed */
    theError = XferMailRun(xferMailParams.check, xferMailParams.send,
                           xferMailParams.manual, xferMailParams.scripted,
                           xferMailParams.flags, &imapInfo);
    CloseProgress();
    SaveSettingsToMainThread(threadData);
    DeleteSettingsForThread(&SettingsRefN);
  }
  if (xferMailParams.send) {
    if (CommandPeriod || theError)
      SendImmediately = false;
    CleanRealOutTOC();
    CleanTempOutTOC();
    SetSendQueue();
  }

  /* On Mac, the Thread Manager called the termination proc automatically.
     With pthreads we must call it explicitly to clean up thread data. */
  ThreadTermination(pthread_self(), threadData);

  return nil;
}

/************************************************************************
 * threadSwitchProcIn - switch intra-thread globals in
 ************************************************************************/
void ThreadSwitchProcIn(pthread_t threadBeingSwitched, void *switchProcParam) {
  (void)threadBeingSwitched;
  threadDataHandle threadData;
  threadContextDataPtr threadContext;

  ASSERT(switchProcParam);

  threadData = (threadDataHandle)switchProcParam;
  threadContext = &threadData->threadContext;

  // pthreads don't need A5 world switching

  // restore thread-globals
  CurThreadGlobals = threadContext->newThreadGlobals;
#ifdef TASK_PROGRESS_ON
  ProgWindow = TaskProgressWindow;
#endif

#ifdef DEBUG
  threadData->switchInTime = TickCount();
  threadData->switchCount++;
#endif
}

/************************************************************************
 * threadSwitchProcOut - switch intra-thread globals out
 * 												switch
 *main-thread globals in
 ************************************************************************/
void ThreadSwitchProcOut(pthread_t threadBeingSwitched, void *switchProcParam) {
  (void)threadBeingSwitched;
  threadDataHandle threadData = nil;
  threadContextDataPtr threadContext = nil;

  ASSERT(switchProcParam);
  threadData = (threadDataHandle)switchProcParam;
  threadContext = &threadData->threadContext;

  // pthreads don't need A5 world switching

  // save globals from this thread
  threadContext->newThreadGlobals = CurThreadGlobals;

  // restore main thread-globals (for this thread's view only, thanks to _Thread_local)
  CurThreadGlobals = &ThreadGlobals;

#ifdef DEBUG
  threadData->totalTimeThread += (TickCount() - threadData->switchInTime);
#endif
}

/************************************************************************
 * SaveSettingsToMainThread - copy changed settings back to main
 ************************************************************************/
// popd resource is saved directly to the original settings file in DisposePOPD
static int SaveSettingsToMainThread(threadDataHandle threadData) {
  int theError = 0;
  PersHandle pers, mainPers, oldCurPers;
  StackHandle prefStack;
  char string[256]; // prefs shouldn't exceed 255 chars!!!
  prefChangeRec prefChange;

  ASSERT(threadData);

  /* Lock mutex — we're about to access main thread data structures */
  MyThreadBeginCritical();

  // prefs that were changed by thread should be changed
  prefStack = threadData->threadContext.prefStack;
  threadData->threadContext.prefStack = nil;
  while (prefStack && !StackPop(&prefChange, prefStack)) {
    oldCurPers = CurPers;
    if ((prefChange.persId && (CurPers = FindPersById(prefChange.persId))) ||
        (!prefChange.persId && !(CurPers = nil))) {
      GetPrefString(prefChange.pref, string);
      CurPers = oldCurPers;

      ThreadSwitchProcOut(nil, threadData);
      oldCurPers = CurPers;
      if ((CurPers = FindPersById(prefChange.persId)))
        SetPref(prefChange.pref, string);
      CurPers = oldCurPers;
      ThreadSwitchProcIn(nil, threadData);
    } else
      CurPers = oldCurPers;
  }
  if (prefStack) {
    free(prefStack);
  }

  // go through all personalities in thread and update associated data in main
  // thread
  for (pers = PersList; pers; pers = pers->next) {
    // change context to main thread
    ThreadSwitchProcOut(nil, threadData);
    if ((mainPers = FindPersById(pers->persId))) {
//			mainPers->sendQueue = pers->sendQueue;
#ifdef DEBUG
      if (!BUG6)
#endif
        mainPers->checkTicks =
            MAX(mainPers->checkTicks, pers->checkTicks);
      mainPers->popSecure = pers->popSecure;
      if (pers->dirty)
        mainPers->dirty = 1;
      mainPers->noUIDL = pers->noUIDL;

      /* only copy passwords back if we're a check thread or a send thread using
       * xtnd xmit */
      /* or if this was an IMAP thread, and the passwords were invalidated */
      if ((threadData->xferMailParams.check ||
           (threadData->xferMailParams.send &&
            (PrefIsSet(PREF_POP_SEND) ||
             (ShouldSMTPAuth() && !PrefIsSet(PREF_SMTP_AUTH_NOTOK))))) ||
          ((threadData->currentTask > SendingTask) &&
           (pers->password[0] == 0) && (pers->secondPass[0] == 0)))
      {
        strncpy(mainPers->password, pers->password, sizeof(mainPers->password) - 1);
        mainPers->password[sizeof(mainPers->password) - 1] = '\0';
        strncpy(mainPers->secondPass, pers->secondPass, sizeof(mainPers->secondPass) - 1);
        mainPers->secondPass[sizeof(mainPers->secondPass) - 1] = '\0';
      }
    }
    ThreadSwitchProcIn(nil, threadData);
  }

  // Fix Prr in the main thread, mostly for benefit of broken NotifyNewMail
  {
    int myPrr = Prr;

    ThreadSwitchProcOut(nil, threadData);
    Prr = myPrr;
    ThreadSwitchProcIn(nil, threadData);
  }

  // Fix FixServers in the main thread
  {
    bool myFixServers = FixServers;

    ThreadSwitchProcOut(nil, threadData);
    FixServers = FixServers || myFixServers;
    ThreadSwitchProcIn(nil, threadData);
  }

  MyThreadEndCritical();
  return theError;
}

/************************************************************************
 * CopySettingsForThread - clone personality list for thread
 * Ported: no resource fork duplication needed on Linux/GTK.
 * Settings are in a GKeyFile; we just clone the personality list in memory.
 ************************************************************************/
static int CopySettingsForThread(short sourceRefN, PersHandle sourcePerslist,
                                   short *destRefN, PersHandle *destPersList,
                                   PersHandle *destCurPers) {
  int theError = 0;
  PersHandle oldPers, clone, lastClone = nil;

  (void)sourceRefN; /* resource file ref not used on Linux */

  MyThreadBeginCritical();

  /* Clone the personality linked list */
  for (oldPers = sourcePerslist; oldPers && !theError;
       oldPers = oldPers->next) {
    /* Allocate a Handle-style clone: pointer to pointer to Personality */
    clone = (PersHandle)calloc(1, sizeof(Personality));
    if (!clone) {
      theError = -108; /* ENOMEM */
      break;
    }
    /* Copy the personality data */
    *clone = *oldPers;
    clone->next = nil;

    if (lastClone)
      lastClone->next = clone;
    else
      *destPersList = clone;
    lastClone = clone;
  }
  *destCurPers = *destPersList;

  /* Mark settings as available with a dummy refN */
  *destRefN = theError ? -1 : 1;

  MyThreadEndCritical();
  if (theError) {
    DeleteSettingsForThread(destRefN);
  }
  return (CommandPeriod ? ECANCELED : theError);
}

/************************************************************************
 * DeleteSettingsForThread - free cloned personality list
 * Ported: no temp file to delete on Linux/GTK.
 * Note: the cloned personality list itself is freed by DisposeThreadGlobals
 * via DisposePersonalities() which walks PersList. We just reset the refN.
 ************************************************************************/
static int DeleteSettingsForThread(short *settingsRefN) {
  if (*settingsRefN == -1)
    return 0;

  *settingsRefN = -1;
  return 0;
}

/************************************************************************
 * getNumBackgroundThreads - return number of background threads
 ************************************************************************/
int GetNumBackgroundThreads(void) { return atomic_load(&gNumBackgroundThreads); }
/************************************************************************
 * PushThreadPrefChange -
 ************************************************************************/
int PushThreadPrefChange(short pref) {
  int err = threadNotFoundErr;
  threadDataHandle threadData;

  GetCurrentThreadData(&threadData);

  if (threadData) {
    prefChangeRec prefChange;
    StackHandle prefStack = threadData->threadContext.prefStack;

    if (!prefStack)
      return 0;
    prefChange.pref = pref;

    prefChange.persId = (CurThreadGlobals && CurPers) ? CurPers->persId : 0;
    err = 0;
    StackPush(&prefChange, &prefStack);
  }
  return err;
}

/************************************************************************
 * incrementNumBackgroundThreads - increase number of background threads
 ************************************************************************/
void IncrementNumBackgroundThreads(void) { atomic_fetch_add(&gNumBackgroundThreads, 1); }

/************************************************************************
 * decrementNumBackgroundThreads - decrease number of background threads
 ************************************************************************/
void DecrementNumBackgroundThreads(void) { atomic_fetch_sub(&gNumBackgroundThreads, 1); }

/************************************************************************
 * threadsAvailable - are threads supported?
 ************************************************************************/
bool ThreadsAvailable(void) {
  // pthreads always available on POSIX systems
  return true;
}

/* Resource Manager thread functions — no-ops on Linux/GTK.
   Mac Eudora used these to access the main thread's settings resource fork
   from background threads. With GKeyFile-based prefs, not needed. */

void *GetResourceMainThread(uint32_t theType, short theID) {
  (void)theType; (void)theID;
  return nil;
}

int ZapSettingsResourceMainThread(uint32_t type, short id) {
  (void)type; (void)id;
  return 0;
}

int AddMyResourceMainThread(void *h, uint32_t type, short id,
                              ConstStr255Param name) {
  (void)h; (void)type; (void)id; (void)name;
  return 0;
}

/************************************************************************
 * SetThreadGlobalCommandPeriod -
 ************************************************************************/
void SetThreadGlobalCommandPeriod(pthread_t threadID, bool value) {
  threadGlobalsPtr newGlobals = nil;
  threadDataHandle threadData = nil;

  GetThreadData(threadID, &threadData);

  if (threadData &&
      (newGlobals = threadData->threadContext.newThreadGlobals))
    newGlobals->tCommandPeriod = value;
}

/************************************************************************
 * GetMainGlobalSettingsRefN -
 ************************************************************************/
short GetMainGlobalSettingsRefN(void) { return (ThreadGlobals.tSettingsRefN); }

/************************************************************************
 * MyBeginCritical - with pthreads, use a real mutex
 ************************************************************************/
void MyThreadBeginCritical(void) {
  pthread_once(&gCriticalMutexOnce, init_critical_mutex);
  pthread_mutex_lock(&gCriticalMutex);
  atomic_fetch_add(&gCriticalSection, 1);
}

/************************************************************************
 * MyEndCritical -
 ************************************************************************/
void MyThreadEndCritical(void) {
  int prev = atomic_fetch_sub(&gCriticalSection, 1);
  if (prev <= 0)
    atomic_store(&gCriticalSection, 0);
  pthread_mutex_unlock(&gCriticalMutex);
}

/************************************************************************
 * MyYieldToAnyThread -
 ************************************************************************/
void MyYieldToAnyThread(void) {
#ifdef DEBUG
  CheckSelectedGlobals();
#endif
  if (ThreadsAvailable() && !atomic_load(&gCriticalSection)) {
    sched_yield();
    ThreadYieldTicks = TickCount();
  }
}

/************************************************************************
 * YieldCPUNow - give up the cpu, period
 ************************************************************************/
void YieldCPUNow(void) {
  ThreadYieldTicks = 0;
  if (!InAThread())
    CyclePendulum();
  MyYieldToAnyThread();
}

/************************************************************************
 * MyInitThreads -
 * 		only call this from main thread during initialization
 ************************************************************************/
int MyInitThreads(void) {
  gMainThreadID = pthread_self();
  return 0;
}

/************************************************************************
 * inAThread - am I running from a thread?
 ************************************************************************/
bool InAThread(void) {
  pthread_t threadID;

  if (!ThreadsAvailable())
    return (false);
  threadID = pthread_self();
  return !pthread_equal(threadID, gMainThreadID);
}

/************************************************************************
 * threadTermination - dispose of memory when thread dies
 ************************************************************************/
void ThreadTermination(pthread_t threadTerminated, void *terminationProcParam) {
  threadDataHandle threadData;

  DecrementNumBackgroundThreads();
  if ((threadData = (threadDataHandle)terminationProcParam)) {
    GtkWidget * TaskProgressWindowWP;
#ifdef DEBUG
    long totalTicks = 0;
    long perc = 0;
#endif
    if (threadData->xferMailParams.check)
      CheckThreadRunning = false;
    if (threadData->xferMailParams.send)
      SendThreadRunning = false;
#ifdef DEBUG_THREAD
    MyDebuggerDisposeThread(threadData->threadContext.threadID);
#endif
#if __profile__
    ProfilerDeleteThread(threadData->threadRef);
#endif
    DisposeThreadGlobals(threadData->threadContext.newThreadGlobals);
    if (threadData->threadContext.prefStack) { // should be set to nil in
                                                  // SaveSettingsToMainThread
      free(threadData->threadContext.prefStack);
    }
    ThreadSwitchProcOut(threadTerminated, terminationProcParam);
#ifdef DEBUG
    // log totalTimeAll, totalTimeThread, switchCount
    totalTicks = TickCount() - threadData->startTime;
    perc = (long)(((float)threadData->totalTimeThread / (float)totalTicks) *
                  100);
    fprintf(stderr,
        "Total ticks %ld; %s%s Thread ticks %ld; Percent %ld; Switch count: %d; "
        "Ticks/Switch: %ld\n",
        totalTicks,
        threadData->xferMailParams.check ? "Check" : "",
        threadData->xferMailParams.send ? " Send" : "",
        threadData->totalTimeThread, perc, threadData->switchCount,
        threadData->switchCount
            ? threadData->totalTimeThread / threadData->switchCount
            : 0L);
#endif
#ifdef TASK_PROGRESS_ON
    ASSERT(threadData->prbl);
    DisposProgress(threadData->prbl);
#endif
    LL_Remove(gThreadData, threadData, (threadDataHandle));
    if (threadData) {
      free(threadData);
    }
    threadData = nil;
    TaskProgressWindowWP = GetMyWindowWindowPtr(TaskProgressWindow);
#ifdef TASK_PROGRESS_ON
    if (TaskProgressWindow &&
        GetWindowKind(TaskProgressWindowWP) == TASKS_WIN) {
      if (!GetNumBackgroundThreads() && !TaskDontAutoClose &&
          PrefIsSet(PREF_TASK_PROGRESS_AUTO) && !NewError) {
        CloseMyWindow(TaskProgressWindowWP);
      } else {
        InvalContent(TaskProgressWindow);
      }
    }
#endif
  }
}


/* gWazooListHead is defined in wazoo.c and exported via wazoo.h */
#include "wazoo.h"

#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif


/*	Another cheap knockoff - this one from linkmng.c */
typedef struct privHistoryDStruct {
  FSSpec spec;    /* the history file */
  void *theData; /* the toc */
  bool ro;        /* read only */
  bool dirty;     /* is the history file dirty? */
} privHistoryDesc, *privHistoryDPtr, *privHistoryDHandle;

/* gHistories is defined in linkmng.c */
extern privHistoryDHandle gHistories;

void CheckSelectedGlobals() {
  /* Walk the wazoo list — plain linked list, no Mac void *validation needed */
  for (WazooData *w = gWazooListHead; w != NULL; w = w->next) {
    ASSERT(w->win != NULL);
  }
  /* gHistories validation is deferred until linkmng.c is ported */
}

/* RemoveTaskErrors is implemented in taskProgress.c */

