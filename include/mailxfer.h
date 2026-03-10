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

#ifndef MAILXFER_H
#define MAILXFER_H

#include "TransStream.h"
#include "filters.h"
#include "message.h"
#include "schizo.h"
#include "toc.h"
#ifdef GTK_ENABLE
#include <glib.h>
#else
#include <glib-object.h>
#endif
#include <stdint.h>

typedef struct XferFlags {
  bool check;
  bool send;
  bool servFetch;
  bool servDel;
  bool nuke;
  bool nukeHard;
  bool stub;
  bool isAuto;
} XferFlags;

#ifdef IMAP
/* IMAP Transfer structure - complete definition */
#ifndef IMAP_TRANSFER_REC_DEFINED
#define IMAP_TRANSFER_REC_DEFINED
/* IMAP Transfer structure - complete definition */
typedef struct IMAPTransferRec_ {
  short command;
  FSSpec targetSpec;
  TOCHandle destToc;
  void *uids;
  bool attachmentsToo;
  TOCHandle delToc;
  bool nuke;
  bool expunge;
  TOCHandle sourceToc;
  bool copy;
  void *attachments;
  FSSpec targetBox;
  void *boxesToSearch;
  void *toSearch;
  void *searchC;
  bool matchAll;
  long firstUID;
  void *toResync;
  void *appendData;
} IMAPTransferRec, *IMAPTransferPtr;
#endif

#endif

short XferMail(bool check, bool send, bool manual, bool ae, bool thread,
               short modifiers);
short XferMailSetup(bool *check, bool *send, bool manual, bool ae,
                    XferFlags *flags, short modifiers);
#ifdef IMAP
short XferMailRun(bool check, bool send, bool manual, bool ae, XferFlags flags,
                  IMAPTransferPtr imapInfo);
#else
short XferMailRun(bool check, bool send, bool manual, bool ae, XferFlags flags);
#endif
void GrabSignature(uint32_t fid);
OSErr SigSpec(FSSpecPtr spec, long id);
OSErr TransmitMessageHi(TransStream stream, MessHandle messH, bool chatter,
                        bool sendDataCmd);
void ShowBoxAt(TOCHandle tocH, short selectMe, WindowPtr behindWin);
short FumLub(TOCHandle tocH);
#ifdef THREADING_ON
void FilterXferMessages(void);
void ResetCheckTime(bool force);
#endif
#ifdef BATCH_DELIVERY_ON
void NotifyNewMail(short gotSome, bool noXfer, TOCHandle tocH,
                   FilterPB *fpbDelivery);
void NotifyNewMailLo(short gotSome, bool noXfer, TOCHandle tocH,
                     FilterPB *fpbDelivery, bool OpenIn);
#else
void NotifyNewMail(short gotSome, bool noXfer, TOCHandle tocH);
#endif
OSErr DoFcc(TOCHandle tocH, short sumNum, CSpecHandle list);
void CompAttDel(MessHandle messH);
WindowPtr OpenBehindMePlease(void);
void ProcessReceivedRegFiles(void);
OSErr OutgoingMIDListSave(void);
OSErr OutgoingMIDListLoad(void);
void BadgeTheSupidDock(short count, PStr text, bool attentionColor);
long GlobalUnreadCount(void);
#define PrefBadgeDo() ((GetPrefLong(PREF_NO_STEENKEEN_BATCHES) & 0x1) == 0)
#define PrefBadgeRecent() ((GetPrefLong(PREF_NO_STEENKEEN_BATCHES) & 0x2) == 0)
#define PrefBadgeOpenBoxes()                                                   \
  ((GetPrefLong(PREF_NO_STEENKEEN_BATCHES) & 0x4) == 0)
#endif
