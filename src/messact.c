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
 * messact.c — Message action functions, GTK4 port
 *
 * Original Mac version used Carbon controls, QuickDraw drawing,
 * Handle-based memory, Pascal strings, Mac window manager, etc.
 *
 * GTK4 port:
 *   - MessHandle still **MessType (double pointer) per message.h
 *   - TOCType * direct pointer (no void *indirection on TOC)
 *   - PETEHandle / bodyPTE / subPTE = GtkWidget* (GtkTextView via gEditCtrl)
 *   - ControlHandle = void* (GtkWidget* buttons/widgets)
 *   - QuickDraw drawing → GTK4 widgets / CSS styling
 *   - Mac window manager → GTK4 window management
 *   - FSSpec → portable struct with path field
 */

#include "messact.h"

#include "Globals.h"
#include "MyRes.h"
#include "StrnDefs.h"
#include "comp.h"
#include "features.h"
#include "filtrun.h"
#include "find.h"
#include "gtk_menus.h"
#include "imapdownload.h"
#include "junk.h"
#include "legacy_shim.h"
#include "mailbox.h"
#include "message.h"
#include "mydefs.h"
#include "nickmng.h"
#include "pop.h"
#include "print.h"
#include "schizo.h"
#include "searchwin.h"
#include "sendmail.h"
#include "toc.h"
#include "util.h"
#include "utl.h"
#include "StringUtil.h"
#include "StringDefs.h"
#include "fileutil.h"
#include "peteglue.h"

/* Forward declarations for functions not in available headers */
extern bool IsMailboxChoice(short menu, short item);
extern bool IsMailboxSubmenu(short menu);
extern bool GetTransferParams(short menu, short item, char * spec, void *p);
extern bool SameSpec(char * a, char * b);
extern void MoveMessage(TOCType *tocH, short sumNum, char * spec, bool copy);
extern void AddXfUndo(TOCType *fromTocH, TOCType *toTocH, short sumNum);
extern void MakeMessTitle(unsigned char *title, TOCType *tocH, short sumNum, bool full);
extern void InvalTopMargin(MyWindowPtr win);
extern void MakeMessFileName(TOCType *tocH, short sumNum, unsigned char *name);
extern void GetAttFolderPath(char *buf, int bufSize);
extern int GetIMAPAttachFolderPath(char *buf, int bufSize);
extern void GetPartsFolder(char *buf, int bufSize);
extern void *MessText(MessHandle messH);
extern void SetMessRich(MessHandle messH);
extern int AccuAddHandle(AccuPtr a, void *h);
extern int AccuAddFromHandle(AccuPtr a, void *h, long from, long to);
extern void AccuTrim(AccuPtr a);
extern int HTMLPreamble(AccuPtr a, unsigned char *title, int flags, bool b);
extern int HTMLPostamble(AccuPtr a, bool b);
extern int BuildHTML(AccuPtr a, GtkWidget *pte, void *p1, long len, long off,
                     void *p2, void *p3, int n, unsigned char *mid, void *p4, void *p5);
extern int BuildEnriched(AccuPtr a, GtkWidget *pte, void *p1, long len, long off,
                         void *p2, bool b);
extern int SaveTextAsMessage(void *extras, void *text, TOCType *tocH, long *fromLen);
extern void ReplyDefaults(short modifiers, bool *all, bool *self, bool *quote);
extern void DoReplyMessage(MyWindowPtr win, bool all, bool self, bool quote,
                           bool b1, short item, bool b2, bool b3, bool b4);
extern void DoRedistributeMessage(MyWindowPtr win, void *addr, bool turbo,
                                  bool b1, bool b2);
extern void DoForwardMessage(MyWindowPtr win, void *addr, bool b);
extern void DoSalvageMessage(MyWindowPtr win, bool b);
extern void DeleteMessage(TOCType *tocH, short sumNum, bool nuke);
extern TOCType *GetTrashTOC(void);
extern void ServerMenuChoice(TOCType *tocH, short sumNum, short item, bool shift);
extern void SetPriority(TOCType *tocH, short sumNum, short priority);
extern short AdjustSpecialMenuSelection(short item);
extern void MakeNickFromSelection(MyWindowPtr win);
extern void MakeMessNick(MyWindowPtr win, short modifiers);
extern void DoMakeFilter(MyWindowPtr win);
extern void SetState(TOCType *tocH, int sumNum, int state);
extern short Item2Status(short item);
extern void SelectBoxRange(TOCType *tocH, int start, int end, bool cmd, int eStart, int eEnd);
extern void BoxCenterSelection(MyWindowPtr win);
extern void BeenThereDoneThat(TOCType *tocH, short sumNum);
extern void *MenuItem2Handle(short menu, short item);
extern void SetTopMargin(MyWindowPtr win, short margin);
extern void SetBGColorsByPers(MessHandle messH);
extern int CacheMessage(TOCType *tocH, short sumNum);
extern void GenerateReceipt(MessHandle messH, int disp, int dispLocal, int action, int sent);
extern bool DisplayGetGraphics(MyWindowPtr win);
extern int Box2Path(const char *boxPath, unsigned char *path);
extern void CompIBarUpdate(MessHandle messH);
extern void InsertCommaIfNeedBe(GtkWidget *pte, HeadSpec *hs);
extern short SubjCompare(unsigned char *s1, unsigned char *s2);
extern TOCType *GetRealTOC(TOCType *tocH, short sumNum, short *realSum);
extern bool FindRealSummary(TOCType *tocH, long serialNum, short *sumNum);
extern void BoxOpen(MyWindowPtr win);

#ifndef peeEvent
#define peeEvent 0
#endif
#ifndef peCantUndo
#define peCantUndo 0
#endif
#ifndef peUndoPaste
#define peUndoPaste 1
#endif
#ifndef ATTACH_GONE
#define ATTACH_GONE 2000
#endif
#ifndef CANT_SAVE_RICH
#define CANT_SAVE_RICH 2001
#endif
#ifndef FCC_PREFIX
#define FCC_PREFIX 3000
#endif
#ifndef MDN_DISPLAYED
#define MDN_DISPLAYED 1
#endif
#ifndef MDN_DISPLAYED_LOCAL
#define MDN_DISPLAYED_LOCAL 2
#endif
#ifndef MDN_AUTO_ACTION
#define MDN_AUTO_ACTION 3
#endif
#ifndef MDN_MAN_ACTION
#define MDN_MAN_ACTION 4
#endif
#ifndef MDN_AUTO_SENT
#define MDN_AUTO_SENT 5
#endif
#ifndef MDN_MAN_SENT
#define MDN_MAN_SENT 6
#endif

#ifndef OPT_WRITE
#define OPT_WRITE  (1<<4)
#endif
#ifndef OPT_EDITED
#define OPT_EDITED (1<<8)
#endif
#ifndef OPT_RECEIPT
#define OPT_RECEIPT (1<<12)
#endif
#ifndef COMP_TOP_MARGIN
#define COMP_TOP_MARGIN 6001
#endif
#ifndef Menu2Label
#define Menu2Label(m) ((m) - ((m) > 10 ? 3 : ((m) > 1 ? 2 : 1)))
#endif
#ifndef OPT_AUTO_OPENED
#define OPT_AUTO_OPENED (1<<14)
#endif
#ifndef OPT_WEIRD_REPLY
#define OPT_WEIRD_REPLY (1<<9)
#endif

/* PrefIsSetOrNot: if rev modifier is held, invert the pref */
#ifndef PrefIsSetOrNot
#define PrefIsSetOrNot(pref, mods, rev) \
  ((mods & (rev)) ? !PrefIsSet(pref) : PrefIsSet(pref))
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

/* ---- MControlsEnum — icon bar button identifiers ---- */
typedef enum {
  mcSend_ = 0x20636530, /* ' ce0' — underscore avoids conflicts */
  mcWrite_,
  mcBlahBlah_,
  mcFetch_,
  mcGetGraphics_,
  mcFixed_,
  mcTrash_,
  mcMesgErrors_,
  mcMesgErrText_,
  mcJunk_,
  mcMesgErrSep_,
  mcLimit_
} MControlsEnum_;

/* ---- Forward declarations for static helpers ---- */
static void MessUpdate(MyWindowPtr win);
static void MessHelp(MyWindowPtr win, Point mouse);
static bool IsAttLine(unsigned char *line, bool wantToOpen);
static int OpenAttLine(GtkWidget *pte, unsigned char *line, bool finderSelect,
                       bool printIt);
static bool GetBlahRect(MyWindowPtr win, short which, Rect *r);
static void MessButton(MyWindowPtr win, ControlHandle button, long modifiers,
                       short part);
static short SaveAsToOpenFileLo(short refN, MessHandle messH);
static void AddNotifyControls(MessHandle messH);
static void PlaceNotifyControls(MyWindowPtr win);

/* Buffer size for UnwrapSave */
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 8192
#endif

/* Priority menu item constants */
#ifndef pymRaise
#define pymRaise 1
#define pymHighest 2
#define pymLowest 6
#define pymLower 7
#endif

/* Local priority conversion (Mac stores 1-5, display 2-6) */
static short PriorToDisp(short p) {
  if (p < 1) p = 1;
  if (p > 5) p = 5;
  return p + 1;
}
static short DispToPrior(short d) {
  if (d < 2) d = 2;
  if (d > 6) d = 6;
  return d - 1;
}

/* Arrow key constants */
#ifndef leftArrowChar
#define leftArrowChar 0x1C
#define rightArrowChar 0x1D
#define upArrowChar 0x1E
#define downArrowChar 0x1F
#endif

/* Key constants */
#ifndef tabChar
#define tabChar '\t'
#endif
#ifndef enterChar
#define enterChar 0x03
#endif
#ifndef returnChar
#define returnChar '\r'
#endif
#ifndef delChar
#define delChar 0x08
#endif

#ifndef charCodeMask
#define charCodeMask 0xFF
#endif

/* I_WIDTH for icon bar */
#define I_WIDTH 29

/* NOTIFY_CNTL reference constant */
#ifndef NOTIFY_CNTL
#define NOTIFY_CNTL 0x4E544659 /* 'NTFY' */
#endif
#ifndef MDN_REQUEST
#define MDN_REQUEST 1500
#endif
#ifndef NOTIFY_SOUND
#define NOTIFY_SOUND 1000
#endif

/* ---- PTE helper wrappers — delegate to gEditCtrl/GtkTextView ---- */
static bool PeteIsValid_(GtkWidget *pte) {
  return pte != NULL && GTK_IS_TEXT_VIEW(pte);
}

static long PeteLen_(GtkWidget *pte) {
  if (!PeteIsValid_(pte)) return 0;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  return gtk_text_buffer_get_char_count(buf);
}

static void PeteSString_(unsigned char *pStr, GtkWidget *pte) {
  if (!PeteIsValid_(pte)) { pStr[0] = 0; return; }
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);
  char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
  if (text) {
    long len = strlen(text);
    if (len > 255) len = 255;
    pStr[0] = (unsigned char)len;
    memcpy(pStr + 1, text, len);
    g_free(text);
  } else {
    pStr[0] = 0;
  }
}

static int PeteGetTextAndSelection_(GtkWidget *pte, char **textOut,
                                    long *selStart, long *selEnd) {
  if (!PeteIsValid_(pte)) return -1;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);
  char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
  if (!text) return -1;
  *textOut = text;
  GtkTextIter selS, selE;
  if (gtk_text_buffer_get_selection_bounds(buf, &selS, &selE)) {
    *selStart = gtk_text_iter_get_offset(&selS);
    *selEnd = gtk_text_iter_get_offset(&selE);
  } else {
    GtkTextMark *insert = gtk_text_buffer_get_insert(buf);
    GtkTextIter cur;
    gtk_text_buffer_get_iter_at_mark(buf, &cur, insert);
    *selStart = *selEnd = gtk_text_iter_get_offset(&cur);
  }
  return 0;
}

static void PeteSetDirty_(GtkWidget *pte) {
  if (!PeteIsValid_(pte)) return;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  gtk_text_buffer_set_modified(buf, TRUE);
}

/* ============================================================
 * MessClose - close a message window
 * ============================================================ */
bool MessClose(MyWindowPtr win) {
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  TOCType *tocH = messH->tocH;
  int sumNum = messH->sumNum;

  if (!GrowZoning && TheBody && messH->subPTE &&
      win->pte == messH->subPTE)
    MessFocus(messH, TheBody);

  if (PeteIsDirty(messH->subPTE))
    MessSaveSub(messH);

  if (!GrowZoning)
    win->isDirty = win->isDirty || PeteIsDirty(TheBody);

  if (!NoSaves && !GrowZoning)
    if (!SaveMessHi(win, true))
      return false;

  LL_Remove(MessList, messH, (MessHandle));

  free(messH->extras.data); messH->extras.data = NULL; messH->extras.offset = messH->extras.size = 0;
  free(messH->aSourceMID.data); messH->aSourceMID.data = NULL; messH->aSourceMID.offset = messH->aSourceMID.size = 0;

  if (messH->etlFiles)
    free(messH->etlFiles);

  win->privateData = nil;

  free(messH->newsGroupAcc.data); messH->newsGroupAcc.data = NULL; messH->newsGroupAcc.offset = messH->newsGroupAcc.size = 0;

  g_free(messH); messH = NULL;
  tocH->sums[sumNum].messH = nil;

  if (tocH->imapTOC)
    IMAPAbortMessageFetch(tocH, sumNum);

  return true;
}

/* ============================================================
 * SaveMessHi - save a message if the user wants and if dirty
 * ============================================================ */
bool SaveMessHi(MyWindowPtr win, bool closing) {
  short which;
  if (win->isDirty) {
    which = WannaSave(win);
    if (which == WANNA_SAVE_CANCEL) return false;
    else if (which == WANNA_SAVE_SAVE) {
      if (!SaveMess(win)) return false;
    } else if (which == WANNA_SAVE_DISCARD) {
      if (!closing) ReopenMessage(win);
    }
  }
  return true;
}

/* ============================================================
 * NewPrior - compute a new priority from a menu and an old priority
 * ============================================================ */
short NewPrior(short item, short prior) {
  short disp;
  switch (item) {
  case pymRaise:
    disp = PriorToDisp(prior);
    disp = (pymHighest > disp - 1) ? pymHighest : disp - 1;
    prior = DispToPrior(disp);
    break;
  case pymLower:
    disp = PriorToDisp(prior);
    disp = (pymLowest < disp + 1) ? pymLowest : disp + 1;
    prior = DispToPrior(disp);
    break;
  default:
    prior = DispToPrior(item);
    break;
  }
  return prior;
}

/* ============================================================
 * TransferMenuChoice - handle a menu choice from a transfer menu
 * ============================================================ */
bool TransferMenuChoice(short menu, short item, TOCType *tocH, short sumNum,
                        long modifiers, bool fcc) {
  FSSpec spec, toSpec;
  short function = REAL_BIG;

  if (!IsMailboxChoice(menu, item)) return false;

  if (menu == TRANSFER_MENU)
    function = TRANSFER;
  else if (IsMailboxSubmenu(menu))
    function = (menu - BOX_MENU_START) / MAX_BOX_LEVELS;

  if (function == TRANSFER) {
    GetMailboxSpec(tocH, sumNum, spec);
    if (GetTransferParams(menu, item, &toSpec, nil) &&
        !SameSpec(&spec, &toSpec)) {
      if (HasFeature(featureFcc) && fcc)
        Fcc(tocH->sums[sumNum].messH, &toSpec);
      else if (sumNum >= 0) {
        if (!(modifiers & optionKey)) {
          if (!tocH->imapTOC)
            AddXfUndo(tocH, TOCBySpec(&toSpec), sumNum);
          EzOpen(tocH, sumNum, 0, modifiers, true, true);
        }
        MoveMessage(tocH, sumNum, &toSpec, (modifiers & optionKey) != 0);
      } else
        MoveSelectedMessages(tocH, &toSpec, (modifiers & optionKey) != 0);
    }
    return true;
  }
  return false;
}

/* ============================================================
 * SetSubject - set the subject in a message summary
 * ============================================================ */
void SetSubject(TOCType *tocH, short sumNum, unsigned char *sub) {
  unsigned char oldSubj[64];
  unsigned char title[256];
  MessHandle messH = tocH->sums[sumNum].messH;

  g_strlcpy((char *)oldSubj, (char *)tocH->sums[sumNum].subj, 64);
  if (!EqualString(oldSubj, sub, true, true)) {
    g_strlcpy((char *)tocH->sums[sumNum].subj, (char *)sub, 60);
    InvalSum(tocH, sumNum);
    TOCSetDirty(tocH, true);

    if (messH) {
      MakeMessTitle(title, tocH, sumNum, true);
      if (messH->win && messH->win->window) {
        char cTitle[257];
        g_strlcpy(cTitle, (char *)title, sizeof(cTitle));
        gtk_window_set_title(GTK_WINDOW(messH->win->window), cTitle);
      }
      if (messH->subPTE && PeteIsValid_(messH->subPTE)) {
        GtkTextBuffer *buf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(messH->subPTE));
        char cSub[256];
        int subLen = sub[0];
        memcpy(cSub, sub + 1, subLen);
        cSub[subLen] = '\0';
        gtk_text_buffer_set_text(buf, cSub, -1);
      }
    }
    SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum,
                    false, false);
  }
}

/* ============================================================
 * SetSender - set the sender in a message summary
 * ============================================================ */
void SetSender(TOCType *tocH, short sumNum, unsigned char *sender) {
  unsigned char oldSender[64];
  unsigned char title[256];
  MessHandle messH = tocH->sums[sumNum].messH;

  g_strlcpy((char *)oldSender, (char *)tocH->sums[sumNum].from, 64);
  if (!EqualString(oldSender, sender, true, true)) {
    g_strlcpy((char *)tocH->sums[sumNum].from, (char *)sender, 48);
    InvalSum(tocH, sumNum);
    TOCSetDirty(tocH, true);
    if (messH) {
      MakeMessTitle(title, tocH, sumNum, true);
      if (messH->win && messH->win->window) {
        char cTitle[257];
        g_strlcpy(cTitle, (char *)title, sizeof(cTitle));
        gtk_window_set_title(GTK_WINDOW(messH->win->window), cTitle);
      }
    }
  }
}

/* ============================================================
 * SetFlag - set one of the message flags
 * ============================================================ */
void SetFlag(TOCType *tocH, short sumNum, long flag, bool on) {
  if (on)
    tocH->sums[sumNum].flags |= flag;
  else
    tocH->sums[sumNum].flags &= ~flag;
  if (tocH->sums[sumNum].messH)
    InvalTopMargin(tocH->sums[sumNum].messH->win);
  TOCSetDirty(tocH, true);
}

/* ============================================================
 * SetOpt - set one of the message options
 * ============================================================ */
void SetOpt(TOCType *tocH, short sumNum, long flag, bool on) {
  if (on)
    tocH->sums[sumNum].opts |= flag;
  else
    tocH->sums[sumNum].opts &= ~flag;
  TOCSetDirty(tocH, true);
}

/* ============================================================
 * AttIsSelected - check/open/color selected attachments
 * ============================================================ */
bool AttIsSelected(MyWindowPtr win, PETEHandle pte, long startWith,
                   long endWith, short what, long *aStart, long *aEnd) {
  char *text = NULL;
  long selStart, selEnd;
  bool found = false;
  bool colorThem = 0 != (what & attColor);
  bool selectThem = 0 != (what & attSelect);
  bool openThem = 0 != (what & attOpen);
  bool finderSelect = 0 != (what & attFinder);
  bool printThem = 0 != (what & attPrint);

  if (!PeteIsValid_(pte) && win) pte = win->pte;
  if (!PeteIsValid_(pte)) return false;

  if (PeteGetTextAndSelection_(pte, &text, &selStart, &selEnd))
    return false;
  if (!text || strlen(text) == 0) { g_free(text); return false; }

  long textLen = strlen(text);
  long sBegin = (startWith < 0) ? selStart : startWith;
  long sEnd = (endWith < 0) ? selEnd : endWith;
  if (sBegin < 0) sBegin = 0;
  if (sBegin >= textLen) goto done;
  if (sEnd > textLen) sEnd = textLen;
  if (sEnd <= 0) goto done;

  /* Expand selection to whole lines */
  while (sBegin > 0 && text[sBegin - 1] != '\n') sBegin--;
  if (selStart != selEnd && sEnd > 0 && text[sEnd - 1] == '\n') sEnd--;
  else while (sEnd < textLen && text[sEnd] != '\n') sEnd++;

  /* Examine each line */
  for (long lBegin = sBegin; lBegin < sEnd;) {
    long lEnd = lBegin;
    while (lEnd < sEnd && text[lEnd] != '\n') lEnd++;

    long lineLen = lEnd - lBegin;
    if (lineLen > 254) lineLen = 254;
    unsigned char line[256];
    line[0] = (unsigned char)lineLen;
    memcpy(line + 1, text + lBegin, lineLen);

    FSSpec spec;
    if (!AttLine2Spec(line, &spec, openThem)) {
      found = true;
      if (colorThem) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
        GtkTextIter iterS, iterE;
        gtk_text_buffer_get_iter_at_offset(buf, &iterS, lBegin);
        gtk_text_buffer_get_iter_at_offset(buf, &iterE, lEnd);
        GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
        GtkTextTag *tag = gtk_text_tag_table_lookup(table, "attachment");
        if (!tag)
          tag = gtk_text_buffer_create_tag(buf, "attachment",
                    "foreground", "blue",
                    "underline", PANGO_UNDERLINE_SINGLE, NULL);
        gtk_text_buffer_apply_tag(buf, tag, &iterS, &iterE);
      }
      if (selectThem) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
        GtkTextIter iterS, iterE;
        gtk_text_buffer_get_iter_at_offset(buf, &iterS, lBegin);
        gtk_text_buffer_get_iter_at_offset(buf, &iterE, lEnd);
        gtk_text_buffer_select_range(buf, &iterS, &iterE);
        PeteSetDirty_(pte);
      }
      if (openThem || printThem)
        OpenAttLine(pte, line, finderSelect, printThem);
      if (aStart) *aStart = lBegin;
      if (aEnd) *aEnd = lEnd;
    }
    lBegin = lEnd + 1;
  }
done:
  g_free(text);
  return found;
}

static bool IsAttLine(unsigned char *line, bool wantToOpen) {
  return !AttLine2Spec(line, nil, wantToOpen);
}

/* ============================================================
 * OpenAttLine - open an attachment via GLib
 * ============================================================ */
static int OpenAttLine(GtkWidget *pte, unsigned char *line, bool finderSelect,
                       bool printIt) {
  int err = 0;
  FSSpec spec;
  (void)finderSelect;
  (void)printIt;

  err = AttLine2Spec(line, &spec, true);
  if (err) return err;

  if (IsIMAPAttachmentStub(&spec))
    err = FetchIMAPAttachment(pte, &spec, true);

  if (err == 0) {
    GError *error = NULL;
    GFile *file = g_file_new_for_path(spec);
    GAppInfo *app = g_file_query_default_handler(file, NULL, &error);
    if (app) {
      GList *files = g_list_append(NULL, file);
      if (!g_app_info_launch(app, files, NULL, &error)) {
        g_warning("Failed to open attachment: %s", error->message);
        g_error_free(error);
        err = -1;
      }
      g_list_free(files);
      g_object_unref(app);
    } else {
      if (error) g_error_free(error);
      err = -1;
    }
    g_object_unref(file);
  }
  return err;
}

/* ============================================================
 * AttLine2Spec - get an FSSpec from an attachment line
 * ============================================================ */
int AttLine2Spec(unsigned char *line, char * spec, bool wantToOpen) {
  long fid = 0;
  int lineLen = line[0];

  /* Trim whitespace */
  while (lineLen > 0 && (line[lineLen] == ' ' || line[lineLen] == '\t'))
    lineLen--;
  line[0] = lineLen;

  if (lineLen < 10) return fnfErr;
  if (!isalnum(line[1]) && line[1] != '/') return fnfErr;

  /* Pick out file ID from trailing [xxxxxxxx] or (xxxxxxxx) */
  if (line[lineLen] != ']' && line[lineLen] != ')') return fnfErr;
  if (line[lineLen - 9] != '[' && line[lineLen - 9] != '(') return fnfErr;

  unsigned char *spot = line + lineLen - 8;
  for (int dig = 8; dig > 0; dig--, spot++) {
    if (*spot >= '0' && *spot <= '9')
      fid = (fid << 4) | (*spot - '0');
    else if (*spot >= 'a' && *spot <= 'f')
      fid = (fid << 4) | (10 + *spot - 'a');
    else if (*spot >= 'A' && *spot <= 'F')
      fid = (fid << 4) | (10 + *spot - 'A');
    else return fnfErr;
  }

  /* Find path separators (: for Mac, / for Unix) */
  unsigned char *sep1 = NULL, *sep2 = NULL;
  for (unsigned char *p = line + 1; p < line + lineLen; p++) {
    if (*p == ':' || *p == '/') {
      if (!sep1) sep1 = p;
      else if (!sep2) { sep2 = p; break; }
    }
  }
  if (!sep1 || !sep2) return fnfErr;

  if (spec) {
    long nameStart = (sep2 - line);
    long nameEnd = lineLen - 10;
    if (nameEnd <= nameStart) return fnfErr;
    long nameLen = nameEnd - nameStart;
    if (nameLen < 1 || nameLen > 255) return fnfErr;

    memcpy((char*)spec_name(spec), (char *)line + nameStart + 1, nameLen);
    ((char*)spec_name(spec))[nameLen] = '\0';

    /* Try attachment folder */
    char attFolder[512];
    GetAttFolderPath(attFolder, sizeof(attFolder));
    snprintf(spec, sizeof(spec), "%s/%s", attFolder, spec_name(spec));

    if (access(spec, F_OK) != 0) {
      char imapFolder[512];
      if (GetIMAPAttachFolderPath(imapFolder, sizeof(imapFolder)) == 0) {
        snprintf(spec, sizeof(spec), "%s/%s",
                 imapFolder, spec_name(spec));
        if (access(spec, F_OK) != 0) {
          if (wantToOpen) WarnUser(ATTACH_GONE, fnfErr);
          return fnfErr;
        }
      } else {
        if (wantToOpen) WarnUser(ATTACH_GONE, fnfErr);
        return fnfErr;
      }
    }
    return noErr;
  } else {
    long sepDist = sep2 - sep1;
    return (sepDist > 1 & sepDist < 33) ? noErr : fnfErr;
  }
}

/* ============================================================
 * RelLine2Spec - get an FSSpec from a "related" line
 * ============================================================ */
int RelLine2Spec(unsigned char *line, char * spec, uLong *cid,
                 uLong *relURL, uLong *absURL) {
  char cLine[256];
  int len = line[0];
  if (len > 254) len = 254;
  memcpy(cLine, line + 1, len);
  cLine[len] = '\0';

  if (strncasecmp(cLine, "related:", 8) != 0) return fnfErr;

  char *saveptr;
  char *token = strtok_r(cLine, ":", &saveptr);
  if (!token) return fnfErr;
  token = strtok_r(NULL, ":", &saveptr); /* space */
  if (!token) return fnfErr;
  token = strtok_r(NULL, ":", &saveptr); /* volume */
  if (!token) return fnfErr;
  char *filename = strtok_r(NULL, ":", &saveptr);
  if (!filename) return fnfErr;
  char *fidStr = strtok_r(NULL, ":", &saveptr);
  if (!fidStr || strlen(fidStr) != 8) return fnfErr;
  char *cidStr = strtok_r(NULL, ":", &saveptr);
  if (!cidStr || strlen(cidStr) != 8) return fnfErr;
  if (cid) *cid = strtoul(cidStr, NULL, 16);
  char *relStr = strtok_r(NULL, ":", &saveptr);
  if (!relStr || strlen(relStr) != 8) return fnfErr;
  if (relURL) *relURL = strtoul(relStr, NULL, 16);
  char *absStr = strtok_r(NULL, ":", &saveptr);
  if (!absStr || strlen(absStr) != 8) return fnfErr;
  if (absURL) *absURL = strtoul(absStr, NULL, 16);

  if (spec) {
    spec_set_name(spec, filename);
    char partsFolder[512];
    GetPartsFolder(partsFolder, sizeof(partsFolder));
    snprintf(spec, sizeof(spec), "%s/%s",
             partsFolder, spec_name(spec));
    if (access(spec, F_OK) != 0) return fnfErr;
  }
  return noErr;
}

/* ============================================================
 * SaveMess - save a message into its own mailbox
 * ============================================================ */
bool SaveMess(MyWindowPtr win) {
  MessHandle messH = Win2MessH(win);
  TOCType *tocH = messH->tocH;
  long fromLen;
  void *text = MessText(messH);
  HeadSpec hSpec;
  Accumulator enriched;
  int err = noErr;
  bool richSave = false;
  unsigned char title[256];
  bool blahBlah = MessFlagIsSet(messH, FLAG_SHOW_ALL);

  if (!blahBlah) {
    SetMessRich(messH);
    memset(&enriched, 0, sizeof(enriched));
    if (MessIsRich(messH)) {
      if (!(err = AccuInit(&enriched))) {
        CompHeadFind(messH, 0, &hSpec);
        if (!messH->extras.offset ||
            !(err = AccuAddHandle(&enriched, messH->extras.data)))
          if (!(err = AccuAddFromHandle(&enriched, text, 0, hSpec.value))) {
            if (MessOptIsSet(messH, OPT_HTML)) {
              unsigned char scratch[256];
              g_strlcpy((char *)scratch, (char *)SumOf(messH)->subj, 256);
              if (!(err = HTMLPreamble(&enriched, scratch, 0, true))) {
                NumToString(SumOf(messH)->msgIdHash, scratch);
                if (!(err = BuildHTML(&enriched, TheBody, nil,
                                      strlen((char *)text), hSpec.value,
                                      nil, nil, 1, scratch, nil, nil)))
                  err = HTMLPostamble(&enriched, true);
              }
            } else {
              err = BuildEnriched(&enriched, TheBody, nil,
                                  PeteLen_(TheBody), hSpec.value, nil, true);
            }
            if (!err) {
              AccuTrim(&enriched);
              err = SaveTextAsMessage(nil, enriched.data, messH->tocH,
                                      &fromLen);
              if (enriched.data) { free(enriched.data); enriched.data = NULL; };
              enriched.data = nil;
              if (err) return false;
              richSave = true;
            }
          }
      }
    }
  }

  if (err) {
    WarnUser(CANT_SAVE_RICH, err);
    if (enriched.data) { free(enriched.data); enriched.data = NULL; };
    ClearMessFlag(messH, FLAG_RICH);
    ClearMessOpt(messH, OPT_HTML);
  }

  if (!richSave) {
    void *extras =
        (!blahBlah & messH->extras.offset) ? messH->extras.data : nil;
    if (SaveTextAsMessage(extras, text, messH->tocH, &fromLen))
      return false;
  }

  MSumType *oldSum = &tocH->sums[messH->sumNum];
  MSumType *newSum = &tocH->sums[tocH->count - 1];

  tocH->usedK -= oldSum->length / 1024;
  tocH->updateBoxSizes = true;
  oldSum->offset = newSum->offset;
  oldSum->length = newSum->length;
  oldSum->bodyOffset = newSum->bodyOffset;
  if (newSum->seconds & !(oldSum->flags & FLAG_OUT)) {
    oldSum->seconds = newSum->seconds;
    oldSum->origZone = newSum->origZone;
  }

  if (tocH->imapTOC && (oldSum->opts && OPT_IMAP_SENT)) {
    if (oldSum->from[0]) g_strlcpy((char *)newSum->from, (char *)oldSum->from, 48);
  } else {
    if (newSum->from[0]) g_strlcpy((char *)oldSum->from, (char *)newSum->from, 48);
  }

  InvalSum(tocH, messH->sumNum);
  if (messH->win && messH->win->window) {
    MakeMessTitle(title, tocH, messH->sumNum, true);
    char cTitle[257];
    g_strlcpy(cTitle, (char *)title, sizeof(cTitle));
    gtk_window_set_title(GTK_WINDOW(messH->win->window), cTitle);
  }

  tocH->count--;

  messH->weeded = fromLen + (blahBlah ? 0 : messH->extras.offset);
  PeteSetURLRescan(TheBody, 0);
  PeteCleanList(win->pte);
  win->isDirty = false;
  free(SumOf(messH)->cache);
  SetMessOpt(messH, OPT_EDITED);
  free(messH->etlFiles);
  if (tocH->previewID == SumOf(messH)->serialNum)
    tocH->previewID = 0;
  return true;
}

/* ============================================================
 * ShowMessageSeparator
 * ============================================================ */
void ShowMessageSeparator(GtkWidget *pte, bool center) {
  if (!PeteIsValid_(pte)) return;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);
  char *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
  if (!text) return;
  long body = BodyOffset(text);
  g_free(text);

  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset(buf, &iter, body);
  gtk_text_buffer_place_cursor(buf, &iter);
  GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
  gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(pte), mark, 0.0, TRUE, 0.0,
                               center ? 0.5 : 1.0);
}

/* ============================================================
 * MessMenu - handle menu choices for a message window
 * ============================================================ */
bool MessMenu(MyWindowPtr win, int menu, int item, short modifiers) {
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  TOCType *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  bool result = false;
  short tableId;
  bool turbo = false;
  void *addr = nil;

  switch (menu) {
  case FILE_MENU:
    switch (item) {
    case FILE_PRINT_ITEM:
    case FILE_PRINT_ONE_ITEM:
      MessFocus(messH, TheBody);
      if ((modifiers & shiftKey) &&
          AttIsSelected(win, win->pte, -1, -1, attOpen + attPrint, nil, nil))
        ;
      else
        PrintOneMessage(messH->win, (modifiers & shiftKey) != 0,
                        item == FILE_PRINT_ONE_ITEM);
      result = true;
      break;
    case FILE_SAVE_ITEM:
      if (win->isDirty) {
        if (messH->subPTE && PeteIsDirty(messH->subPTE))
          MessSaveSub(messH);
        if (messH->bodyPTE && PeteIsDirty(messH->bodyPTE))
          SaveMess(win);
        PeteCleanList(win->pte);
        win->isDirty = false;
      }
      result = true;
      break;
    case FILE_BROWSE_ITEM:
      ExportHTML(messH);
      result = true;
      break;
    case FILE_SAVE_AS_ITEM:
      SaveMessageAs(messH);
      result = true;
      break;
    }
    break;

  case SERVER_HIER_MENU:
    ServerMenuChoice(messH->tocH, messH->sumNum, item,
                     (modifiers & shiftKey) != 0);
    result = true;
    break;

  case MESSAGE_MENU:
    MessFocus(messH, TheBody);
    switch (item) {
    case MESSAGE_REPLY_ITEM: {
      bool all, quote, self;
      ReplyDefaults(modifiers, &all, &self, &quote);
      DoReplyMessage(win, all, self, quote, true, 0, true, true, true);
      result = true;
      break;
    }
    case MESSAGE_REDISTRIBUTE_ITEM:
      DoRedistributeMessage(win, 0,
          PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers, optionKey),
          !(modifiers & shiftKey), true);
      result = true;
      break;
    case MESSAGE_FORWARD_ITEM:
      DoForwardMessage(win, 0, true);
      result = true;
      break;
    case MESSAGE_SALVAGE_ITEM:
      DoSalvageMessage(win, false);
      result = true;
      break;
    case MESSAGE_JUNK_ITEM:
    case MESSAGE_NOTJUNK_ITEM:
      Junk(tocH, sumNum, item == MESSAGE_JUNK_ITEM, true);
      break;
    case MESSAGE_DELETE_ITEM:
      EzOpen(messH->openedFromTocH, -1, messH->openedFromSerialNum,
             modifiers, true, true);
      NoSaves = !MessOptIsSet(Win2MessH(win), OPT_WRITE);
      if (CloseMyWindow(win->window)) {
        AddXfUndo(tocH, GetTrashTOC(), sumNum);
        DeleteMessage(tocH, sumNum,
            (modifiers & (optionKey | shiftKey)) == (optionKey | shiftKey));
      }
      NoSaves = false;
      result = true;
      break;
    }
    break;

  case REPLY_WITH_HIER_MENU:
    if (HasFeature(featureStationery)) {
      bool all, quote, self;
      ReplyDefaults(modifiers, &all, &self, &quote);
      DoReplyMessage(win, all, self, quote, true, item, true, true, true);
    }
    result = true;
    break;

  case FORWARD_TO_HIER_MENU:
    addr = MenuItem2Handle(menu, item);
    DoForwardMessage(win, addr, true);
    result = true;
    break;

  case REDIST_TO_HIER_MENU:
    turbo = PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers, optionKey);
    if (turbo) EzOpen(tocH, sumNum, 0, modifiers, true, true);
    addr = MenuItem2Handle(menu, item);
    DoRedistributeMessage(win, addr, turbo, !(modifiers & shiftKey), true);
    result = true;
    free(addr);
    break;

  case TABLE_HIER_MENU:
    if (Menu2TableId(tocH, GetMHandle(TABLE_HIER_MENU), item, &tableId))
      SetMessTable(tocH, sumNum, tableId);
    result = true;
    break;

  case STATE_HIER_MENU:
    SetState(tocH, sumNum, Item2Status(item));
    result = true;
    break;

  case LABEL_HIER_MENU:
    item = Menu2Label(item);
    SetSumColor(tocH, sumNum, item);
    result = true;
    break;

  case SPECIAL_MENU:
    switch (AdjustSpecialMenuSelection(item)) {
    case SPECIAL_MAKE_NICK_ITEM:
      if (modifiers & shiftKey)
        MakeNickFromSelection(win);
      else
        MakeMessNick(win, modifiers);
      result = true;
      break;
    case SPECIAL_MAKEFILTER_ITEM:
      DoMakeFilter(win);
      break;
    case SPECIAL_FILTER_ITEM: {
      uint32_t ezOpenSN = messH->ezOpenSerialNum;
      FilterMessage(flkManual, tocH, sumNum);
      if (tocH->count) {
        if (ezOpenSN)
          EzOpen(tocH, sumNum, ezOpenSN, modifiers, false, false);
        else
          BoxSelectAfter(tocH->win, sumNum);
      }
      result = true;
      break;
    }
    }
    break;

  case PRIOR_HIER_MENU:
    SetPriority(tocH, sumNum, NewPrior(item, tocH->sums[sumNum].priority));
    result = true;
    break;

  case PERS_HIER_MENU:
    if (HasFeature(featureMultiplePersonalities)) {
      unsigned char name[64];
      SetPers(tocH, sumNum,
              FindPersByName(MyGetItem(GetMHandle(menu), item, name)), true);
      return true;
    }
    break;

  default:
    if (messH->openedFromTocH != messH->tocH)
      sumNum = FindSumBySerialNum(messH->openedFromTocH,
                                  messH->openedFromSerialNum);
    result = TransferMenuChoice(menu, item, messH->openedFromTocH,
                                sumNum, modifiers, false);
    break;
  }
  free(addr);
  return result;
}

/* ============================================================
 * Fcc - folder carbon copy to BCC
 * ============================================================ */
void Fcc(MessHandle messH, char * box) {
  unsigned char scratch[256];
  HeadSpec hs;
  long start;
  unsigned char path[256];

  if (!HasFeature(featureFcc)) return;
  UseFeature(featureFcc);

  if (Box2Path(box, path)) g_strlcpy((char *)path, (char *)spec_name(box), 256);
  scratch[0] = 0;
  PCatC(scratch, '"');
  PCatR(scratch, FCC_PREFIX);
  PCat(scratch, path);
  PCatC(scratch, '"');

  if (CompHeadFind(messH, BCC_HEAD, &hs)) {
    PetePrepareUndo(TheBody, peCantUndo, hs.stop, hs.stop, &start, nil);
    InsertCommaIfNeedBe(TheBody, &hs);
    CompHeadAppendStr(TheBody, &hs, scratch);
    CompHeadFind(messH, BCC_HEAD, &hs);
    PeteSelect(nil, TheBody, hs.stop, hs.stop);
    PeteFinishUndo(TheBody, peUndoPaste, start, hs.stop);
    ClearMessFlag(messH, FLAG_KEEP_COPY);
    CompIBarUpdate(messH);
  }
}

/* ============================================================
 * EzOpenFind - find next candidate for EzOpen
 * ============================================================ */
short EzOpenFind(TOCType *tocH, short origSum) {
  long ez = GetPrefLong(PREF_NO_EZ_OPEN);
  if (origSum < tocH->count - 1) {
    if (ez == 1) return origSum + 1;
    if (tocH->sums[origSum + 1].state == UNREAD) {
      if (ez == 3 || ez == 2) return origSum + 1;
      if (ez == 4) {
        unsigned char s1[64], s2[64];
        g_strlcpy((char *)s1, (char *)tocH->sums[origSum].subj, 64);
        g_strlcpy((char *)s2, (char *)tocH->sums[origSum + 1].subj, 64);
        if (!SubjCompare(s1, s2)) return origSum + 1;
      }
    }
  }
  if (ez == 2)
    for (short s = origSum + 2; s < tocH->count; s++)
      if (tocH->sums[s].state == UNREAD) return s;
  return -1;
}

/* ============================================================
 * EzOpen - easy open
 * ============================================================ */
void EzOpen(TOCType *tocH, short sumNum, uLong serialNum, long modifiers,
            bool hideFront, bool willDelete) {
  short newSumNum;
  short ez;
  bool preview = false;

  if (!tocH->win) return;
  if (sumNum < 0) {
    sumNum = FindSumBySerialNum(tocH, serialNum);
    if (sumNum < 0) return;
  }

  if (hideFront) {
    TOCType *realTOC;
    short realSum;
    realTOC = GetRealTOC(tocH, sumNum, &realSum);
    if (realTOC->sums[realSum].messH)
      serialNum = realTOC->sums[realSum].messH->ezOpenSerialNum;
    else if (tocH->previewPTE && tocH->previewID) {
      preview = true;
      serialNum = tocH->ezOpenSerialNum;
    }
  } else
    sumNum = FindSumBySerialNum(tocH, serialNum);

  if (serialNum & (newSumNum = FindSumBySerialNum(tocH, serialNum)) >= 0 &&
      tocH->sums[newSumNum].state == UNREAD)
    ; /* found */
  else {
    if (!hideFront) newSumNum = EzOpenFind(tocH, sumNum - 1);
    else newSumNum = EzOpenFind(tocH, sumNum);
  }

  if (newSumNum >= 0) {
    ez = GetPrefLong(PREF_NO_EZ_OPEN);
    if (modifiers & shiftKey) ez = 0;
    SelectBoxRange(tocH, newSumNum, newSumNum, false, -1, -1);
    if (ez) {
      if (preview) Preview(tocH, newSumNum);
      else BoxOpen(tocH->win);
    }
    BoxCenterSelection(tocH->win);
  } else {
    if (willDelete)
      newSumNum = sumNum == tocH->count - 1 ? sumNum - 1 : sumNum + 1;
    else
      newSumNum = sumNum;
    if (newSumNum > tocH->count - 1) newSumNum = tocH->count - 1;
    if (newSumNum < 0) newSumNum = 0;
    SelectBoxRange(tocH, newSumNum, newSumNum, false, -1, -1);
  }
}

/* Helper for SaveMessageAs GTK4 native dialog */
static int _save_response;
static GMainLoop *_save_loop;
static void save_msg_dialog_response_cb(GtkNativeDialog *d, int response,
                                        gpointer user_data) {
  (void)d; (void)user_data;
  _save_response = response;
  g_main_loop_quit(_save_loop);
}

/* ============================================================
 * SaveMessageAs - save message as text via GtkFileChooser
 * ============================================================ */
void SaveMessageAs(MessHandle messH) {
  MyWindowPtr win = messH->win;
  if (!win || !win->window) return;

  GtkFileChooserNative *dialog = gtk_file_chooser_native_new(
      "Save Message As", GTK_WINDOW(win->window),
      GTK_FILE_CHOOSER_ACTION_SAVE, "_Save", "_Cancel");

  unsigned char name[32];
  MakeMessFileName(messH->tocH, messH->sumNum, name);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), (const char *)name);

  /* GTK4: run native dialog synchronously with local main loop */
  static int _save_response;
  static GMainLoop *_save_loop;
  _save_response = GTK_RESPONSE_CANCEL;
  _save_loop = g_main_loop_new(NULL, FALSE);
  g_signal_connect(dialog, "response",
      G_CALLBACK(save_msg_dialog_response_cb), NULL);
  gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
  g_main_loop_run(_save_loop);
  g_main_loop_unref(_save_loop);
  int response = _save_response;
  if (response == GTK_RESPONSE_ACCEPT) {
    GFile *file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
    char *path = g_file_get_path(file);
    FILE *fp = fopen(path, "w");
    if (fp) {
      short refN = fileno(fp);
      int err = SaveAsToOpenFile(refN, messH);
      fclose(fp);
      if (err) g_file_delete(file, NULL, NULL);
    }
    g_free(path);
    g_object_unref(file);
  }
  g_object_unref(dialog);
}

/* ============================================================ */
short SaveAsToOpenFile(short refN, MessHandle messH) {
  return SaveAsToOpenFileLo(refN, messH);
}

static short SaveAsToOpenFileLo(short refN, MessHandle messH) {
  long bytes;
  short err = 0;
  unsigned char *where;
  void *text;
  bool para = PrefIsSet(PREF_PARAGRAPHS);
  bool exclHead =
      PrefIsSet(PREF_EXCLUDE_HEADERS) || (SumOf(messH)->flags & FLAG_SUBSEQUENT);
  bool isOut = messH->tocH->which == OUT;
  HeadSpec hs;

  PETEGetRawText(nil, TheBody, &text);
  CompHeadFind(messH, 0, &hs);

  if (!exclHead & isOut & SumOf(messH)->seconds) {
    unsigned char scratch[64];
    BuildDateHeader(scratch, SumOf(messH)->seconds);
    int len = strlen((const char *)scratch);
    write(refN, scratch, len);
    write(refN, "\n", 1);
  }

  where = (unsigned char *)text;
  bytes = strlen((char *)text);
  if (exclHead) {
    where = (unsigned char *)text + hs.value;
    bytes = hs.stop - hs.value;
    while (bytes > 1 & *where == '\n') { where++; bytes--; }
    while (bytes > 1 & where[bytes - 1] == where[bytes - 2] &&
           where[bytes - 1] == '\n')
      bytes--;
  }

  if (!para) {
    long written = write(refN, where, bytes);
    if (written != bytes) err = -1;
    if (bytes > 0 & where[bytes - 1] != '\n')
      write(refN, "\n", 1);
  } else {
    err = UnwrapSave(where, bytes, 0, refN);
  }
  if (!err) BeenThereDoneThat(messH->tocH, messH->sumNum);
  return err;
}

/* ============================================================
 * UnwrapSave - save and unwrap a message
 * ============================================================ */
#define UW_SpaceRuns    ((uwPref & 1) == 0)
#define UW_CentBreak    ((uwPref & 2) == 0)
#define UW_Indent       ((uwPref & 4) == 0)
#define UW_Quote        ((uwPref & 8) == 0)
#define UW_ShortLine    ((uwPref & 16) == 0)
#define UW_BlankLine    ((uwPref & 32) == 0)

int UnwrapSave(unsigned char *text, long length, long offset, short refN) {
  struct LI { int length, indent, quote, needReturn; };
  struct LI lines[2], *cur, *prev;
  int flip, c, begin, spaces;
  unsigned char *tSpot;
  unsigned char *buffer = malloc(BUFFER_SIZE);
  unsigned char *bSpot, *bEnd;
  int err = 0;
  long bytes;
  unsigned char qch = '>';
  bool qspace = true;
  uLong uwPref = GetPrefLong(PREF_UNWRAP_OPTIONS);

  if (!buffer) return -108;
  bEnd = buffer + BUFFER_SIZE;
  bSpot = buffer;
  memset(lines, 0, sizeof(lines));
  cur = lines; prev = lines + 1;
  flip = cur->length = cur->indent = cur->needReturn =
      prev->length = prev->indent = 0;
  prev->needReturn = begin = 1;

#define FLUSH_BUF() do { bytes = bSpot - buffer; if (bytes) { \
    if (write(refN, buffer, bytes) != bytes) { err = -1; goto uwdone; } \
    bSpot = buffer; } } while(0)
#define PUT_CH(ch) do { *bSpot++ = (ch); if (bSpot == bEnd) FLUSH_BUF(); } while(0)

  for (tSpot = text + offset; tSpot < text + length; tSpot++) {
    c = *tSpot;
    if (c == '\n' || c == '\r') {
      cur->needReturn = cur->needReturn ||
          (UW_CentBreak & cur->indent > 20) ||
          (UW_ShortLine & cur->length < 40) ||
          (UW_BlankLine && cur->length == 0);
      if (cur->needReturn) {
        if (cur->length == 0) {
          if (!prev->needReturn) PUT_CH('\n');
          for (int i = 0; i < cur->quote; i++) PUT_CH(qch);
        }
        PUT_CH('\n');
      } else PUT_CH(' ');
      prev = cur; flip = 1 - flip; cur = lines + flip;
      cur->length = cur->quote = cur->indent = cur->needReturn = 0;
      begin = 1; spaces = 0;
    } else if (c == ' ') {
      if (begin) cur->indent++; else spaces++;
    } else if (begin & c == qch) {
      cur->quote++;
    } else {
      if (!begin) {
        if (spaces > 4 & UW_SpaceRuns) { PUT_CH('\t'); cur->needReturn = 1; }
        else if (spaces) { PUT_CH(' '); cur->length++; }
      } else if (((UW_Indent && cur->indent > prev->indent) ||
                  (UW_Quote & cur->quote != prev->quote)) &&
                 !prev->needReturn) {
        PUT_CH('\n');
        for (int i = 0; i < cur->quote; i++) PUT_CH(qch);
        cur->length += cur->quote;
        if (qspace) { cur->length++; PUT_CH(' '); }
      } else if (cur->quote & prev->needReturn) {
        for (int i = 0; i < cur->quote; i++) PUT_CH(qch);
        cur->length += cur->quote;
        if (qspace) { cur->length++; PUT_CH(' '); }
      }
      begin = 0; spaces = 0;
      PUT_CH(c); cur->length++;
    }
  }
  FLUSH_BUF();
uwdone:
  free(buffer);
  return err;
}
#undef FLUSH_BUF
#undef PUT_CH
#undef UW_SpaceRuns
#undef UW_CentBreak
#undef UW_Indent
#undef UW_Quote
#undef UW_ShortLine
#undef UW_BlankLine

/* ============================================================
 * MessKey - handle a keydown in a message window
 * ============================================================ */
bool MessKey(MyWindowPtr win, void *eventPtr) {
  EventRecord *event = (EventRecord *)eventPtr;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  TOCType *tocH = messH->tocH;
  long uLetter = UnadornMessage(event) & charCodeMask;
  bool bodyEdit = !win->ro & win->pte == TheBody;
  bool shift = 0 != (event->modifiers & shiftKey);

  if (leftArrowChar <= uLetter & uLetter <= downArrowChar &&
      IsArrowSwitch(event->modifiers)) {
    NextMess(tocH, messH, uLetter, event->modifiers, false);
    return true;
  } else if ((event->modifiers & cmdKey) &&
             (uLetter == delChar || uLetter == deleteKey)) {
    MessMenu(win, MESSAGE_MENU, MESSAGE_DELETE_ITEM, event->modifiers);
    return true;
  } else if (!bodyEdit & (uLetter == tabChar || uLetter == enterChar)) {
    MessFocus(messH, win->pte == messH->subPTE ? TheBody : messH->subPTE);
    if (win->pte == messH->subPTE) {
      GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(win->pte));
      GtkTextIter start, end;
      gtk_text_buffer_get_start_iter(buf, &start);
      gtk_text_buffer_get_end_iter(buf, &end);
      gtk_text_buffer_select_range(buf, &start, &end);
    }
  } else if (event->modifiers & cmdKey) {
    return false;
  } else if (win->ro & uLetter == ' ') {
    PeteScroll(TheBody, 0, shift ? -1 : 1);
    if (!shift) NextMess(tocH, messH, downArrowChar, 0, true);
    return true;
  } else if (win->ro & DirtyKey(event->message)) {
    if (win->window) {
      GtkAlertDialog *alert = gtk_alert_dialog_new("This message is read-only.");
      gtk_alert_dialog_show(alert, GTK_WINDOW(win->window));
      g_object_unref(alert);
    }
  } else if (!bodyEdit & !win->ro & uLetter == returnChar) {
    gdk_display_beep(gdk_display_get_default());
  } else {
    PeteEdit(win, win->pte, peeEvent, (void *)event);
  }
  PeteSetDirty_(win->pte);
  return true;
}

/* ============================================================
 * NextMess - skip to the next message
 * ============================================================ */
void NextMess(TOCType *tocH, MessHandle messH, short whichWay,
              long modifiers, bool ezOpen) {
  short next, sumNum;
  bool close, diffTOC;

  if (IsArrowSwitch(modifiers))
    modifiers &= ~GetPrefLong(PREF_SWITCH_MODIFIERS);
  close = !(modifiers & optionKey);

  if (ezOpen & tocH == messH->openedFromTocH) {
    EzOpen(tocH, messH->sumNum, 0, 0, true, false);
    CloseMyWindow(messH->win->window);
    return;
  }

  if (messH->openedFromTocH && tocH != messH->openedFromTocH) {
    tocH = messH->openedFromTocH;
    if (!FindRealSummary(tocH, messH->openedFromSerialNum, &sumNum))
      return;
    diffTOC = true;
  } else {
    sumNum = messH->sumNum;
    diffTOC = false;
  }

  switch (whichWay) {
  case upArrowChar: case leftArrowChar: next = sumNum - 1; break;
  case downArrowChar: case rightArrowChar: next = sumNum + 1; break;
  default: return;
  }

  if (next != sumNum) {
    if (close & messH->win->isDirty) {
      if (!CloseMyWindow(messH->win->window)) return;
      close = false;
    }
    if (next >= 0 & next < tocH->count) {
      if (tocH->sums[next].messH) {
        MyWindowPtr nextWin = tocH->sums[next].messH->win;
        if (nextWin && nextWin->window)
          gtk_window_present(GTK_WINDOW(nextWin->window));
      } else {
        GetAMessage(tocH, next, nil, nil, true);
        if (diffTOC) BeenThereDoneThat(tocH, next);
      }
      SelectBoxRange(tocH, next, next, false, -1, -1);
      BoxCenterSelection(tocH->win);
    }
  }
  if (close) CloseMyWindow(messH->win->window);
}

/* ============================================================
 * MessFind - find in the window
 * ============================================================ */
bool MessFind(MyWindowPtr win, char *what) {
  return FindInPTE(win, Win2MessH(win)->bodyPTE, (const char *)what);
}

/* ============================================================
 * MessClick - handle a click in the message window
 * ============================================================ */
void MessClick(MyWindowPtr win, void *event) {
  (void)event;
  if (!win->isActive && win->window) {
    gtk_window_present(GTK_WINDOW(win->window));
    return;
  }
}

/* ============================================================
 * MessButton - click on a button in the message
 * ============================================================ */
static void MessButton(MyWindowPtr win, ControlHandle button,
                       long modifiers, short part) {
  (void)win; (void)button; (void)modifiers; (void)part;
}

/* ============================================================
 * MessMakeEditable - flip the pencil for a message
 * ============================================================ */
int MessMakeEditable(MyWindowPtr win, bool value) {
  MessHandle messH = Win2MessH(win);
  if (!value) {
    if (PeteIsDirty(TheBody)) {
      if (!PrefIsSet(PREF_EZ_SAVE)) {
        if (!SaveMessHi(win, false)) return userCanceledErr;
      } else if (!SaveMess(win)) return userCanceledErr;
    }
    ClearMessOpt(messH, OPT_WRITE);
    win->ro = (win->pte == TheBody);
    if (PeteIsValid_(TheBody))
      gtk_text_view_set_editable(GTK_TEXT_VIEW(TheBody), FALSE);
    win->isDirty = false;
    PeteCleanList(win->pte);
  } else {
    SetMessOpt(messH, OPT_WRITE);
    win->ro = false;
    if (PeteIsValid_(TheBody))
      gtk_text_view_set_editable(GTK_TEXT_VIEW(TheBody), TRUE);
  }
  return noErr;
}

/* ============================================================
 * MessGonnaShow - get ready to show a message window
 * ============================================================ */
int MessGonnaShow(MyWindowPtr win) {
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  short margin;

  win->didResize = MessDidResize;
  win->key = MessKey;
  win->update = MessUpdate;
  win->help = MessHelp;
  win->dontControl = true;
  win->zoomSize = MessZoomSize;
  win->label = SumColor(SumOf(messH));
  win->click = win->bgClick = MessClick;

  margin = GetRLong(COMP_TOP_MARGIN);
  if (!MessFlagIsSet(messH, FLAG_OUT) & MessOptIsSet(messH, OPT_RECEIPT) &&
      !GetPrefLong(PREF_RECEIPT))
    margin += 22 + 6;
  SetTopMargin(win, margin);

  if (messH->subPTE == NULL) {
    GtkWidget *subEntry = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(subEntry), GTK_WRAP_NONE);
    gtk_text_view_set_accepts_tab(GTK_TEXT_VIEW(subEntry), FALSE);
    unsigned char *subj = SumOf(messH)->subj;
    if (subj[0] != '\0') {
      GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(subEntry));
      gtk_text_buffer_set_text(buf, (const char *)subj, -1);
      gtk_text_buffer_set_modified(buf, FALSE);
    }
    messH->subPTE = subEntry;
  }

  PeteFocus(win, TheBody, true);
  SetBGColorsByPers(messH);
  PeteURLScan(win, TheBody);
  PeteCalcOn(TheBody);

  MessIBarUpdate(messH);
  CheckAddNotifyControls(win, messH);
  AddMessErrNote(messH);
  BeenThereDoneThat(messH->tocH, messH->sumNum);
  HiliteOddReply(messH);

  return noErr;
}

/* ============================================================ */
bool CheckAddNotifyControls(MyWindowPtr win, MessHandle messH) {
  bool result = false;
  if (!MessFlagIsSet(messH, FLAG_OUT) & MessOptIsSet(messH, OPT_RECEIPT) &&
      !FindControlByRefCon(win, NOTIFY_CNTL)) {
    switch (GetPrefLong(PREF_RECEIPT)) {
    case 2:
      messH->sound = NOTIFY_SOUND;
      GenerateReceipt(messH, MDN_DISPLAYED, MDN_DISPLAYED_LOCAL,
          MessOptIsSet(messH, OPT_AUTO_OPENED) ? MDN_AUTO_ACTION : MDN_MAN_ACTION,
          MDN_AUTO_SENT);
      ClearMessOpt(messH, OPT_RECEIPT);
      break;
    case 1: ClearMessOpt(messH, OPT_RECEIPT); break;
    case 0: AddNotifyControls(messH); result = true; break;
    }
  }
  return result;
}

static void AddNotifyControls(MessHandle messH) {
  messH->sound = NOTIFY_SOUND;
  PlaceNotifyControls(messH->win);
}

static void PlaceNotifyControls(MyWindowPtr win) { (void)win; }

void AddMessErrNote(MessHandle messH) {
  TOCType *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  mesgErrorHandle mesgErrH = tocH->sums[sumNum].mesgErrH;
  if (mesgErrH || tocH->sums[sumNum].state == MESG_ERR)
    messH->sound = NOTIFY_SOUND;
}

void PlaceMessErrNote(MessHandle messH) { (void)messH; }

/* ============================================================
 * MessIBarUpdate - update the icon bar state
 * ============================================================ */
void MessIBarUpdate(MessHandle messH) {
  MyWindowPtr win = messH->win;
  if (!win) return;

  bool on = MessOnPOPD(POPD_ID, messH);
  bool lmos = PrefIsSet(PREF_LMOS);
  bool fetch = on & MessOnPOPD(FETCH_ID, messH);
  bool del = on & ((fetch & !lmos) || MessOnPOPD(DELETE_ID, messH));
  ControlHandle blah;

  if ((blah = FindControlByRefCon(win, mcWrite_)))
    SetControlValue(blah, MessOptIsSet(messH, OPT_WRITE));
  if ((blah = FindControlByRefCon(win, mcBlahBlah_)))
    SetControlValue(blah, MessFlagIsSet(messH, FLAG_SHOW_ALL));
  if ((blah = FindControlByRefCon(win, mcFixed_)))
    SetControlValue(blah, MessFlagIsSet(messH, FLAG_FIXED_WIDTH));
  if ((blah = FindControlByRefCon(win, mcFetch_))) {
    SetControlValue(blah, fetch);
    if (!on || !MessFlagIsSet(messH, FLAG_SKIPPED))
      SetControlVisibility(blah, false, false);
    messH->hasFetchIcon = (blah != nil);
  }
  if ((blah = FindControlByRefCon(win, mcGetGraphics_))) {
    if (!DisplayGetGraphics(messH->win))
      SetControlVisibility(blah, false, false);
    else
      SetControlVisibility(blah, true, false);
  }
  if ((blah = FindControlByRefCon(win, mcTrash_))) {
    SetControlValue(blah, del);
    if (!on) SetControlVisibility(blah, false, false);
    messH->hasDelIcon = (blah != nil);
  }
}

/* ============================================================
 * ExportHTMLSum / ExportHTML - export as HTML and open in browser
 * ============================================================ */
int ExportHTMLSum(TOCType *tocH, short sumNum) {
  MessHandle messH = tocH->sums[sumNum].messH;
  MyWindowPtr win = nil;
  int err;

  if (tocH->imapTOC) EnsureMsgDownloaded(tocH, sumNum, false);
  if (!messH) {
    win = GetAMessage(tocH, sumNum, nil, nil, false);
    if (!win) return -108;
    messH = Win2MessH(win);
  }
  err = ExportHTML(messH);
  if (win) CloseMyWindow(win->window);
  return err;
}

int ExportHTML(MessHandle messH) {
  int err;
  void *cache;
  long len = 0, grandLen;

  err = CacheMessage(messH->tocH, messH->sumNum);
  if (err) return err;
  cache = SumOf(messH)->cache;
  if (!cache) return -1;

  grandLen = strlen((char *)cache);
  char *data = (char *)cache;
  char *htmlStart = NULL, *htmlEnd = NULL;
  int inHTML = 0;

  for (long off = 0; off < grandLen; off++) {
    if (data[off] == '<') {
      if (off + 5 < grandLen & strncasecmp(data + off + 1, "html", 4) == 0) {
        if (!inHTML) htmlStart = data + off;
        inHTML++;
      } else if (off + 6 < grandLen &&
                 strncasecmp(data + off + 1, "/html", 5) == 0) {
        inHTML--;
        if (!inHTML) {
          char *gt = memchr(data + off, '>', grandLen - off);
          htmlEnd = gt ? gt + 1 : data + grandLen;
          break;
        }
      }
    }
  }
  if (!htmlStart || !htmlEnd) return -1;
  len = htmlEnd - htmlStart;

  char tempPath[256];
  snprintf(tempPath, sizeof(tempPath), "/tmp/eudora_export_%u.html",
           SumOf(messH)->uidHash);
  FILE *fp = fopen(tempPath, "w");
  if (!fp) return -1;
  fwrite(htmlStart, 1, len, fp);
  fclose(fp);

  GError *error = NULL;
  GFile *file = g_file_new_for_path(tempPath);
  GAppInfo *app = g_file_query_default_handler(file, NULL, &error);
  if (app) {
    GList *files = g_list_append(NULL, file);
    if (!g_app_info_launch(app, files, NULL, &error)) {
      g_error_free(error); err = -1;
    }
    g_list_free(files);
    g_object_unref(app);
  } else {
    if (error) g_error_free(error);
    err = -1;
  }
  g_object_unref(file);
  if (err) unlink(tempPath);
  return err;
}

/* ============================================================ */
void HiliteOddReply(MessHandle messH) {
  if (!PeteIsValid_(TheBody)) return;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(TheBody));
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, "odd-reply");
  if (!tag)
    tag = gtk_text_buffer_create_tag(buf, "odd-reply", "foreground", "orange", NULL);

  HeadSpec hs, fromHS;
  unsigned char scratch[256];
  for (int r = 1; GetRString(scratch, ReplyStrn + r) && scratch[0]; r++) {
    if (EqualStrRes(scratch, HEADER_STRN + FROM_HEAD)) break;
    if (CompHeadFindStr(messH, (char *)scratch, &hs)) {
      if (CompHeadFindStr(messH,
              (char *)GetRString(scratch, HEADER_STRN + FROM_HEAD), &fromHS))
        continue;
      GtkTextIter iS, iE;
      gtk_text_buffer_get_iter_at_offset(buf, &iS, hs.start);
      gtk_text_buffer_get_iter_at_offset(buf, &iE, hs.stop);
      gtk_text_buffer_apply_tag(buf, tag, &iS, &iE);
      if (!MessOptIsSet(messH, OPT_WEIRD_REPLY))
        SetMessOpt(messH, OPT_WEIRD_REPLY);
    }
  }
}

static void MessUpdate(MyWindowPtr win) {
  if (!win || !win->window) return;
  if (gtk_widget_get_visible(win->window)) {
    MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
    if (messH->sound) messH->sound = 0;
  }
}

static void MessHelp(MyWindowPtr win, Point mouse) {
  (void)win; (void)mouse;
}

void SetMessTable(TOCType *tocH, short sumNum, short newId) {
  if (tocH->sums[sumNum].tableId != newId) {
    tocH->sums[sumNum].tableId = newId;
    TOCSetDirty(tocH, true);
    if (tocH->previewID == tocH->sums[sumNum].serialNum) tocH->previewID = 0;
    if (tocH->which != OUT && tocH->sums[sumNum].messH)
      ReopenMessage(tocH->sums[sumNum].messH->win);
  }
}

/* ============================================================
 * MessFocus - switch PTE focus, using gtk_widget_grab_focus
 * ============================================================ */
void MessFocus(MessHandle messH, PETEHandle pte) {
  MyWindowPtr win = messH->win;
  bool wasSub = (win->pte == messH->subPTE);
  PeteFocus(win, pte, true);
  win->ro = (win->pte == TheBody) & !MessOptIsSet(messH, OPT_WRITE);
  if (wasSub & win->pte != messH->subPTE)
    MessSaveSub(messH);
  if (PeteIsValid_(pte))
    gtk_widget_grab_focus(pte);
}

/* ============================================================ */
int MessSaveSub(MessHandle messH) {
  unsigned char newSubj[256];
  PeteSString_(newSubj, messH->subPTE);
  SetSubject(messH->tocH, messH->sumNum, newSubj);
  PETEMarkDocDirty(nil, messH->subPTE, false);
  if (PeteIsDirty(messH->win->pte))
    messH->win->isDirty = true;
  else
    messH->win->isDirty = false;
  return noErr;
}

/* ============================================================ */
void MessDidResize(MyWindowPtr win, Rect *oldContR) {
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  (void)oldContR;
  PlaceMessErrNote(messH);
  /* GTK handles layout via containers */
}

void MessSubjRect(MyWindowPtr win, Rect *r) {
  if (!win || !r) return;
  r->left = win->contR.left + 80;
  r->top = 6;
  r->right = win->contR.right - 80;
  r->bottom = r->top + 20;
}

void MessZoomSize(MyWindowPtr win, Rect *zoom) {
  long wi = MessWi(win);
  if (zoom->right - zoom->left > wi)
    zoom->right = zoom->left + wi;
}

short MessWi(MyWindowPtr win) {
  short wi = GetPrefLong(PREF_MWIDTH);
  if (!wi) wi = GetRLong(DEF_MWIDTH);
  wi *= 8; /* approximate character pitch in pixels */
  wi += 15;
  return wi;
}

bool GetPriorityRect(MyWindowPtr win, Rect *pr) {
  if (!win->topMargin) { memset(pr, 0, sizeof(Rect)); return false; }
  pr->left = 3; pr->top = 8; pr->bottom = 30; pr->right = 25;
  return true;
}

void DrawPriority(Rect *pr, short p) { (void)pr; (void)p; }

static bool GetBlahRect(MyWindowPtr win, short which, Rect *r) {
  if (!GetPriorityRect(win, r)) return false;
  short wi = r->right - r->left;
  r->left += (which + 1) * (wi + 3);
  r->right = r->left + wi;
  return true;
}

void MessCursor(Point mouse) { (void)mouse; }

bool MessagePosition(bool save, MyWindowPtr win) {
  (void)save; (void)win; return true;
}

bool MessApp1(MyWindowPtr win, void *event) {
  MessHandle messH = Win2MessH(win);
  PeteEdit(win, messH->bodyPTE, peeEvent, event);
  return true;
}

/* ============================================================
 * IncrementQuoteLevel - adjust quote level via GtkTextBuffer
 * ============================================================ */
int IncrementQuoteLevel(PETEHandle pte, long startSel, long endSel,
                        short increment) {
  if (!PeteIsValid_(pte)) return -1;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));

  GtkTextIter selS, selE;
  if (startSel < 0 || endSel < 0) {
    if (!gtk_text_buffer_get_selection_bounds(buf, &selS, &selE)) return 0;
  } else {
    gtk_text_buffer_get_iter_at_offset(buf, &selS, startSel);
    gtk_text_buffer_get_iter_at_offset(buf, &selE, endSel);
  }

  if (!gtk_text_iter_starts_line(&selS))
    gtk_text_iter_set_line_offset(&selS, 0);
  if (!gtk_text_iter_ends_line(&selE))
    gtk_text_iter_forward_to_line_end(&selE);

  GtkTextIter lineStart = selS;
  while (gtk_text_iter_compare(&lineStart, &selE) < 0) {
    if (increment > 0) {
      for (int i = 0; i < increment; i++)
        gtk_text_buffer_insert(buf, &lineStart, "> ", 2);
    } else if (increment < 0) {
      for (int i = 0; i < -increment; i++) {
        gunichar ch = gtk_text_iter_get_char(&lineStart);
        if (ch == '>') {
          GtkTextIter delEnd = lineStart;
          gtk_text_iter_forward_char(&delEnd);
          if (gtk_text_iter_get_char(&delEnd) == ' ')
            gtk_text_iter_forward_char(&delEnd);
          gtk_text_buffer_delete(buf, &lineStart, &delEnd);
        } else break;
      }
    }
    if (!gtk_text_iter_forward_line(&lineStart)) break;
  }
  gtk_text_buffer_set_modified(buf, TRUE);
  return 0;
}

/* ============================================================
 * AddXlateTables / Menu2TableId — not applicable in GTK port
 * ============================================================ */
short AddXlateTables(bool isOut, short nowId, bool ph, void **pmh) {
  (void)isOut; (void)nowId; (void)ph; (void)pmh;
  return 0;
}

bool Menu2TableId(TOCType *tocH, void **pmh, short item, short *tableId) {
  (void)tocH; (void)pmh; (void)item;
  if (tableId) *tableId = 0;
  return false;
}

/* ============================================================ */
void PetePaneDraw(void *cntl, short part) { (void)cntl; (void)part; }

short GetMesgErrorsHeight(MyWindowPtr win) {
  (void)win;
  return MESG_ERR_WIDTH;
}

bool GetMesgErrorsRect(MyWindowPtr win, Rect *r) {
  r->left = win->contR.left + 3;
  r->right = r->left + MESG_ERR_WIDTH;
  r->bottom = win->topMargin - 2;
  r->top = r->bottom - MESG_ERR_WIDTH + 6;
  return true;
}

RgnHandle MessBuildDragRgn(MessHandle messH) {
  (void)messH;
  return nil;
}
