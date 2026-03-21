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
 *   - MacmbxTOC * direct pointer (no void *indirection on TOC)
 *   - GtkWidget * / bodyPTE / subPTE = GtkWidget* (GtkTextView via gEditCtrl)
 *   - ControlHandle = void* (GtkWidget* buttons/widgets)
 *   - QuickDraw drawing → GTK4 widgets / CSS styling
 *   - Mac window manager → GTK4 window management
 *   - FSSpec → portable struct with path field
 */

#include "messact.h"
#include "../gEditCtrl/geditctrl.h"
#include "../gEditCtrl/gedit-document.h"
#include "crispy_richtext.h"

#include "Globals.h"
#include "MyRes.h"
#include "StrnDefs.h"
#include "comp.h"
#include "features.h"
/* filtrun.h removed — macmbx_filter handles filters */
#include "find.h"
#include "gtk_menus.h"
/* imapdownload.h removed — crispy_imap handles IMAP */
/* junk.h removed — macmbx_junk handles junk */
#include "macmbx.h"
#include "gtk_mailbox.h"
#include "mailbox.h"
#include "prefdefs.h"
#include "gtk_dialogs.h"
#include "message.h"
#include "mydefs.h"
#include "nickmng.h"
/* pop.h removed — crispy_pop3 handles POP */
#include "print.h"
#include "schizo.h"
#include "searchwin.h"
/* sendmail.h removed — crispy handles SMTP */
#include "toc.h"
#include "util.h"
#include "utl.h"
#include "StringUtil.h"
#include "StringDefs.h"
#include "fileutil.h"

/* Forward declarations for functions not in available headers */
extern bool IsMailboxChoice(short menu, short item);
extern bool GetTransferParams(short menu, short item, char * spec, void *p);
extern bool SameSpec(char * a, char * b);
extern void MoveMessage(MacmbxTOC *tocH, short sumNum, char * spec, bool copy);
extern void AddXfUndo(MacmbxTOC *fromTocH, MacmbxTOC *toTocH, short sumNum);
extern void MakeMessTitle(unsigned char *title, MacmbxTOC *tocH, short sumNum, bool full);
extern void InvalTopMargin(MyWindowPtr win);
extern void MakeMessFileName(MacmbxTOC *tocH, short sumNum, unsigned char *name);
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
extern int SaveTextAsMessage(void *extras, void *text, MacmbxTOC *tocH, long *fromLen);
extern void ReplyDefaults(short modifiers, bool *all, bool *self, bool *quote);
extern void DoReplyMessage(MyWindowPtr win, bool all, bool self, bool quote,
                           bool b1, short item, bool b2, bool b3, bool b4);
extern void DoRedistributeMessage(MyWindowPtr win, void *addr, bool turbo,
                                  bool b1, bool b2);
extern void DoForwardMessage(MyWindowPtr win, void *addr, bool b);
extern void DoSalvageMessage(MyWindowPtr win, bool b);
extern void DeleteMessage(MacmbxTOC *tocH, short sumNum, bool nuke);
/* GetTrashTOC is a macro in toc.h using macmbx */
extern void ServerMenuChoice(MacmbxTOC *tocH, short sumNum, short item, bool shift);
extern void SetPriority(MacmbxTOC *tocH, short sumNum, short priority);
extern short AdjustSpecialMenuSelection(short item);
extern void MakeNickFromSelection(MyWindowPtr win);
extern void MakeMessNick(MyWindowPtr win, short modifiers);
/* DoMakeFilter — create a filter rule from the current message using macmbx */
static void on_make_filter_create(GtkWidget *dialog);

void DoMakeFilter(MyWindowPtr win) {
  if (!win) return;
  MessHandle messH = Win2MessH(win);
  if (!messH) return;
  MacmbxTOC *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  if (!tocH || sumNum < 0 || sumNum >= tocH->count) return;

  /* Read headers from macmbx */
  char *from = macmbx_read_header_field(tocH, sumNum, "From");
  char *subject = macmbx_read_header_field(tocH, sumNum, "Subject");
  char *to = macmbx_read_header_field(tocH, sumNum, "To");

  /* Build a dialog */
  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Make Filter");
  gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 350);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(vbox, 16);
  gtk_widget_set_margin_end(vbox, 16);
  gtk_widget_set_margin_top(vbox, 16);
  gtk_widget_set_margin_bottom(vbox, 16);

  /* Match field selector */
  gtk_box_append(GTK_BOX(vbox), gtk_label_new("Match on:"));
  GtkWidget *match_combo = gtk_drop_down_new_from_strings(
    (const char *[]){"From", "Subject", "To", "Any Header", NULL});
  gtk_box_append(GTK_BOX(vbox), match_combo);

  /* Match value (pre-filled) */
  gtk_box_append(GTK_BOX(vbox), gtk_label_new("Contains:"));
  GtkWidget *match_entry = gtk_entry_new();
  if (from) gtk_editable_set_text(GTK_EDITABLE(match_entry), from);
  gtk_box_append(GTK_BOX(vbox), match_entry);

  /* Action selector */
  gtk_box_append(GTK_BOX(vbox), gtk_label_new("Action:"));
  GtkWidget *action_combo = gtk_drop_down_new_from_strings(
    (const char *[]){"Move to mailbox", "Set status", "Set priority", "Set label",
                      "Mark as junk", "Delete", "None", NULL});
  gtk_box_append(GTK_BOX(vbox), action_combo);

  /* Destination (for Move To) */
  gtk_box_append(GTK_BOX(vbox), gtk_label_new("Destination mailbox:"));
  GtkWidget *dest_entry = gtk_entry_new();
  gtk_box_append(GTK_BOX(vbox), dest_entry);

  /* Buttons */
  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
  GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
  GtkWidget *create_btn = gtk_button_new_with_label("Create");
  gtk_widget_add_css_class(create_btn, "suggested-action");
  gtk_box_append(GTK_BOX(btn_box), cancel_btn);
  gtk_box_append(GTK_BOX(btn_box), create_btn);
  gtk_box_append(GTK_BOX(vbox), btn_box);

  /* Store refs for callback */
  g_object_set_data(G_OBJECT(dialog), "match-combo", match_combo);
  g_object_set_data(G_OBJECT(dialog), "match-entry", match_entry);
  g_object_set_data(G_OBJECT(dialog), "action-combo", action_combo);
  g_object_set_data(G_OBJECT(dialog), "dest-entry", dest_entry);

  /* Cancel just closes */
  g_signal_connect_swapped(cancel_btn, "clicked", G_CALLBACK(gtk_window_close), dialog);

  /* Create button: build the rule and save */
  g_signal_connect_swapped(create_btn, "clicked", G_CALLBACK(on_make_filter_create), dialog);

  gtk_window_set_child(GTK_WINDOW(dialog), vbox);
  gtk_window_present(GTK_WINDOW(dialog));

  free(from); free(subject); free(to);
}

static void on_make_filter_create(GtkWidget *dialog) {
  GtkWidget *match_combo = g_object_get_data(G_OBJECT(dialog), "match-combo");
  GtkWidget *match_entry = g_object_get_data(G_OBJECT(dialog), "match-entry");
  GtkWidget *action_combo = g_object_get_data(G_OBJECT(dialog), "action-combo");
  GtkWidget *dest_entry = g_object_get_data(G_OBJECT(dialog), "dest-entry");

  guint match_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(match_combo));
  const char *match_val = gtk_editable_get_text(GTK_EDITABLE(match_entry));
  guint action_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(action_combo));
  const char *dest = gtk_editable_get_text(GTK_EDITABLE(dest_entry));

  /* Build the filter rule */
  MacmbxRule rule;
  memset(&rule, 0, sizeof(rule));
  snprintf(rule.name, sizeof(rule.name), "Filter: %s", match_val);
  rule.when = MACMBX_WHEN_INCOMING;
  rule.condition_count = 1;

  /* Match field */
  const char *headers[] = {"From:", "Subject:", "To:", "Any:"};
  snprintf(rule.conditions[0].header, sizeof(rule.conditions[0].header),
           "%s", headers[match_idx < 4 ? match_idx : 0]);
  rule.conditions[0].verb = MACMBX_VERB_CONTAINS;
  snprintf(rule.conditions[0].value, sizeof(rule.conditions[0].value),
           "%s", match_val);

  /* Action */
  rule.action_count = 1;
  switch (action_idx) {
    case 0: /* Move to mailbox */
      rule.actions[0].type = MACMBX_ACT_TRANSFER;
      if (dest && dest[0])
        snprintf(rule.actions[0].str_value, sizeof(rule.actions[0].str_value), "%s", dest);
      break;
    case 1: rule.actions[0].type = MACMBX_ACT_STATUS; rule.actions[0].int_value = MACMBX_READ; break;
    case 2: rule.actions[0].type = MACMBX_ACT_PRIORITY; rule.actions[0].int_value = 1; break;
    case 3: rule.actions[0].type = MACMBX_ACT_LABEL; rule.actions[0].int_value = 1; break;
    case 4: rule.actions[0].type = MACMBX_ACT_JUNK; break;
    case 5: rule.actions[0].type = MACMBX_ACT_DELETE; break;
    default: rule.actions[0].type = MACMBX_ACT_NONE; break;
  }

  /* Load filters, add rule, save */
  MacmbxStore *store = gtk_mailbox_get_store();
  if (store) {
    char filt_path[PATH_MAX];
    snprintf(filt_path, sizeof(filt_path), "%s/../Filters", store->root_path);
    MacmbxFilterSet *fs = macmbx_filter_load(filt_path);
    if (!fs) fs = macmbx_filter_new();
    macmbx_filter_add_rule(fs, &rule);
    if (filt_path[0]) {
      snprintf(fs->path, sizeof(fs->path), "%s", filt_path);
      macmbx_filter_save(fs);
    }
    macmbx_filter_free(fs);
  }

  gtk_window_close(GTK_WINDOW(dialog));
}
extern void SetState(MacmbxTOC *tocH, int sumNum, int state);
extern short Item2Status(short item);
extern void SelectBoxRange(MacmbxTOC *tocH, int start, int end, bool cmd, int eStart, int eEnd);
extern void BoxCenterSelection(MyWindowPtr win);
extern void BeenThereDoneThat(MacmbxTOC *tocH, short sumNum);
extern void *MenuItem2Handle(short menu, short item);
extern void SetTopMargin(MyWindowPtr win, short margin);
extern void SetBGColorsByPers(MessHandle messH);
extern int CacheMessage(MacmbxTOC *tocH, short sumNum);
extern void GenerateReceipt(MessHandle messH, int disp, int dispLocal, int action, int sent);
extern bool DisplayGetGraphics(MyWindowPtr win);
extern int Box2Path(const char *boxPath, unsigned char *path);
extern int InsertCommaIfNeedBe(GtkWidget *pte, HeadSpec *hs);
extern short SubjCompare(unsigned char *s1, unsigned char *s2);
extern MacmbxTOC *GetRealTOC(MacmbxTOC *tocH, short sumNum, short *realSum);
extern bool FindRealSummary(MacmbxTOC *tocH, long serialNum, short *sumNum);
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
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) return 0;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  return gtk_text_buffer_get_char_count(buf);
}

static void PeteSString_(unsigned char *pStr, GtkWidget *pte) {
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) { pStr[0] = 0; return; }
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
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) return -1;
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
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) return;
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
  gtk_text_buffer_set_modified(buf, TRUE);
}

/* ============================================================
 * MessClose - close a message window
 * ============================================================ */
bool MessClose(MyWindowPtr win) {
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  MacmbxTOC *tocH = messH->tocH;
  int sumNum = messH->sumNum;

  if (!GrowZoning && TheBody && messH->subPTE &&
      win->pte == messH->subPTE)
    MessFocus(messH, TheBody);

  if (geditctrl_is_dirty(messH->subPTE))
    MessSaveSub(messH);

  if (!GrowZoning)
    win->isDirty = win->isDirty || geditctrl_is_dirty(TheBody);

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
  tocH->msgs[sumNum].messH = nil;

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
bool TransferMenuChoice(short menu, short item, MacmbxTOC *tocH, short sumNum,
                        long modifiers, bool fcc) {
  FSSpec spec, toSpec;
  short function = REAL_BIG;

  if (!IsMailboxChoice(menu, item)) return false;

  if (menu == TRANSFER_MENU)
    function = TRANSFER;
  else if (false)
    function = (menu - BOX_MENU_START) / MAX_BOX_LEVELS;

  if (function == TRANSFER) {
    GetMailboxSpec(tocH, sumNum, spec);
    if (GetTransferParams(menu, item, &toSpec, nil) &&
        !SameSpec(&spec, &toSpec)) {
      if (HasFeature(featureFcc) && fcc)
        Fcc(tocH->msgs[sumNum].messH, &toSpec);
      else if (sumNum >= 0) {
        if (!(modifiers & GDK_ALT_MASK)) {
          if (!tocH->virtualTOC)
            AddXfUndo(tocH, macmbx_toc_open(&toSpec), sumNum);
          EzOpen(tocH, sumNum, 0, modifiers, true, true);
        }
        MoveMessage(tocH, sumNum, &toSpec, (modifiers & GDK_ALT_MASK) != 0);
      } else
        MoveSelectedMessages(tocH, &toSpec, (modifiers & GDK_ALT_MASK) != 0);
    }
    return true;
  }
  return false;
}

/* ============================================================
 * SetSubject - set the subject in a message summary
 * ============================================================ */
void SetSubject(MacmbxTOC *tocH, short sumNum, char *sub) {
  unsigned char oldSubj[64];
  unsigned char title[256];
  MessHandle messH = tocH->msgs[sumNum].messH;

  g_strlcpy((char *)oldSubj, (char *)tocH->msgs[sumNum].subject, 64);
  if (!(strcmp((const char *)(oldSubj), (const char *)(sub)) == 0)) {
    g_strlcpy((char *)tocH->msgs[sumNum].subject, (char *)sub, 60);
    InvalSum(tocH, sumNum);
    TOCSetDirty(tocH, true);

    if (messH) {
      MakeMessTitle(title, tocH, sumNum, true);
      if (messH->win && messH->win->window) {
        char cTitle[257];
        g_strlcpy(cTitle, (char *)title, sizeof(cTitle));
        gtk_window_set_title(GTK_WINDOW(messH->win->window), cTitle);
      }
      if (messH->subPTE && (messH->subPTE != NULL && GTK_IS_WIDGET(messH->subPTE))) {
        GtkTextBuffer *buf =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(messH->subPTE));
        char cSub[256];
        int subLen = sub[0];
        memcpy(cSub, sub + 1, subLen);
        cSub[subLen] = '\0';
        gtk_text_buffer_set_text(buf, cSub, -1);
      }
    }
    SearchUpdateSum(tocH, sumNum, tocH, tocH->msgs[sumNum].serial_num,
                    false, false);
  }
}

/* ============================================================
 * SetSender - set the sender in a message summary
 * ============================================================ */
void SetSender(MacmbxTOC *tocH, short sumNum, char *sender) {
  unsigned char oldSender[64];
  unsigned char title[256];
  MessHandle messH = tocH->msgs[sumNum].messH;

  g_strlcpy((char *)oldSender, (char *)tocH->msgs[sumNum].from, 64);
  if (!(strcmp((const char *)(oldSender), (const char *)(sender)) == 0)) {
    g_strlcpy((char *)tocH->msgs[sumNum].from, (char *)sender, 48);
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
void SetFlag(MacmbxTOC *tocH, short sumNum, long flag, bool on) {
  if (on)
    tocH->msgs[sumNum].flags |= flag;
  else
    tocH->msgs[sumNum].flags &= ~flag;
  if (tocH->msgs[sumNum].messH)
    InvalTopMargin(((MessHandle)tocH->msgs[sumNum].messH)->win);
  TOCSetDirty(tocH, true);
}

/* ============================================================
 * SetOpt - set one of the message options
 * ============================================================ */
void SetOpt(MacmbxTOC *tocH, short sumNum, long flag, bool on) {
  if (on)
    tocH->msgs[sumNum].opts |= flag;
  else
    tocH->msgs[sumNum].opts &= ~flag;
  TOCSetDirty(tocH, true);
}

/* ============================================================
 * AttIsSelected - check/open/color selected attachments
 * ============================================================ */
bool AttIsSelected(MyWindowPtr win, GtkWidget * pte, long startWith,
                   long endWith, short what, long *aStart, long *aEnd) {
  char *text = NULL;
  long selStart, selEnd;
  bool found = false;
  bool colorThem = 0 != (what & attColor);
  bool selectThem = 0 != (what & attSelect);
  bool openThem = 0 != (what & attOpen);
  bool finderSelect = 0 != (what & attFinder);
  bool printThem = 0 != (what & attPrint);

  if (!((pte) != NULL && GTK_IS_WIDGET(pte)) && win) pte = win->pte;
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) return false;

  text = (void *)geditctrl_get_text(pte);
  selStart = geditctrl_get_caret_offset(pte);
  selEnd = selStart; /* simplified — full selection tracking via GTK */
  if (!text)
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
        geditctrl_set_dirty(pte, TRUE);
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

  /* macmbx downloads full messages including attachments during check.
   * If the attachment file doesn't exist locally, it may need to be
   * extracted from the mbox via macmbx. For now, just try to open it. */

  if (err == 0 && !g_file_test(spec, G_FILE_TEST_EXISTS)) {
    /* Attachment file not found — may not have been decoded yet.
     * TODO: extract attachment from mbox via macmbx_read_message + MIME parse */
    g_warning("Attachment not found: %s", spec);
    err = -1;
  }

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
int AttLine2Spec(char *line, char * spec, bool wantToOpen) {
  long fid = 0;
  int lineLen = line[0];

  /* Trim whitespace */
  while (lineLen > 0 && (line[lineLen] == ' ' || line[lineLen] == '\t'))
    lineLen--;
  line[0] = lineLen;

  if (lineLen < 10) return ENOENT;
  if (!isalnum(line[1]) && line[1] != '/') return ENOENT;

  /* Pick out file ID from trailing [xxxxxxxx] or (xxxxxxxx) */
  if (line[lineLen] != ']' && line[lineLen] != ')') return ENOENT;
  if (line[lineLen - 9] != '[' && line[lineLen - 9] != '(') return ENOENT;

  unsigned char *spot = line + lineLen - 8;
  for (int dig = 8; dig > 0; dig--, spot++) {
    if (*spot >= '0' && *spot <= '9')
      fid = (fid << 4) | (*spot - '0');
    else if (*spot >= 'a' && *spot <= 'f')
      fid = (fid << 4) | (10 + *spot - 'a');
    else if (*spot >= 'A' && *spot <= 'F')
      fid = (fid << 4) | (10 + *spot - 'A');
    else return ENOENT;
  }

  /* Find path separators (: for Mac, / for Unix) */
  char *sep1 = NULL, *sep2 = NULL;
  for (char *p = line + 1; p < line + lineLen; p++) {
    if (*p == ':' || *p == '/') {
      if (!sep1) sep1 = p;
      else if (!sep2) { sep2 = p; break; }
    }
  }
  if (!sep1 || !sep2) return ENOENT;

  if (spec) {
    long nameStart = (sep2 - line);
    long nameEnd = lineLen - 10;
    if (nameEnd <= nameStart) return ENOENT;
    long nameLen = nameEnd - nameStart;
    if (nameLen < 1 || nameLen > 255) return ENOENT;

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
          if (wantToOpen) WarnUser(ATTACH_GONE, ENOENT);
          return ENOENT;
        }
      } else {
        if (wantToOpen) WarnUser(ATTACH_GONE, ENOENT);
        return ENOENT;
      }
    }
    return 0;
  } else {
    long sepDist = sep2 - sep1;
    return (sepDist > 1 & sepDist < 33) ? 0 : ENOENT;
  }
}

/* ============================================================
 * RelLine2Spec - get an FSSpec from a "related" line
 * ============================================================ */
int RelLine2Spec(char *line, char * spec, uLong *cid,
                 uLong *relURL, uLong *absURL) {
  char cLine[256];
  int len = line[0];
  if (len > 254) len = 254;
  memcpy(cLine, line + 1, len);
  cLine[len] = '\0';

  if (strncasecmp(cLine, "related:", 8) != 0) return ENOENT;

  char *saveptr;
  char *token = strtok_r(cLine, ":", &saveptr);
  if (!token) return ENOENT;
  token = strtok_r(NULL, ":", &saveptr); /* space */
  if (!token) return ENOENT;
  token = strtok_r(NULL, ":", &saveptr); /* volume */
  if (!token) return ENOENT;
  char *filename = strtok_r(NULL, ":", &saveptr);
  if (!filename) return ENOENT;
  char *fidStr = strtok_r(NULL, ":", &saveptr);
  if (!fidStr || strlen(fidStr) != 8) return ENOENT;
  char *cidStr = strtok_r(NULL, ":", &saveptr);
  if (!cidStr || strlen(cidStr) != 8) return ENOENT;
  if (cid) *cid = strtoul(cidStr, NULL, 16);
  char *relStr = strtok_r(NULL, ":", &saveptr);
  if (!relStr || strlen(relStr) != 8) return ENOENT;
  if (relURL) *relURL = strtoul(relStr, NULL, 16);
  char *absStr = strtok_r(NULL, ":", &saveptr);
  if (!absStr || strlen(absStr) != 8) return ENOENT;
  if (absURL) *absURL = strtoul(absStr, NULL, 16);

  if (spec) {
    spec_set_name(spec, filename);
    char partsFolder[512];
    GetPartsFolder(partsFolder, sizeof(partsFolder));
    snprintf(spec, sizeof(spec), "%s/%s",
             partsFolder, spec_name(spec));
    if (access(spec, F_OK) != 0) return ENOENT;
  }
  return 0;
}

/* ============================================================
 * SaveMess - save a message into its own mailbox
 * ============================================================ */
bool SaveMess(MyWindowPtr win) {
  MessHandle messH = Win2MessH(win);
  MacmbxTOC *tocH = messH->tocH;
  long fromLen;
  void *text = MessText(messH);
  HeadSpec hSpec;
  Accumulator enriched;
  int err = 0;
  bool richSave = false;
  unsigned char title[256];
  bool blahBlah = MessFlagIsSet(messH, FLAG_SHOW_ALL);

  if (!blahBlah) {
    /* Detect styled content from gEditCtrl */
    {
      geditDocument *_doc = geditctrl_get_document(TheBody);
      GList *_runs = _doc ? gedit_document_get_style_runs(_doc) : NULL;
      bool _has_styles = false;
      for (GList *_l = _runs; _l; _l = _l->next) {
        geditStyleRun *_r = (geditStyleRun *)_l->data;
        if (_r->bold || _r->italic || _r->underline || _r->font_size ||
            _r->font_family || _r->link_url ||
            _r->color.red > 0.01 || _r->color.green > 0.01 || _r->color.blue > 0.01)
        { _has_styles = true; break; }
      }
      if (_has_styles) {
        if (!PrefIsSet(PREF_SEND_ENRICHED_NEW) || MessOptIsSet(messH, OPT_HTML))
        { SetMessOpt(messH, OPT_HTML); ClearMessFlag(messH, FLAG_RICH); }
        else { SetMessFlag(messH, FLAG_RICH); ClearMessOpt(messH, OPT_HTML); }
      } else {
        ClearMessFlag(messH, FLAG_RICH);
        ClearMessOpt(messH, OPT_HTML);
      }
    }
    memset(&enriched, 0, sizeof(enriched));
    if (MessIsRich(messH)) {
      if (!(err = AccuInit(&enriched))) {
        CompHeadFind(messH, 0, &hSpec);
        if (!messH->extras.offset ||
            !(err = AccuAddHandle(&enriched, messH->extras.data)))
          if (!(err = AccuAddFromHandle(&enriched, text, 0, hSpec.value))) {
            /* Extract styled text from gEditCtrl as HTML or enriched */
            {
              geditDocument *_edoc = geditctrl_get_document(TheBody);
              gchar *_html = _edoc ? gedit_document_get_markup(_edoc, 0, -1) : NULL;
              if (_html) {
                if (MessOptIsSet(messH, OPT_HTML)) {
                  /* Wrap in full HTML document */
                  char *full = g_strdup_printf(
                    "<html><head><title>%s</title></head><body>%s</body></html>",
                    SumOf(messH)->subject, _html);
                  err = AccuAddPtr(&enriched, full, strlen(full));
                  g_free(full);
                } else {
                  char *_enr = crispy_html_to_enriched(_html, -1);
                  if (_enr) {
                    err = AccuAddPtr(&enriched, _enr, strlen(_enr));
                    free(_enr);
                  }
                }
                g_free(_html);
              } else {
                err = -1;
              }
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

  MacmbxMsgSum *oldSum = &tocH->msgs[messH->sumNum];
  MacmbxMsgSum *newSum = &tocH->msgs[tocH->count - 1];

  tocH->usedK -= oldSum->length / 1024;
  tocH->updateBoxSizes = true;
  oldSum->offset = newSum->offset;
  oldSum->length = newSum->length;
  oldSum->body_offset = newSum->body_offset;
  if (newSum->seconds & !(oldSum->flags & FLAG_OUT)) {
    oldSum->seconds = newSum->seconds;
    oldSum->orig_zone = newSum->orig_zone;
  }

  if (tocH->virtualTOC && (oldSum->opts && OPT_IMAP_SENT)) {
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
  ((void)0);
  geditctrl_clean(win->pte);
  win->isDirty = false;
  free(SumOf(messH)->cache);
  SetMessOpt(messH, OPT_EDITED);
  free(messH->etlFiles);
  if (tocH->previewID == SumOf(messH)->serial_num)
    tocH->previewID = 0;
  return true;
}

/* ============================================================
 * ShowMessageSeparator
 * ============================================================ */
void ShowMessageSeparator(GtkWidget *pte, bool center) {
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) return;
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
  MacmbxTOC *tocH = messH->tocH;
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
      if ((modifiers & GDK_SHIFT_MASK) &&
          AttIsSelected(win, win->pte, -1, -1, attOpen + attPrint, nil, nil))
        ;
      else
        PrintOneMessage(messH->win, (modifiers & GDK_SHIFT_MASK) != 0,
                        item == FILE_PRINT_ONE_ITEM);
      result = true;
      break;
    case FILE_SAVE_ITEM:
      if (win->isDirty) {
        if (messH->subPTE && geditctrl_is_dirty(messH->subPTE))
          MessSaveSub(messH);
        if (messH->bodyPTE && geditctrl_is_dirty(messH->bodyPTE))
          SaveMess(win);
        geditctrl_clean(win->pte);
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
                     (modifiers & GDK_SHIFT_MASK) != 0);
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
          PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers, GDK_ALT_MASK),
          !(modifiers & GDK_SHIFT_MASK), true);
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
    case MESSAGE_NOTJUNK_ITEM: {
      /* Mark as junk/not-junk via macmbx */
      MacmbxTOC *mtoc = macmbx_toc_open(tocH->mbox_path);
      if (mtoc && sumNum < mtoc->count) {
        MacmbxJunkConfig jcfg;
        macmbx_junk_config_init(&jcfg);
        macmbx_junk_mark(&jcfg, mtoc, sumNum,
                          item == MESSAGE_JUNK_ITEM,
                          gtk_mailbox_get_store());
        macmbx_toc_save(mtoc);
      }
      break;
    }
    case MESSAGE_DELETE_ITEM:
      EzOpen(messH->openedFromTocH, -1, messH->openedFromSerialNum,
             modifiers, true, true);
      NoSaves = !MessOptIsSet(Win2MessH(win), OPT_WRITE);
      if (CloseMyWindow(win->window)) {
        AddXfUndo(tocH, GetTrashTOC(), sumNum);
        DeleteMessage(tocH, sumNum,
            (modifiers & (GDK_ALT_MASK | GDK_SHIFT_MASK)) == (GDK_ALT_MASK | GDK_SHIFT_MASK));
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
    turbo = PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers, GDK_ALT_MASK);
    if (turbo) EzOpen(tocH, sumNum, 0, modifiers, true, true);
    addr = MenuItem2Handle(menu, item);
    DoRedistributeMessage(win, addr, turbo, !(modifiers & GDK_SHIFT_MASK), true);
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
      if (modifiers & GDK_SHIFT_MASK)
        MakeNickFromSelection(win);
      else
        MakeMessNick(win, modifiers);
      result = true;
      break;
    case SPECIAL_MAKEFILTER_ITEM:
      DoMakeFilter(win);
      break;
    case SPECIAL_FILTER_ITEM: {
      /* Run filters on this message via macmbx */
      MacmbxTOC *mtoc = macmbx_toc_open(tocH->mbox_path);
      MacmbxStore *store = gtk_mailbox_get_store();
      if (mtoc && sumNum < mtoc->count && store) {
        char filt_path[1024];
        snprintf(filt_path, sizeof(filt_path), "%s/../Filters",
                 store->root_path);
        MacmbxFilterSet *fs = macmbx_filter_load(filt_path);
        if (fs) {
          macmbx_filter_apply(fs, mtoc, sumNum, store, NULL, NULL);
          macmbx_toc_save(mtoc);
          macmbx_filter_free(fs);
        }
      }
      result = true;
      break;
    }
    }
    break;

  case PRIOR_HIER_MENU:
    SetPriority(tocH, sumNum, NewPrior(item, tocH->msgs[sumNum].priority));
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
    start = hs.stop; /* save for undo range */
    InsertCommaIfNeedBe(TheBody, &hs);
    CompHeadAppendStr(TheBody, &hs, scratch);
    CompHeadFind(messH, BCC_HEAD, &hs);
    geditctrl_select_range(TheBody, hs.stop, hs.stop);
    geditctrl_set_dirty(TheBody, TRUE);
    ClearMessFlag(messH, FLAG_KEEP_COPY);
    /* CompIBarUpdate — GTK toolbar handles compose UI */
  }
}

/* ============================================================
 * EzOpenFind - find next candidate for EzOpen
 * ============================================================ */
short EzOpenFind(MacmbxTOC *tocH, short origSum) {
  long ez = GetPrefLong(PREF_NO_EZ_OPEN);
  if (origSum < tocH->count - 1) {
    if (ez == 1) return origSum + 1;
    if (tocH->msgs[origSum + 1].state == UNREAD) {
      if (ez == 3 || ez == 2) return origSum + 1;
      if (ez == 4) {
        unsigned char s1[64], s2[64];
        g_strlcpy((char *)s1, (char *)tocH->msgs[origSum].subject, 64);
        g_strlcpy((char *)s2, (char *)tocH->msgs[origSum + 1].subject, 64);
        if (!SubjCompare(s1, s2)) return origSum + 1;
      }
    }
  }
  if (ez == 2)
    for (short s = origSum + 2; s < tocH->count; s++)
      if (tocH->msgs[s].state == UNREAD) return s;
  return -1;
}

/* ============================================================
 * EzOpen - easy open
 * ============================================================ */
void EzOpen(MacmbxTOC *tocH, short sumNum, uLong serialNum, long modifiers,
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
    MacmbxTOC *realTOC;
    short realSum;
    realTOC = GetRealTOC(tocH, sumNum, &realSum);
    if (realTOC->msgs[realSum].messH)
      serialNum = realTOC->msgs[realSum].messH ? ((MessHandle)realTOC->msgs[realSum].messH)->ezOpenSerialNum : 0;
    else if (tocH->previewPTE && tocH->previewID) {
      preview = true;
      serialNum = tocH->ezOpenSerialNum;
    }
  } else
    sumNum = FindSumBySerialNum(tocH, serialNum);

  if (serialNum & (newSumNum = FindSumBySerialNum(tocH, serialNum)) >= 0 &&
      tocH->msgs[newSumNum].state == UNREAD)
    ; /* found */
  else {
    if (!hideFront) newSumNum = EzOpenFind(tocH, sumNum - 1);
    else newSumNum = EzOpenFind(tocH, sumNum);
  }

  if (newSumNum >= 0) {
    ez = GetPrefLong(PREF_NO_EZ_OPEN);
    if (modifiers & GDK_SHIFT_MASK) ez = 0;
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

  text = (void *)geditctrl_get_text(TheBody);
  CompHeadFind(messH, 0, &hs);

  if (!exclHead & isOut & SumOf(messH)->seconds) {
    unsigned char scratch[64];
    /* Format date header — was BuildDateHeader from sendmail.h */
    {
      time_t t = (time_t)SumOf(messH)->seconds;
      struct tm *tm = localtime(&t);
      if (tm) strftime((char*)scratch, sizeof(scratch), "Date: %a, %d %b %Y %H:%M:%S %z\r\n", tm);
      else scratch[0] = '\0';
    }
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

int UnwrapSave(char *text, long length, long offset, short refN) {
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
  /* TODO: Rewrite to receive GDK keyval + modifiers when GTK key dispatch calls this */
  (void)win; (void)eventPtr;
  return false;
#if 0 /* Dead until GTK key dispatch is connected */
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  MacmbxTOC *tocH = messH->tocH;
  long uLetter = 0; /* was: UnadornMessage(event) & charCodeMask */
  bool bodyEdit = !win->ro & win->pte == TheBody;
  bool shift = false;

  if (leftArrowChar <= uLetter & uLetter <= downArrowChar &&
      IsArrowSwitch(event->modifiers)) {
    NextMess(tocH, messH, uLetter, event->modifiers, false);
    return true;
  } else if ((event->modifiers & GDK_META_MASK) &&
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
  } else if (event->modifiers & GDK_META_MASK) {
    return false;
  } else if (win->ro & uLetter == ' ') {
    ((void)0);
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
    geditctrl_handle_key(win->pte, event->keyval, event->keycode, event->modifiers);
  }
#endif
  return false;
}

/* ============================================================
 * NextMess - skip to the next message
 * ============================================================ */
void NextMess(MacmbxTOC *tocH, MessHandle messH, short whichWay,
              long modifiers, bool ezOpen) {
  short next, sumNum;
  bool close, diffTOC;

  if (IsArrowSwitch(modifiers))
    modifiers &= ~GetPrefLong(PREF_SWITCH_MODIFIERS);
  close = !(modifiers & GDK_ALT_MASK);

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
      if (tocH->msgs[next].messH) {
        MyWindowPtr nextWin = ((MessHandle)tocH->msgs[next].messH)->win;
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
    if (geditctrl_is_dirty(TheBody)) {
      if (!PrefIsSet(PREF_EZ_SAVE)) {
        if (!SaveMessHi(win, false)) return ECANCELED;
      } else if (!SaveMess(win)) return ECANCELED;
    }
    ClearMessOpt(messH, OPT_WRITE);
    win->ro = (win->pte == TheBody);
    if ((TheBody != NULL && GTK_IS_WIDGET(TheBody)))
      gtk_text_view_set_editable(GTK_TEXT_VIEW(TheBody), FALSE);
    win->isDirty = false;
    geditctrl_clean(win->pte);
  } else {
    SetMessOpt(messH, OPT_WRITE);
    win->ro = false;
    if ((TheBody != NULL && GTK_IS_WIDGET(TheBody)))
      gtk_text_view_set_editable(GTK_TEXT_VIEW(TheBody), TRUE);
  }
  return 0;
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
    unsigned char *subj = SumOf(messH)->subject;
    if (subj[0] != '\0') {
      GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(subEntry));
      gtk_text_buffer_set_text(buf, (const char *)subj, -1);
      gtk_text_buffer_set_modified(buf, FALSE);
    }
    messH->subPTE = subEntry;
  }

  geditctrl_focus(TheBody);
  SetBGColorsByPers(messH);
  ((void)0);
  ((void)0);

  MessIBarUpdate(messH);
  CheckAddNotifyControls(win, messH);
  AddMessErrNote(messH);
  BeenThereDoneThat(messH->tocH, messH->sumNum);
  HiliteOddReply(messH);

  return 0;
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
  MacmbxTOC *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  mesgErrorHandle mesgErrH = tocH->msgs[sumNum].mesgErrH;
  if (mesgErrH || tocH->msgs[sumNum].state == MESG_ERR)
    messH->sound = NOTIFY_SOUND;
}

void PlaceMessErrNote(MessHandle messH) { (void)messH; }

/* ============================================================
 * MessIBarUpdate - update the icon bar state
 * ============================================================ */
/* ============================================================
 * MessIBarUpdate - update the message status bar (GTK port)
 *
 * Shows message state: read/unread/replied/forwarded/queued/sent,
 * priority, server status (on server / fetched / deleted),
 * and attachment indicator.
 * ============================================================ */
void MessIBarUpdate(MessHandle messH) {
  MyWindowPtr win = messH->win;
  if (!win || !win->window) return;

  MacmbxTOC *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  if (!tocH || sumNum < 0 || sumNum >= tocH->count) return;

  MacmbxMsgSum * sum = &tocH->msgs[sumNum];

  /* Find or create the status bar */
  GtkWidget *winWP = win->window;
  GtkWidget *statusbar = g_object_get_data(G_OBJECT(winWP), "msg-statusbar");
  if (!statusbar) {
    statusbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(statusbar, "msg-statusbar");
    gtk_widget_set_margin_start(statusbar, 8);
    gtk_widget_set_margin_end(statusbar, 8);
    gtk_widget_set_margin_top(statusbar, 2);
    gtk_widget_set_margin_bottom(statusbar, 2);
    g_object_set_data(G_OBJECT(winWP), "msg-statusbar", statusbar);

    /* Insert into window content — find the main vbox */
    GtkWidget *content = gtk_window_get_child(GTK_WINDOW(winWP));
    if (content && GTK_IS_BOX(content))
      gtk_box_prepend(GTK_BOX(content), statusbar);
  }

  /* Clear existing children */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(statusbar)))
    gtk_box_remove(GTK_BOX(statusbar), child);

  /* State indicator */
  const char *state_icon = NULL;
  const char *state_text = NULL;
  switch (sum->state) {
    case UNREAD:     state_icon = "mail-unread-symbolic";   state_text = "Unread"; break;
    case READ:       state_icon = "mail-read-symbolic";     state_text = "Read"; break;
    case REPLIED:    state_icon = "mail-reply-sender-symbolic"; state_text = "Replied"; break;
    case FORWARDED:  state_icon = "mail-forward-symbolic";  state_text = "Forwarded"; break;
    case REDIST:     state_icon = "mail-forward-symbolic";  state_text = "Redirected"; break;
    case QUEUED:     state_icon = "mail-send-symbolic";     state_text = "Queued"; break;
    case SENT:       state_icon = "mail-send-symbolic";     state_text = "Sent"; break;
    case UNSENDABLE: state_icon = "document-edit-symbolic"; state_text = "Draft"; break;
    case UNSENT:     state_icon = "document-edit-symbolic"; state_text = "Draft"; break;
    case MESG_ERR:   state_icon = "dialog-error-symbolic";  state_text = "Error"; break;
    default:         state_icon = "mail-read-symbolic";     state_text = ""; break;
  }
  if (state_icon) {
    GtkWidget *icon = gtk_image_new_from_icon_name(state_icon);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
    gtk_box_append(GTK_BOX(statusbar), icon);
  }
  if (state_text && state_text[0]) {
    GtkWidget *lbl = gtk_label_new(state_text);
    gtk_widget_add_css_class(lbl, "msg-status-text");
    gtk_box_append(GTK_BOX(statusbar), lbl);
  }

  /* Priority */
  if (sum->priority != 0 && sum->priority != 3) {
    const char *pri_text = NULL;
    switch (sum->priority) {
      case 1: pri_text = "Highest"; break;
      case 2: pri_text = "High"; break;
      case 4: pri_text = "Low"; break;
      case 5: pri_text = "Lowest"; break;
    }
    if (pri_text) {
      GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
      gtk_box_append(GTK_BOX(statusbar), sep);
      GtkWidget *pri_lbl = gtk_label_new(pri_text);
      gtk_widget_add_css_class(pri_lbl, "msg-priority");
      gtk_box_append(GTK_BOX(statusbar), pri_lbl);
    }
  }

  /* Attachment indicator */
  if (sum->flags & FLAG_HAS_ATT) {
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(statusbar), sep);
    GtkWidget *att_icon = gtk_image_new_from_icon_name("mail-attachment-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(att_icon), 16);
    gtk_box_append(GTK_BOX(statusbar), att_icon);
    GtkWidget *att_lbl = gtk_label_new("Attachment");
    gtk_box_append(GTK_BOX(statusbar), att_lbl);
  }

  /* Size */
  {
    char size_text[32];
    if (sum->length >= 1024*1024)
      snprintf(size_text, sizeof(size_text), "%.1f MB", sum->length / (1024.0*1024.0));
    else if (sum->length >= 1024)
      snprintf(size_text, sizeof(size_text), "%ld KB", sum->length / 1024);
    else
      snprintf(size_text, sizeof(size_text), "%ld B", sum->length);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(statusbar), sep);
    GtkWidget *size_lbl = gtk_label_new(size_text);
    gtk_widget_add_css_class(size_lbl, "msg-size");
    gtk_widget_set_hexpand(size_lbl, TRUE);
    gtk_label_set_xalign(GTK_LABEL(size_lbl), 1.0);
    gtk_box_append(GTK_BOX(statusbar), size_lbl);
  }
}

/* ============================================================
 * ExportHTMLSum / ExportHTML - export as HTML and open in browser
 * ============================================================ */
int ExportHTMLSum(MacmbxTOC *tocH, short sumNum) {
  MessHandle messH = tocH->msgs[sumNum].messH;
  MyWindowPtr win = nil;
  int err;

  /* macmbx downloads full messages during check — no on-demand fetch needed */
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
           SumOf(messH)->uid_hash);
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
  if (!(TheBody != NULL && GTK_IS_WIDGET(TheBody))) return;
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

void SetMessTable(MacmbxTOC *tocH, short sumNum, short newId) {
  if (tocH->msgs[sumNum].table_id != newId) {
    tocH->msgs[sumNum].table_id = newId;
    TOCSetDirty(tocH, true);
    if (tocH->previewID == tocH->msgs[sumNum].serial_num) tocH->previewID = 0;
    if (tocH->which != OUT && tocH->msgs[sumNum].messH)
      ReopenMessage(((MessHandle)tocH->msgs[sumNum].messH)->win);
  }
}

/* ============================================================
 * MessFocus - switch PTE focus, using gtk_widget_grab_focus
 * ============================================================ */
void MessFocus(MessHandle messH, GtkWidget * pte) {
  MyWindowPtr win = messH->win;
  bool wasSub = (win->pte == messH->subPTE);
  geditctrl_focus(pte);
  win->ro = (win->pte == TheBody) & !MessOptIsSet(messH, OPT_WRITE);
  if (wasSub & win->pte != messH->subPTE)
    MessSaveSub(messH);
  if (((pte) != NULL && GTK_IS_WIDGET(pte)))
    gtk_widget_grab_focus(pte);
}

/* ============================================================ */
int MessSaveSub(MessHandle messH) {
  unsigned char newSubj[256];
  PeteSString_(newSubj, messH->subPTE);
  SetSubject(messH->tocH, messH->sumNum, newSubj);
  geditctrl_set_dirty(messH->subPTE, FALSE);
  if (geditctrl_is_dirty(messH->win->pte))
    messH->win->isDirty = true;
  else
    messH->win->isDirty = false;
  return 0;
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
  geditctrl_handle_key(messH->bodyPTE, 0, 0, 0);
  return true;
}

/* ============================================================
 * IncrementQuoteLevel - adjust quote level via GtkTextBuffer
 * ============================================================ */
int IncrementQuoteLevel(GtkWidget * pte, long startSel, long endSel,
                        short increment) {
  if (!((pte) != NULL && GTK_IS_WIDGET(pte))) return -1;
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

bool Menu2TableId(MacmbxTOC *tocH, void **pmh, short item, short *tableId) {
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

void * MessBuildDragRgn(MessHandle messH) {
  (void)messH;
  return nil;
}
