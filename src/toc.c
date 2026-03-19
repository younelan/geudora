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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. */

/* toc.c - GTK4/glib port of Eudora's Table of Contents management
 *
 * Full port from original Mac toc.c:
 * - Resource fork TOC paths removed (data-fork .toc only)
 * - Pascal strings → C strings, ComposeLogS → g_debug/g_warning
 * - Carbon file APIs → fopen/fread/fwrite/stat
 * - Mac dialog alerts → GTK dialogs
 * - All original functionality preserved
 */

#define FILE_NUM 72
#include "toc.h"
#include <glib/gstdio.h>
#include "buildtoc.h"
#include "euErrors.h"
#include "fileutil.h"
#include "imapmailboxes.h"
#include "junk.h"
#include "mydefs.h"
#include "StringDefs.h"
#include "gtk_prefs.h"

#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* External globals */
extern TOCType * TOCList;

/* Forward declaration — WriteTOC defined below */
int WriteTOC(TOCType * tocH);

/*
 * toc_file_path — derive the .toc filename from a mailbox path.
 * Writes into buf (which must be at least PATH_MAX). Returns buf.
 */
static char *toc_file_path(const char *mailboxPath, char *buf, size_t bufsz)
{
  snprintf(buf, bufsz, "%s.toc", mailboxPath);
  return buf;
}

/* basename helper — pointer into path after last '/' */
/* path_basename provided by mailbox.h */

/* TOC version constants */
#ifndef CURRENT_TOC_MINOR
#define CURRENT_TOC_MINOR 9
#endif
#ifndef TOC_MINOR_HAS_MOOD
#define TOC_MINOR_HAS_MOOD 3
#endif
#ifndef TOC_MINOR_HAS_LONG_K
#define TOC_MINOR_HAS_LONG_K 4
#endif
#ifndef TOC_MINOR_HAS_PROFILE
#define TOC_MINOR_HAS_PROFILE 5
#endif
#ifndef TOC_MINOR_NO_DATE_STRING
#define TOC_MINOR_NO_DATE_STRING 6
#endif
#ifndef TOC_MINOR_FIXED_OUTLOOKISMS
#define TOC_MINOR_FIXED_OUTLOOKISMS 7
#endif
#ifndef TOC_MINOR_NO_UNNECESSARY_UTF8
#define TOC_MINOR_NO_UNNECESSARY_UTF8 9
#endif

/* Forward declarations */
static int ReadDForkTOC(char * aSpec, TOCType * *inTOC);
char * Box2TOCSpec(char * boxSpec, char * tocSpec);
static short GetMailboxType(char * spec);
void CleanseTOC(TOCType * tocH);
static int InsaneTOC(TOCType * tocH);
TOCType * ReadTOC(char * spec);
static short GetTOCK(TOCType * tocH, unsigned long *usedK, unsigned long *totalK);
static bool TOCUnread(TOCType * tocH);
static void FixBoxUnread(TOCType * tocH);
static TOCType * FixErrantTOC(char * spec, TOCType * tocH, short why);
static void CheckStringLen(char *s, int maxLen, int fillLen);

/* External prototypes for functions defined in other modules */
int GetMailbox(const char *path, bool showIt);
bool IsSpool(const char *path);
bool IsDelivery(const char *path);
void InvalBoxSizeBox(void *wp);
void FixSpecUnread(const char *path, bool unread);
void FixMenuUnread(MenuHandle mh, int item, bool unread);
int Spec2Menu(char * spec, bool forXfer, short *menu, short *item);
void BoxFClose(TOCType * tocH, bool flush);
bool IsIMAPMailboxFileLo(char * spec, MailboxNodeHandle *node);
void RemoveUTF8FromSum(MSumPtr sum);
void JunkTOCCleanse(TOCType * tocH);
/* DBNoteUIDHash is inline in legacy_shim.h */
unsigned long GMTDateTime(void);
long GetRLong(int index);
MenuHandle GetMHandle(short menuId);
TOCType * RebuildTOC(const char *path, TOCType * oldTocH, bool resource,
                     bool autoRebuild);

/* Mail directory — use the same path as prefs_get_mailboxes_path() */
static const char *get_mail_dir(void) {
  const char *mb = prefs_get_mailboxes_path();
  if (mb && mb[0])
    return mb;
  /* Fallback before prefs_init() has been called */
  static char dir[1024] = {0};
  if (!dir[0]) {
    const char *home = g_get_home_dir();
    snprintf(dir, sizeof(dir), "%s/.local/share/geudora/mailboxes", home);
  }
  return dir;
}

#define CURRENT_TOC_VERS 1

/*
 * On-disk TOC format — portable 32-bit layout matching original Mac Eudora.
 *
 * Uses fixed-size integer types (int32_t, int16_t) instead of `long`
 * so the binary format is identical whether built on 32-bit or 64-bit.
 * Pointer fields (mesgErrH, cache, messH) are stored as 4-byte zeros.
 *
 * This allows us to read TOC files from original Mac Eudora 6.2.4
 * and write files that are compatible across platforms.
 */

/* Portable disk header — 76 bytes, matches original Mac 32-bit layout */
typedef struct __attribute__((packed)) {
  int32_t majorVersion;
  int32_t minorVersion;
  int16_t count;
  int16_t which;
  int32_t boxSize;
  int32_t writeDate;
  int32_t nextSerialNum;
  int32_t sort;
  int32_t lastSort;
  int32_t pluginKey;
  int32_t pluginValue;
  int32_t previewHi;
  int32_t unreadBase;
  int32_t sorts[6];
  int32_t needsCompact;
} TOCDiskHeader;

/* Portable disk summary — 224 bytes, matches original Mac 32-bit MSumType */
typedef struct __attribute__((packed)) {
  int32_t offset;
  int32_t length;
  int32_t bodyOffset;
  int32_t state;
  int32_t spamBits;        /* spamScore:8 + spamBecause:3 + spare21:21 */
  uint32_t arrivalSeconds;
  uint32_t mesgErrH;       /* was pointer — always 0 on disk */
  uint32_t fromHash;
  uint32_t spare[3];
  int32_t serialNum;
  uint32_t seconds;
  uint32_t flags;
  int16_t savedPos[4];     /* Rect: top, left, bottom, right */
  uint8_t priority;
  uint8_t origPriority;
  int16_t tableId;
  int16_t scoreBits;       /* score:4 + outType:4 + unused:8 */
  int16_t spareShort2;
  int16_t sumRandBytes;
  int16_t origZone;
  uint32_t sigId;
  char from[48];
  uint32_t popPersId;
  uint32_t persId;
  int32_t msgIdHash;
  int16_t subjId;
  int16_t spareShort;
  char subj[60];
  uint32_t opts;
  uint32_t uidHash;
  uint32_t cache;          /* was Handle — always 0 on disk */
  uint8_t selected;
  uint8_t _pad[3];
  uint32_t messH;          /* was pointer — always 0 on disk */
} MSumDisk;

_Static_assert(sizeof(TOCDiskHeader) == 76, "TOCDiskHeader must be 76 bytes");
_Static_assert(sizeof(MSumDisk) == 224, "MSumDisk must be 224 bytes");

#define TOCDiskSize(count) \
  ((long)sizeof(TOCDiskHeader) + (long)(count) * (long)sizeof(MSumDisk))

/* Legacy macro kept for compat */
#define TOCSizeShouldBe(tocH) \
  TOCDiskSize((tocH)->count)

/*
 * Conversion: MSumDisk ↔ MSumType (in-memory)
 */
static void disk_to_sum(const MSumDisk *d, MSumType *s) {
  memset(s, 0, sizeof(*s));
  s->offset = d->offset;
  s->length = d->length;
  s->bodyOffset = d->bodyOffset;
  s->state = (StateEnum)d->state;
  /* Unpack bitfield */
  s->spamScore = (int8_t)(d->spamBits & 0xFF);
  s->spamBecause = (d->spamBits >> 8) & 0x7;
  s->arrivalSeconds = d->arrivalSeconds;
  s->fromHash = d->fromHash;
  memcpy(s->spare, d->spare, sizeof(s->spare));
  s->serialNum = d->serialNum;
  s->seconds = d->seconds;
  s->flags = d->flags;
  /* Rect / VirtualMessData union */
  s->u.savedPos.top = d->savedPos[0];
  s->u.savedPos.left = d->savedPos[1];
  s->u.savedPos.bottom = d->savedPos[2];
  s->u.savedPos.right = d->savedPos[3];
  s->priority = d->priority;
  s->origPriority = d->origPriority;
  s->tableId = d->tableId;
  /* Unpack score bits */
  s->score = d->scoreBits & 0xF;
  s->outType = (d->scoreBits >> 4) & 0xF;
  s->unused = (d->scoreBits >> 8) & 0xFF;
  s->spareShort2 = d->spareShort2;
  s->sumRandBytes = d->sumRandBytes;
  s->origZone = d->origZone;
  s->sigId = d->sigId;

  /* Copy from/subj as C strings — no Pascal conversion needed */
  g_strlcpy(s->from, d->from, sizeof(s->from));

  s->popPersId = d->popPersId;
  s->persId = d->persId;
  s->msgIdHash = d->msgIdHash;
  s->subjId = d->subjId;
  s->spareShort = d->spareShort;

  g_strlcpy(s->subj, d->subj, sizeof(s->subj));

  s->opts = d->opts;
  s->uidHash = d->uidHash;
  /* pointer fields stay NULL */
  s->selected = d->selected;
}

static void sum_to_disk(const MSumType *s, MSumDisk *d) {
  memset(d, 0, sizeof(*d));
  d->offset = (int32_t)s->offset;
  d->length = (int32_t)s->length;
  d->bodyOffset = (int32_t)s->bodyOffset;
  d->state = (int32_t)s->state;
  /* Pack bitfield */
  d->spamBits = (s->spamScore & 0xFF) | ((s->spamBecause & 0x7) << 8);
  d->arrivalSeconds = (uint32_t)s->arrivalSeconds;
  d->mesgErrH = 0;
  d->fromHash = (uint32_t)s->fromHash;
  memcpy(d->spare, s->spare, sizeof(d->spare));
  d->serialNum = (int32_t)s->serialNum;
  d->seconds = (uint32_t)s->seconds;
  d->flags = (uint32_t)s->flags;
  d->savedPos[0] = s->u.savedPos.top;
  d->savedPos[1] = s->u.savedPos.left;
  d->savedPos[2] = s->u.savedPos.bottom;
  d->savedPos[3] = s->u.savedPos.right;
  d->priority = s->priority;
  d->origPriority = s->origPriority;
  d->tableId = s->tableId;
  d->scoreBits = (s->score & 0xF) | ((s->outType & 0xF) << 4) |
                 ((s->unused & 0xFF) << 8);
  d->spareShort2 = s->spareShort2;
  d->sumRandBytes = s->sumRandBytes;
  d->origZone = s->origZone;
  d->sigId = (uint32_t)s->sigId;

  /* Always write as C strings from now on */
  strncpy(d->from, s->from, sizeof(d->from) - 1);
  d->from[sizeof(d->from) - 1] = '\0';

  d->popPersId = (uint32_t)s->popPersId;
  d->persId = (uint32_t)s->persId;
  d->msgIdHash = (int32_t)s->msgIdHash;
  d->subjId = s->subjId;
  d->spareShort = s->spareShort;

  strncpy(d->subj, s->subj, sizeof(d->subj) - 1);
  d->subj[sizeof(d->subj) - 1] = '\0';

  d->opts = (uint32_t)s->opts;
  d->uidHash = (uint32_t)s->uidHash;
  d->cache = 0;
  d->selected = s->selected;
  d->messH = 0;
}

/************************************************************************
 * TOCBySpec - take a spec, return a TOC
 ************************************************************************/
TOCType * TOCBySpec(char * spec) {
  if (!GetMailbox(spec, false))
    return FindTOC(spec);
  return NULL;
}

/************************************************************************
 * TOCByPath - find a TOC by mailbox file path (portable, no FSSpec)
 ************************************************************************/
TOCType * TOCByPath(const char *path) {
  if (!path || !path[0]) return NULL;

  for (TOCType *tocH = TOCList; tocH; tocH = tocH->next) {
    if (tocH->path[0] && strcmp(tocH->path, path) == 0)
      return tocH;
  }
  return NULL;
}

/************************************************************************
 * GetTOCByFSS - open a toc from an FSS
 ************************************************************************/
short GetTOCByFSS(char * specPtr, TOCType * *tocH) {
  *tocH = TOCBySpec(specPtr);
  return (*tocH ? 0 : 1);
}

/************************************************************************
 * KillTOC - remove the .toc file for a mailbox
 * Ported from Mac: resource fork operations → unlink .toc file
 ************************************************************************/
int KillTOC(short refN, char * spec) {
  (void)refN;
  if (!spec)
    return 0;

  char tocPath[PATH_MAX];
  toc_file_path(spec, tocPath, sizeof(tocPath));

  if (g_unlink(tocPath) != 0 && errno != ENOENT) {
    g_warning("KillTOC: failed to remove %s: %s", tocPath, g_strerror(errno));
    return -1;
  }
  return 0;
}

/************************************************************************
 * Box2TOCSpec - make a toc spec out of a mailbox spec
 * Ported from Mac: PCat(name, GetRString(suffix, TOC_SUFFIX)) → snprintf
 ************************************************************************/
char * Box2TOCSpec(char * boxSpec, char * tocSpec) {
  snprintf(tocSpec, PATH_MAX, "%s.toc", boxSpec);
  return tocSpec;
}

/************************************************************************
 * HasExternalTOC - does a file have an external table of contents?
 * Ported from Mac: FSpExists → access()
 ************************************************************************/
bool HasExternalTOC(char * spec) {
  char tocPath[PATH_MAX];
  toc_file_path(spec, tocPath, sizeof(tocPath));
  return g_file_test(tocPath, G_FILE_TEST_EXISTS);
}

/************************************************************************
 * CheckTOC - check a file for a table of contents, and build it if
 * necessary. Combines original CheckTOC + ReadTOC logic.
 * Ported from Mac: resource fork path removed, uses data-fork .toc only
 ************************************************************************/
TOCType * CheckTOC(char * spec) {
  if (!spec || !spec[0])
    return NULL;

  unsigned long box, res, file;
  int err = TOCDates(spec, &box, &res, &file);
  if (err && err != ENOENT)
    return NULL;

  /* No .toc file → build from mailbox and write the .toc */
  if (!file) {
    g_debug("CheckTOC: no .toc for %s, building", path_basename(spec));
    TOCType *built = BuildTOC_Path(spec);
    if (built) {
      g_strlcpy(built->path, spec, sizeof(built->path));
      g_strlcpy(built->mailbox.spec, spec, sizeof(built->mailbox.spec));
      WriteTOC(built);
    }
    return built;
  }

  /* Read the data-fork .toc */
  TOCType *toc = ReadTOC(spec);
  if (!toc) {
    /* .toc file exists but is corrupt or too small — rebuild from mailbox */
    g_debug("CheckTOC: .toc for %s is corrupt, rebuilding", path_basename(spec));
    toc = BuildTOC_Path(spec);
    if (toc) {
      g_strlcpy(toc->path, spec, sizeof(toc->path));
      g_strlcpy(toc->mailbox.spec, spec, sizeof(toc->mailbox.spec));
      WriteTOC(toc);
    }
  }
  return toc;
}

/************************************************************************
 * ReadTOC - read the toc file for a mailbox
 * Ported from Mac: resource fork path removed, only data-fork
 ************************************************************************/
TOCType * ReadTOC(char * spec) {
  TOCType * tocH = NULL;
  int insane = 0;

  insane = ReadDForkTOC(spec, &tocH);

  if (tocH) {
    /* Don't take these for granted */
    tocH->durty = tocH->reallyDirty = false;
    g_strlcpy(tocH->path, spec, sizeof(tocH->path));
    g_strlcpy(tocH->mailbox.spec, spec, sizeof(tocH->mailbox.spec));
    tocH->refN = 0;
    tocH->win = NULL;
    tocH->volumeFree = 0;
    tocH->previewID = 0;
    tocH->previewPTE = NULL;
    tocH->lastSameTicks = 0;
    tocH->mouseTicks = 0;
    tocH->mouseSpot.h = tocH->mouseSpot.v = 0;
    tocH->userActive = 0;
    tocH->drawerWin = NULL;

    /* Check if this is an IMAP mailbox */
    tocH->imapTOC =
        IsIMAPMailboxFileLo(spec, &(tocH->imapMBH)) ? (void *)1 : NULL;

    /* Make sure it hasn't become special or unspecial */
    tocH->which = GetMailboxType(spec);

    /* Clean up leftovers from previous session */
    CleanseTOC(tocH);

    /* Check toc for reasonableness */
    if ((insane = InsaneTOC(tocH)))
      return FixErrantTOC(spec, tocH, insane);

    /* Update sizes */
    unsigned long usedK, totalK;
    GetTOCK(tocH, &usedK, &totalK);
    tocH->usedK = (long)usedK;
    tocH->totalK = (long)totalK;
    tocH->updateBoxSizes = true;

    /* Check unread status */
    tocH->unread = TOCUnread(tocH);

    return tocH;
  } else if (insane == ENOMEM)
    return NULL;
  else
    return FixErrantTOC(spec, NULL, euCorruptTOC);
}

/************************************************************************
 * TOCDates - get the dates off a mailbox
 * Ported from Mac: AFSpGetMod/PeekRTOC → g_stat(). Resource fork date = 0
 ************************************************************************/
int TOCDates(char * spec, unsigned long *box, unsigned long *res, unsigned long *file) {
  GStatBuf st;

  /* Mailbox modification time */
  if (g_stat(spec, &st) == 0)
    *box = (unsigned long)st.st_mtime;
  else
    *box = 0;

  /* .toc file modification time */
  char tocPath[PATH_MAX];
  toc_file_path(spec, tocPath, sizeof(tocPath));
  if (g_stat(tocPath, &st) == 0)
    *file = (unsigned long)st.st_mtime;
  else
    *file = 0;

  /* No resource fork on portable systems */
  *res = 0;

  if (!*box && !*file) {
    g_debug("TOCDates(%s): not found", path_basename(spec));
    return ENOENT;
  }
  return 0;
}

/************************************************************************
 * ReadDForkTOC - read a TOC from the data-fork .toc file
 * Ported from Mac: AFSpOpenDF/file_size/file_read → fopen/fread
 ************************************************************************/
static int ReadDForkTOC(char * aSpec, TOCType * *inTOC) {
  *inTOC = NULL;

  char tocPath[PATH_MAX];
  toc_file_path(aSpec, tocPath, sizeof(tocPath));
  const char *baseName = path_basename(aSpec);

  FILE *fp = fopen(tocPath, "rb");
  if (!fp) {
    g_warning("ReadDForkTOC(%s): %s", baseName, g_strerror(errno));
    return ENOENT;
  }

  /* Get file size */
  fseek(fp, 0, SEEK_END);
  long fileSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (fileSize < (long)sizeof(TOCDiskHeader)) {
    g_warning("ReadDForkTOC: %s too small (%ld < %lu)", tocPath, fileSize,
              (unsigned long)sizeof(TOCDiskHeader));
    fclose(fp);
    return -1;
  }

  /* Read portable 32-bit disk header (76 bytes) */
  TOCDiskHeader hdr;
  if (fread(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) {
    g_warning("ReadDForkTOC(%s): header read failed", baseName);
    fclose(fp);
    return -1;
  }

  if (hdr.count < 0 || hdr.count > 30000) {
    g_warning("ReadDForkTOC(%s): bad count %d", baseName, hdr.count);
    fclose(fp);
    return euCorruptTOC;
  }

  /* Verify file has enough data for all summaries.
   * Also detect stale .toc files from old gEudora builds that used
   * native 64-bit structs (152-byte header, 320-byte sums). */
  long expectedSize = TOCDiskSize(hdr.count);
  if (fileSize < expectedSize) {
    g_debug("ReadDForkTOC(%s): format mismatch (%ld bytes, expected %ld for %d msgs) — will rebuild",
            baseName, fileSize, expectedSize, hdr.count);
    fclose(fp);
    /* Delete the stale .toc so CheckTOC rebuilds from mailbox */
    g_unlink(tocPath);
    return euCorruptTOC;
  }

  /* Allocate full in-memory TOC */
  long tocMemSize = (long)(sizeof(TOCType) + MAX(0, hdr.count - 1) * sizeof(MSumType));
  TOCType *toc = (TOCType *)g_malloc0(tocMemSize);
  if (!toc) {
    fclose(fp);
    return ENOMEM;
  }

  /* Copy header fields into TOC struct */
  toc->majorVersion = hdr.majorVersion;
  toc->minorVersion = hdr.minorVersion;
  toc->count = hdr.count;
  toc->which = hdr.which;
  toc->boxSize = hdr.boxSize;
  toc->writeDate = hdr.writeDate;
  toc->nextSerialNum = hdr.nextSerialNum;
  toc->sort = hdr.sort;
  toc->lastSort = hdr.lastSort;
  toc->pluginKey = hdr.pluginKey;
  toc->pluginValue = hdr.pluginValue;
  toc->previewHi = hdr.previewHi;
  toc->unreadBase = hdr.unreadBase;
  memcpy(toc->sorts, hdr.sorts, sizeof(toc->sorts));
  toc->needsCompact = hdr.needsCompact;

  /* Read message summaries using portable MSumDisk (224 bytes each),
   * then convert to in-memory MSumType */
  if (hdr.count > 0) {
    MSumDisk diskSum;
    for (int i = 0; i < hdr.count; i++) {
      if (fread(&diskSum, 1, sizeof(diskSum), fp) != sizeof(diskSum)) {
        g_warning("ReadDForkTOC(%s): sum[%d] read failed", baseName, i);
        g_free(toc);
        fclose(fp);
        return -1;
      }
      disk_to_sum(&diskSum, &toc->sums[i]);
    }
  }

  fclose(fp);

  g_debug("ReadDForkTOC(%s): %d messages, %ld bytes", baseName, toc->count,
          fileSize);
  *inTOC = toc;
  return 0;
}

/************************************************************************
 * WriteTOC - write a toc to the proper file
 * Ported from Mac: FSpCreate/file_write → fopen/fwrite.
 * Resource fork path removed. Reentrant write protection preserved.
 ************************************************************************/
int WriteTOC(TOCType * tocH) {
  if (!tocH)
    return -1;

  /* Avoid multiple simultaneous writes */
  if (tocH->beingWritten) {
    g_warning("WriteTOC: reentrant write detected");
    tocH->beingWritten = 1;
  }
  tocH->beingWritten++;

  /* Get mailbox file size */
  GStatBuf st;
  long size = 0;
  if (g_stat(tocH->path, &st) == 0)
    size = (long)st.st_size;

  tocH->boxSize = size + 1; /* +1 signals we know it's ok */
  tocH->writeDate = (unsigned long)time(NULL);
  tocH->unreadBase = tocH->count;

  char tocPath[PATH_MAX];
  toc_file_path(tocH->path, tocPath, sizeof(tocPath));

  FILE *fp = fopen(tocPath, "wb");
  if (!fp) {
    g_warning("WriteTOC(%s): %s", path_basename(tocH->path), g_strerror(errno));
    tocH->beingWritten--;
    return -1;
  }

  /* Write portable 32-bit disk header (76 bytes) */
  TOCDiskHeader hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.majorVersion = (int32_t)tocH->majorVersion;
  hdr.minorVersion = (int32_t)tocH->minorVersion;
  hdr.count = (int16_t)tocH->count;
  hdr.which = (int16_t)tocH->which;
  hdr.boxSize = (int32_t)tocH->boxSize;
  hdr.writeDate = (int32_t)tocH->writeDate;
  hdr.nextSerialNum = (int32_t)tocH->nextSerialNum;
  hdr.sort = (int32_t)tocH->sort;
  hdr.lastSort = (int32_t)tocH->lastSort;
  hdr.pluginKey = (int32_t)tocH->pluginKey;
  hdr.pluginValue = (int32_t)tocH->pluginValue;
  hdr.previewHi = (int32_t)tocH->previewHi;
  hdr.unreadBase = (int32_t)tocH->unreadBase;
  for (int i = 0; i < 6; i++)
    hdr.sorts[i] = (int32_t)tocH->sorts[i];
  hdr.needsCompact = (int32_t)tocH->needsCompact;

  size_t written = fwrite(&hdr, 1, sizeof(hdr), fp);
  if (written != sizeof(hdr)) {
    g_warning("WriteTOC(%s): header write failed", path_basename(tocH->path));
    fclose(fp);
    g_unlink(tocPath);
    tocH->beingWritten--;
    return -1;
  }

  /* Write message summaries using portable MSumDisk (224 bytes each) */
  if (tocH->count > 0) {
    MSumDisk diskSum;
    for (int i = 0; i < tocH->count; i++) {
      sum_to_disk(&tocH->sums[i], &diskSum);
      written = fwrite(&diskSum, 1, sizeof(diskSum), fp);
      if (written != sizeof(diskSum)) {
        g_warning("WriteTOC(%s): sum[%d] write failed", path_basename(tocH->path), i);
        fclose(fp);
        g_unlink(tocPath);
        tocH->beingWritten--;
        return -1;
      }
    }
  }

  fclose(fp);

  tocH->durty = tocH->reallyDirty = false;
  g_debug("WriteTOC(%s): %d messages, %ld bytes", path_basename(tocH->path),
          tocH->count, (long)TOCDiskSize(tocH->count));

  /* Fix up menu items */
  FixBoxUnread(tocH);

  /* The size area */
  if (tocH->win && IsWindowVisible(GetMyWindowWindowPtr(tocH->win)))
    InvalBoxSizeBox(tocH->win);

  tocH->beingWritten--;
  return 0;
}

/************************************************************************
 * FixBoxUnread - fix the unread status of a mailbox in the menus
 * Ported from Mac: GetMHandle → uses Spec2Menu + FixMenuUnread
 ************************************************************************/
static void FixBoxUnread(TOCType * tocH) {
  if (!tocH)
    return;

  bool unread = TOCUnread(tocH);
  short myMenu, myItem;
  char spec[PATH_MAX];
  unsigned long total, used;

  tocH->unread = unread;
  tocH->unreadBase = tocH->count;

  myItem = 0;
  /* Build a minimal spec from tocH for legacy Spec2Menu API */
  memset(&spec, 0, sizeof(spec));
  g_strlcpy(spec, tocH->path, sizeof(spec));
  Spec2Menu(spec, false, &myMenu, &myItem);

  if (myItem > 0) {
    FixSpecUnread(spec, unread);
    FixMenuUnread(GetMHandle(myMenu), myItem, unread);
  }

  GetTOCK(tocH, &used, &total);
  if ((unsigned long)tocH->usedK != used || (unsigned long)tocH->totalK != total) {
    tocH->usedK = (long)used;
    tocH->totalK = (long)total;
    tocH->updateBoxSizes = true;
  }
}

/************************************************************************
 * TOCUnread - does a table of contents contain unread messages?
 * Ported from Mac: GetRLong(UNREAD_LIMIT) for date-based filtering
 ************************************************************************/
static bool TOCUnread(TOCType * tocH) {
  if (!tocH)
    return false;

  /* Don't underline Junk if the user has asked us not to */
  if ((tocH->which == JUNK) || IsIMAPJunkMailbox(TOCToMbox(tocH))) {
    if (JunkPrefBoxNoUnread())
      return false;
  }

  /* Date-based unread limit */
  unsigned long minDate = GetRLong(UNREAD_LIMIT) * 24 * 3600;
  if (minDate)
    minDate = GMTDateTime() - minDate;

  for (int i = 0; i < tocH->count; i++) {
    if (tocH->sums[i].state == UNREAD &&
        (!minDate || (unsigned long)tocH->sums[i].seconds > minDate)) {
      return true;
    }
  }
  return false;
}

/************************************************************************
 * TOCUnreadCount - how many messages are unread in this toc?
 * Ported from Mac: supports recentOnly date filtering
 ************************************************************************/
short TOCUnreadCount(TOCType * tocH, bool recentOnly) {
  short count = 0;
  unsigned long minDate = 0;
  if (!tocH)
    return 0;

  if (recentOnly) {
    minDate = GetRLong(UNREAD_LIMIT) * 24 * 3600;
    if (minDate)
      minDate = GMTDateTime() - minDate;
  }

  if (recentOnly && minDate) {
    for (int i = tocH->count - 1; i >= 0; i--)
      if (tocH->sums[i].state == UNREAD &&
          (unsigned long)tocH->sums[i].seconds > minDate)
        count++;
  } else {
    for (int i = tocH->count - 1; i >= 0; i--)
      if (tocH->sums[i].state == UNREAD)
        count++;
  }

  return count;
}

/************************************************************************
 * FindTOC - find a TOC in the TOC window list
 * Ported from Mac: IsAlias fallback removed (Mac-only concept)
 ************************************************************************/
TOCType * FindTOC(const char *path) {
  if (!path)
    return NULL;

  /* Prefer path comparison when available (portable) */
  if (path[0]) {
    for (TOCType *tocH = TOCList; tocH; tocH = tocH->next) {
      if (tocH->path[0] && strcmp(tocH->path, path) == 0)
        return tocH;
    }
  }

  /* Fallback to SameSpec for legacy callers: construct a temporary spec */
  char tmpSpec[PATH_MAX];
  spec_make(NULL, path, tmpSpec);
  for (TOCType *tocH = TOCList; tocH; tocH = tocH->next) {
    char boxSpec[PATH_MAX]; GetMailboxSpec(tocH, -1, boxSpec);
    if (SameSpec(boxSpec, tmpSpec))
      return tocH;
  }
  return NULL;
}

/************************************************************************
 * FlushTOCs - make sure all toc's are quiescent
 * Ported from Mac: full logic with BoxFClose, dirty checks, close logic
 ************************************************************************/
int FlushTOCs(bool andClose, bool canSkip) {
  TOCType *tocH, *nextTocH;
  short err = 0;
  static long lastTime;
  static short delay;
  bool dontCloseIMAPToc;

  if (GetNumBackgroundThreads())
    return 0;

  /* Skip if we recently failed and backoff hasn't expired */
  if (canSkip && lastTime && (long)(time(NULL) - lastTime) < delay)
    return 0;

  for (tocH = TOCList; tocH; tocH = nextTocH) {
    nextTocH = tocH->next;

    /* Close any open mailbox file handle */
    BoxFClose(tocH, true);

    if (tocH->reallyDirty) {
      if ((err = WriteTOC(tocH)))
        break;
    } else if (tocH->unreadBase != tocH->count) {
      FixBoxUnread(tocH);
    }

    if (andClose) {
      GtkWidget *tocWinWP = GetMyWindowWindowPtr(tocH->win);

      /* Don't close IMAP toc that's in use */
      dontCloseIMAPToc =
          tocH->imapTOC && IMAPTOCInUse(TOCToMbox(tocH));

      if (!dontCloseIMAPToc && !IsWindowVisible(tocWinWP)) {
        /* Check no open messages */
        int sNum;
        for (sNum = 0; sNum < tocH->count; sNum++)
          if (tocH->sums[sNum].messH)
            break;
        if (sNum == tocH->count)
          CloseMyWindow(tocWinWP);
      }
    }
  }

  /* Backoff timer on failure */
  if (err) {
    lastTime = (long)time(NULL);
    if (!delay)
      delay = 30;
    else
      delay = MIN(300, (delay * 3) / 2);
  } else {
    lastTime = delay = 0;
  }

  return err;
}

/************************************************************************
 * GetSpecialTOC - find the toc of a special mailbox
 * Ported from Mac: GetRString/FSMakeFSSpec → snprintf/get_mail_dir()
 ************************************************************************/
TOCType * GetSpecialTOC(short nameId) {
  const char *name = NULL;
  switch (nameId) {
  case IN:       name = "In";       break;
  case OUT:      name = "Out";      break;
  case TRASH:    name = "Trash";    break;
  case JUNK:     name = "Junk";     break;
  case IN_TEMP:  name = "In.temp";  break;
  case OUT_TEMP: name = "Out.temp"; break;
  default:       return NULL;
  }

  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s", get_mail_dir(), name);

  /* Create the mailbox file if it doesn't exist */
  GStatBuf st;
  if (g_stat(path, &st) != 0) {
    FILE *fp = fopen(path, "w");
    if (fp) fclose(fp);
  }

  /* Try path-based lookup first */
  TOCType *tocH = TOCByPath(path);
  if (tocH) return tocH;

  /* Fall back to path-based lookup (loads/builds the TOC) */
  char spec[PATH_MAX];
  memset(&spec, 0, sizeof(spec));
  g_strlcpy(spec, path, sizeof(spec));
  return TOCBySpec(spec);
}

/************************************************************************
 * PeekTOC - peek into a .toc file to get basic info
 * Ported from Mac: resource fork path removed, data-fork only
 ************************************************************************/
int PeekTOC(char * spec, TOCType *tocPart) {
  /* If already open, return the live copy */
  TOCType * tocH = FindTOC(spec);
  if (tocH) {
    *tocPart = *tocH;
    return 0;
  }

  /* Check dates */
  unsigned long box, file, res;
  int err = TOCDates(spec, &box, &res, &file);
  if (err)
    return err;

  if (!file)
    return ENOENT;

  /* Read from .toc file (data fork) using disk header format */
  char tocPath[PATH_MAX];
  toc_file_path(spec, tocPath, sizeof(tocPath));

  FILE *fp = fopen(tocPath, "rb");
  if (!fp)
    return ENOENT;

  TOCDiskHeader hdr;
  size_t nread = fread(&hdr, 1, sizeof(hdr), fp);
  fclose(fp);

  if (nread < sizeof(hdr))
    return ENOENT;

  /* Fill in the relevant fields */
  memset(tocPart, 0, sizeof(TOCType));
  tocPart->majorVersion = hdr.majorVersion;
  tocPart->minorVersion = hdr.minorVersion;
  tocPart->count = hdr.count;
  tocPart->which = hdr.which;
  tocPart->boxSize = hdr.boxSize;
  tocPart->writeDate = hdr.writeDate;
  tocPart->unreadBase = hdr.unreadBase;

  return 0;
}

/************************************************************************
 * InsaneTOC - see if a TOC is structurally valid
 * Ported from Mac: FSpDFSize → g_stat(), GetHandleSize_ check skipped
 * (handle sizes are implicit in glib malloc)
 ************************************************************************/
static int InsaneTOC(TOCType * tocH) {
  if (!tocH)
    return -1;

  /* Negative count */
  if (tocH->count < 0)
    return euCorruptTOC;

  /* Figure out how big the mailbox is */
  GStatBuf st;
  long boxSize = 0;
  if (g_stat(tocH->path, &st) == 0)
    boxSize = (long)st.st_size;
  else
    g_warning("InsaneTOC(%s): stat failed for '%s': %s",
              path_basename(tocH->path), tocH->path, g_strerror(errno));

  /* Right size? Allow off-by-one (Mac line ending differences) */
  if (tocH->boxSize && boxSize > 0 &&
      labs(tocH->boxSize - boxSize) > 1) {
    g_warning("InsaneTOC(%s): file size mismatch (toc=%ld, file=%ld)",
              path_basename(tocH->path), tocH->boxSize, boxSize);
    return euMismatchTOC;
  }
  /* If stat failed (boxSize==0), update boxSize from actual file */
  if (boxSize > 0)
    tocH->boxSize = boxSize;

  /* Check for out-of-range pointers */
  for (int i = 0; i < tocH->count; i++) {
    MSumPtr sum = &tocH->sums[i];
    if ((sum->offset < 0 && !tocH->imapTOC) || sum->length < 0 ||
        sum->bodyOffset < 0 || sum->bodyOffset > sum->length ||
        ((sum->offset + sum->length > boxSize) && !tocH->imapTOC)) {
      g_warning("InsaneTOC(%s): bad sum #%d (o=%ld b=%ld l=%ld s=%ld)",
                path_basename(tocH->path), i, (long)sum->offset, (long)sum->bodyOffset, (long)sum->length,
                boxSize);
      return euCorruptTOC;
    }
  }

  /* Wrong version number? */
  if (tocH->majorVersion > CURRENT_TOC_VERS) {
    g_warning("InsaneTOC(%s): version mismatch (%ld != %d)", path_basename(tocH->path),
              tocH->majorVersion, CURRENT_TOC_VERS);
    return euBadVersion;
  }

  return 0;
}

/************************************************************************
 * GetMailboxType - determine which special mailbox this is
 * Ported from Mac: EqualStrRes/IsRoot → strcasecmp on name
 ************************************************************************/
static short GetMailboxType(char * spec) {
  if (!spec || !path_basename(spec)[0])
    return 0;

  if (strcasecmp(path_basename(spec), "In") == 0)
    return IN;
  if (strcasecmp(path_basename(spec), "Out") == 0)
    return OUT;
  if (strcasecmp(path_basename(spec), "Trash") == 0)
    return TRASH;
  if (strcasecmp(path_basename(spec), "Junk") == 0)
    return JUNK;
  if (IsSpool(spec)) {
    if (strcasecmp(path_basename(spec), "In.temp") == 0)
      return IN_TEMP;
    if (strcasecmp(path_basename(spec), "Out.temp") == 0)
      return OUT_TEMP;
  }
  return 0;
}

/* Callback for WantRebuildTOC alert dialog */
typedef struct { int choice; GMainLoop *loop; } TocAlertData;
static void toc_alert_cb(GObject *src, GAsyncResult *res, gpointer data) {
  TocAlertData *d = data;
  d->choice = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(src), res, NULL);
  if (d->choice < 0) d->choice = 0;
  g_main_loop_quit(d->loop);
}

/************************************************************************
 * WantRebuildTOC - ask user if they want to rebuild a corrupt TOC
 * Returns: 0 = use old, 1 = rebuild, 2 = cancel
 ************************************************************************/
short WantRebuildTOC(const char *boxName, int why, bool isIMAP) {
  (void)isIMAP;

  if (!PrefIsSet(PREF_TOC_REBUILD_ALERTS))
    return 1; /* auto-rebuild */

  const char *msg = (why == euMismatchTOC)
    ? "The table of contents for \"%s\" does not match the mailbox.\nWould you like to rebuild it?"
    : "The table of contents for \"%s\" appears to be corrupt.\nWould you like to rebuild it?";
  char *text = g_strdup_printf(msg, boxName);

  const char *buttons_mismatch[] = { "Rebuild", "Use Old", "Cancel", NULL };
  const char *buttons_corrupt[]  = { "Rebuild", "Cancel", NULL };
  const char **buttons = (why == euMismatchTOC) ? buttons_mismatch : buttons_corrupt;

  GtkAlertDialog *alert = gtk_alert_dialog_new("%s", text);
  gtk_alert_dialog_set_buttons(alert, buttons);
  gtk_alert_dialog_set_cancel_button(alert, (why == euMismatchTOC) ? 2 : 1);
  gtk_alert_dialog_set_default_button(alert, 0);
  g_free(text);

  /* Block via nested main loop until user responds */
  TocAlertData ctx;
  ctx.choice = 0; /* default: rebuild */
  ctx.loop = g_main_loop_new(NULL, FALSE);

  extern GtkWidget *get_main_window(void);
  GtkWindow *parent = get_main_window() ? GTK_WINDOW(get_main_window()) : NULL;

  gtk_alert_dialog_choose(alert, parent, NULL, toc_alert_cb, &ctx);

  g_main_loop_run(ctx.loop);
  g_main_loop_unref(ctx.loop);
  g_object_unref(alert);

  int choice = ctx.choice;
  if (why == euMismatchTOC) {
    /* 0=Rebuild, 1=Use Old, 2=Cancel */
    return choice;
  } else {
    /* 0=Rebuild, 1=Cancel */
    return (choice == 0) ? 1 : 2;
  }
}

/************************************************************************
 * FixErrantTOC - we have determined that something is wrong with a TOC
 * Ported from Mac: WantRebuildTOC dialog + RebuildTOC
 ************************************************************************/
static TOCType * FixErrantTOC(char * spec, TOCType * tocH, short why) {
  /* Temp tocs: automatically rebuild */
  short which = GetMailboxType(spec);
  if (which == IN_TEMP || which == OUT_TEMP)
    return RebuildTOC(spec, tocH, false, true);

  short result = WantRebuildTOC(path_basename(spec), why,
                                tocH && tocH->imapTOC != NULL);

  switch (result) {
  case 0: /* use old */
    if (tocH) {
      TOCSetDirty(tocH, true);
      tocH->reallyDirty = true;
    }
    return tocH;
  case 1: /* rebuild */
    return RebuildTOC(spec, tocH, false, false);
  case 2: /* cancel */
  default:
    g_free(tocH);
    return NULL;
  }
}

/************************************************************************
 * GetTOCK - grab the K counts for a mailbox
 * Ported from Mac: AFSpGetHFileInfo → g_stat()
 ************************************************************************/
static short GetTOCK(TOCType * tocH, unsigned long *usedK, unsigned long *totalK) {
  if (!tocH) {
    *usedK = *totalK = 0;
    return -1;
  }

  long used = 0;
  for (int i = 0; i < tocH->count; i++) {
    if (tocH->sums[i].offset >= 0)
      used += tocH->sums[i].length;
  }
  *usedK = (unsigned long)(used / 1024);

  GStatBuf st;
  if (g_stat(tocH->path, &st) == 0)
    *totalK = (unsigned long)(st.st_size / 1024);
  else
    *totalK = 0;

  return 0;
}

/************************************************************************
 * CheckStringLen - make sure a C string field is not too long
 * Ported from Mac: Pascal string length byte → C strlen
 ************************************************************************/
static void CheckStringLen(char *s, int maxLen, int fillLen) {
  int len = (int)strlen(s);
  if (len > fillLen) {
    /* Total junk; destroy */
    memset(s, 0, fillLen);
  } else if (len > maxLen) {
    /* Too long, truncate */
    memset(s + maxLen, 0, fillLen - maxLen);
  }
}

/************************************************************************
 * CleanseTOC - free a newly-read toc of vestiges of its past life
 * Ported from Mac: version migration, serial numbers, string validation
 ************************************************************************/
void CleanseTOC(TOCType * tocH) {
  if (!tocH)
    return;

  long vers = tocH->majorVersion;
  long minor = tocH->minorVersion;
  bool needSerialNum = vers < 1 || (vers == 1 && minor < 2);

  if (needSerialNum)
    tocH->nextSerialNum = 1;

  /* Clear runtime-only fields, validate strings, assign serial numbers */
  for (int i = 0; i < tocH->count; i++) {
    MSumPtr sum = &tocH->sums[i];
    sum->mesgErrH = NULL;
    sum->messH = NULL;
    sum->selected = false;
    sum->cache = NULL;

    /* Version-based migration */
    if (vers < 1) {
      sum->uidHash = sum->msgIdHash = kNeverHashed;
    }
    if (minor < 1)
      sum->msgIdHash = kNeverHashed;

    /* Make sure older versions don't have strings that are too long */
    CheckStringLen(sum->from, sizeof(sum->from) - 1, 64);
    CheckStringLen(sum->subj, sizeof(sum->subj) - 1, 64);

    if (minor < TOC_MINOR_NO_DATE_STRING) {
      sum->spamScore = -1;
      sum->spamBecause = 0;
      sum->spareShort = 0;
      sum->arrivalSeconds = 0;
    }

    if (vers == 1 && minor < TOC_MINOR_NO_UNNECESSARY_UTF8)
      RemoveUTF8FromSum(sum);

    if (!sum->arrivalSeconds)
      sum->arrivalSeconds = sum->seconds;

    if (needSerialNum)
      sum->serialNum = tocH->nextSerialNum++;
  }

  tocH->majorVersion = CURRENT_TOC_VERS;
  tocH->minorVersion = CURRENT_TOC_MINOR;
  tocH->virtualTOC = false;
  tocH->beingWritten = 0;
  tocH->analScanned = false;

  /* Junk processing */
  if (tocH->which == JUNK)
    JunkTOCCleanse(tocH);
}

/************************************************************************
 * IsTOCValid - is the toc on the list of currently-opened toc's
 ************************************************************************/
TOCType * IsTOCValid(TOCType * testTOC) {
  for (TOCType * tocH = TOCList; tocH; tocH = tocH->next)
    if (tocH == testTOC)
      return tocH;
  return NULL;
}

/************************************************************************
 * RedoTOC - refresh a TOC's display
 * Returns true if work was done
 ************************************************************************/
bool RedoTOC(TOCType * tocH) {
  if (!tocH)
    return false;

  /* Check if any summaries need redoing */
  if (tocH->needRedo < 0)
    return false;

  /* In the GTK port, the tree view model handles display updates.
     Mark that we've processed the redo request. */
  tocH->needRedo = -1;

  /* If the window is visible, queue a redraw */
  if (tocH->win) {
    GtkWidget *wp = GetMyWindowWindowPtr(tocH->win);
    if (wp && IsWindowVisible(wp))
      gtk_widget_queue_draw(wp);
  }

  return true;
}

/************************************************************************
 * RedoTOCs - make sure right info is displayed in all TOC's
 * Ported from Mac: traverses TOCList, calls RedoTOC on visible windows
 ************************************************************************/
/************************************************************************
 * toc_free - free a TOC and all its owned resources
 ************************************************************************/
void toc_free(TOCType *toc) {
  if (!toc) return;
  /* Free virtual mailbox spec list */
  if (toc->mailbox.virtualMB.specList) {
    for (long i = 0; i < toc->mailbox.virtualMB.specListCount; i++)
      free(toc->mailbox.virtualMB.specList[i]);
    free(toc->mailbox.virtualMB.specList);
  }
  /* Free cached message handles */
  for (int i = 0; i < toc->count; i++) {
    if (toc->sums[i].cache) free(toc->sums[i].cache);
    if (toc->sums[i].mesgErrH) free(toc->sums[i].mesgErrH);
  }
  free(toc);
}

/************************************************************************
 * toc_get_message - get a message summary by index
 ************************************************************************/
MessageSummary *toc_get_message(TOCType *toc, uint32_t index) {
  if (!toc || index >= (uint32_t)toc->count) return NULL;
  return &toc->sums[index];
}

/************************************************************************
 * toc_get_message_count - return number of messages in TOC
 ************************************************************************/
uint32_t toc_get_message_count(TOCType *toc) {
  if (!toc) return 0;
  return (uint32_t)toc->count;
}

/************************************************************************
 * toc_get_summaries - return pointer to summaries array
 ************************************************************************/
MessageSummary *toc_get_summaries(TOCType *toc, int *count) {
  if (!toc) { if (count) *count = 0; return NULL; }
  if (count) *count = toc->count;
  return toc->sums;
}

/************************************************************************
 * toc_get_unread_count - return number of unread messages
 ************************************************************************/
int toc_get_unread_count(TOCType *toc) {
  if (!toc) return 0;
  return (int)toc->unread;
}

/************************************************************************
 * toc_load - load a TOC from a mailbox path (delegates to CheckTOC)
 ************************************************************************/
TOCType *toc_load(const char *path) {
  if (!path || !path[0]) return NULL;

  /* Try path-based lookup first (path-based) */
  TOCType *tocH = TOCByPath(path);
  if (tocH) return tocH;

  /* Fall back to CheckTOC which loads/builds the TOC */
  char spec[PATH_MAX];
  memset(&spec, 0, sizeof(spec));
  g_strlcpy(spec, path, sizeof(spec));
  return CheckTOC(spec);
}

/************************************************************************
 * toc_save - save a TOC to disk (delegates to WriteTOC)
 ************************************************************************/
int toc_save(TOCType *toc) {
  if (!toc) return -1;
  return WriteTOC(toc);
}

void RedoTOCs(void) {
  TOCType * tocH;

  for (tocH = TOCList; tocH; tocH = tocH->next) {
    if (tocH->win &&
        IsWindowVisible(GetMyWindowWindowPtr(tocH->win))) {
      if (RedoTOC(tocH)) {
        /* We did some work; restart from the top to avoid
           race conditions with threading */
        tocH = TOCList;
        if (!tocH)
          break;
        continue;
      }
    }
  }
}
