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
 * log.c - POSIX port of Eudora's log file system.
 *
 * Replaces Mac HFS file I/O (AFSHOpen, FSWriteP, GetEOF, etc.) with
 * standard POSIX FILE* operations.  The live log-window feature
 * (FindText/PeteAppendText) is not available on POSIX; logging goes
 * to a plain text file only.
 *
 * Thread safety: uses MyThreadBeginCritical()/MyThreadEndCritical()
 * from threading.c, which are already ported to POSIX mutexes.
 */

#include "log.h"
#include "Globals.h"    /* LogRefN, LogLevel, LogTicks, P1..P4, Root */
#include "StringDefs.h" /* LOG_NAME, OLD_LOG, LOG_ROLLOVER resource IDs */
#include "StringUtil.h" /* PCopy, ComposeString, VaComposeString, etc. */
#include "fileutil.h"   /* SimpleMakeFSSpec */
#include "threading.h" /* MyThreadBeginCritical, MyThreadEndCritical, InAThread */
#define FILE_NUM 50

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* ─── Internal log file state ────────────────────────────────────────────── */

/*
 * We keep an internal FILE* for the log file.  LogRefN (an extern short in
 * Globals.h) is kept as a non-zero flag when the file is open so that
 * legacy callers checking `if (LogRefN)` continue to work.
 */
static FILE *gLogFile = NULL;

static void OpenLog(void);

/* ─── Helpers ────────────────────────────────────────────────────────────── */

/*
 * LogFilePath - build the POSIX path to a log file by name (Pascal string).
 * Uses SimpleMakeFSSpec which fills in spec.path from Root.vRef/dirId.
 */
static void LogFilePath(unsigned char *pname, char *outPath, size_t outLen) {
  FSSpec spec;
  SimpleMakeFSSpec(Root.vRef, Root.dirId, pname, &spec);
  strncpy(outPath, spec.path, outLen - 1);
  outPath[outLen - 1] = '\0';
}

/*
 * LogFileSize - return current size of the open log file in bytes.
 */
static long LogFileSize(void) {
  long pos;
  if (!gLogFile)
    return 0;
  pos = ftell(gLogFile);
  fseek(gLogFile, 0, SEEK_END);
  long size = ftell(gLogFile);
  fseek(gLogFile, pos, SEEK_SET);
  return size;
}

/* ─── ComposeLogR ────────────────────────────────────────────────────────── */

unsigned char *ComposeLogR(unsigned long level, unsigned char *into,
                           short format, ...) {
  unsigned char locl[256];
  unsigned char *reallyInto = into ? into : locl;
  va_list args;
  va_start(args, format);
  VaComposeRString(reallyInto, format, args);
  va_end(args);
  Log(level, reallyInto);
  return into;
}

/* ─── ComposeLogS ────────────────────────────────────────────────────────── */

unsigned char *ComposeLogS(unsigned long level, unsigned char *into,
                           unsigned char *format, ...) {
  unsigned char locl[256];
  unsigned char *reallyInto = into ? into : locl;
  va_list args;
  va_start(args, format);
  VaComposeString(reallyInto, format, args);
  va_end(args);
  Log(level, reallyInto);
  return into;
}

/* ─── Log ────────────────────────────────────────────────────────────────── */

unsigned char *Log(unsigned long level, unsigned char *string) {
  MyThreadBeginCritical();

  if (level == 0xFFFFFFFF || (level & LogLevel) != 0) {
    /* Build timestamp: elapsed ticks broken into hour:min:sec */
    long tickDiff = TickCount() - LogTicks;
    long hr = tickDiff / 3600;
    long min = (tickDiff / 60) % 60;
    long sec = tickDiff % 60;

    /* Thread identifier */
    char threadBuf[32];
    if (InAThread()) {
      snprintf(threadBuf, sizeof(threadBuf), "%lu",
               (unsigned long)(uintptr_t)pthread_self());
    } else {
      strncpy(threadBuf, "MAIN", sizeof(threadBuf));
    }

    /* Stamp line: "threadId level:hr:min:sec " */
    char stamp[64];
    snprintf(stamp, sizeof(stamp), "%s %lu:%ld.%ld.%ld ", threadBuf, level, hr,
             min, sec);

    /* Convert Pascal string to C string */
    char cstr[256];
    int clen = (int)(unsigned char)string[0];
    if (clen > (int)sizeof(cstr) - 1)
      clen = (int)sizeof(cstr) - 1;
    memcpy(cstr, string + 1, (size_t)clen);
    cstr[clen] = '\0';

    OpenLog();
    if (gLogFile) {
      fputs(stamp, gLogFile);
      fputs(cstr, gLogFile);
      /* Ensure Unix line ending */
      if (clen == 0 || cstr[clen - 1] != '\n')
        fputc('\n', gLogFile);
      fflush(gLogFile);
    }
  }

  MyThreadEndCritical();
  return string;
}

/* ─── CloseLog ───────────────────────────────────────────────────────────── */

void CloseLog(void) {
  long size = 0;

  if (!gLogFile)
    return;

  size = LogFileSize();
  fclose(gLogFile);
  gLogFile = NULL;
  LogRefN = 0;

  /* Roll over if file exceeds LOG_ROLLOVER KB */
  if (size > GetRLong(LOG_ROLLOVER) * 1024L) {
    char logPath[1024], oldPath[1024];
    unsigned char logName[256], oldName[256];
    GetRString(logName, LOG_NAME);
    GetRString(oldName, OLD_LOG);
    LogFilePath(logName, logPath, sizeof(logPath));
    LogFilePath(oldName, oldPath, sizeof(oldPath));
    remove(oldPath);
    rename(logPath, oldPath);
  }
}

/* ─── OpenLog ────────────────────────────────────────────────────────────── */

static void OpenLog(void) {
  char logPath[1024];
  unsigned char logName[256];
  time_t now;
  struct tm *tm_info;
  char dateBuf[64];

  /* Roll over if already open but too large */
  if (gLogFile && LogFileSize() > GetRLong(LOG_ROLLOVER) * 1024L)
    CloseLog();

  if (gLogFile)
    return; /* already open */

  GetRString(logName, LOG_NAME);
  LogFilePath(logName, logPath, sizeof(logPath));

  /* Open for append, creating if needed */
  gLogFile = fopen(logPath, "a");
  if (!gLogFile)
    return;

  LogRefN = 1; /* signal to callers: log is open */
  LogTicks = TickCount();

  /* Write a date/time header at the start of each session */
  time(&now);
  tm_info = localtime(&now);
  strftime(dateBuf, sizeof(dateBuf), "\r%Y-%m-%d %H:%M:%S\r", tm_info);
  fputs(dateBuf, gLogFile);
  fflush(gLogFile);
}

/* ─── LogAlert ───────────────────────────────────────────────────────────── */

void LogAlert(short template_id) {
  ComposeLogS(LOG_ALRT, NULL, (unsigned char *)"\pALRT %d", template_id);
}

/* ─── MyParamText ────────────────────────────────────────────────────────── */

/*
 * MyParamText - store dialog parameter strings in globals P1..P4 and log them.
 * Replaces the Mac ParamText() call (which fed into Dialog Manager).
 * On POSIX we just stash the strings for use by alert formatters.
 */
void MyParamText(PStr p1, PStr p2, PStr p3, PStr p4) {
  P1[0] = 0;
  P2[0] = 0;
  P3[0] = 0;
  P4[0] = 0;
  if (p1 && *p1) {
    PCopy(P1, p1);
    Log(LOG_ALRT, p1);
  }
  if (p2 && *p2) {
    PCopy(P2, p2);
    Log(LOG_ALRT, p2);
  }
  if (p3 && *p3) {
    PCopy(P3, p3);
    Log(LOG_ALRT, p3);
  }
  if (p4 && *p4) {
    PCopy(P4, p4);
    Log(LOG_ALRT, p4);
  }
  /* No Mac ParamText() call — GTK dialogs read P1..P4 directly */
}

/* ─── CarefulLog ─────────────────────────────────────────────────────────── */

/*
 * CarefulLog - log potentially long/binary data by escaping control chars
 * and splitting into 120-byte chunks.
 */
void CarefulLog(unsigned long level, short format, unsigned char *data,
                short dSize) {
  unsigned char logString[120];
  unsigned char *to, *dataEnd, *logEnd;

  if (!(level & LogLevel))
    return;

  dataEnd = data + dSize;
  do {
    to = logString + 1;
    logEnd = to + sizeof(logString) - 6;
    while (data < dataEnd) {
      if (*data < ' ') {
        *to++ = '\\';
        switch (*data) {
        case '\012':
          *to++ = 'n';
          break;
        case '\015':
          *to++ = 'r';
          break;
        case '\t':
          *to++ = 't';
          break;
        default:
          *to++ = '0' + (*data / 64) % 8;
          *to++ = '0' + (*data / 8) % 8;
          *to++ = '0' + *data % 8;
          break;
        }
      } else {
        *to++ = *data;
      }
      data++;
      if (to > logEnd || data[-1] == '\012')
        break;
    }
    *logString = (unsigned char)(to - logString - 1);
    ComposeLogR(level, NULL, format, logString);
  } while (data < dataEnd);
}

/* ─── LineLog ────────────────────────────────────────────────────────────── */

void LineLog(unsigned long level, short format, unsigned char *data,
             short dSize) {
  unsigned char logString[120];
  unsigned char *to, *dataEnd, *logEnd;

  if (!(level & LogLevel))
    return;

  dataEnd = data + dSize;
  do {
    to = logString + 1;
    logEnd = to + sizeof(logString) - 6;
    while (data < dataEnd) {
      if (*data < ' ') {
        *to++ = '\\';
        switch (*data) {
        case '\012':
          *to++ = 'n';
          break;
        case '\015':
          to--;
          data++;
          goto output;
        case '\t':
          to[-1] = '\t';
          break;
        default:
          *to++ = '0' + (*data / 64) % 8;
          *to++ = '0' + (*data / 8) % 8;
          *to++ = '0' + *data % 8;
          break;
        }
      } else {
        *to++ = *data;
      }
      data++;
      if (to > logEnd || data[-1] == '\012')
        break;
    }
  output:
    *logString = (unsigned char)(to - logString - 1);
    ComposeLogR(level, NULL, format, logString);
  } while (data < dataEnd);
}

/* ─── HexLog ─────────────────────────────────────────────────────────────── */

void HexLog(unsigned long level, short format, unsigned char *data,
            short dSize) {
  unsigned char logString[128];
  unsigned char *to, *dataEnd;

  if (!(level & LogLevel))
    return;

  dataEnd = data + dSize;
  do {
    to = data + 32 < dataEnd ? data + 32 : dataEnd;
    Bytes2Hex(data, (short)(to - data), logString + 1);
    *logString = (unsigned char)(2 * (to - data));
    ComposeLogR(level, NULL, format, logString);
    data = to;
  } while (data < dataEnd);
}
