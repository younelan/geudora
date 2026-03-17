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

#ifndef UUDECODE_H
#define UUDECODE_H

#include <stdbool.h>
#include "mydefs.h"
#include "trans.h"
#include "fileutil.h"
#include "mime.h"
#include "threading.h"
#include "StringUtil.h"
#include "StringDefs.h"
#include "hexbin.h"
#include "pop.h"
#include "binhex.h"
#include "Globals.h"
#include "sendmail.h"
#ifndef MACOSXSUCKS_TYPE_LIST
#define MACOSXSUCKS_TYPE_LIST 'XskT'
#endif
#ifndef MACOSXSUCKS_CREATOR_LIST
#define MACOSXSUCKS_CREATOR_LIST 'XskC'
#endif

#ifndef smSystemScript
#define smSystemScript 0
#endif
#ifndef kTextEncodingUS_ASCII
#define kTextEncodingUS_ASCII 0x0600
#endif

typedef enum {
  NotAb,
  AbHeader,
  AbFinfo,
  AbFDates,
  AbName,
  AbResFork,
  AbDataFork,
  AbSkip,
  AbExcess,
  AbDone,
  AbJustData,
  AbSLimit
} AbStates;

/* `name` is textual — use portable `char *` */
bool BeginAbomination(char *name, HeaderDHandle hdh);
short SaveAbomination(char *text, long size);
bool IsAbLine(char *text, long size, HeaderDHandle hdh);
long UURightLength(char *text, long size);
bool ConvertUUSingle(short refN, char *buf, long *size, POPLineType lineType,
                     long estSize, MIMEMapPtr hintMM, HeaderDHandle hdh);
bool ConvertSingle(short refN, char *buf, long size);
/* `specPath` is a POSIX file path to the file to send */
int SendSingle(TransStream stream, const char *specPath, bool dataToo, AttMapPtr amp);
int SendDouble(TransStream stream, const char *specPath, long flags, short tableID, AttMapPtr amp);
int SendUU(TransStream stream, const char *specPath, AttMapPtr amp);
int SendDataFork(TransStream stream, const char *specPath, long flags, short tableID, AttMapPtr amp);

#endif
