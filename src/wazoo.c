/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
the limitations in the disclaimer below) provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE. */

/* Copyright (c) 1997 by QUALCOMM Incorporated */
/* wazoo.c – GTK4 tabbed window manager
 *
 * Replaces the Mac QuickDraw / ControlHandle wazoo system with GtkNotebook.
 * Each "wazoo" is a single GtkWindow that hosts a GtkNotebook; each notebook
 * page represents one window kind.
 *
 * This file also supplies the generic window-management helpers that are
 * declared in mailbox.h but have no other natural home:
 *   GetWindowKind / SetWindowKind
 *   GetWindowMyWindowPtr / SetWindowMyWindowPtr
 *   GetMyWindowPrivateData / SetMyWindowPrivateData
 *   MyFrontWindow
 */

#include "wazoo.h"
#include "mydefs.h"   /* WKindEnum — MB_WIN, PH_WIN, … */
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Wazooable window kinds
 * ----------------------------------------------------------------------- */
static const short kWazooKinds[] = {
	MB_WIN, PH_WIN, ALIAS_WIN, FILT_WIN,
	PERS_WIN, STA_WIN, SIG_WIN, LINK_WIN, STAT_WIN,
	0  /* sentinel */
};

/* -----------------------------------------------------------------------
 * Global wazoo list
 * ----------------------------------------------------------------------- */
WazooData *gWazooListHead = NULL;

/* -----------------------------------------------------------------------
 * Generic window-management helpers
 *
 * Window kind and MyWindowPtr are stored as GObject data on the
 * underlying GtkWidget (win->window).
 * ----------------------------------------------------------------------- */

short GetWindowKind(void *winWP)
{
	if (!winWP) return 0;
	return (short)(intptr_t)g_object_get_data(G_OBJECT(winWP), "window-kind");
}

void SetWindowKind(void *winWP, short kind)
{
	if (!winWP) return;
	g_object_set_data(G_OBJECT(winWP), "window-kind",
	                  GINT_TO_POINTER((int)kind));
}

MyWindowPtr GetWindowMyWindowPtr(void *winWP)
{
	if (!winWP) return NULL;
	return (MyWindowPtr)g_object_get_data(G_OBJECT(winWP), "mywindow-ptr");
}

void SetWindowMyWindowPtr(void *winWP, MyWindowPtr win)
{
	if (!winWP) return;
	g_object_set_data(G_OBJECT(winWP), "mywindow-ptr", win);
}

void *GetMyWindowPrivateData(MyWindowPtr win)
{
	return win ? win->privateData : NULL;
}

void SetMyWindowPrivateData(MyWindowPtr win, void *data)
{
	if (win) win->privateData = data;
}

/* MyFrontWindow - return the currently focused GtkWindow widget */
void *MyFrontWindow(void)
{
	/* Iterate all toplevels and return the one with focus */
	GList *toplevels = gtk_window_list_toplevels();
	void *front = NULL;
	for (GList *l = toplevels; l; l = l->next) {
		if (gtk_window_is_active(GTK_WINDOW(l->data))) {
			front = l->data;
			break;
		}
	}
	g_list_free(toplevels);
	return front;
}

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */

static bool IsKindWazooable(short kind)
{
	for (int i = 0; kWazooKinds[i]; i++)
		if (kWazooKinds[i] == kind) return true;
	return false;
}

/* FindWazoo - find the WazooData that contains 'kind', set *idx to its tab */
static WazooData *FindWazoo(short kind, int *idx)
{
	for (WazooData *w = gWazooListHead; w; w = w->next) {
		for (int i = 0; i < w->count; i++) {
			if (w->kinds[i] == kind) {
				if (idx) *idx = i;
				return w;
			}
		}
	}
	return NULL;
}

/* RemoveFromList - unlink a WazooData node (does not free it) */
static void RemoveFromList(WazooData *wd)
{
	if (gWazooListHead == wd) {
		gWazooListHead = wd->next;
		return;
	}
	for (WazooData *w = gWazooListHead; w; w = w->next) {
		if (w->next == wd) {
			w->next = wd->next;
			return;
		}
	}
}

/* -----------------------------------------------------------------------
 * Public wazoo API
 * ----------------------------------------------------------------------- */

void InitWazoos(void)
{
	KillWazoos();
}

void KillWazoos(void)
{
	WazooData *w = gWazooListHead;
	while (w) {
		WazooData *next = w->next;
		if (w->win && w->win->window)
			g_object_set_data(G_OBJECT(w->win->window), "wazoo-data", NULL);
		g_free(w);
		w = next;
	}
	gWazooListHead = NULL;
}

/*
 * GetNewWazoo - called when a window of 'windowKind' is about to be opened.
 *
 * If the kind is already in an open wazoo, return its host MyWindowPtr so the
 * caller can reuse the container (switching the active notebook tab).
 * Returns NULL when a fresh standalone window should be created.
 */
MyWindowPtr GetNewWazoo(short windowKind, bool *fIsWazoo)
{
	int idx;
	WazooData *wd = FindWazoo(windowKind, &idx);
	if (wd) {
		*fIsWazoo = true;
		wd->current = idx;
		if (wd->notebook)
			gtk_notebook_set_current_page(GTK_NOTEBOOK(wd->notebook), idx);
		return wd->win;
	}
	/* Not in any wazoo yet — caller creates a standalone window */
	*fIsWazoo = false;
	return NULL;
}

bool IsWazoo(MyWindowPtr win)
{
	if (!win || !win->window) return false;
	return g_object_get_data(G_OBJECT(win->window), "wazoo-data") != NULL;
}

bool IsLonelyWazoo(MyWindowPtr win)
{
	if (!win || !win->window) return false;
	WazooData *wd =
	    (WazooData *)g_object_get_data(G_OBJECT(win->window), "wazoo-data");
	return wd ? (wd->count <= 1) : false;
}

bool IsWazooable(MyWindowPtr win)
{
	if (!win || !win->window) return false;
	return IsKindWazooable(GetWindowKind(win->window));
}

bool IsKindWazoo(short windowKind)
{
	return FindWazoo(windowKind, NULL) != NULL;
}

MyWindowPtr FindOpenWazoo(short windowKind)
{
	int idx;
	WazooData *wd = FindWazoo(windowKind, &idx);
	return wd ? wd->win : NULL;
}

bool SelectOpenWazoo(short windowKind)
{
	int idx;
	WazooData *wd = FindWazoo(windowKind, &idx);
	if (!wd || !wd->win) return false;
	wd->current = idx;
	if (wd->notebook)
		gtk_notebook_set_current_page(GTK_NOTEBOOK(wd->notebook), idx);
	if (wd->win->window)
		gtk_window_present(GTK_WINDOW(wd->win->window));
	return true;
}

bool CloseWazoo(MyWindowPtr win)
{
	if (!win || !win->window) return false;
	WazooData *wd =
	    (WazooData *)g_object_get_data(G_OBJECT(win->window), "wazoo-data");
	if (!wd) return false;
	RemoveFromList(wd);
	g_object_set_data(G_OBJECT(win->window), "wazoo-data", NULL);
	g_free(wd);
	return true;
}

void PromoteToWazoo(MyWindowPtr win)
{
	if (!win || !win->window) return;
	if (IsWazoo(win)) return;
	short kind = GetWindowKind(win->window);
	if (!IsKindWazooable(kind)) return;

	WazooData *wd = g_new0(WazooData, 1);
	wd->count    = 1;
	wd->current  = 0;
	wd->win      = win;
	wd->kinds[0] = kind;
	wd->notebook = gtk_notebook_new();
	wd->next     = gWazooListHead;
	gWazooListHead = wd;
	g_object_set_data(G_OBJECT(win->window), "wazoo-data", wd);
}

void DemoteWazoo(MyWindowPtr win)
{
	if (!win || !win->window) return;
	WazooData *wd =
	    (WazooData *)g_object_get_data(G_OBJECT(win->window), "wazoo-data");
	if (!wd) return;

	if (wd->count <= 1) {
		CloseWazoo(win);
		return;
	}
	/* Remove the current tab from the notebook */
	if (wd->notebook)
		gtk_notebook_remove_page(GTK_NOTEBOOK(wd->notebook), wd->current);
	for (int i = wd->current; i < wd->count - 1; i++)
		wd->kinds[i] = wd->kinds[i + 1];
	wd->count--;
	if (wd->current >= wd->count) wd->current = wd->count - 1;
	g_object_set_data(G_OBJECT(win->window), "wazoo-data", NULL);
}

void UpdateWazoo(MyWindowPtr win)
{
	if (!win || !win->window) return;
	WazooData *wd =
	    (WazooData *)g_object_get_data(G_OBJECT(win->window), "wazoo-data");
	if (!wd || !wd->notebook) return;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(wd->notebook), wd->current);
}

void DirtyWazoo(MyWindowPtr win, short windowKind)
{
	if (!win || !win->window) return;
	WazooData *wd =
	    (WazooData *)g_object_get_data(G_OBJECT(win->window), "wazoo-data");
	if (!wd || !wd->notebook) return;

	for (int i = 0; i < wd->count; i++) {
		if (wd->kinds[i] != windowKind) continue;
		GtkWidget *page =
		    gtk_notebook_get_nth_page(GTK_NOTEBOOK(wd->notebook), i);
		if (!page) break;
		GtkWidget *label =
		    gtk_notebook_get_tab_label(GTK_NOTEBOOK(wd->notebook), page);
		if (GTK_IS_LABEL(label)) {
			const char *text = gtk_label_get_text(GTK_LABEL(label));
			if (text && text[0] != '*') {
				char *dirty = g_strdup_printf("*%s", text);
				gtk_label_set_text(GTK_LABEL(label), dirty);
				g_free(dirty);
			}
		}
		break;
	}
}

/* Stubs for Mac-era notifications that have no GTK equivalent */

void PositionWazoo(MyWindowPtr win) { (void)win; }
void DidResizeWazoo(MyWindowPtr win) { (void)win; }
void WazooPreUpdate(MyWindowPtr win) { (void)win; }
void SetTabBackColor(MyWindowPtr win) { (void)win; }
bool WazooHelp(MyWindowPtr win) { (void)win; return false; }

void SetupDefaultWazoos(void)
{
	/* Saved wazoo configuration is not yet implemented for GTK.
	 * Windows open standalone; PromoteToWazoo() combines them. */
	InitWazoos();
}

void SetWinMinSize(MyWindowPtr win, short h, short v)
{
	if (!win || !win->window) return;
	gtk_widget_set_size_request(win->window, (int)h, (int)v);
}
