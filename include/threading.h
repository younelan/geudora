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
        File: Threading.h
        Author: Clarence Wong <cwong@qualcomm.com>
        Date: February 1997 - ...
        Copyright (c) 1997 by QUALCOMM Incorporated
*/

/* to switch threading on, #define THREADING_ON in PreCompTwoPPC.pch*/

#ifndef THREADING_H
#define THREADING_H

#include "mailbox.h"
#include "mime.h"
#include "schizo.h"
#include "util.h"
#include <stdbool.h>

/* Forward declarations for PGP types - actual definition in pgpin.h */
typedef struct PGPRecvContextStruct *PGPRecvContextPtr;

/* Decoder globals - these were once globals, but need to be thread specific for
 * the decoders to work properly */
typedef struct HexBinGlobals_ *HBGHandle;
typedef struct UUGlobals_ *UUGlobalsHandle;

typedef struct decoderGlobals_ decoderGlobalsRec, *decoderGlobalsPtr,
    *decoderGlobalsHandle;
struct decoderGlobals_ {
  bool DealingWithIdiotIMail;     // is IMail here?
  bool AnyRich;                   /* any body parts with richText? */
  bool AnyHTML;                   /* any body parts with html? */
  bool AnyFlow;                   /* any body parts with flow? */
  bool AnyDelSP;                  /* any body parts with flow? */
  bool AnyCharset;                /* any body parts with charset */
  char *LastAttPath;              /* path of last received attachment */
  bool BadBinHex;                 /* a bad binhex file was found */
  bool BadEncoding;               /* errors were found in encoding */
  bool Headering;                 /* we're fetching only headers */
  bool FixServers;                /* do we need to fix up the server areas? */
  int Prr;                        /* Pop Error code */
  bool NoAttachments;             /* don't recognize attachments file */
  FSSpecHandle SingleSpec;        /* an appledouble attachment */
  MIMEMapHandle MMIn;             /* incoming MIME map */
  MIMEMapHandle MMOut;            /* outgoing MIME map */
  GrowBuf AttachedFiles;          /* list of attachments saved */
  PGPRecvContextPtr PGPRContext;  /* PGP globals */
  UUGlobalsHandle UUG;            /* uudecoder globals */
  HBGHandle HBG;                  /* hexbin globals */
  char * parseHeaderHandle; /* used in HeaderRecvLine */
  long parseHeaderOffset;         /* used in HeaderRecvLine */
  long parseHeaderSize;           /* used in HeaderRecvLine */
  StackHandle tAttFolderStack;    // stack of attachment folders
  FSSpec tCurrentAttFolderSpec;   // our current attachment folder
};

/* Encodier globals - these were once globals, but need to be thread specific
 * for the encoders/send code to work properly */
typedef struct encoderGlobals_ encoderGlobalsRec, *encoderGlobalsPtr,
    *encoderGlobalsHandle;
struct encoderGlobals_ {
  // static variables for BufferSend
  DecoderPB pb;
  void *buffers[2];
  void *buffer;
  DecoderFunc *oldEncoder;
};

/* thread globals */
typedef struct threadGlobals_ threadGlobalsRec, *threadGlobalsPtr,
    *threadGlobalsHandle;
struct threadGlobals_ {
  MyWindowPtr tProgWindow;
  short tCommandPeriod;
  SCHandle tStringCache; /* cache for settings */
  short tSettingsRefN;   /* refNum of settings file */
  StackHandle tPersStack;
  PersHandle tPersList;
  PersHandle tCurPers;
  short tResRefN;
  StackHandle tGWStack;
  TransVector tCurTrans;
  StackHandle tPOPCmds;
  bool tCanPipeline;
  bool tNoProxify;
  bool tETLDeleteRequest; /* The translators have asked that the last decoded
                             message be deleted */
  decoderGlobalsRec decoderG;
  encoderGlobalsRec encoderG;
  char GlobalTemp[256]; /* a temporary buffer used, among other places, while
                        reading headers */
};

extern _Thread_local threadGlobalsPtr CurThreadGlobals;
extern threadGlobalsRec ThreadGlobals; // Main thread globals

#include <pthread.h>
#include "mailxfer.h"
#include "progress.h"

#include "task_types.h"

/* Thread-specific globals access macros */
#define ProgWindow (CurThreadGlobals->tProgWindow)
#undef CommandPeriod
#define CommandPeriod (CurThreadGlobals->tCommandPeriod)
#define EudoraCommandPeriod CommandPeriod
#define StringCache (CurThreadGlobals->tStringCache)
#define SettingsRefN (CurThreadGlobals->tSettingsRefN)
#define PersStack (CurThreadGlobals->tPersStack)
#define PersList (CurThreadGlobals->tPersList)
#define CurPers (CurThreadGlobals->tCurPers)
#define GWStack (CurThreadGlobals->tGWStack)
#define CurTrans (CurThreadGlobals->tCurTrans)
#define POPCmds (CurThreadGlobals->tPOPCmds)
#define CanPipeline (CurThreadGlobals->tCanPipeline)
#define NoProxify (CurThreadGlobals->tNoProxify)
#define ETLDeleteRequest (CurThreadGlobals->tETLDeleteRequest)
#define GlobalTemp (CurThreadGlobals->GlobalTemp)

#define DealingWithIdiotIMail (CurThreadGlobals->decoderG.DealingWithIdiotIMail)
#define AnyRich (CurThreadGlobals->decoderG.AnyRich)
#define AnyHTML (CurThreadGlobals->decoderG.AnyHTML)
#define AnyFlow (CurThreadGlobals->decoderG.AnyFlow)
#define AnyDelSP (CurThreadGlobals->decoderG.AnyDelSP)
#define AnyCharset (CurThreadGlobals->decoderG.AnyCharset)
#define LastAttPath (CurThreadGlobals->decoderG.LastAttPath)
#define Prr (CurThreadGlobals->decoderG.Prr)
#define BadBinHex (CurThreadGlobals->decoderG.BadBinHex)
#define BadEncoding (CurThreadGlobals->decoderG.BadEncoding)
#define Headering (CurThreadGlobals->decoderG.Headering)
#define FixServers (CurThreadGlobals->decoderG.FixServers)
#define NoAttachments (CurThreadGlobals->decoderG.NoAttachments)
#define SingleSpec (CurThreadGlobals->decoderG.SingleSpec)
#define MMIn (CurThreadGlobals->decoderG.MMIn)
#define MMOut (CurThreadGlobals->decoderG.MMOut)
#define AttachedFiles (CurThreadGlobals->decoderG.AttachedFiles)
#define PGPRContext (CurThreadGlobals->decoderG.PGPRContext)
#define UUG (CurThreadGlobals->decoderG.UUG)
#define HBG (CurThreadGlobals->decoderG.HBG)
#define parseHeaderHandle (CurThreadGlobals->decoderG.parseHeaderHandle)
#define parseHeaderOffset (CurThreadGlobals->decoderG.parseHeaderOffset)
#define parseHeaderSize (CurThreadGlobals->decoderG.parseHeaderSize)
#define AttFolderStack (CurThreadGlobals->decoderG.tAttFolderStack)
#define CurrentAttFolderSpec (CurThreadGlobals->decoderG.tCurrentAttFolderSpec)

#define EncoderGlobalsPb (CurThreadGlobals->encoderG.pb)
#define EncoderGlobalsBuffers (CurThreadGlobals->encoderG.buffers)
#define EncoderGlobalsBuffer (CurThreadGlobals->encoderG.buffer)
#define EncoderGlobalsOldEncoder (CurThreadGlobals->encoderG.oldEncoder)

struct threadContextData_ {
  threadGlobalsPtr newThreadGlobals;
  StackHandle prefStack; // keep track of pref changes
};

// keep less than size of int so it can fit in void *
typedef struct xferMailParams_ {
  unsigned check : 1;
  unsigned send : 1;
  unsigned manual : 1;
  unsigned scripted : 1;
  XferFlags flags;
} xferMailParamsRec;

// special structs used inside the IMAPTransferRec_
typedef struct IMAPSCStruct IMAPSCStruct, *IMAPSCPtr, *IMAPSCHandle;
typedef struct IMAPSResultStruct IMAPSResultStruct, *IMAPSResultPtr,
    *IMAPSResultHandle;
typedef struct IMAPAppendStruct IMAPAppendStruct, *IMAPAppendPtr,
    *IMAPAppendHandle;

#ifndef IMAP_TRANSFER_REC_DEFINED
#define IMAP_TRANSFER_REC_DEFINED
// IMAPTransferRec.  Used to pass IMAP parameters into check mail thread.
struct IMAPTransferRec_ {
  FSSpec targetSpec;   // the interesting file
  TOCType * sourceToc; // handle to TOC where messages are ocming from
  TOCType * destToc;   // handle to TOC for messge to be downloaded into
  TOCType * delToc;    // handle to TOC from which message is to be deleted.
  short sumNum;        // sumNum of message to be operated on
  bool nuke;           // don't copy the message to the trash
  bool expunge;        // expunge the mailbox
  bool copy;           // copy messages
  bool attachmentsToo; // download everything in one big chunk
  GArray *uids;         // list of uids
  MailboxNodeHandle targetBox; // the interesting mailbox
  FSSpecHandle attachments;    // list of attachment stubs to fetch
  FSSpecHandle toResync;       // list of mailboxes to resync
  IMAPAppendHandle appendData; // data for message apends

  // message search
  void *toSearch;      // list of FSSpecs to search.  These are indexes into a
                        // BoxCount handle
  void *boxesToSearch; // list of mailboxes to search
  IMAPSCHandle searchC; // list of search criteria
  bool matchAll;        // AND or OR
  IMAPSResultHandle results; // results of the search
  long firstUID; // where to start the search from.  Only makes sense on a
                 // single box search.

  TaskKindEnum command; // what this thread should be doing
};
#endif


typedef struct threadContextData_ threadContextDataRec, *threadContextDataPtr;
typedef struct threadData_ threadDataRec, *threadDataPtr;
typedef threadDataRec *threadDataHandle;
typedef struct IMAPTransferRec_ IMAPTransferRec, *IMAPTransferPtr;

/* data structures */
struct threadData_ {
  TaskKindEnum currentTask;
  pthread_t threadID;
  threadContextDataRec threadContext;
  xferMailParamsRec xferMailParams;
#ifdef TASK_PROGRESS_ON
  ProgressBlock **prbl;
  ControlHandle stopButton;
#endif
#ifdef DEBUG
  long startTime, totalTimeThread, switchInTime;
  short switchCount;
#endif
#if __profile__
  ProfilerThreadRef threadRef;
#endif
  IMAPTransferRec imapInfo;
  threadDataHandle next;
};

/* public functions */

void YieldCPUNow(void);

void CleanTempOutTOC(void);
void CleanRealOutTOC(void);
void DecrementNumBackgroundThreads(void);
short GetMainGlobalSettingsRefN(void);
int GetNumBackgroundThreads(void);

TaskKindEnum GetCurrentTaskKind(void);
void GetCurrentThreadData(threadDataHandle *threadData);
void GetThreadData(pthread_t threadID, threadDataHandle *threadData);
#ifdef TASK_PROGRESS_ON
#include "progress.h"
ProgressBlock **GetCurrentThreadPrbl(void);
#endif
bool InAThread(void);
void IncrementNumBackgroundThreads(void);
int MyInitThreads(void);
void KillThreads(void);
void MyThreadBeginCritical(void);
void MyThreadEndCritical(void);
void MyYieldToAnyThread(void);

int PushThreadPrefChange(short pref);
void SetCurrentTaskKind(TaskKindEnum taskKind);
void SetThreadGlobalCommandPeriod(pthread_t threadID, bool value);
int SetupXferMailThread(bool check, bool send, bool manual, bool scripted,
                          XferFlags flags, IMAPTransferPtr imapInfo);
bool ThreadsAvailable(void);
void RemoveTaskErrors(TaskKindEnum taskKind, long persId);
int SetThreadStackSize(long newSize);
long GetThreadStackSize(void);

void *GetResourceMainThread(uint32_t theType, short theID);
int ZapSettingsResourceMainThread(uint32_t type, short id);
int AddMyResourceMainThread(void *h, uint32_t type, short id,
                              ConstStr255Param name);

#define GetResourceMainThread_(t, i)                                           \
  (void *)GetResourceMainThread((uint32_t)t, i)
#define ZapSettingsResourceMainThread_(t, i)                                   \
  (void *)ZapSettingsResourceMainThread((uint32_t)t, i)
#define AddMyResourceMainThread_(h, t, i, n)                                   \
  AddMyResourceMainThread((void *)(h), (uint32_t)(t), i, (ConstStr255Param)(n))


#endif // THREADING_H