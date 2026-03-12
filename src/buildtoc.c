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
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "buildtoc.h"
#include "fileutil.h"
#include "mailbox.h"
#include "mydefs.h"
#include "StringDefs.h"
#include "util.h"

#ifndef OPT_BULK
#define OPT_BULK 0x0010
#endif

#define FILE_NUM 45

/* fnfErr and noErr defined in mailbox.h */

#ifndef IsWhite
#define IsWhite(c) ((c) == ' ' || (c) == '\t')
#endif

/* Allocate a TOCType directly with malloc (no Mac Handle indirection) */
static TOCType *AllocTOC(void) {
  TOCType *toc = calloc(1, sizeof(TOCType));
  return toc;
}

/* Grow a TOCType to hold `count` message summaries using realloc */
static TOCType *GrowTOC(TOCType *toc, short newCount) {
  size_t need = sizeof(TOCType) + (newCount > 0 ? newCount - 1 : 0) * sizeof(MSumType);
  TOCType *grown = realloc(toc, need);
  return grown;
}

/* Simple hash for Message-ID strings */
static uint32_t HashString(const char *s, long len) {
  uint32_t hash = 5381;
  for (long i = 0; i < len; i++)
    hash = ((hash << 5) + hash) + (unsigned char)s[i];
  return hash ? hash : 1; /* never return 0, that means "no hash" */
}

#define ValidHash(h) ((h) != 0)

/* PtrTimeStamp is in sendmail.c */
extern void PtrTimeStamp(MSumPtr sum, uint32_t seconds, long offset);

/* ZoneSecs is in util.c */
extern long ZoneSecs(void);

/* CleanseTOC is in toc.c */
extern void CleanseTOC(TOCType * toc);

/* IsIMAPMailboxFileLo is in imapmailboxes.c */
extern bool IsIMAPMailboxFileLo(FSSpecPtr spec, MailboxNodeHandle *node);

static int DefaultOutFlags(void) { return 0; }

/************************************************************************
 * IsFromLine - determine whether or not a given line is a sendmail From
 * line.  Ported from original Eudora — validates the date portion.
 ************************************************************************/
bool IsFromLine(unsigned char *line) {
  int num, len;
  int quote = 0;
  char scratch[256];
  char *cp;
  short weekDay, year, tym, day, month, other, remote, from;

#define isdig(c) ('0' <= (c) && (c) <= '9')

  if (line[0] != 'F' || line[1] != 'r' || line[2] != 'o' || line[3] != 'm')
    return false;

  /* check for the space after "From" */
  unsigned char *lp = line + 4;
  if (*lp++ != ' ')
    return false;

  /* skip the return address (may contain quoted strings) */
  while (*lp && (quote || *lp != ' ')) {
    if (*lp == '"')
      quote = !quote;
    lp++;
  }
  if (!*lp++)
    return false;
  while (*lp == ' ')
    lp++;

  remote = from = weekDay = day = year = tym = month = other = 0;
  len = strlen((char *)lp);
  if (len > (int)sizeof(scratch) - 1)
    return false;
  strcpy(scratch, (char *)lp);

  for (cp = strtok(scratch, " \t\r\n,"); cp; cp = strtok(NULL, " \t\r\n,")) {
    len = strlen(cp);
    num = atoi(cp);

    if (num < 24 && len >= 5 && cp[2] == ':' &&
        (len == 5 || (len == 8 && cp[5] == ':'))) {
      if (tym++)
        return false;
    } else if (!year && day && len == 2 && isdig(cp[len - 1])) {
      if (year++)
        return false;
    } else if (len <= 2 && num && num < 32) {
      if (day++)
        return false;
    } else if (len == 4 && num > 1900) {
      if (year++)
        return false;
    } else if (len == 6 && !strcasecmp(cp, "remote")) {
      if (remote++ || from)
        return false;
    } else if (len == 4 && !strcasecmp(cp, "from")) {
      if (!remote || from++)
        return false;
    } else if (len == 3 &&
               !(strcasecmp(cp, "mon") && strcasecmp(cp, "tue") &&
                 strcasecmp(cp, "wed") && strcasecmp(cp, "thu") &&
                 strcasecmp(cp, "fri") && strcasecmp(cp, "sat") &&
                 strcasecmp(cp, "sun"))) {
      if (weekDay++)
        return false;
    } else if (len == 3 &&
               !(strcasecmp(cp, "jan") && strcasecmp(cp, "feb") &&
                 strcasecmp(cp, "mar") && strcasecmp(cp, "apr") &&
                 strcasecmp(cp, "may") && strcasecmp(cp, "jun") &&
                 strcasecmp(cp, "jul") && strcasecmp(cp, "aug") &&
                 strcasecmp(cp, "sep") && strcasecmp(cp, "oct") &&
                 strcasecmp(cp, "nov") && strcasecmp(cp, "dec"))) {
      if (month++)
        return false;
    } else {
      other++;
    }
  }
  return (day && year && month && tym && other <= 2);
}

/************************************************************************
 * MonthNum - get the month number from a month name
 ************************************************************************/
short MonthNum(const char *cp) {
  char m[4];
  memcpy(m, cp, 3);
  m[3] = 0;
  for (int i = 0; i < 3; i++)
    m[i] = tolower(m[i]);

  switch (m[0]) {
  case 'j':
    return m[1] == 'a' ? 1 : (m[2] == 'n' ? 6 : 7);
  case 'f':
    return 2;
  case 'm':
    return m[2] == 'r' ? 3 : 5;
  case 'a':
    return m[1] == 'p' ? 4 : 8;
  case 's':
    return 9;
  case 'o':
    return 10;
  case 'n':
    return 11;
  case 'd':
    return 12;
  }
  return 0;
}

/************************************************************************
 * CStr2Zone - convert a timezone string like "+0100" or "-0500" to seconds
 ************************************************************************/
long CStr2Zone(const char *s) {
  long offset = atoi(s);
  if (offset > 2400 || offset < -2400)
    return ZoneSecs();
  bool neg = (*s == '-');
  if (neg)
    offset = -offset;
  offset = 60 * ((offset / 100) * 60 + offset % 100);
  if (neg)
    offset = -offset;
  return offset;
}

/************************************************************************
 * UnixDate2Secs - convert a UNIX mbox "From " date into seconds
 * Handles format: "Wed Jun 14 12:36:18 1989" and variations
 ************************************************************************/
uint32_t UnixDate2Secs(const char *date) {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));

  /* Try various strptime formats */
  if (strptime(date, "%a %b %d %H:%M:%S %Y", &tm) ||
      strptime(date, "%d %b %Y %H:%M:%S", &tm) ||
      strptime(date, "%a, %d %b %Y %H:%M:%S", &tm) ||
      strptime(date, "%b %d %H:%M:%S %Y", &tm)) {
    return (uint32_t)mktime(&tm);
  }
  return 0;
}

/************************************************************************
 * BeautifyDate - parse an RFC 2822 date header value and return seconds.
 * Sets *origZone to the timezone offset in seconds.
 * The dateStr is a C string (header value after "Date: ").
 ************************************************************************/
uint32_t BeautifyDate(unsigned char *dateStr, long *zoneSecs) {
  struct tm tm;
  const char *rest;

  *zoneSecs = ZoneSecs(); /* default to local zone */
  if (!dateStr || !*dateStr)
    return 0;

  memset(&tm, 0, sizeof(tm));

  /* Try RFC 2822: "Wed, 21 Jan 2026 17:23:50 +0100" */
  rest = strptime((char *)dateStr, "%a, %d %b %Y %H:%M:%S", &tm);
  if (!rest)
    rest = strptime((char *)dateStr, "%d %b %Y %H:%M:%S", &tm);
  if (!rest)
    rest = strptime((char *)dateStr, "%a %b %d %H:%M:%S %Y", &tm);

  if (rest) {
    /* Try to parse timezone from remainder */
    while (*rest && (*rest == ' ' || *rest == '\t'))
      rest++;
    if (*rest == '+' || *rest == '-' || isdigit(*rest)) {
      *zoneSecs = CStr2Zone(rest);
    } else if (!strncasecmp(rest, "GMT", 3) || !strncasecmp(rest, "UTC", 3) ||
               !strncasecmp(rest, "UT", 2)) {
      *zoneSecs = 0;
    } else if (strlen(rest) >= 3 && isalpha(rest[0])) {
      /* Named timezone — common ones */
      if (!strncasecmp(rest, "EST", 3))
        *zoneSecs = -5 * 3600;
      else if (!strncasecmp(rest, "EDT", 3))
        *zoneSecs = -4 * 3600;
      else if (!strncasecmp(rest, "CST", 3))
        *zoneSecs = -6 * 3600;
      else if (!strncasecmp(rest, "CDT", 3))
        *zoneSecs = -5 * 3600;
      else if (!strncasecmp(rest, "MST", 3))
        *zoneSecs = -7 * 3600;
      else if (!strncasecmp(rest, "MDT", 3))
        *zoneSecs = -6 * 3600;
      else if (!strncasecmp(rest, "PST", 3))
        *zoneSecs = -8 * 3600;
      else if (!strncasecmp(rest, "PDT", 3))
        *zoneSecs = -7 * 3600;
      else if (!strncasecmp(rest, "CET", 3))
        *zoneSecs = 1 * 3600;
      else if (!strncasecmp(rest, "CEST", 4))
        *zoneSecs = 2 * 3600;
    }

    /* Convert to UTC seconds: mktime gives local time, adjust */
    tm.tm_isdst = -1;
    time_t secs = mktime(&tm);
    if (secs == (time_t)-1)
      return 0;

    /* mktime interpreted tm as local time; we want UTC interpretation
       adjusted by the parsed zone. Result = UTC seconds. */
    long localOffset = ZoneSecs();
    uint32_t utcSecs = (uint32_t)(secs + localOffset - *zoneSecs);
    return utcSecs;
  }

  /* Couldn't parse — return current time */
  *zoneSecs = ZoneSecs();
  return (uint32_t)time(NULL);
}

/************************************************************************
 * CopyHeaderLine - extract the value portion of a header line.
 * Given "Subject: Hello World\r\n", copies "Hello World" into `to`.
 * Strips leading whitespace after the colon.
 ************************************************************************/
void CopyHeaderLine(unsigned char *to, int size, unsigned char *from) {
  unsigned char *colon;
  int len;

  if (!to || size <= 0)
    return;
  to[0] = '\0';

  if (!from)
    return;

  /* Find the colon */
  colon = (unsigned char *)strchr((char *)from, ':');
  if (!colon) {
    /* No colon — copy whole line */
    colon = from;
  } else {
    colon++; /* skip colon */
  }

  /* Skip leading whitespace */
  while (*colon && IsWhite(*colon))
    colon++;

  /* Copy, stripping trailing CR/LF */
  len = strlen((char *)colon);
  while (len > 0 && (colon[len - 1] == '\r' || colon[len - 1] == '\n'))
    len--;
  if (len >= size)
    len = size - 1;
  memcpy(to, colon, len);
  to[len] = '\0';
}

/************************************************************************
 * BeautifyFrom - clean up a from/to address string.
 * Strips angle brackets, quotes, and extracts the display name or bare
 * address. Input and output are C strings.
 ************************************************************************/
void BeautifyFrom(unsigned char *fromStr) {
  char *s = (char *)fromStr;
  char *lt, *gt, *start;
  char buf[256];
  int len;

  if (!s || !*s)
    return;

  /* Strip leading/trailing whitespace */
  while (*s && IsWhite(*s))
    s++;
  len = strlen(s);
  while (len > 0 && IsWhite(s[len - 1]))
    len--;
  s[len] = '\0';

  /* If there's a display name with angle brackets: "Name <addr>" → "Name" */
  lt = strchr(s, '<');
  gt = lt ? strchr(lt, '>') : NULL;

  if (lt && gt) {
    /* Check if there's a display name before the < */
    char *nameEnd = lt;
    while (nameEnd > s && IsWhite(nameEnd[-1]))
      nameEnd--;
    if (nameEnd > s) {
      /* Use the display name */
      start = s;
      /* Strip surrounding quotes from display name */
      if (*start == '"' && nameEnd > start + 1 && nameEnd[-1] == '"') {
        start++;
        nameEnd--;
      }
      len = nameEnd - start;
      if (len > 0 && len < (int)sizeof(buf)) {
        memcpy(buf, start, len);
        buf[len] = '\0';
        strcpy((char *)fromStr, buf);
        return;
      }
    }
    /* No display name — use the address inside <> */
    start = lt + 1;
    len = gt - start;
    if (len > 0 && len < (int)sizeof(buf)) {
      memcpy(buf, start, len);
      buf[len] = '\0';
      strcpy((char *)fromStr, buf);
      return;
    }
  }

  /* If wrapped in quotes, strip them */
  if (s[0] == '"' && len > 1 && s[len - 1] == '"') {
    memmove(s, s + 1, len - 2);
    s[len - 2] = '\0';
  }

  /* If input was shifted, move back to start of fromStr */
  if (s != (char *)fromStr) {
    memmove(fromStr, s, strlen(s) + 1);
  }
}

/************************************************************************
 * BeautifySubj - clean up a subject line.
 * Removes common Outlook-style reply/forward prefixes.
 ************************************************************************/
void BeautifySubj(unsigned char *subject, short size) {
  char *s = (char *)subject;
  if (!s || !*s)
    return;

  /* Strip leading whitespace */
  while (*s && IsWhite(*s))
    s++;

  /* Remove "Re: ", "Fwd: ", "FW: " prefixes that Outlook localizes.
     We keep it simple: strip leading "Re: " and "Fwd: " / "Fw: " */
  /* (The original used resource-based pattern matching for localized
     Outlook prefixes; we just leave subjects as-is for now since
     stripping prefixes is cosmetic) */

  if (s != (char *)subject)
    memmove(subject, s, strlen(s) + 1);
}

/************************************************************************
 * BeautifySum - beautify a message summary.
 * Calls PtrTimeStamp and BeautifyFrom.
 ************************************************************************/
void BeautifySum(MSumPtr sum) {
  if (sum->seconds)
    PtrTimeStamp(sum, sum->seconds, ZoneSecs());
  BeautifyFrom((unsigned char *)sum->from);
}

/************************************************************************
 * GleanFrom - extract sender address and date from mbox "From " line.
 * Input: "From user@example.com Wed Jun 14 12:36:18 2023"
 * Sets sum->from and calls PtrTimeStamp with parsed date.
 ************************************************************************/
void GleanFrom(unsigned char *line, MSumPtr sum) {
  char copy[512];
  char *cp, *ep;
  long seconds;
  long offset = ZoneSecs();

  strncpy(copy, (char *)line, sizeof(copy) - 1);
  copy[sizeof(copy) - 1] = '\0';

  /* Skip "From " */
  cp = copy;
  while (*cp && *cp != ' ')
    cp++;
  if (*cp)
    cp++;

  /* Extract address (up to next space) */
  for (ep = cp; *ep && *ep != ' '; ep++)
    ;

  {
    int addrLen = ep - cp;
    if (addrLen >= (int)sizeof(sum->from))
      addrLen = sizeof(sum->from) - 1;
    memcpy(sum->from, cp, addrLen);
    sum->from[addrLen] = '\0';
  }

  /* Extract date (rest of line) */
  if (*ep)
    ep++;
  /* Strip trailing CR/LF */
  {
    char *end = ep + strlen(ep);
    while (end > ep && (end[-1] == '\r' || end[-1] == '\n'))
      end--;
    *end = '\0';
  }

  seconds = UnixDate2Secs(ep) - offset;
  PtrTimeStamp(sum, seconds, offset);
  sum->arrivalSeconds = seconds + offset;
}

/************************************************************************
 * IsBulk - does this header line represent an automated mailer?
 * Simplified: checks for common daemon/mailer-daemon patterns.
 ************************************************************************/
bool IsBulk(unsigned char *line) {
  char *s = (char *)line;
  /* Look for common automated sender patterns after the colon */
  char *colon = strchr(s, ':');
  if (colon)
    s = colon + 1;
  while (*s && IsWhite(*s))
    s++;

  if (strcasestr(s, "mailer-daemon") || strcasestr(s, "postmaster") ||
      strcasestr(s, "mail delivery") || strcasestr(s, "noreply") ||
      strcasestr(s, "no-reply") || strcasestr(s, "auto-reply"))
    return true;
  return false;
}

/************************************************************************
 * SumToFrom - determine if a summary is addressed to or from.
 * Returns nonzero if the fromLine matches the outgoing personality.
 * Simplified for the port.
 ************************************************************************/
int SumToFrom(MSumPtr sum, unsigned char *fromLine) {
  (void)sum;
  (void)fromLine;
  return 0;
}

/************************************************************************
 * FindTOCSpot - find the spot for a new message in a TOC.
 * Returns the index. Currently unused in the port.
 ************************************************************************/
long FindTOCSpot(TOCType * tocH, long length) {
  (void)tocH;
  (void)length;
  return 0;
}

/************************************************************************
 * TrimWrap - trim wrapping characters from a string
 ************************************************************************/
unsigned char *TrimWrap(unsigned char *str, int openC, int closeC) {
  (void)openC;
  (void)closeC;
  return str;
}

/************************************************************************
 * TrimNonWord - trim non-word characters
 ************************************************************************/
unsigned char *TrimNonWord(unsigned char *str) { return str; }

/************************************************************************
 * MatchHeader - match a header name (case-insensitive).
 * Returns the header index (tchDate, tchSubject, etc.) or 0.
 * Replaces the original FindSTRNIndex(TOCHeaderStrn, headerName).
 ************************************************************************/
static short MatchHeader(const char *headerName) {
  /* Header name includes the trailing colon, e.g. "Date:" "Subject:" */
  if (!strcasecmp(headerName, "Date:"))
    return 1; /* tchDate */
  if (!strcasecmp(headerName, "Status:"))
    return 3; /* tchStatus */
  if (!strcasecmp(headerName, "To:"))
    return 4; /* tchTo */
  if (!strcasecmp(headerName, "X-Priority:"))
    return 5; /* tchXPrior */
  if (!strcasecmp(headerName, "Bcc:"))
    return 6; /* tchBcc */
  if (!strcasecmp(headerName, "Subject:"))
    return 7; /* tchSubject */
  if (!strcasecmp(headerName, "Importance:"))
    return 8; /* tchImportance */
  if (!strcasecmp(headerName, "Precedence:"))
    return 9; /* tchPrecedence */
  if (!strcasecmp(headerName, "Message-ID:") ||
      !strcasecmp(headerName, "Message-Id:"))
    return 10; /* tchMessageId */
  return 0;
}

/* Header indices (from StrnDefs.h) */
#define tchDate 1
#define tchStatus 3
#define tchTo 4
#define tchXPrior 5
#define tchBcc 6
#define tchSubject 7
#define tchImportance 8
#define tchPrecedence 9
#define tchMessageId 10

/************************************************************************
 * MatchSenderHeader - check if this is a From/Sender/Reply-To/Return-Path
 * header. Returns priority (lower = higher priority) or 0 for no match.
 ************************************************************************/
static short MatchSenderHeader(const char *headerName) {
  if (!strcasecmp(headerName, "From:"))
    return 1;
  if (!strcasecmp(headerName, "Sender:"))
    return 2;
  if (!strcasecmp(headerName, "Reply-To:"))
    return 3;
  if (!strcasecmp(headerName, "Return-Path:"))
    return 4;
  return 0;
}

/************************************************************************
 * ReadSum - read a message summary from the current position in the
 * line I/O routines.  This is the state machine that parses mbox files.
 *
 * Pass sum=NULL to reset internal state.
 * Returns noErr on success, fnfErr at EOF, or an error code.
 ************************************************************************/
OSErr ReadSum(MSumPtr sum, bool isOut, LineIOP lip, bool lookEnvelope) {
  static int type;
  unsigned char line[1024];
  static unsigned char *oldLineData = NULL;
  static long oldLineLen = 0;
  enum { BEGIN, IN_BODY, IN_HEADER, ERROR } state;
  unsigned char duck[256];
  char headerName[64];
  short headerIndex;
  unsigned char *spot;
  long secs;
  long origZone;
  short senderHead = 32767; /* REAL_BIG */
  short outFlags = DefaultOutFlags();
  long len;

  if (!sum) {
    if (oldLineData) {
      free(oldLineData);
      oldLineData = NULL;
      oldLineLen = 0;
    }
    return noErr;
  }

  state = BEGIN;
  memset(sum, 0, sizeof(MSumType));
  sum->state = UNREAD;
  sum->spamScore = -1;

  if (isOut)
    sum->flags = outFlags;

  /* UTF-8 flag — always set on GTK port */
  outFlags |= FLAG_UTF8;
  sum->flags |= FLAG_UTF8;

  while (oldLineData || (type = NLGetLine(line, sizeof(line), &len, lip))) {
    if (oldLineData) {
      if (oldLineLen > 0 && oldLineLen < (long)sizeof(line)) {
        memcpy(line, oldLineData, oldLineLen + 1);
        len = oldLineLen;
      }
      free(oldLineData);
      oldLineData = NULL;
      oldLineLen = 0;
    }

    switch (type) {
    case LINE_START:
      if ((state == BEGIN || lookEnvelope) && IsFromLine(line)) {
        if (state != BEGIN) {
          /* We hit the next message's "From " line — save it and return */
          oldLineData = malloc(len + 1);
          if (!oldLineData) {
            g_warning("ReadSum: out of memory saving From line");
            return -108;
          }
          memcpy(oldLineData, line, len + 1);
          oldLineLen = len;
          goto done;
        }

        /* Start of a new message */
        memset(sum, 0, sizeof(MSumType));
        sum->spamScore = -1;
        GleanFrom(line, sum);
        sum->offset = TellLine(lip);
        sum->state = isOut ? SENT : UNREAD;
        state = IN_HEADER;
        if (isOut)
          sum->flags = outFlags;
        sum->flags |= FLAG_UTF8;
        senderHead = 32767;
      } else if (state == IN_HEADER) {
        /* Inside headers — parse header lines */
        if (*line == '\r' || *line == '\n' || *line == '\0') {
          /* Blank line = end of headers, start of body */
          state = IN_BODY;
          sum->bodyOffset = TellLine(lip) - sum->offset;
        } else if (!IsWhite(*line)) {
          /* New header line — extract header name */
          spot = line;
          while (*spot && *spot != ':')
            spot++;

          if (*spot == ':' && spot > line + 1) {
            int nameLen = spot + 1 - line;
            if (nameLen >= (int)sizeof(headerName))
              nameLen = sizeof(headerName) - 1;
            memcpy(headerName, line, nameLen);
            headerName[nameLen] = '\0';
            headerIndex = MatchHeader(headerName);
          } else {
            headerName[0] = '\0';
            headerIndex = 0;
          }

          switch (headerIndex) {
          case tchDate:
            CopyHeaderLine(duck, sizeof(duck), line);
            secs = BeautifyDate(duck, &origZone);
            if (secs)
              PtrTimeStamp(sum, secs, origZone);
            break;

          case tchTo:
            if (isOut) {
              CopyHeaderLine(duck, sizeof(duck), line);
              BeautifyFrom(duck);
              strncpy(sum->from, (char *)duck, sizeof(sum->from) - 1);
              sum->from[sizeof(sum->from) - 1] = '\0';
              if (sum->from[0])
                sum->state = SENT;
            }
            break;

          case tchBcc:
            if (isOut && (!sum->from[0] || sum->from[0] == '?')) {
              CopyHeaderLine(duck, sizeof(duck), line);
              BeautifyFrom(duck);
              strncpy(sum->from, (char *)duck, sizeof(sum->from) - 1);
              sum->from[sizeof(sum->from) - 1] = '\0';
              if (sum->from[0])
                sum->state = SENT;
            }
            break;

          case tchSubject:
            CopyHeaderLine(duck, sizeof(duck), line);
            BeautifySubj(duck, sizeof(sum->subj));
            strncpy(sum->subj, (char *)duck, sizeof(sum->subj) - 1);
            sum->subj[sizeof(sum->subj) - 1] = '\0';
            break;

          case tchStatus:
            /* Check for "R", "RO" (read), or "Q" (queued) */
            {
              char val[64];
              CopyHeaderLine((unsigned char *)val, sizeof(val), line);
              if (strchr(val, 'Q'))
                sum->state = QUEUED;
              else if (strchr(val, 'R'))
                sum->state = READ;
            }
            break;

          case tchXPrior:
            CopyHeaderLine(duck, sizeof(duck), line);
            {
              int pri = atoi((char *)duck);
              if (pri >= 1 && pri <= 5)
                sum->priority = pri;
            }
            break;

          case tchMessageId:
            if (!ValidHash(sum->msgIdHash)) {
              CopyHeaderLine(duck, sizeof(duck), line);
              (void)0;
              /* Strip angle brackets */
              char *idStart = (char *)duck;
              if (*idStart == '<')
                idStart++;
              char *idEnd = idStart + strlen(idStart);
              if (idEnd > idStart && idEnd[-1] == '>')
                idEnd--;
              sum->msgIdHash = HashString(idStart, idEnd - idStart);
              if (!ValidHash(sum->uidHash))
                sum->uidHash = sum->msgIdHash;
            }
            break;

          case tchPrecedence:
            CopyHeaderLine(duck, sizeof(duck), line);
            if (strcasestr((char *)duck, "bulk") ||
                strcasestr((char *)duck, "list") ||
                strcasestr((char *)duck, "junk"))
              sum->opts |= OPT_BULK;
            break;

          case tchImportance:
            if (sum->priority == 0) {
              CopyHeaderLine(duck, sizeof(duck), line);
              /* Map importance to priority: Low=5, Normal=3, High=1 */
              if (strcasestr((char *)duck, "high"))
                sum->priority = 1;
              else if (strcasestr((char *)duck, "low"))
                sum->priority = 5;
              else
                sum->priority = 3;
            }
            break;

          default:
            /* Check if this is a From/Sender/Reply-To header */
            if (!isOut && headerName[0]) {
              short sIdx = MatchSenderHeader(headerName);
              if (sIdx && !(sum->opts & OPT_BULK) && IsBulk(line))
                sum->opts |= OPT_BULK;
              if (sIdx && sIdx <= senderHead) {
                CopyHeaderLine(duck, sizeof(duck), line);
                BeautifyFrom(duck);
                strncpy(sum->from, (char *)duck, sizeof(sum->from) - 1);
                sum->from[sizeof(sum->from) - 1] = '\0';
                senderHead = sIdx;
              }
            }
            /* Check for bulk headers (X-Mailer, X-Mailing-List, etc.) */
            if (!strcasecmp(headerName, "X-Mailing-List:") ||
                !strcasecmp(headerName, "List-Id:") ||
                !strcasecmp(headerName, "List-Post:") ||
                !strcasecmp(headerName, "Mailing-List:"))
              sum->opts |= OPT_BULK;
            break;
          }
        } else if (headerIndex == tchSubject) {
          /* Continuation line for Subject — append */
          char *cont = (char *)line;
          while (*cont && IsWhite(*cont))
            cont++;
          /* Replace tabs with spaces */
          for (char *t = cont; *t; t++)
            if (*t == '\t')
              *t = ' ';
          /* Strip trailing CR/LF */
          int cLen = strlen(cont);
          while (cLen > 0 && (cont[cLen - 1] == '\r' || cont[cLen - 1] == '\n'))
            cLen--;
          cont[cLen] = '\0';

          int curLen = strlen(sum->subj);
          if (curLen < (int)sizeof(sum->subj) - 2) {
            sum->subj[curLen] = ' ';
            strncpy(sum->subj + curLen + 1, cont,
                    sizeof(sum->subj) - curLen - 2);
            sum->subj[sizeof(sum->subj) - 1] = '\0';
          }
        }
        /* In body: check for HTML tags, etc. */
      } else if (state == IN_BODY && len > 1 && *line == '<') {
        char *tag = (char *)line + 1;
        if (!strncasecmp(tag, "html", 4))
          sum->opts |= OPT_HTML;
        else if (!strncasecmp(tag, "!doctype", 8))
          sum->opts |= OPT_HTML;
      }
      break;

    case LINE_MIDDLE:
      break;

    default:
      return -36; /* I/O error */
    }

    if (state == BEGIN)
      state = IN_HEADER;
  }

done:
  if (state != BEGIN) {
    sum->length = TellLine(lip) - sum->offset;
  }
  return (state == BEGIN) ? fnfErr : noErr;
}

/************************************************************************
 * IsMailbox - check if path is a regular file (i.e., a mailbox)
 ************************************************************************/
static bool IsMailbox(const char *path) {
  if (!path || !*path)
    return false;
  struct stat st;
  return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

/**********************************************************************
 * BuildTOC - build a table of contents for a file.  The TOC is built
 * in memory using plain malloc/realloc.  Only wrapped into a TOCType *
 * at the very end for API compatibility with the rest of the codebase.
 **********************************************************************/
TOCType * BuildTOC(const char *path) {
  TOCType *toc = NULL;
  MSumType sum;
  bool isOut;
  LineIOD lid;
  OSErr err;
  short which = 0;
  const char *filename;
  short capacity = 0;

  if (!path)
    return NULL;

  filename = strrchr(path, '/');
  if (filename)
    filename++;
  else
    filename = path;

  g_debug("BuildTOC(%s)", filename);

  memset(&lid, 0, sizeof(lid));

  if (!IsMailbox(path)) {
    g_warning("BuildTOC: not a mailbox: %s", filename);
    return NULL;
  }

  if ((err = OpenLine(path, fsRdWrPerm, &lid))) {
    g_warning("BuildTOC: cannot open mailbox %s: error %d", filename, err);
    return NULL;
  }

  toc = AllocTOC();
  if (!toc) {
    g_warning("BuildTOC: out of memory");
    CloseLine(&lid);
    return NULL;
  }

  /* Determine mailbox type by filename */
  isOut = false;
  if (!strcasecmp(filename, "Out")) {
    which = OUT;
    isOut = true;
  } else if (!strcasecmp(filename, "In")) {
    which = IN;
  } else if (!strcasecmp(filename, "Trash")) {
    which = TRASH;
  } else if (!strcasecmp(filename, "Junk")) {
    which = JUNK;
  }

  toc->which = which;
  toc->nextSerialNum = 1;

  /* Initialize ReadSum state */
  ReadSum(NULL, false, &lid, true);

  /* Read messages — grow toc with realloc, no handles */
  while (!(err = ReadSum(&sum, isOut, &lid, true))) {
    if (toc->count >= capacity) {
      capacity = capacity ? capacity * 2 : 16;
      TOCType *grown = GrowTOC(toc, capacity);
      if (!grown) {
        g_warning("BuildTOC: out of memory growing TOC");
        goto failure;
      }
      toc = grown;
    }
    sum.serialNum = toc->nextSerialNum++;
    toc->sums[toc->count++] = sum;
  }
  if (err != fnfErr)
    goto failure;

  /* Clean up ReadSum state */
  ReadSum(NULL, false, &lid, true);

  /* Trim to exact size */
  {
    TOCType *trimmed = GrowTOC(toc, toc->count);
    if (trimmed)
      toc = trimmed;
  }

  CloseLine(&lid);

  /* Fill in TOC metadata */
  toc->refN = 0;
  strncpy(toc->mailbox.spec.path, path, sizeof(toc->mailbox.spec.path) - 1);
  toc->mailbox.spec.path[sizeof(toc->mailbox.spec.path) - 1] = '\0';
  strncpy((char *)toc->mailbox.spec.name, filename,
          sizeof(toc->mailbox.spec.name) - 1);
  ((char *)toc->mailbox.spec.name)[sizeof(toc->mailbox.spec.name) - 1] = '\0';
  toc->majorVersion = 1;
  toc->minorVersion = 9;
  toc->durty = true;
  toc->reallyDirty = true;

  /* Check if this is an IMAP mailbox */
  IsIMAPMailboxFileLo(&toc->mailbox.spec, &toc->imapMBH);

  return toc;

failure:
  free(toc);
  CloseLine(&lid);
  return NULL;
}

/************************************************************************
 * BuildTOC_Path - wrapper that takes a path string
 ************************************************************************/
TOCType * BuildTOC_Path(const char *path) { return BuildTOC(path); }

/************************************************************************
 * RebuildTOC - rebuild a corrupt or out-of-date TOC, salvaging what
 * we can from the old one.
 ************************************************************************/
TOCType * RebuildTOC(const char *path, TOCType * oldTocH, bool resource,
                     bool tempBox) {
  TOCType * newTocH = NULL;

  if (oldTocH) {
    if ((newTocH = BuildTOC(path))) {
      if (oldTocH->count && newTocH->count) {
        SalvageTOC(oldTocH, newTocH);
        memcpy(newTocH->sorts, oldTocH->sorts, sizeof(newTocH->sorts));
        newTocH->lastSort = oldTocH->lastSort;
        newTocH->pluginKey = oldTocH->pluginKey;
        newTocH->pluginValue = oldTocH->pluginValue;
        newTocH->previewHi = oldTocH->previewHi;
        newTocH->nextSerialNum = oldTocH->nextSerialNum;
        newTocH->unreadBase = oldTocH->unreadBase;
      }
    }
  } else {
    newTocH = BuildTOC(path);
  }

  if (newTocH)
    newTocH->reallyDirty = true;

  return newTocH;
}

/************************************************************************
 * SalvageTOC - reconcile an old and new TOC.
 * Uses binary search on offset to match messages.
 ************************************************************************/
short SalvageTOC(TOCType *oldToc, TOCType *newToc) {
  short first, last, mid;
  MSumPtr oldSum;
  long offset;
  short salvaged = 0;
  long bo;
  long seconds;

  /* Mark all new sums as REBUILT */
  for (mid = newToc->count - 1; mid >= 0; mid--)
    newToc->sums[mid].state = REBUILT;

  for (oldSum = oldToc->sums; oldSum < oldToc->sums + oldToc->count; oldSum++) {
    offset = oldSum->offset;

    /* Binary search for matching offset in new TOC */
    first = 0;
    last = newToc->count - 1;
    for (mid = (first + last) / 2; first <= last; mid = (first + last) / 2) {
      if (offset < newToc->sums[mid].offset)
        last = mid - 1;
      else if (offset == newToc->sums[mid].offset)
        break;
      else
        first = mid + 1;
    }

    if (first <= last && newToc->sums[mid].length == oldSum->length) {
      salvaged++;
      bo = newToc->sums[mid].bodyOffset;
      seconds = newToc->sums[mid].seconds;
      newToc->sums[mid] = *oldSum;
      newToc->sums[mid].bodyOffset = bo;
      if (newToc->sums[mid].state != TIMED)
        newToc->sums[mid].seconds = seconds;
    } else if (oldSum->msgIdHash) {
      for (first = 0; first < newToc->count; first++) {
        if (oldSum->msgIdHash == newToc->sums[first].msgIdHash &&
            oldSum->length == newToc->sums[first].length) {
          salvaged++;
          bo = newToc->sums[first].bodyOffset;
          offset = newToc->sums[first].offset;
          seconds = newToc->sums[first].seconds;
          newToc->sums[first] = *oldSum;
          newToc->sums[first].bodyOffset = bo;
          newToc->sums[first].offset = offset;
          newToc->sums[first].seconds = seconds;
        }
      }
    }
  }

  CleanseTOC(newToc);
  return salvaged;
}

/* HasUnicode - always true on GLib/GTK (UTF-8 native) */
bool HasUnicode(void) { return true; }

/* GoodUTF8Len - find longest valid UTF-8 prefix of bufLen bytes. */
ByteCount GoodUTF8Len(BytePtr utf8, ByteCount bufLen) {
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

/* HeaderToUTF8 - validate/convert a C string header to valid UTF-8.
 * Unlike the original which used Pascal strings, this operates on
 * null-terminated C strings in-place. */
OSStatus HeaderToUTF8(unsigned char *head) {
  if (!head || !*head)
    return 0;

  int len = strlen((char *)head);

  /* If already valid UTF-8, just return */
  if (g_utf8_validate((const gchar *)head, len, NULL))
    return 0;

  /* Try converting from Latin-1 to UTF-8 as a fallback */
  gsize bytes_written = 0;
  gchar *utf8 =
      g_convert((const gchar *)head, len, "UTF-8", "ISO-8859-1", NULL,
                &bytes_written, NULL);
  if (utf8) {
    /* Don't overflow the buffer — assume 255 max like original */
    if (bytes_written > 255)
      bytes_written = (gsize)GoodUTF8Len((BytePtr)utf8, 255);
    memcpy(head, utf8, bytes_written);
    head[bytes_written] = '\0';
    g_free(utf8);
  }
  return 0;
}
