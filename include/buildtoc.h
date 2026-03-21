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

#ifndef BUILDTOC_H
#define BUILDTOC_H

#include "mailbox.h"
#include "toc.h"
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "lineio.h"

/* Function prototypes */
void BeautifySum(MacmbxMsgSum * sum);
MacmbxTOC * BuildTOC(const char *path);
MacmbxTOC * BuildTOC_Path(const char *path);
MacmbxTOC * RebuildTOC(const char *path, MacmbxTOC * oldTocH, bool resource,
                     bool tempToc);
int ReadSum(MacmbxMsgSum * sum, bool isOut, LineIOP lip, bool lookEnvelope);
int SumToFrom(MacmbxMsgSum * sum, char *fromLine);
void CopyHeaderLine(char *to, int size, char *from);
long FindTOCSpot(MacmbxTOC * tocH, long length);
void BeautifyFrom(char *fromStr);
uint32_t BeautifyDate(char *dateStr, long *zoneSecs);
uint32_t UnixDate2Secs(const char *date);
void BeautifySubj(char *subject, short size);
bool IsFromLine(char *line);
bool IsBulk(char *line);
void GleanFrom(char *line, MacmbxMsgSum * sum);
short MonthNum(const char *cp);
long CStr2Zone(const char *s);
char *TrimWrap(char *str, int openC, int closeC);
char *TrimNonWord(char *str);
short SalvageTOC(MacmbxTOC * oldToc, MacmbxTOC * newToc);

/* Unicode / UTF-8 helpers (GLib-based portable implementations) */
bool HasUnicode(void);
typedef unsigned char *BytePtr;
typedef unsigned long ByteCount;
ByteCount GoodUTF8Len(BytePtr utf8, ByteCount bufLen);
OSStatus HeaderToUTF8(char *head);
#define TrimUTF8(s) do { \
  size_t _tlen = strlen((const char *)(s)); \
  size_t _good = GoodUTF8Len((unsigned char *)(s), _tlen); \
  (s)[_good] = '\0'; \
} while(0)

#endif
