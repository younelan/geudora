/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
 * log.c - Eudora log file system, fully ported to C strings.
 *
 * All Pascal string usage has been removed. ComposeLogS now uses
 * standard C printf formatting (%s instead of %p). Log() takes
 * C strings directly.
 */

#include "log.h"
#include "Globals.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "fileutil.h"
#include "threading.h"
#define FILE_NUM 50

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static FILE *gLogFile = NULL;
static void OpenLog(void);

/* Build log file path from C string name */
static void LogFilePath(const char *name, char *outPath, size_t outLen) {
  const char *home = getenv("HOME");
  if (!home) home = "/tmp";
  snprintf(outPath, outLen, "%s/.eudora/%s", home, name);
}

static long LogFileSize(void) {
  if (!gLogFile)
    return 0;
  long pos = ftell(gLogFile);
  fseek(gLogFile, 0, SEEK_END);
  long size = ftell(gLogFile);
  fseek(gLogFile, pos, SEEK_SET);
  return size;
}

/* ─── Log ────────────────────────────────────────────────────────────────── */

unsigned char *Log(unsigned long level, unsigned char *string) {
  MyThreadBeginCritical();

  if (level == 0xFFFFFFFF || (level & LogLevel) != 0) {
    long tickDiff = TickCount() - LogTicks;
    long hr = tickDiff / 3600;
    long min = (tickDiff / 60) % 60;
    long sec = tickDiff % 60;

    char threadBuf[32];
    if (InAThread()) {
      snprintf(threadBuf, sizeof(threadBuf), "%lu",
               (unsigned long)(uintptr_t)pthread_self());
    } else {
      strncpy(threadBuf, "MAIN", sizeof(threadBuf));
    }

    char stamp[64];
    snprintf(stamp, sizeof(stamp), "%s %lu:%ld.%ld.%ld ", threadBuf, level, hr,
             min, sec);

    const char *cstr = (const char *)string;
    int clen = cstr ? (int)strlen(cstr) : 0;

    OpenLog();
    if (gLogFile) {
      fputs(stamp, gLogFile);
      if (cstr)
        fputs(cstr, gLogFile);
      if (clen == 0 || cstr[clen - 1] != '\n')
        fputc('\n', gLogFile);
      fflush(gLogFile);
    }
  }

  MyThreadEndCritical();
  return string;
}

/* ─── ComposeLogR ────────────────────────────────────────────────────────── */

unsigned char *ComposeLogR(unsigned long level, unsigned char *into,
                           short format, ...) {
  char buf[256];
  snprintf(buf, sizeof(buf), "[LogR format=%d]", format);
  if (into)
    strncpy((char *)into, buf, 255);
  Log(level, (unsigned char *)buf);
  return into;
}

/* ─── ComposeLogS ────────────────────────────────────────────────────────── */
/*
 * C-string logger. Legacy callers pass format strings with Pascal length
 * bytes (\p prefix) and %p (Pascal string arg). We auto-detect and convert:
 *   - Skip leading byte if it looks like a Pascal length (< 32)
 *   - Convert %p to %s
 *   - Convert Mac CR (\015) to Unix LF (\n)
 * This way all existing callers work without modification.
 */
unsigned char *ComposeLogS(unsigned long level, unsigned char *into,
                           unsigned char *format, ...) {
  char buf[512];
  const char *fmt = (const char *)format;

  /* Skip Pascal length byte if present */
  if (fmt && (unsigned char)fmt[0] < 32 && fmt[0] != '\0')
    fmt++;

  /* Convert Pascal %p to C %s, Mac CR to LF */
  char safefmt[512];
  size_t j = 0;
  for (size_t i = 0; fmt[i] && j < sizeof(safefmt) - 1; i++) {
    if (fmt[i] == '%' && fmt[i + 1] == 'p') {
      safefmt[j++] = '%';
      safefmt[j++] = 's';
      i++;
    } else if (fmt[i] == '\015') {
      safefmt[j++] = '\n';
    } else {
      safefmt[j++] = fmt[i];
    }
  }
  safefmt[j] = '\0';

  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), safefmt, args);
  va_end(args);

  if (into) {
    strncpy((char *)into, buf, 255);
    into[255] = '\0';
  }

  Log(level, (unsigned char *)buf);
  return into;
}

/* ─── CloseLog ───────────────────────────────────────────────────────────── */

void CloseLog(void) {
  if (!gLogFile)
    return;

  long size = LogFileSize();
  fclose(gLogFile);
  gLogFile = NULL;
  LogRefN = 0;

  /* Roll over if file exceeds 512 KB */
  if (size > 512L * 1024L) {
    char logPath[1024], oldPath[1024];
    LogFilePath("Eudora Log", logPath, sizeof(logPath));
    LogFilePath("Old Eudora Log", oldPath, sizeof(oldPath));
    remove(oldPath);
    rename(logPath, oldPath);
  }
}

/* ─── OpenLog ────────────────────────────────────────────────────────────── */

static void OpenLog(void) {
  if (gLogFile && LogFileSize() > 512L * 1024L)
    CloseLog();

  if (gLogFile)
    return;

  char logPath[1024];
  LogFilePath("Eudora Log", logPath, sizeof(logPath));

  /* Ensure directory exists */
  char *slash = strrchr(logPath, '/');
  if (slash) {
    char dir[1024];
    size_t dlen = slash - logPath;
    memcpy(dir, logPath, dlen);
    dir[dlen] = '\0';
    mkdir(dir, 0755);
  }

  gLogFile = fopen(logPath, "a");
  if (!gLogFile)
    return;

  LogRefN = 1;
  LogTicks = TickCount();

  char dateBuf[64];
  time_t now;
  time(&now);
  struct tm *tm_info = localtime(&now);
  strftime(dateBuf, sizeof(dateBuf), "\n%Y-%m-%d %H:%M:%S\n", tm_info);
  fputs(dateBuf, gLogFile);
  fflush(gLogFile);
}

/* ─── LogAlert ───────────────────────────────────────────────────────────── */

void LogAlert(short template_id) {
  char buf[64];
  snprintf(buf, sizeof(buf), "ALRT %d", template_id);
  Log(LOG_ALRT, (unsigned char *)buf);
}

/* ─── MyParamText ────────────────────────────────────────────────────────── */

void MyParamText(PStr p1, PStr p2, PStr p3, PStr p4) {
  P1[0] = 0;
  P2[0] = 0;
  P3[0] = 0;
  P4[0] = 0;
  if (p1 && *p1) {
    PCopy(P1, p1);
    /* Convert Pascal to C for logging */
    char cbuf[256];
    int len = (unsigned char)p1[0];
    if (len > 254) len = 254;
    memcpy(cbuf, p1 + 1, len);
    cbuf[len] = '\0';
    Log(LOG_ALRT, (unsigned char *)cbuf);
  }
  if (p2 && *p2) {
    PCopy(P2, p2);
    char cbuf[256];
    int len = (unsigned char)p2[0];
    if (len > 254) len = 254;
    memcpy(cbuf, p2 + 1, len);
    cbuf[len] = '\0';
    Log(LOG_ALRT, (unsigned char *)cbuf);
  }
  if (p3 && *p3) {
    PCopy(P3, p3);
    char cbuf[256];
    int len = (unsigned char)p3[0];
    if (len > 254) len = 254;
    memcpy(cbuf, p3 + 1, len);
    cbuf[len] = '\0';
    Log(LOG_ALRT, (unsigned char *)cbuf);
  }
  if (p4 && *p4) {
    PCopy(P4, p4);
    char cbuf[256];
    int len = (unsigned char)p4[0];
    if (len > 254) len = 254;
    memcpy(cbuf, p4 + 1, len);
    cbuf[len] = '\0';
    Log(LOG_ALRT, (unsigned char *)cbuf);
  }
}

/* ─── CarefulLog ─────────────────────────────────────────────────────────── */

void CarefulLog(unsigned long level, short format, unsigned char *data,
                short dSize) {
  if (!(level & LogLevel))
    return;

  char logBuf[256];
  unsigned char *dataEnd = data + dSize;

  while (data < dataEnd) {
    char *to = logBuf;
    char *toEnd = logBuf + sizeof(logBuf) - 6;

    while (data < dataEnd && to < toEnd) {
      if (*data < ' ') {
        switch (*data) {
        case '\n': *to++ = '\\'; *to++ = 'n'; break;
        case '\r': *to++ = '\\'; *to++ = 'r'; break;
        case '\t': *to++ = '\t'; break;
        default:
          *to++ = '\\';
          *to++ = '0' + (*data / 64) % 8;
          *to++ = '0' + (*data / 8) % 8;
          *to++ = '0' + *data % 8;
          break;
        }
      } else {
        *to++ = *data;
      }
      data++;
      if (data[-1] == '\n')
        break;
    }
    *to = '\0';
    Log(level, (unsigned char *)logBuf);
  }
}

/* ─── LineLog ────────────────────────────────────────────────────────────── */

void LineLog(unsigned long level, short format, unsigned char *data,
             short dSize) {
  if (!(level & LogLevel))
    return;

  char logBuf[256];
  unsigned char *dataEnd = data + dSize;

  while (data < dataEnd) {
    char *to = logBuf;
    char *toEnd = logBuf + sizeof(logBuf) - 6;

    while (data < dataEnd && to < toEnd) {
      if (*data == '\r') {
        data++;
        break; /* line boundary */
      } else if (*data < ' ') {
        switch (*data) {
        case '\n': *to++ = '\\'; *to++ = 'n'; break;
        case '\t': *to++ = '\t'; break;
        default:
          *to++ = '\\';
          *to++ = '0' + (*data / 64) % 8;
          *to++ = '0' + (*data / 8) % 8;
          *to++ = '0' + *data % 8;
          break;
        }
      } else {
        *to++ = *data;
      }
      data++;
      if (data[-1] == '\n')
        break;
    }
    *to = '\0';
    Log(level, (unsigned char *)logBuf);
  }
}

/* ─── HexLog ─────────────────────────────────────────────────────────────── */

void HexLog(unsigned long level, short format, unsigned char *data,
            short dSize) {
  if (!(level & LogLevel))
    return;

  char logBuf[128];
  unsigned char *dataEnd = data + dSize;

  while (data < dataEnd) {
    unsigned char *end = (data + 32 < dataEnd) ? data + 32 : dataEnd;
    char *p = logBuf;
    while (data < end) {
      *p++ = "0123456789ABCDEF"[(*data >> 4) & 0xF];
      *p++ = "0123456789ABCDEF"[*data & 0xF];
      data++;
    }
    *p = '\0';
    Log(level, (unsigned char *)logBuf);
  }
}
