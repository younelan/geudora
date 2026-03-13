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

#ifndef SCHIZO_H
#define SCHIZO_H

#include "mydefs.h"

/* Forward declarations */
typedef struct mstruct **MessHandle;
typedef struct MyWindow *MyWindowPtr;

#define PERS_VERS 0

/* MailboxNode forward declaration - needed for IMAP support */
typedef struct MailboxNode MailboxNode, *MailboxNodePtr, **MailboxNodeHandle;

typedef struct Personality Personality, *PersPtr, **PersHandle;
struct Personality {
  uint32_t persId;
  short version;
  short resId;
  short resEnd;
  char name[32];
  char password[32];
  char secondPass[32];
  bool popSecure;
  bool dirty;
  PersHandle next;
  unsigned int doMeNow : 1;
  unsigned int checked : 1;
  unsigned int checkMeNow : 1;
  unsigned int sendMeNow : 1;
  unsigned int autoCheck : 1;
  unsigned int noUIDL : 1;
  unsigned int uupcIn : 1;
  unsigned int uupcOut : 1;
  unsigned int imapRefresh : 1;
  unsigned int otherFlags : 23;
  long sendQueue;
  uint32_t checkTicks;
  uint32_t ivalTicks;
  MailboxNodeHandle mailboxTree;
  void *proxy;
};

#define PERS_RTYPE 'Pers'

void InitPersonalities(void);
void DisposePersonalities(void);
uint32_t PersCheckTicks(void);
void PersSkipNextCheck(void);
bool PersAnyPasswords(void);
int32_t PersSaveAll(void);
int32_t PersSave(PersHandle pers);
int32_t PersSavePw(PersHandle pers);
int32_t PersFillPw(PersHandle pers, uint32_t whichOnes);
#define kFillRegularPw 1
#define kFillSecondPw 2
PersHandle PersNew(void);
PersHandle FindPersById(uint32_t persId);
PersHandle FindPersByName(unsigned char *name);
uint32_t PersType(uint32_t theType, PersHandle pers);
#ifdef APPLE_EVENTS
OSErr SetPersProperty(AEDescPtr token, AEDescPtr descP);
OSErr GetPersProperty(AEDescPtr token, AppleEvent *reply, long refCon);
OSErr AECreatePersonality(DescType theClass, AEDescPtr inContainer,
                          AppleEvent *event, AppleEvent *reply);
OSErr AEPersObj(PersHandle pers, AEDescPtr obj);
OSErr AESetPers(TOCType * tocH, short sumNum, AEDescPtr descP);
#endif
int32_t PersDelete(PersHandle pers);
long PersCount(void);
int32_t PersSetName(PersHandle pers, unsigned char *name);
void PushPers(PersHandle newCur);
void PopPers(void);
int32_t SetPers(TOCType * tocH, short sumNum, PersHandle pers, bool stationery);
void CheckPers(MyWindowPtr win, bool all);
short Pers2Index(PersHandle goalPers);
PersHandle Index2Pers(short n);
void PersSetAutoCheck(void);
PersHandle PersChoose(unsigned char *prompt);
void PersZapResources(uint32_t type, short resEnd);
void SetBGColorsByPers(MessHandle messH);
bool IsIMAPPers(PersHandle pers);

#define PERS_TYPE(t, r) ((r) ? (((t) & 0xffff0000) | (r)) : (t))
#define PERS_POPD_TYPE(p)                                                      \
  (((p) && ((p) != PersList)) ? (PERS_TYPE(OLD_POPD_TYPE, (*(p))->resEnd))     \
                              : OLD_POPD_TYPE)
#define CUR_POPD_TYPE PERS_POPD_TYPE(CurPers)
#define CUR_STR_TYPE                                                           \
  ((CurPers && (*CurPers)->persId) ? PERS_TYPE('STR ', (*CurPers)->resEnd)     \
                                   : 'STR ')

// Global variables and macros
/* CurPers is always a macro from threading.h: (CurThreadGlobals->tCurPers) */
/* PersList is always a macro from threading.h: (CurThreadGlobals->tPersList) */
/* OLD_POPD_TYPE is a macro in MyRes.h: 'popd' */

/* PERS_FORCE and TS_TO_PERS are defined in pop.h */

#endif