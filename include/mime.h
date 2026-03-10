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

#ifndef MIME_H
#define MIME_H

#include "mydefs.h"
#include "pete_portable.h"
#include "trans.h"
#include "header.h"
#include <stdbool.h>

#define Estimate64(x)                                                          \
  ((M_T1 = (4 * (x) + 3) / 3), M_T1 + ((M_T1 + 75) / 76) * 2)
#define Estimate64Bin(x) (((x) * 3) / 4)

/*
 * state buffer for encoding
 */
typedef struct {
  unsigned char partial[4];
  short partialCount;
  short bytesOnLine;
} Enc64, *Enc64Ptr, **Enc64Handle;

/*
 * state buffer for decoding
 */
typedef struct {
  short decoderState;    /* which of 4 bytes are we seeing now? */
  long invalCount;       /* how many bad chars found so far? */
  long padCount;         /* how many pad chars found so far? */
  unsigned char partial; /* partially decoded byte from/for last/next time */
  bool wasCR;            /* was the last character a carriage return? */
} Dec64, *Dec64Ptr, **Dec64Handle;

typedef enum { qpNormal, qpEqual, qpByte1 } QPStates;

typedef struct {
  QPStates state;
  unsigned char lastChar;
} DecQP, *DecQPPtr, **DecQPHandle;

typedef struct {
  short leftBytes;
  unsigned char buffer[64];
} UUState, *UUStatePtr, **UUStateHandle;

/*
 * to do the encoding/decoding
 */
long Encode64(unsigned char *bin, long len, unsigned char *sixFour,
              unsigned char *newLine, Enc64Ptr e64);
long Decode64(unsigned char *sixFour, long sixFourLen, unsigned char *bin,
              long *binLen, Dec64Ptr d64, bool text);
long EncodeQP(unsigned char *bin, long len, unsigned char *qp,
              unsigned char *newLine, long *bplp);
long DecodeQP(unsigned char *qp, long qpLen, unsigned char *bin, long *binLen,
              DecQPPtr dqp);

#ifndef TOOL

/*
 * encoder/decoder call types
 */
typedef enum { kDecodeInit, kDecodeData, kDecodeDone, kDecodeDispose } CallType;

typedef enum {
  btNotBoundary,
  btInnerBoundary,
  btOuterBoundary,
  btEndOfMessage,
  btError
} BoundaryType;

typedef struct MIMEMapStruct {
  Str31 mimetype;
  Str31 subtype;
  Str31 suffix;
  OSType creator;
  OSType type;
  unsigned long flags;
  OSType specialId;
} MIMEMap, *MIMEMapPtr, **MIMEMapHandle;

typedef struct AttMapStruct {
  MIMEMap mm;
  bool isPostScript;
  bool isText;
  bool isBasic;
  bool suppressXMac;
  Str31 shortName;
  char longName[128]; // was Str127
  Str15 uuName;
} AttMap, *AttMapPtr, **AttMapHandle;

#define ATT_MAP_NAME(a) (*a->longName ? a->longName : a->shortName)

#define mmIsText 0x8000
#define mmIsBasic 0x4000
#define mmIsAURL 0x2000
#define mmAlwaysDetach 0x1000
#define mmIgnoreXType 0x0800
#define mmDiscard 0x0400
#define mmApplySuffix 0x0200

typedef struct DecoderPB DecoderPB, *DecoderPBPtr, **DecoderPBHandle;
typedef struct MIMEState MIMEState, *MIMESPtr, **MIMESHandle;
typedef OSErr DecoderFunc(CallType callType, DecoderPBPtr decPB);
typedef BoundaryType ReadBodyFunc(TransStream stream, short refN,
                                  MIMESHandle mimeSList, char *buf, long bSize,
                                  LineReader *lr);
DecoderFunc *FindMIMEDecoder(unsigned char *encoding, bool *isExtern,
                             bool load);
DecoderFunc QPEncoder, B64Encoder, UUEncoder;
OSErr FindAttMap(FSSpecPtr spec, AttMapPtr mmp);

typedef struct {
  long offset;
  PETETextStyle style;
  long validBits;
  short sizeIndex;
} OffsetAndStyle, *OffsetAndStylePtr, **OffsetAndStyleHandle;

/*
 * for decoders and file savers
 */
struct DecoderPB {
  unsigned char *input;
  long inlen;
  unsigned char *output;
  long outlen;
  long refCon;
  bool text;
  bool noLineBreaks;
};

/*
 * MIME converter state structure
 */
struct MIMEState {
  long headerOffset;
  HeaderDHandle hdh;
  DecoderFunc *decoder;
  DecoderPB dpb;
  ReadBodyFunc *readBody;
  char boundary[128]; // was Str127
  MIMESHandle next;
  bool xDecoder;
  bool xFileSaver;
  bool isDigest;
  void **translators; // Typedef was TLMHandle
  long context;       // translation context
  short mhtmlID;
};

MIMESHandle NewMIMES(TransStream stream, HeaderDHandle hdh, bool forceMIME,
                     short context);
OSErr RecordTLMIME(FSSpecPtr spec, MIMESHandle tlMIME); // guessed handle type
OSErr RecordTL(FSSpecPtr spec, void **tl);              // guessed handle type
void DisposeMIMES(MIMESHandle msh);
#define ZapMIMES(msh)                                                          \
  do {                                                                         \
    DisposeMIMES(msh);                                                         \
    (msh) = nil;                                                               \
  } while (0);
short FindMIMECharsetLo(unsigned char *charSet, bool *found);
#define FindMIMECharset(cset) FindMIMECharsetLo(cset, nil)
void FigureMIMEFromApple(OSType creator, OSType type, unsigned char *name,
                         unsigned char *mimeType, unsigned char *mimeSub,
                         unsigned char *mimeSuffix, long *flags,
                         OSType *specialId);
bool FindMIMEMapPtr(unsigned char *type, unsigned char *subType,
                    unsigned char *name, MIMEMapPtr mmp);
unsigned char *Encode64Data(unsigned char *encoded, unsigned char *data,
                            short len);
void Encode64DataPtr(unsigned char *encoded, long *outLen, unsigned char *data,
                     short len);
#define kMIMEBoring ((MIMESHandle)(-1L))
#define READ_MESSAGE ((void *)-1)
#endif

#endif
