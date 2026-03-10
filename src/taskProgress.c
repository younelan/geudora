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
        File: taskProgress.c
        Author: Clarence Wong <cwong@qualcomm.com>
        Date: Augutst 1997 - ...
        Copyright (c) 1997 by QUALCOMM Incorporated

        Comments:
                Much of this code is borrowed from Alan Bird's stationerywin.c
   and Steve Dorner's progress.c
*/

#include "taskProgress.h"
#include "Globals.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "fileutil.h"
#include "mailbox.h"
#include "message.h"
#include "mydefs.h"
#include "portable-compat.h"
#include "progress.h"
#include "schizo.h"
#include "log.h"
#include "task_ldef.h"
#include "util.h"
#include <gtk/gtk.h>

#ifdef CommandPeriod
#undef CommandPeriod
#endif
extern bool CommandPeriod;
#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif


// Task Progress controls
enum {
  kNetworkPopup = 0,
  kFilterButton,
  kStopButton,
  kHelpButton,
};

#define FILE_NUM 104

static void *FilterButton G_GNUC_UNUSED = NULL;

/************************************************************************
 * prototypes
 ************************************************************************/
static void TPDoDidResize(struct MyWindow *win, struct Rect *oldContR) G_GNUC_UNUSED;
static void TPDoZoomSize(struct MyWindow *win, struct Rect *zoom) G_GNUC_UNUSED;
static void TPDoUpdate(struct MyWindow *win) G_GNUC_UNUSED;
static void TPDoActivate(struct MyWindow *win) G_GNUC_UNUSED;
static bool TPDoClose(struct MyWindow *win) G_GNUC_UNUSED;
static void TPDoButton(struct MyWindow *win, void *button, long modifiers,
                       short part) G_GNUC_UNUSED;
static void TPDoClick(struct MyWindow *win, struct EventRecord *event) G_GNUC_UNUSED;
static void TPDoCursor(struct Point mouse) G_GNUC_UNUSED;
static bool TPDoKey(struct MyWindow *win, struct EventRecord *event) G_GNUC_UNUSED;
static void TPDoShowHelp(struct MyWindow *win, struct Point mouse) G_GNUC_UNUSED;
static bool TPPosition(bool save, struct MyWindow *win) G_GNUC_UNUSED;
static void TPIdle(struct MyWindow *win) G_GNUC_UNUSED;

unsigned char *GetNextCheckTime(unsigned char *item);
unsigned char *GetLastCheckTime(unsigned char *item);
static void DrawTPProgress(struct MyWindow *win, GtkWidget *row, cairo_t *cr,
                           void **data);
static void DrawTPFilter(struct MyWindow *win, GtkWidget *row, cairo_t *cr,
                         void **data) G_GNUC_UNUSED;
static void DrawTPError(struct MyWindow *win, GtkWidget *row, cairo_t *cr,
                        void **data);
static void PositionAllCells(void) G_GNUC_UNUSED;
static void SetTPWTitle(void);
static void FooterUserPaneBackground(void *control, void *info) G_GNUC_UNUSED;
static void SetWindowBackground(void *control, void *info) G_GNUC_UNUSED;
void UpdateTaskProgress(int a, int b);
int TPAddHelpButton(taskErrHandle taskErrs);

#define kListInset 10
#define kTextInset 10
#define kCellIndent 4
#define PROG_BAR_WI 100
#define PROG_TOTAL_HI ((PROG_BOX_HI * 3) + INSET + (2 * kCellIndent))
#define kRemainWidth 40
#define kLastMailCheckWidth 140
#define kNextMailCheckWidth 145
#define kNextTimeRefCon 'tpnx'
#define kLastTimeRefCon 'tpls'
#define kFooterRefCon 'tpft'
#define kNetworkRefCon 'tpnc'
#define kListPaneRefCon 'tplp'
#define kNetworkWidth 24
#define kNetworkHeight 20
#define PROG_CONT_WIDTH_VAL 300
#define PROG_WI (PROG_CONT_WIDTH_VAL + (2 * kListInset))

ListHandle TaskListHandle = NULL;
taskErrHandle TaskErrorList = NULL;

/************************************************************************
 * InitPrbl
 ************************************************************************/
void InitPrbl(ProgressBlock **prbl, short vert, void **stopButton) {
  if (TaskProgressWindow) {
    short stopWidth = 16;
    struct MyWindow *win = TaskProgressWindow;
    struct Rect contR;

    /* LDRef(prbl); */

    /* GTK Port: We skip Mac Control creation for now and just set up rects */

    contR = win->contR;
    /* Simulate InsetRect(contR, 10, 10) */
    contR.left += 10;
    contR.top += 10;
    contR.right -= 10;
    contR.bottom -= 10;

    // title && progbar
    // kpTitle
    (*prbl)->rects[0].left = contR.left + kListInset;
    (*prbl)->rects[0].top = vert;
    (*prbl)->rects[0].right = PROG_WI - PROG_BAR_WI - 10 - kListInset - 16;
    (*prbl)->rects[0].bottom = vert + 20;

    // kpBar
    (*prbl)->rects[1].left = PROG_WI - PROG_BAR_WI - 10 - 16 - kListInset;
    (*prbl)->rects[1].top = vert;
    (*prbl)->rects[1].right = PROG_WI - 10 - 16 - kListInset;
    (*prbl)->rects[1].bottom = vert + 20;

    vert += 20;

    // subtitle and remaining counter
    (*prbl)->rects[2].left = contR.left + 10 + 10;
    (*prbl)->rects[2].top = vert;
    (*prbl)->rects[2].right = PROG_WI - stopWidth - 16 - 10 - 40 - 10;
    (*prbl)->rects[2].bottom = vert + 20;

    (*prbl)->rects[3].left = (*prbl)->rects[2].right + 4;
    (*prbl)->rects[3].top = vert;
    (*prbl)->rects[3].right = PROG_WI - stopWidth - 16 - 10 - 10;
    (*prbl)->rects[3].bottom = vert + 20;

    vert += 20;

    // message
    (*prbl)->rects[4].left = contR.left + 10 + 10;
    (*prbl)->rects[4].top = vert;
    (*prbl)->rects[4].right = PROG_WI - stopWidth - 16 - 10 - 10;
    (*prbl)->rects[4].bottom = vert + 20;

    /* UL(prbl); */
  }
}

/************************************************************************
 * InvalTPRect -
 ************************************************************************/
void InvalTPRect(struct Rect *invalRect) {
  /* GTK port: Invalidation is handled by GTK widgets. */
}

void DrawTaskProgressBar(void *bar) {
  /* GTK port: Drawing is handled by Cairo in the DrawTPProgress callback. */
}

/************************************************************************
 * DrawTP -
 ************************************************************************/
static void DrawTPProgress(struct MyWindow *win, GtkWidget *row, cairo_t *cr,
                           void **data) {
  ProgressBlock ***prbl;
  int which;
  struct threadData_ ***threadData;
  char cMesg[256];

  threadData = (struct threadData_ ***)data;
  /* ASSERT(threadData); */
  if (!threadData)
    return;

  if ((prbl = &(**threadData)->prbl)) {
    /* LDRef(threadData); */
    /* Note: InitPrbl sets up the rects. In GTK/Cairo, we draw relative to row
     * (0,0) */
    InitPrbl(*prbl, 0, &(**threadData)->stopButton);
    /* UL(threadData); */
    /* LDRef(prbl); */

    cairo_set_source_rgb(cr, 0, 0, 0); // Black text

    for (which = 0; which <= 0; which++) {
      struct Rect r;
      memcpy(&r, &(**prbl)->rects[which], sizeof(struct Rect));

      /* Convert Pascal string to C string for Cairo */
      unsigned char *pStr = (**prbl)->messages[which];
      int len = pStr ? pStr[0] : 0;
      if (len > 255)
        len = 255;
      memcpy(cMesg, pStr + 1, len);
      cMesg[len] = '\0';

      cairo_move_to(cr, r.left, r.bottom - 4);
      if (which == 0) {
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_BOLD);
      } else {
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
      }
      cairo_set_font_size(cr, 11);
      cairo_show_text(cr, cMesg);
    }

    /* Draw the progress bar using Cairo */
    if ((**prbl)->bar) {
      struct Rect br;
      memcpy(&br, &(**prbl)->rects[1], sizeof(struct Rect));
      /* Simple Cairo progress bar background */
      cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
      cairo_rectangle(cr, br.left, br.top, br.right - br.left,
                      br.bottom - br.top);
      cairo_fill(cr);

      /* Border */
      cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
      cairo_set_line_width(cr, 1.0);
      cairo_rectangle(cr, br.left + 0.5, br.top + 0.5, br.right - br.left - 1,
                      br.bottom - br.top - 1);
      cairo_stroke(cr);

      /* Progress fill - for now we just draw a partial blue bar */
      cairo_set_source_rgb(cr, 0.2, 0.4, 0.8);
      cairo_rectangle(cr, br.left + 1, br.top + 1,
                      (br.right - br.left - 2) * 0.5, br.bottom - br.top - 2);
      cairo_fill(cr);
    }
    /* UL(prbl); */
  }
}

#ifndef BATCH_DELIVERY_ON
/************************************************************************
 * DrawTPFilter -
 ************************************************************************/
static void DrawTPFilter(struct MyWindow *win, GtkWidget *row, cairo_t *cr,
                         void **data) {
  /* GTK port: Drawing Filter task summary using Cairo */
  char cMesg[256];
  char cNum[32];
  int y = 20;

  cairo_set_source_rgb(cr, 0, 0, 0); // Black text

  /* Title: Waiting to deliver */
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 11);
  cairo_move_to(cr, kTextInset + kListInset, y);
  cairo_show_text(cr, "Waiting to deliver...");

  y += 20;
  /* Status: Left to deliver */
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_move_to(cr, kTextInset + 10 + kListInset, y);
  snprintf(cMesg, sizeof(cMesg), "Left to deliver: %d", TempInCount);
  cairo_show_text(cr, cMesg);

  y += 20;
  cairo_move_to(cr, kTextInset + 10 + kListInset, y);
  cairo_show_text(cr, "Click to deliver");
}

#endif

/************************************************************************
 * DrawTPError -
 ************************************************************************/
static void DrawTPError(struct MyWindow *win, GtkWidget *row, cairo_t *cr,
                        void **data) {
  taskErrHandle taskErr = (taskErrHandle)data;
  char cMesg[256];

  if (taskErr && *taskErr) {
    cairo_set_source_rgb(cr, 0.8, 0, 0); // Error Red
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 11);
    cairo_move_to(cr, kTextInset + kListInset, 20);

    /* Convert taskDesc to string */
    unsigned char *pStr = (*taskErr)->taskDesc;
    int len = pStr ? pStr[0] : 0;
    if (len > 255)
      len = 255;
    memcpy(cMesg, pStr + 1, len);
    cMesg[len] = '\0';

    cairo_show_text(cr, cMesg);
  }
}
static void PositionAllCells(void) {
  /* GTK port: Row positioning is handled by GtkListBox. */
}

static void FooterUserPaneBackground(void *control, void *info) {}
static void SetWindowBackground(void *control, void *info) {}

void OpenTasksWin(void) {
  OpenTasksWinBehind(BehindModal);
  NewError = false;
}
void OpenTasksWinBehind(void *behind) {
  MyWindowPtr win;
  GtkWidget *gtkWin;
  GtkWidget *vbox;
  GtkWidget *scroll;
  GtkWidget *listBox;

  if (TaskProgressWindow) {
    if (GTK_IS_WIDGET(TaskProgressWindow->window)) {
      gtk_window_present(GTK_WINDOW(TaskProgressWindow->window));
    }
    return;
  }

  /* Create the wrapper structure */
  win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
  TaskProgressWindow = win;

  /* Create the GTK window */
  gtkWin = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(gtkWin), "Tasks");
  gtk_window_set_default_size(GTK_WINDOW(gtkWin), 400, 300);
  win->window = gtkWin;

  /* Use object data to link GTK widget back to MyWindow wrapper */
  g_object_set_data(G_OBJECT(gtkWin), "my-window-ptr", win);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(gtkWin), vbox);

  scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_box_append(GTK_BOX(vbox), scroll);

  listBox = gtk_list_box_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listBox);
  TaskListHandle = listBox;

  gtk_window_present(GTK_WINDOW(gtkWin));

  /* Update widgets */
  /* Update widgets */
  if (InAThread())
    ProgWindow = TaskProgressWindow;

  UpdateTaskProgress(0, 0);
}
/************************************************************************
 * TPAddHelpButton - add the help button to an error string
 ************************************************************************/
int TPAddHelpButton(taskErrHandle taskErrs) {
  /* GTK Port: Stub for help button */
  (void)taskErrs;
  return 0;
}

/************************************************************************
 * OpenTaskWinErrors -
 ************************************************************************/
void OpenTasksWinErrors(void) {
  Point pt;
  pt.h = pt.v = -1;

  OpenTasksWin();
  if (TaskProgressWindow) {
    void *TaskProgressWindowWP = GetMyWindowWindowPtr(TaskProgressWindow);
    /* CollapseWindow(TaskProgressWindowWP, false); */
    UpdateMyWindow(TaskProgressWindowWP);
  }
  if (!InBG) {
    /* SysBeep(1); */
  }
  // else nm???
  //	NewError = false;
}

/************************************************************************
 * GetNextCheckTime -
 ************************************************************************/
unsigned char *GetNextCheckTime(unsigned char *item) {
  uLong checkTicks = PersCheckTicks();

  item[0] = 0;
  if (checkTicks) {
    if (!AutoCheckOK()) {
      /* GetRString(item, TP_NEVER); */
      strcpy((char *)item + 1, "Never");
      item[0] = strlen("Never");
    } else {
      checkTicks = MAX(checkTicks, TickCount() + 3600);
      TimeString((LocalDateTime() - TickCount() / 60) + checkTicks / 60, False,
                 item, NULL);
    }
  } else {
    /* GetRString(item, TP_NOT_SCHEDULED); */
    strcpy((char *)item + 1, "Not scheduled");
    item[0] = strlen("Not scheduled");
  }
  return (item);
}

/************************************************************************
 * GetLastCheckTime -
 ************************************************************************/
unsigned char *GetLastCheckTime(unsigned char *item) {
  item[0] = 0;
  if (CheckThreadRunning) {
    /* GetRString(item, TP_CHECKING_NOW); */
    strcpy((char *)item + 1, "Checking...");
    item[0] = strlen("Checking...");
  } else if (LastCheckTime)
    TimeString(LastCheckTime, False, item, NULL);
  else {
    /* GetRString(item, TP_NEVER); */
    strcpy((char *)item + 1, "Never");
    item[0] = strlen("Never");
  }
  return (item);
}

static void TPDoUpdate(MyWindowPtr win) {
  /* GTK port: Drawing is handled by widgets and Cairo callbacks. */
}

static void TPDoActivate(MyWindowPtr win) {
  /* GTK port: Activation handled by GTK. */
}

void TaskProgressRefresh(void) {
  /* GTK port: Refreshing UI elements if needed. */
}

/************************************************************************
 * TPDoDidResize - resize the window
 ************************************************************************/
static void TPDoDidResize(MyWindowPtr win, Rect *oldContR) {
  /* GTK port: Resizing handled by GTK layout. */
}

static void TPDoZoomSize(MyWindowPtr win, Rect *zoom) {
  /* GTK port: Zooming handled by GTK. */
}

static bool TPDoClose(MyWindowPtr win) {
  /* GTK port: Cleanup for GTK widgets. */
  TaskProgressWindow = nil;
  TaskListHandle = nil;
  gTaskProgressInitied = false;
  return True;
}

static bool TPDoKey(MyWindowPtr win, EventRecord *event) {
  /* GTK port: Keyboard handling via GTK signals. */
  return false;
}

static bool TPPosition(bool save, MyWindowPtr win) { return true; }

static void TPDoButton(MyWindowPtr win, ControlHandle button, long modifiers,
                       short part) {
  /* GTK port: Button clicks handled by GTK signals. */
}

/************************************************************************
 * TPDoClick
 ************************************************************************/
static void TPDoClick(struct MyWindow *win, struct EventRecord *event) {
  /* GTK Port: Clicking handled by GTK signals */
  (void)win;
  (void)event;
}

/************************************************************************
 * TPDoCursor - set the cursor properly for the window
 ************************************************************************/
static void TPDoCursor(Point mouse) {
  /* GTK port: cursor management via GTK.
   * Pete's pteList cursor tracking replaced by GTK widget cursor API. */
  if (TaskProgressWindow && GTK_IS_WIDGET(TaskProgressWindow->window)) {
    gtk_widget_set_cursor_from_name(GTK_WIDGET(TaskProgressWindow->window),
                                    "default");
  }
}

/************************************************************************
 * TPDoShowHelp - provide help for the window
 ************************************************************************/
static void TPDoShowHelp(MyWindowPtr win, Point mouse) {
#pragma unused(win, mouse)
}

/************************************************************************
 * AddProgressTask -
 ************************************************************************/
OSErr AddProgressTask(threadDataHandle threadData) {
  OSErr err = noErr;
  if (TaskProgressWindow) {
    err =
        AddListItemEntry(0, DrawTPProgress, (Handle)threadData, TaskListHandle);
    SetTPWTitle();
  }
  return err;
}

/************************************************************************
 * RemoveProgressTask -
 ************************************************************************/
void RemoveProgressTask(threadDataHandle threadData) {
  if (TaskProgressWindow) {
    RemoveListItemEntry((Handle)threadData, TaskListHandle);
    SetTPWTitle();
  }
}

#ifndef BATCH_DELIVERY_ON
/************************************************************************
 * AddFilterTask -
 ************************************************************************/
OSErr AddFilterTask(void) {
  if (TaskProgressWindow && NeedToFilterIn) {
    RemoveListItemEntry((Handle)FilterButton, TaskListHandle); // just in case
    return (AddListItemEntry(SendThreadRunning, DrawTPFilter,
                             (Handle)FilterButton, TaskListHandle));
  }
  return noErr;
}

/************************************************************************
 * RemoveFilterTask -
 ************************************************************************/
void RemoveFilterTask(void) {
  if (TaskProgressWindow) {
    RemoveListItemEntry((Handle)FilterButton, TaskListHandle);
    HideControl(FilterButton);
  }
}
#endif

/************************************************************************
 * AddTaskErrorsString
 ************************************************************************/
OSErr AddTaskErrorsS(const char *error, const char *explanation,
                     TaskKindEnum taskKind, long persId) {
  OSErr err = noErr;
  taskErrHandle taskErrs;
  Str255 name;
  PersHandle pers;

  RemoveTaskErrors(taskKind, persId);
  ComposeLogS(LOG_ALRT, nil, (unsigned char *)"%p %p", error, explanation);
  NewError = true;
  if (!(taskErrs = NuHandleClear(sizeof(taskErrData)))) {
    err = MemError();
    CurThreadGlobals->tCommandPeriod = true;
    if (taskKind == CheckingTask)
      CheckThreadError = err;
    else if (taskKind == SendingTask)
      SendThreadError = err;
    return err;
  }
  (*taskErrs)->taskKind = taskKind;
  (*taskErrs)->persId = persId;

  CtoPCpy((*taskErrs)->errMess, error);
  if ((*taskErrs)->errMess[0] == 255)
    (*taskErrs)->errMess[255] = 0xFF;

  CtoPCpy((*taskErrs)->errExplanation, explanation);
  if ((*taskErrs)->errExplanation[0] == 255)
    (*taskErrs)->errExplanation[255] = 0xFF;

  for (pers = PersList; pers; pers = (*pers)->next)
    if ((*pers)->persId == persId)
      break;
  if (pers)
    CtoPCpy(name, (*pers)->name);
  else
    GetRString(name, TP_UNKNOWN_PERS);
  LDRef(taskErrs);
  switch (taskKind) {
  case CheckingTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_CHECKING_MAIL, name);
    break;
  case SendingTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_SENDING_MAIL, name);
    break;
  case IMAPResyncTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_RESYNCING, name);
    break;
  case IMAPFetchingTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_FETCHING, name);
    break;
  case IMAPDeleteTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_DELETING, name);
    break;
  case IMAPUndeleteTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_UNDELETING, name);
    break;
  case IMAPTransferTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_TRANSFERRING, name);
    break;
  case IMAPExpungeTask:
  case IMAPMultExpungeTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_EXPUNGING, name);
    break;
  case IMAPAttachmentFetch:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_ATTACHMENT_FETCH, name);
    break;
  case IMAPSearchTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_SEARCH, name);
    break;
  case IMAPMultResyncTask:
  case IMAPPollingTask:
    GetRString((*taskErrs)->taskDesc, ERR_MULT_RESYNCING);
    break;
  case IMAPUploadTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_TRANSFERRING, name);
    break;
  case IMAPAlertTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_IMAP_ALERT, name);
    break;
  case IMAPFilterTask:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_FILTERING, name);
    break;
  default:
    ComposeRString((*taskErrs)->taskDesc, ERR_PERS_UNKNOWN_TASK, name);
    break;
  }

  UL(taskErrs);
  LL_Queue(TaskErrorList, taskErrs, (taskErrHandle));
  if (TaskProgressWindow) {
    TPAddHelpButton(taskErrs);
    if ((err = AddListItemEntry(-1, DrawTPError, (Handle)taskErrs,
                               TaskListHandle))) {
      CurThreadGlobals->tCommandPeriod = true;
      if (taskKind == CheckingTask)
        CheckThreadError = err;
      else if (taskKind == SendingTask)
        SendThreadError = err;
    }
  }
  return (err);
}

/************************************************************************
 * SetTPWTitle - update the GTK window title based on active threads
 ************************************************************************/
static void SetTPWTitle(void) {
  if (!TaskProgressWindow || !GTK_IS_WINDOW(TaskProgressWindow->window))
    return;

  const char *title;
  if (CheckThreadRunning && SendThreadRunning)
    title = "Tasks (Checking, Sending)";
  else if (CheckThreadRunning)
    title = "Tasks (Checking)";
  else if (SendThreadRunning)
    title = "Tasks (Sending)";
  else
    title = "Tasks";

  gtk_window_set_title(GTK_WINDOW(TaskProgressWindow->window), title);
}

/************************************************************************
 * RemoveTaskErrors
 ************************************************************************/
void RemoveTaskErrors(TaskKindEnum taskKind, long persId) {
  taskErrHandle current, last, temp;

  for (last = current = TaskErrorList; current;) {
    temp = current;
    current = (*current)->next;
    if (((*temp)->taskKind == taskKind) &&
        (((*temp)->persId == persId) || (persId == -1))) {
      if (temp == TaskErrorList)
        TaskErrorList = (*temp)->next;
      else
        (*last)->next = (*temp)->next;
      if (TaskProgressWindow) {
        RemoveListItemEntry((Handle)temp, TaskListHandle);
        if ((*temp)->helpButton) {
          /* GTK port: helpButton is a GtkWidget* — destroy it properly */
          gtk_widget_unparent(GTK_WIDGET((*temp)->helpButton));
          (*temp)->helpButton = nil;
        }
      }
      DisposeHandle((Handle)temp);
    } else
      last = temp;
  }
  if (!TaskErrorList)
    NewError = false;
}

/************************************************************************
 * TPIdle - idle time handler for the Task Progress window
 ************************************************************************/
void TPIdle(MyWindowPtr win) {
  //	Update the TP window if the connection method has changed.
  //	We're watching the connection method in ActiveUserIdleTasks().

  if (NeedToUpdateTP())
    InvalContent(win);
}

/*
Task Progress:

To do/bug list:

-tp control for toolbar

-hitting cancel and cmd-. sometimes doesn't cancel stdalert
-insert errors at top of error list
-composestdalert should deactivate frontmost window
-append � to button to mean hit this button when alert times out
-Aborting mail send-- can't edit open comp window without closing it first
-next mail check time sometimes off a minute
-"One of the addresses is too long..." should be a per-message error
-"There was an error opening an attachment..." should be a per-message error

done:
-don't auto-hide tp window until filtered
-inset tp list for wazooability
-offline control
-decouple in/out filtering
-don't put up filter pane for outgoing mail, change menu
-don't show filter pane if can't be done
-flickering scroll bar
-if cell is scrolled out of view and cell in view goes away, out of view cell
not accessible until window resized -report error from main thread if error
reporting error -add #of messages to filter pane

*/
