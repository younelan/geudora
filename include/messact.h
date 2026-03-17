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

#ifndef MESSACT_H
#define MESSACT_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * prototypes
 **********************************************************************/

#include "mailbox.h"
#include "mydefs.h"
#include "trans.h"

// Mac types for portable layer
typedef void *RgnHandle;
bool MessClose(MyWindowPtr win);
bool MessMenu(MyWindowPtr win, int menu, int item, short modifiers);
void MessZoomSize(MyWindowPtr win, Rect *zoom);
void HiliteOddReply(MessHandle messH);
// void AlignHeaders(MessHandle messH);
void SaveMessageAs(MessHandle messH);
bool MessKey(MyWindowPtr win, void *event);
short SaveAsToOpenFile(short refN, MessHandle messH);
bool SaveAsFilter(void *dgPtr, void *event, short *item, char *userData);
short SaveAsHook(short item, void *dgPtr, char *userData);
void NextMess(TOCType * tocH, MessHandle messH, short whichWay, long modifiers,
              bool ezOpen);
bool MessFind(MyWindowPtr win, char *what);
void MessClick(MyWindowPtr win, void *event);
short NewPrior(short item, short prior);
int MessGonnaShow(MyWindowPtr win);
int MessMakeEditable(MyWindowPtr win, bool value);
void MessDidResize(MyWindowPtr win, Rect *oldContR);
void MessFocus(MessHandle messH, PETEHandle pte);
void MessCursor(Point mouse);
bool MessagePosition(bool save, MyWindowPtr win);

void PriorMenuHelp(MyWindowPtr win, Rect *priorRect);
bool GetPriorityRect(MyWindowPtr win, Rect *pr);
void DrawPriority(Rect *pr, short p);
short PriorityMenu(MyWindowPtr win);
void ShowMessageSeparator(GtkWidget *pte, bool center);
int UnwrapSave(unsigned char *text, long length, long offset, short refN);
bool MessApp1(MyWindowPtr win, void *event);
void SetSubject(TOCType * tocH, short sumNum, unsigned char *sub);
void SetSender(TOCType * tocH, short sumNum, unsigned char *sender);
void SetFlag(TOCType * tocH, short sumNum, long flag, bool on);
void SetOpt(TOCType * tocH, short sumNum, long flag, bool on);
void MessIBarUpdate(MessHandle messH);
bool CheckAddNotifyControls(MyWindowPtr win, MessHandle messH);
#define attColor 1
#define attSelect 2
#define attOpen 4
#define attFinder 8
#define attAll 7
#define attPrint 16
bool SaveMess(MyWindowPtr win);
int MessSaveSub(MessHandle messH);
void AddMessErrNote(MessHandle messH);
void PlaceMessErrNote(MessHandle messH);
bool SaveMessHi(MyWindowPtr win, bool closing);
bool AttIsSelected(MyWindowPtr win, PETEHandle pte, long startWith,
                   long endWith, short what, long *start, long *stop);
bool TransferMenuChoice(short menu, short item, TOCType * tocH, short sumNum,
                        long modifiers, bool fcc);
int AttLine2Spec(unsigned char *line, FSSpecPtr spec, bool wantToOpen);
int RelLine2Spec(unsigned char *line, FSSpecPtr spec, uLong *cid, uLong *relURL,
                 uLong *absURL);
short AddXlateTables(bool isOut, short nowId, bool ph, void **pmh);
void SetMessTable(TOCType * tocH, short sumNum, short tableId);
RgnHandle MessBuildDragRgn(MessHandle messH);
bool Menu2TableId(TOCType * tocH, void **pmh, short item, short *tableId);
void SetMessTable(TOCType * tocH, short sumNum, short newId);
int ExportHTMLSum(TOCType * tocH, short sumNum);
int ExportHTML(MessHandle messH);
#ifdef TWO
bool GetServerRect(MyWindowPtr win, short which, Rect *r);
#endif
short EzOpenFind(TOCType * tocH, short origSum);
void EzOpen(TOCType * tocH, short sumNum, uLong uidHash, long modifiers,
            bool hideFront, bool willDelete);
void Fcc(MessHandle messH, FSSpecPtr box);
short MessWi(MyWindowPtr win);
#define IsArrowSwitch(m)                                                       \
  (((m) & (shiftKey | optionKey | cmdKey | alphaLock | controlKey)) ==         \
   GetPrefLong(PREF_SWITCH_MODIFIERS))
#endif
bool GetMesgErrorsRect(MyWindowPtr win, Rect *r);
short GetMesgErrorsHeight(MyWindowPtr win);
#define MESG_ERR_WIDTH 32
int IncrementQuoteLevel(PETEHandle pte, long startSel, long endSel,
                        short increment);
short SaveAsHook(short item, void *dgPtr, char *userData);
void PetePaneDraw(void *cntl, short part);
