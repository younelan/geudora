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

#include "mailbox.h"

/* Mac Memory Manager compatibility */
int MemError(void);
size_t GetHandleSize(Handle h);

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

int TruncAtMark(short refN);
short GetMyVR(const char *name);
long GetMyDirID(short refNum);
short GetDirName(char *volName, short vRef, long dirId, char *name);
int ParentSpec(FSSpecPtr child, FSSpecPtr parent);
char *GetMyVolName(short refNum, char *name);
int BlessedDirID(long *sysDirIDPtr);
short BlessedVRef(void);
int IndexVRef(short index, short *vRef);
int MakeResFile(const char *name, int vRef, long dirId, long creator,
                long type);
// void ZapHandle(Handle h);
int ExchangeAndDel(FSSpecPtr tmpSpec, FSSpecPtr spec);
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
short SFPutOpen(FSSpecPtr spec, long creator, long type, short *refN,
                ModalFilterYDUPP filter, DlgHookYDUPP hook, short id,
                FSSpecPtr defaultSpec, const char *windowTitle,
                const char *message);
bool IsText(FSSpecPtr spec);
short MyOpenResFile(short vRef, long dirId, const char *name);
short SpinOnLo(volatile int *rtnCodeAddr, long maxTicks, bool allowCancel,
               bool forever, bool remainCalm, bool allowMouseDown);
#define SpinOn(r, mt, ac, f) SpinOnLo(r, mt, ac, f, false, false)
#define FSZWrite(refN, count, buf)                                             \
  (((*count) > 0) ? AWrite(refN, count, buf) : 0)
bool IsItAFolder(short vRef, long inDirId, const char *name);
bool HFIIsFolder(CInfoPBRec *hfi);
bool HFIIsFolderOrAlias(CInfoPBRec *hfi);
void FolderSizeHi(short vRef, long dirID, uint32_t *cumSize);
void FolderSize(short vRef, long dirID, CInfoPBRec *hfi, uint32_t *cumSize);
#ifdef DEBUG
#define FSpDirCreate MyFSpDirCreate
#define DirCreate MyDirCreate
int MyFSpDirCreate(FSSpecPtr spec, ScriptCode scriptTag, long *createdDirID);
int MyDirCreate(short vRefNum, long parentDirID, const char *directoryName,
                long *createdDirID);
#endif
bool AliasFolderType(OSType type);
#define FSpIsItAFolder(spec)                                                   \
  IsItAFolder((spec)->vRefNum, (spec)->parID, (const char *)(spec)->name)
bool AFSpIsItAFolder(FSSpecPtr spec);
short FolderFileCount(FSSpecPtr spec);
short AFSHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
short GetMyWD(short vRef, long dirID);
short HMove(short vRef, long fromDirId, const char *fromName, long toDirId,
            const char *toName);
OSErr AHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
#define AFSpGetHFileInfo(spec, hfi)                                            \
  AHGetFileInfo((spec)->vRefNum, (spec)->parID, (const char *)(spec)->name, hfi)
#define AFSpSetHFileInfo(spec, hfi)                                            \
  AHSetFileInfo((spec)->vRefNum, (spec)->parID, (const char *)(spec)->name, hfi)
int FSpGetHFileInfo(FSSpecPtr spec, CInfoPBRec *hfi);
long FSpFileSize(FSSpecPtr spec);
long FSpDFSize(FSSpecPtr spec);
long FSpRFSize(FSSpecPtr spec);
int SubFolderSpec(short nameId, FSSpecPtr spec);
int StuffFolderSpec(FSSpecPtr spec);
int FindSubFolderSpec(long domain, long folder, short subfolderID, bool create,
                      FSSpecPtr subSpec);
int SubFolderSpecOf(FSSpecPtr inSpec, short subfolderID, bool create,
                    FSSpecPtr subSpec);
int SubFolderSpecOfStr(FSSpecPtr inSpec, const char *subfolderName, bool create,
                       FSSpecPtr subSpec);
uint32_t FSpModDate(FSSpecPtr spec);
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
int FSpDupFile(FSSpecPtr to, FSSpecPtr from, bool replace, bool progress);
int FSpDupFolder(FSSpecPtr toSpec, FSSpecPtr fromSpec, bool replace,
                 bool progress);
int RemoveDir(FSSpecPtr spec);
int MakeDarnSure(short refN);
int FlushFile(short refN);
int SimpleResolveAlias(AliasHandle alias, FSSpecPtr spec);
int SimpleResolveAliasNoUI(AliasHandle alias, FSSpecPtr spec);
int UniqueSpec(FSSpecPtr spec, short max);
OSErr SplitPerfectlyGoodFilenameIntoNameAndQuoteExtensionUnquote(
    const char *full, char *name, char *ext, short max);
short NCWriteP(short refN, unsigned char *pString);
short AWriteP(short refN, unsigned char *pString);
OSErr TweakFileType(FSSpecPtr spec, OSType type, OSType creator);
OSErr HuntNewline(short refN, long aroundSpot, long *newline, bool *realNl);
OSErr Snarf(FSSpecPtr spec, Handle *hp, long limit);
OSErr SnarfRoman(FSSpecPtr spec, Handle *hp, long limit);
short MyResolveAlias(short *vRef, long *dirId, char *name, bool *wasAlias);
#define FSpMyResolve(s, wasAlias)                                              \
  MyResolveAlias(&(s)->vRefNum, &(s)->parID, (s)->name, wasAlias)
void PromptGetFile(FileFilterProcPtr filter, DlgHookYDProcPtr hook,
                   long hookData, short numTypes, SFTypeList tl,
                   StandardFileReply *reply, PStr prompt);
short FSWriteP(short refN, unsigned char *pString);
short GetFileByRef(short refN, FSSpecPtr specPtr);
OSErr AFSpSetMod(FSSpecPtr spec, uint32_t mod);
uint32_t AFSpGetMod(FSSpecPtr spec);
OSErr ChainDelete(FSSpecPtr spec);
long VolumeFree(short vRef);
bool SpecInSubfolderOf(FSSpecPtr att, FSSpecPtr folder);
short FSTabWrite(short refN, long *count, unsigned char *buf);
short ARead(short refN, long *count, unsigned char *buf);
short AWrite(short refN, long *count, unsigned char *buf);
short GetEOF(short refNum, long *logEOF);
short SetEOF(short refNum, long logEOF);
short GetFPos(short refNum, long *filePos);
short SetFPos(short refNum, short posMode, long posOff);
short NCWrite(short refN, long *count, unsigned char *buf);
void SimpleMakeFSSpec(short vRef, long dirId, PStr name, FSSpecPtr spec);
short HGetCatInfo(short vRef, long inDirId, const char *name, CInfoPBRec *hfi);
short HSetCatInfo(short vRef, long inDirId, const char *name, CInfoPBRec *hfi);
short AFSpGetCatInfo(FSSpecPtr spec, FSSpecPtr newSpec, CInfoPBRec *hfi);
OSErr FSpKillRFork(FSSpecPtr spec);
OSErr FSpRFSane(FSSpecPtr spec, bool *sane);
OSErr TruncOpenFile(short refN, long spot);
OSErr EnsureNewline(short refN);
OSType FileTypeOf(FSSpecPtr spec);
OSType FileCreatorOf(FSSpecPtr spec);
OSErr FSpTrash(FSSpecPtr spec);
char *Mac2OtherName(const char *mac, char *other);
#define Other2MacName(x, y)                                                    \
  SanitizeFN((const char *)(x), (char *)(y), MAC_FN_BAD, MAC_FN_REP, true)
char *SanitizeFN(const char *shortName, char *name, short badCharId,
                 short repCharId, bool kill8);
OSErr EudoraFSpOpenDF(FSSpecPtr spec, short mode, short *refN);
OSErr FSpOpenRF(const char *path, short mode, short *refN);
OSErr EudoraFSpDelete(FSSpecPtr spec);
#undef FSpDirCreate
OSErr FSpDirCreate(FSSpecPtr spec, ScriptCode script, long *dirID);
OSErr AFSpGetFInfo(FSSpecPtr spec, FSSpecPtr newSpec, FInfo *fndrInfo);
OSErr AFSpSetFInfo(FSSpecPtr spec, FSSpecPtr newSpec, FInfo *fndrInfo);
OSErr AFSpSetFLock(FSSpecPtr spec, FSSpecPtr newSpec);
OSErr AFSpRstFLock(FSSpecPtr spec, FSSpecPtr newSpec);
OSErr FSpSetFXInfo(FSSpecPtr spec, FXInfo *fxInfo);
bool IsAlias(FSSpecPtr spec, FSSpecPtr newSpec);
bool IsAliasNoMount(FSSpecPtr spec, FSSpecPtr newSpec);
OSErr ResolveAliasOrElse(FSSpecPtr spec, FSSpecPtr newSpec, bool *wasIt);
OSErr FSMakeFID(FSSpecPtr spec, long *fid);
OSErr FSResolveFID(short vRef, long fid, FSSpecPtr spec);
OSErr DTRef(short vRef, short *dtRef);
OSErr DTGetAppl(short vRef, short dtRef, OSType creator, FSSpecPtr appSpec);
short DTFindAppl(OSType creator);
OSErr DTSetComment(FSSpecPtr spec, PStr comment);
OSErr MorphDesktop(short vRef, FSSpecPtr where);
OSErr Blat(FSSpecPtr spec, Handle text, bool append);
OSErr BlatPtr(FSSpecPtr spec, Ptr text, long size, bool append);
#define SameVRef(vr1, vr2) (vr1 == vr2)
bool SameSpec(FSSpecPtr sp1, FSSpecPtr sp2);
short RealVRef(short wdRef);
long SpecDirId(FSSpecPtr spec);
bool IsRoot(FSSpecPtr spec);
OSErr CanWrite(FSSpecPtr spec, bool *can);
OSErr MakeAFinderAlias(FSSpecPtr originalSpec, FSSpecPtr aliasSpec);
OSErr SpecMove(FSSpecPtr moveMe, FSSpecPtr moveTo);
OSErr SpecMoveAndRename(FSSpecPtr moveMe, FSSpecPtr moveTo);
OSErr GetTrashSpec(short vRef, FSSpecPtr spec);
OSErr ResolveAliasNoMount(FSSpecPtr alias, FSSpecPtr orig, bool *wasAlias);
OSErr WipeSpec(FSSpecPtr spec);
OSErr WipeDiskArea(short refN, long offset, long len);
OSErr NewTempExtSpec(short vRef, PStr name, short extId, FSSpecPtr spec);
OSErr ExchangeFiles(FSSpecPtr tmpSpec, FSSpecPtr spec);
OSErr FSpTouch(FSSpecPtr spec);
OSErr ExtractCreatorFromBndl(FSSpecPtr spec, OSType *creator);
OSErr CreatorToName(OSType creator, PStr appName);
OSErr NewTempSpec(short vRef, long dirId, PStr name, FSSpecPtr spec);
OSErr FSpExists(FSSpecPtr spec);
OSErr AddUniqueExt(FSSpecPtr spec, short extId);
OSErr VolumeMargin(short vRef, long spaceNeeded);
bool DiskSpunUp(void);
short SFPutNew(FSSpecPtr spec);
OSErr FindTemporaryFolder(short vRef, long dirId, long *tempDirId,
                          short *tempVRef);
bool IsPDFFile(FSSpecPtr spec, OSType fileType);
#ifdef DEBUG
OSErr MyFSClose(short refN);
#else
#define MyFSClose FSClose
#endif

#ifdef DEBUG
void MyCloseResFile(short refN);
#define CloseResFile MyCloseResFile
#endif

#ifdef DEBUG
#define FSpDelete MyFSpDelete
OSErr MyFSpDelete(FSSpecPtr);
#endif

#define kStuffFolderBit 0x1
OSErr FindMyFile(FSSpecPtr spec, long whereToLook, short fileName);

void MakeUniqueUntitledSpec(short vRefNum, long dirID, short strResID,
                            FSSpec *spec);
OSErr MisplaceItem(FSSpec *spec);
OSErr FSpGetLongName(FSSpec *spec, TextEncoding destEncoding, Str255 longName);
OSErr FSpGetLongNameUnicode(FSSpec *spec, HFSUniStr255 *longName);

OSErr FSpSetLongName(FSSpec *spec, TextEncoding destEncoding,
                     ConstStr255Param longName, FSSpec *newName);
OSErr FSpSetLongNameUnicode(FSSpec *spec, ConstHFSUniStr255Param longName,
                            FSSpec *newName);

OSErr MakeUniqueLongFileName(short vRefNum, long dirID, StringPtr name,
                             TextEncoding srcEncoding, short maxLen);

/* Refactored to portable paths */
#undef FSpOpenDF
#define FSpOpenDF EudoraFSpOpenDF
#undef FSpDirCreate
#define FSpDirCreate FSpDirCreate
#undef FSpDelete
#define FSpDelete EudoraFSpDelete

bool FSpIsLocked(FSSpecPtr spec);
bool SpecEndsWithExtensionR(FSSpecPtr spec, short resID);

#define kTrashFolderType 'trsh'
#define kTemporaryFolderType 'temp'
#define kCreateFolder true
#define permErr -54
#define afpAccessDenied -5000
#define fidExists -1301
#define afpIDExists -5019
#define gestaltSystemVersion 'sysv'

/* Attachment folder functions */
OSErr GetAttFolderSpec(FSSpecPtr spec);
void GetCurrentAttFolderSpec(FSSpecPtr spec);
bool TypeIsOnListWhereAndIndex(long type, short list, void *ptr, short *index);

/* `SettingsRefN` may be defined as a macro mapping to thread-local storage
 * (see `Include/threading.h`). Only declare the global symbol when the
 * macro is not present to avoid invalid macro expansion in declarations.
 */
#ifndef SettingsRefN
extern short SettingsRefN;
#endif

#endif
