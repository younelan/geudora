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

#define FILE_NUM 73
/* Copyright (c) 1995 by QUALCOMM Incorporated */

#include "StringUtil.h"

#include "Globals.h"
#include "mailbox.h"
#include "mydefs.h"
#include "sendmail.h"
#include "tcp.h"
#include "util.h"
#include "fileutil.h"
#include "threading.h"
#include <pango/pango.h>
#include <stdint.h>
#include <stdlib.h>
char IsWordChar[256] = {0};
#define PluralStrn 1000
#define SQUARE_LEFT 1001
#define SQUARE_RIGHT 1002

/* Quote822 is implemented in lex822.c */
extern unsigned char *Quote822(unsigned char *into, unsigned char *from, bool space);

void NumToString(long n, char * s) {
  if (!s)
    return;
  sprintf((char *)s, "%ld", n);
}

void NumToDot(unsigned long n, char * s) {
  /* Stub: minimal IP formatting */
  sprintf((char *)s, "%lu.%lu.%lu.%lu", (n >> 24) & 0xFF, (n >> 16) & 0xFF,
          (n >> 8) & 0xFF, n & 0xFF);
}

/* GetRString is declared in mailbox.h and implemented elsewhere */

char * FormatString(uintptr_t arg, char * string, short format, short digits);
bool PPtrMatchLWSPSpot(char * look, char * text, uint32_t textLen, char * *matchEnd);

/************************************************************************
 * AllDigits - is a string made up only of digits?
 ************************************************************************/
bool AllDigits(char * s, long len) {
  while (len && '0' <= *s && *s <= '9') {
    s++;
    len--;
  }
  return (len == 0);
}

/************************************************************************
 * HighBits - count high bits in a string
 ************************************************************************/
long HighBits(char * s, long len) {
  long count = 0;
  while (len-- > 0)
    if (*s++ > 127)
      count++;
  return count;
}

/**********************************************************************
 * BeginsWith - does one string begin with another?
 **********************************************************************/
bool BeginsWith(char * string, char * prefix) {
  size_t slen = strlen((const char *)string);
  size_t plen = strlen((const char *)prefix);
  if (slen < plen)
    return false;
  return strncasecmp((const char *)string, (const char *)prefix, plen) == 0;
}

/**********************************************************************
 * CaptureHex - read hex strings
 **********************************************************************/
void CaptureHex(char * from, char * to) {
  unsigned char scratch[256];
  long len;

  len = strlen((const char *)from);
  CaptureHexPtr((char *)from, (char *)scratch, &len);
  scratch[len] = '\0';
  g_strlcpy((char *)to, (const char *)scratch, 256);
}

/**********************************************************************
 * CaptureHexPtr - read hex strings
 **********************************************************************/
void CaptureHexPtr(char * from, char * to, long *pLen) {
  char *spot, *end;
  char *toSpot, *toEnd;

  spot = from;
  end = from + *pLen;

  toSpot = to;
  toEnd = to + *pLen;

  for (; spot < end && toSpot < toEnd; spot++) {
    if ((unsigned char)*spot == lowerDelta) {
      Hex2Bytes((unsigned char *)spot + 1, 2, (unsigned char *)toSpot);
      toSpot++;
      spot += 2;
    } else
      *toSpot++ = *spot;
  }
  *pLen = toSpot - to;
}

/************************************************************************
 * ComposeString - sprintf, only smaller
 * %s - c string
 * %d - int
 * %c - char (int)
 * %p - pascal string
 * %q - internet-style quoted string
 * %i - internet address
 * %I - internet address, turned into hostname
 * %r - string from a resource
 * %O - OSType, including ''s
 * %o - OSType, no ''s
 * %#	- integer argument, prints "s" if not 1
 * %$ - integer argument, prints "es" if not 1
 * %& - integer argument, prints "'s" if not 1
 * %a - AEPrint
 ************************************************************************/
char * ComposeString(char * into, const char *format, ...) {
  va_list args;
  va_start(args, format);
  (void)VaComposeString(into, (char *)format, args);
  va_end(args);
  return (into);
}
char * ComposeRString(char * into, int format, ...) {
  va_list args;
  va_start(args, format);
  (void)VaComposeRString(into, format, args);
  va_end(args);
  return (into);
}
int ComposeRTrans(TransStream stream, int format, ...) {
  unsigned char into[256];
  va_list args;
  va_start(args, format);
  (void)VaComposeRString(into, format, args);
  va_end(args);
  return SendTrans(stream, into, strlen((const char *)into), NULL);
}
int AccuComposeR(AccuPtr a, int format, ...) {
  unsigned char into[256];
  va_list args;
  va_start(args, format);
  (void)VaComposeRString(into, format, args);
  va_end(args);
  return (AccuAddStr(a, into));
}
int AccuCompose(AccuPtr a, char * format, ...) {
  unsigned char into[256];
  va_list args;
  va_start(args, format);
  (void)VaComposeString(into, format, args);
  va_end(args);
  return (AccuAddStr(a, into));
}
char * VaComposeRString(char * into, short format, va_list args) {
  unsigned char stringFormat[256];

  GetRString(stringFormat, format);
  return (VaComposeString(into, stringFormat, args));
}

/* Helper: append char to C string, respecting max */
static void CStrCatC(char * s, int maxLen, unsigned char c) {
  size_t len = strlen((const char *)s);
  if (maxLen <= 0 || (int)len < maxLen - 1) {
    s[len] = c;
    s[len + 1] = '\0';
  }
}

#define MAX_SUBS 5
char * VaComposeStringDouble(char * into, int maxInto, char * format, va_list args,
                           char * into2, int maxInto2, char * format2) {
  char * formatP;
  unsigned char argString[256];
  long n;
  bool suppress;
  unsigned char buffers[MAX_SUBS][256];
  uintptr_t arg;
  short which;
  long resId;
  char * formatEnd;

top:
  which = 0;
  for (n = 0; n < MAX_SUBS; n++)
    buffers[n][0] = '\0';

  into[0] = '\0';
  formatEnd = format + strlen((const char *)format);
  for (formatP = format; formatP < formatEnd; formatP++)
    if (*formatP == lowerOmega && which < MAX_SUBS) {
      formatP++;
      if (*formatP == lowerOmega)
        CStrCatC(into, maxInto, *formatP);
      else {
        arg = va_arg(args, uintptr_t);
        FormatString(arg, buffers[which++], *formatP, 0);
      }
    } else if (*formatP != '%')
      CStrCatC(into, maxInto, *formatP);
    else {
      formatP++;
      if ((suppress = (*formatP == (unsigned char)0xA9)) !=
          0) /* suppress marker */
        formatP++;
      if (*formatP == '%')
        CStrCatC(into, maxInto, '%');
      else {
        if (*formatP == '^' && '0' <= formatP[1] && formatP[1] <= '9') {
          g_strlcpy((char *)argString, (const char *)buffers[formatP[1] - '0'], 256);
          formatP++;
          formatP++; /* skip format chars */
        } else if (*formatP == 'R') {
          resId = 0;
          while (formatP < formatEnd && isdigit(formatP[1])) {
            resId *= 10;
            resId += formatP[1] - '0';
            formatP++;
          }
          GetRString(argString, resId);
        } else {
          short digits = 0;
          if (isdigit(*formatP)) {
            digits = *formatP - '0';
            formatP++;
          }
          arg = va_arg(args, uintptr_t);
          if (suppress)
            argString[0] = '\0';
          else
            FormatString(arg, argString, *formatP, digits);
        }
        if (maxInto <= 0)
          g_strlcat((char *)into, (const char *)argString, 256);
        else
          g_strlcat((char *)into, (const char *)argString, maxInto);
      }
    }

  // ugly hack here...
  if (into2) {
    into = into2;
    format = format2;
    maxInto = maxInto2;
    into2 = format2 = NULL;
    maxInto2 = 0;
    goto top;
  }

  return (into);
}

/************************************************************************
 * EndsWith - does one string end with another?
 ************************************************************************/
bool EndsWith(char * name, char * suffix) {
  size_t nlen = strlen((const char *)name);
  size_t slen = strlen((const char *)suffix);
  if (nlen < slen)
    return false;
  return strncasecmp((const char *)name + nlen - slen, (const char *)suffix, slen) == 0;
}

/************************************************************************
 * HandleEndsWithR - does a handle end with a string from a resource?
 ************************************************************************/
bool HandleEndsWithR(void *name, short index) {
  unsigned char string[256];

  g_strlcpy((char *)string, (const char *)name, 256);
  return (EndsWithR(string, index));
}

/************************************************************************
 * EndsWithR - does a string end with a suffix in a resource?
 ************************************************************************/
bool EndsWithR(char * name, short resId) {
  unsigned char suffix[256];

  GetRString(suffix, resId);
  return (EndsWith(name, suffix));
}

/************************************************************************
 * StartsWithR - does a string start with a prefix in a resource?
 ************************************************************************/
bool StartsWithR(char * name, short resId) {
  unsigned char prefix[256];

  GetRString(prefix, resId);
  return (StartsWith(name, prefix));
}

/************************************************************************
 * StartsWith - does a string start with a prefix?
 ************************************************************************/
bool StartsWith(char * name, char * prefix) {
  return BeginsWith(name, prefix);
}

/************************************************************************
 * StartsWithPtr - does a string start with a prefix?
 ************************************************************************/
bool StartsWithPtr(char * name, uint32_t len, char * prefix) {
  size_t plen = strlen((const char *)prefix);
  return len >= plen && !strincmp(name, prefix, plen);
}

/************************************************************************
 * EqualStrRes - is a string the same as a resource?
 ************************************************************************/
bool EqualStrRes(char * string, short resId) {
  unsigned char s[256];
  GetRString(s, resId);
  return (StringSame((const char *)s, (const char *)string));
}

/************************************************************************
 * EscapeChars - escape characters in a string
 ************************************************************************/
char * EscapeChars(char * string, char * toEscape) {
  unsigned char scratch[256];
  char * to = scratch;
  char * from = string;
  char * end = string + strlen((const char *)string);
  bool escaped = False;

  while (from < end) {
    if (escaped) {
      *to++ = *from;
      escaped = False;
    }

    if (*from == '\\') {
      *to++ = '\\';
      escaped = True;
    } else {
      if (strchr((const char *)toEscape, *from))
        *to++ = '\\';
      *to++ = *from;
    }

    from++;
  }
  *to = '\0';

  g_strlcpy((char *)string, (const char *)scratch, 256);

  return (string);
}

/**********************************************************************
 * EscapeInHex - write hex strings safely
 **********************************************************************/
void EscapeInHex(char * from, char * to) {
  char *spot, *end;
  unsigned char scratch[256];
  char *toSpot, *toEnd;

  spot = from;
  end = spot + strlen((const char *)from);

  toSpot = scratch;
  toEnd = toSpot + 250;

  for (; spot < end && toSpot < toEnd; spot++) {
    if ((*spot > ' ' && *spot != lowerDelta) ||
        (*spot == ' ' && spot != (end - 1)))
      *toSpot++ = *spot;
    else {
      *toSpot++ = lowerDelta;
      Bytes2Hex(spot, 1, toSpot);
      toSpot += 2;
    }
  }
  *toSpot = '\0';
  g_strlcpy((char *)to, (const char *)scratch, 256);
}

/**********************************************************************
 * Transmogrify - change one string into another, using two STR# for xlation
 **********************************************************************/
char * Transmogrify(char * toStr, short toId, char * fromStr, short fromId) {
  short index;

  if ((index = FindSTRNIndex(fromId, fromStr)) != 0)
    GetRString(toStr, toId + index);
  else if (toStr != fromStr)
    g_strlcpy((char *)toStr, (const char *)fromStr, 256);
  return (toStr);
}

/************************************************************************
 * FixNewlines - remove cr, and turn nl into cr
 ************************************************************************/
void FixNewlines(char * string, long *count) {
  char *from, *to;
  long n;

  for (to = from = string, n = *count; n; n--, from++)
    if (*from == '\012')
      *to++ = '\015';
    else if (*from != '\015')
      *to++ = *from;
  *count = to - string;
}

/**********************************************************************
 *
 **********************************************************************/
char * FormatString(uintptr_t arg, char * string, short format, short digits) {
  short n;
  struct hostInfo hi;

  string[0] = '\0';

  switch (format) {
  case 'c':
    string[0] = (unsigned char)arg;
    string[1] = '\0';
    break;
  case 's': {
    const char *sarg = (const char *)(intptr_t)arg;
    g_strlcpy((char *)string, sarg, 254);
    break;
  }
  case 'p':
    /* %p now means same as %s (C string) */
    g_strlcpy((char *)string, (const char *)(intptr_t)arg, 256);
    break;
  case 'e':
    g_strlcpy((char *)string, (const char *)(intptr_t)arg, 256);
    EscapeInHex(string, string);
    break;
  case 'i':
    NumToDot(arg, string);
    break;
  case 'I':
    if (!GetHostByAddr(&hi, arg)) {
      g_strlcpy((char *)string, hi.cname, 256);
    } else {
      char tmp[64];
      sprintf(tmp, "[%lu.%lu.%lu.%lu]", (arg >> 24) & 0xFF, (arg >> 16) & 0xFF,
              (arg >> 8) & 0xFF, arg & 0xFF);
      g_strlcpy((char *)string, tmp, 256);
    }
    break;
  case 'd':
    NumToString(arg, string);
    break;
  case 'K':
    if (arg < 1 K)
      NumToString(arg, string);
    else if (arg < 10 K) {
      arg *= 10;
      arg /= 1 K;
      if (arg % 10)
        sprintf((char *)string, "%lu.%luK", arg / 10, arg % 10);
      else
        sprintf((char *)string, "%luK", arg / 10);
    } else if (arg < 1 K K) {
      arg /= 1 K;
      NumToString(arg, string);
      g_strlcat((char *)string, "K", 256);
    } else if (arg < 10 K K) {
      arg *= 10;
      arg /= 1 K K;
      if (arg % 10)
        sprintf((char *)string, "%lu.%luM", arg / 10, arg % 10);
      else
        sprintf((char *)string, "%luM", arg / 10);
    } else {
      arg /= 1 K K;
      NumToString(arg, string);
      g_strlcat((char *)string, "M", 256);
    }
    break;
  case 'q':
    Quote822(string, (char *)(intptr_t)arg, True);
    break;
  case 'r':
    GetRString(string, arg);
    break;
  case 'b': {
    unsigned char *p = string;
    n = 32;
    while (n--) {
      *p++ = arg & (1 << 31) ? '1' : '0';
      arg <<= 1;
    }
    *p = '\0';
    break;
  }
  case 'x':
    Long2Hex(string, arg);
    if (digits) {
      size_t slen = strlen((const char *)string);
      while ((int)slen < digits) {
        memmove(string + 1, string, slen + 1);
        string[0] = '0';
        slen++;
      }
      if ((int)slen > digits) {
        memmove(string, string + (slen - digits), digits);
        string[digits] = '\0';
      }
    }
    break;
  case '#':
    GetRString(string, PluralStrn + (arg == 1 ? 1 : 2));
    break;
  case '$':
    GetRString(string, PluralStrn + (arg == 1 ? 3 : 4));
    break;
  case '&':
    GetRString(string, PluralStrn + (arg == 1 ? 5 : 6));
    break;
  case '*':
    GetRString(string, PluralStrn + (arg == 1 ? 7 : 8));
    break;
  case 'O':
    string[0] = '\'';
    string[1] = ((unsigned char *)&arg)[0];
    string[2] = ((unsigned char *)&arg)[1];
    string[3] = ((unsigned char *)&arg)[2];
    string[4] = ((unsigned char *)&arg)[3];
    string[5] = '\'';
    string[6] = '\0';
    break;
  case 'o':
    string[0] = ((unsigned char *)&arg)[0];
    string[1] = ((unsigned char *)&arg)[1];
    string[2] = ((unsigned char *)&arg)[2];
    string[3] = ((unsigned char *)&arg)[3];
    string[4] = '\0';
    break;
  case 'B':
    g_strlcpy((char *)string, arg ? "TRUE" : "FALSE", 256);
    break;
  }

  return (string);
}

/************************************************************************
 * LCD - find the least common denom of two strings
 ************************************************************************/
char * LCD(char * s1, char * s2) {
  size_t len1 = strlen((const char *)s1);
  size_t len2 = strlen((const char *)s2);
  size_t minLen = MIN(len1, len2);
  size_t i;
  char c1, c2;

  for (i = 0; i < minLen; i++) {
    if (s1[i] != s2[i]) {
      c1 = s1[i]; c2 = s2[i];
      if (isupper(c1)) c1 = tolower(c1);
      if (isupper(c2)) c2 = tolower(c2);
      if (c1 != c2)
        break;
    }
  }
  s1[i] = '\0';
  return (s1);
}

/**********************************************************************
 * concatenate a pascal string on the end of another
 **********************************************************************/
char * PCat(char * string, char * suffix) {
  g_strlcat((char *)string, (const char *)suffix, 256);
  return (string);
}

/**********************************************************************
 * PCatR - concatenate a string from a resource to the end of a string
 **********************************************************************/
char * PCatR(char * string, short resId) {
  unsigned char suffix[256];

  GetRString(suffix, resId);
  return (PCat(string, suffix));
}

/************************************************************************
 * PCopyTrim - copy and trim a string
 ************************************************************************/
char * PCopyTrim(char * toString, char * fromString, short max) {
  unsigned char tString[256];
  g_strlcpy((char *)tString, (const char *)fromString, 256);
  TrimWhite(tString);
  TrimInitialWhite(tString);
  if ((int)strlen((const char *)tString) >= max)
    tString[max - 1] = '\0';
  g_strlcpy((char *)toString, (const char *)tString, max);
  return (toString);
}

/**********************************************************************
 * concatenate a pascal string on the end of another
 * escape certain chars in the string
 **********************************************************************/
char * PEscCat(char * string, char * suffix, short escape, char *escapeWhat) {
  size_t sufLen = strlen((const char *)suffix);
  char * suffSpot;
  size_t slen = strlen((const char *)string);
  char * stringSpot = string + slen;

  for (suffSpot = suffix; sufLen--; suffSpot++) {
    if (*suffSpot == (unsigned char)escape || strchr(escapeWhat, *suffSpot))
      *stringSpot++ = (unsigned char)escape;
    *stringSpot++ = *suffSpot;
  }
  *stringSpot = '\0';

  return (string);
}

/************************************************************************
 * PIndex - find a char in a pascal string
 ************************************************************************/
char * PIndex(char * string, char c) {
  char * spot = (char *)strchr((const char *)string, (unsigned char)c);
  return spot;
}

/************************************************************************
 * IndexPtr - find a char in a string specified by pointer and length
 ************************************************************************/
char * IndexPtr(char * string, long stringLen, char c) {
  char * spot;

  for (spot = string; spot < string + stringLen; spot++)
    if (*spot == (unsigned char)c)
      return (spot);
  return (NULL);
}

/************************************************************************
 * PRIndex - find a char in a pascal string, backwards
 ************************************************************************/
char * PRIndex(char * string, char c) {
  char * spot = (char *)strrchr((const char *)string, (unsigned char)c);
  return spot;
}

/**********************************************************************
 * PInsert - insert some text
 **********************************************************************/
char * PInsert(char * string, short size, char * insert, char * spot) {
  size_t slen = strlen((const char *)string);
  size_t ilen = strlen((const char *)insert);
  short toInsert = MIN((short)ilen, (short)(size - (short)slen - 1));

  if (toInsert > 0) {
    size_t tailLen = slen - (spot - string);
    memmove(spot + toInsert, spot, tailLen + 1); /* +1 for null */
    memmove(spot, insert, toInsert);
  }
  return (string);
}

/**********************************************************************
 * PInsertC - insert a single character
 **********************************************************************/
char * PInsertC(char * string, short size, Byte c, char * spot) {
  unsigned char s[2];
  s[0] = c;
  s[1] = '\0';
  return (PInsert(string, size, s, spot));
}

/************************************************************************
 * PLCat - concat a long onto a string (preceed it with a space)
 ************************************************************************/
void PLCat(char *dst, long num) {
  char s[64];
  sprintf(s, "%ld", num);
  g_strlcat(dst, s, 256);
}

/************************************************************************
 * PXCat - concat a long onto a string in hex
 ************************************************************************/
char * PXCat(char * string, long num) {
  unsigned char x[32];

  Bytes2Hex((void *)&num, sizeof(long), x);
  x[8] = '\0';
  return (PCat(string, x));
}

/************************************************************************
 * PXWCat - concat a short onto a string in hex
 ************************************************************************/
char * PXWCat(char * string, short num) {
  unsigned char x[32];

  Bytes2Hex((void *)&num, sizeof(short), x);
  x[4] = '\0';
  return (PCat(string, x));
}

/************************************************************************
 * Tr - translate text in a handle
 ************************************************************************/
bool Tr(void *text, char *fromS, char *toS) {
  long len = strlen((char *)text);
  return TrLo((char *)text, len, fromS, toS);
}

/************************************************************************
 * TrLo - translate text in a pointer
 ************************************************************************/
bool TrLo(char *text, long len, char *fromS, char *toS) {
  char *end, *spot;
  bool did = False;
  short fromChar, toChar;

  end = text + len;
  for (; *fromS; fromS++, toS++) {
    fromChar = *fromS;
    toChar = *toS;
    for (spot = text; spot < end; spot++)
      if (*spot == fromChar) {
        did = True;
        *spot = toChar;
      }
  }
  return (did);
}

/************************************************************************
 * PPtrFindSub - is a pascal string a substring of a string
 ************************************************************************/
char * PPtrFindSub(char * sub, char * string, long len) {
  size_t subLen = strlen((const char *)sub);
  char * end = string + len - subLen + 1;
  char *stringSpot, *subSpot, *subEnd;
  Byte c1, c2;

  if (subLen == 0) return string;
  subEnd = sub + subLen;
  while (string < end) {
    for (subSpot = sub, stringSpot = string; subSpot < subEnd;
         subSpot++, stringSpot++) {
      if (*stringSpot != *subSpot) {
        c1 = *stringSpot;
        c2 = *subSpot;
        if (isupper(c1))
          c1 = tolower(c1);
        if (isupper(c2))
          c2 = tolower(c2);
        if (c1 != c2)
          break;
      }
    }
    if (subSpot >= subEnd)
      return (string); /* return start of match */
    string++;
  }
  return (NULL);
}

/************************************************************************
 * PReplace - replace one string with another
 ************************************************************************/
char * PReplace(char * string, char * find, char * replace) {
  char * spot;
  size_t flen = strlen((const char *)find);
  size_t rlen = strlen((const char *)replace);

  if (flen && strcasecmp((const char *)find, (const char *)replace) != 0)
    while ((spot = PPtrFindSub(find, string, strlen((const char *)string))) != NULL) {
      size_t slen = strlen((const char *)string);
      if (slen + rlen - flen > 255)
        break;
      size_t tailLen = slen - (spot - string) - flen;
      memmove(spot + rlen, spot + flen, tailLen + 1);
      memmove(spot, replace, rlen);
    }

  return (string);
}

/* PSCat_C removed — replaced by g_strlcat() at call sites */
/* PStrCopy removed — replaced by g_strlcpy() at call sites */

/**********************************************************************
 * InfiniteString - set string to all 0xFFs
 **********************************************************************/
char * InfiniteString(char * s, short size) {
  memset(s, 0xFF, size - 1);
  s[size - 1] = '\0';
  return s;
}

/************************************************************************
 * ItemFromResAppearsInStr - does a string contain an item from a list of
 *  items in a resource
 ************************************************************************/
bool ItemFromResAppearsInStr(short resID, char * string, char * delims) {
  unsigned char s[256];
  unsigned char token[64];
  char * spot;

  // default delimitter is comma
  if (!delims)
    delims = (char *) ",";

  GetRString(s, resID);
  spot = s;

  while (PToken(s, token, &spot, delims))
    if (PPtrFindSub(token, string, strlen((const char *)string)))
      return true;

  return false;
}

/************************************************************************
 * StrIsItemFromRes - is a string one of the items in a resource?
 ************************************************************************/
bool StrIsItemFromRes(char * string, short resID, char * delims) {
  unsigned char s[256];
  unsigned char token[64];
  char * spot;

  // default delimitter is comma
  if (!delims)
    delims = (char *) ",";

  GetRString(s, resID);
  spot = s;

  while (PToken(s, token, &spot, delims))
    if (StringSame((const char *)token, (const char *)string))
      return true;

  return false;
}

/************************************************************************
 * PToken - grab a token out of a string
 *  Returns pointer to token argument
 *  Saves state in spotP
 ************************************************************************/
char * PToken(char * string, char * token, char * *spotP, char * delims) {
  char * spot;
  char * end = string + strlen((const char *)string);
  char * tSpot = token;

  token[0] = '\0';
  if (*spotP >= end)
    return (NULL);
  for (spot = *spotP; spot < end; spot++)
    if (!strchr((const char *)delims, (unsigned char)*spot))
      *tSpot++ = *spot;
    else
      break;
  *spotP = spot + 1;
  *tSpot = '\0';
  return (token);
}

/************************************************************************
 * TokenPtr - grab a token out of text
 *  Returns boolean indicating if token found
 *  Saves state in spotP
 ************************************************************************/
bool TokenPtr(char * string, long stringLen, char * *token, long *tokenLen,
              char * *spotP, char * delims) {
  char * spot;
  char * end = (char *)string + stringLen;
  long len = 0;

  *token = (char *)*spotP;
  if (*spotP >= end)
    return (false);
  for (spot = *spotP; spot < end; spot++)
    if (!strchr((const char *)delims, (unsigned char)*spot))
      len++;
    else
      break;
  *spotP = spot + 1;
  *tokenLen = len;
  return (true);
}

/************************************************************************
 * TokenPtr - grab a token out of text
 *  Returns boolean indicating if token found
 *  Saves state in spotP
 *	Returns token in p-string
 ************************************************************************/
bool PTokenPtr(char * string, long stringLen, char * token, char * *spotP,
               char * delims) {
  char * tokenPtr;
  long tokenLen;
  Boolean result;

  if ((result = TokenPtr(string, stringLen, (char * *)&tokenPtr, &tokenLen, spotP,
                         delims)) != 0) {
    size_t copyLen = MIN(tokenLen, 255);
    memcpy(token, tokenPtr, copyLen);
    token[copyLen] = '\0';
  }
  return result;
}

/**********************************************************************
 * ReMatch - does a string have a reply intro?
 **********************************************************************/
bool ReMatch(char * string, char * re) {
  char * colon;
  char * reSpot;
  unsigned char remainder[256];
  size_t reLen = strlen((const char *)re);

  if (reLen > 0) { /* intro string not empty */
    if ((colon = PIndex(string, re[reLen - 1])) != NULL) { /* final char */
      if ((size_t)(colon - string) >= reLen) { /* long enough */
        reSpot = re + reLen - 1;
        do {
          reSpot--;
          colon--;
          while (reSpot > re && !IsWordChar[*reSpot])
            reSpot--;
          while (colon > string && !IsWordChar[*colon])
            colon--;
          if (colon > string && reSpot > re)
            if ((*colon & 0x1f) != (*reSpot & 0x1f))
              break;
        } while (colon > string && reSpot > re);

        if (reSpot != re)
          return false;
        if (colon == string)
          return true;

        size_t remLen = MIN((size_t)(colon - string), 255);
        memcpy(remainder, string, remLen);
        remainder[remLen] = '\0';
        TrimAllWhite(remainder);
        TrimSquares(remainder, true, true);
        return (remainder[0] == '\0');
      }
    }
  }
  return (False);
}

/************************************************************************
 * TrimSquares - trim square-bracketed stuff from start of string
 ************************************************************************/
bool TrimSquares(char * s, bool multiple, bool internal) {
  unsigned char left[32], right[32];
  char * spot;
  bool result = false;

  GetRString(left, SQUARE_LEFT);
  GetRString(right, SQUARE_RIGHT);
  TrimInitialWhite(s);

  size_t slen = strlen((const char *)s);
  size_t leftLen = strlen((const char *)left);

  if (!internal) {
    while (slen > 2) {
      /* find matching left delimiter for s[0] */
      spot = (char *)strchr((const char *)left, s[0]);
      if (spot != NULL) {
        size_t idx = spot - (char *)left;
        char * rspot = PIndex(s, (char)right[idx]);
        if (rspot != NULL) {
          size_t cutLen = rspot - s + 1;
          memmove(s, rspot + 1, slen - cutLen + 1);
          slen -= cutLen;
          TrimInitialWhite(s);
          slen = strlen((const char *)s);
          result = true;
        } else
          break;
      } else
        break;
      if (!multiple)
        break;
    }
  } else {
    size_t brk;
    char * lPtr;
    char * rPtr;

    for (brk = 0; slen > 2 && brk < leftLen; brk++) {
      if ((lPtr = PIndex(s, (char)left[brk])) != NULL)
        if ((rPtr = PIndex(s, (char)right[brk])) != NULL)
          if (lPtr < rPtr) {
            size_t cutLen = rPtr - lPtr + 1;
            memmove(lPtr, rPtr + 1, slen - (rPtr - s) );
            slen -= cutLen;
            result = true;
            if (multiple)
              brk--; /* try again */
            else
              break;
          }
    }
  }

  return result;
}

/************************************************************************
 * RemoveParens - remove parenthesized information
 ************************************************************************/
void RemoveParens(char * string) {
  char *to, *from, *end;
  short pLevel = 0;

  end = string + strlen((const char *)string);
  for (to = from = string; from < end; from++)
    switch (*from) {
    case '(':
      pLevel++;
      break;
    case ')':
      if (pLevel)
        pLevel--;
      else
        *to++ = *from;
      break;
    case ' ':
      if (!pLevel)
        break;
      /* fall through is deliberate */
    default:
      *to++ = *from;
      break;
    }
  *to = '\0';
}

/************************************************************************
 * PStripChar - remove all occurrences of a char from a string
 ************************************************************************/
char * PStripChar(char * string, Byte c) {
  size_t newLen = StripChar((char *)string, strlen((const char *)string), c);
  string[newLen] = '\0';
  return (string);
}

/************************************************************************
 * StripChar - remove all occurrences of a char from text, return new length
 ************************************************************************/
long StripChar(char * string, long len, Byte c) {
  char *from, *to, *end;

  end = string + len;
  from = string - 1;
  to = string;

  while (++from < end)
    if ((unsigned char)*from != c)
      *to++ = *from;
  return to - string;
}

/************************************************************************
 * strincmp - compare two strings, don't care about case
 ************************************************************************/
int strincmp(char * s1, char * s2, short n) {
  register int c1, c2;
  for (c1 = *s1, c2 = *s2; n--; c1 = *++s1, c2 = *++s2) {
    if (c1 - c2) {
      if (isupper(c1))
        c1 = tolower(c1);
      if (isupper(c2))
        c2 = tolower(c2);
      if (c1 - c2)
        return (c1 - c2);
    }
  }
  return (0);
}

/**********************************************************************
 * striscmp - compare two strings, up to the length of the shorter string,
 * and ignoring case
 **********************************************************************/
int striscmp(char * s1, char * s2) {
  register int c1, c2;
  for (c1 = *s1, c2 = *s2; c1 && c2; c1 = *++s1, c2 = *++s2) {
    if (c1 - c2) {
      if (isupper(c1))
        c1 = tolower(c1);
      if (isupper(c2))
        c2 = tolower(c2);
      if (c1 - c2)
        return (c1 - c2);
    }
  }
  return (0);
}

/**********************************************************************
 * strscmp - compare two strings, up to the length of the shorter string,
 * paying attention to case
 **********************************************************************/
int strscmp(char * s1, char * s2) {
  register int c1, c2;
  for (c1 = *s1, c2 = *s2; c1 && c2; c1 = *++s1, c2 = *++s2)
    if (c1 - c2)
      return (c1 - c2);
  return (0);
}

/************************************************************************
 * Tokenize - set pointers to the beginning and end of a delimited token
 ************************************************************************/
char * Tokenize(char * string, int size, char * *start, char * *end, char * delims) {
  char * stop = string + size;
  char safe = (char)*stop;
  char * last;

  *stop = 0;
  while (strchr((const char *)delims, *string))
    string++;
  *stop = *delims;
  for (last = string; !strchr((const char *)delims, *last); last++)
    ;
  *stop = (unsigned char)safe;
  if (string == stop)
    return (NULL);
  *start = string;
  *end = stop;
  return (string);
}

/**********************************************************************
 * TrimInitialWhite - remove whitespace characters from the beginning of a
 *string
 **********************************************************************/
char * TrimInitialWhite(char * s) {
  char * cp = s;
  while (*cp && IsSpace(*cp))
    cp++;
  if (cp > s)
    memmove(s, cp, strlen((const char *)cp) + 1);
  return (s);
}

/**********************************************************************
 * TrimInternalWhite - collapse internal whitespace into single
 **********************************************************************/
char * TrimInternalWhite(char * s) {
  bool wasWhite = false;
  bool isWhite;
  unsigned char *spot;
  unsigned char *end = s + strlen((const char *)s);
  unsigned char *copySpot = s;

  for (spot = s; spot < end; spot++) {
    isWhite = IsSpace(*spot);
    if (isWhite && wasWhite)
      continue;
    *copySpot++ = *spot;
    wasWhite = isWhite;
  }
  *copySpot = '\0';

  return (s);
}

/************************************************************************
 * TrimPrefix - strip a prefix from a string
 ************************************************************************/
bool TrimPrefix(char * string, char * prefix) {
  size_t slen = strlen((const char *)string);
  size_t plen = strlen((const char *)prefix);

  if (slen < plen)
    return false;

  if (strncasecmp((const char *)string, (const char *)prefix, plen) == 0) {
    memmove(string, string + plen, slen - plen + 1);
    return true;
  }
  return false;
}

/**********************************************************************
 * StringSame - are two strings the same?
 **********************************************************************/
bool StringSame(const char *s1, const char *s2) {
  return strcasecmp(s1, s2) == 0;
}

/**********************************************************************
 * StringComp - return whether s1<s2 (negative), s1==s2 (0), s1>s2 (positive)
 **********************************************************************/
long StringComp(char * s1, char * s2) {
  return strcasecmp((const char *)s1, (const char *)s2);
}

/**********************************************************************
 * MyUpperText - uppercase text with or without fancy furrin stuff
 **********************************************************************/
void MyUpperText(char * buffer, long bufferSize) {
  short i;
  for (i = 0; i < bufferSize; i++)
    buffer[i] = g_ascii_toupper(buffer[i]);
}

/**********************************************************************
 * MyLowerText - lowercase text with or without fancy furrin stuff
 **********************************************************************/
void MyLowerText(char * buffer, long bufferSize) {
  short i;
  for (i = 0; i < bufferSize; i++)
    buffer[i] = g_ascii_tolower(buffer[i]);
}

/**********************************************************************
 * MyLowercaseText - call lowercasetext carefully
 **********************************************************************/
int MyLowercaseText(char * text, long len) {
  MyLowerText(text, len);
  return (0);
}

/**********************************************************************
 * TrimReLo - remove an Re: string
 **********************************************************************/
bool TrimReLo(char * string, char * re) {
  char * colon;
  size_t reLen = strlen((const char *)re);
  if (ReMatch(string, re)) {
    colon = PIndex(string, re[reLen - 1]); /* last char of re */
    while (colon[1] && IsWhite(colon[1]))
      colon++;
    size_t slen = strlen((const char *)string);
    memmove(string, colon + 1, slen - (colon - string) + 1);
    return (True);
  }
  return (False);
}

/************************************************************************
 * TrimRe - trim Re: and Fwd: from a string
 ************************************************************************/
bool TrimRe(char * string, bool squares) {
  bool did = False;

  while (TrimReLo(string, (char *)Re) || TrimReLo(string, (char *)Fwd) ||
         TrimReLo(string, (char *)OFwd) ||
         (squares && TrimSquares(string, false, false)))
    did = True;

  return (did);
}

/**********************************************************************
 * TrimWhite - remove whitespace characters from the end of a string
 **********************************************************************/
char * TrimWhite(char * s) {
  size_t len = strlen((const char *)s);
  while (len > 0 && IsSpace(s[len - 1]))
    len--;
  s[len] = '\0';
  return (s);
}

/**********************************************************************
 * CollapseLWSP - convert lwsp runs to single spaces
 **********************************************************************/
char * CollapseLWSP(char * s) {
  char *to, *from, *end;
  bool space = true; // beginning of string counts as space
  bool lwsp;

  end = s + strlen((const char *)s);
  for (to = from = s; from < end; from++) {
    lwsp = IsLWSP(*from);
    if (space && lwsp)
      continue;

    if (lwsp)
      *to++ = ' ';
    else
      *to++ = *from;

    space = lwsp;
  }
  *to = '\0';

  // strip trailing space
  size_t len = strlen((const char *)s);
  if (len > 0 && s[len - 1] == ' ')
    s[len - 1] = '\0';

  return s;
}

/**********************************************************************
 * IsAllWhitePtr - is a string all whitespace?
 **********************************************************************/
bool IsAllWhitePtr(char * s, long len) {
  for (; len-- > 0; s++)
    if (!IsWhite(*s))
      return false;
  return true;
}

/**********************************************************************
 * IsAllLWSPPtr - is a string all lwsp?
 **********************************************************************/
bool IsAllLWSPPtr(char * s, long len) {
  for (; len-- > 0; s++)
    if (!IsLWSP(*s))
      return false;
  return true;
}

/**********************************************************************
 * IsAllUpper - is a string all lwsp?
 **********************************************************************/
bool IsAllUpper(char * s) {
  if (!*s)
    return false;

  for (; *s; s++)
    if (!isupper(*s))
      return false;

  return true;
}

/**********************************************************************
 * Uncomma - reformat a name to remove a comma
 **********************************************************************/
char * Uncomma(char * name) {
  unsigned char scratch[256];
  char * comma;

  if ((comma = PIndex(name, ':')) != NULL) {
    *comma = '\0'; /* truncate at colon */
  } else if (isupper(name[0]) && (comma = PIndex(name, ',')) != NULL) {
    /* "Last, First" → "First Last" */
    g_strlcpy((char *)scratch, (const char *)comma + 1, 256);
    *comma = '\0'; /* name now has just "Last" */
    TrimInitialWhite(scratch);
    TrimWhite(scratch);
    TrimInitialWhite(name);
    TrimWhite(name);
    /* Build "First Last" */
    unsigned char result[256];
    snprintf((char *)result, 256, "%s %s", scratch, name);
    g_strlcpy((char *)name, (const char *)result, 256);
  }
  return (name);
}

/************************************************************************
 * CharWidthInFont - how wide is a character in a given font?
 * Uses Pango for font measurement (GTK port).
 ************************************************************************/
short CharWidthInFont(Byte c, short font, short size) {
  PangoFontDescription *fdesc;
  PangoFont *pfont;
  PangoFontMetrics *metrics;
  PangoLanguage *lang;
  PangoContext *ctx;
  short width = size / 2; /* default fallback */

  fdesc = pango_font_description_new();
  pango_font_description_set_family(fdesc, "monospace");
  pango_font_description_set_size(fdesc, size * PANGO_SCALE);
  ctx = pango_font_map_create_context(pango_cairo_font_map_get_default());
  pfont = pango_context_load_font(ctx, fdesc);
  if (pfont) {
    lang = pango_language_get_default();
    metrics = pango_font_get_metrics(pfont, lang);
    width = (short)PANGO_PIXELS(
        pango_font_metrics_get_approximate_char_width(metrics));
    pango_font_metrics_unref(metrics);
    g_object_unref(pfont);
  }
  g_object_unref(ctx);
  pango_font_description_free(fdesc);
  return width;
}

/************************************************************************
 * UTF8ToMac - convert utf8 to mac
 ************************************************************************/
char * UTF8ToMac(char * string) {
  long len = strlen((const char *)string);

  UTF8To88591((char *)string, len, (char *)string, &len);
  string[len] = '\0';
  return (string);
}

/************************************************************************
 * UTF8To88591 - convert utf8 to 8859-1
 ************************************************************************/
void UTF8To88591(char * inStr, long inLen, char * outStr, long *outLen) {
  long len;
  Byte tempChar;

  len = 0L;
  while (--inLen >= 0L) {
    tempChar = *inStr++;
    if (tempChar & 0x80) {
      if (tempChar & 0x3C) {
        *outStr++ = '?';
        ++len;
        while ((tempChar <<= 1) & 0x80) {
          --inLen;
          ++inStr;
        }
      } else {
        *outStr++ = ((tempChar & 0x03) << 6) + (*inStr & 0x7F);
        ++len;
        --inLen;
      }
    } else {
      *outStr++ = tempChar;
      ++len;
    }
  }
  *outLen = len;
}

#undef StringToNum
void MyStringToNum(char * string, long *num) {
  if (!string[0] || !(isdigit(string[0]) || string[0] == '-' || string[0] == '+'))
    *num = 0L;
  else
    *num = strtol((const char *)string, NULL, 10);
}

/************************************************************************
 * PtrPtrMatchLWSP - match two strings, considering all LWSP as the same
 ************************************************************************/
bool PtrPtrMatchLWSP(char * lookFor, short lookLen, char * text, uint32_t textLen,
                     bool atStart, bool atEnd) {
  unsigned char shortLook[256];
  char *spot, *end;
  char *matchEnd;

  lookLen = MIN(lookLen, 250);

  // First, copy the string being looked for, collapsing LWSP
  end = lookFor + lookLen;
  spot = (char *)shortLook;
  while (lookFor < end) {
    if (IsLWSP((unsigned char)*lookFor)) {
      *spot++ = ' ';
      do {
        lookFor++;
      } while (lookFor < end && IsLWSP((unsigned char)*lookFor));
    }
    else
      *spot++ = *lookFor++;
  }
  *spot = '\0';
  TrimAllWhite(shortLook);
  if (!shortLook[0])
    return true; // empty matches all

  // does match need to be at beginning?
  if (atStart) {
    if (PPtrMatchLWSPSpot(shortLook, text, textLen, (char * *)&matchEnd)) {
      if (atEnd)
        return matchEnd == text + textLen;
      else
        return true;
    } else {
      return false;
    }
  }

  // Now, test at each spot in the string
  end = text + textLen - strlen((const char *)shortLook) + 1;
  spot = text;

  while (spot < end) {
    if (PPtrMatchLWSPSpot(shortLook, spot, (uint32_t)textLen,
                          (char * *)&matchEnd))
      if (atEnd) {
        if (matchEnd == text + textLen + 1)
          return true;
      } else
        return true;

    if (IsLWSP((unsigned char)*spot))
      do {
        spot++;
      } while (spot < end && IsLWSP((unsigned char)*spot));
    else
      spot++;
  }

  return false;
}

/************************************************************************
 * PPtrMatchLWSPSpot - match two strings, considering all LWSP as the same,
 *starting at a given spot
 ************************************************************************/
bool PPtrMatchLWSPSpot(char * look, char * text, uint32_t textLen, char * *matchEnd) {
  char * textEnd = text + textLen;
  char * lookEnd = look + strlen((const char *)look);
  char * lookSpot = look;
  Byte c1, c2;

  while (1) {
    // advance text to next non-LWSP char
    while (text < textEnd && IsLWSP((unsigned char)*text))
      text++;

    // advance look to next non-LWSP char
    while (lookSpot < lookEnd && IsLWSP(*lookSpot))
      lookSpot++;

    // did we run out of string being looked for?  If so, we've succeeded
    if (lookSpot == lookEnd)
      break;

    // did we run out of string being looked in?  If so, we've failed
    if (text == textEnd)
      return (false);

    c1 = *lookSpot;
    c2 = (unsigned char)*text;
    if (isupper(c1))
      c1 = (Byte)tolower(c1);
    if (isupper(c2))
      c2 = (Byte)tolower(c2);
    if (c1 != c2)
      return false;

    lookSpot++;
    text++;

    if (lookSpot != lookEnd &&
        (text == textEnd || IsLWSP((unsigned char)*text)) !=
            (IsLWSP(*lookSpot)))
      return false;
  }

  if (matchEnd)
    *matchEnd = (char *)text;

  return true;
}

/************************************************************************
 * PStrToNum - StringToNum as a function
 ************************************************************************/
long PStrToNum(char * string) {
  return strtol((const char *)string, NULL, 10);
}

/************************************************************************
 * ShortVersString - turn a short into an x.x.x.x version string
 ************************************************************************/
char * ShortVersString(short vers, char * versionStr) {
  char scratch[64];
  char hex[256];

  versionStr[0] = '\0';
  hex[0] = '\0';

  sprintf(scratch, "%x", vers);
  char *scan = scratch;

  /* skip leading 0's */
  while (*scan == '0')
    scan++;

  while (*scan) {
    size_t hlen = strlen(hex);
    hex[hlen] = *scan;
    hex[hlen + 1] = '.';
    hex[hlen + 2] = '\0';
    scan++;
  }
  /* remove trailing period */
  size_t hlen = strlen(hex);
  if (hlen > 0 && hex[hlen - 1] == '.')
    hex[hlen - 1] = '\0';

  g_strlcpy((char *)versionStr, hex, 256);
  return (versionStr);
}

/************************************************************************
 * StripLeadingItems - strip items from the beginning of a string
 ************************************************************************/
char * StripLeadingItems(char * string, short resID) {
  unsigned char s[256];
  unsigned char token[64];
  char * spot;

  GetRString(s, resID);
  spot = s;

  while (PToken(s, token, &spot, (char *) ",")) {
    if (BeginsWith(string, token)) {
      size_t tlen = strlen((const char *)token);
      memmove(string, string + tlen, strlen((const char *)string) - tlen + 1);
      break;
    }
  }

  return string;
}

/************************************************************************
 * StripTrailingItems - strip items from the end of a string
 ************************************************************************/
char * StripTrailingITems(char * string, short resID) {
  unsigned char s[256];
  unsigned char token[64];
  char * spot;

  GetRString(s, resID);
  spot = s;

  while (PToken(s, token, &spot, (char *) ",")) {
    if (EndsWith(string, token)) {
      size_t slen = strlen((const char *)string);
      size_t tlen = strlen((const char *)token);
      string[slen - tlen] = '\0';
      break;
    }
  }

  return string;
}

/************************************************************************
 * EndsWithItem - does a string end with one of these items?
 ************************************************************************/
bool EndsWithItem(char * string, short resID) {
  unsigned char s[256];
  unsigned char token[64];
  char * spot;

  GetRString(s, resID);
  spot = s;

  while (PToken(s, token, &spot, (char *) ","))
    if (EndsWith(string, token))
      return true;

  return false;
}
/************************************************************************
 * ensure_utf8 - ensure a string is valid UTF-8, convert from Win-1252 if not
 ************************************************************************/
gchar *ensure_utf8(const char *text) {
  if (!text)
    return NULL;
  if (g_utf8_validate(text, -1, NULL))
    return g_strdup(text);

  /* Not valid UTF-8, assume Windows-1252 and convert */
  GError *err = NULL;
  gchar *converted = g_convert(text, -1, "UTF-8", "WINDOWS-1252", NULL, NULL, &err);
  if (err) {
    g_warning("ensure_utf8 failed: %s", err->message);
    g_error_free(err);
    /* Fallback to make_valid which replaces invalid sequences */
    return g_utf8_make_valid(text, -1);
  }
  return converted;
}
