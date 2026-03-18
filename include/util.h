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

#ifndef UTIL_H
#define UTIL_H

#include "mydefs.h"
#include <stdbool.h>
#include <stdint.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * some linked-list macros
 **********************************************************************/
/* Linked-list macros — direct pointer style (item->next, not (*item)->next).
 * cast is e.g. (MyType *) and is applied to void* traversal pointers. */
#define LL_Remove(head, item, cast)                                            \
  do {                                                                         \
    if ((head) == (item))                                                      \
      (head) = (head)->next;                                                   \
    else {                                                                     \
      void *_ll = (head);                                                      \
      while (_ll && ((cast _ll)->next != (item)))                              \
        _ll = ((cast _ll)->next);                                              \
      if (_ll) ((cast _ll)->next) = (item)->next;                              \
    }                                                                          \
  } while (0)

#define LL_Push(head, item)                                                    \
  do { (item)->next = (head); (head) = (item); } while (0)
#define LL_Queue(head, item, cast)                                             \
  do {                                                                         \
    if (head) {                                                                \
      void *_ll = (head);                                                      \
      while (((cast _ll)->next))                                               \
        _ll = ((cast _ll)->next);                                              \
      ((cast _ll)->next) = (item);                                             \
    } else                                                                     \
      (head) = (item);                                                         \
  } while (0)
#define LL_Last(head, item)                                                    \
  do {                                                                         \
    (item) = (head);                                                           \
    while ((item)->next)                                                       \
      (item) = (item)->next;                                                   \
  } while (0)
#define LL_Parent(head, item, parent)                                          \
  do {                                                                         \
    (parent) = (head);                                                         \
    while ((parent) && (parent)->next != (item))                               \
      (parent) = (parent)->next;                                               \
  } while (0)

/************************************************************************
 * Associative Array stuff
 ************************************************************************/
typedef struct AssocArray {
  short keySize;  /* length of keys */
  short dataSize; /* length of data blocks */
} AssocArray, *AAPtr, *AAHandle;
AAHandle AANew(short keySize, short dataSize);
int AAAddItem(AAHandle aa, bool replace, char *key, char *data);
int AAAddResItem(AAHandle aa, bool replace, short keyId, char *data);
int AADeleteKey(AAHandle aa, char *key);
int AAFetchData(AAHandle aa, char *key, char *data);
int AAFetchResData(AAHandle aa, short keyId, char *data);
int AAFetchIndData(AAHandle aa, short index, char *data);
int AAFetchIndKey(AAHandle aa, short index, char *key);
short AACountItems(AAHandle aa);
#define AAZap(aaH) free(aaH)
short AAFindKey(AAHandle aa, char *key);

typedef struct {
  short id;
  uLong used;
  uLong persId;
  unsigned char string[256];
} StringCacheEntry, *SCPtr, *SCHandle;

/**********************************************************************
 * replace RemoveResource calls to fix OS bug
 **********************************************************************/
// #define RmveResource(h) MyRemoveResource(h)	* Already defined to
// RemoveResource
#define RemoveResource(h) MyRemoveResource(h)

/**********************************************************************
 * declarations for functions in util.c
 **********************************************************************/
typedef struct {
  long elSize;
  short elCount;
  short capacity;
} StackType_Util, *StackPtr_Util, **StackHandle_Util;
// Stack functions - declared in mailbox.h with different signatures
int StackInit(long size, StackHandle *stack);
int StackPush(void *what, StackHandle *pStack);
int StackPop(void *into, StackHandle stack);
int StackItem(void *into, short item, StackHandle stack);
int StackQueue(void *what, StackHandle *pStack);
int StackTop(void *into, StackHandle stack);
void StackCompact(StackHandle *pStack);
short StackStringFind(char *find, StackHandle stack);
void SCClear(short theId);
short CountStrnRes(unsigned char * resH);
void Rude(void);
void CheckNone(MenuHandle mh);
bool OnBatteries(void);
// void WriteZero(void *pointer,long size);
// #define Zero(x) WriteZero(&(x),sizeof(x))
void MacInitialize(int masterCount, long ensureStack);
// unsigned char * GetRString(unsigned char * theString,short theIndex);
int GetFontID(char *theName);
bool GrabEvent(void *theEvent);
short SubmenuId(MenuHandle mh, short item);
bool MyOSEventAvail(short mask, void *event);
#define OSEventAvail MyOSEventAvail
// struct Accumulator moved to mydefs.h
// Accumulator typedefs moved to mailbox.h to avoid conflicts
// typedef struct Accumulator *AccuPtr, *AccuHandle;
#define ACCU_TYPEDEF_DONE
int AccuInit(AccuPtr a);
void AccuInitWithPtr(AccuPtr a, char *buf, long size);
void AccuTrim(AccuPtr a);
int AccuAddTrPtr(AccuPtr a, void *bytes, long len, char *from, char *to);
int AccuAddTrHandle(AccuPtr a, void *data, char *from, char *to);
int AccuAddPtrVoid(AccuPtr a, void *bytes, long len);
#define AccuAddPtr AccuAddPtrVoid
int AccuAddPtrB64(AccuPtr a, void *bytes, long len);
int AccuAddHandle(AccuPtr a, void *data);
int AccuAddHandleToPtr(AccuPtr a, char *data, long size);
int AccuAddLong(AccuPtr a, uLong longVal);
#define AccuAddHandleToStr(a, s) AccuAddHandleToPtr((a), (s), strlen((const char *)(s)))
int AccuAddChar(AccuPtr a, unsigned char c);
int AccuInsertPtr(AccuPtr a, char *bytes, long len, long offset);
int AccuInsertChar(AccuPtr a, unsigned char c, long offset);
int AccuAddFromHandle(AccuPtr a, void *data, long offset, long len);
long AccuFTell(AccuPtr a, short refN);
int AccuFSeek(AccuPtr a, short refN, long fromStart);
long Atoi(char *s);
uint32_t ATouint32_t(char *s);
int AccuAddRes(AccuPtr a, short res);
int AccuWrite(AccuPtr a, short refN);
#define AccuAddStr(a, s) AccuAddPtr(a, (s), strlen((const char *)(s)))
#define AccuAddStrB64(a, s) AccuAddPtrB64(a, (s), strlen((const char *)(s)))
#define AccuToStr(a, s)                                                        \
  do {                                                                         \
    long _ats_len = min(255, (a)->offset);                                     \
    BMD((a)->data, (s), _ats_len);                                             \
    (s)[_ats_len] = '\0';                                                      \
  } while (0)
int AccuStrip(AccuPtr a, long num);
long AccuFindPtr(AccuPtr a, const char *stuff, short len);
long AccuFindLong(AccuPtr a, uLong theLong);
int AccuAddSortedLong(AccuPtr a, long addVal);
short DecodeB64Accu(AccuPtr a, bool isText);
void CheckFontSize(int menu, int size, bool check);
void CheckFont(int menu, int fontID, bool check);
void OutlineFontSizes(int menu, int fontID);
int GetLeading(int fontID, int fontSize);
int GetWidth(int fontID, int fontSize);
int GetDescent(int fontID, int fontSize);
int GetAscent(int fontID, int fontSize);
bool IsFixed(int fontID, int fontSize);
void AwaitKey(void);
void AddPResource(char *, int, long, int, char *);
void ChangePResource(char *theData, int theLength, long theType,
                     int theID, char *theName);
/* GetRLong declared in mailbox.h with short id */
uint32_t GetRuint32_t(int index);
int ResourceCpy(short toRef, short fromRef, long type, int id);
void WhiteRect(Rect *r);
void DrawTruncString(char *string, int len);
// int CalcTextTrunc(unsigned char * text,short length,short width,GrafPtr
// port); #define CalcTrunc(text,width,port)
// CalcTextTrunc((text)+1,*(text),width,port)
int WannaSave(MyWindowPtr win);
void ButtonFit(void *button);
uint32_t GestaltBits(uint32_t selector);
void GetPassStuff(char *persName, char *uName,
                  char *hName);
#ifdef KERBEROS
int GetPassword(void);
#else
int GetPassword(char *personality, char *userName, char *serverName,
                char *word, int size, short prompt);
#endif
void CenterRectIn(Rect *inner, Rect *outer);
void TopCenterRectIn(Rect *inner, Rect *outer);
void BottomCenterRectIn(Rect *inner, Rect *outer);
// void ThirdCenterRectIn(Rect *inner, Rect *outer);

short MyUniqueID(uint32_t type);
bool HasDragManager();
typedef void *DragReference;
// typedef void *RgnHandle;
typedef uint32_t FlavorFlags;
#ifndef RGBCOLOR_DEFINED
#define RGBCOLOR_DEFINED
typedef struct RGBColor {
  unsigned short red, green, blue;
} RGBColor;
#endif
typedef struct DateTimeRec {
  short year, month, day, hour, minute, second, dayOfWeek;
} DateTimeRec;
int FinderDragVoodoo(DragReference drag);
// typedef enum {Stop, Note, Caution, Normal} AlertEnum;
void MyAppendMenu(MenuHandle menu, char *name);
void MyInsMenuItem(MenuHandle menu, char *name, short afterItem);
void MySetItem(MenuHandle menu, short item, char *itemStr);
char *MyGetItem(MenuHandle menu, short item, char *name);
int CopyMenuItem(MenuHandle fromMenu, short fromItem, MenuHandle toMenu,
                 short toItem);
short CurrentModifiers(void);
void SpecialKeys(void *event);
short FindItemByName(MenuHandle menu, char *name);
short BinFindItemByName(MenuHandle menu, char *name);
void AttachHierMenu(short menu, short item, short hierId);
void *NuDHTempBetter(void *data, long size);
bool DirtyKey(long keyAndChar);
long RemoveChar(unsigned char c, char *text, long size);
long RemoveCharHandle(unsigned char c, unsigned char * text);

char *GetRStr(char *string, short id);
char *LocalDateTimeStr(char *string);
char *LocalDateTimeShortStr(char *s);
// (jp) Universal Headers 3.4 now contains a structure named "LocalDateTime"
/* Map public name to internal implementation and use fixed-width type */
#define LocalDateTime MyLocalDateTime
uint32_t MyLocalDateTime(void);
uLong GMTDateTime(void);
long MyMenuKeyLo(void *event, bool enable);
#define MyMenuKey(e) MyMenuKeyLo((e), true)
#define UnadornMessage(event) UnadornKey((event)->message, (event)->modifiers)
long UnadornKey(long message, short modifiers);
char *ChangeStrn(short resId, short num, char *string);
typedef void *RgnHandle;

int MyTrackDrag(DragReference drag, void *event, RgnHandle rgn);
int MySetDragItemFlavorData(DragReference drag, short item, uint32_t type,
                            void *data, long len);
short DragOrMods(DragReference drag);
bool RecountStrn(short resId);
short CountStrn(short resId);
void NukeMenuItemByName(short menuId, char *itemName);
void RenameItem(short menuId, char *oldName, char *newName);
bool HasSubmenu(MenuHandle mh, short item);
int ComposeRTrans(TransStream stream, int format, ...);
bool SetGreyControl(void *button, bool shdBeGrey);
bool IsAUX(void);
long ZoneSecs(void);
short MyCountDragItems(DragReference drag);
short MyCountDragItemFlavors(DragReference drag, short item);
uint32_t MyGetDragItemFlavorType(DragReference drag, short item, short flavor);
FlavorFlags MyGetDragItemFlavorFlags(DragReference drag, short item,
                                     short flavor);
bool MyDragHas(void *drag, short item, uint32_t type);
int MyGetDragItemData(void *drag, short item, uint32_t type, void **data);
void NOOP(void);
bool WNE(short eventMask, void *event, long sleep);
long RoundDiv(long quantity, long unit);
void TransLitString(char *string);
void TransLit(char *string, long len, char *table);
// void TransLitRes(unsigned char * string, long len, short resId);
long TZName2Offset(CStr zoneName);
#ifdef DEBUG
#define UseResFile MyUseResFile
void MyUseResFile(short refN);
#endif
#undef InvalidatePasswords
void InvalidatePasswords(bool pwGood, bool auxpwGood, bool all);
void InvalidateCurrentPasswords(bool pwGood, bool auxpwGood);
// bool MiniEventsLo(long sleepTime, uLong mask);
#define MiniEvents() MiniEventsLo(0, MINI_MASK)
short FindSTRNIndex(short resId, char *string);
short FindSTRNIndexRes(unsigned char * resH, char *string);
short FindSTRNSubIndex(short resId, char *string);
short FindSTRNSubIndexRes(unsigned char * resH, char *string);
void *Event2Window(void *event);
char *Long2Hex(char *hex, long aLong);
char *Bytes2Hex(char *bytes, long size, char *hex);
int Hex2Bytes(char *hex, long size, char *bytes);
/* void *NuHTempOK(long size); */
/* void *NuHTempBetter(long size); */
// void *NewIOBHandle(long min, long max);
long AFPopUpMenuSelect(MenuHandle mh, short top, short left, short item);
bool GetTableCName(short tid, char *name);
bool GetTableID(char *name, short *tid);
bool EventPending(void);
void ShowDragRectHilite(DragReference drag, Rect *r, bool inside);
char *WeekDay(char *string, long secs);
void TimeString(long secs, bool wantSeconds, char *str, void *intlHandle);
int ZapResourceLo(uint32_t type, short id, bool one);
#define ZapResource(x, y) ZapResourceLo(x, y, false)
#define Zap1Resource(x, y) ZapResourceLo(x, y, true)
/* ZapSettingsResource: same as ZapResource in GTK port (no separate settings
 * file) */
#define ZapSettingsResource(x, y) ZapResourceLo(x, y, false)

/* Alert with printf-style string resource formatting */
void Aprintf(short alertType, short noteType, short strn, ...);

/* Personality audit logging */
void AuditPersCreate(uint32_t hash);
void AuditPersDelete(uint32_t persId);
void AuditPersRename(uint32_t oldId, uint32_t newHash);

/* String override (per-personality string resource override) */
void SetStrOverride(short strn, const char *str);
void GetPrefNoDominant(char *buf, short prefId);

/* void *management */
void ReleaseResource(void **h);
void SetHandleBig(void **h, long size);
#define ControlIsGrey(cntl) (GetControlHilite(cntl) == 255)
void AddMyResource(void *h, uint32_t type, short id, ConstStr255Param name);
#define CurrentPSN(psn)                                                        \
  (((psn)->highLongOfPSN = 0), ((psn)->lowLongOfPSN = kCurrentProcess), (psn))
long CountChars(void *text, unsigned char c);
long CountCharsPtr(char *ptr, long size, unsigned char c);
int HandleLinebreaks(void *text, long ***breaks, short inWidth);

short MenuWidth(MenuHandle mh);
//#define IsColorWin(win) \
//	(ThereIsColor && \
//	 (((GrafPtr)(win))->portBits.rowBytes & 0xC000) && \
//   ((**((CGrafPtr)(win))->portPixMap).pixelSize > 1))
bool IsColorWin(void *winWP);
#define PurgeIfClean(h)                                                        \
  do {                                                                         \
    UL(h);                                                                     \
    if (!(GetResAttrs((void *)h) & resChanged))                                \
      HPurge((void *)h);                                                       \
  } while (0)
typedef struct {
  void **textH;
  char *textP;
  long len;
  long lineBegin;
  long lineEnd;
  short partial;
} WrapDescriptor, WrapPtr;
void PlayNamedSound(char *name);
void PlaySoundId(short id);
short FindMenuByName(char *name);
RGBColor *GetItemColor(short menu, short item, RGBColor *color);
bool IsHexDig(unsigned char c);
bool IsEnabled(short menu, short item);
bool SafeToAllocate(long size);
RGBColor *GetRColor(RGBColor *color, int index);
RGBColor *GetRTextColor(RGBColor *color, int index);
int AddLf(void *text);
void *NuDHTempOK(void *data, long size);
char *Color2String(char *string, RGBColor *color);
void InitWrap(WrapPtr wp, void **textH, char *textP, long len,
              long offset, long lastLen);
short Wrap(WrapPtr wp);
bool IsPowerNoVM(void);
void SetHiliteMode(void);
void SetItemReducedIcon(MenuHandle menu, short item, short iconid);
void *PStr2Handle(char *string);
long ScriptVar(short selector);
bool MyWaitMouseMoved(Point pt, bool honorControl);
// void *ZeroHandle(void *hand);
void SetItemR(MenuHandle menu, short item, short id);
bool IsVICOM(void);
int MyRemoveResource(void *h);
short ShortCompare(short value1, short value2);
short DateCompare(DateTimeRec *date1, DateTimeRec *date2);
short TimeCompare(DateTimeRec *date1, DateTimeRec *date2);
short GetOSVersion(void);
#define HaveTheDiseaseCalledOSX HaveOSX
bool HaveOSX(void);
void AddSoundsToMenu(MenuHandle mh);
void PlaySoundIdle(void);

#ifdef DEBUG
void SetBalloons(bool on);
#else
#define SetBalloons HMSetBalloons
#endif
#define Pause(t)                                                               \
  do {                                                                         \
    long tk = TickCount() + t;                                                 \
    while (TickCount() < tk) {                                                 \
      WNE(0, nil, tk - TickCount());                                           \
    }                                                                          \
  } while (0)
#define DIR_MASK 8 /* mask to use to test file attrib for directory bit */
/* #define OPTIMAL_BUFFER (64 K) */ /* file buffer size */
#define SAME_COLOR(c1, c2)                                                     \
  ((c1).red == (c2).red && (c1).green == (c2).green && (c1).blue == (c2).blue)

#define RectHi(r) ((r).bottom - (r).top)
#define RectWi(r) ((r).right - (r).left)

#define optSpace 0xca
#define enterChar 0x03
#define escChar 0x1b
#define clearChar 0x1b
#define escKey 0x35
#define clearKey 0x47
#define delChar 0x7f
#define backSpace 0x08
#define returnChar 0x0d
/* #define bulletChar 0xa5 */
#ifndef tabChar
#define tabChar 0x09
#endif
#define leftArrowChar 0x1c
#define rightArrowChar 0x1d
#define upArrowChar 0x1e
#define downArrowChar 0x1f
#define homeChar 0x01
#define endChar 0x04
#define helpChar 0x05
#define pageUpChar 0x0b
#define pageDownChar 0x0c
#define undoKey 0x7a
#define cutKey 0x78
#define copyKey 0x63
#define pasteKey 0x76
#define clearKey 0x47
#define betaChar 0xa7
#define diamondChar ((unsigned char)0xD7)
#define lowerDelta ((unsigned char)0x9F)
#define nbSpaceChar 0xca
#define lowerOmega ((unsigned char)0xBE)
#define ellipsesChar 0xC9
#define lessThanOrEqualToChar 0xB2
#define optionCommaChar lessThanOrEqualToChar
#define periodChar 0x2E

enum { smScriptSmallSysFondSize = 0x1243 };

/* #define MINI_MASK (everyEvent & ~(highLevelEventMask | keyDownMask | \
                ((InAThread() || !ModalWindow) ? (mDownMask | mUpMask) : 0))) */

/* Removed Mac OS 9 Speed Doubler NoSLGet wrappers */
#endif
