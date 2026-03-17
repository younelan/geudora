#define FILEUTIL_C
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

/* RgnHandle and other basic types are in legacy_shim.h */

#include "../include/fileutil.h"
#include "../include/StringDefs.h"
#include "../include/StringUtil.h"
#include "../include/mailbox.h"
#include "../include/progress.h"
#include "../include/util.h"
#include <assert.h>
#include <stdarg.h>

extern bool HaveOSX(void);

extern char *ComposeString(char *dst, const char *fmt, ...);
extern void AlertStr(short alertID, short type, const char *message);
#define Note 1
#define OK_ALRT 1001

extern FSSpec SettingsSpec;

#define MINI_MASK 0
#define SAVEAS_DLOG 1026
#define SAVEAS_NAV_DITL 1077
extern bool FakeTabs;

#ifndef BMD
#define BMD(s, d, l) memmove(d, s, l)
#endif
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <malloc/malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __has_include
#if __has_include(<gio/gio.h>)
#include <gio/gio.h>
#endif
#endif

/* CommandPeriod: thread-local cancel flag, defined in threading.h.
   fileutil.c can't include threading.h (conflicts with local stubs),
   so bring in the minimal declarations needed. */
struct threadGlobals_;
typedef struct threadGlobals_ *threadGlobalsPtr;
extern _Thread_local threadGlobalsPtr CurThreadGlobals;
/* Access the cancel flag via an inline helper to avoid needing the full
   struct definition here. Implemented in threading.c. */
extern short *_CommandPeriodPtr(void);
void MyCloseResFile(short refN);
#ifndef CommandPeriod
#define CommandPeriod (*_CommandPeriodPtr())
#endif

#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif

#define FILE_NUM 13
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Prototypes */


/**********************************************************************
 * various useful functions related to the filesystem
 **********************************************************************/

// Mac compatibility macros
#ifndef nil
#define nil NULL
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef REAL_BIG
#define REAL_BIG 32766
#endif
#ifndef ASSERT
#define ASSERT(expr) assert(expr)
#endif
#ifndef PREF_CORVAIR
#define PREF_CORVAIR 0
#endif
#ifndef fInited
#define fInited 0x0100 // Mac Finder flag: file has been initialized
#endif

// Mac void *lock/unlock stubs - no-ops in standard C
#ifndef LDRef
#define h (*(h))
#endif
#ifndef UL
#define  ((void)0)
#endif
#ifndef GetHandleSize_
#define GetHandleSize_(h) GetHandleSize(h)
#endif

#define FILL(pb, name, vRef, dirId)

/* Forward declarations */
static int GenerateUniqueName(short volume, long *startSeed, long dir1,
                                long dir2, char *uniqueName);
int FSpExchangeFilesCompat(const FSSpec *source, const FSSpec *dest);

int DirIterate(const FSSpec *dir, void *data,
                 bool (*callback)(DirIterateInfo *info)) {
  DIR *dp;
  struct dirent *entry;
  int err = 0;

  if (!(dp = opendir(dir->path))) {
    return ioErr; // Or a more specific error based on errno
  }

  while ((entry = readdir(dp)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    DirIterateInfo info;
    memset(&info, 0, sizeof(DirIterateInfo));
    info.spec.vRefNum = dir->vRefNum;
    info.spec.parID = dir->parID; // Parent ID is the directory being iterated
    g_strlcpy(info.spec.name, entry->d_name, sizeof(info.spec.name));
    snprintf(info.spec.path, sizeof(info.spec.path), "%s/%s", dir->path,
             entry->d_name);

    struct stat st;
    if (stat(info.spec.path, &st) == 0) {
      info.isDir = S_ISDIR(st.st_mode);
      info.isSymLink = S_ISLNK(st.st_mode);
      info.size = st.st_size;
      info.createDate = st.st_ctime;
      info.modifyDate = st.st_mtime;
    } else {
      // void *stat error if necessary, maybe skip this entry or log
      continue;
    }

    info.data = data;
    if (!callback(&info)) {
      // Callback returned false, stop iteration
      break;
    }
  }

  closedir(dp);
  return err;
}

void FileIDHack(void); // JDB 980720 Hack to work around apple's fileID bug

/**********************************************************************
 * Stubs and Pascal String Utilities
 **********************************************************************/

/**********************************************************************
 * Stubs and Modern String Utilities
 **********************************************************************/



/* FS API Stubs - mostly removed or simplified */
#ifndef fnfErr
#define fnfErr (-43)
#endif
#ifndef ioErr
#define ioErr (-36)
#endif
#ifndef paramErr
#define paramErr (-50)
#endif
#ifndef dupFNErr
#define dupFNErr (-48)
#endif
#ifndef SyncRW
#define SyncRW 0
#endif
#ifndef afpAccessDenied
#define afpAccessDenied (-5000)
#endif

#ifndef pascal
#define pascal
#endif

/* spec_for - build an FSSpec from a directory path and filename.
 * Returns 0 if the resulting path exists, fnfErr if not. */
short spec_for(const char *dir, const char *name, FSSpec * spec) {
  if (!spec)
    return paramErr;
  memset(spec, 0, sizeof(FSSpec));
  if (name && name[0])
    g_strlcpy(spec->name, name, sizeof(spec->name));
  if (!name || !name[0]) {
    if (dir && dir[0])
      g_strlcpy(spec->path, dir, sizeof(spec->path));
  } else if (!dir || !dir[0] || name[0] == '/') {
    g_strlcpy(spec->path, name, sizeof(spec->path));
  } else {
    snprintf(spec->path, sizeof(spec->path), "%s/%s", dir, name);
  }
  return access(spec->path, F_OK) == 0 ? 0 : fnfErr;
}

/* spec_make - like spec_for but doesn't hit the filesystem */
void spec_make(const char *dir, const char *name, FSSpec * spec) {
  if (!spec)
    return;
  memset(spec, 0, sizeof(FSSpec));
  if (name && name[0])
    g_strlcpy(spec->name, name, sizeof(spec->name));
  if (!name || !name[0]) {
    if (dir && dir[0])
      g_strlcpy(spec->path, dir, sizeof(spec->path));
  } else if (!dir || !dir[0] || name[0] == '/') {
    g_strlcpy(spec->path, name, sizeof(spec->path));
  } else {
    snprintf(spec->path, sizeof(spec->path), "%s/%s", dir, name);
  }
}

/* spec_parent - get the parent directory of a spec's path into buf */
const char *spec_parent(FSSpec * spec, char *buf, size_t bufsz) {
  if (spec->path[0]) {
    g_strlcpy(buf, spec->path, bufsz);
    char *slash = strrchr(buf, '/');
    if (slash && slash != buf)
      *slash = '\0';
    else if (slash == buf)
      buf[1] = '\0';
    return buf;
  }
  buf[0] = '\0';
  return buf;
}
short MyFSpCreateResFile(FSSpec * spec, uint32_t creator, uint32_t type,
                       ScriptCode script) {
  // POSIX doesn't have resource forks. Just create the data fork.
  return (short)MyFSpCreate(spec, creator, type, script);
}

int MyFSpCreate(FSSpec * spec, uint32_t creator, uint32_t fileType,
              ScriptCode script) {
  if (!spec)
    return paramErr;
  int fd = creat(spec->path, 0644);
  if (fd < 0) {
    if (errno == EEXIST)
      return 0; // Or dupFNErr if we want to be strict
    return ioErr;
  }
  close(fd);
  return 0;
}
int MemError() { return 0; }

short ResError() { return 0; }

void AddResource(void *h, ResType type, short id, const char *name) {
  // Stub - resource forks don't exist on Unix
  // In a full implementation, this would store resources in a separate file
}

int LowLevelFSpOpenDF(FSSpec *spec, short mode, short *refN) {
  const char *path = spec->path;
  int flags = O_RDONLY;
  if (mode == fsWrPerm)
    flags = O_WRONLY;
  else if (mode == fsRdWrPerm)
    flags = O_RDWR;

  int fd = open(path, flags);
  if (fd < 0)
    return fnfErr;
  *refN = (short)fd;
  return 0;
}

int LowLevelFSpOpenRF(const char *path, short permission, short *refNum) {
  if (!path || !refNum)
    return paramErr;

  int flags = O_RDONLY;
  if (permission == fsWrPerm)
    flags = O_WRONLY;
  else if (permission == fsRdWrPerm)
    flags = O_RDWR;

  int fd = open(path, flags);
  if (fd < 0)
    return fnfErr;
  *refNum = (short)fd;
  return 0;
}

int LowLevelFSpGetFInfo(FSSpec *spec, FInfo *fndrInfo) {
  struct stat st;

  if (!spec || !fndrInfo)
    return paramErr;

  memset(fndrInfo, 0, sizeof(FInfo));

  if (lstat(spec->path, &st) < 0)
    return fnfErr;

  /* Basic info */
  fndrInfo->fdType = 0;
  fndrInfo->fdCreator = 0;
  fndrInfo->fdFlags = 0;
  if (S_ISLNK(st.st_mode))
    fndrInfo->fdFlags |= kIsAlias;
  if (S_ISDIR(st.st_mode))
    fndrInfo->fdFlags |= ioDirMask;

  /* size/location fields reused via HFileInfo when requested */
  return 0;
}

int LowLevelFSpSetFInfo(FSSpec *spec, FInfo *fndrInfo) {
  /* Minimal portable implementation */
  (void)spec;
  (void)fndrInfo;
  return 0;
}
int LowLevelFSpDelete(FSSpec *spec) {
  if (unlink(spec->path) == 0)
    return 0;
  return ioErr;
}

short GetEOF(short refNum, long *logEOF) {
  struct stat st;
  if (fstat(refNum, &st) < 0)
    return ioErr;
  *logEOF = (long)st.st_size;
  return 0;
}
short SetEOF(short refNum, long logEOF) {
  if (ftruncate(refNum, (off_t)logEOF) < 0)
    return ioErr;
  return 0;
}
short SetFPos(short refNum, short posMode, long posOffset) {
  int whence = SEEK_SET;
  if (posMode == fsFromLEOF)
    whence = SEEK_END;
  else if (posMode == fsFromMark)
    whence = SEEK_CUR;

  if (lseek(refNum, (off_t)posOffset, whence) < 0)
    return ioErr;
  return 0;
}
short GetFPos(short refNum, long *filePos) {
  off_t pos = lseek(refNum, 0, SEEK_CUR);
  if (pos < 0)
    return ioErr;
  *filePos = (long)pos;
  return 0;
}
/* No-op resource management for portability */
void MyCloseResFile(short refN) {
  ASSERT(refN != SettingsRefN);
}
extern int GetNumBackgroundThreads(void);
extern void MiniEventsLo(short mask, bool background);

short ARead(short refNum, long *count, unsigned char *buffer) {
  ssize_t bytes = read(refNum, buffer, *count);
  if (bytes < 0) {
    *count = 0;
    return ioErr;
  }
  *count = (long)bytes;
  return 0;
}
short AWrite(short refNum, long *count, unsigned char *buffer) {
  /* Normalize line endings when writing: convert lone CR (\r / 0x0D)
     to CRLF (\r\n) to produce canonical mailbox files. If the
     buffer already contains CRLF sequences, leave them intact. This
     expands size at most by the number of CR bytes. */
  if (!buffer || *count <= 0) {
    return 0;
  }

  long inlen = *count;
  /* Fast path: scan for any CR that isn't followed by LF in the same
     buffer. If none found, write directly. */
  bool need_expand = false;
  for (long i = 0; i < inlen; ++i) {
    if (buffer[i] == '\r') {
      if (i + 1 >= inlen || buffer[i + 1] != '\n') {
        need_expand = true;
        break;
      }
    }
  }

  if (!need_expand) {
    ssize_t bytes = write(refNum, buffer, inlen);
    if (bytes < 0) {
      *count = 0;
      return ioErr;
    }
    *count = (long)bytes;
    return 0;
  }

  /* Need to expand: allocate a temporary buffer sized conservatively
     (in worst case every byte could be CR and need an extra LF). */
  long max_out = inlen * 2;
  unsigned char *out = malloc((size_t)max_out);
  if (!out) {
    *count = 0;
    return ioErr;
  }

  long oi = 0;
  for (long i = 0; i < inlen; ++i) {
    unsigned char c = buffer[i];
    if (c == '\r') {
      out[oi++] = '\r';
      if (i + 1 < inlen && buffer[i + 1] == '\n') {
        /* already CRLF; copy LF and skip next char */
        out[oi++] = '\n';
        ++i;
      } else {
        /* insert LF after CR */
        out[oi++] = '\n';
      }
    } else {
      out[oi++] = c;
    }
  }

  ssize_t written = write(refNum, out, oi);
  free(out);
  if (written < 0) {
    *count = 0;
    return ioErr;
  }
  *count = (long)written;
  return 0;
}
short NCWrite(short refNum, long *count, unsigned char *buffer) {
  return 0;
}
/* Portable Pascal-string write: compute length and use AWrite. */
short FSWriteP(short refN, unsigned char *pString) {
  if (!pString) return 0;
  long count = (long)strlen((const char *)pString);
  return AWrite(refN, &count, pString);
}

/* No-op FlushVol for POSIX (flush volumes not applicable). */
short FlushVol(unsigned char *name, short vRefNum) {
  (void)name; (void)vRefNum;
  return 0;
}

/* PBCreate/PBResolve stubs for systems without FileIDRef APIs. Use the
   same prototype as declared in the headers. */
short PBCreateFileIDRefSync(HParmBlkPtr pb) { (void)pb; return 0; }
short PBResolveFileIDRefSync(HParmBlkPtr pb) { (void)pb; return 0; }
char *FileUtilGetRString(char *name, short id) {
  if (name)
    *name = 0;
  return name;
}
#define GetRString FileUtilGetRString

/* Extra CtoPCpy removed */
/* Error handling stubs - actual implementations in error_handlers.c or
 * similar
 */
void DieWithError(short errorId, int err) {
  fprintf(stderr, "Fatal Error %d: %d\n", errorId, err);
  exit(1);
}

int FileSystemError(short errorId, const char *name, int err) {
  const char *cName;
  if (name)
    cName = name;
  else
    cName = "unknown";
  fprintf(stderr, "File System Error %d on %s: %d\n", errorId, cName, err);
  return err;
}


int MyFSpRename(FSSpec * spec, const char *newName) {
  char parent[1024];
  char newPath[1024];

  if (!spec || !newName || !newName[0])
    return paramErr;

  /* determine parent directory */
  spec_parent(spec, parent, sizeof(parent));
  if (!parent[0])
    return paramErr;

  snprintf(newPath, sizeof(newPath), "%s/%s", parent, newName);

  if (rename(spec->path, newPath) != 0) {
    return ioErr;
  }

  /* update spec to reflect new name/path */
  g_strlcpy(spec->name, newName, sizeof(spec->name));
  g_strlcpy(spec->path, newPath, sizeof(spec->path));
  return 0;
}
/**********************************************************************
 * CycleBalls - animate progress indicator and yield to the event loop
 *
 * Mac original: spun a beach-ball cursor every 10 ticks (~167ms) and
 * called MiniEventsLo to let other threads run.
 *
 * GTK port: process pending GLib/GTK events so the UI stays responsive
 * during long-running filter/download loops. Throttled to avoid
 * burning CPU on event iteration.
 **********************************************************************/
void CycleBalls(void) {
  static uint32_t lastTick = 0;
  uint32_t now = TickCount();

  if (now > lastTick + 10) {
    lastTick = now;
    /* Process pending GTK events to keep UI responsive */
    GMainContext *ctx = g_main_context_default();
    while (g_main_context_pending(ctx))
      g_main_context_iteration(ctx, FALSE);
  }
}
void ModernProgress(short id, short percent, void *p1, void *p2, void *p3) {}

/* utl_RFSanity is implemented in utl.c */
int PBSetCatInfoSync(CInfoPBPtr pb) { return 0; }
void *MyGet1IndResource(OSType type, short index) { (void)type; (void)index; return NULL; }
void *MyGetIndResource(OSType type, short index) { (void)type; (void)index; return NULL; }
int ResolveAliasFile(FSSpec * spec, bool resolveAliasChains,
                       bool *targetIsFolder, bool *wasAliased) {
  /* On POSIX there's no Finder alias; emulate by resolving symlinks */
  if (!spec)
    return paramErr;

  if (targetIsFolder)
    *targetIsFolder = false;
  if (wasAliased)
    *wasAliased = false;

  /* if path is empty, nothing to do */
  if (!spec->path[0])
    return fnfErr;

  char resolved[PATH_MAX];
  if (realpath(spec->path, resolved) == NULL) {
    /* if realpath fails, leave spec alone but indicate file-not-found */
    return fnfErr;
  }

  /* Update spec->path to resolved path */
  g_strlcpy(spec->path, resolved, sizeof(spec->path));
  /* update name to basename */
  const char *base = strrchr(resolved, '/');
  if (base && base[1])
    g_strlcpy(spec->name, base + 1, sizeof(spec->name));

  struct stat st;
  if (stat(spec->path, &st) == 0) {
    if (targetIsFolder)
      *targetIsFolder = S_ISDIR(st.st_mode);
  }

  /* If original path contained a symlink component, consider it aliased */
  if (wasAliased) {
    if (strcmp(spec->path, resolved) != 0)
      *wasAliased = true;
  }

  return 0;
}
int PBCatMoveSync(CMovePBPtr pb) { return 0; }
int PBHGetFInfoSync(HParmBlkPtr pb) { return 0; }
int PBHSetFInfoSync(HParmBlkPtr pb) { return 0; }
bool GetFolderNav(char *name, short *volume, long *folder) { return false; }
int Gestalt(OSType selector, long *response) { return 0; }
int MyFSpExchangeFiles(FSSpec * source, FSSpec * dest) {
  /* Swap two files by renaming through a temporary name in dest directory. */
  char tempTpl[PATH_MAX];
  char destParent[PATH_MAX];
  char destTemp[PATH_MAX];

  if (!source || !dest) return paramErr;

  /* dest parent */
  spec_parent(dest, destParent, sizeof(destParent));
  if (!destParent[0]) return paramErr;

  snprintf(tempTpl, sizeof(tempTpl), "%s/.exchange-XXXXXX", destParent);
  int fd = mkstemp(tempTpl);
  if (fd < 0) return ioErr;
  close(fd);
  /* tempTpl now contains a unique filename; use it as temporary holder */
  strncpy(destTemp, tempTpl, sizeof(destTemp));

  /* move dest -> temp */
  if (rename(dest->path, destTemp) != 0) {
    unlink(destTemp);
    return ioErr;
  }

  /* move source -> dest */
  if (rename(source->path, dest->path) != 0) {
    /* try to roll back */
    rename(destTemp, dest->path);
    unlink(destTemp);
    return ioErr;
  }

  /* move temp -> source */
  if (rename(destTemp, source->path) != 0) {
    /* catastrophic; try best-effort rollback */
    rename(dest->path, source->path);
    rename(destTemp, dest->path);
    unlink(destTemp);
    return ioErr;
  }

  return 0;
}
int FSpDirCreate(FSSpec * spec, ScriptCode script, long *dirID) {
  if (!spec)
    return paramErr;
  if (mkdir(spec->path, 0755) != 0) {
    if (errno == EEXIST)
      return 0;
    return ioErr;
  }
  if (dirID)
    *dirID = 0;
  return 0;
}
int DirCreate(short vRefNum, long parentDirID, const char *directoryName, long *createdDirID) { return 0; }
/* Redundant PBSetCatInfoSync removed */
// ByteProgress stub removed - declared in progress.h with different signature
extern bool HaveOSX(void);
short GetMBarHeight(void) { return 20; }
extern long GetRLong(int index);
bool MommyMommy(short id, void *p) { return true; }
bool UseNavServices(void) { return false; }
int SFPutOpenNav(FSSpec * spec, uint32_t creator, uint32_t type, short *refN,
                 short ditlID, uint32_t *script, FSSpec * defaultSpec,
                 const char *windowTitle, const char *message) {
  return 0;
}
void WhackFinder(FSSpec * spec) {}
int SniffAndConvertHandleToRoman(void ***h) { (void)h; return 0; }
OSType DefaultCreator(void) { return 'EUDR'; }
short GetResFileAttrs(short refNum) { return 0; }
void UpdateResFile(short refNum) {}
extern void TransLitRes(char *string, long len, short resId);
short PBGetFCBInfo(FCBInfoPBPtr pb, bool async) { return 0; }
short PBHRenameSync(HParmBlkPtr pb) { return 0; }
short FSRead(short refNum, long *count, void *buffer) {
  return ARead(refNum, count, (unsigned char *)buffer);
}
short FSWrite(short refNum, long *count, const void *buffer) {
  return AWrite(refNum, count, (unsigned char *)buffer);
}
short PBWriteAsync(IOParam *pb) { return 0; }
#define kTextEncodingUnicodeDefault 0
#define kioFlAttribLockedMask 0x01
/* PDF_QUOTE_EXTENSION_UNQUOTE already defined in StringDefs.h */

typedef void *TextToUnicodeInfo;
typedef unsigned long ByteCount;
typedef unsigned long OptionBits;

void MyNumToString(long n, char *s);

int FSRenameUnicode(FSRef *ref, int len, const void *name, int encoding,
                      FSRef *newRef);


void MyNumToString(long n, char *s) {
  if (s)
    sprintf(s, "%ld", n);
}
int MyInvalidateFolderDescriptorCache(short vRef, long dirID) {
  return 0;
}

int CreateTextToUnicodeInfoByEncoding(OSType encoding,
                                        TextToUnicodeInfo *info) {
  return 0;
}
int ConvertFromPStringToUnicode(TextToUnicodeInfo info, const char *pStr,
                                  ByteCount maxLen, ByteCount *len,
                                  UniChar *dst) {
  *len = 0;
  return 0;
}
int DisposeTextToUnicodeInfo(TextToUnicodeInfo *info) { return 0; }

int CreateUnicodeToTextInfoByEncoding(OSType encoding,
                                        UnicodeToTextInfo *info) {
  return 0;
}
int ConvertFromUnicodeToText(UnicodeToTextInfo info, long len,
                               const void *ptr, OptionBits options,
                               OptionBits mask, void *fallback,
                               void *fallbackInfo, void *fallbackBuffer,
                               long bufferLen, long *charsUsed, long *charsOut,
                               void *outBuffer) {
  return 0;
}
int DisposeUnicodeToTextInfo(UnicodeToTextInfo *info) { return 0; }
int FSpMakeFSRef(FSSpec * spec, FSRef *ref) { return 0; }
int FSGetCatalogInfo(FSRef *ref, long bitmap, void *info,
                       HFSUniStr255 *outName, void *fsSpec, void *parentRef) {
  return 0;
}

int PBMakeFSRefSync(FileParam *pb) { return 0; }

OSType CreateTextEncoding(OSType encoding, OSType representation,
                          OSType variant) {
  return 0;
}
#undef HaveTheDiseaseCalledOSX
bool HaveTheDiseaseCalledOSX(void) { return HaveOSX(); }
extern bool StringSame(const char *s1, const char *s2);
int PBDTSetCommentSync(DTPBRec *pb) { return 0; }

bool bHasFileIDs = false;

extern bool PrefIsSet(short prefId);
extern void ThirdCenterRectIn(void *r, void *in);
void GetQDGlobalsScreenBits(void *bits) {}
int PtrToHand(const void *srcPtr, void **dstHndl, size_t size) {
  if (!srcPtr || !dstHndl || size == 0) return -1;
  void *h = malloc(size);
  if (!h) return -1;
  memcpy(h, srcPtr, size);
  *dstHndl = h;
  return 0;
}

size_t InlineGetHandleSize(void *h) {
  if (!h) return 0;
  return malloc_size(h);
}

void HLock(void *h) { (void)h; }
void HUnlock(void *h) { (void)h; }
size_t GetHandleSize(void *h) { return InlineGetHandleSize(h); }
void *NuPtr(size_t size) { return malloc(size); }

void *buf_append(void *buf, const void *data, size_t n) {
  if (!data || n == 0) return buf;
  size_t old = buf ? InlineGetHandleSize(buf) : 0;
  void *p = realloc(buf, old + n);
  if (!p) return NULL;
  memcpy((char *)p + old, data, n);
  return p;
}

void *buf_concat(void *dst, const void *src) {
  if (!src) return dst;
  size_t n = InlineGetHandleSize(src);
  return n ? buf_append(dst, src, n) : dst;
}


void BlockMoveData(const void *src, void *dest, size_t size) {
  if (src && dest && size > 0) {
    memmove(dest, src, size);
  }
}

/**********************************************************************
 * GetMyVR - get a volume ref number
 **********************************************************************/
short GetMyVR(const char *name) {
  // Volume references are Mac-specific, return 0 for POSIX
  return 0;
}

/************************************************************************
 * ParentSpec - get the FSSpec of a parent
 ************************************************************************/
int ParentSpec(FSSpec * child, FSSpec * parent) {
  char parentPath[1024];
  char *lastSlash;

  strncpy(parentPath, child->path, sizeof(parentPath));
  lastSlash = strrchr(parentPath, '/');
  if (lastSlash && lastSlash != parentPath) {
    *lastSlash = '\0';
    strncpy(parent->path, parentPath, sizeof(parent->path));
    parent->vRefNum = child->vRefNum;
    parent->parID = 0;
    *parent->name = 0;
    return 0;
  }
  return ioErr;
}

/**********************************************************************
 * get a name, given a vRefNum
 **********************************************************************/
short GetDirName(char *volName, short vRef, long dirId, char *name) {
  return 0;
}

/**********************************************************************
 * get a volume name, given a vRefNum
 **********************************************************************/
char *GetMyVolName(short refNum, char *name) { return 0; }

/************************************************************************
 * IndexVRef - return vref's by index
 ************************************************************************/
int IndexVRef(short index, short *vRef) {
  // Volume enumeration is Mac-specific
  // Return fnfErr to indicate no more volumes
  return fnfErr;
}

/************************************************************************
 * BlessedVRef - find the vref of the blessed folder
 ************************************************************************/
short BlessedVRef(void) {
  int vRefNum;
  long dirID;

  FindFolder(kOnSystemDisk, kSystemFolderType, false, &vRefNum, &dirID);
  return (short)vRefNum;
}

/**********************************************************************
 * MakeResFile - create a resource file in a given directory
 **********************************************************************/
int MakeResFile(const char *name, const char *dir, long creator,
                long type) {
  int err;
  FSSpec spec;

  spec_for(dir, name, &spec);
  MyFSpCreateResFile(&spec, creator, type, 0);
  err = ResError();
  // (jp) NetWare servers incorrectly report noMacDskErr when attempting to
  //			create a resource file (the signature bytes are
  // apparently wrong) 			We can create a file with both
  // forks, however.
  if (err == noMacDskErr) {
    int fd = creat(spec.path, 0644);
    if (fd < 0)
      err = ioErr;
    else {
      close(fd);
      err = 0;
    }
  }
  return (err == dupFNErr ? 0 : err);
}

/**********************************************************************
 * DirIterate - iterate over the files in a directory.
 **********************************************************************/
/* Legacy DirIterate removed */
/* Dangling code removed */

/**********************************************************************
 * FileIsInvisible - is this file invisible?
 **********************************************************************/
bool FileIsInvisible(CInfoPBRec *hfi) {
  return (hfi->hFileInfo.ioFlFndrInfo.fdFlags & fInvisible) ||
         (HaveOSX() && hfi->hFileInfo.ioNamePtr[0] &&
          hfi->hFileInfo.ioNamePtr[1] == '.');
}

/**********************************************************************
 * CopyFBytes - copy bytes from one file to another
 **********************************************************************/
int CopyFBytes(short fromRefN, long fromOffset, long length, short toRefN,
               long toOffset) {
  int err;
  unsigned char *buffer;
  long size;
  long count;
  long fromEnd = fromOffset + length;
  long toEnd;

  if ((err = GetEOF(toRefN, &toEnd)))
    return (err);

  if (toEnd < toOffset + length - 1)
    if ((err = SetEOF(toRefN, toOffset + length - 1)))
      return (err);
  toEnd = toOffset + length;

  size = MIN(OPTIMAL_BUFFER, length);
  if (size < 255) size = 255;
  buffer = malloc(size);
  if (!buffer)
    return (WarnUser(MEM_ERR, MemError()));

  do {
    CycleBalls();
    count = size > length ? length : size;

    if ((err = SetFPos(fromRefN, fsFromStart, fromEnd - count)))
      break;
    if ((err = ARead(fromRefN, &count, buffer)))
      break;

    if ((err = SetFPos(toRefN, fsFromStart, toEnd - count)))
      break;
    if ((err = NCWrite(toRefN, &count, buffer)))
      break;

    length -= count;
    fromEnd -= count;
    toEnd -= count;
  } while (length);
  free(buffer);
  return (err);
}

#define HNLSIZE 2048
/************************************************************************
 * HuntNewline - find a newline in a file (portable: use malloc/free)
 ************************************************************************/
int HuntNewline(short refN, long aroundSpot, long *newline, bool *realNl) {
  unsigned char *buffer = malloc(HNLSIZE);
  long spot, count;
  unsigned char *nl1, *nl2, *end;
  unsigned char *aSpot;
  short err;

  if (!buffer)
    return (WarnUser(MEM_ERR, MemError()));

  /* read in a buffer containing aSpot */
  spot = MAX(0, aroundSpot - HNLSIZE / 2);
  count = HNLSIZE;

  if ((err = SetFPos(refN, fsFromStart, spot))) {
    FileSystemError(READ_MBOX, "", err);
    goto done;
  }

  err = ARead(refN, &count, buffer);
  if (err == eofErr && count > 0)
    err = 0; /* ignore running off the end of the file as long as we got some bytes */
  if (err) {
    FileSystemError(READ_MBOX, "", err);
    goto done;
  }

  aSpot = buffer + (aroundSpot - spot);
  end = buffer + count;

  /* search both forwards and backwards for newlines */
  for (nl1 = aSpot; nl1 >= buffer; nl1--)
    if (*nl1 == '\015' || *nl1 == '\012')
      break;
  for (nl2 = aSpot; nl2 < end; nl2++)
    if (*nl2 == '\015' || *nl2 == '\012')
      break;

  /* take the closest newline to the desired spot */
  if (nl1 < buffer) {
    if (nl2 < end)
      aSpot = nl2;
  } else if (nl2 > end)
    aSpot = nl1;
  else
    aSpot = ((nl2 - aSpot) < (aSpot - nl1)) ? nl2 : nl1;

  *realNl = *aSpot == '\015';
  *newline = spot + (aSpot - buffer) + 1;

done:
  free(buffer);
  return (err);
}

/************************************************************************
 * TruncOpenFile - truncate an open file to a given spot
 ************************************************************************/
int TruncOpenFile(short refN, long spot) {
  short err;

  if ((err = SetFPos(refN, fsFromStart, spot)))
    return (err);
  return (SetEOF(refN, spot));
}

/************************************************************************
 * TruncAtMark - truncate an open file at the current spot
 ************************************************************************/
int TruncAtMark(short refN) {
  short err;
  long spot;

  if ((err = GetFPos(refN, &spot)))
    return (err);
  return (SetEOF(refN, spot));
}

/************************************************************************
 * StdFilespot - figure out where a stdfile dialog should go
 ************************************************************************/
void StdFileSpot(Point *where, short id) {
  Rect r, in;
  DialogTHndl dTempl;
  if ((dTempl = (DialogTHndl)GetResource_('ALRT', id)) ||
      (dTempl = (DialogTHndl)GetResource_('DLOG', id))) {
    BitMap screenBits;

    r.top = dTempl->boundsRect.top;
    r.left = dTempl->boundsRect.left;
    r.bottom = dTempl->boundsRect.bottom;
    r.right = dTempl->boundsRect.right;

    GetQDGlobalsScreenBits(&screenBits);

    in.top = screenBits.bounds.top;
    in.left = screenBits.bounds.left;
    in.bottom = screenBits.bounds.bottom;
    in.right = screenBits.bounds.right;

    in.top += GetMBarHeight();
    ThirdCenterRectIn(&r, &in);
    where->h = r.left;
    where->v = r.top;
  } else {
    *where = (const Point){100, 100};
  }
}

/************************************************************************
 * AFSHOpen - like FSOpen, but takes a dirId and permissions, too.
 ************************************************************************/
short AFSHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm) {
  if (!name)
    return paramErr;
  char path[1024];
  snprintf(path, sizeof(path), "./%s", name);

  int flags = O_RDONLY;
  if (perm == fsWrPerm)
    flags = O_WRONLY;
  else if (perm == fsRdWrPerm)
    flags = O_RDWR;

  int fd = open(path, flags);
  if (fd < 0)
    return fnfErr;
  *refN = (short)fd;
  return 0;
}

/************************************************************************
 * ARFHOpen - like RFOpen, but with dirId and permissions
 * Note: Resource forks are Mac-specific, return success but don't open anything
 ************************************************************************/
short ARFHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm) {
  *refN = -1;   // Invalid file descriptor to indicate no resource fork
  return 0; // Not an error in portable code
}

/************************************************************************
 * VolumeMargin - is there enough space on a volume for something?
 ************************************************************************/
int VolumeMargin(short vRef, long spaceNeeded) {
  long margin = GetRLong(VOLUME_MARGIN);

  if (margin && VolumeFree(vRef) < spaceNeeded + margin)
    return (dskFulErr);

  return (0);
}

/************************************************************************
 * MyAllocate - allocate disk space for a file
 ************************************************************************/
int MyAllocate(short refN, long size) {
// Try to preallocate space
#ifdef __linux__
  if (posix_fallocate(refN, 0, size) == 0)
    return 0;
#endif
  // Fallback: extend file size
  if (ftruncate(refN, size) < 0)
    return ioErr;
  return 0;
}

/************************************************************************
 * SFPutOpen - open a file for write, using stdfile
 ************************************************************************/
short SFPutOpen(FSSpec * spec, long creator, long type, short *refN,
                ModalFilterYDUPP filter, DlgHookYDUPP hook, short id,
                FSSpec * defaultSpec, const char *windowTitle,
                const char *message) {
  FInfo info;
  ScriptCode script;
  int theError;
  short ditlID;

  if (!MommyMommy(ATTENTION, nil))
    return (1);

  if (UseNavServices()) {
    switch (id) {
    case SAVEAS_DLOG:
      ditlID = SAVEAS_NAV_DITL;
      break;
    default:
      ditlID = 0;
      break;
    }
    theError =
        SFPutOpenNav(spec, creator, type, refN, ditlID, (uint32_t *)&script,
                     defaultSpec, windowTitle, message);
  }
  if (theError)
    return (theError);

  /*
   * create && open the file
   */
  int fd = creat(spec->path, 0644);
  if (fd < 0) {
    theError = (errno == EEXIST) ? dupFNErr : ioErr;
    if (theError == dupFNErr)
      theError = 0;
    else {
      FileSystemError(COULDNT_SAVEAS, (const char *)spec->name, theError);
      return (theError);
    }
  } else {
    close(fd);
    theError = 0;
  }

  if ((theError = MyFSpOpenDF(spec, fsRdWrPerm, refN))) {
    FileSystemError(COULDNT_SAVEAS, (const char *)spec->name, theError);
    MyFSpDelete(spec);
    return (theError);
  }

  if (!(theError = MyFSpGetFInfo(spec, NULL, &info))) {
    info.fdType = type;
    MyFSpSetFInfo(spec, NULL, &info);
  }

  if ((theError = SetEOF(*refN, 0))) {
    FileSystemError(COULDNT_SAVEAS, (const char *)spec->name, theError);
    MyFSpDelete(spec);
    return (theError);
  }

  WhackFinder(spec);

  return (0);
}

/**********************************************************************
 * FSpModDate - get the mod date of a file
 **********************************************************************/
uint32_t FSpModDate(FSSpec * spec) {
  CInfoPBRec hfi;

  if (FSpGetHFileInfo(spec, &hfi))
    return (0);
  return (hfi.hFileInfo.ioFlMdDat);
}

/************************************************************************
 * Snarf - read a whole file into a handle
 ************************************************************************/
int Snarf(FSSpec * spec, void ***hp, long limit) {
  short refN;
  long bytes;
  short err;

  if (!((err = MyFSpOpenDF(spec, fsRdPerm, &refN)))) {
    if (!((err = GetEOF(refN, &bytes)))) {
      if (limit)
        bytes = MIN(bytes, limit);
      *hp = NuHTempOK(bytes);
      if (!*hp)
        err = MemError();
      else if ((err = ARead(refN, &bytes, (unsigned char *)(*hp)))) {
        free(*hp);
        *hp = NULL;
      }
    }
    close(refN);
  }
  return (err);
}

/************************************************************************
 * SnarfRoman - read a whole file into a handle, and make it roman text if we
 *can
 ************************************************************************/
int SnarfRoman(FSSpec * spec, void ***hp, long limit) {
  // grab the actual text
  int err = Snarf(spec, hp, limit);

  if (!err)
    err = SniffAndConvertHandleToRoman(hp);

  if (err) {
    free(*hp);
    *hp = NULL;
  }

  return err;
}

/************************************************************************
 * Blat - blat a handle out to a text file
 ************************************************************************/
int Blat(FSSpec * spec, void *text, bool append) {
  int err;

  err = BlatPtr(spec, text, GetHandleSize_(text), append);
  return (err);
}

/************************************************************************
 * BlatPtr - blat text out to a text file
 ************************************************************************/
int BlatPtr(FSSpec * spec, char * text, long size, bool append) {
  FSSpec newSpec;
  int err;
  short refN;

  int fd = creat(spec->path, 0644);
  if (fd >= 0)
    close(fd);

  if ((err = MyFSpOpenDF(spec, fsRdWrPerm, &refN)))
    FileSystemError(TEXT_WRITE, (const char *)spec->name, err);
  else {
    if (append && (err = SetFPos(refN, fsFromLEOF, 0)))
      FileSystemError(TEXT_WRITE, (const char *)spec->name, err);
    if ((err = AWrite(refN, &size, (unsigned char *)text)))
      FileSystemError(TEXT_WRITE, (const char *)spec->name, err);
    else if ((err = TruncAtMark(refN)))
      FileSystemError(TEXT_WRITE, (const char *)spec->name, err);
    close(refN);
  }
  return (err);
}

/**********************************************************************
 * FileTypeOf - get the type of a file
 **********************************************************************/
OSType FileTypeOf(FSSpec * spec) {
  FInfo info;
  if (!MyFSpGetFInfo(spec, NULL, &info))
    return (info.fdType);
  else
    return (0);
}

/**********************************************************************
 * FlushFile - flush a file buffer
 **********************************************************************/
int FlushFile(short refN) {
  if (fsync(refN) < 0)
    return ioErr;
  return 0;
}

/**********************************************************************
 * MyUpdateResFile - udpate a resource file, and MAKE darn SURE
 **********************************************************************/
int MyUpdateResFile(short resFile) {
  int err = 0;

  if (GetResFileAttrs(resFile) & mapChanged) {
    UpdateResFile(resFile);
    if (!((err = ResError())) && !PrefIsSet(PREF_CORVAIR))
      err = MakeDarnSure(resFile);
  }
  return (err);
}

/**********************************************************************
 * MakeDarnSure - a file is intact on disk
 **********************************************************************/
int MakeDarnSure(short refN) {
  int err;
  FSSpec spec;

  if (!((err = FlushFile(refN))))
    if (!((err = GetFileByRef(refN, &spec))))
      err = FlushVol(nil, spec.vRefNum);
  return (err);
}

/**********************************************************************
 * FileCreatorOf - get the creator of a file
 **********************************************************************/
OSType FileCreatorOf(FSSpec * spec) {
  FInfo info;
  if (!MyFSpGetFInfo(spec, NULL, &info))
    return (info.fdCreator);
  else
    return (0);
}

/************************************************************************
 * IsText - is a file of type TEXT or not?
 ************************************************************************/
bool IsText(FSSpec * spec) {
  FInfo info;
  FSSpec newSpec;
  short err;

  err = MyFSpGetFInfo(spec, &newSpec, &info);
  return (!err && info.fdType == 'TEXT');
}

/************************************************************************
 * SanitizeFN - make a filename more palatable
 ************************************************************************/
char *SanitizeFN(const char *shortName, char *name, short badCharId,
                 short repCharId, bool kill8) {
  if (!name || !shortName)
    return name;
  strcpy(name, shortName);
  return name;
}

/************************************************************************
 * Mac2OtherName - transmogrify mac name to acceptable outworld name
 ************************************************************************/
char *Mac2OtherName(const char *mac, char *other) {
  if (other && mac)
    strcpy(other, mac);
  return other;
}


/************************************************************************
 * SpinOn - spin until a return code is not inProgress or cacheFault
 ************************************************************************/
short SpinOnLo(volatile int *rtnCodeAddr, long maxTicks, bool allowCancel,
               bool forever, bool remainCalm, bool allowMouseDown) {
  long ticks = TickCount();
  long startTicks = ticks + 120;
  long now;
#ifdef CTB
  extern ConnHandle CnH;
#endif
  bool oldCommandPeriod = CommandPeriod;
  bool slow = false;
  static short slowThresh;

  if (!slowThresh)
    slowThresh = GetRLong(SPIN_LENGTH);

  if (allowCancel)
    YieldTicks = 0;
  if (allowCancel || *rtnCodeAddr == inProgress || *rtnCodeAddr == cacheFault) {
    CommandPeriod = false;
    do {
      now = TickCount();
      if (now > startTicks && now - ticks > slowThresh) {
        slow = true;
        if (!InAThread())
          CyclePendulum();
        else
          MyYieldToAnyThread();
        ticks = now;
      }
      if (slow && !InAThread())
        YieldTicks = 0;
      MiniEventsLo((!remainCalm || GetNumBackgroundThreads()) ? 0 : 300,
                   allowMouseDown ? MINI_MASK | mDownMask : MINI_MASK);
      if (CommandPeriod && !forever)
        return (userCancelled);
      if (maxTicks && startTicks + maxTicks < now + 120)
        break;
    } while (*rtnCodeAddr == inProgress || *rtnCodeAddr == cacheFault);
    if (CommandPeriod)
      return (userCancelled);
    CommandPeriod = oldCommandPeriod;
  }
  return (*rtnCodeAddr);
}

/**********************************************************************
 * FSpTrash - trash a file
 **********************************************************************/
int FSpTrash(FSSpec * spec) {
  FSSpec trashSpec;
  int err;
  FSSpec exist, newExist;

  if (!((err = GetTrashSpec(spec->vRefNum, &trashSpec)))) {
    if (!spec_for(trashSpec.path, (const char *)spec->name, &exist)) {
      newExist = exist;
      UniqueSpec(&newExist, 31);
      MyFSpRename(&exist, (char *)newExist.name);
    }
    err = SpecMove(spec, &trashSpec);
  }
  return (err);
}

/**********************************************************************
 * UniqueSpec - make a unique filename
 **********************************************************************/
int UniqueSpec(FSSpec * spec, short max) {
  short i;
  CInfoPBRec hfi;
  char dfName[256];
  char dfQuoteExtensionUnquote[256];
  char iAscii[256];
  int err;

  //
  // Now that we've entered unix-land, we have to pay attention to
  // "extensions".
  //
  max = SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
      (char *)spec->name, dfName, dfQuoteExtensionUnquote, max);

  for (i = 1; i < 9999; i++) {
    /*
     * does file exist?
     */
    err = HGetCatInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                      &hfi);
#ifdef NEVER
    if (RunType != Production)
      Dprintf("HGetCatInfo %p: %d;sc;g", spec->name, err);
#endif
    if (err == fnfErr)
      return (0);

    /*
     * oops.  file exists.  increment number on end of filename
     */
    g_strlcpy(spec->name, dfName, sizeof(spec->name));
    *iAscii = 0;
    PLCat(iAscii, i);
    if (*spec->name + *iAscii < max)
      g_strlcat(spec->name, iAscii, 256);
    else {
      BMD(iAscii + 1, spec->name + (max + 1) - *iAscii, *iAscii);
      *spec->name = max;
    }

    //
    // add "extension"
    if (*dfQuoteExtensionUnquote) {
      g_strlcat(spec->name, ".", sizeof(spec->name));
      g_strlcat(spec->name, dfQuoteExtensionUnquote, sizeof(spec->name));
    }
  }
  return (dupFNErr);
}

/**********************************************************************
 * SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote -
 *   we hate Windows
 **********************************************************************/
int SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
    const char *full, char *name, char *ext, short max) {
  char *spot;

  if (name != full)
    strcpy(name, full);
  *ext = '\0';

  if ((spot = strrchr(name, '.'))) {
    // found the last period
    // create the "extension"
    strcpy(ext, spot + 1);

    // the "extension" must be nonzero and mustn't be, like, rilly big
    size_t extLen = strlen(ext);
    size_t nameLen = strlen(name);
    if (extLen > 0 && extLen < nameLen - 1 && extLen < 8) {
      *spot = '\0';
      max -= (short)(extLen + 1);
    } else
      *ext = '\0';
  }

  return max;
}

/**********************************************************************
 * TweakFileType - set a file's type, and make sure the Finder catches on
 **********************************************************************/
int TweakFileType(FSSpec * spec, OSType type, OSType creator) {
  FInfo info;
  int err;
  FSSpec dirSpec;

  /*
   * get parent info
   */
  if (!((err = MyFSpGetFInfo(spec, NULL,  &info)))) {
    info.fdType = type;
    info.fdCreator = creator;
    info.fdFlags &= ~kHasBeenInited;
    if (type == 'APPL')
      info.fdFlags |= fHasBundle;
    if (!((err = MyFSpSetFInfo(spec, NULL,  &info)))) {
      { char pdir[1024]; spec_parent(spec, pdir, sizeof(pdir));
      if (!((err = spec_for(pdir, NULL, &dirSpec))))
        err = FSpTouch(&dirSpec); }
    }
  }
  return (err);
}

/**********************************************************************
 * FSpTouch - set a file's mod date to now
 **********************************************************************/
int FSpTouch(FSSpec * spec) {
  CInfoPBRec hfi;
  int err;
  char name[256];

  g_strlcpy(name, spec->name, 256);
  if (!((err = HGetCatInfo(spec->vRefNum, spec->parID, name, &hfi)))) {
    hfi.hFileInfo.ioFlMdDat = LocalDateTime();
    g_strlcpy(name, spec->name, 256);
    err = HSetCatInfo(spec->vRefNum, spec->parID, name, &hfi);
  }
  return (err);
}

/**********************************************************************
 * FSpExists - see if a file exists
 **********************************************************************/
int FSpExists(FSSpec * spec) {
  FInfo info;
  return (MyFSpGetFInfo(spec, NULL,  &info));
}

/**********************************************************************
 * FSpRFSane - is a resource file sane?
 **********************************************************************/
int FSpRFSane(FSSpec * spec, bool *sane) {
  int err;

  err = utl_RFSanity(spec->path, sane);
  return (err);
}

/**********************************************************************
 * FSpKillRFork - kill the resource fork of a file
 **********************************************************************/
int FSpKillRFork(FSSpec * spec) {
  int err;
  short refN;

  if (!((err = MyFSpOpenRF(spec->path, fsRdWrPerm, &refN)))) {
    err = SetEOF(refN, 0);
    close(refN);
  }
  return err;
}

/************************************************************************
 * AliasFolderType - does alias'es filetype represent a folder?
 ************************************************************************/
bool AliasFolderType(OSType type) {
  OSType types[] = {kExportedFolderAliasType,    kContainerServerAliasType,
                    kContainerFloppyAliasType,   kContainerFolderAliasType,
                    kContainerHardDiskAliasType, kMountedFolderAliasType,
                    kSharedFolderAliasType};
  short i = sizeof(types) / sizeof(OSType);

  while (i--)
    if (type == types[i])
      return true;
  return false;
}

/************************************************************************
 * IsItAFolder - is the specified file a folder?
 ************************************************************************/
bool IsItAFolder(short vRef, long dirId, const char *name) {
  struct stat st;
  char path[256];

  if (!name)
    return false;

  // Assuming 'name' is already a C string.
  // If 'name' is a Pascal string, PtoCcpy would be needed first.
  // For now, construct the path directly.
  snprintf(path, sizeof(path), "./%s", name);

  if (stat(path, &st) == 0) {
    return S_ISDIR(st.st_mode);
  }
  return false;
}

/************************************************************************
 * HFIIsFolder - is this thing a folder?  special case since we have hfi
 *already
 ************************************************************************/
bool HFIIsFolder(CInfoPBRec *hfi) {
  return 0 != (hfi->hFileInfo.ioFlAttrib & 0x10);
}

/************************************************************************
 * HFIIsFolderOrAlias - is this thing a folder?  special case since we have
 *hfi already
 ************************************************************************/
bool HFIIsFolderOrAlias(CInfoPBRec *hfi) {
  if (hfi->hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
    return (AliasFolderType(hfi->hFileInfo.ioFlFndrInfo.fdType));
  else
    return HFIIsFolder(hfi);
}

/************************************************************************
 * FolderSizeHi - how big is a folder?
 ************************************************************************/
void FolderSizeHi(const char *dir, uint32_t *cumSize) {
  CInfoPBRec hfi;
  char name[256];

  hfi.hFileInfo.ioNamePtr = name;
  *cumSize = 0;

  FolderSize(dir, &hfi, cumSize);

  return;
}

/************************************************************************
 * FolderSize - how big is a folder?
 ************************************************************************/
static bool FolderSizeCallback(DirIterateInfo *info) {
  uint32_t *cumSize = (uint32_t *)info->data;
  if (info->isDir) {
    uint32_t subSize = 0;
    FolderSize(info->spec.path, nil, &subSize);
    *cumSize += subSize;
  } else {
    *cumSize += info->size;
  }
  return true;
}

void FolderSize(const char *dir, CInfoPBRec *hfi, uint32_t *cumSize) {
  FSSpec spec;
  spec_for(dir, NULL, &spec);
  DirIterate(&spec, cumSize, FolderSizeCallback);
}

/************************************************************************
 * HGetCatInfo - get cat info for a file?
 ************************************************************************/
short HGetCatInfo(short vRef, long inDirId, const char *name, CInfoPBRec *hfi) {
  struct stat st;
  char fullPath[1024];

  Zero(*hfi);
  if (name && *name) {
    // This is still a bit hacky for the path
    snprintf(fullPath, sizeof(fullPath), "./%s", name);
  } else {
    // Should use inDirId to find the path
    strcpy(fullPath, ".");
  }

  if (stat(fullPath, &st) != 0)
    return fnfErr;

  hfi->hFileInfo.ioFlMdDat = (long)st.st_mtime;
  hfi->hFileInfo.ioFlLgLen = (long)st.st_size;
  if (S_ISDIR(st.st_mode))
    hfi->hFileInfo.ioFlAttrib |= ioDirMask;

  return 0;
}

/************************************************************************
 * HSetCatInfo - get cat info for a file?
 ************************************************************************/
short HSetCatInfo(short vRefNum, long dirID, const char *name, CInfoPBPtr pb) {
  return 0;
}

short HMove(short vRef, long dirId, const char *name, long destDirId,
            const char *newName) {
  char fromPath[1024];
  char toPath[1024];
  const char *destName = newName ? newName : name;

  if (!name)
    return paramErr;

  /* Build simple relative paths; in this port the dirId/vRef are ignored */
  snprintf(fromPath, sizeof(fromPath), "./%s", name);
  snprintf(toPath, sizeof(toPath), "./%s", destName);

  if (rename(fromPath, toPath) == 0)
    return 0;

  return ioErr;
}

/**********************************************************************
 * FSpOpenResFile - open the (data) file for read/write. Resource forks are
 * not available on POSIX; we open the data fork instead and return its fd.
 **********************************************************************/
short FSpOpenResFile(FSSpec * spec, SignedByte permission) {
  if (!spec)
    return -1;

  int flags = O_RDONLY;
  if (permission == fsWrPerm)
    flags = O_WRONLY;
  else if (permission == fsRdWrPerm)
    flags = O_RDWR;

  int fd = open(spec->path, flags);
  if (fd < 0)
    return -1;
  return (short)fd;
}

/**********************************************************************
 * ExtractCreatorFromBndl - figure out what an app's creator used to be
 **********************************************************************/
int ExtractCreatorFromBndl(FSSpec * spec, OSType *creator) {
  int err;
  short refN;
  void *bndl;
  short oldResF = CurResFile();

  if (-1 != (refN = FSpOpenResFile(spec, fsRdPerm))) {
    if ((bndl = MyGetIndResource('BNDL', 1))) {
      *creator = *(long *)bndl;
      err = 0;
    } else
      err = resNotFound;
    MyCloseResFile(refN);
    UseResFile(oldResF);
  } else
    err = ResError();
  return (err);
}

/************************************************************************
 * MyFSpGetCatInfo - cat info, resolving aliases
 ************************************************************************/
short MyFSpGetCatInfo(FSSpec * spec, FSSpec * newSpec, CInfoPBRec *hfi) {
  int err;
  bool folder, wasIt;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  return (HGetCatInfo(newSpec->vRefNum, newSpec->parID,
                      (const char *)newSpec->name, hfi));
}

/************************************************************************
 * FolderFileCount - count the files in a folder
 ************************************************************************/
short FolderFileCount(FSSpec * spec) {
  CInfoPBRec hfi;
  FSSpec newSpec;

  if (MyFSpGetCatInfo(spec, &newSpec, &hfi))
    return (-1);
  return (hfi.hFileInfo.ioFlStBlk);
}

/**********************************************************************
 * RemoveDir - remove a directory
 **********************************************************************/
int RemoveDir(FSSpec * spec) {
  DIR *dir;
  struct dirent *entry;
  char fullpath[PATH_MAX];
  FSSpec folder;
  int err = 0;

  folder = *spec;
  IsAlias(&folder, &folder);

  // Open the directory
  dir = opendir(folder.path);
  if (!dir) {
    return errno ? errno : ioErr;
  }

  // Delete all entries in the directory
  while ((entry = readdir(dir)) != NULL) {
    // Skip . and ..
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    // Build full path
    snprintf(fullpath, sizeof(fullpath), "%s/%s", folder.path, entry->d_name);

    // Delete the entry
    {
      FSSpec tmpSpec;
      spec_make(NULL, fullpath, &tmpSpec);
      err = MyFSpDelete(&tmpSpec);
    }
    if (err) {
      closedir(dir);
      return err;
    }
  }

  closedir(dir);
  return ChainDelete(spec);
}

/************************************************************************
 * ChainDelete - delete an entire alias chain
 ************************************************************************/
int ChainDelete(FSSpec * spec) {
  FSSpec chain;
  bool wasAlias, isFolder;

  chain = *spec;
  if (!ResolveAliasFile(&chain, false, &isFolder, &wasAlias) && wasAlias)
    ChainDelete(&chain);
  return (MyFSpDelete(spec));
}

/************************************************************************
 * MyAHGetFileInfo - get info on a file
 ************************************************************************/
int MyAHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi) {
  char cname[256];
  struct stat st;

  // Assuming 'name' is already a C string.
  // If it were a Pascal string, PtoCcpy would be needed.
  // For consistency with other functions, we'll use it directly.
  strncpy(cname, name, sizeof(cname) - 1);
  cname[sizeof(cname) - 1] = '\0'; // Ensure null termination

  WriteZero(hfi, sizeof(*hfi));

  if (stat(cname, &st) < 0)
    return ioErr;

  hfi->hFileInfo.ioFlMdDat = st.st_mtime;
  hfi->hFileInfo.ioFlLgLen = st.st_size;

  return 0;
}

#pragma segment FileUtil2

/************************************************************************
 * FSpGetHFileInfo - get info, don't resolve alias
 ************************************************************************/
int FSpGetHFileInfo(FSSpec * spec, CInfoPBRec *hfi) {
  struct stat st;

  Zero(*hfi);
  if (stat(spec->path, &st) < 0)
    return ioErr;

  // Fill in basic info
  hfi->hFileInfo.ioFlMdDat = st.st_mtime;
  hfi->hFileInfo.ioFlLgLen = st.st_size;  // data fork size
  hfi->hFileInfo.ioFlRLgLen = 0;  // resource fork size (0 on Unix)
  
  // Fill in Finder info with defaults for portable build
  // These would normally come from extended attributes or a metadata file
  hfi->hFileInfo.ioFlFndrInfo.fdType = 'TEXT';  // default type
  hfi->hFileInfo.ioFlFndrInfo.fdCreator = '????';  // default creator
  hfi->hFileInfo.ioFlFndrInfo.fdFlags = 0;  // no special flags

  return 0;
}

short MyAHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi) {
  FILL(hfi->hFileInfo, newName, vRef, dirId);
  return (PBHSetFInfoSync((HParmBlkPtr)hfi));
}

/************************************************************************
 * I am indebted to Tim Maroney (tim@toad.com) for the following routines.
 ************************************************************************/
static bool good, noSys, needWrite, allowFloppy, allowDesktop;

pascal bool FolderFilter(FileParam *pb) {
#pragma unused(pb)
  return true;
}


bool GetFolder(char *name, short *volume, long *folder, bool writeable,
                  bool system, bool floppy, bool desktop) {
  bool results;

  results = GetFolderNav(name, volume, folder);
  return (results);
}

/************************************************************************
 * CopyFork - copy the resource fork from one file to another
 ************************************************************************/
short CopyFork(short vRef, long dirId, const char *name, short fromVRef,
               long fromDirId, const char *fromName, bool rFork,
               bool progress) {
  short err;
  short fromRef, toRef;
  unsigned char *buffer = malloc(OPTIMAL_BUFFER);
  long bSize = OPTIMAL_BUFFER;
  long eof = 0;

  if (!buffer)
    return (MemError());

  // If resource fork requested, just return success (portable code has no
  // resource forks)
  if (rFork) {
    free(buffer);
    return 0;
  }

  // Convert Pascal string names to C strings and build paths
  char fromCName[256], toCName[256];
  g_strlcpy(fromCName, (const char *)fromName, 256);
  g_strlcpy(toCName, (const char *)name, 256);

  char fromPath[1024], toPath[1024];
  snprintf(fromPath, sizeof(fromPath), "./%s", fromCName);
  snprintf(toPath, sizeof(toPath), "./%s", toCName);

  if ((fromRef = open(fromPath, O_RDONLY)) >= 0) {
    if ((toRef = open(toPath, O_RDWR)) >= 0) {
      err = 0;
      GetEOF(fromRef, &eof);
      for (bSize = MIN(OPTIMAL_BUFFER, eof); !err && eof;
           bSize = MIN(OPTIMAL_BUFFER, eof)) {
        if (!(err = ARead(fromRef, &bSize, buffer))) {
          if (progress)
            ByteProgress(NULL, -1, bSize);
          eof -= bSize;
          err = AWrite(toRef, &bSize, buffer);
          if (progress)
            ByteProgress(NULL, -1, bSize);
        }
      }
      TruncAtMark(toRef);
      close(toRef);
    } else {
      err = fnfErr;
    }
    close(fromRef);
  } else {
    err = fnfErr;
  }
  free(buffer);
  return (err);
}

/**********************************************************************
 * FSpDupFile - duplicate a file
 **********************************************************************/
int FSpDupFile(FSSpec * to, FSSpec * from, bool replace, bool progress) {
  int err;
  bool hasRFork = FSpRFSize(from) != 0;
#ifdef DEBUG
  char s[256];
#endif

  if (hasRFork) {
    MyFSpCreateResFile(to, '----', '----', 0);
    err = ResError();
  } else {
    // Create file using path
    int fd = creat(to->path, 0644);
    if (fd < 0)
      err = ioErr;
    else {
      close(fd);
      err = 0;
    }
  }

  if (err && err == dupFNErr && replace)
    err = 0;
#ifdef DEBUG
  if (err) {
    ComposeString((unsigned char *)s, "MyFSpDuplicate: create failed %d.%d.%s; %d",
                  to->vRefNum, to->parID, (char *)to->name, err);
    AlertStr(OK_ALRT, Note, s);
  }
#endif
  if (err)
    return (err);

  if (progress)
    ByteProgress(NULL, 0, 2 * (FSpDFSize(to) + FSpRFSize(to)));
  if (!hasRFork || !(err = FSpCopyRFork(to, from, progress))) {

    if (!(err = FSpCopyDFork(to, from, progress))) {
      if (!progress)
        MiniEvents();
      else
        Progress(100, 0, nil, nil, nil);
      if (!(err = FSpCopyFInfo(to, from)))
        return (0);
    }
#ifdef DEBUG
    else {
      ComposeString((unsigned char *)s,
                    "MyFSpDuplicate: dfork failed %d.%d.%s->%d.%d.%s; %d",
                    from->vRefNum, from->parID, (char *)from->name, to->vRefNum,
                    to->parID, (char *)to->name, err);
      AlertStr(OK_ALRT, Note, s);
    }
#endif
  }
#ifdef DEBUG
  else if (hasRFork) {
    ComposeString((unsigned char *)s,
                  "MyFSpDuplicate: rfork failed %d.%d.%s->%d.%d.%s; %d",
                  from->vRefNum, from->parID, (char *)from->name, to->vRefNum,
                  to->parID, (char *)to->name, err);
    AlertStr(OK_ALRT, Note, s);
  }
#endif

  MyFSpDelete(to);
  return err;
}

/**********************************************************************
 * FSpDupFolder - duplicate a folder
 **********************************************************************/
int FSpDupFolder(FSSpec * toSpec, FSSpec * fromSpec, bool replace,
                   bool progress) {
  CInfoPBRec hfi;
  FSSpec to, from;
  int err = 0;

  to = *toSpec;
  from = *fromSpec;

  // TODO: Properly implement directory iteration with callback
  // For now, just return an error to indicate directory copying needs
  // implementation
  return ioErr;

  /* Original code that needs proper callback implementation:
  hfi.hFileInfo.ioNamePtr = (unsigned char *)from.name;
  hfi.hFileInfo.ioFDirIndex = 0;
  while (!err && !DirIterate(fromSpec, NULL, NULL)) {
    g_strlcpy(to.name, from.name, sizeof(to.name));
    if ((hfi.hFileInfo.ioFlAttrib & ioDirMask)) {
      //	copy folder
      long saveFrom, saveTo, createdDirID;

      //	save current parID's so we can reuse specs
      //	point specs to the folder
      if (!FSpDirCreate(to.path, 0, &createdDirID)) {
        saveFrom = from.parID;
        from.parID = SpecDirId(&from);
        saveTo = to.parID;
        to.parID = createdDirID;
        //	recurse
        err = FSpDupFolder(&to, &from, replace, progress);
        //	restore the parID's
        from.parID = saveFrom;
        to.parID = saveTo;
      }
    } else {
      //	copy file
      FSpDupFile(&to, &from, replace, progress);
    }
  }
  */
  // return err;
}

/************************************************************************
 * CopyFInfo - copy the file info from one file to another
 ************************************************************************/
short CopyFInfo(short vRef, long dirId, const char *name, short fromVRef,
                long fromDirId, const char *fromName) {
  short err;
  CInfoPBRec hfi;

  if (!(err = HGetCatInfo(fromVRef, fromDirId, fromName, &hfi))) {
    hfi.hFileInfo.ioFlFndrInfo.fdFlags &=
        ~fInited; // make the finder move it someplace rational
    Zero(hfi.hFileInfo.ioFlFndrInfo.fdLocation);
    err = HSetCatInfo(vRef, dirId, name, &hfi);
  }
  return (err);
}

/************************************************************************
 * MyResolveAlias - resolve an alias
 ************************************************************************/
short MyResolveAlias(const char *dir, char *name, bool *wasAlias) {
  FSSpec theSpec;
  bool folder;
  long haveAlias;
  short err = 0;
  bool wasIt;

  if (wasAlias)
    *wasAlias = false;
  if (!Gestalt(gestaltAliasMgrAttr, &haveAlias) && haveAlias & 0x1) {
    if (!(err = spec_for(dir, name, &theSpec)) &&
        !(err = ResolveAliasFile(&theSpec, true, &folder, &wasIt))) {
      if (wasIt) {
        g_strlcpy(name, theSpec.name, sizeof(theSpec.name));
        if (wasAlias)
          *wasAlias = true;
      }
    }
  }
  return (err);
}


/************************************************************************
 * ExchangeAndDel - exchange two files, deleting one
 ************************************************************************/
int ExchangeAndDel(FSSpec * tmpSpec, FSSpec * spec) {
  short err;

  if ((err = ExchangeFiles(tmpSpec, spec))) {
    FileSystemError(TEXT_WRITE, tmpSpec->name, err);
    return (err);
  }
  MyFSpDelete(tmpSpec);
  return (0);
}

/************************************************************************
 * ExchangeFiles - FSpExchangeFiles, with support for dopey AFP servers
 ************************************************************************/
int ExchangeFiles(FSSpec * tmpSpec, FSSpec * spec) {
  int err = MyFSpExchangeFiles(tmpSpec, spec);

  if (err)
    err = FSpExchangeFilesCompat(tmpSpec, spec);
  return (err);
}

/************************************************************************
 * FSpExchangeFilesCompat - do FSpExchangeFiles if FSpExchangeFiles not
 *supported from MoreFiles
 ************************************************************************/
int FSpExchangeFilesCompat(const FSSpec *source, const FSSpec *dest) {
  char temp_path[PATH_MAX];
  struct stat st_source, st_dest;
  int result = 0;

  // Verify both files exist
  if (stat(source->path, &st_source) != 0) {
    return fnfErr;
  }
  if (stat(dest->path, &st_dest) != 0) {
    return fnfErr;
  }

  // Create a temporary name in the same directory as source
  snprintf(temp_path, sizeof(temp_path), "%s.swap.tmp", source->path);

  // Three-way rename: source -> temp, dest -> source, temp -> dest
  if (rename(source->path, temp_path) != 0) {
    return errno;
  }
  if (rename(dest->path, source->path) != 0) {
    rename(temp_path, source->path); // Try to restore
    return errno;
  }
  if (rename(temp_path, dest->path) != 0) {
    // Try to restore original state
    rename(source->path, dest->path);
    rename(temp_path, source->path);
    return errno;
  }

  result = 0;

  return (result);
}

/**********************************************************************
 * HasFileIDs - does volume support file ID's?
 **********************************************************************/
bool HasFileIDs(const GetVolParmsInfoBuffer *volParms) {
  return ((volParms->vMAttrib & (1L << bHasFileIDs)) != 0);
}

/************************************************************************
 * GenerateUniqueName - generates a name that is unique in both dir1 and dir2
 ************************************************************************/
static int GenerateUniqueName(short volume, long *startSeed, long dir1,
                                long dir2, char *uniqueName) {
  int error = 0;
  long i;
  CInfoPBRec cinfo;
  unsigned char hexStr[16];

  for (i = 0; i < 16; ++i) {
    if (i < 10) {
      hexStr[i] = 0x30 + i;
    } else {
      hexStr[i] = 0x37 + i;
    }
  }

  char hex_name[9];

  // Generate unique names by incrementing seed
  // In POSIX we just generate a unique hex name, no volume references
  while (error != fnfErr) {
    (*startSeed)++;

    // Create hex name from seed
    snprintf(hex_name, sizeof(hex_name), "%08lx", *startSeed);

    // Convert to Pascal string
    uniqueName[0] = 8;
    memcpy(uniqueName + 1, hex_name, 8);

    // In POSIX world, we assume the name is unique based on seed
    // The original code checked if file exists in two directories,
    // but without volume references we can't do that properly
    error = fnfErr; // Assume unique after incrementing
  }
  return (0);
}

/**********************************************************************
 * MorphDesktop - if a spec points to the boot disk desktop and the
 *  requested volume is not the desktop, then set the spec to the requested
 *  volume's desktop
 **********************************************************************/
int MorphDesktop(short vRef, FSSpec * where) {
  FSSpec desk;
  int err;

  if (*where->name)
    return (0); // not pointing to folder
  if (SameVRef(vRef, where->vRefNum))
    return (0); // same volume
  if ((err = FindFolder(kOnSystemDisk, kDesktopFolderType, false,
                        (short *)&desk.vRefNum, &desk.parID)))
    return (err);
  if (SameVRef(vRef, desk.vRefNum))
    return (0); // same volume as system
  if (SameVRef(where->vRefNum, desk.vRefNum) && desk.parID == where->parID) {
    // ok, spec points to desktop folder
    // point instead to desktop folder on volume
    err = FindFolder(vRef, kDesktopFolderType, false, (int *)&where->vRefNum,
                     &where->parID);
  }
  return (err);
}

/************************************************************************
 * MyFSpIsItAFolder - is a file a folder?
 ************************************************************************/
bool MyFSpIsItAFolder(FSSpec * spec) {
  struct stat st;
  if (!spec || !spec->path[0]) return false;
  if (lstat(spec->path, &st) == 0) {
    return S_ISDIR(st.st_mode);
  }
  return false;
}

bool AFSpIsItAFolder(FSSpec *spec) {
  return MyFSpIsItAFolder(spec);
}

/************************************************************************
 * GetFileByRef - figure out the name & vol of a file from an open file
 ************************************************************************/
short GetFileByRef(short refN, FSSpec * specPtr) {
  FCBPBRec fcb;
  short err;
  char name[256];

  fcb.ioCompletion = nil;
  fcb.ioVRefNum = 0;
  fcb.ioRefNum = refN;
  fcb.ioFCBIndx = 0;
  fcb.ioNamePtr = (unsigned char *)name;
  if ((err = PBGetFCBInfo((FCBInfoPBPtr)&fcb, false)))
    return (err);
  return (spec_for(NULL, name, specPtr));
}

/************************************************************************
 * VolumeFree - return the free space on a volume
 ************************************************************************/
long VolumeFree(short vRef) {
#include <sys/statvfs.h>


  struct statvfs vfs;
  if (statvfs(".", &vfs) != 0)
    return 0;
  return (long)(vfs.f_bavail * vfs.f_frsize);
}

/************************************************************************
 * FSTabWrite - write, expanding tabs
 ************************************************************************/
short FSTabWrite(short refN, long *count, unsigned char *buf) {
  unsigned char *p;
  unsigned char *end = buf + *count;
  long written = 0;
  short err = 0;
  long writing;
  static short charsOnLine = 0;
  unsigned char *nl;
  short stops = 0;

  if (!FakeTabs)
    return (FSZWrite(refN, count, buf));
  for (p = buf; p < end; p = buf = p + 1) {
    nl = buf - charsOnLine - 1;
    while (p < end && *p != tabChar) {
      if (*p == '\015' || *p == '\012')
        nl = p;
      p++;
    }
    writing = p - buf;
    err = FSZWrite(refN, &writing, buf);
    written += writing;
    if (err)
      break;
    charsOnLine = p - nl - 1;
    if (p < end) {
      if (!stops)
        stops = GetRLong(TAB_DISTANCE);
      writing = stops - (charsOnLine) % stops;
      charsOnLine = 0;
      err = FSZWrite(refN, &writing, (unsigned char *)"              ");
      written += writing;
      if (err)
        break;
    }
  }
  *count = written;
  return (err);
}

// Redundant ARead removed

/************************************************************************
 * NCWriteP - write a Pascal string
 ************************************************************************/
short NCWriteP(short refN, const char *pString) {
  long count = strlen(pString);
  return (NCWrite(refN, &count, (unsigned char *)pString));
}

/************************************************************************
 * AWriteP - write a Pascal string
 ************************************************************************/
short AWriteP(short refN, const char *pString) {
  long count = strlen(pString);
  return (AWrite(refN, &count, (unsigned char *)pString));
}

// Redundant AWrite removed

/**********************************************************************
 * WipeSpec - wipe a file
 **********************************************************************/
int WipeSpec(FSSpec * spec) {
  short refN;
  long eof;
  int err;

  if (!(err = MyFSpOpenDF(spec, fsRdWrPerm, &refN))) {
    if (!(err = GetEOF(refN, &eof)))
      err = WipeDiskArea(refN, 0, eof);
    close(refN);
    if (!(err = MyFSpOpenRF(spec->path, fsRdWrPerm, &refN))) {
      if (!(err = GetEOF(refN, &eof)))
        err = WipeDiskArea(refN, 0, eof);
      close(refN);
    }
  }
  if (!err) {
    FlushVol(nil, spec->vRefNum);
    err = MyFSpDelete(spec);
  }

  if (err && err != fnfErr)
    FileSystemError(WIPE_ERROR, (const char *)spec->name, err);

  return (err);
}

/**********************************************************************
 * WipeDiskArea - wipe part of a disk
 **********************************************************************/
int WipeDiskArea(short refN, long offset, long len) {
  long bSize = MIN(len, OPTIMAL_BUFFER);
  unsigned char *h;
  long size;
  unsigned char *spot, *end;
  int err;

  if (!bSize)
    return (0);
  h = malloc((size_t)bSize);
  if (!h)
    return (MemError());

  /* fill it with returns */
  end = h + bSize;
  for (spot = h; spot < end; spot++)
    *spot = ' ';
  spot[-1] = '\015';
  for (spot = h; spot < end; spot += 50)
    *spot = '\015';

  /*
   * blat it over the disk area
   */
  if (!(err = SetFPos(refN, fsFromStart, offset)))
    for (size = bSize; len; size = MIN(bSize, len)) {
      err = NCWrite(refN, &size, h);
      if (err)
        break;
      len -= size;
    }

  free(h);
  return (err);
}

/************************************************************************
 * EnsureNewline - make sure there is a newline at or just before the
 * current file position.
 ************************************************************************/
int EnsureNewline(short refN) {
  char chars[256];
  long offset, count;
  short err;

  /*
   * where are we?
   */
  if ((err = GetFPos(refN, &offset)))
    return (err);

  /*
   * BOF counts as newline
   */
  if (!offset)
    return (0);

  /*
   * back up one character
   */
  if ((err = SetFPos(refN, fsFromStart, offset - 1)))
    return (err);

  /*
   * read it
   */
  count = 1;
  if ((err = ARead(refN, &count, (unsigned char *)chars)))
    return (err);

  /*
   * is newline?
   */
  if (*chars == '\015' || *chars == '\012')
    return (0);

  /*
   * make it so
   */
  long one = 1;
  char cr = Cr;
  return (FSWrite(refN, &one, &cr));
}

/************************************************************************
 * MyFSpOpenDF - OpenDF, but resolve the alias first
 ************************************************************************/
int MyFSpOpenDF(FSSpec * spec, short permission,
                 short *refNum) {
  int err;
  bool folder, wasIt;
  FSSpec newSpec = *spec;
  short localRef;
  if ((err = ResolveAliasFile(&newSpec, true, &folder, &wasIt)))
    return (err);

  err = LowLevelFSpOpenDF(&newSpec, permission, &localRef);
  if (!err)
    *refNum = localRef;
  return err;
}

/************************************************************************
 * MyFSpOpenRF
 ************************************************************************/
int MyFSpOpenRF(const char *path, short permission,
                 short *refNum) {
  int err;
  FSSpec spec, newSpec;
  bool folder, wasIt;
  short localRef;

  spec_for(path, NULL, &spec);
  newSpec = spec;
  if ((err = ResolveAliasFile(&newSpec, true, &folder, &wasIt)))
    return (err);

  err = LowLevelFSpOpenRF(newSpec.path, permission, &localRef);
  if (!err)
    *refNum = localRef;
  return err;
}

/************************************************************************
 * MyFSpDelete
 ************************************************************************/
int MyFSpDelete(FSSpec * spec) {
  int err;
  bool folder, wasIt;
  FSSpec newSpec = *spec;
  if ((err = ResolveAliasFile(&newSpec, true, &folder, &wasIt)))
    return (err);

  return (LowLevelFSpDelete(&newSpec));
}

/************************************************************************
 * MyFSpGetFInfo
 ************************************************************************/
int MyFSpGetFInfo(FSSpec * spec, FSSpec * newSpec, FInfo *fndrInfo) {
  int err;
  bool folder, wasIt;
  FSSpec localNewSpec;
  FSSpec *targetSpec = newSpec ? newSpec : &localNewSpec;
  *targetSpec = *spec;
  if ((err = ResolveAliasFile(targetSpec, true, &folder, &wasIt)))
    return (err);

  return (LowLevelFSpGetFInfo(targetSpec, fndrInfo));
}

short MyFSpGetHFileInfo(FSSpec *spec, CInfoPBRec *hfi) {
  return (short)MyAHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name, hfi);
}

int MyFSpSetFInfo(FSSpec *spec, FSSpec *newSpec, FInfo *fndrInfo) {
  int err;
  bool folder, wasIt;
  FSSpec localNewSpec;
  FSSpec *targetSpec = newSpec ? newSpec : &localNewSpec;
  *targetSpec = *spec;
  if ((err = ResolveAliasFile(targetSpec, true, &folder, &wasIt)))
    return (err);

  return (LowLevelFSpSetFInfo(targetSpec, fndrInfo));
}

short MyFSpSetHFileInfo(FSSpec *spec, CInfoPBRec *hfi) {
  return (short)MyAHSetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name, hfi);
}

int MyFSpSetFLock(FSSpec *spec, FSSpec *newSpec) {
  (void)spec; (void)newSpec;
  return 0;
}

int MyFSpRstFLock(FSSpec *spec, FSSpec *newSpec) {
  (void)spec; (void)newSpec;
  return 0;
}

/************************************************************************
 * FSpFileSize - get the size of a file
 ************************************************************************/
long FSpFileSize(FSSpec * spec) {
  CInfoPBRec hfi;

  if (!MyAHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlLgLen + hfi.hFileInfo.ioFlRLgLen);
  else
    return (0);
}

/**********************************************************************
 *
 **********************************************************************/
int MyFSpSetMod(FSSpec * spec, uint32_t mod) {
  CInfoPBRec hfi;
  int err;
  FSSpec newSpec;
  bool folder, wasIt;

  newSpec = *spec;
  if ((err = ResolveAliasFile(&newSpec, true, &folder, &wasIt)))
    return (err);

  if (!(err = MyAHGetFileInfo(newSpec.vRefNum, newSpec.parID,
                            (const char *)newSpec.name, &hfi))) {
    hfi.hFileInfo.ioFlMdDat = mod;
    err = MyAHSetFileInfo(newSpec.vRefNum, newSpec.parID,
                        (const char *)newSpec.name, &hfi);
  }
  return (err);
}

/**********************************************************************
 *
 **********************************************************************/
uint32_t MyFSpGetMod(FSSpec * spec) {
  CInfoPBRec hfi;

  if (!MyAHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlMdDat);
  else
    return (0);
}

/************************************************************************
 * FSpDFSize - get the size of the data fork a file
 ************************************************************************/
long FSpDFSize(FSSpec * spec) {
  CInfoPBRec hfi;

  if (!MyAHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlLgLen);
  else
    return (0);
}

/************************************************************************
 * FSpRFSize - get the size of the data fork a file
 ************************************************************************/
long FSpRFSize(FSSpec * spec) {
  CInfoPBRec hfi;

  if (!MyAHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlRLgLen);
  else
    return (0);
}

/************************************************************************
 * FSpSetFXInfo - set the FXInfo for a file
 ************************************************************************/
int FSpSetFXInfo(FSSpec * spec, FXInfo *fxInfo) {
  int err;
  CInfoPBRec hfi;

  if (!(err = MyFSpGetHFileInfo(spec, &hfi))) {
    hfi.hFileInfo.ioFlXFndrInfo = *fxInfo;
    err = HSetCatInfo(spec->vRefNum, spec->parID, spec->name, &hfi);
  }
  return (err);
}

/************************************************************************
 * IsAlias - is a file an alias?
 ************************************************************************/
bool IsAlias(FSSpec * spec, FSSpec * newSpec) {
  bool folder, wasIt = false;
  *newSpec = *spec;
  ResolveAliasFile(newSpec, true, &folder, &wasIt); // if error, wasIt will
                                                    // either be set correctly
                                                    // or else still be false
  return (wasIt);
}


/************************************************************************
 * ResolveAliasOrElse - Resolve an alias or fail
 ************************************************************************/
int ResolveAliasOrElse(FSSpec * spec, FSSpec * newSpec, bool *wasIt) {
  bool folder, isAlias = false;
  int err;
  FSSpec resolvedSpec = *spec;
  err = ResolveAliasFile(&resolvedSpec, true, &folder,
                         &isAlias); // if error, isAlias will either
                                    // be set correctly or else still
                                    // be false
  if (wasIt)
    *wasIt = isAlias;
  if (newSpec)
    *newSpec = err ? *spec : resolvedSpec; // if error, put original file there
  return (err);
}

/**********************************************************************
 * SubFolderSpec - get the FSSpec for the signature folder
 **********************************************************************/
int SubFolderSpec(short nameId, FSSpec * spec) {
  char string[256];
  int err;
  static StackHandle specStack;
  CSpec cSpec;
  short i;

  // clear cache?
  if (!spec) {
    if (specStack)
      specStack->elCount = 0;
    return 0;
  }

  // search for folder in cache
  if (specStack)
    for (i = 0; i < specStack->elCount; i++) {
      StackItem(&cSpec, i, (void **)specStack);
      if (cSpec.count == nameId) {
        *spec = cSpec.spec;
        return 0;
      }
    }

  // not in cache.  Go look for it
  err = spec_for(Root.path, (const char *)GetRString(string, nameId), spec);
  if (err == fnfErr) {
    /* Directory doesn't exist — create it */
    g_mkdir_with_parents(spec->path, 0755);
    err = spec_for(Root.path, (const char *)GetRString(string, nameId), spec);
  }
  if (!err) {
    /*
     * maybe the folder is an alias
     */
    IsAlias(spec, spec);

    /*
     * point inside the folder
     */
    spec->parID = SpecDirId(spec);
    *spec->name = 0;

    /*
     * cache it, now that we have it
     */
    if (specStack || !StackInit(sizeof(CSpec), (void ***)&specStack)) {
      cSpec.spec = *spec;
      cSpec.count = nameId;
      StackPush(&cSpec, (void **)specStack);
    }
  }
  return (err);
}

/************************************************************************
 * FindSubFolderSpec - Find our sub folder of a specific system folder
 ************************************************************************/
int FindSubFolderSpec(long domain, long folder, short subfolderID,
                        bool create, FSSpec * spec) {
  FSSpec localSpec;
  int err =
      FindFolder(domain, folder, create, &localSpec.vRefNum, &localSpec.parID);

  if (!err)
    err = SubFolderSpecOf(&localSpec, subfolderID, create, spec);

  return err;
}

/************************************************************************
 * SubFolderSpecOf - find a subfolder of a given fsspec
 ************************************************************************/
int SubFolderSpecOf(FSSpec * inSpec, short subfolderID, bool create,
                      FSSpec * subSpec) {
  char subfolderName[256];

  GetRString((char *)subfolderName, subfolderID);
  return SubFolderSpecOfStr(inSpec, (const char *)subfolderName, create,
                            subSpec);
}

int SubFolderSpecOfStr(FSSpec * inSpec, const char *subfolderName,
                         bool create, FSSpec * subSpec) {
  FSSpec localSpec = *inSpec;
  long dirID;

  g_strlcpy(localSpec.name, subfolderName, sizeof(localSpec.name));
  if (create)
    FSpDirCreate(&localSpec, 0, &dirID);

  IsAlias(&localSpec, &localSpec);
  localSpec.parID = SpecDirId(&localSpec);
  if (localSpec.parID == 0)
    return fnfErr;
  if (subSpec) {
    *localSpec.name = 0;
    *subSpec = localSpec;
  }
  return 0;
}

/************************************************************************
 * StuffFolderSpec - find the stuff folder
 ************************************************************************/
int StuffFolderSpec(FSSpec * spec) {
  FSSpec localSpec;
  char name[256];
  int err = GetFileByRef(AppResFile, &localSpec);

  if (!err) {
    char pdir[1024]; spec_parent(&localSpec, pdir, sizeof(pdir));
    err = spec_for(pdir, (const char *)GetRString(name, STUFF_FOLDER), &localSpec);
  }
  if (!err) {
    IsAlias(&localSpec, &localSpec);
    spec->vRefNum = localSpec.vRefNum;
    spec->parID = SpecDirId(&localSpec);
    *spec->name = 0;
  }
  return err;
}

/************************************************************************
 * SpecInSubfolderOf - is a spec in a folder or subfolder
 ************************************************************************/
bool SpecInSubfolderOf(FSSpec * att, FSSpec * folder) {
  FSSpec parent = *att;

  for (;;) {
    if (SameVRef(parent.vRefNum, folder->vRefNum) &&
        parent.parID == folder->parID)
      return (true);
    if (parent.parID == 2)
      return (false);
    if (ParentSpec(&parent, &parent))
      return (false);
  }
}

/************************************************************************
 * FSMakeFID - make a fileid for a spec
 ************************************************************************/
int FSMakeFID(FSSpec * spec, long *fid) {
  HParamBlockRec fidpb;
  short err;

  Zero(fidpb);

  fidpb.fidParam.ioCompletion = nil;
  fidpb.fidParam.ioNamePtr = (unsigned char *)spec->name;
  fidpb.fidParam.ioVRefNum = spec->vRefNum;
  fidpb.fidParam.ioSrcDirID = spec->parID;

  err = PBCreateFileIDRefSync((HParmBlkPtr)&fidpb);
  FileIDHack();
  if (err == fidExists || err == afpIDExists)
    err = 0; /* ignore; ioFileID is good */

  if (!err)
    *fid = fidpb.fidParam.ioFileID;
  return (err);
}

/************************************************************************
 * FileIDHack - hack to work around apple fileID bug.
 *	 JDB 980720
 *
 *		There's a problem with OS 8.1 machines that create fileIDs for
 *	files saved onto standard HFS partitions.  If a PBSetFInfo is done
 *	too soon afterwards (like happens with SetMod later during attachment
 *	receiving), the fileID can become corrupt and the attachments can;t be
 *	located agan.
 *  SD 8/6/98 - The workaround is to GetCatInfo on a different file.
 ************************************************************************/
void FileIDHack(void) {
  long sysVers = 0;
  long affected = 0;
  int err = 0;
  FSSpec spec;
  CInfoPBRec info;

  // get the system version of this machine
  err = Gestalt(gestaltSystemVersion, &sysVers);
  if (err == 0) {
    // is this system version affected by the bug?
    affected = GetRLong(FILEID_AFFECTED_SYSVERSION);
    if (affected == sysVers) {
      Zero(spec);
      GetFileByRef(SettingsRefN, &spec);
      MyFSpGetCatInfo(&spec, &spec, &info);
    }
  }
}

/************************************************************************
 * FSResolveFID - resolve a vRef & fileid into a spec
 ************************************************************************/
int FSResolveFID(short vRef, long fid, FSSpec * spec) {
  HParamBlockRec fidpb;
  short err;
  char name[256];

  Zero(fidpb);

  *name = 0;
  fidpb.fidParam.ioCompletion = nil;
  fidpb.fidParam.ioNamePtr = (unsigned char *)name;
  fidpb.fidParam.ioFileID = fid;
  fidpb.fidParam.ioVRefNum = vRef;

  err = PBResolveFileIDRefSync((HParmBlkPtr)&fidpb);

  if (!err)
    err = spec_for(NULL, name, spec);

  return (err);
}

/************************************************************************
 * SpecMove - move a file from one place to another
 ************************************************************************/
int SpecMove(FSSpec * moveMe, FSSpec * moveTo) {
  FInfo info;
  /* Try to preserve Finder flags as best-effort on POSIX */
  if (!MyFSpGetFInfo(moveMe, NULL,  &info)) {
    info.fdFlags &= ~fInited;
    Zero(info.fdLocation);
    MyFSpSetFInfo(moveMe, NULL,  &info);
  }

  /* If we have explicit paths, prefer them for a reliable rename */
  if (moveMe && moveTo && moveMe->path[0] && moveTo->path[0]) {
    /* Fast path: atomic rename when possible */
    if (rename(moveMe->path, moveTo->path) == 0)
      return 0;

    /* If rename failed due to cross-device move, attempt GIO move/copy */
    if (errno == EXDEV) {
#ifdef __has_include
#if __has_include(<gio/gio.h>)
      {
        GError *gerr = NULL;
        GFile *src = g_file_new_for_path(moveMe->path);
        GFile *dst = g_file_new_for_path(moveTo->path);

        /* Try a move via GIO which can handle cross-filesystem moves */
        if (g_file_move(src, dst, G_FILE_COPY_NONE, NULL, NULL, NULL, &gerr)) {
          g_object_unref(src);
          g_object_unref(dst);
          return 0;
        }

        /* If move failed, fall back to copy+unlink */
        g_clear_error(&gerr);
        if (g_file_copy(src, dst, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &gerr)) {
          g_object_unref(src);
          g_object_unref(dst);
          /* remove source after successful copy */
          if (unlink(moveMe->path) == 0)
            return 0;
          return ioErr;
        }

        g_object_unref(src);
        g_object_unref(dst);
        g_clear_error(&gerr);
      }
#endif
#endif

      /* Last-resort: do a manual copy and unlink */
      {
        int in = open(moveMe->path, O_RDONLY);
        if (in < 0)
          return ioErr;
        int out = creat(moveTo->path, 0644);
        if (out < 0) {
          close(in);
          return ioErr;
        }
        char buf[8192];
        ssize_t r;
        while ((r = read(in, buf, sizeof(buf))) > 0) {
          ssize_t w = write(out, buf, r);
          if (w != r) {
            close(in);
            close(out);
            return ioErr;
          }
        }
        close(in);
        close(out);
        if (unlink(moveMe->path) == 0)
          return 0;
        return ioErr;
      }
    }

    /* rename failed for other reasons */
    return ioErr;
  }

  /* Fallback: use HMove behaviour for callers that supply names only */
  return HMove(moveMe->vRefNum, moveMe->parID, moveMe->name, moveTo->parID,
               nil);
}

/************************************************************************
 * SpecMoveAndRename - move a file from one place to another, and rename
 ************************************************************************/
int SpecMoveAndRename(FSSpec * moveMe, FSSpec * moveTo) {
  int err;

  if ((err = SpecMove(moveMe, moveTo)))
    return err;

  err = MyFSpRename(moveMe, moveTo->name);

  return err;
}

/************************************************************************
 * DiskSpunUp - is the disk a'spinnin'?
 ************************************************************************/
bool DiskSpunUp(void) {
  // They killed HardDiskPowered -- Those bxxxxxxs!!
  // Even worse, they made it return false when they killed it, not true
  // They should go to hell.  They should go to hell and they should die.
  // if (HardDiskPowered && !HardDiskPowered()) return false;
  return true;
}

/************************************************************************
 * GetTrashSpec - get an FSSpec describing the trash;
 ************************************************************************/
int GetTrashSpec(short vRef, FSSpec * spec) {
  spec->name[0] = 0;
  spec->vRefNum = vRef;
  int vRefInt = vRef;
  int err =
      FindFolder(vRef, kTrashFolderType, kCreateFolder, &vRefInt, &spec->parID);
  if (!err) {
    vRef = (short)vRefInt;
  }
  return err;
}

/************************************************************************
 * DTRef - return the ref number for the desktop db
 ************************************************************************/
int DTRef(short vRef, short *dtRef) { return 0; }

int DTGetAppl(short vRef, short dtRef, OSType creator, FSSpec * appSpec) {
  return fnfErr;
}

/************************************************************************
 * DTFindAppl - find which dtRef an application lives in
 ************************************************************************/
short DTFindAppl(OSType creator) {
  int err;
  short volIndex;
  short vRef;
  short dtRef;
  FSSpec junk;

  for (volIndex = 1; !IndexVRef(volIndex, &vRef); volIndex++)
    if (!(err = DTRef(vRef, &dtRef)))
      if (!(err = DTGetAppl(vRef, dtRef, creator, &junk)))
        return (dtRef);
  return (0);
}

/************************************************************************
 * DTSetComment - set the comment for an attachment
 ************************************************************************/
int DTSetComment(FSSpec * spec, char *comment) {
  DTPBRec pb;
  short dtRef;
  int err;

  if (!HaveTheDiseaseCalledOSX() && !(err = DTRef(spec->vRefNum, &dtRef))) {
    Zero(pb);
    pb.ioNamePtr = (unsigned char *)spec->name;
    pb.ioDTRefNum = dtRef;
    pb.ioDTBuffer = (char *)comment;
    pb.ioDTReqCount = MIN(200, strlen((char *)comment));
    pb.ioDirID = spec->parID;
    return (PBDTSetCommentSync(&pb));
  }
  return (err);
}

/************************************************************************
 * SameSpec - do two specs refer to same file?
 * Portable: prefer path comparison when available, fall back to
 * parID/vRefNum/name for legacy callers that haven't set path yet.
 ************************************************************************/
bool SameSpec(FSSpec * sp1, FSSpec * sp2) {
  if (sp1->path[0] && sp2->path[0])
    return strcmp(sp1->path, sp2->path) == 0;
  return (sp1->parID == sp2->parID && SameVRef(sp1->vRefNum, sp2->vRefNum) &&
          StringSame(sp1->name, sp2->name));
}

/************************************************************************
 * SpecDirId - find the dirId of the directory referenced by a spec
 ************************************************************************/
long SpecDirId(FSSpec * spec) {
  CInfoPBRec hfi;
  FSSpec newSpec;
  char name[256];

  Zero(hfi);
  hfi.hFileInfo.ioNamePtr = name;
  MyFSpGetCatInfo(spec, &newSpec, &hfi);
  return (hfi.hFileInfo.ioDirID);
}

/************************************************************************
 * CanWrite - can we write on a file?  Only one way to tell on a macintosh
 ************************************************************************/
int CanWrite(FSSpec * spec, bool *can) {
  FSSpec newSpec = *spec;
  short refN;
  Byte buff = 13;
  long len;
  CInfoPBRec hfi;
  int err;
  bool b;

  *can = false;
  if (!(err = ResolveAliasFile(&newSpec, true, &b, &b)) &&
      !(err = MyAHGetFileInfo(newSpec.vRefNum, newSpec.parID,
                            (const char *)newSpec.name, &hfi)))
    if (!(err = MyFSpOpenDF(&newSpec, fsRdWrPerm, &refN))) {
      len = 1;
      if (!(err = SetFPos(refN, fsFromLEOF, 0))) {
        if (!FSWrite(refN, &len, &buff)) {
          *can = true;
          SetFPos(refN, fsFromLEOF, -1);
          if (!GetFPos(refN, &len))
            TruncOpenFile(refN, len);
        }
      }
      close(refN);
      MyFSpSetHFileInfo(&newSpec, &hfi); /* restore mod date */
    } else {
      if ((err == permErr || err == afpAccessDenied) &&
          !(err = MyFSpOpenDF(&newSpec, fsRdPerm, &refN))) {
        close(refN);
        *can = false;
      }
    }
  return (err);
}


/**********************************************************************
 * NewTempSpec - make a temp file spec
 **********************************************************************/
int NewTempSpec(short vRef, long dirId, char *name, FSSpec * spec) {
  long tempId;
  int err;
  char fName[256];
  static unsigned char n;

  if ((err = FindTemporaryFolder(vRef, dirId, &tempId, &vRef)))
    return err;

  n++;

  if (name)
    g_strlcpy(fName, name, sizeof(fName));
  else {
    MyNumToString(TickCount(), fName);
    g_strlcat(fName, "+", sizeof(fName));
    PLCat(fName, n);
  }

  spec_make(NULL, fName, spec);
  return (UniqueSpec(spec, 27));
}

/**********************************************************************
 * FindTemporaryFolder - find the Temporary Folder
 *	use spool folder if not available on server
 **********************************************************************/
int FindTemporaryFolder(short vRef, long dirId, long *tempDirId,
                          short *tempVRef) {
  int err = 0;

  //	tell FindFolder to forget everything it knows, and look at the disk
  err = MyInvalidateFolderDescriptorCache(0, 0L);
  ASSERT(0 == err);
  int tempVRefInt;
  err = FindFolder(vRef, kTemporaryFolderType, true, &tempVRefInt, tempDirId);
  if (!err)
    *tempVRef = (short)tempVRefInt;

#ifdef DEBUG
  // Some versions of OS X will return 0 and a bad value for the temp
  // folder if it existed once, but does no longer. So, we check to see if it
  // exists.
  //	*** Darn their eyes! ***
  if (0 == err) {
    CInfoPBRec pb;

    Zero(pb);
    pb.dirInfo.ioFDirIndex = -1; // use ioVRefNum and ioDrDirID only
    pb.dirInfo.ioVRefNum = *tempVRef;
    pb.dirInfo.ioDrDirID = *tempDirId;
    err = PBGetCatInfoSync(&pb);
    ASSERT(0 == err);
  }
#endif

  // If findfolder fails, or if it returns a different volume, use
  // the spool folder
  if (err || *tempVRef != vRef) {
    err = 0;
    *tempVRef = vRef;
    if (dirId)
      *tempDirId = dirId;
    else {
      FSSpec netbootSucksSpec;

      if (SubFolderSpec(SPOOL_FOLDER, &netbootSucksSpec) ||
          netbootSucksSpec.vRefNum != vRef)
        *tempDirId = 2;
      else
        *tempDirId = netbootSucksSpec.parID;
    }
  }
  return (err);
}

/**********************************************************************
 *
 **********************************************************************/
int AddUniqueExt(FSSpec * spec, short extId) {
  FSSpec newSpec;
  short n = 0;
  char extStr[256], nStr[256];
  int err;

  g_strlcpy(extStr, ".", sizeof(extStr));
  PCatR(extStr, extId);
  *nStr = 0;

  for (;;) {
    newSpec = *spec;
    *newSpec.name = MIN(*newSpec.name, 31 - *extStr - *nStr);
    if (n++)
      g_strlcat(newSpec.name, nStr, 256);
    g_strlcat(newSpec.name, extStr, 256);
    if ((err = FSpExists(&newSpec)))
      break;
    MyNumToString(n, nStr);
  }

  if (err == fnfErr) {
    err = MyFSpRename(spec, newSpec.name);
    if (!err)
      g_strlcpy(spec->name, newSpec.name, sizeof(spec->name));
  }

  return (err);
}

/**********************************************************************
 * NewTempSpec - make a temp file spec
 **********************************************************************/
int NewTempExtSpec(short vRef, char *name, short extId, FSSpec * spec) {
  long dirId;
  int err = FindTemporaryFolder(vRef, 0L, &dirId, &vRef);
  char fName[256];
  static short n;

  if (err)
    return (err);

  do {
    n++;
    if (n > REAL_BIG)
      n = 0;

    if (name)
      g_strlcpy(fName, name, sizeof(fName));
    else {
      MyNumToString(TickCount(), fName);
      g_strlcat(fName, "+", sizeof(fName));
      PLCat(fName, n);
    }
    g_strlcat(fName, ".", sizeof(fName));
    PCatR(fName, extId);

  } while (!spec_for(NULL, fName, spec));
  if (err == fnfErr)
    err = 0;
  return (err);
}


/* SimpleMakeFSSpec - DEPRECATED, use spec_make() instead */

/************************************************************************
 * FindMyFile - Like FindFile, only Eudora-related
 ************************************************************************/
int FindMyFile(FSSpec * spec, long whereToLook, short fileName) {
  FSSpec mySpec;
  int err = fnfErr;
  char nameStr[256];

  if (whereToLook & kStuffFolderBit) {
    if (!(err = GetFileByRef(AppResFile, &mySpec))) {
      char pdir[1024]; spec_parent(&mySpec, pdir, sizeof(pdir));
      if (!(err = spec_for(pdir,
                           (const char *)GetRString(nameStr, STUFF_FOLDER), &mySpec))) {
        IsAlias(&mySpec, &mySpec);
        if (!(err = spec_for(mySpec.path,
                             (const char *)GetRString(nameStr, fileName), &mySpec))) {
          IsAlias(&mySpec, &mySpec);
          *spec = mySpec;
          return 0;
        }
      }
    }
  }

  return err;
}

#ifdef DEBUG
/* Bottleneck variants defined in fileutil.h */
#endif

/************************************************************************
 * MakeUniqueUntitledSpec - Make a unique "untitled" name in some folder
 ************************************************************************/
void MakeUniqueUntitledSpec(const char *dir, short strResID,
                            FSSpec *spec)

{
  char name[256], s[256];
  long suffix;

  //	Make a unique "untitled" name
  GetRString(name, strResID);
  suffix = 2;
  while (!spec_for(dir, name, spec)) {
    GetRString(name, strResID); // Reset to base name
    MyNumToString(suffix++, s);
    g_strlcat(name, " ", 256);
    g_strlcat(name, s, 256);
  }
}

int MisplaceItem(FSSpec *spec)

{
  FSSpec misplacedFolder, exist, newExist;
  int theError;
  long dirID;

  // Find the Misplaced Items folder
  if ((theError = SubFolderSpec(MISPLACED_FOLDER, &misplacedFolder))) {
    spec_make(Root.path,
              (const char *)GetRString((char *)misplacedFolder.name, MISPLACED_FOLDER),
              &misplacedFolder);
    theError = FSpDirCreate(&misplacedFolder, 0, &dirID);
    if (!theError)
      misplacedFolder.parID = dirID;
  }
  if (!theError) {
    IsAlias(&misplacedFolder, &misplacedFolder);
    if (!spec_for(misplacedFolder.path, spec->name, &exist)) {
      newExist = exist;
      UniqueSpec(&newExist, 31);
      MyFSpRename(&exist, newExist.name);
    }
    theError = SpecMove(spec, &misplacedFolder);
  }
  return (theError);
}

int FSpGetLongName(FSSpec *spec, TextEncoding destEncoding, char *longName) {
  int err = 0;
  HFSUniStr255 uniName;

  if (destEncoding == kTextEncodingUnknown)
    destEncoding = CreateTextEncoding(kTextEncodingMacRoman, 0, 0);

  //	Get the unicode name
  err = FSpGetLongNameUnicode(spec, &uniName);
  if (err == 0) {
    UnicodeToTextInfo info;

    //	Convert the name back to UTF-8 or something
    err = CreateUnicodeToTextInfoByEncoding(destEncoding, &info);
    if (err == 0) {
      // err = ConvertFromUnicodeToPString ( info, uniName.length * sizeof (
      // UniChar ), uniName.unicode, longName );
      long charsUsed = 0, charsOut = 0;
      err = ConvertFromUnicodeToText(
          info, uniName.length * sizeof(UniChar), uniName.unicode,
          kUnicodeUseFallbacksMask | kUnicodeLooseMappingsMask, 0, nil, nil,
          nil, 127, &charsUsed, &charsOut, longName + 1);
      *longName = charsOut;
      ASSERT(!err || err == kTECUsedFallbacksStatus);
      if (*longName)
        err = 0; // if we got something, use it

      (void)DisposeUnicodeToTextInfo(&info);
    }
  }

  return err;
}

int FSpGetLongNameUnicode(FSSpec *spec, HFSUniStr255 *longName) {
  int err = 0;
  FSRef aRef;

  err = FSpMakeFSRef(spec, &aRef);
  if (err == 0) {
    err = FSGetCatalogInfo(&aRef, kFSCatInfoNone, NULL, longName, NULL, NULL);
  }

  return err;
}

int FSpSetLongName(FSSpec *spec, TextEncoding srcEncoding,
                     const char *longName, FSSpec *newSpec) {
  int err = 0;
  HFSUniStr255 uniName;
  TextToUnicodeInfo info;

  if (srcEncoding == kTextEncodingUnknown)
    srcEncoding = CreateTextEncoding(kTextEncodingMacRoman, 0, 0);

  err = CreateTextToUnicodeInfoByEncoding(srcEncoding, &info);
  if (err == 0) {
    ByteCount uniStrLen;
    err = ConvertFromPStringToUnicode(info, longName, 255 * sizeof(UniChar),
                                      &uniStrLen, uniName.unicode);
    uniName.length = uniStrLen / 2;
    if (err == 0)
      err = FSpSetLongNameUnicode(spec, &uniName, newSpec);
    DisposeTextToUnicodeInfo(&info);
  }
  return err;
}

int FSpSetLongNameUnicode(FSSpec *spec, ConstHFSUniStr255Param longName,
                            FSSpec *newSpec) {
  int err = 0;
  FSRef aRef;

  err = FSpMakeFSRef(spec, &aRef);
  if (err == 0) {
    FSRef newRef;
    FSRef *refPtr = newSpec != NULL ? &newRef : NULL;
    err = FSRenameUnicode(&aRef, longName->length, longName->unicode,
                          kTextEncodingUnicodeDefault, refPtr);
    //	Convert the FSRef back into a FSSpec for the caller
    if (err == 0 && refPtr != NULL)
      (void)FSGetCatalogInfo(refPtr, kFSCatInfoNone, NULL, NULL, newSpec, NULL);
  }

  return err;
}

int MakeUniqueLongFileName(short vRefNum, long dirID, char *name,
                           TextEncoding srcEncoding, short maxLen) {
  char base[256], suffix[256], tryName[1024];
  long nextFile = 1;

  ASSERT(name != NULL);
  ASSERT(maxLen >= 63);

  // Split the file name into base name and suffix
  SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(name, base,
                                                             suffix, maxLen);

  while (true) {
    snprintf(tryName, sizeof(tryName), "%s %ld.%s", base, nextFile, suffix);

    // See if the file exists - using a simple path check for POSIX
    char checkPath[1024];
    snprintf(checkPath, sizeof(checkPath), "./%s", tryName);

    if (access(checkPath, F_OK) != 0) {
      // Found a name that doesn't exist
      g_strlcpy(name, tryName, maxLen);
      return 0;
    }

    if (++nextFile > 1000)
      return ioErr;
  }
}

/**********************************************************************
 * The following block of functions is intended to allow Eudora to wait
 * for access to its files, in case other programs are using them.  It will
 * wait up to a certain number of seconds (currently 10) for files
 * to become free.  It waits if it gets opWrErr or permErr.  This latter
 * is what most calls seem to return for busy files, though you'd think
 * opWrErr would be more appropriate.  Unfortunately, permErr is what
 * you also get for a locked file, so if we get it we have to rule it out.
 * We get afpAccessDenied for permission errors, even local unix ones,
 * so at least we don't have to worry about that
 **********************************************************************/

// Redundant persistent open functions removed for POSIX compliance

bool FSpIsLocked(FSSpec * spec) {
  CInfoPBRec cfi;
  if (!HGetCatInfo(spec->vRefNum, spec->parID, spec->name, &cfi)) {
    return (cfi.hFileInfo.ioFlAttrib & kioFlAttribLockedMask) != 0;
  }
  return false;
}

/**********************************************************************
 * end file wait functions
 **********************************************************************/

/**********************************************************************
 * IsPDFFile - is a spec a pdf file?
 **********************************************************************/
bool IsPDFFile(FSSpec * spec, OSType fileType) {
  if (fileType == 0x50444620)
    return true;
  // Avi must die.
  if (EndsWithR((unsigned char *)spec->name, PDF_QUOTE_EXTENSION_UNQUOTE))
    return true;
  return false;
}

/**********************************************************************
 * SpecEndsWithExtensionR - does a spec (long name) end with an extension
 *  on a list?
 **********************************************************************/
bool SpecEndsWithExtensionR(FSSpec * spec, short resID) {
  char longName[256];

  if (FSpGetLongName(spec, 0, longName))
    g_strlcpy(longName, spec->name, 256);

  return EndsWithItem((unsigned char *)longName, resID);
}
int FSRenameUnicode(FSRef *ref, int len, const void *name, int encoding,
                      FSRef *newRef) {
  return 0;
}
int PBMakeFSRefUnicodeSync(void *pb) { return 0; }
/* EndsWithR and EndsWithItem are implemented in stringutil.c */

/**********************************************************************
 * GetAttFolderSpec - get the attachment folder spec
 * Stub implementation - returns AttFolderSpec global
 **********************************************************************/
int GetAttFolderSpec(FSSpec * spec) {
  extern FSSpec AttFolderSpec;
  if (spec) {
    *spec = AttFolderSpec;
    return 0;
  }
  return paramErr;
}

/**********************************************************************
 * GetCurrentAttFolderSpec - get the current attachment folder spec
 * Stub implementation - returns CurrentAttFolderSpec global
 **********************************************************************/
void GetCurrentAttFolderSpec(FSSpec * spec) {
  extern FSSpec CurrentAttFolderSpec;
  if (spec) {
    *spec = CurrentAttFolderSpec;
  }
}

/**********************************************************************
 * TypeIsOnListWhereAndIndex - check if file type is on executable list
 * Stub implementation - always returns false for safety
 **********************************************************************/
bool TypeIsOnListWhereAndIndex(long type, short list, void *ptr, short *index) {
  // For safety, always return false - don't treat any files as executable
  if (index) *index = 0;
  return false;
}

/**********************************************************************
 * PtrAndHand - append ptr[0..size-1] to a Handle (malloc-backed)
 **********************************************************************/
int PtrAndHand(const void *ptr, void **hand, long size) {
  if (!ptr || !hand || size <= 0) return -50; /* paramErr */
  size_t oldSize = *hand ? GetHandleSize(hand) : 0;
  void *resized = realloc(*hand, oldSize + (size_t)size);
  if (!resized) return -108; /* memFullErr */
  memmove((char *)resized + oldSize, ptr, (size_t)size);
  *hand = resized;
  return 0; /* 0 */
}


/* GrowBuf helpers removed — avoid adding new abstractions; use
 * direct malloc/realloc/free in callers for portability. */
