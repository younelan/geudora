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

#include <gtk/gtk.h>
#include <stdbool.h>

#ifndef MYWINDOWPTR_DEFINED
#define MYWINDOWPTR_DEFINED
typedef struct MyWindow *MyWindowPtr;
#endif

#include "Globals.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "comp.h"
#include "fileutil.h"
#include "message.h"
#include "stationerywin.h"
#include "toc.h"
#include "util.h"
#include "wazoo.h"

#define FILE_NUM 88
/* Copyright (c) 1996 by QUALCOMM Incorporated */

#pragma segment StationeryWin

// Stationery Window controls
enum { kctlNew = 0, kctlEdit, kctlRemove };

typedef struct {
  GtkWidget *list;
  MyWindowPtr win;
  GtkWidget *ctlEdit, *ctlNew, *ctlRemove;
  bool inited;
  bool dontOpenAfterAll; // Don't open; set when deleting stationery that may
                         // just have been created
} WinData;
static WinData gWin;

/************************************************************************
 * prototypes
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect *oldContR);
static void DoZoomSize(MyWindowPtr win, Rect *zoom);
static void DoUpdate(MyWindowPtr win);
static bool DoClose(MyWindowPtr win);
static void DoClick(MyWindowPtr win, void *event);
static void DoCursor(Point mouse);
static void DoActivate(MyWindowPtr win);
static bool DoKey(MyWindowPtr win, void *event);
static void DoShowHelp(MyWindowPtr win, Point mouse);
static bool DoMenuSelect(MyWindowPtr win, int menu, int item, short modifiers);
static void EditStationery(GtkWidget *widget, gpointer data);
static void DeleteStationery(GtkWidget *widget, gpointer data);
static void NewStationeryFile(GtkWidget *widget, gpointer data);
static void DoGrow(MyWindowPtr win, Point *newSize);
static bool StnyFind(MyWindowPtr win, char * what);

/************************************************************************
 * GTK UI Callbacks
 ************************************************************************/
static void DeleteStationery(GtkWidget *widget, gpointer data) {
  /* GTK Port: TODO implement GtkListStore deletion */
  BuildStationeryList();
}

static void EditStationery(GtkWidget *widget, gpointer data) {
  /* GTK Port: TODO implement Edit Stationery */
}

static void NewStationeryFile(GtkWidget *widget, gpointer data) {
  if (false /* featureStationery always on */)
    return;
  UseFeature(featureStationery);
  /* GTK Port: TODO Implement File Save picker and list population */
}

static void OnStationerySelected(GtkListBox *box, GtkListBoxRow *row,
                                 gpointer user_data) {
  bool fSelect = (row != NULL);
  gWin.win->hasSelection = fSelect;
  if (gWin.ctlRemove)
    gtk_widget_set_sensitive(gWin.ctlRemove, fSelect);
  if (gWin.ctlEdit)
    gtk_widget_set_sensitive(gWin.ctlEdit, fSelect);
}

static void OnStationeryActivated(GtkListBox *box, GtkListBoxRow *row,
                                  gpointer user_data) {
  /* GTK Port: TODO open the stationery */
}

/************************************************************************
 * OpenStationeryWin - open the stationery window
 ************************************************************************/
void OpenStationeryWin(void) {
  if (false /* featureStationery always on */)
    return;

  if (SelectOpenWazoo(STA_WIN))
    return; //	Already opened in a wazoo

  if (!gWin.inited) {
    short err = 0;

    gWin.win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
    if (!gWin.win) {
      err = ENOMEM; /* or proper out of memory err */
      goto fail;
    }
    gWin.win->window = gtk_window_new();

    SetWinMinSize(gWin.win, -1, -1);

    /* GTK handles its own port and theme backgrounds, these are unnecessary */

    // GTK Layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(gWin.win->window), vbox);

    // List
    gWin.list = gtk_list_box_new();
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), gWin.list);
    gtk_box_append(GTK_BOX(vbox), scroll);

    // Header / Buttons layout equivalent
    GtkWidget *bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gWin.ctlNew = gtk_button_new_with_label("New");
    gWin.ctlRemove = gtk_button_new_with_label("Remove");
    gWin.ctlEdit = gtk_button_new_with_label("Edit");

    gtk_widget_set_sensitive(gWin.ctlEdit, FALSE);
    gtk_widget_set_sensitive(gWin.ctlRemove, FALSE);

    g_signal_connect(gWin.ctlNew, "clicked", G_CALLBACK(NewStationeryFile),
                     NULL);
    g_signal_connect(gWin.ctlEdit, "clicked", G_CALLBACK(EditStationery), NULL);
    g_signal_connect(gWin.ctlRemove, "clicked", G_CALLBACK(DeleteStationery),
                     NULL);

    g_signal_connect(gWin.list, "row-selected",
                     G_CALLBACK(OnStationerySelected), NULL);
    g_signal_connect(gWin.list, "row-activated",
                     G_CALLBACK(OnStationeryActivated), NULL);

    gtk_box_append(GTK_BOX(bbox), gWin.ctlNew);
    gtk_box_append(GTK_BOX(bbox), gWin.ctlRemove);
    gtk_box_append(GTK_BOX(bbox), gWin.ctlEdit);
    gtk_box_append(GTK_BOX(vbox), bbox);

    gWin.win->didResize = DoDidResize;
    gWin.win->close = DoClose;
    gWin.win->update = DoUpdate;
    /* gWin.win->position = PositionPrefsTitle; - not applicable */
    gWin.win->click = DoClick;
    gWin.win->bgClick = DoClick;
    gWin.win->dontControl = true;
    gWin.win->cursor = DoCursor;
    gWin.win->activate = DoActivate;
    gWin.win->help = DoShowHelp;
    gWin.win->menu = DoMenuSelect;
    gWin.win->key = DoKey;
    gWin.win->app1 = DoKey;
    gWin.win->zoomSize = DoZoomSize;
    gWin.win->grow = DoGrow;
    gWin.win->find = StnyFind;
    gWin.win->showsSponsorAd = true;
    /* GTK manages resizing naturally, no need to manually call
     * MyWindowDidResize */
    gWin.inited = true;
    return;

  fail:
    if (gWin.win)
      CloseMyWindow(GetMyWindowWindowPtr(gWin.win));
    if (err)
      g_warning("Could not create window: %d", err);
    return;
  }
  gtk_window_present(GTK_WINDOW(gWin.win->window));
}

/************************************************************************
 * DoDidResize - resize the window
 ************************************************************************/
static void DoDidResize(MyWindowPtr win, Rect *oldContR) {
#pragma unused(win)
#pragma unused(oldContR)
  /* GTK handles layout and resizing automatically. */
}

/************************************************************************
 * DoZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void DoZoomSize(MyWindowPtr win, Rect *zoom) {
  /* GTK manages zoom through native window manager controls. */
}

/************************************************************************
 * DoGrow - adjust grow size
 ************************************************************************/
static void DoGrow(MyWindowPtr win, Point *newSize) {
  /* GTK manages window size constraints through geometry hints natively. */
}

/************************************************************************
 * DoClose - close the window
 ************************************************************************/
static bool DoClose(MyWindowPtr win) {
#pragma unused(win)

  if (gWin.inited) {
    // GTK Native: List will be disposed automatically with the parent window
    // unref.
    gWin.inited = false;
  }
  return (true);
}

/************************************************************************
 * DoUpdate - draw the window
 ************************************************************************/
static void DoUpdate(MyWindowPtr win) {
  /* GTK Native: Drawing is done asynchronously by GTK on expose events. */
}

/************************************************************************
 * DoActivate - activate the window
 ************************************************************************/
static void DoActivate(MyWindowPtr win) {
#pragma unused(win)
}

/************************************************************************
 * DoKey - key stroke
 ************************************************************************/
static bool DoKey(MyWindowPtr win, void *event) {
#pragma unused(win)
  /* GTK port: key events are handled by GTK widget signal handlers */
  return false;
}

/************************************************************************
 * DoClick - click in window
 ************************************************************************/
void DoClick(MyWindowPtr win, void *event) {
  /* GTK port: clicks are handled by GTK button callbacks */
  gWin.dontOpenAfterAll = false;
}

/**********************************************************************
 * StnyFind - find in the window
 **********************************************************************/
static bool StnyFind(MyWindowPtr win, char * what) {
  return false; // GTK natively handles Ctrl-F find dialogs in GtkTreeView if
                // set up
}

/************************************************************************
 * DoCursor - set the cursor properly for the window
 ************************************************************************/
static void DoCursor(Point mouse) {
  /* GTK manages cursors based on widget areas automatically */
}

/************************************************************************
 * DoShowHelp - provide help for the window
 ************************************************************************/
static void DoShowHelp(MyWindowPtr win, Point mouse) {
  /* GTK uses tooltip properties on widgets directly */
}

/************************************************************************
 * DoMenuSelect - menu choice in the window
 ************************************************************************/
static bool DoMenuSelect(MyWindowPtr win, int menu, int item, short modifiers) {
#pragma unused(win, modifiers)

  switch (menu) {
  case FILE_MENU:
    switch (item) {
    case FILE_OPENSEL_ITEM:
      EditStationery(NULL, NULL);
      return (true);
      break;
    }
    break;

  case EDIT_MENU:
    switch (item) {
    case EDIT_SELECT_ITEM:
      /* GTK port: handled natively by GtkListBox selection mode */
      break;
    case EDIT_COPY_ITEM:
      /* GTK port: handled natively by GtkTreeView clipboard */
      return true;
    }
    break;
  }
  return (false);
}
