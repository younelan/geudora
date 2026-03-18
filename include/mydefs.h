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

#ifndef MYDEFS_C
#define MYDEFS_C
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * This file contains important things common to all source files
 **********************************************************************/
#include "conf.h"
#if __profile__
#include "Profiler.h"
#endif
#include "pete_shim.h"

/* ensure fixed-width integer and boolean types are available for portable
 * typedefs */
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

/* Portable malloc_size: query usable size of a heap allocation */
#if defined(__APPLE__)
#include <malloc/malloc.h>
/* malloc_size() already available */
#elif defined(__linux__)
#include <malloc.h>
#define malloc_size(p) malloc_usable_size(p)
#else
/* Fallback: no way to query — return 0 */
#define malloc_size(p) ((size_t)0)
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef uLong
typedef unsigned long uLong;
#endif
#ifndef uShort
typedef unsigned short uShort;
#endif
#ifndef CStr
typedef char *CStr;
#endif
/* Byte typedef removed — use unsigned char directly */
#ifndef SInt8
typedef signed char SInt8;
#endif
typedef void *InetSvcRef;
typedef void *EndpointRef;
typedef int OTResult;
typedef int OTEventCode;
struct TCPiopb;
typedef struct TCPiopb TCPiopb;

/* GrowBuf: simple growing byte buffer (replaces Handle-based buffers) */
typedef struct {
  char *data;
  long size;
  long capacity;
} GrowBuf;

void GrowBuf_Init(GrowBuf *buf);
int GrowBuf_Append(GrowBuf *buf, const void *ptr, long len);
void GrowBuf_Reset(GrowBuf *buf);
void GrowBuf_Free(GrowBuf *buf);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef false
/* #define False 0 — removed, using C99 true/false */
#endif
#ifndef true
/* #define True 1 — removed, using C99 true/false */
#endif
#ifndef everyEvent
#define everyEvent -1
#endif
#ifndef everyEvent
#define everyEvent -1
#endif
/* highLevelEventMask, keyDownMask, mDownMask, mUpMask defined in mailbox.h or
 * system headers */

/* BUILDING_MBX_LIB is provided by build flags when needed. */

/* Carbon is no longer used; prefer portable shims for legacy Mac types.
        Include project compatibility shims that provide minimal placeholders
        for QuickTime, Printing, and other legacy types needed by the code. */
/* Do not include system Carbon headers — this repo is being ported
   GTK-first and Carbon is removed. Use project-provided shims that
   define the minimal legacy types required to compile on modern
   systems without Carbon. */
// Include project-provided shims first to define basic types like Handle
#include "mailbox.h"

// Accumulator definition moved to mailbox.h
#include "mailbox.h"

/* Forward-declare a few mac/Carbon types used across the codebase.
   Provide minimal struct forward-declarations when the system headers
   don't expose them yet. Guards avoid redefinition when system types
   are present. */
/* EventRecord defined in mailbox.h */
/* EventPtr defined in mailbox.h */

#ifndef AEDesc
typedef struct AEDesc AEDesc;
#endif
#ifndef HAVE_AEDESCPTR
typedef AEDesc *AEDescPtr;
#define HAVE_AEDESCPTR 1
#endif

#ifndef AEAddressDesc
typedef struct AEAddressDesc AEAddressDesc;
#endif
#ifndef HAVE_AEADDRESSDESCPTR
typedef AEAddressDesc *AEAddressDescPtr;
#define HAVE_AEADDRESSDESCPTR 1
#endif

/* Portable placeholder for THz used in legacy APIs. */
#ifndef THz
typedef void *THz;
#endif

#ifndef AppleEvent
typedef struct AppleEvent AppleEvent;
#endif
#ifndef HAVE_APPLEEVENTPTR
typedef AppleEvent *AppleEventPtr;
#define HAVE_APPLEEVENTPTR 1
#endif

/* Basic status types - Now defined in mailbox.h */

/**********************************************************************
 * a pointer into nowhere
 **********************************************************************/
#ifndef nil
#define nil NULL
#endif

/**********************************************************************
 * make dealing with handles a little less unpleasant
 **********************************************************************/
/* Flat pointer model: Handle = void*, no double-indirection */
#define LDRef(aHandle) (aHandle)
#define UL(aHandle) ((void)0)
#define New(aType) ((aType *)NuPtr(sizeof(aType)))
#define NewH(aType) ((aType *)NuHTempOK(sizeof(aType)))
#define NewZH(aType) ((aType *)calloc(1, sizeof(aType)))
#define NewHTB(aType) ((aType *)NuHTempBetter(sizeof(aType)))
#define NewZHTB(aType) ((aType *)calloc(1, sizeof(aType)))
/* ZapHandle removed — use free() directly */
#ifndef DisposePtr
#define DisposePtr(p) free(p)
#endif
#define ZapPtr(aPtr)                                                           \
  while (aPtr) {                                                               \
    DisposePtr((void *)aPtr);                                                  \
    aPtr = nil;                                                                \
  }
/* HandleCount REMOVED — use explicit count fields */
#define DEC_STATE(h) char h##_state;
#define L_STATE(h)                                                             \
  do {                                                                         \
    h##_state = HGetState((void *)h);                                          \
    HLock((void *)h);                                                          \
  } while (0)
#define U_STATE(h)                                                             \
  do {                                                                         \
    HSetState((void *)h, h##_state);                                           \
  } while (0)

// (jp) Lots of casting will be evil under Carbon, for now we'll concern
// ourselves
//			with Window/Dialog/etc types of casts that won't work in
// a world without 			extended WindowRecords
#ifdef FLOAT_WIN
#define FrontWindow_ MyFrontNonFloatingWindow
#else // FLOAT_WIN
#define FrontWindow_ FrontWindow
#endif // FLOAT_WIN
#define HandToHand_(h) MyHandToHand((void **)(h))
#define AddResource_(h, t, i, n)                                               \
  AddResource((void *)(h), (uint32_t)(t), i, (ConstStr255Param)(n))
#define AddMyResource_(h, t, i, n)                                             \
  AddMyResource((void *)(h), (uint32_t)(t), i, (ConstStr255Param)(n))
#define GetResource_(t, i) (void *)GetResource((ResType)t, i)
#define GetIndResource_(t, i) (void *)GetIndResource((ResType)t, i)
#define AEGetParamPtr_(e, k, dt, t, p, m, a)                                   \
  AEGetParamPtr((void *)e, (AEKeyword)k, (DescType)dt, (void *)t, (void *)p,   \
                (Size)m, (void *)a)
#define SetPort_(p) SetPort(p)
/* GetHandleSize_ REMOVED — use explicit size tracking */
#define HNoPurge_(h) HNoPurge((void *)h)
#define GetAuxWin_(w, h) GetAuxWin(w, h)
#define GetNewControl_(i, w) GetNewControl(i, w)
#define GetNewControlSmall_(i, w) GetNewControlSmall(i, w)
#define ReleaseResource_(r) ReleaseResource((void *)r)
/* buf_append/buf_concat: no macros needed — use functions directly */
/* SetHandleBig_: resize a malloc'd buffer in-place via realloc.
   h must be an lvalue (pointer variable). */
#define SetHandleBig_(h, s) do { \
  void *_tmp = realloc((void *)(h), (size_t)(s)); \
  if (_tmp) (h) = _tmp; \
} while(0)
#define SetWTitle_(w, t) SetWTitle(w, t)
#define FindWindow_(p, w) FindWindow(p, w)
#define GetDItem_(d, i, t, h, r) GetDialogItem(d, i, t, (void *)h, r)
#ifdef FLOAT_WIN
#define SelectWindow_(w) gtk_window_present(GTK_WINDOW(w))
#else // FLOAT_WIN
#define gtk_window_present(GTK_WINDOW(w)) SelectWindow(w)
#endif // FLOAT_WIN
#ifdef FLOAT_WIN
#define HideWindow_(w) MyHideWindow(w)
#else // FLOAT_WIN
#define HideWindow_(w) HideWindow(w)
#endif // FLOAT_WIN

#define DisposeWindow_(aWindowPtr) MyDisposeWindow(aWindowPtr)
#define DisposeDialog_(aDialogPtr) MyDisposeDialog(aDialogPtr)
#define DisposDialog_(aDialogPtr) MyDisposeDialog(aDialogPtr)

/************************************************************************
 * Make UPP's less painful
 ************************************************************************/
#if TARGET_RT_MAC_CFM

/*
 * NOTE: We grabbed BUILD_ROUTINE_DESCRIPTOR from MixedMode.h and put it here
 * because we need to call old plug-ins when running under CarbonLib in MacOS
 * classic.  SD 6/11/02
 */

/* A macro which creates a static instance of a non-dispatched routine
 * descriptor */
#define BUILD_ROUTINE_DESCRIPTOR(procInfo, procedure)                                \
  {                                                                                  \
    _MixedModeMagic,               /* Mixed Mode A-Trap */                           \
        kRoutineDescriptorVersion, /* version */                                     \
        kSelectorsAreNotIndexable, /* RD Flags - not dispatched */                   \
        0,                         /* reserved 1 */                                  \
        0,                         /* reserved 2 */                                  \
        0,                         /* selector info */                               \
        0,                         /* number of routines */                          \
    {                              /* It�s an array */                               \
            {                                       /* It�s a struct */             \
                (procInfo),                         /* the ProcInfo */              \
                0,                                  /* reserved */                  \
                GetCurrentArchitecture(),           /* ISA and RTA */               \
                kProcDescriptorIsAbsolute |         /* Flags - it�s absolute addr */\
                kFragmentIsPrepared |               /* It�s prepared */             \
                kUseNativeISA,                      /* Always use native ISA */     \
                (ProcPtr)(procedure),               /* the procedure */             \
                0,                                  /* reserved */                  \
                0                                   /* Not dispatched */            \
            }                                                                       \
        }                                                                            \
  }

#endif

/************************************************************************
 * repeat ten times: "I hate the Memory Manager."
 ************************************************************************/
#define OFFSET_RECT(tangle, dh, dv)                                            \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    OffsetRect(&r, dh, dv);                                                    \
    *(tangle) = r;                                                             \
  } while (0)
#define INSET_RECT(tangle, dh, dv)                                             \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    InsetRect(&r, dh, dv);                                                     \
    *(tangle) = r;                                                             \
  } while (0)
#define SET_RECT(tangle, lf, tp, rt, bt)                                       \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    SetRect(&r, lf, tp, rt, bt);                                               \
    *(tangle) = r;                                                             \
  } while (0)
#define INVAL_RECT(tangle)                                                     \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    InvalRect(&r);                                                             \
  } while (0)
#define VALID_RECT(tangle)                                                     \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    ValidRect(&r);                                                             \
  } while (0)
#define ERASE_RECT(tangle)                                                     \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    EraseRect(&r);                                                             \
  } while (0)
#define FRAME_RECT(tangle)                                                     \
  do {                                                                         \
    Rect r = *(tangle);                                                        \
    FrameRect(&r);                                                             \
  } while (0)

/**********************************************************************
 * some #defines that don't seem to belong anywhere else
 **********************************************************************/
#define INSET 6
// (jp) already defined in Universal Headers 3.4
// #define TRUE	true
// #define FALSE false
/* Use standard true/false from stdbool.h */
#define IsSpace(c)                                                             \
  ((c) == '\t' || (c) == ' ' || (c) == '\f' || (c) == '\r' || (c) == '\n')
#define ENVIRONS_VERSION 2   /* the version of SysEnvirons we expect */
#define InFront ((void *)-1) /* for window creation */
#define BehindModal                                                            \
  (ModalWindow ? ModalWindow : InFront) /* for window creation */
#define REAL_BIG 32766                  /* REAL_BIG, more or less */
#define TABKEY 9
#define CANCEL_ITEM (-1)
/* MIN, MAX, ABS defined by GLib */
#define GROW_SIZE 15
#define MAX_DEPTH 12     /* max depth for alias tree */
/* fInited is defined in mailbox.h */
#define IsWhite(c) (c == ' ' || c == '\t')
#define IsLWSP(c) (c == ' ' || c == '\t' || c == '\r')
#define IsAnySP(c) (c == optSpace || c == ' ' || c == '\t' || c == '\r')
#define K *1024
#define MAX_ALIAS 200
/* Mac QuickDraw port management — no-op in GTK */
#define SAVE_PORT
#define REST_PORT
#ifdef DEBUG
#define CHECKPOINT                                                             \
  do {                                                                         \
    SpinSpot = __LINE__;                                                       \
  } while (0)
#define DBLINE                                                                 \
  do {                                                                         \
    unsigned char s[256];                                                       \
    DebugStr(ComposeString(s, (const unsigned char *)"%s %d;hc;sc;g", __FILE__, __LINE__)); \
  } while (0)
#include <assert.h>
#define ASSERT assert
#define VERIFY(expr) assert(expr)
#else
#define CHECKPOINT
#define DBLINE
#include <assert.h>
#define ASSERT assert
#define VERIFY(expr) expr
#endif
#define MEM_CRITICAL (SPARE_SIZE / 4)
#ifdef DEBUG
#define LOGLINE ComposeLogS(-1, nil, (unsigned char *)"%r:%d", FNAME_STRN + FILE_NUM, __LINE__)
#else
#define LOGLINE
#endif

#define A822_FLAVOR 'a822'

enum { kMyIntl0 = 1000 };

#ifdef EXP_YEAR
#define CHECK_EXPIRE                                                           \
  do {                                                                         \
    DateTimeRec dtr;                                                           \
    GetTime(&dtr);                                                             \
    if (dtr.year > EXP_YEAR ||                                                 \
        dtr.year == EXP_YEAR && dtr.month > EXP_MONTH) {                       \
      if (ComposeStdAlert(kAlertStopAlert, BETA_EXPIRED) ==                    \
          kAlertStdAlertOKButton)                                              \
        OpenAdwareURL(GetNagState(), UPDATE_SITE, actionUpdate, updateQuery,   \
                      nil);                                                    \
      EjectBuckaroo = true;                                                    \
    }                                                                          \
  } while (0)
#else
#define CHECK_EXPIRE
#endif

#ifdef DEMO
bool DemoExpired(void);
#ifndef LIGHT
#define CHECK_DEMO DemoExpired()
#else // LIGHT
#define CHECK_DEMO false
#endif // LIGHT
#else
#define CHECK_DEMO
#endif

#define TICKS2MINS 3600

/* TransStream header will be included later after transport typedefs and
 * OpenSSL types are available. */

/**********************************************************************
 *
 **********************************************************************/
#ifdef DEBUG
#define DebugStr0                                                              \
  if (BUG0)                                                                    \
  DebugStr
#define DebugStr1                                                              \
  if (BUG1)                                                                    \
  DebugStr
#define DebugStr2                                                              \
  if (BUG2)                                                                    \
  DebugStr
#define DebugStr3                                                              \
  if (BUG3)                                                                    \
  DebugStr
#define DebugStr4                                                              \
  if (BUG4)                                                                    \
  DebugStr
#define DebugStr5                                                              \
  if (BUG5)                                                                    \
  DebugStr
#define DebugStr6                                                              \
  if (BUG6)                                                                    \
  DebugStr
#define DebugStr7                                                              \
  if (BUG7)                                                                    \
  DebugStr
#define DebugStr8                                                              \
  if (BUG8)                                                                    \
  DebugStr
#define DebugStr9                                                              \
  if (BUG9)                                                                    \
  DebugStr
#define DebugStr10                                                             \
  if (BUG10)                                                                   \
  DebugStr
#define DebugStr11                                                             \
  if (BUG11)                                                                   \
  DebugStr
#define DebugStr12                                                             \
  if (BUG12)                                                                   \
  DebugStr
#define DebugStr13                                                             \
  if (BUG13)                                                                   \
  DebugStr
#define DebugStr14                                                             \
  if (BUG14)                                                                   \
  DebugStr
#define DebugStr15                                                             \
  if (BUG15)                                                                   \
  DebugStr
#else
#define DebugStr0(x)
#define DebugStr1(x)
#define DebugStr2(x)
#define DebugStr3(x)
#define DebugStr4(x)
#define DebugStr5(x)
#define DebugStr6(x)
#define DebugStr7(x)
#define DebugStr8(x)
#define DebugStr9(x)
#define DebugStr10(x)
#define DebugStr11(x)
#define DebugStr12(x)
#define DebugStr13(x)
#define DebugStr14(x)
#define DebugStr15(x)
#endif

/**********************************************************************
 * some handy types
 **********************************************************************/
typedef enum { Single, Double, Triple } ClickEnum;
typedef enum {
  OUR_WIN = 0x20, // The first window kind we recognize as our own
  MBOX_WIN = OUR_WIN,
  CBOX_WIN,
  COMP_WIN,
  TEXT_WIN,
  MESS_WIN,
  FIND_WIN,
  ALIAS_WIN,
  PH_WIN,
  MB_WIN,
  PROG_WIN,
  FILT_WIN,
  TBAR_WIN,
  PICT_WIN,
  PREF_WIN,
  ETL_ABOUT_WIN,
  PERS_WIN,
  SIG_WIN,
  STA_WIN,
  TASKS_WIN,
  LOG_WIN,
  SEARCH_WIN,
  AD_WIN,
  LINK_WIN,
  PAY_WIN,
  HELP_WIN,
  STAT_WIN,
  IMPORTER_WIN,
  TOOLBAR_POPUP_WIN,
  DRAWER_WIN,
  LIMIT_WIN
} WKindEnum;

#define CONFIG_KIND(k)                                                         \
  (k == PH_WIN || k == TBAR_WIN || k == FILT_WIN || k == PREF_WIN ||           \
   k == FIND_WIN || k == MB_WIN || k == ALIAS_WIN || k == PERS_WIN ||          \
   k == SIG_WIN || k == STA_WIN || k == LINK_WIN || k == PAY_WIN)
/* Use fixed-width integer types for portability */
/* avoid project-specific aliases in new code — use standard C types where
 * possible */
/* Use standard C types - no Mac typedefs */
/* uShort already defined above */
typedef enum { Production, Debugging, Steve } RunTypeEnum;
#endif
#ifndef HAVE_AEDESCPTR
typedef AEDesc *AEDescPtr;
#define HAVE_AEDESCPTR 1
#endif
#ifndef HAVE_AEADDRESSDESCPTR
typedef AEAddressDesc *AEAddressDescPtr;
#define HAVE_AEADDRESSDESCPTR 1
#endif
#ifndef HAVE_APPLEEVENTPTR
typedef AppleEvent *AppleEventPtr;
#define HAVE_APPLEEVENTPTR 1
#endif
typedef struct MIMEMapStruct *MIMEMapHandle;
#ifndef POPLINETYPE_DEFINED
#define POPLINETYPE_DEFINED
typedef enum { plComplete, plPartial, plEndOfMessage, plError } POPLineType;
#endif
/* forward-declare TransStream so it can be used in typedefs before the full
        definition/header is included later. */
#ifndef TRANSSTREAM_PTR_DEFINED
typedef struct TransStreamStruct TransStreamStruct;
typedef TransStreamStruct *TransStream;
#define TRANSSTREAM_PTR_DEFINED 1
#endif
typedef POPLineType LineReader(TransStream stream, unsigned char *buf,
                               long bSize, long *len);
typedef char *TextAddrHandle;  /* C string handle for address text */
typedef const char *TextAddrPtr;
typedef char *BinAddrHandle;   /* NULL-terminated char** array — free with g_strfreev() */
typedef char *BinAddrPtr;
typedef void *NickHandle;
typedef unsigned char *NickPtr;
typedef void *TabFieldHandle;
typedef unsigned char *TabFieldPtr;

#ifndef MYDEFS_C_2
#define MYDEFS_C_2
typedef struct {
  short flags;
  unsigned long prefSize;
  unsigned long minSize;
} SizeRec, *SizePtr, *SizeHandle;

/************************************************************************
 * transport mechanisms
 ************************************************************************/
/* Forward-declare wdsEntry to avoid pulling in precompiled Mac headers */
typedef struct wdsEntry wdsEntry;

typedef struct {
  int (*vConnectTrans)(TransStream stream, const char *serverName, long port,
                       bool silently, unsigned long timeout);
  int (*vSendTrans)(TransStream stream, const char *text, long size, ...);
  int (*vRecvTrans)(TransStream stream, char *line, long *size);
  int (*vDisTrans)(TransStream stream);
  int (*vDestroyTrans)(TransStream stream);
  int (*vTransError)(TransStream stream);
  void (*vSilenceTrans)(TransStream stream, bool silence);
  int (*vSendWDS)(TransStream stream, wdsEntry *theWDS);
  char *(*vWhoAmI)(TransStream stream, char *who);
  int (*vRecvLine)(TransStream stream, char *line, long *size);
  int (*vAsyncSendTrans)(TransStream stream, char *buffer, long size);
} TransVector;

int ConnectTrans(TransStream stream, const char *serverName, long port,
                 bool silently, unsigned long timeout);

#define ConnectTransLo (*CurTrans.vConnectTrans)
#define SendTrans (*CurTrans.vSendTrans)
#define RecvTrans (*CurTrans.vRecvTrans)
#define DisTrans (*CurTrans.vDisTrans)
#define DestroyTrans (*CurTrans.vDestroyTrans)
#ifndef TransError
#define TransError (*CurTrans.vTransError)
#endif
#define SilenceTrans (*CurTrans.vSilenceTrans)
#define SendWDS (*CurTrans.vSendWDS)
#define WhoAmI (*CurTrans.vWhoAmI)
#define RecvLine (*CurTrans.vRecvLine)
#define AsyncSendTrans (*CurTrans.vAsyncSendTrans)
#endif

// Kerberos client disabled for portable build
// #include "KClient.h"
// #include "KClientCompat.h"
// #include "KClientDeprecated.h"
#if TARGET_RT_MAC_CFM
// #include "GSS.h" // !!! Marshall sez - not yet for MachO
#endif
/* OpenSSL / TransStream handling
 * - Non-portable build: use project OpenSSL wrapper + trans.h
 * - Portable (BUILDING_MBX_LIB): prefer system OpenSSL headers if available,
 *   otherwise provide minimal forward declarations so TransStream and SSL
 *   related types can be referenced without the project's OpenSSL wrapper.
 */
#ifndef BUILDING_MBX_LIB
/* #include "OpenSSL.h" */ /* needed for TransStream - TODO: port TransStream */
/* #include "trans.h" */
#else
#if defined(__has_include)
#if __has_include(<openssl/opensslv.h>) && __has_include(<openssl/types.h>)
#include <openssl/opensslv.h>
#include <openssl/types.h>
#else
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct ssl_method_st SSL_METHOD;
typedef struct X509_name_st X509_NAME;
typedef struct X509_st X509;
#endif
#else
/* Fallback when __has_include not available: provide minimal typedefs */
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct x509_store_ctx_st X509_STORE_CTX;
typedef struct ssl_method_st SSL_METHOD;
typedef struct X509_name_st X509_NAME;
typedef struct X509_st X509;
#endif
/* When building portable, include TransStream after SSL/type setup */
#endif

/* Now include TransStream which depends on types above */
/* Try to include the real TransStream header when available; otherwise
   provide a guarded forward-declaration so `TransStream` is defined only once.
 */
#if !defined(TRANSSTREAM_PTR_DEFINED)
#if defined(__has_include)
#if __has_include("TransStream.h")
#include "TransStream.h"
#endif
#endif
#ifndef TRANSSTREAM_PTR_DEFINED
/* Fallback forward-declaration */
typedef struct TransStreamStruct *TransStream;
#define TRANSSTREAM_PTR_DEFINED 1
#endif
#endif
/* #include "color.h" */ /* TODO: Port color management */
#include "euErrors.h"
/* #include "lineio.h" */   /* TODO: Port line I/O */
/* #include "aeutil.h" */   /* TODO: Port Apple Event utilities */
/* #include "icon.h" */     /* TODO: Port icon management */
#include "prefdefs.h" /* Preference definitions - needed for ACAP and other modules */
#include "StrnDefs.h" /* String resource definitions - needed for ACAP and other modules */
/* #include "nickae.h" */   /* TODO: Port nickname Apple Events */
#ifdef WINTERTREE
#if defined(__has_include)
#if __has_include("ssce.h")
#include "ssce.h"
#endif
#else
/* If compiler doesn't support __has_include, attempt include and let build fail
        only when WINTERTREE truly requires ssce.h */
#include "ssce.h"
#endif
#endif
/* #include "spell.h" */ /* TODO: Port spell checking */
#ifndef ONE
/* #include "pgpin.h" */ /* TODO: Port PGP support */
#endif
#ifdef SPEECH_ENABLED
/* #include "speechutil.h" */ /* TODO: Port speech utilities */
#endif
/* #include "wrappers.h" */ /* TODO: Port wrappers */
/* #include "util.h" */     /* TODO: Port utilities */
/* #include "anal.h" */     /* TODO: Port analysis */
#include "pete_shim.h"

/* #include "menusharing.h" */
/* #include "lex822.h" */
/* #include "header.h" */
/* #include "numcode.h" */
/* #include "MyRes.h" */
/* #include "features.h" */
/* #include "featureldef.h" */
/* #include "downloadurl.h" */
/* #include "nag.h" */
/* #include "StringUtil.h" */
/* #include "StringDefs.h" */
/* #include "adutil.h" */
/* #include "appleevent.h" */
/* #include "cursor.h" */
/* #include "mime.h" */
/* #include "threading.h" */
/* #include "progress.h" */
/* #include "taskProgress.h" */
/* #include "mywindow.h" */
/* #include "peteglue.h" */
/* #include "modeless.h" */
/* #include "ends.h" */
/* #include "functions.h" */
/* #include "appcdef.h" */
/* #include "inet.h" */
/* #include "mailbox.h" */
/* #include "toc.h" */
/* #include "boxact.h" */
/* #include "main.h" */
/* #include "message.h" */
/* #include "messact.h" */
/* #include "navUtils.h" */
/* #include "comp.h" */
/* #include "compact.h" */
/* #include "concentrator.h" */
/* #include "multi.h" */
/* #include "fmtbar.h" */
/* #include "register.h" */
/* #include "search.h" */
/* #include "proxy.h" */
/* #include "schizo.h" */
/* #include "sendmail.h" */
/* #include "filegraphic.h" */
/* #include "pop.h" */
/* #include "tcp.h" */
/* #include "ctb.h" */
/* #include "menu.h" */
/* #include "shame.h" */
/* #include "sort.h" */
/* #include "tefuncs.h" */
/* #include "fileutil.h" */
/* #include "winutil.h" */
/* #include "tabmania.h" */
/* #include "labelfield.h" */
#ifdef VCARD
/* #include "vcard.h" */
#endif
/* #include "nickwin.h" */
/* #include "nickexp.h" */
/* #include "paywin.h" */
/* #include "peteuserpane.h" */
/* #include "oops.h" */
/* #include "listview.h" */
/* #include "find.h" */
/* #include "searchwin.h" */
/* #include "text.h" */
/* #include "address.h" */
/* #include "print.h" */
/* #include "nickmng.h" */
/* #include "binhex.h" */
/* #include "hexbin.h" */
/* #include "ph.h" */
/* #include "utl.h" */
/* #include "prefs.h" */
/* #include "mbwin.h" */
/* #include "buildtoc.h" */
/* #include "squish.h" */
/* #include "uudecode.h" */
/* #include "uupc.h" */
/* #include "md5.h" */
/* #include "lmgr.h" */
/* #include "log.h" */
/* #include "filtdefs.h" */
/* #include "filters.h" */
/* #include "filtmng.h" */
/* #include "filtwin.h" */
/* #include "filtrun.h" */
/* #include "link.h" */
/* #include "rich.h" */
/* #include "mstore.h" */
/* #include "msmaildb.h" */
/* #include "msiddb.h" */
/* #include "mstoc.h" */
/* #include "msinfo.h" */
#ifndef ONE
/* #include "pgpout.h" */
#endif
/* #include "url.h" */
/* #include "mailxfer.h" */
/* #include "adwin.h" */
/* #include "toolbar.h" */
/* #include "toolbarpopup.h" */
/* #include "html.h" */
/* #include "TransStream.h" */
/* #include "listcdef.h" */
/* #include "wazoo.h" */
/* #include "personalitieswin.h" */
/* #include "signaturewin.h" */
/* #include "stationerywin.h" */
/* #include "floatingwin.h" */
/* #include "floatingwin.h" */
/* #include "stickypopup.h" */
/* #include "appear_util.h" */
/* #include "makefilter.h" */
/* #include "table.h" */
/* #include "acap.h" */
/* #include "ldaputils.h" */
/* #include "filtthread.h" */
/* #include "mail.h" */
/* #include "env.h" */
/* #include "fs.h" */
/* #include "misc.h" */
/* #include "imapnetlib.h" */
/* #include "imapmailboxes.h" */
/* #include "imapdownload.h" */
/* #include "imapconnections.h" */
/* #include "imapauth.h" */
/* #include "NetworkSetup.h" */
/* #include "MoreNetworkSetup.h" */
/* #include "networksetuplibrary.h" */
/* #include "linkwin.h" */
/* #include "linkmng.h" */
/* #include "dial.h" */
/* #include "import.h" */
/* #include "export.h" */

/* #include "spool.h" */
/* #include "unicode.h" */

/* #include "audit.h" */
/* #include "auditdefs.h" */

/* #include "graph.h" */
/* #include "statmng.h" */
/* #include "statwin.h" */
/* #include "xml.h" */
/* #include "scriptmenu.h" */
/* #include "carbonutil.h" */
/* #include "fileview.h" */
/* #include "palmconduitae.h" */
/* #include "junk.h" */

/* #include "sasl.h" */
/* #include "mbdrawer.h" */
#include "emoticon.h"

/* #include "osxabsync.h" */

//	SSLCerts.h includes SSL.h, which requires a bunch of #defines
//	before you include it.
//	in SSLCerts.h
/* bool	CanDoSSL ( void ); */

#ifdef DEMO
/* #include "timebomb.h" */
#endif

/* #include "regcode_eudora.h" */

/* #include "light.h" */

/* #include "Globals.h" */

/* void DebugSomething(void); */
