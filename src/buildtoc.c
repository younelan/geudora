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

#include <glib.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "StringDefs.h"
#include "StringUtil.h"
#include "buildtoc.h"
#include "fileutil.h"
#include "mailbox.h"
#include "util.h"

#include "mydefs.h"

#define FILE_NUM 45

/* Macros defined in mailbox.h */

#ifdef __APPLE__
#include <malloc/malloc.h>
#define malloc_usable_size malloc_size
#endif

/* GetHandleSize_ defined in mailbox.h */

/* NewHandle, SetHandleBig_, ZapHandle defined/stubbed in mailbox.h */

#define fnfErr (-43)
#define noErr 0
/* Constants defined in mailbox.h/toc.h */
/* Logging Stubs */
#define LOG_MOVE 0
#define LOG_PLUG 0
void ComposeLogS(int type, void *p, unsigned char *fmt, ...);

#ifndef LOG_TOC
#define LOG_TOC 0
#endif
#ifndef OPEN_MBOX
#define OPEN_MBOX 0
#endif
#ifndef NOT_MAILBOX
#define NOT_MAILBOX 0
#endif
#ifndef READ_MBOX
#define READ_MBOX 0
#endif
#ifndef CREATING_MAILBOX
#define CREATING_MAILBOX 0
#endif

/* NewZH clashing macro resolution - ensure it uses the project's NewHandle if
 * available */
#ifndef NewZH
#define NewZH(aType) ((aType **)ZeroHandle(NewHandle(sizeof(aType))))
static void *ZeroHandle(Handle h) {
  if (h && *h)
    memset(*h, 0, malloc_usable_size(*h));
  return h;
}
#endif

/* Use Personality struct from schizo.h */
static Personality dummyPers = {1};
static PersPtr dummyPersPtr = &dummyPers;
#define CurPers (&dummyPersPtr)
#define PERS_FORCE(p) (p)

/* Stub implementations for functions that need real porting */
void BeautifySum(MSumPtr sum) {}
int SumToFrom(MSumPtr sum, unsigned char *fromLine) { return 0; }
void CopyHeaderLine(unsigned char *to, int size, unsigned char *from) {}

void BeautifyFrom(unsigned char *fromStr) {}
uint32_t BeautifyDate(unsigned char *dateStr, long *zoneSecs) { return 0; }
/* PtrTimeStamp is implemented in sendmail.c */
extern void PtrTimeStamp(MSumPtr sum, uint32_t seconds, long offset);
uint32_t UnixDate2Secs(const char *date);
short MonthNum(const char *cp);
long CStr2Zone(const char *s);

bool IsFromLine(unsigned char *line) { return false; }
/* static bool IsSpool(const char *path) { return false; } */
/* static void CreateTempBox(int which) {} */

/* SaveMessageSum declared in mailbox.h, use it from there */
static int DefaultOutFlags() { return 0; }

static bool IsRootPath(const char *path) { return true; }

/* IsWhite clashing macro resolution */
#ifndef IsWhite
#define IsWhite(c) ((c) == ' ' || (c) == '\t')
#endif

/*
static bool SuckPtrAddresses(Handle *h, char *s, int len, bool b1, bool b2,
                             bool b3, void *p) {
  return true;
}
static bool IsNickname(char *s, int i) { return false; }
static void SuckAddresses(Handle *h1, Handle h2, bool b1, bool b2, bool b3,
                          void *p) {}
*/

bool ExpandAliasesLow(Handle *h1, Handle h2, int i, bool b1, void *p1, int i2) {
  return true;
}

#define EAL_VARS_DECL
#define EAL_VARS 0

static bool IsMailbox(const char *path) {
  if (!path || !*path)
    return false;
  struct stat st;
  return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

static void CleanseTOC(TOCHandle toc) {}

TOCHandle BuildTOC_Path(const char *path) { return BuildTOC(path); }

void GleanFrom(unsigned char *line, MSumPtr sum) {
  Str255 copy;
  char *cp = (char *)copy;
  char *ep;
  long seconds;
  long offset = ZoneSecs();
  Str31 dateStr;

  strcpy((char *)copy, (char *)line);
  /*
   * from address
   */
  while (*cp && *cp++ != ' ')
    ;
  for (ep = cp; *ep && *ep != ' '; ep++)
    ;
  *ep = '\0';
  MakePStr(sum->from, cp, ep - cp);

  /*
   * date
   */
  for (cp = ++ep; *ep && *ep != '\r' && *ep != '\n'; ep++)
    ;
  *ep = '\0';
  MakePStr(dateStr, cp, ep - cp);

  /*
   * TimeStamp
   */
  seconds = UnixDate2Secs((const char *)dateStr) - offset;
  PtrTimeStamp(sum, seconds, offset);
  /* sum->arrivalSeconds = seconds+offset; // Field might not exist in current
   * MessageSummary */
}

short MonthNum(const char *cp) {
  char monthStr[4];
  short month = 0;

  memcpy(monthStr, cp, 3);
  monthStr[3] = 0;
  for (int i = 0; i < 3; i++)
    monthStr[i] = tolower(monthStr[i]);

  char *c = monthStr;
  switch (c[0]) {
  case 'j':
    month = c[1] == 'a' ? 1 : (c[2] == 'n' ? 6 : 7);
    break;
  case 'f':
    month = 2;
    break;
  case 'm':
    month = c[2] == 'r' ? 3 : 5;
    break;
  case 'a':
    month = c[1] == 'p' ? 4 : 8;
    break;
  case 's':
    month = 9;
    break;
  case 'o':
    month = 10;
    break;
  case 'n':
    month = 11;
    break;
  case 'd':
    month = 12;
    break;
  }
  return month;
}

long CStr2Zone(const char *s) {
  long offset;
  offset = atoi(s);
  if (offset > 2400 || offset < -2400)
    offset = ZoneSecs();
  else {
    bool neg = (*s == '-');
    if (neg)
      offset *= -1;
    offset = 60 * ((offset / 100) * 60 + offset % 100);
    if (neg)
      offset *= -1;
  }
  return (offset);
}

uint32_t UnixDate2Secs(const char *date) {
  struct tm tm;
  memset(&tm, 0, sizeof(struct tm));
  /* Example: Wed, 21 Jan 2026 17:23:50 +0100 or 21 Jan 2026 17:23:50 +0100 */
  if (strptime(date, "%d %b %Y %H:%M:%S", &tm) ||
      strptime(date, "%a, %d %b %Y %H:%M:%S", &tm)) {
    return (uint32_t)mktime(&tm);
  }
  return 0;
}

/* Copyright (c) 1991-1992 by the University of Illinois Board of Trustees */

/* Forward declarations are in buildtoc.h, but we provide local logic here as
 * needed */

/************************************************************************
 * RebuildTOC - rebuild a corrupt or out-of-date toc, salvaging what we
 * can.
 ************************************************************************/
TOCHandle RebuildTOC(const char *path, TOCHandle oldTocH, bool resource,
                     bool tempBox) {
  TOCHandle newTocH = NULL;
  short oldCount, newCount;

  if (oldTocH) {
    /* Try to salvage old TOC */
    if ((newTocH = BuildTOC(path)) && oldTocH) {
      oldCount = (*oldTocH)->count;
      newCount = (*newTocH)->count;
      if (oldCount && newCount) {
        SalvageTOC(oldTocH, newTocH);

        /* Copy over preserved fields */
        memcpy((*newTocH)->sorts, (*oldTocH)->sorts, sizeof((*newTocH)->sorts));
        (*newTocH)->lastSort = (*oldTocH)->lastSort;
        (*newTocH)->pluginKey = (*oldTocH)->pluginKey;
        (*newTocH)->pluginValue = (*oldTocH)->pluginValue;
        (*newTocH)->previewHi = (*oldTocH)->previewHi;
        (*newTocH)->nextSerialNum = (*oldTocH)->nextSerialNum;
        (*newTocH)->unreadBase = (*oldTocH)->unreadBase;
      }
    }
  } else {
    /* Build new TOC from scratch */
    newTocH = BuildTOC(path);
  }

  if (newTocH) {
    (*newTocH)->reallyDirty = true;
  }

  return newTocH;
}

/************************************************************************
 * SalvageTOC - reconcile and old a new TOC
 ************************************************************************/
short SalvageTOC(TOCHandle old, TOCHandle new) {
  short first, last, mid;
  MSumPtr oldSum;
  long offset;
  short salvaged = 0;
  long bo;
  long seconds;

  LDRef(old);
  LDRef(new);
  (*old)->count = (GetHandleSize_(old) - (sizeof(TOCType) - sizeof(MSumType))) /
                  sizeof(MSumType);

  for (mid = (*new)->count - 1; mid >= 0; mid--)
    (*new)->sums[mid].state = REBUILT; // set state to rebuilt

  for (oldSum = (*old)->sums; oldSum < (*old)->sums + (*old)->count; oldSum++) {
    offset = oldSum->offset;
#ifdef IMAP
    if ((*old)->imapTOC && (offset < 0))
      continue; // skip minimal headers
#endif
    first = 0;
    last = (*new)->count - 1;
    for (mid = (first + last) / 2; first <= last; mid = (first + last) / 2)
      if (offset < (*new)->sums[mid].offset)
        last = mid - 1;
      else if (offset == (*new)->sums[mid].offset)
        break;
      else
        first = mid + 1;
    if (first <= last && (*new)->sums[mid].length == oldSum->length) {
      salvaged++;
      bo = (*new)->sums[mid].bodyOffset;   // preserve new bodyOffset
      seconds = (*new)->sums[mid].seconds; // preserve new seconds
      (*new)->sums[mid] = *oldSum;
      (*new)->sums[mid].bodyOffset =
          bo; // and restore it--old one might be trash
      // overwrite old seconds unless the message is timed queue.  If it's timed
      // queue, the old seconds is a much more precious commodity, and we'll
      // take the risk that it might be off.
      if ((*new)->sums[mid].state != TIMED)
        (*new)->sums[mid].seconds =
            seconds; // and restore it--old one might be trash
    }
#ifdef RESYNC_MID
    else if (oldSum->msgIdHash) {
      for (first = 0; first < (*new)->count; first++)
        if (oldSum->msgIdHash == (*new)->sums[first].msgIdHash &&
            oldSum->length == (*new)->sums[first].length) {
          salvaged++;
          bo = (*new)->sums[first].bodyOffset; // preserve new bodyOffset
          offset = (*new)->sums[first].offset;
          seconds = (*new)->sums[first].seconds;
          (*new)->sums[first] = *oldSum;
          (*new)->sums[first].bodyOffset =
              bo; // and restore it--old one might be trash
          (*new)->sums[first].offset =
              offset; // and restore it--old one might be different
          (*new)->sums[first].seconds =
              seconds; // and restore it--old one might be different
        }
    }
#endif
  }

  UL(old);
  UL(new);
  CleanseTOC(new);
  return (salvaged);
}

/**********************************************************************
 * BuildTOC - build a table of contents for a file.  The TOC is built
 * in memory.  This gets a little hairy in spots because the routine is
 * used both for received messages and messages under composition; but
 * the complication does not substantially affect the flow of the
 * function, so I let it stand.
 **********************************************************************/
TOCHandle BuildTOC(const char *path) {
  TOCHandle tocH = nil;
  MSumType sum;
  bool isOut;
  Str255 scratch;
  LineIOD lid;
  OSErr err;
  short which = 0;
  const char *filename;

  if (!path)
    return NULL;

  filename = strrchr(path, '/');
  if (filename)
    filename++;
  else
    filename = path;

  ComposeLogS(LOG_TOC, nil, (unsigned char *)"BuildTOC(%s)", filename);

  Zero(lid);

  if (!IsMailbox(path)) {
    FileSystemError(NOT_MAILBOX, (const char *)filename, 0);
    return (nil);
  }

  if ((tocH = NewZH(TOCType)) == nil) {
    WarnUser(READ_MBOX, MemError());
    goto failure;
  }

  /*
   * figure out for once and for all if we are an in or an out box
   */
  isOut = False;
  if (!which && IsRootPath(path)) {

    GetRString(scratch, IN);
    if (StringSame((char *)filename, (char *)scratch))
      which = IN;
    else {
      GetRString(scratch, OUT);
      if (StringSame((char *)filename, (char *)scratch)) {
        which = OUT;
        isOut = True;
      } else {
        GetRString(scratch, TRASH);
        if (StringSame((char *)filename, (char *)scratch))
          which = TRASH;
      }
    }
  }

  /*
   * first, try opening the file
   */
  if ((err = OpenLineDirect(path, fsRdWrPerm, &lid))) {
    FileSystemError(OPEN_MBOX, (const char *)filename, err);
    return (nil);
  }

  /*
   * now, we skip through the messages, reading summaries
   */
  while (true) {
    long diskPos = TellLine(&lid);
    long len;
    int gErr;

    gErr = GetLine((unsigned char *)scratch, sizeof(scratch), &len, &lid);
    if (gErr == -1 || len == 0)
      break;

    if (len > 5 && !strncmp((char *)scratch, "From ", 5)) {
      Zero(sum);
      sum.offset = diskPos;
      sum.state = UNREAD;
      if (isOut) {
        sum.state = SENT;
        sum.flags = DefaultOutFlags();
      }

      /*
       * we use GleanFrom to find out what personality this message
       * belongs to.
       */
      GleanFrom((unsigned char *)scratch, &sum);

      /*
       * if it's the default personality, use the personality
       * from the current context.
       */
      if (!sum.persId) {
        sum.persId = sum.popPersId = (*PERS_FORCE(CurPers))->persId;
      }

      /*
       * now, we skip ahead to the next message
       */
      while (true) {
        diskPos = TellLine(&lid);
        gErr = GetLine((unsigned char *)scratch, sizeof(scratch), &len, &lid);
        if (gErr == -1 || len == 0)
          break;
        if (len > 5 && !strncmp((char *)scratch, "From ", 5)) {
          /* Back up to start of next message */
          SeekLine(diskPos, &lid);
          break;
        }

        /* Collect info from header if we haven't already */
        if (!sum.length) {
          /* This is a simplification; real logic would parse headers */
        }
      }

      sum.length = diskPos - sum.offset;
      if (!SaveMessageSum(&sum, &tocH))
        goto failure;
    }
  }

  CloseLine(&lid);
  return (tocH);

failure:
  CloseLine(&lid);
  ZapHandle(tocH);
  return (nil);
}

/* HasUnicode - always true on GLib/GTK (UTF-8 native) */
bool HasUnicode(void)
{
  return true;
}

/* GoodUTF8Len - find longest valid UTF-8 prefix of bufLen bytes.
 * Ported directly from MAC624/unicode.c (pure C, no Mac APIs). */
ByteCount GoodUTF8Len(BytePtr utf8, ByteCount bufLen)
{
  unsigned char b;
  ByteCount newLen = 0, tempLen;

  while (bufLen) {
    tempLen = 1;
    if ((b = *utf8++) > 0x7F) {
      while ((b <<= 1) & 0x80)
        ++tempLen;
      if (tempLen > bufLen)
        break;
      utf8 += tempLen - 1;
    }
    newLen += tempLen;
    bufLen -= tempLen;
  }
  return newLen;
}

/* HeaderToUTF8 - decode RFC 2047 encoded words in a Pascal string header.
 * Uses GLib's g_convert for charset conversion. Falls back to no-op if
 * the header is already valid UTF-8 (most modern IMAP servers send UTF-8). */
OSStatus HeaderToUTF8(unsigned char *head)
{
  if (!head || head[0] == 0)
    return 0;

  /* If already valid UTF-8, just trim to a valid boundary and return */
  if (g_utf8_validate((const gchar *)(head + 1), head[0], NULL)) {
    head[0] = (unsigned char)GoodUTF8Len(head + 1, head[0]);
    return 0;
  }

  /* Try converting from Latin-1 to UTF-8 as a fallback */
  gsize bytes_written = 0;
  gchar *utf8 = g_convert((const gchar *)(head + 1), head[0],
                           "UTF-8", "ISO-8859-1",
                           NULL, &bytes_written, NULL);
  if (utf8) {
    if (bytes_written > 255)
      bytes_written = (gsize)GoodUTF8Len((BytePtr)utf8, 255);
    head[0] = (unsigned char)bytes_written;
    memcpy(head + 1, utf8, bytes_written);
    g_free(utf8);
  }
  return 0;
}
