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

/**********************************************************************
 * String utilities
 **********************************************************************/
#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include "mailbox.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MakePPtr(pstr, ptr, len)                                               \
  do {                                                                         \
    *pstr = MIN(len, sizeof(Str255) - 1);                                      \
    BMD(ptr, pstr + 1, (uint8_t)*pstr);                                        \
  } while (false)
#define CtoPPtrCpy(p, c)                                                       \
  do {                                                                         \
    short l = strlen(c);                                                       \
    MakePPtr(p, c, l);                                                         \
  } while (0)
#define StringToNum(a, b) MyStringToNum(a, b)
void MyStringToNum(PStr string, long *num);
bool AllDigits(UPtr chars, long len);
bool BeginsWith(PStr string, PStr prefix);
void CaptureHex(PStr from, PStr to);
void CaptureHexPtr(Ptr from, Ptr to, long *pLen);
bool EqualStrRes(PStr string, short resId);
UPtr ComposeRString(UPtr into, int format, ...);
bool StringSame(const char *s1, const char *s2);
unsigned char *ComposeString(unsigned char *dst, const char *fmt, ...);
#define CtoPCpy(p, c)                                                          \
  do {                                                                         \
    short l = strlen(c);                                                       \
    MakePStr(p, c, l);                                                         \
  } while (0)
bool HandleEndsWithR(Handle name, short resId);
bool EndsWith(PStr name, PStr suffix);
bool EndsWithR(PStr name, short resId);
bool EqualStrRes(PStr string, short resId);
bool StartsWithR(PStr name, short resId);
bool StartsWith(PStr name, PStr prefix);
bool StartsWithPtr(UPtr name, uint32_t len, PStr prefix);
OSErr AccuComposeR(AccuPtr a, int format, ...);
OSErr AccuCompose(AccuPtr a, PStr format, ...);
long HighBits(UPtr s, long len);
#define MixedHighBits(s, len) ((M_T1 = HighBits(s, len)), M_T1 && M_T1 < len)
#define AnyHighBits(s, len) (HighBits(s, len) > 0)
#define AllHighBits(s, len) (HighBits(s, len) == len)
void MyLowerText(UPtr buffer, long bufferSize);
void MyUpperText(UPtr buffer, long bufferSize);
#define MyLowerStr(s) MyLowerText((s) + 1, *(s))
#define MyUpperStr(s) MyUpperText((s) + 1, *(s))
/* BMD is BlockMoveData: (src, dst, len) -> memmove(dst, src, len) */
#ifndef BMD
#define BMD(src, dst, len) memmove(dst, src, len)
#endif
#undef isupper
/* Use standard ctype.h functions - don't redefine them */
#include <ctype.h>
#define IsWordOrDigit(c) (IsWordChar[c] || isdigit(c))
bool IsAllUpper(PStr s);
PStr EscapeChars(PStr string, PStr toEscape);
void EscapeInHex(PStr from, PStr to);
void FixNewlines(UPtr string, long *count);
PStr LCD(PStr s1, PStr s2);
#define MakePStr(pasc, ptr, l)                                                 \
  do {                                                                         \
    *pasc = MIN(l, sizeof(pasc) - 2);                                          \
    BMD(ptr, pasc + 1, *pasc);                                                 \
    pasc[*pasc + 1] = 0;                                                       \
  } while (0)
UPtr PCat(PStr string, PStr suffix);
#define PCatC(string, c)                                                       \
  do {                                                                         \
    unsigned char *__pstr = (unsigned char *)(string);                         \
    if (*__pstr < 255)                                                         \
      __pstr[++*__pstr] = (unsigned char)(c);                                  \
  } while (0)
#define PSCatC(string, c)                                                      \
  do {                                                                         \
    unsigned char *__pstr = (unsigned char *)(string);                         \
    if (*__pstr < sizeof(string) - 1)                                          \
      PCatC(__pstr, c);                                                        \
  } while (0)
#define PMaxCatC(string, max, c)                                               \
  do {                                                                         \
    if (max <= 0 || *(string) < max)                                           \
      PCatC(string, c);                                                        \
    else                                                                       \
      string[*string] = (unsigned char)0xC9;                                   \
  } while (0)
UPtr PCatR(PStr string, short resId);
#define PCopy(to, from) PStrCopy(to, from, 256)
PStr PStrCopy(PStr to, PStr from, short max);
PStr PCopyTrim(PStr toString, PStr fromString, short max);
#define PCSTrim(t, f) PCopyTrim(t, f, sizeof(t))
UPtr PEscCat(UPtr string, UPtr suffix, short escape, char *escapeWhat);
#define PFindSub(sub, string) PPtrFindSub(sub, (string) + 1, *(string))
bool TrimSquares(PStr s, bool multiple, bool internal);
UPtr PIndex(PStr string, char c);
UPtr IndexPtr(UPtr string, long stringLen, char c);
UPtr PRIndex(PStr string, char c);
PStr PInsert(PStr string, short size, PStr insert, UPtr spot);
PStr PInsertC(PStr string, short size, uint8_t c, UPtr spot);
// UPtr PLCat(UPtr string, long num);
UPtr PXCat(UPtr string, long num);
UPtr PXWCat(UPtr string, short num);
UPtr PPtrFindSub(PStr sub, UPtr string, long len);
#define PSCat(string, suffix) PSCat_C(string, suffix, sizeof(string))
UPtr PSCat_C(PStr string, PStr suffix, short max);
#define PSCopy PCopy
// UPtr PtoCcpy(UPtr cStr, UPtr pStr);
#define PTerminate(s) (s)[*(s) + 1] = 0
PStr PToken(PStr string, PStr token, UPtr *spotP, UPtr delims);
bool TokenPtr(Ptr string, long stringLen, Ptr *token, long *tokenLen,
              UPtr *spotP, UPtr delims);
bool PTokenPtr(Ptr string, long stringLen, Ptr token, UPtr *spotP, UPtr delims);
PStr Transmogrify(PStr toStr, short toId, PStr fromStr, short fromId);
PStr PReplace(PStr string, PStr find, PStr replace);
long PStrToNum(PStr string);
bool ReMatch(PStr string, PStr Re);
void RemoveParens(UPtr string);
int strincmp(UPtr s1, UPtr s2, short n);
int striscmp(UPtr s1, UPtr s2);
int strscmp(UPtr s1, UPtr s2);
#define ExactlyEqualString(s1, s2) (*(s1) == *(s2) && !memcmp(s1, s2, *(s1)))
UPtr Tokenize(UPtr string, int size, UPtr *start, UPtr *end, UPtr delims);
PStr TrimInitialWhite(PStr s);
PStr TrimInternalWhite(PStr s);
#define TrimAllWhite(s) TrimInitialWhite(TrimWhite(s))
bool TrimPrefix(UPtr string, UPtr prefix);
bool TrimReLo(PStr string, PStr re);
bool TrimRe(PStr string, bool squares);
PStr TrimWhite(PStr s);
PStr CollapseLWSP(PStr s);
UPtr VaComposeRString(UPtr into, short format, va_list args);
#ifndef VaComposeStringDouble
UPtr VaComposeStringDouble(UPtr into, int maxInto, UPtr format, va_list args,
                           UPtr into2, int maxInto2, UPtr format2);
#endif
#define VaComposeString(i, f, a)                                               \
  VaComposeStringDouble((i), -1, (f), (a), nil, -1, nil)
bool StringSame(const char *s1, const char *s2);
long StringComp(PStr s1, PStr s2);
bool Tr(Handle text, UPtr fromS, UPtr toS);
bool TrLo(UPtr text, long len, UPtr fromS, UPtr toS);
#define PTr(string, from, to) TrLo((string) + 1, *(string), from, to)
PStr PStripChar(PStr string, uint8_t c);
long StripChar(Ptr string, long len, uint8_t c);
PStr Uncomma(PStr name);
OSErr MyLowercaseText(UPtr text, long len);
void UTF8To88591(Ptr inStr, long inLen, Ptr outStr, long *outLen);
PStr UTF8ToMac(PStr str);
short CharWidthInFont(uint8_t c, short font, short size);
bool IsAllWhitePtr(UPtr s, long len);
#define IsAllWhite(s) IsAllWhitePtr(s + 1, *s)
bool IsAllLWSPPtr(UPtr s, long len);
#define IsAllLWSP(s) IsAllLWSPPtr(s + 1, *s)
bool PtrPtrMatchLWSP(Ptr lookFor, short lookLen, Ptr text, uint32_t textLen,
                     bool atStart, bool atEnd);
#define PPMatchLWSP(lf, tx, s, e)                                              \
  PtrPtrMatchLWSP((lf) + 1, *(lf), (tx) + 1, *(tx), s, e)
#define PPtrMatchLWSP(lf, tx, txLen, s, e)                                     \
  PtrPtrMatchLWSP((lf) + 1, *(lf), tx, txLen, s, e)
#include <glib.h>

char *ensure_utf8(const char *text);

UPtr ShortVersString(short vers, UPtr versionStr);
PStr InfiniteString(PStr s, short size);
bool ItemFromResAppearsInStr(short resID, PStr string, UPtr delims);
bool StrIsItemFromRes(PStr string, short resID, UPtr delims);
PStr StripLeadingItems(PStr string, short resID);
PStr StripTrailingITems(PStr string, short resID);
bool EndsWithItem(PStr string, short resID);

/* Pascal string -> C string (static buffer, NOT thread safe) */
static inline const char *pstr_to_c(const unsigned char *pstr) {
  static char buf[256];
  if (!pstr || pstr[0] == 0) return "";
  int len = pstr[0];
  if (len > 255) len = 255;
  memcpy(buf, pstr + 1, len);
  buf[len] = '\0';
  return buf;
}

#endif /* STRINGUTIL_H */
