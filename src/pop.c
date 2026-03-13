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
#include "pop.h"
#include "Globals.h"    /* For BUG15 */
#include "MyRes.h"      /* For POPD_ID, FETCH_ID, DELETE_ID */
#include "StringDefs.h" /* For POP_PORT, KERB_POP_PORT, POP_SSL_PORT, etc. */
#include "StringUtil.h" /* For string manipulation functions */
#include "acap.h"       /* For GetPOPPref */
#include "buildtoc.h"   /* For ReadSum */
#include "fileutil.h"   /* For file utility functions */
#include "fileutil.h"
#include "gtk_dialogs.h" /* For ComposeStdAlert, SetPrefLong */
#include "lex822.h"      /* For EndOfHeader */
#include "log.h"
#include "statmng.h"
#include "utl.h"
#include <libgen.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <malloc/malloc.h>
#include <unistd.h>
#ifndef DisposePtr
#define DisposePtr free
#endif
#ifndef resChanged
#define resChanged 2
#define GetResAttrs(h) 0
#define ChangedResource(h)
#endif
#ifndef MINI_MASK
#define MINI_MASK 0
#endif
#ifndef NumToString
#define NumToString MyNumToString
#endif
extern void MyNumToString(long n, char *s);
static inline void Fix1MessServerArea(void *win) {}
#ifndef cKrbNotLoggedIn
#define cKrbNotLoggedIn -1
#endif
static inline void KClientDisposeSessionCompat(void *s) {}
static inline int KClientLogoutCompat(void) { return 0; }
static inline int KClientGetUserNameDeprecated(char *name) { return -1; }
static inline int KClientNewSessionCompat(void *s, int a, int b, int c, int d) {
  return -1;
}
static inline int KClientMakeSendAuthCompat(void *a, char *b, void *c, long *d,
                                            int e, char *f) {
  return -1;
}
static inline int KClientLoginCompat(void *a, void *b) { return 0; }
static inline void GetCurrentThread(void *t) {}
static inline long ThreadCurrentStackSpace(ThreadID t, long *s) {
  *s = 1024 * 1024;
  return 0;
}
static inline long StackSpace(void) { return 1024 * 1024; }
static inline int GetCurrentISA(void) { return 0; }
#ifndef kPowerPCISA
#define kPowerPCISA 1
#endif
#ifndef kStatReceivedAttach
#define kStatReceivedAttach 0
#endif
static inline char *URLEscape(char *s) { return s; }
/* c2pstr removed - all strings are C strings now */
/* IsWindowVisible provided by mailbox.h as static inline */
#ifndef MIDHash
#define MIDHash(s, l) 0
#endif
#ifndef kUnresolvedCFragSymbolAddress
#define kUnresolvedCFragSymbolAddress 0
#endif
#include "lineio.h" /* For LineIOD, OpenLine, SeekLine */
#include "mailbox.h"
#include "myssl.h"
#include "progress.h"  /* For Progress, ProgressR, ByteProgress, etc. */
#include "schizo.h"    /* For CUR_POPD_TYPE */
#include "tcp.h"       /* For OTFlushInput */
#include "threading.h" /* For Prr, POPCmds, FixServers, CanPipeline macros */
#include "StringUtil.h"
#include "taskProgress.h"
#include "util.h"      /* For Accumulator functions */

#ifdef CommandPeriod
#undef CommandPeriod
#endif
extern bool CommandPeriod;
#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif

#define FILE_NUM 30

/* Missing constant definitions - stubs for logging and other features */
#define LOG_LMOS 0             /* Stub for logging constant */
#define LOG_RETR 0             /* Stub for logging constant */
#define LOG_TPUT 0             /* Stub for logging constant */
#define Note 0                 /* Stub for alert type */
#define unimpErr -4            /* Unimplemented error code */
#define OPT_DELSP 0x0010       /* Stub for message option */
#define OPT_RECEIPT 0x0020     /* Stub for message option */
#define OPT_IMAP_SENT 0x0040   /* Stub for message option */
#define kStatReceivedMail 0    /* Stub for statistics */
#define EMSF_ON_ARRIVAL 0x0001 /* Stub for EMS flag */
#ifndef popRStatus
#define popRStatus 0
#endif
#ifndef popRLast
#define popRLast 1
#endif
#ifndef popRUIDL
#define popRUIDL 3
#endif
#ifndef FLAG_FIRST
#define FLAG_FIRST (1 << 18)
#endif
#ifndef FLAG_SUBSEQUENT
#define FLAG_SUBSEQUENT (1 << 19)
#endif
extern long GetPrefLong(short prefId);
extern bool PrefIsSet(short prefId);
extern int GetPOPInfoLo(unsigned char *server, unsigned char *s2, long *port);
extern void Dprintf(const char *fmt, ...);
extern void *GetResource(uint32_t type, short id);
extern void DeleteSum(void *tocH, short sumNum);
extern bool TOCIsDirty(void *tocH);
extern int WriteTOC(void *tocH);
extern void Aprintf(short alertType, short noteType, short strn, ...);
extern int SpecMoveAndRename(FSSpecPtr from, FSSpecPtr to);
extern OSErr StackQueue(void *what, void **stack);
extern OSErr StackTop(void *into, void **stack);
extern bool ValidHash(uint32_t hash);
extern void RemoveUTF8FromSum(void *sum);
void MakeMessTitle(unsigned char *title, TOCType * tocH, int sumNum,
                   bool useSummary);
extern void MyParamText(PStr p1, PStr p2, PStr p3, PStr p4);
extern void BeginHexBin(HeaderDHandle hdh);
extern void EndHexBin(void);
bool BeginAbomination(PStr name, HeaderDHandle hdh);
short SaveAbomination(UPtr text, long size);
short ClearAbomination(void);
bool ConvertHexBin(short refN, UPtr buf, long *size, POPLineType lineType,
                   long estSize);
bool ConvertUUSingle(short refN, UPtr buf, long *size, POPLineType lineType,
                     long estSize, MIMEMapPtr hintMM, HeaderDHandle hdh);
/* ZapSettingsResourceMainThread_ - don't redeclare, it's a macro */
/* Progress functions are in progress.h - don't redeclare them */
OSErr RecordTransAttachments(const char *path);
extern void FixURLString(PStr url);
/* URLEscape - don't declare, it's defined elsewhere or is a macro */

/* Missing constants */
/* NoChange and NoBar are in progress.h enum */
/* NeedToFilterIn is now a global in Globals.h */
#define ksStatReceivedAttach 0
#define kSystemIconsCreator 'macs'
#define kGenericFolderIcon 'fldr'

/* SASL function stubs - not yet implemented */
static inline int SASLFind(PStr service, PStr token, int mech) { return 0; }
static inline int SASLDo(PStr service, int mech, short rounds, long *state,
                         void *chalAcc, void *respAcc) {
  return -1;
}
static inline void SASLDone(PStr service, int mech, short rounds, long *state,
                            int code){}

/* min macro if not defined */
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/*
5/29/97 cwong
NOTE:

You should use the following macros when accessing POPD resources in the
settings file:

         GetResourceMainThread_
         ZapSettingsResourceMainThread
         AddMyResourceMainThread_

They access the main thread's Settings file (not the background thread's copy.)
*/

/************************************************************************
 * functions for dealing with a pop 2 server
 ************************************************************************/

#ifdef KERBEROS
#include <krb.h>


#endif
#pragma segment POP

#define CMD_BUFFER 514
#define PSIZE (UseCTB ? 256 : 4096)
#define errNotFound -2

#define MatchPOPD(dh, spot, hash)                                              \
  ((hash) && (dh)->data[spot].uidHash == (hash))

#define POP_TERM(buffer, size)                                                 \
  ((size) == 2 && (buffer)[0] == '.' && (buffer)[1] == '\015')

/************************************************************************
 * POPDHandle helper functions (replaces Mac Handle operations)
 ************************************************************************/
POPDHandle POPDNew(int count) {
  POPDHandle h = (POPDHandle)malloc(sizeof(POPDArray));
  if (!h) return NULL;
  if (count > 0) {
    h->data = (POPDesc *)calloc(count, sizeof(POPDesc));
    if (!h->data) { free(h); return NULL; }
  } else {
    h->data = NULL;
  }
  h->count = count;
  return h;
}

void POPDFree(POPDHandle *pH) {
  if (pH && *pH) {
    free((*pH)->data);
    free(*pH);
    *pH = NULL;
  }
}

int POPDAppend(POPDHandle h, const POPDesc *item) {
  if (!h) return -1;
  POPDesc *newData = (POPDesc *)realloc(h->data, (h->count + 1) * sizeof(POPDesc));
  if (!newData) return -1;
  h->data = newData;
  h->data[h->count] = *item;
  h->count++;
  return 0;
}

void POPDRemoveAt(POPDHandle h, int index) {
  if (!h || index < 0 || index >= h->count) return;
  if (index < h->count - 1)
    memmove(&h->data[index], &h->data[index + 1],
            (h->count - 1 - index) * sizeof(POPDesc));
  h->count--;
  if (h->count == 0) {
    free(h->data);
    h->data = NULL;
  } else {
    h->data = (POPDesc *)realloc(h->data, h->count * sizeof(POPDesc));
  }
}

/************************************************************************
 * GrowBuf helper functions (replaces Handle-based growing buffers)
 ************************************************************************/
void GrowBuf_Init(GrowBuf *buf) {
  buf->data = NULL;
  buf->size = 0;
  buf->capacity = 0;
}

int GrowBuf_Append(GrowBuf *buf, const void *ptr, long len) {
  if (len <= 0) return 0;
  if (buf->size + len > buf->capacity) {
    long newCap = buf->capacity ? buf->capacity * 2 : 256;
    while (newCap < buf->size + len) newCap *= 2;
    char *newData = (char *)realloc(buf->data, newCap);
    if (!newData) return -1;
    buf->data = newData;
    buf->capacity = newCap;
  }
  memcpy(buf->data + buf->size, ptr, len);
  buf->size += len;
  return 0;
}

void GrowBuf_Reset(GrowBuf *buf) {
  buf->size = 0;
}

void GrowBuf_Free(GrowBuf *buf) {
  free(buf->data);
  buf->data = NULL;
  buf->size = 0;
  buf->capacity = 0;
}

/************************************************************************
 * private routines
 ************************************************************************/
void POPDelDup(POPDHandle popDH);
OSErr POPPreFetch(TransStream stream, POPDHandle popDH, short message,
                  bool *capabilities);
int POPGetReplyLo(TransStream stream, short cmd, unsigned char *buffer,
                  long *size, AccuPtr resAcc);
#define POPGetReply(stream, cmd, buffer, size)                                 \
  POPGetReplyLo(stream, cmd, buffer, size, nil)
void RelatedNote(const char *path, HeaderDHandle hdh, const char *theMessage);
int POPByeBye(TransStream stream);
int POPCmdLo(TransStream stream, short cmd, unsigned char *args,
             AccuPtr argsAcc);
#define POPCmd(stream, cmd, args) POPCmdLo(stream, cmd, args, nil)
int POPGetMessage(TransStream, long messageNumber, short *gotSome,
                  POPDHandle popDH, bool *capabilities);
int DupHeader(short refN, unsigned char *buff, long bSize, long offset,
              long headerSize);
int SaveAndSplit(TransStream stream, short refN, long estSize,
                 HeaderDHandle *hdhp, bool isIMAP);
bool StackLowErr = false;
bool PopConnected;
int FirstUnread(TransStream stream, int count);
bool HasBeenRead(TransStream stream, short msgNum, short count);
void StampPartNumber(MSumPtr sum, short part, short count);
unsigned char *ExtractStamp(unsigned char *stamp, unsigned char *banner);
short POPLast(TransStream, short *lastRead);
POPLineType ReadPlainBody(TransStream stream, short refN, char *buf, long bSize,
                          long estSize);
short SplitMessage(short refN, long hStart, long hEnd, long msgEnd);
void DisposePOPD(POPDHandle *popDH);
OSErr BuildPOPD(TransStream stream, POPDHandle *popDH, short count,
                XferFlags *flags, bool *capabilities);
void FillPOPD(POPDPtr pdp, HeaderDHandle hdh);
short CountFetch(POPDHandle popDH);
PStr HeaderMsgId(HeaderDHandle hdh, PStr msgId);
uint32_t FakeMIDHash(HeaderDHandle hdh);
void SetFetchDel(POPDHandle popDH, short from, short to, bool fetch,
                 bool delete);
void SetFetched(POPDHandle popDH, short from, short to);
void SetBeforeAfter(POPDHandle popDH, uint32_t gmt, short *after,
                    short *before);
OSErr POPMsgSize(short messageNumber, long *msgsize);
short FindExistSpot(POPDHandle popDH, uint32_t hash);
OSErr DeletePOPMessage(TransStream stream, short number, long uidHash);
OSErr FillWithUidl(TransStream stream, POPDHandle popDH);
OSErr FillWithTop(TransStream stream, POPDHandle new, POPDHandle old);
OSErr FillSizesWithList(TransStream stream, POPDHandle popDH);
short FindUndelete(POPDHandle popDH, uint32_t gmt, uint32_t hash);
OSErr FillPOPDFromServer(TransStream stream, POPDHandle popDH, short spot);

OSErr InitKerberos();
OSErr KerbGetTicket(PStr popName, PStr host, PStr realm, PStr version,
                    unsigned char **ticket);
OSErr SendPOPTicket(TransStream stream);
void LogPOPD(PStr intro, POPDHandle newDH);
void Log1POPD(PStr intro, PStr which, POPDHandle popDH);
bool NoClearPass(bool *capabilities, unsigned char *response, short len);
void PrunePOPD(OSType listType, short listId, POPDHandle onServer);
OSErr ReapCmds(TransStream stream, short cmd);
void PopCapabilities(TransStream stream, bool *capabilities, SASLEnum *mechPtr);
int POPSasl(TransStream stream, bool *capabilities, SASLEnum mech,
            unsigned char *buffer, long *size);
OSErr FixLongFilename(HeaderDHandle hdh, const char *path);
PStr Un2184Append(PStr dest, short sizeofDest, PStr orig, PStr charset,
                  bool isEncoded);
PStr Un2184(PStr dest, PStr orig, PStr charset);

/* stack sniffer defines */

// 5k seems to work for ppc. may need to tweak it some more. s/b smaller for
// 68k?
#define kLowStackSize ((GetCurrentISA() == kPowerPCISA) ? (5 K) : (4 K))
#define kMoreStackSpace (10 K)

/* Globals */

bool gPOPKerbInited = false; // true when Kerberos has been initialized for POP
// Kerberos types stubbed for non-Kerberos builds
typedef struct {
  int dummy;
} KClientSessionInfo;
typedef struct {
  int dummy;
} KClientKey;
KClientSessionInfo gSession; // session info
KClientKey gPrivateKey;      // private key

/************************************************************************
 * GetMyMail - the biggie; transfers mail into In mailbox
 ************************************************************************/
short GetMyMail(TransStream stream, bool quietly, short *gotSome,
                struct XferFlags *flags) {
#pragma unused(quietly)
  int messageCount;
  unsigned char msgname[256];
  unsigned char hostName[64];
  long port;
  TOCType * tocH;
  short err;
  short fetchCount, message, fetched;
  POPDHandle popDH = nil;
  bool built = True;
  int beforeBytes, actualBytes, approxBytes;
#ifdef BATCH_DELIVERY_ON
  bool inThread = InAThread();
#endif
  bool capabilities[pcapaLimit + 1];

  WriteZero(capabilities,
            sizeof(capabilities)); // we don't know if we have them yet!!!

  if (Prr = StackInit(sizeof(short), &POPCmds))
    return (Prr);

  *gotSome = 0;
#ifdef ESSL
  stream->ESSLSetting = GetPrefLong(PREF_SSL_POP_SETTING);
  if (stream->ESSLSetting & esslUseAltPort)
    port = GetRLong(POP_SSL_PORT);
  else
#endif
    port =
        PrefIsSet(PREF_KERBEROS) ? GetRLong(KERB_POP_PORT) : GetRLong(POP_PORT);
  GetPOPInfo(msgname, hostName);
  if ((err = StartPOP(stream, hostName, port)) == noErr) {
      messageCount = POPIntroductions(stream, msgname, capabilities);
#ifdef DEBUG
      if (BUG15)
        Dprintf("%d;sc;g", Prr);
#endif
      if (capabilities[0] && !capabilities[pcapaUIDL]) {
        (*CurPers)->noUIDL = true;
        Log(LOG_LMOS, (UPtr)"CAPA says no UIDL");
      }
      if (!Prr) {
        if (messageCount == 0) {
          FixServers =
              FixServers || nil != GetResource_(CUR_POPD_TYPE, POPD_ID);
          ZapSettingsResourceMainThread_(CUR_POPD_TYPE, POPD_ID);
          ZapSettingsResourceMainThread_(CUR_POPD_TYPE, DELETE_ID);
          ZapSettingsResourceMainThread_(CUR_POPD_TYPE, FETCH_ID);
        } else if (!BuildPOPD(stream, &popDH, messageCount, flags,
                              capabilities)) {
          CanPipeline = (capabilities[0] ? capabilities[pcapaPipelining]
                                         : PrefIsSet(PREF_CAN_PIPELINE)) &&
                        !(*CurPers)->noUIDL;

          ComposeLogR(LOG_RETR, nil, START_POP_LOG, hostName, port,
                      messageCount);
          if (tocH = GetInTOC()) {
            /*
             * run through the pure deletes
             */
            for (message = 0; message < messageCount; message++)
              if (popDH->data[message].delete && !popDH->data[message].retr &&
                  !popDH->data[message].stub) {
                Prr = DeletePOPMessage(stream, message,
                                       popDH->data[message].uidHash);
                if (!Prr)
                  popDH->data[message].deleted = True;
                else
                  break;
              }

            /*
             * and now do the fetches
             */
            if (!Prr) {
#ifdef BATCH_DELIVERY_ON
              short batchNum = GetRLong(DELIVERY_BATCH);
#endif
              fetchCount = CountFetch(popDH);
              if (CanPipeline)
                POPPreFetch(stream, popDH, 0, capabilities);
              for (fetched = message = 0; message < messageCount; message++)
                if (popDH->data[message].retr || popDH->data[message].stub) {
                  ProgressR(NoChange, fetchCount - fetched, 0, LEFT_TO_TRANSFER,
                            nil);
                  TOCSetDirty(tocH, true);
                  beforeBytes = GetProgressBytes();
#ifdef DEBUG
                  if (BUG15)
                    Dprintf("%d;sc;g", Prr);
#endif
                  if (CommandPeriod || POPGetMessage(stream, message, gotSome,
                                                     popDH, capabilities))
                    break;
#ifdef DEBUG
                  if (BUG15)
                    Dprintf("%d;sc;g", Prr);
#endif
                  fetched++;
                  // adjust progress bar
                  actualBytes = GetProgressBytes() - beforeBytes;
                  approxBytes = popDH->data[message].stub
                                    ? (3 K)
                                    : popDH->data[message].msgSize;
                  if (actualBytes < approxBytes)
                    ByteProgress(nil, actualBytes - approxBytes, 0);
                  else
                    ByteProgressExcess(approxBytes - actualBytes);

                  // delete this message if a translator requested it.
                  if (ETLDeleteRequest) {
                    DeleteSum(tocH, tocH->count - 1);
                    fetched--;
                    ETLDeleteRequest = false;
                  }

#ifdef BATCH_DELIVERY_ON
                  if (inThread && (fetched % batchNum == 0)) {
                    tocH = RenameInTemp(tocH);
                    // This is bad.  We don't know why this happens,
                    // and if this codebase had a future, we would need
                    // to find out.  But for now, we're just going to stop
                    // the crashing and feel ashamed.  SD 5/2005
                    if (!tocH)
                      break;
#ifdef THIS_CODE_HAD_A_FUTURE
#error FIX ME!
#endif
                  }
#endif
                }
#ifdef BATCH_DELIVERY_ON
              if (inThread)
                RenameInTemp(tocH);
#endif
            }

            if (CommandPeriod)
              Prr = userCancelled;
          } else
            Prr = 1;
          if (!Prr) {
            ProgressMessageR(kpSubTitle, CLEANUP_CONNECTION);
          }

          PrunePOPD(CUR_POPD_TYPE, DELETE_ID, popDH);
          PrunePOPD(CUR_POPD_TYPE, FETCH_ID, popDH);
          DisposePOPD(&popDH);
        } else /* popd build failed */
          POPDFree(&popDH);
      }
    }
  if (!err)
    err = Prr;
  if (!err && messageCount == 0)
    ZapSettingsResourceMainThread_(CUR_POPD_TYPE, POPD_ID);
  ProgressMessageR(kpSubTitle, CLEANUP_CONNECTION);
  if (AttachedFiles.data)
    GrowBuf_Reset(&AttachedFiles);
  err = Prr;
  EndPOP(stream);

  ZapHandle(POPCmds);
  return (err);
}

#ifdef BATCH_DELIVERY_ON
/**********************************************************************
 * RenameInTemp
 **********************************************************************/
TOCType * RenameInTemp(TOCType * tocH) {
  unsigned char name[64];
  FSSpec deliverSpec, inSpec, deliverFolder;
  FSSpec deliverTOCSpec, tocSpec;
  CInfoPBRec hfi;
  long maxFileNum = 0, fileNum;
  OSErr err;

  if (!tocH || !tocH->count)
    return tocH;
  if (err = SubFolderSpec(DELIVERY_FOLDER, &deliverFolder)) {
    Aprintf(OK_ALRT, Note, THREAD_SUBFOLDER_ERR, DELIVERY_FOLDER, err);
    return tocH;
  }

  // make sure the toc is written
  if (TOCIsDirty(tocH) || tocH->reallyDirty)
    if (err = WriteTOC(tocH))
      return tocH;

  /* find highest-numbered file in delivery folder */
  /* TODO: Port DirIterate to use portable FSSpec-based API */
  maxFileNum = 0; /* Start from 0 for now */
#if 0             /* Old Mac-specific code - needs porting */
	Zero(hfi);
	hfi.hFileInfo.ioNamePtr = name;
	hfi.hFileInfo.ioFDirIndex=0;
	while(!DirIterate(deliverFolder.vRefNum,deliverFolder.parID,&hfi))
	{
		if (hfi.hFileInfo.ioFlFndrInfo.fdType==MAILBOX_TYPE)	
		{
			StringToNum(name, &fileNum);
			if (fileNum > maxFileNum)
				maxFileNum = fileNum;
		}
	}
#endif

  // Make name for new mailbox
  inSpec = GetMailboxSpec(tocH, -1);
  NumToString(maxFileNum + 1, name);
  while (strlen((const char *)name) < 6)
    PInsertC(name, sizeof(name), '0', name);
  FSMakeFSSpec(deliverFolder.vRefNum, deliverFolder.parID, name, &deliverSpec);

  // toc file?
  tocSpec = inSpec;
  PCatR(tocSpec.name, TOC_SUFFIX);
  if (!FSpExists(&tocSpec)) {
    FSMakeFSSpec(deliverFolder.vRefNum, deliverFolder.parID, name,
                 &deliverTOCSpec);
    PCatR(&deliverTOCSpec.name, TOC_SUFFIX);
  } else
    *tocSpec.name = 0;

  // Move files
  if (tocH->win)
    CloseMyWindow(GetMyWindowWindowPtr(tocH->win));
  if (err = SpecMoveAndRename(&inSpec, &deliverSpec)) {
    Aprintf(OK_ALRT, Note, THREAD_DELIVER_CREATE_ERR, deliverSpec.name, err);
  } else {
    if (*tocSpec.name) {
      if (err = SpecMoveAndRename(&tocSpec, &deliverTOCSpec)) {
        Aprintf(OK_ALRT, Note, THREAD_DELIVER_CREATE_ERR, deliverTOCSpec.name,
                err);
        FSpDelete(&tocSpec); // hell with it.  We can rebuild it
      }
    }

    // Ok, we have moved the temp.in.  Make a new one
    if (err = MakeResFile(inSpec.name, inSpec.vRefNum, inSpec.parID, CREATOR,
                          MAILBOX_TYPE)) {
      Aprintf(OK_ALRT, Note, THREAD_DELIVER_CREATE_ERR, inSpec.name, err);
      // this is bad
    }

    NeedToFilterIn++; // some filtering to be done
  }

  tocH = GetTempInTOC();

  return tocH;
}
#endif

/**********************************************************************
 * POPPreFetch
 **********************************************************************/
OSErr POPPreFetch(TransStream stream, POPDHandle popDH, short message,
                  bool *capabilities) {
  short messageCount = popDH->count;
  unsigned char args[64];
  unsigned char top[16];
  OSErr err = noErr;
  short cmd;

  for (; message < messageCount; message++)
    if (popDH->data[message].retr || popDH->data[message].stub) {
      NumToString(message + 1, args);
      if (popDH->data[message].stub) {
        NumToString(GetRLong(BIG_MESSAGE_FRAGMENT), top);
        PCatC(args, ' ');
        PCat(args, top);
        if (capabilities[pcapaMangle] || capabilities[pcapaXMangle]) {
          PCatC(args, ' ');
          PCatR(args, POPCapaStrn + (capabilities[pcapaMangle] ? pcapaMangle
                                                               : pcapaXMangle));
          PCatR(args, MANGLE_ARGS);
        }
        cmd = kpcTop;
      } else
        cmd = kpcRetr;

      return (POPCmd(stream, cmd, args));
    }
  return (noErr);
}

/************************************************************************
 * POPrror - see if there was a POP error
 ************************************************************************/
int POPrror(void) { return (Prr); }

/************************************************************************
 * private routines
 ************************************************************************/
/************************************************************************
 * StartPOP - get connected to the POP server
 ************************************************************************/
int StartPOP(TransStream stream, unsigned char *serverName, long port) {
  PopConnected = False;
  Prr = ConnectTrans(stream, serverName, port, False, GetRLong(OPEN_TIMEOUT));
  return (Prr);
}

/************************************************************************
 * EndPOP - get rid of the POP server
 ************************************************************************/
int EndPOP(TransStream stream) {
  SilenceTrans(stream, True);
  if (CommandPeriod && TransError(stream) == userCancelled)
    POPCmd(stream, kpcQuit, nil);
  if (!Prr) {
    if (!CommandPeriod)
      (void)POPByeBye(stream);
    Prr = DisTrans(stream);
  }
  Prr = DestroyTrans(stream) || Prr;
  return (Prr);
}

/************************************************************************
 * POPIntroductions - sniff the POP server's bottom, and vice-versa
 ************************************************************************/
int POPIntroductions(TransStream stream, PStr user, bool *capabilities) {
  unsigned char buffer[256];
  unsigned char args[256];
  long size;
  int result = -1;
  bool useAPOP = PrefIsSet(PREF_APOP);
  bool kerb4 = PrefIsSet(PREF_KERBEROS) && !PrefIsSet(PREF_K5_POP);
  unsigned char digest[256];
  SASLEnum mech = 0;

  if (kerb4)
    if (Prr = SendPOPTicket(stream)) {
      (*CurPers)->popSecure = False;
      goto done;
    }

  do {
    size = sizeof(buffer) - 1;
    Prr = RecvLine(stream, buffer, &size);
    if (Prr)
      goto done;
    if (size < sizeof(buffer))
      buffer[size] = '\0';
    ProgressMessage(kpMessage, buffer);
  } while (buffer[0] != '+' && buffer[0] != '-');

  PopConnected = size && (buffer[0] == '+' || buffer[0] == '-');
  if (buffer[0] != '+') {
    Prr = buffer[0];
    POPCmdError(-1, nil, buffer);
    if (kerb4 && !NoClearPass(capabilities, buffer, size))
      KerbDestroy();
    goto done;
  }

  if (capabilities)
    PopCapabilities(stream, capabilities, &mech);

#ifdef ESSL
  if (ShouldUseSSL(stream) && !(stream->ESSLSetting & esslSSLInUse)) {
    if (!capabilities || !capabilities[pcapaSTLS]) {
      if (!(stream->ESSLSetting & esslOptional)) {
        Prr = unimpErr;
        ComposeStdAlert(Note, ALRTStringsStrn + NO_SERVER_SSL);
        goto done;
      }
    } else {
      StringPtr errStr;

      errStr = buffer;
      size = sizeof(buffer) - 1;
      Prr = POPCmdGetReply(stream, kpcStls, nil, buffer, &size);
      if (!Prr) {
        Prr = ESSLStartSSL(stream);
        if (Prr) {
        DoSSLErrString:
          GetRString(buffer, SSL_ERR_STRING);
          errStr = buffer + 1;
          goto DoSSLErr;
        } else if (stream->ESSLSetting & esslSSLInUse)
          PopCapabilities(stream, capabilities, &mech);
        else {
          // Cyrus sucks.
          // After a failed TLS negotiation, Cyrus will issue a bogus
          // -ERR response to the NEXT command.  They shouldn't be issuing
          // any protocol-level response at all.  bxxxxxxs
          OTFlushInput(stream, GetRLong(FLUSH_TIMEOUT));
        }
      } else
      DoSSLErr: {
        if (stream->ESSLSetting & esslOptional)
          Prr = noErr;
        else {
          POPCmdError(kpcStls, nil, errStr);
          goto done;
        }
      }
    }
  }
#endif

  ProgressMessageR(kpSubTitle, LOGGING_IN);

  if (mech && !kerb4) {
    // SASL ahoy!
    size = sizeof(buffer) - 1;
    Prr = POPSasl(stream, capabilities, mech, buffer, &size);
  } else {
    if (useAPOP) {
      PCopy(args, (*CurPers)->password);
      useAPOP = GenDigest(buffer, args, digest);
    }

    if (useAPOP) {
      size = sizeof(buffer) - 1;
      if (PrefIsSet(PREF_POP_SENDHOST))
        GetPOPPref(args);
      else
        PCopy(args, user);
      PCatC(args, ' ');
      PCat(args, digest);
      Prr = POPCmdGetReply(stream, kpcApop, args, buffer, &size);
    } else {
      if (PrefIsSet(PREF_POP_SENDHOST))
        GetPOPPref(args);
      else
        PCopy(args, user);
      size = sizeof(buffer) - 1;
      Prr = POPCmdGetReply(stream, kpcUser, args, buffer, &size);
      if (Prr || *buffer != '+') {
        if (!Prr)
          POPCmdError(kpcUser, args, buffer);
        Prr = '-';
        goto done;
      }

      if (kerb4)
        GetRString(args, KERBEROS_FAKE_PASS);
      else
        PCopy(args, (*CurPers)->password);

      size = sizeof(buffer) - 1;
      Prr = POPCmdGetReply(stream, kpcPass, args, buffer, &size);
    }
  }
  if (Prr || *buffer != '+') {
    if (!Prr) {
      (*CurPers)->popSecure = False;
      POPCmdError(kpcPass, nil, buffer);
      if (!NoClearPass(capabilities, buffer, size))
        InvalidatePasswords(False, True, False);
    }
    Prr = '-';
    goto done;
  }
  (*CurPers)->popSecure = True;
  SetPrefLong(PREF_POP_LAST_AUTH, GMTDateTime());

  ProgressMessageR(kpSubTitle, LOOK_MAIL);

  size = sizeof(buffer) - 1;
  Prr = POPCmdGetReply(stream, kpcStat, nil, buffer, &size);
  if (Prr || *buffer != '+') {
    if (!Prr)
      POPCmdError(kpcStat, nil, buffer);
    Prr = '-';
    goto done;
  }

  result = Atoi(buffer + 3);
done:
  return (result);
}

/************************************************************************
 * PopCapabilities - what can our pop server do?
 ************************************************************************/
void PopCapabilities(TransStream stream, bool *capabilities,
                     SASLEnum *mechPtr) {
  short i;
  unsigned char buffer[256];
  long size;
  unsigned char *spot;
  unsigned char token[32];
  unsigned char service[32];

  for (i = 0; i <= pcapaLimit; i++)
    capabilities[i] = 0;
  *mechPtr = 0;

  if (*GetRString(buffer, POPCmdsStrn + kpcCapa) <= 1)
    return; // hack, but hey
  GetRString(service, K5_POP_SERVICE);

  size = sizeof(buffer);
  if (!POPCmdGetReply(stream, kpcCapa, nil, buffer, &size) && *buffer == '+') {
    capabilities[0] = 1; // we have them!
    for (;;) {
      size = sizeof(buffer);
      if (RecvLine(stream, buffer, &size))
        break;
      if (size > 0 && buffer[size - 1] == '\015')
        buffer[--size] = '\0';
      if (size == 1 && buffer[0] == '.')
        break;
      spot = buffer;
      if (PToken(buffer, token, &spot, " ")) {
        i = FindSTRNIndex(POPCapaStrn, token);
        if (i && i < pcapaLimit)
          capabilities[i] = 1;

        if (i == pcapaSASL) {
          while (PToken(buffer, token, &spot, " "))
            *mechPtr = SASLFind(service, token, *mechPtr);
        }
      }
    }
  }
}

/**********************************************************************
 * POPSasl - do SASL for POP
 **********************************************************************/
int POPSasl(TransStream stream, bool *capabilities, SASLEnum mech,
            unsigned char *buffer, long *size) {
  Accumulator chalAcc, respAcc;
  short rounds = 0;
  long bSize = *size;
  short smtpEquivCode = 501;
  unsigned char service[64];
  long state = 0;
  unsigned char scratch[256];

  Zero(chalAcc);
  Zero(respAcc);

  // put auth command in inital response
  AccuAddRes(&respAcc, EsmtpStrn + esmtpAuth);
  AccuAddChar(&respAcc, ' ');

  // Grab stuff only Kerberos wants
  GetRString(service, K5_POP_SERVICE);

  // run the mechanism
  do {
    // Build the response
    if (SASLDo(service, mech, rounds++, &state, &chalAcc, &respAcc))
      Prr = '-';
    else {
      // Send the response
      if (POPCmdLo(stream, kpcAuth, nil, &respAcc))
        Prr = '-';
      else {
        // get the reply
        bSize = *size;
        Prr = POPGetReplyLo(stream, kpcAuth, buffer, &bSize, &respAcc);
        chalAcc.offset = 0;
        if (!Prr && respAcc.offset && **(unsigned char **)respAcc.data == '+') {
          if (respAcc.offset > 1 &&
              (*(unsigned char **)respAcc.data)[1] != ' ') {
            // We win!  We win!
            Prr = 0;
          } else {
            Prr = ' ';
            AccuAddFromHandle(&chalAcc, respAcc.data, 1, respAcc.offset - 1);
            // clean it out
            respAcc.offset = 0;
          }
        }
      }
    }
  } while (Prr == ' ');

  if (Prr || **(unsigned char **)respAcc.data != '+') {
    (*CurPers)->popSecure = False;
    AccuToStr(&respAcc, scratch);
    POPCmdError(kpcAuth, nil, scratch);
    if (!NoClearPass(capabilities, scratch, *size))
      smtpEquivCode = 535;
  } else
    smtpEquivCode = 237; // auth succeeded!

  // Let the sasl mechanism know how it all came out
  SASLDone(service, mech, rounds, &state, smtpEquivCode);

  do {
    void **_azh = (chalAcc).data;
    if (_azh) {
      if (*_azh)
        free(*_azh);
      free(_azh);
    }
    (chalAcc).data = NULL;
    (chalAcc).offset = (chalAcc).size = 0;
  } while (0);
  do {
    void **_azh = (respAcc).data;
    if (_azh) {
      if (*_azh)
        free(*_azh);
      free(_azh);
    }
    (respAcc).data = NULL;
    (respAcc).offset = (respAcc).size = 0;
  } while (0);

  return Prr;
}

/**********************************************************************
 * NoClearPass - is a pass error one that should not reset the password?
 **********************************************************************/
bool NoClearPass(bool *capabilities, unsigned char *response, short len) {
  unsigned char string[256];
  short i;

  // If the pop server has the AUTH_RESP_CODE caapability,
  // then errors do NOT clear the password unless
  // [auth] appears in them
  if (capabilities && capabilities[pcapaAuthRespCode]) {
    GetRString(string, POP3_AUTH_RESP_CODE);
    return !PFindSub(string, response);
  }

  // without the capability, we have to refer to
  // our list of non-clearing errors
  for (i = 1; *GetRString(string, NoClearPassStrn + i); i++)
    if (PPtrFindSub(string, response, len))
      return (True);

  // Have we ever auth'ed using this password?  If so,
  // let's assume this is a server problem and not an authentication
  // problem.
  if (GetPrefLong(PREF_POP_LAST_AUTH))
    return true;

  // all else as failed.  Sigh.
  return (False);
}

/************************************************************************
 * POPByeBye - tell the POP server we're leaving
 ************************************************************************/
int POPByeBye(TransStream stream) {
  char buffer[CMD_BUFFER];
  long size = sizeof(buffer);
  if (!PopConnected)
    return (noErr);
  if (PrefIsSet(PREF_SLOW_QUIT))
    Prr = POPCmdGetReply(stream, kpcQuit, nil, buffer, &size);
  else {
    Prr = POPCmd(stream, kpcQuit, nil);
    *buffer = '+'; /* fast TCP disconnect */
  }
  return (Prr || *buffer != '+');
}

/************************************************************************
 * POPCmd - Send a command to the POP server
 ************************************************************************/
int POPCmdLo(TransStream stream, short cmd, unsigned char *args,
             AccuPtr argsAcc) {
  char buffer[CMD_BUFFER];
  short err;

  /*
   * reap outstanding commands
   */
  if (!CanPipeline || POPCmds && (*POPCmds)->elCount >= 15) {
    err = ReapCmds(stream, -1);
    if (err == fnfErr)
      err = noErr;
    if (err)
      return (Prr = err);
  }

  if (CanPipeline && cmd == kpcQuit)
    ReapCmds(stream, 0);

  if (cmd)
    GetRString(buffer, POP_STRN + cmd);
  if (cmd == kpcPass || cmd == kpcAuth)
    ProgressMessage(kpMessage, buffer);
  if (cmd == kpcAuth)
    *buffer = 0;
  if (args && *args)
    PCat(buffer, args);
  if (cmd != kpcAuth && cmd != kpcPass && cmd != kpcRetr && cmd != kpcTop)
    ProgressMessage(kpMessage, buffer);
  if (cmd == kpcRetr || cmd == kpcDele)
    Log(LOG_LMOS, buffer);

  if (!argsAcc)
    PCat(buffer, NewLine);
  else if (cmd && cmd != kpcAuth)
    PCatC(buffer, ' ');

  err = SendTrans(stream, buffer + 1, *buffer, nil);
  if (!err && argsAcc) {
    // add a newline to the accumulator
    AccuAddStr(argsAcc, NewLine);

    // send the data
    err = SendTrans(stream, *argsAcc->data, argsAcc->offset, nil);

    // erase what we did to the accumulator
    argsAcc->offset -= *NewLine;
  }

  if (!err && POPCmds)
    err = StackQueue(&cmd, POPCmds);

  return (err);
}

/**********************************************************************
 * ReapCmds - reap commands until the named command is at the top
 *            of the stack, ready to be handled
 **********************************************************************/
OSErr ReapCmds(TransStream stream, short cmd) {
  unsigned char buffer[256];
  long size;
  OSErr err = noErr;
  short thisCmd = 0;

  if (!POPCmds)
    return (noErr);

  while ((*POPCmds)->elCount) {
    if (cmd != -1) {
      StackTop(&thisCmd, POPCmds);
      if (cmd == thisCmd)
        return (noErr);
    }
    StackPop(&thisCmd, POPCmds);
    do {
      size = sizeof(buffer);
      err = RecvLine(stream, buffer, &size);
    } while (!err && *buffer != '+' && *buffer != '-');
    if (thisCmd == cmd)
      return (noErr);
    if (thisCmd == kpcTop || thisCmd == kpcRetr) {
      do {
        size = sizeof(buffer);
        err = RecvLine(stream, buffer, &size);
      } while (!err && !POP_TERM(buffer, size));
    }
    if (cmd == -1)
      return (noErr);
  }

  if (err)
    Prr = err;

  return (err ? err : fnfErr);
}

/************************************************************************
 * POPCmdGetReply - send a POP command and get a reply
 ************************************************************************/
int POPCmdGetReply(TransStream stream, short cmd, unsigned char *args,
                   unsigned char *buffer, long *size) {
  if (cmd >= 0 && (Prr = POPCmd(stream, cmd, args)))
    return (Prr); /* error in transmission */

  return (POPGetReply(stream, cmd, buffer, size));
}

/************************************************************************
 * POPGetReply - get a reply to a POP command
 ************************************************************************/
int POPGetReplyLo(TransStream stream, short cmd, unsigned char *buffer,
                  long *size, AccuPtr resAcc) {
  long rSize;

  // So what's errChar?  Well, two things:
  // 1. Way Back When, when we did serial lines, some POP servers echoed;
  //    errChar is used to detect and ignore echoes.
  // 2. Some POP servers add extraneous blank lines (not that I'm naming
  //    names, but if someone were to nuke a company named "ipswitch", we
  //    might not have this problem anymore), and we can skip those, too
  // So when we see an actual valid POP error indicator (either + or -), we
  // know the true response has begun.  Hence, errChar.
  Byte errChar = 0;

  if (Prr = ReapCmds(stream, cmd))
    return (Prr);
  if (resAcc)
    resAcc->offset = 0;
  do {
    rSize = *size;
    Prr = RecvLine(stream, buffer, &rSize);
    if (!rSize) {
      errChar = '-';
      break;
    }
    if (!errChar && (*buffer == '+' || *buffer == '-'))
      errChar = *buffer;
    if (!Prr && errChar && POPCmds && buffer[rSize - 1] == '\r')
      StackPop(nil, POPCmds);
    if (errChar && resAcc)
      AccuAddPtr(resAcc, buffer, rSize);
  } while (!Prr && !CommandPeriod && !(errChar && buffer[rSize - 1] == '\r'));
  *size = rSize;
  buffer[0] = errChar;
  return (Prr);
}

/************************************************************************
 * POPGetMessage - get a message from the POP server
 ************************************************************************/
int POPGetMessage(TransStream stream, long messageNumber, short *gotSome,
                  POPDHandle popDH, bool *capabilities) {
  char buffer[CMD_BUFFER];
  long size = sizeof(buffer);
  short count;
  TOCType * tocH =
      GetInTOC(); /* shd already be in memory, so NBD to grab it here */
  POPDesc pd;
  long msgsize;
  bool notFetched = False;

  /*
   * if there's no room at all, we won't even try
   */
  if (RoomForMessage(0))
    return (WarnUser(NOT_ENOUGH_ROOM, Prr = dskFulErr));

  pd = popDH->data[messageNumber];
  msgsize = pd.msgSize;

  POPPreFetch(stream, popDH, CanPipeline ? messageNumber + 1 : messageNumber,
              capabilities);

  /*
   * stub or retr
   */
  if (pd.stub) {
    msgsize *= -1; /* let everyone down the line know what's going down */
    size = sizeof(buffer);
    Prr = POPGetReply(stream, kpcTop, buffer, &size);
    NoAttachments = True; /* don't do BinHex */
    RemIdFromPOPD(CUR_POPD_TYPE, DELETE_ID,
                  popDH->data[messageNumber]
                      .uidHash); /* and clear the force del flag if set */
  } else {
    NoAttachments = pd.error ? True : False;
  refetch:
    size = sizeof(buffer);
    Prr = POPGetReply(stream, kpcRetr, buffer, &size);
  }

  if (Prr)
    return (Prr);
  if (*buffer != '+') {
    POPCmdError(kpcRetr, nil, buffer);
    return (Prr = 1);
  }

  /*
   * command issued and accepted - now read the message
   */
  BadBinHex = False;
  BadEncoding = 0;
#ifdef DEBUG
  if (BUG15)
    Dprintf("%d;sc;g", Prr);
#endif
  count = FetchMessageText(stream, msgsize, &pd, messageNumber, nil);
#ifdef DEBUG
  if (BUG15)
    Dprintf("%d;sc;g", Prr);
#endif

  if (CommandPeriod && StackLowErr) {
    StackLowErr = false;
    // increase thread stack size for next try
    if (InAThread()) {
      WarnUser(THREAD_LOW_STACK, 0);
    } else
      // try lowering appllimit for non-threaded operation????
      // ...Will look into if users frequently experience
      WarnUser(LOW_STACK, 0);
  }

  /*
   * did it work?
   */
  if (!Prr && !CommandPeriod) {
    /*
     * ask user what to do about encoding errors
     */
#ifdef BAD_ENCODING_HANDLING
    if (BadBinHex || BadEncoding) {
      pd.delete = False;
      pd.stubbed = True;
      pd.error = True;
      notFetched = True;
    }
#endif
    if (pd.delete) {
      Prr = DeletePOPMessage(stream, messageNumber, pd.uidHash);
      if (!Prr)
        pd.deleted = True;
    }
    if (pd.stub)
      pd.stubbed = True;
    else if (!notFetched && pd.retr) {
      pd.retred = True;
      RemIdFromPOPD(CUR_POPD_TYPE, FETCH_ID, pd.uidHash);
    }
  }

  popDH->data[messageNumber] = pd;

  if (!Prr)
    (*gotSome) += count;
  return (Prr);
}

/************************************************************************
 * DeletePOPMessage - delete a message from the POP server
 ************************************************************************/
OSErr DeletePOPMessage(TransStream stream, short number, long uidHash) {
  unsigned char buffer[256];
  unsigned char args[64];
  long size;

  NumToString(number + 1, args);
  size = sizeof(buffer);
  Prr = POPCmd(stream, kpcDele, args);
  if (!Prr)
    RemIdFromPOPD(CUR_POPD_TYPE, DELETE_ID, uidHash);
  return (Prr);
}

/************************************************************************
 * FillSizesWithList - fill message sizes with the LIST command
 ************************************************************************/
OSErr FillSizesWithList(TransStream stream, POPDHandle popDH) {
  unsigned char buffer[128];
  long size = sizeof(buffer);
  short msgNum;
  unsigned char *spot;
  short n = popDH->count;

  if (Prr = POPCmdGetReply(stream, kpcList, nil, buffer, &size))
    return (Prr);

  if (*buffer != '+') {
    Prr = *buffer;
    POPCmdError(kpcList, nil, buffer);
    return (Prr);
  }

  //	if (n>100) ByteProgress(nil,0,n);

  for (size = sizeof(buffer);
       !(Prr = RecvLine(stream, buffer, &size)) && !POP_TERM(buffer, size);
       size = sizeof(buffer)) {
    CycleBalls();
    if (CommandPeriod)
      break;

    spot = strtok(buffer, " \t");
    if (!spot)
      continue;
    msgNum = Atoi(spot);

    spot = strtok(nil, " \t");
    if (!spot)
      continue;
    size = Atoi(spot);

    if (msgNum < 1 || msgNum > n)
      continue;

    //		if (n>100) ByteProgress(nil,-1,0);

    popDH->data[msgNum - 1].msgSize = size;
  }
  if (CommandPeriod && !Prr)
    Prr = userCancelled;

  //	if (n > 100 && !Prr) ByteProgress(nil,1,1);
  Progress(NoBar, 0, nil, nil, nil);

  return (Prr);
}

/************************************************************************
 * POPCmdError - report an error for an POP command
 ************************************************************************/
int POPCmdError(short cmd, unsigned char *args, unsigned char *message) {
  unsigned char theCmd[256];
  unsigned char theError[256];
  int err;

  *theCmd = 0;
  GetRString(theCmd, POP_STRN + cmd);
  if (args && *args)
    PCat(theCmd, args);
  strcpy(theError, message);
  {
    int len = strlen(theError);
    if (len > 0 && theError[len - 1] == '\012')
      theError[--len] = '\0';
    if (len > 0 && theError[len - 1] == '\015')
      theError[--len] = '\0';
  }

  if (InAThread()) {
    char c_cmd[256], c_err[256];
    int len;
    
    len = (unsigned char)theCmd[0];
    if (len > 255) len = 255;
    memcpy(c_cmd, theCmd + 1, len);
    c_cmd[len] = '\0';

    len = (unsigned char)theError[0];
    if (len > 255) len = 255;
    memcpy(c_err, theError + 1, len);
    c_err[len] = '\0';

    AddTaskErrorsS(c_cmd, c_err, CheckingTask, (*CurPers)->persId);
    return 1;
  }

  MyParamText(theCmd, theError, (UPtr)"POP", (UPtr)"");
  err = ReallyDoAnAlert(PROTO_ERR_ALRT, Note);
  return (err);
}

/************************************************************************
 * FetchMessageText - read in the body of a message
 ************************************************************************/
int FetchMessageText(TransStream stream, long estSize, POPDPtr pdp,
                     short messageNumber, TOCType * useTocH) {
  return (FetchMessageTextLo(stream, estSize, pdp, messageNumber, useTocH,
                             false, false));
}

/************************************************************************
 * FetchMessageText - read in the body of a message
 ************************************************************************/
int FetchMessageTextLo(TransStream stream, long estSize, POPDPtr pdp,
                       short messageNumber, TOCType * useTocH, bool imap,
                       bool import) {
  unsigned char *text = nil;
  TOCType * tocH;
  MSumType sum;
  long eof, chopHere;
  unsigned char name[256];
  short count = 0, part;
  HeaderDHandle hdh = nil;
  LineIOD lid;
  OSErr err;
  FSSpec spec;
  extern OSErr ImportErr;
  unsigned char savedSub[64];

  /*
   * make the message summary
   */
  WriteZero(&sum, sizeof(MSumType));
  *savedSub = 0;

  /*
   * haven't seen any rich text yet
   */
  AnyRich = AnyHTML = AnyFlow = AnyCharset = AnyDelSP = False;
  if (LastAttPath) { free(LastAttPath); LastAttPath = NULL; } /* or attachments */
  ETLDeleteRequest =
      False; /* and no translators have been run on this message yet */

  /*
   * grab the destination mailbox (usually "In")
   */
  tocH = useTocH ? useTocH : GetInTOC();
  if (!tocH) {
    Prr = -108;
    return (0);
  }
  spec = GetMailboxSpec(tocH, -1);
  PCopy(name, spec.name);

  // if we're adding IMAP messages or importing mail, we've taken care of
  // opening the mailbox already
  if (!imap && !import) {
    Prr = BoxFOpen(tocH);
    if (Prr) {
      FileSystemError(OPEN_MBOX, name, Prr);
      goto done;
    }
  }

  eof = FindTOCSpot(tocH, estSize);

  Prr = SetFPos(tocH->refN, fsFromStart, eof);
  if (Prr) {
    FileSystemError(WRITE_MBOX, name, Prr);
    goto done;
  }

#ifdef DEBUG ////////////////////////////
  if (BUG15)
    Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////

  count = SaveAndSplit(stream, tocH->refN, estSize, &hdh, tocH->imapTOC);

#ifdef DEBUG ////////////////////////////
  if (BUG15)
    Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////

done:
  if (!Prr && !GetFPos(tocH->refN, &chopHere))
    SetEOF(tocH->refN, chopHere);

  // if we're adding IMAP messages, or importing mail, we'll close and flush
  // later.
  if (Prr || (!imap && !import)) {
    BoxFClose(tocH, true);
    if (Prr || !count)
      return (0);
  }

  /*
   * now, read it back from the file
   */
  if (Prr = OpenLine(spec.path, (imap || import) ? fsRdPerm : fsRdWrPerm,
                     &lid)) {
    FileSystemError(READ_MBOX, name, Prr);
    return (0);
  }
  if (Prr = SeekLine(eof, &lid)) {
    FileSystemError(READ_MBOX, name, Prr);
    return (0);
  }

  ReadSum(nil, False, &lid, True);
  for (part = 1; !(err = ReadSum(&sum, False, &lid, True)); part++) {
    if (!*savedSub)
      PSCopy(savedSub, sum.subj);
    if (part == 1 && pdp) {
      FillPOPD(pdp, hdh);
      DBNoteUIDHash(sum.uidHash, pdp->uidHash);
      sum.uidHash = pdp->uidHash;
    } else {
      DBNoteUIDHash(sum.uidHash, kNoMessageId);
      sum.uidHash = kNoMessageId;
    }
    if (!(*hdh)->isMIME) {
      TransLitString(sum.from);
      TransLitString(sum.subj);
    } else
      sum.tableId = ViewTable(hdh);
    if (part == 1)
      sum.msgIdHash = (*hdh)->msgIdHash;
    if (!ValidHash(sum.uidHash))
      sum.uidHash = sum.msgIdHash;
#ifdef BAD_ENCODING_HANDLING
    if ((estSize < 0) || BadBinHex || BadEncoding)
      sum.flags |= FLAG_SKIPPED;
#else
    if ((estSize < 0))
      sum.flags |= FLAG_SKIPPED;
#endif

    // set or clear html/enriched flags.  Clear is necessary because toc build
    // might give false positive
    if (count == 1 && AnyRich)
      sum.flags |= FLAG_RICH;
    else
      sum.flags &= ~FLAG_RICH;
    if (count == 1 && AnyHTML)
      sum.opts |= OPT_HTML;
    else
      sum.opts &= ~OPT_HTML;
    if (count == 1 && AnyFlow)
      sum.opts |= OPT_FLOW;
    else
      sum.opts &= ~OPT_FLOW;
    if (count == 1 && AnyDelSP)
      sum.opts |= OPT_DELSP;
    else
      sum.opts &= ~OPT_DELSP;
    if (count == 1 && AnyCharset)
      sum.opts |= OPT_CHARSET;
    else
      sum.opts &= ~OPT_CHARSET;
    if ((*hdh)->hasMDN)
      sum.opts |= OPT_RECEIPT;
    if (LastAttPath)
      sum.flags |= FLAG_HAS_ATT;
    if (!sum.seconds)
      sum.seconds = GMTDateTime();
    if (count > 1)
      StampPartNumber(&sum, part, count);
    sum.spamScore = 0;
    sum.arrivalSeconds = GMTDateTime();
    if (Prr)
      break;
    if (useTocH && !import) // create a new summary if we're importing
    {
      MSumType newSum;

      newSum = tocH->sums[messageNumber];
      newSum.offset = sum.offset;
      newSum.length = sum.length;
      newSum.bodyOffset = sum.bodyOffset;
      newSum.flags |= sum.flags;
      newSum.opts |= sum.opts;
      newSum.msgIdHash = sum.msgIdHash;
      if (!newSum.priority)
        newSum.priority = sum.priority;
      PSCopy(newSum.subj, sum.subj);
      if (!(newSum.opts & OPT_IMAP_SENT))
        PSCopy(newSum.from, sum.from);
      RemoveUTF8FromSum(&newSum);
      tocH->sums[messageNumber] = newSum;
    } else if (!SaveMessageSum(&sum, &tocH)) {
      if (import)
        ImportErr = memFullErr; // stop if we're importing.
      break;
    }
  }
  ReadSum(nil, False, &lid, True);
  if (err != fnfErr)
    Prr = err;
  Prr = Prr || part <= count;
  ZapHeaderDesc(hdh);

  CloseLine(&lid);

#ifdef DEBUG
  if (ETLDeleteRequest)
    ComposeLogS(LOG_PLUG, nil,
                (UPtr)"A plugin has requested the deletion of '%p' in '%p'",
                savedSub, spec.name);
#endif

  if (BadBinHex || BadEncoding) {
    unsigned char hex[256], enc[256], errorStr[256];

    if (BadBinHex)
      GetRString(hex, BAD_HEX_MSG);
    else
      *hex = 0;
    if (BadEncoding)
      ComposeRString(enc, BAD_ENC_MSG, BadEncoding, BadEncoding);
    else
      *enc = 0;

    ComposeRString(errorStr,
                   (imap ? IMAP_BAD_HEXBIN_ERR_TEXT : BAD_HEXBIN_ERR_TEXT), hex,
                   enc);

    if (imap) // add the message error to the IMAP message we just downloaded
      AddMesgError(tocH, messageNumber, errorStr, -1);
    else // add it to the last message added to the mailbox.  OK for POP.
      AddMesgError(tocH, tocH->count - 1, errorStr, -1);
  }

  if (Prr) {
    WarnUser(READ_MBOX, Prr);
    return (0);
  }

  count = part - 1;

  InvalSum(tocH, useTocH ? messageNumber : tocH->count - 1);
  if (!PrefIsSet(PREF_CORVAIR) && !(tocH->count % 5)) {
    Prr = WriteTOC(tocH);
    FlushVol(nil, spec.vRefNum);
  }
  MakeMessTitle(name, tocH, useTocH ? messageNumber : tocH->count - count,
                False);
  ComposeLogR(LOG_RETR, nil, MSG_GOT, name, count);
  if (!imap)
    UpdateNumStatWithTime(
        kStatReceivedMail, 1,
        tocH->sums[useTocH ? messageNumber : tocH->count - 1].seconds +
            ZoneSecs());
  return (Prr ? 0 : count);
}

/************************************************************************
 * SaveAndSplit - read a message, (possibly) splitting it into parts and
 * saving it.
 ************************************************************************/
int SaveAndSplit(TransStream stream, short refN, long estSize,
                 HeaderDHandle *hdhp, bool isIMAP) {
  unsigned char buf[256];
  short count = 0;
  HeaderDHandle hdh = NewHeaderDesc(nil);
  long fromSize;
  long end;
  long oldStart;
  short lastHeaderTokenType;
  long ticks = TickCount();

  if (!hdh) {
    Prr = MemError();
    return (0);
  }

  //	if (estSize>0) ByteProgress(nil,0,estSize);

  if (Prr = PutOutFromLine(refN, &fromSize))
    return (0);

reRead:
  if (!hdh) {
    Prr = MemError();
    return (0);
  }

  lastHeaderTokenType = ReadHeader(stream, hdh, estSize, refN, False);

#ifdef DEBUG ////////////////////////////
  if (BUG15)
    Dprintf("%d;sc;g", lastHeaderTokenType);
#endif // DEBUG //////////////////////////

  if (lastHeaderTokenType != EndOfHeader &&
      lastHeaderTokenType != EndOfMessage) {
    Prr = 1;
    goto done;
  }

  if (fromSize) {
    (*hdh)->diskStart -=
        fromSize; /* count the envelope as part of the header */
    fromSize = 0; /* in case we pass this way again. */
  } else
    (*hdh)->diskStart = oldStart;

  if (!Prr) {
    /*
     * I've wanted to do this for years.  say who it's from!
     */
    PCopy(buf, (*hdh)->who);
    { size_t _l = strlen((const char *)buf); if (_l > 31) buf[31] = '\0'; } // not too long here...
    PCatC(buf, ',');
    PCatC(buf, ' ');
    PSCat(buf, (*hdh)->subj);
    if (!(*hdh)->isMIME)
      TransLitString(buf);
    ProgressMessage(kpMessage, buf);

    // regenerate full info for comment
    PCopy(buf, (*hdh)->who);
    PCatC(buf, ',');
    PCatC(buf, ' ');
    PSCat(buf, (*hdh)->subj);
    PSCopy((*hdh)->summaryInfo, buf);

    /*
     * now, go save the body
     */
    if (lastHeaderTokenType != EndOfMessage)
      ReadEitherBody(stream, refN, hdh, buf, sizeof(buf), estSize,
                     EMSF_ON_ARRIVAL);
    else
      { long _len = 2; AWrite(refN, &_len, "\015\015"); }

#ifdef DEBUG ////////////////////////////
    if (BUG15)
      Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////

    /*
     * darn encapsulated stuf
     */
    if (Prr == '82') {
      oldStart = (*hdh)->diskStart;
      ZapHeaderDesc(hdh);
      hdh = NewHeaderDesc(nil);
      PSCopy((*hdh)->summaryInfo, buf);
      Prr = noErr;
      goto reRead;
    }

    /*
     * ok, got real body now
     */
#ifdef DEBUG ////////////////////////////
    if (BUG15)
      Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////

    EnsureNewline(refN);
    ticks = TickCount() - ticks + 1;
    {
      long rate = (estSize * 600) / (ticks * 1024);
      ComposeLogS(LOG_TPUT, nil, (UPtr)"%dK in %d.%d sec; %d.%d KBps",
                  estSize / 1024, ticks / 60, (ticks / 6) % 10, rate / 10,
                  rate % 10);
    }
#ifdef DEBUG ////////////////////////////
    if (BUG15)
      Dprintf("%d %d;file %x;sc;g", Prr, GetFPos(refN, &end), refN);
#endif // DEBUG //////////////////////////
    if (!Prr && !(Prr = GetFPos(refN, &end))) {
#ifdef DEBUG ////////////////////////////
      if (BUG15)
        Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////
      TruncOpenFile(refN, end);
#ifdef DEBUG ////////////////////////////
      if (BUG15)
        Dprintf("%d e %d ds %d st %d;sc;g", Prr, end, (*hdh)->diskStart,
                GetRLong(SPLIT_THRESH));
#endif // DEBUG //////////////////////////
      if (!isIMAP && (end - (*hdh)->diskStart > GetRLong(SPLIT_THRESH)))
        count = SplitMessage(refN, (*hdh)->diskStart, (*hdh)->diskEnd, end);
      else
        count = 1;
#ifdef DEBUG ////////////////////////////
      if (BUG15)
        Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////
    }
  }

#ifdef DEBUG ////////////////////////////
  if (BUG15)
    Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////

done:
  *hdhp = hdh;
  if (Prr)
    return (0);
  return (estSize < 0 && GetPrefLong(PREF_POP_MODE) == popRStatus &&
                  *(*hdh)->status
              ? 0
              : count);
}

/************************************************************************
 * ReadEitherBody - read the body of a message from a pop-3 server
 ************************************************************************/
short ReadEitherBody(TransStream stream, short refN, HeaderDHandle hdh,
                     char *buf, long bSize, long estSize, long context) {
  MIMESHandle mimeSList = nil;
  BoundaryType endType;

  /*
   * is our MIME converter interested in this thing?
   */
  if (!NoAttachments && estSize >= 0) {
    mimeSList = NewMIMES(stream, hdh, False, context);
    if (!mimeSList)
      return (Prr = MemError());
    if (mimeSList == kMIMEBoring)
      mimeSList = nil;
    else if ((*mimeSList)->readBody == READ_MESSAGE) {
      ZapMIMES(mimeSList);
      return (Prr = '82');
    }
  }

  /*
   * call the proper body reading function
   */
  endType = mimeSList ? (*(*mimeSList)->readBody)(stream, refN, mimeSList, buf,
                                                  bSize, ReadPOPLine)
                      : ReadPlainBody(stream, refN, buf, bSize, estSize);

  if (endType == btError)
    Prr = 1;
#ifdef DEBUG ////////////////////////////
  if (BUG15)
    Dprintf("%d;sc;g", Prr);
#endif // DEBUG //////////////////////////
  ZapMIMES(mimeSList);
  return (Prr);
}

/**********************************************************************
 * RoomForMessage - make sure there's room for a message on both the
 *  attachments folder volume and the in box volume
 **********************************************************************/
OSErr RoomForMessage(long msgsize) {
  FSSpec attSpec;
  OSErr err = noErr;

  GetAttFolderSpec(&attSpec);
  err = VolumeMargin(MailRoot.vRef, msgsize);
  if (!err && MailRoot.vRef == attSpec.vRefNum)
    return (noErr);
  if (!err)
    err = VolumeMargin(attSpec.vRefNum, msgsize);

  return (err);
}

/************************************************************************
 * ReadPlainBody - handle the body of a non-MIME message.
 ************************************************************************/
POPLineType ReadPlainBody(TransStream stream, short refN, char *buf, long bSize,
                          long estSize) {
  long size;
  bool hexing, singling, pgping;
  POPLineType lineType;
#ifdef OLDPGP
  PGPContext pgp;
#endif

  /*
   * prepare converters
   */
  if (!NoAttachments) {
    BeginHexBin(nil);
    BeginAbomination("", nil);
#ifdef OLDPGP
    BeginPGP(&pgp);
#endif
    hexing = singling = pgping = False;
  }
  ReadPOPLine(stream, nil, 0, nil);

  /*
   * main processing loop
   */
  for (lineType = ReadPOPLine(stream, buf, bSize, &size);
       lineType != plError && lineType != plEndOfMessage;
       lineType = ReadPOPLine(stream, buf, bSize, &size)) {
    /*
     * give each converter a crack at the line
     */
    if (!NoAttachments) {
      if (!(singling || pgping))
        hexing = ConvertHexBin(refN, buf, &size, lineType, estSize);
      if (!(hexing || pgping))
        singling =
            ConvertUUSingle(refN, buf, &size, lineType, estSize, nil, nil);
#ifdef OLDPGP
      if (!(singling || hexing))
        pgping = ConvertPGP(refN, buf, &size, lineType, estSize, &pgp);
#endif
    }

    /*
     * write the line
     */
    if (size && (Prr = AWrite(refN, &size, buf)))
      break;
  }

  /*
   * record skipped message
   */
  if (!Prr && lineType == plEndOfMessage && estSize < 0) {
    unsigned char msg[256];
    long count;
    if (Headering || PrefIsSet(PREF_NO_BIGGIES))
      ComposeRString(msg, BIG_MESSAGE_MSG2, -estSize);
    else
      ComposeRString(msg, NOSPACE_SKIP, -estSize);
    count = strlen((const char *)msg);
    Prr = AWrite(refN, &count, msg);
  }

  /*
   * close converters
   */
  if (!NoAttachments) {
    EndHexBin();
    SaveAbomination(nil, 0);
#ifdef OLDPGP
    EndPGP(&pgp);
#endif

    /*
     * write attachment notes, if any
     */
    WriteAttachNote(refN);
  }

  /*
   * report error (if any)
   */
  if (Prr)
    FileSystemError(WRITE_MBOX, "", Prr);
  //	else if (!UUPCIn) Progress(100,NoChange,nil,nil,nil);

  return (Prr ? btError : btEndOfMessage);
}

/************************************************************************
 * PutOutFromLine - write an envelope
 ************************************************************************/
int PutOutFromLine(short refN, long *fromLen) {
  unsigned char fromLine[256];
  long len;

  *fromLen = len = SumToFrom(nil, fromLine);
  if (Prr = AWrite(refN, &len, fromLine))
    return (FileSystemError(WRITE_MBOX, "", Prr));
  return (noErr);
}

/************************************************************************
 * DupHeader - copy the header of a split message
 ************************************************************************/
int DupHeader(short refN, unsigned char *buff, long bSize, long offset,
              long headerSize) {
  long currentOffset;
  long readBytes, writeBytes;
  long copied;

  if (Prr = GetFPos(refN, &currentOffset))
    return (FileSystemError(READ_MBOX, "", Prr));
  for (copied = 0; copied < headerSize; copied += readBytes) {
    if (Prr = SetFPos(refN, fsFromStart, offset + copied))
      return (FileSystemError(READ_MBOX, "", Prr));
    readBytes = bSize < headerSize - copied ? bSize : headerSize - copied;
    if (Prr = ARead(refN, &readBytes, buff))
      return (FileSystemError(READ_MBOX, "", Prr));
    if (Prr = SetFPos(refN, fsFromStart, currentOffset))
      return (FileSystemError(WRITE_MBOX, "", Prr));
    writeBytes = readBytes;
    if (Prr = FSZWrite(refN, &writeBytes, buff))
      return (FileSystemError(WRITE_MBOX, "", Prr));
    currentOffset += writeBytes;
  }
  return (noErr);
}

/************************************************************************
 * FirstUnread - find the first unread message
 *	 We do try to be clever about it.
 ************************************************************************/
int FirstUnread(TransStream stream, int count) {
  short first, theLast, on;
  static short lastCount = 0;
  bool hasBeen;

  /*
   * give LAST a whirl
   */
  if (POPLast(stream, &theLast))
    return (count + 1);
  theLast = MAX(theLast, 0);
  theLast = MIN(theLast, count);

  /*
   * if LAST returns nonzero, believe it
   * Also believe it if the user tells us to believe it
   */
  if (theLast || GetPrefLong(PREF_POP_MODE) == popRLast)
    return (theLast + 1);

  /*
   * LAST was a dead end.  Do it the hard way
   */
  on = lastCount;
  lastCount = count;

#define SETHASBEEN(o, c)                                                       \
  do {                                                                         \
    hasBeen = HasBeenRead(stream, o, c);                                       \
    if (CommandPeriod)                                                         \
      return (c + 1);                                                          \
  } while (0)
  if (PrefIsSet(PREF_NO_BIGGIES)) {
    /* Heuristics */
    if (on && on <= count) {
      SETHASBEEN(on, count);
      if (hasBeen) {
        SETHASBEEN(on + 1, count);
        if (!hasBeen)
          return (on + 1);
      }
    }
    SETHASBEEN(count, count);
    if (hasBeen)
      return (count + 1);
    if (count == 1)
      return (1);

    /* search... */
    for (on = count - 1; on; on--) {
      SETHASBEEN(on, count);
      if (hasBeen)
        break;
    }
    return (on + 1);
  } else {
    first = 1;
    theLast = count;
    /*
     * try to cut the search short via heuristics
     */
    if (on && on <= count) {
      SETHASBEEN(on, count);
      if (hasBeen) {
        if (on < count) {
          SETHASBEEN(++on, count);
          if (!hasBeen)
            return (on);
        }
        first = on + 1;
      } else
        theLast = on - 1;
    } else {
      SETHASBEEN(count, count);
      if (hasBeen)
        return (count + 1);
      SETHASBEEN(1, count);
      if (count == 1 || !hasBeen)
        return (1);
      theLast = count - 1;
      first = 2;
      on = count;
      hasBeen = False;
    }

    /*
     * hi ho, hi ho, it's off to search we go
     */
    while (first <= theLast) {
      on = (first + theLast) / 2;
      SETHASBEEN(on, count);
      if (hasBeen)
        first = on + 1;
      else
        theLast = on - 1;
    }
    if (!hasBeen)
      return (on);
    else
      return (on + 1);
  }
}

/************************************************************************
 * HasBeenRead - has a particular message been read?
 * look for a "Status:" header; if it's "Status: R<something>", message
 * has been read
 ************************************************************************/
bool HasBeenRead(TransStream stream, short msgNum, short count) {
  unsigned char scratch[128];
  bool unread = False, statFound = False;
  unsigned char terminate[32];
  unsigned char status[32];
  unsigned char *cp;
  long size;

  if (msgNum > count)
    return (0);
  Progress((msgNum * 100) / count, NoBar, nil,
           GetRString(scratch, FIRST_UNREAD), nil);
  GetRString(terminate, ALREADY_READ);
  GetRString(status, STATUS);
  NumToString(msgNum, scratch);
  PLCat(scratch, 1);
  POPCmd(stream, kpcTop, scratch);
  for (size = sizeof(scratch);
       !(Prr = RecvLine(stream, scratch, &size)) && !POP_TERM(scratch, size);
       size = sizeof(scratch))
    if (!unread && !statFound && !striscmp(scratch, status)) {
      statFound = True;
      for (cp = scratch; cp < scratch + size; cp++) {
        if (*cp == ':') {
          for (cp++; cp <= scratch + size - strlen(terminate); cp++)
            if (!striscmp(cp, terminate))
              break;
          unread = cp > scratch + size - strlen(terminate);
          break;
        }
      }
    }
  ComposeLogS(LOG_LMOS, nil, (UPtr)"HasBeenRead: %d sf %d un %d %p", msgNum,
              statFound, !unread, statFound && !unread ? (UPtr)"READ" : (UPtr)"UNREAD");
  return (statFound && !unread);
}

/************************************************************************
 * StampPartNumber - put the part number on a mail message
 ************************************************************************/
void StampPartNumber(MSumPtr sum, short part, short count) {
  char *spot;
  short i, digits, len;

  if (part == 1)
    sum->flags |= FLAG_FIRST;
  else
    sum->flags |= FLAG_SUBSEQUENT;

  for (i = count, digits = 0; i; i /= 10, digits++)
    ;
  len = 2 * digits + 2;
  spot = sum->subj + MIN(*sum->subj + len, sizeof(sum->subj) - 1);
  *sum->subj = spot - sum->subj;
  for (i = digits; i; i--, count /= 10)
    *spot-- = '0' + count % 10;
  *spot-- = '/';
  for (i = digits; i; i--, part /= 10)
    *spot-- = '0' + part % 10;
  *spot = ' ';
}

/************************************************************************
 * RecordAttachment - note that we've attached a file
 ************************************************************************/
OSErr RecordAttachment(const char *path, HeaderDHandle hdh) {
  unsigned char theMessage[256];
  OSErr err;
  bool deleted = false;
  FSSpec tmpSpec = {0};

  /* Build a temporary FSSpec from the path for functions that still need one */
  strncpy(tmpSpec.path, path, sizeof(tmpSpec.path) - 1);
  {
    char pathCopy[1024];
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';
    const char *base = basename(pathCopy);
    strncpy(tmpSpec.name, base, sizeof(tmpSpec.name) - 1);
  }

  if (FileTypeOf(&tmpSpec) == REG_FILE_TYPE) {
    if (!hdh || AAFetchResData((*hdh)->contentAttributes,
                               AttributeStrn + aRegFile, theMessage)) {
      remove(path);
      deleted = true;
      GetRString(theMessage, STOLEN_REG_FILE);
    } else {
      /* gRegFiles registration file tracking - legacy feature, no-op */
      (void)tmpSpec;
    }
  }

  // Long filename?
  if (hdh && !(*hdh)->relatedPart)
    FixLongFilename(hdh, path);

  /*
   * update global path record for last attachment
   */
  if (!deleted) {
    if (LastAttPath) free(LastAttPath);
    LastAttPath = strdup(path);
  }

  // only record top-level files
  if (AttFolderStack && !SameSpec(&CurrentAttFolderSpec, &AttFolderSpec))
    return noErr;

  if (!deleted) {
    if (hdh && (*hdh)->relatedPart)
      RelatedNote(path, hdh, (const char *)theMessage);
    else
      AttachNoteLo(path, (const char *)theMessage);
  }

  /*
   * tack on the note
   */
  if (GrowBuf_Append(&AttachedFiles, theMessage, strlen((const char *)theMessage))) {
    err = memFullErr;
  }
  if (err) {
    WarnUser(BINHEX_MEM, err);
    CommandPeriod = true;
    return (err);
  }

  if (deleted)
    return (noErr);

  RecordTransAttachments(path);

  /*
   * add a comment?
   */
  if (hdh && !PrefIsSet(PREF_NO_ATT_COMMENT))
    DTSetComment(&tmpSpec, PCopy(theMessage, (*hdh)->summaryInfo));

  /*
   * is there a date?
   */
  if (hdh && !AAFetchResData((*hdh)->contentAttributes,
                             AttributeStrn + aModDate, theMessage)) {
    uint32_t mod;
    long zone;

    if (mod = BeautifyDate(theMessage, &zone))
      if (mod > (long)GetRLong(TOO_EARLY_FILE))
        AFSpSetMod(&tmpSpec, mod + ZoneSecs());
  }

  //	record for attachment received statistics
  UpdateNumStatWithTime(kStatReceivedAttach, 1,
                        hdh ? (*hdh)->gmtSecs + ZoneSecs() : LocalDateTime());

  return (noErr);
}

/************************************************************************
 * FixLongFilename - attach a long filename to an FSSpec made from a shorter
 *  filename
 ************************************************************************/
OSErr FixLongFilename(HeaderDHandle hdh, const char *path) {
  unsigned char longFilename[128];
  unsigned char filenameAtt[32];
  unsigned char part[256];
  unsigned char charset[256];
  FSSpec tmpSpec = {0};

  /* Build a temporary FSSpec from the path for functions that still need one */
  strncpy(tmpSpec.path, path, sizeof(tmpSpec.path) - 1);
  {
    char pathCopy[1024];
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';
    const char *base = basename(pathCopy);
    strncpy(tmpSpec.name, base, sizeof(tmpSpec.name) - 1);
  }

  *longFilename = *charset = 0;

  // is there a filename at all?
  if (AAFetchData((*hdh)->contentAttributes,
                  GetRString(filenameAtt, AttributeStrn + aFilename),
                  longFilename)) {
    // no "filename".  Is there a "filename*"?
    PCatC(filenameAtt, '*');
    if (!AAFetchData((*hdh)->contentAttributes, filenameAtt, part))
      Un2184Append(longFilename, sizeof(longFilename), part, charset, true);
    else {
      // no "filename*'.
      short i;
      for (i = 0;; i++) {
        // Is there a "filename*0"?
        if (!AAFetchData(
                (*hdh)->contentAttributes,
                ComposeRString(filenameAtt, AttributeStrn + aFilename, i),
                part)) {
          if (i == 0)
            GetRString(charset, UNSPECIFIED_CHARSET);
          Un2184Append(longFilename, sizeof(longFilename), part, charset,
                       false);
        }
        // Is there a "filename*0*"?
        else if (!AAFetchData((*hdh)->contentAttributes,
                              PCat(filenameAtt, (UPtr)"*"), part))
          Un2184Append(longFilename, sizeof(longFilename), part, charset, true);
        else
          break;
      }
    }
  }

  // check for applesingle
  if (EqualStrRes((*hdh)->contentSubType, MIME_APPLEFILE))
    if (longFilename[0] == '%' && strlen(longFilename) > 1)
      BMD(longFilename + 1, longFilename, strlen(longFilename));

  // is it a short filename?
  if (strlen(longFilename) <= 31 && !AnyHighBits(longFilename, strlen(longFilename)))
    return noErr;

  // Ok, it's a long filename.  Set the name of the file to it
  //	Make sure that the file is unique
  (void)MakeUniqueLongFileName(tmpSpec.vRefNum, tmpSpec.parID, longFilename,
                               kTextEncodingUnknown, sizeof(longFilename) - 1);
  return FSpSetLongName(&tmpSpec, kTextEncodingUnknown, longFilename, &tmpSpec);
}

/************************************************************************
 * Un2184Append - undo some 2184, and append to an existing string
 ************************************************************************/
PStr Un2184Append(PStr dest, short sizeofDest, PStr orig, PStr charset,
                  bool isEncoded) {
  unsigned char decoded[256];
  short spaceNeeded;
  short leftAppend = GetRLong(MIN_LEFT_APPEND);
  unsigned char elide[32];

  if (isEncoded)
    Un2184(decoded, orig, charset);
  else
    PCopy(decoded, orig);

  spaceNeeded = 1 + *dest + *decoded - sizeofDest;
  if (spaceNeeded > 0) {
    // add in space for the ellipsis
    spaceNeeded += *GetRString(elide, ELIDE_2184_STRING);
    // Make sure we leave at least 64 characters of the original string
    if (*dest > leftAppend) {
      // we can get all we want from the excess of dest
      if (spaceNeeded <= *dest - leftAppend)
        *dest -= spaceNeeded;
      else {
        // take some from dest...
        spaceNeeded -= *dest - leftAppend;

        // and take the rest from the RIGHT side of decoded
        BMD(decoded + 1, decoded + 1 + spaceNeeded, *decoded - spaceNeeded);
        *decoded -= spaceNeeded;
      }
    }

    // copy the elision string
    PCat(dest, elide);
  }

  // copy the decoded string
  PCat(dest, decoded);

  return dest;
}

/************************************************************************
 * Un2184 - undo 2184 encoding
 ************************************************************************/
PStr Un2184(PStr dest, PStr orig, PStr charset) {
  short tableID;
  bool found;

  // copy it in
  PCopy(dest, orig);

  // do we have a charset?
  if (!*charset) {
    if (PIndex(orig, '\'')) {
      unsigned char *spot = orig;
      PToken(orig, charset, &spot, "'"); // grab charset
      PToken(orig, dest, &spot, "'");    // skip language
      PToken(orig, dest, &spot, "'");    // put the rest into dest
    }
    if (!*charset)
      GetRString(charset, UNSPECIFIED_CHARSET);
  }

  // undo the QP
  FixURLString(dest);

  // transliterate
  tableID = FindMIMECharsetLo(charset, &found);
  if (!found)
    tableID = FindMIMECharset(GetRString(charset, UNSPECIFIED_CHARSET));
  TransLitRes(dest + 1, *dest, tableID);

  return dest;
}

/************************************************************************
 * AttachNoteLo - format the attachment note
 ************************************************************************/
void AttachNoteLo(const char *path, const char *theMessage) {
  unsigned char folderName[32];
  unsigned char typeString[16], creatorString[16];
  FInfo info;
  long fid;
  unsigned char fidStr[32];
  FSSpec tmpSpec = {0};
  char pathCopy[1024];

  /* Build a temporary FSSpec from the path for functions that still need one */
  strncpy(tmpSpec.path, path, sizeof(tmpSpec.path) - 1);
  strncpy(pathCopy, path, sizeof(pathCopy) - 1);
  pathCopy[sizeof(pathCopy) - 1] = '\0';
  {
    const char *base = basename(pathCopy);
    strncpy(tmpSpec.name, base, sizeof(tmpSpec.name) - 1);
  }

  if (FSpIsItAFolder(&tmpSpec)) {
    info.fdCreator = kSystemIconsCreator;
    info.fdType = kGenericFolderIcon;
    fid = SpecDirId(&tmpSpec);
  } else {
    FSpGetFInfo(&tmpSpec, &info);
    FSMakeFID(&tmpSpec, &fid);
  }

  BMD(&info.fdCreator, creatorString, 4);
  creatorString[4] = '\0';
  BMD(&info.fdType, typeString, 4);
  typeString[4] = '\0';
  SanitizeFN(creatorString, creatorString, ATTCONV_BAD_CHARS, ATTCONV_REP_CHARS,
             false);
  SanitizeFN(typeString, typeString, ATTCONV_BAD_CHARS, ATTCONV_REP_CHARS,
             false);
  GetMyVolName(tmpSpec.vRefNum, folderName);
  /* theMessage is output parameter - cast to unsigned char* for ComposeRString */
  ComposeRString((unsigned char *)theMessage, FILE_FOLDER_FMT, folderName,
                 tmpSpec.name, typeString, creatorString, Long2Hex(fidStr, fid));
}

/************************************************************************
 * RelatedNote - format the related note
 ************************************************************************/
void RelatedNote(const char *path, HeaderDHandle hdh, const char *theMessage) {
  unsigned char folderName[32];
  long fid;
  unsigned char quoteName[256];
  FSSpec tmpSpec = {0};
  char pathCopy[1024];

  /* Build a temporary FSSpec from the path for functions that still need one */
  strncpy(tmpSpec.path, path, sizeof(tmpSpec.path) - 1);
  strncpy(pathCopy, path, sizeof(pathCopy) - 1);
  pathCopy[sizeof(pathCopy) - 1] = '\0';
  {
    const char *base = basename(pathCopy);
    strncpy(tmpSpec.name, base, sizeof(tmpSpec.name) - 1);
  }

  FSMakeFID(&tmpSpec, &fid);
  strncpy((char *)quoteName, tmpSpec.name, sizeof(quoteName) - 1);
  quoteName[sizeof(quoteName) - 1] = '\0';

  GetMyVolName(tmpSpec.vRefNum, folderName);
  /* theMessage is output parameter - cast to unsigned char* for ComposeRString */
  ComposeRString((unsigned char *)theMessage, RELATED_FMT, MIME_RELATED,
                 folderName, URLEscape(quoteName), fid, (*hdh)->cidHash,
                 (*hdh)->relURLHash, (*hdh)->absURLHash);
}

/************************************************************************
 * AddAttachInfo - attach a note about problems with the enclosure
 ************************************************************************/
void AddAttachInfo(short theIndex, long result) {
  unsigned char theMessage[256];

  ComposeString(theMessage, "%r%d\015", theIndex, result);
  GrowBuf_Append(&AttachedFiles, theMessage, strlen((const char *)theMessage));
}

/************************************************************************
 * WriteAttachNote - write the attachment note
 ************************************************************************/
OSErr WriteAttachNote(short refN) {
  long size;
  short err = noErr;

  if (AttachedFiles.data && (size = AttachedFiles.size) > 0) {
    if (!(err = EnsureNewline(refN))) {
      err = AWrite(refN, &size, AttachedFiles.data);
      GrowBuf_Reset(&AttachedFiles);
    }
  }
  return (err);
}

/************************************************************************
 * POPLast - give the LAST command to find the last unread message
 ************************************************************************/
short POPLast(TransStream stream, short *lastRead) {
  unsigned char buffer[128];
  unsigned char *spot;
  long size = sizeof(buffer);

  if (Prr = POPCmdGetReply(stream, kpcLast, nil, buffer, &size))
    return (Prr);
  ComposeLogS(LOG_LMOS, nil, (UPtr)"Last: %s", buffer);
  if (*buffer != '+')
    Prr = *buffer;
  else {
    strtok(buffer, " "); /* skip ok */
    if (spot = strtok(nil, " \015"))
      *lastRead = Atoi(spot); /* read message size */
    else
      return (1);
  }
  return (0);
}

/************************************************************************
 * ReadPOPLine - read a line from the POP server.
 *  Returns the kind of line it is:
 *		plComplete	- a line that began with a newline
 *		plPartial - the remainder of a line
 *		plBlank - a blank line
 *		plEndOfMessage - the message is OVER.
 *		plError - there has been an error (recorded in global Prr)
 *  Also removes the "." that escapes lines beginning with ".",
 *   and adds a ">" to escape envelopes
 ************************************************************************/
POPLineType ReadPOPLine(TransStream stream, unsigned char *buf, long bSize,
                        long *len) {
  static int wasNl;
  POPLineType returnType;
  long freeStack;

// sniff the stack to see if we're low
  if (InAThread()) {
    ThreadID threadID;

    GetCurrentThread(&threadID);
    ThreadCurrentStackSpace(threadID, &freeStack);
  } else
    freeStack = StackSpace();

  if (freeStack < kLowStackSize) {
    CommandPeriod = true;
    StackLowErr = true;
  }

  /*
   * nil buffer initializes
   */
  if (!buf) {
    wasNl = True;
    return (plEndOfMessage);
  }

  /*
   * grab the line
   */
  *len = bSize - 1; /* allow extra char for escaped envelopes */
  if (Prr = RecvLine(stream, buf, len))
    return (plError);

  /*
   * update progress indicator
   */
  ByteProgress(nil, -*len, 0);

  /*
   * are we looking at the beginning of a line?
   */
  if (wasNl) {
    returnType = plComplete;

    if (buf[0] == '.') /* leading period */
    {
      if (buf[1] == '.') /* if dot doubled, was in message data */
      {
        BMD(buf + 1, buf, *len);
        --*len;
      } else if (buf[1] == '\015') /* if dot followed by \015, end of message */
        returnType = plEndOfMessage;
    } else if (IsFromLine(buf)) /* is envelope? */
    {
      BMD(buf, buf + 1, *len); /* escape with '>' */
      ++*len;
      buf[0] = '>';
    }
  } else
    returnType = plPartial;

  wasNl = *len ? buf[*len - 1] == '\015' : True; /* set for next go-round */

  return (returnType);
}

/************************************************************************
 * SplitMessage - split a message into pieces
 ************************************************************************/
short SplitMessage(short refN, long hStart, long hEnd, long msgEnd) {
  long splitSize = GetRLong(FRAGMENT_SIZE);
  long bodySplit;
  short err;
  short count;
  short headerNl = 0;
  long *froms = nil;
  long *tos = nil;
  bool *reals = nil;
  short i;
  bool headerReal;

  if (hEnd - hStart > splitSize) {
    /* uh-oh.  the header is waaaaaaay big */
    if (err = HuntNewline(refN, hStart + 4096, &hEnd, &headerReal))
      goto done;
    headerNl = headerReal ? 1 : 2;
  }

  /*
   * how many splits do we need?
   */
  bodySplit = splitSize -
              (hEnd - hStart); /* how much of the body goes into each split */
  count = (msgEnd - hEnd + bodySplit - 1) /
          bodySplit;                   /* how many splits do we need? */
  bodySplit = (msgEnd - hEnd) / count; /* divide them evenly */

  /*
   * Ok, now let's find all the split locations
   */
  if (!(froms = NuPtr((count + 1) * sizeof(long *))) ||
      !(tos = NuPtr((count + 1) * sizeof(long *))) ||
      !(reals = NuPtr((count + 1) * sizeof(Boolean)))) {
    WarnUser(MEM_ERR, err = MemError());
    goto done;
  }

  for (i = 1, *froms = hEnd; i < count; i++) {
    if (err = HuntNewline(refN, hEnd + i * bodySplit, &froms[i], &reals[i]))
      goto done;
  }

  /*
   * stuff the end of the message into the last split location,
   * and the beginning into the first
   */
  froms[count] = msgEnd;
  reals[count] = True;
  reals[0] = True;
  tos[0] = hEnd + headerNl;

  /*
   * now, calculate where it all goes
   */
  for (i = 1; i < count; i++) {
    tos[i] = tos[i - 1] +               /* start of last body */
             hEnd - hStart + headerNl + /* header size */
             (reals[i] ? 0 : 1) +       /* room for extra nl? */
             froms[i] - froms[i - 1];   /* size of last body segment */
  }
  tos[count] = tos[count - 1] + froms[count] - froms[count - 1];

  /*
   * copy the segments, one by one
   */
  for (i = count - 1; i >= 0; i--) {
    if (tos[i] != froms[i]) {
      /*
       * copy body bytes
       */
      if (err = CopyFBytes(refN, froms[i], froms[i + 1] - froms[i], refN,
                           tos[i])) {
        WarnUser(WRITE_MBOX, err);
        goto done;
      }

      if (i) {
        /*
         * copy the header bytes
         */
        if (err = CopyFBytes(refN, hStart, hEnd - hStart, refN,
                             tos[i] - (hEnd - hStart + headerNl))) {
          FileSystemError(WRITE_MBOX, "", err);
          goto done;
        }
      }

      /*
       * do we need to add an ending newline to the header?
       */
      if (headerNl) {
        if (err = SetFPos(refN, fsFromStart, tos[i] - headerNl)) {
          FileSystemError(WRITE_MBOX, "", err);
          goto done;
        }
        if (err = FSWriteP(refN, headerReal ? (unsigned char *)"\r"
                                            : (unsigned char *)"\015\015")) {
          FileSystemError(WRITE_MBOX, "", err);
          goto done;
        }
      }
    }

    /*
     * do we need to add an ending newline to the body?
     */
    if (!reals[i + 1]) {
      if (err = SetFPos(refN, fsFromStart, tos[i] + froms[i + 1] - froms[i])) {
        FileSystemError(WRITE_MBOX, "", err);
        goto done;
      }
      if (err = FSWriteP(refN, (unsigned char *)"\r")) {
        FileSystemError(WRITE_MBOX, "", err);
        goto done;
      }
    }
  }

  TruncOpenFile(refN, tos[count]);

done:
  if (tos)
    ZapPtr(tos);
  if (froms)
    ZapPtr(froms);
  if (reals)
    ZapPtr(reals);
  Prr = err;
  return (err ? 0 : count);
}

#ifdef POPSECURE
/************************************************************************
 * VetPOP - make sure the 's POP account is ok.
 ************************************************************************/
short VetPOP(void) {
  unsigned char host[256];
  long port;
  short err;

  if (UUPCIn && !UUPCOut) {
    WarnUser(UUPC_SECURE, 0);
    return (1);
  }
  GetPOPInfo(, host);
  port = GetRLong(POP_PORT);
  if ((err = StartPOP(host, port)) == noErr) {
    (void)POPIntroductions();
    if (Prr)
      err = Prr;
  }
  EndPOP();
  if (UseCTB && !err)
    err = CTBNavigateSTRN(NAVMID);
  return (err);
}
#endif

#pragma segment MD5
static char hex[] = "0123456789abcdef";
#ifndef MD5_CTX
typedef struct {
  unsigned char digest[16];
  unsigned long state[4];
  unsigned long count[2];
  unsigned char buffer[64];
} MD5_CTX;
#endif

#ifndef MD5Init
#define MD5Init(x)
#define MD5Update(x, y, z)
#define MD5Final(x)
#endif

/************************************************************************
 * GenDigest - generate a digest for APOP
 ************************************************************************/
bool GenDigest(unsigned char *banner, unsigned char *secret,
               unsigned char *digest) {
  unsigned char stamp[256];
  MD5_CTX md5;
  short i;

  if (!*ExtractStamp(stamp, banner))
    return (False);

  MD5Init(&md5);
  MD5Update(&md5, stamp + 1, *stamp);
  MD5Update(&md5, secret + 1, *secret);
  MD5Final(&md5);

  for (i = 0; i < sizeof(md5.digest); i++) {
    digest[2 * i + 1] = hex[(md5.digest[i] >> 4) & 0xf];
    digest[2 * i + 2] = hex[md5.digest[i] & 0xf];
  }
  digest[0] = 2 * sizeof(md5.digest);
  return (True);
}

#define kmd5opad (0x5C)
#define kmd5ipad (0x36)
#define kmd5Len (64)
/************************************************************************
 * GenKeyedDigest - generate a keyed digest for APOP
 ************************************************************************/
bool GenKeyedDigest(unsigned char *banner, unsigned char *secret,
                    unsigned char *digest) {
  unsigned char stamp[256];
  MD5_CTX /*ipadMD5,*/ md5;
  short i;
  /*	Str255 localS; */

  if (!*ExtractStamp(stamp, banner))
    return (False);

  /*
          Zero(localS);
          BMD(secret+1,localS,*secret);
          for (i=0;i<kmd5Len;i++) localS[i] ^= kmd5ipad;

          MD5Init(&ipadMD5);
          MD5Update(&ipadMD5,localS,kmd5Len);
          MD5Update(&ipadMD5,stamp+1,*stamp);
          MD5Final(&ipadMD5);

          Zero(localS);
          BMD(secret+1,localS,*secret);
          for (i=0;i<kmd5Len;i++) localS[i] ^= kmd5opad;

          MD5Init(&md5);
          MD5Update(&md5,localS,kmd5Len);
          MD5Update(&md5,ipadMD5.digest,sizeof(ipadMD5.digest));
          MD5Final(&md5);
  */
#ifndef hmac_md5
#define hmac_md5(s, sl, k, kl, d)
#endif
  hmac_md5(stamp + 1, *stamp, secret + 1, *secret, &md5.digest);

  for (i = 0; i < sizeof(md5.digest); i++) {
    digest[2 * i + 1] = hex[(md5.digest[i] >> 4) & 0xf];
    digest[2 * i + 2] = hex[md5.digest[i] & 0xf];
  }
  digest[0] = 2 * sizeof(md5.digest);
  return (True);
}

/************************************************************************
 * ExtractStamp - grab the timestamp out of a POP banner
 ************************************************************************/
unsigned char *ExtractStamp(unsigned char *stamp, unsigned char *banner) {
  unsigned char *cp1, *cp2;

  *stamp = 0;
  if (cp1 = strchr(banner, '<'))
    if (cp2 = strchr(cp1 + 1, '>')) {
      int len = cp2 - cp1 + 1;
      strncpy(stamp, cp1, len);
      stamp[len] = '\0';
    }
  return (stamp);
}

/************************************************************************
 * New LMOS strategy
 ************************************************************************/

/************************************************************************
 * FillPOPD - fill the pop descriptor with important info from a header
 ************************************************************************/
void FillPOPD(POPDPtr pdp, HeaderDHandle hdh) {
  unsigned char msgId[256];
  uint32_t hash;

  if (pdp->uidHash == 0) {
    pdp->receivedGMT = GMTDateTime();
    if (pdp->uidHash == kNeverHashed || pdp->uidHash == kNoMessageId) {
      if (*HeaderMsgId(hdh, msgId))
        hash = MIDHash(msgId + 1, *msgId);
      else
        hash = FakeMIDHash(hdh);
      pdp->uidHash = hash;
    }
  }
}

/************************************************************************
 * BuildPOPD - build the descriptor of the work we have to do with the
 *  pop server
 * -- HERE BE DRAGONS --
 *  be careful with this code.  It does things in a specific order for a reason
 ************************************************************************/
OSErr BuildPOPD(TransStream stream, POPDHandle *popDH, short count,
                XferFlags *flags, bool *capabilities) {
  POPDesc new, old;
  short i;
  short spot;
  bool room;
  bool anyRoom =
      True; /* we wouldn't be here if there weren't at least a little room */
  long skipSize = GetRLong(BIG_MESSAGE) K;
  POPDHandle oldDH = nil;
  bool sbm = PrefIsSet(PREF_NO_BIGGIES);
  bool lmos = PrefIsSet(PREF_LMOS);
  long spaceNeeded = 0;
  short lastRead = 0; /* for the status or last methods */
  uint32_t age = GetPrefLong(PREF_LMOS_XDAYS);
  bool onFetch, onDelete;
  bool aged;
  bool plentyRoom;
  short popMode = GetPrefLong(PREF_POP_MODE);
  int total = 0;
  TOCType * tempInTocH = nil;

  if (InAThread())
    tempInTocH = GetTempInTOC();

  age = (age && lmos) ? (GMTDateTime() - 24 * 3600 * age) : 0;
  /*
   * allocate it
   */
  if (!(*popDH = POPDNew(count)))
    return (WarnUser(MEM_ERR, Prr = MemError()));

  if (Prr = FillSizesWithList(stream, *popDH))
    return (Prr);

  if (popMode != popRUIDL)
    lastRead = FirstUnread(stream, count) - 1;

  if (!(*CurPers)->noUIDL && (Prr = FillWithUidl(stream, *popDH)))
    return (Prr);

  /* Resource-based old POPD lookup removed (was Mac-specific no-op).
   * oldDH is always nil now. */
  oldDH = nil;
  {
    Prr = noErr;
    if (Prr)
      return (WarnUser(BUILD_POPD, Prr));
  }

  if (LogLevel & LOG_LMOS)
    Log1POPD((UPtr)"BuildPOPD", (UPtr)"Old", oldDH);

  if ((*CurPers)->noUIDL && (Prr = FillWithTop(stream, *popDH, oldDH)))
    return (Prr);

  for (i = 0; i < count; i++)
    spaceNeeded += (*popDH)->data[i].msgSize;
  plentyRoom = !RoomForMessage(spaceNeeded);
  spaceNeeded = 0;

  /*
   * examine each of the new messages
   */
  for (i = 0; i < count; i++) {
    MyThreadBeginCritical();
    CycleBalls();
    MyThreadEndCritical();
    aged = False;

    new = (*popDH)->data[i];

    /*
     * have we seen it before?
     */
    if (!oldDH || errNotFound == (spot = FindExistSpot(oldDH, new.uidHash))) {
      Zero(old);
      old.retred = i < lastRead;
      old.stubbed = i < lastRead;

      /*
       * if the message is already in in.temp and we don't have a popd entry,
       * machine probably crashed during download before popd could get updated.
       * mark these messages as fetched. make an exception for the last message,
       * since it could be incomplete. remove this message and re-fetch it.
       */
      if (InAThread()) {
        short sum;

        if (tempInTocH) {
          sum = FindSumByHash(tempInTocH, new.uidHash);
          if (sum != -1) {
            if (sum == (tempInTocH->count - 1)) {
              DeleteSum(tempInTocH, sum);
              old.retred = False;
              old.stubbed = False;
            } else {
              old.retred = True;
              new.retred = True;
            }
          }
        }
      }
    } else {
      old = oldDH->data[spot];
      new = old;
      new.retr = False;
      new.stub = False;
      new.delete = False;
      if (popMode != popRUIDL) {
        old.stubbed = old.stubbed || i < lastRead;
        old.retred = old.retred || i < lastRead;
      }
    }

    /*
     * hard nuke?
     */
    if (flags->nukeHard)
      new.delete = True;

    /*
     * just stub?
     */
    else if (flags->stub) {
      new.head = new.stub = True;
    }

    /*
     * is this message supposed to be toast?
     */
    else if (old.deleted)
      new.delete = True; /* yes.  Just kill it. */
    else {
      /*
       * ok, message is not just to be killed.  Make a rough pass on whether
       * or not it should be fetched or deleted.  We'll refine our decision
       * later
       */

      /*
       * check lists
       */
      onFetch = IdIsOnPOPD(CUR_POPD_TYPE, FETCH_ID, new.uidHash);
      onDelete = IdIsOnPOPD(CUR_POPD_TYPE, DELETE_ID, new.uidHash);

      /*
       * is there room?
       */
      room =
          anyRoom && (plentyRoom || !RoomForMessage(spaceNeeded + new.msgSize));
      if (room)
        new.big2 = False; /* clear old flag */

      /*
       * Should we fetch it? (ROUGH)
       */
      if (!old.retred)
        new.retr = flags->check;

      /*
       * should we delete it? (ROUGH)
       */
      aged = age && new.receivedGMT &&age > new.receivedGMT;
      new.delete = !lmos || flags->servDel &&onDelete || /* user told us to */
                   aged &&new.retred;

      /*
       * ok, now we fine-tune the process.  Check that there is room for
       * the message, and that we don't want to skip it because of the
       * skip big messages preference.
       */

      /*
       * if we're skipping big messages, see if we want to skip this one
       */
      if (!sbm)
        new.skip = False; /* clear old flag */
      else if (new.msgSize > skipSize && skipSize > 0)
        new.skip = True;
      else
        new.skip = False;

      if (new.retr &&new.skip && !onFetch) {
        new.retr = False;
        if (!old.stubbed) {
          new.stub = True;
          new.delete = False; /* don't delete, because only fetching a stub */
        }
      }

      /*
       * under what circumstances do we not fetch when we already have a stub?
       *	We get stubs from:
       *		Fetching headers - such stubs are never expanded except
       *manually Skip big messages - if the pref changes, the messages will be
       *fetched Not enough room - if the user makes room, the messages will be
       *fetched
       */
      if (old.stubbed && new.retr) {
        if (new.head || new.error)
          new.retr = False; // if the user fetched headers, must request fetch
      }

      new.retr = new.retr || flags->servFetch &&onFetch;

      /*
       * If we want the whole message, do we have room?
       */
      if (new.retr && !room) {
        new.big2 = True;
        new.delete =
            False; /* since we want to fetch it but can't, don't delete it */
        new.retr = False;
        if (!old.stubbed)
          new.stub = True; /* shd we fetch the stub? */
      }
    }

    /*
     * maybe we don't even have room for the stub?
     */
    if (!anyRoom && new.stub) {
      new.stub = False; /* no room at the inn */
      new.delete =
          False; /* again, since we want it but can't get it, don't delete it */
    }

    /*
     * ok, we did it!  Now, record how much disk this message will use.
     */
    if (new.stub)
      spaceNeeded += 3 K; /* arbitrary conservative guess at stub size */
    else if (new.retr)
      spaceNeeded += new.msgSize;
    anyRoom = anyRoom && (plentyRoom || !RoomForMessage(spaceNeeded));

    /*
     * special processing
     */
    if (flags->nuke)
      new.delete = True;

    /*
     * last sanity check
     */
    if (!(new.retred || new.retr) && (!onDelete || onFetch) &&
        !flags->nukeHard && !old.deleted)
      new.delete = False;

    /*
     * store it back
     */
    (*popDH)->data[i] = new;
    if (new.retr)
      total += new.msgSize;
    else if (new.stub)
      total += 3 K;
  }
  ByteProgress(nil, 0, total);
  POPDelDup(*popDH);

  if (LogLevel & LOG_LMOS)
    LogPOPD((UPtr)"BUILT", *popDH);

  return (noErr);
}

/**********************************************************************
 * POPDelDup - delete duplicate messages before downloading
 **********************************************************************/
void POPDelDup(POPDHandle popDH) {
  short i, j, n;
  short killme;

  n = popDH->count;
  for (i = 0; i < n; i++)
    for (j = i + 1; j < n; j++)
      if (popDH->data[i].uidHash && popDH->data[i].uidHash == popDH->data[j].uidHash) {
        if (popDH->data[i].retred)
          killme = j;
        else if (popDH->data[j].retred)
          killme = i;
        else if (popDH->data[i].msgSize > popDH->data[j].msgSize)
          killme = j;
        else
          killme = i;
        popDH->data[killme].retr = popDH->data[killme].stub = False;
        popDH->data[killme].delete = True;
      }
}

/************************************************************************
 * FillWithTop - fill up the descriptor without uidl.  Not fun.
 ************************************************************************/
OSErr FillWithTop(TransStream stream, POPDHandle new, POPDHandle old) {
  short oldCount;
  short newCount = new->count;
  short oldSpot, newSpot;
  short oldUndelCount;
  short oldUndelSpot;

  if (old) {
    oldCount = old->count;
    /*
     * find last undeleted message in old descriptor, and count them
     */
    oldUndelCount = 0;
    for (oldSpot = 0; oldSpot < oldCount; oldSpot++) {
      if (!old->data[oldSpot].deleted && old->data[oldSpot].uidHash) {
        oldUndelSpot = oldSpot;
        oldUndelCount++;
      }
    }

    /*
     * I DON'T TRUST THIS
     */
    if (!oldUndelCount)
      return (noErr); /* we will either delete or stub these, so we can stop
                         now.  maybe */

    /*
     * Now, check to see if it's in the same spot on the server
     */
    if (oldUndelCount && newCount >= oldUndelCount) {
      Prr = FillPOPDFromServer(stream, new, oldUndelCount - 1);
      if (Prr)
        return (Prr);

      if (MatchPOPD(old, oldUndelSpot, new->data[oldUndelCount - 1].uidHash)) {
        /*
         * ok, now we make the big leap of faith.  Assume that since we found
         * this one where we expected it, the others will be there, too
         */
        ComposeLogS(LOG_LMOS, nil, (UPtr)"Copy old to %d", oldUndelCount);
        for (newSpot = oldSpot = 0; oldSpot < oldCount; oldSpot++) {
          if (!old->data[oldSpot].deleted) {
            new->data[newSpot].uidHash = old->data[oldSpot].uidHash;
            new->data[newSpot++].receivedGMT = old->data[oldSpot].receivedGMT;
          }
        }

        /*
         * NO TRUST HERE
         */
        return (
            noErr); /* we can stop now, we'll fetch or stub the rest.  maybe */
      }
    }
  } else
    /*
     * DONT TRUST
     */
    return (noErr); /* no old one, so don't need to top.  maybe */

  /*
   * ok, so much for clever.  now, we act with force
   */
  for (newSpot = 0; !Prr && newSpot < newCount; newSpot++)
    Prr = FillPOPDFromServer(stream, new, newSpot);

  return (Prr);
}

/************************************************************************
 * FillPOPDFromServer - fille the POP Descriptor block by asking the server
 ************************************************************************/
OSErr FillPOPDFromServer(TransStream stream, POPDHandle popDH, short spot) {
  long msgSize;
  unsigned char scratch[256];
  HeaderDHandle hdh = nil;
  short refN = 0;
  Token822Enum tokenType;
  POPDesc pd;
  long size;

  if (popDH->data[spot].uidHash) {
    return (noErr);
  } /* already done */
  pd = popDH->data[spot];

  ComposeRString(scratch, FIRST_UNREAD, spot + 1);
  ProgressMessage(kpSubTitle, scratch);

  if (!(hdh = NewHeaderDesc(nil)))
    return (WarnUser(MEM_ERR, Prr = MemError()));

  /*
   * get rest of message
   */
  NumToString(spot + 1, scratch);
  PLCat(scratch, 1);
  size = sizeof(scratch);
  if (Prr = POPCmdGetReply(stream, kpcTop, scratch, scratch, &size))
    goto done;

  /*
   * read in the header
   */
  tokenType = ReadHeader(stream, hdh, pd.msgSize, 0, False);
  if (CommandPeriod || tokenType == ErrorToken) {
    Prr = ErrorToken;
    goto done;
  }

  if (tokenType != EndOfMessage) {
    for (msgSize = sizeof(scratch);
         !(Prr = RecvLine(stream, scratch, &msgSize)) &&
         !POP_TERM(scratch, msgSize);
         msgSize = sizeof(scratch))
      ;
  }

  /*
   * fill the descriptor
   */
  FillPOPD(&pd, hdh);
  ComposeLogS(LOG_LMOS, nil, (UPtr)"Fill %d: hash %x gmt %x.", spot, pd.uidHash,
              pd.receivedGMT);

  popDH->data[spot] = pd;

done:
  ZapHeaderDesc(hdh);
  return (Prr);
}

/************************************************************************
 * FindExistSpot - find a message already in a descriptor
 ************************************************************************/
short FindExistSpot(POPDHandle popDH, uint32_t hash) {
  short spot = 0;
  short count = popDH ? popDH->count : 0;

  for (spot = 0; spot < count; spot++)
    if (MatchPOPD(popDH, spot, hash))
      return (spot);
  return (errNotFound);
}

/************************************************************************
 * FindUndelete - find a message already in a descriptor,
 *                without the delete flag if possible
 ************************************************************************/
short FindUndelete(POPDHandle popDH, uint32_t gmt, uint32_t hash) {
  short spot;
  short count = popDH ? popDH->count : 0;
  short found = errNotFound;

  for (spot = 0; spot < count; spot++)
    if (MatchPOPD(popDH, spot, hash)) {
      if (!popDH->data[spot].delete)
        return (spot);
      found = spot;
    }
  return (found);
}

/************************************************************************
 * DisposePOPD - get rid of the POP descriptors
 ************************************************************************/
void DisposePOPD(POPDHandle *popDH) {
  if (*popDH) {
    if (LogLevel & LOG_LMOS)
      LogPOPD((UPtr)"AFTER", *popDH);

    /* Resource-based POPD persistence removed (was Mac-specific).
     * TODO: Implement GKeyFile-based POPD persistence if needed. */
    ZapSettingsResourceMainThread_(CUR_POPD_TYPE, POPD_ID);

    FixServers = True;
    POPDFree(popDH);
  }
}

/**********************************************************************
 * FixMessServerAreas - fix the server displays of message windows
 **********************************************************************/
void FixMessServerAreas(void) {
  WindowPtr winWP;
  MyWindowPtr win;

  for (winWP = FrontWindow_(); winWP; winWP = GetNextWindow(winWP))
    if (IsWindowVisible(winWP))
      if (win = GetWindowMyWindowPtr(winWP))
        Fix1MessServerArea(win);
}

/************************************************************************
 * HeaderMsgId - nab the message-id
 ************************************************************************/
PStr HeaderMsgId(HeaderDHandle hdh, PStr msgId) {
  unsigned char scratch[256];

  if (!AAFetchResData((*hdh)->funFields, InterestHeadStrn + hMessageId,
                      scratch))
    PCopyTrim(msgId, scratch, 128);
  else
    *msgId = 0;
  return (msgId);
}

/************************************************************************
 * FakeMIDHash - fake a message-id for something that doesn't have one
 ************************************************************************/
uint32_t FakeMIDHash(HeaderDHandle hdh) {
  unsigned char scratch[256];

  /*
   * no message-id in this message.  Look for other headers of interest,
   * and add them to our string one at a time
   */
  *scratch = 0;
  AAFetchResData((*hdh)->funFields, InterestHeadStrn + hReceived, scratch);
  if (!*scratch)
    AAFetchResData((*hdh)->funFields, InterestHeadStrn + hDate, scratch);

  PSCat(scratch, (*hdh)->who);
  PSCat(scratch, (*hdh)->subj);

  return (Hash(scratch));
}

/************************************************************************
 * CountFetch - count the number of messages to be fetched
 ************************************************************************/
short CountFetch(POPDHandle popDH) {
  short n;
  short fetch = 0;

  if (popDH && popDH->data) {
    n = popDH->count;
    while (n--) {
      if (popDH->data[n].retr || popDH->data[n].stub)
        fetch++;
    }
  }
  return (fetch);
}

/************************************************************************
 * AddIdToPOPD - add a message to a POPD list
 ************************************************************************/
OSErr AddIdToPOPD(OSType theType, short listId, uint32_t uidHash, bool dupOk) {
  POPDHandle resH;
  short n;
  short i;
  POPDesc popd;

  if (!ValidHash(uidHash))
    return (errNotFound);
  resH = (POPDHandle)GetResourceMainThread_(theType, listId);
  if (!resH) {
    resH = POPDNew(0);
    if (!resH)
      return (resNotFound);
    AddMyResourceMainThread_(resH, theType, listId, "");
  }

  n = resH->count;
  for (i = 0; i < n; i++)
    if (resH->data[i].uidHash == uidHash)
      return (noErr);

  /*
   * not there.  add it.
   */
  Zero(popd);
  popd.uidHash = uidHash;
  if (POPDAppend(resH, &popd))
    WarnUser(MEM_ERR, memFullErr);
  return (noErr);
}

/************************************************************************
 * RemIdFromPOPD - remove a message from a POPD list
 ************************************************************************/
void RemIdFromPOPD(OSType theType, short listId, uint32_t uidHash) {
  POPDHandle resH = (POPDHandle)GetResourceMainThread_(theType, listId);
  short n;
  short i;

  if (!ValidHash(uidHash))
    return;
  if (!resH)
    return;

  n = resH->count;
  for (i = 0; i < n; i++)
    if (resH->data[i].uidHash == uidHash) {
      POPDRemoveAt(resH, i);
      break;
    }
}

/**********************************************************************
 * PrunePOPD - prune old stuff from fetch & delete lists
 **********************************************************************/
void PrunePOPD(OSType theType, short listId, POPDHandle onServer) {
  POPDHandle resH = (POPDHandle)GetResourceMainThread_(theType, listId);
  short n;
  uint32_t uidHash;
  short sCount;
  short i;

  if (!resH)
    return;
  n = resH->count;
  sCount = onServer->count;
  while (n--) {
    uidHash = resH->data[n].uidHash;
    for (i = 0; i < sCount; i++) {
      if (onServer->data[i].uidHash == uidHash)
        break;
    }
    if (i == sCount) {
      ComposeLogS(LOG_LMOS, nil, (UPtr)"Prune %p: %d %x",
                  listId % 4 == DELETE_ID % 4 ? (UPtr)"DELETE" : (UPtr)"FETCH", n,
                  uidHash);
      RemIdFromPOPD(theType, listId, uidHash);
    }
  }
}

/************************************************************************
 * IdIsOnPOPD - is a message on a POPD list?
 ************************************************************************/
bool IdIsOnPOPD(OSType listType, short listId, uint32_t uidHash) {
  POPDHandle resH = (POPDHandle)GetResourceMainThread_(listType, listId);
  short n;
  short i;
  bool result = False;

  if (!ValidHash(uidHash))
    return (False);
  if (!resH)
    return (False);

  n = resH->count;
  for (i = 0; i < n; i++)
    if (resH->data[i].uidHash == uidHash) {
      result = !resH->data[i].deleted;
      break;
    }
  return (result);
}

/************************************************************************
 * FillWithUidl - fill the descriptor using the uidl command
 ************************************************************************/
OSErr FillWithUidl(TransStream stream, POPDHandle popDH) {
  unsigned char buffer[256];
  long size = sizeof(buffer);
  unsigned char *spot, *end;
  short msgNum;
  short n = popDH->count;
  uint32_t uidHash;
  POPDHandle oldDH = (POPDHandle)GetResourceMainThread_(CUR_POPD_TYPE, POPD_ID);
  short i;

  for (i = n; i--;)
    popDH->data[i].uidHash = 0;
  i = 0;

  if (Prr = POPCmdGetReply(stream, kpcUidl, nil, buffer, &size))
    return (Prr);

  if (*buffer == '-') {
    buffer[size] = 0;
    ComposeLogS(LOG_LMOS, nil, (UPtr)"UIDL err: %s", buffer);
    (*CurPers)->noUIDL = True;
    return (noErr);
  }

  //	if (n>100) ByteProgress(nil,0,n);

  for (size = sizeof(buffer);
       !(Prr = RecvLine(stream, buffer, &size)) && !POP_TERM(buffer, size);
       size = sizeof(buffer)) {
    MiniEvents();
    if (CommandPeriod)
      break;
    CycleBalls();
    if (*buffer != ' ' && *buffer != '\t') {
      msgNum = Atoi(buffer);
      if (msgNum < 1 || msgNum > n)
        continue;

      //			if (n>100) ByteProgress(nil,-1,0);

#ifdef DEBUG
      if (RunType != Production && ++i != msgNum)
        AlertStr(OK_ALRT, Stop, (UPtr)"Bad UIDL!");
#endif

      end = buffer + size;
      while (end[-1] <= ' ')
        end--;
      for (spot = buffer; spot < end && *spot == ' '; spot++)
        ;
      while (spot < end && *spot != ' ')
        spot++;
      while (spot < end && *spot == ' ')
        spot++;
      if (spot < end) {
        spot[-1] = end - spot;
        uidHash = Hash(spot - 1);
        popDH->data[msgNum - 1].uidHash = uidHash;
        popDH->data[msgNum - 1].receivedGMT = GMTDateTime();
        ComposeLogS(LOG_LMOS, nil, (UPtr)"UIDL %d \xC7%p\xC8 %x", msgNum, spot - 1,
                    uidHash);
      }
    }
  }
  //	if (n > 100 && !Prr) ByteProgress(nil,1,1);

  for (i = 0; i < n; i++)
    if (popDH->data[i].uidHash == 0)
      FillPOPDFromServer(stream, popDH, i);

  Progress(NoBar, 0, nil, nil, nil);

  if (CommandPeriod && !Prr)
    Prr = userCancelled;
  return (Prr);
}

/************************************************************************
 * LogPOPD - write the POPD's to the log file
 ************************************************************************/
void LogPOPD(PStr intro, POPDHandle newDH) {
  Log1POPD(intro, (UPtr)"Fetch", (void *)GetResource(CUR_POPD_TYPE, FETCH_ID));
  Log1POPD(intro, (UPtr)"Delete", (void *)GetResource(CUR_POPD_TYPE, DELETE_ID));
  Log1POPD(intro, (UPtr)"Old", (void *)GetResource(CUR_POPD_TYPE, POPD_ID));
  Log(LOG_LMOS, (UPtr)"---");
  Log1POPD(intro, (UPtr)"New", newDH);
}

/************************************************************************
 * Log1POPD - write a POPD to the log file
 ************************************************************************/
void Log1POPD(PStr intro, PStr which, POPDHandle popDH) {
  short i;
  short count;

  if (popDH == nil || popDH->count == 0)
    ComposeLogS(LOG_LMOS, nil, (UPtr)"%p: %p: <empty>", intro, which);
  else {
    count = popDH->count;
    for (i = 0; i < count; i++) {
      CycleBalls();
      ComposeLogS(
          LOG_LMOS, nil, (UPtr)"%p: %p: %d hash %x gmt %x %c%c %c%c %c%c %c%c",
          intro, which, i + 1, popDH->data[i].uidHash, popDH->data[i].receivedGMT,
          popDH->data[i].retr ? 'F' : 'f', popDH->data[i].retred ? 'F' : 'f',
          popDH->data[i].stub ? 'S' : 's', popDH->data[i].stubbed ? 'S' : 's',
          popDH->data[i].big2 ? 'T' : 't', popDH->data[i].skip ? 'K' : 'k',
          popDH->data[i].delete ? 'D' : 'd', popDH->data[i].deleted ? 'D' : 'd');
    }
  }
}

/**********************************************************************
 * KerbDestroy - destroy the user's current ticket
 **********************************************************************/
OSErr KerbDestroy(void) {
  if (gPOPKerbInited) {
    KClientDisposeSessionCompat(&gSession);
    gPOPKerbInited = false;
  }
  return (KClientLogoutCompat());
}

/**********************************************************************
 * KerbDestroyUser - destroy the username
 **********************************************************************/
OSErr KerbDestroyUser(void) {
  OSErr err = noErr;

  /* KerbDestroy does everything that it needs to */

  return (err);
}

/**********************************************************************
 *
 **********************************************************************/
OSErr KerbUsername(PStr name) {
  OSErr err;
  unsigned char *atSign;

  *name = 0;
  err = KClientGetUserNameDeprecated(name);

  if (err == cKrbNotLoggedIn) {
    /*
     * log kerberos in
     */

    if (err = KClientNewSessionCompat(
            &gSession, 1 /*nLocalAddress*/, GetRLong(POP_PORT) /*inLocalPort*/,
            2 /*inRemoteAddress*/, GetRLong(POP_PORT) /*inRemotePort*/))
      return (err);
    if (err = KClientLoginCompat(&gSession, &gPrivateKey))
      return (err);

    /*
     * get username again
     */
    if (noErr != (err = KClientGetUserNameDeprecated(name)))
      return (err);
  }

  /*
   * trim realm (everything from '@' onward)
   */
  if (!err) {
    char *atSign = strchr((char *)name, '@');
    if (atSign)
      *atSign = '\0';
  }

  return (err);
}

/**********************************************************************
 * KerbGetTicket - get our ticket
 **********************************************************************/
OSErr KerbGetTicket(PStr serviceName, PStr inHost, PStr realm, PStr version,
                    unsigned char **ticket) {
  unsigned char fmt[64];
  unsigned char fullName[256];
  unsigned char scratch[256];
  unsigned char host[256];
  unsigned char shortHost[256];
  OSErr err;
  unsigned char *spot;
  struct hostInfo *hip, hi;
  unsigned long bufLen;

  // create a new session if we must
  err = InitKerberos();
  if (err != noErr)
    return (err);

  // get the user name from KCLient
  err = KerbUsername(scratch);
  if (err != noErr)
    return (err);

  /*
   * the best thing in the world is four million DNS calls
   */
  if (err = GetHostByName(inHost, &hip))
    return (err);
  if (err = GetHostByAddr(&hi, hip->addr[0]))
    return (err);
  CtoPCpy(host, hi.cname);

  /*
   * build the service name
   */
  spot = host;
  PToken(host, shortHost, &spot, ".");
  EscapeChars(host, GetRString(fmt, KERBEROS_ESCAPES));
  EscapeChars(serviceName, GetRString(fmt, KERBEROS_ESCAPES));
  GetRString(fmt, KERBEROS_SERVICE_FMT);
  utl_PlugParams(fmt, fullName, serviceName, host, realm, shortHost);
  ProgressMessage(kpMessage,
                  ComposeRString(scratch, KERBEROS_TICK_FMT, fullName));

  // Already null-terminated C strings, no conversion needed

  bufLen = GetRLong(KERBEROS_BSIZE);
  *ticket = (unsigned char *)malloc(bufLen);
  if (!*ticket)
    return (MemError());

  /*
   * call the driver
   */

  err = KClientMakeSendAuthCompat(&gSession, fullName + 1, *ticket, &bufLen,
                                  GetRLong(KERBEROS_CHECKSUM), version + 1);

  /*
   * adjust the ticket size
   */
  if (err) {
    free(*ticket);
    *ticket = NULL;
  } else {
    unsigned char *newTicket = (unsigned char *)realloc(*ticket, bufLen);
    if (newTicket) *ticket = newTicket;
  }

  /*
   * Cleanup
   */

  return (err);
}

/**********************************************************************
 * InitKerberos - get ready to start making Kerberos calls
 **********************************************************************/
OSErr InitKerberos() {
  OSErr err = noErr;

  if (!gPOPKerbInited) {
    // make sure Kerberos is present before we start calling it.
    if ((Ptr)(KClientNewSessionCompat) ==
        (Ptr)(kUnresolvedCFragSymbolAddress)) {
      // Kerberos is not installed.  Warn and crap out.
      WarnUser(NO_KERBEROS, err);
      err = fnfErr;
    } else {
      // Kerberos is here.  Try to start a new session.
      err = KClientNewSessionCompat(&gSession, 1, GetRLong(POP_PORT), 2,
                                    GetRLong(POP_PORT));
      if (err != noErr)
        return (err);
      else
        gPOPKerbInited = true;
    }
  }
  return (err);
}

/**********************************************************************
 * SendPOPTicket - send a ticket to the Pop server
 **********************************************************************/
OSErr SendPOPTicket(TransStream stream) {
  unsigned char popName[64], host[64], realm[64], version[64];
  unsigned char *ticket = nil;
  unsigned long ticketLen = 0;
  OSErr err;

  GetPOPInfo(popName, host);
  err = noErr;
  GetRString(popName, KERBEROS_POP_SERVICE);
  GetPref(realm, PREF_REALM);
  GetRString(version, KERBEROS_VERSION);

  if ((err = KerbGetTicket(popName, host, realm, version, &ticket)))
    return (WarnUser(NO_KERBEROS, err));

  ticketLen = ticket ? malloc_size(ticket) : 0;
  err = SendTrans(stream, ticket, ticketLen, nil);
  free(ticket);
  return (err);
}
