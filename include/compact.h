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

#ifndef COMPACT_H
#define COMPACT_H

/* Expects message.h and toc.h included first */

#ifndef ICON_BAR_NUM
#define ICON_BAR_NUM 6
#endif

typedef enum {kEuSendNow, kEuSendNext, kEuSendLater, kEuSendNever} SendTypeEnum;

#ifdef THREADING_ON
#define SENT_OR_SENDING(state) ((state)==SENT || (state)==BUSY_SENDING)
#else
#define SENT_OR_SENDING(state) ((state)==SENT)
#endif

#ifndef FLAG_ATYPE_LO
#define FLAG_ATYPE_LO (1L<<6)
#define FLAG_ATYPE_HI (1L<<7)
#endif
#define AttachOptNumber(flags) (((flags & (FLAG_ATYPE_LO|FLAG_ATYPE_HI))>>6)&0x3)
#define SetAOptNumber(flags,num) \
	do{(flags) &= ~(FLAG_ATYPE_LO|FLAG_ATYPE_HI); (flags) |= (num)<<6;}while(0)

/* Queue and send */
int QueueMessage(TOCType *tocH, short sumNum, SendTypeEnum st, long secs, bool noSpell, bool noAnalDelay);
bool ModifyQueue(short *state, uLong *when, bool swap);
void WarpQueue(uLong secs);

/* Attachment handling */
void CompAttach(MyWindowPtr win, bool insertDefault);
void CompAttachStd(MyWindowPtr win, bool insertDefault);
void CompAttachSpec(MyWindowPtr win, FSSpec *spec);
void CompUnattach(MyWindowPtr win);
void CompDelAttachment(MessHandle messH, void *hs);
void AttachSelect(MessHandle messH);
short CountAttachments(MessHandle messH);
int AttachDoc(MyWindowPtr win, FSSpec *spec);

/* Composition UI */
void CompSetFormatBarIcon(MyWindowPtr win, bool visible);
int AddPriorityPopup(MessHandle messH);
void CompActivateAppropriate(MessHandle messH);
void ForceCompWindowRecalcAndRedraw(MyWindowPtr win);
void CompReallyPreferBody(MyWindowPtr win);
void CompIBarUpdate(MessHandle messH);
void CompSendBtnUpdate(MyWindowPtr win);
void CompUpdateScore(MyWindowPtr win);

/* Field navigation */
int CompLeaving(MessHandle messH, short head);
int NickExpandAndCacheHead(MessHandle messH, short head, bool cacheOnly);

/* Signature and attachment type */
void SetSig(TOCType *tocH, short sumNum, int sigId);
void SetAttachmentType(TOCType *tocH, short sumNum, short type);

/* Translators */
int AddMessTranslator(MessHandle messH, long which, void *properties);
int RemoveMessTranslator(MessHandle messH, long which);
bool InTranslator(TransInfoHandle hTranslators, long id);

/* Stationery */
uLong ApproxMessageSize(MessHandle messH);
int SaveStationeryStuff(short refN, MessHandle messH);
int GetStationerySum(unsigned char *text, long textLen, MSumPtr pSum);
void ApplyStationery(MyWindowPtr win, FSSpec *spec, bool dontCleanse, bool personality);
void ApplyStationeryLo(MyWindowPtr win, FSSpec *spec, bool dontCleanse, bool personality, bool editStationery);
void ApplyStationeryHandle(MyWindowPtr win, unsigned char *text, long textLen, bool dontCleanse, bool personality, bool editStationery);

/* Drawing */
void PlotFlag(Rect *r, bool on, short which);
void DrawPopIBox(Rect *r, short sicnId);
void DrawShadowBox(Rect *r);

/* Recipient gathering */
int GatherRecipientAddresses(MessHandle messH, char **dest, bool wantComments);

#endif
