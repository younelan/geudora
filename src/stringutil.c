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
#include <pango/pango.h>
#include <stdint.h>
#include <stdlib.h>
#pragma segment StringUtil
unsigned char IsWordChar[256] = {0};
#define PluralStrn 1000
#define SQUARE_LEFT 1001
#define SQUARE_RIGHT 1002

/* Quote822 is implemented in lex822.c */
extern unsigned char *Quote822(unsigned char *into, unsigned char *from, bool space);
/* GetHandleSize is implemented in fileutil.c */
extern long GetHandleSize(void **h);

void NumToString(long n, PStr s) {
  if (!s)
    return;
  sprintf((char *)s + 1, "%ld", n);
  s[0] = (unsigned char)strlen((char *)s + 1);
}

void NumToDot(unsigned long n, PStr s) {
  /* Stub: minimal IP formatting */
  sprintf((char *)s + 1, "%lu.%lu.%lu.%lu", (n >> 24) & 0xFF, (n >> 16) & 0xFF,
          (n >> 8) & 0xFF, n & 0xFF);
  s[0] = (unsigned char)strlen((char *)s + 1);
}

/* GetRString is declared in mailbox.h and implemented elsewhere */

PStr FormatString(unsigned int arg, PStr string, short format, short digits);
bool PPtrMatchLWSPSpot(PStr look, Ptr text, uint32_t textLen, UPtr *matchEnd);

/************************************************************************
 * AllDigits - is a string made up only of digits?
 ************************************************************************/
bool AllDigits(UPtr s, long len) {
  while (len && '0' <= *s && *s <= '9') {
    s++;
    len--;
  }
  return (len == 0);
}

/************************************************************************
 * HighBits - count high bits in a string
 ************************************************************************/
long HighBits(UPtr s, long len) {
  long count = 0;
  while (len-- > 0)
    if (*s++ > 127)
      count++;
  return count;
}

/**********************************************************************
 * BeginsWith - does one string begin with another?
 **********************************************************************/
bool BeginsWith(PStr string, PStr prefix) {
  uShort size;
  bool result;

  if (*string < *prefix)
    return (False);
  size = *string;
  *string = *prefix;
  result = StringSame((const char *)string, (const char *)prefix);
  *string = size;
  return (result);
}

/**********************************************************************
 * CaptureHex - read hex strings
 **********************************************************************/
void CaptureHex(PStr from, PStr to) {
  Str255 scratch;
  long len;

  len = *from;
  CaptureHexPtr((Ptr)(from + 1), (Ptr)(scratch + 1), &len);
  *scratch = len;
  PCopy(to, scratch);
}

/**********************************************************************
 * CaptureHexPtr - read hex strings
 **********************************************************************/
void CaptureHexPtr(Ptr from, Ptr to, long *pLen) {
  Ptr spot, end;
  Ptr toSpot, toEnd;

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
UPtr ComposeString(UPtr into, const char *format, ...) {
  va_list args;
  va_start(args, format);
  (void)VaComposeString(into, (UPtr)format, args);
  va_end(args);
  return (into);
}
UPtr ComposeRString(UPtr into, int format, ...) {
  va_list args;
  va_start(args, format);
  (void)VaComposeRString(into, format, args);
  va_end(args);
  return (into);
}
OSErr ComposeRTrans(TransStream stream, int format, ...) {
  Str255 into;
  va_list args;
  va_start(args, format);
  (void)VaComposeRString(into, format, args);
  va_end(args);
  return (SendPString(stream, into));
}
OSErr AccuComposeR(AccuPtr a, int format, ...) {
  Str255 into;
  va_list args;
  va_start(args, format);
  (void)VaComposeRString(into, format, args);
  va_end(args);
  return (AccuAddStr(a, into));
}
OSErr AccuCompose(AccuPtr a, PStr format, ...) {
  Str255 into;
  va_list args;
  va_start(args, format);
  (void)VaComposeString(into, format, args);
  va_end(args);
  return (AccuAddStr(a, into));
}
UPtr VaComposeRString(UPtr into, short format, va_list args) {
  Str255 stringFormat;

  GetRString(stringFormat, format);
  return (VaComposeString(into, stringFormat, args));
}

#define MAX_SUBS 5
UPtr VaComposeStringDouble(UPtr into, int maxInto, UPtr format, va_list args,
                           UPtr into2, int maxInto2, UPtr format2) {
  UPtr formatP;
  Str255 argString;
  long n;
  bool suppress;
  Str255 buffers[MAX_SUBS];
  long arg;
  short which;
  long resId;

top:
  which = 0;
  for (n = 0; n < MAX_SUBS; n++)
    buffers[n][0] = 0;

  *into = 0;
  for (formatP = format + 1; formatP < format + *format + 1; formatP++)
    if (*formatP == lowerOmega && which < MAX_SUBS) {
      formatP++;
      if (*formatP == lowerOmega)
        PMaxCatC(into, maxInto, *formatP);
      else {
        arg = va_arg(args, unsigned int);
        FormatString(arg, buffers[which++], *formatP, 0);
      }
    } else if (*formatP != '%')
      PMaxCatC(into, maxInto, *formatP);
    else {
      formatP++;
      if ((suppress = (*formatP == (unsigned char)0xA9)) !=
          0) /* suppress marker */
        formatP++;
      if (*formatP == '%')
        PMaxCatC(into, maxInto, '%');
      else {
        if (*formatP == '^' && '0' <= formatP[1] && formatP[1] <= '9') {
          PSCopy(argString, buffers[formatP[1] - '0']);
          formatP++;
          formatP++; /* skip format chars */
        } else if (*formatP == 'R') {
          resId = 0;
          while (formatP < format + *format && isdigit(formatP[1])) {
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
          arg = va_arg(args, unsigned int);
          if (suppress)
            *argString = 0;
          else
            FormatString(arg, argString, *formatP, digits);
        }
        if (maxInto <= 0)
          PCat(into, argString);
        else
          PSCat_C(into, argString, maxInto);
      }
    }

  into[*into + 1] = 0;

  // ugly hack here...
  if (into2) {
    into = into2;
    format = format2;
    maxInto = maxInto2;
    into2 = format2 = nil;
    maxInto2 = 0;
    goto top;
  }

  return (into);
}

/************************************************************************
 * EndsWith - does one string end with another?
 ************************************************************************/
bool EndsWith(PStr name, PStr suffix) {
  bool res;
  Byte c;
  UPtr spot;
  if (*name < *suffix)
    return (False); /* too short */

  spot = name + *name - *suffix; /* before start of putative suffix */
  c = *spot;                     /* save byte */
  *spot = *suffix;               /* pretend equal length */
  res = StringSame((const char *)suffix, (const char *)spot);
  *spot = c; /* restore byte */
  return (res);
}

/************************************************************************
 * HandleEndsWithR - does a handle end with a string from a resource?
 ************************************************************************/
bool HandleEndsWithR(Handle name, short index) {
  Str255 string;

  PCopy(string, *name);
  return (EndsWithR(string, index));
}

/************************************************************************
 * EndsWithR - does a string end with a suffix in a resource?
 ************************************************************************/
bool EndsWithR(PStr name, short resId) {
  Str255 suffix;

  GetRString(suffix, resId);
  return (EndsWith(name, suffix));
}

/************************************************************************
 * StartsWithR - does a string start with a prefix in a resource?
 ************************************************************************/
bool StartsWithR(PStr name, short resId) {
  Str255 prefix;

  GetRString(prefix, resId);
  return (StartsWith(name, prefix));
}

/************************************************************************
 * StartsWith - does a string start with a prefix?
 ************************************************************************/
bool StartsWith(PStr name, PStr prefix) {
  name[*name + 1] = 0;
  prefix[*prefix + 1] = 0;
  return *name >= *prefix && !striscmp(name + 1, prefix + 1);
}

/************************************************************************
 * StartsWithPtr - does a string start with a prefix?
 ************************************************************************/
bool StartsWithPtr(UPtr name, uint32_t len, PStr prefix) {
  return len >= *prefix && !strincmp(name, prefix + 1, *prefix);
}

/************************************************************************
 * EqualStrRes - is a string the same as a resource?
 ************************************************************************/
bool EqualStrRes(PStr string, short resId) {
  Str255 s;
  return (StringSame((const char *)GetRString(s, resId), (const char *)string));
}

/************************************************************************
 * EscapeChars - escape characters in a string
 ************************************************************************/
PStr EscapeChars(PStr string, PStr toEscape) {
  Str255 scratch;
  UPtr to = scratch + 1;
  UPtr from = string + 1;
  UPtr end = string + *string + 1;
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
      if (PIndex(toEscape, *from))
        *to++ = '\\';
      *to++ = *from;
    }

    from++;
  }

  *scratch = to - scratch - 1;

  /*
   * did we change anything?
   */
  if (*scratch != *string)
    PCopy(string, scratch);

  return (string);
}

/**********************************************************************
 * EscapeInHex - write hex strings safely
 **********************************************************************/
void EscapeInHex(PStr from, PStr to) {
  UPtr spot, end;
  Str255 scratch;
  UPtr toSpot, toEnd;

  spot = from + 1;
  end = spot + *from;

  toSpot = scratch + 1;
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

  *scratch = toSpot - scratch - 1;
  PCopy(to, scratch);
}

/**********************************************************************
 * Transmogrify - change one string into another, using two STR# for xlation
 **********************************************************************/
PStr Transmogrify(PStr toStr, short toId, PStr fromStr, short fromId) {
  short index;

  if ((index = FindSTRNIndex(fromId, fromStr)) != 0)
    GetRString(toStr, toId + index);
  else if (toStr != fromStr)
    PCopy(toStr, fromStr);
  return (toStr);
}

/************************************************************************
 * FixNewlines - remove cr, and turn nl into cr
 ************************************************************************/
void FixNewlines(UPtr string, long *count) {
  UPtr from, to;
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
PStr FormatString(unsigned int arg, PStr string, short format, short digits) {
  short n;
  struct hostInfo hi;

  *string = 0;

  switch (format) {
  case 'c':
    string[0] = 1;
    string[1] = arg;
    break;
  case 's': {
    const char *sarg = (const char *)(intptr_t)arg;
    *string = (unsigned char)strlen(sarg);
    *string = MIN(*string, 253);
    memmove(string + 1, sarg, *string);
    break;
  }
  case 'p':
    PCopy(string, (PStr)(intptr_t)arg);
    break;
  case 'e':
    PCopy(string, (PStr)(intptr_t)arg);
    EscapeInHex(string, string);
    break;
  case 'i':
    NumToDot(arg, string);
    break;
  case 'I':
    if (!GetHostByAddr(&hi, arg)) {
      *string = strlen(hi.cname);
      memmove(string + 1, hi.cname, *string);
    } else {
      NumToDot(arg, string + 1);
      string[0] = string[1] + 2;
      string[1] = '[';
      string[string[0]] = ']';
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
        ComposeString(string, "%d.%dK", arg / 10, arg % 10);
      else
        ComposeString(string, "%dK", arg / 10);
    } else if (arg < 1 K K) {
      arg /= 1 K;
      NumToString(arg, string);
      PCatC(string, 'K');
    } else if (arg < 10 K K) {
      arg *= 10;
      arg /= 1 K K;
      if (arg % 10)
        ComposeString(string, "%d.%dM", arg / 10, arg % 10);
      else
        ComposeString(string, "%dM", arg / 10);
    } else {
      arg /= 1 K K;
      NumToString(arg, string);
      PCatC(string, 'M');
    }
    break;
  case 'q':
    Quote822(string, (UPtr)(intptr_t)arg, True);
    break;
  case 'r':
    GetRString(string, arg);
    break;
  case 'b':
    n = *string = 32;
    for (string++; n; string++) {
      *string = arg & (1 << 31) ? '1' : '0';
      arg <<= 1;
      n--;
    }
    break;
  case 'x':
    Long2Hex(string, arg);
    if (digits) {
      while (*string < digits)
        PInsertC(string, 256, '0', string + 1);
      if (*string > digits) {
        memmove(string + 1, string + 1 + (*string - digits), digits);
        *string = digits;
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
    *string = 6;
    string[1] = string[6] = '\'';
    string[2] = ((Uptr)&arg)[0];
    string[3] = ((Uptr)&arg)[1];
    string[4] = ((Uptr)&arg)[2];
    string[5] = ((Uptr)&arg)[3];
    break;
  case 'o':
    *string = 4;
    string[1] = ((Uptr)&arg)[0];
    string[2] = ((Uptr)&arg)[1];
    string[3] = ((Uptr)&arg)[2];
    string[4] = ((Uptr)&arg)[3];
    break;
  case 'B':
    if (arg)
      CtoPCpy(string, "TRUE");
    else
      CtoPCpy(string, "FALSE");
    break;
  }

  return (string);
}

/************************************************************************
 * LCD - find the least common denom of two strings
 ************************************************************************/
PStr LCD(PStr s1, PStr s2) {
  UPtr p1, p2, end;
  char c1, c2;

  end = s1 + MIN(*s1, *s2) + 1;
  for (p1 = s1 + 1, p2 = s2 + 1; p1 < end; p1++, p2++)
    if (*p1 != *p2) {
      c1 = *p1;
      c2 = *p2;
      if (isupper(c1))
        c1 = tolower(c1);
      if (isupper(c2))
        c2 = tolower(c2);
      if (c1 != c2)
        break;
    }
  *s1 = p1 - s1 - 1;
  s1[*s1 + 1] = 0;
  return (s1);
}

/**********************************************************************
 * concatenate a pascal string on the end of another
 **********************************************************************/
UPtr PCat(PStr string, PStr suffix) {
  short sufLen;

  sufLen = MIN(255 - *string, *suffix);

  memmove(string + *string + 1, suffix + 1, sufLen);
  *string += sufLen;

  return (string);
}

/**********************************************************************
 * PCatR - concatenate a string from a resource to the end of a string
 **********************************************************************/
UPtr PCatR(PStr string, short resId) {
  Str255 suffix;

  GetRString(suffix, resId);
  return (PCat(string, suffix));
}

/************************************************************************
 * PCopyTrim - copy and trim a string
 ************************************************************************/
PStr PCopyTrim(PStr toString, PStr fromString, short max) {
  Str255 tString;
  PCopy(tString, fromString);
  TrimWhite(tString);
  TrimInitialWhite(tString);
  *tString = MIN(*tString, max - 1);
  PCopy(toString, tString);
  return (toString);
}

/**********************************************************************
 * concatenate a pascal string on the end of another
 * escape certain chars in the string
 **********************************************************************/
UPtr PEscCat(UPtr string, UPtr suffix, short escape, char *escapeWhat) {
  short sufLen;
  UPtr suffSpot, stringSpot;

  sufLen = *suffix;
  stringSpot = string + *string + 1;

  for (suffSpot = suffix + 1; sufLen--; suffSpot++) {
    if (*suffSpot == (unsigned char)escape || strchr(escapeWhat, *suffSpot))
      *stringSpot++ = (unsigned char)escape;
    *stringSpot++ = *suffSpot;
  }
  *string = (unsigned char)(stringSpot - string - 1);

  return (string);
}

/************************************************************************
 * PIndex - find a char in a pascal string
 ************************************************************************/
UPtr PIndex(PStr string, char c) {
  UPtr spot;

  for (spot = string + 1; spot < string + *string + 1; spot++)
    if (*spot == (unsigned char)c)
      return (spot);
  return (nil);
}

/************************************************************************
 * IndexPtr - find a char in a string specified by pointer and length
 ************************************************************************/
UPtr IndexPtr(UPtr string, long stringLen, char c) {
  UPtr spot;

  for (spot = string; spot < string + stringLen; spot++)
    if (*spot == (unsigned char)c)
      return (spot);
  return (nil);
}

/************************************************************************
 * PRIndex - find a char in a pascal string, backwards
 ************************************************************************/
UPtr PRIndex(PStr string, char c) {
  UPtr spot;

  for (spot = string + *string; spot > string; spot--)
    if (*spot == (unsigned char)c)
      return (spot);
  return (nil);
}

/**********************************************************************
 * PInsert - insert some text
 **********************************************************************/
PStr PInsert(PStr string, short size, PStr insert, UPtr spot) {
  short toInsert = MIN(*insert, size - *string - 1);

  if (toInsert > 0) {
    memmove(spot + toInsert, spot, *string - (spot - string - 1));
    memmove(spot, insert + 1, toInsert);
    *string += toInsert;
  }
  return (string);
}

/**********************************************************************
 * PInsertC - insert a single character
 **********************************************************************/
PStr PInsertC(PStr string, short size, Byte c, UPtr spot) {
  unsigned char s[16];

  *s = 1;
  s[1] = c;

  return (PInsert(string, size, s, spot));
}

/************************************************************************
 * PLCat - concat a long onto a string (preceed it with a space)
 ************************************************************************/
void PLCat(char *dst, long num) {
  unsigned char s[256];

  NumToString(num, s);
  PCat((PStr)dst, s);
}

/* GetHandleSize is implemented in fileutil.c */

/************************************************************************
 * PXCat - concat a long onto a string in hex
 ************************************************************************/
UPtr PXCat(UPtr string, long num) {
  unsigned char x[32];

  Bytes2Hex((void *)&num, sizeof(long), x + 1);
  *x = 8;
  return (PCat(string, x));
}

/************************************************************************
 * PXWCat - concat a short onto a string in hex
 ************************************************************************/
UPtr PXWCat(UPtr string, short num) {
  unsigned char x[32];

  Bytes2Hex((void *)&num, sizeof(short), x + 1);
  *x = 4;
  return (PCat(string, x));
}

/************************************************************************
 * Tr - translate text in a handle
 ************************************************************************/
bool Tr(Handle text, Uptr fromS, Uptr toS) {
  long len = GetHandleSize(text);
  return (TrLo(*text, len, fromS,
               toS)); // no handle lock; keep in segment with TrLo
}

/************************************************************************
 * TrLo - translate text in a pointer
 ************************************************************************/
bool TrLo(UPtr text, long len, Uptr fromS, Uptr toS) {
  UPtr end, spot;
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
UPtr PPtrFindSub(PStr sub, UPtr string, long len) {
  UPtr end = string + len - *sub + 1;
  UPtr stringSpot, subSpot, subEnd;
  Byte c1, c2;

  subEnd = sub + *sub + 1;
  while (string < end) {
    for (subSpot = sub + 1, stringSpot = string; subSpot < subEnd;
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
      return (stringSpot - *sub);
    string++;
  }
  return (nil);
}

/************************************************************************
 * PReplace - replace one string with another
 ************************************************************************/
PStr PReplace(PStr string, PStr find, PStr replace) {
  UPtr spot;

  if (*find && !EqualString(find, replace, true, true))
    while ((spot = PFindSub(find, string)) != NULL) {
      if (*string + *replace - *find > 255)
        break;
      memmove(spot + *replace, spot + *find,
              *string - (spot - string - 1) - *find);
      memmove(spot, replace + 1, *replace);
      *string += *replace - *find;
    }

  return (string);
}

/************************************************************************
 * PSCat_C - C routine to concat a string, worrying about length
 ************************************************************************/
UPtr PSCat_C(PStr string, PStr suffix, short max) {
  short tot = MIN(max - 1, *string + *suffix);
  short add = tot - *string;
  if (add > 0) {
    memmove(string + *string + 1, suffix + 1, add);
    *string += add;
  }
  return (string);
}

/**********************************************************************
 * copy a pascal string into a c string
 **********************************************************************/
/* PtoCcpy is provided as a static inline in legacy_shim.h */

/**********************************************************************
 * PStrCopy - copy a pascal string
 **********************************************************************/
PStr PStrCopy(PStr to, PStr from, short max) {
  long len = MIN(max, *from + 1); //	length includes length byte
  BlockMoveData(from, to, len);
  *to = len - 1;
  return to;
}

/**********************************************************************
 * InfiniteString - set string to all 0xFFs
 **********************************************************************/
PStr InfiniteString(PStr s, short size) {
  short i;

  *s = size - 1;
  for (i = 1; i <= size; i++)
    s[i] = 0xFF;
  return s;
}

/************************************************************************
 * ItemFromResAppearsInStr - does a string contain an item from a list of
 *  items in a resource
 ************************************************************************/
bool ItemFromResAppearsInStr(short resID, PStr string, UPtr delims) {
  Str255 s;
  unsigned char token[64];
  UPtr spot;

  // default delimitter is comma
  if (!delims)
    delims = (UPtr) ",";

  GetRString(s, resID);
  spot = s + 1;

  while (PToken(s, token, &spot, delims))
    if (PFindSub(token, string))
      return true;

  return false;
}

/************************************************************************
 * StrIsItemFromRes - is a string one of the items in a resource?
 ************************************************************************/
bool StrIsItemFromRes(PStr string, short resID, UPtr delims) {
  Str255 s;
  unsigned char token[64];
  UPtr spot;

  // default delimitter is comma
  if (!delims)
    delims = (UPtr) ",";

  GetRString(s, resID);
  spot = s + 1;

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
PStr PToken(PStr string, PStr token, UPtr *spotP, UPtr delims) {
  UPtr spot;
  UPtr end = string + *string + 1;
  UPtr tSpot = token + 1;

  *token = 0;
  if (*spotP >= end)
    return (nil);
  for (spot = *spotP; spot < end; spot++)
    if (!strchr((const char *)delims, (unsigned char)*spot))
      *tSpot++ = *spot;
    else
      break;
  *spotP = spot + 1;
  *token = (unsigned char)(tSpot - token - 1);
  return (token);
}

/************************************************************************
 * TokenPtr - grab a token out of text
 *  Returns boolean indicating if token found
 *  Saves state in spotP
 ************************************************************************/
bool TokenPtr(Ptr string, long stringLen, Ptr *token, long *tokenLen,
              UPtr *spotP, UPtr delims) {
  UPtr spot;
  UPtr end = (UPtr)string + stringLen;
  long len = 0;

  *token = (Ptr)*spotP;
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
bool PTokenPtr(Ptr string, long stringLen, Ptr token, UPtr *spotP,
               UPtr delims) {
  UPtr tokenPtr;
  long tokenLen;
  Boolean result;

  if ((result = TokenPtr(string, stringLen, (Ptr *)&tokenPtr, &tokenLen, spotP,
                         delims)) != 0)
    MakePPtr(token, tokenPtr, tokenLen);
  return result;
}

/**********************************************************************
 * ReMatch - does a string have a reply intro?
 **********************************************************************/
bool ReMatch(PStr string, PStr re) {
  UPtr colon;
  UPtr reSpot;
  Str255 remainder;

  if (*re) { /* intro string not empty */
    if ((colon = PIndex(string, re[*re])) != NULL) { /* final char appears */
      if (colon - string >= *re) {                   /* it's long enough */
        reSpot = re + *re;
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

        // ok, we didn't use up all of the subject.  See if it's a lyris-type
        // thing
        MakePStr(remainder, string + 1, colon - string + 1);
        TrimAllWhite(remainder);
        TrimSquares(remainder, true, true);
        return (*remainder == 0);
      }
    }
  }
  return (False);
}

/************************************************************************
 * TrimSquares - trim square-bracketed stuff from start of string
 ************************************************************************/
bool TrimSquares(PStr s, bool multiple, bool internal) {
  unsigned char left[32], right[32]; /* pascal strings: length byte + content */
  UPtr spot;
  bool result = false;

  GetRString(left, SQUARE_LEFT);
  GetRString(right, SQUARE_RIGHT);
  TrimInitialWhite(s);

  if (!internal) {
    while (*s > 2) {
      if ((spot = PIndex(left, (char)s[1])) != NULL) /* left delimiter */
      {
        if ((spot = PIndex(s, (char)right[spot - left])) !=
            NULL) /* found both delimiters! */
        {
          memmove(s + 1, spot + 1, *s - (spot - s));
          *s -= spot - s;
          TrimInitialWhite(s);
          result = true;
        } else
          break; /* didn't find it */
      } else
        break; /* didn't find it */
      if (!multiple)
        break;
    }
  } else {
    short brk;
    UPtr lPtr;
    UPtr rPtr;

    for (brk = 1; *s > 2 && brk <= *left; brk++) {
      if ((lPtr = PIndex(s, (char)left[brk])) != NULL)
        if ((rPtr = PIndex(s, (char)right[brk])) != NULL)
          if (lPtr < rPtr) {
            if (rPtr < s + *s)
              memmove(lPtr, rPtr + 1, *s - (lPtr - s - 1 + rPtr - lPtr + 1));
            *s -= rPtr - lPtr + 1;
            result = true;
            if (multiple)
              brk--; /* try again with this one */
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
void RemoveParens(UPtr string) {
  UPtr to, from, end;
  short pLevel = 0;

  for (to = from = string + 1, end = string + *string; from <= end; from++)
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
  *string = to - string - 1;
}

/************************************************************************
 * PStripChar - remove all occurrences of a char from a string
 ************************************************************************/
PStr PStripChar(PStr string, Byte c) {
  *string = StripChar((char *)(string + 1), *string, c);
  return (string);
}

/************************************************************************
 * StripChar - remove all occurrences of a char from text, return new length
 ************************************************************************/
long StripChar(Ptr string, long len, Byte c) {
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
int strincmp(UPtr s1, UPtr s2, short n) {
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
int striscmp(UPtr s1, UPtr s2) {
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
int strscmp(UPtr s1, UPtr s2) {
  register int c1, c2;
  for (c1 = *s1, c2 = *s2; c1 && c2; c1 = *++s1, c2 = *++s2)
    if (c1 - c2)
      return (c1 - c2);
  return (0);
}

/************************************************************************
 * Tokenize - set pointers to the beginning and end of a delimited token
 ************************************************************************/
UPtr Tokenize(UPtr string, int size, UPtr *start, UPtr *end, UPtr delims) {
  UPtr stop = string + size;
  char safe = (char)*stop;
  UPtr last;

  *stop = 0;
  while (strchr((const char *)delims, *string))
    string++;
  *stop = *delims;
  for (last = string; !strchr((const char *)delims, *last); last++)
    ;
  *stop = (unsigned char)safe;
  if (string == stop)
    return (nil);
  *start = string;
  *end = stop;
  return (string);
}

/**********************************************************************
 * TrimInitialWhite - remove whitespace characters from the beginning of a
 *string
 **********************************************************************/
PStr TrimInitialWhite(PStr s) {
  UPtr cp = s + 1;
  short len;

  for (cp = s + 1; cp <= s + *s && IsSpace(*cp); cp++)
    ;
  if (cp > s + 1 && cp <= s + *s) {
    len = *s - (cp - (s + 1));
    memmove(s + 1, cp, len);
    *s = len;
  }
  return (s);
}

/**********************************************************************
 * TrimInternalWhite - collapse internal whitespace into single
 **********************************************************************/
PStr TrimInternalWhite(PStr s) {
  bool wasWhite = false;
  bool isWhite;
  unsigned char *spot;
  unsigned char *end = s + *s;
  unsigned char *copySpot = s + 1;

  for (spot = s + 1; spot <= end; spot++) {
    isWhite = IsSpace(*spot);
    if (isWhite && wasWhite)
      continue;          /* if both white, skip */
    *copySpot++ = *spot; /* otherwise, copy the char */
    wasWhite = isWhite;  /* remember if we were looking at whitespace */
  }
  *s = (unsigned char)(copySpot - s - 1);

  return (s);
}

/************************************************************************
 * TrimPrefix - strip a prefix from a string
 ************************************************************************/
bool TrimPrefix(UPtr string, UPtr prefix) {
  short oldLen = *string;

  if (oldLen < *prefix)
    return (False);

  *string = *prefix;
  if (StringSame((const char *)string, (const char *)prefix)) {
    memmove(string + 1, string + 1 + *prefix, oldLen - *prefix);
    *string = oldLen - *prefix;
    return (True);
  } else {
    *string = oldLen;
    return (False);
  }
}

/**********************************************************************
 * StringSame - are two strings the same?
 **********************************************************************/
bool StringSame(const char *s1, const char *s2) {
  const unsigned char *us1 = (const unsigned char *)s1;
  const unsigned char *us2 = (const unsigned char *)s2;
  if (*us1 != *us2)
    return false; /* quick length test */
  /* Case-insensitive pascal string compare (ignoring FurrinSort locale) */
  return (strncasecmp((const char *)us1 + 1, (const char *)us2 + 1, *us1) == 0);
}

/**********************************************************************
 * StringComp - return whether s1<s2 (negative), s1==s2 (0), s1>s2 (positive)
 **********************************************************************/
long StringComp(PStr s1, PStr s2) {
  /* Portable case-insensitive comparison of pascal strings */
  int minlen = (*s1 < *s2) ? *s1 : *s2;
  int result = strncasecmp((const char *)s1 + 1, (const char *)s2 + 1, minlen);
  if (result != 0)
    return result;
  return (int)*s1 - (int)*s2;
}

/**********************************************************************
 * MyUpperText - uppercase text with or without fancy furrin stuff
 **********************************************************************/
void MyUpperText(UPtr buffer, long bufferSize) {
  short i;
  for (i = 0; i < bufferSize; i++)
    buffer[i] = g_ascii_toupper(buffer[i]);
}

/**********************************************************************
 * MyLowerText - lowercase text with or without fancy furrin stuff
 **********************************************************************/
void MyLowerText(UPtr buffer, long bufferSize) {
  short i;
  for (i = 0; i < bufferSize; i++)
    buffer[i] = g_ascii_tolower(buffer[i]);
}

/**********************************************************************
 * MyLowercaseText - call lowercasetext carefully
 **********************************************************************/
OSErr MyLowercaseText(UPtr text, long len) {
  MyLowerText(text, len);
  return (0);
}

/**********************************************************************
 * TrimReLo - remove an Re: string
 **********************************************************************/
bool TrimReLo(PStr string, PStr re) {
  UPtr colon;
  if (ReMatch(string, re)) {
    colon = PIndex(string, re[*re]);
    while (IsWhite(colon[1]) && colon < string + *string - 1)
      colon++;
    memmove(string + 1, colon + 1, *string - (colon - string));
    *string -= colon - string;
    return (True);
  }
  return (False);
}

/************************************************************************
 * TrimRe - trim Re: and Fwd: from a string
 ************************************************************************/
bool TrimRe(PStr string, bool squares) {
  bool did = False;

  while (TrimReLo(string, (PStr)Re) || TrimReLo(string, (PStr)Fwd) ||
         TrimReLo(string, (PStr)OFwd) ||
         (squares && TrimSquares(string, false, false)))
    did = True;

  return (did);
}

/**********************************************************************
 * TrimWhite - remove whitespace characters from the end of a string
 **********************************************************************/
PStr TrimWhite(PStr s) {
  register int len = *s;
  register UPtr cp = s + len;

  while (len && IsSpace(*cp))
    cp--, len--;

  *s = len;
  return (s);
}

/**********************************************************************
 * CollapseLWSP - convert lwsp runs to single spaces
 **********************************************************************/
PStr CollapseLWSP(PStr s) {
  UPtr to, from, end;
  bool space = true; // beginning of string counts as space
  bool lwsp;

  end = s + *s + 1;
  for (to = from = s + 1; from < end; from++) {
    // is current char space?
    lwsp = IsLWSP(*from);
    if (space && lwsp)
      continue; // skip subsequent space

    // if we are adding lwsp, add a space
    if (lwsp)
      *to++ = ' ';
    // for regular characters, just add
    else
      *to++ = *from;

    space = lwsp;
  }

  // count chars
  *s = to - s - 1;

  // strip trailing space, if there is one
  if (s[*s] == ' ')
    --*s;

  // ta da
  return s;
}

/**********************************************************************
 * IsAllWhitePtr - is a string all whitespace?
 **********************************************************************/
bool IsAllWhitePtr(UPtr s, long len) {
  for (; len-- > 0; s++)
    if (!IsWhite(*s))
      return false;
  return true;
}

/**********************************************************************
 * IsAllLWSPPtr - is a string all lwsp?
 **********************************************************************/
bool IsAllLWSPPtr(UPtr s, long len) {
  for (; len-- > 0; s++)
    if (!IsLWSP(*s))
      return false;
  return true;
}

/**********************************************************************
 * IsAllUpper - is a string all lwsp?
 **********************************************************************/
bool IsAllUpper(PStr s) {
  UPtr end = s + *s + 1;

  if (!*s)
    return false;

  for (s++; s < end; s++)
    if (!isupper(*s))
      return false;

  return true;
}

/**********************************************************************
 * Uncomma - reformat a name to remove a comma
 **********************************************************************/
PStr Uncomma(PStr name) {
  Str255 scratch;
  UPtr comma;

  if ((comma = PIndex(name, ':')) != NULL)
    *name = comma - name - 1;
  else if (isupper(name[1]) && (comma = PIndex(name, ',')) != NULL) {
    MakePStr(scratch, comma + 1, *name - (comma - name));
    *name = *name - *scratch - 1;
    TrimInitialWhite(scratch);
    TrimWhite(scratch);
    TrimInitialWhite(name);
    TrimWhite(name);
    PInsertC(name, *name + 2, ' ', name + 1); /* insert a single space */
    PInsert(name, *name + *scratch + 2, scratch, name + 1);
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
PStr UTF8ToMac(PStr string) {
  long len = *string;

  UTF8To88591((char *)(string + 1), *string, (char *)(string + 1), &len);
  *string = (unsigned char)len;
  /* TransLitRes(ktISOMac) not available in GTK port; UTF-8→Latin-1 is
   * sufficient */
  return (string);
}

/************************************************************************
 * UTF8To88591 - convert utf8 to 8859-1
 ************************************************************************/
void UTF8To88591(Ptr inStr, long inLen, Ptr outStr, long *outLen) {
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
void MyStringToNum(PStr string, long *num) {
  if (!*string || !(isdigit(string[1]) || string[1] == '-' || string[1] == '+'))
    *num = 0L;
  else {
    char buf[256];
    memcpy(buf, string + 1, *string);
    buf[*string] = '\0';
    *num = strtol(buf, NULL, 10);
  }
}

/************************************************************************
 * PtrPtrMatchLWSP - match two strings, considering all LWSP as the same
 ************************************************************************/
bool PtrPtrMatchLWSP(Ptr lookFor, short lookLen, Ptr text, uint32_t textLen,
                     bool atStart, bool atEnd) {
  Str255 shortLook;
  Ptr spot, end;
  Ptr matchEnd;

  lookLen = MIN(lookLen, 250);

  // First, copy the string being looked for, collapsing LWSP
  end = lookFor + lookLen;
  spot = (Ptr)(shortLook + 1);
  while (lookFor < end) {
    // For lwsp, copy a single space
    if (IsLWSP((unsigned char)*lookFor)) {
      *spot++ = ' ';
      do {
        lookFor++;
      } while (lookFor < end && IsLWSP((unsigned char)*lookFor));
    }
    // Copy anything else
    else
      *spot++ = *lookFor++;
  }
  *shortLook = (unsigned char)(spot - (Ptr)(shortLook + 1));
  TrimAllWhite(shortLook);
  if (!*shortLook)
    return true; // empty matches all

  // does match need to be at beginning?
  if (atStart) {
    if (PPtrMatchLWSPSpot(shortLook, text, textLen, (UPtr *)&matchEnd)) {
      if (atEnd)
        return matchEnd == text + textLen;
      else
        return true;
    } else {
      return false;
    }
  }

  // Now, test at each spot in the string
  end = text + textLen - *shortLook + 1;
  spot = text;

  while (spot < end) {
    if (PPtrMatchLWSPSpot(shortLook, spot, (uint32_t)textLen,
                          (UPtr *)&matchEnd))
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
bool PPtrMatchLWSPSpot(PStr look, Ptr text, uint32_t textLen, UPtr *matchEnd) {
  Ptr textEnd = text + textLen;
  UPtr lookEnd = look + *look + 1;
  UPtr lookSpot = look + 1;
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
    *matchEnd = (UPtr)text;

  return true;
}

/************************************************************************
 * PStrToNum - StringToNum as a function
 ************************************************************************/
long PStrToNum(PStr string) {
  long num = 0;
  MyStringToNum(string, &num);
  return num;
}

/************************************************************************
 * ShortVersString - turn a short into an x.x.x.x version string
 ************************************************************************/
UPtr ShortVersString(short vers, UPtr versionStr) {
  Str255 scratch, hex;
  unsigned char *scan;

  *versionStr = 0;
  *hex = 0;

  ComposeString(scratch, "%x", vers);
  scan = scratch + 1;

  // skip all leading 0's
  while ((scan <= (scratch + scratch[0])) && (*scan == '0'))
    scan++;

  while (scan <= (scratch + scratch[0])) {
    PCatC(hex, *scan);
    PCatC(hex, '.');
    scan++;
  }
  hex[0]--; // take off trailing period

  PCopy(versionStr, hex);
  return (versionStr);
}

/************************************************************************
 * StripLeadingItems - strip items from the beginning of a string
 ************************************************************************/
PStr StripLeadingItems(PStr string, short resID) {
  Str255 s;
  unsigned char token[64];
  UPtr spot;

  GetRString(s, resID);
  spot = s + 1;

  while (PToken(s, token, &spot, (UPtr) ",")) {
    if (BeginsWith(string, token)) {
      memmove(string + 1, string + 1 + *token, *string - *token);
      *string -= *token;
      break;
    }
  }

  return string;
}

/************************************************************************
 * StripTrailingItems - strip items from the end of a string
 ************************************************************************/
PStr StripTrailingITems(PStr string, short resID) {
  Str255 s;
  unsigned char token[64];
  UPtr spot;

  GetRString(s, resID);
  spot = s + 1;

  while (PToken(s, token, &spot, (UPtr) ",")) {
    if (EndsWith(string, token)) {
      *string -= *token;
      break;
    }
  }

  return string;
}

/************************************************************************
 * EndsWithItem - does a string end with one of these items?
 ************************************************************************/
bool EndsWithItem(PStr string, short resID) {
  Str255 s;
  unsigned char token[64];
  UPtr spot;

  GetRString(s, resID);
  spot = s + 1;

  while (PToken(s, token, &spot, (UPtr) ","))
    if (EndsWith(string, token))
      return true;

  return false;
}
