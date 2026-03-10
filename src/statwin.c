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

#include "statwin.h"
#include "message.h"
#include <gtk/gtk.h>

#define FILE_NUM 132
/* Copyright (c) 2000 by Qualcomm, Inc */

enum { kHeaderHeight = 30 };

typedef struct {
  MyWindowPtr win;
  bool inited;
  GtkWidget *ctlHeader, *ctlTimePeriod, *ctlMore;
  GtkWidget *pte;
  short timePeriod, more;
} WinData;
static WinData gWin;

/************************************************************************
 * prototypes
 ************************************************************************/
static bool StatClose(MyWindowPtr win);
static void StatDidResize(MyWindowPtr win, Rect *oldContR);
static void StatClick(MyWindowPtr win, void *event);
static bool StatMenu(MyWindowPtr win, int menu, int item, short modifiers);
static void StatZoomSize(MyWindowPtr win, Rect *zoom);
static bool StatKey(MyWindowPtr win, void *event);

/************************************************************************
 * OpenStatWin - open statistics window
 ************************************************************************/
void OpenStatWin(void) {
  if (!gWin.inited) {
    gWin.win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
    if (!gWin.win) {
      WarnUser(1, 0);
      return;
    }

    gWin.win->window = gtk_window_new();

    // Layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(gWin.win->window), vbox);

    // Header controls
    GtkWidget *bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gWin.ctlHeader = gtk_label_new("Statistics");
    gWin.ctlTimePeriod =
        gtk_drop_down_new(NULL, NULL); // Placeholder for Time menu
    gWin.ctlMore = gtk_check_button_new_with_label("More Details");

    gtk_box_append(GTK_BOX(bbox), gWin.ctlHeader);
    gtk_box_append(GTK_BOX(bbox), gWin.ctlTimePeriod);
    gtk_box_append(GTK_BOX(bbox), gWin.ctlMore);
    gtk_box_append(GTK_BOX(vbox), bbox);

    // Web view or Text View
    gWin.pte = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gWin.pte), FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), gWin.pte);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(vbox), scroll);

    gWin.win->dontControl = true;
    gWin.win->close = StatClose;
    gWin.win->didResize = StatDidResize;
    gWin.win->click = StatClick;
    gWin.win->bgClick = StatClick;
    gWin.win->menu = StatMenu;
    gWin.win->zoomSize = StatZoomSize;
    gWin.win->key = StatKey;

    gtk_window_present(GTK_WINDOW(gWin.win->window));
    gWin.inited = true;
    RedisplayStats();
    return;
  }

  if (gWin.win && gWin.win->window) {
    gtk_window_present(GTK_WINDOW(gWin.win->window));
  }

  if (gWin.win && gWin.win->window) {
    gtk_window_present(GTK_WINDOW(gWin.win->window));
  }
}

/************************************************************************
 * RedisplayStats - redisplay the stats
 ************************************************************************/
/* GTK Port: Currently implemented via legacy stub in legacy_shim.h */

/************************************************************************
 * StatClose - close the window
 ************************************************************************/
static bool StatClose(MyWindowPtr win) {
  if (gWin.inited) {
    gWin.inited = false;
  }
  return (true);
}

/************************************************************************
 * StatDidResize - resize the window
 ************************************************************************/
static void StatDidResize(MyWindowPtr win, Rect *oldContR) {}

/************************************************************************
 * StatClick - handle a click in the stat window
 ************************************************************************/
static void StatClick(MyWindowPtr win, void *event) {
  /* GTK port: clicks handled by callbacks */
}

/************************************************************************
 * StatKey - handle key stroke for stat window
 ************************************************************************/
static bool StatKey(MyWindowPtr win, void *event) { return false; }

/************************************************************************
 * StatMenu - handle menu choices for stat window
 ************************************************************************/
static bool StatMenu(MyWindowPtr win, int menu, int item, short modifiers) {
  return false;
}

/************************************************************************
 * StatZoomSize - zoom to only the maximum size of list
 ************************************************************************/
static void StatZoomSize(MyWindowPtr win, Rect *zoom) {}
