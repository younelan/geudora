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

#ifndef SENDMAIL_H
#define SENDMAIL_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for dealing with sendmail
 ************************************************************************/
/* Forward declaration for AttMapPtr - actual definition in mime.h */
struct AttMapStruct;
typedef struct AttMapStruct *AttMapPtr;
/* Use the project's filters definitions for CSpecHandle */
#include "filters.h"

/* Forward declaration for DecoderFunc - CallType defined in mime.h */
/* Include mime.h to get CallType definition */
#include "mime.h"
struct DecoderPB;
typedef struct DecoderPB DecoderPB, *DecoderPBPtr;

/* Forward declarations for plugin/translator types */
typedef struct emsMIMEtype *emsMIMEHandle;
typedef struct HeadSpec {
  long offset; /* start offset in text */
  long length; /* length of header value */
  long stop;   /* end offset (offset + length) */
  long value;  /* offset of header value within the header line */
  long start;  /* start of the header label in text (alias for offset) */
  short index; /* header index (e.g. SUBJ_HEAD, FROM_HEAD, etc.) */
} HeadSpec, *HSPtr;
/* typedef int (*DecoderFunc)(int callType, void *decPB); */

int AllAttachOnBoardLo(MessHandle messH, bool errReport);
char * MessReturnAddr(MessHandle messH, char * buffer);
char * GetReturnAddr(char * addr, bool wantDefault);
int StartSMTP(TransStream stream, char *serverName, long Port);
int MySendMessage(TransStream stream, TOCType * tocH, int sumNum,
                  CSpecHandle specList);
int SMTPError(TransStream stream);
int EndSMTP(TransStream stream);
MessHandle SaveB4Send(TOCType * tocH, short sumNum);
int DoRcptTos(TransStream stream, MessHandle messH, bool chatter,
              CSpecHandle fccList, AccuPtr newsGroupAcc);
int DoRcptTosFrom(TransStream stream, MessHandle messH, short index,
                  bool chatter, CSpecHandle fccList, AccuPtr newsGroupAcc);
int TransmitMessageLo(TransStream stream, MessHandle messH, bool chatter,
                      bool mime, bool others, emsMIMEHandle *tlMIME,
                      bool sendDataCmd, bool finishSMTP, bool doTopLevel);
int TransmitMessage(TransStream stream, MessHandle messH, bool chatter,
                    bool mime, bool others, emsMIMEHandle *tlMIME,
                    bool sendDataCmd);
int TransmitHeaders(TransStream stream, MessHandle messH, void *enriched,
                      char * boundary, bool mime, bool others,
                      emsMIMEHandle *tlMIME, bool isRelated);
void TimeStamp(TOCType * tocH, short sumNum, uint32_t when, long delta);
void PtrTimeStamp(MSumPtr sum, uint32_t when, long delta);
#define GetReply(stream, buffer, size, chatter, isEhlo)                        \
  GetReplyLo(stream, buffer, size, nil, chatter, isEhlo)
int GetReplyLo(TransStream stream, char *buffer, int size,
               AccuPtr bufAcc, bool chatter, bool isEhlo);
int SendBodyLines(TransStream stream, char *text, long length, long offset,
                  long flags, bool forceLines, short *lineStarts, short nLines,
                  bool partial, DecoderFunc *encoder);
void BuildDateHeader(char *buffer, long seconds);
char * R822Date(char * date, long seconds);
char * SimpleNameCharset(char * charset, short tid);
short SendPlain(TransStream stream, char *spec, long flags, short tableId,
                AttMapPtr amp);
int GetIndAttachment(MessHandle messH, short index, char * spec,
                       HSPtr where);
int GetIndAttachmentLo(void *text, short index, char * spec, HSPtr where,
                         HeadSpec *hs);
void BuildBoundary(MessHandle messH, char * boundary, char * middle);
bool IsPostScript(char * spec);
short EffectiveTID(short tid);
short TransOutTablID(void);
char * TransOutTablName(char * name);
char *PriorityHeader(char *buffer, uint8_t priority);
int SendRawMIME(TransStream stream, char * spec);
int SendTextFile(TransStream stream, char * spec, long flags,
                   DecoderFunc *encoder);
int FinishSMTP(TransStream stream, MessHandle messH);
int AddFccToList(char * fcc, CSpecHandle list);
char * BuildContentID(char * into, char * mid, long partID, short index);
typedef enum {
  SysStatCode = 211,
  HelpCode = 214,
  ReadyCode = 220,
  CloseCode,
  OkCode = 250,
  ForwardCode = 251,

  StartInputCode = 354,

  NoServiceCode = 421,
  BoxBusyCode = 450,
  LocalErrCode,
  SysFullCode,

  SyntaxCode = 500,
  ArgsBadCode,
  CmdUnImpCode,
  OrderBadCode,
  ArgUnImpCode,
  NoBoxCode = 550,
  YouForwardCode,
  BoxFullCode,
  BoxBadCode,
  PuntCode,

  TransErr = 601,
  RecvErr,
  ReplyErr
} SMErrEnum;

int BufferSend(TransStream stream, DecoderFunc *encoder, char *data,
                 long dataLen, bool text);
#define BS(stream, x, y, z)                                                    \
  do {                                                                         \
    lastLen = z;                                                               \
    lastC = (y)[z - 1];                                                        \
    if (sErr = BufferSend(stream, x, y, z, true))                              \
      goto done;                                                               \
  } while (0)
#define BSCLOSE(stream, e)                                                     \
  do {                                                                         \
    if (sErr = BufferSend(stream, e, nil, 0L, true))                           \
      goto done;                                                               \
  } while (0)
#define SendBoundary(stream)                                                   \
  (SendTrans(stream, "--", 2, boundary, strlen((const char *)boundary),        \
             NewLine, strlen((const char *)NewLine), nil))
#define BufferSendRelease(stream) (void)BufferSend(stream, nil, nil, -1, false)
int SendPString(TransStream stream, char *string);
char * FormatZone(char * string, long delta);
char *GetFlatten(void);
int SendExtras(TransStream stream, void *extras, bool allowQP, short tid);
bool AnyFunny(char *text, long textLen, long offset);

typedef struct {
  MessHandle messH;            // The message we are sending
  bool strip;                  // Should we strip styles?
  bool bloat;                  // Should we send multipart/alternative?
  bool rich;                   // Should we send text/enriched?
  bool html;                   // Should we send text/html?
  bool isRelated;              // Should we send multipart/related?
  bool hasAttachments;         // Do we have attachments?
  bool mime;                   // Do we want to generate the MIME headers?
  bool others;                 // Do we want to generate the other headers?
  bool hasSig;                 // Does the message have a signature?
  bool receipt;                // Are we sending a return receipt?
  bool allLWSP;                // Is the message solely LWSP?
  StackHandle parts;           // Stack of parts
  struct Accumulator enriched; // Rich data of message body, including preamble
  long flags;                  // local copy of message flags
  long opts;                   // local copy of message options
  HeadSpec hs;                 // headspec describing body of message
  TransStream stream;          // the stream to send it on
  DecoderFunc *encoder;        // and the encoder
  emsMIMEHandle *tlMIME;       // MIME headers for translators
} TransmitPB, *TransmitPBPtr, *TransmitPBHandle;

int TransmitMimeVersion(TransmitPBPtr pb);

#define IsAddrErr(c) (((c) / 10) == 55 || (c) == 503 || (c) == 543)

#endif
