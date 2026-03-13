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
 * mywindow.c - GTK port of Eudora's window management
 *
 * Window creation, display, disposal, and utility functions.
 * Window-kind and MyWindowPtr mapping functions live in wazoo.c.
 */

#include "mailbox.h"
#include "message.h"
#include "mydefs.h"
#include "wazoo.h"
#include <gtk/gtk.h>

/* Global window index counter */
static int Windex = 0;

/* Hash table mapping GtkWidget* -> MyWindowPtr for window lookup */
static GHashTable *gWindowMap = NULL;

static void ensure_window_map(void) {
  if (!gWindowMap)
    gWindowMap = g_hash_table_new(g_direct_hash, g_direct_equal);
}

/**********************************************************************
 * GetNewMyWindow - create a new MyWindow backed by a GtkWindow
 **********************************************************************/
MyWindowPtr GetNewMyWindow(short resId, void *wStorage, MyWindowPtr win,
                           void *behind, bool hBar, bool vBar,
                           short windowKind) {
  (void)resId;
  (void)wStorage;
  (void)behind;

  ensure_window_map();

  if (!win) {
    win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
    if (!win)
      return NULL;
  } else {
    memset(win, 0, sizeof(MyWindow));
  }

  GtkWidget *gtkWin = gtk_window_new();
  if (!gtkWin) {
    g_free(win);
    return NULL;
  }

  switch (windowKind) {
  case MBOX_WIN:
  case CBOX_WIN:
    gtk_window_set_default_size(GTK_WINDOW(gtkWin), 640, 400);
    break;
  case COMP_WIN:
    gtk_window_set_default_size(GTK_WINDOW(gtkWin), 600, 500);
    break;
  case MESS_WIN:
    gtk_window_set_default_size(GTK_WINDOW(gtkWin), 600, 450);
    break;
  default:
    gtk_window_set_default_size(GTK_WINDOW(gtkWin), 500, 400);
    break;
  }

  win->window = gtkWin;
  win->isActive = false;
  g_hash_table_insert(gWindowMap, gtkWin, win);

  /* Use the same GObject data keys as wazoo.c */
  SetWindowMyWindowPtr(gtkWin, win);
  SetWindowKind(gtkWin, windowKind);

  Windex++;

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  if (vBar || hBar) {
    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled),
        hBar ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER,
        vBar ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);
    g_object_set_data(G_OBJECT(gtkWin), "scrolled", scrolled);
    gtk_box_append(GTK_BOX(vbox), scrolled);
  }

  gtk_window_set_child(GTK_WINDOW(gtkWin), vbox);
  gtk_widget_set_size_request(gtkWin, 200, 150);

  return win;
}

/**********************************************************************
 * ShowMyWindow - make a window visible
 **********************************************************************/
bool ShowMyWindow(void *winWP) {
  if (!winWP)
    return true;
  if (GTK_IS_WINDOW(winWP)) {
    gtk_window_present(GTK_WINDOW(winWP));
    return false;
  }
  return true;
}

/**********************************************************************
 * ShowMyWindowBehind - show window behind another (just shows it in GTK)
 **********************************************************************/
void ShowMyWindowBehind(void *winWP, void *behindWP) {
  (void)behindWP;
  ShowMyWindow(winWP);
}

/**********************************************************************
 * GetMyWindowWindowPtr - get the GtkWidget from a MyWindow
 **********************************************************************/
WindowPtr GetMyWindowWindowPtr(MyWindowPtr win) {
  if (!win)
    return NULL;
  return win->window;
}

/**********************************************************************
 * UsingWindow / NotUsingWindow - ref counting (GTK GObject handles this)
 **********************************************************************/
void UsingWindow(GtkWidget *win) { (void)win; }
void NotUsingWindow(GtkWidget *win) { (void)win; }

/**********************************************************************
 * MyWindowDidResize - handle window resize
 **********************************************************************/
void MyWindowDidResize(MyWindowPtr win, void *oldContR) {
  if (!win)
    return;
  if (win->window) {
    int w = gtk_widget_get_width(win->window);
    int h = gtk_widget_get_height(win->window);
    win->contR.left = 0;
    win->contR.top = win->topMargin;
    win->contR.right = w;
    win->contR.bottom = h;
  }
  if (win->didResize)
    win->didResize(win, (Rect *)oldContR);
}

/**********************************************************************
 * UpdateMyWindow - redraw a window
 **********************************************************************/
void UpdateMyWindow(void *winWP) {
  if (winWP && GTK_IS_WIDGET(winWP))
    gtk_widget_queue_draw(GTK_WIDGET(winWP));
}

/**********************************************************************
 * MySelectWindow - bring window to front (SelectWindow_ macro maps here)
 **********************************************************************/
void MySelectWindow(void *winWP) {
  if (winWP && GTK_IS_WINDOW(winWP))
    gtk_window_present(GTK_WINDOW(winWP));
}

/**********************************************************************
 * UserSelectWindow - user-initiated window selection
 **********************************************************************/
void UserSelectWindow(void *winWP) {
  if (winWP && GTK_IS_WINDOW(winWP))
    gtk_window_present(GTK_WINDOW(winWP));
}

/**********************************************************************
 * InvalContent - invalidate the content area of a window
 **********************************************************************/
void InvalContent(MyWindowPtr win) {
  if (win && win->window)
    gtk_widget_queue_draw(win->window);
}

/**********************************************************************
 * SetWTitle_ - set window title from Pascal string
 **********************************************************************/
void SetWTitle(void *winWP, unsigned char *title) {
  if (!winWP || !title)
    return;
  int len = title[0];
  char buf[256];
  if (len > 255)
    len = 255;
  memcpy(buf, title + 1, len);
  buf[len] = '\0';
  gtk_window_set_title(GTK_WINDOW(winWP), buf);
}

/**********************************************************************
 * MyDisposeWindow - close and free a window
 **********************************************************************/
void MyDisposeWindow(void *winWP) {
  if (!winWP)
    return;

  ensure_window_map();

  MyWindowPtr win = GetWindowMyWindowPtr(winWP);
  g_hash_table_remove(gWindowMap, winWP);

  if (GTK_IS_WINDOW(winWP))
    gtk_window_destroy(GTK_WINDOW(winWP));

  if (win) {
    win->window = NULL;
    g_free(win);
  }
}

/**********************************************************************
 * ZeroWinFuncs - zero out all callback function pointers
 **********************************************************************/
void ZeroWinFuncs(MyWindowPtr win) {
  if (!win)
    return;
  win->close = NULL;
  win->menu = NULL;
  win->gonnaShow = NULL;
  win->position = NULL;
  win->cursor = NULL;
  win->button = NULL;
  win->app1 = NULL;
  win->find = NULL;
  win->curAddr = NULL;
  win->didResize = NULL;
  win->update = NULL;
  win->click = NULL;
  win->bgClick = NULL;
  win->activate = NULL;
  win->key = NULL;
  win->drag = NULL;
  win->zoomSize = NULL;
  win->grow = NULL;
  win->help = NULL;
  win->idle = NULL;
}

/**********************************************************************
 * IsMyWindow - is this one of our windows?
 **********************************************************************/
bool IsMyWindow(void *winWP) {
  if (!winWP || !gWindowMap)
    return false;
  return g_hash_table_contains(gWindowMap, winWP);
}

/**********************************************************************
 * SetTopMargin / SetBotMargin - set content area margins
 **********************************************************************/
void SetTopMargin(MyWindowPtr win, short h) {
  if (win)
    win->topMargin = h;
}

void SetBotMargin(MyWindowPtr win, short h) {
  (void)win;
  (void)h;
}

/**********************************************************************
 * Window iteration — GTK port of Mac GetWindowList/GetNextWindow
 *
 * Mac used a linked list of WindowRecords. GTK tracks windows via
 * GtkApplication. We snapshot the list on GetWindowList() and walk
 * it with GetNextWindow().
 **********************************************************************/
static GList *gWindowIterList = NULL;
static GList *gWindowIterCurrent = NULL;

void *GetWindowList(void) {
  if (gWindowIterList)
    g_list_free(gWindowIterList);
  gWindowIterList = NULL;
  gWindowIterCurrent = NULL;

  GtkApplication *app = GTK_APPLICATION(g_application_get_default());
  if (!app)
    return NULL;

  /* gtk_application_get_windows returns the list in MRU order (front-to-back),
     which matches the Mac GetWindowList behaviour. We copy the list so
     GetNextWindow can walk it safely even if windows change. */
  GList *appWindows = gtk_application_get_windows(app);
  for (GList *l = appWindows; l; l = l->next) {
    gWindowIterList = g_list_append(gWindowIterList, l->data);
  }

  gWindowIterCurrent = gWindowIterList;
  return gWindowIterCurrent ? gWindowIterCurrent->data : NULL;
}

void *GetNextWindow(void *win) {
  (void)win;
  if (!gWindowIterCurrent)
    return NULL;
  gWindowIterCurrent = gWindowIterCurrent->next;
  return gWindowIterCurrent ? gWindowIterCurrent->data : NULL;
}

/**********************************************************************
 * ReZoomMyWindow - resize window to fill available space
 *
 * Mac: toggled between user-size and "standard" (maximized) size.
 * GTK: maximize the window. If already maximized, restore.
 **********************************************************************/
void ReZoomMyWindow(void *winWP) {
  if (!winWP || !GTK_IS_WINDOW(winWP))
    return;
  GtkWindow *win = GTK_WINDOW(winWP);
  if (gtk_window_is_maximized(win))
    gtk_window_unmaximize(win);
  else
    gtk_window_maximize(win);
}

/**********************************************************************
 * SendBehind - place winWP behind behindWP in the window stack
 *
 * Mac: Carbon Window Manager SendBehind.
 * GTK4/Wayland: compositors control stacking, apps can't force z-order.
 * Best approximation: present the "behind" window to bring it to front,
 * which effectively pushes winWP behind it.
 **********************************************************************/
void SendBehind(void *winWP, void *behindWP) {
  if (behindWP && GTK_IS_WINDOW(behindWP))
    gtk_window_present(GTK_WINDOW(behindWP));
}

/**********************************************************************
 * CloseMyWindow - close and destroy a window
 *
 * Mac: DisposeWindow + cleanup.
 * GTK: destroy the window via gtk_window_destroy.
 **********************************************************************/
bool CloseMyWindow(void *winWP) {
  if (!winWP)
    return false;

  MyWindowPtr myWin = GetWindowMyWindowPtr(winWP);
  if (myWin && myWin->close) {
    /* Let the window's close handler run first (may veto the close) */
    if (!myWin->close(myWin))
      return false;
  }

  MyDisposeWindow(winWP);
  return true;
}
