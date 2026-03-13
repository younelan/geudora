/*
 * globals.c — Global variables for Eudora, GTK4 port.
 *
 * Original had ~350 lines of Mac-specific globals. This defines only
 * the globals actually referenced by the ported code.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "mailbox.h"
#include "message.h"
#include "threading.h"

/* ---- Font parameters ---- */
int FontID;
int FontSize;
int FixedID;
int FixedSize;
int FontWidth;
int FontLead;
int FontDescent;
int FontAscent;
bool FontIsFixed;

/* ---- Application state ---- */
bool Done;           /* set to true when we're done */
bool NoSaves;        /* don't prompt user to save documents */
bool Offline;        /* are we offline? */
bool AmQuitting;     /* are we quitting? */
bool StartingUp;     /* starting up? */
short InBG;          /* are we in the background? */
bool NoMenus = true; /* disable menus during init */

/* ---- Search state ---- */
bool Sensitive;      /* case-sensitive search? */
bool WholeWord;      /* whole word search? */

/* ---- Message / TOC lists ---- */
MessHandle MessList;       /* list of open messages */
short ClickType;           /* single, double, or triple click */

/* ---- Transfer undo ---- */
short DragSumNum;

/* ---- Logging ---- */
short LogRefN;
long LogLevel;

/* ---- Network ---- */
long gActiveConnections = 0;
bool gConnecting = false;

/* ---- Filter state ---- */
void *Filters;           /* filter rules */
short FiltersRefCount;   /* reference count for filters */

/* ---- Send queue ---- */
int SendQueue;
unsigned long ForceSend;

/* ---- Attachment folder ---- */
FSSpec AttFolderSpec;

/* ---- Signature ---- */
void *eSignature;
void *RichSignature;
void *HTMLSignature;

/* ---- Prefs/UI state ---- */
bool DontTranslate;
short FakeTabs;
bool Typing;
bool TypingRecently;
long TypingTicks;
long ActiveTicks;
bool WashMe;
bool EjectBuckaroo;
bool TBarHasChanged;
bool PleaseQuit;
short RunType;

/* Some threading globals defined in ends.c, mailxfer.c, imapdownload.c */
/* The rest defined here: */
short TempInCount;
short NeedToFilterOut;
short NeedToNotify;
bool gSkipIMAPBoxes;
long ThreadYieldTicks;
bool CheckThreadRunning = false;
int CheckThreadError = 0;
int SendThreadError = 0;

/* ---- Misc ---- */
bool DragFxxkOff;
short NoScannerResets;
bool NoInitialCheck;
unsigned char Re[16] = {3, 'R', 'e', ':'};
unsigned char Fwd[16] = {4, 'F', 'w', 'd', ':'};

/* ---- Standard font sizes ---- */
static short StdSizesData[] = { 9, 10, 12, 14, 18, 24, 36, 48, 64, 72 };
static short *StdSizesPtr = StdSizesData;
short **StdSizes = &StdSizesPtr;

/* PersList lives inside ThreadGlobals.tPersList when THREADING_ON */

/* ---- Stationery/signature ---- */
short pStationeryLabel;
bool UseInlineSig;

/* historyAddressBook is an enum in nickmng.h */

/* ---- Threading/mail check globals ---- */
MyWindowPtr TaskProgressWindow;
bool gTaskProgressInitied;
short NeedToFilterIn;
short IMAPCheckThreadRunning;
short gNewMessages;
bool NoXfer;
bool SendImmediately;
bool SendThreadRunning;
bool NoNewMailMe;
bool gStayConnected;
bool HesOK;
bool gWasManualIMAPCheck;
bool gPPPConnectFailed;
unsigned long gCheckSessionID;
unsigned long LastCheckTime;
long NonNullTicks;
int TotalQueuedSize;
bool TaskDontAutoClose;
bool DFWTC;

/* ---- Translation tables ---- */
unsigned char *TransOut;
unsigned char *TransIn;
unsigned char *Flatten;
bool NewTables;
bool UseCTB;
int CTBTimeout;

/* ---- Misc globals ---- */
void *ModalWindow;
unsigned char NoStr[2];
unsigned char YesStr[2];
unsigned char Cr_bytes[2];
unsigned char Lf[2];
unsigned char CrLf[3];
unsigned char Slash[3];
bool CheckNow;
bool OpenAddrErrs;
bool AttentionNeeded;
long YieldTicks;
unsigned long GlobalIdleTicks;
bool MemCanFail;
bool SyncRW;
bool D3;
bool ScrapFull;
bool SFWTC;

/* ---- Temp vars for macros ---- */
unsigned long M_T1;
unsigned long M_T2;
unsigned long M_T3;
