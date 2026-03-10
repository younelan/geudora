/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. */

#ifndef LINKWIN_H
#define LINKWIN_H

/* Copyright (c) 1999 by QUALCOMM Incorporated */

#include "linkmng.h"
#include "mailbox.h"
#include "util.h"

/* GTK port: ViewList is the Mac List Views / LV* API.
   Define as opaque struct for compilation. */
#ifndef VIEWLIST_DEFINED
#define VIEWLIST_DEFINED
typedef struct ViewList_ ViewList;
#endif

/* GTK port: CellRec is the Mac List View cell record — defines icon, name,
 * style */
#ifndef CELLREC_DEFINED
#define CELLREC_DEFINED
typedef struct {
  VLNodeID nodeID;
  short iconID;
  unsigned char name[64];
  struct {
    short style;
  } misc;
} CellRec;
#endif

/* GTK port: VLCallbackMessage is enum for ListView callbacks */
#ifndef VLCALLBACKMESSAGE_DEFINED
#define VLCALLBACKMESSAGE_DEFINED
typedef enum {
  kLVAddItem = 1,
  kLVDeleteItem,
  kLVSelectItem,
  kLVDeselectItem,
  kLVOpenItem,
  kLVQueryItem,
  kLVAddNodeItems,
  kLVRenameItem,
  kLVSendDragData,
} VLCallbackMessage;
#endif

/* GTK port: DrawDetailsStruct for LVNewWithDetails */
#ifndef DRAWDETAILSSTRUCT_DEFINED
#define DRAWDETAILSSTRUCT_DEFINED
typedef struct {
  short arrowLeft;
  short iconTop;
  short iconLeft;
  short iconLevelWidth;
  short textBottom;
  short rowHt;
  short nameAddMargin;
  short maxNameWidth;
  short keyNavTicks;
  void *DrawRowCallback;
  void *FillBlankCallback;
  void *GetCellRectsCallBack;
  void *InterestingClickCallback;
} DrawDetailsStruct;
#endif

/* GTK port: LINK_*_CNTL constants are in MyRes.h */

/* Link History Window public API */
void OpenLinkWin(void);
void NotifyLinkWin(void);
void LinkTickle(void);
void RemindSortLinkWin(void);
bool RemindUserNow(void);
bool CanRemindUser(void);

#endif /* LINKWIN_H */
