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

#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include "mydefs.h"

/* Mac Memory Manager compatibility */
int MemError(void);
size_t GetHandleSize(void *h);

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

int TruncAtMark(short refN);
short GetMyVR(const char *name);
long GetMyDirID(short refNum);
short GetDirName(char *volName, short vRef, long dirId, char *name);
int ParentSpec(FSSpec *child, FSSpec *parent);
char *GetMyVolName(short refNum, char *name);
int BlessedDirID(long *sysDirIDPtr);
short BlessedVRef(void);
int IndexVRef(short index, short *vRef);
int MakeResFile(const char *name, const char *dir, long creator,
                long type);
// void free(void *h);
int ExchangeAndDel(FSSpec *tmpSpec, FSSpec *spec);
typedef struct {
  FSSpec spec;
  bool isDir;
  bool isSymLink;
  long size;
  time_t createDate;
  time_t modifyDate;
  void *data;
} DirIterateInfo;

int DirIterate(const FSSpec *dir, void *data,
               bool (*callback)(DirIterateInfo *info));
int CopyFBytes(short fromRefN, long fromOffset, long length, short toRefN,
               long toOffset);
void StdFileSpot(Point *where, short id);
short ARFHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
int MyAllocate(short refN, long size);
short SFPutOpen(FSSpec *spec, long creator, long type, short *refN,
                void *filter, void *hook, short id,
                FSSpec *defaultSpec, const char *windowTitle,
                const char *message);
bool IsText(FSSpec *spec);
short MyOpenResFile(short vRef, long dirId, const char *name);
short SpinOnLo(volatile int *rtnCodeAddr, long maxTicks, bool allowCancel,
               bool forever, bool remainCalm, bool allowMouseDown);
#define SpinOn(r, mt, ac, f) SpinOnLo(r, mt, ac, f, false, false)
#define FSZWrite(refN, count, buf)                                             \
  (((*count) > 0) ? AWrite(refN, count, buf) : 0)
bool IsItAFolder(short vRef, long inDirId, const char *name);
bool HFIIsFolder(CInfoPBRec *hfi);
bool HFIIsFolderOrAlias(CInfoPBRec *hfi);
void FolderSizeHi(const char *dir, uint32_t *cumSize);
void FolderSize(const char *dir, CInfoPBRec *hfi, uint32_t *cumSize);
int FSpDirCreate(FSSpec *spec, ScriptCode script, long *dirID);
bool AliasFolderType(OSType type);
#define FSpIsItAFolder(spec)                                                   \
  IsItAFolder((spec)->vRefNum, (spec)->parID, (const char *)(spec)->name)
bool AFSpIsItAFolder(FSSpec *spec);
short FolderFileCount(FSSpec *spec);
short AFSHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
short GetMyWD(short vRef, long dirID);
short HMove(short vRef, long fromDirId, const char *fromName, long toDirId,
            const char *toName);
int AHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
int FSpGetHFileInfo(FSSpec *spec, CInfoPBRec *hfi);
long FSpFileSize(FSSpec *spec);
long FSpDFSize(FSSpec *spec);
long FSpRFSize(FSSpec *spec);
int SubFolderSpec(short nameId, FSSpec *spec);
int StuffFolderSpec(FSSpec *spec);
int FindSubFolderSpec(long domain, long folder, short subfolderID, bool create,
                      FSSpec *subSpec);
int SubFolderSpecOf(FSSpec *inSpec, short subfolderID, bool create,
                    FSSpec *subSpec);
int SubFolderSpecOfStr(FSSpec *inSpec, const char *subfolderName, bool create,
                       FSSpec *subSpec);
uint32_t FSpModDate(FSSpec *spec);
short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
bool GetFolder(char *name, short *volume, long *folder, bool writeable,
               bool system, bool floppy, bool desktop);
#define FSpCopyRFork(t, f, p)                                                  \
  CopyRFork((t)->vRefNum, (t)->parID, (const char *)(t)->name, (f)->vRefNum,   \
            (f)->parID, (const char *)(f)->name, p)
#define FSpCopyDFork(t, f, p)                                                  \
  CopyDFork((t)->vRefNum, (t)->parID, (const char *)(t)->name, (f)->vRefNum,   \
            (f)->parID, (const char *)(f)->name, p)
#define FSpCopyFInfo(t, f)                                                     \
  CopyFInfo((t)->vRefNum, (t)->parID, (const char *)(t)->name, (f)->vRefNum,   \
            (f)->parID, (const char *)(f)->name)
short CopyFork(short vRef, long dirId, const char *name, short fromVRef,
               long fromDirId, const char *fromName, bool rFork, bool progress);
#define CopyRFork(v, d, n, fv, fd, fn, p) CopyFork(v, d, n, fv, fd, fn, true, p)
#define CopyDFork(v, d, n, fv, fd, fn, p)                                      \
  CopyFork(v, d, n, fv, fd, fn, false, p)
short CopyFInfo(short vRef, long dirId, const char *name, short fromVRef,
                long fromDirId, const char *fromName);
int MyUpdateResFile(short resFile);
int FSpDupFile(FSSpec *to, FSSpec *from, bool replace, bool progress);
int FSpDupFolder(FSSpec *toSpec, FSSpec *fromSpec, bool replace,
                 bool progress);
int RemoveDir(FSSpec *spec);
int MakeDarnSure(short refN);
int FlushFile(short refN);
int UniqueSpec(FSSpec *spec, short max);
int SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
    const char *full, char *name, char *ext, short max);
short NCWriteP(short refN, const char *pString);
short AWriteP(short refN, const char *pString);
int TweakFileType(FSSpec *spec, OSType type, OSType creator);
int HuntNewline(short refN, long aroundSpot, long *newline, bool *realNl);
int Snarf(FSSpec *spec, void ***hp, long limit);
int SnarfRoman(FSSpec *spec, void ***hp, long limit);
short MyResolveAlias(const char *dir, char *name, bool *wasAlias);
#define FSpMyResolve(s, wasAlias)                                              \
  MyResolveAlias((s)->path, (s)->name, wasAlias)
void PromptGetFile(FileFilterProcPtr filter, DlgHookYDProcPtr hook,
                   long hookData, short numTypes, SFTypeList tl,
                   StandardFileReply *reply, char *prompt);
/* Use portable unsigned-byte pointer for write-params */
short FSWriteP(short refN, unsigned char *pString);
short GetFileByRef(short refN, FSSpec *specPtr);
int AFSpSetMod(FSSpec *spec, uint32_t mod);
uint32_t AFSpGetMod(FSSpec *spec);
int ChainDelete(FSSpec *spec);
long VolumeFree(short vRef);
bool SpecInSubfolderOf(FSSpec *att, FSSpec *folder);
short FSTabWrite(short refN, long *count, unsigned char *buf);
short ARead(short refN, long *count, unsigned char *buf);
short AWrite(short refN, long *count, unsigned char *buf);
short GetEOF(short refNum, long *logEOF);
short SetEOF(short refNum, long logEOF);
short GetFPos(short refNum, long *filePos);
short SetFPos(short refNum, short posMode, long posOff);
short NCWrite(short refN, long *count, unsigned char *buf);
/* SimpleMakeFSSpec removed — use spec_make(dir, name, spec) instead */
short HGetCatInfo(short vRef, long inDirId, const char *name, CInfoPBRec *hfi);
short HSetCatInfo(short vRef, long inDirId, const char *name, CInfoPBRec *hfi);
short AFSpGetCatInfo(FSSpec *spec, FSSpec *newSpec, CInfoPBRec *hfi);
int FSpKillRFork(FSSpec *spec);
int FSpRFSane(FSSpec *spec, bool *sane);
int TruncOpenFile(short refN, long spot);
int EnsureNewline(short refN);
OSType FileTypeOf(FSSpec *spec);
OSType FileCreatorOf(FSSpec *spec);
int FSpTrash(FSSpec *spec);
char *Mac2OtherName(const char *mac, char *other);
#define Other2MacName(x, y)                                                    \
  SanitizeFN((const char *)(x), (char *)(y), MAC_FN_BAD, MAC_FN_REP, true)
char *SanitizeFN(const char *shortName, char *name, short badCharId,
                 short repCharId, bool kill8);
/* Portable filesystem helpers implemented in src/fileutil.c */
int MyFSpOpenDF(FSSpec *spec, short mode, short *refN);
int MyFSpOpenRF(const char *path, short mode, short *refN);
int MyFSpDelete(FSSpec *spec);
int MyFSpGetFInfo(FSSpec *spec, FSSpec *newSpec, FInfo *fndrInfo);
int MyFSpSetFInfo(FSSpec *spec, FSSpec *newSpec, FInfo *fndrInfo);
int MyFSpSetFLock(FSSpec *spec, FSSpec *newSpec);
int MyFSpRstFLock(FSSpec *spec, FSSpec *newSpec);
int MyFSpCreate(FSSpec *spec, uint32_t creator, uint32_t fileType, ScriptCode script);
int MyFSpRename(FSSpec *spec, const char *newName);
int MyFSpExchangeFiles(FSSpec *source, FSSpec *dest);
short MyFSpCreateResFile(FSSpec *spec, uint32_t creator, uint32_t type, ScriptCode script);
int MyAHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
short MyAHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
int MyFSpSetMod(FSSpec *spec, uint32_t mod);
uint32_t MyFSpGetMod(FSSpec *spec);
short MyFSpGetCatInfo(FSSpec *spec, FSSpec *newSpec, CInfoPBRec *hfi);
short MyFSpGetHFileInfo(FSSpec *spec, CInfoPBRec *hfi);
short MyFSpSetHFileInfo(FSSpec *spec, CInfoPBRec *hfi);
void *MyGet1IndResource(OSType type, short index);
void *MyGetIndResource(OSType type, short index);
void MyCloseResFile(short refN);
bool MyFSpIsItAFolder(FSSpec *spec);
int FSpSetFXInfo(FSSpec *spec, FXInfo *fxInfo);
bool IsAlias(FSSpec *spec, FSSpec *newSpec);
int ResolveAliasOrElse(FSSpec *spec, FSSpec *newSpec, bool *wasIt);
int FSMakeFID(FSSpec *spec, long *fid);
int FSResolveFID(short vRef, long fid, FSSpec *spec);
int DTRef(short vRef, short *dtRef);
int DTGetAppl(short vRef, short dtRef, OSType creator, FSSpec *appSpec);
short DTFindAppl(OSType creator);
int DTSetComment(FSSpec *spec, char *comment);
int MorphDesktop(short vRef, FSSpec *where);
int Blat(FSSpec *spec, void *text, bool append);
int BlatPtr(FSSpec *spec, char *text, long size, bool append);
#define SameVRef(vr1, vr2) (vr1 == vr2)
bool SameSpec(FSSpec *sp1, FSSpec *sp2);
short RealVRef(short wdRef);
long SpecDirId(FSSpec *spec);
bool IsRoot(const char *path);
int CanWrite(FSSpec *spec, bool *can);
int SpecMove(FSSpec *moveMe, FSSpec *moveTo);
int SpecMoveAndRename(FSSpec *moveMe, FSSpec *moveTo);
int GetTrashSpec(short vRef, FSSpec *spec);
int WipeSpec(FSSpec *spec);
int WipeDiskArea(short refN, long offset, long len);
int NewTempExtSpec(short vRef, char *name, short extId, FSSpec *spec);
int ExchangeFiles(FSSpec *tmpSpec, FSSpec *spec);
int FSpTouch(FSSpec *spec);
int ExtractCreatorFromBndl(FSSpec *spec, OSType *creator);
int CreatorToName(OSType creator, char *appName);
int NewTempSpec(short vRef, long dirId, char *name, FSSpec *spec);
int FSpExists(FSSpec *spec);
int AddUniqueExt(FSSpec *spec, short extId);
int VolumeMargin(short vRef, long spaceNeeded);
bool DiskSpunUp(void);
short SFPutNew(FSSpec *spec);
int FindTemporaryFolder(short vRef, long dirId, long *tempDirId,
                          short *tempVRef);
bool IsPDFFile(FSSpec *spec, OSType fileType);
#define kStuffFolderBit 0x1
int FindMyFile(FSSpec *spec, long whereToLook, short fileName);

void MakeUniqueUntitledSpec(const char *dir, short strResID,
                            FSSpec *spec);
int MisplaceItem(FSSpec *spec);
int FSpGetLongName(FSSpec *spec, TextEncoding destEncoding, char *longName);
int FSpGetLongNameUnicode(FSSpec *spec, HFSUniStr255 *longName);

int FSpSetLongName(FSSpec *spec, TextEncoding destEncoding,
                     const char *longName, FSSpec *newName);
int FSpSetLongNameUnicode(FSSpec *spec, ConstHFSUniStr255Param longName,
                            FSSpec *newName);

int MakeUniqueLongFileName(short vRefNum, long dirID, char *name,
                             TextEncoding srcEncoding, short maxLen);

/* Refactored to portable paths */
bool FSpIsLocked(FSSpec *spec);
bool SpecEndsWithExtensionR(FSSpec *spec, short resID);

#define kTrashFolderType 'trsh'
#define kTemporaryFolderType 'temp'
#define kCreateFolder true
#define permErr -54
#define afpAccessDenied -5000
#define fidExists -1301
#define afpIDExists -5019
#define gestaltSystemVersion 'sysv'

/* Attachment folder functions */
int GetAttFolderSpec(FSSpec *spec);
void GetCurrentAttFolderSpec(FSSpec *spec);
bool TypeIsOnListWhereAndIndex(long type, short list, void *ptr, short *index);

/* SettingsRefN may be defined as a macro mapping to thread-local storage */
#ifndef SettingsRefN
extern short SettingsRefN;
#endif

#endif
