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

/* GTK/Window related */
#include "message.h"
#include <gtk/gtk.h>
#include <stdbool.h>

bool ShowMyWindow(void *winWP);
bool CloseMyWindow(void *winWP);

#ifndef COMP_H
#define COMP_H

#include "sendmail.h"

#include "mailbox.h"
#include "toc.h"
#include <gtk/gtk.h>
#include <stdbool.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Exported comp APIs */
MyWindowPtr DoComposeNew(int type);
void ApplyStationeryLo(MyWindowPtr win, char * spec, bool b1, bool b2,
                       bool b3);

/* Helpers to fill compose window fields */
void comp_set_field(MyWindowPtr win, const char *key, const char *value);
void comp_set_body(MyWindowPtr win, const char *text);
void comp_set_body_quoted(MyWindowPtr win, const char *attribution,
                          const char *body);

/* Main composition functions - ported to use gEditCtrl instead of Pete */
bool SaveComp(MyWindowPtr win);
MyWindowPtr OpenComp(TOCType * tocH, int sumNum, GtkWidget *winWP,
                     MyWindowPtr win, bool showIt, bool new);
int QueueSelectedMessages(TOCType * tocH, short toState, unsigned long when);
int CreateMessageBody(char *buffer, unsigned long *hashPtr);
long CountCompBytes(MessHandle messH);
void UpdateSum(MessHandle messH, long offset, long length);
char *NewMessageId(char *id);
long BodyOffset(char *text);

/* Address and identity functions - ported to standard C strings */
bool IsMe(const char *address);
int WriteTranslators(short refN, void *translators);
int AddTranslatorsFromPtr(MessHandle messH, char *text, long len);

/* Header manipulation functions - ported to use gEditCtrl and standard C types
 */
HSPtr CompHeadFind(MessHandle messH, short index, HSPtr hSpec);
HSPtr CompHeadFindStr(MessHandle messH, char *name, HSPtr hSpec);
HSPtr HandleHeadFindStr(char *text, char *name, HSPtr hSpec);
HSPtr HandleHeadFindReply(char *text, HSPtr hSpec);
int HandleHeadGetAddrs(char *text, HSPtr hs, char **addrs);
int HandleHeadCopyAddrs(char *text, HSPtr hs, MessHandle messH, short headerID,
                        AccuPtr addrAcc, bool cacheThem);
int CompHeadAppendAddrStr(MessHandle messH, HSPtr targetHS, char *addr);
int AddSelfAddrHashes(AccuPtr addrAcc);

/* gEditCtrl-based header functions - ported from Pete */
int CompHeadActivate(GtkWidget *pte, HSPtr hSpec);
int CompHeadSet(GtkWidget *pte, HSPtr hSpec, char *text);
int CompHeadAppend(GtkWidget *pte, HSPtr hSpec, char *text);
int CompHeadSetPtr(GtkWidget *pte, HSPtr hSpec, char *text, long size);
#define CompHeadSetStr(messH, hSpec, str)                                      \
  CompHeadSetPtr((messH), (hSpec), (str), strlen(str))
int CompHeadSetIndexPtr(GtkWidget *pte, short index, char *text, long size);
#define CompHeadSetIndexStr(messH, index, str)                                 \
  CompHeadSetIndexPtr((messH), (index), (str), strlen(str))
int CompHeadPrependPtr(GtkWidget *pte, HSPtr hSpec, char *text, long size);
int CompHeadAppendPtr(GtkWidget *pte, HSPtr hSpec, char *text, long size);
#define CompHeadAppendStr(messH, hSpec, str)                                   \
  CompHeadAppendPtr((messH), (hSpec), (str), strlen(str))

/* Text extraction functions - ported to standard C strings */
int HandleHeadGetText(char *textIn, HSPtr hSpec, char **text);
int HandleHeadGetIdText(char *textIn, short id, char **text);
int CompHeadGetText(GtkWidget *pte, HSPtr hSpec, char **text);
int CompHeadGetTextPtr(GtkWidget *pte, HSPtr hSpec, long offset, char *text,
                       long textSize, long *bytes);
char *HandleHeadGetPStr(char *text, short head, char *val);

/* Header retrieval functions - ported to standard C */
int GetRHeaderAnywhere(MessHandle messH, short header, char **text);
int GetRHeaderAnywherePtr(MessHandle messH, short header, char *text,
                          long textSize, long *bytes);
int GetHeaderAnywhere(MessHandle messH, char *header, char **text);
int GetHeaderAnywherePtr(MessHandle messH, char *header, char *text,
                         long textSize, long *bytes);

/* Composition utility functions - ported to gEditCtrl */
short CompHeadCurrent(GtkWidget *pte);
int CompHeadGetStrLo(MessHandle messH, short index, char *string, short size);
int CompAddExtraHeaderDangerDangerLookOutWillRobinson(MessHandle messH,
                                                      char *headName,
                                                      char *headContents);
void SuckHeaderText(MessHandle messH, char *string, long size, short index);
char *CompGetMID(MessHandle messH, char *mid);
#define CompHeadGetStr(p, i, s) CompHeadGetStrLo(p, i, s, sizeof(s))

/* Window identification macro - ported to GTK */
#define IsCompWindow(aWindowPtr)                                               \
  (((aWindowPtr) && GetWindowKind(aWindowPtr) == COMP_WIN) ? true : false)

/* Nickname and address functions - ported to gEditCtrl */
bool IsHeaderNickField(GtkWidget *pte);
int HiliteCompHeader(GtkWidget *pte, bool hilite);
bool GetCompNickFieldRange(GtkWidget *pte, long *start, long *end);
int CompGatherRecipientAddresses(MessHandle messH, bool wantComments);
bool IsAllLWSPMess(MessHandle messH);

/* Signature functions - ported to standard C */
unsigned long GetSigByName(char *name);
int AddInlineSig(MessHandle messH);
int RemoveInlineSig(MessHandle messH);

/* Subject manipulation - ported to standard C */
void PersonalizeSubject(MessHandle messH);
void SerializeSubject(MessHandle messH);
void CompSelectSecondUnquoted(MessHandle messH);

/* Header spec utility macro */
#define HSIsEmpty(hs) ((hs)->stop <= (hs)->value + 1)

/* Additional composition window functions - ported to GTK */
void CompDidResize(MyWindowPtr win);
bool CompClick(MyWindowPtr win, GdkEvent *event);
bool CompMenu(MyWindowPtr win, int menuItem);
bool CompKey(MyWindowPtr win, GdkEvent *event);
bool CompButton(MyWindowPtr win, GtkWidget *button, GdkEvent *event);
void CompHelp(MyWindowPtr win, int helpType);
void CompGonnaShow(MyWindowPtr win);
bool CompDragHandler(MyWindowPtr win, void *dragEvent);
void CompIdle(MyWindowPtr win);
bool CompClose(MyWindowPtr win);
bool CompSend(MessHandle messH);
bool CompSave(MessHandle messH);

/* Address gathering function */
int GatherCompAddresses(MyWindowPtr win, char *addrList);

/* Utility functions */
char *CompCurAddr(MyWindowPtr win, char *addr);

#endif