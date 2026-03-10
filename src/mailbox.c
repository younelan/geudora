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

#include "mailbox.h"
#include "../include/pete_shim.h"
#include "Globals.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "features.h"
#include "fileutil.h"
#include "junk.h"
#include "legacy_shim.h"
#include "lineio.h"
#include "mesg_error_store.h"
#include "mydefs.h"
#include "pop.h"
#include "sort.h"
#include "toc.h"
#include "trans.h"
#include <assert.h>
#include <stdarg.h>
#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif
#include "gtk_menus.h"
#include "imapmailboxes.h" /* For IsIMAPCacheFolder */
int AddMesgError(TOCType * tocH, short sum, unsigned char *errorStr,
                 int errorCode);
#include "message.h"  /* For MyWindow struct */
#include "prefdefs.h" /* For PREF_THREADING_OFF */
#include <stdlib.h>

#include "gtk_menus.h"
#include "log.h"
#include "uudecode.h"
#include <stdbool.h>
int ReallyDoAnAlert(int templ, int which);
#ifndef kStatReceivedMail
#define kStatReceivedMail 0
#endif

/* Missing Definitions */
#ifndef WindowPtr
typedef void *WindowPtr;
#endif
typedef WindowPtr DialogPtr; /* Added */

/* Re-define kAMOAvoidAll */
#undef kAMOAvoidAll
#define kAMOAvoidAll 2
#ifndef smSystemScript
enum { smSystemScript = 0 };
#endif
extern short BoxMapCount;
enum { kStatReadMsg = 0 };
#ifndef nil
#define nil 0
#endif
#define userCanceledErr -128
enum { OPT_OPEN = 1, OPT_AUTO_OPENED = 2 };
/* MAILBOX_MENU defined in gtk_menus.h */

/* Missing Structs */
/* Missing Definitions / Decls */
/* FSSpec defined in headers */
/* MemError defined in headers */
/* PCopyTrim defined in headers */
/* ResolveAliasOrElse defined in headers */
#define BoxFOpen BoxFOpenImp
#define LOG_FLOW 7

/* Missing definitions restored */
#ifndef noErr
#define noErr 0
#endif
extern long AnyTOCDirty;
typedef unsigned long uLong;
struct MenuAndScore;
typedef struct MenuAndScore **MenuAndScoreHandle;
void BoxCenterSelection(WindowPtr win);

int BoxMatchMenuItems(unsigned char *name, MenuAndScoreHandle *mashPtr,
                      int score());
int BoxMatchMenuItemsInMenu(MenuHandle mh, AccuPtr a, unsigned char *name,
                            int score());
int BoxMatchMenuItemsIn1Menu(MenuHandle mh, AccuPtr a, unsigned char *name,
                             int score());

// ... existing declarations ...

// Fix pointer math in MailboxAlias/SpecAlias usage area
#define MAILBOX_ALIAS_MATH(leaf, spot, name)                                   \
  MakePStr(leaf, spot + 1, *name - ((char *)spot - name))
char *MailboxAlias(short which, char *name);
char *MailboxSpecAlias(FSSpecPtr spec, char *name);
void BuildBoxMenus(void);
bool DeleteSum(TOCType * tocH, int sumNum);
void MakeMessTitle(unsigned char *name, TOCType * tocH, int sumNum, bool b);
bool IsQueued(TOCType * tocH, int sumNum);

/* New Missing Decls */
void MBTickle(void *u1, void *u2);
void AddBoxHigh(FSSpecPtr spec);
/* AttIsSelected declared in messact.h (via junk.h) */
unsigned char *PeteSelectedString(void *u1, void *pte);
bool MenuItemIsSeparator(MenuHandle mh, short item);
unsigned char *CollapseLWSP(unsigned char *s);
void AppendMenu(MenuHandle mh, const unsigned char *item);
void SetMenuItemCommandID(MenuHandle mh, short item, long id);
void SetMenuItemHierarchicalMenu(MenuHandle mh, short item, MenuHandle subMenu);
void SelectBoxRange(TOCType * tocH, int start, int end, bool add, int u1,
                    int u2);

/* Restored declarations */
/* GetRString declared in gtk_dialogs.h */
/* HGetState/HSetState provided by legacy_shim.h */
char *FindHeaderString(char *text, char *header, long *len, bool caseSens);
void MBDrawerOpen(MyWindowPtr win);
/* TOCType * GetOutTOC(void); - Redundant macro conflict */
void BeautifyFrom(unsigned char *who);
MyWindowPtr GetNewMyDialog(short id, void *w, void *h, void *behind);
void CycleBalls(void);
long GetPrefLong(short pref);
/* SetWTitle declared in mailbox.h */
/* GetAMessage declared in message.h */
void RemoveUTF8FromSum(MSumPtr sum);
void SetHandleBig(Handle h, long size);
long TempMaxMem(long *grow);
short FindDirLevel(short vRefNum, long dirID);
/* DeleteMenuItem, EnableItem, etc. are in gtk_menus.h */
/* FindItemByName, MyGetItem, CountMenuItems, etc. are in gtk_menus.h */
void HideDialogItem(DialogPtr dp, short item);
/* MyParamText declared in log.h with PStr params */
void StartMovableModal(DialogPtr dp);
void ShowWindow(WindowPtr win);
/* Cursor and Dialog Decls */
WindowPtr GetDialogWindow(DialogPtr dp);
#define GetMyWindowDialogPtr(win) ((DialogPtr)GetMyWindowWindowPtr(win))
/* Legacy Dialog Functions */
/* CloseMyWindow provided by legacy_shim.h */
TOCType * FindTOC(FSSpecPtr spec);
void Box2TOCSpec(FSSpecPtr spec, FSSpecPtr tocSpec);
void utl_SaveWindowPos(WindowPtr win, Rect *r, bool *zoomed);
UPtr GetMailboxName(TOCType * tocH, short sum, UPtr name);
/* UpdateIMAPMailbox declared in mailbox.h */

void GetPortBounds(void *port, Rect *r);
void *GetWindowPort(WindowPtr win);
typedef struct BoxMapStruct BoxCountElem; /* Guessing same layout */
void GetMenuItemText(MenuHandle mh, short item, unsigned char *text);
/* Restored Function Declarations */
void utl_RestoreWindowPos(WindowPtr win, Rect *r, bool zoomed, short u1,
                          short u2, short u3, void *cb1, void *cb2);
short MessWi(MyWindowPtr win);
typedef struct BoxMapStruct BoxMapType;
short TitleBarHeight(WindowPtr win);
short LeftRimWidth(WindowPtr win);
void FigureZoom(void);
void DefPosition(void);
void AddBoxCountItem(short item, short vRef, long dirId);
void AddBoxCountMenu(short menu, short item, short vRef, long dirId,
                     bool force);
void **PtrPlusHand(const void *ptr, void **hand, long size);
void MovableModalDialog(DialogPtr dp, void *filter, short *item);
void SetDItemState(DialogPtr dp, short item, bool state);
bool GetDItemState(DialogPtr dp, short item);
void PopCursor(void);
void GetDIText(DialogPtr dp, short item, char *text);
bool BadMailboxName(FSSpecPtr spec, bool folder);
bool BadMailboxNameChars(FSSpecPtr spec);
void TooLong(unsigned char *name); /* Fixed const */
void EndMovableModal(DialogPtr dp);
void MyDisposeDialog(DialogPtr dp);
short HRename(short vRefNum, long dirID, const unsigned char *oldName,
              const unsigned char *newName);
/* Note is already #defined as 1 in headers */
struct BoxMapStruct {
  short vRef;
  long dirId;
  short item;
};
void HiliteButtonOne(DialogPtr dp);
void SelectDialogItemText(DialogPtr dp, short item, short start, short end);
void PushCursor(void *cursor);
extern void *iBeamCursor; /* Likely global */

/* GetItemStyle, SetItemStyle, CountMenuItems, SubmenuId are in gtk_menus.h */
bool IsQueuedState(int state);
void MenuID2VD(short menuID, short *vRef, long *dirID);

#define LOG_FILT 6 /* Stub */
bool DeleteSum(TOCType * tocH, int sumNum);
void CycleBalls(void);
long GetPrefLong(short pref);
/* SetWTitle, ShowMyWindow, UserSelectWindow, GetNewMyWindow, OpenMailbox,
   InitMailboxWin, MyDisposeWindow — all declared in mailbox.h */
bool IsWindowVisible(WindowPtr win);
long TOCDelDup(TOCType * tocH);
TOCType * CheckTOC(FSSpecPtr spec);
TOCType * GetTOCFromSearchWin(FSSpecPtr spec);

/* Map legacy types to shim types */
/* mesgErrorPtr and MesgErrorType moved to top */
/* mesgErrorHandle is defined in legacy_shim.h */

typedef enum {
  kDoAdd,
  kDoDelete,
  kDoUpdate,
  kDoDeleteAttachments,
  kDoCopy
} IMAPUpdateType;

typedef struct MenuAndScore {
  short menu;
  short item;
  long score;
} MenuAndScore, *MenuAndScorePtr, **MenuAndScoreHandle;

void ZeroMailbox(TOCType * tocH);
int AddBoxMap(short vRef, long dirId);
bool WantRebuildTOC(UPtr boxName, int why);
void AddBox(short function, UPtr name, short level, bool unread);
void RemoveBox(short function, UPtr name, short level);
/* Forward declaration to ensure calls earlier in this file see the
 * canonical implementation declared/defined here (mailbox.h may vary
 * between platforms). Kept local to this translation unit.
 */
/* AddMesgError implementation is below */
int BoxSpecByNameInMenu(MenuHandle mh, FSSpecPtr spec, unsigned char *name);
long TOCDelEmpty(TOCType * tocH);
short FindBoxByNameIn1Menu(MenuHandle mh, unsigned char *name);
int RedoWho(TOCType * tocH, short sumNum);
int ChainTrash(FSSpecPtr spec);
void SetSumColorLo(TOCType * tocH, short sumNum, short color);
void SetStateLo(TOCType * tocH, int sumNum, int state);
void SetState(TOCType * tocH, int sumNum, int state);

int BoxMatchScore(unsigned char *name, unsigned char *candidate);
bool IsFromLine(unsigned char *line);
int BoxMatchMenuItemsIn1Menu(MenuHandle mh, AccuPtr a, unsigned char *name,
                             int score());
int CompareMAS(MenuAndScorePtr mas1, MenuAndScorePtr mas2);
void SwapMAS(MenuAndScorePtr mas1, MenuAndScorePtr mas2);
static void ProcessIMAPChanges(Handle sumList, TOCType * toc,
                               IMAPUpdateType message);
static int IMAPRecvLine(TransStream stream, UPtr buffer, long *size);
void DeleteIMAPSum(TOCType * tocH, int sumNum);
void DeleteMessageLo(TOCType * tocH, int sumNum, bool nuke);
void DecodeIMAPMessages(TOCType * tocH, FSSpecPtr spec);

/* Allocate/free helpers that replace legacy "Handle" based NuHandle
 * allocation. These keep mailbox.c self-contained and use standard
 * malloc/calloc so we don't touch other files.
 */
/* Use the project's portable Handle APIs rather than redefining Mac
 * structs here. Allocate a fixed buffer large enough for expected
 * mesg-error contents (uid + 256-byte pascal string + error code).
 */
#define MESG_ERR_BUF_SIZE (sizeof(uLong) + 256 + sizeof(int))

static LineIOP Lip;
static long gIMAPMsgEnd;

/************************************************************************
 * TOCSetDirty - set the dirty bit
 ************************************************************************/
void TOCSetDirty(TOCType * tocH, bool dirty) {
  tocH->durty = dirty;
  AnyTOCDirty++;
}

/************************************************************************
 * AddOutgoingMesgError
 ************************************************************************/
int AddOutgoingMesgError(short sumNum, uLong uidHash, int errorCode,
                         int template, ...) {
  TOCType * tocH = NULL;
  TOCType * tempTocH = NULL;
  short outSumNum = sumNum;
  Str255 fmtdError, error;
  va_list args;

#ifdef THREADING_ON

  if (InAThread()) {
    tocH = GetRealOutTOC();
    tempTocH = GetTempOutTOC();
    outSumNum = FindSumByHash(tocH, uidHash);
  } else
    tempTocH = tocH = GetRealOutTOC();
#else
  tempTocH = tocH = GetOutTOC();
#endif

  if (tempTocH && tocH) {
    // get message error string
    GetRString((char *)error, template);

    va_start(args, template);
    (void)VaComposeStringDouble(fmtdError, sizeof(fmtdError) - 1, error, args,
                                NULL, 0, NULL);
    va_end(args);

    // attach mesg resource to real out toc
    if (fmtdError[0])
      AddMesgError(tocH, outSumNum, fmtdError, errorCode);
    // mark state of temp out toc entry since thread will update real out toc
    // entry
    SetState(tempTocH, sumNum, MESG_ERR);
  } else
    return (-1);
  return (noErr);
}

/************************************************************************
 * DeleteMesgError
 ************************************************************************/

int DeleteMesgError(TOCType * tocH, short sum) {
  mesgErrorHandle mesgErrH;

  if ((mesgErrH = (mesgErrorHandle)tocH->sums[sum].mesgErrH)) {
    /* Free using standard allocation primitives: free the inner
     * mesgError struct, then free the handle (pointer-to-pointer).
     */
    if (*mesgErrH) {
      free(*mesgErrH);
      *mesgErrH = NULL;
    }
    free(mesgErrH);
    tocH->sums[sum].mesgErrH = NULL;
    /* persist changes to sidecar */
    mesg_error_store_save_all(tocH);
  }
  return noErr;
}

/************************************************************************
 * AddMesgError
 ************************************************************************/

int AddMesgError(TOCType * tocH, short sum, unsigned char *errorStr,
                 int errorCode) {
  int err = noErr;
  (void)err; /* Error ignored according to legacy comment below */
  mesgErrorHandle mesgErrH = NULL;

  /* tocH and sum should be valid, mesgErrH should be empty */
  ASSERT(tocH && (sum != -1) && !tocH->sums[sum].mesgErrH &&
         (sum < tocH->count));
  if (!(tocH && (sum != -1) && (sum < tocH->count)))
    return -1;

  /* if for some reason, mesgErrH isn't empty, overwrite it */
  mesgErrH = (mesgErrorHandle)tocH->sums[sum].mesgErrH;
  if (!mesgErrH) {
    mesgErrorPtr inner = (mesgErrorPtr)calloc(1, sizeof(MesgErrorType));
    if (inner) {
      mesgErrH = (mesgErrorHandle)malloc(sizeof(mesgErrorPtr));
      if (mesgErrH) {
        *mesgErrH = inner;
      } else {
        free(inner);
      }
    }
    if (!mesgErrH)
      err = MemError();
  }
  if (mesgErrH) {
    /* For the GTK port we do not write message-error resources to
     * classic Mac resource forks. Keep the mesgError in memory
     * attached to the TOC so the UI can display errors. Persistence
     * of these errors can be added later via sidecar files.
     */
    if (errorStr)
      PCopyTrim((*mesgErrH)->errorStr, errorStr, sizeof((*mesgErrH)->errorStr));
    (*mesgErrH)->uidHash = tocH->sums[sum].uidHash;
    (*mesgErrH)->errorCode = errorCode;
  }
  // let's ignore the error since we can set the mesg state
  tocH->sums[sum].state = MESG_ERR;
  tocH->sums[sum].mesgErrH = (void *)mesgErrH;
  TOCSetDirty(tocH, true);
  tocH->reallyDirty = true;
  /* persist current mesg error state to sidecar */
  mesg_error_store_save_all(tocH);
  return (noErr);
}

/************************************************************************
 * FillMesgErrors - fill toc
 ************************************************************************/

int FillMesgErrors(TOCType * tocH) {
  /* Load per-mailbox JSON sidecar and populate in-memory mesgErrH entries.
   * Legacy resource-fork persistence removed during GTK port.
   */
  ASSERT(tocH);
  if (!tocH)
    return paramErr;
  return mesg_error_store_load(tocH);
}

/**********************************************************************
 * GetMailbox - put a mailbox window frontmost; open if necessary
 **********************************************************************/
int GetMailbox(FSSpecPtr spec, bool showIt) {
  TOCType * toc;

  if (ResolveAliasOrElse(spec, nil, nil))
    return (userCanceledErr);

  // if this is an IMAP folder we're going to open, adjust the spec so it points
  // to the mailbox inside
  if (IsIMAPCacheFolder(spec))
    spec->parID = SpecDirId(spec);

  if ((toc = TOCBySpec(spec))) {
    WindowPtr tocWinWP;
    tocWinWP = GetMyWindowWindowPtr(toc->win);
    UsingWindow(tocWinWP);
    if (showIt) {
      if (!IsWindowVisible(tocWinWP)) {
        ShowMyWindow(tocWinWP);

        // if we're showing an IMAP mailbox, resync it.
        if (toc->imapTOC) {
          (void)FetchNewMessages(toc, true, false, true, false);
          UpdateIMAPMailbox(toc);
        } else {
          // resync the mailbox when it's convenient
          FlagForResync(toc);
        }
      }
    }
    UserSelectWindow(tocWinWP);
  }
  return (0);
}

/**********************************************************************
 * OpenMailbox - open the named mailbox
 **********************************************************************/
int OpenMailbox(FSSpecPtr spec, bool showIt, TOCType * toc) {
  MyWindow *win;
  WindowPtr winWP;

  /*
   * create window
   */
  MyThreadBeginCritical(); // We may be in a thread. Don't yield until the
                           // window is all set up.
  if ((win = GetNewMyWindow(MAILBOX_WIND, nil, nil, showIt ? BehindModal : 0,
                            True, True, MBOX_WIN)) == nil) {
    WarnUser(COULDNT_WIN, MemError());
    MyThreadEndCritical();
    return (MemError());
  }

  winWP = GetMyWindowWindowPtr(win);

  // win->hPitch = FontWidth;
  // win->vPitch = FontLead+FontDescent;

  /*
   * read or build toc for window if we don't have it yet
   */
  if (!toc && !(toc = GetTOCFromSearchWin(spec))) {
    toc = CheckTOC(spec);
    if (toc == nil) {
      DisposeWindow_(winWP);
      MyThreadEndCritical();
      return (1);
    }
  }

  FillMesgErrors(toc);

  /*
   * set up window data
   */
  InitMailboxWin(win, toc, showIt);

  TOCDelEmpty(toc);
  if (showIt && PrefIsSet(PREF_DELDUP))
    TOCDelDup(toc); // don't bother deleting dups unless we're going to show the
                    // mailbox.

  // Show the window if the caller wants
  if (showIt) {
    ShowMyWindow(winWP);

    // Open mailbox drawer?
    if (toc->drawer && !toc->drawerWin) {
      // Don't open draw if there is one already open
      // Open drawer
      TOCType * tocTemp;

      for (tocTemp = TOCList; tocTemp; tocTemp = tocTemp->next) {
        if (tocTemp->drawerWin) {
          // Found another one. Don't open this one.
          toc->drawer = false;
          break;
        }
      }
      if (toc->drawer)
        // Open drawer
        MBDrawerOpen(win);
    }
  }

  /*
   * push it onto list of open toc's
   */
  toc->next = TOCList;
  TOCList = toc;

  MyThreadEndCritical();

  // if we're opening and showing an IMAP mailbox, fetch new messages
  if (showIt && toc->imapTOC && AutoCheckOK() && !StartingUp) {
    (void)FetchNewMessages(toc, true, false, true, false);
    UpdateIMAPMailbox(toc);
  } else {
    // resync the mailbox when it's convenient
    FlagForResync(toc);
  }
}

/**********************************************************************
 * InitMailboxWin - initialize mailbox window data
 **********************************************************************/
/* Stubs for Window Callbacks */
bool BoxClose(MyWindowPtr win) { return true; }
bool BoxButton(MyWindowPtr win, GtkWidget *widget, GdkEvent *event) {
  return false;
}
bool BoxMenu(MyWindowPtr win, int menu, int item, short modifiers) {
  return false;
}
int BoxGonnaShow(MyWindowPtr win) { return 0; }
int BoxPosition(MyWindowPtr win) { return 0; }
void BoxCursor(Point mouse) {}
bool BoxFind(MyWindowPtr win, unsigned char *text) { return false; }
// Missing: BoxActivate, BoxKey, BoxHelp, BoxDidResize, BoxGrowSize, BoxIdle

/* MBDrawerOpen Stub */
void MBDrawerOpen(MyWindowPtr win) {}

/**********************************************************************
 * InitMailboxWin - initialize mailbox window data
 **********************************************************************/
void InitMailboxWin(MyWindowPtr win, TOCType * toc, bool showIt) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  Str255 scratch;

  if (!winWP)
    return;

  // SetTopMargin(win,win->vPitch+2*FontDescent);

  // SetWindowKind (winWP,toc->which==OUT ? CBOX_WIN : MBOX_WIN);
  SetMyWindowPrivateData(win, (void *)toc);
  win->close = BoxClose;
  win->button = BoxButton; /* Was click/bgClick */
  // win->bgClick = BoxClick;
  // win->activate = BoxActivate;
  win->menu = BoxMenu;
  // win->key = BoxKey;
  // win->help = BoxHelp;
  // win->didResize = BoxDidResize;
  win->gonnaShow = BoxGonnaShow;
  win->position = BoxPosition;
  win->cursor = BoxCursor;
  // win->grow = BoxGrowSize;
  // win->minSize.h = 2*GetRLong(BOX_SIZE_SIZE);
  // win->idle = BoxIdle;
  win->find = BoxFind;

  toc->win = win;

  // Title the window.
  if (!IMAPMailboxTitle(toc, scratch))
    PCopy((unsigned char *)scratch,
          (unsigned char *)MailboxAlias(
              toc->which,
              (char *)PCopy((unsigned char *)scratch,
                            (unsigned char *)toc->mailbox.spec.name)));

  // Insert a NULL if the user is trying to avoid AMO
  if (GetPrefLong(PREF_AMO_AVOIDANCE) == kAMOAvoidAll ||
      (!showIt && *scratch > 1))
    PInsertC(scratch, sizeof(scratch), 0, scratch + 2);
  SetWTitle_(winWP, scratch);
}

/**********************************************************************
 * TOCDelDup - delete duplicate messages from a table of contents
 **********************************************************************/
long TOCDelDup(TOCType * tocH) {
  long i, j, nuke;
  long count = 0;
  short n, removed;
  long cycleCount = 50000;
  MSumPtr iSum, jSum;

  // this doesn't work on IMAP mailboxes
  if (tocH->imapTOC)
    return (0);

  n = tocH->count;

  for (i = 0, iSum = tocH->sums; i < n; i++, iSum++) {
    if (iSum->msgIdHash != kNeverHashed && iSum->msgIdHash != -2 &&
        iSum->msgIdHash != kNoMessageId)
      for (j = i + 1, jSum = iSum + 1; j < n; j++, jSum++) {
        if (!--cycleCount) {
          //	We don't need to call this very often compared to
          //	how quickly we go through this list
          CycleBalls();
          cycleCount = 50000;
        }
        if (iSum->msgIdHash == jSum->msgIdHash) {
          nuke = -1;
          if ((iSum->flags & FLAG_SKIPPED) && !(jSum->flags & FLAG_SKIPPED))
            nuke = i; // i is stub; delete
          else if (!(iSum->flags & FLAG_SKIPPED) &&
                   (jSum->flags & FLAG_SKIPPED))
            nuke = j; // j is stub; delete
          else if (iSum->state != UNREAD && jSum->state == UNREAD)
            nuke = j; // j is unread; delete
          else if (iSum->state == UNREAD && jSum->state != UNREAD)
            nuke = i; // i is unread; delete
          else
            nuke = j; // i and j identical; delete j
          if (nuke >= 0) {
            tocH->sums[nuke].msgIdHash = -2;
            count++;
          }
        }
      }
  }
  if (count) {
    removed = 0;
    for (n = tocH->count; n-- && removed < count;) {
      if (tocH->sums[n].msgIdHash == -2) {
        if (!DeleteSum(tocH, n)) // changed by Clarence, 4/28/97
          removed++;
      }
    }
  }
  return (count);
}

/**********************************************************************
 * TOCDelEmpty - delete empty messages from a table of contents
 **********************************************************************/
long TOCDelEmpty(TOCType * tocH) {
  long count = 0;
  short n;

  for (n = tocH->count; n--;) {
    if (tocH->sums[n].length == 0) {
      if (!DeleteSum(tocH, n)) // changed by Clarence, 4/28/97
        count++;
    }
  }
  return (count);
}

/**********************************************************************
 * OpenFilterMessages - open messages that filters say we should
 **********************************************************************/
int OpenFilterMessages(FSSpecPtr spec) {
  TOCType * tocH = TOCBySpec(spec);
  short i;
  bool openedOne;

  if (!tocH)
    return (1);

  ComposeLogS(LOG_FILT, nil, (unsigned char *)"\031Opening messages from %p",
              spec->name);

  do {
    openedOne = false;
    for (i = tocH->count - 1; i >= 0; i--)
      if (tocH->sums[i].opts & OPT_OPEN) {
        tocH->sums[i].opts &= ~OPT_OPEN;
        tocH->sums[i].opts |= OPT_AUTO_OPENED;
        TOCSetDirty(tocH, true);
        {
          Str63 name;
          ComposeLogS(LOG_FILT, nil, (unsigned char *)"\022Opening message %p",
                      (unsigned char *)name);
          PSCopy((unsigned char *)name, (unsigned char *)tocH->sums[i].subj);
        }
        GetAMessage(tocH, i, nil, nil, true);
        openedOne = true;
      }
  } while (openedOne);

  return (noErr);
}

/**********************************************************************
 * SaveMessageSum - save a message summary into a TOC
 **********************************************************************/
bool SaveMessageSum(void *vsum, TOCType **tocH) {
  MSumPtr sum = (MSumPtr)vsum;
  if (!tocH || !*tocH) return false;
  TOCType *toc = *tocH;

  RemoveUTF8FromSum(sum);
  sum->serialNum = toc->nextSerialNum++;

  /* Grow the TOC to hold one more summary */
  size_t newSize = sizeof(TOCType) + MAX(0, toc->count) * sizeof(MSumType);
  TOCType *grown = (TOCType *)g_realloc(toc, newSize);
  if (!grown) {
    WarnUser(SAVE_SUM_ERR, memFullErr);
    return false;
  }
  *tocH = grown;
  toc = grown;

  toc->needRedo = toc->count;
  toc->resort = kResortWhenever;
  memcpy(&toc->sums[toc->count], sum, sizeof(MSumType));
  toc->count++;
  InvalSum(toc, toc->count - 1);
  TOCSetDirty(toc, true);
  toc->reallyDirty = true;
  toc->analScanned = false;
  return true;
}

/************************************************************************
 * IsRoot - is a spec at the root of the Eudora Folder?
 ************************************************************************/
bool IsRoot(FSSpecPtr spec) {
  return (spec->parID == MailRoot.dirId &&
          SameVRef(spec->vRefNum, MailRoot.vRef));
}

/************************************************************************
 * IsSpool - is a spec in the Spool Folder?
 ************************************************************************/
bool IsSpool(FSSpecPtr spec) {
  FSSpec folderSpec;

  if (SubFolderSpec(SPOOL_FOLDER, &folderSpec))
    return (false);

  return (spec->parID == folderSpec.parID &&
          SameVRef(spec->vRefNum, folderSpec.vRefNum));
}

#ifdef BATCH_DELIVERY_ON
/************************************************************************
 * IsDelivery - is a spec in the Delivery Folder?
 ************************************************************************/
bool IsDelivery(FSSpecPtr spec) {
  FSSpec folderSpec;

  if (SubFolderSpec(DELIVERY_FOLDER, &folderSpec))
    return (false);

  return (spec->parID == folderSpec.parID &&
          SameVRef(spec->vRefNum, folderSpec.vRefNum));
}
#endif

/**********************************************************************
 * Spec2Menu - find the menu params for a given FSSpec
 **********************************************************************/
int Spec2Menu(FSSpecPtr spec, bool forXfer, short *menu, short *item) {
  Str63 name;
  long dirID = spec->parID;
  FSSpec parentSpec;

  if (IsIMAPMailboxFile(spec)) {
    ParentSpec(spec, &parentSpec);
    dirID = parentSpec.parID;
  }

  if (0 <= (*menu = FindDirLevel(spec->vRefNum, dirID))) {
    *menu = *menu ? *menu : MAILBOX_MENU;
    MailboxSpecAlias(spec, name);
    *item = FindItemByName(GetMHandle(*menu), (unsigned char *)name);
    if (forXfer)
      *menu = (*menu == MAILBOX_MENU) ? TRANSFER_MENU : *menu + MAX_BOX_LEVELS;
    if (*item > 0)
      return (noErr);
  }
  *item = 0;
  return (fnfErr);
}

/**********************************************************************
 * TOCH2Menu - find the menu item that corresponds to a toch
 **********************************************************************/
int TOCH2Menu(TOCType * tocH, bool forXfer, short *mnu, short *item) {
  FSSpec spec = GetMailboxSpec(tocH, -1);
  return (Spec2Menu(&spec, forXfer, mnu, item));
}

/**********************************************************************
 * FixSpecUnread - fix the unread-ness of a file
 **********************************************************************/
void FixSpecUnread(FSSpecPtr spec, bool unread) {
  FInfo info;

  FSpGetFInfo(spec, &info);
  if (((info.fdFlags & 0xe) != 0) != unread) {
    if (unread)
      info.fdFlags |= 0xe;
    else
      info.fdFlags &= ~0xe;
    FSpSetFInfo(spec, &info);
  }
}

/************************************************************************
 * FixMenuUnread - fix unread status in the menus
 ************************************************************************/
void MBFixUnread(MenuHandle mh, short item, bool unread) {}

void FixMenuUnread(MenuHandle mh, int item, bool unread) {
  Style oldStyle;
  Style newStyle;
  Boolean mailboxMenu, haveIMAP;

  newStyle = unread ? UnreadStyle : 0;
  oldStyle = GetItemStyle(mh, item);

  if (oldStyle == newStyle)
    return; /* done! */

  SetItemStyle(mh, item, newStyle);
  MBFixUnread(mh, item, unread); //	Update Mailboxes window

  mailboxMenu = mh == GetMHandle(MAILBOX_MENU);
  haveIMAP = IMAPExists();

  if (!newStyle)
    for (item = CountMenuItems(mh); item; item--) {
      if (mailboxMenu && haveIMAP) {
        //	Ignore IMAP mailfolder in main mailboxes menu
        short vRef;
        long dirID;
        short menuID;

        if ((menuID = SubmenuId(mh, item))) {
          MenuID2VD(menuID, &vRef, &dirID);
          if (IsIMAPVD(vRef, dirID))
            continue;
        }
        haveIMAP = false; //	No more IMAP
      }
      newStyle = GetItemStyle(mh, item);
      newStyle &= ~fontItalic;
      if (newStyle)
        break;
    }

  if (!mailboxMenu) {
    mh = ParentMailboxMenu(mh, &item);
    if (mh)
      FixMenuUnread(mh, item, newStyle != 0);
  } else
    MBFixUnread(mh, 0, newStyle != 0); //	Update Mailboxes window
}

/**********************************************************************
 * Box2Path - walk up our menus, looking for the one true way
 **********************************************************************/
int Box2Path(FSSpecPtr box, char *path) {
  short menu, item;
  int err = noErr;
  MenuHandle mh = NULL;
  Str63 name;

  PCopy((unsigned char *)path, box->name);
  err = Spec2Menu(box, False, &menu, &item);
  if (!err)
    mh = GetMHandle(menu);

  while (!err && mh && GetMenuID(mh) != MAILBOX_MENU) {
    if ((mh = ParentMailboxMenu(mh, &item))) {
      MyGetItem(mh, item, name);
      PCatC(name, ':');
      PInsert((unsigned char *)path, 255, name, (unsigned char *)path + 1);
    }
  }

  if (err)
    return (err);
  if (!mh)
    return (fnfErr);

  PInsert((unsigned char *)path, 255,
          (unsigned char *)"\001:", (unsigned char *)path + 1);
  return (noErr);
}

/**********************************************************************
 * Path2Box - walk back down our menut
 **********************************************************************/
int Path2Box(char *path, FSSpecPtr box) {
  int err = fnfErr;
  UPtr spot;
  Str31 name;

  box->vRefNum = MailRoot.vRef;
  box->parID = MailRoot.dirId;
  spot = (unsigned char *)path + 2;

  while (PToken((unsigned char *)path, (unsigned char *)name, &spot,
                (unsigned char *)":")) {
    // does the file exist?
    if ((err = FSMakeFSSpec(box->vRefNum, box->parID, (char *)name, box)))
      break;

    // if it's an alias, resolve
    IsAlias(box, box);

    // if it's not a folder, we're done
    if (!FSpIsItAFolder(box))
      break;

    // get the folder's spec
    box->parID = SpecDirId(box);
  }

  // look through the IMAP folder if we haven't found this box yet.
  if (err == fnfErr) {
    box->vRefNum = IMAPMailRoot.vRef;
    box->parID = IMAPMailRoot.dirId;
    spot = (unsigned char *)path + 2;

    while (PToken((unsigned char *)path, (unsigned char *)name, &spot,
                  (unsigned char *)":")) {
      // does the file exist?
      if ((err = FSMakeFSSpec(box->vRefNum, box->parID, (char *)name, box)))
        break;

      // if it's an alias, resolve
      IsAlias(box, box);

      // if it's not a folder, we're done
      if (!FSpIsItAFolder(box))
        break;

      // get the folder's spec
      box->parID = SpecDirId(box);
    }
  }

  if (err)
    return (err);
  if (PToken((unsigned char *)path, (unsigned char *)name, &spot,
             (unsigned char *)":"))
    return (fnfErr); // we didn't use up all the names in the string.  bad
  return (noErr);    // we made it!
}

#ifdef NEVER
/************************************************************************
 * CalcAllSumLengths - calculate all the lengths for all the sums in a toc
 ************************************************************************/
void CalcAllSumLengths(TOCType * toc) {
  int sumNum;

  for (sumNum = 0; sumNum < toc->count; sumNum++)
    CalcSumLengths(toc, sumNum);
}

/************************************************************************
 * CalcSumLengths - calculcate how long the strings in a sum can be
 ************************************************************************/
void CalcSumLengths(TOCType * tocH, int sumNum) {
  Str255 scratch;
  short trunc;
  short dWidth = (*BoxLines)[WID_DATE] - (*BoxLines)[WID_DATE - 1];
  short fWidth = (*BoxLines)[WID_FROM] - (*BoxLines)[WID_FROM - 1];

  if (FontIsFixed) {
    tocH->sums[sumNum].dateTrunc = dWidth / FontWidth - 1;
    tocH->sums[sumNum].fromTrunc = fWidth / FontWidth - 1;
  } else {
    PCopy(scratch, tocH->sums[sumNum].date);
    trunc = CalcTrunc(scratch, dWidth, InsurancePort);
    if (trunc && trunc < *scratch)
      trunc--;
    tocH->sums[sumNum].dateTrunc = trunc;

    PCopy(scratch, tocH->sums[sumNum].from);
    trunc = CalcTrunc(scratch, fWidth, InsurancePort);
    if (trunc && trunc < *scratch)
      trunc--;
    tocH->sums[sumNum].fromTrunc = trunc;
  }
}
#endif

/**********************************************************************
 * SetState - set a message's state in its summary,
 * 				handle virtual TOCs, too
 **********************************************************************/
void SetState(TOCType * tocH, int sumNum, int state) {
  TOCType * realTOC;
  short realSum;

  SetStateLo(tocH, sumNum, state);
  realTOC = GetRealTOC(tocH, sumNum, &realSum);
  if (realTOC && realTOC != tocH) {
    // do real mailbox also if working in virtual mailbox
    SetStateLo(realTOC, realSum, state);
    tocH = realTOC;
    sumNum = realSum;
  }
  SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum, false,
                  false); //	Notify search window
}

/**********************************************************************
 * SetStateLo - set a message's state in its summary.
 **********************************************************************/
void SetStateLo(TOCType * tocH, int sumNum, int state) {
  int oldState = tocH->sums[sumNum].state;

  if (oldState == state)
    return; /* nothing to do */

  InvalTocBox(tocH, sumNum, blStat);

  tocH->sums[sumNum].state = state;
  TOCSetDirty(tocH, true);
  if (tocH->sums[sumNum].selected)
    tocH->lastSameTicks = 1; // no unreading, pal

  if (oldState == UNREAD || state == UNREAD)
    tocH->unreadBase = -1; // force update

  if (IsQueuedState(oldState) || IsQueuedState(state))
    tocH->reallyDirty = True;
  if (IsQueuedState(state))
    DeleteMesgError(tocH, sumNum);

  if (tocH->which != OUT) {
    if (((state == SENT || state == UNSENT) &&
         (oldState == READ || oldState == UNREAD)) ||
        ((oldState == SENT || oldState == UNSENT) &&
         (state == READ || state == UNREAD))) {
      // Hoo, boy.  Ugly, ugly, ugly
      RedoWho(tocH, sumNum);
    }
  }

  QueueMessFlagChange(
      tocH, sumNum, state,
      false); // save the message state change, notify the server later.

  if (state == READ) {
    TOCType * realTOC;
    short realSum;

    realTOC = GetRealTOC(tocH, sumNum, &realSum);
    if (!tocH->imapTOC || PrefIsSet(PREF_COUNT_ALL_IMAP) ||
        // count only those IMAP messages that are in InBox
        TOCToMbox(realTOC) == LocateInboxForPers(TOCToPers(realTOC)))
      UpdateNumStatWithTime(kStatReadMsg, 1,
                            tocH->sums[sumNum].seconds + ZoneSecs());
  }
}

short FindSumByHash(TOCType * tocH, uint32_t hash) {
  short sumNum, myCount;

#ifdef THREADING_ON
  // check toc sanity-- InsaneTOC doesn't work unless we WriteTOC immediately
  // before
  myCount = tocH->count; /* Was GetHandleSize_-based, now direct */
  if (myCount > tocH->count)
#endif
    myCount = tocH->count;

  for (sumNum = myCount - 1; sumNum >= 0; sumNum--)
    if (tocH->sums[sumNum].uidHash == hash)
      break;
  return (sumNum);
}

/**********************************************************************
 * RedoWho - Redo the who field because of an in/out transition.
 **********************************************************************/
int RedoWho(TOCType * tocH, short sumNum) {
  Str255 who;
  short hState;
  int err = noErr;
  UPtr spot;
  UPtr text;
  long len, hLen;
  Str255 hName;
  short i;

  if (!(err = CacheMessage(tocH, sumNum)))
    if (tocH->sums[sumNum].cache) {
      hState = HGetState(tocH->sums[sumNum].cache);
      text = LDRef(tocH->sums[sumNum].cache);
      len = GetHandleSize(tocH->sums[sumNum].cache);
      *who = 0;
      if (tocH->sums[sumNum].state == SENT ||
          tocH->sums[sumNum].state == UNSENT) {
        hLen = len;
        spot = (unsigned char *)FindHeaderString(
            (char *)text, GetRString((char *)hName, HEADER_STRN + TO_HEAD),
            &hLen, False);
        if (!spot || !len) {
          hLen = len;
          spot = (unsigned char *)FindHeaderString(
              (char *)text, GetRString((char *)hName, HEADER_STRN + BCC_HEAD),
              &hLen, False);
        }
      } else {
        for (i = 1; *GetRString((char *)hName, SUM_SENDER_HEADS + i); i++) {
          hLen = len;
          spot = (unsigned char *)FindHeaderString((char *)text, (char *)hName,
                                                   &hLen, False);
          if (spot && hLen)
            break;
        }
      }
      if (spot && hLen) {
        MakePStr(who, (char *)spot, hLen);
        BeautifyFrom(who);
        PSCopy((unsigned char *)tocH->sums[sumNum].from, who);
        if (tocH->sums[sumNum].messH) {
          MakeMessTitle(hName, tocH, sumNum, True);
          SetWTitle_(GetMyWindowWindowPtr((*tocH->sums[sumNum].messH)->win),
                     hName);
        }
      }
      HSetState(tocH->sums[sumNum].cache, hState);
    }
  if (!err)
    InvalSum(tocH, sumNum);
  return (err);
}

#ifdef TWO

/**********************************************************************
 * GetSumColor - get a message's color from its summary.
 **********************************************************************/
short GetSumColor(TOCType * tocH, short sumNum) {
  return (SumColor(tocH->sums + sumNum));
}

/**********************************************************************
 * SetSumColor - set a message's color in its summary,
 * handle virtual TOCs, too
 **********************************************************************/
void SetSumColor(TOCType * tocH, short sumNum, short color) {
  TOCType * realTOC;
  short realSum;

  SetSumColorLo(tocH, sumNum, color);
  realTOC = GetRealTOC(tocH, sumNum, &realSum);
  if (realTOC && realTOC != tocH) {
    // do real mailbox also if working in virtual mailbox
    SetSumColorLo(realTOC, realSum, color);
    tocH = realTOC;
    sumNum = realSum;
  }
  SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum, false,
                  false); //	Notify search window
}

/**********************************************************************
 * SetSumColorLo - set a message's color in its summary.
 **********************************************************************/
void SetSumColorLo(TOCType * tocH, short sumNum, short color) {
  int oldColor = GetSumColor(tocH, sumNum);
  MessHandle messH;

  if (oldColor == color)
    return; /* nothing to do */

  InvalSum(tocH, sumNum);

  tocH->sums[sumNum].flags &=
      ~(FLAG_HUE1 | FLAG_HUE2 | FLAG_HUE3 | FLAG_HUE4);
  tocH->sums[sumNum].flags |= (color << 14);
  TOCSetDirty(tocH, true);

  /* set the priority display in the message */
  if (messH = tocH->sums[sumNum].messH) {
    WindowPtr messWinWP = GetMyWindowWindowPtr((*messH)->win);
    (*messH)->win->label = color;
    if (IsColorWin(messWinWP)) {
      if (IsWindowVisible(messWinWP))
        InvalTopMargin((*messH)->win);
      AppCdefBGChange((*messH)->win);
    }
  }

  /* If this is an IMAP message, tell the server about the label change */
  if (tocH->imapTOC)
    QueueMessFlagChange(tocH, sumNum, tocH->sums[sumNum].state, false);
}
#endif

/**********************************************************************
 * BoxFOpen - open the mailbox file represented by a toc
 * may be called on open mailbox, and reports error to user
 **********************************************************************/
int BoxFOpenLo(TOCType * tocH, short sumNum) {
  short refN;
  int err = 0;
  FSSpec newSpec, spec;

  if (tocH->refN == 0) {
    tocH;
    spec = GetMailboxSpec(tocH, sumNum);
    err = AFSpOpenDF(&spec, &newSpec, fsRdWrPerm, &refN);
    if (err)
      FileSystemError(OPEN_MBOX, spec.name, err);
    else
      tocH->refN = refN;
    ComposeLogS(LOG_FLOW, nil, (unsigned char *)"\014BoxFOpen %p\r", spec.name);
    (void)0;
  }

  return (err);
}

/**********************************************************************
 * BoxFOpen - open the mailbox file represented by a toc
 * may be called on open mailbox, and reports error to user
 **********************************************************************/
/* Local Decls for usage */
void NoteFreeSpace(TOCType * tocH);
int MyFSClose(short refN);
// short FSClose(short refNum); removed - defined in mailbox.h as void*

int BoxFOpen(TOCType * tocH) { return BoxFOpenLo(tocH, -1); }
// #pragma segment Main
/**********************************************************************
 * BoxFClose - close a mailbox file represented by a toc.  May be
 * called on open mailbox, reports any errors to user.
 **********************************************************************/
void BoxFClose(TOCType * tocH, bool flush) {
  int err;
  FSSpec spec;

  if (tocH->refN > 0) {
    NoteFreeSpace(tocH);
    err = flush ? MyFSClose(tocH->refN) : (int)(long)FSClose(tocH->refN);
    tocH->refN = 0;
    spec = GetMailboxSpec(tocH, -1);
    if (err)
      FileSystemError(CLOSE_MBOX, spec.name, err);
    (void)0;
  }
}

/************************************************************************
 * NoteFreeSpace - note the free space on a volume
 ************************************************************************/
void NoteFreeSpace(TOCType * tocH) {
  FSSpec newSpec;

  newSpec = GetMailboxSpec(tocH, -1);
  IsAlias(&newSpec, &newSpec);
  tocH->volumeFree = VolumeFree(newSpec.vRefNum);
}

void Preview(TOCType * tocH, short sumNum);

#pragma segment Mailbox

/**********************************************************************
 * DeleteSum - remove a sum from a toc
 **********************************************************************/
bool DeleteSum(TOCType * tocH, int sumNum) {
  MSumPtr sum;
  int mNum;
  Str31 name;

  ASSERT(tocH);
  ASSERT(sumNum < tocH->count);
  ASSERT(sumNum >= 0);
  ASSERT(tocH->sums[sumNum].state != BUSY_SENDING);
  if (!tocH || !(sumNum < tocH->count) || !(sumNum >= 0) ||
      (tocH->sums[sumNum].state == BUSY_SENDING))
    return -1;

  if (LogLevel & LOG_MOVE)
    ComposeLogS(LOG_MOVE, nil, "\pDelete %p,%p from %p\015",
                tocH->sums[sumNum].from, tocH->sums[sumNum].subj,
                GetMailboxName(tocH, sumNum, name));

  tocH->analScanned = false;

  if (tocH->previewID == tocH->sums[sumNum].uidHash &&
      tocH->previewPTE)
    Preview(tocH, -1);
  // tocH->maxValid = MIN(tocH->maxValid, sumNum - 1);
  if (IsQueued(tocH, sumNum))
    ForceSend = 0;
  if (tocH->sums[sumNum].cache) {
    DisposeHandle(tocH->sums[sumNum].cache);
    tocH->sums[sumNum].cache = NULL;
  }
  if (!tocH->virtualTOC)
    DeleteMesgError(tocH, sumNum);
  if (sumNum < tocH->count - 1) /* is this not the last sum? */
  {
    sum = tocH->sums + sumNum;
    BMD(sum + 1, sum, (tocH->count - 1 - sumNum) * sizeof(MSumType));
    for (mNum = sumNum; mNum < tocH->count - 1; mNum++)
      if ((MessHandle)tocH->sums[mNum].messH)
        (*(MessHandle)tocH->sums[mNum].messH)->sumNum--;
  }
  /* Shrink TOC by one summary */
  {
    size_t newSize = sizeof(TOCType) + MAX(0, tocH->count - 2) * sizeof(MSumType);
    if (newSize < sizeof(TOCType)) newSize = sizeof(TOCType);
    /* Note: caller must update their pointer if realloc moves memory.
       For now, shrink in-place (realloc shrink usually doesn't move). */
    TOCType *shrunk = (TOCType *)g_realloc(tocH, newSize);
    if (shrunk) tocH = shrunk;
  }
  if (--tocH->count == 0 && !tocH->virtualTOC)
    ZeroMailbox(tocH);

  TOCSetDirty(tocH, true);
  return noErr;
}

// bool IsQueued(TOCType * tocH, int sumNum); MOVED TO TOP

/**********************************************************************
 * InvalSum - invalidate an entire message summary line
 **********************************************************************/
void InvalSum(TOCType * tocH, short sum) {
  /* GTK Port: Invalidation handled by widgets
  Rect r;
  MyWindowPtr win = tocH->win;
  // GrafPtr oldPort;
  long top, bottom;

  if (!win)
    return;
  if (!IsWindowVisible(GetMyWindowWindowPtr(win)))
    return;

  top = win->topMargin + win->vPitch * (sum - GetControlValue(win->vBar));
  bottom = top + win->vPitch + 1;
  if (bottom < win->contR.top || top > win->contR.bottom)
    //	This summary is not visible
    return;


  r.top = top;
  r.bottom = bottom;
  r.left = win->contR.left;
  r.right = win->contR.right;
  InvalWindowRect(GetMyWindowWindowPtr(win), &r);
  tocH->resort = MAX(tocH->resort, kNoSlowResort);
  */
}

/************************************************************************
 * AddBox - add a mailbox to the menus
 ************************************************************************/
void AddBox(short function, UPtr name, short level, bool unread) {
  short base = function * MAX_BOX_LEVELS;
  short menuId = level ? base + level
                       : (function == MAILBOX ? MAILBOX_MENU : TRANSFER_MENU);
  MenuHandle mh = GetMHandle(menuId);
  short item, lastItem;
  Style theStyle;
  Str63 scratch;
  Boolean skipIMAP =
      (menuId == MAILBOX_MENU || menuId == TRANSFER_MENU) && IMAPExists();

  lastItem = CountMenuItems(mh);
  for (item = lastItem; item > 0; item--) {
    if (HasSubmenu(mh, item))
      continue;
    theStyle = GetItemStyle(mh, item);
    if (theStyle & fontItalic)
      break; /* "new" is italicized */
    MyGetItem(mh, item, scratch);
    if (skipIMAP) {
      if (scratch[1] == '-')
        skipIMAP = false;
      continue;
    } else if (scratch[1] == '-')
      break; /* menu separator (transfer) */

    if (StringComp(scratch, name) < 0)
      break;
  }
  MyInsMenuItem(mh, name, item);
  if (function == MAILBOX) {
    //	Set unread status
    FixMenuUnread(mh, item + 1, unread);
  }
}

/************************************************************************
 * RemoveBox - remove a mailbox from the mailbox menus
 ************************************************************************/
void RemoveBox(short function, UPtr name, short level) {
  short base = function * MAX_BOX_LEVELS;
  short menuId = level ? base + level
                       : (function == MAILBOX ? MAILBOX_MENU : TRANSFER_MENU);
  MenuHandle mh = GetMHandle(menuId);
  short item;

  if (item = FindItemByName(mh, name)) {
    if (function == MAILBOX) {
      //	Make sure unread status of parent menu is correct
      FixMenuUnread(mh, item,
                    false); //	Make this one unread before removing
    }
    DeleteMenuItem(mh, item);
    if (CountMenuItems(mh) >= 31)
      //	Under MacOS 8.5 item 32 going to item 31 may become disabled
      EnableItem(mh, 31);
  }
}

/**********************************************************************
 * GetMBDirName - get a the name of a mail folder, with shenanigans for
 *top-level
 **********************************************************************/
/* void HideDialogItem(DialogPtr dp, short item); Added below */

// ...
short GetMBDirName(short vRef, long dirId, UPtr name) {
  /* CInfoPBRec hfi; REMOVED */
  int sysVRef;
  long sysDirId;
  Str255 sTemp;
  int err;
  FSSpec spec; /* Restored */
  // FSSpec spec; // This was removed as part of the change, as it's not in the
  // new decls Str63 sTemp; // This was changed to Str255 sTemp and moved up

  // If we're at the mail root, pretend we're one up
  if (vRef == MailRoot.vRef && dirId == MailRoot.dirId) {
    vRef = Root.vRef;
    dirId = Root.dirId;
  }

  if (err = GetDirName(nil, vRef, dirId, name)) //	Name of Mail Folder
    return err;

  //	If the standard Eudora Folder is in use (ie, named FOLDER_NAME and in
  //	the system folder) show FILE_ALIAS_EUDORA_FOLDER at the top
  //	level of the mailboxes window.
  FindFolder(kOnSystemDisk, kSystemFolderType, False, &sysVRef, &sysDirId);
  if (vRef == sysVRef &&
      !FSMakeFSSpec(sysVRef, sysDirId, GetRString(sTemp, FOLDER_NAME),
                    &spec) && //	Spec for standard Eudora folder
      dirId == SpecDirId(&spec))
    GetRString(name, FILE_ALIAS_EUDORA_FOLDER);

  return noErr;
}

/**********************************************************************
 * GetNewMailbox - get the name of and create a new mailbox
 * returns 1 for normal mb's, or else dirId
 **********************************************************************/
bool GetNewMailbox(short vRef, long inDirId, FSSpecPtr spec, bool *folder,
                   bool *xfer) {
  MyWindowPtr dgPtrWin;
  DialogPtr dgPtr;
  short item;
  Str255 name;
  Str63 folderName;
  extern ModalFilterYDUPP DlgFilterUPP;

  if (GetMBDirName(vRef, inDirId, folderName))
    *folderName = 0;

  if ((dgPtrWin = GetNewMyDialog(NEW_MAILBOX_DLOG, nil, nil, InFront)) == nil) {
    WarnUser(GENERAL, MemError());
    return (False);
  }

  dgPtr = GetMyWindowDialogPtr(dgPtrWin);

  if (!xfer)
    HideDialogItem(dgPtr, NEW_MAILBOX_NOXF);
  MyParamText(folderName, "", "", "");
  StartMovableModal(dgPtr);
  ShowWindow(GetDialogWindow(dgPtr));
  HiliteButtonOne(dgPtr);
  do {
    SelectDialogItemText(dgPtr, NEW_MAILBOX_NAME, 0, REAL_BIG);
    PushCursor(iBeamCursor);
    do {
      MyParamText(folderName, "", "", "");
      MovableModalDialog(dgPtr, DlgFilterUPP, &item);
      if (item == NEW_MAILBOX_FOLDER)
        SetDItemState(dgPtr, item, !GetDItemState(dgPtr, item));
      else if (item == NEW_MAILBOX_NOXF)
        SetDItemState(dgPtr, item, !GetDItemState(dgPtr, item));
    } while (item == NEW_MAILBOX_FOLDER || item == NEW_MAILBOX_NOXF);
    PopCursor();
    GetDIText(dgPtr, NEW_MAILBOX_NAME, name);
    *folder = GetDItemState(dgPtr, NEW_MAILBOX_FOLDER);
    if (xfer)
      *xfer = GetDItemState(dgPtr, NEW_MAILBOX_NOXF);
    spec->vRefNum = vRef;
    spec->parID = inDirId;
    PSCopy(spec->name,
           name); /* FSMakeFSSpec screws up this step if the name is
                     too long. We want to catch that in BadMailboxName,
                     not here, so don't use FSMakeFSSpec. */
  } while (item == NEW_MAILBOX_OK && BadMailboxName(spec, *folder));

  EndMovableModal(dgPtr);
  DisposDialog_(dgPtr);

  return (item == NEW_MAILBOX_OK);
}

/**********************************************************************
 * RenameMailbox - rename a mailbox
 **********************************************************************/
int RenameMailbox(FSSpecPtr spec, UPtr newName, bool folder) {
  int err;
  Str63 oldTOCName, suffix;
  Str63 newTOCName;
  FSSpec tocSpec;

  err = HRename(spec->vRefNum, spec->parID, spec->name, newName);
  if (err)
    return (FileSystemError(RENAMING_BOX, spec->name, err));

  if (!folder) {
    //	Rename TOC file also if it exists
    GetRString(suffix, TOC_SUFFIX);
    PCopy(oldTOCName, spec->name);
    PCopy(newTOCName, newName);
    PCat(oldTOCName, suffix);
    PCat(newTOCName, suffix);

    //	Check for existence of old .TOC file
    if (!FSMakeFSSpec(spec->vRefNum, spec->parID, oldTOCName, &tocSpec)) {
      if (*newTOCName > 31) {
        //	TOC file name too long
        TooLong(newName);
        err = bdNamErr;
      } else {
        err = HRename(spec->vRefNum, spec->parID, oldTOCName, newTOCName);
        if (err == fnfErr)
          err = 0;
        if (err) {
          FileSystemError(RENAMING_BOX, oldTOCName, err);
        }
      }
      if (err)
        //	Restore mailbox name since we couldn't rename TOC file
        (void)HRename(spec->vRefNum, spec->parID, newName, spec->name);
    }
  }

  return (err);
}

/**********************************************************************
 * BadMailboxName - figure out if a mailbox name is ok by trying to
 * create the mailbox.
 **********************************************************************/
bool BadMailboxName(FSSpecPtr spec, bool folder) {
  int err;
  Str15 suffix;
  long newDirId;
  bool success;

  // Is this box being created inside an IMAP cache folder?  Then it's an IMAP
  // mailbox.
  if (IMAPAddMailbox(spec, folder, &success, false)) {
    if (!success)
      Zero(*spec); // zero out the spec if we fail so we don't do anthing too
                   // stupid
    return (false);
  }

  if (*spec->name > 31 - *GetRString(suffix, TOC_SUFFIX)) {
    TooLong(spec->name);
    return (True);
  }

  if (BadMailboxNameChars(spec))
    return (True);

  if (folder) {
    if (BoxMapCount > MAX_BOX_LEVELS) {
      WarnUser(TOO_MANY_LEVELS, MAX_BOX_LEVELS);
      return (True);
    }
    if (err = FSpDirCreate(spec, smSystemScript, &newDirId)) {
      FileSystemError(CREATING_MAILBOX, spec->name, err);
      return (True);
    }
    AddBoxMap(spec->vRefNum, newDirId);
    spec->parID = newDirId;
    *spec->name = 0;
  } else {
    err = FSpCreate(spec, CREATOR, MAILBOX_TYPE, smSystemScript);
    if (err) {
      FileSystemError(CREATING_MAILBOX, spec->name, err);
      return (True);
    }
  }
  return (False);
}

/**********************************************************************
 * BadMailboxNameChars - return TRUE if this mailbox name has some
 *	inappropriate characters.
 **********************************************************************/
bool BadMailboxNameChars(FSSpecPtr spec) {
  char *cp;

  if (spec->name[1] == '.') {
    WarnUser(LEADING_PERIOD, 0);
    return (True);
  }

  for (cp = spec->name + *spec->name; cp > spec->name; cp--) {
    if (*cp == ':') {
      WarnUser(NO_COLONS_HERE, 0);
      return (True);
    }
  }

  return (False);
}

/************************************************************************
 * ZeroMailbox - set a mailbox's size to zero.  Assumes box is empty
 ************************************************************************/
void ZeroMailbox(TOCType * tocH) {
  if (!BoxFOpen(tocH)) {
    SetEOF(tocH->refN, 0L);
    BoxFClose(tocH, false);
  }
}

/************************************************************************
 * ChainTrash - move an entire alias chain to the trash
 ************************************************************************/
int ChainTrash(FSSpecPtr spec) {
  FSSpec chain;
  bool wasAlias, isFolder;

  chain = *spec;
  if (!ResolveAliasFile(&chain, False, &isFolder, &wasAlias) && wasAlias)
    ChainTrash(&chain);
  return (FSpTrash(spec));
}

/************************************************************************
 * RemoveMailbox - move a mailbox to the trash
 ************************************************************************/
int RemoveMailbox(FSSpecPtr spec, bool trashChain) {
  TOCType * tocH;
  int err;
  FSSpec tocSpec;
  short sumNum;

  /*
   * open windows
   */
  if (tocH = FindTOC(spec)) {
    TOCSetDirty(tocH, false);
    for (sumNum = 0; sumNum < tocH->count; sumNum++)
      if (tocH->sums[sumNum].messH)
        CloseMyWindow(
            GetMyWindowWindowPtr((*tocH->sums[sumNum].messH)->win));
    if (tocH->win)
      CloseMyWindow(GetMyWindowWindowPtr(tocH->win));
  }

  /*
   * files
   */
  if (err = trashChain ? ChainTrash(spec) : FSpTrash(spec))
    return (FileSystemError(DELETING_BOX, spec->name, err));
  Box2TOCSpec(spec, &tocSpec);
  err = trashChain ? ChainTrash(&tocSpec) : FSpTrash(&tocSpec);
  if (err == fnfErr || err == bdNamErr || err == paramErr)
    err = 0;
  if (err)
    return (FileSystemError(DELETING_BOX, tocSpec.name, err));

  return (noErr);
}

/************************************************************************
 * MessagePosition - save/restore position for a new message window
 ************************************************************************/
bool MessagePosition(bool save, MyWindowPtr win) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  TOCType * tocH = (*(MessHandle)GetMyWindowPrivateData(win))->tocH;
  short sumNum = (*(MessHandle)GetMyWindowPrivateData(win))->sumNum;
  Rect r;
  bool zoomed;
  bool res = True;

  if (save) {
    if (!tocH->virtualTOC) {
      /*      utl_SaveWindowPos(winWP, &r, &zoomed);
            tocH->sums[sumNum].u.savedPos = r;
            if (zoomed)
              tocH->sums[sumNum].flags |= FLAG_ZOOMED;
            else
              tocH->sums[sumNum].flags &= ~FLAG_ZOOMED; */
      TOCSetDirty(tocH, true);
    }
  } else {
    /*    r = tocH->sums[sumNum].u.savedPos;
        if (r.right - r.left > 100 && r.bottom - r.top > 40) {
          zoomed = (tocH->sums[sumNum].flags & FLAG_ZOOMED) != 0;
          utl_RestoreWindowPos(winWP, &r, zoomed, 1, 0, 0, 0);
        } else */
    {
      Point corner;
      short val;

      GetPortBounds(GetWindowPort(winWP), &r);
      if (!(val = GetPrefLong(PREF_MWIDTH)))
        val = GetRLong(DEF_MWIDTH);
      r.right = r.left + MessWi(win);
      /*      if (val = GetPrefLong(PREF_MHEIGHT))
              r.bottom = r.top + win->vPitch * val;
            MyStaggerWindow(winWP, &r, &corner);
            OffsetRect(&r, corner.h - r.left, corner.v - r.top); */
    }
    // SanitizeSize(win, &r);
    // MyMoveWindow(winWP, r.left, r.top, false);
    // MySizeWindow(winWP, r.right - r.left, r.bottom - r.top, true);
    utl_RestoreWindowPos(winWP, &r, zoomed, 1, TitleBarHeight(winWP),
                         LeftRimWidth(winWP), (void *)FigureZoom,
                         (void *)DefPosition);
  }
  return (res);
}

/************************************************************************
 * TooLong - complain about a name that's too long
 ************************************************************************/
void TooLong(UPtr name) {
  Str63 toolong1, toolong2;
  MyParamText(GetRString(toolong1, BOX_TOO_LONG1), name,
              GetRString(toolong2, BOX_TOO_LONG2), "");
  ReallyDoAnAlert(OK_ALRT, Note);
}

/************************************************************************
 * FindDirLevel - find the level of a particular subfolder
 ************************************************************************/
short FindDirLevel(short vRef, long dirId) {
  short level;
  short n = BoxMapCount;

  for (level = 0; level < n; level++)
    if ((*BoxMap)[level].dirId == dirId &&
        SameVRef((*BoxMap)[level].vRef, vRef))
      return (level && g16bitSubMenuIDs ? level + BOX_MENU_START - 1 : level);

  return (-1);
}

/************************************************************************
 * BuildBoxCount - build the numbered list of mailboxes (for Find)
 ************************************************************************/
void BuildBoxCount(void) {
  if (BoxCount) {
    DisposeHandle(BoxCount);
    BoxCount = NULL;
  }
  BoxCount = NuHandle(0);
  if (!BoxCount) {
    WarnUser(MEM_ERR, MemError());
    return;
  }

  AddBoxCountItem(MAILBOX_IN_ITEM, MailRoot.vRef, MailRoot.dirId);
  AddBoxCountItem(MAILBOX_OUT_ITEM, MailRoot.vRef, MailRoot.dirId);
  AddBoxCountItem(MAILBOX_JUNK_ITEM, MailRoot.vRef, MailRoot.dirId);
  AddBoxCountItem(MAILBOX_TRASH_ITEM, MailRoot.vRef, MailRoot.dirId);
  AddBoxCountMenu(MAILBOX_MENU, MAILBOX_FIRST_USER_ITEM, MailRoot.vRef,
                  MailRoot.dirId, true);
}

/************************************************************************
 * AddBoxMap - add an entry to the boxmap handle
 ************************************************************************/
int AddBoxMap(short vRef, long dirId) {
  BoxMapType bmt;
  short err;

  bmt.vRef = vRef;
  bmt.dirId = dirId;
  if (!PtrPlusHand_(&bmt, BoxMap, sizeof(bmt))) {
    err = MemError();
    if (!err)
      err = memFullErr;
    WarnUser(MEM_ERR, err);
    return err;
  }
  return noErr;
}

/************************************************************************
 * AddBoxCountMenu - add the contents of a menu to the BoxCount thingy
 ************************************************************************/
void AddBoxCountMenu(short menuID, short item, short vRef, long dirId,
                     bool includeIMAP) {
  MenuHandle mh = GetMHandle(menuID);
  short n = CountMenuItems(mh);
  short it;
  Str255 s;

  for (; item <= n; item++) {
    if (HasSubmenu(mh, item)) {
      short thisVRef;
      long thisDirId;

      it = SubmenuId(mh, item);
      MenuID2VD(it, &thisVRef, &thisDirId);
      AddBoxCountMenu(it, MAILBOX_FIRST_USER_ITEM - MAILBOX_BAR1_ITEM, thisVRef,
                      thisDirId, true);
    } else {
      GetMenuItemText(mh, item, s);
      if (StringSame(s, "\p-")) {
        if (menuID == MAILBOX_MENU && !includeIMAP)
          return; // don't add IMAP folders
      } else
        AddBoxCountItem(item, vRef, dirId);
    }
  }
}

/************************************************************************
 * AddBoxCountItem - add a single item to the BoxCount list
 ************************************************************************/
void AddBoxCountItem(short item, short vRef, long dirId) {
  BoxCountElem bce;

  bce.item = item;
  bce.dirId = dirId;
  bce.vRef = vRef;
  if (PtrPlusHand_(&bce, BoxCount, sizeof(bce)))
    WarnUser(MEM_ERR, MemError());
}

/**********************************************************************
 * IsMailboxChoice - is this menu choice for a mailbox?
 **********************************************************************/
bool IsMailboxChoice(short menu, short item) {
  MenuHandle mh;
  short levels = HandleCount(BoxMap);

  if (menu == TRANSFER_MENU || menu == MAILBOX_MENU ||
      (g16bitSubMenuIDs
           ? (menu >= BOX_MENU_START && menu < BOX_MENU_START + levels ||
              menu >= BOX_MENU_START + gMaxBoxLevels &&
                  menu < BOX_MENU_START + gMaxBoxLevels + levels)
           : (menu >= 1 && menu < 1 + levels ||
              menu >= MAX_BOX_LEVELS && menu < MAX_BOX_LEVELS + levels)))
    return (item >= 1 && (mh = GetMHandle(menu)) && item <= CountMenuItems(mh));
  else
    return (False);
}

/**********************************************************************
 * MailboxAlias - get the aliased name of a mailbox
 **********************************************************************/
char *MailboxAlias(short which, char *name) {
  switch (which) {
  case IN:
    GetRString(name, FILE_ALIAS_IN);
    break;
  case OUT:
    GetRString(name, FILE_ALIAS_OUT);
    break;
  case JUNK:
    GetRString(name, FILE_ALIAS_JUNK);
    break;
  case TRASH:
    GetRString(name, FILE_ALIAS_TRASH);
    break;
  }
  return (name);
}

/**********************************************************************
 * MailboxSpecAlias - get the aliased name of a mailbox from a filespec
 **********************************************************************/
char *MailboxSpecAlias(FSSpecPtr spec, char *name) {
  if (spec->vRefNum == MailRoot.vRef && spec->parID == MailRoot.dirId) {
    if (EqualStrRes(spec->name, IN))
      GetRString(name, FILE_ALIAS_IN);
    else if (EqualStrRes(spec->name, OUT))
      GetRString(name, FILE_ALIAS_OUT);
    else if (EqualStrRes(spec->name, JUNK))
      GetRString(name, FILE_ALIAS_JUNK);
    else if (EqualStrRes(spec->name, TRASH))
      GetRString(name, FILE_ALIAS_TRASH);
    else
      PCopy(name, spec->name);
  } else
    PCopy(name, spec->name);
  return (name);
}

/**********************************************************************
 * MailboxFile - get the filename of a mailbox
 **********************************************************************/
char *MailboxFile(short which, char *name) {
  if (which)
    GetRString(name, which);
  return (name);
}

/**********************************************************************
 * MailboxMenuFile - get the filename from a menu
 **********************************************************************/
char *MailboxMenuFile(short mid, short item, char *name) {
  Str31 prefix;

  if ((mid == MAILBOX_MENU || mid == TRANSFER_MENU) &&
      item <= MAILBOX_BAR1_ITEM) {
    switch (item) {
    case MAILBOX_IN_ITEM:
      GetRString(name, IN);
      break;
    case MAILBOX_OUT_ITEM:
      GetRString(name, OUT);
      break;
    case MAILBOX_JUNK_ITEM:
      GetRString(name, JUNK);
      break;
    case MAILBOX_TRASH_ITEM:
      GetRString(name, TRASH);
      break;
    }
    if (mid == TRANSFER_MENU)
      PInsert(name, 31, GetRString(prefix, TRANSFER_PREFIX), name + 1);
  } else
    MyGetItem(GetMHandle(mid), item, name);
  return (name);
}

/************************************************************************
 * GetTransferParams - Turn menu choice into mailbox items.
 *
 * xfer - set to true if the choice was a transfer
 * spec - the FSSpec of the mailbox in question
 * function result - true if the current message(s) should be transferred to
 *   the chosen mailbox
 ************************************************************************/
bool GetTransferParams(short menu, short item, FSSpecPtr spec, bool *xfer) {
  bool folder = False, noxfer = False;
  Str31 fix;
  FSSpec newSpec;
  bool root;
  MenuHandle mh = GetMHandle(menu);

  /*
   * find owning directory
   */
  if (xfer)
    *xfer = menu == TRANSFER_MENU || menu == MAILBOX_MENU
                ? menu == TRANSFER_MENU
                : menu > MAX_BOX_LEVELS;

  if (spec) {
    short vRef;
    long parID;
    MenuID2VD(menu, &vRef, &parID);
    spec->vRefNum = vRef;
    spec->parID = parID;
    root = IsRoot(spec);
    if (root ? item == TRANSFER_NEW_ITEM
             : item == TRANSFER_NEW_ITEM - TRANSFER_BAR1_ITEM) {
      do {
        if (GetNewMailbox(spec->vRefNum, spec->parID, &newSpec, &folder,
                          xfer ? &noxfer : nil)) {
          bool wasIMAP =
              IsIMAPMailboxFile(&newSpec) || IsIMAPCacheFolder(&newSpec);

          // if we just added a folder or an IMAP mailbox (which is a folder),
          // rebuild the whole mailbox tree
          if (folder || wasIMAP) {
            BuildBoxMenus();
            MBTickle(nil, nil);
          }
          // otherwise, make sure the mailbox got created and add it.  It might
          // not have gotten created if we failed to add an IMAP box.
          else if (FSpExists(&newSpec) == noErr)
            AddBoxHigh(&newSpec);

          *spec = newSpec;
        } else
          return (False);
      } while (folder);
    }
#ifdef TWO
    else if (root ? item == TRANSFER_OTHER_ITEM
                  : item == TRANSFER_OTHER_ITEM - TRANSFER_BAR1_ITEM) {
      // if this is the "This Mailbox" item, return a spec pointing to the
      // parent folder.
      GetMenuTitle(mh, spec->name);
      if (IsIMAPVD(spec->vRefNum, spec->parID))
        return (true);
      else
        return (!AskGraft(spec->vRefNum, spec->parID, spec));
    }
#endif
    else {
      MailboxMenuFile(menu, item, spec->name);
      TrimPrefix(spec->name, GetRString(fix, TRANSFER_PREFIX));
    }

    // if this was an IMAP mailbox, then the spec is pointing to the folder.
    if (IsIMAPCacheFolder(spec))
      spec->parID = SpecDirId(spec);
  }
  return (!noxfer);
}

/************************************************************************
 * AppendXferSelection - append the menu item for transfer to selection, if
 *appropriate
 ************************************************************************/
int AppendXferSelection(PETEHandle pte, MenuHandle contextMenu) {
  Str255 s;
  MenuAndScoreHandle mash;
  bool divided = false;
  Str31 name;
  int err = fnfErr;
  short smid;

  if (!AttIsSelected(nil, pte, -1, -1, 0, nil, nil))
    if (*CollapseLWSP(PeteSelectedString(s, pte)))
      if (*s < 31)
        if (IsEnabled(TRANSFER_MENU, 0))
          if (!BoxMatchMenuItems(s, &mash, BoxMatchScore)) {
            short n = HandleCount(mash);
            short i = GetRLong(MAX_CONTEXT_FILE_CHOICES);

            n = MIN(n, i);

            for (i = 0; i < n; i++) {
              short menu = (*mash)[i].menu;
              short item = (*mash)[i].item;
              short newItem;
              short realMenu =
                  menu == MAILBOX_MENU ? TRANSFER_MENU : menu + MAX_BOX_LEVELS;

              if (IsEnabled(realMenu, item)) {
                if (!divided) {
                  divided = true;
                  if (CountMenuItems(contextMenu) &&
                      !MenuItemIsSeparator(contextMenu,
                                           CountMenuItems(contextMenu)))
                    AppendMenu(contextMenu, "\p-"); // add a divider
                }
                CopyMenuItem(GetMHandle(realMenu), item, contextMenu, REAL_BIG);
                MyGetItem(GetMHandle(menu), item, name);
                newItem = CountMenuItems(contextMenu);
                SetMenuItemCommandID(contextMenu, newItem,
                                     (realMenu << 16) | item);
                // rename for clarity
                ComposeRString(s, TRANSFER_CONTEXT_FMT, name);
                MySetItem(contextMenu, newItem, s);

                // special case for IMAP mailboxes with children
                smid = SubmenuId(contextMenu, newItem);
                if (smid != 0) {
                  // remove the submenu
                  SetMenuItemHierarchicalMenu(contextMenu, newItem, NULL);

                  // set the command id to "This Mailbox..." in the submeny
                  SetMenuItemCommandID(contextMenu, newItem, (smid << 16) | 2);
                }
                err = noErr;
              }
            }
            ZapHandle(mash);
          }

  return err;
}

/************************************************************************
 * Menu2VD - menu into vref & dirID
 ************************************************************************/
void Menu2VD(MenuHandle mh, short *vRef, long *dirId) {
  MenuID2VD(GetMenuID(mh), vRef, dirId);
}

/************************************************************************
 * MenuID2VD - menu into vref & dirID
 ************************************************************************/
void MenuID2VD(short menuID, short *vRef, long *dirId) {
  if (menuID == MAILBOX_MENU || menuID == TRANSFER_MENU) {
    *vRef = MailRoot.vRef;
    *dirId = MailRoot.dirId;
  } else {
    if (g16bitSubMenuIDs)
      menuID -= BOX_MENU_START - 1;
    if (menuID > gMaxBoxLevels)
      menuID -= gMaxBoxLevels; //	was from Transfer menu
    *vRef = (*BoxMap)[menuID].vRef;
    *dirId = (*BoxMap)[menuID].dirId;
  }
}

/************************************************************************
 * VD2MenuId - turn vref & dirID into menu id
 ************************************************************************/
short VD2MenuId(short vRef, long dirId) {
  return ((dirId == MailRoot.dirId && SameVRef(vRef, MailRoot.vRef))
              ? MAILBOX_MENU
              : FindDirLevel(vRef, dirId));
}

/************************************************************************
 * SelectMessage - select a single message in a mailbox
 ************************************************************************/
void SelectMessage(TOCType * tocH, short mNum) {
  SelectBoxRange(tocH, mNum, mNum, False, 0, 0);
  BoxCenterSelection(tocH->win);
}

/************************************************************************
 * BoxSpecByName - Find a given mailbox by a (possibly partial path) name
 ************************************************************************/
int BoxSpecByName(FSSpecPtr spec, char *name) {
  MenuHandle mh = GetMHandle(MAILBOX_MENU);
  UPtr spot;
  Str31 leaf;

  if (*name && name[1] == ':') {
    if (!Path2Box(name, spec))
      return (noErr);
    else {
      spot = PRIndex(name, ':');
      MakePStr(leaf, spot + 1, *name - ((char *)spot - name));
      return (BoxSpecByNameInMenu(mh, spec, leaf));
    }
  }
  return (BoxSpecByNameInMenu(mh, spec, name));
}

/************************************************************************
 * BoxMatchMenuItems - Find a list of mailboxes that more or less match a name
 ************************************************************************/
int BoxMatchMenuItems(unsigned char *name, MenuAndScoreHandle *mashPtr,
                      int score()) {
  MenuHandle mh = GetMHandle(MAILBOX_MENU);
  Accumulator a;
  int err;

  Zero(a);

  err = BoxMatchMenuItemsInMenu(mh, &a, name, score);

  if (err)
    do {
      void **_azh = (a).data;
      if (_azh) {
        if (*_azh)
          free(*_azh);
        free(_azh);
      }
      (a).data = NULL;
      (a).offset = (a).size = 0;
    } while (0);
  else if (a.offset) {
    AccuTrim(&a);
    *mashPtr = (MenuAndScoreHandle)a.data;
    QuickSort((UPtr)LDRef(a.data), sizeof(MenuAndScore), 0,
              a.offset / sizeof(MenuAndScore) - 1, CompareMAS, SwapMAS);
    UL(a.data);
  }

  return err ? err : (a.data ? noErr : fnfErr);
}

int CompareMAS(MenuAndScorePtr mas1, MenuAndScorePtr mas2) {
  return (mas1->score - mas2->score);
}

void SwapMAS(MenuAndScorePtr mas1, MenuAndScorePtr mas2) {
  MenuAndScore mas;

  mas = *mas2;
  *mas2 = *mas1;
  *mas1 = mas;
}

/************************************************************************
 * BoxMatchMenuItemsInMenu - Find a list of mailboxes that more or less match a
 *name
 ************************************************************************/
int BoxMatchMenuItemsInMenu(MenuHandle mh, AccuPtr a, unsigned char *name,
                            int score()) {
  short item;
  short n;
  short sub;
  MenuHandle subMH;
  int err = fnfErr;

  if (err = BoxMatchMenuItemsIn1Menu(mh, a, name, score))
    return err;

  n = CountMenuItems(mh);
  for (item = 1; item <= n; item++)
    if (HasSubmenu(mh, item)) {
      sub = SubmenuId(mh, item);
      if (subMH = GetMHandle(sub)) {
        err = BoxMatchMenuItemsInMenu(subMH, a, name, score);
        if (err && err != fnfErr)
          break;
      }
    }
  return (err);
}

/**********************************************************************
 * BoxMatchMenuItemsIn1Menu - find a mailbox in a single menu
 **********************************************************************/
int BoxMatchMenuItemsIn1Menu(MenuHandle mh, AccuPtr a, unsigned char *name,
                             int score()) {
  Str63 itemTitle;
  short i;
  short n = CountMenuItems(mh);
  short res;
  bool root = GetMenuID(mh) == MAILBOX_MENU;
  MenuAndScore mas;
  int err = noErr;

  mas.menu = GetMenuID(mh);

  for (i = (root ? 1 : 3); i <= n; i++)
    if (!root || (i != MAILBOX_BAR1_ITEM && i != MAILBOX_NEW_ITEM &&
                  i != MAILBOX_OTHER_ITEM)) {
      if (HasSubmenu(mh, i)) {
        short vRefNum;
        long dirID;

        Menu2VD(mh, &vRefNum, &dirID);
        if (!IsIMAPVD(vRefNum, dirID))
          //	If not IMAP mailbox, we have reached the folders, so no more
          // mailboxes
          return (0);
      }

      if (root)
        MailboxMenuFile(MAILBOX_MENU, i, itemTitle);
      else
        MyGetItem(mh, i, itemTitle);
      res = score(name, itemTitle);
      //	The following optimiation check fails in IMAP mailboxes where
      //	the Inbox mailbox is at the top of the list so they mailboxes
      //	are not in alphabetical order
      //			if (res<0 && (!root||i>MAILBOX_BAR1_ITEM))
      // return(0);	// hit one greater than we
      if (res >= 0) {
        // there is some sort of match here
        mas.item = i;
        mas.score = res;
        err = AccuAddPtr(a, &mas, sizeof(mas));
        if (err)
          break;
      }
    }
  return (err);
}

/**********************************************************************
 * BoxMatchScore - score how well a candidate mailbox matches a name
 *   0 is a perfect match
 *   1 is a substring match (ie, the name is a substring of the mailbox)
 *  -1 is no match
 **********************************************************************/
int BoxMatchScore(unsigned char *name, unsigned char *candidate) {
  UPtr spot;
  int score = 0;

  if (spot = PFindSub(name, candidate)) {
    // equal strings is best score (0)
    if (*name == *candidate)
      return 0;

    // if we don't start at the start of the string, add 50
    if (spot > candidate + 1) {
      score += 50;
      // if the character before us is a word character, add 20
      if (IsWordChar[spot[-1]])
        score += 20;
    }
    // Now, check the end
    spot += *name;
    // if we don't end at the end of the string, add 50
    if (spot < candidate + *candidate) {
      score += 50;
      // if the character after us is a word char, add 50
      if (IsWordChar[spot[1]])
        score += 20;
    }
    // Finally, add points for any additional characters
    score += *candidate - *name;
    return score;
  } else
    return -1;
}

/**********************************************************************
 * BoxSpecByNameInMenu - search a given menu for a mailbox
 **********************************************************************/
int BoxSpecByNameInMenu(MenuHandle mh, FSSpecPtr spec, unsigned char *name) {
  short item;
  short n;
  short sub;
  MenuHandle subMH;
  int err = fnfErr;

  if (item = FindBoxByNameIn1Menu(mh, name)) {
    Menu2VD(mh, &spec->vRefNum, &spec->parID);
    SimpleMakeFSSpec(spec->vRefNum, spec->parID, name, spec);
    return (noErr);
  }

  n = CountMenuItems(mh);
  for (item = 1; item <= n; item++)
    if (HasSubmenu(mh, item)) {
      sub = SubmenuId(mh, item);
      if (subMH = GetMHandle(sub)) {
        err = BoxSpecByNameInMenu(subMH, spec, name);
        if (!err)
          break;
        if (err != fnfErr)
          break;
      }
    }
  return (err);
}

/**********************************************************************
 * FindBoxByNameIn1Menu - find a mailbox in a single menu
 **********************************************************************/
short FindBoxByNameIn1Menu(MenuHandle mh, unsigned char *name) {
  Str63 itemTitle;
  short i;
  short n = CountMenuItems(mh);
  short res;
  bool root = GetMenuID(mh) == MAILBOX_MENU;

  for (i = (root ? 1 : 3); i <= n; i++)
    if (!root || (i != MAILBOX_BAR1_ITEM && i != MAILBOX_NEW_ITEM &&
                  i != MAILBOX_OTHER_ITEM)) {
      if (HasSubmenu(mh, i)) {
        short vRefNum;
        long dirID;

        Menu2VD(mh, &vRefNum, &dirID);
        if (!IsIMAPVD(vRefNum, dirID))
          //	If not IMAP mailbox, we have reached the folders, so no more
          // mailboxes
          return (0);
      }

      if (root)
        MailboxMenuFile(MAILBOX_MENU, i, itemTitle);
      else
        MyGetItem(mh, i, itemTitle);
      res = StringComp(name, itemTitle);
      //	The following optimiation check fails in IMAP mailboxes where
      //	the Inbox mailbox is at the top of the list so they mailboxes
      //	are not in alphabetical order
      //			if (res<0 && (!root||i>MAILBOX_BAR1_ITEM))
      // return(0);	// hit one greater than we
      if (!res)
        return (i); // found it!
    }
  return (0);
}

/************************************************************************
 * FirstMsgSelected - return index of first message selected
 ************************************************************************/
short FirstMsgSelected(TOCType * tocH) {
  short i;

  for (i = 0; i < tocH->count; i++)
    if (tocH->sums[i].selected)
      return (i);
  return (-1);
}

/************************************************************************
 * LastMsgSelected - return index of last message selected
 ************************************************************************/
short LastMsgSelected(TOCType * tocH) {
  short i;

  for (i = tocH->count - 1; i >= 0; i--)
    if (tocH->sums[i].selected)
      break;
  return (i);
}

/**********************************************************************
 * CountSelectedMessages - count the number of messages selected
 **********************************************************************/
short CountSelectedMessages(TOCType * tocH) {
  short i;
  short n = 0;

  for (i = 0; i < tocH->count; i++)
    if (tocH->sums[i].selected)
      n++;
  return (n);
}

/**********************************************************************
 * SizeSelectedMessages - figure out how big all the selected messages are
 *  Set countOpenOnes to false to ignore ones that are already open
 **********************************************************************/
long SizeSelectedMessages(TOCType * tocH, bool countOpenOnes) {
  short sum;
  long size = 0;

  for (sum = tocH->count; sum--;) {
    if (tocH->sums[sum].selected)
      if (!countOpenOnes && tocH->sums[sum].messH)
        continue;
      else
        size += tocH->sums[sum].length;
  }
  return size;
}

/**********************************************************************
 * CountFlaggedMessages - count the number of messages flagged for
 * 	filtering
 **********************************************************************/
long CountFlaggedMessages(TOCType * tocH) {
  short i;
  long n = 0;

  for (i = 0; i < tocH->count; i++)
    if (tocH->sums[i].flags & FLAG_UNFILTERED)
      n++;
  return (n);
}

/************************************************************************
 * IsMailbox - is a TEXT file a mailbox?
 ************************************************************************/
bool IsMailbox(FSSpecPtr spec) {
  Handle data;
  short refN = 0;
  long count;
  UPtr spot;
  uLong box, res, file;
  bool from;
  OSType type;

  /*
   * is name too long?
   */
  if (*spec->name > MAX_BOX_NAME)
    return (False);

  /*
   * is file the right type?
   */
  type = FileTypeOf(spec);
  if (type != 'DROP' && type != 'TEXT' && type != IMAP_MAILBOX_TYPE)
    return (False);

  /*
   * toc's?
   */
  TOCDates(spec, &box, &res, &file);
  if (res || file)
    return (True);

  /*
   * No .toc, but maybe that's just because we need to build one
   */
  if (!FSpDFSize(spec))
    return (True); /* empty.  vacuuously ok */

  /*
   * read the first line
   */
  if (Snarf(spec, &data, 255))
    return (False); /* can't read.  don't show */

  /*
   * is it an envelope?
   */
  count = GetHandleSize(data) - 1;
  for (spot = (UPtr)LDRef(data);
       *spot != '\015' && spot < (UPtr)(*data) + count; spot++)
    ;
  spot[1] = 0;
  from = IsFromLine(*data);
  ZapHandle(data);

  return (from);
}

#ifdef TWO
/************************************************************************
 * AskGraft - ask the user for a mailbox to graft
 ************************************************************************/
int AskGraft(short vRef, long dirId, FSSpecPtr spec) {
  SFTypeList types;
  FSSpec fetchedSpec;
  Str255 prompt;
  int theError;
  Boolean good;

  types[0] = 'TEXT';
  types[1] = 0;

  GetRString(prompt, CHOOSE_MBOX);

  /*
   * fetch file
   */
  theError = GetFileNav(types, CHOOSE_MAILBOX_NAV_TITLE, prompt, 0, false,
                        &fetchedSpec, &good, nil);
  /*
   * is it a mailbox?
   */
  if (good && !IsMailbox(&fetchedSpec)) {
    FileSystemError(NOT_MAILBOX, fetchedSpec.name, 0);
    return (userCancelled);
  }

  /*
   * make & return alias
   */
  if (good)
    return (GraftMailbox(vRef, dirId, &fetchedSpec, spec,
                         vRef == MailRoot.vRef && dirId == MailRoot.dirId));
  else
    return (userCancelled);
}

/************************************************************************
 * GraftMailbox - graft a mailbox into the tree
 ************************************************************************/
int GraftMailbox(short vRef, long dirId, FSSpecPtr realSpec, FSSpecPtr boxSpec,
                 bool temporary) {
  short err = 1;
  FSSpec tocSpec, localSpec, realTocSpec;
  FInfo info;
  bool wasThere = !HGetFInfo(vRef, dirId, realSpec->name, &info);

  /*
   * if mailbox exists and is in the current tree, just return, do not
   * graft
   */
  if (FindDirLevel(realSpec->vRefNum, realSpec->parID) >= 0) {
    *boxSpec = *realSpec;
    return (noErr);
  }

  /*
   * if mailbox is currently aliased in the folder in question, just return
   */
  if (!FSMakeFSSpec(vRef, dirId, realSpec->name, &localSpec) &&
      IsAlias(&localSpec, &tocSpec) && SameSpec(&tocSpec, realSpec)) {
    *boxSpec = localSpec;
    return (noErr);
  }

  if (!(err = MakeAFinderAlias(realSpec, &localSpec))) {
    /*
     * if the graft is temporary, mark the alias
     */
    if (temporary && !FSpGetFInfo(&localSpec, &info)) {
      // set the stationery bit.  Yeah, it's a hack.
      info.fdFlags |= kIsStationery;
      FSpSetFInfo(&localSpec, &info);
    }

    /*
     * is there an external toc?
     */
    Box2TOCSpec(realSpec, &tocSpec);
    if (!FSpGetFInfo(&tocSpec, &info)) {
      if (PrefIsSet(PREF_NEW_TOC)) {
        {
          TOCType * tocH = CheckTOC(realSpec);
          if (tocH)
            WriteTOC(tocH);
          g_free(tocH);
        }
      } else {
        realTocSpec = tocSpec;
        tocSpec.vRefNum = vRef;
        tocSpec.parID = dirId;
        err = MakeAFinderAlias(&realTocSpec, &tocSpec);
        if (temporary && !FSpGetFInfo(&tocSpec, &info)) {
          // set the stationery bit.  Yeah, it's a hack.
          info.fdFlags |= kIsStationery;
          FSpSetFInfo(&tocSpec, &info);
        }
      }
    }

    if (!err) {
      /*
       * ok.  aliases in place; all is well with the world
       */
      /*BuildBoxMenus();*/
      AddBoxHigh(&localSpec);
    }
  }

  if (err)
    FileSystemError(OPEN_MBOX, realSpec->name, err);
  else
    *boxSpec = localSpec;
  return (err);
}

#endif

/**********************************************************************
 * RemoveBoxHigh - remove a box from the menus
 **********************************************************************/
void RemoveBoxHigh(FSSpecPtr spec) {
  short level = FindDirLevel(spec->vRefNum, spec->parID);
  Str63 xferName;

  RemoveBox(MAILBOX, spec->name, level);

  GetRString(xferName, TRANSFER_PREFIX);
  PSCat(xferName, spec->name);
  RemoveBox(TRANSFER, xferName, level);
  BuildBoxCount();
  MBTickle(nil, nil);
}

/**********************************************************************
 * AddBoxHigh - add a box to the menus
 **********************************************************************/
void AddBoxHigh(FSSpecPtr spec) {
  short level = FindDirLevel(spec->vRefNum, spec->parID);
  Str63 xferName;
  FInfo info;

  FSpGetFInfo(spec, &info); //	Get unread status of box
  AddBox(MAILBOX, spec->name, level, (info.fdFlags & 0xe) != 0);
  GetRString(xferName, TRANSFER_PREFIX);
  PSCat(xferName, spec->name);
  AddBox(TRANSFER, xferName, level, false);
  BuildBoxCount();
  MBTickle(nil, nil);
}

/**********************************************************************
 * PopupMailboxPath - popup a list of mailboxes and folders
 **********************************************************************/
void PopupMailboxPath(MyWindowPtr win, TOCType * tocH, short sum, Point pt) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  MenuHandle hMenu;
  short top, left;
  Rect rStruct;
  Str255 s;
  Boolean fMessage = false;
  short menuIdx = 0;
  FSSpec spec;
  Boolean IsIMAP = false;
  enum { kSelNone, kSelMailbox, kSelFolder };
  short selection;

  if (hMenu = NewMenu(MB_POPUP_MENU, "")) {
    //	Build menu
    //	Start with current window
    short menu, item;
    MessHandle messH;

    if (win) {
      if (fMessage = IsMessWindow(winWP)) {
        //	This is a message.

        GetWTitle(winWP, s); //	Add name of message
        MyAppendMenu(hMenu, s);
        messH = Win2MessH(win);
        tocH = (*messH)->tocH;
        sum = (*messH)->sumNum;
      } else {
        //	This is a mailbox.
        tocH = (TOCType *)GetMyWindowPrivateData(win);
        if (tocH->virtualTOC)
          //	no popup on virtual mailboxes
          return;
      }
    }

    //	Build all folders
    if (!TOCH2Menu(tocH, false, &menu, &item)) {
      MenuHandle hAddMenu;

      for (hAddMenu = GetMHandle(menu); hAddMenu;
           hAddMenu = ParentMailboxMenu(hAddMenu, &item)) {
        MyGetItem(hAddMenu, item, s);
        MyAppendMenu(hMenu, s);
      }
    }

    //	Name of Eudora Folder
    spec = GetMailboxSpec(tocH, -1);
    IsIMAP = tocH->imapTOC;

    if (!IsIMAP) {
      GetDirName(nil, Root.vRef, Root.dirId, s);
      MyAppendMenu(hMenu, s);
    }

    InsertMenu(hMenu, -1);

    selection = kSelNone;
    if (win) {
      winWP = GetMyWindowWindowPtr(win);
      //	Popup from window title
      GetWindowStructureBounds(winWP, &rStruct);
      top = rStruct.top + 2;
      left = rStruct.right + rStruct.left - MyGetWindowTitleWidth(winWP) - 29;
      left = left / 2;
      if (left < rStruct.left + 19)
        //	Don't go too far to the left
        left = rStruct.left + 19;
      item = PopUpMenuSelect(hMenu, top, left, 0);
      if (item > 1)
        selection = item == 2 && fMessage ? kSelMailbox : kSelFolder;
    } else {
      //	Popup in mailbox summary
      item = AFPopUpMenuSelect(hMenu, pt.v, pt.h, 0);
      if (item)
        selection = item == 1 ? kSelMailbox : kSelFolder;
    }

    switch (selection) {
      WindowPtr tocWinWP;
      Handle hStringList;

    case kSelMailbox:
      //	Open mailbox window
      tocWinWP = GetMyWindowWindowPtr(tocH->win);
      ShowMyWindow(tocWinWP);
      UserSelectWindow(tocWinWP);
      SelectMessage(tocH, sum);
      if (MainEvent.modifiers & optionKey)
        BoxSelectSame(tocH, SORT_SUBJECT_ITEM, sum);
      break;

    case kSelFolder:

      //	Create a list of names of mailbox folders starting with the
      // highest 	level not including the Eudora folder and working on
      // down to the item 	the was selected
      if (hStringList = NuHandle(0)) {
        short itemIdx;

        itemIdx = CountMenuItems(hMenu);
        for (; itemIdx >= item; itemIdx--) {
          MyGetItem(hMenu, itemIdx, s);
          PtrAndHand(s, hStringList, *s + 1);
        }
        //	Add a null
        *s = 0;
        PtrAndHand(s, hStringList, 1);

        MBOpenFolder(hStringList, IsIMAP);
        DisposeHandle(hStringList);
      }
      break;
    }
    DeleteMenu(MB_POPUP_MENU);
    DisposeMenu(hMenu);
  }
}

/**********************************************************************
 *	GetMailboxSpec - get the filespec for the indicated mailbox (and
 *message)
 **********************************************************************/
FSSpec GetMailboxSpec(TOCType * tocH, short sum) {
  FSSpec spec;

  if (tocH) {
    if (tocH->virtualTOC) {
      // virtual mailbox
      short index;

      if (sum < 0 || sum > tocH->count)
        goto error;
      index = tocH->sums[sum].u.virtualMess.virtualMBIdx;
      if (index < 0 || index >= tocH->mailbox.virtualMB.specListCount)
        goto error;
      return (*tocH->mailbox.virtualMB.specList)[index];
    }

    // normal mailbox
    return tocH->mailbox.spec;
  }

// no tocH--shouldn't happen
error:
  Zero(spec);
  return spec;
}

/**********************************************************************
 *	GetMailboxName - get the name of the indicated mailbox (and message)
 **********************************************************************/
UPtr GetMailboxName(TOCType * tocH, short sum, UPtr name) {
  FSSpec spec;

  spec = GetMailboxSpec(tocH, sum);
  PCopy(name, spec.name);
  return name;
}

/**********************************************************************
 *	GetRealSummary - find real summary from a message serial number
 **********************************************************************/
MSumPtr FindRealSummary(TOCType * tocH, long serialNum, short *realSum) {
  short i, count;
  MSumPtr sum;

  if (tocH) {
    count = tocH->count;
    for (i = 0, sum = tocH->sums; i < count; i++, sum++)
      if (serialNum == sum->serialNum) {
        // found it!
        *realSum = i;
        return sum;
      }
  }
  return nil; // not found!
}

/**********************************************************************
 *	FindSumBySerialNum - find real summary from a message serial number
 **********************************************************************/
short FindSumBySerialNum(TOCType * tocH, long serialNum) {
  short sumNum;

  return FindRealSummary(tocH, serialNum, &sumNum) ? sumNum : -1;
}

/**********************************************************************
 *	GetRealTOC - if virtual TOC, return real one
 **********************************************************************/
TOCType * GetRealTOC(TOCType * tocH, short sum, short *realSum) {
  *realSum = sum;
  if (tocH) {
    if (tocH->virtualTOC) {
      // virtual mailbox
      FSSpec spec = GetMailboxSpec(tocH, sum);
      TOCType * realTocH;

      if (!spec.vRefNum)
        goto error;

      realTocH = FindTOC(&spec);
      if (!realTocH) {
        if (GetMailbox(&spec, false))
          goto error;
        realTocH = FindTOC(&spec);
      }
      if (!realTocH)
        goto error;
      if (sum < 0 || sum > tocH->count)
        goto error;

      // search for the message in the real TOC by message serial number
      return FindRealSummary(
                 realTocH, tocH->sums[sum].u.virtualMess.linkSerialNum, realSum)
                 ? realTocH
                 : nil;
    } else {
      // not virtual TOC
      return tocH;
    }
  }

error:
  return nil;
}

/**********************************************************************
 * ProcessIMAPChanges - process IMAP adds, deletes, or updates
 **********************************************************************/
static void ProcessIMAPChanges(Handle sumList, TOCType * toc,
                               IMAPUpdateType message) {
  short count;
  MSumPtr pSum;
  short sum;
  int err = noErr;
  bool selected;
  MailboxNodeHandle mbox = TOCToMbox(toc);
  UIDCopyPtr pCopy;
  TOCType * toTocH;
  TOCType *tocH = NULL, *hidTocH = NULL;
  long numMessages, numUidResponses;
  long j, newUid;
  // NOTE: reverse PREF_IMAP_VISIBLE_SUM_FILTER once we're comfortable with
  // hiding summaries while filtering.
  bool bHideUnfilteredSums = !PrefIsSet(PREF_FOREGROUND_IMAP_FILTERING) &&
                             PrefIsSet(PREF_IMAP_VISIBLE_SUM_FILTER);
  short flaggedColor = GetRLong(IMAP_FLAGGED_LABEL);

  if (!sumList)
    return;

  // find the hidden toc if needed
  hidTocH = GetHiddenCacheMailbox(mbox, false, true);

  // Handle copies first.  The sumList doesn't actually contains summaries.
  // As this deals with local cached message only, a best shot approach should
  // suffice
  if (message == kDoCopy) {
    count = GetHandleSize_(sumList) / sizeof(UIDCopyStruct);
    for (pCopy = LDRef(sumList); count--; pCopy++) {
      toTocH = TOCBySpec(&(pCopy->toSpec));

      if (toTocH && pCopy->hOldSums && pCopy->hNewUIDs) {
        numMessages = GetHandleSize_(pCopy->hOldSums) / sizeof(MSumType);
        numUidResponses = GetHandleSize_(pCopy->hNewUIDs) / sizeof(long);

        for (j = 0; j < numUidResponses; j++) {
          newUid = ((long *)(*(pCopy->hNewUIDs)))[j];
          pSum = &(((
              MSumPtr)(*(pCopy->hOldSums)))[numMessages - numUidResponses + j]);
          IMAPTransferLocalCache(toc, pSum, toTocH, newUid, pCopy->copy);
        }
        if (!IMAPFilteringUnderway() && (!pCopy->copy))
          AddIMAPXfUndoUIDs(toc, toTocH, pCopy->hNewUIDs, false);
      }

      // Cleanup
      ZapHandle(pCopy->hOldSums);
      ZapHandle(pCopy->hNewUIDs);
    }

    goto done;
  }

  count = GetHandleSize_(sumList) / sizeof(MSumType);
  for (pSum = LDRef(sumList); count--; pSum++) {
    Boolean found;

    // Spin the cursor every 100 messages or so.
    if (count && !(count % 100))
      CycleBalls();

    //	search for summary with same UID hash
    if (message != kDoAdd) {
      found = false;

      tocH = toc;
      for (sum = 0; sum < tocH->count; sum++) {
        if (pSum->uidHash == tocH->sums[sum].uidHash) {
          // found it!
          found = true;
          break;
        }
      }

      // look in the hidden toc if appropriate
      if (!found && hidTocH) {
        tocH = hidTocH;
        for (sum = 0; sum < tocH->count; sum++) {
          if (pSum->uidHash == tocH->sums[sum].uidHash) {
            // found it!
            found = true;
            break;
          }
        }
      }
    }

    switch (message) {
    case kDoAdd:
      // does this message already exist in the cache?  Maybe it was copied.
      // Skip it.
      if ((FindSumByHash(toc, pSum->uidHash) != -1) ||
          (hidTocH && (FindSumByHash(hidTocH, pSum->uidHash) != -1)))
        break;

      // is this message a deleted or unfiltered message and should we hide it?
      if (hidTocH && ((pSum->opts & OPT_DELETED) ||
                      (bHideUnfilteredSums && (pSum->flags & FLAG_UNFILTERED))))
        tocH = hidTocH;
      else
        tocH = toc;

      if (!SaveMessageSum(pSum, &tocH)) {
        // end the resync
        IMAPAbortResync(toc);
        goto done;
      }

      // did we add a deleted message?
      if (pSum->opts & OPT_DELETED)
        SetIMAPMailboxNeeds(TOCToMbox(tocH), kNeedsAutoExp, true);

      if (PrefIsSet(PREF_COUNT_ALL_IMAP) ||
          // count only those messages received in InBox
          TOCToMbox(toc) == LocateInboxForPers(TOCToPers(toc)))
        UpdateNumStatWithTime(kStatReceivedMail, 1, pSum->seconds + ZoneSecs());
      break;

    case kDoDeleteAttachments:
      // go trash this message's attachments, if we should
      if (found)
        CleanUpAttachmentsAfterIMAPTransfer(tocH, sum);
      break;

    case kDoDelete:
      if (found) {
        selected = tocH->sums[sum].selected;
        DeleteIMAPSum(tocH, sum); // delete the summary

        // select the next summary if we oughtta
        if (tocH->win && selected && !IMAPFilteringUnderway())
          BoxSelectAfter(tocH->win, sum);
      }
      break;

    case kDoUpdate:
      // Don't update this message if it's in the list of pending changes.
      if (found && !PendingMessFlagChange(tocH->sums[sum].uidHash, mbox)) {
        // update the message state, unless it's in a state we want to keep
        if (UpdatableIMAPState(tocH->sums[sum].state))
          tocH->sums[sum].state = pSum->state;

        // update the message label if we ought to
        if (SumColor(pSum) ==
            flaggedColor) // new label is flagged, update the existing label
          SetSumColor(tocH, sum, flaggedColor);
        else if (SumColor(tocH->sums + sum) ==
                 flaggedColor) // old label is flagged, new label is not, turn
                               // off label
          SetSumColor(tocH, sum, 0);
        // else
        //  leave the label alone

        // update the deleted status
        if (pSum->opts & OPT_DELETED)
          MarkSumAsDeleted(tocH, sum, true);
        else
          MarkSumAsDeleted(tocH, sum, false);
        ;

        // make sure the summary is in the right toc.
        if (hidTocH)
          HideShowSummary(tocH, toc, hidTocH, sum);

        // redraw the summary
        InvalSum(tocH, sum);

        // save the changes
        TOCSetDirty(tocH, true);
      }
      break;
    }
  }

done:
  if (message != kDoDeleteAttachments)
    ZapHandle(sumList);
}

/************************************************************************
 * IMAPRecvLine - read a line at a time from the spool file. Returns ".\015"
 * at the ends of messages.
 ************************************************************************/
static int IMAPRecvLine(TransStream stream, UPtr buffer, long *size) {
  static bool wasFrom;
  static bool wasNl = True;
  short lineType;

  if (!buffer) {
    bool retVal = wasFrom;
    wasFrom = False;
    return (retVal);
  }
  wasFrom = False;
  (*size)--;

  lineType = GetLine(buffer, *size, size, Lip);
  if (*buffer == '\012') {
    //	remove linefeed char
    BMD(buffer + 1, buffer, *size - 1);
    (*size)--;
    buffer[*size] = 0;
  }
  if (!*size || !lineType ||
      /*wasNl&&(wasFrom=IsFromLine(buffer)) ||*/ TellLine(Lip) >= gIMAPMsgEnd) {
    //	signal end-of-message
    *size = 2;
    buffer[0] = '.';
    buffer[1] = '\015';
    buffer[2] = 0;
  } else if (lineType && wasNl && *buffer == '.') {
    //	insert '.' at beginning of line
    BMD(buffer, buffer + 1, *size);
    (*size)++;
    *buffer = '.';
    buffer[*size] = 0;
  }
  wasNl = !lineType || buffer[*size - 1] == '\015';
  return (noErr);
}

/**********************************************************************
 * UpdateIMAPMailbox - check for any changes to the local IMAP mailbox
 **********************************************************************/
OSErr UpdateIMAPMailbox(TOCType * toc) {
  Handle toAdd = nil, toUpdate = nil, toDelete = nil, toCopy = nil;
  IMAPSResultHandle results = nil;
  short sumNum;
  MailboxNodeHandle mbox = nil;
  FSSpec spec;
  bool filter = false;
  bool checkAttachments = false;

  //
  //	Resync this mailbox if it needs it, and filtering is NOT currently
  // underway
  //

  if (!IMAPFilteringUnderway() && (mbox = TOCToMbox(toc))) {
    if (DoesIMAPMailboxNeed(mbox, kNeedsResync)) {
      // wait if there's already a resync operation underway
      if (!IsIMAPOperationUnderway(IMAPResyncTask)) {
        if (PrefIsSet(PREF_FOREGROUND_IMAP_FILTERING))
          SetIMAPMailboxNeeds(mbox, kNeedsResync, false);

        // is it visible?
        LockMailboxNodeHandle(mbox);
        if (toc->win && IsWindowVisible(GetMyWindowWindowPtr(toc->win))) {
          FetchNewMessages(toc, true, false, true, false);
          SetIMAPMailboxNeeds(mbox, kNeedsResync, false);
        }
        UnlockMailboxNodeHandle(mbox);
      }
    }
  }

  //
  //	Check for changes to TOC summaries
  //

  if (IMAPDelivery(toc, &toAdd, &toUpdate, &toDelete, &toCopy, &filter,
                   &results, &mbox, &checkAttachments)) {
    //	Install IMAP summary changes

    // process copies first.  Deletes could step on them later.
    ProcessIMAPChanges(toCopy, toc, kDoCopy);

    if (toDelete == MSUM_DELETE_ALL) {
      //	delete everything
      for (sumNum = toc->count; sumNum--;)
        DeleteIMAPSum(toc, sumNum);
    } else {
      if (checkAttachments)
        ProcessIMAPChanges(toDelete, toc, kDoDeleteAttachments);
      ProcessIMAPChanges(toDelete, toc, kDoDelete);
    }

    ProcessIMAPChanges(toAdd, toc, kDoAdd);
    ProcessIMAPChanges(toUpdate, toc, kDoUpdate);

    // if we succeeded ...
    if (mbox) {
      // update the mailbox information ...
      spec = toc->mailbox.spec;
      WriteIMAPMailboxInfo(&spec, mbox);

      if (toc && toc->win &&
          IsWindowVisible(GetMyWindowWindowPtr(toc->win))) {
        // resort mailbox if needed
        if (toc->resort)
          MBResort(toc);

        /// do message selection
        if (DoesIMAPMailboxNeed(mbox, kNeedsSelect)) {
          SetIMAPMailboxNeeds(mbox, kNeedsSelect, true);

          // make selection if nothing is selected.
          if (LastMsgSelected(toc) < 0)
            ShowBoxAt(toc, toc->previewPTE ? -1 : FumLub(toc),
                      GetMyWindowWindowPtr(toc->win));
        }
      }
    }
  }

  //
  //	move downloaded messages from their spool file to the mailbox
  //

  while (IMAPMessagesWaiting(toc, &spec)) {
    TOCType * hidTocH = NULL;

    // first, decode messages in the visible portion of the toc
    DecodeIMAPMessages(toc, &spec);

    // next, do the messages in the hidden tocH
    if ((hidTocH = GetHiddenCacheMailbox(mbox, false, false)) != NULL)
      DecodeIMAPMessages(hidTocH, &spec);

    // mark this temp file as having been processed ...
    MarkAsProcessed(&spec);

    //	Don't need spool file anymore
    FSpDelete(&spec);
  }

  //
  // do filtering
  //

  if (filter) {
    // the user has asked us filter the old fashioned foreground way
    if (PrefIsSet(PREF_FOREGROUND_IMAP_FILTERING)) {
      PersHandle oldPers = CurPers;

      // Make sure this personality is set to filter incoming IMAP mail
      mbox = TOCToMbox(toc);
      CurPers = TOCToPers(toc);

      if (CurPers && !PrefIsSet(PREF_IMAP_NO_FILTER_INBOX)) {
        // if there are any no new mail alerts pending, forget them.
        NoNewMailMe = false;

        // filter the mailbox, display the mail alerts
        NotifyNewMail(1, false, toc, nil);
      }

      CurPers = oldPers;

      // reset the filter flags on all messages, whether filtering happened or
      // not.
      ResetFilterFlags(toc);
    } else {
      // start background IMAP filtering ...
      NeedToFilterIMAP = true;
    }
  }
}

/**********************************************************************
 * DeleteIMAPSum - remove an IMAP summary from a toc
 **********************************************************************/
void DeleteIMAPSum(TOCType * tocH, int sumNum) {
  SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum, false,
                  true);
  DeleteMessageLo(tocH, sumNum, true);
}

/**********************************************************************
 * UpdateIMAPMailbox - check for any changes to the local IMAP mailbox
 **********************************************************************/
bool IsMailboxSubmenu(short menu) {
  return g16bitSubMenuIDs ? menu >= BOX_MENU_START && menu < BOX_MENU_LIMIT
                          : menu >= 1 && menu < FIND_HIER_MENU;
}

/**********************************************************************
 * UpdateIMAPMailbox - check for any changes to the local IMAP mailbox
 *
 *	Note, this needs to be COMPLETELY REMOVED.  This is extremely slow
 *	and should be done another way.  That's  abig project for another
 *	day, though.
 **********************************************************************/
void DecodeIMAPMessages(TOCType * toc, FSSpecPtr spec) {
  int err;
  short count;
  short fileRef;
  IndexStruct IMAPIdx;
  IndexStruct **hIMAPIndex;
  short curIMAPIndex;
  short countIMAP;
  TransVector saveCurTrans = CurTrans;
  short saveRefN = CurResFile();
  PersHandle oldPers = CurPers;
  static TransVector IMAPTrans = {nil, nil, nil, nil,          nil, nil,
                                  nil, nil, nil, IMAPRecvLine, nil};
  MailboxNodeHandle mbox = TOCToMbox(toc);
  short sumNum;

  /*
   * allocate the lineio pointer
   */
  if (!Lip)
    Lip = NuPtrClear(sizeof(*Lip));
  if (!Lip) {
    (WarnUser(MEM_ERR, MemError()));
    goto msgDone;
  }
  if (FSpOpenLine(spec, fsRdWrPerm, Lip))
    goto msgDone;

  // CurPers must be set to the owning personality
  mbox = TOCToMbox(toc);
  CurPers = TOCToPers(toc);

  // just trash this file if the personality it belongs to has gone away.
  if (!CurPers)
    goto msgDone;

  //	Get the IMAP index resource
  hIMAPIndex = nil;
  if ((fileRef = FSpOpenResFile(spec, fsRdPerm)) != -1) {
    hIMAPIndex = (IndexStruct **)Get1Resource(INDEX_RES_TYPE, INDEX_RES_ID);
    DetachResource((Handle)hIMAPIndex);
    CloseResFile(fileRef);
  }
  if (!hIMAPIndex)
    goto msgDone;

  // now, grab messages
  TOCSetDirty(toc, true);
  countIMAP = GetHandleSize_(hIMAPIndex) / sizeof(IndexStruct);

  // open the destination mailbox
  err = BoxFOpen(toc);
  if (err != noErr) {
    FileSystemError(OPEN_MBOX, toc->mailbox.spec.name, err);
  } else {
    for (curIMAPIndex = 0; curIMAPIndex < countIMAP; curIMAPIndex++) {
      BadBinHex = False;
      BadEncoding = 0;
      if (!AttachedFiles)
        AttachedFiles = NuHandle(0);
      SetHandleBig_(AttachedFiles, 0);

      IMAPIdx = (*hIMAPIndex)[curIMAPIndex];
      //	search for the summary
      count = 0;
      for (sumNum = 0; sumNum < toc->count; sumNum++) {
        if (toc->sums[sumNum].uidHash == IMAPIdx.uid) {
          MessHandle messH;
          MyWindowPtr win;

          SeekLine(IMAPIdx.offset, Lip);
          gIMAPMsgEnd = IMAPIdx.offset + IMAPIdx.length;
          CurTrans = IMAPTrans;
          count = FetchMessageTextLo(nil, toc->sums[sumNum].length, nil,
                                     sumNum, toc, true, false);
          CurTrans = saveCurTrans;
          SetHandleBig_(AttachedFiles, 0);
          SaveAbomination(nil, 0);
#ifdef BAD_ENCODING_HANDLING
          if (BadBinHex || BadEncoding)
            NoAttachments = True;
          else
#endif
            NoAttachments = False;

          // set the sum options if this message needs to have an attachment
          // downloaded.
          if (HasStubFileAttachment(toc, sumNum))
            toc->sums[sumNum].opts |= OPT_FETCH_ATTACHMENTS;
          else
            toc->sums[sumNum].opts &= ~OPT_FETCH_ATTACHMENTS;

          // moodmail?
          if (AnalDoIncoming())
            AnalBox(toc, sumNum, sumNum);

          // spamwatch?
          if (HasFeature(featureJunk) && JunkPrefBoxHold() && CanScoreJunk()) {
            // only score message if it hasn't been manually scored before
            if (toc->sums[sumNum].spamBecause != JUNK_BECAUSE_USER)
              JunkScoreIMAPBox(toc, sumNum, sumNum, false);
          }

          // redraw the summary
          InvalSum(toc, sumNum);

          //	Redisplay message
          if ((messH = toc->sums[sumNum].messH) && (*messH)->bodyPTE &&
              (win = (*messH)->win))
            RedisplayIMAPMessage(win);

          //	Update the preview pane.
          if (toc->previewID == toc->sums[sumNum].serialNum)
            toc->previewID = 0; // redraw previewed message
          else
            toc->conConMultiScan = true; // run concentrator.

          // delete the message if a translator has asked us to
          if (ETLDeleteRequest) {
            toc->sums[sumNum].opts |=
                OPT_ORPHAN_ATT; // don't delete its attachments

            // jdboyd 7/30/04
            //
            // No matter what the download options and fitlering type are, this
            // message gets deleted during the filtering process.  Turning off
            // the unfiltered flag keeps it around.  I don't know what this was
            // there before, but it's causing problems removing ESP commands.
            //
            // toc->sums[sumNum].flags &= ~FLAG_UNFILTERED;
            // // don't run filters on this message

            toc->sums[sumNum].opts |=
                OPT_EMSR_DELETE_REQUESTED; // mark this message so we know to
                                           // delete later
            ETLDeleteRequest = false;
          }

          // mark this toc as dirty so the changes get saved.
          TOCSetDirty(toc, true);

          break;
        }
      }
    }
  }

  // close and flush the new messages to disk
  err = 0; // BoxFClose is void
  BoxFClose(toc, true);

msgDone:
  if (Lip && Lip->fd) {
    CloseLine(Lip);
    free(Lip);
    Lip = nil;
  }
  NoAttachments = False;
  ZapHandle(hIMAPIndex);

  UseResFile(saveRefN);

  CurPers = oldPers;

  // take care of any registration files that may have come with this message
  ProcessReceivedRegFiles();
}
