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

#ifndef UTL_H
#define UTL_H

/*______________________________________________________________________

                                utl.h - Utilities Interface.

                                Copyright 1988, 1989, 1990 Northwestern
University.  Permission is granted to use this code in your own projects,
provided you give credit to both John Norstad and Northwestern University in
your about box or document.
_____________________________________________________________________*/

#ifndef __utl__
#define __utl__

#include <gtk/gtk.h>
#include <stdbool.h>

/* FSSpec is defined in mailbox.h as char[PATH_MAX] */


/* ASCII codes. */

#define homeKey 0x01     /* ascii code for home key */
#define enterKey 0x03    /* ascii code for enter key */
#define endKey 0x04      /* ascii code for end key */
#define helpKey 0x05     /* ascii code for help key */
#define deleteKey 0x08   /* ascii code for delete key */
#define tabKey 0x09      /* ascii code for tab key */
#define pageUpKey 0x0B   /* ascii code for page up key */
#define pageDownKey 0x0C /* ascii code for page down key */
#define returnKey 0x0D   /* ascii code for return key */
#define escapeKey 0x1B   /* ascii code for escape key */
#define leftArrow 0x1C   /* ascii code for left arrow key */
#define rightArrow 0x1D  /* ascii code for right arrow key */
#define upArrow 0x1E     /* ascii code for up arrow key */
#define downArrow 0x1F   /* ascii code for down arrow key */

/* System constants. */

#define FCBSPtr 0x34E  /* pointer to file control blocks */
#define MenuList 0xA1C /* handle to menu list */

#define sfNoPrivs -3993         /* rsrc id of standard file alert */
#define sfBadChar -3994         /* rsrc id of standard file alert */
#define sfSysErr -3995          /* rsrc id of standard file alert */
#define sfReplaceExisting -3996 /* rsrc id of standard file alert */
#define sfDiskLocked -3997      /* rsrc id of standard file alert */

/* Type definitions. */

typedef void (*utl_ComputeStdStatePtr)(GtkWidget *theWindow);

typedef void (*utl_ComputeDefStatePtr)(GtkWidget *theWindow, GdkRectangle *userState);

/* Function definitions. */

extern void utl_GetRectGD(GdkRectangle *windRect, GdkMonitor **gd);
extern void utl_CenterDlogRect(GdkRectangle *rect, bool centerMain);
extern void utl_CenterRect(GdkRectangle *rect);
extern bool utl_CheckPack(short packNum, bool preload);
extern void utl_CopyPString(char *dest, char *source);
extern bool utl_CouldDrag(GtkWidget *theWindow, GdkRectangle *windRect, short offset,
                          short titleBarHeight, short leftRimWidth);
extern void utl_FixStdFile(void);
extern void utl_FlashButton(GtkWidget *theDialog, short itemNo);
extern void utl_FrameItem(GtkWidget *theDialog, short itemNo);
extern short utl_GetApplVol(void);
extern bool utl_GetFontNumber(char *fontName, short *fontNum);
extern short utl_GetMBarHeight(void);
extern GtkWidget *utl_GetNewControl(short controlID, GtkWidget *theWindow);
extern GtkWidget *utl_GetNewDialog(short dialogID, void *dStorage,
                                   GtkWidget *behind);
extern GtkWidget *utl_GetNewWindow(short windowID, void *wStorage,
                                   GtkWidget *behind);
extern short utl_GetSysVol(void);
extern void utl_GetWindGD(GtkWidget *theWindow, GdkMonitor **gd, GdkRectangle *screenRect,
                          GdkRectangle *windRect, bool *hasMB);
extern void utl_GetRectGDStuff(GdkMonitor **gd, GdkRectangle *screenRect, GdkRectangle *windRect,
                               short titleBarHeight, short leftRimWidth,
                               bool *hasMB);
extern long utl_GetVolFilCnt(short volRefNum);
extern bool utl_HaveColor(void);
extern void utl_InitSpinCursor(GdkCursor **cursArray, short numCurs,
                               short tickInterval);
extern void utl_InvalGrow(GtkWidget *theWindow);
extern bool utl_IsDAWindow(GtkWidget *theWindow);
extern void utl_PlugParams(char *line1, char *line2, char *p0, char *p1,
                           char *p2, char *p3);
extern void utl_RestoreWindowPos(GtkWidget *theWindow, GdkRectangle *userState,
                                 bool zoomed, short offset,
                                 short titleBarHeight, short leftRimWidth,
                                 utl_ComputeStdStatePtr computeStdState,
                                 utl_ComputeDefStatePtr computeDefState);
extern bool utl_Rom64(void);
extern int utl_RFSanity(const char *spec, bool *sane);
extern void utl_SaveWindowPos(GtkWidget *theWindow, GdkRectangle *userState,
                              bool *zoomed);
extern short utl_ScaleFontSize(short fontNum, short fontSize, short percent,
                               bool laser);
extern void utl_SpinCursor(void);
extern void utl_GetIndGD(short n, GdkMonitor **gd, GdkRectangle *screenRect, bool *hasMB);
extern void utl_StaggerWindow(GtkWidget *theWindow, GdkRectangle *windRect,
                              short initialOffset, short offset,
                              short titleBarHeight, short leftRimWidth,
                              gint *pos, short specificDevice);
extern short utl_StopAlert(short alertID, gboolean (*filterProc)(GtkWidget*, GdkEvent*, gpointer),
                           short cancelItem);
extern bool utl_VolIsMFS(short vRefNum);
extern int GetDropLocationDirectory(GdkDrop *dropLocation, short *volumeID,
                                    long *directoryID);
extern bool DragTargetWasTrash(GdkDrop *dragRef);

#endif

#endif
