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
 * String utilities - ALL FUNCTIONS NOW USE C STRINGS (null-terminated)
 * No more Pascal string format (length byte at [0]).
 **********************************************************************/
#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include "mailbox.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <glib.h>

/* CtoPPtrCpy: C string copy (was C-to-Pascal, now just copy) */
#define CtoPPtrCpy(p, c) g_strlcpy((char *)(p), (c), sizeof(p))
#define StringToNum(a, b) MyStringToNum(a, b)
void MyStringToNum(char *string, long *num);
bool AllDigits(char *chars, long len);
bool BeginsWith(char *string, char *prefix);
void CaptureHex(char *from, char *to);
void CaptureHexPtr(char * from, char * to, long *pLen);
bool EqualStrRes(char *string, short resId);
char *ComposeRString(char *into, int format, ...);
bool StringSame(const char *s1, const char *s2);
char *ComposeString(char *dst, const char *fmt, ...);
/* CtoPCpy: C string copy (was C-to-Pascal, now just copy) */
#define CtoPCpy(p, c) g_strlcpy((char *)(p), (c), sizeof(p))
bool HandleEndsWithR(void *name, short resId);
bool EndsWith(char *name, char *suffix);
bool EndsWithR(char *name, short resId);
bool StartsWithR(char *name, short resId);
bool StartsWith(char *name, char *prefix);
bool StartsWithPtr(char *name, uint32_t len, char *prefix);
int AccuComposeR(AccuPtr a, int format, ...);
int AccuCompose(AccuPtr a, char *format, ...);
long HighBits(char *s, long len);
#define MixedHighBits(s, len) ((M_T1 = HighBits(s, len)), M_T1 && M_T1 < len)
#define AnyHighBits(s, len) (HighBits(s, len) > 0)
#define AllHighBits(s, len) (HighBits(s, len) == len)
void MyLowerText(char *buffer, long bufferSize);
void MyUpperText(char *buffer, long bufferSize);
#define MyLowerStr(s) MyLowerText((char *)(s), strlen((const char *)(s)))
#define MyUpperStr(s) MyUpperText((char *)(s), strlen((const char *)(s)))
/* BMD is BlockMoveData: (src, dst, len) -> memmove(dst, src, len) */
#ifndef BMD
#define BMD(src, dst, len) memmove(dst, src, len)
#endif
#undef isupper
/* Use standard ctype.h functions - don't redefine them */
#include <ctype.h>
#define IsWordOrDigit(c) (IsWordChar[c] || isdigit(c))
bool IsAllUpper(char *s);
char *EscapeChars(char *string, char *toEscape);
void EscapeInHex(char *from, char *to);
void FixNewlines(char *string, long *count);
char *LCD(char *s1, char *s2);
char *PCat(char *string, char *suffix);
/* PCatC: append a single character to C string */
#define PCatC(string, c)                                                       \
  do {                                                                         \
    char *__pstr = (char *)(string);                                           \
    size_t __plen = strlen(__pstr);                                            \
    if (__plen < 255) {                                                        \
      __pstr[__plen] = (c);                                                    \
      __pstr[__plen + 1] = '\0';                                              \
    }                                                                          \
  } while (0)
#define PSCatC(string, c) PCatC(string, c)
#define PMaxCatC(string, max, c) PCatC(string, c)
char *PCatR(char *string, short resId);
char *PCopyTrim(char *toString, char *fromString, short max);
#define PCSTrim(t, f) PCopyTrim(t, f, sizeof(t))
char *PEscCat(char *string, char *suffix, short escape, char *escapeWhat);
/* PFindSub: find sub in string (both C strings now) */
#define PFindSub(sub, string) PPtrFindSub(sub, (string), strlen((const char *)(string)))
bool TrimSquares(char *s, bool multiple, bool internal);
char *PIndex(char *string, char c);
char *IndexPtr(char *string, long stringLen, char c);
char *PRIndex(char *string, char c);
char *PInsert(char *string, short size, char *insert, char *spot);
char *PInsertC(char *string, short size, uint8_t c, char *spot);
void PLCat(char *dst, long num);
char *PXCat(char *string, long num);
char *PXWCat(char *string, short num);
char *PPtrFindSub(char *sub, char *string, long len);
/* PTerminate: no-op for C strings (already null-terminated) */
#define PTerminate(s) ((void)0)
char *PToken(char *string, char *token, char **spotP, char *delims);
bool TokenPtr(char * string, long stringLen, char * *token, long *tokenLen,
              char **spotP, char *delims);
bool PTokenPtr(char * string, long stringLen, char * token, char **spotP, char *delims);
char *Transmogrify(char *toStr, short toId, char *fromStr, short fromId);
char *PReplace(char *string, char *find, char *replace);
long PStrToNum(char *string);
bool ReMatch(char *string, char *Re);
void RemoveParens(char *string);
int strincmp(char *s1, char *s2, short n);
int striscmp(char *s1, char *s2);
int strscmp(char *s1, char *s2);
#define ExactlyEqualString(s1, s2) (strcmp((const char *)(s1), (const char *)(s2)) == 0)
char *Tokenize(char *string, int size, char **start, char **end, char *delims);
char *TrimInitialWhite(char *s);
char *TrimInternalWhite(char *s);
#define TrimAllWhite(s) TrimInitialWhite(TrimWhite(s))
bool TrimPrefix(char *string, char *prefix);
bool TrimReLo(char *string, char *re);
bool TrimRe(char *string, bool squares);
char *TrimWhite(char *s);
char *CollapseLWSP(char *s);
char *VaComposeRString(char *into, short format, va_list args);
#ifndef VaComposeStringDouble
char *VaComposeStringDouble(char *into, int maxInto, char *format, va_list args,
                            char *into2, int maxInto2, char *format2);
#endif
#define VaComposeString(i, f, a)                                               \
  VaComposeStringDouble((i), -1, (f), (a), nil, -1, nil)
long StringComp(char *s1, char *s2);
bool Tr(void *text, char *fromS, char *toS);
bool TrLo(char *text, long len, char *fromS, char *toS);
#define PTr(string, from, to) TrLo((char *)(string), strlen((const char *)(string)), from, to)
char *PStripChar(char *string, uint8_t c);
long StripChar(char * string, long len, uint8_t c);
char *Uncomma(char *name);
int MyLowercaseText(char *text, long len);
void UTF8To88591(char * inStr, long inLen, char * outStr, long *outLen);
char *UTF8ToMac(char *str);
short CharWidthInFont(uint8_t c, short font, short size);
bool IsAllWhitePtr(char *s, long len);
#define IsAllWhite(s) IsAllWhitePtr((char *)(s), strlen((const char *)(s)))
bool IsAllLWSPPtr(char *s, long len);
#define IsAllLWSP(s) IsAllLWSPPtr((char *)(s), strlen((const char *)(s)))
bool PtrPtrMatchLWSP(char * lookFor, short lookLen, char * text, uint32_t textLen,
                     bool atStart, bool atEnd);
#define PPMatchLWSP(lf, tx, s, e)                                              \
  PtrPtrMatchLWSP((char *)(lf), strlen((const char *)(lf)), (char *)(tx), strlen((const char *)(tx)), s, e)
#define PPtrMatchLWSP(lf, tx, txLen, s, e)                                     \
  PtrPtrMatchLWSP((char *)(lf), strlen((const char *)(lf)), tx, txLen, s, e)

char *ensure_utf8(const char *text);

char *ShortVersString(short vers, char *versionStr);
char *InfiniteString(char *s, short size);
bool ItemFromResAppearsInStr(short resID, char *string, char *delims);
bool StrIsItemFromRes(char *string, short resID, char *delims);
char *StripLeadingItems(char *string, short resID);
char *StripTrailingITems(char *string, short resID);
bool EndsWithItem(char *string, short resID);

/* pstr_to_c: identity for C strings (backward compat) */
static inline const char *pstr_to_c(const char *pstr) {
  return pstr;
}

#endif /* STRINGUTIL_H */
