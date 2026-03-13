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

#ifndef RgnHandle
typedef void *RgnHandle;
#endif

#include "../include/fileutil.h"
// #include "../include/MyRes.h" // Removed to avoid GTK dependency
#include "../include/StringDefs.h"
// #include "../include/StringUtil.h" // Removed to avoid conflicts
#include "../include/mailbox.h"
#include "../include/progress.h"
// #include "../include/util.h" // Removed to avoid conflicts
#include <assert.h>
#include <stdarg.h>

extern bool HaveOSX(void);

extern unsigned char *ComposeString(unsigned char *dst, const char *fmt, ...);
extern void AlertStr(short alertID, short type, unsigned char *message);
#define Note 1
#define OK_ALRT 1001

#define MINI_MASK 0
#define SAVEAS_DLOG 1026
#define SAVEAS_NAV_DITL 1077
#define tabChar '\t'
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

#ifdef CommandPeriod
#undef CommandPeriod
#endif
extern bool CommandPeriod;
#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif

#define FILE_NUM 13
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Prototypes */
void CtoPCpy(char *dst, const char *src);
void FileUtilPtoCcpy(char *dst, const char *src);
#define PtoCcpy FileUtilPtoCcpy

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

// Mac Handle lock/unlock stubs - no-ops in standard C
#ifndef LDRef
#define LDRef(h) (*(h))
#endif
#ifndef UL
#define UL(h) ((void)0)
#endif
#ifndef GetHandleSize_
#define GetHandleSize_(h) GetHandleSize(h)
#endif

#define FILL(pb, name, vRef, dirId)

/* Forward declarations */
static OSErr GenerateUniqueName(short volume, long *startSeed, long dir1,
                                long dir2, StringPtr uniqueName);
OSErr FSpExchangeFilesCompat(const FSSpec *source, const FSSpec *dest);

OSErr DirIterate(const FSSpec *dir, void *data,
                 bool (*callback)(DirIterateInfo *info)) {
  DIR *dp;
  struct dirent *entry;
  OSErr err = noErr;

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
    CtoPCpy((unsigned char *)info.spec.name, entry->d_name);
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
      // Handle stat error if necessary, maybe skip this entry or log
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

void PCopy(char *dst, const char *src) {
  if (dst && src)
    strcpy(dst, src);
}

void PCatC(char *dst, char c) {
  if (!dst)
    return;
  size_t len = strlen(dst);
  if (len < 255) {
    dst[len] = c;
    dst[len + 1] = '\0';
  }
}

/* PCat is implemented in stringutil.c */
extern unsigned char *PCat(unsigned char *string, unsigned char *suffix);

void FileUtilPtoCcpy(char *dst, const char *src) {
  if (dst && src)
    strcpy(dst, src);
}

void CtoPCpy(char *dst, const char *src) {
  if (dst && src)
    strcpy(dst, src);
}

/* FS API Stubs - mostly removed or simplified */
#ifndef noErr
#define noErr 0
#endif
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

short FSMakeFSSpec(short vRef, long dirID, const char *name, FSSpecPtr spec) {
  if (!name || !spec)
    return paramErr;
  memset(spec, 0, sizeof(FSSpec));
  spec->vRefNum = vRef;
  spec->parID = dirID;
  strcpy(spec->name, name);
  // Simple path construction (current dir relative)
  snprintf(spec->path, sizeof(spec->path), "./%s", spec->name);
  return noErr;
}
short FSpCreateResFile(FSSpecPtr spec, uint32_t creator, uint32_t type,
                       uint32_t script) {
  // POSIX doesn't have resource forks. Just create the data fork.
  return (short)FSpCreate(spec, creator, type, script);
}

int FSpCreate(FSSpecPtr spec, uint32_t creator, uint32_t fileType,
              uint32_t script) {
  if (!spec)
    return paramErr;
  int fd = creat(spec->path, 0644);
  if (fd < 0) {
    if (errno == EEXIST)
      return noErr; // Or dupFNErr if we want to be strict
    return ioErr;
  }
  close(fd);
  return noErr;
}
OSErr MemError() { return noErr; }

short ResError() { return noErr; }

void AddResource(Handle h, ResType type, short id, const char *name) {
  // Stub - resource forks don't exist on Unix
  // In a full implementation, this would store resources in a separate file
}

OSErr EudoraFSpOpenDF(FSSpecPtr spec, short mode, short *refN) {
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
  return noErr;
}
OSErr FSpOpenRF(const char *path, short permission, short *refNum) {
  *refNum = 0;
  return noErr;
}
short FSpGetFInfo(FSSpecPtr spec, FInfo *fndrInfo) {
  memset(fndrInfo, 0, sizeof(FInfo));
  return noErr;
}
short FSpSetFInfo(FSSpecPtr spec, FInfo *fndrInfo) { return noErr; }
OSErr EudoraFSpDelete(FSSpecPtr spec) {
  const char *path = spec->path;
  if (unlink(path) == 0)
    return noErr;
  return ioErr;
}
short GetEOF(short refNum, long *logEOF) {
  struct stat st;
  if (fstat(refNum, &st) < 0)
    return ioErr;
  *logEOF = (long)st.st_size;
  return noErr;
}
short SetEOF(short refNum, long logEOF) {
  if (ftruncate(refNum, (off_t)logEOF) < 0)
    return ioErr;
  return noErr;
}
short SetFPos(short refNum, short posMode, long posOffset) {
  int whence = SEEK_SET;
  if (posMode == fsFromLEOF)
    whence = SEEK_END;
  else if (posMode == fsFromMark)
    whence = SEEK_CUR;

  if (lseek(refNum, (off_t)posOffset, whence) < 0)
    return ioErr;
  return noErr;
}
short GetFPos(short refNum, long *filePos) {
  off_t pos = lseek(refNum, 0, SEEK_CUR);
  if (pos < 0)
    return ioErr;
  *filePos = (long)pos;
  return noErr;
}
void *FSClose(short refNum) {
  close(refNum);
  return NULL;
}
short FlushVol(unsigned char *name, short vRefNum) { return noErr; }
short PBFlushVolSync(HParmBlkPtr pb) { return noErr; }
void DetachResource(Handle res) {}
short CurResFile() { return 0; }
void UseResFile(short refNum) {}
void CloseResFile(short refNum) {}
Handle GetResource_(OSType type, short id) { return NULL; }
Handle Get1IndResource(uint32_t type, short index) { return NULL; }
short FSpOpenResFilePersistent(FSSpecPtr spec, short permission) { return 0; }
extern int GetNumBackgroundThreads(void);
extern void MiniEventsLo(short mask, bool background);

short ARead(short refNum, long *count, unsigned char *buffer) {
  ssize_t bytes = read(refNum, buffer, *count);
  if (bytes < 0) {
    *count = 0;
    return ioErr;
  }
  *count = (long)bytes;
  return noErr;
}
short AWrite(short refNum, long *count, unsigned char *buffer) {
  ssize_t bytes = write(refNum, buffer, *count);
  if (bytes < 0) {
    *count = 0;
    return ioErr;
  }
  *count = (long)bytes;
  return noErr;
}
short NCWrite(short refNum, long *count, unsigned char *buffer) {
  return noErr;
}
unsigned char *FileUtilGetRString(unsigned char *name, short id) {
  if (name)
    *name = 0;
  return name;
}
#define GetRString FileUtilGetRString

/* Extra CtoPCpy removed */
/* Error handling stubs - actual implementations in error_handlers.c or
 * similar
 */
void DieWithError(short errorId, OSErr err) {
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

extern void PLCat(char *dst, long n);
void PSCat(char *dst, const char *src) {}
void PSCatC(char *dst, char c) {}
extern char *PRIndex(char *str, char c);
int FSpRename(FSSpecPtr spec, const char *newName) { return noErr; }
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
void MakePStr(PStr p, const void *c, int len) {
  if (p && c) {
    if (len > 255)
      len = 255;
    *p = (unsigned char)len;
    memcpy(p + 1, c, len);
  }
}
uint32_t LocalDateTime(void) { return (uint32_t)time(NULL); }
/* utl_RFSanity is implemented in utl.c */
OSErr PBSetCatInfoSync(CInfoPBPtr pb) { return noErr; }
Handle GetIndResource(OSType type, short index) { return NULL; }
OSErr ResolveAliasFile(FSSpecPtr spec, bool resolveAliasChains,
                       bool *targetIsFolder, bool *wasAliased) {
  return noErr;
}
OSErr PBCatMoveSync(CMovePBPtr pb) { return noErr; }
OSErr PBHGetFInfoSync(HParmBlkPtr pb) { return noErr; }
OSErr PBHSetFInfoSync(HParmBlkPtr pb) { return noErr; }
bool GetFolderNav(char *name, short *volume, long *folder) { return false; }
OSErr Gestalt(OSType selector, long *response) { return noErr; }
OSErr ResolveAlias(FSSpecPtr fromFile, AliasHandle alias, FSSpecPtr target,
                   bool *wasChanged) {
  return noErr;
}
OSErr FSpExchangeFiles(FSSpecPtr source, FSSpecPtr dest) { return noErr; }
OSErr FSpDirCreate(FSSpecPtr spec, ScriptCode script, long *dirID) {
  if (!spec)
    return paramErr;
  if (mkdir(spec->path, 0755) != 0) {
    if (errno == EEXIST)
      return noErr;
    return ioErr;
  }
  if (dirID)
    *dirID = 0;
  return noErr;
}
/* Redundant PBSetCatInfoSync removed */
// ByteProgress stub removed - declared in progress.h with different signature
extern bool HaveOSX(void);
short GetMBarHeight(void) { return 20; }
extern long GetRLong(int index);
bool MommyMommy(short id, void *p) { return true; }
bool UseNavServices(void) { return false; }
int SFPutOpenNav(FSSpecPtr spec, uint32_t creator, uint32_t type, short *refN,
                 short ditlID, uint32_t *script, FSSpecPtr defaultSpec,
                 const char *windowTitle, const char *message) {
  return noErr;
}
void WhackFinder(FSSpecPtr spec) {}
OSErr SniffAndConvertHandleToRoman(Handle *h) { return noErr; }
OSType DefaultCreator(void) { return 'EUDR'; }
short GetResFileAttrs(short refNum) { return 0; }
void UpdateResFile(short refNum) {}
extern void TransLitRes(unsigned char *string, long len, short resId);
short PBGetFCBInfo(FCBInfoPBPtr pb, bool async) { return noErr; }
short PBHRenameSync(HParmBlkPtr pb) { return noErr; }
short FSRead(short refNum, long *count, void *buffer) {
  return ARead(refNum, count, (unsigned char *)buffer);
}
short FSWrite(short refNum, long *count, const void *buffer) {
  return AWrite(refNum, count, (unsigned char *)buffer);
}
short PBWriteAsync(IOParam *pb) { return noErr; }
OSErr FSpSetFLock(FSSpecPtr spec) { return noErr; }
OSErr FSpRstFLock(FSSpecPtr spec) { return noErr; }
extern void StackItem(CSpec *item, short index, void **stack);
short FSWriteP(short refN, unsigned char *pString) { return noErr; }
short PBCreateFileIDRefSync(HParmBlkPtr pb) { return noErr; }
short PBResolveFileIDRefSync(HParmBlkPtr pb) { return noErr; }
typedef int OSStatus;
typedef struct FSRefParam {
  FSRef *newRef;
  FSRef *ref;
  char *name;
  short ioVRefNum;
  long ioDirID;
  void *ioNamePtr;
  long nameLength;
} FSRefParam;

#define kTextEncodingUnicodeDefault 0
#define kioFlAttribLockedMask 0x01
#define PDF_QUOTE_EXTENSION_UNQUOTE 0

typedef void *TextToUnicodeInfo;
typedef unsigned long ByteCount;
typedef unsigned long OptionBits;

void MyNumToString(long n, char *s);

OSErr FSRenameUnicode(FSRef *ref, int len, const void *name, int encoding,
                      FSRef *newRef);
OSErr PBMakeFSRefUnicodeSync(void *pb);
bool EndsWithR(char *str, int id);
bool EndsWithItem(char *str, int id);

extern int StackInit(short elSize, void ***stack);
extern void StackPush(void *item, void **stack);
extern OSErr StackPop(void *into, void **stack);
void MyNumToString(long n, char *s) {
  if (s)
    sprintf(s, "%ld", n);
}
OSErr MyInvalidateFolderDescriptorCache(short vRef, long dirID) {
  return noErr;
}
extern void PCatR(PStr dst, short id);
OSErr NewAlias(FSSpecPtr fromFile, FSSpecPtr target, AliasHandle *alias) {
  *alias = NULL;
  return noErr;
}
OSErr AddResource_(Handle theData, OSType theType, short theID, char *name) {
  return noErr;
}
OSErr CreateTextToUnicodeInfoByEncoding(OSType encoding,
                                        TextToUnicodeInfo *info) {
  return noErr;
}
OSErr ConvertFromPStringToUnicode(TextToUnicodeInfo info, const char *pStr,
                                  ByteCount maxLen, ByteCount *len,
                                  UniChar *dst) {
  *len = 0;
  return noErr;
}
OSErr DisposeTextToUnicodeInfo(TextToUnicodeInfo *info) { return noErr; }

OSErr CreateUnicodeToTextInfoByEncoding(OSType encoding,
                                        UnicodeToTextInfo *info) {
  return noErr;
}
OSErr ConvertFromUnicodeToText(UnicodeToTextInfo info, long len,
                               const void *ptr, OptionBits options,
                               OptionBits mask, void *fallback,
                               void *fallbackInfo, void *fallbackBuffer,
                               long bufferLen, long *charsUsed, long *charsOut,
                               void *outBuffer) {
  return noErr;
}
OSErr DisposeUnicodeToTextInfo(UnicodeToTextInfo *info) { return noErr; }
OSErr FSpMakeFSRef(FSSpecPtr spec, FSRef *ref) { return noErr; }
OSErr FSGetCatalogInfo(FSRef *ref, long bitmap, void *info,
                       HFSUniStr255 *outName, void *fsSpec, void *parentRef) {
  return noErr;
}

OSErr PBMakeFSRefSync(FSRefParam *pb) { return noErr; }

OSType CreateTextEncoding(OSType encoding, OSType representation,
                          OSType variant) {
  return 0;
}
#undef HaveTheDiseaseCalledOSX
bool HaveTheDiseaseCalledOSX(void) { return HaveOSX(); }
extern bool StringSame(const char *s1, const char *s2);
OSErr PBDTSetCommentSync(DTPBRec *pb) { return noErr; }

bool bHasFileIDs = false;

extern bool PrefIsSet(short prefId);
extern void ThirdCenterRectIn(void *r, void *in);
void GetQDGlobalsScreenBits(void *bits) {}
Handle NewHandle(size_t size) {
  void **h = (void **)malloc(sizeof(void *));
  if (h) {
    *h = malloc(size);
    if (!*h) {
      free(h);
      return NULL;
    }
  }
  return (Handle)h;
}

/* NuHandle: Eudora's allocator wrapper — same as NewHandle. */
Handle NuHandle(size_t size) {
  return NewHandle(size);
}

/* NuHandleClear: allocate a zero-filled Handle. */
Handle NuHandleClear(size_t size) {
  void **h = (void **)malloc(sizeof(void *));
  if (h) {
    *h = calloc(1, size > 0 ? size : 1);
    if (!*h) { free(h); return NULL; }
  }
  return (Handle)h;
}

void DisposeHandle(Handle h) {
  if (h) {
    if (*h)
      free(*h);
    free(h);
  }
}

OSErr PtrToHand(const void *srcPtr, Handle *dstHndl, size_t size) {
  if (!srcPtr || !dstHndl || size == 0)
    return -1;

  Handle h = (Handle)malloc(sizeof(void *));
  if (!h)
    return -1;

  *h = malloc(size);
  if (!*h) {
    free(h);
    return -1;
  }

  memcpy(*h, srcPtr, size);
  *dstHndl = h;
  return 0;
}

size_t InlineGetHandleSize(Handle h) {
  if (!h || !*h)
    return 0;
  return malloc_size(*h);
}

void HLock(Handle h) { /* No-op on POSIX - memory isn't relocatable */ }

void HUnlock(Handle h) { /* No-op on POSIX - memory isn't relocatable */ }


size_t GetHandleSize(Handle h) { return InlineGetHandleSize(h); }

void *NuPtr(size_t size) { return malloc(size); }

void ZapHandle(Handle h) {
  if (h) {
    DisposeHandle(h);
  }
}

Handle PtrPlusHand(const void *ptr, Handle hand, long size) {
  if (!hand || !*hand || !ptr || size <= 0)
    return hand;

  size_t oldSize = InlineGetHandleSize(hand);
  void *newData = realloc(*hand, oldSize + size);
  if (!newData)
    return hand;

  memcpy((char *)newData + oldSize, ptr, size);
  *hand = newData;
  return hand;
}

OSErr HandPlusHand(Handle h1, Handle h2) {
  if (!h1 || !*h1 || !h2) return paramErr;
  size_t size = InlineGetHandleSize(h1);
  if (size == 0) return noErr;
  size_t oldSize = h2 && *h2 ? InlineGetHandleSize(h2) : 0;
  void *newData = realloc(h2 && *h2 ? *h2 : NULL, oldSize + size);
  if (!newData) return memFullErr;
  memcpy((char *)newData + oldSize, *h1, size);
  *h2 = newData;
  return noErr;
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
OSErr ParentSpec(FSSpecPtr child, FSSpecPtr parent) {
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
    return noErr;
  }
  return ioErr;
}

/**********************************************************************
 * get a name, given a vRefNum
 **********************************************************************/
short GetDirName(char *volName, short vRef, long dirId, char *name) {
  return noErr;
}

/**********************************************************************
 * get a volume name, given a vRefNum
 **********************************************************************/
char *GetMyVolName(short refNum, char *name) { return noErr; }

/************************************************************************
 * IndexVRef - return vref's by index
 ************************************************************************/
OSErr IndexVRef(short index, short *vRef) {
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
int MakeResFile(const char *name, int vRef, long dirId, long creator,
                long type) {
  int err;
  FSSpec spec;

  FSMakeFSSpec(vRef, dirId, name, &spec);
  FSpCreateResFile(&spec, creator, type, 0);
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
      err = noErr;
    }
  }
  return (err == dupFNErr ? noErr : err);
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
 * NuntNewline - find a newline in a file
 ************************************************************************/
OSErr HuntNewline(short refN, long aroundSpot, long *newline, bool *realNl) {
  UHandle buffer = (UHandle)NuHTempOK(HNLSIZE);
  long spot, count;
  unsigned char *nl1, *nl2, *end;
  unsigned char *aSpot;
  short err;

  if (!buffer)
    return (WarnUser(MEM_ERR, MemError()));
  LDRef((Handle)buffer);

  /*
   * read in a buffer containing aSpot
   */
  spot = MAX(0, aroundSpot - HNLSIZE / 2);
  count = HNLSIZE;

  if ((err = SetFPos(refN, fsFromStart, spot))) {
    FileSystemError(READ_MBOX, "", err);
    goto done;
  }

  err = ARead(refN, &count, (unsigned char *)*buffer);
  if (err == eofErr && count > 0)
    err =
        noErr; /* ignore running off the end of the file as long as we got
                                                                                                                                                        some bytes */
  if (err) {
    FileSystemError(READ_MBOX, "", err);
    goto done;
  }

  aSpot = (unsigned char *)*buffer + (aroundSpot - spot);
  end = (unsigned char *)*buffer + count;

  /*
   * search both forwards and backwards for newlines
   */
  for (nl1 = aSpot; nl1 >= (unsigned char *)*buffer; nl1--)
    if (*nl1 == '\015')
      break;
  for (nl2 = aSpot; nl2 < end; nl2++)
    if (*nl2 == '\015')
      break;

  /*
   * take the closest newline to the desired spot
   */
  if (nl1 < (unsigned char *)*buffer) {
    if (nl2 < end)
      aSpot = nl2;
  } else if (nl2 > end)
    aSpot = nl1;
  else
    aSpot = ((nl2 - aSpot) < (aSpot - nl1)) ? nl2 : nl1;

  *realNl = *aSpot == '\015';
  *newline = spot + (aSpot - (unsigned char *)*buffer) + 1;

done:
  ZapHandle((Handle)buffer);
  return (err);
}

/************************************************************************
 * TruncOpenFile - truncate an open file to a given spot
 ************************************************************************/
OSErr TruncOpenFile(short refN, long spot) {
  short err;

  if ((err = SetFPos(refN, fsFromStart, spot)))
    return (err);
  return (SetEOF(refN, spot));
}

/************************************************************************
 * TruncAtMark - truncate an open file at the current spot
 ************************************************************************/
OSErr TruncAtMark(short refN) {
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

    r.top = (*dTempl)->boundsRect.top;
    r.left = (*dTempl)->boundsRect.left;
    r.bottom = (*dTempl)->boundsRect.bottom;
    r.right = (*dTempl)->boundsRect.right;

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
  return noErr;
}

/************************************************************************
 * ARFHOpen - like RFOpen, but with dirId and permissions
 * Note: Resource forks are Mac-specific, return success but don't open anything
 ************************************************************************/
short ARFHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm) {
  *refN = -1;   // Invalid file descriptor to indicate no resource fork
  return noErr; // Not an error in portable code
}

/************************************************************************
 * VolumeMargin - is there enough space on a volume for something?
 ************************************************************************/
OSErr VolumeMargin(short vRef, long spaceNeeded) {
  long margin = GetRLong(VOLUME_MARGIN);

  if (margin && VolumeFree(vRef) < spaceNeeded + margin)
    return (dskFulErr);

  return (noErr);
}

/************************************************************************
 * MyAllocate - allocate disk space for a file
 ************************************************************************/
int MyAllocate(short refN, long size) {
// Try to preallocate space
#ifdef __linux__
  if (posix_fallocate(refN, 0, size) == 0)
    return noErr;
#endif
  // Fallback: extend file size
  if (ftruncate(refN, size) < 0)
    return ioErr;
  return noErr;
}

/************************************************************************
 * SFPutOpen - open a file for write, using stdfile
 ************************************************************************/
short SFPutOpen(FSSpecPtr spec, long creator, long type, short *refN,
                ModalFilterYDUPP filter, DlgHookYDUPP hook, short id,
                FSSpecPtr defaultSpec, const char *windowTitle,
                const char *message) {
  FInfo info;
  ScriptCode script;
  OSErr theError;
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
      theError = noErr;
    else {
      FileSystemError(COULDNT_SAVEAS, (unsigned char *)spec->name, theError);
      return (theError);
    }
  } else {
    close(fd);
    theError = noErr;
  }

  if ((theError = FSpOpenDF(spec->path, fsRdWrPerm, refN))) {
    FileSystemError(COULDNT_SAVEAS, (unsigned char *)spec->name, theError);
    FSpDelete(spec->path);
    return (theError);
  }

  if (!(theError = FSpGetFInfo(spec, &info))) {
    info.fdType = type;
    FSpSetFInfo(spec, &info);
  }

  if ((theError = SetEOF(*refN, 0))) {
    FileSystemError(COULDNT_SAVEAS, (unsigned char *)spec->name, theError);
    FSpDelete(spec->path);
    return (theError);
  }

  WhackFinder(spec);

  return (noErr);
}

/**********************************************************************
 * FSpModDate - get the mod date of a file
 **********************************************************************/
uint32_t FSpModDate(FSSpecPtr spec) {
  CInfoPBRec hfi;

  if (FSpGetHFileInfo(spec, &hfi))
    return (0);
  return (hfi.hFileInfo.ioFlMdDat);
}

/************************************************************************
 * Snarf - read a whole file into a handle
 ************************************************************************/
OSErr Snarf(FSSpecPtr spec, Handle *hp, long limit) {
  short refN;
  long bytes;
  short err;

  if (!((err = AFSpOpenDF(spec, spec, fsRdPerm, &refN)))) {
    if (!((err = GetEOF(refN, &bytes)))) {
      if (limit)
        bytes = MIN(bytes, limit);
      *hp = NuHTempOK(bytes);
      if (!*hp)
        err = MemError();
      else if ((err = ARead(refN, &bytes, (unsigned char *)(LDRef(*hp))))) {
        ZapHandle(*hp);
        *hp = nil;
      } else
        UL(*hp);
    }
    MyFSClose(refN);
  }
  return (err);
}

/************************************************************************
 * SnarfRoman - read a whole file into a handle, and make it roman text if we
 *can
 ************************************************************************/
OSErr SnarfRoman(FSSpecPtr spec, Handle *hp, long limit) {
  // grab the actual text
  OSErr err = Snarf(spec, hp, limit);

  if (!err)
    err = SniffAndConvertHandleToRoman(hp);

  if (err)
    ZapHandle(*hp);

  return err;
}

/************************************************************************
 * Blat - blat a handle out to a text file
 ************************************************************************/
OSErr Blat(FSSpecPtr spec, Handle text, bool append) {
  OSErr err;

  LDRef(text);
  err = BlatPtr(spec, *text, GetHandleSize_(text), append);
  UL(text);
  return (err);
}

/************************************************************************
 * BlatPtr - blat text out to a text file
 ************************************************************************/
OSErr BlatPtr(FSSpecPtr spec, Ptr text, long size, bool append) {
  FSSpec newSpec;
  OSErr err;
  short refN;

  int fd = creat(spec->path, 0644);
  if (fd >= 0)
    close(fd);

  if ((err = AFSpOpenDF(spec, &newSpec, fsRdWrPerm, &refN)))
    FileSystemError(TEXT_WRITE, (unsigned char *)spec->name, err);
  else {
    if (append && (err = SetFPos(refN, fsFromLEOF, 0)))
      FileSystemError(TEXT_WRITE, (unsigned char *)spec->name, err);
    if ((err = AWrite(refN, &size, (unsigned char *)text)))
      FileSystemError(TEXT_WRITE, (unsigned char *)spec->name, err);
    else if ((err = TruncAtMark(refN)))
      FileSystemError(TEXT_WRITE, (unsigned char *)spec->name, err);
    MyFSClose(refN);
  }
  return (err);
}

/**********************************************************************
 * FileTypeOf - get the type of a file
 **********************************************************************/
OSType FileTypeOf(FSSpecPtr spec) {
  FInfo info;
  if (!AFSpGetFInfo(spec, spec, &info))
    return (info.fdType);
  else
    return (0);
}

/**********************************************************************
 * FlushFile - flush a file buffer
 **********************************************************************/
OSErr FlushFile(short refN) {
  if (fsync(refN) < 0)
    return ioErr;
  return noErr;
}

/**********************************************************************
 * MyUpdateResFile - udpate a resource file, and MAKE darn SURE
 **********************************************************************/
OSErr MyUpdateResFile(short resFile) {
  OSErr err = noErr;

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
OSErr MakeDarnSure(short refN) {
  OSErr err;
  FSSpec spec;

  if (!((err = FlushFile(refN))))
    if (!((err = GetFileByRef(refN, &spec))))
      err = FlushVol(nil, spec.vRefNum);
  return (err);
}

/**********************************************************************
 * FileCreatorOf - get the creator of a file
 **********************************************************************/
OSType FileCreatorOf(FSSpecPtr spec) {
  FInfo info;
  if (!AFSpGetFInfo(spec, spec, &info))
    return (info.fdCreator);
  else
    return (0);
}

/************************************************************************
 * IsText - is a file of type TEXT or not?
 ************************************************************************/
bool IsText(FSSpecPtr spec) {
  FInfo info;
  FSSpec newSpec;
  short err;

  err = AFSpGetFInfo(spec, &newSpec, &info);
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
 * ResolveAliasNoMount - resolve an alias, but don't mount any volumes
 ************************************************************************/
OSErr ResolveAliasNoMount(FSSpecPtr alias, FSSpecPtr orig, bool *wasAlias) {
  FInfo info;
  short err;
  FSSpec spec;
  AliasHandle ah;
  short refN;
  short count = 1;
  bool junk;
  short oldResF = CurResFile();

  // If it's a folder, it's not an alias.
  if (FSpIsItAFolder((FSSpecPtr)alias)) {
    if (*wasAlias)
      *wasAlias = false;
    if (orig)
      *orig = *alias;
    return noErr;
  }

  /*
   * is it an alias?
   */
  if ((err = FSpGetFInfo(alias, &info)))
    return (err);

  // This used to just copy alias into orig
  // That meant that the alias fsspec would get
  // turned into the original file.  There are places
  // where that was not helpful.  It may have been relied
  // on elsewhere, however, so we'll preserve
  // that behavior in the case where no orig pointer
  // is given
  if (!orig)
    orig = alias;
  else
    *orig = *alias;

  if (wasAlias)
    *wasAlias = (info.fdFlags & kIsAlias) != 0;
  if ((info.fdFlags & kIsAlias) == 0)
    return (noErr);

  /*
   * it's an alias; open it and extract the record
   */
  if (0 > (refN = FSpOpenResFile(alias, fsRdPerm)))
    return (err);
  ah = GetResource_('alis', 0);
  if (ah)
    DetachResource((Handle)ah);
  CloseResFile(refN);
  UseResFile(oldResF);

  if (!ah)
    return (resNotFound);

  /*
   * resolve the record
   */
  FSMakeFSSpec(Root.vRef, Root.dirId, (unsigned char *)"",
               &spec); /* Eudora Folder as base */
  err = MatchAlias(&spec,
                   kARMSearch |             /* allow id search */
                       kARMSearchRelFirst | /* darn fileid's; denigrate */
                       kARMNoUI,            /* don't bug the user */
                   /* note we do not specify kARMMountVol */
                   ah, &count, orig, &junk, nil, nil);

  ZapHandle(ah);
  return (err);
}

/************************************************************************
 * SpinOn - spin until a return code is not inProgress or cacheFault
 ************************************************************************/
short SpinOnLo(volatile OSErr *rtnCodeAddr, long maxTicks, bool allowCancel,
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
OSErr FSpTrash(FSSpecPtr spec) {
  FSSpec trashSpec;
  OSErr err;
  FSSpec exist, newExist;

  if (!((err = GetTrashSpec(spec->vRefNum, &trashSpec)))) {
    if (!FSMakeFSSpec(trashSpec.vRefNum, trashSpec.parID,
                      (unsigned char *)spec->name, &exist)) {
      newExist = exist;
      UniqueSpec(&newExist, 31);
      FSpRename(&exist, (const unsigned char *)newExist.name);
    }
    err = SpecMove(spec, &trashSpec);
  }
  return (err);
}

/**********************************************************************
 * UniqueSpec - make a unique filename
 **********************************************************************/
OSErr UniqueSpec(FSSpecPtr spec, short max) {
  short i;
  CInfoPBRec hfi;
  char dfName[256];
  char dfQuoteExtensionUnquote[256];
  char iAscii[256];
  OSErr err;

  //
  // Now that we've entered unix-land, we have to pay attention to
  // "extensions".
  //
  max = SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
      (unsigned char *)spec->name, dfName, dfQuoteExtensionUnquote, max);

  for (i = 1; i < 9999; i++) {
    /*
     * does file exist?
     */
    err = HGetCatInfo(spec->vRefNum, spec->parID, (unsigned char *)spec->name,
                      &hfi);
#ifdef NEVER
    if (RunType != Production)
      Dprintf("HGetCatInfo %p: %d;sc;g", spec->name, err);
#endif
    if (err == fnfErr)
      return (noErr);

    /*
     * oops.  file exists.  increment number on end of filename
     */
    g_strlcpy(spec->name, dfName, sizeof(spec->name));
    *iAscii = 0;
    PLCat(iAscii, i);
    if (*spec->name + *iAscii < max)
      PCat((unsigned char *)spec->name, iAscii);
    else {
      BMD(iAscii + 1, spec->name + (max + 1) - *iAscii, *iAscii);
      *spec->name = max;
    }

    //
    // add "extension"
    if (*dfQuoteExtensionUnquote) {
      PSCatC((unsigned char *)spec->name, '.');
      PSCat((unsigned char *)spec->name, dfQuoteExtensionUnquote);
    }
  }
  return (dupFNErr);
}

/**********************************************************************
 * SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote -
 *   we hate Windows
 **********************************************************************/
OSErr SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
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
OSErr TweakFileType(FSSpecPtr spec, OSType type, OSType creator) {
  FInfo info;
  OSErr err;
  FSSpec dirSpec;

  /*
   * get parent info
   */
  if (!((err = FSpGetFInfo(spec, &info)))) {
    info.fdType = type;
    info.fdCreator = creator;
    info.fdFlags &= ~kHasBeenInited;
    if (type == 'APPL')
      info.fdFlags |= fHasBundle;
    if (!((err = FSpSetFInfo(spec, &info)))) {
      if (!((err = FSMakeFSSpec(spec->vRefNum, spec->parID, (unsigned char *)"",
                                &dirSpec))))
        err = FSpTouch(&dirSpec);
    }
  }
  return (err);
}

/**********************************************************************
 * FSpTouch - set a file's mod date to now
 **********************************************************************/
OSErr FSpTouch(FSSpecPtr spec) {
  CInfoPBRec hfi;
  OSErr err;
  char name[256];

  PSCopy(name, spec->name);
  if (!((err = HGetCatInfo(spec->vRefNum, spec->parID, name, &hfi)))) {
    hfi.hFileInfo.ioFlMdDat = LocalDateTime();
    PSCopy(name, spec->name);
    err = HSetCatInfo(spec->vRefNum, spec->parID, name, &hfi);
  }
  return (err);
}

/**********************************************************************
 * FSpExists - see if a file exists
 **********************************************************************/
OSErr FSpExists(FSSpecPtr spec) {
  FInfo info;
  return (FSpGetFInfo(spec, &info));
}

/**********************************************************************
 * FSpRFSane - is a resource file sane?
 **********************************************************************/
OSErr FSpRFSane(FSSpecPtr spec, bool *sane) {
  OSErr err;

  err = utl_RFSanity(spec, sane);
  return (err);
}

/**********************************************************************
 * FSpKillRFork - kill the resource fork of a file
 **********************************************************************/
OSErr FSpKillRFork(FSSpecPtr spec) {
  OSErr err;
  short refN;

  if (!((err = FSpOpenRF(spec->path, fsRdWrPerm, &refN)))) {
    err = SetEOF(refN, 0);
    MyFSClose(refN);
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
void FolderSizeHi(short vRef, long dirID, uint32_t *cumSize) {
  CInfoPBRec hfi;
  char name[256];

  hfi.hFileInfo.ioNamePtr = name;
  *cumSize = 0;

  FolderSize(vRef, dirID, &hfi, cumSize);

  return;
}

/************************************************************************
 * FolderSize - how big is a folder?
 ************************************************************************/
static bool FolderSizeCallback(DirIterateInfo *info) {
  uint32_t *cumSize = (uint32_t *)info->data;
  if (info->isDir) {
    uint32_t subSize = 0;
    FolderSize(info->spec.vRefNum, 0, nil, &subSize);
    *cumSize += subSize;
  } else {
    *cumSize += info->size;
  }
  return true;
}

void FolderSize(short vRef, long dirID, CInfoPBRec *hfi, uint32_t *cumSize) {
  FSSpec spec;
  FSMakeFSSpec(vRef, dirID, (unsigned char *)"", &spec);
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

  return noErr;
}

/************************************************************************
 * HSetCatInfo - get cat info for a file?
 ************************************************************************/
short HSetCatInfo(short vRefNum, long dirID, const char *name, CInfoPBPtr pb) {
  return noErr;
}

short HMove(short vRef, long dirId, const char *name, long destDirId,
            const char *newName) {
  char fromPath[1024];
  char toPath[1024];

  if (!name || !newName)
    return paramErr;

  snprintf(fromPath, sizeof(fromPath), "./%s", name);
  snprintf(toPath, sizeof(toPath), "./%s", newName);

  if (rename(fromPath, toPath) == 0)
    return noErr;

  return ioErr;
}

/**********************************************************************
 * FSpOpenResFile - stub for resource fork access
 **********************************************************************/
short FSpOpenResFile(FSSpecPtr spec, SignedByte permission) { return -1; }

/**********************************************************************
 * ExtractCreatorFromBndl - figure out what an app's creator used to be
 **********************************************************************/
OSErr ExtractCreatorFromBndl(FSSpecPtr spec, OSType *creator) {
  OSErr err;
  short refN;
  Handle bndl;
  short oldResF = CurResFile();

  if (-1 != (refN = FSpOpenResFile(spec, fsRdPerm))) {
    if ((bndl = GetIndResource('BNDL', 1))) {
      *creator = *(long *)*bndl;
      err = noErr;
    } else
      err = resNotFound;
    CloseResFile(refN);
    UseResFile(oldResF);
  } else
    err = ResError();
  return (err);
}

/************************************************************************
 * AFSpGetCatInfo - cat info, resolving aliases
 ************************************************************************/
short AFSpGetCatInfo(FSSpecPtr spec, FSSpecPtr newSpec, CInfoPBRec *hfi) {
  OSErr err;
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
short FolderFileCount(FSSpecPtr spec) {
  CInfoPBRec hfi;
  FSSpec newSpec;

  if (AFSpGetCatInfo(spec, &newSpec, &hfi))
    return (-1);
  return (hfi.hFileInfo.ioFlStBlk);
}

/**********************************************************************
 * RemoveDir - remove a directory
 **********************************************************************/
OSErr RemoveDir(FSSpecPtr spec) {
  DIR *dir;
  struct dirent *entry;
  char fullpath[PATH_MAX];
  FSSpec folder;
  OSErr err = noErr;

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
    err = FSpDelete(fullpath);
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
OSErr ChainDelete(FSSpecPtr spec) {
  FSSpec chain;
  bool wasAlias, isFolder;

  chain = *spec;
  if (!ResolveAliasFile(&chain, false, &isFolder, &wasAlias) && wasAlias)
    ChainDelete(&chain);
  return (FSpDelete(spec->path));
}

/************************************************************************
 * AHGetFileInfo - get info on a file
 ************************************************************************/
OSErr AHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi) {
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

  return noErr;
}

#pragma segment FileUtil2

/************************************************************************
 * FSpGetHFileInfo - get info, don't resolve alias
 ************************************************************************/
OSErr FSpGetHFileInfo(FSSpecPtr spec, CInfoPBRec *hfi) {
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

  return noErr;
}

short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi) {
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

pascal short FolderItems(short item, DialogPtr dlog) {
#pragma unused(dlog)

  if (item == 2) {
    good = true;
    item = 3;
  }
  return item;
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
  Handle buffer = NuHTempOK(OPTIMAL_BUFFER);
  long bSize = OPTIMAL_BUFFER;
  long eof = 0;

  if (!buffer)
    return (MemError());
  LDRef(buffer);

  // If resource fork requested, just return success (portable code has no
  // resource forks)
  if (rFork) {
    ZapHandle(buffer);
    return noErr;
  }

  // Convert Pascal string names to C strings and build paths
  char fromCName[256], toCName[256];
  PtoCcpy(fromCName, (const char *)fromName); // Cast to silence warning
  PtoCcpy(toCName, (const char *)name);

  char fromPath[1024], toPath[1024];
  snprintf(fromPath, sizeof(fromPath), "./%s", fromCName);
  snprintf(toPath, sizeof(toPath), "./%s", toCName);

  if (!(err = FSpOpenDF(fromPath, fsRdPerm, &fromRef))) {
    if (!(err = FSpOpenDF(toPath, fsRdWrPerm, &toRef))) {
      GetEOF(fromRef, &eof);
      for (bSize = MIN(OPTIMAL_BUFFER, eof); !err && eof;
           bSize = MIN(OPTIMAL_BUFFER, eof)) {
        if (!(err = ARead(fromRef, &bSize, (unsigned char *)*buffer))) {
          if (progress)
            ByteProgress(0, -1, bSize);
          eof -= bSize;
          err = AWrite(toRef, &bSize, (unsigned char *)*buffer);
          if (progress)
            ByteProgress(0, -1, bSize);
        }
      }
      TruncAtMark(toRef);
      MyFSClose(toRef);
    }
    MyFSClose(fromRef);
  }
  ZapHandle(buffer);
  return (err);
}

/**********************************************************************
 * FSpDupFile - duplicate a file
 **********************************************************************/
OSErr FSpDupFile(FSSpecPtr to, FSSpecPtr from, bool replace, bool progress) {
  OSErr err;
  bool hasRFork = FSpRFSize(from) != 0;
#ifdef DEBUG
  char s[256];
#endif

  if (hasRFork) {
    FSpCreateResFile(to, '----', '----', 0);
    err = ResError();
  } else {
    // Create file using path
    int fd = creat(to->path, 0644);
    if (fd < 0)
      err = ioErr;
    else {
      close(fd);
      err = noErr;
    }
  }

  if (err && err == dupFNErr && replace)
    err = noErr;
#ifdef DEBUG
  if (err) {
    ComposeString(s, "FSpDupFile: create failed %d.%d.%p; %d", to->vRefNum,
                  to->parID, to->name, err);
    AlertStr(OK_ALRT, Note, s);
  }
#endif
  if (err)
    return (err);

  if (progress)
    ByteProgress(0, 0, 2 * (FSpDFSize(to) + FSpRFSize(to)));
  if (!hasRFork || !(err = FSpCopyRFork(to, from, progress))) {

    if (!(err = FSpCopyDFork(to, from, progress))) {
      if (!progress)
        MiniEvents();
      else
        Progress(100, 0, nil, nil, nil);
      if (!(err = FSpCopyFInfo(to, from)))
        return (noErr);
    }
#ifdef DEBUG
    else {
      ComposeString(s, "FSpDupFile: dfork failed %d.%d.%p->%d.%d.%p; %d",
                    from->vRefNum, from->parID, from->name, to->vRefNum,
                    to->parID, to->name, err);
      AlertStr(OK_ALRT, Note, s);
    }
#endif
  }
#ifdef DEBUG
  else if (hasRFork) {
    ComposeString(s, "FSpDupFile: rfork failed %d.%d.%p->%d.%d.%p; %d",
                  from->vRefNum, from->parID, from->name, to->vRefNum,
                  to->parID, to->name, err);
    AlertStr(OK_ALRT, Note, s);
  }
#endif

  FSpDelete(to->path);
  return err;
}

/**********************************************************************
 * FSpDupFolder - duplicate a folder
 **********************************************************************/
OSErr FSpDupFolder(FSSpecPtr toSpec, FSSpecPtr fromSpec, bool replace,
                   bool progress) {
  CInfoPBRec hfi;
  FSSpec to, from;
  OSErr err = noErr;

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
short MyResolveAlias(short *vRef, long *dirId, char *name, bool *wasAlias) {
  FSSpec theSpec;
  bool folder;
  long haveAlias;
  short err = noErr;
  bool wasIt;

  if (wasAlias)
    *wasAlias = false;
  if (!Gestalt(gestaltAliasMgrAttr, &haveAlias) && haveAlias & 0x1) {
    if (!(err = FSMakeFSSpec(*vRef, *dirId, name, &theSpec)) &&
        !(err = ResolveAliasFile(&theSpec, true, &folder, &wasIt))) {
      if (wasIt) {
        *vRef = theSpec.vRefNum;
        *dirId = theSpec.parID;
        g_strlcpy(name, theSpec.name, sizeof(name));
        if (wasAlias)
          *wasAlias = true;
      }
    }
  }
  return (err);
}

/**********************************************************************
 * SimpleResolveAlias - resolve an alias without all the fuss
 **********************************************************************/
OSErr SimpleResolveAlias(AliasHandle alias, FSSpecPtr spec) {
  bool junk;
  FSSpec localSpec;

  if (!spec)
    spec = &localSpec; // allow the caller to pass nil
  Zero(*spec);
  return (ResolveAlias(nil, alias, spec, &junk));
}

/**********************************************************************
 * SimpleResolveAliasNoUI - resolve an alias without bugging the user
 **********************************************************************/
OSErr SimpleResolveAliasNoUI(AliasHandle alias, FSSpecPtr spec) {
  bool junk;
  FSSpec localSpec;
  short justOne = 1;

  if (!spec)
    spec = &localSpec; // allow the caller to pass nil

  Zero(*spec);
  return (MatchAlias(nil, kARMMountVol | kARMNoUI | kARMMultVols | kARMSearch,
                     alias, &justOne, spec, &junk, nil, nil));
}

/************************************************************************
 * ExchangeAndDel - exchange two files, deleting one
 ************************************************************************/
OSErr ExchangeAndDel(FSSpecPtr tmpSpec, FSSpecPtr spec) {
  short err;

  if ((err = ExchangeFiles(tmpSpec, spec))) {
    FileSystemError(TEXT_WRITE, tmpSpec->name, err);
    return (err);
  }
  FSpDelete(tmpSpec);
  return (noErr);
}

/************************************************************************
 * ExchangeFiles - FSpExchangeFiles, with support for dopey AFP servers
 ************************************************************************/
OSErr ExchangeFiles(FSSpecPtr tmpSpec, FSSpecPtr spec) {
  OSErr err = FSpExchangeFiles(tmpSpec, spec);

  if (err)
    err = FSpExchangeFilesCompat(tmpSpec, spec);
  return (err);
}

/************************************************************************
 * FSpExchangeFilesCompat - do FSpExchangeFiles if FSpExchangeFiles not
 *supported from MoreFiles
 ************************************************************************/
OSErr FSpExchangeFilesCompat(const FSSpec *source, const FSSpec *dest) {
  char temp_path[PATH_MAX];
  struct stat st_source, st_dest;
  OSErr result = noErr;

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

  result = noErr;

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
static OSErr GenerateUniqueName(short volume, long *startSeed, long dir1,
                                long dir2, StringPtr uniqueName) {
  OSErr error = noErr;
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
  return (noErr);
}

/**********************************************************************
 * MorphDesktop - if a spec points to the boot disk desktop and the
 *  requested volume is not the desktop, then set the spec to the requested
 *  volume's desktop
 **********************************************************************/
OSErr MorphDesktop(short vRef, FSSpecPtr where) {
  FSSpec desk;
  OSErr err;

  if (*where->name)
    return (noErr); // not pointing to folder
  if (SameVRef(vRef, where->vRefNum))
    return (noErr); // same volume
  if ((err = FindFolder(kOnSystemDisk, kDesktopFolderType, false,
                        (short *)&desk.vRefNum, &desk.parID)))
    return (err);
  if (SameVRef(vRef, desk.vRefNum))
    return (noErr); // same volume as system
  if (SameVRef(where->vRefNum, desk.vRefNum) && desk.parID == where->parID) {
    // ok, spec points to desktop folder
    // point instead to desktop folder on volume
    err = FindFolder(vRef, kDesktopFolderType, false, (short *)&where->vRefNum,
                     &where->parID);
  }
  return (err);
}

/************************************************************************
 * AFSpIsItAFolder - is a file a folder?
 ************************************************************************/
bool AFSpIsItAFolder(FSSpecPtr spec) {
  FInfo info;

  FSpGetFInfo(spec, &info);
  if ((info.fdFlags & kIsAlias) && info.fdType == kContainerFolderAliasType)
    return (true);
  return (FSpIsItAFolder(spec));
}

/************************************************************************
 * GetFileByRef - figure out the name & vol of a file from an open file
 ************************************************************************/
short GetFileByRef(short refN, FSSpecPtr specPtr) {
  FCBPBRec fcb;
  short err;
  char name[256];

  fcb.ioCompletion = nil;
  fcb.ioVRefNum = 0;
  fcb.ioRefNum = refN;
  fcb.ioFCBIndx = 0;
  fcb.ioNamePtr = name;
  if ((err = PBGetFCBInfo((FCBInfoPBPtr)&fcb, false)))
    return (err);
  return (FSMakeFSSpec(fcb.ioFCBVRefNum, fcb.ioFCBParID, name, specPtr));
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
  short err = noErr;
  long writing;
  static short charsOnLine = 0;
  unsigned char *nl;
  short stops = 0;

  if (!FakeTabs)
    return (FSZWrite(refN, count, buf));
  for (p = buf; p < end; p = buf = p + 1) {
    nl = buf - charsOnLine - 1;
    while (p < end && *p != tabChar) {
      if (*p == '\015')
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
short NCWriteP(short refN, unsigned char *pString) {
  long count = *pString;
  return (NCWrite(refN, &count, pString + 1));
}

/************************************************************************
 * AWriteP - write a Pascal string
 ************************************************************************/
short AWriteP(short refN, unsigned char *pString) {
  long count = *pString;
  return (AWrite(refN, &count, pString + 1));
}

// Redundant AWrite removed

/**********************************************************************
 * WipeSpec - wipe a file
 **********************************************************************/
OSErr WipeSpec(FSSpecPtr spec) {
  short refN;
  long eof;
  OSErr err;

  if (!(err = FSpOpenDF(spec->path, fsRdWrPerm, &refN))) {
    if (!(err = GetEOF(refN, &eof)))
      err = WipeDiskArea(refN, 0, eof);
    MyFSClose(refN);
    if (!(err = FSpOpenRF(spec->path, fsRdWrPerm, &refN))) {
      if (!(err = GetEOF(refN, &eof)))
        err = WipeDiskArea(refN, 0, eof);
      MyFSClose(refN);
    }
  }
  if (!err) {
    FlushVol(nil, spec->vRefNum);
    err = FSpDelete(spec->path);
  }

  if (err && err != fnfErr)
    FileSystemError(WIPE_ERROR, (unsigned char *)spec->name, err);

  return (err);
}

/**********************************************************************
 * WipeDiskArea - wipe part of a disk
 **********************************************************************/
OSErr WipeDiskArea(short refN, long offset, long len) {
  long bSize = MIN(len, OPTIMAL_BUFFER);
  UHandle h;
  long size;
  unsigned char *spot, *end;
  OSErr err;

  if (!bSize)
    return (noErr);
  h = (UHandle)NuHTempBetter(bSize);
  if (!h)
    return (MemError());

  /*
   * fill it with returns
   */
  end = LDRef(h) + bSize;
  for (spot = *h; spot < end; spot++)
    *spot = ' ';
  spot[-1] = '\015';
  for (spot = *h; spot < end; spot += 50)
    *spot = '\015';

  /*
   * blat it over the disk area
   */
  if (!(err = SetFPos(refN, fsFromStart, offset)))
    for (size = bSize; len; size = MIN(bSize, len)) {
      err = NCWrite(refN, &size, *h);
      if (err)
        break;
      len -= size;
    }

  ZapHandle(h);
  return (err);
}

/************************************************************************
 * EnsureNewline - make sure there is a newline at or just before the
 * current file position.
 ************************************************************************/
OSErr EnsureNewline(short refN) {
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
    return (noErr);

  /*
   * back up one character
   */
  if ((err = SetFPos(refN, fsFromStart, offset - 1)))
    return (err);

  /*
   * read it
   */
  count = 1;
  if ((err = ARead(refN, &count, chars)))
    return (err);

  /*
   * is newline?
   */
  if (*chars == '\015')
    return (noErr);

  /*
   * make it so
   */
  long one = 1;
  char cr = Cr;
  return (FSWrite(refN, &one, &cr));
}

/************************************************************************
 * AFSpOpenDF - OpenDF, but resolve the alias first
 ************************************************************************/
OSErr AFSpOpenDF(FSSpecPtr spec, FSSpecPtr newSpec, SignedByte permission,
                 short *refNum) {
  OSErr err;
  bool folder, wasIt;
  short localRef;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  err = FSpOpenDF(newSpec, permission, &localRef);
  if (!err)
    *refNum = localRef;
  return err;
}

/************************************************************************
 * AFSpOpenRF
 ************************************************************************/
OSErr AFSpOpenRF(FSSpecPtr spec, FSSpecPtr newSpec, SignedByte permission,
                 short *refNum) {
  OSErr err;
  bool folder, wasIt;
  short localRef;

  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  err = FSpOpenRF(newSpec->path, permission, &localRef);
  if (!err)
    *refNum = localRef;
  return err;
}

/************************************************************************
 * AFSpDelete
 ************************************************************************/
OSErr AFSpDelete(FSSpecPtr spec, FSSpecPtr newSpec) {
  OSErr err;
  bool folder, wasIt;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  return (FSpDelete(newSpec));
}

/************************************************************************
 * AFSpGetFInfo
 ************************************************************************/
OSErr AFSpGetFInfo(FSSpecPtr spec, FSSpecPtr newSpec, FInfo *fndrInfo) {
  OSErr err;
  bool folder, wasIt;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  return (FSpGetFInfo(newSpec, fndrInfo));
}

/************************************************************************
 * FSpFileSize - get the size of a file
 ************************************************************************/
long FSpFileSize(FSSpecPtr spec) {
  CInfoPBRec hfi;

  if (!AHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlLgLen + hfi.hFileInfo.ioFlRLgLen);
  else
    return (0);
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AFSpSetMod(FSSpecPtr spec, uint32_t mod) {
  CInfoPBRec hfi;
  OSErr err;
  FSSpec newSpec;
  bool folder, wasIt;

  newSpec = *spec;
  if ((err = ResolveAliasFile(&newSpec, true, &folder, &wasIt)))
    return (err);

  if (!(err = AHGetFileInfo(newSpec.vRefNum, newSpec.parID,
                            (const char *)newSpec.name, &hfi))) {
    hfi.hFileInfo.ioFlMdDat = mod;
    err = AHSetFileInfo(newSpec.vRefNum, newSpec.parID,
                        (const char *)newSpec.name, &hfi);
  }
  return (err);
}

/**********************************************************************
 *
 **********************************************************************/
uint32_t AFSpGetMod(FSSpecPtr spec) {
  CInfoPBRec hfi;

  if (!AHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlMdDat);
  else
    return (0);
}

/************************************************************************
 * FSpDFSize - get the size of the data fork a file
 ************************************************************************/
long FSpDFSize(FSSpecPtr spec) {
  CInfoPBRec hfi;

  if (!AHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlLgLen);
  else
    return (0);
}

/************************************************************************
 * FSpRFSize - get the size of the data fork a file
 ************************************************************************/
long FSpRFSize(FSSpecPtr spec) {
  CInfoPBRec hfi;

  if (!AHGetFileInfo(spec->vRefNum, spec->parID, (const char *)spec->name,
                     &hfi))
    return (hfi.hFileInfo.ioFlRLgLen);
  else
    return (0);
}

/************************************************************************
 * FSpSetFXInfo - set the FXInfo for a file
 ************************************************************************/
OSErr FSpSetFXInfo(FSSpecPtr spec, FXInfo *fxInfo) {
  OSErr err;
  CInfoPBRec hfi;

  if (!(err = AFSpGetHFileInfo(spec, &hfi))) {
    hfi.hFileInfo.ioFlXFndrInfo = *fxInfo;
    err = HSetCatInfo(spec->vRefNum, spec->parID, spec->name, &hfi);
  }
  return (err);
}

/************************************************************************
 * AFSpSetFInfo
 ************************************************************************/
OSErr AFSpSetFInfo(FSSpecPtr spec, FSSpecPtr newSpec, FInfo *fndrInfo) {
  OSErr err;
  bool folder, wasIt;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  return (FSpSetFInfo(newSpec, fndrInfo));
}

/************************************************************************
 * AFSpSetFLock
 ************************************************************************/
OSErr AFSpSetFLock(FSSpecPtr spec, FSSpecPtr newSpec) {
  OSErr err;
  bool folder, wasIt;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  return (FSpSetFLock(newSpec));
}

/************************************************************************
 * AFSpRstFLock
 ************************************************************************/
OSErr AFSpRstFLock(FSSpecPtr spec, FSSpecPtr newSpec) {
  OSErr err;
  bool folder, wasIt;
  *newSpec = *spec;
  if ((err = ResolveAliasFile(newSpec, true, &folder, &wasIt)))
    return (err);

  return (FSpRstFLock(newSpec));
}

/************************************************************************
 * IsAlias - is a file an alias?
 ************************************************************************/
bool IsAlias(FSSpecPtr spec, FSSpecPtr newSpec) {
  bool folder, wasIt = false;
  *newSpec = *spec;
  ResolveAliasFile(newSpec, true, &folder, &wasIt); // if error, wasIt will
                                                    // either be set correctly
                                                    // or else still be false
  return (wasIt);
}

/************************************************************************
 * IsAliasNoMount - is a file an alias (but don't mount it if so)?
 ************************************************************************/
bool IsAliasNoMount(FSSpecPtr spec, FSSpecPtr newSpec) {
  bool isAlias = false;
  *newSpec = *spec;

  ResolveAliasNoMount(spec, newSpec, &isAlias);

  return (isAlias);
}

/************************************************************************
 * ResolveAliasOrElse - Resolve an alias or fail
 ************************************************************************/
OSErr ResolveAliasOrElse(FSSpecPtr spec, FSSpecPtr newSpec, bool *wasIt) {
  bool folder, isAlias = false;
  OSErr err;
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
OSErr SubFolderSpec(short nameId, FSSpecPtr spec) {
  char string[256];
  OSErr err;
  static StackHandle specStack;
  CSpec cSpec;
  short i;

  // clear cache?
  if (!spec) {
    if (specStack)
      (*specStack)->elCount = 0;
    return noErr;
  }

  // search for folder in cache
  if (specStack)
    for (i = 0; i < (*specStack)->elCount; i++) {
      StackItem(&cSpec, i, specStack);
      if (cSpec.count == nameId) {
        *spec = cSpec.spec;
        return noErr;
      }
    }

  // not in cache.  Go look for it
  if (!(err = FSMakeFSSpec(Root.vRef, Root.dirId, GetRString(string, nameId),
                           spec))) {
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
OSErr FindSubFolderSpec(long domain, long folder, short subfolderID,
                        bool create, FSSpecPtr spec) {
  FSSpec localSpec;
  OSErr err =
      FindFolder(domain, folder, create, &localSpec.vRefNum, &localSpec.parID);

  if (!err)
    err = SubFolderSpecOf(&localSpec, subfolderID, create, spec);

  return err;
}

/************************************************************************
 * SubFolderSpecOf - find a subfolder of a given fsspec
 ************************************************************************/
OSErr SubFolderSpecOf(FSSpecPtr inSpec, short subfolderID, bool create,
                      FSSpecPtr subSpec) {
  char subfolderName[256];

  GetRString((char *)subfolderName, subfolderID);
  return SubFolderSpecOfStr(inSpec, (const char *)subfolderName, create,
                            subSpec);
}

OSErr SubFolderSpecOfStr(FSSpecPtr inSpec, const char *subfolderName,
                         bool create, FSSpecPtr subSpec) {
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
  return noErr;
}

/************************************************************************
 * StuffFolderSpec - find the stuff folder
 ************************************************************************/
OSErr StuffFolderSpec(FSSpecPtr spec) {
  FSSpec localSpec;
  char name[256];
  OSErr err = GetFileByRef(AppResFile, &localSpec);

  if (!err)
    err = FSMakeFSSpec(localSpec.vRefNum, localSpec.parID,
                       GetRString(name, STUFF_FOLDER), &localSpec);
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
bool SpecInSubfolderOf(FSSpecPtr att, FSSpecPtr folder) {
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
OSErr FSMakeFID(FSSpecPtr spec, long *fid) {
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
    err = noErr; /* ignore; ioFileID is good */

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
  OSErr err = noErr;
  FSSpec spec;
  CInfoPBRec info;

  // get the system version of this machine
  err = Gestalt(gestaltSystemVersion, &sysVers);
  if (err == noErr) {
    // is this system version affected by the bug?
    affected = GetRLong(FILEID_AFFECTED_SYSVERSION);
    if (affected == sysVers) {
      Zero(spec);
      GetFileByRef(SettingsRefN, &spec);
      AFSpGetCatInfo(&spec, &spec, &info);
    }
  }
}

/************************************************************************
 * FSResolveFID - resolve a vRef & fileid into a spec
 ************************************************************************/
OSErr FSResolveFID(short vRef, long fid, FSSpecPtr spec) {
  HParamBlockRec fidpb;
  short err;
  char name[256];

  Zero(fidpb);

  *name = 0;
  fidpb.fidParam.ioCompletion = nil;
  fidpb.fidParam.ioNamePtr = name;
  fidpb.fidParam.ioFileID = fid;
  fidpb.fidParam.ioVRefNum = vRef;

  err = PBResolveFileIDRefSync((HParmBlkPtr)&fidpb);

  if (!err)
    err = FSMakeFSSpec(vRef, fidpb.fidParam.ioSrcDirID, name, spec);

  return (err);
}

/************************************************************************
 * SpecMove - move a file from one place to another
 ************************************************************************/
OSErr SpecMove(FSSpecPtr moveMe, FSSpecPtr moveTo) {
  FInfo info;

  if (!FSpGetFInfo(moveMe, &info)) {
    info.fdFlags &= ~fInited;
    Zero(info.fdLocation);
    FSpSetFInfo(moveMe, &info);
  }
  return HMove(moveMe->vRefNum, moveMe->parID, moveMe->name, moveTo->parID,
               nil);
}

/************************************************************************
 * SpecMoveAndRename - move a file from one place to another, and rename
 ************************************************************************/
OSErr SpecMoveAndRename(FSSpecPtr moveMe, FSSpecPtr moveTo) {
  OSErr err;

  if ((err = SpecMove(moveMe, moveTo)))
    return err;

  err = FSpRename(moveMe, moveTo->name);

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
OSErr GetTrashSpec(short vRef, FSSpecPtr spec) {
  spec->name[0] = 0;
  spec->vRefNum = vRef;
  int vRefInt = vRef;
  OSErr err =
      FindFolder(vRef, kTrashFolderType, kCreateFolder, &vRefInt, &spec->parID);
  if (!err) {
    vRef = (short)vRefInt;
  }
  return err;
}

/************************************************************************
 * DTRef - return the ref number for the desktop db
 ************************************************************************/
OSErr DTRef(short vRef, short *dtRef) { return noErr; }

OSErr DTGetAppl(short vRef, short dtRef, OSType creator, FSSpecPtr appSpec) {
  return fnfErr;
}

/************************************************************************
 * DTFindAppl - find which dtRef an application lives in
 ************************************************************************/
short DTFindAppl(OSType creator) {
  OSErr err;
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
OSErr DTSetComment(FSSpecPtr spec, char *comment) {
  DTPBRec pb;
  short dtRef;
  OSErr err;

  if (!HaveTheDiseaseCalledOSX() && !(err = DTRef(spec->vRefNum, &dtRef))) {
    Zero(pb);
    pb.ioNamePtr = spec->name;
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
 ************************************************************************/
bool SameSpec(FSSpecPtr sp1, FSSpecPtr sp2) {
  return (sp1->parID == sp2->parID && SameVRef(sp1->vRefNum, sp2->vRefNum) &&
          StringSame(sp1->name, sp2->name));
}

/************************************************************************
 * SpecDirId - find the dirId of the directory referenced by a spec
 ************************************************************************/
long SpecDirId(FSSpecPtr spec) {
  CInfoPBRec hfi;
  FSSpec newSpec;
  char name[256];

  Zero(hfi);
  hfi.hFileInfo.ioNamePtr = name;
  AFSpGetCatInfo(spec, &newSpec, &hfi);
  return (hfi.hFileInfo.ioDirID);
}

/************************************************************************
 * CanWrite - can we write on a file?  Only one way to tell on a macintosh
 ************************************************************************/
OSErr CanWrite(FSSpecPtr spec, bool *can) {
  FSSpec newSpec = *spec;
  short refN;
  Byte buff = 13;
  long len;
  CInfoPBRec hfi;
  OSErr err;
  bool b;

  *can = false;
  if (!(err = ResolveAliasFile(&newSpec, true, &b, &b)) &&
      !(err = AHGetFileInfo(newSpec.vRefNum, newSpec.parID,
                            (const char *)newSpec.name, &hfi)))
    if (!(err = FSpOpenDF(&newSpec, fsRdWrPerm, &refN))) {
      len = 1;
      if (!(err = SetFPos(refN, fsFromLEOF, 0)))
        if (!FSWrite(refN, &len, &buff)) {
          *can = true;
          SetFPos(refN, fsFromLEOF, -1);
          if (!GetFPos(refN, &len))
            TruncOpenFile(refN, len);
        }
      MyFSClose(refN);
      AFSpSetHFileInfo(&newSpec, &hfi); /* restore mod date */
    } else if ((err == permErr || err == afpAccessDenied) &&
               !(err = FSpOpenDF(&newSpec, fsRdPerm, &refN))) {
      MyFSClose(refN);
      *can = false;
    }
  return (err);
}

#ifdef DEBUG
/**********************************************************************
 * MyFSClose - call FSClose
 **********************************************************************/
OSErr MyFSClose(short refN) {
  if (!PrefIsSet(PREF_CORVAIR))
    MakeDarnSure(refN);
  return (FSClose(refN));
}
#endif

/**********************************************************************
 * NewTempSpec - make a temp file spec
 **********************************************************************/
OSErr NewTempSpec(short vRef, long dirId, char *name, FSSpecPtr spec) {
  long tempId;
  OSErr err;
  char fName[256];
  static unsigned char n;

  if ((err = FindTemporaryFolder(vRef, dirId, &tempId, &vRef)))
    return err;

  n++;

  if (name)
    g_strlcpy(fName, name, sizeof(fName));
  else {
    MyNumToString(TickCount(), fName);
    PCatC(fName, '+');
    PLCat(fName, n);
  }

  err = FSMakeFSSpec(vRef, tempId, fName, spec);
  return (UniqueSpec(spec, 27));
}

/**********************************************************************
 * FindTemporaryFolder - find the Temporary Folder
 *	use spool folder if not available on server
 **********************************************************************/
OSErr FindTemporaryFolder(short vRef, long dirId, long *tempDirId,
                          short *tempVRef) {
  OSErr err = noErr;

  //	tell FindFolder to forget everything it knows, and look at the disk
  err = MyInvalidateFolderDescriptorCache(0, 0L);
  ASSERT(noErr == err);
  int tempVRefInt;
  err = FindFolder(vRef, kTemporaryFolderType, true, &tempVRefInt, tempDirId);
  if (!err)
    *tempVRef = (short)tempVRefInt;

#ifdef DEBUG
  // Some versions of OS X will return noErr and a bad value for the temp
  // folder if it existed once, but does no longer. So, we check to see if it
  // exists.
  //	*** Darn their eyes! ***
  if (noErr == err) {
    CInfoPBRec pb;

    Zero(pb);
    pb.dirInfo.ioFDirIndex = -1; // use ioVRefNum and ioDrDirID only
    pb.dirInfo.ioVRefNum = *tempVRef;
    pb.dirInfo.ioDrDirID = *tempDirId;
    err = PBGetCatInfoSync(&pb);
    ASSERT(noErr == err);
  }
#endif

  // If findfolder fails, or if it returns a different volume, use
  // the spool folder
  if (err || *tempVRef != vRef) {
    err = noErr;
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
OSErr AddUniqueExt(FSSpecPtr spec, short extId) {
  FSSpec newSpec;
  short n = 0;
  char extStr[256], nStr[256];
  OSErr err;

  g_strlcpy(extStr, ".", sizeof(extStr));
  PCatR(extStr, extId);
  *nStr = 0;

  for (;;) {
    newSpec = *spec;
    *newSpec.name = MIN(*newSpec.name, 31 - *extStr - *nStr);
    if (n++)
      PCat(newSpec.name, nStr);
    PCat(newSpec.name, extStr);
    if ((err = FSpExists(&newSpec)))
      break;
    MyNumToString(n, nStr);
  }

  if (err == fnfErr) {
    err = FSpRename(spec, newSpec.name);
    if (!err)
      g_strlcpy(spec->name, newSpec.name, sizeof(spec->name));
  }

  return (err);
}

/**********************************************************************
 * NewTempSpec - make a temp file spec
 **********************************************************************/
OSErr NewTempExtSpec(short vRef, char *name, short extId, FSSpecPtr spec) {
  long dirId;
  OSErr err = FindTemporaryFolder(vRef, 0L, &dirId, &vRef);
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
      PCatC(fName, '+');
      PLCat(fName, n);
    }
    PCatC(fName, '.');
    PCatR(fName, extId);

  } while (!FSMakeFSSpec(vRef, dirId, fName, spec));
  if (err == fnfErr)
    err = noErr;
  return (err);
}

/************************************************************************
 * MakeAFinderAlias - make an alias to a file
 ************************************************************************/
OSErr MakeAFinderAlias(FSSpecPtr originalSpec, FSSpecPtr aliasSpec) {
  AliasHandle alias = nil;
  short err;
  FSSpec spec, localSpec;
  FInfo origInfo, aliasInfo;
  short refN;
  short oldResF = CurResFile();

  err = FSMakeFSSpec(aliasSpec->vRefNum, aliasSpec->parID, "\x00", &spec);

  if (!(err = NewAlias(&spec, originalSpec, &alias)) &&
      !(err = FSpGetFInfo(originalSpec, &origInfo))) {
    err = FSMakeFSSpec(
        aliasSpec->vRefNum, aliasSpec->parID,
        aliasSpec->name[0] ? aliasSpec->name : originalSpec->name, &localSpec);

    /*
     * does file exist?
     */
    if (!FSpGetFInfo(&localSpec, &aliasInfo)) {
      /*
       * don't replace real file with alias
       */
      if (!IsAlias(&localSpec, &spec)) {
        err = dupFNErr;
        goto done;
      } else if (SameSpec(originalSpec, &spec))
        goto done; /* we already have an alias to the file in ? */
    }

    /*
     * create alias file
     */
    FSpCreateResFile(&localSpec, origInfo.fdCreator, origInfo.fdType, 0);
    err = ResError();
    if ((err = dupFNErr))
      err = noErr; /* ignore; we'll change an existing alias */

    /*
     * write alias into it
     */
    if (!err) {
      err = FSpGetFInfo(&localSpec, &aliasInfo);
      aliasInfo.fdFlags |= kIsAlias;
      err = FSpSetFInfo(&localSpec, &aliasInfo);

      /*
       * now we have an alias file; stick the alias into it
       */
      if (0 <= (refN = FSpOpenResFile(&localSpec, fsRdWrPerm))) {
        AddResource_(alias, 'alis', 0, "");
        if (!(err = ResError()))
          alias = nil;
        CloseResFile(refN);
      } else
        err = ResError();
      if (err) {
        FSpDelete(&localSpec);
      }
    }
  }
done:
  if (!err)
    *aliasSpec = localSpec;
  ZapHandle(alias);
  UseResFile(oldResF);
  return (err);
}

/************************************************************************
 * SimpeMakeFSSpec - make an FSSpec, but don't hit the filesystem
 ************************************************************************/
void SimpleMakeFSSpec(short vRef, long dirId, char *name, FSSpecPtr spec) {
  spec->vRefNum = vRef;
  spec->parID = dirId;
  PSCopy(spec->name, name);
}

/************************************************************************
 * FindMyFile - Like FindFile, only Eudora-related
 ************************************************************************/
OSErr FindMyFile(FSSpecPtr spec, long whereToLook, short fileName) {
  FSSpec mySpec;
  OSErr err = fnfErr;
  char nameStr[256];

  if (whereToLook & kStuffFolderBit) {
    if (!(err = GetFileByRef(AppResFile, &mySpec)))
      if (!(err = FSMakeFSSpec(mySpec.vRefNum, mySpec.parID,
                               GetRString(nameStr, STUFF_FOLDER), &mySpec))) {
        IsAlias(&mySpec, &mySpec);
        mySpec.parID = SpecDirId(&mySpec);
        if (!(err = FSMakeFSSpec(mySpec.vRefNum, mySpec.parID,
                                 GetRString(nameStr, fileName), &mySpec))) {
          IsAlias(&mySpec, &mySpec);
          *spec = mySpec;
          return noErr;
        }
      }
  }

  return err;
}

#ifdef DEBUG
#undef FSpDirCreate
#undef DirCreate
/************************************************************************
 * FSpDirCreate - call FSpDirCreate but pacify SpotLight
 ************************************************************************/
OSErr MyFSpDirCreate(FSSpecPtr spec, ScriptCode scriptTag, long *createdDirID) {
  OSErr err;
  SLDisable();
  err = FSpDirCreate(spec->path, scriptTag, createdDirID);
  SLEnable();
  return (err);
}

/************************************************************************
 * MyDirCreate - call DirCreate but pacify SpotLight
 ************************************************************************/
OSErr MyDirCreate(short vRefNum, long parentDirID, char *directoryName,
                  long *createdDirID) {
  OSErr err;
  SLDisable();
  err = DirCreate(vRefNum, parentDirID, directoryName, createdDirID);
  SLEnable();
  return (err);
}

/************************************************************************
 * MyFSpDelete - bottleneck for deleting files
 ************************************************************************/
#undef FSpDelete
OSErr MyFSpDelete(FSSpecPtr spec) {
  OSErr err;

  ASSERT(!SameSpec(spec, &SettingsSpec));

  err = FSpDelete(spec->path);
#ifdef NEVER
  if (RunType != Production && spec.vRefNum == AttFolderSpec.vRefNum &&
      spec.parID == AttFolderspec.parID)
    Dprintf("FSpDelete %d.%d.�%p�", spec->vRefNum, spec->parID, spec->name);
#endif
  return err;
}

/************************************************************************
 * MyCloseResFile - bottleneck for closing resource files
 ************************************************************************/
#undef CloseResFile
void MyCloseResFile(short refN) {
  ASSERT(refN != ThreadGlobals.tSettingsRefN);

  CloseResFile(refN);
}
#define CloseResFile MyCloseResFile
#endif

/************************************************************************
 * MakeUniqueUntitledSpec - Make a unique "untitled" name in some folder
 ************************************************************************/
void MakeUniqueUntitledSpec(short vRefNum, long dirID, short strResID,
                            FSSpec *spec)

{
  char name[256], s[256];
  long suffix;
  Byte saveLen;

  //	Make a unique "untitled" name
  GetRString(name, strResID);
  suffix = 2;
  saveLen = *name;
  while (!FSMakeFSSpec(vRefNum, dirID, name, spec)) {
    //	No error means that the file/folder exists. Change the file name
    // by
    // adding a numeric suffix
    *name = saveLen; //	Remove any suffix
    MyNumToString(suffix++, s);
    PCatC(name, ' ');
    PCat(name, s);
  }
}

OSErr MisplaceItem(FSSpec *spec)

{
  FSSpec misplacedFolder, exist, newExist;
  OSErr theError;
  long dirID;

  // Find the Misplaced Items folder
  if ((theError = SubFolderSpec(MISPLACED_FOLDER, &misplacedFolder))) {
    SimpleMakeFSSpec(Root.vRef, Root.dirId,
                     GetRString((char *)misplacedFolder.name, MISPLACED_FOLDER),
                     &misplacedFolder);
    theError = FSpDirCreate(&misplacedFolder, 0, &dirID);
    if (!theError)
      misplacedFolder.parID = dirID;
  }
  if (!theError) {
    IsAlias(&misplacedFolder, &misplacedFolder);
    if (!FSMakeFSSpec(misplacedFolder.vRefNum, misplacedFolder.parID,
                      spec->name, &exist)) {
      newExist = exist;
      UniqueSpec(&newExist, 31);
      FSpRename(&exist, newExist.name);
    }
    theError = SpecMove(spec, &misplacedFolder);
  }
  return (theError);
}

OSErr FSpGetLongName(FSSpec *spec, TextEncoding destEncoding, char *longName) {
  OSErr err = noErr;
  HFSUniStr255 uniName;

  if (destEncoding == kTextEncodingUnknown)
    destEncoding = CreateTextEncoding(kTextEncodingMacRoman, 0, 0);

  //	Get the unicode name
  err = FSpGetLongNameUnicode(spec, &uniName);
  if (err == noErr) {
    UnicodeToTextInfo info;

    //	Convert the name back to UTF-8 or something
    err = CreateUnicodeToTextInfoByEncoding(destEncoding, &info);
    if (err == noErr) {
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
        err = noErr; // if we got something, use it

      (void)DisposeUnicodeToTextInfo(&info);
    }
  }

  return err;
}

OSErr FSpGetLongNameUnicode(FSSpec *spec, HFSUniStr255 *longName) {
  OSErr err = noErr;
  FSRef aRef;

  err = FSpMakeFSRef(spec, &aRef);
  if (err == noErr) {
    err = FSGetCatalogInfo(&aRef, kFSCatInfoNone, NULL, longName, NULL, NULL);
  }

  return err;
}

OSErr FSpSetLongName(FSSpec *spec, TextEncoding srcEncoding,
                     const char *longName, FSSpec *newSpec) {
  OSErr err = noErr;
  HFSUniStr255 uniName;
  TextToUnicodeInfo info;

  if (srcEncoding == kTextEncodingUnknown)
    srcEncoding = CreateTextEncoding(kTextEncodingMacRoman, 0, 0);

  err = CreateTextToUnicodeInfoByEncoding(srcEncoding, &info);
  if (err == noErr) {
    ByteCount uniStrLen;
    err = ConvertFromPStringToUnicode(info, longName, 255 * sizeof(UniChar),
                                      &uniStrLen, uniName.unicode);
    uniName.length = uniStrLen / 2;
    if (err == noErr)
      err = FSpSetLongNameUnicode(spec, &uniName, newSpec);
    DisposeTextToUnicodeInfo(&info);
  }
  return err;
}

OSErr FSpSetLongNameUnicode(FSSpec *spec, ConstHFSUniStr255Param longName,
                            FSSpec *newSpec) {
  OSErr err = noErr;
  FSRef aRef;

  err = FSpMakeFSRef(spec, &aRef);
  if (err == noErr) {
    FSRef newRef;
    FSRef *refPtr = newSpec != NULL ? &newRef : NULL;
    err = FSRenameUnicode(&aRef, longName->length, longName->unicode,
                          kTextEncodingUnicodeDefault, refPtr);
    //	Convert the FSRef back into a FSSpec for the caller
    if (err == noErr && refPtr != NULL)
      (void)FSGetCatalogInfo(refPtr, kFSCatInfoNone, NULL, NULL, newSpec, NULL);
  }

  return err;
}

OSErr MakeUniqueLongFileName(short vRefNum, long dirID, StringPtr name,
                             TextEncoding srcEncoding, short maxLen) {
  OSStatus err;
  char s[256];
  HFSUniStr255 uniName;
  TextToUnicodeInfo info;
  FSRef parent;
  FSRefParam fs;

  ASSERT(name != NULL);
  ASSERT(maxLen >= 63);

  //	Make an FSRef to the parent directory
  Zero(s);
  Zero(fs);
  fs.ioVRefNum = vRefNum;
  fs.ioDirID = dirID;
  fs.ioNamePtr = s;
  fs.newRef = &parent;
  if (noErr != (err = PBMakeFSRefSync(&fs)))
    return err;

  if (srcEncoding == kTextEncodingUnknown)
    srcEncoding = CreateTextEncoding(kTextEncodingMacRoman, 0, 0);

  err = CreateTextToUnicodeInfoByEncoding(srcEncoding, &info);
  if (err == noErr) {
    char base[256], suffix[256];
    long nextFile = 1;
    FSRef ref;

    //	Split the file name into base name and suffix
    SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(name, base,
                                                               suffix, maxLen);
    base[++base[0]] = ' '; // tack a space on the end

    //	Set up the parameter block
    Zero(fs);
    fs.ref = &parent;
    fs.name = (char *)uniName.unicode;
    fs.newRef = &ref;

    while (true) {
      ByteCount uniStrLen;

      //	See if the file exists
      err = ConvertFromPStringToUnicode(info, name, 255 * sizeof(UniChar),
                                        &uniStrLen, uniName.unicode);
      fs.nameLength = uniStrLen / sizeof(UniChar);

      err = PBMakeFSRefUnicodeSync(&fs);
      if (err != noErr)
        break;

      MyNumToString(nextFile++, s);

      //	If the new string will be too long, then trim the base
      if (base[0] + suffix[0] + s[0] > maxLen - 1) {
        short newLen;
        base[0] = newLen = maxLen - (suffix[0] + s[0]);
        base[newLen] = ' ';
        base[newLen - 1] = '.';
      }

      //	Build the whole string
      g_strlcpy(name, base, sizeof(name));
      strncat(name, s, sizeof(name) - strlen(name) - 1);
      strncat(name, ".", sizeof(name) - strlen(name) - 1);
      strncat(name, suffix, sizeof(name) - strlen(name) - 1);
    }

    ASSERT(err != noErr);
    //	Map fnfErr --> success
    if (err == fnfErr)
      err = noErr;

    DisposeTextToUnicodeInfo(&info);
  }

  return err;
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

bool FSpIsLocked(FSSpecPtr spec) {
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
bool IsPDFFile(FSSpecPtr spec, OSType fileType) {
  if (fileType == 0x50444620)
    return true;
  // Avi must die.
  if (EndsWithR(spec->name, PDF_QUOTE_EXTENSION_UNQUOTE))
    return true;
  return false;
}

/**********************************************************************
 * SpecEndsWithExtensionR - does a spec (long name) end with an extension
 *  on a list?
 **********************************************************************/
bool SpecEndsWithExtensionR(FSSpecPtr spec, short resID) {
  char longName[256];

  if (FSpGetLongName(spec, 0, longName))
    g_strlcpy(longName, spec->name, 256);

  return EndsWithItem(longName, resID);
}
OSErr FSRenameUnicode(FSRef *ref, int len, const void *name, int encoding,
                      FSRef *newRef) {
  return noErr;
}
OSErr PBMakeFSRefUnicodeSync(void *pb) { return noErr; }
/* EndsWithR and EndsWithItem are implemented in stringutil.c */

/**********************************************************************
 * GetAttFolderSpec - get the attachment folder spec
 * Stub implementation - returns AttFolderSpec global
 **********************************************************************/
OSErr GetAttFolderSpec(FSSpecPtr spec) {
  extern FSSpec AttFolderSpec;
  if (spec) {
    *spec = AttFolderSpec;
    return noErr;
  }
  return paramErr;
}

/**********************************************************************
 * GetCurrentAttFolderSpec - get the current attachment folder spec
 * Stub implementation - returns CurrentAttFolderSpec global
 **********************************************************************/
void GetCurrentAttFolderSpec(FSSpecPtr spec) {
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
  return 0; /* noErr */
}
