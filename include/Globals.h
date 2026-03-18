/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
the limitations in the disclaimer below) provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE. */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include "mailbox.h"
#include "message.h"
#include "pete_portable.h"
#include "threading.h"
#include "features.h"
#include "trans.h"

/**********************************************************************
 * Global variables — ported from Mac original.
 * Mac-specific types replaced with portable equivalents:
 *   TOCHandle    -> TOCType *
 *   Handle       -> void *  (or typed pointer)
 *   GrafPtr      -> void *
 *   WindowPtr    -> void *
 *   MyWindowPtr  -> void *
 *   MenuHandle   -> void *
 *   RgnHandle    -> void *
 *   bool      -> bool
 *   unsigned char         -> unsigned char
 *   int        -> int
 *   void -> void *
 *   EventRecord  -> void *  (unused in GTK)
 *   Style        -> unsigned char
 *   Rect         -> struct { short top, left, bottom, right; }
 **********************************************************************/

/* ---- Font parameters ---- */
extern int FontID;
extern int FontSize;
extern int FixedID;
extern int FixedSize;
extern int FontWidth;
extern int FontLead;
extern int FontDescent;
extern int FontAscent;

/* ---- Application state ---- */
extern bool Done;
extern bool DontTranslate;
extern bool FontIsFixed;
extern short InBG;
extern bool NoSaves;
extern bool SFWTC;
extern bool ScrapFull;
extern bool UseCTB;
extern bool Offline;
extern bool Stationery;
extern bool AmQuitting;
extern bool WrapWrong;
extern bool HasHelp;
extern bool HasPM;
extern bool NewTables;
extern bool LockCode;
extern bool CantLock;
extern bool Toshiba;
extern bool LooseTrans;
extern bool QTMoviesInited;

/* ---- Mailbox/message lists ---- */
extern void *BoxCount;              /* BoxCountHandle — list of mailboxes for find */
extern size_t BoxCountSize;         /* byte size of BoxCount buffer */
extern void *XfUndoH;              /* XfUndoHandle — for undoing transfers */
extern void *InsurancePort;        /* GrafPtr — fallback port (unused in GTK) */
extern MessHandle MessList;
extern void *HandyMyWindow;        /* MyWindow * — spare window record */
extern void *MousePen;             /* RgnHandle — pen for mouse */
extern bool TBTurnedOnBalloons;
extern unsigned char NewLine[4];
extern unsigned char CheckOnIdle;
extern char Type2SelString[16];
extern uLong Type2SelTicks;
extern short DragSumNum;
extern TOCType *DragTOCFrom;
extern TOCType *DragTOCTo;
extern short DragModsWere;
extern uLong DragSequence;
extern short StationVRef;
extern long StationDirId;
/* PETE — now a macro in pete_portable.h: #define PETE ((PETEInst)NULL) */

#ifdef DEBUG
extern long ____RandomFailThresh;
#endif

#define kCheck 0x02
#define kSend 0x01
extern TOCType *TOCList;

extern struct AliasDStruct **Aliases;

extern unsigned char * eSignature;
extern unsigned char * RichSignature;
extern unsigned char * HTMLSignature;
extern bool SigStyled;
extern unsigned char * TransIn;
extern unsigned char * TransOut;
extern unsigned char * Flatten;
extern int CTBTimeout;
extern void *HIQ;                   /* HostInfoQHandle */
extern bool NoPreflight;
extern bool NoNewMailMe;
extern short Dragging;
extern PETEStyleListHandle Pslh;
extern int SendQueue;
extern uLong ForceSend;
extern short *StdSizes;
extern short *FixedSizes;
extern struct BoxMapStruct *BoxMap;
extern size_t BoxMapSize;
extern short *BoxWidths;
extern short AliasRefCount;
extern short ICMPAvail;
extern short RunType;
extern void *MyNMRec;              /* NMRec * */
extern void *CheckedMenu;          /* MenuHandle */
extern short CheckedItem;
extern void *SpareSpace[4];        /* NSpare = 4 in original */
extern bool DoMonitor;
extern bool EjectBuckaroo;
extern bool GrowZoning;
extern unsigned short WhyTCPTerminated;
extern void *MainEvent;            /* EventRecord — unused in GTK */
extern bool NoInitialCheck;
extern long YieldTicks;
extern bool HesOK;
extern uLong GlobalIdleTicks;
extern bool NoAnalDictionary;
extern bool WashMe;
extern void *ModalWindow;          /* WindowPtr */
/* Root is declared as RootSpec in mailbox.h */
extern VDId MailRoot;
extern VDId IMAPMailRoot;
extern VDId ItemsFolder;
#define UnreadStyle fontItalic      /* also in mailbox.h */
extern short LogRefN;
extern long LogLevel;
extern long LogTicks;
extern Accumulator AuditAccu;
extern void *FMBMain;              /* void * */
#define AppResFile 0               /* also in mailbox.h */
extern short HelpResFile;
extern void *Filters;              /* Handle — filter rules */
extern short FiltersRefCount;
extern void *PreFilters;
extern void *PostFilters;
extern void *WordServices;         /* void ** */
extern short OriginalHelpCount;
extern short EndHelpCount;
extern char IsWordChar[256];
extern short PrefPlugEnd;
extern long TypeToOpen;
extern void *UglyHackFrontWindow;  /* WindowPtr */
extern char MyHostname[128];
extern TOCType *DamagedTOC;
extern bool ThereIsColor;
extern bool NoDominant;
extern void *ICache;               /* void * */
extern bool VM;
extern bool BreakMe;
extern uLong Yesterday;
extern bool MemCanFail;
extern short FakeTabs;
extern void *WrapHandle;           /* void **/
extern short ClickType;
extern short Windex;
extern short SysRefN;
extern bool StartingUp;
extern bool SyncRW;
extern bool AutoDoubler;
extern bool PrefsPlugIns;
extern bool TBarHasChanged;
extern bool D3;
extern bool Typing;
extern bool TypingRecently;
extern short PlaylistNagCount;
extern short NewClientModePlusOne;
extern long TypingTicks;
extern long ActiveTicks;
extern long NonNullTicks;
extern bool OpenedMacSLIP;
extern char Re[16];
extern char Fwd[16];
extern char OFwd[16];
extern unsigned char TOCInversionMatrix[2][16]; /* BoxLinesLimit */
extern bool DragFxxkOff;
extern bool Sensitive;
extern bool WholeWord;
extern bool FurrinSort;
extern void *DragSource;           /* MyWindowPtr */
extern void *DragTOCSource;        /* MyWindowPtr */
extern short DragSourceKind;
extern bool EmoTurdCache;
extern bool AttentionNeeded;
extern unsigned char YesStr[2];
extern unsigned char NoStr[2];
extern unsigned char Slash[3];
extern unsigned char Cr_bytes[2];  /* renamed to avoid Cr macro conflict */
extern unsigned char Lf[2];
extern unsigned char CrLf[3];
extern bool OTIs;
extern bool OptiMEMIs;
extern bool CheckNow;
extern long StupidTagForACAPandI4;
extern uLong gCheckSessionID;

#ifdef DEBUG
extern short BugFlags;
#define BUG0 ((BugFlags&(1<<0))!=0)
#define BUG1 ((BugFlags&(1<<1))!=0)
#define BUG2 ((BugFlags&(1<<2))!=0)
#define BUG3 ((BugFlags&(1<<3))!=0)
#define BUG4 ((BugFlags&(1<<4))!=0)
#define BUG5 ((BugFlags&(1<<5))!=0)
#define BUG6 ((BugFlags&(1<<6))!=0)
#define BUG7 ((BugFlags&(1<<7))!=0)
#define BUG8 ((BugFlags&(1<<8))!=0)
#define BUG9 ((BugFlags&(1<<9))!=0)
#define BUG10 ((BugFlags&(1<<10))!=0)
#define BUG11 ((BugFlags&(1<<11))!=0)
#define BUG12 ((BugFlags&(1<<12))!=0)
#define BUG13 ((BugFlags&(1<<13))!=0)
#define BUG14 ((BugFlags&(1<<14))!=0)
#define BUG15 ((BugFlags&(1<<15))!=0)
#endif

extern long SpinSpot;
extern void *InsertWin;            /* MyWindowPtr */
extern FSSpec AttFolderSpec;
extern TransVector UUPCTrans;
extern TransVector OTTCPTrans;
extern struct MTS *pendingCloses;
extern bool gUseOT;
extern bool gHasOTPPP;
extern bool gPPPConnectFailed;
extern FSSpec TCPprefFileSpec;
extern FSSpec PPPprefFileSpec;
extern bool gMissingNSLib;
extern long gActiveConnections;
extern bool gConnecting;
extern bool gStayConnected;
extern void *TransContextStack;    /* StackHandle */
extern void *MBRenameStack;        /* StackHandle */
extern long MemLastFailed;
extern long LastTotalSpace, LastContigSpace;
extern bool EmptyRecip;
extern short NoScannerResets;
extern bool DirtyHackForChooseMailbox;
extern bool OpenAddrErrs;
#ifdef WINTERTREE
extern short SpellSession;
extern uLong WinterTreeOptions;
#endif
extern bool gHasCMM;
extern bool gHave85MenuMgr;

/* ---- Appearance Manager ---- */
extern bool gAppearanceIsLoaded;
extern bool gUseAppearance;
extern bool gGoodAppearance;
extern bool gBetterAppearance;
extern bool gBestAppearance;
extern int32_t gLastCtlValue;
extern bool gUseLiveScroll;
extern bool gAXIsSupported;

extern bool VicomIs;
extern long VicomFactor;
extern bool NoMenus;
extern bool PleaseQuit;
extern bool g16bitSubMenuIDs;
extern short gMaxBoxLevels;
extern void *gIMAPConnectionPool;  /* IMAPConnectionHandle */
extern char gIMAPErrorString[256];
extern bool gbDisplayIMAPWarnings;
extern unsigned char FunctionKeys[];

/************************************************************************
 * mimestore declarations
 ************************************************************************/
extern void *MSSubs;               /* void[] — simplified */

/**********************************************************************
 * temp vars for macros
 **********************************************************************/
extern uLong M_T1, M_T2, M_T3;

/**********************************************************************
 * thread globals
 **********************************************************************/
extern threadGlobalsRec ThreadGlobals;
extern _Thread_local threadGlobalsPtr CurThreadGlobals;

extern short TempInCount;
extern short NeedToFilterIn;
extern short NeedToFilterOut;
extern short NeedToNotify;
extern short NeedToFilterIMAP;
extern bool NoXfer;
extern bool SendImmediately;
extern atomic_bool CheckThreadRunning;
extern atomic_bool SendThreadRunning;
extern threadDataHandle gThreadData;
extern short IMAPCheckThreadRunning;
extern short gNewMessages;
extern bool gSkipIMAPBoxes;
extern bool gWasManualIMAPCheck;
extern uLong LastCheckTime;
extern bool TaskDontAutoClose;
extern MyWindowPtr TaskProgressWindow;
extern bool gTaskProgressInitied;
extern long ThreadYieldTicks;
extern int CheckThreadError;
extern int SendThreadError;
extern bool DFWTC;

extern int TotalQueuedSize;
extern char P1[256], P2[256], P3[256], P4[256];
extern bool NewError;
extern long BgYieldInterval;
extern long FgYieldInterval;
extern long GroupSubjThreshTime;

#ifdef NAG
extern void *nagState;             /* void * */
extern long gHighestAppVersionAtLaunch;
#endif

extern void * gFeatureList;
extern bool gNeedRemind;
extern char *gRegFiles;           /* char ** simplified */

extern void *normFonts;            /* void — unused in GTK */
extern void *monoFonts;
extern void *printNormFonts;
extern void *printMonoFonts;

extern bool gImportersAvailable;
extern bool gScreenChange;
extern bool gMenuBarIsSetup;
extern void *Proxies;              /* void * */
extern void *CompactStack;         /* StackHandle */
extern FSSpec SettingsSpec;

extern bool gCanPayMode;
#define CurPersSafe PERS_FORCE(CurPers)
extern short gEnterWheelHandlerCount;
extern bool UsingAnyWindows;
extern short ActiveSearchCount;
extern long AnyTOCDirty;
extern GArray *OutgoingMIDList;  /* GArray of uint32_t hashes */
extern bool OutgoingMIDListDirty;
extern AccuPtr ExportErrors;

/* ---- Stationery/signature ---- */
extern short pStationeryLabel;
extern bool UseInlineSig;

/* historyAddressBook is an enum in nickmng.h, not a global */

/* ---- Personalities ---- */
#include "schizo.h"
/* PersList: when THREADING_ON, it's a macro in threading.h.
   Otherwise it's an extern in schizo.h. No need to redeclare here. */

#endif /* GLOBALS_H */
