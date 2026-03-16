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

#include "progress.h"
#include "Globals.h"
#include "StringDefs.h" /* For PROGRESS and related constants */
#include "StringUtil.h"
#include "gtk_dialogs.h"
#include "log.h" /* For Log() */
#include "mailbox.h"
#include "taskProgress.h" /* For InvalTPRect, DrawTaskProgressBar */
#include "threading.h"
#include "util.h"
#include <glib.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * Progress monitoring software
 ************************************************************************/

#define FILE_NUM 33
#ifndef TASK_PROGRESS_ON
#define PROG_BAR_HI 12
#define PROG_BOX_HI 16
#endif
#pragma segment Progress

/************************************************************************
 * other stuff
 ************************************************************************/

bool ProgressOff;

/************************************************************************
 * Private functions
 ************************************************************************/
void DisposProgress(ProgressBHandle prbl);
void InvalProgress(ProgressRectEnum which);

void ProgressButton(MyWindowPtr win, ControlHandle button, long modifiers,
                    short part);
void InstallProgMessage(const char *string, ProgressRectEnum which);
bool ProgPosition(bool save, MyWindowPtr win);
bool ProgClose(MyWindowPtr win);
/* CycleBalls is in legacy_shim.h */
#include "taskProgress.h"
#include <glib.h>

/* TickCount - Mac-style ticks (1/60th of a second) using GLib monotonic time */
uint32_t TickCount(void) {
  return (uint32_t)(g_get_monotonic_time() * 60 / 1000000);
}

ProgressBlock **GetPrbl(MyWindowPtr win);
void SetPrbl(MyWindowPtr win, ProgressBlock **prbl);

/**********************************************************************
 * ProgressR - progress with string resource IDs
 **********************************************************************/
void ProgressR(short percent, short remaining, short titleId, short subTitleId,
               const char *message) {
  char title[256] = "", subTitle[256] = "";

  if (titleId)
    GetRString((unsigned char *)title, titleId);
  if (subTitleId)
    GetRString((unsigned char *)subTitle, subTitleId);
  Progress(percent, remaining, titleId ? title : nil,
           subTitleId ? subTitle : nil, message);
}

/**********************************************************************
 * ProgressMessage - update one section of task progress
 **********************************************************************/
void ProgressMessage(short which, const char *message) {
  const char *title = NULL, *subTitle = NULL, *msg = NULL;

  switch (which) {
  case kpTitle:    title = message; break;
  case kpSubTitle: subTitle = message; break;
  case kpMessage:  msg = message; break;
  }
  UpdateTaskProgress(which == kpMessage ? NoChange : NoBar, NoBar);
  UpdateTaskMessage(title, subTitle, msg);
}

#pragma segment Progress

/************************************************************************
 * GetPrbl - get appropriate ProgressBlock
 ************************************************************************/
ProgressBlock **GetPrbl(MyWindowPtr win) {
#ifndef TASK_PROGRESS_ON
  return ((ProgressBlock **)GetMyWindowPrivateData(ProgWindow));
#else
  ProgressBlock **prbl;

  if (win && (win != TaskProgressWindow))
    prbl =
        ProgWindow ? (ProgressBlock **)GetMyWindowPrivateData(ProgWindow) : nil;
  else
    prbl = GetCurrentThreadPrbl();
  return (prbl);
#endif
}

/************************************************************************
 * SetPrbl - set appropriate ProgressBlock
 ************************************************************************/
void SetPrbl(MyWindowPtr win, ProgressBlock **prbl) {
#ifndef TASK_PROGRESS_ON
  SetMyWindowPrivateData(win, (long)prbl);
#else
  if (win && (win != TaskProgressWindow)) {
    SetMyWindowPrivateData(win, prbl);
  } else {
    threadDataHandle threadData = nil;

    GetCurrentThreadData(&threadData);
    if (threadData)
      threadData->prbl = prbl;
  }
#endif
}

/**********************************************************************
 * ProgressMessageR - like progressmessage, but with resource id instead
 **********************************************************************/
void ProgressMessageR(short which, short messageId) {
  char message[256];

  ProgressMessage(which, (const char *)GetRString((unsigned char *)message, messageId));
}

/************************************************************************
 * GetProgressBytes - return the number of bytes transmitted so far
 ************************************************************************/
int GetProgressBytes(void) {
  return 0;
}

void ByteProgressExcess(int excess) {
  (void)excess;
}

/************************************************************************
 * ByteProgress - keep track of the number of bytes transmitted so far
 ************************************************************************/
void ByteProgress(const char *message, int onLine, int totLines) {
  static long lastTicks;
  static int currentOn, currentTotal;

  CycleBalls();

  if (onLine >= 0) {
    currentOn = onLine;
    lastTicks = 0;
  } else {
    currentOn -= onLine;
  }
  if (totLines) {
    currentTotal = totLines;
    lastTicks = 0;
  }
  if (!currentTotal)
    return;

  if (TickCount() - lastTicks > 10) {
    lastTicks = TickCount();
    Progress((100 * currentOn) / currentTotal, NoChange, nil, nil, message);
  }
}

/************************************************************************
 * OpenProgress - create the progress window
 ************************************************************************/
static gboolean open_progress_idle(gpointer data) {
  (void)data;
  OpenTasksWin();
  return G_SOURCE_REMOVE;
}

int OpenProgress(void) {
  if (InAThread())
    g_idle_add(open_progress_idle, NULL);
  else
    OpenTasksWin();
  return 0;
}
#pragma segment Progress

/**********************************************************************
 * ProgressButton - handle the stop button
 **********************************************************************/
void ProgressButton(MyWindowPtr win, ControlHandle button, long modifiers,
                    short part) {
  /* Stub for Mac event handling */
}

/************************************************************************
 * CloseProgress - close the progress window
 ************************************************************************/
static gboolean close_progress_idle(gpointer data) {
  (void)data;
#ifdef TASK_PROGRESS_ON
  if (ProgWindow && (ProgWindow != TaskProgressWindow))
    CloseMyWindow(GetMyWindowWindowPtr(ProgWindow));
#else
  if (ProgWindow)
    CloseMyWindow(GetMyWindowWindowPtr(ProgWindow));
#endif
  return G_SOURCE_REMOVE;
}

void CloseProgress(void) {
  if (InAThread())
    g_idle_add(close_progress_idle, NULL);
  else
    close_progress_idle(NULL);
}

/**********************************************************************
 * ProgClose - deal with closing the progress window
 **********************************************************************/
bool ProgClose(MyWindowPtr win) {
  DisposProgress((ProgressBHandle)GetMyWindowPrivateData(win));
  SetMyWindowPrivateData(win, nil);
  ProgWindow = nil;
  return (True);
}

/************************************************************************
 * DisposProgress - get rid of the progress chain
 ************************************************************************/
void DisposProgress(ProgressBHandle prbl) {
  if (prbl == nil)
    return;
  /* Control handling stubbed for GTK port */
  if ((*prbl)->next)
    DisposProgress((*prbl)->next);
  ZapHandle(prbl);
}

/************************************************************************
 * Progress - update task progress (thread-safe, no legacy Mac code)
 ************************************************************************/
void Progress(short percent, short remaining, const char *title,
              const char *subTitle, const char *message) {
  /* Log message if provided */
  if (message && *message)
    Log(LOG_PROG, message);

  /* Update the GTK task progress panel via g_idle_add (thread-safe) */
  UpdateTaskProgress(percent, remaining);
  UpdateTaskMessage(title, subTitle, message);
}

/************************************************************************
 * ProgPosition - stub (no legacy progress window)
 ************************************************************************/
bool ProgPosition(bool save, MyWindowPtr win) {
  (void)save; (void)win;
  return true;
}

/* InstallProgMessage - legacy stub */
void InstallProgMessage(const char *string, ProgressRectEnum which) {
  (void)string; (void)which;
}

/* InvalProgress - legacy stub */
void InvalProgress(ProgressRectEnum which) {
  (void)which;
}

/************************************************************************
 * ProgressUpdate - update the progress window
 ************************************************************************/
void ProgressUpdate(MyWindowPtr win) {}

/* PushProgress - legacy stub */
void PushProgress(void) {}

/* PopProgress - legacy stub */
void PopProgress(bool messageOnly) { (void)messageOnly; }

/* PressStop - legacy stub */
void PressStop(void) {}

#pragma segment Main

/************************************************************************
 * ProgressIsOpen - is the progress window open?
 ************************************************************************/
bool ProgressIsOpen(void) { return (ProgWindow != nil); }
/************************************************************************
 * ProgressIsTop - is the progress window topmost?
 ************************************************************************/
bool ProgressIsTop(void) {
  return (ProgWindow != nil &&
          GetMyWindowWindowPtr(ProgWindow) == FrontWindow_());
}
