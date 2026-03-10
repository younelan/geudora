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

#ifndef WAZOO_H
#define WAZOO_H

/* GTK4 tabbed-window manager — replaces the Mac QuickDraw/ControlHandle wazoo */

#include <gtk/gtk.h>
#include <stdbool.h>
#include "message.h"

#define kMaxTabs 16

/* WazooData: one entry per combined (tabbed) GtkWindow */
typedef struct WazooData {
	int            count;             /* number of tabs (1..kMaxTabs) */
	int            current;           /* active tab index (0..count-1) */
	struct WazooData *next;           /* linked list */
	MyWindowPtr    win;               /* the GtkWindow hosting this notebook */
	GtkWidget     *notebook;          /* GtkNotebook widget */
	short          kinds[kMaxTabs];   /* window kind for each tab */
} WazooData;

extern WazooData *gWazooListHead;

/* Wazoo lifecycle */
void        InitWazoos(void);
void        KillWazoos(void);
void        SetupDefaultWazoos(void);

/* Window opening / switching */
MyWindowPtr GetNewWazoo(short windowKind, bool *fIsWazoo);
bool        SelectOpenWazoo(short windowKind);
MyWindowPtr FindOpenWazoo(short windowKind);

/* Window promotion / demotion */
void        PromoteToWazoo(MyWindowPtr win);
void        DemoteWazoo(MyWindowPtr win);
bool        CloseWazoo(MyWindowPtr win);

/* Update / display */
void        UpdateWazoo(MyWindowPtr win);
void        DirtyWazoo(MyWindowPtr win, short windowKind);
void        WazooPreUpdate(MyWindowPtr win);
void        DidResizeWazoo(MyWindowPtr win);
void        PositionWazoo(MyWindowPtr win);
void        SetTabBackColor(MyWindowPtr win);
bool        WazooHelp(MyWindowPtr win);

/* Queries */
bool        IsWazoo(MyWindowPtr win);
bool        IsLonelyWazoo(MyWindowPtr win);
bool        IsWazooable(MyWindowPtr win);
bool        IsKindWazoo(short windowKind);

/* Window geometry */
void        SetWinMinSize(MyWindowPtr win, short h, short v);

/* Window-kind store/retrieve (implemented here; declared in mailbox.h too) */
void        SetWindowKind(void *winWP, short kind);
void        SetWindowMyWindowPtr(void *winWP, MyWindowPtr win);

#endif /* WAZOO_H */
