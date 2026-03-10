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

        -- SEE END OF FILE FOR DESIGN OVERVIEW --
        -- SEE END OF FILE FOR BUG LIST --
*/

#define FILE_NUM 90

#ifdef THREADING_ON

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "StringUtil.h"
#include "legacy_shim.h"
#include "log.h"
#include "mailbox.h"
#include "mydefs.h"
#include "portable-compat.h"

#include "Globals.h"
#include "StringDefs.h"
#include "fileutil.h"
#include "message.h" /* IsQueued */
#include "sendmail.h"
#include "threading.h"
#include "toc.h" /* WriteTOC, SetState */

#define THREAD_STACK_SIZE 0 // Let pthread use default stack size

// Mac OS cursor stubs
#define watchCursor 0
#define SetMyCursor(c)                                                         \
  do {                                                                         \
  } while (0)
#ifndef MINI_MASK
#define MINI_MASK 0
#endif

// External declarations for missing headers
extern bool IsQueued(TOCHandle tocH, short sumNum);
extern long ApproxMessageSize(MessHandle messH);
extern void SetState(TOCHandle tocH, short sumNum, short state);
extern void WriteTOC(TOCHandle tocH);

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
extern bool DeleteSum(TOCHandle tocH, int sumNum);

#define threadNotFoundErr (-32767)

#define kNoThreadID ((pthread_t)0)

/*
        private data structures
*/

typedef struct prefChange_ {
  short pref;
  uLong persId;
} prefChangeRec, *prefChangePtr;

int gNumBackgroundThreads = 0;
static pthread_t gMainThreadID = 0;
static short gCriticalSection = 0;
static pthread_mutex_t gCriticalMutex = PTHREAD_MUTEX_INITIALIZER;

// Thread-local storage for globals
threadGlobalsRec ThreadGlobals;
threadGlobalsPtr CurThreadGlobals = &ThreadGlobals;

/*
         private functions
*/
static OSErr CopySettingsForThread(short sourceRefN, PersHandle sourcePerslist,
                                   short *destRefN, PersHandle *destPersList,
                                   PersHandle *destCurPers);
static OSErr DeleteSettingsForThread(short *settingsRefN);
static OSErr SaveSettingsToMainThread(threadDataHandle threadData);
static OSErr InitThreadGlobals(threadGlobalsPtr *newThreadGlobals);
static OSErr DisposeThreadGlobals(threadGlobalsPtr threadGlobals);

int GetFCCs(MessHandle messH, CSpecHandle fccSpecs); // move to sendmail.c?

#ifdef IMAP
static OSErr NewXferMail(threadDataHandle *tData, bool check, bool send,
                         bool manual, bool ae, XferFlags flags,
                         IMAPTransferPtr imapInfo);
#else
static OSErr NewXferMail(threadDataHandle *tData, bool check, bool send,
                         bool manual, bool ae, XferFlags flags);
#endif
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

#ifdef DEBUG_THREAD
static void MyDebuggerNewThread(pthread_t threadCreated);
static void MyDebuggerDisposeThread(pthread_t threadDeleted);

/* This guy isn't getting called!!!! */

static pascal void MyDebuggerNewThread(ThreadID threadCreated) {
  DebugStr("\pThread started");
}

/* This guy isn't getting called!!!! */

static pascal void MyDebuggerDisposeThread(ThreadID threadDeleted) {
  DebugStr("\pThread ended");
}

static pascal ThreadID
MyDebuggerThreadScheduler(SchedulerInfoRecPtr schedulerInfo) {
  Str255 text;

  ComposeString(text, "\pContext Switch -- tickCount:%d curThread:%d",
                TickCount(), (int)schedulerInfo->CurrentThreadID);
  DebugStr(text);

  return (kNoThreadID);
}
#endif

/************************************************************************
 * GetThreadData -
 ************************************************************************/
void GetThreadData(ThreadID threadID, threadDataHandle *threadData) {
  threadDataHandle index;

  *threadData = nil;
  for (index = gThreadData; index; index = (*index)->next)
    if (pthread_equal((*index)->threadID, threadID)) {
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
  return (threadData ? (*threadData)->prbl : nil);
}

#pragma segment newthreads

/*
 put functions that create threads in a separate segment:
         they can safely be unloaded from the main event loop (provided they
 don't Yield to other threads) they should be in a separate segment from
 referenced procptrs (to ensure that procptrs are A5-relative)
*/

/************************************************************************
 * InitThreadGlobals - create thread and allocate data for it
 ************************************************************************/
static OSErr InitThreadGlobals(threadGlobalsPtr *newThreadGlobals) {
  if (!(*newThreadGlobals =
            (threadGlobalsPtr)calloc(1, sizeof(**newThreadGlobals))))
    return -108;
  (*newThreadGlobals)->tSettingsRefN = -1;
  (*newThreadGlobals)->tResRefN = -1;
  (*newThreadGlobals)->tCurTrans = CurTrans;
  return (noErr);
}

/************************************************************************
 * CopyOutToTemp - copy queued messages in Out mailbox to temporary Out mailbox
 ************************************************************************/
static OSErr CopyOutToTemp(void) {
  int ii;
  TOCHandle tocH, tempTocH;
  StateEnum state;
  MessHandle messH;
  OSErr err = noErr;
  uLong gmtSecs = GMTDateTime();
  PersHandle pers;

  MyThreadBeginCritical();
  SetMyCursor(watchCursor);
  tocH = GetRealOutTOC();
  tempTocH = GetTempOutTOC();

  TotalQueuedSize = 0;
  if (tocH && tempTocH) {
    // only copy messages ready to be sent now
    for (ii = 0; ii < (*tocH)->count; ii++) {
      if (!(*tocH)->sums[ii].messH && IsQueued(tocH, ii)) {
        pers = FindPersById((*tocH)->sums[ii].persId);
        ASSERT(pers);
        if (pers && (*pers)->sendMeNow &&
            (*tocH)->sums[ii].seconds <= gmtSecs) {
          /*
           * handle open, dirty windows
           */
          if ((messH = SaveB4Send(tocH, ii)) != NULL) {
            MiniEvents(); // in case we're hogging time
            TotalQueuedSize += (ApproxMessageSize(messH) * 1024);
            state = (*tocH)->sums[ii].state;
            err = MoveMessageLo(tocH, ii, &(*tempTocH)->mailbox.spec, true,
                                true, true);
            if (err)
              break;
            SetState(tocH, ii, BUSY_SENDING);
            SetState(tempTocH, (*tempTocH)->count - 1, state);
            if ((*messH)->win)
              CloseMyWindow(GetMyWindowWindowPtr((*messH)->win));
          }
        }
      }
    }
    BoxFClose(tempTocH, true);
    WriteTOC(tempTocH);
  }
  MyThreadEndCritical();
  return err;
}

/************************************************************************
 * NewXferMail - create thread and allocate data for it
 ************************************************************************/
#ifdef IMAP
static OSErr NewXferMail(threadDataHandle *tData, bool check, bool send,
                         bool manual, bool ae, XferFlags flags,
                         IMAPTransferPtr imapInfo)
#else
static OSErr NewXferMail(threadDataHandle *tData, bool check, bool send,
                         bool manual, bool ae, XferFlags flags)
#endif
{
  threadDataHandle threadData = nil;
  threadContextDataPtr threadContext = nil;
  ThreadID threadID;
  OSErr theError = noErr;
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
  threadData = (threadDataHandle)calloc(1, sizeof(threadDataPtr));
  if (threadData)
    *threadData = (threadDataPtr)calloc(1, sizeof(**threadData));
  if (!threadData || !*threadData)
    theError = -108;
  if (threadData)
    (*threadData)->next = gThreadData;
  gThreadData = threadData;
  if (!theError)
    theError = InitThreadGlobals(&newThreadGlobals);
  if (!theError) {
    threadContext = &(*threadData)->threadContext;
    // pthreads don't need A5 world
    threadContext->newThreadGlobals = newThreadGlobals;

    theError = StackInit(sizeof(prefChangeRec), &threadContext->prefStack);
  }
  if (!theError) {
#ifdef TASK_PROGRESS_ON
    if (!((*threadData)->prbl = NewZH(ProgressBlock)))
      theError = MemError();
#endif
    (*threadData)->xferMailParams.send = send;
    (*threadData)->xferMailParams.check = check;
    (*threadData)->xferMailParams.manual = manual;
    (*threadData)->xferMailParams.ae = ae;
    BlockMoveData(&flags, &(*threadData)->xferMailParams.flags,
                  sizeof((*threadData)->xferMailParams.flags));
#ifdef IMAP
    (*threadData)->imapInfo.command = UndefinedTask;
    if (imapInfo)
      BlockMoveData(imapInfo, &((*threadData)->imapInfo),
                    sizeof(IMAPTransferRec));
#endif
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
    (*threadData)->threadID = threadID;
#if __profile__
  if (!theError) {
    Size stackSize;
    ProfilerThreadRef threadRef;

    GetDefaultThreadStackSize(kCooperativeThread, &stackSize);
    theError = ProfilerCreateThread(100, stackSize, &threadRef);
    (*threadData)->threadRef = threadRef;
    ASSERT(!theError);
  }
#endif
  // pthreads start immediately - no SetThreadState needed

  if (!theError)
    (*threadData)->threadID = threadID;
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
      ASSERT((*threadData)->prbl);
      DisposProgress((*threadData)->prbl);
#endif
      LL_Remove(gThreadData, threadData, (threadDataHandle));
      DisposeThreadGlobals(newThreadGlobals);
      if (threadData) {
        free(*threadData);
        free(threadData);
      }
      threadData = nil;
    }
    if (!manual)
      ResetCheckTime(True);
    CheckOnIdle = false;
  }
  *tData = threadData;
  return (CommandPeriod ? userCanceledErr : theError);
}

/************************************************************************
 * SetupXferMailThread - create thread and allocate data for it
 ************************************************************************/
#ifdef IMAP
OSErr SetupXferMailThread(bool check, bool send, bool manual, bool ae,
                          XferFlags flags, IMAPTransferPtr imapInfo)
#else
OSErr SetupXferMailThread(bool check, bool send, bool manual, bool ae,
                          XferFlags flags)
#endif
{
  threadDataHandle sendData = nil, checkData = nil;
  OSErr err = noErr;

  // we tried to send it
  if (send)
    SendImmediately = False;
  if (PrefIsSet(PREF_THREADING_SEND_OFF) || PrefIsSet(PREF_POP_SEND) ||
      !(check && send)) {
#ifdef IMAP
    err = NewXferMail(&checkData, check, send, manual, ae, flags, imapInfo);
#else
    err = NewXferMail(&checkData, check, send, manual, ae, flags);
#endif
    sendData = checkData;
  } else {
#ifdef IMAP
    if (!(err = NewXferMail(&checkData, check, false, manual, ae, flags,
                            imapInfo))) {
      if (NewXferMail(&sendData, false, send, manual, ae, flags, imapInfo))
#else
    if (!(err = NewXferMail(&checkData, check, false, manual, ae, flags))) {
      if (NewXferMail(&sendData, false, send, manual, ae, flags))
#endif
      {
        if (!SendThreadRunning) {
          ASSERT(checkData);
          if (checkData) {
            (*checkData)->xferMailParams.send = send;
            sendData = checkData;
            SendThreadRunning = send;
          }
        }
      }
    }
  }
#ifdef TASK_PROGRESS_ON
#ifndef BATCH_DELIVERY_ON
  if (!err && check && checkData && TaskProgressWindow && NeedToFilterIn)
    RemoveFilterTask();
#endif
#endif
  if (err != userCanceledErr) {
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

#pragma segment Threads

/************************************************************************
 * DisposeThreadGlobals -
 ************************************************************************/
static OSErr DisposeThreadGlobals(threadGlobalsPtr threadGlobals) {
  if (!threadGlobals)
    return paramErr;

  CurThreadGlobals = threadGlobals;
  // kill string cache
  if (StringCache) {
    free(*StringCache);
    free(StringCache);
    StringCache = nil;
  }

  /*	Kill personalities	*/
  DisposePersonalities();
  PersList = nil;
  CurPers = nil;
  if (PersStack) {
    free(*PersStack);
    free(PersStack);
    PersStack = nil;
  }

  // MIME Maps
  if (MMIn) {
    free(*MMIn);
    free(MMIn);
    MMIn = nil;
  }
  if (MMOut) {
    free(*MMOut);
    free(MMOut);
    MMOut = nil;
  }

  if (GWStack) {
    free(*GWStack);
    free(GWStack);
    GWStack = nil;
  }
  if (threadGlobals)
    free(threadGlobals);
  threadGlobals = nil;
  CurThreadGlobals = &ThreadGlobals;
  return (noErr);
}

/************************************************************************
 * CleanTempOutTOC - now that we update mesg status immediately and
 *									user can
 *move these unlocked messages, we can just clear temp out box
 ************************************************************************/
void CleanTempOutTOC(void) {
  TOCHandle tempTocH;
  short ii;
  uLong oldForceSend = ForceSend;

  tempTocH = GetTempOutTOC();
  ASSERT(tempTocH);
  // maybe there's a quicker way???
  if (tempTocH)
    for (ii = 0; ii < (*tempTocH)->count; ii++)
      if (!DeleteSum(tempTocH, ii))
        ii--;
  ForceSend = oldForceSend;
}

/************************************************************************
 * CleanRealOutTOC -
 ************************************************************************/
void CleanRealOutTOC(void) {
  TOCHandle tocH = GetRealOutTOC(), tempTocH = GetTempOutTOC();
  int ii;
  short sum;

  if (!(tocH && tempTocH))
    return;

  for (ii = (*tocH)->count - 1; ii >= 0; ii--) {
    if ((*tocH)->sums[ii].state == BUSY_SENDING) {
      sum = FindSumByHash(tempTocH, (*tocH)->sums[ii].uidHash);
      SetState(tocH, ii, (sum != -1) ? (*tempTocH)->sums[sum].state : SENDABLE);
    }
  }
}

/************************************************************************
 * GetCurrentTaskKind -
 ************************************************************************/
TaskKindEnum GetCurrentTaskKind(void) {
  threadDataHandle threadData = nil;

  GetCurrentThreadData(&threadData);
  return (threadData ? (*threadData)->currentTask : UndefinedTask);
}

/************************************************************************
 * SetCurrentTaskKind -
 ************************************************************************/
void SetCurrentTaskKind(TaskKindEnum taskKind) {
  threadDataHandle threadData = nil;

  GetCurrentThreadData(&threadData);
  if (threadData)
    (*threadData)->currentTask = taskKind;
}

/************************************************************************
 * killThreads - cancel each thread and wait until they're all dead
 ************************************************************************/
void KillThreads(void) {
  threadDataHandle threadDataIndex;
  // Set cancel flag for each thread
  for (threadDataIndex = gThreadData; threadDataIndex;
       threadDataIndex = (*threadDataIndex)->next)
    SetThreadGlobalCommandPeriod((*threadDataIndex)->threadID, true);

  // Wait for threads to finish
  while (GetNumBackgroundThreads())
    sched_yield();
}

/************************************************************************
 * XferMailThread - open threaded progress, xfermail, save
 * and delete settings
 ************************************************************************/
void *XferMailThread(void *threadParameter) {
  threadDataHandle threadData = nil;
  OSErr theError = noErr;
  xferMailParamsRec xferMailParams;
#ifdef IMAP
  IMAPTransferRec imapInfo;
#endif

  ASSERT(threadParameter);
  if (!threadParameter)
    return nil;

  threadData = (threadDataHandle)threadParameter;
#ifdef DEBUG
  (*threadData)->startTime = TickCount();
#endif
  BlockMoveData(&(*threadData)->xferMailParams, &xferMailParams,
                sizeof(xferMailParams));
#ifdef IMAP
  BlockMoveData(&(*threadData)->imapInfo, &imapInfo, sizeof(IMAPTransferRec));
#endif
#ifdef DEBUG_THREAD
  MyDebuggerNewThread((*threadData)->threadContext.threadID);
#endif
  if (!theError) {
#ifndef BATCH_DELIVERY_ON
    if (xferMailParams.manual) {
      short idleCount = GetRLong(MODAL_IDLE_SECS) * 60;
      if (!NeedToFilterIn && !NeedToFilterOut)
        ActiveTicks = (ActiveTicks > idleCount) ? (ActiveTicks - idleCount) : 0;
    }
#endif
#ifdef IMAP
    theError = XferMailRun(xferMailParams.check, xferMailParams.send,
                           xferMailParams.manual, xferMailParams.ae,
                           xferMailParams.flags, &imapInfo);
#else
    theError = XferMailRun(xferMailParams.check, xferMailParams.send,
                           xferMailParams.manual, xferMailParams.ae,
                           xferMailParams.flags);
#endif
    CloseProgress();
    SaveSettingsToMainThread(threadData);
    DeleteSettingsForThread(&SettingsRefN);
  }
  if (xferMailParams.send) {
    if (CommandPeriod || theError)
      SendImmediately = False;
    CleanRealOutTOC();
    CleanTempOutTOC();
    SetSendQueue();
  }
  return nil;
}

/************************************************************************
 * threadSwitchProcIn - switch intra-thread globals in
 ************************************************************************/
void ThreadSwitchProcIn(pthread_t threadBeingSwitched, void *switchProcParam) {
#pragma unused(threadBeingSwitched)
  threadDataHandle threadData;
  threadContextDataPtr threadContext;

  ASSERT(switchProcParam);

  threadData = (threadDataHandle)switchProcParam;
  threadContext = &(*threadData)->threadContext;

  // pthreads don't need A5 world switching

  // restore thread-globals
  CurThreadGlobals = threadContext->newThreadGlobals;
#ifdef TASK_PROGRESS_ON
  ProgWindow = TaskProgressWindow;
#endif
  // make sure curresfile is thread's SettingsRefN
  if (SettingsRefN != -1)
    UseResFile(SettingsRefN);

#ifdef TASK_PROGRESS_ON
#ifdef NEVER
  if (TaskProgressWindow) {
    ThreadID threadID, oldThreadID;

    /* we're actually still in the main thread right now. so before we update
       the task progress beat, set this thread's id to the main thread's id */
    InvalTaskProgressBeat((*threadData)->prbl);
    oldThreadID = (*threadData)->threadID;
    GetCurrentThread(&threadID);
    (*threadData)->threadID = threadID;
    UpdateMyWindow(GetMyWindowWindowPtr(TaskProgressWindow));
    (*threadData)->threadID = oldThreadID;
  }
#endif
#endif

#if 0
  myA5 = SetA5(currentA5);
#endif
#if __profile__
  // ProfilerSwitchToThread((*threadData)->threadRef);
  // ProfilerSetStatus(profilerWas);
#endif
#ifdef DEBUG
  (*threadData)->switchInTime = TickCount();
  (*threadData)->switchCount++;
#endif
}

/************************************************************************
 * threadSwitchProcOut - switch intra-thread globals out
 * 												switch
 *main-thread globals in
 ************************************************************************/
void ThreadSwitchProcOut(pthread_t threadBeingSwitched, void *switchProcParam) {
#pragma unused(threadBeingSwitched)
  threadDataHandle threadData = nil;
  threadContextDataPtr threadContext = nil;

  ASSERT(switchProcParam);
  threadData = (threadDataHandle)switchProcParam;
  threadContext = &(*threadData)->threadContext;

  // pthreads don't need A5 world switching

  // save globals from this thread
  threadContext->newThreadGlobals = CurThreadGlobals;
  CurThreadGlobals->tResRefN = CurResFile();

  // restore main thread-globals
  CurThreadGlobals = &ThreadGlobals;
  UseResFile(SettingsRefN);

#ifdef TASK_PROGRESS_ON
#ifdef NEVER
  if (TaskProgressWindow) {
    InvalTaskProgressBeat((*threadData)->prbl);
    UpdateMyWindow(GetMyWindowWindowPtr(TaskProgressWindow));
  }
#endif
#endif

#if 0
  myA5 = SetA5(currentA5);
#endif
#if __profile__
  // ProfilerSwitchToThread(ProfilerGetMainThreadRef());
  // ProfilerSetStatus(profilerWas);
#endif
#ifdef DEBUG
  (*threadData)->totalTimeThread += (TickCount() - (*threadData)->switchInTime);
#endif
}

/************************************************************************
 * SaveSettingsToMainThread - copy changed settings back to main
 ************************************************************************/
// popd resource is saved directly to the original settings file in DisposePOPD
static OSErr SaveSettingsToMainThread(threadDataHandle threadData) {
  OSErr theError = noErr;
  PersHandle pers, mainPers, oldCurPers;
  StackHandle prefStack;
  char string[256]; // prefs shouldn't exceed 255 chars!!!
  prefChangeRec prefChange;

  ASSERT(threadData);

  // prefs that were changed by thread should be changed
  prefStack = (*threadData)->threadContext.prefStack;
  (*threadData)->threadContext.prefStack = nil;
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
    free(*prefStack);
    free(prefStack);
  }

  // go through all personalities in thread and update associated data in main
  // thread
  for (pers = PersList; pers; pers = (*pers)->next) {
    // change context to main thread
    ThreadSwitchProcOut(nil, threadData);
    if ((mainPers = FindPersById((*pers)->persId))) {
//			(*mainPers)->sendQueue = (*pers)->sendQueue;
#ifdef DEBUG
      if (!BUG6)
#endif
        (*mainPers)->checkTicks =
            MAX((*mainPers)->checkTicks, (*pers)->checkTicks);
      (*mainPers)->popSecure = (*pers)->popSecure;
      if ((*pers)->dirty)
        (*mainPers)->dirty = 1;
      (*mainPers)->noUIDL = (*pers)->noUIDL;

#ifdef IMAP
      /* only copy passwords back if we're a check thread or a send thread using
       * xtnd xmit */
      /* or if this was an IMAP thread, and the passwords were invalidated */
      if (((*threadData)->xferMailParams.check ||
           ((*threadData)->xferMailParams.send &&
            (PrefIsSet(PREF_POP_SEND) ||
             (ShouldSMTPAuth() && !PrefIsSet(PREF_SMTP_AUTH_NOTOK))))) ||
          (((*threadData)->currentTask > SendingTask) &&
           ((*pers)->password[0] == 0) && ((*pers)->secondPass[0] == 0)))
#else
      /* only copy passwords back if we're a check thread or a send thread using
       * xtnd xmit */
      if ((*threadData)->xferMailParams.check ||
          ((*threadData)->xferMailParams.send && PrefIsSet(PREF_POP_SEND)))
#endif
      {
        strncpy((*mainPers)->password, (*pers)->password, sizeof((*mainPers)->password) - 1);
        (*mainPers)->password[sizeof((*mainPers)->password) - 1] = '\0';
        strncpy((*mainPers)->secondPass, (*pers)->secondPass, sizeof((*mainPers)->secondPass) - 1);
        (*mainPers)->secondPass[sizeof((*mainPers)->secondPass) - 1] = '\0';
      }
    }
    ThreadSwitchProcIn(nil, threadData);
  }

  // Fix Prr in the main thread, mostly for benefit of broken NotifyNewMail
  {
    OSErr myPrr = Prr;

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
  return theError;
}

/************************************************************************
 * CopySettingsForThread - copy settings for thread
 ************************************************************************/
static OSErr CopySettingsForThread(short sourceRefN, PersHandle sourcePerslist,
                                   short *destRefN, PersHandle *destPersList,
                                   PersHandle *destCurPers) {
  FSSpec tempSpec, settingsSpec;
  OSErr theError = noErr;
  short oldRefN = CurResFile();

  Zero(tempSpec);

  MyThreadBeginCritical();
  SetMyCursor(watchCursor);
  UpdateResFile(sourceRefN);
  theError = GetFileByRef(sourceRefN, &settingsSpec);
  if (!theError) {
    theError = NewTempSpec(Root.vRef, Root.dirId, nil, &tempSpec);
    if (!theError) {
      theError = FSpDupFile(&tempSpec, &settingsSpec, True, False);
      if (!theError) {
        *destRefN = FSpOpenResFile(&tempSpec, fsRdWrPerm); // change to RdWr
        if (*destRefN == -1)
          theError = ResError();

        // What about the cache? for now. we'll just leave it initialized to nil
        // instead of copying it.

        if (!theError) {
          // copy personalities
          PersHandle oldPers, clone, lastClone = nil;

          for (oldPers = sourcePerslist; oldPers && !theError;
               oldPers = (*oldPers)->next) {
            clone = oldPers;
            if (!(theError = 0)) {
              (*clone)->next = nil;
              if (lastClone)
                (*lastClone)->next = clone;
              else
                *destPersList = clone;
              lastClone = clone;
            } else
              FileSystemError(READ_SETTINGS, tempSpec.name, theError);
            *destCurPers = *destPersList;
          }
        } else
          FileSystemError(READ_SETTINGS, tempSpec.name, theError);
      } else
        FileSystemError(READ_SETTINGS, tempSpec.name, theError);
    } else
      FileSystemError(READ_SETTINGS, tempSpec.name, theError);
  } else
    FileSystemError(READ_SETTINGS, tempSpec.name, theError);

  MyThreadEndCritical();
  if (theError) {
    DeleteSettingsForThread(destRefN);
  }
  UseResFile(oldRefN);
  return (CommandPeriod ? userCanceledErr : theError);
}

/************************************************************************
 * DeleteSettingsForThread - do it
 ************************************************************************/
static OSErr DeleteSettingsForThread(short *settingsRefN) {
  FSSpec tempSpec;
  OSErr theError = noErr;

  if (*settingsRefN == -1)
    return noErr;

  // close and delete temp settings file
  theError = GetFileByRef(*settingsRefN, &tempSpec);
  if (!theError) {
    CloseResFile(*settingsRefN);
    *settingsRefN = -1;
    theError = ResError();
  }
  if (!theError)
    theError = FSpDelete(&tempSpec);

  ASSERT(!theError); // temp file should get deleted!!!
  return theError;
}

/************************************************************************
 * getNumBackgroundThreads - return number of background threads
 ************************************************************************/
#pragma segment Main
int GetNumBackgroundThreads(void) { return (int)(gNumBackgroundThreads); }
#pragma segment Threads

/************************************************************************
 * PushThreadPrefChange -
 ************************************************************************/
OSErr PushThreadPrefChange(short pref) {
  OSErr err = threadNotFoundErr;
  threadDataHandle threadData;

  GetCurrentThreadData(&threadData);

  if (threadData) {
    prefChangeRec prefChange;
    StackHandle prefStack = (*threadData)->threadContext.prefStack;

    if (!prefStack)
      return noErr;
    prefChange.pref = pref;

    prefChange.persId = (CurThreadGlobals && CurPers) ? (*CurPers)->persId : 0;
    err = noErr;
    StackPush(&prefChange, prefStack);
  }
  return err;
}

/************************************************************************
 * incrementNumBackgroundThreads - increase number of background threads
 ************************************************************************/
void IncrementNumBackgroundThreads(void) { gNumBackgroundThreads++; }

/************************************************************************
 * decrementNumBackgroundThreads - decrease number of background threads
 ************************************************************************/
void DecrementNumBackgroundThreads(void) { gNumBackgroundThreads--; }

/************************************************************************
 * threadsAvailable - are threads supported?
 ************************************************************************/
bool ThreadsAvailable(void) {
  // pthreads always available on POSIX systems
  return true;
}

/* folowing three functions primarily used in pop.c cause we want to write to
the settings file belonging to the main thread, not the background thread */

/************************************************************************
 * GetResourceMainThread -
 ************************************************************************/
Handle GetResourceMainThread(ResType theType, short theID) {
  short refN = SettingsRefN, curRefN;
  Handle res = nil;

  SettingsRefN = GetMainGlobalSettingsRefN();
  curRefN = CurResFile();
  UseResFile(SettingsRefN);
  res = GetResource_(theType, theID);
  SettingsRefN = refN;
  UseResFile(curRefN);
  return (res);
}

/************************************************************************
 * ZapSettingsResourceMainThread -
 ************************************************************************/
OSErr ZapSettingsResourceMainThread(OSType type, short id) {
  OSErr theError = noErr;
  short refN = SettingsRefN;

  SettingsRefN = GetMainGlobalSettingsRefN();
  theError = ZapSettingsResource(type, id);
  SettingsRefN = refN;
  return (theError);
}

/************************************************************************
 * AddMyResourceMainThread -
 ************************************************************************/
OSErr AddMyResourceMainThread(Handle h, OSType type, short id,
                              ConstStr255Param name) {
  short refN = SettingsRefN, curRefN;
  OSErr err = noErr;

  SettingsRefN = GetMainGlobalSettingsRefN();
  curRefN = CurResFile();
  AddMyResource(h, type, id, name);
  err = ResError();
  SettingsRefN = refN;
  UseResFile(curRefN);
  return err;
}

/************************************************************************
 * SetThreadGlobalCommandPeriod -
 ************************************************************************/
void SetThreadGlobalCommandPeriod(ThreadID threadID, bool value) {
  threadGlobalsPtr newGlobals = nil;
  threadDataHandle threadData = nil;

  GetThreadData(threadID, &threadData);

  if (threadData &&
      (newGlobals = (*threadData)->threadContext.newThreadGlobals))
    newGlobals->tCommandPeriod = value;
}

/************************************************************************
 * GetMainGlobalSettingsRefN -
 ************************************************************************/
short GetMainGlobalSettingsRefN(void) { return (ThreadGlobals.tSettingsRefN); }

/************************************************************************
 * MyBeginCritical -
 ************************************************************************/
void MyThreadBeginCritical(void) { ++gCriticalSection; }

/************************************************************************
 * MyEndCritical -
 ************************************************************************/
void MyThreadEndCritical(void) {
  if (--gCriticalSection < 0)
    gCriticalSection = 0;
}

/************************************************************************
 * MyYieldToAnyThread -
 ************************************************************************/
void MyYieldToAnyThread(void) {
#ifdef DEBUG
  CheckSelectedGlobals();
#endif
  if (ThreadsAvailable()) {
    if (!gCriticalSection) {
      short curRes = CurResFile(); //	Thread might change res file

      sched_yield(); // POSIX sched_yield
      ThreadYieldTicks = TickCount();
      UseResFile(curRes);
    } // WNELo stubbed out for POSIX
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
OSErr MyInitThreads(void) {
#ifdef DEBUG_THREAD
  SetDebuggerNotificationProcs(MyDebuggerNewThread, MyDebuggerDisposeThread,
                               MyDebuggerThreadScheduler);
#endif
  gMainThreadID = pthread_self();
  return noErr;
}

/************************************************************************
 * inAThread - am I running from a thread?
 ************************************************************************/
bool InAThread(void) {
  ThreadID threadID;

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
    WindowPtr TaskProgressWindowWP;
#ifdef DEBUG
    long totalTicks = 0;
    long perc = 0;
#endif
    if ((*threadData)->xferMailParams.check)
      CheckThreadRunning = false;
    if ((*threadData)->xferMailParams.send)
      SendThreadRunning = false;
#ifdef DEBUG_THREAD
    MyDebuggerDisposeThread((*threadData)->threadContext.threadID);
#endif
#if __profile__
    ProfilerDeleteThread((*threadData)->threadRef);
#endif
    DisposeThreadGlobals((*threadData)->threadContext.newThreadGlobals);
    if ((*threadData)->threadContext.prefStack) { // should be set to nil in
                                                  // SaveSettingsToMainThread
      free(*((*threadData)->threadContext.prefStack));
      free((*threadData)->threadContext.prefStack);
    }
    ThreadSwitchProcOut(threadTerminated, terminationProcParam);
#ifdef DEBUG
    // log totalTimeAll, totalTimeThread, switchCount
    totalTicks = TickCount() - (*threadData)->startTime;
    perc = (long)(((float)(*threadData)->totalTimeThread / (float)totalTicks) *
                  100);
    fprintf(stderr,
        "Total ticks %ld; %s%s Thread ticks %ld; Percent %ld; Switch count: %d; "
        "Ticks/Switch: %ld\n",
        totalTicks,
        (*threadData)->xferMailParams.check ? "Check" : "",
        (*threadData)->xferMailParams.send ? " Send" : "",
        (*threadData)->totalTimeThread, perc, (*threadData)->switchCount,
        (*threadData)->switchCount
            ? (*threadData)->totalTimeThread / (*threadData)->switchCount
            : 0L);
#endif
#ifdef TASK_PROGRESS_ON
    ASSERT((*threadData)->prbl);
    DisposProgress((*threadData)->prbl);
#endif
    LL_Remove(gThreadData, threadData, (threadDataHandle));
    if (threadData) {
      free(*threadData);
      free(threadData);
    }
    threadData = nil;
    TaskProgressWindowWP = GetMyWindowWindowPtr(TaskProgressWindow);
#ifdef TASK_PROGRESS_ON
    if (TaskProgressWindow &&
        GetWindowKind(TaskProgressWindowWP) == TASKS_WIN) {
#ifdef BATCH_DELIVERY_ON
      if (!GetNumBackgroundThreads() && !TaskDontAutoClose &&
          PrefIsSet(PREF_TASK_PROGRESS_AUTO) && !NewError) {
        CloseMyWindow(TaskProgressWindowWP);
      } else {
        InvalContent(TaskProgressWindow);
      }
#else
      if (!GetNumBackgroundThreads() && !TaskDontAutoClose &&
          PrefIsSet(PREF_TASK_PROGRESS_AUTO) && !NeedToFilterIn && !NewError) {
        CloseMyWindow(TaskProgressWindowWP);
      } else {
        InvalContent(TaskProgressWindow);
      }
#endif
    }
#endif
  }
}

#endif

/* gWazooListHead is defined in wazoo.c and exported via wazoo.h */
#include "wazoo.h"

#ifdef CommandPeriod
#undef CommandPeriod
#endif
extern bool CommandPeriod;
#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif


/*	Another cheap knockoff - this one from linkmng.c */
typedef struct privHistoryDStruct {
  FSSpec spec;    /* the history file */
  Handle theData; /* the toc */
  bool ro;        /* read only */
  bool dirty;     /* is the history file dirty? */
} privHistoryDesc, *privHistoryDPtr, **privHistoryDHandle;

/* gHistories is defined in linkmng.c */
extern privHistoryDHandle gHistories;

void CheckSelectedGlobals() {
  /* Walk the wazoo list — plain linked list, no Mac Handle validation needed */
  for (WazooData *w = gWazooListHead; w != NULL; w = w->next) {
    ASSERT(w->win != NULL);
  }
  /* gHistories validation is deferred until linkmng.c is ported */
}

/* RemoveTaskErrors is implemented in taskProgress.c */

/*
-----------------------------------------------------------------------------------------------------------------------
                                                                                                                                                                                                                        DESIGN
-----------------------------------------------------------------------------------------------------------------------

Here's my plan for implementing phase 1 of threads. Comments are welcome.

I am using the Thread Manager to implement this stuff. All of the thread changes
are #ifdefed so we can switch off threading for debugging. The Checkmail thread
is mostly written.

Here's a breakdown of the changes:

General Threading

Can we thread?
        - during initialization, see if this OS supports threads. Warn and run
single-threaded if we can't.

Make Progress Dialog thread-safe and modeless
        - don't disable menus anymore (but do disable checkmail and sendmail)
        - attach thread context data structure to the myWindowPtr structure. The
thread context data structure contains two copies of certain global variables.
One copy is global to the main thread, and the other is global to the background
thread. Examples of some of these thread-sensitive variables are: ProgWindow,
CommandPeriod, and StringCache. These globals are switched in and out by the
thread context switch procs, which are automatically called by the Thread
Manager.
        - Replace Swinging Pendulum and Spinning Beach Ball with Arrow cursor
        - Yield to other threads from Progress() function
        - replace progreessButton () and progressKey () callbacks with
thread-aware versions

Make network code thread-safe
        I'm not sure how much work is involved here or if it's even necessary
for phase 1. For now I've just disabled the Ph and Finger buttons when a
checkmail or sendmail thread is in progress.

Cooperation
        CHANGED -- Yield once from main event loop, which is in the main thread.
Background threads will also yield from the Progress() function. We need to be
extra cautious about "locking up" the main thread by not yielding enough from
background threads. We're now yielding from WNE. The tcp code calls this when
it's waiting to connect. Previously, not yielding from this code locked the UI.
        Another big change is, we're not sleeping when a background thread is
running. We were giving up too much time to other apps when the thread was
running.

Segmentation
        For now, I've disabled unloadSegs () and unloadUneeded () while a thread
is running. Unloading segments that are in use by another thread is not a
healthy thing to do! It sounds like we will be doing away with segmentation so
these functions will not need to be made thread-safe?


Check Mail Thread
-----------------
from main thread...
        - create thread (allocate and initialize data and install procptrs:
thread entry point, context switch functions, thread termination function)
        - continue cycling through event loop (and call thread-aware progress
callbacks when cancel button hit or command-. hit)

from check mail thread...
        - copy settings, stringcache, and personality data (since user could
change these while the thread is running)
        - open thread-safe, modeless progress dialog
        - run modified xfermail()
                download messages to temporary In mailbox
                make progress modal
                run filters on temp mailbox
                transfer unfiltered messages to In mailbox
        - close progress dialog
        - clean up (dispose of memory)

Send Mail Thread 1 (implemented)
------------------
from main thread...
        - create thread (allocate and initialize data and install procptrs:
thread entry point, context switch functions, thread termination function)
        - copy queued messages in Out mailbox to temporary Out mailbox
        - set state of messages in out box to BUSY_SENDING (they're locked and
unmodifiable)
        - continue cycling through event loop (and call thread-aware progress
callbacks when cancel button hit or command-. hit)

from send mail thread...
        - copy settings, stringcache, and personality data (since user could
change these while the thread is running)
        - open thread-safe, modeless progress dialog
        - run modified xfermail()
                run emsapi plug-ins (shouldn't be a problem unless plug-in calls
YieldToAnyThread ()) expand nicknames parse headers (to, from, fcc...) upload
messages from the temporary Out mailbox mark state of summary (i.e. SENT)

        make progress modal
        - for each message in out box marked as BUSY_SENDING
                set state of summary (i.e. SENT or QUEUED)
                expand nicknames
                fcc
                filter
                delete summary if pref set
                delete summary in temp out box
        - close progress dialog
        - clean up (dispose of memory)


Send Mail Thread 2 (future implementation)
------------------
from main thread...
        - create thread (allocate and initialize data and install procptrs:
thread entry point, context switch functions, thread termination function)
        + spool queued messages in out box (expand nicknames, encode
attachments, run EMSAPI q4transmission plug-ins) with name = uidHash
        - set state of messages in out box to BUSY_SENDING (they're locked and
unmodifiable)
        - continue cycling through event loop (and call thread-aware progress
callbacks when cancel button hit or command-. hit)

from send mail thread...
        - copy settings, stringcache, and personality data (since user could
change these while the thread is running)
        - open thread-safe, modeless progress dialog
        - run modified xfermail()
        +	upload messages from spool directory

                make progress modal
        - for each locked summary entry
                set state of summary (i.e. SENT or QUEUED)
                fcc
                filter
                delete summary if pref set
                delete spool file
        - close progress dialog
        - clean up (dispose of memory)

-----------------------------------------------------------------------------------------------------------------------
                                                                                                                                                                                                                                BUGS
-----------------------------------------------------------------------------------------------------------------------

        To do:

        - don't bring up filter progress window unless process takes more than 3
seconds
        - allow filtering during thread
        - test for weak imports in threads available
        - send queued messages after send thread finishes (watch for ppp hanging
up between xfers)
        - thread should have own toclist and messlist

        Known bugs:

        - unloadUneeded () and unloadSegs() disabled when thread started
        - changing pop accounts while checking mail - pw should become invalid
        - some sent messages are still editable

        Non-reproducible bugs:
        - editing � (busy sending) messages (chris pepper)
        - can't edit contents of mess from aborted send
        - possible race condition when sending

        Fixed bugs:

        - background check mail reporting error -38 when downloading mail
3/19/97
        - messages aren't written to temp toc
3/20/97
        - check mail is checking all personalities
3/20/97
        - quitting while check mail thread running causes crash
4/97
        - not all messages transfered from temp.in to in mailbox
4/11/97
        - send mail unsupported -- started support
4/15/97
        - auto-check needs to be threaded. it's interfering with threaded check.
4/15/97
        - repeating check mail when auto-checking
4/18/97
        - fix warning verbiage when TM not supported and run single-threaded.
4/29/97
        - out.temp mailbox can't be rebuilt on startup/NickMatchFound  (steve)
4/30/97
        - temp toc is grafted in-- should be hidden
5/1/97
        - messages sent multiple times when left in out temp
5/2/97
        - location of progress window not saved
5/2/97
        - FCC and filter not working yet for outgoing messages
5/2/97
        - not sending queued messages when quitting/ trash "out of date" warning
sometimes causes crash		5/5/97
        - modal dialogs crashing
5/6/97
        - font changing/port problem/ PopGWorld assertion failing (steve)
5/6/97
        - "You have new mail" dialog box crash (john)
5/6/97
        - get password not hiding text
5/6/97
        - popd lmos data not copied to original settings fork
5/6/97
        - when progress window becomes modal, it should change appearance.
5/6/97
        - option-cmd-W cmd-M cmd-1 (not processing keyboard commands besides
cancel) 	5/9/97
        - gworld stack - thread now maintains it's own
5/9/97
        - filtering mail while check mail running
5/13/97
        - closing progress window crash
5/13/97
        - should wait for idle time before modeless dlog turns modal
5/13/97
        - settings dialog turns modeless when check mail brings in box to front
5/13/97
        - personality fields (for the main thread) (e.g. passwords) aren't
updated	5/14/97
        - logging
5/14/97
        - timed queued messages not being sent
5/15/97
        - "Send Queued Messages" deactive when check mail without sending and
there's a queued mesg		5/15/97
        - 68k version crashes while in threads (but not when program locked--
suspect it's segmentation fault)	5/16/97
        - open in.temp instead of in for incoming filter with any header (pete)
5/21/97
        - open message filter for incoming opens and closes mesg
5/21/97
        - filter reply with on incoming messages creates mesg but doesn't send
(steve)	5/22/97
        - do something so steve doesn't get scared of "waiting for idle"
progress dlog 	5/22/97 (remove progress bar, resource for idle time, change
stop to go)
        - deleting message from toc by hitting delete causes slow update when
waiting for idle time	5/22/97
        - deleting message from toc by hitting delete changes cursor to beach
ball		5/22/97
        - hang when progress dialogs disabled (andrew starr)
5/22/97
        - SetPref needs to be thread-aware (steve)
5/23/97
        - mesgs with bad addresses not editable when brought up, then crashing
5/23/97
        - should automatically rebuild temp in/out mailboxes instead of asking
user	5/23/97
        - SetupXferMail () needs to handle errors better
5/27/97
        - don't need to wait for idle time when progress frontmost
5/27/97
        - keeps downloading message that was partially downloaded then fetched
(josh)			5/28/97
        - cleaning up after checking mail too long (pete)
5/28/97
        - filter to set server options to delete- doesn't work (pete)
5/29/97
        - need to fix out.temp automatically when corrupt
5/30/97
        - progress dialog flickers on and off during checkmail with send
5/30/97
        - progress dialog waits twice
5/30/97
        - buildtoc.c line 27 (not checking for nil toc)
6/10/97
        - after progress window sleeps, sometimes outgoing filters don't work
6/11/97
        - passwords not preserved for session
6/12/97
        - repeatedly checking mail
6/12/97
        - typing is slower when waiting for idle time
6/16/97
        - use global structs in globals.c
6/16/97
        - unsent messages are filtered anyway
6/19/97
        - don't need to wait for idle time when starting background check mail
6/19/97
        - clicking stop button sometimes doesn't cancel thread
6/20/97 by John B
        - hitting "allow all connections" and "work offline" not working
6/24/97
        - closing all windows causes crash (was closing progwindow and message
6/24/97 and toc which were being used by thread)
        - memory leak size = 26600
6/26/97
        - globals causing heap corruption/crash
6/26/97
        - ThreadsAvailable failing for 68k
6/26/97
        - ASSERT(CurResFile()==SettingsRefN) failing (usually when app is in
background and check mail starts)	6/27/97 Always happens when check mail,
and bring up In mailbox from menu -- flushtocs (), mightswitch ()?
        - close all messages causes crash -- in.temp getting closed
        - not filtering "old" messages in temp boxes
        - increase stack size for thread when too small
        - crash recovery - need to mark messages as sent?
        - slow typing
        - downloading messages still really slow-- don't yield as much when
!active, look into NEEDYIELD, profiler
        - don't use reserve memory for thread creation
        - move filtering to main thread
        - don't download duplicate messages in in.temp
        - SMTP error messages open in front of Filtering dialog
        - update status of outgoing immediately
9/4/97
        - redo filter temp out toc
9/4/97
        - don't allow user to change status
        - don't read filters when sending messages
        - improve error handling for incoming/outgoing messages, MDN header, toc
flags
        - implement transport status window
        - send immediately doesn't work
        - network code isn't multi-threaded; only one connection at a time
        - about box, settings shouldn't stall background thread
        - link error in 68k version
        - set skipped flag for encoding errors
        - unqueue edited messages
        - uucp unsupported
        - hitting cancel when copying settings file crashes?

        Phase 2:

        - need to spool outgoing files instead of xfering to temp out mbox

*/
