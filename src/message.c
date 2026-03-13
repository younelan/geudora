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

/**********************************************************************
 * Includes and portability shims
 **********************************************************************/

#include <glib-object.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../gEditCtrl/geditctrl.h"
#include "Globals.h"

#include "message.h"
#include "portable-compat.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "comp.h"
#include "features.h"
#include "fileutil.h"
#include "gtk_dialogs.h"
#include "imapdownload.h"
#include "imapmailboxes.h"
#include "junk.h"
#include "legacy_shim.h"
#include "messact.h"
#include "boxact.h"
#include "peteglue.h"
#include "rich.h"

#include "lineio.h"

#include "log.h"
#include "mailbox.h"
#include "pop.h"
#include "prefdefs.h"
#include "schizo.h"
#include "threading.h"
#include "toc.h"
#include "trans.h"
#include "util.h"
#include "utl.h"
#ifndef LOG_PLUG
#define LOG_PLUG 0x100
#endif
#include "gtk_menus.h"

int ReallyDoAnAlert(int templ, int which);

short FindTOCSpot(TOCType * tocH, long serialNum);

char *FindHeaderString(char *text, char *headerName, long *size, bool bodyToo);
void BeautifyFrom(unsigned char *fromStr);
PStr GrabAttribution(short attrId, MyWindowPtr win, PStr attribution);
OSErr EnsureMessNewline(MessHandle messH);
int LastMsgSelected(TOCType * tocH);
OSErr RedirectAnnotation(MessHandle messH);
void CacheRecentNickname(unsigned char *name);
int QueueMessage(TOCType * tocH, short sumNum, int when, int flags, bool b1,
                 bool b2);
int InsertCommaIfNeedBe(GtkWidget *pte, HeadSpec *hs);
void CompAttachSpec(MyWindowPtr win, FSSpec *spec);
void NumToString(long num, unsigned char *str);
void CompDelAttachment(MessHandle messH, HeadSpec *where);
void AttachSelect(MessHandle messH);
int SigValidate(short sigId);
#define ValidHash(h) 1
/* SelectBoxRange declared in boxact.h */
void *FindRealSummary(TOCType * tocH, long serialNum, short *realSum);
int ConConMess(MessHandle messH, GtkWidget *pte, void *profile, void *a,
               void *b);
void *BoxPreviewProfile(void *name, TOCType * tocH, int which);
/* BeenThereDoneThat declared in boxact.h */
OSErr MakeAttSubFolder(MessHandle messH, unsigned long uidHash,
                       FSSpecPtr folder);
void ConConMultiple(TOCType * tocH, GtkWidget *pte, void *profile, int rule,
                    void *a, void *b);
long Munger(Handle h, long offset, void *ptr1, long len1, void *ptr2,
            long len2);
int StackQueue(void *stack, void *elem);
long BeautifyDate(unsigned char *dateStr, long *zoneSecs);

extern _Thread_local threadGlobalsPtr CurThreadGlobals;
#define MINI_MASK 0

/************************************************************************
 * ComputeLocalDate - format a message's date as a C string
 ************************************************************************/
void ComputeLocalDate(void *sum, unsigned char *dateStr) {
  if (!dateStr) return;
  dateStr[0] = '\0';
  if (sum) {
    MSumType *ms = (MSumType *)sum;
    if (ms->seconds) {
      time_t t = (time_t)ms->seconds;
      struct tm *tm = localtime(&t);
      if (tm)
        strftime((char *)dateStr, 64, "%Y-%m-%d %H:%M", tm);
    }
  }
}

/**********************************************************************
 * MakeMessTitle - make a reasonable message title from a summary
 **********************************************************************/
void MakeMessTitle(unsigned char *title, TOCType * tocH, int sumNum,
                   bool useSummary) {
  unsigned char from[64], date[64], time[64], mailbox[64], subject[64];
  unsigned char pattern[64];
  unsigned char datetime[64];
  long secs;
  unsigned char zoneStr[32];
  long zone;

  strcpy(from, (const char *)tocH->sums[sumNum].from);
  if (useSummary) {
    int maxW = (*BoxWidths)[blFrom - 1];
    if ((int)strlen((char *)from) > maxW)
      from[maxW] = '\0';
  }

  if (useSummary) {
    if ((*BoxWidths)[blDate - 1] > 1) {
      ComputeLocalDate(tocH->sums + sumNum, (unsigned char *)datetime);
      int maxDW = (*BoxWidths)[blDate - 1];
      if ((int)strlen((char *)datetime) > maxDW)
        datetime[maxDW] = '\0';
    } else
      datetime[0] = '\0';
  } else {
    if ((secs = tocH->sums[sumNum].seconds)) {
      zone = PrefIsSet(PREF_LOCAL_DATE) ? ZoneSecs()
                                        : 60 * tocH->sums[sumNum].origZone;
      secs += zone;
      TimeString(secs, false, (unsigned char *)time, NULL);
      DateString(secs, shortDate, (unsigned char *)date, NULL);
      utl_PlugParams((unsigned char *)GetRString((char *)pattern, DATE_SUM_FMT),
                     (unsigned char *)datetime, (unsigned char *)date,
                     (unsigned char *)time, (unsigned char *)zoneStr, "");
    } else
      datetime[0] = '\0';
  }

  GetMailboxName(tocH, sumNum, (unsigned char *)mailbox);
  strcpy(subject, (const char *)tocH->sums[sumNum].subj);

  utl_PlugParams((unsigned char *)GetRString((char *)pattern, MESS_TITLE_PLUG),
                 title, (unsigned char *)mailbox, (unsigned char *)from,
                 (unsigned char *)datetime, (unsigned char *)subject);
}

/* ... rest of file unchanged aside from token replacements ... */

/* Missing types/defs */
/* FindAttPtr will be the pointer to the struct defined below. */

typedef struct {
  gchar *text;
  bool outgoing;
  bool attach;
  HeadSpec hs;
  long offset;
} FindAttRec, *FindAttRecPtr;
/* Redefine FindAttPtr to correct type */
#define FindAttPtr FindAttRecPtr

/* File Info Types - Assuming defined in mailbox.h/Globals.h */

#define OPT_WIPE 0x0200
#define OPT_REDIRECTED 0x0010
#define peModLock 1
#define peSelectLock 2
#define peClickBeforeLock 4
#define peColorValid 0
#define SIG_NONE 0
#define kEuSendNow 1
#define kEuSendNext 0
#define OPT_HAS_SPOOL 1
#define smSystemScript 0
#define PreviewReadTimer 0
#define LABEL_COPY_MASK 0
#define pReplyLabel 0
#define fgAttachment 1
#define EAL_VARS_DECL int _eal_dummy = 0
#define EAL_VARS _eal_dummy
#define kPETECurrentStyle -1L
#define OPT_DELSP 0
#define OPT_BULK 0
#define OPT_WEIRD_REPLY 0
#define FLAG_ENCRYPT 0
#define kAlertNoteAlert 1
bool UseFlowOutExcerpt = false;

/* Feature Flags */
#define mcFetch 128
#define mcTrash 129
#define PREF_SUBJECT_IN_INLINE 12345 /* Dummy value */
/* Mac Event Macros */
#ifndef optionKey
#define optionKey 2048
#endif
#ifndef shiftKey
#define shiftKey 512
#endif

/* PETE Macros */
#ifndef peAllValid
#define peAllValid -1
#endif
#ifndef kPETELastPara
#define kPETELastPara -1L
#endif

#ifndef OPT_WRITE
#define OPT_WRITE 1
#endif
#ifndef MessOptIsSet
#define MessOptIsSet(mh, opt) 0
#endif

#define FILE_NUM 25
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Ported function declarations - replacing Mac types with standard C/GTK types
 */
char *GetMessText(MessHandle messH);
int MessErr;
int FindAndCopyHeader(MessHandle origMH, MessHandle newMH, char *fromHead,
                      short toHead);
void WeedHeaders(char **buf, size_t *bufSize, long *weeded, short toWeed, AccuPtr weeds);
long StripTrailingNewlines(char **buf, size_t *bufSize, long stop);
MyWindowPtr OpenMessage(TOCType * tocH, short sumNum, GtkWidget *winWP,
                        MyWindowPtr win, bool showIt, bool preview);
int RemoveSelf(MessHandle messH, short head, bool wantErrors);
void FindFrom(unsigned char *who, GtkWidget *pte);
void Attribute(short attrId, MessHandle origMessH, MessHandle newMessH,
               bool atEnd);
void XferCustomTable(MessHandle origMessH, MessHandle newMessH);
void WeedXAttachments(MessHandle messH, bool errReport);
void RemoveIndAttachment(MessHandle messH, short index);
void PeteApplyStyles(GtkWidget *pte, void *styles);
int CopyToOut(TOCType * fromTocH, short sumNum, TOCType * toTocH);
int UniqueHeader(MessHandle messH, short head, bool wantErrors);
int FindMessageByMID(unsigned long mid, TOCType * *tocH, short *sumNum);
OSErr TOCFindMessByMID(uLong mid, TOCType * tocH, long *sumNum);
int WipeMessage(TOCType * tocH, short sumNum);
int MessageWarnings(TOCType * tocH, short sumNum, bool toTrash, bool nuke,
                    bool *queuedWarning, bool *unsentWarning,
                    bool *unreadWarning, bool *busyWarning);
int SelectedWarnings(TOCType * tocH, bool toTrash, bool nuke);
int SingleWarnings(TOCType * tocH, short sumNum, bool toTrash, bool nuke);
void DeleteMessageLo(TOCType * tocH, int sumNum, bool nuke);
long CompBodyOffset(MessHandle messH);
int SpoolAttachments(MessHandle messH);
int CopyAttachments(MessHandle messH);
TOCType * GetRealTOC(TOCType * tocH, short sumN, short *realSumNum);
int ReadMessage(TOCType * tocH, int sumN, UPtr buffer);
uLong GetMessageLength(TOCType * tocH, short sumNum);
void FixSourceStatus(TOCType * tocH, short sumNum);
int AppendMessage(TOCType * fromTocH, int fromN, TOCType * toTocH, bool copy,
                  bool toTemp, bool isIMAPtoPopTransfer);
void DeleteSum(TOCType * tocH, int sumNum);
void Rehash(TOCType * tocH, int sumNum, UPtr buffer);

/* NOTE: using geditDocument APIs directly per request; no helpers here */

int ReplyReferences(MessHandle origMessH, MessHandle newMessH);
// void HTMLifyText(MyWindowPtr win, char *, long modifiers, bool nuke, Handle
// text);
void HTMLifyText(MyWindowPtr win, Handle text);

/* Missing Prototypes */
OSErr SubFolderSpec(short vRefNum, FSSpecPtr spec);

OSErr RemoveDir(FSSpecPtr spec);
bool PrefIsSetOrNot(int pref, int modifiers, int mask);
long SizeSelectedMessages(TOCType * tocH, bool includeText);
bool MemoryPreflight(long size);
bool MultiMessageOpOK(int warnType, int count);
void AddXfUndo(TOCType * tocH, TOCType * trashTOC, int unused);
TOCType * GetTrashTOC(void);
void MonitorGrow(bool flag);
bool DoMessageMenu(short item, TOCType * tocH, short sumNum, short toWhom,
                   void *addr, long modifiers, bool nuke, bool *busy);
void SetState(TOCType * tocH, short sumNum, int state);

/* ServerMenuChoice, SetPriority, BoxSelectAfter, BeenThereDoneThat — in boxact.h */
int Menu2Label(short menu);
MyWindowPtr GetAMessage(TOCType * tocH, short sumNum, void *u1, void *u2,
                        bool b1);
void NotUsingWindow(GtkWidget *win);
OSErr SavePtrAsMessage(UPtr preText, long preSize, UPtr text, long size,
                       TOCType * tocH, long *fromLen);
OSErr PutOutFromLine(short refN, long *len);
OSErr TruncAtMark(short refN);
int ReadSum(void *u1, bool b1, LineIOP lip, bool b2);
void NicknameWatcherFocusChange(GtkWidget *pte);
MyWindowPtr DoComposeNew(int type);
/* DELETE_ID is defined in MyRes.h (1003) */

#define mLoPlain 1
int CopyNewsgroups(MessHandle origMH, MessHandle newMH);
bool AttStillInFolder(FSSpecPtr att, FSSpecPtr folder);
unsigned char *MessCurAddr(MyWindowPtr win, unsigned char *addr);
void MakeMessTitle(unsigned char *title, TOCType * tocH, int sumNum,
                   bool useSummary);

void DoGStringGlobalReplace(GString *theString, const char *stringToFind,
                            const char *replacement);

/* PeteExtra declared in peteglue.h */
void RehashLo(TOCType * tocH, short sumNum, UHandle text, bool soft);
bool IsIMAPMessageProcessed(TOCType * tocH, short sumNum);
void ShowBoxSizes(MyWindowPtr win);
bool Mom(short button, short item, short pref, short warning, short verb);
bool IsQueued(TOCType * tocH, short sumNum);
/* Forward declarations */
#define SetHandleBig SetHandleSize
void MovingAttachments(TOCType * tocH, short sumNum, bool a, bool b, bool c,
                       bool d);
OSErr WipeDiskArea(short refN, long offset, long len);
void PlayNamedSound(unsigned char *name);
MyWindowPtr DoRedistributeMessage(MyWindowPtr win, void *toWhom, bool turbo,
                                  bool andDelete, bool showIt);
MyWindowPtr DoForwardMessage(MyWindowPtr win, void *toWhom, bool turbo);
short FindSumBySerialNum(TOCType * tocH, long serialNum);
MyWindowPtr DoReplyMessage(MyWindowPtr win, bool all, bool self, bool quote,
                           bool doFcc, short withWhich, bool vis, bool station,
                           bool caching);

unsigned char *PeteSelectedString(void *res, GtkWidget *pte);
unsigned char *GetRealname(unsigned char *addr);
int TextFindAndCopyHeader(char *body, long size, MessHandle newMH,
                          char *fromHead, short toHead, short label);
OSErr SpoolIndAttachment(MessHandle messH, short i);
void SumInfoCpy(MSumPtr newSum, MSumPtr oldSum);
void Preview(TOCType * tocH, short sumNum);
void ReplyDefaults(short modifiers, bool *all, bool *self, bool *quote);
MyWindowPtr DoSalvageMessage(MyWindowPtr win, bool forXfer);
MyWindowPtr DoSalvageMessageLo(MyWindowPtr win, bool forXfer, bool forIMAP);

/************************************************************************
 * GetAMessage - grab a message
 ************************************************************************/
MyWindowPtr GetAMessageLo(TOCType * origTocH, int origSumNum, GtkWidget *winWP,
                          MyWindowPtr win, bool showIt, bool *newWin) {
  GtkWidget *messWinWP;
  MessHandle messH;
  TOCType * tocH;
  short sumNum;

  if (newWin)
    *newWin = true;
  tocH = GetRealTOC(origTocH, origSumNum, &sumNum);
  if (!tocH || tocH->count <= sumNum)
    return (NULL);
  if ((messH = tocH->sums[sumNum].messH)) {
    messWinWP = (GtkWidget *)GetMyWindowWindowPtr(messH->win);
    if (newWin)
      *newWin = false;
    if (showIt) {
      if (!gtk_widget_get_visible(messWinWP))
        gtk_widget_set_visible(messWinWP, TRUE);
      gtk_window_present(GTK_WINDOW(messWinWP));
    }
    win = messH->win;
  } else if (tocH->which == MBX_OUT)
    win = OpenComp(tocH, sumNum, winWP, win, showIt, false);
  else
    win = OpenMessage(tocH, sumNum, winWP, win, showIt, false);
  if (win) {
    Win2MessH(win)->openedFromTocH = origTocH;
    Win2MessH(win)->openedFromSerialNum =
        origTocH->sums[origSumNum].serialNum;
  }
  return (win);
}

/**********************************************************************
 * OpenMessage - open a message in its own window
 **********************************************************************/
MyWindowPtr OpenMessage(TOCType * tocH, short sumNum, GtkWidget *winWP,
                        MyWindowPtr win, bool showIt, bool preview)
{
  MessHandle messH;
  char *text = NULL;
  short ezOpenSum;
  int err = 0;
  long size;
  long partial = GetRLong(PETE_NIBBLE) * 1024;
  bool useLizzie = false;
  bool disableButtons __attribute__((unused)) = false;

  if (!(tocH = GetRealTOC(tocH, sumNum, &sumNum)))
    return NULL;

  if ((messH = g_malloc0(sizeof(MessType))) == NULL)
    return (NULL);

  // Replace GetNewMyWindow with GTK window creation
  if (!win) {
    win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
    if (!win)
      return NULL;
    win->window = gtk_window_new();
    gtk_window_set_default_size(GTK_WINDOW(win->window), 600, 400);
    win->pte = NULL;
  }
  if (!win) {
    g_free(messH);
    return (NULL);
  }

  winWP = (GtkWidget *)GetMyWindowWindowPtr(win);

  tocH->sums[sumNum].messH = messH;
  messH->win = win;
  messH->sumNum = sumNum;
  messH->tocH = tocH;
  /* apply FLAG_OUT ex post facto */
  if (tocH->sums[sumNum].state == SENT ||
      tocH->sums[sumNum].state == UNSENT)
    tocH->sums[sumNum].flags |= FLAG_OUT;

  SetMyWindowPrivateData(win, (void *)messH);
  win->close = MessClose;
  messH->next = MessList; MessList = messH;

  if (MessOptIsSet(messH, OPT_WRITE))
    ClearMessOpt(messH, OPT_WRITE);

  // Actually go fetch this message if we must
  if (tocH->imapTOC && !IMAPMessageDownloaded(tocH, sumNum)) {
    // threading is off.  Download message now
    if (PrefIsSet(PREF_THREADING_OFF) || !ThreadsAvailable()) {
      if (EnsureMsgDownloaded(tocH, sumNum, true))
        text = GetMessText(messH);
    } else {
      char scratch[256];
      bool reallyGetIt = !preview || (!Offline && AutoCheckOK());

      // start the message download thread.  If we're opening this message to be
      // previewed ONLY, don't fetch it if we're offline.
      err = 0;
      if (!IMAPMessageBeingDownloaded(tocH, sumNum) &&
          !IMAPMessageDownloaded(tocH, sumNum) && reallyGetIt)
        err = UIDDownloadMessage(tocH, tocH->sums[sumNum].uidHash, false,
                                 false);
      if (err)
        goto Abort;

      // put the "waiting for download" text in the message window.
      if (!reallyGetIt)
        GetRString(scratch, IMAP_GETTING_MESSAGE_OFFLINE);
      else
        GetRString(scratch, IMAP_GETTING_MESSAGE);
      text = g_strdup(scratch);
      disableButtons = true;
    }
  } else
  {
    text = GetMessText(messH);
  }
  useLizzie = false;

  // Create gEditCtrl widget instead of Pete
  win->pte = geditctrl_new();
  if (!win->pte || !text)
    goto Abort;
  TheBody = win->pte;

  if (MessFlagIsSet(messH, FLAG_FIXED_WIDTH)) {
    // Apply CSS styling for fixed width font
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(
        provider, "textview { font-family: monospace; font-size: 12pt; }");
    gtk_widget_add_css_class(TheBody, "monospace-text");
    g_object_unref(provider);
  }

  {
    // Initialize gEditCtrl document instead of Pete stack
    geditDocument *doc = geditctrl_get_document(TheBody);

    /* Ensure text is valid UTF-8 — try charset from message headers,
     * then common encodings, then lossy fallback */
    if (!g_utf8_validate(text, -1, NULL)) {
      static const char *encodings[] = {
        "WINDOWS-1252", "ISO-8859-1", "MACINTOSH", "ISO-8859-15", NULL
      };
      char *utf8_text = NULL;
      for (int ei = 0; encodings[ei] && !utf8_text; ei++) {
        utf8_text = g_convert(text, -1, "UTF-8", encodings[ei],
                              NULL, NULL, NULL);
      }
      if (utf8_text) {
        g_free(text);
        text = utf8_text;
      }
      /* Last resort: replace invalid bytes */
      if (!g_utf8_validate(text, -1, NULL)) {
        char *clean = g_utf8_make_valid(text, -1);
        g_free(text);
        text = clean;
      }
    }

    /* Auto-detect HTML if flags aren't set — look for <html or <body tag
     * in the first 4K of the body */
    {
      long bodyOff = SumOf(messH)->bodyOffset - messH->weeded;
      if (bodyOff < 0) bodyOff = 0;
      const char *body = text + bodyOff;
      if (!MessOptIsSet(messH, OPT_HTML)) {
        /* Quick scan for HTML indicators */
        const char *scan = body;
        size_t scanLen = strlen(scan);
        if (scanLen > 4096) scanLen = 4096;
        for (size_t si = 0; si < scanLen - 5; si++) {
          if (scan[si] == '<' &&
              (g_ascii_strncasecmp(scan + si, "<html", 5) == 0 ||
               g_ascii_strncasecmp(scan + si, "<body", 5) == 0 ||
               g_ascii_strncasecmp(scan + si, "<!doc", 5) == 0)) {
            SetMessOpt(messH, OPT_HTML);
            break;
          }
        }
      }
    }

    // Calculate text size and handle partial loading
    bool isHTML = !MessFlagIsSet(messH, FLAG_SHOW_ALL) &&
                  (MessFlagIsSet(messH, FLAG_RICH)
                   || MessOptIsSet(messH, OPT_HTML)
                   || MessOptIsSet(messH, OPT_FLOW)
                   || MessOptIsSet(messH, OPT_CHARSET));

    if (!showIt || MessIsRich(messH) || MessOptIsSet(messH, OPT_CHARSET))
      partial = 0;
    size = strlen(text);
    if (!partial || size < partial) {
      if (isHTML) {
        /* For HTML messages, pass the body portion through markup parser.
         * Headers (before bodyOffset) are plain text. */
        long bodyOff = SumOf(messH)->bodyOffset - messH->weeded;
        if (bodyOff < 0) bodyOff = 0;
        if (bodyOff > 0 && bodyOff < size) {
          /* Insert headers as plain text */
          char *hdr = g_strndup(text, bodyOff);
          gedit_document_insert_text(doc, 0, hdr);
          g_free(hdr);
          /* Insert body as markup */
          gedit_document_insert_markup(doc, -1, text + bodyOff);
        } else {
          gedit_document_insert_markup(doc, 0, text);
        }
        g_free(text);
      } else {
        // Insert plain text
        gedit_document_insert_text(doc, 0, text);
        g_free(text);
      }

      if (!err) {
        text = NULL;
      }
    } else {
      // Insert partial text
      char *partial_text = g_strndup(text, partial);
      gedit_document_insert_text(doc, 0, partial_text);

      g_free(partial_text);
    }
  }
  if (err) {
    if (err != userCanceledErr)
      WarnUser(PETE_ERR, err);
  } else {
    char title[256];
    title[0] = '\0';
    MakeMessTitle((unsigned char *)title, tocH, sumNum, true);

    /* Ensure title is valid UTF-8 for GTK */
    if (title[0] && !g_utf8_validate(title, -1, NULL)) {
      char *utf8_title = g_convert(title, -1, "UTF-8", "WINDOWS-1252",
                                   NULL, NULL, NULL);
      if (utf8_title) {
        g_strlcpy(title, utf8_title, sizeof(title));
        g_free(utf8_title);
      } else {
        char *clean = g_utf8_make_valid(title, -1);
        g_strlcpy(title, clean, sizeof(title));
        g_free(clean);
      }
    }
    if (!title[0])
      g_strlcpy(title, "Message", sizeof(title));

    gtk_window_set_title(GTK_WINDOW(winWP), title);
    win->menu = MessMenu;
    win->gonnaShow = MessGonnaShow;
    win->position_new = (int (*)(bool, struct MyWindow *))MessagePosition;
    win->cursor = MessCursor;

    win->button = CompButton; /* it will do */
    win->app1 = MessApp1;
    win->find = MessFind;
    win->curAddr = MessCurAddr;

    /*
     * interpret rich text
     */
    /* HTML rendering is handled at insertion time (line 542) by
     * gedit_document_insert_markup() which parses tags and applies
     * styles. No separate interpretation pass needed. */

    if (showIt) {
      /*
       * prepare for easy open
       */
      ezOpenSum = EzOpenFind(tocH, sumNum);
      if (ShowMyWindow(winWP)) {
        return (NULL);
      }

      /*
       * scroll to show first line of body
       */
      /* PETE legacy calls - commented out for GTK port */
      /* ShowMessageSeparator(TheBody, true); */
      /* PeteFocus(win, TheBody, true); */

      /* Setup for non-PETE editor (gedit) */
      // MessFocus(messH, NULL);

      /*
       * rest of the text?
       */
      if (text && !useLizzie) {
        PeteCalcOff(win->pte);
        gedit_document_insert_text(geditctrl_get_document(win->pte), -1,
                                   text + partial);
        PeteCalcOn(win->pte);
        PeteSetURLRescan(win->pte, 0);

        if (err) {
          if (err != userCanceledErr)
            WarnUser(PETE_ERR, err);
        }
      }

      /*
       * prepare for ezopen
       */
      if (!err && ezOpenSum >= 0) {
        messH->ezOpenSerialNum = tocH->sums[ezOpenSum].serialNum;
        CacheMessage(tocH, ezOpenSum);
      }
      if (disableButtons)
        EnableMsgButtons(win, false);
    }
  }

  PeteSetDirty(TheBody, false);
  win->isDirty = false;

  if (err) {
  Abort:
    PeteCleanList(win->pte);
    win->isDirty = false;
    CloseMyWindow(winWP);
    win = NULL;
    g_free(text);
    text = NULL;
  }

  return (win);
}

/**********************************************************************
 * MessCurAddr - return the address most closely associated with this message
 **********************************************************************/
PStr MessCurAddr(MyWindowPtr win, PStr addr) {
  MessHandle messH = Win2MessH(win);
  /* extern PStr CompCurAddr(MyWindowPtr win, PStr addr); */

  *addr = 0;

  if (messH) {
    if (MessFlagIsSet(messH, FLAG_OUT))
      CompCurAddr(win, addr);
    else {
      if (win->hasSelection)
        return CurAddrSel(win, addr);
      else {
        SuckHeaderText(messH, (char *)addr, 256, FROM_HEAD);
        ShortAddr(addr, addr);
      }
    }
  }

  return *addr ? addr : NULL;
}

/**********************************************************************
 * EnsureMID - make sure a message has an id
 **********************************************************************/
OSErr EnsureMID(TOCType * tocH, short sumNum) {
  OSErr err = noErr;

  if (tocH->sums[sumNum].uidHash == kNeverHashed ||
      tocH->sums[sumNum].msgIdHash == kNeverHashed) {
    if (!(err = CacheMessage(tocH, sumNum)))
      RehashLo(tocH, sumNum, (UHandle)tocH->sums[sumNum].cache, true);
  }
  return (err);
}

/**********************************************************************
 * EnsureFromHash - make sure a message has a from id
 **********************************************************************/
OSErr EnsureFromHash(TOCType * tocH, short sumNum) {
  OSErr err = noErr;
  unsigned char scratch[256], shortAddr[256];
  uLong addrHash;

  if (tocH->sums[sumNum].fromHash == kNeverHashed) {
    if (!(err = CacheMessage(tocH, sumNum))) {
      HeaderName(FROM_HEAD); // weird--goes into scratch
      TrimWhite((unsigned char *)scratch);
      if (*HandleHeadGetPStr((char *)tocH->sums[sumNum].cache,
                             HEADER_STRN + FROM_HEAD, (char *)scratch)) {
        ShortAddr(shortAddr, scratch);
        MyLowerStr(shortAddr);
        addrHash = Hash(shortAddr);
      } else
        addrHash = kNoMessageId;

      tocH->sums[sumNum].fromHash = addrHash;
      TOCSetDirty(tocH, true);
    }
  }
  return (err);
}

/**********************************************************************
 * CacheMessage - put a message into the cache
 **********************************************************************/
OSErr CacheMessage(TOCType * tocH, short sumNum) {
  Handle cache;
  OSErr err = noErr;

  if (0 > sumNum || sumNum >= tocH->count)
    return (fnfErr);

  // don't do anything with IMAP messages that have not been downloaded yet.
  if (tocH->sums[sumNum].offset < 0)
    return (fnfErr);

  /*
   * is it there?
   */
  if (tocH->sums[sumNum].cache) {
    if (*tocH->sums[sumNum].cache)
      return (noErr); /* in the cache */
    else
      ZapHandle(tocH->sums[sumNum].cache); /* wipe out remnant */
  }

  /*
   * allocate it
   */
  if ((cache = NuHTempOK(GetMessageLength(tocH, sumNum)))) {
    /*
     * read it
     */
    err = ReadMessage(tocH, sumNum, *cache);
    if (err)
      ZapHandle(cache);
    else {
      tocH->sums[sumNum].cache = cache;
      ASSERT((*(unsigned char **)cache)[GetHandleSize(cache) - 1] == '\015');
    }
  } else
    err = MemError();

  return (err);
}

/**********************************************************************
 * GetMessText - put the text of a message
 **********************************************************************/
char *GetMessText(MessHandle messH) {
  MyWindowPtr win = messH->win;
  TOCType *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  char *buf = NULL;
  size_t bufSize = 0;
  long weeded;
  Accumulator weeds;

  Zero(weeds);

  /*
   * grab cached text, if any
   */
  Handle cache = NULL;
  if (tocH->sums[sumNum].cache && *tocH->sums[sumNum].cache) {
    cache = tocH->sums[sumNum].cache;
  } else
    ZapHandle(tocH->sums[sumNum].cache);

  /*
   * allocate buffer
   */
  bufSize = GetMessageLength(tocH, sumNum);
  buf = (char *)g_malloc(bufSize + 1);
  if (!buf) {
    if (!cache) {
      WarnUser(NO_MESS_BUF, MemError());
      return NULL;
    }
  }

  /*
   * read it
   */
  if (cache) {
    if (buf) {
      size_t cacheSize = GetHandleSize(cache);
      if (cacheSize > bufSize) cacheSize = bufSize;
      memmove(buf, *cache, cacheSize);
      bufSize = cacheSize;
    } else {
      /* use cache directly — take ownership */
      bufSize = GetHandleSize(cache);
      buf = (char *)g_malloc(bufSize + 1);
      if (buf) memmove(buf, *cache, bufSize);
      ZapHandle(cache);
      tocH->sums[sumNum].cache = NULL;
    }
  } else {
    if ((MessErr = ReadMessage(tocH, sumNum, (UPtr)buf)))
      goto failure;

    /* create cache copy */
    size_t cacheSize = bufSize;
    unsigned char *cacheData = (unsigned char *)g_malloc(cacheSize);
    Handle newCache = (Handle)g_malloc(sizeof(unsigned char *));
    if (cacheData && newCache) {
      *(unsigned char **)newCache = cacheData;
      memmove(cacheData, buf, cacheSize);
      tocH->sums[sumNum].cache = newCache;
    } else {
      g_free(cacheData);
      g_free(newCache);
    }
  }
  buf[bufSize] = '\0';

  /*
   * set hash, if we haven't already
   */
  if (tocH->sums[sumNum].uidHash == kNeverHashed)
    Rehash(tocH, sumNum, (UPtr)buf);

  /*
   * detect content-type before weeding removes the header
   * handles both \r\n (RFC 2822) and \n line endings
   */
  if (!MessOptIsSet(messH, OPT_HTML)) {
    const char *ct = strcasestr(buf, "\nContent-Type:");
    if (!ct && g_ascii_strncasecmp(buf, "Content-Type:", 13) == 0)
      ct = buf;
    if (ct) {
      const char *val = ct + (ct == buf ? 13 : 14);
      char ctbuf[256];
      int ci = 0;
      while (*val && ci < 255) {
        /* end of header: newline not followed by continuation whitespace */
        if (*val == '\r' || *val == '\n') {
          const char *next = val;
          if (*next == '\r') next++;
          if (*next == '\n') next++;
          if (*next != ' ' && *next != '\t') break;
        }
        ctbuf[ci++] = *val++;
      }
      ctbuf[ci] = '\0';
      if (strcasestr(ctbuf, "text/html"))
        SetMessOpt(messH, OPT_HTML);
    }
  }

  /*
   * weed headers?
   */
  if (!(SumOf(messH)->flags & FLAG_SHOW_ALL)) {
    WeedHeaders(&buf, &bufSize, &weeded, BadHeadStrn, &weeds);
    messH->weeded = weeded;
    StripTrailingNewlines(&buf, &bufSize,
                          SumOf(messH)->bodyOffset - weeded + 1);
  } else
    messH->weeded = 0;
  WeedHeaders(&buf, &bufSize, &weeded, FROM_STRN, NULL);
  messH->weeded += weeded;
  AccuTrim(&weeds);
  messH->extras = weeds;

  win->ro = !MessOptIsSet(messH, OPT_WRITE);
  return buf;

failure:
  g_free(buf);
  return NULL;
}

/**********************************************************************
 * ReadMessage - read a given message into a preallocated buffer
 **********************************************************************/
int ReadMessage(TOCType * tocH, int sumN, UPtr buffer) {
  long count;
  char name[256];
  short sumNum;

  tocH = GetRealTOC(tocH, sumN, &sumNum);
  if (!tocH)
    return fnfErr; // unable to find real TOC from virtual TOC
  GetMailboxName(tocH, sumNum, (unsigned char *)name);
  count = tocH->sums[sumNum].length;

  if (!(MessErr = BoxFOpenLo(tocH, sumNum)))
    if ((MessErr = SetFPos(tocH->refN, fsFromStart,
                           tocH->sums[sumNum].offset)) ||
        (MessErr = ARead(tocH->refN, &count, buffer)))
      FileSystemError(READ_MBOX, name, MessErr);

  return (MessErr);
}

/**********************************************************************
 * MoveMessage - transfer a message from one box to another
 * called when the transfer menu is invoked with a message frontmost
 **********************************************************************/
int MoveMessage(TOCType * tocH, int sumNum, FSSpecPtr toSpec, bool copy) {
  TOCType * toTocH;


  if ((toTocH = TOCBySpec(toSpec)) == NULL)
    return (1);

  if (toTocH->which == OUT && !copy &&
      !(tocH->sums[sumNum].flags & FLAG_OUT))
    if (ReallyDoAnAlert(XFER_TO_OUT, Caution) != 1)
      return (1);

  if (!copy) {
    if (SingleWarnings(tocH, sumNum, toTocH->which == TRASH, false))
      return (1);
  }

  return (MoveMessageLo(tocH, sumNum, toSpec, copy, false, true));
}

/**********************************************************************
 * MoveMessageLo - transfer a message from one box to another, no warnings
 **********************************************************************/
int MoveMessageLo(TOCType * tocH, int sumNum, FSSpecPtr toSpec, bool copy,
                  bool toTemp, bool holdOpen) {
  TOCType * toTocH;
  MessHandle messH = tocH->sums[sumNum].messH;
  char name[256];
  long serialNum;
  short realSumNum;
  bool isIMAPtoPopTransfer = false;

  // can't transfer a message we're sending
  if (!copy && (tocH->sums[sumNum].state == BUSY_SENDING))
    return (-1);
  if (LogLevel & LOG_MOVE)
    ComposeLogS(
        LOG_MOVE, NULL, (unsigned char *)"%s \"%s,%s\"  \"%s\"->\"%s\"\r",
        copy ? "Copy" : "Transfer", tocH->sums[sumNum].from,
        tocH->sums[sumNum].subj,
        GetMailboxName(tocH, sumNum, (unsigned char *)name), toSpec->name);


  if ((toTocH = TOCBySpec(toSpec)) == NULL)
    return (1);

  tocH = GetRealTOC(tocH, sumNum, &realSumNum);
  sumNum = realSumNum;

  // handle special transfer cases for IMAP mailbox transfers
  if (toTocH->imapTOC) {
    OSErr err = noErr;

    // IMAP to IMAP. Do an IMAP transfer.
    if (toTocH->imapTOC && tocH->imapTOC) {
      err = IMAPTransferMessage(tocH, toTocH, tocH->sums[sumNum].uidHash,
                                copy, false);
    }
    // POP to IMAP
    else if (toTocH->imapTOC && !tocH->imapTOC) {
      err = IMAPTransferMessageToServer(tocH, toTocH, sumNum, copy, false);
    }

    return (err);
  }

  if (tocH->which == OUT && !copy)
    FixSourceStatus(
        tocH, sumNum); /* in case we're deleting a reply, set orig state back */

  if (messH && messH->subPTE && PeteIsDirty(messH->subPTE))
    MessSaveSub(messH);

  // if this is an IMAP to POP transfer, close the message window
  if (tocH->imapTOC) {
    // IMAP to POP.  Download the message.
    bool downloaded = EnsureMsgDownloaded(tocH, sumNum, true);

    // drop the message if a translator asked to delete it
    if (tocH->sums[sumNum].opts & OPT_EMSR_DELETE_REQUESTED) {
      // message was fetched, but a translator deleted it.
      tocH->sums[sumNum].opts |=
          OPT_ORPHAN_ATT; // be real sure the attachments are left alone
      ComposeLogS(
          LOG_PLUG, NULL,
          (unsigned char *)"A plugin has deleted an IMAP message: '%s' in '%s'",
          tocH->sums[sumNum].subj, tocH->mailbox.spec.name);
      return (noErr);
    } else if (!downloaded)
      return (1);

    // if the message is open, close it.
    if (!copy && messH)
      CloseMyWindow(GetMyWindowWindowPtr(messH->win));
  }


  // if this is an IMAP to POP transfer, the POP copy will point to the
  // attachment
  isIMAPtoPopTransfer =
      tocH && tocH->imapTOC && toTocH && !toTocH->imapTOC;

  MessErr =
      AppendMessage(tocH, sumNum, toTocH, copy, toTemp, isIMAPtoPopTransfer);

  if (MessErr)
    return (MessErr);
  if (!holdOpen) {
    (void)BoxFClose(tocH, false);
    (void)BoxFClose(toTocH, true);
  }


  // if we moved an IMAP message to a POP mailbox, forget about its attachments
  // so they don't get tidied up.
  if (tocH->imapTOC && !toTocH->imapTOC)
    tocH->sums[sumNum].opts |= OPT_ORPHAN_ATT;

  if (!copy)
  {
    serialNum = tocH->sums[sumNum].serialNum;
    // if this was an IMAP message, we'll need to delete it from the server.
    if (tocH->imapTOC) {
      MessErr = IMAPDeleteMessage(tocH, tocH->sums[sumNum].uidHash, false,
                                  false, false)
                    ? noErr
                    : 1;
    } else
      DeleteSum(tocH, sumNum);
    //	Check for updates to search results
    SearchUpdateSum(toTocH, toTocH->count - 1, tocH, serialNum, true, false);
  }

  CheckBox(GetWindowMyWindowPtr(FrontWindow_()), false);
  return (MessErr);
}

/**********************************************************************
 * AppendMessage - add a message to a mailbox.	Message comes from another
 * mailbox.  Things in here are a little touchy, as there are several
 * things to do, any one of which could fail.  In order, this is
 * what is done:
 * 1. Move the bytes from one mailbox to the other.
 * 2. Copy the message summary from one toc to the other, updating
 *		tocH and sumNum in the message handle (if any).
 * 3. If the message window is open, fix pointers so the message
 *		belongs to the new box, not the old one.
 * Steps 1 and 2 could fail.	In either case, no real harm is done,
 * except that we might waste some space in the new mailbox.
 * Step 3 shouldn't ever fail.
 **********************************************************************/
int AppendMessage(TOCType * fromTocH, int fromN, TOCType * toTocH, bool copy,
                  bool toTemp, bool destHasAtt) {
  // UHandle buffer = NULL; // Unused variable removed
  MSumType sum;
  long eof;
  long count;
  // short err = 0; // Unused variable removed
  MessHandle fromMH;
  long newBodyOffset, newLength;
  mesgErrorHandle mesgErrH;
  FSSpec toSpec;
  /*
   * if it's an outgoing message, save it first
   */
  fromMH = fromTocH->sums[fromN].messH;
  if (fromMH) {
    MyWindowPtr win = fromMH->win;
    if (win->saveSize && win->position)
      (*win->position_new)(true, win);
    if (fromTocH->which == OUT) {
      MyWindowPtr win = fromMH->win;
      if ((win->isDirty || !fromTocH->sums[fromN].length) && !SaveComp(win))
        return (1);
    } else if (win->isDirty) {
      PeteFocus(fromMH->win, fromMH->bodyPTE, true);
      if (!SaveMessHi(win, false))
        return (1);
    }
  }

  /*
   * open the relevant mailboxes
   */
  MessErr = BoxFOpen(fromTocH);
  if (MessErr)
    return (MessErr);
  MessErr = BoxFOpen(toTocH);
  if (MessErr)
    return (MessErr);

  /*
   * is there space?
   */
  toSpec = GetMailboxSpec(toTocH, -1);
  if ((MessErr = VolumeMargin(toSpec.vRefNum, fromTocH->sums[fromN].length)))
    return (MessErr);

  /*
   * Are there too many messages in the destination?
   */
  if (toTocH->count >= MAX_MESSAGES_PER_MAILBOX) {
    unsigned char s[256];
    ComposeStdAlert(Stop, TOO_MANY_MESSAGES,
                    strcpy((char *)s, toTocH->mailbox.spec.name));
    return MessErr = 1;
  }

  /*
   * copy the message from one to the other
   */
  if (!copy)
    fromTocH->sums[fromN].flags &= ~FLAG_SKIPWARN;
  eof = FindTOCSpot(toTocH, fromTocH->sums[fromN].length);
  if (toTocH->which == OUT && !(fromTocH->sums[fromN].flags & FLAG_OUT)) {
    if ((MessErr = CopyToOut(fromTocH, fromN, toTocH)))
      return (MessErr);
  } else {
    if (fromTocH->sums[fromN].cache && *fromTocH->sums[fromN].cache) {
      count = fromTocH->sums[fromN].length;
      MessErr = SetFPos(toTocH->refN, fsFromStart, eof);
      if (!MessErr)
        MessErr = AWrite(toTocH->refN, &count,
                         *fromTocH->sums[fromN].cache);
      if (RunType != Production) {
        long controls = 0;
        UPtr dbspot = *fromTocH->sums[fromN].cache;
        UPtr dbend = dbspot + fromTocH->sums[fromN].length;
        for (; dbspot < dbend; dbspot++)
          if (*dbspot != '\015' && *dbspot < ' ' && *dbspot != '\t')
            controls++;
        if (controls * 20 > fromTocH->sums[fromN].length) {
          unsigned char buffer[256];
          ComposeString(buffer,
                        "Warning: %d control characters; may be a problem",
                        controls);
          AlertStr(OK_ALRT, Stop, buffer);
        }
      }
    } else
      MessErr =
          CopyFBytes(fromTocH->refN, fromTocH->sums[fromN].offset,
                     fromTocH->sums[fromN].length, toTocH->refN, eof);
    if (MessErr) {
      FileSystemError(COPY_FAILED, toSpec.name, MessErr);
      return (MessErr);
    }
    (void)SetEOF(toTocH->refN, eof + fromTocH->sums[fromN].length);
    newBodyOffset = fromTocH->sums[fromN].bodyOffset;
    newLength = fromTocH->sums[fromN].length;

    /*
     * now, create a new summary for the copied message, and put it in the
     * new TOC.
     */
    sum = fromTocH->sums[fromN];
    sum.offset = eof;
    sum.length = newLength;
    sum.bodyOffset = newBodyOffset;
    sum.selected = false;
    sum.messH = NULL;    /* break connection with open message window */
    sum.mesgErrH = NULL; /* clear mesgErrH. add it below */
    sum.serialNum = toTocH->nextSerialNum++;

    // Junk processing
    if (toTocH->which == JUNK && sum.spamBecause == 0) {
      sum.spamScore = GetRLong(JUNK_XFER_SCORE);
      sum.spamBecause = JUNK_BECAUSE_XFER;
    } else if (fromTocH->which == JUNK &&
               fromTocH->sums[fromN].spamBecause == JUNK_BECAUSE_XFER) {
      sum.spamScore = 0;
      sum.spamBecause = 0;
    }

    if (copy)
      sum.cache = NULL; // because we're copying, leave the cache alone
    else
      fromTocH->sums[fromN].cache =
          NULL; // let the cache go with the transferred message

    // the copy won't have an attachment.  That way, if it gets deleted, the
    // attachment will remain until the original is deleted. - jdboyd12/14/04
    if (copy && !destHasAtt)
      sum.flags &= ~FLAG_HAS_ATT;

    if (!sum.seconds)
      sum.seconds = GMTDateTime();
    if (fromTocH->which == OUT && !toTemp) {
      if (sum.state != SENT)
        sum.state = UNSENT;
      sum.tableId = NO_TABLE; /* don't translate */
      sum.flags |= FLAG_OUT;
    }

    if (toTocH->count) {
      MessErr = (PtrPlusHand_(&sum, toTocH, sizeof(sum)) != NULL) ? 0 : -1;
      if (MessErr) {
        ZapHandle(sum.cache);
        return (MessErr);
      }
    } else
      toTocH->sums[0] = sum;
    TOCSetDirty(toTocH, true);
    toTocH->reallyDirty = true;
    toTocH->resort = kResortWhenever;
    toTocH->count++;
    toTocH->analScanned = false;
  }
  TOCSetDirty(toTocH, true);
  toTocH->needRedo = MIN(toTocH->needRedo, toTocH->count - 1);

  /*
   * add mesg error to new toc entry
   */
  if ((mesgErrH = (mesgErrorHandle)fromTocH->sums[fromN].mesgErrH)) {
    unsigned char errorStr[256];
    AddMesgError(toTocH, toTocH->count - 1,
                 strcpy((char *)errorStr, (const char *)(*mesgErrH)->errorStr),
                 (*mesgErrH)->errorCode);
  }

  /*
   * the message window, if any, should be closed
   */
  if (!copy && fromTocH->sums[fromN].messH)
    CloseMyWindow(GetMyWindowWindowPtr(
        ((MessHandle)fromTocH->sums[fromN].messH)->win));

  return (MessErr); // return(noErr);	// 12/11/97 ccw
}

/**********************************************************************
 * MoveSelectedMessages - transfer all selected messages from one mail
 * box to another.
 **********************************************************************/
int MoveSelectedMessages(TOCType * tocH, FSSpecPtr toSpec, bool copy) {
  return (MoveSelectedMessagesLo(tocH, toSpec, copy, false, true, true));
}

/**********************************************************************
 * MoveSelectedMessages - transfer all selected messages from one mail
 * box to another.
 **********************************************************************/
int MoveSelectedMessagesLo(TOCType * tocH, FSSpecPtr toSpec, bool copy,
                           bool delete, bool undo, bool warnings) {
  TOCType * toTocH;
  int sumNum;
  int lastSelected = -1;
  unsigned char trashName[32];
  short oldCount;
  bool toTrash =
      IsRoot(toSpec) && StringSame(toSpec->name, GetRString(trashName, TRASH));
  long needRoom = 0;
  bool outWarning;
  long count;
  uLong pTicks = TickCount();
  Handle uidsH = NULL;
  OSErr err = noErr;
  TOCType * realTocH;
  short realSum;
  unsigned char name[256];

  if ((toTocH = TOCBySpec(toSpec)) == NULL)
    return (1);
  outWarning = !copy && toTocH->which == OUT;

  if (tocH->virtualTOC) {
    if (delete)
      IMAPDeleteMessagesFromSearchWindow(tocH);
    else {
      // do search window transfers
      err = IMAPTransferMessagesFromSearchWindow(tocH, toTocH, copy);
      if (err != noErr)
        return (err);
    }
  }

  //
  // Perform transfers to IMAP mailboxes.  There will be nothing left to do
  // after this.
  //

  if (toTocH->imapTOC) {
    return (IMAPMoveIMAPMessages(tocH, toTocH, copy));
  }

  //
  //	Handle the rest of the messages that have not been transferred yet.
  //

  if (!copy && warnings && SelectedWarnings(tocH, toTrash, false))
    return (0);

  count = CountSelectedMessages(tocH);
  for (sumNum = 0; sumNum < tocH->count; sumNum++)
    if (tocH->sums[sumNum].selected) {
      FixSourceStatus(
          tocH,
          sumNum); /* in case we're deleting a reply, set orig state back */
      needRoom += tocH->sums[sumNum].length + sizeof(MSumType);
      if (outWarning && !(tocH->sums[sumNum].flags & FLAG_OUT)) {
        outWarning = false; /* don't need anymore */
        if (ReallyDoAnAlert(XFER_TO_OUT, Caution) != 1)
          return (0);
        else
          break;
      }
    }

  NoteFreeSpace(toTocH);
  if (needRoom > toTocH->volumeFree - GetRLong(VOLUME_MARGIN) -
                     sizeof(MSumType) * count) {
    WarnUser(NOT_ENOUGH_ROOM, dskFulErr);
    return (0);
  }


  if (count > 10)
    OpenProgress();
  ProgressMessageR(kpSubTitle, LEFT_TO_TRANSFER);
  Progress(NoChange, count, NULL, NULL, NULL);

  //
  // If we are transferring from an IMAP mailbox to a POP mailbox,
  // ensure that the messages have been fetched.
  //

  if (tocH->imapTOC) {
    short c, totalToDownload;

    // must be online to do an IMAP to POP transfer, no matter if the message is
    // downloaded or not.
    if (!copy && Offline && GoOnline())
      return (0);

    // go through selected messages, and make sure they've all been downloaded.
    if ((totalToDownload = CountSelectedMessages(tocH)) > 0) {
      // figure out how many of the selected messages need to be downloaded
      for (sumNum = 0; sumNum < tocH->count; sumNum++)
        if (tocH->sums[sumNum].selected)
          if (IMAPMessageBeingDownloaded(tocH, sumNum) ||
              IMAPMessageDownloaded(tocH, sumNum))
            totalToDownload--;

      // Make a handle big enough for them
      if (totalToDownload > 0) {
        uidsH = NuHandleClear(totalToDownload * sizeof(unsigned long));
        if (uidsH) {
          // and stick them in the handle
          c = totalToDownload;
          for (sumNum = 0; sumNum < tocH->count; sumNum++)
            if (tocH->sums[sumNum].selected)
              if (!IMAPMessageBeingDownloaded(tocH, sumNum) &&
                  !IMAPMessageDownloaded(tocH, sumNum))
                memcpy(&((unsigned long *)(*uidsH))[--c],
                    &(tocH->sums[sumNum].uidHash), sizeof(unsigned long));

          // fetch them all in the foreground
          err = UIDDownloadMessages(tocH, uidsH, true, false);

          if (err == noErr) {
            short i;

            // put ALL of the messages in the mailbox.
            for (i = 0; i < totalToDownload; i++)
              UpdateIMAPMailbox(tocH);
          }
        } else {
          err = MemError();
        }
      }
    }

    if (err == noErr) {
      // make a new handle to store uids of deleted messages.  We'll build this
      // on the fly.
      uidsH = NuHandleClear(0);
      if (!uidsH)
        err = MemError();
    }

    if (err != noErr) {
      if (!CommandPeriod && (err != OFFLINE))
        WarnUser(err, MEM_ERR);
      return (err);
    }
  }
  for (sumNum = 0; sumNum < tocH->count; sumNum++) {
    if (tocH->sums[sumNum].selected) {
          if (count > 1)
        MiniEvents();
      if (EjectBuckaroo || CommandPeriod)
        break;
      if (!(--count % 10) || TickCount() - pTicks > 30) {
        Progress(NoBar, count, NULL, NULL, NULL);
        pTicks = TickCount();
      }

      // if this is a virtual TOC, and the message has already been processed,
      // skip it.
      if (tocH->virtualTOC && IsIMAPMessageProcessed(tocH, sumNum)) {
        continue;
      }

      if (tocH->imapTOC && !tocH->virtualTOC) {
        // Make sure IMAP message has been downloaded and stuck in the mailbox
        if (!EnsureMsgDownloaded(tocH, sumNum, true))
          continue; //	Couldn't download message
      }
      oldCount = tocH->count;
      if (!(realTocH = GetRealTOC(tocH, sumNum, &realSum)))
        continue;
      // if this is a virtual mailbox, and the real mailbox is an IMAP mailbox,
      // don't do anything with this message
      if (tocH->virtualTOC && realTocH->imapTOC)
        MessErr = noErr;
      else
        MessErr = AppendMessage(realTocH, realSum, toTocH, copy, false, false);
      if (MessErr) {
        if (MessErr != userCanceledErr)
          WarnUser(WRITE_MBOX, MessErr);
        break;
      }

      // if we moved this IMAP message to a POP mailbox, forget about its
      // attachments so they don't get tidied up.
      if (tocH->imapTOC && !toTocH->imapTOC)
        tocH->sums[sumNum].opts |= OPT_ORPHAN_ATT;

      // Log the transfer
      if (LogLevel & LOG_MOVE)
        ComposeLogS(LOG_MOVE, NULL,
                    (unsigned char *)"%s \"%s,%s\"  \"%s\"->\"%s\"\r",
                    copy ? "Copy" : "Transfer", realTocH->sums[realSum].from,
                    realTocH->sums[realSum].subj,
                    GetMailboxName(realTocH, realSum, name), toSpec->name);

      if (oldCount != tocH->count) {
        lastSelected = sumNum;
        sumNum--;
      } else if (!copy) {
        long serialNum = realTocH->sums[realSum].serialNum;

        if (!tocH->virtualTOC) {
          if (tocH->imapTOC && uidsH) {
            //	If not copying, we'll need to delete original IMAP message when
            // done.
            if (uidsH && !copy) {
              unsigned long uid = tocH->sums[sumNum].uidHash;
              err = PtrAndHand(&uid, uidsH, sizeof(uid));
              if (err != noErr) {
                WarnUser(err, MEM_ERR);
                ZapHandle(uidsH);
                return (err);
              }
            }
          }
          else {
            DeleteSum(tocH, sumNum);
            if (1) {
              lastSelected = sumNum;
              sumNum--; /* back up, so we can try again */
            } else
              lastSelected = sumNum;
          }

          if (realTocH != tocH)
            // delete real message summary
            DeleteSum(realTocH, realSum);

          //	Check for updates to search results
          SearchUpdateSum(toTocH, toTocH->count - 1, realTocH, serialNum,
                          true, false);
        }
      }
    }

    // now delete the IMAP messages that were transferred, if the transfer
    // completed successfully.
    if (!copy && tocH->imapTOC && (MessErr == noErr) && !CommandPeriod &&
        !EjectBuckaroo) {
      // uidsH contains uids of messages that have been successfully
      // transferred.
      IMAPDeleteMessages(tocH, uidsH, false, false, false, false);
    }

    (void)BoxFClose(tocH, false);
    (void)BoxFClose(toTocH, true);
    CloseProgress();

    if (tocH->win && !copy && !CommandPeriod)
      BoxSelectAfter(tocH->win, lastSelected);
    CheckBox(GetWindowMyWindowPtr(FrontWindow_()), false);
    ShowBoxSizes(tocH->win);
    return (MessErr);
  }
  return MessErr; // Add missing return for non-error path
}

/**********************************************************************
 * SelectedWarnings - Warnings for the selected message
 **********************************************************************/
OSErr SelectedWarnings(TOCType * tocH, bool toTrash, bool nuke) {
  bool queuedWarning, unsentWarning, unreadWarning, busyWarning;
  OSErr err = noErr;
  short i;

  MessageWarnings(tocH, -1, toTrash, nuke, &queuedWarning, &unsentWarning,
                  &unreadWarning, &busyWarning);

  for (i = 0; i < tocH->count && !err; i++)
    if (tocH->sums[i].selected)
      err = MessageWarnings(tocH, i, toTrash, nuke, &queuedWarning,
                            &unsentWarning, &unreadWarning, &busyWarning);

  return (err);
}

/**********************************************************************
 * SingleWarnings - Give warnings for a single message
 **********************************************************************/
OSErr SingleWarnings(TOCType * tocH, short sumNum, bool toTrash, bool nuke) {
  bool queuedWarning, unsentWarning, unreadWarning, busyWarning;

  MessageWarnings(tocH, -1, toTrash, nuke, &queuedWarning, &unsentWarning,
                  &unreadWarning, &busyWarning);

  return (MessageWarnings(tocH, sumNum, toTrash, nuke, &queuedWarning,
                          &unsentWarning, &unreadWarning, &busyWarning));
}

/**********************************************************************
 *
 **********************************************************************/
OSErr MessageWarnings(TOCType * tocH, short sumNum, bool toTrash, bool nuke,
                      bool *queuedWarning, bool *unsentWarning,
                      bool *unreadWarning, bool *busyWarning) {
  if (sumNum < 0) {
    *queuedWarning = tocH->which == OUT && !PrefIsSet(PREF_EASY_DEL_QUEUED);
    *unreadWarning = !PrefIsSet(PREF_EASY_DEL_UNREAD);
    *unsentWarning = tocH->which == OUT && !PrefIsSet(PREF_EASY_DEL_UNSENT);
    *busyWarning = true; // for now, until/if we get a pref for it
  } else {
    short button, verb;

    if (toTrash)
      if (nuke) {
        button = NUKE_BTN;
        verb = NUKE_VERB;
      } else {
        button = TRASH_BTN;
        verb = TRASH_VERB;
      }
    else {
      button = XFER_BTN;
      verb = XFER_VERB;
    }

    // clarence 4/25/97
    if (tocH->sums[sumNum].state == BUSY_SENDING) {
      WarnUser(SENDING_WARNING, 0);
      *busyWarning = false;
      return (1);
    }

    if (tocH->sums[sumNum].flags & FLAG_SKIPWARN)
      return (noErr);
    if (toTrash && *unreadWarning && (tocH->sums[sumNum].state == UNREAD)) {
      if (!Mom(button, 0, PREF_EASY_DEL_UNREAD, UNREAD_WARNING, verb))
        return (1);
      *unreadWarning = false; /* been there, done that. */
    }
    if (*queuedWarning && IsQueued(tocH, sumNum)) {
      if (!Mom(button, 0, PREF_EASY_DEL_QUEUED, QUEUED_WARNING, verb))
        return (1);
      *queuedWarning = *unsentWarning = false; /* been there, done that. */
    }
    if (*unsentWarning && (tocH->sums[sumNum].state != SENT)) {
      if (!Mom(button, 0, PREF_EASY_DEL_UNSENT, UNSENT_WARNING, verb))
        return (1);
      *unsentWarning = false; /* been there, done that. */
    }
  }
  return (noErr);
}

/**********************************************************************
 * FixSourceStatus - fix the status of a source message
 **********************************************************************/
void FixSourceStatus(TOCType * tocH, short sumNum) {
  MessHandle messH = tocH->sums[sumNum].messH;
  TOCType * sourceTocH;
  short sourceNum;
  uLong **midList;
  short i;

  if (tocH->which == OUT && messH && SumOf(messH)->state != SENT &&
      messH->aSourceMID.offset) {
    midList = (uLong **)messH->aSourceMID.data;
    for (i = messH->aSourceMID.offset / (3 * sizeof(long)) - 1; i >= 0;
         i--) {
      uLong sourceMID = (*midList)[i * 3];
      short sourceOrigState = (*midList)[i * 3 + 1];
      short sourceNewState = (*midList)[i * 3 + 2];
      if (sourceMID && sourceMID != kNeverHashed &&
          sourceOrigState != sourceNewState) {
        if (!FindMessageByMID(sourceMID, &sourceTocH, &sourceNum)) {
          if (sourceNewState == sourceTocH->sums[sourceNum].state)
            SetState(sourceTocH, sourceNum, sourceOrigState);
        }
      }
    }
    /* data is char* (flat buffer in CrispinIMAP Accumulator), just free once */
    if (messH->aSourceMID.data) {
      free(messH->aSourceMID.data);
      messH->aSourceMID.data = NULL;
    }
    messH->aSourceMID.offset = messH->aSourceMID.size = 0;
  }
}

/**********************************************************************
 * FindMessageByMID - find a message by mid.  Works only on open toc's at the
 *moment
 **********************************************************************/
OSErr FindMessageByMID(uLong mid, TOCType * *tocH, short *sumNum) {
  TOCType * lTocH;
  long lsum;

  for (lTocH = TOCList; lTocH; lTocH = (TOCType *)lTocH->next) {
    if (!TOCFindMessByMID(mid, lTocH, &lsum)) {
      *sumNum = lsum;
      return (noErr);
    }
  }
  return (fnfErr);
}

/**********************************************************************
 * TOCFindMessByMID - find a message in a toc by uid hash
 **********************************************************************/
OSErr TOCFindMessByMID(uLong mid, TOCType * tocH, long *sumNum) {
  short lSumNum;

  for (lSumNum = tocH->count - 1; lSumNum >= 0; lSumNum--)
    if (tocH->sums[lSumNum].uidHash == mid) {
      *sumNum = lSumNum;
      return (noErr);
    }
  return (fnfErr);
}

/**********************************************************************
 * TOCFindMessByMsgID - find a message in a toc by message id
 **********************************************************************/
OSErr TOCFindMessByMsgID(uLong mid, TOCType * tocH, long *sumNum) {
  short lSumNum;

  for (lSumNum = tocH->count - 1; lSumNum >= 0; lSumNum--)
    if (tocH->sums[lSumNum].msgIdHash == mid) {
      *sumNum = lSumNum;
      return (noErr);
    }
  return (fnfErr);
}


/************************************************************************
 * FindAnAttachment - find an attachment from a line of text
 ************************************************************************/
long FindAnAttachment(Handle text, long offset, FSSpecPtr spec, bool attach,
                      uLong *cid, uLong *relURL, uLong *absURL) {
  UPtr spot, newLine, end;
  bool result = false;
  unsigned char line[256];

  end = *text + GetHandleSize_(text);
  spot = *text + offset;
  while (spot < end && *spot++ != '\015')
    ;

  for (; spot < end; spot = newLine + 1) {
    for (newLine = spot; newLine < end && *newLine != '\015'; newLine++)
      ;
    if (newLine - spot > 24 && newLine - spot < 255) {
      { long _len = newLine - spot; if (_len > (long)sizeof(line) - 1) _len = sizeof(line) - 1; memcpy(line, spot, _len); line[_len] = '\0'; }
      if (attach) {
        result = !AttLine2Spec(line, spec, false);
        if (result)
          break;
      } else {
        result = !RelLine2Spec(line, spec, cid, relURL, absURL);
        if (result)
          break;
      }
    }
  }
  offset = result ? spot - (UPtr)*text : -1;
  return (offset);
}

/**********************************************************************
 * InitAttachmentFinder - initialize data for finding attachments in
 *    outgoing or received messages
 **********************************************************************/
void InitAttachmentFinder(FindAttPtr pData, Handle text, bool attach,
                          TOCType * tocH, MSumPtr sum) {
  pData->text = text;
  pData->outgoing = tocH->which == OUT || sum->state == SENT ||
                    sum->state == UNSENT || sum->flags & FLAG_OUT;
  pData->attach = attach;
  if (pData->outgoing) {
    // Outgoing message. Find Attachments header
    unsigned char hdrName[64];

    GetRString(hdrName, HeaderStrn + ATTACH_HEAD);
    HandleHeadFindStr((char *)text, hdrName, &pData->hs);
  } else {
    // Received message. Start at beginning of message body
    pData->offset = sum->bodyOffset - 1;
  }
}

/**********************************************************************
 * GetNextAttachment - find next attachment
 **********************************************************************/
bool GetNextAttachment(FindAttPtr pData, FSSpecPtr spec) {
  bool result = false;

  if (pData->text) {
    if (pData->outgoing) {
      result = pData->attach ? (GetIndAttachmentLo(pData->text, 1, spec, NULL,
                                                   &pData->hs) != 1)
                             : false;
    } else {
      pData->offset = FindAnAttachment(pData->text, pData->offset, spec,
                                       pData->attach, NULL, NULL, NULL);
      result = pData->offset != -1;
    }
    if (!result)
      // No more attachments. Signal we are done
      pData->text = NULL;
  }
  return result;
}

/**********************************************************************
 * DeleteMessage - delete a summary from a toc, and fix the screen, too
 **********************************************************************/
void DeleteMessage(TOCType * tocH, int sumNum, bool nuke) {
  if (tocH->imapTOC) {
    // close the IMAP message to be deleted, even if it's just going to be
    // marked.
    MessHandle messH = (MessHandle)tocH->sums[sumNum].messH;
    if (messH)
      CloseMyWindow(GetMyWindowWindowPtr(messH->win));

    IMAPDeleteMessage(tocH, tocH->sums[sumNum].uidHash, nuke, false, false);

    return;
  }

  if (SingleWarnings(tocH, sumNum, true, nuke || tocH->which == TRASH))
    return;
  DeleteMessageLo(tocH, sumNum, nuke);
}

/**********************************************************************
 * DeleteMessage - delete a summary from a toc, and fix the screen, too
 **********************************************************************/
void DeleteMessageLo(TOCType * tocH, int sumNum, bool nuke) {
  MessHandle messH = (MessHandle)tocH->sums[sumNum].messH;
  // bool dirt = 0; // Unused variable removed
  FSSpec trashSpec;
  int oldN = tocH->count;
  bool wipe = PrefIsSet(PREF_WIPE) && (tocH->sums[sumNum].opts & OPT_WIPE);

  if (tocH->which != TRASH && !wipe && !nuke) {
    trashSpec.vRefNum = MailRoot.vRef;
    trashSpec.parID = MailRoot.dirId;
    GetRString(trashSpec.name, TRASH);
    MoveMessageLo(tocH, sumNum, &trashSpec, false, false, true);
  } else {
    if (wipe)
      WipeMessage(tocH, sumNum);
    if (messH)
      CloseMyWindow(GetMyWindowWindowPtr(messH->win));
    if (tocH->count == oldN)
      DeleteSum(tocH, sumNum);
  }
}

/**********************************************************************
 * WipeMessage - clear the contents of a message, really
 **********************************************************************/
OSErr WipeMessage(TOCType * tocH, short sumNum) {
  OSErr err;

  MovingAttachments(tocH, sumNum, true, true, false, false);
  MovingAttachments(tocH, sumNum, false, true, false, false);

  if ((err = BoxFOpen(tocH)))
    return (err);

  err = WipeDiskArea(tocH->refN, tocH->sums[sumNum].offset,
                     tocH->sums[sumNum].length);
  return err;
}

/************************************************************************
 * MessageError - return the most recent error code from these functions
 ************************************************************************/
int MessageError(void) { return (MessErr); }

/**********************************************************************
 * StripTrailingNewlines - strip trailing newlines from a text buffer
 **********************************************************************/
long StripTrailingNewlines(char **buf, size_t *bufSize, long stop) {
  long size = (long)*bufSize;
  char *spot = *buf + size;

  while (spot > *buf + stop && (spot[-1] == '\n' || spot[-1] == '\r'))
    spot--;

  long newSize = spot - *buf;
  *bufSize = newSize;
  (*buf)[newSize] = '\0';
  return (size - newSize);
}

/************************************************************************
 * WeedHeaders - weed a message's headers, leaving only the interesting
 * ones.
 ************************************************************************/
/* Standard Eudora bad headers to weed (replaces Mac STR# 3200 resource) */
static const char *kBadHeaders[] = {
  "Received:", "Return-Path:", "X-Sender:", "X-Mailer:",
  "X-Attachments:", "Message-Id:", "MIME-Version:", "Content-Type:",
  "Content-Transfer-Encoding:", "Content-Disposition:",
  "X-Priority:", "X-MSMail-Priority:", "Importance:",
  "X-MimeOLE:", "X-Spam-Status:", "X-Spam-Score:",
  "X-Spam-Flag:", "X-Virus-Scanned:", "X-Original-To:",
  "Delivered-To:", "Authentication-Results:", "DKIM-Signature:",
  "DomainKey-Signature:", "X-Google-DKIM-Signature:",
  "ARC-Seal:", "ARC-Message-Signature:", "ARC-Authentication-Results:",
  "X-Gm-Message-State:", "X-Google-Smtp-Source:",
  "X-Received:", "X-Forwarded-To:", "X-Forwarded-For:",
  NULL
};

/* Standard Eudora "From " line header to weed (replaces Mac STR# 3400 resource) */
static const char *kFromHeaders[] = {
  "From ", /* Unix mbox "From " line */
  NULL
};

void WeedHeaders(char **buf, size_t *bufSize, long *weeded, short toWeed, AccuPtr weeds) {
  const char **badList;
  if (toWeed == FROM_STRN)
    badList = kFromHeaders;
  else
    badList = kBadHeaders;

  int badN = 0;
  while (badList[badN]) badN++;
  if (badN == 0) return;

  /* Helper: is this a line break? (\r\n, \r, or \n) */
  #define IS_EOL(p) ((p) < end && (*(p) == '\n' || *(p) == '\r'))
  /* Advance past one line break (handles \r\n, \r, \n) */
  #define SKIP_EOL(p) do { if (*(p)=='\r') { (p)++; if ((p)<end && *(p)=='\n') (p)++; } else if (*(p)=='\n') (p)++; } while(0)

  unsigned char *spot, *done, *end, *found;

  done = spot = (unsigned char *)*buf;
  end = spot + *bufSize;

  while (spot < end) {
    /* blank line = end of headers */
    if (IS_EOL(spot)) {
      unsigned char *peek = spot;
      SKIP_EOL(peek);
      if (peek <= end && (peek == end || IS_EOL(peek)))
        break; /* two consecutive line breaks */
      /* single EOL at start of line followed by non-continuation */
      break;
    }

    int bad;
    bool matched = false;
    for (bad = 0; bad < badN; bad++) {
      size_t hlen = strlen(badList[bad]);
      if ((size_t)(end - spot) >= hlen &&
          g_ascii_strncasecmp((const char *)spot, badList[bad], hlen) == 0) {
        found = spot;
        /* skip this header and any continuation lines */
        while (spot < end) {
          if (*spot == '\r' || *spot == '\n') {
            unsigned char *after = spot;
            SKIP_EOL(after);
            if (after >= end || (*after != ' ' && *after != '\t')) {
              spot = after;
              break;
            }
            spot = after; /* continuation line, keep going */
          } else {
            spot++;
          }
        }
        if (weeds)
          AccuAddPtr(weeds, found, spot - found);
        matched = true;
        break;
      }
    }
    if (matched)
      continue;

    /* keep this header — copy it through */
    while (spot < end) {
      if (*spot == '\r' || *spot == '\n') {
        /* copy the line ending */
        if (*spot == '\r') { *done++ = *spot++; if (spot < end && *spot == '\n') *done++ = *spot++; }
        else *done++ = *spot++;
        /* check for continuation */
        if (spot >= end || (*spot != ' ' && *spot != '\t'))
          break;
      } else {
        *done++ = *spot++;
      }
    }
  }

  /* copy everything after headers (blank line + body) */
  while (spot < end)
    *done++ = *spot++;

  #undef IS_EOL
  #undef SKIP_EOL

  long newSize = done - (unsigned char *)*buf;
  if (weeded)
    *weeded = (long)*bufSize - newSize;
  *bufSize = newSize;
  (*buf)[newSize] = '\0';
}

/************************************************************************
 * SetMessText - stick some text into one of the fields of a message.
 ************************************************************************/
OSErr SetMessText(MessHandle messH, short whichTXE, UPtr string, long size) {
  HeadSpec hs;

  if (CompHeadFind(messH, whichTXE, &hs))
    return (CompHeadSetPtr(TheBody, &hs, (char *)string, size));
  else
    return (fnfErr);
}

/**********************************************************************
 * Fix1MessServerArea - fix the server display of a single message
 **********************************************************************/
void Fix1MessServerArea(MyWindowPtr win) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  MessHandle messH;

  if (GetWindowKind(winWP) == MESS_WIN && IsWindowVisible(winWP)) {
    messH = Win2MessH(win);
    uLong uidHash = SumOf(messH)->uidHash;
    OSType popdType;
    {
      PersHandle p = (PersHandle)MESS_TO_PPERS(messH);
      popdType = PERS_POPD_TYPE(p);
    }

    /* Look up fetch/trash buttons on the message window toolbar */
    GtkWidget *fetch = winWP ? g_object_get_data(G_OBJECT(winWP), "btn-fetch") : NULL;
    GtkWidget *trash = winWP ? g_object_get_data(G_OBJECT(winWP), "btn-trash") : NULL;

    if (IdIsOnPOPD(popdType, POPD_ID, uidHash)) {
      bool lmos = PrefIsSet(PREF_LMOS);
      bool onFetch = IdIsOnPOPD(popdType, FETCH_ID, uidHash);
      bool onDelete = IdIsOnPOPD(popdType, DELETE_ID, uidHash);
      if ((messH->hasFetchIcon = MessFlagIsSet(messH, FLAG_SKIPPED))) {
        if (fetch) {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fetch), onFetch);
          gtk_widget_set_visible(fetch, TRUE);
        }
      }
      messH->hasDelIcon = true;
      if (trash) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(trash), onDelete);
        gtk_widget_set_sensitive(trash, !(onFetch && !lmos));
        gtk_widget_set_visible(trash, TRUE);
      }
    } else {
      messH->hasFetchIcon = messH->hasDelIcon = false;
      if (fetch)
        gtk_widget_set_visible(fetch, FALSE);
      if (trash)
        gtk_widget_set_visible(trash, FALSE);
    }
  } else if (GetWindowKind(winWP) == MBOX_WIN ||
             GetWindowKind(winWP) == CBOX_WIN) {
    InvalTocBox((TOCType *)GetMyWindowPrivateData(win), -2, blServer);
  }
}

/**********************************************************************
 * RecordTransAttachments - record the fact that we have attachments from a
 *translator
 **********************************************************************/
OSErr RecordTransAttachments(const char *path) {
  WindowPtr InsertWinWP = GetMyWindowWindowPtr(InsertWin);
  MessHandle messH;
  FSSpecHandle h;
  FSSpec tmpSpec = {0};

  /* Build a temporary FSSpec from the path for PtrPlusHand_ storage */
  if (path) {
    strncpy(tmpSpec.path, path, sizeof(tmpSpec.path) - 1);
    {
      char pathCopy[1024];
      strncpy(pathCopy, path, sizeof(pathCopy) - 1);
      pathCopy[sizeof(pathCopy) - 1] = '\0';
      const char *base = basename(pathCopy);
      strncpy(tmpSpec.name, base, sizeof(tmpSpec.name) - 1);
    }
  }

  if (InsertWin && GetWindowKind(InsertWinWP) == MESS_WIN) {
    messH = Win2MessH(InsertWin);
    if (!messH->etlFiles) {
      h = NuHTempBetter(0);
      if (!h)
        return (MemError());
      messH->etlFiles = h;
    }
    return (PtrPlusHand_(&tmpSpec, messH->etlFiles, sizeof(tmpSpec)) != NULL)
               ? 0
               : -1;
  }
  return noErr;
}

/************************************************************************
 * CleanSpoolFolder - clean up the spool folder
 ************************************************************************/
static uLong spoolAge;
static bool CleanSpoolCallback(DirIterateInfo *info) {
  if (EventPending())
    return false; /* Stop iteration */
  MiniEvents();

  if (info->isDir &&
      AllDigits((unsigned char *)info->spec.name, strlen(info->spec.name))) {
    if (info->modifyDate < spoolAge) {
      RemoveDir(&info->spec);
    }
  }
  return true; /* Continue iteration */
}

OSErr CleanSpoolFolder(uLong age) {
  FSSpec spec;

  if (SubFolderSpec(SPOOL_FOLDER, &spec))
    return (noErr);

  spoolAge = LocalDateTime() - 24 * 3600 * age;
  return DirIterate(&spec, NULL, CleanSpoolCallback);
}

/************************************************************************
 * AppendMessText - stick some text after one of the fields of a message.
 ************************************************************************/
OSErr AppendMessText(MessHandle messH, short whichTXE, UPtr string, long size) {
  HeadSpec hs;

  if (CompHeadFind(messH, whichTXE, &hs)) {
    return (CompHeadAppendPtr(TheBody, (HSPtr)&hs, (char *)string, size));
  } else
    return (fnfErr);
}

/************************************************************************
 * MessPlainBytes - make sure bytes are plain
 ************************************************************************/
OSErr MessPlainBytes(MessHandle messH, short whichTXE, short bytes) {
  OSErr err = noErr;
  HeadSpec hs;
  long start, stop __attribute__((unused));

  if (!CompHeadFind(messH, whichTXE, &hs))
    err = fnfErr;
  else {
    if (bytes < 0) {
      start = (hs.offset + hs.length) + bytes;
      start = MAX(start, hs.offset);
      stop = (hs.offset + hs.length);
    } else {
      start = hs.offset;
      stop = start + bytes;
    }
    /* gEditCtrl supports richtext. Legacy PETE calls commented out for porting.
     */
    /* PeteParaConvert(TheBody, start, stop); */
    /* PetePlain(TheBody, start, stop, peAllValid); */
    /* PeteParaRange(TheBody, &start, &stop); */
    if (whichTXE ==
        mLoPlain) // Assuming 'item' in the instruction refers to 'whichTXE'
      /* PetePlainPara(TheBody, kPETELastPara); */
      ;
    /* PetePlainParaAt(TheBody, start, stop); */
  }
  return err;
}

/************************************************************************
 * DoIterativeThingyLo - do something over all selected messages
 ************************************************************************/
void DoIterativeThingyLo(TOCType * tocH, int item, long modifiers,
                         TextAddrHandle addr, bool warnings) {
  int sumNum;
  short toWhom = (short)(long)addr;
  short lastSelected = -1;
  unsigned char title[256];
  FSSpec trashSpec;
  long count;
  long size;
  long gran;
  uLong pTicks = TickCount();
  bool nuke =
      item == MESSAGE_DELETE_ITEM &&
      (tocH->which == TRASH ||
       (optionKey | shiftKey) == (modifiers & (optionKey | shiftKey))) &&
      (!tocH->imapTOC || PrefIsSet(PREF_ALLOW_IMAP_NUKE));
  Boolean busy = false;
  uLong oldEzOpenSerialNum = tocH->previewID ? tocH->ezOpenSerialNum : 0;


  if (item == MESSAGE_DELETE_ITEM && tocH->imapTOC && !nuke) {
    Handle uids = NULL;
    long c = CountSelectedMessages(tocH);
    long sumNum;

    // only do something if there are some selected messages
    if (c) {
      // build a list of uids to be deleted
      uids = NuHandleClear(c * sizeof(unsigned long));
      if (uids) {
        for (sumNum = 0; sumNum < tocH->count && c; sumNum++)
          if (tocH->sums[sumNum].selected) {
            // Close this message if it's open
            if (tocH->sums[sumNum].messH)
              CloseMyWindow(
                  GetMyWindowWindowPtr(tocH->sums[sumNum].messH->win));

            memcpy(&((unsigned long *)(*uids))[--c],
                &(tocH->sums[sumNum].uidHash), sizeof(unsigned long));
          }

        // and delete them.
        IMAPDeleteMessages(tocH, uids, nuke, false, false, false);

        // preview the next message ...
        if (oldEzOpenSerialNum &&
            (sumNum = FindSumBySerialNum(tocH, oldEzOpenSerialNum)) >= 0 &&
            !(modifiers & shiftKey))
          Preview(tocH, sumNum);
      } else {
        WarnUser(MemError(), MEM_ERR);
      }
    }

    return;
  }

  if (item == MESSAGE_DELETE_ITEM && !nuke) {
    trashSpec.vRefNum = MailRoot.vRef;
    trashSpec.parID = MailRoot.dirId;
    GetRString(trashSpec.name, TRASH);
    MoveSelectedMessagesLo(tocH, &trashSpec, false, true, true, warnings);
    if (oldEzOpenSerialNum &&
        (sumNum = FindSumBySerialNum(tocH, oldEzOpenSerialNum)) >= 0 &&
        !(modifiers & shiftKey))
      Preview(tocH, sumNum);
    tocH->userActive = !(modifiers & shiftKey);
    return;
  }

  if (nuke && warnings && SelectedWarnings(tocH, true, true))
    return;

  // memory preflight
  count = CountSelectedMessages(tocH);
  if (item == MESSAGE_FORWARD_ITEM || item == MESSAGE_REDISTRIBUTE_ITEM ||
      item == MESSAGE_REPLY_ITEM || item == MESSAGE_SALVAGE_ITEM) {
    if (item == MESSAGE_REDISTRIBUTE_ITEM &&
        PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers, optionKey) && toWhom)
      ; // nevermind, turbo redirect
    else {
      size = count * 7 K; // account just for the windows
      if (item != MESSAGE_REPLY_ITEM || !(modifiers & shiftKey))
        size += SizeSelectedMessages(tocH, true);
      if (MemoryPreflight(size))
        return; // too much memory asked for

      if (count > GetRLong(WIN_GEN_WARNING_THRESH))
        if (!MultiMessageOpOK(WIN_GEN_WARNING, count))
          return;
    }
  }


  /*
   * progress stuff
   */
  switch (item) {
  case PERS_HIER_MENU:
    // No Personalities menu in Light
    if (HasFeature(featureMultiplePersonalities))
      gran = tocH->which == OUT ? 1 : 50;
    break;
  case MESSAGE_DELETE_ITEM:
    gran = 10;
    break;
  case STATE_HIER_MENU:
  case LABEL_HIER_MENU:
  case PRIOR_HIER_MENU:
  case TABLE_HIER_MENU:
  case SERVER_HIER_MENU:
    gran = 50;
    break;
  default:
    gran = 1;
    break;
  }

  if (tocH->imapTOC // some IMAP operations will open their own progress
                       // window
      && (item == MESSAGE_DELETE_ITEM // no progress for deletes from the
                                      // message menu, or server menu options
          || (item == SERVER_HIER_MENU)))
    ;
  else {
    if (count > gran)
      OpenProgress();
    ProgressMessageR(kpSubTitle, LEFT_TO_PROCESS);
    Progress(NoBar, count, NULL, NULL, NULL);
  }

  if (!nuke && item == MESSAGE_DELETE_ITEM)
    AddXfUndo(tocH, GetTrashTOC(), -1);

  for (sumNum = tocH->count - 1; sumNum >= 0; sumNum--) {
    if (tocH->sums[sumNum].selected) {
      Boolean doVirtualMB;
      TOCType * realTOC;
      short realSum;

      lastSelected = sumNum;
      MakeMessTitle(title, tocH, sumNum, false);
      if (count > 1)
        MiniEvents();
      if (CommandPeriod)
        break;
      if (!(--count % gran) || TickCount() - pTicks > 30) {
        Progress(NoBar, count, NULL, NULL, title);
        pTicks = TickCount();
      }
      realTOC = GetRealTOC(tocH, sumNum, &realSum);
      if (nuke && realTOC == tocH)
        SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum,
                        false, true);
      doVirtualMB = DoMessageMenu(item, tocH, sumNum, toWhom, addr, modifiers,
                                  nuke, &busy);
      if (!nuke && realTOC == tocH && doVirtualMB)
        //	Check for updates to search results
        SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum,
                        false, false);
      if (realTOC && realTOC != tocH && doVirtualMB)
        // do real mailbox also if working in virtual mailbox
        DoMessageMenu(item, realTOC, realSum, toWhom, addr, modifiers, nuke,
                      &busy);
      // hack: The  Delete from Server, Fetch Message Text, and Fetch
      // Attachments menu choices handle all selected messages for IMAP boxes
      if (tocH->imapTOC && (item == SERVER_HIER_MENU) &&
          ((toWhom == isvmDelete) || (toWhom == isvmFetchMessage) ||
           (toWhom == isvmFetchAttachments)))
        break;
    }
    MonitorGrow(true);
  }
  // out: // Unused label removed

  CloseProgress();
  ShowBoxSizes(tocH->win);
  if (busy)
    WarnUser(SENDING_WARNING, 0);
  if (!CommandPeriod) {
    BoxSelectAfter(tocH->win, lastSelected);
  }
  if (oldEzOpenSerialNum && !tocH->previewID &&
      (sumNum = FindSumBySerialNum(tocH, oldEzOpenSerialNum)) >= 0 &&
      !(modifiers & shiftKey))
    Preview(tocH, sumNum);
  CheckBox(GetWindowMyWindowPtr(FrontWindow_()), false);
}

/************************************************************************
 * DoMessageMenu - do something to a messages
 ************************************************************************/
bool DoMessageMenu(short item, TOCType * tocH, short sumNum, short toWhom,
                   void *addr, long modifiers, bool nuke, bool *busy) {
  MyWindowPtr win;
  Boolean doVirtualMB = true;
  unsigned char s[256];

  switch (item) {
  case MESSAGE_DELETE_ITEM:
    DeleteMessageLo(tocH, sumNum, nuke);
    break;
  case STATE_HIER_MENU:
    if (tocH->sums[sumNum].state == BUSY_SENDING)
      *busy = true;
    else
    {
      SetState(tocH, sumNum, toWhom);
      doVirtualMB = false; //	Already took care of this
    }
    break;
  case PERS_HIER_MENU:
    // No personalities menu in Light
    if (HasFeature(featureMultiplePersonalities))
      SetPers(tocH, sumNum,
              FindPersByName(MyGetItem(GetMHandle(item), toWhom, s)), true);
    break;
  case LABEL_HIER_MENU:
    SetSumColor(tocH, sumNum, Menu2Label(toWhom));
    doVirtualMB = false; //	Already took care of this
    break;
  case SERVER_HIER_MENU:
    ServerMenuChoice(tocH, sumNum, toWhom, (modifiers & shiftKey) != 0);
    break;
  case TABLE_HIER_MENU:
    SetMessTable(tocH, sumNum, toWhom);
    break;
  case FILE_MENU:
    ExportHTMLSum(tocH, sumNum);
    break;
  case PRIOR_HIER_MENU:
    SetPriority(tocH, sumNum, NewPrior(toWhom, tocH->sums[sumNum].priority));
    doVirtualMB = false; //	Already took care of this
    break;
  default:
    // must have the message before we can do anything here.
    // right now, fetch the entire message, including attachments, unless
    // we're just replying.
    if (EnsureMsgDownloaded(tocH, sumNum, item != MESSAGE_REPLY_ITEM)) {
      if ((win = GetAMessage(tocH, sumNum, NULL, NULL, false))) {
        WindowPtr winWP = GetMyWindowWindowPtr(win);
        switch (item) {
        case MESSAGE_SALVAGE_ITEM:
          if (!DoSalvageMessage(win, false))
            CommandPeriod = true;
          break;
        case MESSAGE_FORWARD_ITEM:
          if (!DoForwardMessage(win, addr, true))
            CommandPeriod = true;
          break;
        case MESSAGE_REPLY_ITEM: {
          bool all, quote, self;
          ReplyDefaults(modifiers, &all, &self, &quote);
          if (!DoReplyMessage(win, all, self, quote, true, toWhom, true, true,
                              true))
            CommandPeriod = true;
        } break;
        case MESSAGE_REDISTRIBUTE_ITEM:
          if (!DoRedistributeMessage(
                  win, addr,
                  PrefIsSetOrNot(PREF_TURBO_REDIRECT, modifiers, optionKey),
                  false, true))
            CommandPeriod = true;
          break;
        }
        if (!IsWindowVisible(winWP))
          CloseMyWindow(winWP);
        else
          NotUsingWindow(winWP);
      } else
        CommandPeriod = true;
    } else
      CommandPeriod = true;
    doVirtualMB = false;
    break;
  }
  return doVirtualMB;
}

/************************************************************************
 * ReplyDefaults - get the defaults for reply options
 ************************************************************************/
void ReplyDefaults(short modifiers, bool *all, bool *self, bool *quote) {
  *all = PrefIsSetOrNot(PREF_REPLY_ALL, modifiers, optionKey);
  *self = !PrefIsSet(PREF_NOT_ME);
  *quote = (modifiers & shiftKey) == 0;
}

/************************************************************************
 * BoxNextSelected - find first selection in a mailbox
 ************************************************************************/
short BoxNextSelected(TOCType * tocH, short afterNum) {
  int sNum, count;

  count = tocH->count;

  for (sNum = afterNum + 1; sNum < count; sNum++)
    if (tocH->sums[sNum].selected)
      return (sNum);

  return (-1);
}

/**********************************************************************
 * SaveTextAsMessage - save a text block as a message
 **********************************************************************/
OSErr SaveTextAsMessage(Handle preText, Handle text, TOCType * tocH,
                        long *fromLen) {
  long size = GetHandleSize(text);
  OSErr err;

  if (size && (*(char **)text)[size - 1] != '\015') {
    PtrPlusHand("\015", text, 1);
    size++;
  }
  err = SavePtrAsMessage(preText ? *preText : NULL,
                         preText ? GetHandleSize(preText) : 0, *text,
                         size, tocH, fromLen);

  return (err);
}

/**********************************************************************
 * SavePtrAsMessage - save a text block as a message
 **********************************************************************/
OSErr SavePtrAsMessage(UPtr preText, long preSize, UPtr text, long size,
                       TOCType * tocH, long *fromLen) {
  OSErr err;
  long eof;
  char name[256];
  LineIOD lid;
  MSumType sum;
  FSSpec spec;

  GetMailboxName(tocH, -1, (unsigned char *)name);

  /*
   * open mailbox and write the bytes
   */
  if (!(err = BoxFOpen(tocH))) {
    eof = FindTOCSpot(tocH, size);
    err = SetFPos(tocH->refN, fsFromStart, eof);
    if (!err)
      err = PutOutFromLine(tocH->refN, fromLen);
    if (!err && preText)
      err = AWrite(tocH->refN, &preSize, preText);
    if (!err)
      err = AWrite(tocH->refN, &size, text);
    if (!err)
      err = TruncAtMark(tocH->refN);
    TOCSetDirty(tocH, true);
    BoxFClose(tocH, true);
  }

  /*
   * did it work?
   */
  if (err) {
    FileSystemError(WRITE_MBOX, name, err);
    return (err);
  }

  /*
   * read it back
   */
  spec = GetMailboxSpec(tocH, -1);
  if ((err = OpenLine(spec.path, fsRdWrPerm, &lid)))
    return (FileSystemError(READ_MBOX, name, err));
  if ((err = SeekLine(eof, &lid)))
    return (FileSystemError(READ_MBOX, name, err));
  ReadSum(NULL, false, &lid, true);
  Zero(sum);
  if (!ReadSum(&sum, false, &lid, false))
    err = !SaveMessageSum(&sum, &tocH);
  else
    err = 1;
  CloseLine(&lid);
  return (err);
}

/************************************************************************
 * DoSalvageMessage - glean what you can from a bounced message's headers
 ************************************************************************/
MyWindowPtr DoSalvageMessage(MyWindowPtr win, bool forXfer) {
  return DoSalvageMessageLo(win, forXfer, false);
}

/************************************************************************
 * DoSalvageMessageLo - glean what you can from a bounced message's headers
 ************************************************************************/
MyWindowPtr DoSalvageMessageLo(MyWindowPtr win, bool forXfer, bool forIMAP) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
  MessHandle newMessH;
  MyWindowPtr newWin;
  WindowPtr newWinWP;
  unsigned char scratch[256];
  short field;
  HeadSpec oldHS, newHS;
  OSErr err = noErr;
  UHandle text;
  long newBo;
  bool html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);

  NicknameWatcherFocusChange(win->pte); /* MJN */

  /* variables used by salvage logic (moved here for consistent
   * scope/initialization) */
  unsigned char *spot = NULL, *oldSpot = NULL, *beginning = NULL, *end = NULL;
  long size = 0, total = 0;
  bool toFound = false;
  char received[64];
  long offset = 0;

  if (GetWindowKind(winWP) != COMP_WIN && !SaveMessHi(win, false))
    //	Make sure message has been downloaded if IMAP
    if (!EnsureMsgDownloaded(origMessH->tocH, origMessH->sumNum, true))
      return NULL;

  PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
  if ((newWin = DoComposeNew(0))) {
    long sigLen;

    newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
    XferCustomTable(origMessH, newMessH);
    if (GetWindowKind(winWP) == COMP_WIN ||
        MessFlagIsSet(origMessH, FLAG_OUT)) {
      // if (win->qWindow.windowKind!=COMP_WIN) AlignHeaders(origMessH);
      for (field = 0; !err && field <= ATTACH_HEAD; field++) {
        if (field != FROM_HEAD ||
            (!CompHeadGetStr(origMessH, field, (char *)scratch) &&
             (MessOptIsSet(origMessH, OPT_REDIRECTED) ||
              IsMe((char *)scratch)))) {
          if (CompHeadFind(origMessH, field, &oldHS)) {
            // If we have the body, the oldHS will not include the sig
            // This is bad for copying
            // However, we do need to remember this fact, so we can
            // make sure we lock things correctly later
            if (field == 0) {
              sigLen = gedit_document_get_length(
                           geditctrl_get_document(origMessH->bodyPTE)) -
                       (oldHS.offset + oldHS.length);
              oldHS.length += sigLen;
            }
            /* check for overlap */
            if (oldHS.offset < (oldHS.offset + oldHS.length)) {
              /*
               * If the old header is not empty, then we need to copy it
               * to the new message.
               */
              GetRString((char *)scratch, HEADER_STRN + field);
              if (field == SUBJ_HEAD && PrefIsSet(PREF_SUBJECT_IN_INLINE))
                err = CompHeadSetStr(newMessH->bodyPTE, &newHS,
                                     (char *)scratch);
              else {
                /* Copy header text from original to new message */
                void *rawText = NULL;
                PETEGetRawText(NULL, origMessH->bodyPTE, &rawText);
                if (rawText && oldHS.length > 0) {
                  err = PeteInsertPtr(newMessH->bodyPTE,
                                      newHS.offset + newHS.length,
                                      (const char *)rawText + oldHS.offset,
                                      oldHS.length);
                } else
                  err = 0;
              }
            }
            if (!err && CompHeadFind(newMessH, field, &newHS)) {
              /* Header locking: in GTK, header fields are managed by
               * CompHeadFind/CompHeadSet — no need for PETE-style range locking.
               * Signature and header regions are protected by the comp window
               * field structure rather than text range locks. */
            }
          }
        }
      }
    }
    TOCSetDirty(newMessH->tocH, true);
    /* label/color field not yet in MSumType — use 0 (no label) */
    SetSumColor(newMessH->tocH, newMessH->sumNum, 0);
    SumOf(newMessH)->flags = SumOf(origMessH)->flags;
    SumOf(newMessH)->opts = SumOf(origMessH)->opts;
    SumOf(newMessH)->sigId = SumOf(origMessH)->sigId;
    SumOf(newMessH)->priority = SumOf(origMessH)->priority;
    /* SumOf(newMessH)->origPriority = SumOf(origMessH)->priority; */
    if (GetWindowKind(winWP) != COMP_WIN &&
        MessOptIsSet(newMessH, OPT_INLINE_SIG))
      AddInlineSig(newMessH);

    if (GetWindowKind(winWP) == COMP_WIN && origMessH->hTranslators) {
      /* Duplicate the translator handle */
      Handle origH = (Handle)origMessH->hTranslators;
      long hSize = GetHandleSize(origH);
      Handle dup = NuHTempBetter(hSize);
      if (dup) memcpy(*dup, *origH, hSize);
      newMessH->hTranslators = (void *)dup;
    } else if (!GetRHeaderAnywhere(origMessH, HEADER_STRN + TRANSLATOR_HEAD,
                                   (char **)&text)) {
      /* Translator info from headers — not critical for GTK port */
      ZapHandle(text);
    }
  } else {

    if (SumOf(origMessH)->flags & FLAG_OUT) {
      SumOf(newMessH)->flags = SumOf(origMessH)->flags;
      SumOf(newMessH)->sigId = SumOf(origMessH)->sigId;
    }

    PeteGetTextAndSelection(origMessH->bodyPTE, (void **)&text, NULL, NULL);
    if (text) {
      beginning = spot = *text;
      total = size = GetHandleSize_(text);
      end = spot + size;
    } else {
      beginning = spot = end = NULL;
      total = size = 0;
    }

    if (forXfer || SumOf(origMessH)->flags & FLAG_OUT)
      oldSpot = NULL;
    else {
      /*
       * find the last "Received:" header
       */
      GetRString(received, RECEIVED_HEAD);
      TrimWhite((unsigned char *)received);
      oldSpot = NULL;
      while ((spot = (unsigned char *)FindHeaderString((char *)spot, received, &size, true))) {
        oldSpot = spot;
        spot += size;
        size = end - spot;
      }
    }

    /*
     * copy the relevant parts
     */
    if (!oldSpot && beginning)
      oldSpot = beginning;
    if (oldSpot != beginning && oldSpot && beginning)
      SumOf(newMessH)->sigId = SIG_NONE;
    if (oldSpot && beginning && total > 0) {
      size = total - (oldSpot - beginning);
      TextFindAndCopyHeader(oldSpot, size, newMessH, HeaderName(SUBJ_HEAD),
                            SUBJ_HEAD, 0);
      TextFindAndCopyHeader(oldSpot, size, newMessH, HeaderName(CC_HEAD),
                            CC_HEAD, 0);
      TextFindAndCopyHeader(oldSpot, size, newMessH, HeaderName(ATTACH_HEAD),
                            ATTACH_HEAD, 0);
      toFound = TextFindAndCopyHeader(oldSpot, size, newMessH,
                                      HeaderName(TO_HEAD), TO_HEAD, 0);
      TextFindAndCopyHeader(oldSpot, size, newMessH, HeaderName(BCC_HEAD),
                            BCC_HEAD, 0);
    }
    /*
     * find the body
     */
    if (oldSpot && size > 0) {
      for (spot = oldSpot; spot < oldSpot + size - 1; spot++)
        if (spot[0] == '\015' && spot[1] == '\015')
          break;
      spot += 2;
      offset = spot - beginning;
    } else {
      spot = NULL;
      offset = 0;
    }
    if (spot && oldSpot && size > 0 && spot < oldSpot + size - 1) {
      newBo = gedit_document_get_length(
          geditctrl_get_document(newMessH->bodyPTE));
      /* Copy body text from original to salvaged message */
      {
        long copyLen = total - (spot - beginning);
        if (copyLen > 0) {
          err = PeteInsertPtr(newMessH->bodyPTE, newBo,
                              (const char *)spot, copyLen);
        } else
          err = 0;
      }
    }
    if (MessFlagIsSet(origMessH, FLAG_HAS_ATT)) {
      CopyAttachments(newMessH);
      SpoolAttachments(newMessH);
    }
  }
  newWinWP = GetMyWindowWindowPtr(newWin);
  if (err && newWin) {
    NoSaves = true;
    CloseMyWindow(newWinWP);
    NoSaves = false;
    WarnUser(PETE_ERR, err);
    PopPers();
    return (NULL);
  }
  if (!forXfer || forIMAP) {
    WeedXAttachments(newMessH, !forIMAP);
    if (/* UseInlineSig */ false && !MessOptIsSet(newMessH, OPT_INLINE_SIG))
      AddInlineSig(newMessH);
    UpdateSum(newMessH, SumOf(newMessH)->offset, SumOf(newMessH)->length);
    if (!forIMAP) {
      ShowMyWindow(newWinWP);
      newWin->isDirty = false;
      PeteCleanList(newWin->pte);
    }
  }
  /* Brace removed */
  PopPers();
  return (newWin);
}

/************************************************************************
 * UniqueHeader - make sure the addresses in a header are unique
 ************************************************************************/
OSErr UniqueHeader(MessHandle messH, short head, bool wantErrors) {
  Handle addresses = NULL;
  short oldSize;
  HeadSpec hs;
  OSErr err = noErr;

  if (CompHeadFind(messH, head, &hs) &&
      !CompHeadGetText(TheBody, &hs, &addresses)) {
    oldSize = GetHandleSize_(addresses);
    err = NickUniq(addresses, (unsigned char *)", ", wantErrors);
    if (oldSize != GetHandleSize_(addresses))
      CompHeadSet(TheBody, &hs, (char *)addresses);
    ZapHandle(addresses);
  }
  return err;
}

/************************************************************************
 * RemoveSelf - Remove "me" from a list of addresses
 *	Ray Davison, SFU
 ************************************************************************/
OSErr RemoveSelf(MessHandle messH, short head, bool wantErrors) {
  unsigned char temp[256];
  EAL_VARS_DECL;
  UHandle rawMyself = NULL, cookedMyself = NULL;
  UHandle rawAddress = NULL, spewHandle = NULL;
  unsigned char *myself_data = g_malloc(1);
  UHandle myself = (UHandle)g_malloc(sizeof(unsigned char *));
  *myself = myself_data;
  long offset, meOffset;
  bool removed = false;
  Handle text = NULL;
  Handle oldText = NULL;
  HeadSpec hs;
  bool group;
  bool groupWas = false;
  OSErr err = noErr;

  /* Get a definition of who I am */

  GetRString((char *)temp, ME);
  PtrPlusHand_(temp, myself, strlen((char *)temp));
  PtrPlusHand_(",", myself, 1);
  GetReturnAddr(temp, true);
  PtrPlusHand_(temp, myself, strlen((char *)temp));
  if (!(err = SuckAddresses((void ***)&rawMyself, (void **)myself, false,
                            wantErrors, false, NULL)) &&
      !(err = ExpandAliasesLow((Handle *)&cookedMyself, (Handle)rawMyself, 0,
                               false, "",
                               EAL_VARS))) // no autoqual
  {
    ZapHandle(rawMyself);
    ZapHandle(myself);

    /* expand the text */

    if (CompHeadFind(messH, head, &hs) &&
        !CompHeadGetText(TheBody, &hs, &oldText)) {
      if (!(err = SuckAddresses((void ***)&rawAddress, (void **)oldText, true,
                                wantErrors, false, NULL)) &&
          (text = NuHTempBetter(0L))) {
        /* Remove myself from address */
        if (rawAddress && cookedMyself) {
          for (offset = 0; (*rawAddress)[offset];
               offset += (*rawAddress)[offset] + 2) {
            /* clean up the address */
            SuckPtrAddresses((void ***)&spewHandle, (*rawAddress) + offset + 1,
                             (*rawAddress)[offset], false, wantErrors, true,
                             NULL);
            if (spewHandle) {
              group = (*spewHandle)[**spewHandle] == ':';
              /* look for this in the "me" addresses */
              for (meOffset = 0; (*cookedMyself)[meOffset];
                   meOffset += (*cookedMyself)[meOffset] + 2) {
                if (StringSame((const char *)*spewHandle,
                               (const char *)(*cookedMyself) + meOffset))
                  break;
              }
              ZapHandle(spewHandle);
            }

            /* if we didn't find it, then add this address to the result */
            if (!(*cookedMyself)[meOffset]) {

              if (!groupWas && GetHandleSize_(text))
                PtrPlusHand_(", ", text, 2);
              PtrPlusHand_((*rawAddress) + offset + 1, text,
                           (*rawAddress)[offset]);
            } else
              removed = true;
            groupWas = group;
          }
        }
      }
    }
  }

  ZapHandle(oldText);
  ZapHandle(rawMyself);
  ZapHandle(cookedMyself);
  ZapHandle(rawAddress);
  ZapHandle(spewHandle);
  ZapHandle(myself);

  if (removed) {
    CompHeadSet(TheBody, &hs, (char *)*text);
    CompGatherRecipientAddresses(messH, true);
  }

  ZapHandle(text);
  return err;
}

/**********************************************************************
 * MessText - get the text of a message
 **********************************************************************/
UHandle MessText(MessHandle messH) {
  UHandle text = NULL;
  PeteGetRawText(TheBody, (void **)&text);
  return (text);
}

/**********************************************************************
 * MessVisibleText - get only the visible text of a message
 **********************************************************************/
UHandle MessVisibleText(MessHandle messH) {
  geditDocument *doc = geditctrl_get_document(TheBody);
  gchar *rawText = gedit_document_get_text(doc);
  if (!rawText)
    return NULL;

  /* Return a Handle (void **) wrapping the text string */
  long len = (long)strlen(rawText);
  Handle h = NuHTempBetter(len);
  if (h) {
    memcpy(*h, rawText, len);
  }
  g_free(rawText);
  return (UHandle)h;
}

/* MessHasGraphic Stub Removed from here */

/************************************************************************
 * ReopenMessage - reopen the current message
 ************************************************************************/
MyWindowPtr ReopenMessage(MyWindowPtr win) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  char *text;
  MessHandle messH = Win2MessH(win);
  OSErr err = noErr;

  text = GetMessText(Win2MessH(win));

  if (text) {
    // stick in the text
    {
      PeteSetTextPtr(TheBody, NULL, 0);
      PeteKillUndo(TheBody);
      PeteCalcOff(TheBody);

      /* (*PeteExtra(TheBody))->emoDesired = !MessFlagIsSet(messH,
       * FLAG_SHOW_ALL); */

      PetePlain(TheBody, kPETECurrentStyle, kPETECurrentStyle, peAllValid);
      PetePlainPara(TheBody, 0);

      if (!MessFlagIsSet(messH, FLAG_SHOW_ALL) &&
          (MessFlagIsSet(messH, FLAG_RICH)
           || MessOptIsSet(messH, OPT_HTML)
           || MessOptIsSet(messH, OPT_FLOW) ||
           MessOptIsSet(messH, OPT_CHARSET))) {
        geditDocument *doc = geditctrl_get_document(TheBody);
        gedit_document_insert_markup(doc, 0, text);
      } else {
        geditDocument *doc = geditctrl_get_document(TheBody);
        gedit_document_insert_text(doc, 0, text);
      }

      if (!err) {
        PeteSmallParas(TheBody);
        g_free(text);
        text = NULL;
        // align headers if need be
        // if (!PrefIsSet(PREF_DONT_ALIGN_HEADERS)) AlignHeaders(messH);

        /* HTML rendering is handled by InsertRichLo → InsertHTMLLo →
         * gedit_document_insert_markup() which parses tags and applies styles. */

        if (!err)
          HiliteOddReply(messH);

        if (!err)
          PeteTrimTrailingReturns(TheBody, true);

        if (!err) {
          // recalculate
          PeteCalcOn(TheBody);

          //	add notification control if necessary
          CheckAddNotifyControls(win, messH);

          if (PrefIsSet(PREF_ZOOM_OPEN))
            ReZoomMyWindow(winWP);

          // scroll to correct position
          if (!MessFlagIsSet(messH, FLAG_SHOW_ALL))
            ShowMessageSeparator(TheBody, true);
          InvalContent(win);
        }
      }
    }

    // mark document as clean
    PETEMarkDocDirty(PETE, TheBody, false);
    win->isDirty = false;
    PeteSetURLRescan(TheBody, 0);

    // kill text if still here
    g_free(text);
    text = NULL;

    if (err) {
      WarnUser(DOC_DAMAGED_ERR, err);
      CloseMyWindow(winWP);
      win = NULL;
    }
  } else {
    CloseMyWindow(winWP);
    win = NULL;
  }
  /* } Closing ReopenMessage - removed extra brace */

  if (win)
    PeteCalcOn(TheBody);

  return (win);
}

/************************************************************************
 * FindFrom - find a (nicely formatted) From address
 ************************************************************************/
void FindFrom(unsigned char *who, GtkWidget *pte) {
  char *found;
  char header[32];
  long len;

  char *text = gedit_document_get_text(geditctrl_get_document(pte));
  len = text ? (long)strlen(text) : 0;
  GetRString((unsigned char *)header, FROM_HEAD + HEADER_STRN);
  if ((found = FindHeaderString(text, header, &len, false))) {
    long copyLen = MIN(62, len);
    memcpy(who, found, copyLen);
    who[copyLen] = 0;
    BeautifyFrom(who);
  } else
    *who = 0;
  g_free(text);
}

/************************************************************************
 * QuoteLines - put a quote prefix before the specified TextEdit lines
 ************************************************************************/
void QuoteLines(GtkWidget *pte, long from, long to, short pfid, long *qEnd) {
  long this;
  unsigned char prefix[16];
  UHandle text;
  long count = 0;
  bool first = true;
  PETEStyleEntry pse;
  RGBColor color;
  bool withSpace = false;
  long numSpaces = 0;
  Byte quoteChar;

  Zero(pse);

  if (qEnd)
    *qEnd = to;

  GetRString(prefix, pfid);
  long prefixLen = strlen((char *)prefix);
  if (!prefixLen)
    return;

  // if trailing space, remove and note
  if (prefixLen == 2 && prefix[1] == ' ') {
    withSpace = true;
    prefix[1] = '\0';
    prefixLen = 1;
    quoteChar = prefix[0];
  }

  PETEGetRawText(PETE, pte, &text);
  this = GetHandleSize(text);
  to = MIN(to, this);

  for (this = to - 2; this >= from; this --)
    if (!this || (*text)[this] == '\015') {
      if (withSpace && (*text)[this + 1] != quoteChar) {
        PeteInsertChar(pte, this ? this + 1 : this, ' ', NULL);
        numSpaces++;
      }
      PeteInsertPtr(pte, this ? this + 1 : this, prefix, prefixLen);
      count++;
    }
  if (qEnd)
    *qEnd = to + count * prefixLen + numSpaces; // adjust for inserted prefixes

  // do the scanner's work for it
  if (!Black(GetRColor(&color, QUOTE_COLOR))) {
    /* pse.psStyle.textStyle.tsLabel = pQuoteLabel; */
    PETESetTextStyle(PETE, pte, from, to + count * prefixLen,
                     &pse.psStyle.textStyle, peLabelValid);
  }
}

/************************************************************************
 * PrependMessText - stick some text before one of the fields of a message.
 ************************************************************************/
OSErr PrependMessText(MessHandle messH, short whichTXE, UPtr string,
                      long size) {
  HeadSpec hs;

  if (CompHeadFind(messH, whichTXE, &hs))
    return (CompHeadPrependPtr(TheBody, &hs, string, size));
  else
    return (fnfErr);
}

static inline TOCType * Win2TOC(void *win) { return NULL; }

/************************************************************************
 * FindHeaderString - pick the given header out of a message, by name
 *  Returns pointer to header value within text, sets *size to value length.
 *  Handles \r\n, \r, and \n line endings (RFC 2822 + legacy formats).
 *  Supports RFC 2822 header continuation (folded headers with leading whitespace).
 ************************************************************************/
char *FindHeaderString(char *text, char *headerName, long *size, bool bodyToo) {
  char *end = text + *size;
  char *lineStart = text;
  long nameLen = strlen(headerName);

  /* Skip leading blank lines — mbox offsets can include trailing newlines
   * from the previous message */
  while (lineStart < end && (*lineStart == '\r' || *lineStart == '\n'))
    lineStart++;

  while (lineStart < end) {
    /* Find end of this logical line (skip continuation lines with leading whitespace) */
    char *lineEnd = lineStart;
    for (;;) {
      /* Find end of current physical line */
      while (lineEnd < end && *lineEnd != '\r' && *lineEnd != '\n')
        lineEnd++;
      /* Skip the line ending */
      char *nextLine = lineEnd;
      if (nextLine < end && *nextLine == '\r') nextLine++;
      if (nextLine < end && *nextLine == '\n') nextLine++;
      /* Check for continuation (next line starts with whitespace) */
      if (nextLine < end && IsWhite(*nextLine)) {
        lineEnd = nextLine; /* include continuation in this logical line */
        continue;
      }
      break;
    }

    /* Empty line = end of headers */
    if (lineEnd == lineStart && !bodyToo)
      break;

    /* Check for header name match (case-insensitive).
     * headerName includes the colon, e.g. "To:", "From:" */
    if ((lineEnd - lineStart) >= nameLen &&
        strncasecmp(lineStart, headerName, nameLen) == 0) {
      /* Found it — extract value after "Header:" */
      char *val = lineStart + nameLen;
      while (val < lineEnd && IsWhite(*val))
        val++;
      /* Trim trailing whitespace */
      char *valEnd = lineEnd - 1;
      while (valEnd >= val && (IsWhite(*valEnd) || *valEnd == '\r' || *valEnd == '\n'))
        valEnd--;
      *size = MAX(0, valEnd - val + 1);
      return val;
    }

    /* Advance past line ending */
    char *nextLine = lineEnd;
    if (nextLine < end && *nextLine == '\r') nextLine++;
    if (nextLine < end && *nextLine == '\n') nextLine++;
    lineStart = nextLine;
  }
  return NULL;
}
/************************************************************************
 * DoReplyMessage - craft a reply to a message
 ************************************************************************/
MyWindowPtr DoReplyMessage(MyWindowPtr win, bool all, bool self, bool quote,
                           bool doFcc, short withWhich, bool vis, bool station,
                           bool caching) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
  MessHandle newMessH;
  unsigned char subj[256], scratch[256], replyTo[256];
  long bodyOffset = -1;
  MyWindowPtr newWin = NULL;
  WindowPtr newWinWP;
  short r;
  long len;
  gchar *text = NULL;
  long origLen;
  long selStart, selEnd;
  unsigned char soundName[256];
  bool doSound;
  HeadSpec hs;
  OSErr err = noErr;
  geditStyleRun style;
  long newBo;
  bool rich;
  bool html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);
  GtkWidget *copyFromPTE;
  TOCType * origTOCH;
  bool listReply =
      MessOptIsSet(origMessH, OPT_BULK) && PrefIsSet(PREF_LIST_REPLYTO);
  bool cacheReplyTo = !PrefIsSet(PREF_NICK_CACHE) &&
                      !PrefIsSet(PREF_NICK_CACHE_NOT_ADD_REPLY_TO);
  bool wantErrors = true;

  // Dunno why this was done
  // if (GetWindowKind(winWP)!=COMP_WIN && !SaveMessHi(win,false))
  // return(NULL);

  //	Make sure message has been downloaded if IMAP.  Don't care about
  // attachments.
  if (!EnsureMsgDownloaded(origMessH->tocH, origMessH->sumNum, false))
    return NULL;

  PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
  if ((newWin = DoComposeNew(0))) {
    newWinWP = GetMyWindowWindowPtr(newWin);
    newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
    replyTo[0] = 0;
    rich = UseFlowOutExcerpt || (MessIsRich(origMessH) || MessIsRich(newMessH));

    XferCustomTable(origMessH, newMessH);

    {
      geditDocument *doc = geditctrl_get_document(origMessH->bodyPTE);
      text = gedit_document_get_text(doc);
      GtkTextBuffer *buf = gedit_document_get_buffer(doc);
      GtkTextIter start, end;
      if (gtk_text_buffer_get_selection_bounds(buf, &start, &end)) {
        selStart = gtk_text_iter_get_offset(&start);
        selEnd = gtk_text_iter_get_offset(&end);
      } else {
        selStart = selEnd = -1;
      }
      origLen = gedit_document_get_length(doc);
    }

    /* handle the subject */
    FindAndCopyHeader(origMessH, newMessH, HeaderName(SUBJ_HEAD), SUBJ_HEAD);
    GetRString(scratch, REPLY_INTRO);
    TrimWhite(scratch);
    CompHeadGetStr(newMessH, SUBJ_HEAD, subj);
    if (!ReMatch(subj, scratch)) {
      GetRString(scratch, REPLY_INTRO);
      PrependMessText(newMessH, SUBJ_HEAD, scratch, strlen((const char *)scratch));
    }

    /* reply to sender */
    replyTo[0] = 0;
    if (listReply && !all) {
      FindAndCopyHeader(origMessH, newMessH, HeaderName(FROM_HEAD), TO_HEAD);
      if (HasFeature(featureNicknameWatching) && cacheReplyTo)
        SuckHeaderText(origMessH, replyTo, sizeof(replyTo), FROM_HEAD);
    } else
      for (r = 1; *GetRString(scratch, ReplyStrn + r); r++) {
        TrimWhite(scratch);
        if (FindAndCopyHeader(origMessH, newMessH, scratch, TO_HEAD)) {
          doSound = MessOptIsSet(origMessH, OPT_WEIRD_REPLY);
          if (!doSound)
            listReply = false; // as you were
          // (jp) added to grab the reply to address
          if (!PrefIsSet(PREF_NO_SPOKEN_WARNINGS) || cacheReplyTo)
            SuckHeaderText(newMessH, replyTo, sizeof(replyTo), TO_HEAD);
          break;
        }
      }
    if (caching && HasFeature(featureNicknameWatching) && cacheReplyTo &&
        replyTo[0])
      CacheRecentNickname(replyTo);

    /* bring over the other recipients and cc's, if desired */
    if (all) {
      FindAndCopyHeader(origMessH, newMessH, HeaderName(TO_HEAD),
                        PrefIsSet(PREF_CC_REPLY) ? CC_HEAD : TO_HEAD);
      FindAndCopyHeader(origMessH, newMessH, HeaderName(CC_HEAD), CC_HEAD);

      // If we've found a reply-to header and we have a mailing list, also
      // copy From to the cc: field
      if (doSound && listReply)
        FindAndCopyHeader(origMessH, newMessH, HeaderName(FROM_HEAD), CC_HEAD);

      /* remove self, if desired */
      if (!self) {
        if (RemoveSelf(newMessH, TO_HEAD, wantErrors))
          wantErrors = false;
        if (RemoveSelf(newMessH, CC_HEAD, wantErrors))
          wantErrors = false;
      }

      if (UniqueHeader(newMessH, TO_HEAD, wantErrors))
        wantErrors = false;
      UniqueHeader(newMessH, CC_HEAD, wantErrors);
    }

    /*
     * for Larry's benefit
     */
    if (PrefIsSet(PREF_NEWSGROUP_HANDLING))
      CopyNewsgroups(origMessH, newMessH);

    /*
     * auto-fcc
     */
    // Folder Carbon Copy - don't allow the auto FCC in Light
    if (HasFeature(featureFcc) && doFcc && PrefIsSet(PREF_AUTO_FCC) &&
        !origMessH->tocH->which &&
        !IMAPDontAutoFccMailbox(origMessH->tocH))
    {
      FSSpec spec = GetMailboxSpec(origMessH->tocH, -1);
      Fcc(newMessH, &spec);
    }

    /*
     * in-reply-to and references
     */
    ReplyReferences(origMessH, newMessH);

    /*
     * copy the body.  Use the preview pane if that's what we have
     */
    copyFromPTE = origMessH->bodyPTE;
    origTOCH = origMessH->openedFromTocH ? origMessH->openedFromTocH
                                            : origMessH->tocH;
    if (origTOCH->win == GetWindowMyWindowPtr(FrontWindow_()) &&
        origTOCH->previewID &&
        origTOCH->previewID == origMessH->openedFromSerialNum && !quote) {
      /* If the preview pane has a selection, use it as the source */
      geditDocument *pdoc = geditctrl_get_document(origTOCH->previewPTE);
      GtkTextBuffer *pbuf = gedit_document_get_buffer(pdoc);
      GtkTextIter pstart, pend;
      if (gtk_text_buffer_get_selection_bounds(pbuf, &pstart, &pend)) {
        copyFromPTE = origTOCH->previewPTE;
        text = gedit_document_get_text(pdoc);
        selStart = gtk_text_iter_get_offset(&pstart);
        selEnd = gtk_text_iter_get_offset(&pend);
      }
    }

    if (quote) {
      bodyOffset = 0;
      while (bodyOffset < origLen - 1) {
        if (text[bodyOffset + 1] != '\n')
          bodyOffset += 2;
        else if (text[bodyOffset] == '\n')
          break;
        else
          bodyOffset++;
      }

      while (bodyOffset < origLen && text[bodyOffset] == '\n')
        bodyOffset++;
      len = origLen - bodyOffset;
    } else {
      if (selEnd != selStart) {
        bodyOffset = selStart;
        len = selEnd - bodyOffset;
      }
    }

    if (BUG14) {
      InvalContent(win);
      UpdateMyWindow(winWP);
    }

    if (len > 0 && bodyOffset >= 0) {
      while (len && text[bodyOffset + len - 1] == '\n')
        len--;
      newBo = gedit_document_get_length(
          geditctrl_get_document(newMessH->bodyPTE));
      if (len > 0)
        err = gedit_document_copy_range(
            geditctrl_get_document(copyFromPTE), bodyOffset, len,
            geditctrl_get_document(newMessH->bodyPTE), newBo, FALSE);
      /* original code unlocked a handle here (HUnlock); no-op for
       * gedit_document */
      /* style.tsLock = 0; Removed */
      if (BUG14) {
        InvalContent(win);
        UpdateMyWindow(winWP);
      }
      PETESetTextStyle(PETE, newMessH->bodyPTE, newBo, 0x7fffffff, &style,
                       0 /* peLockValid */);
      if (rich) {
        geditDocument *_doc = geditctrl_get_document(newMessH->bodyPTE);
        gint _end = gedit_document_get_length(_doc) - 1;
        if (_end > newBo)
          gedit_document_set_quote_level(_doc, newBo, _end - newBo, 1);
      }

      if (BUG14) {
        InvalContent(win);
        UpdateMyWindow(winWP);
      }
      if (err) {
        NoSaves = true;
        CloseMyWindow(newWinWP);
        NoSaves = false;
        PopPers();
        WarnUser(PETE_ERR, err);
        return (NULL);
      }

      /*
       * make sure the last line is blank
       */
      EnsureMessNewline(newMessH);

      if (BUG14) {
        InvalContent(win);
        UpdateMyWindow(winWP);
      }
      if (rich) {
        geditDocument *_doc = geditctrl_get_document(newMessH->bodyPTE);
        gint _off = gedit_document_get_length(_doc);
        gint _ls = gedit_document_find_line_start(_doc, _off);
        gint _le = gedit_document_find_line_end(_doc, _off);
        gedit_document_set_quote_level(_doc, _ls, _le - _ls, 0);
      }
      if (BUG14) {
        InvalContent(win);
        UpdateMyWindow(winWP);
      }

      /*
       * quote them
       */
      CompHeadFind(newMessH, 0,
                   &hs); /* HeadSpec map: start->offset, stop->offset+length */
      if (hs.offset != -1) {
        if (!rich) {
          QuoteLines(newMessH->bodyPTE, hs.offset, hs.offset + hs.length - 1,
                     QUOTE_PREFIX, &len);
          hs.length += len;
        } else if (PrefIsSet(PREF_SCHEERDER)) {
          /* Select the quoted range in the editor */
          GtkTextBuffer *_buf = gedit_document_get_buffer(
              geditctrl_get_document(newMessH->bodyPTE));
          GtkTextIter _s, _e;
          gtk_text_buffer_get_iter_at_offset(_buf, &_s, hs.offset);
          gtk_text_buffer_get_iter_at_offset(_buf, &_e, hs.offset + hs.length);
          gtk_text_buffer_select_range(_buf, &_s, &_e);
        }
      }
      if (BUG14) {
        InvalContent(win);
        UpdateMyWindow(winWP);
      }

      /*
       * annotate
       */
      if (rich)
        Attribute(QUOTH, origMessH, newMessH, false);
      Attribute(all ? ATTRIBUTION : REP_SEND_ATTR, origMessH, newMessH, false);
      if (rich)
        Attribute(UNQUOTH, origMessH, newMessH, true);
    }

    if (BUG14) {
      InvalContent(win);
      UpdateMyWindow(winWP);
    }
    /*
     * copy priority (not my idea)
     */
    if (!PrefIsSet(PREF_NO_XF_PRIOR))
      /* SumOf(newMessH)->origPriority = SumOf(newMessH)->priority =
          SumOf(origMessH)->origPriority; */
      SumOf(newMessH)->priority = SumOf(origMessH)->priority;

    /* SumOf(newMessH)->outType = OUT_REPLY; */

    /*
     * encryption
     */
    if (MessOptIsSet(origMessH, OPT_WIPE))
      SetMessFlag(newMessH, FLAG_ENCRYPT);

    if (station) {
      if (!withWhich)
        ApplyDefaultStationery(newWin, true, true);
      // Stationery - no support for this in Light
      else if (HasFeature(featureStationery))
        ApplyIndexStationery(newWin, withWhich, true, true);
    }

    /* (*PeteExtra(newMessH->bodyPTE))->quoteScanned =
        (*PeteExtra(origMessH->bodyPTE))->quoteScanned; */ // don't need to run the scanner, quotelines did it already

    if (vis) {
      // shall we select the quote?
      quote = quote || selEnd - selStart >= GetRLong(OPEN_AT_END_THRESH);
      newMessH->openToEnd = !quote;

      AccuAddLong(&newMessH->aSourceMID, SumOf(origMessH)->uidHash);
      AccuAddLong(&newMessH->aSourceMID, SumOf(origMessH)->state);
      AccuAddLong(&newMessH->aSourceMID, REPLIED);
      SetState(origMessH->tocH, origMessH->sumNum, REPLIED);
      UpdateSum(newMessH, SumOf(newMessH)->offset, SumOf(newMessH)->length);
      ShowMyWindow(newWinWP);
      UpdateMyWindow(newWinWP);
      if (doSound)
        if (PrefIsSet(PREF_NO_SPOKEN_WARNINGS))
          PlayNamedSound(GetRString(soundName, REPLY_SOUND));
        else
          (void)ComposeStdAlert(kAlertNoteAlert, PASSIVE_REPLY_TO_ASTR,
                                replyTo);
    }
    newWin->isDirty = false;
    gtk_text_buffer_set_modified(
        gedit_document_get_buffer(geditctrl_get_document(newWin->pte)), FALSE);
  }
  PopPers();
  return (newWin);
}

/**********************************************************************
 * ReplyReferences
 **********************************************************************/
OSErr ReplyReferences(MessHandle origMessH, MessHandle newMessH) {
  OSErr err = noErr;
  UHandle /*t1=NULL, */ t2 = NULL, t3 = NULL;
  Accumulator extras = newMessH->extras;
  long origLen = extras.offset;

  /* Get the bodies of all three headers */
  //	GetRHeaderAnywhere(origMessH,HeaderStrn+IN_REPLY_TO_HEAD,&t1);
  GetRHeaderAnywhere(origMessH, HeaderStrn + REFERENCES_HEAD, &t2);
  GetRHeaderAnywhere(origMessH, HeaderStrn + MSGID_HEAD, &t3);

  /* Strip returns - I don't know why we do this for IRT and MID, but whatever
   */
  /* Should probably strip anything that's not an MID (like phrases) */
  //	if (t1)
  //	{
  //		SetHandleSize(t1,RemoveChar('\015',LDRef(t1),GetHandleSize(t1)));
  //		UL(t1);
  //	}
  if (t2) {
    SetHandleSize(t2, RemoveChar('\015', *t2, GetHandleSize(t2)));
  }
  if (t3) {
    SetHandleSize(t3, RemoveChar('\015', *t3, GetHandleSize(t3)));
  }

  /* If there's a message ID, add IRT header */
  if (t3) {
    /* Add the header and the colon */
    if (!(err = AccuAddRes(&extras, HeaderStrn + IN_REPLY_TO_HEAD)))
      /* Add a space */
      if (!(err = AccuAddChar(&extras, ' ')))
        /* Add the message ID */
        if (!(err = AccuAddHandle(&extras, t3)))
          /* Add a return */
          err = AccuAddChar(&extras, '\015');
  }

  /* If there's any id's (from Refs, IRT, or MID), add Refs header */
  if (!err && (/*t1 || */ t2 || t3)) {
    /* Add the header and the colon */
    if (!(err = AccuAddRes(&extras, HeaderStrn + REFERENCES_HEAD)))
      /* Add a space */
      if (!(err = AccuAddChar(&extras, ' ')))
        /* Add the Refs message IDs if they exist */
        if (!t2 || !(err = AccuAddHandle(&extras, t2))) {
          //					/* Add the IRT message ID if it
          // exists */ 					if (t1 /* && t1
          // does not appear in t2 */)
          //						/* Add a space if there
          // were Refs message IDs */
          // if (!t2 || !(err = AccuAddChar(&extras,'
          //'))) 							err =
          // AccuAddHandle(&extras,t1);

          /* Add the MID if it exists */
          if (!err && t3)
            /* Add a space if there were Refs or IRT message IDs */
            if ((/*!t1 && */ !t2) || !(err = AccuAddChar(&extras, ' ')))
              err = AccuAddHandle(&extras, t3);

          /* Add a return */
          if (!err)
            err = AccuAddChar(&extras, '\015');
        }
  }

  if (err)
    extras.offset = origLen;
  AccuTrim(&extras);
  newMessH->extras = extras;
  //	ZapHandle(t1);
  ZapHandle(t2);
  ZapHandle(t3);
  return (err);
}
/************************************************************************
 * EnsureMessNewline - make sure a message ends in a newline
 ************************************************************************/
OSErr EnsureMessNewline(MessHandle messH) {
  Handle text;
  long size;
  OSErr err;

  text = (Handle)gedit_document_get_text(geditctrl_get_document(TheBody));
  err = noErr;
  if (text) {
    size = strlen((char *)text);
    if (size < 1 || ((char *)text)[size - 1] != '\015')
      gedit_document_insert_text(
          geditctrl_get_document(TheBody),
          gedit_document_get_length(geditctrl_get_document(TheBody)),
          "\015\015");
    else if (size < 2 || ((char *)text)[size - 2] != '\015')
      gedit_document_insert_text(
          geditctrl_get_document(TheBody),
          gedit_document_get_length(geditctrl_get_document(TheBody)), "\015");
    /* TODO: replace PETEInsertParaPtr semantics with paragraph API if needed */
    MessPlainBytes(messH, 0, -1);
  }
  return err;
}

/************************************************************************
 * Attribute - make an attribution
 ************************************************************************/
void Attribute(short attrId, MessHandle origMessH, MessHandle newMessH,
               bool atEnd) {
  unsigned char attribution[256];

  if (*GrabAttribution(attrId, origMessH->win, attribution)) {
    long attrLen = strlen((const char *)attribution);
    if (atEnd) {
      geditDocument *_doc = geditctrl_get_document(newMessH->bodyPTE);
      long endOfText = gedit_document_get_length(_doc);
      gchar *_tmp = g_strndup((const gchar *)attribution, attrLen);
      gedit_document_insert_text(_doc, endOfText - 1, _tmp);
      g_free(_tmp);
      // if (MessIsRich(newMessH) || MessIsRich(origMessH))	//quote color
      // leaking, so disable check
      MessPlainBytes(newMessH, 0, -(short)attrLen - 1);
    } else {
      { size_t _l = strlen((const char *)attribution); attribution[_l] = '\015'; attribution[_l+1] = '\0'; }
      attrLen = strlen((const char *)attribution);
      PrependMessText(newMessH, 0, attribution, attrLen);
      // if (MessIsRich(newMessH) || MessIsRich(origMessH))	//quote color
      // leaking, so disable check
      MessPlainBytes(newMessH, 0, (short)attrLen);
    }
  }
}

/************************************************************************
 * GrabAttribution - compute the attribution for a message
 ************************************************************************/
PStr GrabAttribution(short attrId, MyWindowPtr win, PStr attribution) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  unsigned char template[256];
  unsigned char date[64], time[64];
  unsigned char who[128];
  long secs;
  long zone;
  long sumNum;
  MSumType sum;

  *attribution = 0;
  if (IsMessWindow(winWP))
    sum = *SumOf(Win2MessH(win));
  else if (GetWindowKind(winWP) == MBOX_WIN &&
           (0 <= (sumNum = LastMsgSelected(Win2TOC(win)))))
    sum = Win2TOC(win)->sums[sumNum];
  else
    return (attribution);

  GetRString(template, attrId);
  if (*template) {
    FindFrom(who, win->pte);
    if (*who) {
      zone = /* whine, whine, whine PrefIsSet(PREF_LOCAL_DATE) ? ZoneSecs() :
              */
          60 * sum.origZone;
      secs = sum.seconds + zone;
      TimeString(secs, false, attribution, NULL);
      FormatZone(date, zone);
      ComposeRString(time, ATTR_TIME_FMT, attribution, date);
      DateString(secs, shortDate, date, NULL);
      if (date[1] == optSpace)
        date[1] = ' ';
      utl_PlugParams(template, attribution, who, date, sum.subj, time);
    }
  }
  return (attribution);
}

/************************************************************************
 * DoRedistributeMessage - craft a reply to a message
 ************************************************************************/
MyWindowPtr DoRedistributeMessage(MyWindowPtr win, void *toWhom, bool turbo,
                                  bool andDelete, bool showIt) {
  TextAddrHandle addr = (TextAddrHandle)toWhom;
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
  MessHandle newMessH;
  unsigned char scratch[256];
  int bodyOffset;
  MyWindowPtr newWin;
  long newBo;
  OSErr err;
  PETETextStyle style;
  bool html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);

  if (GetWindowKind(winWP) != COMP_WIN && !SaveMessHi(win, false))
    return (NULL);

  //	Make sure message has been downloaded if IMAP
  if (!EnsureMsgDownloaded(origMessH->tocH, origMessH->sumNum, true))
    return NULL;

  PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));

  if ((newWin = DoComposeNew(0))) {
    WindowPtr newWinWP = GetMyWindowWindowPtr(newWin);
    newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
    XferCustomTable(origMessH, newMessH);
    SetMessOpt(newMessH, OPT_REDIRECTED);
    SetMessText(newMessH, FROM_HEAD, "", 0);
    FindAndCopyHeader(origMessH, newMessH, HeaderName(SUBJ_HEAD), SUBJ_HEAD);
    FindAndCopyHeader(origMessH, newMessH, HeaderName(FROM_HEAD), FROM_HEAD);
    if (MessFlagIsSet(origMessH, FLAG_OUT))
      FindAndCopyHeader(origMessH, newMessH, HeaderName(ATTACH_HEAD),
                        ATTACH_HEAD);
    RedirectAnnotation(newMessH);

    bodyOffset = CompBodyOffset(origMessH);
    {
      geditDocument *_srcDoc = geditctrl_get_document(origMessH->bodyPTE);
      geditDocument *_dstDoc = geditctrl_get_document(newMessH->bodyPTE);
      newBo = gedit_document_get_length(_dstDoc);
      gint _srcLen = gedit_document_get_length(_srcDoc);
      if (BUG14)
        UpdateMyWindow(winWP);
      err = gedit_document_copy_range(_srcDoc, bodyOffset, _srcLen - bodyOffset,
                                      _dstDoc, newBo, FALSE);
    }
    if (BUG14)
      UpdateMyWindow(winWP);
    /* style.tsLock = PrefIsSet(PREF_LOCK_REDIR) ? peModLock : peNoLock; */
    if (BUG14)
      UpdateMyWindow(winWP);
    PETESetTextStyle(PETE, newMessH->bodyPTE, newBo, 0x7fffffff, &style,
                     0 /* peLockValid */);
    if (BUG14)
      UpdateMyWindow(winWP);

    if (err) {
      NoSaves = true;
      CloseMyWindow(newWinWP);
      NoSaves = false;
      WarnUser(PETE_ERR, err);
      PopPers();
      return (NULL);
    }
    if (SumOf(origMessH)->state != SENT && SumOf(origMessH)->state != UNSENT)
      SumOf(newMessH)->sigId = SIG_NONE;

    /*
     * copy priority
     */
    /* SumOf(newMessH)->origPriority = SumOf(newMessH)->priority =
        SumOf(origMessH)->origPriority; */
    SumOf(newMessH)->priority = SumOf(origMessH)->priority;

    /* SumOf(newMessH)->outType = OUT_REDIRECT; */

    /*
     * state stuff
     */
    AccuAddLong(&newMessH->aSourceMID, SumOf(origMessH)->uidHash);
    AccuAddLong(&newMessH->aSourceMID, SumOf(origMessH)->state);
    AccuAddLong(&newMessH->aSourceMID, REDIST);
    SetState(origMessH->tocH, origMessH->sumNum, REDIST);
    UpdateSum(newMessH, SumOf(newMessH)->offset, SumOf(newMessH)->length);
    WeedXAttachments(newMessH, true);
    if (MessFlagIsSet(origMessH, FLAG_HAS_ATT))
      CopyAttachments(newMessH);
    SpoolAttachments(newMessH);
    {
      ShowMyWindow(newWinWP);
      newWin->isDirty = false;
      gtk_text_buffer_set_modified(
          gedit_document_get_buffer(geditctrl_get_document(newWin->pte)),
          FALSE);
    }
  }
  PopPers();
  return (newWin);
}

/**********************************************************************
 * RedirectAnnotation - add proper annotation to redirect
 **********************************************************************/
OSErr RedirectAnnotation(MessHandle messH) {
  unsigned char scratch[256], who[256], orig[256];
  OSErr err = noErr;

  CompHeadGetStr(messH, FROM_HEAD, orig);
  if (!IsMe(orig)) {
    // Trim the trailing comment
    long origLen = strlen((char *)orig);
    UPtr spot = orig + origLen;
    if (origLen > 0 && spot[-1] == ')') {
      spot--;
      while (spot > orig && *spot != '(' && *spot != '<')
        spot--;
      if (spot > orig && *spot == '(') {
        spot[-1] = '\0';
      }
    }

    // Now massage
    /* Fixed GetRealname Indirection */
    if (GetRealname(who)) {
      ComposeRString(scratch, REDIST_ANNOTATE, who);
      TrimWhite(orig);
      PCat(orig, scratch);
      err = SetMessText(messH, FROM_HEAD, orig, strlen((char *)orig));
    }
  }
  return err;
}

/**********************************************************************
 * CopyAttachments - copy attachments out of the message body and into
 *  the attachments line
 **********************************************************************/
OSErr CopyAttachments(MessHandle messH) {
  long offset, absOffset;
  FSSpec attSpec;
  HeadSpec hs;

  geditDocument *_doc = geditctrl_get_document(TheBody);
  gchar *_rawText = gedit_document_get_text(_doc);
  if (_rawText) {
    /* Work with the text as a simple C string for attachment scanning */
    UPtr textPtr = (UPtr)_rawText;
    long textLen = (long)strlen(_rawText);
    CompHeadFind(messH, 0, &hs);
    for (offset = 0; 0 <= (absOffset = FindAnAttachment(
                               (UHandle)&textPtr, offset + hs.offset, &attSpec,
                               false, NULL, NULL, NULL));) {
      offset = absOffset - hs.offset - 1;
      UPtr spot;
      for (spot = textPtr + absOffset;
           *spot != '\n' && spot < textPtr + hs.offset + hs.length; spot++)
        ;
      /* Delete the attachment text from the document */
      gedit_document_delete_range(_doc, absOffset,
                                  (spot - textPtr) - absOffset);
      /* Re-fetch text after deletion */
      g_free(_rawText);
      _rawText = gedit_document_get_text(_doc);
      textPtr = (UPtr)_rawText;
      CompHeadFind(messH, 0, &hs);
    }
    for (offset = 0; 0 <= (absOffset = FindAnAttachment(
                               (UHandle)&textPtr, offset + hs.offset, &attSpec,
                               true, NULL, NULL, NULL));) {
      offset = absOffset - hs.offset - 1;
      UPtr spot;
      for (spot = textPtr + absOffset;
           spot < textPtr + hs.offset + hs.length && *spot != '\n'; spot++)
        ;
      gedit_document_delete_range(_doc, absOffset,
                                  (spot - textPtr) - absOffset);
      g_free(_rawText);
      _rawText = gedit_document_get_text(_doc);
      textPtr = (UPtr)_rawText;
      CompHeadFind(messH, 0, &hs);
    }
    g_free(_rawText);
  }
  return noErr;
}

/************************************************************************
 * DoForwardMessage - forward a message to someone
 ************************************************************************/
MyWindowPtr DoForwardMessage(MyWindowPtr win, void *toWhom, bool turbo) {
  bool showIt = !turbo;
  TextAddrHandle addr = (TextAddrHandle)toWhom;
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  MessHandle origMessH = (MessHandle)GetMyWindowPrivateData(win);
  MessHandle newMessH;
  MyWindowPtr newWin = NULL;
  WindowPtr newWinPtr;
  unsigned char scratch[256], subj[256];
  long offset;
  PETETextStyle style;
  bool rich;
  long origBo;
  HeadSpec hs;
  bool html = !PrefIsSet(PREF_SEND_ENRICHED_NEW);

  NicknameWatcherFocusChange(win->pte); /* MJN */

  if (GetWindowKind(winWP) != COMP_WIN && !SaveMessHi(win, false))
    return (NULL);

  //	Make sure message has been downloaded if IMAP
  if (!EnsureMsgDownloaded(origMessH->tocH, origMessH->sumNum, true))
    return NULL;

  PushPers(PERS_FORCE(MESS_TO_PERS(origMessH)));
  if ((newWin = DoComposeNew(0))) {
    newWinPtr = GetMyWindowWindowPtr(newWin);
    if (BUG14) {
      ShowMyWindow(newWinPtr);
      UpdateMyWindow(newWinPtr);
    }
    newMessH = (MessHandle)GetMyWindowPrivateData(newWin);
    rich = UseFlowOutExcerpt || (MessIsRich(origMessH) || MessIsRich(newMessH));

    XferCustomTable(origMessH, newMessH);
    if (SumOf(origMessH)->flags & FLAG_OUT) {
      BuildDateHeader(scratch, SumOf(origMessH)->seconds);
      AppendMessText(newMessH, 0, scratch, strlen((const char *)scratch));
      AppendMessText(newMessH, 0, "\015", 1);
    }

    /* handle the subject */
    FindAndCopyHeader(origMessH, newMessH, HeaderName(SUBJ_HEAD), SUBJ_HEAD);
    if (*GetRString(scratch, FWD_PREFIX)) {
      TrimWhite(scratch);
      CompHeadGetStr(newMessH, SUBJ_HEAD, subj);
      if (!ReMatch(subj, scratch)) {
        GetRString(scratch, FWD_PREFIX);
        PrependMessText(newMessH, SUBJ_HEAD, scratch, strlen((const char *)scratch));
      }
    }

    /*
     * stick it in — copy original body into new message using gEditCtrl
     */
    {
      geditDocument *_origDoc = geditctrl_get_document(origMessH->bodyPTE);
      geditDocument *_newDoc = geditctrl_get_document(newMessH->bodyPTE);
      offset = gedit_document_get_length(_newDoc);
      CompHeadFind(origMessH, 0, &hs);
      origBo = hs.offset;

      /* Copy headers portion (0..origBo) */
      if (origBo > 0)
        gedit_document_copy_range(_origDoc, 0, origBo, _newDoc, offset, TRUE);

      /* Insert paragraph break at end */
      gedit_document_insert_text(_newDoc, gedit_document_get_length(_newDoc),
                                 "\n");

      /* Copy body (origBo..end) without labels */
      gint _origEnd = gedit_document_get_length(_origDoc);
      gint _newEnd = gedit_document_get_length(_newDoc);
      if (_origEnd > origBo)
        gedit_document_copy_range(_origDoc, origBo, _origEnd - origBo, _newDoc,
                                  _newEnd, FALSE);

      if (rich && *GetRString(scratch, FWD_QUOTE)) {
        gint _qEnd = gedit_document_get_length(_newDoc) - 1;
        if (_qEnd > offset)
          gedit_document_set_quote_level(_newDoc, offset, _qEnd - offset, 1);
        if (PrefIsSet(PREF_SCHEERDER)) {
          CompHeadFind(newMessH, 0, &hs);
          GtkTextBuffer *_buf = gedit_document_get_buffer(_newDoc);
          GtkTextIter _s, _e;
          gtk_text_buffer_get_iter_at_offset(_buf, &_s, hs.offset);
          gtk_text_buffer_get_end_iter(_buf, &_e);
          gtk_text_buffer_select_range(_buf, &_s, &_e);
        }
      }
    }

    /*
     * make sure the last line is blank
     */
    EnsureMessNewline(newMessH);

    /*
     * deal with attachments
     */
    if (SumOf(origMessH)->flags & FLAG_OUT) {
      HeadSpec hs;
      UHandle text = NULL;
      if (CompHeadFind(origMessH, ATTACH_HEAD, &hs) &&
          !CompHeadGetText(origMessH->bodyPTE, &hs, &text) &&
          CompHeadFind(newMessH, ATTACH_HEAD, &hs)) {
        CompHeadSet(newMessH->bodyPTE, &hs, text);
      }
      ZapHandle(text);
    } else {
      if (MessFlagIsSet(origMessH, FLAG_HAS_ATT))
        CopyAttachments(newMessH);
      SpoolAttachments(newMessH);
    }

    /*
     * quote them
     */
    if (!rich && !(0 /* MainEvent.modifiers */ & optionKey))
      QuoteLines(newMessH->bodyPTE, CompBodyOffset(newMessH) - 1, 0x7fffffff,
                 FWD_QUOTE, NULL);

    if (GetWindowKind(winWP) != COMP_WIN) {
      AccuAddLong(&newMessH->aSourceMID, SumOf(origMessH)->uidHash);
      AccuAddLong(&newMessH->aSourceMID, SumOf(origMessH)->state);
      AccuAddLong(&newMessH->aSourceMID, FORWARDED);
      SetState(origMessH->tocH, origMessH->sumNum, FORWARDED);
    }

    /*
     * Attributions
     */
    Attribute(FWD_INTRO, origMessH, newMessH, false);
    Attribute(FWD_TRAIL, origMessH, newMessH, true);

    /*
     * copy priority (not my idea)
     */
    if (!PrefIsSet(PREF_NO_XF_PRIOR))
      /* SumOf(newMessH)->origPriority = SumOf(newMessH)->priority =
          SumOf(origMessH)->origPriority; */
      SumOf(newMessH)->priority = SumOf(origMessH)->priority;

    /* SumOf(newMessH)->outType = OUT_FORWARD; //	for statistics */

    UpdateSum(newMessH, SumOf(newMessH)->offset, SumOf(newMessH)->length);
    if (showIt) {
      ShowMyWindow(newWinPtr);
      newWin->isDirty = false;
      gtk_text_buffer_set_modified(
          gedit_document_get_buffer(geditctrl_get_document(newWin->pte)),
          FALSE);
    }
  }
  PopPers();
  return (newWin);
}

/**********************************************************************
 * CompBodyOffset - find the body in a comp message
 **********************************************************************/
long CompBodyOffset(MessHandle messH) {
  HeadSpec hs;

  if (CompHeadFind(messH, 0, &hs))
    return (hs.offset);
  else
    return (0);
}

/**********************************************************************
 * DoFordirectMessage - forward or redirect a message, silently
 **********************************************************************/
OSErr DoFordirectMessage(TOCType * tocH, short sumNum, short flk,
                         PStr addresses, bool bulk) {
  bool iOpened = !tocH->sums[sumNum].messH;
  MyWindowPtr origWin = GetAMessage(tocH, sumNum, NULL, NULL, false);
  MessHandle newMessH;
  OSErr err;
  MyWindowPtr win;

  // Enhanced Filters	no such filter functionality in Light
  if (!HasFeature(flk == flkForward ? featureFilterForward
                                    : featureFilterRedirect))
    return (1);

  UseFeature(flk == flkForward ? featureFilterForward : featureFilterRedirect);
  if (!origWin)
    return (1);

  win = flk == flkForward
            ? DoForwardMessage(origWin, 0, false)
            : DoRedistributeMessage(origWin, 0, false, false, false);

  if (iOpened)
    CloseMyWindow(GetMyWindowWindowPtr(origWin));

  if (!win)
    return (1);

  SetState(tocH, sumNum, flk == flkForward ? FORWARDED : REDIST);
  newMessH = Win2MessH(win);
  if (bulk)
    SetMessOpt(newMessH, OPT_BULK);
  SetMessText(newMessH, TO_HEAD, addresses, strlen((char *)addresses));
  if (err = QueueMessage(newMessH->tocH, newMessH->sumNum, kEuSendNext, 0,
                         true, true))
    DeleteMessage(newMessH->tocH, newMessH->sumNum, false);
  return (err);
}

/**********************************************************************
 * DoReplyClosed - reply to a closed message, silently
 **********************************************************************/
OSErr DoReplyClosed(TOCType * tocH, short sumNum, bool all, bool self,
                    bool quote, bool doFcc, short withWhich, bool vis,
                    bool station) {
  bool iOpened = !tocH->sums[sumNum].messH;
  MyWindowPtr origWin = GetAMessage(tocH, sumNum, NULL, NULL, false);
  MessHandle newMessH;
  OSErr err;
  MyWindowPtr win;

  if (!origWin)
    return (1);

  win = DoReplyMessage(origWin, all, self, quote, doFcc, withWhich, false,
                       station, true);

  if (iOpened)
    CloseMyWindow(GetMyWindowWindowPtr(origWin));

  if (!win)
    return (1);

  SetState(tocH, sumNum, REPLIED);
  newMessH = Win2MessH(win);
  SetMessOpt(newMessH, OPT_BULK);
  if (err = QueueMessage(newMessH->tocH, newMessH->sumNum, kEuSendNext, 0,
                         true, true))
    DeleteMessage(newMessH->tocH, newMessH->sumNum, false);
  return (err);
}

/************************************************************************
 * FindAndCopyHeader - pick the given header out of a message, and
 * copy it to a composition message
 ************************************************************************/
int FindAndCopyHeader(MessHandle origMH, MessHandle newMH, char *fromHead,
                      short toHead) {
  long size;
  int result;
  char *text =
      gedit_document_get_text(geditctrl_get_document(origMH->bodyPTE));
  if (!text)
    return 0;
  size = strlen(text);
  result = TextFindAndCopyHeader(text, size, newMH, fromHead, toHead, 0);
  g_free(text);
  return result;
}

/************************************************************************
 * CopyNewsgroups - copy newsgroups into new message
 ************************************************************************/
OSErr CopyNewsgroups(MessHandle origMH, MessHandle newMH) {
  unsigned char s[256];
  HeadSpec hs, origHS;
  UHandle text = NULL;
  UHandle addresses = NULL;
  OSErr err = fnfErr;
  UPtr address;
  bool first = true;

  GetRString(s, NEWSGROUPS);

  if (CompHeadFindStr(origMH, s, &origHS))
    if (!(err = CompHeadGetText(origMH->bodyPTE, &origHS, &text)))
      if (!(err = SuckAddresses((void ***)&addresses, (void **)text, false,
                                false, false, NULL)) &&
          addresses && *addresses && **addresses) {
        if (CompHeadFind(newMH, BCC_HEAD, &hs)) {
          if (*GetRString(s, MAIL2NEWS)) {
            if (hs.length > 0)
              InsertCommaIfNeedBe(newMH->bodyPTE, &hs);
            CompHeadAppendPtr(newMH->bodyPTE, &hs, s, strlen((char *)s));
            first = false;
          }
          for (address = (UPtr)*addresses; *address;
               address += *address + 2) {
            if (!first || hs.length > 0)
              if (!InsertCommaIfNeedBe(newMH->bodyPTE, &hs))
                CompHeadAppendPtr(newMH->bodyPTE, &hs, " ", 1);

            if (err = CompHeadAppendPtr(newMH->bodyPTE, &hs, "�", 1))
              break;
            if (err = CompHeadAppendPtr(newMH->bodyPTE, &hs, address + 1,
                                        *address))
              break;
            first = false;
          }
        }
      }
  return (err);
}

/************************************************************************
 * TextFindAndCopyHeader - pick the given header out of a text block, and
 * copy it to a composition message
 ************************************************************************/
int TextFindAndCopyHeader(char *body, long size, MessHandle newMH, char *fromHead,
                          short toHead, short label) {
  MyWindowPtr newWin = newMH->win;
  char *bodyEnd = body + size;
  char *spot;
  bool first = true;
  HeadSpec hs;
  long labStart;

  if ((body = FindHeaderString(body, fromHead, &size, false)) && size) {
    if (CompHeadFind(newMH, toHead, &hs)) {
      labStart = hs.offset + hs.length;
      for (;;) {
        if (!first || hs.length > 0)
          if (!InsertCommaIfNeedBe(newMH->bodyPTE, &hs))
            CompHeadAppendPtr(newMH->bodyPTE, &hs, " ", 1);

        CompHeadAppendPtr(newMH->bodyPTE, &hs, body, size);

        first = false;

        body += size;
        while (IsWhite(*body) && body < bodyEnd)
          body++;                             /* skip to newline */
        body++;                               /* skip newline */
        if (body < bodyEnd && IsWhite(*body)) /* continuation */
        {
          while (IsWhite(*body) && body < bodyEnd)
            body++;
          for (spot = body; spot < bodyEnd && *spot != '\015'; spot++)
            ;
          do {
            spot--;
          } while (IsWhite(*spot));
          size = spot - body + 1;
          if (!size)
            break;
        } else
          break;
      }
      if (label && CompHeadFind(newMH, toHead, &hs) &&
          (hs.offset + hs.length) > labStart)
        geditctrl_set_label(newMH->bodyPTE, labStart, hs.offset + hs.length,
                            label);
    }
  }
  return (body != NULL);
}

/************************************************************************
 * XferCustomTable - transfer a custom table from a message to its reply
 * (or forward or redirect)
 ************************************************************************/
void XferCustomTable(MessHandle origMessH, MessHandle newMessH) {
  short origTable;
  bool isOut;

  /*
   * under the old regime, we don't do this step
   */
  if (!NewTables)
    return;

  origTable = SumOf(origMessH)->tableId;

  /*
   * if the original message had the default table, give the default table
   * to the new message (which has already been done, so return)
   */
  if (origTable == DEFAULT_TABLE)
    return;

  /*
   * if no table was on the original mail, use the default table
   * this is different from non-MIME mail
   */
  if (!origTable)
    return;

  /*
   * if original was outgoing, use same table
   */
  isOut = (SumOf(origMessH)->flags & FLAG_OUT) != 0;
  if (isOut)
    SumOf(newMessH)->tableId = origTable;

  /*
   * Otherwise, look for an out table that corresponds to the In table
   * and use that.
   */
  else {
    /* Translation tables not supported in GTK port — use same table ID */
    SumOf(newMessH)->tableId = origTable;
  }
}

/************************************************************************
 * WeedXAttachments - figure out if all the attachments are there
 ************************************************************************/
void WeedXAttachments(MessHandle messH, bool errReport) {
  short i;
  FSSpec spec;
  short err;
  short removed = 0;
  bool foundAny = false;
  HeadSpec hs;

  for (i = 1; 1 != (err = GetIndAttachment(messH, i, &spec, NULL)); i++) {
    if (err) {
      /* attachment does not exist */
      removed++;
      RemoveIndAttachment(messH, i);
      i--;
    } else
      foundAny = true;
  }
  if (!foundAny && CompHeadFind(messH, ATTACH_HEAD, &hs) && hs.length > 0) {
    CompHeadSetPtr(TheBody, &hs, "", 0);
    removed = true;
  }
  if (removed && errReport)
    WarnUser(ATTACH_REMOVED, removed);
}

/**********************************************************************
 * SpoolAttachments - copy attachments to the spool area
 **********************************************************************/
OSErr SpoolAttachments(MessHandle messH) {
  short n;
  short i;
  OSErr err = noErr;
  FSSpec spec;

  /*
   * count the attachments
   */
  for (n = 0; !GetIndAttachment(messH, n + 1, &spec, NULL); n++)
    ;

  for (i = 1; i <= n; i++)
    if (err = SpoolIndAttachment(messH, 1))
      break;

  return (err);
}

/**********************************************************************
 * SpoolIndAttachment - spool a single attachment
 **********************************************************************/
OSErr SpoolIndAttachment(MessHandle messH, short i) {
  FSSpec spec, newSpec;
  OSErr err = noErr;

  /*
   * make the folder
   */
  if (err = MakeAttSubFolder(messH, SumOf(messH)->uidHash, &newSpec))
    return (FileSystemError(COPY_ATTACHMENT, newSpec.name, err));

  /*
   * grab it
   */
  if (!GetIndAttachment(messH, i, &spec, NULL)) {
    /*
     * Copy the attachment
     */
    strcpy(newSpec.name, spec.name);
    if (!SameSpec(&spec, &newSpec) && !FSpIsItAFolder(&spec)) {
      unsigned char longName[256];

      if ((err = FSpDupFile(&newSpec, &spec, false, false)))
        return (FileSystemError(COPY_ATTACHMENT, spec.name, err));

      // handle long filename
      if (!FSpGetLongName(&spec, kTextEncodingUnknown, longName) &&
          *longName > 31)
        FSpSetLongName(&newSpec, kTextEncodingUnknown, longName, &newSpec);

      CompAttachSpec(messH->win, &newSpec);
      RemoveIndAttachment(messH, i);
    }
  }
  return (err);
}

/**********************************************************************
 * MakeAttSubFolder - make the subfolder for attachment spooling
 **********************************************************************/
OSErr MakeAttSubFolder(MessHandle messH, uLong uidHash, FSSpecPtr folder) {
  OSErr err;
  FSSpec spool;
  unsigned char scratch[256];
  long dirID;

  if (messH && !MessOptIsSet(messH, OPT_HAS_SPOOL))
    SetMessOpt(messH, OPT_HAS_SPOOL);
  GetRString(folder->name, SPOOL_FOLDER); // in case of error

  /*
   * find the folder
   */
  if (err = SubFolderSpec(SPOOL_FOLDER, &spool))
    return (err);

  /*
   * message id
   */
  NumToString(uidHash, scratch);

  /*
   * specify
   */
  SimpleMakeFSSpec(spool.vRefNum, spool.parID, scratch, folder);

  /*
   * create
   */
  err = FSpDirCreate(folder, smSystemScript, &dirID);
  if (err == dupFNErr) {
    dirID = SpecDirId(folder);
    if (dirID)
      err = noErr;
  }
  if (err)
    return (err);

  /*
   * and make it point into the folder
   */
  folder->parID = dirID;
  *folder->name = 0;

  return (err);
}

/************************************************************************
 * RemoveIndAttachment - remove a particular attachment
 ************************************************************************/
void RemoveIndAttachment(MessHandle messH, short index) {
  HeadSpec where;
  FSSpec spec;
  OSErr err = GetIndAttachment(messH, index, &spec, &where);

  if (err != 1) {
    CompDelAttachment(messH, &where);
    AttachSelect(messH);
  }
}

/************************************************************************
 * CopyToOut - copy a message to the Out mailbox
 ************************************************************************/
OSErr CopyToOut(TOCType * fromTocH, short sumNum, TOCType * toTocH) {
  MessHandle messH;
  short err = 1;
  MyWindowPtr win = NULL, newWin = NULL;
  MSumType newSum, oldSum;

  if (!(win = GetAMessage(fromTocH, sumNum, NULL, NULL, false)))
    return (1);
  messH = Win2MessH(win);
  fromTocH = messH->tocH;
  sumNum = messH->sumNum;

  if (newWin = DoSalvageMessage(messH->win, true)) {
    if (SaveComp(newWin)) {
      newSum = toTocH->sums[toTocH->count - 1];
      oldSum = fromTocH->sums[sumNum];
      SumInfoCpy(&newSum, &oldSum);
      newSum.flags |= FLAG_OUT;
      toTocH->sums[toTocH->count - 1] = newSum;
      err = noErr;
    }
    NoSaves = true;
    CloseMyWindow(GetMyWindowWindowPtr(newWin));
    NoSaves = false;
  }
  if (win)
    CloseMyWindow(GetMyWindowWindowPtr(win));
  return (err);
}

/************************************************************************
 * SumInfoCpy - copy non-essential summary fields
 ************************************************************************/
void SumInfoCpy(MSumPtr newSum, MSumPtr oldSum) {
  newSum->state = oldSum->state;
  strcpy((char *)newSum->from, (const char *)oldSum->from);
  strcpy((char *)newSum->subj, (const char *)oldSum->subj);
  newSum->seconds = oldSum->seconds;
  newSum->flags = oldSum->flags;
  newSum->tableId = oldSum->tableId;
  newSum->sigId = SigValidate(oldSum->sigId);
  newSum->priority = oldSum->priority;
  // newSum->origPriority = oldSum->origPriority;
  // BMD(&oldSum->spareShort2, &newSum->spareShort2,
  // sizeof(newSum->spareShort2));
  if (oldSum->opts & OPT_INLINE_SIG)
    newSum->opts |= OPT_INLINE_SIG;
}

/* The following hashing algorithm, KRHash, is derived from Karp & Rabin,
    Harvard Center for Research in Computing Technology Tech Report TR-31-81.
 */
/* The prime number in use is KRHashPrime.  It happens to be
    the largest prime number that will fit in 31 bits, except for 2^31-1
   itself.
 */

#define KRHashPrime (2147483629)

/************************************************************************
 * HashWithSeed - generate a hash from a string and a seed.
 ************************************************************************/
uLong HashWithSeedLo(unsigned char *s, uLong n, uLong seed) {
  uLong sum = seed - 1;
  int Bit;

  for (; n; n--, s++) {
    for (Bit = 0x80; Bit != 0; Bit >>= 1) {
      sum += sum;
      if (sum >= KRHashPrime)
        sum -= KRHashPrime;
      if ((*s) & Bit)
        ++sum;
      if (sum >= KRHashPrime)
        sum -= KRHashPrime;
    }
  }
  return (sum + 1);
}

/************************************************************************
 * MIDHash - hash a message id, stripping <>'s first
 ************************************************************************/
uLong MIDHash(UPtr text, long size) {
  unsigned char scratch[256];
  UHandle addresses;

  SuckPtrAddresses(&addresses, text, size, false, false, false, NULL);

  if (addresses) {
    strcpy((char *)scratch, (const char *)*addresses);
    ZapHandle(addresses);
    return (Hash(scratch));
  } else
    return (kNoMessageId);
}

/************************************************************************
 * SetHash - set a message's hash function
 ************************************************************************/
void SetHashLo(TOCType * tocH, short sumNum, uLong hash, bool soft) {
  DBNoteUIDHash(tocH->sums[sumNum].uidHash, hash);
  if (!soft || !ValidHash(tocH->sums[sumNum].uidHash))
    tocH->sums[sumNum].uidHash = hash;
  if (!soft || !ValidHash(tocH->sums[sumNum].msgIdHash))
    tocH->sums[sumNum].msgIdHash = hash;
  TOCSetDirty(tocH, true);
}

/************************************************************************
 * Rehash - recompute the hash for a message
 ************************************************************************/
void RehashLo(TOCType * tocH, short sumNum, UHandle text, bool soft) {
  unsigned char scratch[256];
  UPtr spot;
  long size = GetHandleSize_(text);
  uLong hash;

  spot = (unsigned char *)(*text);
  GetRString(scratch, HEADER_STRN + MSGID_HEAD);
  if ((spot = (unsigned char *)FindHeaderString((char *)spot, (char *)scratch, &size, false)))
    hash = Hash(spot);
  else
    hash = kNeverHashed;

  if (soft && tocH->sums[sumNum].msgIdHash && hash == kNeverHashed)
    return; // Don't overwrite good hash with bad

  tocH->sums[sumNum].msgIdHash = hash;
  TOCSetDirty(tocH, true);
}

void Rehash(TOCType * tocH, int sumNum, UPtr buffer) {
  RehashLo(tocH, sumNum, (UHandle)buffer, true);
}

#define PREVIEW_ID_MULT_REDO (-3)
#define PREVIEW_ID_MULT (-2)
/************************************************************************
 * Preview - preview a message, if desired
 ************************************************************************/
void Preview(TOCType * tocH, short sumNum) {
  WindowPtr tocWinWP = GetMyWindowWindowPtr(tocH->win);
  MessHandle messH;
  long id;
  GtkWidget *pte; // gEditCtrl widget (was PETEHandle)
  MyWindowPtr messWin = NULL;
  bool active = false;
  short ezOpenSum;
  OSErr err;
  unsigned char profileName[64];
  short oldPreview;

  if (!(pte = tocH->previewPTE))
    return;

  messH = tocH->sums[sumNum].messH;
  if (sumNum >= 0) {
    EnsureMID(tocH, sumNum);
    id = tocH->sums[sumNum].serialNum;
  } else if (sumNum == -2) {
    // if (!tocH->conConMultiScan)
    id = tocH->previewID;
    /* else if (ConConMultipleAppropriate(tocH)) {
      tocH->previewID = PREVIEW_ID_MULT_REDO;
      id = PREVIEW_ID_MULT_REDO;
    } else
      return; */
    // else id = 0;
  } else
    id = 0;

  if (tocH->previewID == id) {
    PeteCalcOn(tocH->previewPTE);
    if (id && tocH->lastSameTicks != 1 &&
        id != PREVIEW_ID_MULT) // we have a message and we're not already done
    {
      if (!InBG                         // front only
          && IsWindowVisible(tocWinWP)  // visible
          && tocWinWP == FrontWindow_() // frontmost
          && tocH->userActive) // user has recently clicked or typed at me
      {
        if (TickCount() - tocH->lastSameTicks >
                10              // it's been a while since we last checked
            && PreviewReadTimer // the user wants this marking
            &&
            TickCount() - tocH->lastSameTicks >
                GetRLong(PREVIEW_READ_SECS) * 60) // and it's been long enough
        {
          tocH->lastSameTicks = 1;
          // do not automatically mark IMAP minimal headers as read, ever.
          if (!tocH->imapTOC || IMAPMessageDownloaded(tocH, sumNum))
            BeenThereDoneThat(tocH, sumNum);
        }
      } else
        tocH->lastSameTicks =
            TickCount(); // reset counter if user not active
    }
    return;
  } else
    tocH->lastSameTicks = TickCount();

  if (tocH->previewID && tocH->imapTOC) {
    // Cancel the IMAP download if this is an imap message ...
    FindRealSummary(tocH, tocH->previewID, &oldPreview);
    IMAPAbortMessageFetch(tocH, oldPreview);
  }

  tocH->previewID = id;
  tocH->ezOpenSerialNum = 0;

  {
    /* Initialize the editor with placeholder then clear */
    geditDocument *_doc = geditctrl_get_document(pte);
    gedit_document_insert_text(_doc, 0, " ");
    gedit_document_set_quote_level(_doc, 0, 1, 0);
    geditctrl_lock_range(pte, 0, 1, 0);
    gedit_document_delete_range(_doc, 0, gedit_document_get_length(_doc));
  }

  if (id && id != PREVIEW_ID_MULT) {
    SelectBoxRange(tocH, sumNum, sumNum, false, -1, -1);

    // need to open?
    if (!messH) {
      short realSumNum;
      TOCType * realTocH;
      if (!(realTocH = GetRealTOC(tocH, sumNum, &realSumNum)))
        return;

      if (!(messH = realTocH->sums[realSumNum].messH)) {
        messWin =
            realTocH->which == OUT
                ? OpenComp(realTocH, realSumNum, NULL, NULL, false, false)
                :
                OpenMessage(realTocH, realSumNum, NULL, NULL, false, true);
        if (messWin)
          messH = Win2MessH(messWin);
      }
      //
      // fire off a thread to fetch the next message as well, if it's not
      // already there.
      // ... but don't do this for the search window.
      //

      if (!Offline && AutoCheckOK() && !(tocH->virtualTOC)) {
        short sumToOpenNext;

        // open the next summary according to EZOpen
        sumToOpenNext = EzOpenFind(realTocH, realSumNum);
        if (sumToOpenNext >= 0) {
          if (!IMAPMessageDownloaded(realTocH, sumToOpenNext) &&
              !IMAPMessageBeingDownloaded(realTocH, sumToOpenNext))
            UIDDownloadMessage(realTocH,
                               realTocH->sums[sumToOpenNext].uidHash, false,
                               false);
        }
      }
    }

    if (messH) {
      bool junk = SumOf(messH)->spamScore >= GetRLong(JUNK_MAILBOX_THRESHHOLD);

      // (*PeteExtra(pte))->containsJunkMail = junk;
      PeteCalcOff(pte);
      /* undo is always available in gEditCtrl */
      if (!ConConMess(
              messH, pte,
              BoxPreviewProfile(profileName, tocH, CONCON_PREVIEW_PROFILE),
              NULL, NULL)) {
        /* Lock the entire text to prevent editing in preview */
        geditctrl_lock_range(
            pte, 0, gedit_document_get_length(geditctrl_get_document(pte)), 0);
        PeteCalcOn(pte);
      } else {
        long body = SumOf(messH)->bodyOffset - messH->weeded;
        long len;
        long scanned;
        UHandle text;
        long oldID;
        long para;
        PETEParaInfo pinfo;
        long bite;

        // Make sure we examine it for funny business
        HiliteOddReply(messH);

        {
          /* Copy message text from TheBody into the preview pte using gEditCtrl
           */
          geditDocument *_srcDoc = geditctrl_get_document(TheBody);
          geditDocument *_dstDoc = geditctrl_get_document(pte);
          len = gedit_document_get_length(_srcDoc);
          bite = len;

          /* Limit initial copy to ~3K chars for responsiveness */
          if (bite > 3 K)
            bite = 3 K;

          /* Find paragraph boundary near bite */
          bite = gedit_document_find_line_end(_srcDoc, bite - 1);
          if (bite <= 0)
            bite = len;

          /* Trim trailing whitespace/newlines */
          gchar *_srcText = gedit_document_get_text(_srcDoc);
          if (_srcText) {
            long _trimBite = bite;
            if (_trimBite == len)
              while (_trimBite && (_srcText[_trimBite - 1] == '\n' ||
                                   _srcText[_trimBite - 1] == ' '))
                _trimBite--;
            if (_trimBite > 0)
              bite = _trimBite;

            /* Copy first chunk */
            gedit_document_copy_range(_srcDoc, 0, bite, _dstDoc, 0, FALSE);

            /* Scroll to body start */
            GtkTextBuffer *_buf = gedit_document_get_buffer(_dstDoc);
            GtkTextIter _iter;
            gtk_text_buffer_get_iter_at_offset(_buf, &_iter, body + 1);
            gtk_text_buffer_place_cursor(_buf, &_iter);

            /* Copy remainder if there is more */
            if (bite < len) {
              long _trimLen = len;
              while (_trimLen && (_srcText[_trimLen - 1] == '\n'))
                _trimLen--;
              if (bite < _trimLen) {
                gint _dstEnd = gedit_document_get_length(_dstDoc);
                gedit_document_copy_range(_srcDoc, bite, _trimLen - bite,
                                          _dstDoc, _dstEnd, FALSE);
              }
            }
            g_free(_srcText);
          }
        }

        // And lock all the text
        geditctrl_lock_range(
            pte, 0, gedit_document_get_length(geditctrl_get_document(pte)), 0);
      }
    }

    if (messWin)
      CloseMyWindow(GetMyWindowWindowPtr(messWin));
    if (!PrefIsSet(PREF_NO_EZ_OPEN)) {
      ezOpenSum = EzOpenFind(tocH, sumNum);
      if (ezOpenSum >= 0) {
        tocH->ezOpenSerialNum = tocH->sums[ezOpenSum].serialNum;
        CacheMessage(tocH, ezOpenSum);
      }
    }

  } else if (id == PREVIEW_ID_MULT) {
    ConConMultiple(
        tocH, pte,
        BoxPreviewProfile(profileName, tocH, CONCON_MULTI_PREVIEW_PROFILE),
        conConOutSeparatorRule, NULL, NULL);
    tocH->previewID = id;
  } else
    BoxPreviewProfile(NULL, tocH, 0);

  /* Activate the preview editor — grab focus */
  gtk_widget_grab_focus(pte);
}

/************************************************************************
 * HTMLifyText - insert appropriate BR's, process and remove "related:" lines,
 *etc.
 ************************************************************************/
void HTMLifyText(MyWindowPtr win, Handle text) {
  Accumulator a;
  Ptr spot, end, lastSpot;
  long len;
  unsigned char sBR[32];
  char lastChar;
  long offset = 0;
  PartDesc pd;
  StackHandle stack;

  //	Insert <BR> after each header line
  if (!AccuInit(&a)) {

    GetRString(sBR, HTMLTagsStrn + htmlBR);

    len = 0;
    spot = *text;
    lastChar = 0;
    lastSpot = spot;
    for (end = spot + GetHandleSize(text); spot < end; spot++) {
      if (*spot == '\r') {
        AccuAddPtr(&a, lastSpot, spot - lastSpot);
        AccuAddChar(&a, '<'); //	Add <BR>
        AccuAddStr(&a, sBR);
        AccuAddChar(&a, '>');
        if (lastChar == '\r')
          break; //	Hit 2 newlines in a row. End of headers.
        lastSpot = spot + 1;
      }
      lastChar = *spot;
    }

    Munger(text, 0, NULL, (char *)spot - (char *)*text, (const void *)a.data,
           a.offset);
    offset = a.offset;
    /* data is char* flat buffer — just free once */
    if (a.data) {
      free(a.data);
      a.data = NULL;
    }
    a.offset = a.size = 0;
  }

  //	Build parts stack from "related:" lines and remove them
  StackInit(sizeof(PartDesc), &stack);
  while (0 <= (offset = FindAnAttachment(text, offset, &pd.spec, false, &pd.cid,
                                         &pd.relURL, &pd.absURL))) {
    Ptr lineEnd, end;
    long len;

    StackQueue(&pd, stack);
    //	Remove related line
    end = *text + GetHandleSize_(text);
    for (lineEnd = *text + offset; lineEnd < end && *lineEnd != '\015';
         lineEnd++)
      ;
    len = (char *)lineEnd - (char *)*text;
    if (lineEnd < end)
      len++; //	Get past CR
    Munger(text, (char *)lineEnd - (char *)*text, NULL,
           (char *)lineEnd - (char *)*text + 1, (Ptr) "",
           0); //	Delete the line
  }
  // (*PeteExtra(win->pte))->partStack = stack;
}

/************************************************************************
 * EnsureMsgDownloaded - if IMAP message, make sure it is downloaded
 ************************************************************************/
bool EnsureMsgDownloaded(TOCType * tocH, int sumNum, bool attachmentsToo) {
  // Actually go fetch this message if we must
  if (tocH->imapTOC) {
    short n;
    bool result;

    // make sure this message has already been downloaded ...
    if (!IMAPMessageDownloaded(tocH, sumNum)) {
      if (IMAPMessageBeingDownloaded(tocH, sumNum)) {
        //	This message is currently being downloaded. Wait for it to
        // finish
        while (IMAPMessageBeingDownloaded(tocH, sumNum)) {
                  MyYieldToAnyThread();
          if (CommandPeriod)
            return (false);
        }
      } else {
        //	Download this message in the foreground
        if (UIDDownloadMessage(tocH, tocH->sums[sumNum].uidHash, true,
                               false) != noErr)
          return false;
      }
    }

    // put the message in the mailbox
    for (n = 100; n && !IMAPMessageDownloaded(tocH, sumNum); n--)
      //	call a reasonable number of times to get the job done
      //	there may be other messages that also need to be processed
      UpdateIMAPMailbox(tocH);

    // did we get the message ok?
    if ((result = IMAPMessageDownloaded(tocH, sumNum)) && attachmentsToo) {
      // go fetch all the attachments for this message.  Wait for them.
      if (!FetchAllIMAPAttachments(tocH, sumNum, true))
        result = false;
    }
    return result;
  }
  return true;
}

/************************************************************************
 * EnableMsgButtons - enable or disable message buttons
 ************************************************************************/
void EnableMsgButtons(MyWindowPtr win, bool enable) {
  if (!win || !win->window) return;
  GtkWidget *winWP = (GtkWidget *)win->window;
  /* Enable/disable toolbar buttons stored on the window */
  static const char *btn_names[] = {
    "btn-fetch", "btn-trash", "btn-delete", "btn-reply",
    "btn-forward", "btn-redirect", "btn-blahblah", NULL
  };
  for (int i = 0; btn_names[i]; i++) {
    GtkWidget *btn = g_object_get_data(G_OBJECT(winWP), btn_names[i]);
    if (btn)
      gtk_widget_set_sensitive(btn, enable);
  }
}

/************************************************************************
 * GetMessageLength - return length of message
 ************************************************************************/
uLong GetMessageLength(TOCType * tocH, short sumNum) {
  short realSum;
  TOCType * realTOC;

  return (realTOC = GetRealTOC(tocH, sumNum, &realSum))
             ? realTOC->sums[realSum].length
             : 0;
}

/************************************************************************
 * RedateTS - redate a message
 ************************************************************************/
void RedateTS(TOCType * tocH, short sumNum) {
  unsigned char dateStr[256];
  uLong secs;
  uLong zoneSecs;
  OSErr err = CacheMessage(tocH, sumNum);

  if (!err && tocH->sums[sumNum].cache) {
    HandleHeadGetPStr(tocH->sums[sumNum].cache, HeaderStrn + DATE_HEAD,
                      dateStr);
    if (!*dateStr)
      return;
    secs = BeautifyDate(dateStr, &zoneSecs);
    TimeStamp(tocH, sumNum, secs, zoneSecs);
  }
  return;
}

/************************************************************************
 * CurAddr - extract the current address from a window, if we have one
 ************************************************************************/
PStr CurAddr(MyWindowPtr win, PStr addr) {
  if (win->curAddr)
    return win->curAddr(win, addr);
  else
    return CurAddrSel(win, addr);
}

/************************************************************************
 * CurAddrSel - extract the current address from the selection, if we have one
 ************************************************************************/
PStr CurAddrSel(MyWindowPtr win, PStr addr) {
  if (win->pte && *PeteSelectedString(addr, win->pte)) {
    ShortAddr(addr, addr);
    if (*addr)
      return addr;
  }
  return NULL;
}

/************************************************************************
 * SpoolMessage - write a message to a spool file for IMAP-to-POP transfer.
 * Portable POSIX implementation: copies raw message bytes from the mailbox
 * to the destination file.
 ************************************************************************/
OSErr SpoolMessage(MessHandle messH, FSSpecPtr theSpec, short refN) {
  if (!messH)
    return -1;
  TOCType * tocH = messH->tocH;
  int sumNum = messH->sumNum;
  if (!tocH)
    return -1;

  long offset = tocH->sums[sumNum].offset;
  long length = tocH->sums[sumNum].length;

  FILE *src = fopen(tocH->mailbox.spec.path, "rb");
  if (!src)
    return -1;
  if (fseek(src, offset, SEEK_SET) != 0) {
    fclose(src);
    return -1;
  }

  FILE *dst;
  if (refN != 0) {
    dst = fdopen(dup(refN), "ab");
  } else {
    dst = fopen(theSpec->path, "wb");
  }
  if (!dst) {
    fclose(src);
    return -1;
  }

  char buf[4096];
  while (length > 0) {
    size_t n = fread(
        buf, 1,
        (size_t)(length < (long)sizeof(buf) ? length : (long)sizeof(buf)), src);
    if (n == 0)
      break;
    fwrite(buf, 1, n, dst);
    length -= (long)n;
  }
  fclose(src);
  fclose(dst);
  return noErr;
}

/************************************************************************
 * FileGraphicChangeGraphic - update an inline graphic in the gEditCtrl
 * widget at the given text offset with the image from spec.
 ************************************************************************/
OSErr FileGraphicChangeGraphic(GtkWidget *pte, long offset, FSSpecPtr spec) {
  if (!pte || !spec || !spec->path[0])
    return -1;
  GError *err = NULL;
  GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(spec->path, &err);
  if (!pixbuf) {
    if (err)
      g_error_free(err);
    return -1;
  }
  geditctrl_insert_image(pte, pixbuf, gdk_pixbuf_get_width(pixbuf),
                         gdk_pixbuf_get_height(pixbuf));
  g_object_unref(pixbuf);
  return noErr;
}
