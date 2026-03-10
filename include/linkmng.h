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

#ifndef LINKMNG_H
#define LINKMNG_H

/* Copyright (c) 1999 by QUALCOMM Incorporated */

#include <stdbool.h>
#include <stdint.h>

/* Forward declare FSSpec so we don't pull in all of mailbox.h here.
   Files that use FSSpec members must include mailbox.h themselves. */
struct FSSpec;

/**********************************************************************
 *  routines to manage multiple link history lists
 **********************************************************************/

/* LinkSortTypeEnum - types of sorts we can do */
typedef enum {
  sType = 0x0000,
  sName = 0x0001,
  sDate = 0x0002,
  sSpecialRemind = 0xFFFF,
  LinkSortTypeLimit
} LinkSortTypeEnum;

/* LinkSortMaskEnum - modifications of sorts we can do */
typedef enum {
  kLinkSortTypeMask = 0x0000FFFF,     /* sort type mask */
  kLinkHasNoCustomIcons = 0x00010000, /* no custom icons */
  kLinkReverseSort = 0x00020000,      /* reverse sort order */
  LinkSortModLimit
} LinkSortMaskEnum;

/* Offline Link Dialog Actions - errors returned by OpenURL we are interested in
 */
enum {
  oldaCancel = 4747, /* user pressed cancel in offline link dialog */
  oldaBookmark,      /* bookmark the link */
  oldaRemind         /* remind the user to visit later */
};

/* AdId - ad identifier (was Mac-specific struct) */
typedef struct {
  int32_t server;
  int32_t ad;
} AdId;

/* VLNodeID - view list node identifier (was Mac opaque type) */
typedef uint32_t VLNodeID;

/* VLNodeInfo - view list node information (was Mac-specific struct) */
typedef struct {
  char name[256];
  VLNodeID nodeID;
  int32_t flags;
  long query;
} VLNodeInfo;

/* ViewListPtr - pointer to a view list */
struct ViewList_;
typedef struct ViewList_ *ViewListPtr;

/* Link management */
int GenHistoriesList(void);
void ZapHistoriesList(bool destroy);
int AddURLToMainHistory(unsigned char *url, unsigned char *name,
                        int urlOpenErr);
int AddAdToLinkHistory(AdId adId, char *pUrl, unsigned char adTitle[256],
                       char *adGraphic);
int SaveAllHistoryFiles(void);
void AdWasClicked(AdId adId, int openErr);
void AgeLinks(void);

/* Link Window related */
void AddAllHistoryItems(ViewListPtr pView, bool needsSort,
                        LinkSortTypeEnum sortType);
void DeleteHistoryEntry(VLNodeInfo *info);
int OpenHistoryEntry(VLNodeInfo *info);
bool GetDateString(VLNodeID id, unsigned char dateStr[256]);
void **GetLinkURL(VLNodeInfo *info);
void **GetLHPreviewIcon(VLNodeID id);
void ZapPVICache(void);

/* Offline Link Dialog related */
bool IsMarkedRemind(VLNodeID id);
bool FindRemindLink(void);
void UnRemindLinks(bool labelToo);

/* Icon functions (QuickDraw-derived; GTK port stubs these) */
int MakeIconSuite(void *gWorld, void *pRect, void *transparentColor,
                  unsigned char *name);
void **MakeIcon(void *srcGWorld, void *srcRect, int16_t dstDepth,
                int16_t iconSize);
void **MakeICN_pound(void *gwp, void *srcRect, int16_t iconDimension,
                     void *transparentColor);

#endif
