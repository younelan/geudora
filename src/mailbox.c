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
#include <stdbool.h>
#include <string.h>
#include "Globals.h"

/*
 * IsRoot - is the path's parent the MailRoot.path path?
 */
bool IsRoot(const char *path) {
  char parent[1024];
  /* use strncpy to avoid depending on glib here (headers included later) */
  strncpy(parent, path, sizeof(parent) - 1);
  parent[sizeof(parent) - 1] = '\0';
  char *p = strrchr(parent, '/');
  if (!p)
    return false;
  *p = '\0';
  return (strcmp(parent, MailRoot.path) == 0);
}
#include "features.h"
#include "fileutil.h"
extern void MBOpenFolder(void *hStringList, bool isIMAP);
#include <fcntl.h>
#include <sys/stat.h>
/* junk.h removed — macmbx_junk handles junk */
#include "lineio.h"
#include "mesg_error_store.h"
#include "mydefs.h"
#include "MyRes.h"
/* pop.h removed — crispy_pop3 handles POP */
#include "print.h"
#include "sort.h"
#include "toc.h"
#include "gtk_prefs.h"
/* trans.h removed — crispy handles network */
#include <assert.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef ASSERT
#define ASSERT(x) assert(x)
#endif
#include "gtk_menus.h"
#include "boxact.h"
/* imapmailboxes.h removed — crispy_imap handles IMAP */
#include "threading.h"
int AddMesgError(MacmbxTOC * tocH, short sum, char *errorStr,
                 int errorCode);
#include "message.h"  /* For MyWindow struct */
#include "prefdefs.h" /* For PREF_THREADING_OFF */
#include <stdlib.h>

#include "comp.h"
#include "gtk_menus.h"
#include "log.h"
#include "theme.h"
/* uudecode.h removed — crispy handles encoding */
#include "macmbx.h"
#include "gtk_mailbox.h"
#include "StringUtil.h"
#include "StringDefs.h"
#include <stdbool.h>

/* IMAP removed — crispy_imap + macmbx handle everything. */
int ReallyDoAnAlert(int templ, int which);
#ifndef kStatReceivedMail
#define kStatReceivedMail 0
#endif

/* Missing Definitions */
#ifndef GtkWidget *
/* WindowPtr typedef removed — use GtkWidget * directly */
#endif
/* WindowPtr typedef removed — use GtkWidget * directly */

#define kAMOAvoidAll 2
#ifndef smSystemScript
enum { smSystemScript = 0 };
#endif
static short BoxMapCount = 0;
enum { kStatReadMsg = 0 };
#ifndef nil
#define nil 0
#endif
#define ECANCELED -128
/* OPT_OPEN and OPT_AUTO_OPENED defined in mailbox.h */
/* MAILBOX_MENU defined in gtk_menus.h */

/* Backwards-compatible menu ID names expected by legacy code */
#ifndef MAILBOX_MENU
#define MAILBOX_MENU MENU_MAILBOX
#endif
#ifndef TRANSFER_MENU
#define TRANSFER_MENU MENU_TRANSFER
#endif

/* Missing Structs */
/* Missing Definitions / Decls */
/* FSSpec defined in headers */
/* MemError defined in headers */
/* PCopyTrim defined in headers */
/* ResolveAliasOrElse defined in headers */
/* BoxFOpen is the real implementation — stub removed from mac_stubs.c */
#define LOG_FLOW 7

/* Missing definitions restored */
/* noErr removed — use 0 directly */
extern long AnyTOCDirty;
typedef unsigned long uLong;
struct MenuAndScore;
typedef struct MenuAndScore *MenuAndScoreHandle;
/* BoxCenterSelection declared in boxact.h */

int BoxMatchMenuItems(unsigned char *name, MenuAndScoreHandle *mashPtr,
                      int score(), int *outCount);
int BoxMatchMenuItemsInMenu(MenuHandle mh, AccuPtr a, unsigned char *name,
                            int score());
int BoxMatchMenuItemsIn1Menu(MenuHandle mh, AccuPtr a, unsigned char *name,
                             int score());

// ... existing declarations ...

// Fix pointer math in MailboxAlias/SpecAlias usage area
#define MAILBOX_ALIAS_MATH(leaf, spot, name)                                   \
  { size_t _mpl = ( *name - ((char *)spot - name)); memcpy(leaf,  spot + 1, _mpl); ((char*)(leaf))[_mpl] = '\0'; }
char *MailboxSpecAlias(const char *specPath, char *name);
void BuildBoxMenus(void);
bool DeleteSum(MacmbxTOC * tocH, int sumNum);
void MakeMessTitle(unsigned char *name, MacmbxTOC * tocH, int sumNum, bool b);
bool IsQueued(MacmbxTOC * tocH, int sumNum);

/* New Missing Decls */
void MBTickle(void *u1, void *u2);
void AddBoxHigh(const char *specPath);
unsigned char *PeteSelectedString(void *u1, void *pte);
bool MenuItemIsSeparator(MenuHandle mh, short item);
/* CollapseLWSP declared in StringUtil.h as char *CollapseLWSP(char *s) */
void AppendMenu(MenuHandle mh, const unsigned char *item);
void SetMenuItemCommandID(MenuHandle mh, short item, long id);
void SetMenuItemHierarchicalMenu(MenuHandle mh, short item, MenuHandle subMenu);
void SelectBoxRange(MacmbxTOC * tocH, int start, int end, bool add, int u1,
                    int u2);

/* Restored declarations */
/* GetRString declared in gtk_dialogs.h */
/* HGetState/HSetState provided by legacy_shim.h */
char *FindHeaderString(char *text, char *headerName, long *size, bool bodyToo);
void MBDrawerOpen(MyWindowPtr win);
/* MacmbxTOC * GetOutTOC(void); - Redundant macro conflict */
/* BeautifyFrom → crispy_rfc822_beautify_from in crispy_rfc822.h */
MyWindowPtr GetNewMyDialog(short id, void *w, void *h, void *behind);
void CycleBalls(void);
long GetPrefLong(short pref);
/* SetWTitle declared in mailbox.h */
/* GetAMessage declared in message.h */
void RemoveUTF8FromSum(MacmbxMsgSum * sum);
/* SetHandleBig declared in util.h */
long TempMaxMem(long *grow);
short FindDirLevel(short vRefNum, long dirID);
/* DeleteMenuItem, EnableItem, etc. are in gtk_menus.h */
/* FindItemByName, MyGetItem, CountMenuItems, etc. are in gtk_menus.h */
void HideDialogItem(DialogPtr dp, short item);
/* MyParamText declared in log.h with char * params */
void StartMovableModal(DialogPtr dp);
void ShowWindow(GtkWidget * win);
/* Cursor and Dialog Decls */
GtkWidget * GetDialogWindow(DialogPtr dp);
#define GetMyWindowDialogPtr(win) ((DialogPtr)GetMyWindowWindowPtr(win))
/* Legacy Dialog Functions */
/* CloseMyWindow provided by legacy_shim.h */
MacmbxTOC * macmbx_registry_find(const char *path);
void utl_SaveWindowPos(GtkWidget * win, Rect *r, bool *zoomed);
char *GetMailboxName(MacmbxTOC * tocH, short sum, char *name);
/* Port/window helpers are provided by platform headers or central stubs; remove
  redundant prototypes here to avoid duplicate declarations. */
typedef struct BoxMapStruct BoxCountElem; /* Guessing same layout */
void GetMenuItemText(MenuHandle mh, short item, unsigned char *text);
/* Restored Function Declarations */
void utl_RestoreWindowPos(GtkWidget * win, Rect *r, bool zoomed, short u1,
                          short u2, short u3, void *cb1, void *cb2);
short MessWi(MyWindowPtr win);
typedef struct BoxMapStruct BoxMapType;
/* Title bar / rim helpers handled by platform-specific code. */
void FigureZoom(void);
void DefPosition(void);
void AddBoxCountItem(short item, short vRef, long dirId);
void AddBoxCountMenu(short menu, short item, short vRef, long dirId,
                     bool force);
void MovableModalDialog(DialogPtr dp, void *filter, short *item);
void SetDItemState(DialogPtr dp, short item, bool state);
bool GetDItemState(DialogPtr dp, short item);
void PopCursor(void);
/* Dialog item text helpers implemented centrally (mac_stubs.c) */
bool BadMailboxName(char * spec, bool folder);
bool BadMailboxNameChars(char * spec);
void TooLong(unsigned char *name); /* Fixed const */
void EndMovableModal(DialogPtr dp);
void MyDisposeDialog(DialogPtr dp);
short HRename(short vRefNum, long dirID, const unsigned char *oldName,
              const unsigned char *newName);
struct BoxMapStruct {
  short vRef;
  long dirId;
  short item;
};
void HiliteButtonOne(DialogPtr dp);
void SelectDialogItemText(DialogPtr dp, short item, short start, short end);
void GetDIText(DialogPtr dp, short item, unsigned char *name);
void PushCursor(void *cursor);
extern void *iBeamCursor; /* Likely global */

/* GetItemStyle, SetItemStyle, CountMenuItems, SubmenuId are in gtk_menus.h */
bool IsQueuedState(int state);
void MenuID2VD(short menuID, short *vRef, long *dirID);

#define LOG_FILT 6 /* Stub */
bool DeleteSum(MacmbxTOC * tocH, int sumNum);
void CycleBalls(void);
long GetPrefLong(short pref);
/* SetWTitle, ShowMyWindow, UserSelectWindow, GetNewMyWindow, OpenMailbox,
   InitMailboxWin, MyDisposeWindow — all declared in mailbox.h */
/* `IsWindowVisible` is an inline in include/mailbox.h */
long TOCDelDup(MacmbxTOC * tocH);
MacmbxTOC * GetTOCFromSearchWin(char * spec);

/* Map legacy types to shim types */
/* mesgErrorPtr and MesgErrorType moved to top */
/* mesgErrorHandle is defined in legacy_shim.h */

typedef struct MenuAndScore {
  short menu;
  short item;
  long score;
} MenuAndScore, *MenuAndScorePtr, *MenuAndScoreHandle;

void ZeroMailbox(MacmbxTOC * tocH);
int AddBoxMap(short vRef, long dirId);
bool WantRebuildTOC(unsigned char * boxName, int why);
void AddBox(short function, unsigned char * name, short level, bool unread);
void RemoveBox(short function, unsigned char * name, short level);
int Path2Box(char *path, char * box);
/* Forward declaration to ensure calls earlier in this file see the
 * canonical implementation declared/defined here (mailbox.h may vary
 * between platforms). Kept local to this translation unit.
 */
/* AddMesgError implementation is below */
int BoxSpecByNameInMenu(MenuHandle mh, char * spec, unsigned char *name);
long TOCDelEmpty(MacmbxTOC * tocH);
short FindBoxByNameIn1Menu(MenuHandle mh, unsigned char *name);
int RedoWho(MacmbxTOC * tocH, short sumNum);
int ChainTrash(char * spec);
void SetSumColorLo(MacmbxTOC * tocH, short sumNum, short color);
void SetStateLo(MacmbxTOC * tocH, int sumNum, int state);
void SetState(MacmbxTOC * tocH, int sumNum, int state);

int BoxMatchScore(unsigned char *name, unsigned char *candidate);
/* IsFromLine → crispy_rfc822_is_from_line */
#include "crispy_rfc822.h"
int BoxMatchMenuItemsIn1Menu(MenuHandle mh, AccuPtr a, unsigned char *name,
                             int score());
int CompareMAS(MenuAndScorePtr mas1, MenuAndScorePtr mas2);
void SwapMAS(MenuAndScorePtr mas1, MenuAndScorePtr mas2);
void DeleteMessageLo(MacmbxTOC * tocH, int sumNum, bool nuke);

/* Allocate/free helpers that replace legacy "Handle" based NuHandle
 * allocation. These keep mailbox.c self-contained and use standard
 * malloc/calloc so we don't touch other files.
 */
/* Use the project's portable void *APIs rather than redefining Mac
 * structs here. Allocate a fixed buffer large enough for expected
 * mesg-error contents (uid + 256-byte pascal string + error code).
 */
#define MESG_ERR_BUF_SIZE (sizeof(uLong) + 256 + sizeof(int))


/************************************************************************
 * TOCSetDirty - set the dirty bit
 ************************************************************************/
void TOCSetDirty(MacmbxTOC * tocH, bool dirty) {
  if (!tocH) return;
  tocH->dirty = dirty;
  AnyTOCDirty++;
}

/************************************************************************
 * AddOutgoingMesgError
 ************************************************************************/
int AddOutgoingMesgError(short sumNum, uLong uidHash, int errorCode,
                         int template, ...) {
  MacmbxTOC * tocH = NULL;
  MacmbxTOC * tempTocH = NULL;
  short outSumNum = sumNum;
  char fmtdError[256], error[256];
  va_list args;

  if (InAThread()) {
    tocH = GetRealOutTOC();
    tempTocH = GetTempOutTOC();
    outSumNum = FindSumByHash(tocH, uidHash);
  } else
    tempTocH = tocH = GetRealOutTOC();

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
  return (0);
}

/************************************************************************
 * DeleteMesgError
 ************************************************************************/

int DeleteMesgError(MacmbxTOC * tocH, short sum) {
  mesgErrorHandle mesgErrH;

  if ((mesgErrH = (mesgErrorHandle)tocH->msgs[sum].mesgErrH)) {
    /* Free the flat mesgError struct */
    free(mesgErrH);
    tocH->msgs[sum].mesgErrH = NULL;
    /* persist changes to sidecar */
    mesg_error_store_save_all(tocH);
  }
  return 0;
}

/************************************************************************
 * AddMesgError
 ************************************************************************/

int AddMesgError(MacmbxTOC * tocH, short sum, char *errorStr,
                 int errorCode) {
  int err = 0;
  (void)err; /* Error ignored according to legacy comment below */
  mesgErrorHandle mesgErrH = NULL;

  /* tocH and sum should be valid, mesgErrH should be empty */
  ASSERT(tocH && (sum != -1) && !tocH->msgs[sum].mesgErrH &&
         (sum < tocH->count));
  if (!(tocH && (sum != -1) && (sum < tocH->count)))
    return -1;

  /* if for some reason, mesgErrH isn't empty, overwrite it */
  mesgErrH = (mesgErrorHandle)tocH->msgs[sum].mesgErrH;
  if (!mesgErrH) {
    mesgErrH = (mesgErrorHandle)calloc(1, sizeof(MesgErrorType));
    if (!mesgErrH)
      err = 0;
  }
  if (mesgErrH) {
    /* For the GTK port we do not write message-error resources to
     * classic Mac resource forks. Keep the mesgError in memory
     * attached to the TOC so the UI can display errors. Persistence
     * of these errors can be added later via sidecar files.
     */
    if (errorStr)
      PCopyTrim(mesgErrH->errorStr, errorStr, sizeof(mesgErrH->errorStr));
    mesgErrH->uidHash = tocH->msgs[sum].uid_hash;
    mesgErrH->errorCode = errorCode;
  }
  // let's ignore the error since we can set the mesg state
  tocH->msgs[sum].state = MESG_ERR;
  tocH->msgs[sum].mesgErrH = (void *)mesgErrH;
  TOCSetDirty(tocH, true);
  tocH->dirty = true;
  /* persist current mesg error state to sidecar */
  mesg_error_store_save_all(tocH);
  return (0);
}

/************************************************************************
 * FillMesgErrors - fill toc
 ************************************************************************/

int FillMesgErrors(MacmbxTOC * tocH) {
  /* Load per-mailbox JSON sidecar and populate in-memory mesgErrH entries.
   * Legacy resource-fork persistence removed during GTK port.
   */
  ASSERT(tocH);
  if (!tocH)
    return EINVAL;
  return mesg_error_store_load(tocH);
}

/**********************************************************************
 * GetMailbox - put a mailbox window frontmost; open if necessary
 **********************************************************************/
int GetMailbox(const char *path, bool showIt) {
  MacmbxTOC * toc;
  FSSpec tmpSpec;

  /* Build a temporary FSSpec from the POSIX path for legacy helpers */
  spec_make(NULL, path, &tmpSpec);

  if (ResolveAliasOrElse(&tmpSpec, nil, nil))
    return (ECANCELED);

  if ((toc = macmbx_registry_find(tmpSpec))) {
    GtkWidget * tocWinWP;
    tocWinWP = GetMyWindowWindowPtr(toc->win);
    UsingWindow(tocWinWP);
    if (showIt) {
      if (!IsWindowVisible(tocWinWP)) {
        ShowMyWindow(tocWinWP);
      }
    }
    UserSelectWindow(tocWinWP);
    return (0);
  }

  return OpenMailbox(tmpSpec, showIt, NULL);
}

/**********************************************************************
 * OpenMailbox - open the named mailbox
 **********************************************************************/
int OpenMailbox(const char *path, bool showIt, MacmbxTOC * toc) {
  MyWindow *win;
  GtkWidget * winWP;
  FSSpec tmpSpec;
  spec_make(NULL, path, &tmpSpec);

  /*
   * create window
   */
  MyThreadBeginCritical(); // We may be in a thread. Don't yield until the
                           // window is all set up.
  if ((win = GetNewMyWindow(MAILBOX_WIND, nil, nil, showIt ? BehindModal : 0,
                            true, true, MBOX_WIN)) == nil) {
    WarnUser(COULDNT_WIN, 0);
    MyThreadEndCritical();
    return (0);
  }

  winWP = GetMyWindowWindowPtr(win);

  // win->hPitch = FontWidth;
  // win->vPitch = FontLead+FontDescent;

  /*
   * read or build toc for window if we don't have it yet
   */
  if (!toc & !(toc = GetTOCFromSearchWin(&tmpSpec))) {
    toc = macmbx_toc_open(&tmpSpec);
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
  if (showIt & PrefIsSet(PREF_DELDUP))
    TOCDelDup(toc); // don't bother deleting dups unless we're going to show the
                    // mailbox.

  // Show the window if the caller wants
  if (showIt) {
    ShowMyWindow(winWP);

    // Open mailbox drawer?
    if (toc->drawer & !toc->drawerWin) {
      // Don't open draw if there is one already open
      // Open drawer
      MacmbxTOC * tocTemp;

      for (tocTemp = macmbx_registry_head(); tocTemp; tocTemp = tocTemp->next) {
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

  MyThreadEndCritical();

  return 0;
}

/**********************************************************************
 * InitMailboxWin - initialize mailbox window data
 **********************************************************************/
/* BoxClose, BoxButton, BoxMenu, BoxGonnaShow, BoxPosition, BoxFind — in boxact.c */
void BoxCursor(Point mouse) {}
void MBDrawerOpen(MyWindowPtr win) {
  if (!win) return;
  GtkWidget *parent = GetMyWindowWindowPtr(win);
  if (!parent) return;

  MacmbxTOC *toc = (MacmbxTOC *)GetMyWindowPrivateData(win);
  if (!toc) return;
  if (toc->drawerWin) return; /* already open */

  GtkWidget *drawer = gtk_window_new();
  /* Make it transient for the mailbox window so WM positions it sensibly */
  if (GTK_IS_WINDOW(drawer) & GTK_IS_WINDOW(parent))
    gtk_window_set_transient_for(GTK_WINDOW(drawer), GTK_WINDOW(parent));

  /* Reasonable default size; prefs may drive this later */
  gtk_window_set_default_size(GTK_WINDOW(drawer), 320, 480);

  /* Minimal content for now: a container that other code can populate later */
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(drawer), box);

  /* Track the drawer widget on the TOC so callers can detect/open/close it */
  toc->drawerWin = (void *)drawer;

  /* Present the drawer */
  gtk_window_present(GTK_WINDOW(drawer));
}

/* Column indices for mailbox list */
enum {
  COL_STATUS = 0,    /* message state icon/text */
  COL_PRIORITY,      /* priority 1-5 */
  COL_ATTACH,        /* attachment indicator */
  COL_LABEL,         /* label/color number */
  COL_WHO,           /* From (incoming) or To (outgoing) */
  COL_DATE,          /* date string */
  COL_SIZE,          /* size string */
  COL_JUNK,          /* junk score */
  COL_SUBJECT,       /* subject */
  COL_INDEX,         /* hidden: TOC index */
  COL_LABEL_COLOR,   /* hidden: foreground color from label */
  NUM_MBOX_COLS
};

/* Context for message list selection callback */
typedef struct {
  MacmbxTOC *toc;
  GtkWidget *preview;      /* Preview container box */
  GtkWidget *preview_hdr;  /* Header grid container */
  GtkWidget *preview_body; /* Body GtkTextView */
} MboxSelCtx;

/* Forward declarations for callbacks */
static void on_mbox_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                  GtkTreeViewColumn *column, gpointer data);
static void attach_mbox_context_menu(GtkWidget *tree, MacmbxTOC *toc);

/* Forward declarations for shared header helpers */
char *read_raw_headers(MacmbxTOC *tocH, int sumNum, long *hdr_len);

/* ── Drag source for message list (drag messages to sidebar mailboxes) ── */

/* GTK4 DnD: attach GtkDragSource to the ScrolledWindow that wraps the TreeView.
 * This avoids conflicting with TreeView's internal gesture handling.
 * The prepare callback reads the TreeView selection. */

static GdkContentProvider *on_msg_drag_prepare(GtkDragSource *source,
                                                double x, double y,
                                                gpointer ud) {
  (void)source; (void)x; (void)y;
  GtkWidget *tree = GTK_WIDGET(ud);
  MacmbxTOC *toc = g_object_get_data(G_OBJECT(tree), "mbox-toc");
  if (!toc) return NULL;

  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  if (gtk_tree_selection_count_selected_rows(sel) == 0) return NULL;

  GList *rows = gtk_tree_selection_get_selected_rows(sel, NULL);
  if (!rows) return NULL;

  GString *data = g_string_new("msg:");
  g_string_append(data, toc->mbox_path);
  g_string_append_c(data, '\t');

  bool first = true;
  for (GList *l = rows; l; l = l->next) {
    GtkTreePath *path = l->data;
    int *indices = gtk_tree_path_get_indices(path);
    if (indices) {
      if (!first) g_string_append_c(data, ',');
      g_string_append_printf(data, "%d", indices[0]);
      first = false;
    }
  }
  g_list_free_full(rows, (GDestroyNotify)gtk_tree_path_free);

  char *str = g_string_free(data, FALSE);
  GdkContentProvider *provider = gdk_content_provider_new_typed(G_TYPE_STRING, str);
  g_free(str);
  return provider;
}

/* Attach drag source directly to the message list TreeView.
 * GtkDragSource with propagation phase BUBBLE avoids interfering
 * with TreeView's row selection (which uses CAPTURE phase). */
static void attach_msg_drag_source(GtkWidget *tree, MacmbxTOC *toc) {
  g_object_set_data(G_OBJECT(tree), "mbox-toc", toc);

  GtkDragSource *drag = gtk_drag_source_new();
  gtk_drag_source_set_actions(drag, GDK_ACTION_MOVE);
  g_signal_connect(drag, "prepare", G_CALLBACK(on_msg_drag_prepare), tree);
  gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(drag),
                                              GTK_PHASE_BUBBLE);
  gtk_widget_add_controller(tree, GTK_EVENT_CONTROLLER(drag));
}

/* No-op — kept for compatibility with existing call sites */
static void wire_msg_drag_to_scroll(GtkWidget *scroll, GtkWidget *tree) {
  (void)scroll; (void)tree;
}
static gchar *extract_header(const char *text, long textLen, const char *name);
static GtkWidget *build_header_grid(const char *raw, long hdr_len, MacmbxMsgSum * sum);

/* Message list selection callback */
/* Find blank line separating headers from body.
 * A "blank line" is two consecutive line endings with nothing between them.
 * Handles any mix of \r\n, \r, \n line endings (real emails mix them). */
static long find_body_start(const char *text, long len) {
  long i = 0;
  /* Skip leading blank lines (mbox offset may include trailing newlines) */
  while (i < len & (text[i] == '\r' || text[i] == '\n'))
    i++;
  while (i < len) {
    /* Skip to end of current line (find the line ending) */
    while (i < len & text[i] != '\r' & text[i] != '\n')
      i++;
    if (i >= len) break;
    /* Skip this line ending */
    long eol_start = i;
    if (text[i] == '\r') { i++; if (i < len & text[i] == '\n') i++; }
    else if (text[i] == '\n') { i++; }
    /* Check if next char is also a line ending (= blank line) */
    if (i < len & (text[i] == '\r' || text[i] == '\n')) {
      /* Skip the second line ending to get body start */
      if (text[i] == '\r') { i++; if (i < len && text[i] == '\n') i++; }
      else if (text[i] == '\n') { i++; }
      return i;
    }
    /* Check if we're at start of a continuation line (whitespace = folded header) */
    /* Not a blank line, continue scanning */
  }
  return 0;
}

/* Extract a header value using Eudora's FindHeaderString.
 * Returns newly allocated UTF-8 string or NULL.
 * name is without colon, e.g. "To", "From" — colon is added here. */
static gchar *extract_header(const char *text, long textLen, const char *name) {
  char hdr[MAX_HEADER];
  snprintf(hdr, sizeof(hdr), "%s:", name);
  long size = textLen;
  char *val = FindHeaderString((char *)text, hdr, &size, false);
  if (!val || size <= 0) return NULL;
  gchar *dup = g_strndup(val, size);
  gchar *utf8 = ensure_utf8(dup);
  if (utf8 != dup) g_free(dup);
  return utf8;
}

static void on_mbox_msg_selected(GtkTreeSelection *sel, gpointer data) {
  MboxSelCtx *ctx = (MboxSelCtx *)data;
  MacmbxTOC *toc = ctx->toc;
  GtkWidget *preview = ctx->preview;
  GtkTreeModel *model;
  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return;
  int idx = -1;
  gtk_tree_model_get(model, &iter, COL_INDEX, &idx, -1);
  if (idx < 0 || idx >= toc->count) return;

  /* Read the message from the mailbox file — same path as full view */
  MacmbxMsgSum * sum = &toc->msgs[idx];
  GtkWidget *hdr_box = ctx->preview_hdr;
  GtkWidget *body_tv = ctx->preview_body;
  if (!hdr_box || !body_tv) return;

  /* Clear previous header grid */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(hdr_box)))
    gtk_box_remove(GTK_BOX(hdr_box), child);

  /* Build header grid — same build_header_grid() as full view */
  long hdr_len = 0;
  char *raw = read_raw_headers(toc, idx, &hdr_len);
  geditDocument *doc = geditctrl_get_document(body_tv);
  gint docLen = gedit_document_get_length(doc);
  if (docLen > 0) gedit_document_delete_range(doc, 0, docLen);

  if (!raw) {
    gedit_document_insert_text(doc, 0, "(cannot open mailbox file)");
    return;
  }

  GtkWidget *grid = build_header_grid(raw, hdr_len, sum);
  gtk_box_append(GTK_BOX(hdr_box), grid);

  /* Set body text — use markup renderer for HTML messages */
  long rawTotal = strlen(raw);
  if (hdr_len < rawTotal) {
    gchar *body_utf8 = ensure_utf8(raw + hdr_len);
    bool isHTML = (sum->opts & OPT_HTML) != 0;
    if (isHTML)
      gedit_document_insert_markup(doc, 0, body_utf8);
    else
      gedit_document_insert_text(doc, 0, body_utf8);
    g_free(body_utf8);
  }
  g_free(raw);
}

/* Populate the message list from TOC summaries */
/* State to display string */
static const char *state_str(StateEnum s) {
  switch (s) {
    case UNREAD:       return "\xe2\x97\x8f"; /* ● */
    case READ:         return "";
    case REPLIED:      return "\xe2\x86\xa9"; /* ↩ */
    case FORWARDED:    return "\xe2\x86\x92"; /* → */
    case REDIST:       return "\xe2\x87\x89"; /* ⇉ */
    case UNSENDABLE:   return "\xe2\x9c\x8f"; /* ✏ */
    case SENDABLE:     return "\xe2\x9c\x93"; /* ✓ */
    case QUEUED:       return "\xe2\x8f\xb3"; /* ⏳ */
    case SENT:         return "\xe2\x9c\x89"; /* ✉ */
    case UNSENT:       return "\xe2\x9c\x8f"; /* ✏ */
    case TIMED:        return "\xe2\x8f\xb0"; /* ⏰ */
    case BUSY_SENDING: return "\xe2\x87\xa7"; /* ⇧ */
    case MESG_ERR:     return "\xe2\x9a\xa0"; /* ⚠ */
    case REBUILT:      return "";
    default:           return "";
  }
}

/* Priority to display string */
static const char *priority_str(int priority) {
  /* Priority: 1=Highest, 2=High, 3=Normal, 4=Low, 5=Lowest, 0=unset */
  switch (priority) {
    case 1: return "\xe2\x86\x91\xe2\x86\x91"; /* ↑↑ Highest */
    case 2: return "\xe2\x86\x91";              /* ↑  High */
    case 3: return "";                           /*    Normal */
    case 4: return "\xe2\x86\x93";              /* ↓  Low */
    case 5: return "\xe2\x86\x93\xe2\x86\x93"; /* ↓↓ Lowest */
    default: return "";
  }
}

/* Label number from flags (0=none, 1-7) using bits 14-17 */
static int label_from_flags(unsigned long flags) {
  int hue = 0;
  if (flags & FLAG_HUE1) hue |= 1;
  if (flags & FLAG_HUE2) hue |= 2;
  if (flags & FLAG_HUE3) hue |= 4;
  return hue;
}

/* Get label color as "#RRGGBB" string. Returns "" for no label. */
static const char *label_color_str(int label) {
  static char buf[8];
  if (label <= 0 || label > 7) return "";
  /* Default colors matching original Eudora */
  static const char *colors[] = {
    "", "#FF0000", "#0000FF", "#009900", "#990099",
    "#FF8000", "#009999", "#666666"
  };
  /* Try loading from prefs */
  char key[32];
  snprintf(key, sizeof(key), "color_r_%d", label - 1);
  int r = prefs_get_int(PREFS_GROUP_LABELS, key, -1);
  if (r >= 0) {
    snprintf(key, sizeof(key), "color_g_%d", label - 1);
    int g = prefs_get_int(PREFS_GROUP_LABELS, key, 0);
    snprintf(key, sizeof(key), "color_b_%d", label - 1);
    int b = prefs_get_int(PREFS_GROUP_LABELS, key, 0);
    snprintf(buf, sizeof(buf), "#%02X%02X%02X",
             (int)(r * 255 / 1000), (int)(g * 255 / 1000), (int)(b * 255 / 1000));
    return buf;
  }
  return (label < 8) ? colors[label] : "";
}

void populate_mbox_list(GtkListStore *store, MacmbxTOC *toc) {
  gtk_list_store_clear(store);
  bool isOut = (toc->which == OUT);
  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum * sum = &toc->msgs[i];
    /* Format date */
    char datebuf[32] = "";
    if (sum->seconds > 0) {
      time_t t = (time_t)sum->seconds;
      struct tm *tm = localtime(&t);
      if (tm) strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M", tm);
    }
    /* Format size */
    char sizebuf[16];
    if (sum->length >= 1024)
      snprintf(sizebuf, sizeof(sizebuf), "%ldK", sum->length / 1024);
    else
      snprintf(sizebuf, sizeof(sizebuf), "%ld", sum->length);

    /* Attachment indicator */
    const char *attach = (sum->flags & FLAG_HAS_ATT) ? "\xf0\x9f\x93\x8e" : "";  /* 📎 */

    /* Label — show colored circle ● */
    int label = label_from_flags(sum->flags);
    const char *labelbuf = (label > 0) ? "\xe2\x97\x8f" : "";  /* ● */

    /* Junk score */
    char junkbuf[8] = "";
    if (sum->spam_score > 0)
      snprintf(junkbuf, sizeof(junkbuf), "%ld", (long)sum->spam_score);

    /* Ensure UTF-8 for display strings */
    gchar *safe_who = ensure_utf8(sum->from);
    gchar *safe_subject = ensure_utf8(sum->subject);

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       COL_STATUS,   state_str(sum->state),
                       COL_PRIORITY, priority_str(sum->priority),
                       COL_ATTACH,   attach,
                       COL_LABEL,    labelbuf,
                       COL_WHO,      safe_who,
                       COL_DATE,     datebuf,
                       COL_SIZE,     sizebuf,
                       COL_JUNK,     junkbuf,
                       COL_SUBJECT,  safe_subject,
                       COL_INDEX,    i,
                       COL_LABEL_COLOR, label > 0 ? label_color_str(label) : NULL,
                       -1);
    g_free(safe_who);
    g_free(safe_subject);
  }
}

/**********************************************************************
 * InitMailboxWin - initialize mailbox window with GTK message list
 * Replaces Mac custom QuickDraw drawing with GtkTreeView + preview.
 **********************************************************************/
/* Cell data func: apply label foreground color, but let theme handle selected rows */
static void label_color_cell_func(GtkTreeViewColumn *col, GtkCellRenderer *cell,
                                   GtkTreeModel *model, GtkTreeIter *iter,
                                   gpointer data) {
  (void)col; (void)data;
  gchar *color = NULL;
  gtk_tree_model_get(model, iter, COL_LABEL_COLOR, &color, -1);
  if (color && color[0]) {
    g_object_set(cell, "foreground", color, "foreground-set", TRUE, NULL);
  } else {
    g_object_set(cell, "foreground-set", FALSE, NULL);
  }
  g_free(color);
}

void InitMailboxWin(MyWindowPtr win, MacmbxTOC * toc, bool showIt) {
  GtkWidget *winWP = GetMyWindowWindowPtr(win);
  if (!winWP) return;

  SetMyWindowPrivateData(win, (void *)toc);
  win->close = BoxClose;
  win->button = BoxButton;
  win->menu = BoxMenu;
  win->gonnaShow = BoxGonnaShow;
  win->position = BoxPosition;
  win->cursor = BoxCursor;
  win->find = BoxFind;
  toc->win = win;

  /* Set window title from mailbox name */
  const char *title = spec_name(toc->mbox_path);
  if (title && *title)
    theme_setup_headerbar(winWP, title);

  /* Build the mailbox window content:
     ┌────────────────────────────┐
     │ Status | From | Subject | Date | Size  │  <- GtkTreeView
     │────────────────────────────│
     │ Message preview (GtkTextView)          │
     └────────────────────────────┘  */

  GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(vpaned), 250);

  /* --- Message list (GtkTreeView) --- */
  /* Columns match original Eudora: Status, Priority, Attach, Label,
     Who (From/To), Date, Size, Junk, Subject, Index(hidden) */
  GtkListStore *store = gtk_list_store_new(NUM_MBOX_COLS,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  bool isOut = (toc && toc->which == OUT);

  /* Column definitions: title, model column, width (-1 = expand), visible */
  struct { const char *title; int col; int width; bool visible; } cols[] = {
    {"",         COL_STATUS,   28,  true},
    {"!",        COL_PRIORITY, 28,  true},
    {"\xf0\x9f\x93\x8e", COL_ATTACH, 28, true},  /* 📎 */
    {"Label",    COL_LABEL,    40,  true},
    {isOut ? "To" : "From", COL_WHO, 150, true},
    {"Date",     COL_DATE,    130,  true},
    {"Size",     COL_SIZE,     60,  true},
    {"Junk",     COL_JUNK,     40,  false}, /* hidden by default */
    {"Subject",  COL_SUBJECT,  -1,  true},
  };
  int ncols = sizeof(cols) / sizeof(cols[0]);
  for (int c = 0; c < ncols; c++) {
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(
        cols[c].title, r, "text", cols[c].col, NULL);
    /* Apply label color only to non-selected rows */
    gtk_tree_view_column_set_cell_data_func(col, r, label_color_cell_func,
                                             GINT_TO_POINTER(cols[c].col), NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_reorderable(col, TRUE);
    if (cols[c].width > 0)
      gtk_tree_view_column_set_fixed_width(col, cols[c].width);
    else
      gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_column_set_visible(col, cols[c].visible);
    gtk_tree_view_column_set_sort_column_id(col, cols[c].col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
  }
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);

  /* Populate from TOC */
  populate_mbox_list(store, toc);

  GtkWidget *scroll1 = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll1), tree);
  gtk_widget_set_vexpand(scroll1, TRUE);
  wire_msg_drag_to_scroll(scroll1, tree);
  gtk_paned_set_start_child(GTK_PANED(vpaned), scroll1);
  gtk_paned_set_resize_start_child(GTK_PANED(vpaned), TRUE);

  /* --- Message preview: header grid + body text --- */
  GtkWidget *preview_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *preview_hdr = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(preview_hdr, "msg-header-box");
  gtk_box_append(GTK_BOX(preview_box), preview_hdr);

  GtkWidget *preview_body = geditctrl_new();
  geditctrl_set_editable(preview_body, FALSE);
  gtk_widget_set_vexpand(preview_body, TRUE);
  theme_apply_to_editor(preview_body);
  gtk_box_append(GTK_BOX(preview_box), preview_body);

  g_object_set_data(G_OBJECT(winWP), "preview-hdr", preview_hdr);
  g_object_set_data(G_OBJECT(winWP), "preview-body", preview_body);
  g_object_set_data(G_OBJECT(winWP), "preview", preview_box);
  g_object_set_data(G_OBJECT(winWP), "mbox-tree", tree);

  GtkWidget *scroll2 = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll2), preview_box);
  gtk_widget_set_vexpand(scroll2, TRUE);
  gtk_paned_set_end_child(GTK_PANED(vpaned), scroll2);
  gtk_paned_set_resize_end_child(GTK_PANED(vpaned), TRUE);

  /* Connect selection change to preview */
  MboxSelCtx *ctx = g_new0(MboxSelCtx, 1);
  ctx->toc = toc;
  ctx->preview = preview_box;
  ctx->preview_hdr = preview_hdr;
  ctx->preview_body = preview_body;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  g_signal_connect_data(sel, "changed", G_CALLBACK(on_mbox_msg_selected),
                        ctx, (GClosureNotify)g_free, 0);

  /* Double-click / Enter opens message */
  g_signal_connect(tree, "row-activated", G_CALLBACK(on_mbox_row_activated), toc);

  /* Right-click context menu */
  attach_mbox_context_menu(tree, toc);

  /* Drag source: drag messages from list to sidebar mailboxes */
  attach_msg_drag_source(tree, toc);

  /* Auto-select first message */
  if (toc->count > 0) {
    GtkTreePath *first = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(sel, first);
    gtk_tree_path_free(first);
  }

  /* Replace the empty scrolled-window content with our paned layout */
  gtk_window_set_child(GTK_WINDOW(winWP), vpaned);
}

/* ── Message viewer toolbar helpers ── */

/* Read raw message headers via macmbx. Caller must g_free the result.
 * Sets *hdr_len to byte length of headers (up to body start). */
char *read_raw_headers(MacmbxTOC *tocH, int sumNum, long *hdr_len) {
  long msg_len = 0;
  char *raw = macmbx_read_message(tocH, sumNum, &msg_len);
  if (!raw) { *hdr_len = 0; return NULL; }

  MacmbxMsgSum *sum = &tocH->msgs[sumNum];
  long body_off = sum->body_offset;
  if (body_off <= 0 || body_off > msg_len)
    body_off = find_body_start(raw, msg_len);
  *hdr_len = (body_off > 0) ? body_off : msg_len;

  /* Transfer to g_malloc'd memory */
  gchar *gbuf = g_strdup(raw);
  free(raw);
  return gbuf;
}

/* Build a normal (weeded) header grid from raw message text.
 * Uses extract_header() — same path as preview, single code path. */
static GtkWidget *build_header_grid(const char *raw, long hdr_len, MacmbxMsgSum * sum) {
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
  gtk_widget_set_margin_start(grid, 12);
  gtk_widget_set_margin_end(grid, 12);
  gtk_widget_set_margin_top(grid, 8);
  gtk_widget_set_margin_bottom(grid, 8);
  int row = 0;

  struct { const char *label; const char *name; bool isSubject; } fields[] = {
    {"Subject:", "Subject", true}, {"From:", "From", false},
    {"To:", "To", false}, {"Cc:", "Cc", false}, {"Date:", "Date", false},
  };
  for (int i = 0; i < 5; i++) {
    gchar *vstr = extract_header(raw, hdr_len, fields[i].name);
    /* Fallback to TOC summary for Subject/From if not found in raw */
    if (!vstr || !vstr[0]) {
      g_free(vstr);
      if (fields[i].isSubject & sum->subject[0])
        vstr = ensure_utf8(sum->subject);
      else if (i == 1 & sum->from[0])  /* From */
        vstr = ensure_utf8(sum->from);
      else
        continue;
    }
    if (!vstr || !vstr[0]) { g_free(vstr); continue; }

    GtkWidget *lbl = gtk_label_new(fields[i].label);
    gtk_widget_add_css_class(lbl, "msg-hdr-label");
    gtk_widget_set_halign(lbl, GTK_ALIGN_END);
    gtk_widget_set_valign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);

    GtkWidget *vlbl = gtk_label_new(vstr);
    gtk_label_set_wrap(GTK_LABEL(vlbl), TRUE);
    gtk_label_set_selectable(GTK_LABEL(vlbl), TRUE);
    gtk_widget_set_halign(vlbl, GTK_ALIGN_START);
    gtk_widget_set_hexpand(vlbl, TRUE);
    if (fields[i].isSubject)
      gtk_widget_add_css_class(vlbl, "msg-subject-value");
    gtk_grid_attach(GTK_GRID(grid), vlbl, 1, row, 1, 1);
    g_free(vstr);
    row++;
  }
  return grid;
}

/* Build the "all headers" view — a formatted text view with bold header names */
static GtkWidget *build_all_headers_view(const char *raw, long hdr_len) {
  GtkWidget *tv = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 12);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 12);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(tv), 4);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(tv), 4);

  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
  GtkTextTag *bold_tag = gtk_text_buffer_create_tag(buf, NULL,
      "weight", PANGO_WEIGHT_BOLD, NULL);

  /* Parse each header line; bold the "Name:" part */
  gchar *utf8_hdrs = ensure_utf8(g_strndup(raw, hdr_len));
  /* Normalize line endings for display */
  GString *norm = g_string_sized_new(hdr_len);
  for (const char *p = utf8_hdrs; *p; p++) {
    if (*p == '\r') {
      g_string_append_c(norm, '\n');
      if (p[1] == '\n') p++;
    } else {
      g_string_append_c(norm, *p);
    }
  }
  g_free(utf8_hdrs);

  /* Insert line by line, bolding header names */
  const char *text = norm->str;
  GtkTextIter iter;
  gtk_text_buffer_get_start_iter(buf, &iter);
  while (*text) {
    const char *eol = strchr(text, '\n');
    if (!eol) eol = text + strlen(text);
    const char *colon = memchr(text, ':', eol - text);
    if (colon && colon > text && !IsWhite(*text)) {
      /* Bold the "Header:" part */
      gtk_text_buffer_insert_with_tags(buf, &iter, text, colon - text + 1,
                                        bold_tag, NULL);
      /* Rest of line as normal text */
      if (colon + 1 < eol)
        gtk_text_buffer_insert(buf, &iter, colon + 1, eol - (colon + 1));
    } else {
      /* Continuation line or other — insert as-is */
      gtk_text_buffer_insert(buf, &iter, text, eol - text);
    }
    if (*eol == '\n') {
      gtk_text_buffer_insert(buf, &iter, "\n", 1);
      text = eol + 1;
    } else {
      text = eol;
    }
  }
  g_string_free(norm, TRUE);

  GtkWidget *sw = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(sw), 250);
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(sw), TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), tv);
  return sw;
}

/* ── Message viewer toolbar callbacks ── */

/* Blah-blah-blah: toggle between weeded and all headers */
static void on_blahblah_toggled(GtkToggleButton *btn, gpointer ud) {
  (void)ud;
  MessHandle messH = g_object_get_data(G_OBJECT(btn), "messH");
  if (!messH || !messH->tocH) return;

  bool showAll = gtk_toggle_button_get_active(btn);
  MacmbxTOC *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  MacmbxMsgSum * sum = &tocH->msgs[sumNum];

  if (showAll)
    SetMessFlag(messH, FLAG_SHOW_ALL);
  else
    ClearMessFlag(messH, FLAG_SHOW_ALL);

  long hdr_len = 0;
  char *raw = read_raw_headers(tocH, sumNum, &hdr_len);
  if (!raw) return;

  GtkWidget *hdr_box = g_object_get_data(G_OBJECT(btn), "hdr-box");
  if (hdr_box) {
    /* Remove old content */
    GtkWidget *child;
    while ((child = gtk_widget_get_first_child(hdr_box)))
      gtk_box_remove(GTK_BOX(hdr_box), child);

    if (showAll) {
      gtk_box_append(GTK_BOX(hdr_box), build_all_headers_view(raw, hdr_len));
    } else {
      gtk_box_append(GTK_BOX(hdr_box), build_header_grid(raw, hdr_len, sum));
    }
  }
  g_free(raw);
}

/* Reply to the message shown in this window */
static void on_msg_reply(GtkButton *btn, gpointer ud) {
  (void)ud;
  MessHandle messH = g_object_get_data(G_OBJECT(btn), "messH");
  if (!messH || !messH->win) return;
  extern MyWindowPtr DoReplyMessage(MyWindowPtr win, bool all, bool self,
    bool quote, bool doFcc, short withWhich, bool vis, bool station, bool caching);
  MyWindowPtr replyWin = DoReplyMessage(messH->win, false, false, true,
                                         true, 0, true, false, false);
  if (replyWin && replyWin->window)
    gtk_window_present(GTK_WINDOW(replyWin->window));
}

/* Reply All */
static void on_msg_reply_all(GtkButton *btn, gpointer ud) {
  (void)ud;
  MessHandle messH = g_object_get_data(G_OBJECT(btn), "messH");
  if (!messH || !messH->win) return;
  extern MyWindowPtr DoReplyMessage(MyWindowPtr win, bool all, bool self,
    bool quote, bool doFcc, short withWhich, bool vis, bool station, bool caching);
  MyWindowPtr replyWin = DoReplyMessage(messH->win, true, false, true,
                                         true, 0, true, false, false);
  if (replyWin && replyWin->window)
    gtk_window_present(GTK_WINDOW(replyWin->window));
}

/* Forward */
static void on_msg_forward(GtkButton *btn, gpointer ud) {
  (void)ud;
  MessHandle messH = g_object_get_data(G_OBJECT(btn), "messH");
  if (!messH || !messH->win) return;
  extern MyWindowPtr DoForwardMessage(MyWindowPtr win, void *toWhom, bool turbo);
  MyWindowPtr fwdWin = DoForwardMessage(messH->win, NULL, false);
  if (fwdWin && fwdWin->window)
    gtk_window_present(GTK_WINDOW(fwdWin->window));
}

/* Delete / move to trash */
static void on_msg_delete(GtkButton *btn, gpointer ud) {
  (void)ud;
  MessHandle messH = g_object_get_data(G_OBJECT(btn), "messH");
  if (!messH || !messH->tocH) return;
  extern void DeleteMessage(MacmbxTOC *tocH, int sumNum, bool nuke);
  MacmbxTOC *tocH = messH->tocH;
  int sumNum = messH->sumNum;
  GtkWidget *win = messH->win ? messH->win->window : NULL;
  DeleteMessage(tocH, sumNum, false);
  /* Close the message window */
  if (win)
    gtk_window_destroy(GTK_WINDOW(win));
}

static void on_fixed_toggled(GtkToggleButton *btn, gpointer ud) {
  (void)ud;
  GtkWidget *v = g_object_get_data(G_OBJECT(btn), "body-view");
  if (!v) return;
  if (gtk_toggle_button_get_active(btn))
    gtk_widget_add_css_class(v, "monospace");
  else
    gtk_widget_remove_css_class(v, "monospace");
}

/* Double-click / Enter on tree row → open message window */
static void on_msg_print(GtkButton *btn, gpointer ud) {
  (void)ud;
  MyWindowPtr mwin = g_object_get_data(G_OBJECT(btn), "mywindow");
  if (mwin)
    PrintOneMessage(mwin, false, false);
}

static void on_mbox_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                  GtkTreeViewColumn *column, gpointer data) {
  (void)column;
  MacmbxTOC *toc = (MacmbxTOC *)data;
  if (!toc) return;

  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path)) return;

  int idx = -1;
  gtk_tree_model_get(model, &iter, COL_INDEX, &idx, -1);
  if (idx < 0 || idx >= toc->count) return;

  MacmbxMsgSum * sum = &toc->msgs[idx];

  /* Outgoing/draft messages open in compose window */
  if (toc->which == OUT ||
      sum->state == UNSENDABLE || sum->state == SENDABLE ||
      sum->state == QUEUED || sum->state == UNSENT ||
      sum->state == TIMED) {
    MyWindowPtr win = OpenComp(toc, idx, NULL, NULL, true, false);
    if (win && win->window)
      gtk_window_present(GTK_WINDOW(win->window));
    return;
  }

  /* Incoming messages: use OpenMessage which handles header weeding,
   * rich text, IMAP download, and all the original Eudora logic */
  extern MyWindowPtr OpenMessage(MacmbxTOC *tocH, short sumNum, GtkWidget *winWP,
                                  MyWindowPtr win, bool showIt, bool preview);
  MyWindowPtr mwin = OpenMessage(toc, idx, NULL, NULL, false, false);
  if (!mwin || !mwin->pte) return;

  GtkWidget *winWP = (GtkWidget *)mwin->window;
  gtk_window_set_default_size(GTK_WINDOW(winWP), 750, 600);

  /* ── CSS: themed header background + toolbar ── */
  {
    static gboolean msgview_css_loaded = FALSE;
    if (!msgview_css_loaded) {
      GtkCssProvider *css = gtk_css_provider_new();
      const char *css_data =
        ".msg-header-box {"
        "  background-color: alpha(currentColor, 0.06);"
        "  border-bottom: 1px solid alpha(currentColor, 0.15);"
        "}";
      gtk_css_provider_load_from_data(css, css_data, -1);
      gtk_style_context_add_provider_for_display(
          gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
          GTK_STYLE_PROVIDER_PRIORITY_USER);
      g_object_unref(css);
      msgview_css_loaded = TRUE;
    }
  }

  /* ── Toolbar: matches original Eudora message icon bar ── */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_add_css_class(toolbar, "msg-toolbar");
  gtk_widget_set_margin_start(toolbar, 6);
  gtk_widget_set_margin_end(toolbar, 6);
  gtk_widget_set_margin_top(toolbar, 3);
  gtk_widget_set_margin_bottom(toolbar, 3);

  GtkWidget *trash_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
  gtk_widget_set_tooltip_text(trash_btn, "Delete message");
  gtk_box_append(GTK_BOX(toolbar), trash_btn);

  GtkWidget *reply_btn = gtk_button_new_from_icon_name("mail-reply-sender-symbolic");
  gtk_widget_set_tooltip_text(reply_btn, "Reply");
  gtk_box_append(GTK_BOX(toolbar), reply_btn);

  GtkWidget *reply_all_btn = gtk_button_new_from_icon_name("mail-reply-all-symbolic");
  gtk_widget_set_tooltip_text(reply_all_btn, "Reply All");
  gtk_box_append(GTK_BOX(toolbar), reply_all_btn);

  GtkWidget *fwd_btn = gtk_button_new_from_icon_name("mail-forward-symbolic");
  gtk_widget_set_tooltip_text(fwd_btn, "Forward");
  gtk_box_append(GTK_BOX(toolbar), fwd_btn);

  gtk_box_append(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  GtkWidget *blah_btn = gtk_toggle_button_new_with_label("All Headers");
  gtk_widget_set_tooltip_text(blah_btn, "Show all message headers");
  gtk_box_append(GTK_BOX(toolbar), blah_btn);

  GtkWidget *fixed_btn = gtk_toggle_button_new_with_label("Fixed");
  gtk_widget_set_tooltip_text(fixed_btn, "Use fixed-width font");
  gtk_box_append(GTK_BOX(toolbar), fixed_btn);

  GtkWidget *print_btn = gtk_button_new_from_icon_name("printer-symbolic");
  gtk_widget_set_tooltip_text(print_btn, "Print message");
  gtk_box_append(GTK_BOX(toolbar), print_btn);

  /* ── Header area from TOC summary (always reliable) ── */
  GtkWidget *hdr_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(hdr_box, "msg-header-box");

  /* Build header grid from raw message — same source for initial view,
   * blah-blah toggle off, and preview. Uses shared build_header_grid(). */
  MessHandle messH = Win2MessH(mwin);
  long hdr_len = 0;
  char *raw = read_raw_headers(toc, idx, &hdr_len);
  GtkWidget *hdr_grid = build_header_grid(raw ? raw : "", hdr_len, sum);
  g_free(raw);
  gtk_box_append(GTK_BOX(hdr_box), hdr_grid);

  /* Strip headers from editor — the editor text starts with kept headers
   * (after WeedHeaders). Find the blank line separating headers from body
   * in the editor text (which has normalized \n line endings). */
  {
    gchar *edtext = gedit_document_get_text(geditctrl_get_document(mwin->pte));
    if (edtext) {
      const char *blank = strstr(edtext, "\n\n");
      if (blank) {
        long byte_off = (blank - edtext) + 2; /* past the blank line */
        gint char_off = (gint)g_utf8_strlen(edtext, byte_off);
        gedit_document_delete_range(geditctrl_get_document(mwin->pte), 0, char_off);
      }
      g_free(edtext);
    }
  }

  /* ── Body: the gEditCtrl from OpenMessage (already a scrolled window) ── */
  gtk_widget_set_vexpand(mwin->pte, TRUE);

  /* Make body read-only and apply theme */
  geditctrl_set_editable(mwin->pte, false);
  theme_apply_to_editor(mwin->pte);

  /* ── Connect toolbar button handlers ── */
  g_object_set_data(G_OBJECT(trash_btn), "messH", messH);
  g_signal_connect(trash_btn, "clicked", G_CALLBACK(on_msg_delete), NULL);

  g_object_set_data(G_OBJECT(reply_btn), "messH", messH);
  g_signal_connect(reply_btn, "clicked", G_CALLBACK(on_msg_reply), NULL);

  g_object_set_data(G_OBJECT(reply_all_btn), "messH", messH);
  g_signal_connect(reply_all_btn, "clicked", G_CALLBACK(on_msg_reply_all), NULL);

  g_object_set_data(G_OBJECT(fwd_btn), "messH", messH);
  g_signal_connect(fwd_btn, "clicked", G_CALLBACK(on_msg_forward), NULL);

  g_object_set_data(G_OBJECT(fixed_btn), "body-view", mwin->pte);
  g_signal_connect(fixed_btn, "toggled", G_CALLBACK(on_fixed_toggled), NULL);

  g_object_set_data(G_OBJECT(blah_btn), "messH", messH);
  g_object_set_data(G_OBJECT(blah_btn), "hdr-box", hdr_box);
  g_signal_connect(blah_btn, "toggled", G_CALLBACK(on_blahblah_toggled), NULL);

  g_object_set_data(G_OBJECT(print_btn), "mywindow", mwin);
  g_signal_connect(print_btn, "clicked", G_CALLBACK(on_msg_print), NULL);

  /* Store MyWindowPtr on the window for action_print */
  SetWindowMyWindowPtr(winWP, mwin);

  /* ── Layout: toolbar + header + body ── */
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append(GTK_BOX(vbox), toolbar);
  gtk_box_append(GTK_BOX(vbox), hdr_box);
  gtk_box_append(GTK_BOX(vbox), mwin->pte);
  gtk_window_set_child(GTK_WINDOW(winWP), vbox);

  /* Make transient to the toplevel */
  GtkWidget *toplevel = gtk_widget_get_ancestor(GTK_WIDGET(tree_view),
                                                 GTK_TYPE_WINDOW);
  if (toplevel)
    gtk_window_set_transient_for(GTK_WINDOW(winWP), GTK_WINDOW(toplevel));

  gtk_window_present(GTK_WINDOW(winWP));
}

/**********************************************************************
 * CreateMailboxPanel - build mailbox content as an embeddable widget.
 * Same layout as InitMailboxWin but returns the vpaned instead of
 * attaching to a window.  Used by main_eudora.c for notebook tabs.
 **********************************************************************/
/* ── Context menu helpers ── */

/* Get selected message index from tree view, -1 if none */
static int mbox_tree_selected_index(GtkTreeView *tree) {
  GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
  GtkTreeModel *model;
  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return -1;
  int idx = -1;
  gtk_tree_model_get(model, &iter, COL_INDEX, &idx, -1);
  return idx;
}

/* Read raw message from mailbox via macmbx */
static gchar *mbox_read_raw(MacmbxTOC *toc, int idx) {
  if (!toc || idx < 0 || idx >= toc->count) return NULL;
  long len = 0;
  char *buf = macmbx_read_message(toc, idx, &len);
  if (!buf) return NULL;
  if (!g_utf8_validate(buf, len, NULL)) {
    gchar *utf8 = ensure_utf8(buf);
    free(buf);
    return utf8;
  }
  /* Transfer to g_malloc */
  gchar *gbuf = g_strdup(buf);
  free(buf);
  return buf;
}

/* Find body start in raw message (after blank line) */
static const char *mbox_find_body(const char *raw) {
  if (!raw) return "";
  const char *p = raw;
  while (*p) {
    if (*p == '\n') {
      if (p[1] == '\n') return p + 2;
      if (p[1] == '\r' & p[2] == '\n') return p + 3;
    }
    if (*p == '\r' & p[1] == '\n' & p[2] == '\r' & p[3] == '\n')
      return p + 4;
    p++;
  }
  return p;
}

/* Quote text for reply */
static gchar *mbox_quote_text(const char *text) {
  if (!text || !*text) return g_strdup("");
  GString *q = g_string_new(NULL);
  const char *p = text;
  while (*p) {
    g_string_append(q, "> ");
    const char *eol = strchr(p, '\n');
    if (eol) {
      g_string_append_len(q, p, eol - p + 1);
      p = eol + 1;
    } else {
      g_string_append(q, p);
      g_string_append_c(q, '\n');
      break;
    }
  }
  return g_string_free(q, FALSE);
}

/* Context menu action callbacks — user_data is the GtkTreeView */
static void on_ctx_reply(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)a; (void)p;
  GtkTreeView *tree = GTK_TREE_VIEW(ud);
  MacmbxTOC *toc = g_object_get_data(G_OBJECT(tree), "toc");
  int idx = mbox_tree_selected_index(tree);
  if (idx < 0 || !toc) return;

  MacmbxMsgSum * sum = &toc->msgs[idx];
  gchar *raw = mbox_read_raw(toc, idx);
  const char *body = mbox_find_body(raw);

  MyWindowPtr win = DoComposeNew(0);
  if (!win || !win->window) { g_free(raw); return; }

  comp_set_field(win, "comp-to", sum->from);
  const char *subj = sum->subject;
  gchar *re_subj = (subj && g_ascii_strncasecmp(subj, "Re:", 3) == 0)
      ? g_strdup(subj) : g_strdup_printf("Re: %s", subj ? subj : "");
  comp_set_field(win, "comp-subject", re_subj);

  /* Insert body with quote bars (not "> " prefix) */
  gchar *attribution = g_strdup_printf("On %s wrote:\n", sum->from);
  comp_set_body_quoted(win, attribution, body);
  g_free(attribution);

  gtk_window_present(GTK_WINDOW(win->window));
  g_free(raw); g_free(re_subj);
}

static void on_ctx_forward(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)a; (void)p;
  GtkTreeView *tree = GTK_TREE_VIEW(ud);
  MacmbxTOC *toc = g_object_get_data(G_OBJECT(tree), "toc");
  int idx = mbox_tree_selected_index(tree);
  if (idx < 0 || !toc) return;

  MacmbxMsgSum * sum = &toc->msgs[idx];
  gchar *raw = mbox_read_raw(toc, idx);
  const char *body = mbox_find_body(raw);

  MyWindowPtr win = DoComposeNew(0);
  if (!win || !win->window) { g_free(raw); return; }

  const char *subj = sum->subject;
  gchar *fwd_subj = g_strdup_printf("Fwd: %s", subj ? subj : "");
  comp_set_field(win, "comp-subject", fwd_subj);

  if (body && *body) {
    gchar *fb = g_strdup_printf(
        "---------- Forwarded message ----------\n"
        "From: %s\nSubject: %s\n\n%s",
        sum->from, subj ? subj : "", body);
    comp_set_body(win, fb);
    g_free(fb);
  }
  gtk_window_present(GTK_WINDOW(win->window));
  g_free(raw); g_free(fwd_subj);
}

/* Forward declarations for macmbx helpers used by context menu */
static bool ctx_get_sel(gpointer ud, GtkTreeView **tree, MacmbxTOC **mtoc, int *idx);
static const char *macmbx_state_str(uint8_t s);

static void on_ctx_delete(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)a; (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;

  macmbx_delete_message(mtoc, idx);
  macmbx_toc_save(mtoc);

  /* Remove from visible list */
  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      int row_idx = -1;
      gtk_tree_model_get(model, &iter, COL_INDEX, &row_idx, -1);
      if (row_idx == idx) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        break;
      }
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/* Helper: update a tree row's column for the selected message */
static void mbox_update_row(GtkTreeView *tree, int idx, int col, const char *val) {
  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter_first(model, &iter)) return;
  do {
    int row_idx = -1;
    gtk_tree_model_get(model, &iter, COL_INDEX, &row_idx, -1);
    if (row_idx == idx) {
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, col, val, -1);
      break;
    }
  } while (gtk_tree_model_iter_next(model, &iter));
}

/* Helper: get MacmbxTOC + selected index from tree */
static bool ctx_get_sel(gpointer ud, GtkTreeView **tree, MacmbxTOC **mtoc, int *idx) {
  *tree = GTK_TREE_VIEW(ud);
  /* Walk up to the vpaned parent which has the macmbx-toc data */
  GtkWidget *vpaned = gtk_widget_get_ancestor(GTK_WIDGET(*tree), GTK_TYPE_PANED);
  *mtoc = vpaned ? g_object_get_data(G_OBJECT(vpaned), "macmbx-toc") : NULL;
  if (!*mtoc) {
    /* Fallback: open via path stored on tree */
    const char *path = g_object_get_data(G_OBJECT(*tree), "mbox-path");
    if (path) *mtoc = macmbx_toc_open(path);
  }
  *idx = mbox_tree_selected_index(*tree);
  return (*idx >= 0 && *mtoc && *idx < (*mtoc)->count);
}

/* --- Change Status actions --- */

static void on_ctx_set_state(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;

  const char *name = g_action_get_name(G_ACTION(a));
  uint8_t state = MACMBX_READ;
  if (strcmp(name, "mark-unread") == 0)      state = MACMBX_UNREAD;
  else if (strcmp(name, "mark-read") == 0)   state = MACMBX_READ;
  else if (strcmp(name, "mark-replied") == 0) state = MACMBX_REPLIED;
  else if (strcmp(name, "mark-forwarded") == 0) state = MACMBX_FORWARDED;

  macmbx_set_state(mtoc, idx, state);
  macmbx_toc_save(mtoc);
  mbox_update_row(tree, idx, COL_STATUS, macmbx_state_str(state));
}

/* on_ctx_mark_read/unread now handled by on_ctx_set_state */

/* --- Change Priority actions --- */

static void on_ctx_set_priority(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;

  const char *name = g_action_get_name(G_ACTION(a));
  uint8_t pri = 3;
  if (strcmp(name, "priority-highest") == 0) pri = 1;
  else if (strcmp(name, "priority-high") == 0) pri = 2;
  else if (strcmp(name, "priority-normal") == 0) pri = 3;
  else if (strcmp(name, "priority-low") == 0) pri = 4;
  else if (strcmp(name, "priority-lowest") == 0) pri = 5;

  macmbx_set_priority(mtoc, idx, pri);
  macmbx_toc_save(mtoc);
  mbox_update_row(tree, idx, COL_PRIORITY, priority_str(pri));
}

/* --- Transfer To actions --- */

static void on_ctx_transfer(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;

  const char *mbName = g_object_get_data(G_OBJECT(a), "mailbox-name");
  if (!mbName) return;

  /* Open destination via macmbx store */
  MacmbxStore *store = gtk_mailbox_get_store();
  if (!store) return;
  MacmbxNode *dst_node = macmbx_store_find_by_name(store, mbName);
  if (!dst_node || dst_node->type != MACMBX_NODE_MAILBOX) return;
  MacmbxTOC *dst = macmbx_toc_open(dst_node->path);
  if (!dst) return;

  macmbx_transfer(mtoc, idx, dst, false /* move, not copy */);
  macmbx_toc_save(mtoc);
  macmbx_toc_save(dst);

  /* Remove from visible list */
  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      int row_idx = -1;
      gtk_tree_model_get(model, &iter, COL_INDEX, &row_idx, -1);
      if (row_idx == idx) {
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        break;
      }
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/* --- Change Label actions --- */

static void on_ctx_set_label(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;

  const char *name = g_action_get_name(G_ACTION(a));
  int label = 0;
  if (sscanf(name, "label-%d", &label) != 1) label = 0;

  macmbx_set_label(mtoc, idx, (uint8_t)label);
  macmbx_toc_save(mtoc);

  const char *labelbuf = (label > 0) ? "\xe2\x97\x8f" : "";
  mbox_update_row(tree, idx, COL_LABEL, labelbuf);

  /* Update row color */
  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_first(model, &iter)) {
    do {
      int row_idx = -1;
      gtk_tree_model_get(model, &iter, COL_INDEX, &row_idx, -1);
      if (row_idx == idx) {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COL_LABEL_COLOR, label > 0 ? label_color_str(label) : NULL, -1);
        break;
      }
    } while (gtk_tree_model_iter_next(model, &iter));
  }
}

/* --- Junk actions --- */

static void on_ctx_junk(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)a; (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;
  MacmbxJunkConfig jcfg;
  macmbx_junk_config_init(&jcfg);
  macmbx_junk_mark(&jcfg, mtoc, idx, true, gtk_mailbox_get_store());
  macmbx_toc_save(mtoc);
  mbox_update_row(tree, idx, COL_JUNK, "100");
}

static void on_ctx_not_junk(GSimpleAction *a, GVariant *p, gpointer ud) {
  (void)a; (void)p;
  GtkTreeView *tree; MacmbxTOC *mtoc; int idx;
  if (!ctx_get_sel(ud, &tree, &mtoc, &idx)) return;
  MacmbxJunkConfig jcfg;
  macmbx_junk_config_init(&jcfg);
  macmbx_junk_mark(&jcfg, mtoc, idx, false, gtk_mailbox_get_store());
  macmbx_toc_save(mtoc);
  mbox_update_row(tree, idx, COL_JUNK, "");
}

/* Right-click handler — show context popover */
static void on_mbox_right_click(GtkGestureClick *gesture, int n_press,
                                double x, double y, gpointer ud) {
  (void)gesture; (void)n_press;
  GtkWidget *tree = GTK_WIDGET(ud);
  GtkWidget *popover = g_object_get_data(G_OBJECT(tree), "ctx-popover");
  if (!popover) return;
  GdkRectangle rect = { (int)x, (int)y, 1, 1 };
  gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);
  gtk_popover_popup(GTK_POPOVER(popover));
}

/* Build and attach a right-click context menu to the tree view */
static void attach_mbox_context_menu(GtkWidget *tree, MacmbxTOC *toc) {
  g_object_set_data(G_OBJECT(tree), "toc", toc);

  /* Action group for context menu */
  GSimpleActionGroup *grp = g_simple_action_group_new();
  const GActionEntry entries[] = {
    { "reply",            on_ctx_reply,        NULL, NULL, NULL },
    { "reply-all",        on_ctx_reply,        NULL, NULL, NULL },
    { "forward",          on_ctx_forward,      NULL, NULL, NULL },
    { "redirect",         on_ctx_forward,      NULL, NULL, NULL },
    { "delete",           on_ctx_delete,       NULL, NULL, NULL },
    { "mark-read",        on_ctx_set_state,    NULL, NULL, NULL },
    { "mark-unread",      on_ctx_set_state,    NULL, NULL, NULL },
    { "mark-replied",     on_ctx_set_state,    NULL, NULL, NULL },
    { "mark-forwarded",   on_ctx_set_state,    NULL, NULL, NULL },
    { "junk",             on_ctx_junk,         NULL, NULL, NULL },
    { "not-junk",         on_ctx_not_junk,     NULL, NULL, NULL },
    { "priority-highest", on_ctx_set_priority, NULL, NULL, NULL },
    { "priority-high",    on_ctx_set_priority, NULL, NULL, NULL },
    { "priority-normal",  on_ctx_set_priority, NULL, NULL, NULL },
    { "priority-low",     on_ctx_set_priority, NULL, NULL, NULL },
    { "priority-lowest",  on_ctx_set_priority, NULL, NULL, NULL },
    { "label-0",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-1",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-2",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-3",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-4",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-5",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-6",          on_ctx_set_label,    NULL, NULL, NULL },
    { "label-7",          on_ctx_set_label,    NULL, NULL, NULL },
  };
  g_action_map_add_action_entries(G_ACTION_MAP(grp), entries,
                                  G_N_ELEMENTS(entries), tree);
  gtk_widget_insert_action_group(tree, "msg", G_ACTION_GROUP(grp));

  /* Menu model */
  GMenu *menu = g_menu_new();

  /* Reply / Forward section */
  GMenu *section1 = g_menu_new();
  g_menu_append(section1, "Reply",        "msg.reply");
  g_menu_append(section1, "Reply to All", "msg.reply-all");
  g_menu_append(section1, "Forward",      "msg.forward");
  g_menu_append(section1, "Redirect",     "msg.redirect");
  g_menu_append_section(menu, NULL, G_MENU_MODEL(section1));

  /* Change Status submenu */
  GMenu *status_sub = g_menu_new();
  g_menu_append(status_sub, "Unread",    "msg.mark-unread");
  g_menu_append(status_sub, "Read",      "msg.mark-read");
  g_menu_append(status_sub, "Replied",   "msg.mark-replied");
  g_menu_append(status_sub, "Forwarded", "msg.mark-forwarded");

  /* Change Priority submenu */
  GMenu *priority_sub = g_menu_new();
  g_menu_append(priority_sub, "Highest", "msg.priority-highest");
  g_menu_append(priority_sub, "High",    "msg.priority-high");
  g_menu_append(priority_sub, "Normal",  "msg.priority-normal");
  g_menu_append(priority_sub, "Low",     "msg.priority-low");
  g_menu_append(priority_sub, "Lowest",  "msg.priority-lowest");

  /* Change Label submenu — read names from settings */
  GMenu *label_sub = g_menu_new();
  g_menu_append(label_sub, "None", "msg.label-0");
  {
    static const char *default_names[] = {
      "Label 1","Label 2","Label 3","Label 4",
      "Label 5","Label 6","Label 7"
    };
    for (int li = 0; li < 7; li++) {
      char key[32], action[32];
      snprintf(key, sizeof(key), "name_%d", li);
      gchar *lname = prefs_get_string(PREFS_GROUP_LABELS, key,
                                       default_names[li]);
      snprintf(action, sizeof(action), "msg.label-%d", li + 1);
      g_menu_append(label_sub, lname, action);
      g_free(lname);
    }
  }

  GMenu *section2 = g_menu_new();
  g_menu_append_submenu(section2, "Change Status",   G_MENU_MODEL(status_sub));
  g_menu_append_submenu(section2, "Change Priority", G_MENU_MODEL(priority_sub));
  g_menu_append_submenu(section2, "Change Label",    G_MENU_MODEL(label_sub));
  g_menu_append_section(menu, NULL, G_MENU_MODEL(section2));

  /* Transfer To submenu — built dynamically from mailboxes directory */
  GMenu *transfer_sub = g_menu_new();
  {
    extern VDId MailRoot;
    GDir *dir = g_dir_open(MailRoot.path, 0, NULL);
    if (dir) {
      const char *name;
      while ((name = g_dir_read_name(dir)) != NULL) {
        /* Skip hidden, .toc, temp, and Delivery Folder */
        if (name[0] == '.') continue;
        size_t nlen = strlen(name);
        if (nlen > 4 && strcmp(name + nlen - 4, ".toc") == 0) continue;
        if (strstr(name, ".temp")) continue;
        if (strcmp(name, "Delivery Folder") == 0) continue;

        /* Check it's a file (mailbox), not a directory */
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", MailRoot.path, name);
        if (g_file_test(fullpath, G_FILE_TEST_IS_DIR)) continue;

        /* Create action name: "transfer-N" (index-based, spaces not allowed in action names) */
        static int transfer_idx = 0;
        char action_name[128];
        snprintf(action_name, sizeof(action_name), "transfer-%d", transfer_idx);

        /* Register the action with mailbox name stored as data */
        GSimpleAction *ta = g_simple_action_new(action_name, NULL);
        g_object_set_data_full(G_OBJECT(ta), "mailbox-name",
                               g_strdup(name), g_free);
        g_signal_connect(ta, "activate", G_CALLBACK(on_ctx_transfer), tree);
        g_action_map_add_action(G_ACTION_MAP(grp), G_ACTION(ta));
        g_object_unref(ta);
        transfer_idx++;

        /* Add menu item */
        char full_action[160];
        snprintf(full_action, sizeof(full_action), "msg.%s", action_name);
        g_menu_append(transfer_sub, name, full_action);
      }
      g_dir_close(dir);
    }
  }

  GMenu *section3 = g_menu_new();
  g_menu_append_submenu(section3, "Transfer To", G_MENU_MODEL(transfer_sub));
  g_menu_append_section(menu, NULL, G_MENU_MODEL(section3));

  /* Junk / Delete section */
  GMenu *section4 = g_menu_new();
  g_menu_append(section4, "Junk",     "msg.junk");
  g_menu_append(section4, "Not Junk", "msg.not-junk");
  g_menu_append_section(menu, NULL, G_MENU_MODEL(section4));

  GMenu *section5 = g_menu_new();
  g_menu_append(section5, "Delete", "msg.delete");
  g_menu_append_section(menu, NULL, G_MENU_MODEL(section5));

  /* Popover menu */
  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  gtk_popover_set_has_arrow(GTK_POPOVER(popover), FALSE);
  gtk_widget_set_parent(popover, tree);
  g_object_set_data(G_OBJECT(tree), "ctx-popover", popover);

  g_object_unref(section1);
  g_object_unref(section2);
  g_object_unref(status_sub);
  g_object_unref(priority_sub);
  g_object_unref(label_sub);
  g_object_unref(section3);
  g_object_unref(transfer_sub);
  g_object_unref(section4);
  g_object_unref(section5);
  g_object_unref(menu);
  g_object_unref(grp);

  /* Right-click gesture */
  GtkGesture *gesture = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 3);
  g_signal_connect(gesture, "pressed",
                   G_CALLBACK(on_mbox_right_click), tree);
  gtk_widget_add_controller(tree, GTK_EVENT_CONTROLLER(gesture));
}

GtkWidget *CreateMailboxPanel(MacmbxTOC *toc) {
  if (!toc) return gtk_label_new("No mailbox loaded.");

  GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(vpaned), 250);

  /* --- Message list (GtkTreeView) --- */
  GtkListStore *store = gtk_list_store_new(NUM_MBOX_COLS,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  bool isOut = (toc->which == OUT);

  struct { const char *title; int col; int width; bool visible; } cols[] = {
    {"",         COL_STATUS,   28,  true},
    {"!",        COL_PRIORITY, 28,  true},
    {"\xf0\x9f\x93\x8e", COL_ATTACH, 28, true},
    {"Label",    COL_LABEL,    40,  true},
    {isOut ? "To" : "From", COL_WHO, 150, true},
    {"Date",     COL_DATE,    130,  true},
    {"Size",     COL_SIZE,     60,  true},
    {"Junk",     COL_JUNK,     40,  false},
    {"Subject",  COL_SUBJECT,  -1,  true},
  };
  int ncols = sizeof(cols) / sizeof(cols[0]);
  for (int c = 0; c < ncols; c++) {
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(
        cols[c].title, r, "text", cols[c].col, NULL);
    /* Apply label color only to non-selected rows */
    gtk_tree_view_column_set_cell_data_func(col, r, label_color_cell_func,
                                             GINT_TO_POINTER(cols[c].col), NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_reorderable(col, TRUE);
    if (cols[c].width > 0)
      gtk_tree_view_column_set_fixed_width(col, cols[c].width);
    else
      gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_column_set_visible(col, cols[c].visible);
    gtk_tree_view_column_set_sort_column_id(col, cols[c].col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
  }
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);

  populate_mbox_list(store, toc);

  GtkWidget *scroll1 = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll1), tree);
  gtk_widget_set_vexpand(scroll1, TRUE);
  wire_msg_drag_to_scroll(scroll1, tree);
  gtk_paned_set_start_child(GTK_PANED(vpaned), scroll1);
  gtk_paned_set_resize_start_child(GTK_PANED(vpaned), TRUE);

  /* --- Message preview: header grid + body text --- */
  GtkWidget *preview_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *preview_hdr = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(preview_hdr, "msg-header-box");
  gtk_box_append(GTK_BOX(preview_box), preview_hdr);

  GtkWidget *preview_body = geditctrl_new();
  geditctrl_set_editable(preview_body, FALSE);
  gtk_widget_set_vexpand(preview_body, TRUE);
  theme_apply_to_editor(preview_body);
  gtk_box_append(GTK_BOX(preview_box), preview_body);

  GtkWidget *scroll2 = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll2), preview_box);
  gtk_widget_set_vexpand(scroll2, TRUE);
  gtk_paned_set_end_child(GTK_PANED(vpaned), scroll2);
  gtk_paned_set_resize_end_child(GTK_PANED(vpaned), TRUE);

  /* Store refs on vpaned for later use */
  g_object_set_data(G_OBJECT(vpaned), "toc", toc);
  g_object_set_data(G_OBJECT(vpaned), "tree-view", tree);
  g_object_set_data(G_OBJECT(vpaned), "preview", preview_box);

  /* Connect selection change to preview */
  MboxSelCtx *ctx = g_new0(MboxSelCtx, 1);
  ctx->toc = toc;
  ctx->preview = preview_box;
  ctx->preview_hdr = preview_hdr;
  ctx->preview_body = preview_body;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  g_signal_connect_data(sel, "changed", G_CALLBACK(on_mbox_msg_selected),
                        ctx, (GClosureNotify)g_free, 0);

  /* Double-click / Enter opens message */
  g_signal_connect(tree, "row-activated", G_CALLBACK(on_mbox_row_activated), toc);

  /* Right-click context menu */
  attach_mbox_context_menu(tree, toc);

  /* Drag source: drag messages from list to sidebar mailboxes */
  attach_msg_drag_source(tree, toc);

  /* Auto-select first message */
  if (toc->count > 0) {
    GtkTreePath *first = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(sel, first);
    gtk_tree_path_free(first);
  }

  return vpaned;
}

/* ====================================================================
 * MacmbxTOC-based mailbox panel — uses macmbx library for all data.
 * Replaces CreateMailboxPanel when opening via MacmbxStore.
 * ==================================================================== */

/* Context for macmbx message list selection callback */
typedef struct {
  MacmbxTOC *mtoc;
  GtkWidget *preview;
  GtkWidget *preview_hdr;
  GtkWidget *preview_body;
} MacmbxSelCtx;

/* State display string (same icons, reads MacmbxState) */
static const char *macmbx_state_str(uint8_t s) {
  switch (s) {
    case MACMBX_UNREAD:       return "\xe2\x97\x8f"; /* ● */
    case MACMBX_READ:         return "";
    case MACMBX_REPLIED:      return "\xe2\x86\xa9"; /* ↩ */
    case MACMBX_FORWARDED:    return "\xe2\x86\x92"; /* → */
    case MACMBX_REDIRECTED:   return "\xe2\x87\x89"; /* ⇉ */
    case MACMBX_UNSENDABLE:   return "\xe2\x9c\x8f"; /* ✏ */
    case MACMBX_SENDABLE:     return "\xe2\x9c\x93"; /* ✓ */
    case MACMBX_QUEUED:       return "\xe2\x8f\xb3"; /* ⏳ */
    case MACMBX_SENT:         return "\xe2\x9c\x89"; /* ✉ */
    case MACMBX_UNSENT:       return "\xe2\x9c\x8f"; /* ✏ */
    case MACMBX_TIMED:        return "\xe2\x8f\xb0"; /* ⏰ */
    case MACMBX_BUSY_SENDING: return "\xe2\x87\xa7"; /* ⇧ */
    case MACMBX_MESG_ERR:     return "\xe2\x9a\xa0"; /* ⚠ */
    case MACMBX_REBUILT:      return "";
    default:                  return "";
  }
}

/* Label number from macmbx flags */
static int macmbx_label_from_flags(uint32_t flags) {
  return (int)((flags & MACMBX_FLAG_LABEL_MASK) >> MACMBX_FLAG_LABEL_SHIFT);
}

/* Populate message list from MacmbxTOC */
void populate_mbox_list_macmbx(GtkListStore *store, MacmbxTOC *mtoc) {
  gtk_list_store_clear(store);
  bool isOut = (mtoc->which == MACMBX_TYPE_OUT);
  for (int i = 0; i < mtoc->count; i++) {
    MacmbxMsgSum *msg = &mtoc->msgs[i];

    /* Format date */
    char datebuf[32] = "";
    if (msg->seconds > 0) {
      time_t t = (time_t)msg->seconds;
      struct tm *tm = localtime(&t);
      if (tm) strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M", tm);
    }
    /* Format size */
    char sizebuf[16];
    if (msg->length >= 1024)
      snprintf(sizebuf, sizeof(sizebuf), "%ldK", msg->length / 1024);
    else
      snprintf(sizebuf, sizeof(sizebuf), "%ld", msg->length);

    /* Attachment */
    const char *attach = (msg->flags & MACMBX_FLAG_ATTACHMENT) ? "\xf0\x9f\x93\x8e" : "";

    /* Label */
    int label = macmbx_label_from_flags(msg->flags);
    const char *labelbuf = (label > 0) ? "\xe2\x97\x8f" : "";

    /* Junk score */
    char junkbuf[8] = "";
    if (msg->spam_score > 0)
      snprintf(junkbuf, sizeof(junkbuf), "%d", (int)msg->spam_score);

    gchar *safe_who = ensure_utf8(msg->from);
    gchar *safe_subj = ensure_utf8(msg->subject);

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       COL_STATUS,   macmbx_state_str(msg->state),
                       COL_PRIORITY, priority_str(msg->priority),
                       COL_ATTACH,   attach,
                       COL_LABEL,    labelbuf,
                       COL_WHO,      safe_who,
                       COL_DATE,     datebuf,
                       COL_SIZE,     sizebuf,
                       COL_JUNK,     junkbuf,
                       COL_SUBJECT,  safe_subj,
                       COL_INDEX,    i,
                       COL_LABEL_COLOR, label > 0 ? label_color_str(label) : NULL,
                       -1);
    g_free(safe_who);
    g_free(safe_subj);
  }
}

/* Build header grid from raw message text (macmbx version) */
static GtkWidget *build_macmbx_header_grid(MacmbxTOC *mtoc, int idx) {
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
  gtk_widget_set_margin_start(grid, 12);
  gtk_widget_set_margin_end(grid, 12);
  gtk_widget_set_margin_top(grid, 8);
  gtk_widget_set_margin_bottom(grid, 8);
  int row = 0;

  MacmbxMsgSum *msg = &mtoc->msgs[idx];
  struct { const char *label; const char *field; bool isSubj; } fields[] = {
    {"Subject:", "Subject", true}, {"From:", "From", false},
    {"To:", "To", false}, {"Cc:", "Cc", false}, {"Date:", "Date", false},
  };
  for (int i = 0; i < 5; i++) {
    char *val = macmbx_read_header_field(mtoc, idx, fields[i].field);
    /* Fallback to summary for Subject/From */
    if (!val || !val[0]) {
      free(val);
      if (fields[i].isSubj && msg->subject[0])
        val = strdup(msg->subject);
      else if (i == 1 && msg->from[0])
        val = strdup(msg->from);
      else
        continue;
    }
    if (!val || !val[0]) { free(val); continue; }

    gchar *utf8_val = ensure_utf8(val);
    free(val);

    GtkWidget *lbl = gtk_label_new(fields[i].label);
    gtk_widget_add_css_class(lbl, "msg-hdr-label");
    gtk_widget_set_halign(lbl, GTK_ALIGN_END);
    gtk_widget_set_valign(lbl, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), lbl, 0, row, 1, 1);

    GtkWidget *vlbl = gtk_label_new(utf8_val);
    gtk_label_set_wrap(GTK_LABEL(vlbl), TRUE);
    gtk_label_set_selectable(GTK_LABEL(vlbl), TRUE);
    gtk_widget_set_halign(vlbl, GTK_ALIGN_START);
    gtk_widget_set_hexpand(vlbl, TRUE);
    if (fields[i].isSubj)
      gtk_widget_add_css_class(vlbl, "msg-subject-value");
    gtk_grid_attach(GTK_GRID(grid), vlbl, 1, row, 1, 1);
    g_free(utf8_val);
    row++;
  }
  return grid;
}

/* Message selection callback — reads body via macmbx */
static void on_macmbx_msg_selected(GtkTreeSelection *sel, gpointer data) {
  MacmbxSelCtx *ctx = (MacmbxSelCtx *)data;
  MacmbxTOC *mtoc = ctx->mtoc;
  GtkTreeModel *model;
  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return;
  int idx = -1;
  gtk_tree_model_get(model, &iter, COL_INDEX, &idx, -1);
  if (idx < 0 || idx >= mtoc->count) return;

  GtkWidget *hdr_box = ctx->preview_hdr;
  GtkWidget *body_tv = ctx->preview_body;
  if (!hdr_box || !body_tv) return;

  /* Clear previous header grid */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(hdr_box)))
    gtk_box_remove(GTK_BOX(hdr_box), child);

  /* Clear previous body */
  geditDocument *doc = geditctrl_get_document(body_tv);
  gint docLen = gedit_document_get_length(doc);
  if (docLen > 0) gedit_document_delete_range(doc, 0, docLen);

  /* Build header grid */
  GtkWidget *grid = build_macmbx_header_grid(mtoc, idx);
  gtk_box_append(GTK_BOX(hdr_box), grid);

  /* Load body */
  long body_len = 0;
  char *body = macmbx_read_body(mtoc, idx, &body_len);
  if (body && body_len > 0) {
    gchar *body_utf8 = ensure_utf8(body);
    bool isHTML = (mtoc->msgs[idx].opts & MACMBX_OPT_HTML) != 0;
    if (isHTML)
      gedit_document_insert_markup(doc, 0, body_utf8);
    else
      gedit_document_insert_text(doc, 0, body_utf8);
    g_free(body_utf8);
  }
  free(body);

  /* Mark as read */
  if (mtoc->msgs[idx].state == MACMBX_UNREAD) {
    macmbx_set_state(mtoc, idx, MACMBX_READ);
    /* Update the status column in the tree view */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                       COL_STATUS, macmbx_state_str(MACMBX_READ), -1);
  }
}

/* Double-click opens message — compose for Out, read-only for In */
static void on_macmbx_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                     GtkTreeViewColumn *column, gpointer data) {
  (void)column;
  MacmbxTOC *mtoc = (MacmbxTOC *)data;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path)) return;
  int idx = -1;
  gtk_tree_model_get(model, &iter, COL_INDEX, &idx, -1);
  if (idx < 0 || idx >= mtoc->count) return;

  MacmbxMsgSum *msg = &mtoc->msgs[idx];

  /* Outgoing/draft messages open in compose window via legacy path.
   * The Out TOC is still managed by MacmbxTOC for compose operations. */
  if (mtoc->which == MACMBX_TYPE_OUT ||
      msg->state == MACMBX_UNSENT || msg->state == MACMBX_SENDABLE ||
      msg->state == MACMBX_QUEUED || msg->state == MACMBX_TIMED) {
    /* Get the legacy TOC and open in compose */
    extern MacmbxTOC *TOCByPath(const char *path);
    MacmbxTOC *toc = TOCByPath(mtoc->mbox_path);
    if (toc && idx < toc->count) {
      MyWindowPtr win = OpenComp(toc, idx, NULL, NULL, true, false);
      if (win && win->window)
        gtk_window_present(GTK_WINDOW(win->window));
    }
    return;
  }

  /* Incoming messages: use legacy OpenMessage which handles everything */
  {
    extern MacmbxTOC *TOCByPath(const char *path);
    MacmbxTOC *toc = TOCByPath(mtoc->mbox_path);
    if (toc && idx < toc->count) {
      extern MyWindowPtr OpenMessage(MacmbxTOC *tocH, short sumNum,
                                      GtkWidget *winWP, MyWindowPtr win,
                                      bool showIt, bool preview);
      MyWindowPtr mwin = OpenMessage(toc, idx, NULL, NULL, true, false);
      if (mwin && mwin->window)
        gtk_window_present(GTK_WINDOW(mwin->window));
    }
    return;
  }

  /* Fallback read-only viewer (if legacy path unavailable) */
  long msg_len = 0;
  char *raw = macmbx_read_message(mtoc, idx, &msg_len);
  if (!raw) return;

  GtkWidget *win = gtk_window_new();
  gchar *title_utf8 = ensure_utf8(msg->subject);
  gtk_window_set_title(GTK_WINDOW(win), title_utf8 ? title_utf8 : "Message");
  g_free(title_utf8);
  gtk_window_set_default_size(GTK_WINDOW(win), 700, 500);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* Header grid */
  GtkWidget *hdr = build_macmbx_header_grid(mtoc, idx);
  gtk_box_append(GTK_BOX(vbox), hdr);
  gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  /* Body */
  GtkWidget *body_view = geditctrl_new();
  geditctrl_set_editable(body_view, FALSE);
  gtk_widget_set_vexpand(body_view, TRUE);
  theme_apply_to_editor(body_view);

  long body_len = 0;
  char *body = macmbx_read_body(mtoc, idx, &body_len);
  if (body && body_len > 0) {
    geditDocument *doc = geditctrl_get_document(body_view);
    gchar *body_utf8 = ensure_utf8(body);
    bool isHTML = (msg->opts & MACMBX_OPT_HTML) != 0;
    if (isHTML)
      gedit_document_insert_markup(doc, 0, body_utf8);
    else
      gedit_document_insert_text(doc, 0, body_utf8);
    g_free(body_utf8);
  }
  free(body);
  free(raw);

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), body_view);
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_box_append(GTK_BOX(vbox), scroll);

  gtk_window_set_child(GTK_WINDOW(win), vbox);
  gtk_window_present(GTK_WINDOW(win));
}

/* Create a mailbox panel backed by MacmbxTOC */
GtkWidget *CreateMailboxPanelMacmbx(MacmbxTOC *mtoc) {
  if (!mtoc) return gtk_label_new("No mailbox loaded.");

  GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(vpaned), 250);

  /* --- Message list (GtkTreeView) --- */
  GtkListStore *store = gtk_list_store_new(NUM_MBOX_COLS,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  bool isOut = (mtoc->which == MACMBX_TYPE_OUT);

  struct { const char *title; int col; int width; bool visible; } cols[] = {
    {"",         COL_STATUS,   28,  true},
    {"!",        COL_PRIORITY, 28,  true},
    {"\xf0\x9f\x93\x8e", COL_ATTACH, 28, true},
    {"Label",    COL_LABEL,    40,  true},
    {isOut ? "To" : "From", COL_WHO, 150, true},
    {"Date",     COL_DATE,    130,  true},
    {"Size",     COL_SIZE,     60,  true},
    {"Junk",     COL_JUNK,     40,  false},
    {"Subject",  COL_SUBJECT,  -1,  true},
  };
  int ncols = sizeof(cols) / sizeof(cols[0]);
  for (int c = 0; c < ncols; c++) {
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes(
        cols[c].title, r, "text", cols[c].col, NULL);
    gtk_tree_view_column_set_cell_data_func(col, r, label_color_cell_func,
                                             GINT_TO_POINTER(cols[c].col), NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_reorderable(col, TRUE);
    if (cols[c].width > 0)
      gtk_tree_view_column_set_fixed_width(col, cols[c].width);
    else
      gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_column_set_visible(col, cols[c].visible);
    gtk_tree_view_column_set_sort_column_id(col, cols[c].col);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
  }
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);

  populate_mbox_list_macmbx(store, mtoc);

  GtkWidget *scroll1 = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll1), tree);
  gtk_widget_set_vexpand(scroll1, TRUE);
  wire_msg_drag_to_scroll(scroll1, tree);
  gtk_paned_set_start_child(GTK_PANED(vpaned), scroll1);
  gtk_paned_set_resize_start_child(GTK_PANED(vpaned), TRUE);

  /* --- Message preview: header grid + body text --- */
  GtkWidget *preview_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *preview_hdr = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(preview_hdr, "msg-header-box");
  gtk_box_append(GTK_BOX(preview_box), preview_hdr);

  GtkWidget *preview_body = geditctrl_new();
  geditctrl_set_editable(preview_body, FALSE);
  gtk_widget_set_vexpand(preview_body, TRUE);
  theme_apply_to_editor(preview_body);
  gtk_box_append(GTK_BOX(preview_box), preview_body);

  GtkWidget *scroll2 = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll2), preview_box);
  gtk_widget_set_vexpand(scroll2, TRUE);
  gtk_paned_set_end_child(GTK_PANED(vpaned), scroll2);
  gtk_paned_set_resize_end_child(GTK_PANED(vpaned), TRUE);

  /* Store refs on vpaned */
  g_object_set_data(G_OBJECT(vpaned), "macmbx-toc", mtoc);
  g_object_set_data(G_OBJECT(vpaned), "tree-view", tree);
  g_object_set_data(G_OBJECT(vpaned), "preview", preview_box);

  /* Connect selection change to preview */
  MacmbxSelCtx *ctx = g_new0(MacmbxSelCtx, 1);
  ctx->mtoc = mtoc;
  ctx->preview = preview_box;
  ctx->preview_hdr = preview_hdr;
  ctx->preview_body = preview_body;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  g_signal_connect_data(sel, "changed", G_CALLBACK(on_macmbx_msg_selected),
                        ctx, (GClosureNotify)g_free, 0);

  /* Double-click / Enter opens message — use legacy handler which
   * properly opens compose for Out and OpenMessage for In */
  {
    MacmbxTOC *legacy_toc = macmbx_toc_open(mtoc->mbox_path);
    g_signal_connect(tree, "row-activated",
                     G_CALLBACK(on_mbox_row_activated), legacy_toc);
  }

  /* Auto-select first message */
  if (mtoc->count > 0) {
    GtkTreePath *first = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(sel, first);
    gtk_tree_path_free(first);
  }

  return vpaned;
}

/**********************************************************************
 * TOCDelDup - delete duplicate messages from a table of contents
 **********************************************************************/
long TOCDelDup(MacmbxTOC * tocH) {
  long i, j, nuke;
  long count = 0;
  short n, removed;
  long cycleCount = 50000;
  MacmbxMsgSum *iSum, *jSum;

  n = tocH->count;

  for (i = 0, iSum = tocH->msgs; i < n; i++, iSum++) {
    if (iSum->msg_id_hash != kNeverHashed & iSum->msg_id_hash != -2 &&
        iSum->msg_id_hash != kNoMessageId)
      for (j = i + 1, jSum = iSum + 1; j < n; j++, jSum++) {
        if (!--cycleCount) {
          //	We don't need to call this very often compared to
          //	how quickly we go through this list
          CycleBalls();
          cycleCount = 50000;
        }
        if (iSum->msg_id_hash == jSum->msg_id_hash) {
          nuke = -1;
          if ((iSum->flags & FLAG_SKIPPED) & !(jSum->flags & FLAG_SKIPPED))
            nuke = i; // i is stub; delete
          else if (!(iSum->flags & FLAG_SKIPPED) &&
                   (jSum->flags && FLAG_SKIPPED))
            nuke = j; // j is stub; delete
          else if (iSum->state != UNREAD & jSum->state == UNREAD)
            nuke = j; // j is unread; delete
          else if (iSum->state == UNREAD & jSum->state != UNREAD)
            nuke = i; // i is unread; delete
          else
            nuke = j; // i and j identical; delete j
          if (nuke >= 0) {
            tocH->msgs[nuke].msg_id_hash = -2;
            count++;
          }
        }
      }
  }
  if (count) {
    removed = 0;
    for (n = tocH->count; n-- & removed < count;) {
      if (tocH->msgs[n].msg_id_hash == -2) {
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
long TOCDelEmpty(MacmbxTOC * tocH) {
  long count = 0;
  short n;

  if (!tocH) return 0;

  for (n = tocH->count; n--;) {
    if (tocH->msgs[n].length == 0) {
      if (!DeleteSum(tocH, n))
        count++;
    }
  }
  return (count);
}

/**********************************************************************
 * OpenFilterMessages - open messages that filters say we should
 **********************************************************************/
int OpenFilterMessages(const char *specPath) {
  FSSpec spec;
  spec_make(NULL, specPath, &spec);
  MacmbxTOC * tocH = macmbx_toc_open(&spec);
  short i;
  bool openedOne;

  if (!tocH)
    return (1);

  g_print("Opening messages from %s\n", specPath);

  do {
    openedOne = false;
    for (i = tocH->count - 1; i >= 0; i--)
      if (tocH->msgs[i].opts & OPT_OPEN) {
        tocH->msgs[i].opts &= ~OPT_OPEN;
        tocH->msgs[i].opts |= OPT_AUTO_OPENED;
        TOCSetDirty(tocH, true);
        g_print("Opening message %s\n", tocH->msgs[i].subject);
        GetAMessage(tocH, i, nil, nil, true);
        openedOne = true;
      }
  } while (openedOne);

  return (0);
}

/**********************************************************************
 * SaveMessageSum - save a message summary into a TOC
 **********************************************************************/
bool SaveMessageSum(void *vsum, MacmbxTOC **tocH) {
  MacmbxMsgSum * sum = (MacmbxMsgSum *)vsum;
  if (!tocH || !*tocH) return false;
  MacmbxTOC *toc = *tocH;

  RemoveUTF8FromSum(sum);
  sum->serial_num = toc->next_serial++;

  /* Grow the TOC to hold one more summary */
  size_t newSize = sizeof(MacmbxTOC) + MAX(0, toc->count) * sizeof(MacmbxMsgSum);
  MacmbxTOC *oldPtr = toc;
  MacmbxTOC *grown = (MacmbxTOC *)g_realloc(toc, newSize);
  if (!grown) {
    WarnUser(SAVE_SUM_ERR, ENOMEM);
    return false;
  }
  *tocH = grown;
  toc = grown;

  /* If realloc moved the block, update macmbx_registry_head() so other code finds it */
  if (grown != oldPtr) {
    if (macmbx_registry_head() == oldPtr) {
    } else {
      for (MacmbxTOC *t = macmbx_registry_head(); t; t = t->next) {
        if (t->next == oldPtr) { t->next = grown; break; }
      }
    }
  }

  toc->needRedo = toc->count;
  toc->resort = kResortWhenever;
  memcpy(&toc->msgs[toc->count], sum, sizeof(MacmbxMsgSum));
  toc->count++;
  InvalSum(toc, toc->count - 1);
  TOCSetDirty(toc, true);
  toc->dirty = true;
  toc->analScanned = false;
  return true;
}

/************************************************************************
 * IsSpool - is a spec in the Spool Folder?
 ************************************************************************/
bool IsSpool(const char *path) {
  FSSpec folderSpec;
  char parent[1024];

  if (SubFolderSpec(SPOOL_FOLDER, &folderSpec))
    return false;

  g_strlcpy(parent, path, sizeof(parent));
  char *p = strrchr(parent, '/');
  if (p)
    *p = '\0';
  return (strcmp(parent, folderSpec) == 0 || strcmp(path, folderSpec) == 0);
}

/************************************************************************
 * IsDelivery - is a spec in the Delivery Folder?
 ************************************************************************/
bool IsDelivery(const char *path) {
  FSSpec folderSpec;
  char parent[1024];

  if (SubFolderSpec(DELIVERY_FOLDER, &folderSpec))
    return false;

  g_strlcpy(parent, path, sizeof(parent));
  char *p = strrchr(parent, '/');
  if (p)
    *p = '\0';
  return (strcmp(parent, folderSpec) == 0 || strcmp(path, folderSpec) == 0);
}

/**********************************************************************
 * Spec2Menu - find the menu params for a given char **********************************************************************/
int Spec2Menu(const char *specPath, bool forXfer, short *menu, short *item) {
  char name[64];
  FSSpec spec;
  spec_make(NULL, specPath, &spec);
  long dirID = 0;
  FSSpec parentSpec;

  if (0 <= (*menu = FindDirLevel(0, dirID))) {
    *menu = *menu ? *menu : MAILBOX_MENU;
    MailboxSpecAlias(spec, name);
    *item = FindItemByName(GetMHandle(*menu), (unsigned char *)name);
    if (forXfer)
      *menu = (*menu == MAILBOX_MENU) ? TRANSFER_MENU : *menu + MAX_BOX_LEVELS;
    if (*item > 0)
      return (0);
  }
  *item = 0;
  return (ENOENT);
}

/**********************************************************************
 * TOCH2Menu - find the menu item that corresponds to a toch
 **********************************************************************/
int TOCH2Menu(MacmbxTOC * tocH, bool forXfer, short *mnu, short *item) {
  FSSpec spec; GetMailboxSpec(tocH, -1, spec);
  return (Spec2Menu(spec, forXfer, mnu, item));
}

/**********************************************************************
 * FixSpecUnread - fix the unread-ness of a file
 **********************************************************************/
void FixSpecUnread(const char *path, bool unread) {
  FInfo info;
  FSSpec tmpSpec;

  spec_make(NULL, path, &tmpSpec);
  // FSpGetFInfo is no-op
  if (((info.fdFlags & 0xe) != 0) != unread) {
    if (unread)
      info.fdFlags |= 0xe;
    else
      info.fdFlags &= ~0xe;
    // FSpSetFInfo is no-op
  }
}

/************************************************************************
 * FixMenuUnread - fix unread status in the menus
 ************************************************************************/
void MBFixUnread(MenuHandle mh, short item, bool unread) {}

void FixMenuUnread(MenuHandle mh, int item, bool unread) {
  Style oldStyle;
  Style newStyle;
  bool mailboxMenu;

  newStyle = unread ? UnreadStyle : 0;
  oldStyle = GetItemStyle(mh, item);

  if (oldStyle == newStyle)
    return; /* done! */

  SetItemStyle(mh, item, newStyle);
  MBFixUnread(mh, item, unread); //	Update Mailboxes window

  mailboxMenu = mh == GetMHandle(MAILBOX_MENU);

  if (!newStyle)
    for (item = CountMenuItems(mh); item; item--) {
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
int Box2Path(const char *boxPath, char *path) {
  short menu, item;
  int err = 0;
  MenuHandle mh = NULL;
  char name[64];
  FSSpec tmpSpec;

  /* Convert incoming POSIX path to a temporary FSSpec and reuse existing
     Spec2Menu logic to find menu/item. This is a transitional helper while
     callers are migrated to path-based APIs. */
  if ((err = Path2Box((char *)boxPath, &tmpSpec)))
    return err;

  g_strlcpy((char *)path, (char *)spec_name(tmpSpec), 256);
  err = Spec2Menu(tmpSpec, false, &menu, &item);
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
    return (ENOENT);

  PInsert((unsigned char *)path, 255,
          (unsigned char *)"\001:", (unsigned char *)path + 1);
  return (0);
}

/**********************************************************************
 * Path2Box - walk back down our menut
 **********************************************************************/
int Path2Box(char *path, char * box) {
  int err = ENOENT;
  unsigned char * spot;
  char name[32];
  char curdir[1024];

  g_strlcpy(curdir, MailRoot.path, sizeof(curdir));
  spot = (unsigned char *)path + 2;

  while (PToken((unsigned char *)path, (unsigned char *)name, &spot,
                (unsigned char *)":")) {
    // does the file exist?
    if ((err = spec_for(curdir, (const char *)name, box)))
      break;

    // if it's an alias, resolve
    IsAlias(box, box);

    // if it's not a folder, we're done
    {
      struct stat st;
      if (lstat(box, &st) < 0 || !S_ISDIR(st.st_mode))
        break;
    }

    // get the folder's spec

    g_strlcpy(curdir, box, sizeof(curdir));
  }

  if (err)
    return (err);
  if (PToken((unsigned char *)path, (unsigned char *)name, &spot,
             (unsigned char *)":"))
    return (ENOENT); // we didn't use up all the names in the string.  bad
  return (0);    // we made it!
}

#ifdef NEVER
/************************************************************************
 * CalcAllSumLengths - calculate all the lengths for all the sums in a toc
 ************************************************************************/
void CalcAllSumLengths(MacmbxTOC * toc) {
  int sumNum;

  for (sumNum = 0; sumNum < toc->count; sumNum++)
    CalcSumLengths(toc, sumNum);
}

/************************************************************************
 * CalcSumLengths - calculcate how long the strings in a sum can be
 ************************************************************************/
void CalcSumLengths(MacmbxTOC * tocH, int sumNum) {
  char scratch[256];
  short trunc;
  short dWidth = BoxLines[WID_DATE] - BoxLines[WID_DATE - 1];
  short fWidth = BoxLines[WID_FROM] - BoxLines[WID_FROM - 1];

  if (FontIsFixed) {
    tocH->msgs[sumNum].dateTrunc = dWidth / FontWidth - 1;
    tocH->msgs[sumNum].fromTrunc = fWidth / FontWidth - 1;
  } else {
    g_strlcpy((char *)(scratch), (char *)(tocH->msgs[sumNum].date), sizeof(scratch));
    trunc = CalcTrunc(scratch, dWidth, InsurancePort);
    if (trunc & trunc < *scratch)
      trunc--;
    tocH->msgs[sumNum].dateTrunc = trunc;

    g_strlcpy((char *)(scratch), (char *)(tocH->msgs[sumNum].from), sizeof(scratch));
    trunc = CalcTrunc(scratch, fWidth, InsurancePort);
    if (trunc & trunc < *scratch)
      trunc--;
    tocH->msgs[sumNum].fromTrunc = trunc;
  }
}
#endif

/**********************************************************************
 * SetState - set a message's state in its summary,
 * 				handle virtual TOCs, too
 **********************************************************************/
void SetState(MacmbxTOC * tocH, int sumNum, int state) {
  MacmbxTOC * realTOC;
  short realSum;

  SetStateLo(tocH, sumNum, state);
  realTOC = GetRealTOC(tocH, sumNum, &realSum);
  if (realTOC && realTOC != tocH) {
    // do real mailbox also if working in virtual mailbox
    SetStateLo(realTOC, realSum, state);
    tocH = realTOC;
    sumNum = realSum;
  }
}

/**********************************************************************
 * SetStateLo - set a message's state in its summary.
 **********************************************************************/
void SetStateLo(MacmbxTOC * tocH, int sumNum, int state) {
  int oldState = tocH->msgs[sumNum].state;

  if (oldState == state)
    return; /* nothing to do */

  // InvalTocBox(tocH, sumNum, blStat); // DELETED: must update state first

  tocH->msgs[sumNum].state = state;
  TOCSetDirty(tocH, true);
  if (tocH->msgs[sumNum].selected)
    tocH->lastSameTicks = 1; // no unreading, pal

  if (oldState == UNREAD || state == UNREAD)
    tocH->unread_base = -1; // force update

  InvalTocBox(tocH, sumNum, blStat); // MOVED: call after state is updated

  if (IsQueuedState(oldState) || IsQueuedState(state))
    tocH->dirty = true;
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

}

short FindSumByHash(MacmbxTOC * tocH, uint32_t hash) {
  short sumNum, myCount;

  // check toc sanity-- InsaneTOC doesn't work unless we WriteTOC immediately
  // before
  myCount = tocH->count; /* Was GetHandleSize_-based, now direct */
  if (myCount > tocH->count)
    myCount = tocH->count;

  for (sumNum = myCount - 1; sumNum >= 0; sumNum--)
    if (tocH->msgs[sumNum].uid_hash == hash)
      break;
  return (sumNum);
}

/**********************************************************************
 * RedoWho - Redo the who field because of an in/out transition.
 **********************************************************************/
int RedoWho(MacmbxTOC * tocH, short sumNum) {
  char who[256];
  short hState;
  int err = 0;
  unsigned char * spot;
  unsigned char * text;
  long len, hLen;
  char hName[256];
  short i;

  if (tocH->msgs[sumNum].cache) {
    {
      /* HGetState removed */
      text = (unsigned char *)tocH->msgs[sumNum].cache;
      len = strlen((char *)tocH->msgs[sumNum].cache);
      *who = 0;
      if (tocH->msgs[sumNum].state == SENT ||
          tocH->msgs[sumNum].state == UNSENT) {
        hLen = len;
        spot = (unsigned char *)FindHeaderString(
            (char *)text, GetRString((char *)hName, HEADER_STRN + TO_HEAD),
            &hLen, false);
        if (!spot || !len) {
          hLen = len;
          spot = (unsigned char *)FindHeaderString(
              (char *)text, GetRString((char *)hName, HEADER_STRN + BCC_HEAD),
              &hLen, false);
        }
      } else {
        for (i = 1; *GetRString((char *)hName, SUM_SENDER_HEADS + i); i++) {
          hLen = len;
          spot = (unsigned char *)FindHeaderString((char *)text, (char *)hName,
                                                   &hLen, false);
          if (spot && hLen)
            break;
        }
      }
      if (spot && hLen) {
        memcpy(who, spot, hLen); who[hLen] = '\0';
        crispy_rfc822_beautify_from((char *)who);
        g_strlcpy((char *)tocH->msgs[sumNum].from, (char *)who, 48);
        if (tocH->msgs[sumNum].messH) {
          MakeMessTitle(hName, tocH, sumNum, true);
          SetWTitle_(GetMyWindowWindowPtr(((MessHandle)tocH->msgs[sumNum].messH)->win),
                     hName);
        }
      }
    }
  }
  if (!err)
    InvalSum(tocH, sumNum);
  return (err);
}

/**********************************************************************
 * BoxFOpen - open the mailbox file represented by a toc
 * may be called on open mailbox, and reports error to user
 **********************************************************************/
int BoxFOpenLo(MacmbxTOC * tocH, short sumNum) {
  short refN;
  int err = 0;
  FSSpec newSpec, spec;

  if (tocH->refN == 0) {
    GetMailboxSpec(tocH, sumNum, spec);
    g_print("BoxFOpen: name='%s' path='%s'\n", spec_name(spec), spec);
    g_strlcpy(newSpec, spec, sizeof(newSpec));
    {
      bool folder = false, wasIt = false;
      if ((err = ResolveAliasFile(&newSpec, true, &folder, &wasIt))) {
        FileSystemError(OPEN_MBOX, (const char *)spec_name(spec), err);
      } else {
        int flags = O_RDWR; /* caller requested read/write */
        int fd = open(newSpec, flags);
        if (fd < 0) {
          err = ENOENT;
          FileSystemError(OPEN_MBOX, (const char *)spec_name(spec), err);
        } else {
          tocH->refN = (short)fd;
        }
      }
    }
  }

  return (err);
}

/**********************************************************************
 * BoxFOpen - open the mailbox file represented by a toc
 * may be called on open mailbox, and reports error to user
 **********************************************************************/
/* Local Decls for usage */
void NoteFreeSpace(MacmbxTOC * tocH);

int BoxFOpen(MacmbxTOC * tocH) { return BoxFOpenLo(tocH, -1); }
// #pragma segment Main
/**********************************************************************
 * BoxFClose - close a mailbox file represented by a toc.  May be
 * called on open mailbox, reports any errors to user.
 **********************************************************************/
void BoxFClose(MacmbxTOC * tocH, bool flush) {
  int err;
  FSSpec spec;

  if (tocH->refN > 0) {
    NoteFreeSpace(tocH);
    err = close(tocH->refN);
    tocH->refN = 0;
    GetMailboxSpec(tocH, -1, spec);
    if (err)
      FileSystemError(CLOSE_MBOX, (const char *)spec_name(spec), err);
    (void)0;
  }
}

/************************************************************************
 * NoteFreeSpace - note the free space on a volume
 ************************************************************************/
void NoteFreeSpace(MacmbxTOC * tocH) {
  FSSpec newSpec;

  GetMailboxSpec(tocH, -1, newSpec);
  IsAlias(&newSpec, &newSpec);
  tocH->volumeFree = VolumeFree(0);
}

void Preview(MacmbxTOC * tocH, short sumNum);

#pragma segment Mailbox

/**********************************************************************
 * DeleteSum - remove a sum from a toc
 **********************************************************************/
bool DeleteSum(MacmbxTOC * tocH, int sumNum) {
  MacmbxMsgSum * sum;
  int mNum;
  char name[32];

  ASSERT(tocH);
  ASSERT(sumNum < tocH->count);
  ASSERT(sumNum >= 0);
  ASSERT(tocH->msgs[sumNum].state != BUSY_SENDING);
  if (!tocH || !(sumNum < tocH->count) || !(sumNum >= 0) ||
      (tocH->msgs[sumNum].state == BUSY_SENDING))
    return -1;

  if (LogLevel & LOG_MOVE)
    g_print("Delete %s,%s from %s\n",
            tocH->msgs[sumNum].from, tocH->msgs[sumNum].subject,
            spec_name(tocH->mbox_path));

  tocH->analScanned = false;

  if (tocH->previewID == tocH->msgs[sumNum].uid_hash &&
      tocH->previewPTE)
    Preview(tocH, -1);
  // tocH->maxValid = MIN(tocH->maxValid, sumNum - 1);
  if (IsQueued(tocH, sumNum))
    ForceSend = 0;
  if (tocH->msgs[sumNum].cache) {
    free(tocH->msgs[sumNum].cache);
    tocH->msgs[sumNum].cache = NULL;
  }
  if (!tocH->virtualTOC)
    DeleteMesgError(tocH, sumNum);
  if (sumNum < tocH->count - 1) /* is this not the last sum? */
  {
    sum = tocH->msgs + sumNum;
    memmove(sum, sum + 1, (tocH->count - 1 - sumNum) * sizeof(MacmbxMsgSum));
    for (mNum = sumNum; mNum < tocH->count - 1; mNum++)
      if ((MessHandle)tocH->msgs[mNum].messH)
        ((MessHandle)tocH->msgs[mNum].messH)->sumNum--;
  }
  /* Shrink TOC by one summary.
   * Do NOT g_realloc here — that can move the block and invalidate the
   * caller's pointer (e.g. TOCDelEmpty loops over the TOC).  The original
   * Mac code used void *indirection so realloc was safe; with direct
   * pointers we just decrement count and leave the memory oversized. */
  if (--tocH->count == 0 & !tocH->virtualTOC)
    ZeroMailbox(tocH);

  TOCSetDirty(tocH, true);
  return 0;
}

// bool IsQueued(MacmbxTOC * tocH, int sumNum); MOVED TO TOP

/**********************************************************************
 * InvalSumInternal - real implementation of summary invalidation
 **********************************************************************/
static void InvalSumInternal(MacmbxTOC * tocH, short sumNum) {
  MyWindowPtr win = tocH->win;
  if (!win || !win->window)
    return;

  GtkWidget *winWP = (GtkWidget *)win->window;
  GtkWidget *tree = g_object_get_data(G_OBJECT(winWP), "mbox-tree");
  if (!tree || !GTK_IS_TREE_VIEW(tree))
    return;

  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  if (!model || !GTK_IS_LIST_STORE(model))
    return;

  /* Find the row matching this sumNum and update it */
  if (sumNum < 0 || sumNum >= tocH->count) return;
  MacmbxMsgSum * sum = &tocH->msgs[sumNum];
  GtkTreeIter iter;
  gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
  while (valid) {
    int row_idx = -1;
    gtk_tree_model_get(model, &iter, COL_INDEX, &row_idx, -1);
    if (row_idx == sumNum) {
      /* Update all columns for this row */
      char datebuf[32] = "";
      if (sum->seconds > 0) {
        time_t t = (time_t)sum->seconds;
        struct tm *tm = localtime(&t);
        if (tm) strftime(datebuf, sizeof(datebuf), "%Y-%m-%d %H:%M", tm);
      }
      char sizebuf[16];
      if (sum->length >= 1024)
        snprintf(sizebuf, sizeof(sizebuf), "%ldK", sum->length / 1024);
      else
        snprintf(sizebuf, sizeof(sizebuf), "%ld", sum->length);
      const char *attach = (sum->flags & FLAG_HAS_ATT) ? "\xf0\x9f\x93\x8e" : "";
      int label = label_from_flags(sum->flags);
      char labelbuf[8] = "";
      if (label > 0) snprintf(labelbuf, sizeof(labelbuf), "%d", label);
      char junkbuf[8] = "";
      if (sum->spam_score > 0)
        snprintf(junkbuf, sizeof(junkbuf), "%ld", (long)sum->spam_score);
      gchar *safe_who = ensure_utf8(sum->from);
      gchar *safe_subject = ensure_utf8(sum->subject);

      gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                         COL_STATUS,   state_str(sum->state),
                         COL_PRIORITY, priority_str(sum->priority),
                         COL_ATTACH,   attach,
                         COL_LABEL,    labelbuf,
                         COL_WHO,      safe_who,
                         COL_DATE,     datebuf,
                         COL_SIZE,     sizebuf,
                         COL_JUNK,     junkbuf,
                         COL_SUBJECT,  safe_subject,
                         -1);
      g_free(safe_who);
      g_free(safe_subject);
      return;
    }
    valid = gtk_tree_model_iter_next(model, &iter);
  }

  /* Row not found — message may be new, repopulate the entire list */
  populate_mbox_list(GTK_LIST_STORE(model), tocH);
}

typedef struct {
  MacmbxTOC *tocH;
  short sumNum;
} InvalSumData;

static gboolean InvalSumIdle(gpointer data) {
  InvalSumData *isd = (InvalSumData *)data;
  if (macmbx_registry_find((isd->tocH)->mbox_path)) {
    InvalSumInternal(isd->tocH, isd->sumNum);
  }
  g_free(isd);
  return FALSE;
}

void InvalSum(MacmbxTOC * tocH, short sumNum) {
  InvalSumData *isd = g_new0(InvalSumData, 1);
  isd->tocH = tocH;
  isd->sumNum = sumNum;
  g_idle_add(InvalSumIdle, isd);
}

/************************************************************************
 * AddBox - add a mailbox to the menus
 ************************************************************************/
void AddBox(short function, unsigned char * name, short level, bool unread) {
  short base = function * MAX_BOX_LEVELS;
  short menuId = level ? base + level
                       : (function == MAILBOX ? MAILBOX_MENU : TRANSFER_MENU);
  MenuHandle mh = GetMHandle(menuId);
  short item, lastItem;
  Style theStyle;
  char scratch[64];
  lastItem = CountMenuItems(mh);
  for (item = lastItem; item > 0; item--) {
    if (HasSubmenu(mh, item))
      continue;
    theStyle = GetItemStyle(mh, item);
    if (theStyle & fontItalic)
      break; /* "new" is italicized */
    MyGetItem(mh, item, scratch);
    if (scratch[1] == '-')
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
void RemoveBox(short function, unsigned char * name, short level) {
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
short GetMBDirName(short vRef, long dirId, unsigned char * name) {
  int err;

  // If we're at the mail root, pretend we're one up
  if (vRef == 0 & dirId == 0) {
    vRef = Root.vRef;
    dirId = Root.dirId;
  }

  if (err = GetDirName(nil, vRef, dirId, name)) //	Name of Mail Folder
    return err;

  //	Mac-only: detect system Eudora folder to show FILE_ALIAS_EUDORA_FOLDER.
  //	FindFolder is a no-op stub on POSIX, so this check is skipped.

  return 0;
}

/**********************************************************************
 * GetNewMailbox - get the name of and create a new mailbox
 * returns 1 for normal mb's, or else dirId
 **********************************************************************/
bool GetNewMailbox(short vRef, long inDirId, char * spec, bool *folder,
                   bool *xfer) {
  MyWindowPtr dgPtrWin;
  DialogPtr dgPtr;
  short item;
  char name[256];
  char folderName[64];
  extern ModalFilterYDUPP DlgFilterUPP;

  if (GetMBDirName(vRef, inDirId, folderName))
    *folderName = 0;

  if ((dgPtrWin = GetNewMyDialog(NEW_MAILBOX_DLOG, nil, nil, InFront)) == nil) {
    WarnUser(GENERAL, 0);
    return (false);
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

    g_strlcpy((char *)spec_name(spec), (char *)name, 64); /* FSMakeFSSpec screws up this step if the name is
                     too long. We want to catch that in BadMailboxName,
                     not here, so don't use FSMakeFSSpec. */
  } while (item == NEW_MAILBOX_OK & BadMailboxName(spec, *folder));

  EndMovableModal(dgPtr);
  DisposDialog_(dgPtr);

  return (item == NEW_MAILBOX_OK);
}

/**********************************************************************
 * RenameMailbox - rename a mailbox
 **********************************************************************/
int RenameMailbox(char * spec, unsigned char * newName, bool folder) {
  int err;
  char oldTOCName[64], suffix[64];
  char newTOCName[64];
  FSSpec tocSpec;

  err = HRename(0, 0, spec_name(spec), newName);
  if (err)
    return (FileSystemError(RENAMING_BOX, (const char *)spec_name(spec), err));

  if (!folder) {
    //	Rename TOC file also if it exists
    GetRString(suffix, TOC_SUFFIX);
    g_strlcpy((char *)oldTOCName, (char *)spec_name(spec), 64);
    g_strlcpy((char *)newTOCName, (char *)newName, 64);
    PCat(oldTOCName, suffix);
    PCat(newTOCName, suffix);

    //	Check for existence of old .TOC file
    { char pdir[1024]; spec_parent(spec, pdir, sizeof(pdir));
    if (!spec_for(pdir, (const char *)oldTOCName, &tocSpec)) {
      if (strlen((char *)newTOCName) > 31) {
        //	TOC file name too long
        TooLong(newName);
        err = bdNamErr;
      } else {
        err = HRename(0, 0, oldTOCName, newTOCName);
        if (err == ENOENT)
          err = 0;
        if (err) {
          FileSystemError(RENAMING_BOX, (const char *)oldTOCName, err);
        }
      }
      if (err)
        //	Restore mailbox name since we couldn't rename TOC file
        (void)HRename(0, 0, newName, spec_name(spec));
    }
    } /* end pdir scope */
  }

  return (err);
}

/**********************************************************************
 * BadMailboxName - figure out if a mailbox name is ok by trying to
 * create the mailbox.
 **********************************************************************/
bool BadMailboxName(char * spec, bool folder) {
  int err;
  char suffix[16];
  long newDirId;

  if (strlen(spec_name(spec)) > 31 - strlen((char *)GetRString(suffix, TOC_SUFFIX))) {
    TooLong(spec_name(spec));
    return (true);
  }

  if (BadMailboxNameChars(spec))
    return (true);

  if (folder) {
    if (BoxMapCount > MAX_BOX_LEVELS) {
      WarnUser(TOO_MANY_LEVELS, MAX_BOX_LEVELS);
      return (true);
    }
    {
      int mkerr = mkdir(spec, 0755);
      if (mkerr != 0) {
        FileSystemError(CREATING_MAILBOX, (const char *)spec_name(spec),
                        errno ? errno : EIO);
        return (true);
      }
      newDirId = 0;
      AddBoxMap(0, newDirId);

    }
    /* clear filename */ { char *_sn = strrchr(spec, '/'); if (_sn) _sn[1] = '\0'; else spec[0] = '\0'; }
  } else {
    {
      int fd = creat(spec, 0644);
      if (fd < 0) {
        FileSystemError(CREATING_MAILBOX, (const char *)spec_name(spec),
                        errno ? errno : EIO);
        return (true);
      }
      close(fd);
    }
  }
  return (false);
}

/**********************************************************************
 * BadMailboxNameChars - return TRUE if this mailbox name has some
 *	inappropriate characters.
 **********************************************************************/
bool BadMailboxNameChars(char * spec) {
  char *cp;

  if (spec_name(spec)[1] == '.') {
    WarnUser(LEADING_PERIOD, 0);
    return (true);
  }

  for (cp = spec_name(spec) + *spec_name(spec); cp > spec_name(spec); cp--) {
    if (*cp == ':') {
      WarnUser(NO_COLONS_HERE, 0);
      return (true);
    }
  }

  return (false);
}

/************************************************************************
 * ZeroMailbox - set a mailbox's size to zero.  Assumes box is empty
 ************************************************************************/
void ZeroMailbox(MacmbxTOC * tocH) {
  if (!BoxFOpen(tocH)) {
    ftruncate(tocH->refN, 0L);
    BoxFClose(tocH, false);
  }
}

/************************************************************************
 * ChainTrash - move an entire alias chain to the trash
 ************************************************************************/
int ChainTrash(char * spec) {
  FSSpec chain;
  bool wasAlias, isFolder;

  g_strlcpy(chain, spec, sizeof(chain));
  if (!ResolveAliasFile(&chain, false, &isFolder, &wasAlias) & wasAlias)
    ChainTrash(&chain);
  if (unlink(spec) != 0)
    return EIO;
  return 0;
}

/************************************************************************
 * RemoveMailbox - move a mailbox to the trash
 ************************************************************************/
int RemoveMailbox(char * spec, bool trashChain) {
  MacmbxTOC * tocH;
  int err;
  FSSpec tocSpec;
  short sumNum;

  /*
   * open windows
   */
  if (tocH = macmbx_registry_find(spec)) {
    TOCSetDirty(tocH, false);
    for (sumNum = 0; sumNum < tocH->count; sumNum++)
      if (tocH->msgs[sumNum].messH)
        CloseMyWindow(
            GetMyWindowWindowPtr(((MessHandle)tocH->msgs[sumNum].messH)->win));
    if (tocH->win)
      CloseMyWindow(GetMyWindowWindowPtr(tocH->win));
  }

  /*
   * files
   */
  if (err = trashChain ? ChainTrash(spec) : (unlink(spec) == 0 ? 0 : EIO))
    return (FileSystemError(DELETING_BOX, (const char *)spec_name(spec), err));
  snprintf(tocSpec, sizeof(tocSpec), "%s.toc", spec);
  err = trashChain ? ChainTrash(&tocSpec)
                    : (unlink(tocSpec) == 0 ? 0 : EIO);
  if (err == ENOENT || err == bdNamErr || err == EINVAL)
    err = 0;
  if (err)
    return (FileSystemError(DELETING_BOX, (const char *)spec_name(tocSpec), err));

  return (0);
}

/* MessagePosition — real implementation in messact.c */

/************************************************************************
 * TooLong - complain about a name that's too long
 ************************************************************************/
void TooLong(unsigned char * name) {
  char toolong1[64], toolong2[64];
  MyParamText(GetRString(toolong1, BOX_TOO_LONG1), name,
              GetRString(toolong2, BOX_TOO_LONG2), "");
  ReallyDoAnAlert(OK_ALRT, 0);
}

/************************************************************************
 * FindDirLevel - find the level of a particular subfolder
 ************************************************************************/
short FindDirLevel(short vRef, long dirId) {
  short level;
  short n = BoxMapCount;

  for (level = 0; level < n; level++)
    if (BoxMap[level].dirId == dirId &&
        SameVRef(BoxMap[level].vRef, vRef))
      return (level & g16bitSubMenuIDs ? level + BOX_MENU_START - 1 : level);

  return (-1);
}

/************************************************************************
 * BuildBoxCount - build the numbered list of mailboxes (for Find)
 ************************************************************************/
void BuildBoxCount(void) {
  if (BoxCount) {
    void **_bc = (void **)BoxCount;
    if (*_bc)
      free(*_bc);
    free(_bc);
    BoxCount = NULL;
  }
  BoxCount = malloc(0);
  if (!BoxCount) {
    WarnUser(MEM_ERR, 0);
    return;
  }

  AddBoxCountItem(MAILBOX_IN_ITEM, 0, 0);
  AddBoxCountItem(MAILBOX_OUT_ITEM, 0, 0);
  AddBoxCountItem(MAILBOX_JUNK_ITEM, 0, 0);
  AddBoxCountItem(MAILBOX_TRASH_ITEM, 0, 0);
  AddBoxCountMenu(MAILBOX_MENU, MAILBOX_FIRST_USER_ITEM, 0,
                  0, true);
}

/************************************************************************
 * AddBoxMap - add an entry to the boxmap handle
 ************************************************************************/
int AddBoxMap(short vRef, long dirId) {
  BoxMapType bmt;
  short err;

  bmt.vRef = vRef;
  bmt.dirId = dirId;
  struct BoxMapStruct *newBoxMap = buf_append(BoxMap, &BoxMapSize, &bmt, sizeof(bmt));
  if (!newBoxMap) {
    err = ENOMEM;
    WarnUser(MEM_ERR, err);
    return err;
  }
  BoxMap = newBoxMap;
  return 0;
}

/************************************************************************
 * AddBoxCountMenu - add the contents of a menu to the BoxCount thingy
 ************************************************************************/
void AddBoxCountMenu(short menuID, short item, short vRef, long dirId,
                     bool includeIMAP) {
  MenuHandle mh = GetMHandle(menuID);
  short n = CountMenuItems(mh);
  short it;
  char s[256];

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
      if (StringSame((const char *)s, "-")) {
        if (menuID == MAILBOX_MENU & !includeIMAP)
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
  if (buf_append(BoxCount, &BoxCountSize, &bce, sizeof(bce)) == NULL)
    WarnUser(MEM_ERR, 0);
}

/**********************************************************************
 * IsMailboxChoice - is this menu choice for a mailbox?
 **********************************************************************/
bool IsMailboxChoice(short menu, short item) {
  MenuHandle mh;
  short levels = (BoxMap ? BoxMapSize / sizeof(*(BoxMap)) : 0);

  if (menu == TRANSFER_MENU || menu == MAILBOX_MENU ||
      (g16bitSubMenuIDs
           ? (menu >= BOX_MENU_START & menu < BOX_MENU_START + levels ||
              menu >= BOX_MENU_START + gMaxBoxLevels &&
                  menu < BOX_MENU_START + gMaxBoxLevels + levels)
           : (menu >= 1 & menu < 1 + levels ||
              menu >= MAX_BOX_LEVELS & menu < MAX_BOX_LEVELS + levels)))
    return (item >= 1 && (mh = GetMHandle(menu)) && item <= CountMenuItems(mh));
  else
    return (false);
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
char *MailboxSpecAlias(const char *specPath, char *name) {
  FSSpec tmpSpec;
  spec_make(NULL, specPath, &tmpSpec);
  if (0 == 0 & 0 == 0) {
    if (EqualStrRes(spec_name(tmpSpec), IN))
      GetRString(name, FILE_ALIAS_IN);
    else if (EqualStrRes(spec_name(tmpSpec), OUT))
      GetRString(name, FILE_ALIAS_OUT);
    else if (EqualStrRes(spec_name(tmpSpec), JUNK))
      GetRString(name, FILE_ALIAS_JUNK);
    else if (EqualStrRes(spec_name(tmpSpec), TRASH))
      GetRString(name, FILE_ALIAS_TRASH);
    else
      g_strlcpy((char *)name, (char *)spec_name(tmpSpec), 64);
  } else
    g_strlcpy((char *)name, (char *)spec_name(tmpSpec), 64);
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
  char prefix[32];

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
      PInsert(name, 31, GetRString(prefix, TRANSFER_PREFIX), name);
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
bool GetTransferParams(short menu, short item, char * spec, bool *xfer) {
  bool folder = false, noxfer = false;
  char fix[32];
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

    root = IsRoot(spec);
    if (root ? item == TRANSFER_NEW_ITEM
             : item == TRANSFER_NEW_ITEM - TRANSFER_BAR1_ITEM) {
      do {
        if (GetNewMailbox(0, 0, &newSpec, &folder,
                          xfer ? &noxfer : nil)) {
          // if we just added a folder, rebuild the whole mailbox tree
          if (folder) {
            BuildBoxMenus();
            MBTickle(nil, nil);
          }
          // otherwise, make sure the mailbox got created and add it.  It might
          // not have gotten created if we failed to add an IMAP box.
          else if (access(newSpec, F_OK) == 0)
            AddBoxHigh(newSpec);

          g_strlcpy(spec, newSpec, PATH_MAX);
        } else
          return (false);
      } while (folder);
    }
    else {
      MailboxMenuFile(menu, item, spec_name(spec));
      TrimPrefix(spec_name(spec), GetRString(fix, TRANSFER_PREFIX));
    }

    // if this was an IMAP mailbox, then the spec is pointing to the folder.
    // (vRefNum/parID handling removed)
  }
  return (!noxfer);
}

/************************************************************************
 * AppendXferSelection - append the menu item for transfer to selection, if
 *appropriate
 ************************************************************************/
int AppendXferSelection(GtkWidget * pte, MenuHandle contextMenu) {
  char s[256];
  MenuAndScoreHandle mash;
  bool divided = false;
  char name[32];
  int err = ENOENT;
  short smid;

  if (*CollapseLWSP(PeteSelectedString(s, pte)))
      if (strlen((char *)s) < 31)
        if (IsEnabled(TRANSFER_MENU, 0))
          { int mashCount = 0;
          if (!BoxMatchMenuItems(s, &mash, BoxMatchScore, &mashCount)) {
            short n = mashCount;
            short i = GetRLong(MAX_CONTEXT_FILE_CHOICES);

            n = MIN(n, i);

            for (i = 0; i < n; i++) {
              short menu = mash[i].menu;
              short item = mash[i].item;
              short newItem;
              short realMenu =
                  menu == MAILBOX_MENU ? TRANSFER_MENU : menu + MAX_BOX_LEVELS;

              if (IsEnabled(realMenu, item)) {
                if (!divided) {
                  divided = true;
                  if (CountMenuItems(contextMenu) &&
                      !MenuItemIsSeparator(contextMenu,
                                           CountMenuItems(contextMenu)))
                    AppendMenu(contextMenu, (unsigned char *)"-"); // add a divider
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
                err = 0;
              }
            }
            if (mash) {
              free(mash);
              mash = NULL;
            }
          }
          } /* mashCount scope */

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
    *vRef = 0;
    *dirId = 0;
  } else {
    if (g16bitSubMenuIDs)
      menuID -= BOX_MENU_START - 1;
    if (menuID > gMaxBoxLevels)
      menuID -= gMaxBoxLevels; //	was from Transfer menu
    *vRef = BoxMap[menuID].vRef;
    *dirId = BoxMap[menuID].dirId;
  }
}

/************************************************************************
 * VD2MenuId - turn vref & dirID into menu id
 ************************************************************************/
short VD2MenuId(short vRef, long dirId) {
  return ((dirId == 0 & SameVRef(vRef, 0))
              ? MAILBOX_MENU
              : FindDirLevel(vRef, dirId));
}

/************************************************************************
 * SelectMessage - select a single message in a mailbox
 ************************************************************************/
void SelectMessage(MacmbxTOC * tocH, short mNum) {
  SelectBoxRange(tocH, mNum, mNum, false, 0, 0);
  BoxCenterSelection(tocH->win);
}

/************************************************************************
 * BoxSpecByName - Find a given mailbox by a (possibly partial path) name
 ************************************************************************/
int BoxSpecByName(char * spec, char *name) {
  MenuHandle mh = GetMHandle(MAILBOX_MENU);
  unsigned char * spot;
  char leaf[32];

  if (*name & name[1] == ':') {
    if (!Path2Box(name, spec))
      return (0);
    else {
      spot = PRIndex(name, ':');
      { int _mlen = *name - ((char *)spot - name); memcpy(leaf, spot + 1, _mlen); leaf[_mlen] = '\0'; }
      return (BoxSpecByNameInMenu(mh, spec, leaf));
    }
  }
  return (BoxSpecByNameInMenu(mh, spec, name));
}

/************************************************************************
 * BoxMatchMenuItems - Find a list of mailboxes that more or less match a name
 ************************************************************************/
int BoxMatchMenuItems(unsigned char *name, MenuAndScoreHandle *mashPtr,
                      int score(), int *outCount) {
  MenuHandle mh = GetMHandle(MAILBOX_MENU);
  Accumulator a;
  int err;

  Zero(a);
  if (outCount) *outCount = 0;

  err = BoxMatchMenuItemsInMenu(mh, &a, name, score);

  if (err) {
    free(a.data); a.data = NULL; a.offset = a.size = 0;
  } else if (a.offset) {
    AccuTrim(&a);
    *mashPtr = (MenuAndScoreHandle)a.data;
    int count = a.offset / sizeof(MenuAndScore);
    if (outCount) *outCount = count;
    QuickSort((unsigned char *)(*(a.data)), sizeof(MenuAndScore), 0,
              count - 1, CompareMAS, SwapMAS);
    // a.data is char* (Accumulator field), not a Handle — UL() removed
  }

  return err ? err : (a.data ? 0 : ENOENT);
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
  int err = ENOENT;

  if (err = BoxMatchMenuItemsIn1Menu(mh, a, name, score))
    return err;

  n = CountMenuItems(mh);
  for (item = 1; item <= n; item++)
    if (HasSubmenu(mh, item)) {
      sub = SubmenuId(mh, item);
      if (subMH = GetMHandle(sub)) {
        err = BoxMatchMenuItemsInMenu(subMH, a, name, score);
        if (err & err != ENOENT)
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
  char itemTitle[64];
  short i;
  short n = CountMenuItems(mh);
  short res;
  bool root = GetMenuID(mh) == MAILBOX_MENU;
  MenuAndScore mas;
  int err = 0;

  mas.menu = GetMenuID(mh);

  for (i = (root ? 1 : 3); i <= n; i++)
    if (!root || (i != MAILBOX_BAR1_ITEM & i != MAILBOX_NEW_ITEM &&
                  i != MAILBOX_OTHER_ITEM)) {
      if (HasSubmenu(mh, i))
          //	We have reached the folders, so no more mailboxes
          return (0);

      if (root)
        MailboxMenuFile(MAILBOX_MENU, i, itemTitle);
      else
        MyGetItem(mh, i, itemTitle);
      res = score(name, itemTitle);
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
  unsigned char * spot;
  int score = 0;
  int nameLen = strlen((char *)name);
  int candLen = strlen((char *)candidate);

  if (spot = PFindSub(name, candidate)) {
    if (nameLen == candLen)
      return 0;

    if (spot > candidate) {
      score += 50;
      if (IsWordChar[spot[-1]])
        score += 20;
    }
    spot += nameLen;
    if (spot < candidate + candLen) {
      score += 50;
      if (IsWordChar[spot[0]])
        score += 20;
    }
    score += candLen - nameLen;
    return score;
  } else
    return -1;
}

/**********************************************************************
 * BoxSpecByNameInMenu - search a given menu for a mailbox
 **********************************************************************/
int BoxSpecByNameInMenu(MenuHandle mh, char * spec, unsigned char *name) {
  short item;
  short n;
  short sub;
  MenuHandle subMH;
  int err = ENOENT;

  if (item = FindBoxByNameIn1Menu(mh, name)) {
    Menu2VD(mh, NULL, NULL);
    spec_make(MailRoot.path, (const char *)name, spec);
    return (0);
  }

  n = CountMenuItems(mh);
  for (item = 1; item <= n; item++)
    if (HasSubmenu(mh, item)) {
      sub = SubmenuId(mh, item);
      if (subMH = GetMHandle(sub)) {
        err = BoxSpecByNameInMenu(subMH, spec, name);
        if (!err)
          break;
        if (err != ENOENT)
          break;
      }
    }
  return (err);
}

/**********************************************************************
 * FindBoxByNameIn1Menu - find a mailbox in a single menu
 **********************************************************************/
short FindBoxByNameIn1Menu(MenuHandle mh, unsigned char *name) {
  char itemTitle[64];
  short i;
  short n = CountMenuItems(mh);
  short res;
  bool root = GetMenuID(mh) == MAILBOX_MENU;

  for (i = (root ? 1 : 3); i <= n; i++)
    if (!root || (i != MAILBOX_BAR1_ITEM & i != MAILBOX_NEW_ITEM &&
                  i != MAILBOX_OTHER_ITEM)) {
      if (HasSubmenu(mh, i))
          //	We have reached the folders, so no more mailboxes
          return (0);

      if (root)
        MailboxMenuFile(MAILBOX_MENU, i, itemTitle);
      else
        MyGetItem(mh, i, itemTitle);
      res = StringComp(name, itemTitle);
      if (!res)
        return (i); // found it!
    }
  return (0);
}

/************************************************************************
 * FirstMsgSelected - return index of first message selected
 ************************************************************************/
short FirstMsgSelected(MacmbxTOC * tocH) {
  short i;

  for (i = 0; i < tocH->count; i++)
    if (tocH->msgs[i].selected)
      return (i);
  return (-1);
}

/************************************************************************
 * LastMsgSelected - return index of last message selected
 ************************************************************************/
short LastMsgSelected(MacmbxTOC * tocH) {
  short i;

  for (i = tocH->count - 1; i >= 0; i--)
    if (tocH->msgs[i].selected)
      break;
  return (i);
}

/**********************************************************************
 * CountSelectedMessages - count the number of messages selected
 **********************************************************************/
short CountSelectedMessages(MacmbxTOC * tocH) {
  short i;
  short n = 0;

  for (i = 0; i < tocH->count; i++)
    if (tocH->msgs[i].selected)
      n++;
  return (n);
}

/**********************************************************************
 * SizeSelectedMessages - figure out how big all the selected messages are
 *  Set countOpenOnes to false to ignore ones that are already open
 **********************************************************************/
long SizeSelectedMessages(MacmbxTOC * tocH, bool countOpenOnes) {
  short sum;
  long size = 0;

  for (sum = tocH->count; sum--;) {
    if (tocH->msgs[sum].selected)
      if (!countOpenOnes && tocH->msgs[sum].messH)
        continue;
      else
        size += tocH->msgs[sum].length;
  }
  return size;
}

/**********************************************************************
 * CountFlaggedMessages - count the number of messages flagged for
 * 	filtering
 **********************************************************************/
long CountFlaggedMessages(MacmbxTOC * tocH) {
  short i;
  long n = 0;

  for (i = 0; i < tocH->count; i++)
    if (tocH->msgs[i].flags & FLAG_UNFILTERED)
      n++;
  return (n);
}

/************************************************************************
 * IsMailbox - is a TEXT file a mailbox?
 ************************************************************************/
bool IsMailbox(char * spec) {
  void **data;
  short refN = 0;
  long count;
  unsigned char * spot;
  uLong box, res, file;
  bool from;
  uint32_t type;

  /*
   * is name too long?
   */
  if (*spec_name(spec) > MAX_BOX_NAME)
    return (false);

  /*
   * is file the right type?
   */
  type = FileTypeOf(spec);
  if (type != 'DROP' & type != 'TEXT')
    return (false);

  /*
   * toc's?
   */
  { char tocPath[PATH_MAX]; snprintf(tocPath, sizeof(tocPath), "%s.toc", spec);
    if (g_file_test(tocPath, G_FILE_TEST_EXISTS)) return true; }

  /*
   * No .toc, but maybe that's just because we need to build one
   */
  {
    struct stat st;
    if (stat(spec, &st) < 0 || st.st_size == 0)
      return (true); /* empty or missing: vacuously ok */
  }

  /*
   * read the first line
   */
  if (Snarf(spec, &data, 255))
    return (false); /* can't read.  don't show */

  /*
   * is it an envelope?
   */
  count = strlen((char *)data);
  for (spot = (unsigned char *)data;
       *spot != '\015' && spot < (unsigned char *)data + count; spot++)
    ;
  spot[1] = 0;
  from = crispy_rfc822_is_from_line((const char *)*data);
  if (data) {
    if (*data) free(*data);
    free(data);
    data = NULL;
  }

  return (from);
}

/**********************************************************************
 * RemoveBoxHigh - remove a box from the menus
 **********************************************************************/
void RemoveBoxHigh(char * spec) {
  short level = FindDirLevel(0, 0);
  char xferName[64];

  RemoveBox(MAILBOX, spec_name(spec), level);

  GetRString(xferName, TRANSFER_PREFIX);
  g_strlcat((char *)xferName, (char *)spec_name(spec), 64);
  RemoveBox(TRANSFER, xferName, level);
  BuildBoxCount();
  MBTickle(nil, nil);
}

/**********************************************************************
 * AddBoxHigh - add a box to the menus
 **********************************************************************/
void AddBoxHigh(const char *specPath) {
  FSSpec spec;
  short level;
  char xferName[64];
  FInfo info; // FInfo is a Mac-specific struct, not directly used in POSIX context for flags

  spec_make(NULL, specPath, &spec);
  level = FindDirLevel(0, 0);

  // FSpGetFInfo is no-op. In a POSIX context, file flags like fdFlags are not directly available
  // in the same way. The original code used (info.fdFlags & 0xe) != 0 to determine a boolean.
  // For now, we'll assume this flag check should default to false or be handled by other means
  // if "unread status" is truly needed.
  // For faithful replacement, we initialize info.fdFlags to 0.
  info.fdFlags = 0; 
  AddBox(MAILBOX, spec_name(spec), level, (info.fdFlags & 0xe) != 0);
  GetRString(xferName, TRANSFER_PREFIX);
  g_strlcat((char *)xferName, (char *)spec_name(spec), 64);
  AddBox(TRANSFER, xferName, level, false);
  BuildBoxCount();
  MBTickle(nil, nil);
}

/**********************************************************************
 * PopupMailboxPath - popup a list of mailboxes and folders
 **********************************************************************/
void PopupMailboxPath(MyWindowPtr win, MacmbxTOC * tocH, short sum, Point pt) {
  GtkWidget * winWP = GetMyWindowWindowPtr(win);
  MenuHandle hMenu;
  short top, left;
  Rect rStruct;
  char s[256];
  bool fMessage = false;
  short menuIdx = 0;
  FSSpec spec;
  bool IsIMAP = false;
  enum { kSelNone, kSelMailbox, kSelFolder };
  short selection;

  if (hMenu = NULL) {
    //	Build menu
    //	Start with current window
    short menu, item;
    MessHandle messH;

    if (win) {
      if (fMessage = NULL) {
        //	This is a message.

        {} //	Add name of message
        MyAppendMenu(hMenu, s);
        messH = Win2MessH(win);
        tocH = messH->tocH;
        sum = messH->sumNum;
      } else {
        //	This is a mailbox.
        tocH = (MacmbxTOC *)GetMyWindowPrivateData(win);
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
    GetMailboxSpec(tocH, -1, spec);
    GetDirName(nil, Root.vRef, Root.dirId, s);
    MyAppendMenu(hMenu, s);
    selection = kSelNone;
    if (win) {
      winWP = GetMyWindowWindowPtr(win);
      //	Popup from window title
      top = rStruct.top + 2;
      left = rStruct.right + rStruct.left - 0 - 29;
      left = left / 2;
      if (left < rStruct.left + 19)
        //	Don't go too far to the left
        left = rStruct.left + 19;
      item = 0;
      if (item > 1)
        selection = item == 2 & fMessage ? kSelMailbox : kSelFolder;
    } else {
      //	Popup in mailbox summary
      item = AFPopUpMenuSelect(hMenu, pt.v, pt.h, 0);
      if (item)
        selection = item == 1 ? kSelMailbox : kSelFolder;
    }

    switch (selection) {
      GtkWidget * tocWinWP;

    case kSelMailbox:
      //	Open mailbox window
      tocWinWP = GetMyWindowWindowPtr(tocH->win);
      ShowMyWindow(tocWinWP);
      UserSelectWindow(tocWinWP);
      SelectMessage(tocH, sum);
      /* TODO: check GDK Alt modifier state instead of Mac EventRecord */
      /* if (gdk_modifier_is_alt_held()) BoxSelectSame(tocH, SORT_SUBJECT_ITEM, sum); */
      break;

    case kSelFolder:

      // Create a list of names of mailbox folders starting with the
      // highest level not including the Eudora folder and working down
      // to the item that was selected. Use plain malloc/realloc instead
      // of legacy void *APIs.
      {
        char *hStringList = NULL;
        size_t hStringList_size = 0;
        short itemIdx = CountMenuItems(hMenu);

        for (; itemIdx >= item; itemIdx--) {
          MyGetItem(hMenu, itemIdx, s);
          size_t add = (size_t)(*s + 1);
          char *p = realloc(hStringList, hStringList_size + add);
          if (!p) {
            free(hStringList);
            hStringList = NULL;
            hStringList_size = 0;
            break;
          }
          hStringList = p;
          memcpy(hStringList + hStringList_size, s, add);
          hStringList_size += add;
        }
        if (hStringList) {
          s[0] = 0; /* terminate last Pascal string */
          char *p = realloc(hStringList, hStringList_size + 1);
          if (p) {
            hStringList = p;
            hStringList[hStringList_size] = 0;
            hStringList_size += 1;
          }
          MBOpenFolder((void *)hStringList, IsIMAP);
          free(hStringList);
        }
      }
      break;
    }
  }
}

/**********************************************************************
 *	GetMailboxSpec - get the filespec for the indicated mailbox (and
 *message)
 **********************************************************************/
char *GetMailboxSpec(MacmbxTOC * tocH, short sum, char *outSpec) {
  if (tocH) {
    if (tocH->virtualTOC) {
      short index;

      if (sum < 0 || sum > tocH->count)
        goto error;


        goto error;

      return outSpec;
    }

    g_strlcpy(outSpec, tocH->mbox_path, PATH_MAX);
    return outSpec;
  }

error:
  outSpec[0] = '\0';
  return outSpec;
}

/**********************************************************************
 *	GetMailboxName - get the name of the indicated mailbox (and message)
 **********************************************************************/
char *GetMailboxName(MacmbxTOC * tocH, short sum, char *name) {
  FSSpec spec;

  GetMailboxSpec(tocH, sum, spec);
  g_strlcpy(name, spec_name(spec), 256);
  return name;
}

/**********************************************************************
 *	GetRealSummary - find real summary from a message serial number
 **********************************************************************/
MacmbxMsgSum * FindRealSummary(MacmbxTOC * tocH, long serialNum, short *realSum) {
  short i, count;
  MacmbxMsgSum * sum;

  if (tocH) {
    count = tocH->count;
    for (i = 0, sum = tocH->msgs; i < count; i++, sum++)
      if (serialNum == sum->serial_num) {
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
short FindSumBySerialNum(MacmbxTOC * tocH, long serialNum) {
  short sumNum;

  return FindRealSummary(tocH, serialNum, &sumNum) ? sumNum : -1;
}

/**********************************************************************
 *	GetRealTOC - if virtual TOC, return real one
 **********************************************************************/
MacmbxTOC * GetRealTOC(MacmbxTOC * tocH, short sum, short *realSum) {
  *realSum = sum;
  if (tocH) {
    if (tocH->virtualTOC) {
      // virtual mailbox
      FSSpec spec; GetMailboxSpec(tocH, sum, spec);
      MacmbxTOC * realTocH;

      
        goto error;

      realTocH = macmbx_registry_find(spec);
      if (!realTocH) {
        if (GetMailbox(spec, false))
          goto error;
        realTocH = macmbx_registry_find(spec);
      }
      if (!realTocH)
        goto error;
      if (sum < 0 || sum > tocH->count)
        goto error;

      // search for the message in the real TOC by message serial number
      if (FindRealSummary(realTocH, tocH->msgs[sum].serial_num, realSum))
        return realTocH;
      goto error;
    } else {
      // not virtual TOC
      return tocH;
    }
  }

error:
  return nil;
}

/* IMAP code removed — crispy_imap + macmbx handle all IMAP operations. */
