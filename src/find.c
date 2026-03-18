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

#include "find.h"
#define FILE_NUM 14
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/************************************************************************
 * functions for finding text — GTK4 port
 *
 * Original used:
 *   - Mac ControlHandle for find/word/case buttons → GTK4 GtkWidget buttons
 *   - PETEHandle (Pete editor) for query text → GtkWidget* (gEditCtrl)
 *   - FindHandle (void *to FindVars) → FindVars* (malloc'd)
 *   - Pascal strings (Str63, Str255, char *) → C strings (char[])
 *   - Mac window manager (FrontWindow_, GetNextWindow, etc.) → GTK4 windows
 *   - Mac controls (NewControlSmall, SetControlValue, etc.) → GtkWidget
 *   - unsigned char * text -> char* text
 *
 * PETEHandle = GtkWidget* (gEditCtrl text view widget)
 ************************************************************************/

#include "Globals.h"
#include "MyRes.h"
#include "StringUtil.h"
#include "legacy_shim.h"
#include "searchwin.h"
#include "threading.h"
#include <gtk/gtk.h>
#include <string.h>
#include <strings.h>  /* strcasecmp, strncasecmp */
#include <ctype.h>

/************************************************************************
 * FindVars — state for the find system
 *
 * Original: FindHandle FG was a Mac Handle (FindVars**) with:
 *   char what[64] — Pascal string, the search term
 *   MyWindowPtr win — the Find window
 *   bool findDone, finding
 *   short kind — which window kind we're finding in
 *   ControlHandle controls[fcLimit] — Mac controls
 *   PETEHandle queryPTE — Pete editor for query string
 *   Rect qRect
 *   char whereStr[256]
 *
 * GTK4 port: flat struct, malloc'd. C strings. GtkWidget* for controls.
 ************************************************************************/
typedef enum {
    fcFind,     /* Find button */
    fcWord,     /* Whole word checkbox */
    fcCase,     /* Case sensitive checkbox */
    fcFindTxt,  /* "Find:" label */
    fcOptionsTxt, /* "Options:" label */
    fcLimit
} FindControlEnum;

typedef struct {
    char what[256];                /* the string we're finding (was Str63) */
    MyWindowPtr win;               /* our find window, if any */
    bool findDone;                 /* are we done? */
    short kind;                    /* which kind of find? */
    GtkWidget *controls[fcLimit];  /* our controls (was ControlHandle[]) */
    GtkWidget *queryEntry;         /* text entry for search string (was PETEHandle queryPTE) */
    bool finding;                  /* we're actively finding */
    char whereStr[256];            /* description of where we are (was Str255) */
} FindVars;

static FindVars *FG = NULL;

/************************************************************************
 * Private function declarations
 ************************************************************************/
static void FindOpen(void);
static short InitFind(void);
static bool FindClose(MyWindowPtr win);
static void DoFindOK(void);
static void ReportFindFailure(void);
static void DoEnterSelection(void);
static void DoWebFind(void);
static long FindByteOffset(const char *sub, const char *buffer, bool sensitive);
static bool FindInCollapsed(VLNodeInfo *info, MyWindowPtr win,
                            ViewListPtr pView, const char *what);

/* External functions from the search system */
extern void SearchNewFindStringLo(const char *str, bool withPrejudice);
extern MyWindowPtr SearchOpen(int searchMode);
extern void OpenSearchMenu(short item);

/* LV functions — Mac List View API, ported elsewhere */
extern bool LVGetItem(ViewListPtr pView, int row, VLNodeInfo *info, bool b);
extern bool LVSelect(ViewListPtr pView, VLNodeID nodeID,
                      unsigned char *name, bool b);
extern bool MBFindInCollapsed(MyWindowPtr win, ViewListPtr pView,
                               const char *what, short menuID);
extern short MBGetFolderMenuID(VLNodeID nodeID, unsigned char *name);

/* Pete/gEditCtrl compat functions */
extern long PeteFindString(const char *what, long startHere, GtkWidget *pte);
extern void PeteSelect(MyWindowPtr win, GtkWidget *pte, long start, long end);
extern void PeteScroll(GtkWidget *pte, int hScroll, int vScroll);
extern void PeteFocus(MyWindowPtr win, GtkWidget *pte, bool focus);
extern void MessFocus(MessHandle messH, GtkWidget *pte);

/* Scroll constants (from original peteglue.h) */
#ifndef pseNoScroll
#define pseNoScroll 0
#define pseCenterSelection 3
#endif

/* Mac key modifier constants — mapped to GDK equivalents */
#ifndef shiftKey
#define shiftKey GDK_SHIFT_MASK
#endif

/************************************************************************
 * Helper: get text from a gEditCtrl (GtkWidget* text view)
 *
 * Original used PeteSString / PeteSelectedString which operated on
 * Pascal strings. This returns a newly allocated C string.
 ************************************************************************/
static char *GetPTEText(GtkWidget *pte)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;

    if (!pte || !GTK_IS_TEXT_VIEW(pte))
        return g_strdup("");

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
    if (!buf)
        return g_strdup("");

    gtk_text_buffer_get_start_iter(buf, &start);
    gtk_text_buffer_get_end_iter(buf, &end);
    return gtk_text_buffer_get_text(buf, &start, &end, TRUE);
}

/************************************************************************
 * Helper: get selected text from a gEditCtrl
 *
 * Original: PeteSelectedString(s, pte) — filled Pascal string with selection.
 * Returns newly allocated C string, or NULL if no selection.
 ************************************************************************/
static char *GetPTESelectedText(GtkWidget *pte)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;

    if (!pte || !GTK_IS_TEXT_VIEW(pte))
        return NULL;

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
    if (!buf)
        return NULL;

    if (!gtk_text_buffer_get_selection_bounds(buf, &start, &end))
        return NULL;

    return gtk_text_buffer_get_text(buf, &start, &end, TRUE);
}

/************************************************************************
 * Helper: set text in a gEditCtrl
 *
 * Original: PeteSetString(what, pte) set Pascal string in Pete editor.
 ************************************************************************/
static void SetPTEText(GtkWidget *pte, const char *text)
{
    GtkTextBuffer *buf;

    if (!pte || !GTK_IS_TEXT_VIEW(pte))
        return;

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
    if (buf)
        gtk_text_buffer_set_text(buf, text ? text : "", -1);
}

/************************************************************************
 * Helper: select all text in a gEditCtrl
 *
 * Original: PeteSelect(Win, QueryPTE, 0, REAL_BIG) selected all text.
 ************************************************************************/
static void SelectAllPTE(GtkWidget *pte)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;

    if (!pte || !GTK_IS_TEXT_VIEW(pte))
        return;

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
    if (!buf)
        return;

    gtk_text_buffer_get_start_iter(buf, &start);
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_select_range(buf, &start, &end);
}

/************************************************************************
 * FindTopUserWindow - find the topmost (non-find) user window
 *
 * Original: iterated Mac window list via FrontWindow_() / GetNextWindow(),
 * skipping invisible, floating, modal, and FIND_WIN windows.
 * Checked GetWindowKind > userKind.
 *
 * GTK4: iterate the GtkApplication's window list (toplevels).
 * Return the first visible, non-find, non-floating, non-modal window.
 ************************************************************************/
GtkWidget *FindTopUserWindow(void)
{
    /* GTK4 doesn't have a strict Z-ordered window list like Mac's
       FrontWindow_()/GetNextWindow() chain. We get the list of toplevels
       and find the first suitable one. GtkApplication tracks focus. */
    GtkApplication *app = NULL;
    GList *windows, *l;
    GtkWidget *best = NULL;

    /* Get the default GtkApplication if one exists */
    app = GTK_APPLICATION(g_application_get_default());
    if (!app)
        return NULL;

    windows = gtk_application_get_windows(app);
    for (l = windows; l; l = l->next) {
        GtkWindow *w = GTK_WINDOW(l->data);
        GtkWidget *wid = GTK_WIDGET(w);

        if (!gtk_widget_get_visible(wid))
            continue;

        short kind = GetWindowKind(wid);
        if (kind == FIND_WIN)
            continue;

        /* Skip floating/modal — original checked IsFloating() and ModalWindow */
        if (gtk_window_get_modal(w))
            continue;

        /* Original: GetWindowKind > userKind (8). Our window kinds are
           all in the OUR_WIN+ range (MBOX_WIN, MESS_WIN, COMP_WIN, etc.) */
        if (kind >= MBOX_WIN) {
            best = wid;
            break;  /* first match = topmost */
        }
    }

    return best;
}

/************************************************************************
 * EnableFindMenu - do the enabling for the find menu
 *
 * Original: used Mac Menu Manager — GetMHandle, EnableItem, EnableIf,
 * DisableItem, SetMenuItemModifiers for the Special→Find submenu.
 *
 * GTK4: enable/disable GAction entries in the application's action map.
 * The menu items are driven by GActions, not MenuHandle/EnableItem.
 ************************************************************************/
void EnableFindMenu(bool all)
{
    GtkWidget *winWP = FindTopUserWindow();
    MyWindowPtr win = winWP ? (MyWindowPtr)g_object_get_data(G_OBJECT(winWP), "mywindow") : NULL;
    short winKind = winWP ? GetWindowKind(winWP) : 0;
    bool hasMB = (winKind == COMP_WIN || winKind == MESS_WIN ||
                  winKind == MBOX_WIN || winKind == CBOX_WIN);

    /* Original enabled/disabled Mac menu items:
       SPECIAL_FIND_ITEM, FIND_ENTER_ITEM, FIND_AGAIN_ITEM,
       FIND_SEARCH_BOX_ITEM, FIND_SEARCH_FOLDER_ITEM, FIND_SEARCH_WEB_ITEM.

       GTK4: Set the enabled state of corresponding GActions.
       This is a simplified implementation — the full menu system port
       will wire these up properly via GActionMap. */
    GApplication *app = g_application_get_default();
    if (!app)
        return;

    GAction *action;

    action = g_action_map_lookup_action(G_ACTION_MAP(app), "find");
    if (action)
        g_simple_action_set_enabled(G_SIMPLE_ACTION(action), true);

    action = g_action_map_lookup_action(G_ACTION_MAP(app), "find-again");
    if (action)
        g_simple_action_set_enabled(G_SIMPLE_ACTION(action),
                                     all || (FG && FG->what[0]));

    action = g_action_map_lookup_action(G_ACTION_MAP(app), "find-enter-selection");
    if (action)
        g_simple_action_set_enabled(G_SIMPLE_ACTION(action),
                                     all || (win && win->hasSelection));

    action = g_action_map_lookup_action(G_ACTION_MAP(app), "search-mailbox");
    if (action)
        g_simple_action_set_enabled(G_SIMPLE_ACTION(action), all || hasMB);

    action = g_action_map_lookup_action(G_ACTION_MAP(app), "search-web");
    if (action)
        g_simple_action_set_enabled(G_SIMPLE_ACTION(action), true);
}

/************************************************************************
 * DoEnterSelection - enter find selection from top window
 *
 * Original: got top user window, called PeteSelectedString to fill
 * Pascal Str255, then FindEnterSelection.
 *
 * GTK4: get selected text from gEditCtrl, pass as C string.
 ************************************************************************/
static void DoEnterSelection(void)
{
    GtkWidget *winWP = FindTopUserWindow();
    MyWindowPtr win = winWP ? (MyWindowPtr)g_object_get_data(G_OBJECT(winWP), "mywindow") : NULL;

    if (!win || !win->pte || !win->hasSelection)
        return;

    char *sel = GetPTESelectedText(win->pte);
    if (sel && sel[0]) {
        FindEnterSelection(sel, true);
    }
    g_free(sel);
}

/************************************************************************
 * DoFind - handle the user choosing find from the menu
 *
 * Original: switch on menu item (FIND_FIND_ITEM, FIND_ENTER_ITEM,
 * FIND_SEARCH_ITEM, etc). Called FindOpen, DoEnterSelection, SearchOpen,
 * DoWebFind, DoFindOK.
 *
 * Modifier check: shiftKey → also enter selection before action.
 ************************************************************************/
void DoFind(short item, short modifiers)
{
    if (InitFind())
        return;

    if (item > FIND_MENU_LIMIT) {
        OpenSearchMenu(item);
        return;
    }

    switch (item) {
    case FIND_FIND_ITEM:
        if (modifiers & shiftKey)
            DoEnterSelection();
        FindOpen();
        return;
    case FIND_ENTER_ITEM:
        DoEnterSelection();
        return;
    case FIND_SEARCH_ITEM:
    case FIND_SEARCH_ALL_ITEM:
    case FIND_SEARCH_BOX_ITEM:
    case FIND_SEARCH_FOLDER_ITEM:
        if (modifiers & shiftKey)
            DoEnterSelection();
        SearchOpen(item);
        return;
    case FIND_SEARCH_WEB_ITEM:
        DoWebFind();
        return;
    }
    DoFindOK();
}

/************************************************************************
 * DoWebFindWarning - warn the user about what we're going to do
 *
 * Original: checked PREF_SEARCH_WEB_BITS, used PeteSelectedString
 * (Pascal), called ComposeStdAlert.
 *
 * GTK4: use GtkAlertDialog for the warning.
 ************************************************************************/
bool DoWebFindWarning(short menu, short item)
{
    if (menu != FIND_HIER_MENU || item != FIND_SEARCH_WEB_ITEM)
        return true;

    GtkWidget *winWP = FindTopUserWindow();
    MyWindowPtr win = winWP ? (MyWindowPtr)g_object_get_data(G_OBJECT(winWP), "mywindow") : NULL;

    if (!win || !win->pte)
        return true;

    char *sel = GetPTESelectedText(win->pte);
    if (!sel || !sel[0]) {
        g_free(sel);
        return true;
    }

    /* Original showed ComposeStdAlert(Note, SEARCH_TEXT_WARNING) with
       cancel/ok/don't-ask-again buttons. For now, just allow it. */
    g_free(sel);
    return true;
}

/************************************************************************
 * DoWebFind - search for something on the web
 *
 * Original: got selected text via PeteSelectedString (Pascal),
 * called CollapseLWSP, then OpenAdwareURL with the search query.
 *
 * GTK4: get selected text, launch URL in default browser via g_app_info.
 ************************************************************************/
static void DoWebFind(void)
{
    GtkWidget *winWP = FindTopUserWindow();
    MyWindowPtr win = winWP ? (MyWindowPtr)g_object_get_data(G_OBJECT(winWP), "mywindow") : NULL;
    char *sel = NULL;

    if (win && win->pte)
        sel = GetPTESelectedText(win->pte);

    if (sel && sel[0])
        DoWebFindStr(sel);

    g_free(sel);
}

/************************************************************************
 * DoWebFindStr - search for a string on the web
 *
 * Original: OpenAdwareURL(GetNagState(), SEARCH_SITE, actionSearch,
 *           searchQuery, (long)s)
 * This called into the Eudora ad/nag system to build a search URL.
 *
 * GTK4: build a search URL and open in the default browser.
 ************************************************************************/
void DoWebFindStr(const char *s)
{
    if (!s || !s[0])
        return;

    /* URL-encode the search string */
    char *escaped = g_uri_escape_string(s, NULL, TRUE);
    if (!escaped)
        return;

    /* Build search URL — original used Eudora's configured search site.
       Default to a standard search engine. */
    char *url = g_strdup_printf("https://www.google.com/search?q=%s", escaped);

    /* Launch in default browser — replaces OpenAdwareURL */
    GError *error = NULL;
    gtk_show_uri(NULL, url, GDK_CURRENT_TIME);
    (void)error;

    g_free(url);
    g_free(escaped);
}

/************************************************************************
 * Find window callbacks for GTK4
 *
 * Original had FindOpen creating a Mac window with:
 *   GetNewMyWindow(FIND_WIND, ...) — from template resource
 *   PeteCreate for the query text field
 *   NewControlSmall for Find button, Word checkbox, Case checkbox,
 *     "Find:" label, "Options:" label
 *   FindDidResize to lay everything out
 *   Window callbacks: close, position, bgClick, button, key, help, idle
 *
 * GTK4: create a small dialog with GtkEntry + checkboxes + Find button.
 ************************************************************************/

/* Callback: query entry text changed */
static void on_query_changed(GtkEditable *editable, gpointer user_data)
{
    (void)user_data;
    if (!FG)
        return;

    const char *text = gtk_editable_get_text(editable);
    if (text)
        snprintf(FG->what, sizeof(FG->what), "%s", text);
    else
        FG->what[0] = '\0';
}

/* Callback: Find button clicked or Enter pressed */
static void on_find_clicked(GtkWidget *button, gpointer user_data)
{
    (void)button;
    (void)user_data;
    DoFindOK();
}

/* Callback: Case checkbox toggled */
static void on_case_toggled(GtkCheckButton *check, gpointer user_data)
{
    (void)user_data;
    Sensitive = gtk_check_button_get_active(check);
}

/* Callback: Word checkbox toggled */
static void on_word_toggled(GtkCheckButton *check, gpointer user_data)
{
    (void)user_data;
    WholeWord = gtk_check_button_get_active(check);
}

/* Callback: window close request */
static gboolean on_find_close(GtkWindow *window, gpointer user_data)
{
    (void)user_data;
    if (FG)
        FG->win = NULL;
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
    return TRUE;  /* prevent destruction, just hide */
}

/* Callback: Enter key in the text entry activates find */
static void on_entry_activate(GtkEntry *entry, gpointer user_data)
{
    (void)entry;
    (void)user_data;
    DoFindOK();
}

/************************************************************************
 * FindOpen - open the find window
 *
 * Original:
 *   - InitFind to allocate FindVars void **   - GetNewMyWindow(FIND_WIND) from Mac window template resource
 *   - PeteCreate for query text editor
 *   - PETESetCallback for text change notification
 *   - ConfigFontSetup, MySetThemeWindowBackground
 *   - FindCreateControls: NewControlSmall for Find button, Word/Case
 *     checkboxes, Find/Options labels
 *   - FindDidResize to lay everything out
 *   - Set window callbacks: close, position, bgClick, button, idle,
 *     key, help
 *   - g_strlcpy((char *)(what), (char *)(What), sizeof(what)) to set query, PeteSetString to display it
 *   - SetControlValue for Case/Word from prefs
 *   - ShowMyWindow/ShowWindow, UserSelectWindow
 *   - PeteSelect to select all query text
 *
 * GTK4: build a small window with GtkBox layout containing:
 *   - GtkEntry for the search query
 *   - GtkCheckButton for "Case sensitive"
 *   - GtkCheckButton for "Whole word"
 *   - GtkButton "Find"
 ************************************************************************/
static void FindOpen(void)
{
    if (InitFind())
        return;

    if (!FG->win) {
        /* Create the find window */
        GtkWidget *window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(window), "Find");
        gtk_window_set_default_size(GTK_WINDOW(window), 400, -1);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

        /* Main vertical box */
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_margin_start(vbox, 12);
        gtk_widget_set_margin_end(vbox, 12);
        gtk_widget_set_margin_top(vbox, 12);
        gtk_widget_set_margin_bottom(vbox, 12);
        gtk_window_set_child(GTK_WINDOW(window), vbox);

        /* Top row: "Find:" label + entry + Find button */
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(vbox), hbox);

        GtkWidget *label = gtk_label_new("Find:");
        gtk_box_append(GTK_BOX(hbox), label);
        FG->controls[fcFindTxt] = label;

        GtkWidget *entry = gtk_entry_new();
        gtk_widget_set_hexpand(entry, TRUE);
        gtk_box_append(GTK_BOX(hbox), entry);
        FG->queryEntry = entry;

        GtkWidget *findBtn = gtk_button_new_with_label("Find");
        gtk_box_append(GTK_BOX(hbox), findBtn);
        FG->controls[fcFind] = findBtn;

        /* Options row: "Options:" label + checkboxes */
        GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_box_append(GTK_BOX(vbox), hbox2);

        GtkWidget *optLabel = gtk_label_new("Options:");
        gtk_box_append(GTK_BOX(hbox2), optLabel);
        FG->controls[fcOptionsTxt] = optLabel;

        GtkWidget *wordCheck = gtk_check_button_new_with_label("Whole Word");
        gtk_check_button_set_active(GTK_CHECK_BUTTON(wordCheck), WholeWord);
        gtk_box_append(GTK_BOX(hbox2), wordCheck);
        FG->controls[fcWord] = wordCheck;

        GtkWidget *caseCheck = gtk_check_button_new_with_label("Case Sensitive");
        gtk_check_button_set_active(GTK_CHECK_BUTTON(caseCheck), Sensitive);
        gtk_box_append(GTK_BOX(hbox2), caseCheck);
        FG->controls[fcCase] = caseCheck;

        /* Connect signals — replacing Mac's FindButton, FindKey, FindStrChanged */
        g_signal_connect(entry, "changed", G_CALLBACK(on_query_changed), NULL);
        g_signal_connect(entry, "activate", G_CALLBACK(on_entry_activate), NULL);
        g_signal_connect(findBtn, "clicked", G_CALLBACK(on_find_clicked), NULL);
        g_signal_connect(caseCheck, "toggled", G_CALLBACK(on_case_toggled), NULL);
        g_signal_connect(wordCheck, "toggled", G_CALLBACK(on_word_toggled), NULL);
        g_signal_connect(window, "close-request", G_CALLBACK(on_find_close), NULL);

        /* Create a MyWindow wrapper so the rest of the code can reference it */
        MyWindowPtr myWin = (MyWindowPtr)calloc(1, sizeof(MyWindow));
        if (myWin) {
            myWin->window = window;
            myWin->pte = entry;
            myWin->close = FindClose;
            g_object_set_data(G_OBJECT(window), "mywindow", myWin);
        }
        FG->win = myWin;
    }

    /* Set the query text from the current search string */
    if (FG->what[0] && FG->queryEntry)
        gtk_editable_set_text(GTK_EDITABLE(FG->queryEntry), FG->what);

    /* Update checkbox state from global prefs */
    gtk_check_button_set_active(GTK_CHECK_BUTTON(FG->controls[fcCase]), Sensitive);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(FG->controls[fcWord]), WholeWord);

    /* Show and raise the find window */
    if (FG->win && FG->win->window) {
        gtk_widget_set_visible(FG->win->window, TRUE);
        gtk_window_present(GTK_WINDOW(FG->win->window));
    }

    /* Select all text in the entry — original: PeteSelect(Win, QueryPTE, 0, REAL_BIG) */
    if (FG->queryEntry) {
        gtk_editable_select_region(GTK_EDITABLE(FG->queryEntry), 0, -1);
        gtk_widget_grab_focus(FG->queryEntry);
    }
}

/************************************************************************
 * InitFind - get the find stuff ready to go
 *
 * Original: FG = NewZH(FindVars) — allocated a zeroed Mac Handle.
 * GTK4: calloc a flat struct.
 ************************************************************************/
static short InitFind(void)
{
    if (FG)
        return 0;

    FG = (FindVars *)calloc(1, sizeof(FindVars));
    if (!FG)
        return -1;

    return 0;
}

/************************************************************************
 * FindClose - close the find window
 *
 * Original: set Win = nil, returned true to allow close.
 ************************************************************************/
static bool FindClose(MyWindowPtr win)
{
    (void)win;
    if (FG)
        FG->win = NULL;
    return true;
}

/************************************************************************
 * DoFindOK - execute the find operation
 *
 * Original:
 *   - Get top user window via FindTopUserWindow
 *   - If Find window open, read query from PeteSString(s, QueryPTE)
 *   - Copy to What (global search string)
 *   - Set Sensitive/WholeWord from prefs (PREF_SENSITIVE, PREF_FIND_WORD)
 *   - Call win->find(win, s) — the window's find callback
 *   - If not found, ReportFindFailure
 *
 * GTK4: same logic, but C strings and gEditCtrl widgets.
 ************************************************************************/
static void DoFindOK(void)
{
    GtkWidget *winWP = FindTopUserWindow();
    MyWindowPtr win = winWP ? (MyWindowPtr)g_object_get_data(G_OBJECT(winWP), "mywindow") : NULL;

    if (!FG)
        return;

    /* Read current query from the entry widget if the find window is open */
    if (FG->win && FG->queryEntry) {
        const char *text = gtk_editable_get_text(GTK_EDITABLE(FG->queryEntry));
        if (text)
            snprintf(FG->what, sizeof(FG->what), "%s", text);
    }

    if (!FG->what[0] || !win)
        return;  /* nothing to find or no target window */

    FG->findDone = false;

    /* Original: if (PrefIsSet(PREF_SENSITIVE)) Sensitive = true;
       GTK4: Sensitive/WholeWord are already set by checkbox callbacks.
       But if the Find window isn't open, read from prefs. */
    if (!FG->win) {
        Sensitive = PrefIsSet(PREF_SENSITIVE);
        WholeWord = PrefIsSet(PREF_FIND_WORD);
    }

    /* Call the window's find callback — original: (*win->find)(win, s) */
    if (win->find) {
        FG->kind = GetWindowKind(winWP);
        /* Original passed Pascal string; port passes C string.
           win->find takes unsigned char* — cast accordingly. */
        if (!(*win->find)(win, (unsigned char *)FG->what))
            ReportFindFailure();
    }
}

/************************************************************************
 * FindInPTE - find text in a gEditCtrl (PETEHandle = GtkWidget*)
 *
 * Original:
 *   - PeteGetTextAndSelection to get current cursor position (startHere)
 *   - PeteFindString(what, startHere, pte) to search forward
 *   - If not found and startHere > 0, wrap: PeteFindString(what, 0, pte)
 *   - If found:
 *     - ShowMyWindow / UserSelectWindow / UpdateMyWindow
 *     - If MESS_WIN: MessFocus(Win2MessH(win), bodyPTE) to focus body
 *     - Else PeteFocus(win, pte, true)
 *     - Check AttIsSelected for attachment name matches
 *     - PeteSelect to highlight the found text
 *     - PeteScroll to center selection
 *     - Set win->hasSelection, SFWTC
 *
 * GTK4: Use GtkTextBuffer search API (gtk_text_iter_forward_search).
 * The gEditCtrl widget is a GtkTextView.
 ************************************************************************/
bool FindInPTE(MyWindowPtr win, GtkWidget *pte, const char *what)
{
    GtkTextBuffer *buf;
    GtkTextIter startIter, matchStart, matchEnd;
    GtkTextSearchFlags flags;
    bool found = false;
    long selEnd = 0;

    if (!pte || !GTK_IS_TEXT_VIEW(pte) || !what || !what[0])
        return false;

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
    if (!buf)
        return false;

    /* Get current selection end as starting point (original: PeteGetTextAndSelection) */
    GtkTextMark *insertMark = gtk_text_buffer_get_insert(buf);
    gtk_text_buffer_get_iter_at_mark(buf, &startIter, insertMark);
    selEnd = gtk_text_iter_get_offset(&startIter);

    /* Search flags — original checked global Sensitive flag */
    flags = GTK_TEXT_SEARCH_TEXT_ONLY;
    if (!Sensitive)
        flags |= GTK_TEXT_SEARCH_CASE_INSENSITIVE;

    /* Search forward from cursor */
    found = gtk_text_iter_forward_search(&startIter, what, flags,
                                          &matchStart, &matchEnd, NULL);

    /* Wrap around if not found and we didn't start at the beginning */
    if (!found && selEnd > 0) {
        GtkTextIter beginIter;
        gtk_text_buffer_get_start_iter(buf, &beginIter);
        found = gtk_text_iter_forward_search(&beginIter, what, flags,
                                              &matchStart, &matchEnd, NULL);
    }

    if (found) {
        /* Show the window if hidden — original: ShowMyWindow / UserSelectWindow */
        if (win && win->window) {
            gtk_widget_set_visible(win->window, TRUE);
            gtk_window_present(GTK_WINDOW(win->window));
        }

        /* Select the found text — original: PeteSelect(nil, win->pte, offset, offset+*what) */
        gtk_text_buffer_select_range(buf, &matchStart, &matchEnd);

        /* Scroll to selection — original: PeteScroll(win->pte, pseNoScroll, pseCenterSelection) */
        GtkTextMark *mark = gtk_text_buffer_get_insert(buf);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(pte), mark, 0.0, TRUE, 0.0, 0.5);

        if (win) {
            win->hasSelection = true;
            SFWTC = true;
        }

        return true;
    }

    return false;
}

/************************************************************************
 * FindListView - find in a list view
 *
 * Original:
 *   - Started from last selected row + 1
 *   - Iterated rows, checking LVGetItem for each
 *   - Compared name using FindStrStr (case-insensitive substring match)
 *   - For collapsed parent items, called FindInCollapsed to search inside
 *   - If found: LVSelect, ShowMyWindow, SelectWindow_, UpdateMyWindow
 *   - Wrapped around from bottom to top
 *
 * GTK4: ViewList is partially ported. The list view search follows
 * the same row-by-row iteration pattern.
 ************************************************************************/
bool FindListView(MyWindowPtr win, ViewListPtr pView, const char *what)
{
    VLNodeInfo info;
    int startRow = 0;
    bool wrapped = false;
    bool found = false;
    int row;

    if (!pView || !what || !what[0])
        return false;

    /* Original: started from last selected row + 1.
       pView->selectCount and pView->hSelectList tracked selection.
       For now, start from row 0 (simplified — full selection tracking
       requires the ViewList port). */
    /* TODO: get selection state from pView when ViewList is fully ported */

    for (row = startRow; !found && (row != startRow || !wrapped); row++) {
        /* Original: if (row >= hList->dataBounds.bottom) wrap.
           We don't have dataBounds in the GTK port yet, so we rely
           on LVGetItem returning false when past the end. */
        if (!LVGetItem(pView, row + 1, &info, false)) {
            if (wrapped)
                break;
            row = -1;  /* will become 0 on next iteration */
            wrapped = true;
            if (!startRow)
                break;
            continue;
        }

        /* Original: FindStrStr(what, info.name) — case-insensitive substring.
           info.name is unsigned char* (Pascal-ish). Convert to C string match. */
        if (strcasestr((const char *)info.name, what) != NULL) {
            if (LVSelect(pView, info.nodeID, info.name, false))
                found = true;
        }

        if (!found)
            found = FindInCollapsed(&info, win, pView, what);
    }

    if (found && win && win->window) {
        gtk_widget_set_visible(win->window, TRUE);
        gtk_window_present(GTK_WINDOW(win->window));
        SFWTC = true;
    }

    return found;
}

/************************************************************************
 * FindInCollapsed - if it's a collapsed list item, search inside
 *
 * Original: checked info->isParent && info->isCollapsed, then for
 * MB_WIN or search windows, called MBFindInCollapsed.
 ************************************************************************/
static bool FindInCollapsed(VLNodeInfo *info, MyWindowPtr win,
                            ViewListPtr pView, const char *what)
{
    /* Original VLNodeInfo had isParent/isCollapsed booleans.
       Port uses flags field — define bits if not already defined. */
#ifndef kVLNodeParent
#define kVLNodeParent    0x0001
#define kVLNodeCollapsed 0x0002
#endif
    if (!info || !(info->flags & kVLNodeParent) || !(info->flags & kVLNodeCollapsed))
        return false;

    GtkWidget *winWP = win ? win->window : NULL;
    if (!winWP)
        return false;

    short kind = GetWindowKind(winWP);
    if (kind == MB_WIN || IsSearchWindow(winWP))
        return MBFindInCollapsed(win, pView, what,
                                  MBGetFolderMenuID(info->nodeID, info->name));

    return false;
}

/************************************************************************
 * SetFindString - set string used for find and search
 *
 * Original: checked selection wasn't too long or containing returns,
 * then called PeteSelectedString(what, pte) — Pascal string.
 *
 * GTK4: get selection from GtkTextBuffer, check constraints, copy to
 * C string buffer.
 ************************************************************************/
bool SetFindString(char *what, int maxLen, GtkWidget *pte)
{
    GtkTextBuffer *buf;
    GtkTextIter selStart, selEnd;
    char *sel;
    int selLen;

    if (!what || maxLen <= 0) {
        if (what) what[0] = '\0';
        return false;
    }
    what[0] = '\0';

    if (!pte || !GTK_IS_TEXT_VIEW(pte))
        return false;

    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
    if (!buf)
        return false;

    if (!gtk_text_buffer_get_selection_bounds(buf, &selStart, &selEnd))
        return false;

    sel = gtk_text_buffer_get_text(buf, &selStart, &selEnd, TRUE);
    if (!sel)
        return false;

    selLen = (int)strlen(sel);

    /* Original: don't use text if too long or contains a return */
    if (selLen >= maxLen || selLen > 255 || memchr(sel, '\n', selLen) ||
        memchr(sel, '\r', selLen)) {
        g_free(sel);
        return false;
    }

    strncpy(what, sel, maxLen - 1);
    what[maxLen - 1] = '\0';
    g_free(sel);
    return true;
}

/************************************************************************
 * FindEnterSelection - enter a selection into the find system
 *
 * Original: g_strlcpy((char *)(What), (char *)(what), sizeof(What)); if Win, PeteSetString(what, QueryPTE);
 * if searchToo, SearchNewFindString.
 *
 * GTK4: copy C string, set entry widget text.
 ************************************************************************/
void FindEnterSelection(const char *what, bool searchToo)
{
    if (!FG) InitFind();
    if (!FG) return;

    if (what)
        snprintf(FG->what, sizeof(FG->what), "%s", what);
    else
        FG->what[0] = '\0';

    /* If find window is open, update the query entry */
    if (FG->win && FG->queryEntry)
        gtk_editable_set_text(GTK_EDITABLE(FG->queryEntry), FG->what);

    if (searchToo)
        SearchNewFindStringLo(FG->what, false);
}

/************************************************************************
 * GetFindString - get the current find string
 *
 * Original: g_strlcpy((char *)(what), (char *)(What), sizeof(what)) — Pascal copy.
 * GTK4: C string copy.
 ************************************************************************/
bool GetFindString(char *what, int maxLen)
{
    if (!what || maxLen <= 0)
        return false;

    if (FG && FG->what[0]) {
        strncpy(what, FG->what, maxLen - 1);
        what[maxLen - 1] = '\0';
        return true;
    }

    what[0] = '\0';
    return false;
}

/************************************************************************
 * FindSub - find a substring in some text
 *
 * Original: brute force search. Appended sub to void *text (buf_append),
 * null-terminated both, called FindByteOffset, then restored void *size.
 * Used global Sensitive for case sensitivity.
 *
 * GTK4: simple string search on flat buffers. No void *manipulation.
 * sub and text are C strings with explicit lengths.
 ************************************************************************/
long FindSub(const char *sub, long subLen, char *text, long textLen, long offset)
{
    const char *haystack;
    const char *found;

    if (!sub || subLen <= 0 || !text || textLen <= 0 || offset >= textLen)
        return textLen;  /* not found — return end of text like original */

    haystack = text + offset;

    if (Sensitive) {
        found = strstr(haystack, sub);
    } else {
        found = strcasestr(haystack, sub);
    }

    if (found)
        return (long)(found - text);

    return textLen;  /* not found */
}

/************************************************************************
 * FindByteOffset - find a substring in some text (brute force)
 *
 * Original: if Sensitive, exact byte comparison. Else striscmp
 * (case-insensitive prefix match). Both strings null-terminated.
 * Sub MUST be in buffer.
 *
 * GTK4: strstr / strcasestr. Kept as internal helper.
 ************************************************************************/
static long FindByteOffset(const char *sub, const char *buffer, bool sensitive)
{
    const char *found;

    if (sensitive)
        found = strstr(buffer, sub);
    else
        found = strcasestr(buffer, sub);

    if (found)
        return (long)(found - buffer);

    return -1;
}

/************************************************************************
 * ReportFindFailure - alert the user that find failed
 *
 * Original: if CommandPeriod return. If PREF_NO_NOT_FOUND_ALERT,
 * SysBeep. Else AlertStr(NOT_FOUND_ALRT, Note, what).
 *
 * GTK4: show an info bar or dialog, or just beep.
 ************************************************************************/
static void ReportFindFailure(void)
{
    if (CommandPeriod)
        return;

    if (!FG)
        return;

    /* Original: if PrefIsSet(PREF_NO_NOT_FOUND_ALERT) gdk_display_beep(gdk_display_get_default());
       else AlertStr(NOT_FOUND_ALRT, Note, what); */
    if (PrefIsSet(PREF_NO_NOT_FOUND_ALERT)) {
        gdk_display_beep(gdk_display_get_default());
    } else {
        /* Show a simple alert dialog */
        GtkWidget *winWP = FindTopUserWindow();
        GtkWindow *parent = winWP ? GTK_WINDOW(winWP) : NULL;
        if (!parent && FG->win && FG->win->window)
            parent = GTK_WINDOW(FG->win->window);

        GtkAlertDialog *alert = gtk_alert_dialog_new("Not found: \"%s\"", FG->what);
        gtk_alert_dialog_show(alert, parent);
        g_object_unref(alert);
    }
}
