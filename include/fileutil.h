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
int ParentSpec(char *child, char *parent);
char *GetMyVolName(short refNum, char *name);
int BlessedDirID(long *sysDirIDPtr);
short BlessedVRef(void);
int IndexVRef(short index, short *vRef);
int MakeResFile(const char *name, const char *dir, long creator,
                long type);
// void free(void *h);
int ExchangeAndDel(char *tmpSpec, char *spec);
typedef struct {
  FSSpec spec;
  bool isDir;
  bool isSymLink;
  long size;
  time_t createDate;
  time_t modifyDate;
  void *data;
} DirIterateInfo;

int DirIterate(const char *dir, void *data,
               bool (*callback)(DirIterateInfo *info));
int CopyFBytes(short fromRefN, long fromOffset, long length, short toRefN,
               long toOffset);
void StdFileSpot(Point *where, short id);
short ARFHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
int MyAllocate(short refN, long size);
short SFPutOpen(char *spec, long creator, long type, short *refN,
                void *filter, void *hook, short id,
                char *defaultSpec, const char *windowTitle,
                const char *message);
bool IsText(char *spec);
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
int FSpDirCreate(char *spec, ScriptCode script, long *dirID);
bool AliasFolderType(OSType type);
#define FSpIsItAFolder(spec)                                                   \
  IsItAFolder(0, 0, spec_name(spec))
bool AFSpIsItAFolder(char *spec);
short FolderFileCount(char *spec);
short AFSHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
short GetMyWD(short vRef, long dirID);
short HMove(short vRef, long fromDirId, const char *fromName, long toDirId,
            const char *toName);
int AHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
int FSpGetHFileInfo(char *spec, CInfoPBRec *hfi);
long FSpFileSize(char *spec);
long FSpDFSize(char *spec);
long FSpRFSize(char *spec);
int SubFolderSpec(short nameId, char *spec);
int StuffFolderSpec(char *spec);
int FindSubFolderSpec(long domain, long folder, short subfolderID, bool create,
                      char *subSpec);
int SubFolderSpecOf(char *inSpec, short subfolderID, bool create,
                    char *subSpec);
int SubFolderSpecOfStr(char *inSpec, const char *subfolderName, bool create,
                       char *subSpec);
uint32_t FSpModDate(char *spec);
short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
bool GetFolder(char *name, short *volume, long *folder, bool writeable,
               bool system, bool floppy, bool desktop);
#define FSpCopyRFork(t, f, p)                                                  \
  CopyRFork(0, 0, spec_name(t), 0, 0, spec_name(f), p)
#define FSpCopyDFork(t, f, p)                                                  \
  CopyDFork(0, 0, spec_name(t), 0, 0, spec_name(f), p)
#define FSpCopyFInfo(t, f)                                                     \
  CopyFInfo(0, 0, spec_name(t), 0, 0, spec_name(f))
short CopyFork(short vRef, long dirId, const char *name, short fromVRef,
               long fromDirId, const char *fromName, bool rFork, bool progress);
#define CopyRFork(v, d, n, fv, fd, fn, p) CopyFork(v, d, n, fv, fd, fn, true, p)
#define CopyDFork(v, d, n, fv, fd, fn, p)                                      \
  CopyFork(v, d, n, fv, fd, fn, false, p)
short CopyFInfo(short vRef, long dirId, const char *name, short fromVRef,
                long fromDirId, const char *fromName);
int MyUpdateResFile(short resFile);
int FSpDupFile(char *to, char *from, bool replace, bool progress);
int FSpDupFolder(char *toSpec, char *fromSpec, bool replace,
                 bool progress);
int RemoveDir(char *spec);
int MakeDarnSure(short refN);
int FlushFile(short refN);
int UniqueSpec(char *spec, short max);
int SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
    const char *full, char *name, char *ext, short max);
short NCWriteP(short refN, const char *pString);
short AWriteP(short refN, const char *pString);
int TweakFileType(char *spec, OSType type, OSType creator);
int HuntNewline(short refN, long aroundSpot, long *newline, bool *realNl);
int Snarf(char *spec, void ***hp, long limit);
int SnarfRoman(char *spec, void ***hp, long limit);
short MyResolveAlias(const char *dir, char *name, bool *wasAlias);
#define FSpMyResolve(s, wasAlias)                                              \
  MyResolveAlias((s), (char *)spec_name(s), wasAlias)
void PromptGetFile(FileFilterProcPtr filter, DlgHookYDProcPtr hook,
                   long hookData, short numTypes, SFTypeList tl,
                   StandardFileReply *reply, char *prompt);
/* Use portable unsigned-byte pointer for write-params */
short FSWriteP(short refN, unsigned char *pString);
short GetFileByRef(short refN, char *specPtr);
int AFSpSetMod(char *spec, uint32_t mod);
uint32_t AFSpGetMod(char *spec);
int ChainDelete(char *spec);
long VolumeFree(short vRef);
bool SpecInSubfolderOf(char *att, char *folder);
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
short AFSpGetCatInfo(char *spec, char *newSpec, CInfoPBRec *hfi);
int FSpKillRFork(char *spec);
int FSpRFSane(char *spec, bool *sane);
int TruncOpenFile(short refN, long spot);
int EnsureNewline(short refN);
OSType FileTypeOf(char *spec);
OSType FileCreatorOf(char *spec);
int FSpTrash(char *spec);
char *Mac2OtherName(const char *mac, char *other);
#define Other2MacName(x, y)                                                    \
  SanitizeFN((const char *)(x), (char *)(y), MAC_FN_BAD, MAC_FN_REP, true)
char *SanitizeFN(const char *shortName, char *name, short badCharId,
                 short repCharId, bool kill8);
/* Portable filesystem helpers implemented in src/fileutil.c */
int MyFSpOpenDF(char *spec, short mode, short *refN);
int MyFSpOpenRF(const char *path, short mode, short *refN);
int MyFSpDelete(char *spec);
int MyFSpGetFInfo(char *spec, char *newSpec, FInfo *fndrInfo);
int MyFSpSetFInfo(char *spec, char *newSpec, FInfo *fndrInfo);
int MyFSpSetFLock(char *spec, char *newSpec);
int MyFSpRstFLock(char *spec, char *newSpec);
int MyFSpCreate(char *spec, uint32_t creator, uint32_t fileType, ScriptCode script);
int MyFSpRename(char *spec, const char *newName);
int MyFSpExchangeFiles(char *source, char *dest);
short MyFSpCreateResFile(char *spec, uint32_t creator, uint32_t type, ScriptCode script);
int MyAHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
short MyAHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
int MyFSpSetMod(char *spec, uint32_t mod);
uint32_t MyFSpGetMod(char *spec);
short MyFSpGetCatInfo(char *spec, char *newSpec, CInfoPBRec *hfi);
short MyFSpGetHFileInfo(char *spec, CInfoPBRec *hfi);
short MyFSpSetHFileInfo(char *spec, CInfoPBRec *hfi);
void *MyGet1IndResource(OSType type, short index);
void *MyGetIndResource(OSType type, short index);
void MyCloseResFile(short refN);
bool MyFSpIsItAFolder(char *spec);
int FSpSetFXInfo(char *spec, FXInfo *fxInfo);
bool IsAlias(char *spec, char *newSpec);
int ResolveAliasOrElse(char *spec, char *newSpec, bool *wasIt);
int FSMakeFID(char *spec, long *fid);
int FSResolveFID(short vRef, long fid, char *spec);
int DTRef(short vRef, short *dtRef);
int DTGetAppl(short vRef, short dtRef, OSType creator, char *appSpec);
short DTFindAppl(OSType creator);
int DTSetComment(char *spec, char *comment);
int MorphDesktop(short vRef, char *where);
int Blat(char *spec, void *text, bool append);
int BlatPtr(char *spec, char *text, long size, bool append);
#define SameVRef(vr1, vr2) (vr1 == vr2)
bool SameSpec(char *sp1, char *sp2);
short RealVRef(short wdRef);
long SpecDirId(char *spec);
bool IsRoot(const char *path);
int CanWrite(char *spec, bool *can);
int SpecMove(char *moveMe, char *moveTo);
int SpecMoveAndRename(char *moveMe, char *moveTo);
int GetTrashSpec(short vRef, char *spec);
int WipeSpec(char *spec);
int WipeDiskArea(short refN, long offset, long len);
int NewTempExtSpec(short vRef, char *name, short extId, char *spec);
int ExchangeFiles(char *tmpSpec, char *spec);
int FSpTouch(char *spec);
int ExtractCreatorFromBndl(char *spec, OSType *creator);
int CreatorToName(OSType creator, char *appName);
int NewTempSpec(short vRef, long dirId, char *name, char *spec);
int FSpExists(char *spec);
int AddUniqueExt(char *spec, short extId);
int VolumeMargin(short vRef, long spaceNeeded);
bool DiskSpunUp(void);
short SFPutNew(char *spec);
int FindTemporaryFolder(short vRef, long dirId, long *tempDirId,
                          short *tempVRef);
bool IsPDFFile(char *spec, OSType fileType);
#define kStuffFolderBit 0x1
int FindMyFile(char *spec, long whereToLook, short fileName);

void MakeUniqueUntitledSpec(const char *dir, short strResID,
                            char *spec);
int MisplaceItem(char *spec);
int FSpGetLongName(char *spec, TextEncoding destEncoding, char *longName);
int FSpGetLongNameUnicode(char *spec, HFSUniStr255 *longName);

int FSpSetLongName(char *spec, TextEncoding destEncoding,
                     const char *longName, char *newName);
int FSpSetLongNameUnicode(char *spec, ConstHFSUniStr255Param longName,
                            char *newName);

int MakeUniqueLongFileName(short vRefNum, long dirID, char *name,
                             TextEncoding srcEncoding, short maxLen);

/* Refactored to portable paths */
bool FSpIsLocked(char *spec);
bool SpecEndsWithExtensionR(char *spec, short resID);

#define kTrashFolderType 'trsh'
#define kTemporaryFolderType 'temp'
#define kCreateFolder true
#define permErr -54
#define afpAccessDenied -5000
#define fidExists -1301
#define afpIDExists -5019
#define gestaltSystemVersion 'sysv'

/* Attachment folder functions */
int GetAttFolderSpec(char *spec);
void GetCurrentAttFolderSpec(char *spec);
bool TypeIsOnListWhereAndIndex(long type, short list, void *ptr, short *index);

/* SettingsRefN may be defined as a macro mapping to thread-local storage */
#ifndef SettingsRefN
extern short SettingsRefN;
#endif

#endif
