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
void InstallProgMessage(PStr string, ProgressRectEnum which);
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
 * ProgressR - progress, but with resource id's for title/subtitle
 **********************************************************************/
void ProgressR(short percent, short remaining, short titleId, short subTitleId,
               PStr message) {
  Str255 title, subTitle;

  *title = *subTitle = 0;

  if (titleId)
    GetRString(title, titleId);
  if (subTitleId)
    GetRString(subTitle, subTitleId);
  Progress(percent, remaining, titleId ? title : nil,
           subTitleId ? subTitle : nil, message);
}

/**********************************************************************
 * ProgressMessage - put something in one section of progress
 **********************************************************************/
void ProgressMessage(short which, const unsigned char *message) {
  UPtr msgs[kpTitle + 1];
  Str15 empty;
  ProgressRectEnum remaining;

  *empty = 0;
  remaining = which == kpMessage ? NoChange : NoBar;
  WriteZero(msgs, sizeof(msgs));
  msgs[which] = message;
  while (which)
    msgs[--which] = empty;
  Progress(remaining, remaining, msgs[kpTitle], msgs[kpSubTitle],
           msgs[kpMessage]);
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
  Str255 message;

  ProgressMessage(which, GetRString(message, messageId));
}

/************************************************************************
 * GetProgressBytes - return the number of bytes transmitted so far
 ************************************************************************/
int GetProgressBytes(void) {
  ProgressBlock **prbl;
  if (prbl = (ProgressBlock **)GetPrbl(ProgWindow))
    return ((*prbl)->on);
  return 0;
}

/************************************************************************
 * ByteProgressExcess - we under-estimated
 ************************************************************************/
void ByteProgressExcess(int excess) {
  ProgressBlock **prbl;
  if (prbl = (ProgressBlock **)GetPrbl(ProgWindow))
    (*prbl)->excessOn += excess;
}

/************************************************************************
 * ByteProgress - keep track of the number of bytes transmitted so far
 ************************************************************************/
void ByteProgress(UPtr message, int onLine, int totLines) {
  ProgressBlock **prbl;
  static long lastTicks;

  CycleBalls();

  if (prbl = (ProgressBlock **)GetPrbl(ProgWindow)) {
    if (onLine >= 0) {
      (*prbl)->on = onLine;
      (*prbl)->excessOn = 0;
      lastTicks = 0;
    } else {
      if ((*prbl)->excessOn > 0) {
        if (((*prbl)->excessOn -= onLine) < 0)
          onLine = (*prbl)->excessOn;
        else
          onLine = 0;
      }
      (*prbl)->on -= onLine;
    }
    if (totLines) {
      (*prbl)->excessOn = 0;
      (*prbl)->total = totLines;
      lastTicks = 0;
    }

    if (!(*prbl)->total)
      return;

    if (TickCount() - lastTicks > 10) {
      lastTicks = TickCount();
      ASSERT((*prbl)->total != 0);
      Progress((100 * (*prbl)->on) / (*prbl)->total, NoChange, nil, nil,
               message);
    }
  }
}

/************************************************************************
 * OpenProgress - create the progress window
 ************************************************************************/
int OpenProgress(void) {
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
void CloseProgress(void) {
#ifdef TASK_PROGRESS_ON
  if (ProgWindow && (ProgWindow != TaskProgressWindow))
    CloseMyWindow(GetMyWindowWindowPtr(ProgWindow));
#else
  if (ProgWindow)
    CloseMyWindow(GetMyWindowWindowPtr(ProgWindow));
#endif
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
 * Progress - record progress in the progress window
 ************************************************************************/
// thread-aware version

void Progress(short percent, short remaining, PStr title, PStr subTitle,
              PStr message) {
  /* This should call the GTK version of Progress or update ProgressBlock */
  ProgressBlock **prbl;
  char c_title[256], c_subtitle[256];

  if (!(prbl = (ProgressBlock **)GetPrbl(ProgWindow))) {
    /* If no legacy window, just update the taskProgress wazoo */
    UpdateTaskProgress(percent, remaining);
    UpdateTaskMessage(title ? (char *)pstr_to_c(title) : NULL,
                      subTitle ? (char *)pstr_to_c(subTitle) : NULL,
                      message ? (char *)pstr_to_c(message) : NULL);
    return;
  }

  if (percent != NoChange)
    (*prbl)->percent = percent;
  if (title)
    PCopy((*prbl)->title, title);

  InstallProgMessage(message, kpMessage);
  InstallProgMessage(subTitle, kpSubTitle);

  /* Log message if needed */
  if (message)
    Log(LOG_PROG, message);

  /* Also update the taskProgress wazoo */
  UpdateTaskProgress(percent, remaining);
  UpdateTaskMessage(title ? (char *)pstr_to_c(title) : NULL,
                    subTitle ? (char *)pstr_to_c(subTitle) : NULL,
                    message ? (char *)pstr_to_c(message) : NULL);
}

/************************************************************************
 * ProgPosition - ph window position
 ************************************************************************/
bool ProgPosition(bool save, MyWindowPtr win) {
  Str31 progress;

  SetWTitle_(GetMyWindowWindowPtr(win), GetRString(progress, PROGRESS));
  return (PositionPrefsTitle(save, win));
}

/**********************************************************************
 * InstallProgressMessage - install a progress message
 **********************************************************************/
void InstallProgMessage(PStr string, ProgressRectEnum which) {
  Str255 scratch;
  ProgressBlock **prbl;

  if (!(prbl = (ProgressBlock **)GetPrbl(ProgWindow)))
    return;

  if (string) {
    PCopyTrim(scratch, string, sizeof(scratch));
    LDRef(prbl);
    if (!StringSame(scratch, (*prbl)->messages[which])) {
      PCopy((*prbl)->messages[which], scratch);
      InvalProgress(which);
      if (which == kpMessage)
        Log(LOG_PROG, scratch);
    }
    UL(prbl);
  }
}

/************************************************************************
 * InvalProgress - invalidate the selected part of the progress window
 ************************************************************************/
void InvalProgress(ProgressRectEnum which) {
  ProgressBlock **prbl;
  short start, stop;

#ifdef TASK_PROGRESS_ON
  if (!ProgWindow)
    return;
  if (ProgWindow == TaskProgressWindow &&
      GetWindowKind(GetMyWindowWindowPtr(TaskProgressWindow)) != TASKS_WIN)
    return;
#endif
  if (!(prbl = (ProgressBlock **)GetPrbl(ProgWindow)))
    return;
  PushGWorld();
  SetPort_(GetMyWindowCGrafPtr(ProgWindow));
  if (which == kpTitle) {
    start = 0;
    stop = kpTitle;
  } else
    start = stop = which;
  for (; start <= stop; start++) {
    Rect textRect = (*prbl)->rects[start];
#ifdef TASK_PROGRESS_ON
    if (ProgWindow == TaskProgressWindow)
      InvalTPRect(&textRect);
    else
#endif
      InvalWindowRect(GetMyWindowWindowPtr(ProgWindow), &textRect);
  }
  PopGWorld();
}

/************************************************************************
 * ProgressUpdate - update the progress window
 ************************************************************************/
void ProgressUpdate(MyWindowPtr win) {}

/************************************************************************
 * PushProgress - stash a copy of the progress info
 ************************************************************************/
void PushProgress(void) {
  ProgressBHandle prbl;
  ProgressBHandle pH;

  if (!ProgWindow)
    return;

  if (!(prbl = (ProgressBlock **)GetPrbl(ProgWindow)))
    return;

  UL(prbl);
  GetWTitle(GetMyWindowWindowPtr(ProgWindow), (*prbl)->title);
  UL(prbl);
  if (pH = NuHandle(sizeof(ProgressBlock))) {
    **pH = **prbl;
    (*pH)->next = prbl;
    SetPrbl(ProgWindow, pH);
  }
}

/************************************************************************
 * PopProgress - restore the progress info
 ************************************************************************/
void PopProgress(bool messageOnly) {
  WindowPtr ProgWindowWP = GetMyWindowWindowPtr(ProgWindow);
  ProgressBHandle prbl;
  ProgressBHandle pNext;

  if (!ProgWindow)
    return;

  if (!(prbl = (ProgressBlock **)GetPrbl(ProgWindow)))
    return;

  if ((*prbl)->next) {
    pNext = (*prbl)->next;
    (*pNext)->percent = (*prbl)->percent;
    (*pNext)->on = (*prbl)->on;
    (*pNext)->total = (*prbl)->total;
    SetWTitle_(ProgWindowWP, LDRef(pNext)->title);
    UL(pNext);
    if ((*prbl)->bar) {
      SetControlValue((*prbl)->bar, (*prbl)->percent);
      if ((*prbl)->percent == NoBar)
        SetControlVisibility((*prbl)->bar, false, true);
      //				HideControl((*prbl)->bar);
      else {
        SetControlVisibility((*prbl)->bar, true, false);
#ifdef TASK_PROGRESS_ON
        if (ProgWindow == TaskProgressWindow)
          DrawTaskProgressBar((*prbl)->bar);
        else
#endif
          ShowControl((*prbl)->bar);
      }
    }

    SetPrbl(ProgWindow, pNext);
    InvalProgress(kpTitle);
    ZapHandle(prbl);
    UpdateMyWindow(ProgWindowWP);
  }
}

/**********************************************************************
 * PressStop - press the stop button
 **********************************************************************/
void PressStop(void) {
  ProgressBlock **prbl = (ProgressBlock **)GetPrbl(ProgWindow);
  ControlHandle stopH;

  if (ProgWindow && prbl && (stopH = (*prbl)->stop)) {
    HiliteControl(stopH, 1);
    Pause(20L);
    HiliteControl(stopH, 0);
  }
}

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
