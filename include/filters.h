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

#ifndef FILTERS_H
#define FILTERS_H

#include "FiltDefs.h"
#include "mailbox.h"
#include "mydefs.h"
#include "toc.h"
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct FAction *FActionPtr, *FActionHandle;
typedef struct FAction {
  void **data;
  FActionHandle next;
  FilterKeywordEnum action;
} FAction;

typedef enum {
  faeInit,
  faeDraw,
  faeClose,
  faeDispose,
  faeResize,
  faeRead,
  faeWrite,
  faeButton,
  faeClick,
  faeSave,
  faeDo,
  faeFind,
  faeCursor,
  faePrint,
  faeHelp,
  faeMBWillRename,
  faeMBDidRename,
  faeMouseGoingDown,
  faeNew,
  faeListDraw,
  faeLimit
} FACallEnum;

typedef short FActionProc(FACallEnum callType, FActionHandle action, Rect *r,
                          void *dataPtr);
#define CallAction(callType, act, r, dataPtr)                                  \
  (*(FActionProc *)FATable((act)->action))(callType, act, r, dataPtr)

#define MAX_ACTIONS 5

typedef enum {
  mbmContains = 1,
  mbmNotContains,
  mbmIs,
  mbmIsnt,
  mbmStarts,
  mbmEnds,
  mbmAppears,
  mbmNotAppears,
  mbmIntersects,
  mbmNotIntersects,
  mbmIntersectsFile,
  mbmNotIntersectsFile,
  mbmRegEx,
  mbmJunkLess,
  mbmJunkMore,
  mbmLimit
} MatchEnum;

typedef enum { cjIgnore = 1, cjAnd, cjOr, cjUnless, cjLimit } ConjunctionEnum;

typedef struct {
  char header[64];
  short headerID; // 0 or FILTER_BODY|FILTER_ADDRESSEE|FILTER_ANY
  char value[128];
  MatchEnum verb;
  void **nickExpanded;
  void **nickAddresses;
  regex_t *regex;
} MatchTerm, *MTPtr, *MTHandle;

typedef struct {
  long id;
  uint32_t lastMatch;
} FilterUse, *FUPtr, *FUHandle;

typedef struct {
  char name[32];
  char transferSpec[PATH_MAX];
  bool incoming;
  bool outgoing;
  bool manual;
  MatchTerm terms[2];
  ConjunctionEnum conjunction;
  FActionHandle actions;
  FilterUse fu;
  bool kill;
} FilterRecord, *FRPtr, *FRHandle;

#define NFilters (Filters ? malloc_size(Filters) / sizeof(FilterRecord) : 0)
#define FR (*(FRHandle)Filters)

typedef struct {
  TOCType * tocH;
  short sumNum;
  bool openMailbox;
  bool openMessage;
  bool doReport;
  bool dontReport;
  bool dontUser;
  bool xferred;
  bool xferredFromIMAP;
  bool print;
  short doNotifyThing;
  char spec[PATH_MAX];
  CSpecHandle report;
  CSpecHandle mailbox;
  CSpecHandle message;
  short *sounds;
  int soundsCount;
  short notify;
  char to[16];
  char cc[16];
  char bcc[16];
  BinAddrHandle toAddresses;
  BinAddrHandle ccAddresses;
  BinAddrHandle bccAddresses;
} FilterPB, *FilterPBPtr, *FilterPBHandle;

#define afbOpenMailbox 1
#define afbOpenMessage 2
#define afbUser 1
#define afbReport 2
#define afbTrash 1
#define afbFetch 2

#endif /* FILTERS_H */
