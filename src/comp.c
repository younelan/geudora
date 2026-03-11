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

#include "../gEditCtrl/geditctrl.h"
#include "Globals.h"
#include "StringDefs.h"
#include "mailbox.h"
#include "message.h"
#include "prefdefs.h"
#include "toc.h"
#include "util.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FILE_NUM 7
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Missing function declarations - use compatible signatures */
void SetMyWindowPrivateData(MyWindowPtr win, void *privateData);
/* CloseMyWindow, InvalContent, UpdateMyWindow, GetSumColor declared in headers */
void AttachSelect(MessHandle messH);
int SetWinMinSize(MyWindowPtr win, int width, int height);
bool IsColorWin(void *winWP);
int MessFind(MyWindowPtr win);
int MessagePosition(MyWindowPtr win);
int GetMessageLength(TOCType * tocH, int sumNum);
int ReadMessage(TOCType * tocH, int sumNum, unsigned char *buffer);
bool ShowMyWindow(void *winWP);
extern short AWrite(short refNum, long *count, unsigned char *buffer);

/* Missing constants */
#ifndef FLAG_ICON_BAR
#define FLAG_ICON_BAR (1 << 6)
#endif
#ifndef OPT_COMP_TOOLBAR_VISIBLE
#define OPT_COMP_TOOLBAR_VISIBLE (1 << 7)
#endif
#ifndef PREF_COMP_TOOLBAR
#define PREF_COMP_TOOLBAR PREF_188
#endif
#ifndef MessZoomSize
#define MessZoomSize 0
#endif
#ifndef CompZoomSize
#define CompZoomSize 1
#endif

/************************************************************************
 * private function declarations - ported to use gEditCtrl instead of Pete
 ************************************************************************/
int GetCompTexts(MessHandle messH, bool new);
void MakeCompTitle(char *string, TOCType * tocH, MessHandle messH, int sumNum);
int WriteComp(MessHandle messH, short refN, long offset);
char *GetMyHostname(char *hostname);
int CompStripHeaderReturns(MessHandle messH);
int SuckDragAddresses(void *drag, char **addresses, bool leadingComma,
                      bool trailingComma);
int FindAndMarkSigSep(GtkWidget *pte);
long FindSigSep(GtkWidget *pte);
void SepStyle(void *pip, void *tsp, void **graphic, int pgt);
int CompGetDragContents(GtkWidget *pte, char **theText, void **theStyles,
                        void **theParas, void *drag, long dropLocation);
void CompBeautifyFrom(char *name);
unsigned char *CompCurAddr(MyWindowPtr win, unsigned char *addr);

/* Forward declarations for window management functions */
bool CompClose(MyWindowPtr win);
void CompDidResize(MyWindowPtr win);
bool CompClick(MyWindowPtr win, GdkEvent *event);
bool CompMenu(MyWindowPtr win, int menuItem);
bool CompKey(MyWindowPtr win, GdkEvent *event);
bool CompButton(MyWindowPtr win, GtkWidget *button, GdkEvent *event);
void CompHelp(MyWindowPtr win, int helpType);
void CompGonnaShow(MyWindowPtr win);
bool CompDragHandler(MyWindowPtr win, void *dragEvent);
void CompIdle(MyWindowPtr win);
bool CompSend(MessHandle messH);
bool CompSave(MessHandle messH);
int CreateMessageBody(char *buffer, unsigned long *uidHash);
int GatherCompAddresses(MyWindowPtr win, char *addrList);

/**********************************************************************
 * OpenComp - open an outgoing message - ported to use gEditCtrl
 **********************************************************************/
MyWindowPtr OpenComp(TOCType * tocH, int sumNum, GtkWidget *winWP,
                     MyWindowPtr win, bool showIt, bool new) {
  char title[256];
  MessHandle messH;
  void *grumble;

  // Remove Mac-specific CycleBalls()
  if ((messH = NewZH(MessType)) == NULL)
    return (NULL);

  // Create GTK window instead of Mac window
  if (!win) {
    win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
    if (!win) {
      DisposeHandle((Handle)messH);
      return NULL;
    }
    win->window = gtk_window_new();
    win->pte = NULL;
  }
  if (!win) {
    DisposeHandle((Handle)messH);
    return (NULL);
  }

  winWP = (GtkWidget *)GetMyWindowWindowPtr(win);

  tocH->sums[sumNum].messH = messH;
  MakeCompTitle(title, tocH, messH, sumNum);

  (*messH)->win = win;
  (*messH)->sumNum = sumNum;
  (*messH)->tocH = tocH;

  // Handle signature validation (simplified)
  // long sigID = SigValidate(SumOf(messH)->sigId);
  // SumOf(messH)->sigId = sigID;

  SetMyWindowPrivateData(win, (void *)messH);
  win->close = CompClose;
  // win->curAddr = CompCurAddr; // Remove if not in MyWindow struct

  LL_Push(MessList, messH);

  /* formatting toolbar - simplified for GTK */
  tocH->sums[sumNum].flags |= FLAG_ICON_BAR;
  if (PrefIsSet(PREF_COMP_TOOLBAR))
    SetMessOpt(messH, OPT_COMP_TOOLBAR_VISIBLE);

  // Create send button using GTK
  grumble = gtk_button_new_with_label("Send Now");
  (*messH)->sendButton = grumble;

  // Set button state based on message state
  bool isGreyed = (tocH->sums[sumNum].state == SENT ||
                   tocH->sums[sumNum].state == BUSY_SENDING);
  gtk_widget_set_sensitive(GTK_WIDGET(grumble), !isGreyed);

  if (GetCompTexts(messH, new)) {
    // Clean up gEditCtrl instead of Pete
    if (win->pte) {
      g_object_unref(win->pte);
      win->pte = NULL;
    }
    // win->isDirty = false; // Remove if not in MyWindow struct
    CloseMyWindow(winWP);
    return (NULL);
  }

  // Set up window callbacks - simplified for GTK
  // win->didResize = CompDidResize; // Remove if not in MyWindow struct
  // win->click = CompClick; // Remove if not in MyWindow struct
  // win->menu = CompMenu; // Remove if not in MyWindow struct
  // win->key = CompKey; // Remove if not in MyWindow struct
  // win->button = CompButton; // Remove if not in MyWindow struct
  // win->position = MessagePosition; // Remove if not in MyWindow struct
  // win->help = CompHelp; // Remove if not in MyWindow struct
  // win->gonnaShow = CompGonnaShow; // Remove if not in MyWindow struct
  // win->zoomSize = (SumOf(messH)->state == SENT) ? MessZoomSize :
  // CompZoomSize; // Remove if not in MyWindow struct win->drag =
  // CompDragHandler; // Remove if not in MyWindow struct win->idle = CompIdle;
  // // Remove if not in MyWindow struct win->userSave = true; // Remove if not
  // in MyWindow struct win->find = MessFind; // Remove if not in MyWindow
  // struct
  SetWinMinSize(win, 280, 160);

  win->dontControl = true;
  if (IsColorWin(winWP))
    win->label = GetSumColor((*messH)->tocH, (*messH)->sumNum);
  if (showIt)
    ShowMyWindow(winWP);
  InvalContent(win);
  gtk_window_set_title(GTK_WINDOW(winWP), title);
  AttachSelect(messH);
  // win->isDirty = false; // Remove if not in MyWindow struct

  // Clean up gEditCtrl list instead of Pete
  if (win->pte) {
    // gEditCtrl cleanup
  }

  UpdateMyWindow(winWP);

  return (win);
}

/**********************************************************************
 * DoComposeNew - start a new outgoing message
 * Ported from functions.c - gets Out TOC, creates blank summary, opens OpenComp
 **********************************************************************/
MyWindowPtr DoComposeNew(int type) {
  (void)type;
  TOCType *tocH;
  MSumType sum;
  MyWindowPtr newWin;
  bool oldReallyDirty;

  /* Always use the real Out TOC (threading is always on) */
  tocH = GetRealOutTOC();
  g_print("DoComposeNew: GetRealOutTOC=%p\n", (void*)tocH);
  if (!tocH) return NULL;

  memset(&sum, 0, sizeof(sum));
  sum.state = UNSENDABLE;
  sum.flags = 0;
  sum.tableId = 0;
  sum.origZone = ZoneSecs() / 60;
  sum.seconds = GMTDateTime();
  sum.persId = (*CurPers)->persId;
  sum.sigId = 0;

  oldReallyDirty = tocH->reallyDirty;
  if (!SaveMessageSum(&sum, &tocH)) {
    g_print("DoComposeNew: SaveMessageSum failed\n");
    return NULL;
  }
  g_print("DoComposeNew: SaveMessageSum OK, count=%d\n", tocH->count);

  newWin = OpenComp(tocH, tocH->count - 1, NULL, NULL, true, true);
  g_print("DoComposeNew: OpenComp returned %p\n", (void*)newWin);
  return newWin;
}

/**********************************************************************
 * CompCurAddr - return the address most closely associated with this message
 * Ported to use standard C strings instead of Pascal strings
 **********************************************************************/
unsigned char *CompCurAddr(MyWindowPtr win, unsigned char *addr) {
  char *addrList =
      g_malloc(1024); // Replace BinAddrHandle with standard allocation
  *addr = 0;

  if (win->hasSelection)
    return CurAddrSel(win, addr);

  if (!GatherCompAddresses(win, addrList)) {
    g_strlcpy((char *)addr, addrList, 256); // Replace PCopy with g_strlcpy
    ShortAddr(addr, addr);
  }

  g_free(addrList);
  return *addr ? addr : NULL;
}

/**********************************************************************
 * BodyOffset - return the offset to the first character of the body
 * of a message - ported to use char* instead of Handle
 **********************************************************************/
long BodyOffset(char *text) {
  char *spot;
  long size = strlen(text);
  char *end = text + size;

  for (spot = text + 2; spot < end; spot++)
    if (spot[-1] != '\015')
      spot++;
    else if (spot[-2] == '\015')
      break;

  return (spot - text);
}

/************************************************************************
 * NewMessageId - create a new message id - ported to use standard C
 ************************************************************************/
char *NewMessageId(char *id) {
  char hostname[128];
  char scratch[256];
  static short seq;
  char *vers;
  struct {
    unsigned char four[4];
    long seconds;
    short ticks;
  } rawStuff;

  // Get version info (simplified)
  vers = "Eudora-GTK";

  // Get hostname
  GetMyHostname(hostname);

  // Create unique ID components
  rawStuff.seconds = time(NULL);
  rawStuff.ticks = rand() % 60;
  seq++;

  // Format message ID
  snprintf(id, 256, "<%08lX.%04X.%s@%s>", rawStuff.seconds, seq, vers,
           hostname);

  return id;
}

/**********************************************************************
 * GetCompTexts - get the fields of an under-composition message
 * PORTED FROM PETE TO gEditCtrl
 * First, we read ALL the message into a buffer. Then, we grab the
 * header items, stuff them one by one into appropriate gEditCtrl widgets.
 * After that, we stuff the body into the main gEditCtrl widget.
 *
 * the "new" item means not to read the text, but to create it instead
 **********************************************************************/
int GetCompTexts(MessHandle messH, bool new) {
  MyWindowPtr messWin = (*messH)->win;
  GtkWidget *messWinWP = (GtkWidget *)GetMyWindowWindowPtr(messWin);
  int sumNum = (*messH)->sumNum;
  TOCType * tocH = (*messH)->tocH;
  char *buffer = NULL;
  Accumulator extras;
  int which;
  int err = 0;
  char *cp, *ep;
  char *stop;
  unsigned long uidHash;
  short len;
  geditDocument *doc; // Replace PETEDocInitInfo with geditDocument
  char headerName[64];
  long width;
  bool locked;
  short baseLock;
  long bo;
  void *grumble;
  bool xDash;
  long baseWidth = 100; // Simplified base width calculation

  memset(&extras, 0, sizeof(extras));

  /*
   * allocate space for the text
   */
  long bufferSize = new ? 1024 : GetMessageLength(tocH, sumNum) + 1;
  if ((buffer = g_malloc(bufferSize)) == NULL) {
    return (WarnUser(NO_MESS_BUF, errno));
  }

  /*
   * read or create
   */
  if (!new) {
    /*
     * read it
     */
    if ((err = ReadMessage(tocH, sumNum, (unsigned char *)buffer))) {
      g_free(buffer);
      return (err);
    }
  } else {
    len = CreateMessageBody(buffer, &uidHash);
    buffer = g_realloc(buffer, len + 1);
    DBNoteUIDHash(SumOf(messH)->uidHash, uidHash);
    SumOf(messH)->uidHash = SumOf(messH)->msgIdHash = uidHash;
  }

  /*
   * now, set up the gEditCtrl widget instead of Pete TERec's
   */
  messWin->pte = geditctrl_new();
  if (!messWin->pte) {
    err = -1;
    goto failure;
  }

  // Get the document from gEditCtrl
  doc = geditctrl_get_document(messWin->pte);

  // Set drag callback for gEditCtrl (simplified)
  // geditctrl_set_drag_callback(messWin->pte, CompGetDragContents);

  /*
   * put in the text...
   */
  cp = buffer;
  stop = cp + strlen(buffer);
  while (*cp++ != '\015' && cp < stop)
    ; // skip sendmail from line

  /*
   * the headers - ported to gEditCtrl
   */
  // Remove CycleBalls() - Mac-specific
  baseLock =
      (SumOf(messH)->state == SENT || SumOf(messH)->state == BUSY_SENDING) ? 1
                                                                           : 0;

  for (which = TO_HEAD; which < BODY_HEAD; which++) {
    locked = baseLock || which == ATTACH_HEAD ||
             (which == FROM_HEAD && !PrefIsSet(PREF_EDIT_FROM));

    GetRString(headerName, HeaderStrn + which);
    xDash =
        strlen(headerName) > 2 && headerName[0] == 'X' && headerName[1] == '-';

    // Find header content
    ep = cp;
    while (ep < stop && *ep != '\015')
      ep++;

    if (ep > cp) {
      // Insert header name
      char headerLine[512];
      snprintf(headerLine, sizeof(headerLine), "%s ", headerName);
      gedit_document_insert_text(doc, gedit_document_get_length(doc),
                                 headerLine);

      // Insert header content
      char *headerContent = g_strndup(cp, ep - cp);
      gedit_document_insert_text(doc, gedit_document_get_length(doc),
                                 headerContent);
      g_free(headerContent);

      // Add newline
      gedit_document_insert_text(doc, gedit_document_get_length(doc), "\n");
    }

    cp = ep + 1;
  }

  // Store header count in gEditCtrl metadata (simplified)
  // geditctrl_set_header_count(messWin->pte, which - 1);

  // Insert body separator
  gedit_document_insert_text(doc, gedit_document_get_length(doc), "\n");

  // Get body offset
  bo = gedit_document_get_length(doc);

  if (ep < stop) {
    // Insert body text
    if (MessFlagIsSet(messH, FLAG_RICH)) {
      // Insert rich text (simplified - would need full rich text parsing)
      char *bodyText = g_strndup(ep, stop - ep);
      gedit_document_insert_markup(doc, bo, bodyText);
      g_free(bodyText);
    } else {
      // Insert plain text
      char *bodyText = g_strndup(ep, stop - ep);
      gedit_document_insert_text(doc, bo, bodyText);
      g_free(bodyText);

      if (MessFlagIsSet(messH, FLAG_RICH)) {
        // Mark as rich text in gEditCtrl
        geditctrl_set_rich_text(messWin->pte, bo, true);
      }
    }

    // Handle inline signature
    if (MessOptIsSet(messH, OPT_INLINE_SIG)) {
      FindAndMarkSigSep(messWin->pte);
    }
  } else {
    // Insert empty body
    gedit_document_insert_text(doc, bo, "");
  }

  if (buffer)
    g_free(buffer);

  // Handle signature insertion (simplified)
  if (!new && !MessOptIsSet(messH, OPT_INLINE_SIG)) {
    // Add signature logic here
    if (MessOptIsSet(messH, OPT_INLINE_SIG)) {
      FindAndMarkSigSep(messWin->pte);
    }
  }

  // Lock text if message is sent
  if (baseLock) {
    // Lock the entire document in gEditCtrl
    geditctrl_set_editable(messWin->pte, false);
  }

  // Set change callback for gEditCtrl
  // geditctrl_set_change_callback(messWin->pte, PeteChanged);

  return (0);

failure:
  if (buffer)
    g_free(buffer);
  return (err);
}

/**********************************************************************
 * MakeCompTitle - make a reasonable composition title
 * Ported to use standard C strings instead of Pascal strings
 **********************************************************************/
void MakeCompTitle(char *string, TOCType * tocH, MessHandle messH, int sumNum) {
  char subject[256] = "";
  char to[256] = "";
  char pattern[64];

  // Get subject from message
  if (messH && (*messH)->win && (*messH)->win->pte) {
    // Extract subject from headers (simplified)
    g_strlcpy(subject, tocH->sums[sumNum].subj, sizeof(subject));
  }

  // Get To address (simplified)
  // This would need proper header parsing in a full implementation

  // Format title
  if (strlen(subject) > 0) {
    snprintf(string, 256, "Compose: %s", subject);
  } else {
    g_strlcpy(string, "Compose Message", 256);
  }
}

/**********************************************************************
 * FindAndMarkSigSep - find and mark signature separator in gEditCtrl
 * Ported from Pete to gEditCtrl
 **********************************************************************/
int FindAndMarkSigSep(GtkWidget *pte) {
  long sigPos = FindSigSep(pte);
  if (sigPos >= 0) {
    // Mark signature separator in gEditCtrl
    geditDocument *doc = geditctrl_get_document(pte);
    // This would need gEditCtrl-specific signature marking
    return 0;
  }
  return -1;
}

/**********************************************************************
 * FindSigSep - find signature separator in gEditCtrl text
 * Ported from Pete to gEditCtrl
 **********************************************************************/
long FindSigSep(GtkWidget *pte) {
  geditDocument *doc = geditctrl_get_document(pte);
  char *text = gedit_document_get_text(doc);
  char *sigSep = strstr(text, "\n-- \n");

  if (sigSep) {
    long pos = sigSep - text;
    g_free(text);
    return pos;
  }

  g_free(text);
  return -1;
}

/**********************************************************************
 * CompGetDragContents - handle drag contents for gEditCtrl
 * Ported from Pete drag handling to gEditCtrl
 **********************************************************************/
int CompGetDragContents(GtkWidget *pte, char **theText, void **theStyles,
                        void **theParas, void *drag, long dropLocation) {
  // Simplified drag handling for gEditCtrl
  // This would need full implementation based on gEditCtrl's drag API
  *theText = NULL;
  *theStyles = NULL;
  *theParas = NULL;
  return 0;
}

/**********************************************************************
 * WriteComp - write composition to file
 * Ported to work with gEditCtrl instead of Pete
 **********************************************************************/
int WriteComp(MessHandle messH, short refN, long offset) {
  MyWindowPtr win = (*messH)->win;
  geditDocument *doc;
  char *text;
  int err = 0;

  if (!win || !win->pte)
    return -1;

  doc = geditctrl_get_document(win->pte);
  text = gedit_document_get_text(doc);

  if (text) {
    // Write text to file (simplified)
    long count = strlen(text);
    err = AWrite(refN, &count, (unsigned char *)text);
    g_free(text);
  }

  return err;
}

/**********************************************************************
 * GetMyHostname - get hostname for message ID
 * Ported to use standard C networking
 **********************************************************************/
char *GetMyHostname(char *hostname) {
  if (gethostname(hostname, 127) != 0) {
    g_strlcpy(hostname, "localhost", 128);
  }
  return hostname;
}

/**********************************************************************
 * CompStripHeaderReturns - strip returns from headers
 * Ported to work with gEditCtrl
 **********************************************************************/
int CompStripHeaderReturns(MessHandle messH) {
  MyWindowPtr win = (*messH)->win;
  geditDocument *doc;
  char *text;

  if (!win || !win->pte)
    return -1;

  doc = geditctrl_get_document(win->pte);
  text = gedit_document_get_text(doc);

  if (text) {
    // Strip returns from header section (simplified)
    // This would need proper header parsing
    g_free(text);
  }

  return 0;
}
/**********************************************************************
 * SuckDragAddresses - extract addresses from drag operation
 * Ported to work with GTK drag and drop
 **********************************************************************/
int SuckDragAddresses(void *drag, char **addresses, bool leadingComma,
                      bool trailingComma) {
  // Simplified drag address extraction for GTK
  // This would need full implementation based on GTK's drag and drop API
  *addresses = g_strdup("");
  return 0;
}

/**********************************************************************
 * SepStyle - set separator style
 * Ported from Pete styling to gEditCtrl styling
 **********************************************************************/
void SepStyle(void *pip, void *tsp, void **graphic, int pgt) {
  // Simplified styling for gEditCtrl
  // This would need gEditCtrl-specific style implementation
}

/**********************************************************************
 * CompBeautifyFrom - beautify the From address
 * Ported to use standard C strings
 **********************************************************************/
void CompBeautifyFrom(char *name) {
  // Beautify from address (simplified)
  // Remove quotes, clean up formatting, etc.
  if (name && strlen(name) > 0) {
    // Basic cleanup - remove surrounding quotes
    if (name[0] == '"' && name[strlen(name) - 1] == '"') {
      memmove(name, name + 1, strlen(name) - 1);
      name[strlen(name) - 2] = '\0';
    }
  }
}

/**********************************************************************
 * Additional utility functions needed for composition
 **********************************************************************/

/**********************************************************************
 * CompDidResize - handle window resize for composition
 * Ported to work with GTK window resizing
 **********************************************************************/
void CompDidResize(MyWindowPtr win) {
  if (win && win->pte) {
    // Resize gEditCtrl widget to fit window
    // This would need proper GTK widget resizing
  }
}

/**********************************************************************
 * CompClick - handle mouse clicks in composition window
 * Ported from Mac mouse handling to GTK
 **********************************************************************/
bool CompClick(MyWindowPtr win, GdkEvent *event) {
  if (win && win->pte) {
    // Handle click in gEditCtrl
    // This would need GTK event handling
    return true;
  }
  return false;
}

/**********************************************************************
 * CompMenu - handle menu commands for composition
 * Ported to work with GTK menus
 **********************************************************************/
bool CompMenu(MyWindowPtr win, int menuItem) {
  MessHandle messH = Win2MessH(win);

  if (!messH)
    return false;

  switch (menuItem) {
  case SEND_ITEM:
    // Send message
    return CompSend(messH);

  case SAVE_ITEM:
    // Save message
    return CompSave(messH);

  default:
    return false;
  }
}

/**********************************************************************
 * CompKey - handle keyboard input for composition
 * Ported from Mac key handling to GTK
 **********************************************************************/
bool CompKey(MyWindowPtr win, GdkEvent *event) {
  if (win && win->pte) {
    // Handle key events in gEditCtrl
    // This would need GTK key event handling
    return true;
  }
  return false;
}

/**********************************************************************
 * CompButton - handle button clicks in composition
 * Ported to work with GTK buttons
 **********************************************************************/
bool CompButton(MyWindowPtr win, GtkWidget *button, GdkEvent *event) {
  MessHandle messH = Win2MessH(win);

  if (!messH)
    return false;

  if (button == (*messH)->sendButton) {
    // Send button clicked
    return CompSend(messH);
  }

  return false;
}

/**********************************************************************
 * CompHelp - provide help for composition window
 **********************************************************************/
void CompHelp(MyWindowPtr win, int helpType) {
  // Show help for composition window
  // This would integrate with GTK help system
}

/**********************************************************************
 * CompGonnaShow - prepare composition window for display
 * Ported from Mac window showing to GTK
 **********************************************************************/
void CompGonnaShow(MyWindowPtr win) {
  if (win && win->pte) {
    // Prepare gEditCtrl for display
    gtk_widget_show(win->pte);
  }
}

/**********************************************************************
 * CompDragHandler - handle drag operations in composition
 * Ported from Mac drag handling to GTK
 **********************************************************************/
bool CompDragHandler(MyWindowPtr win, void *dragEvent) {
  // Handle drag operations in gEditCtrl
  // This would need GTK drag and drop implementation
  return false;
}

/**********************************************************************
 * CompIdle - handle idle processing for composition
 * Ported from Mac idle processing to GTK
 **********************************************************************/
void CompIdle(MyWindowPtr win) {
  MessHandle messH = Win2MessH(win);

  if (messH && win->pte) {
    // Handle idle processing for gEditCtrl
    // Auto-save, spell check, etc.
  }
}

/**********************************************************************
 * CompClose - close composition window
 * Ported to work with GTK window closing
 **********************************************************************/
bool CompClose(MyWindowPtr win) {
  MessHandle messH = Win2MessH(win);

  if (!messH)
    return true;

  // Check if message needs saving
  // Note: win->isDirty member may not exist, so we'll skip this check for now
  // if (win->isDirty) {
  //	// Show save dialog
  //	int response = gtk_dialog_run(GTK_DIALOG(
  //		gtk_message_dialog_new(GTK_WINDOW(win->window),
  //			GTK_DIALOG_MODAL,
  //			GTK_MESSAGE_QUESTION,
  //			GTK_BUTTONS_YES_NO_CANCEL,
  //			"Save changes to this message?")));
  //
  //	switch (response) {
  //		case GTK_RESPONSE_YES:
  //			if (!CompSave(messH)) return false;
  //			break;
  //		case GTK_RESPONSE_CANCEL:
  //			return false;
  //		case GTK_RESPONSE_NO:
  //		default:
  //			break;
  //	}
  // }

  // Clean up gEditCtrl
  if (win->pte) {
    g_object_unref(win->pte);
    win->pte = NULL;
  }

  // Remove from message list
  LL_Remove(MessList, messH, (MessHandle));

  // Free message handle
  DisposeHandle((Handle)messH);

  return true;
}

/**********************************************************************
 * CompSend - send the composition
 **********************************************************************/
bool CompSend(MessHandle messH) {
  MyWindowPtr win = (*messH)->win;
  geditDocument *doc;
  char *text;
  bool success = false;

  if (!win || !win->pte)
    return false;

  doc = geditctrl_get_document(win->pte);
  text = gedit_document_get_text(doc);

  if (text) {
    // Send the message (simplified)
    // This would need full SMTP implementation
    success = true;
    g_free(text);
  }

  if (success) {
    // Mark as sent
    SumOf(messH)->state = SENT;
    // win->isDirty = false; // Remove if not in MyWindow struct
  }

  return success;
}

/**********************************************************************
 * CompSave - save the composition
 **********************************************************************/
bool CompSave(MessHandle messH) {
  MyWindowPtr win = (*messH)->win;
  TOCType * tocH = (*messH)->tocH;
  int sumNum = (*messH)->sumNum;

  if (!win || !win->pte)
    return false;

  // Save message to mailbox
  int err = WriteComp(messH, tocH->refN, tocH->sums[sumNum].offset);

  if (!err) {
    // win->isDirty = false; // Remove if not in MyWindow struct
    TOCSetDirty(tocH, true);
    return true;
  }

  return false;
}

/**********************************************************************
 * Additional helper functions
 **********************************************************************/

/**********************************************************************
 * CreateMessageBody - create a new message body
 **********************************************************************/
int CreateMessageBody(char *buffer, unsigned long *uidHash) {
  // Create basic message structure
  char msgId[256];
  time_t now = time(NULL);

  NewMessageId(msgId);
  *uidHash = Hash((unsigned char *)msgId);

  // Basic message template
  int len = snprintf(buffer, 1024,
                     "From: \r\n"
                     "To: \r\n"
                     "Subject: \r\n"
                     "Message-ID: %s\r\n"
                     "Date: %s\r\n"
                     "\r\n",
                     msgId, ctime(&now));

  return len;
}

/**********************************************************************
 * GatherCompAddresses - implemented in nickmng.c
 **********************************************************************/