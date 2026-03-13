#ifndef MAILBOX_H
#define MAILBOX_H

#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef OSERR_DEFINED
#define OSERR_DEFINED
typedef int OSErr;
typedef int OSStatus;
#endif

typedef struct mstruct *MessHandle; /* message.h */
typedef enum {
  /*    1 */ UNREAD = 1,
  /*    2 */ READ,
  /*    3 */ REPLIED,
  /*    4 */ REDIST,
  /*    5 */ UNSENDABLE,
  /*    6 */ SENDABLE,
  /*    7 */ QUEUED,
  /*    8 */ FORWARDED,
  /*    9 */ SENT,
  /*   10 */ UNSENT,
  /*   11 */ TIMED,
  /*   12 */ BUSY_SENDING,
  /*   13 */ MESG_ERR,
  /*   14 */ REBUILT,
  STATE_ENUM_LIMIT
} StateEnum;

typedef enum {
  /*    0 */ OUT_NEW_MSG,
  /*    1 */ OUT_FORWARD,
  /*    2 */ OUT_REPLY,
  /*    3 */ OUT_REDIRECT,
  OUT_TYPE_ENUM_LIMIT
} OutTypeEnum;

#ifndef SIGNED_BYTE_DEFINED
#define SIGNED_BYTE_DEFINED
typedef int8_t SignedByte;
typedef unsigned char Byte;
#endif

#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED
typedef void **Handle;
typedef unsigned char **UHandle;
typedef long Size;
typedef char *Ptr;
typedef unsigned char *PStr;
typedef unsigned char *UPtr;
#endif
typedef Handle AliasHandle;

#ifndef PSTR_DEFINED
#define PSTR_DEFINED
typedef char *StringPtr;
typedef const char *ConstPStr;
typedef const char *ConstStr255Param;
typedef GMenuModel *MenuHandle;
typedef short Style;
enum {
  fontNormal = 0,
  fontBold = 1 << 0,
  fontItalic = 1 << 1,
  fontUnderline = 1 << 2
};
#define UnreadStyle fontItalic
#endif

#ifndef ACCUMULATOR_DEFINED
#define ACCUMULATOR_DEFINED
typedef struct Accumulator {
  unsigned char **data;  /* Handle — pointer-to-pointer to byte buffer */
  long size;
  long offset;
  int err;
} Accumulator, *AccuPtr, **AccuHandle;
#endif

#ifndef TEXT_ENCODING_DEFINED
#define TEXT_ENCODING_DEFINED
typedef uint32_t TextEncoding;
#endif

#ifndef OSTYPE_DEFINED
#define OSTYPE_DEFINED
typedef uint32_t OSType;
typedef uint32_t ResType;
typedef uint32_t DescType;
typedef uint32_t AEKeyword;
#endif

#ifndef SCRIPT_DEFINED
#define SCRIPT_DEFINED
typedef short ScriptCode;
#endif

/* Priority conversion: displayed priority 0-5 → internal 0-200 */
#define Display2Prior(p) ((p) * 40)

#define FLAG_ADDRERR (1 << 28)    /* addressing error in outgoing message */
#define FLAG_UNFILTERED (1 << 23) /* message hasn't been filtered yet */
#define FLAG_HUE1 (1 << 14)
#define FLAG_HUE2 (1 << 15)
#define FLAG_HUE3 (1 << 16)
#define FLAG_HUE4 (1 << 17)
#define FLAG_UTF8 (1 << 31)     /* summary is UTF-8 */
#define OPT_IMAP_SENT (1 << 26) /* This is a sent IMAP message */
#define OPT_OPEN         (1 << 0)  /* Open after transfer */
#define OPT_AUTO_OPENED  (1 << 14) /* Was auto-opened */
#define OPT_BULK      (1 << 10) /* Bulk/list mail */
#define FLAG_HAS_ATT  (1 << 8)  /* Message has attachments */
#define FLAG_SKIPWARN  (1 << 9)  /* Skip delete warning */
typedef void **FSSpecHandle;
typedef void *ControlHandle;
typedef struct MyWindow *MyWindowPtr;
typedef void *WindowPtr;
typedef struct TOCType TOCType, *TOCPtr;

#ifndef RECT_DEFINED
#define RECT_DEFINED
typedef struct Rect {
  short top, left, bottom, right;
} Rect;
#endif

#ifndef POINT_DEFINED
#define POINT_DEFINED
typedef struct Point {
  short v, h;
} Point;
#endif
#ifndef STR255_DEFINED
#define STR255_DEFINED
typedef unsigned char Str255[256];
#endif
#ifndef STR31_DEFINED
#define STR31_DEFINED
typedef char Str31[32];
#endif
#ifndef STR63_DEFINED
#define STR63_DEFINED
typedef char Str63[64];
#endif
#ifndef STR15_DEFINED
#define STR15_DEFINED
typedef char Str15[16];
#endif

/* FSSpec: File specification structure */
#ifndef FSSPEC_DEFINED
#define FSSPEC_DEFINED
typedef struct FSSpec {
  int vRefNum;
  long parID;
  char name[256];
  char path[1024];
} FSSpec, *FSSpecPtr;
#endif

/* VDId: Volume/Directory ID pair — used for folder references */
typedef struct { short vRef; long dirId; } VDId, *VDIdPtr, **VDIdHandle;

/* CSpec: Counted file specification for tracking file references */
typedef struct CountedSpecStruct {
  FSSpec spec;
  short count;
} CSpec, *CSpecPtr, **CSpecHandle;

typedef void *ModalFilterYDUPP;
typedef void *DlgHookYDUPP;
typedef void *DlgHookYDProcPtr;
#ifndef NAVIGATION_H
typedef void *FileFilterProcPtr;
#endif
typedef void *StandardFileReplyPtr;
typedef void *DialogPtr;
#ifndef NAVIGATION_H
typedef uint32_t SFTypeList[4];
#endif

typedef struct BitMap {
  char *baseAddr;
  short rowBytes;
  struct {
    short top, left, bottom, right;
  } bounds;
} BitMap;

typedef struct DialogTemplate {
  struct {
    short top, left, bottom, right;
  } boundsRect;
} DialogTemplate, *DialogTemplatePtr, **DialogTemplateHandle;

typedef DialogTemplateHandle DialogTHndl;

#ifndef NAVIGATION_H
/* StandardFileReply: only define if Navigation.h hasn't provided it */
typedef struct StandardFileReply {
  bool sfGood;
  bool sfReplacing;
  uint32_t sfType;
  FSSpec sfFile;
  uint32_t sfScript;
  int16_t sfFlags;
  bool sfIsFolder;
  bool sfIsVolume;
} StandardFileReply;
#endif

typedef struct HFSUniStr255 {
  uint16_t length;
  uint16_t unicode[255];
} HFSUniStr255;

typedef const HFSUniStr255 *ConstHFSUniStr255Param;

#ifndef RGBCOLOR_DEFINED
#define RGBCOLOR_DEFINED
typedef struct RGBColor {
  unsigned short red, green, blue;
} RGBColor;
#endif

/* Mac-style error codes */
#define noErr 0
#define fnfErr (-43)
#define dskFulErr (-34)
#define memFullErr (-108)
#define nsvErr (-35)
#define ioErr (-36)
#define bdNamErr (-37)
#define resNotFound (-192)
#define resFNotFound (-193)
#define memReadOnlyErr (-113)
#define memLockedErr (-117)
#define dupFNErr (-48)
#define opWrErr (-49)
#define paramErr (-50)
#define rfNumErr (-51)
#define eofErr (-39)
#define noMacDskErr (-57)
#define mapChanged 0x01
#define diffVolErr (-123)
#define errAENotModifiable                                                     \
  (-1704) /* can't modify (e.g. delete default personality) */
#define errAENoSuchObject (-1728) /* object not found */
#define fsRdPerm 0x01
#define fsWrPerm 0x02
#define fsRdWrPerm 0x03

/* Finder-related constants */
#define fInvisible 0x4000
#define kIsAlias 0x8000
#define fHasBundle 0x2000
#define fOnDesk 0x0001
#define fInited 0x0080
#define kHasBeenInited 0x0100

/* Pervasive Mac Constants */
/* Use standard true/false from stdbool.h */
#define bulletChar '*'
#define ioDirMask 0x10

/* Memory operations */
#define BlockMove(s, d, l) memmove(d, s, l)

#define fsAtMark 0
#define fsFromStart 1
#define fsFromLEOF 2
#define fsFromMark 3

#ifndef mDownMask
#define mDownMask 0x0001
#endif
#define ktMacUSHidden 0

#define kARMSearch 0x01
#define kARMSearchRelFirst 0x02
#define kARMNoUI 0x04
#define kARMMountVol 0x00010000
#define kARMMultVols 0x00020000
#define gestaltAliasMgrAttr 'ali '
#define kioFlAttribDirMask 0x10
#define notAFileErr (-1302)
#define kDesktopFolderType 'dktp'
#define Cr '\r'
#define AppResFile 0
/* MISPLACED_FOLDER defined in StringDefs.h */
/* #define MISPLACED_FOLDER (-1) */
#define kTextEncodingUnknown 0xffff
#define kTextEncodingMacRoman 0
#define kUnicodeUseFallbacksMask 0
#define kUnicodeLooseMappingsMask 0
#define kTECUsedFallbacksStatus 0
#define kFSCatInfoNone 0
#define kOnSystemDisk 0
#define kSystemFolderType 'syst'
#define OPTIMAL_BUFFER (32 * 1024)

typedef void *UnicodeToTextInfo;
typedef uint16_t UniChar;
typedef struct FSRef {
  unsigned char hidden[80];
} FSRef;

/* Alias types */
#define kExportedFolderAliasType 'exfo'
#define kContainerServerAliasType 'srvr'
#define kContainerFloppyAliasType 'flpy'
#define kContainerFolderAliasType 'fldr'
#define kContainerHardDiskAliasType 'hdsk'
#define kMountedFolderAliasType 'mtfo'
#define kSharedFolderAliasType 'shfo'

/* Resource and Alert IDs */
#define WriteZero(ptr, len) memset(ptr, 0, len)
#define PSCopy PCopy
#define Zero(v) memset(&(v), 0, sizeof(v))

/* FS Structures */
typedef struct {
  char name[64];
  uint32_t fdType;
  uint32_t fdCreator;
  uint16_t fdFlags;
  struct {
    short v, h;
  } fdLocation;
  short fdFldr;
} FInfo;

typedef struct {
  short fdIconID;
  short fdUnused[3];
  short fdComment;
  long fdPutAway;
} FXInfo;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  long ioDirID;
  short ioFDirIndex;
  int8_t ioFlAttrib;
  short ioFlStBlk;
  FInfo ioFlFndrInfo;
  long ioFlCrDat;
  long ioFlMdDat;
  long ioFlBkDat;
  long ioFlLgLen;
  long ioFlPyLen;
  long ioFlRLgLen;
  long ioFlRPyLen;
  int8_t ioACUser;
  FXInfo ioFlXFndrInfo;
} HFileInfo;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  long ioDirID;
  unsigned char *ioNewName;
  long ioNewDirID;
  long ioCopyName;
  short ioSourceVRefNum;
  long ioSourceDirID;
} HFileParam;

typedef struct {
  HFileInfo hFileInfo;
} CopyParam;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  short ioRefNum;
  short ioFCBIndx;
  long ioFCBFlNm;
  uint8_t ioFCBFlags;
  long ioFCBStBlk;
  long ioFCBEOF;
  long ioFCBPLen;
  long ioFCBRLen;
  long ioFCBParID;
#define ioFCBVRefNum ioVRefNum
} FCBPBRec, *FCBPBPtr;

typedef struct {
  long elSize;
  short elCount;
} Stack, *StackPtr, **StackHandle;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  short ioFCBRefNum; // Alias for ioRefNum if needed
  short ioFCBIndx;
} FCBInfoPBRec, *FCBInfoPBPtr;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  long ioDirID;
  unsigned char *ioNewName;
  long ioNewDirID;
} CMovePBRec, *CMovePBPtr;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  long ioDestDirID;
  unsigned char *ioDestNamePtr;
  long ioSrcDirID;
  unsigned char *ioSrcNamePtr;
  long ioFileID;
} FIDParam;

typedef struct {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  long ioDrDirID;
  short ioFDirIndex;
  long ioDrParID;
  long ioDrNmFls;
} DirInfo;

typedef struct {
  short ioDTRefNum;
  short ioIndex;
  uint32_t ioFileCreator;
  unsigned char *ioNamePtr;
  int ioResult;
  void *ioCompletion;
  short ioVRefNum;
  char *ioDTBuffer;
  long ioDTReqCount;
  long ioDirID;
  long ioAPPLParID;
} DTPBRec;

#ifndef NAVIGATION_H
/* CInfoPBRec: only define if Navigation.h hasn't provided it */
typedef union {
  HFileInfo hFileInfo;
  DirInfo dirInfo;
} CInfoPBRec, *CInfoPBPtr;
#endif

typedef struct VolumeParam {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  short ioVolIndex;
  long ioVFrBlk;
  long ioVAlBlkSiz;
} VolumeParam;

typedef struct IOParam {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  short ioPermssn;
  short ioRefNum;
  void *ioMisc;
  long ioReqCount;
  long ioActCount;
  short ioPosMode;
  long ioPosOffset;
  short ioVersNum;
  void *ioBuffer;
  int ioResult;
} IOParam;

typedef struct FileParam {
  void *ioCompletion;
  unsigned char *ioNamePtr;
  short ioVRefNum;
  long ioDirID;
} FileParam;

typedef union ParamBlockRec {
  VolumeParam volumeParam;
  IOParam ioParam;
  FileParam fileParam;
  HFileInfo hFileInfo;
  FIDParam fidParam;
  CopyParam copyParam;
} ParamBlockRec, *ParmBlkPtr, HParamBlockRec, *HParmBlkPtr;

/* Function Declarations (Implemented in fileutil.c or elsewhere) */

/* Pascal string utilities - moved to StringUtil.h / fileutil.h / modernized */
void PLCat(char *dst, long n);
short FSpOpenResFile(FSSpecPtr spec, int8_t permission);
void AddResource(Handle h, ResType type, short id, ConstStr255Param name);
short ResError(void);
short FlushVol(unsigned char *name, short vRefNum);

/* File utilities - more in fileutil.h */
int FSpRename(FSSpecPtr spec, const char *newName);
int UniqueSpec(FSSpecPtr spec, short max);
uint32_t LocalDateTime(void);
int utl_RFSanity(const char *spec, bool *sane);

typedef struct GetVolParmsInfoBuffer {
  long vMVersion;
  long vMAttrib;
} GetVolParmsInfoBuffer;

/* Mac FS API */
short PBHGetVInfoSync(HParmBlkPtr pb);
short PBGetCatInfoSync(void *pb);
short PBHOpenSync(HParmBlkPtr pb);
short PBHOpenRFSync(HParmBlkPtr pb);
short PBAllocateSync(IOParam *pb);
short PBFlushFileSync(ParamBlockRec *pb);
short PBGetCatInfo(CInfoPBPtr pb, bool async);
short PBHGetVInfo(HParmBlkPtr pb, bool async);
short FindFolder(short vRef, uint32_t type, bool create, int *foundVRef,
                 long *foundDirID);
short FSMakeFSSpec(short vRef, long dirID, const char *name, FSSpecPtr spec);
short FSpCreateResFile(FSSpecPtr spec, uint32_t creator, uint32_t type,
                       uint32_t script);
int FSpCreate(FSSpecPtr spec, uint32_t creator, uint32_t fileType,
              uint32_t script);
short FSpGetFInfo(FSSpecPtr spec, FInfo *info);
short FSpSetFInfo(FSSpecPtr spec, FInfo *info);
short HGetCatInfo(short vRefNum, long dirID, const char *name, CInfoPBPtr pb);
short HSetCatInfo(short vRefNum, long dirID, const char *name, CInfoPBPtr pb);
short HMove(short vRef, long dirId, const char *name, long destDirId,
            const char *newName);
OSErr AHGetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
short AHSetFileInfo(short vRef, long dirId, const char *name, CInfoPBRec *hfi);
int FSpDelete(const char *path);
int ResolveAliasFile(FSSpecPtr spec, bool resolveAliasChains,
                     bool *targetIsFolder, bool *wasAliased);
int PBCatMoveSync(CMovePBPtr pb);
int PBHGetFInfoSync(HParmBlkPtr pb);
int PBHSetFInfoSync(HParmBlkPtr pb);
int PBExchangeFilesSync(HParmBlkPtr pb);
int FSpExchangeFiles(FSSpecPtr source, FSSpecPtr dest);
int PBSetCatInfoSync(CInfoPBPtr pb);

/* Resource Manager */
void **GetIndResource(uint32_t type, short index);
/* Legacy-style Resource API used widely across the codebase. Implemented in
 * src/fileutil.c; may consult the GTK `resource_manager` bundle when running
 * the GTK build to provide images/strings from the embedded GResource.
 */
void *GetResource(uint32_t type, short id);

/* System Services */
int Gestalt(uint32_t selector, long *response);
int ResolveAlias(FSSpecPtr fromFile, void **alias, FSSpecPtr target,
                 bool *wasChanged);
int FileSystemError(short errorId, const char *name, int err);
void DieWithError(short errorId, int err);
short MatchAlias(FSSpecPtr spec, long flags, ...);
uint32_t TickCount(void);
bool InAThread(void);
void CyclePendulum(void);
void MyYieldToAnyThread(void);
void MiniEventsLo(short mask, bool background);
int GetNumBackgroundThreads(void);
void MiniEvents(void);
short GetMBarHeight(void);
long GetRLong(int index);
bool MommyMommy(short id, void *p);
bool UseNavServices(void);
int SFPutOpenNav(FSSpecPtr spec, uint32_t creator, uint32_t type, short *refN,
                 short ditlID, uint32_t *script, FSSpecPtr defaultSpec,
                 const char *windowTitle, const char *message);
void WhackFinder(FSSpecPtr spec);
int SniffAndConvertHandleToRoman(void ***h);
uint32_t DefaultCreator(void);
short GetResFileAttrs(short refNum);
void UpdateResFile(short refNum);
void TransLitRes(UPtr string, long len, short resId);
int FSpGetHFileInfo(FSSpecPtr spec, CInfoPBPtr hfi);
int AFSpOpenDF(FSSpecPtr spec, FSSpecPtr newSpec, int8_t permission,
               short *refNum);
int MyFSClose(short refN);
short PBGetFCBInfo(FCBInfoPBPtr pb, bool async);
short PBHRenameSync(HParmBlkPtr pb);
short FSRead(short refNum, long *count, void *buffer);
short FSWrite(short refNum, long *count, const void *buffer);
void *FSClose(short refNum);
void *NuHTempBetter(long size);
short PBWriteAsync(IOParam *pb);
int FSpSetFLock(FSSpecPtr spec);
int FSpRstFLock(FSSpecPtr spec);
short FSWriteP(short refN, unsigned char *pString);
short PBCreateFileIDRefSync(HParmBlkPtr pb);
short PBResolveFileIDRefSync(HParmBlkPtr pb);
unsigned char *GetRString(unsigned char *name, short id);
short AFSHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
short ARFHOpen(const char *name, short vRefN, long dirId, short *refN,
               short perm);
short SFPutOpen(FSSpecPtr spec, long creator, long type, short *refN,
                void *filter, void *hook, short id, FSSpecPtr defaultSpec,
                const char *windowTitle, const char *message);
bool StringSame(const char *s1, const char *s2);

/* UI and Progress */
/* Conflict resolution: Use definitions from progress.h instead */
/* void ByteProgress(short id, long count, long total); */
/* void Progress(short id, short percent, void *p1, void *p2, void *p3); */
int WarnUser(short stringId, int err);
void progress_DieWithError(short stringId, int err);
void ThirdCenterRectIn(void *r, void *in);
void GetQDGlobalsScreenBits(void *bits);
void **NewHandle(size_t size);
void DisposeHandle(void **h);
int PtrToHand(const void *srcPtr, void ***dstHndl, size_t size);
size_t InlineGetHandleSize(void **h);
void HLock(void **h);
void HUnlock(void **h);
void *ZeroHandle(void *hand);
void **NuHandle(size_t size);
void **NuHandleClear(size_t size);
void *NuHTempOK(long size);
void *NuHTempBetter(long size);
void *NuPtr(size_t size);
void **PtrPlusHand(const void *ptr, void **hand, long size);
void BlockMoveData(const void *src, void *dest, size_t size);
bool HaveOSX(void);
Handle NewIOBHandle(long min, long max);

typedef struct {
  short vRef;
  long dirId;
} RootSpec;

/* Globals */
extern RootSpec Root;
extern long YieldTicks;
extern int inProgress;
extern int cacheFault;
extern int userCancelled;
bool AutoCheckOK(void);

extern bool bHasFileIDs;

/* Window management functions */
void *MyFrontWindow(void);
MyWindowPtr GetWindowMyWindowPtr(void *winWP);
WindowPtr GetMyWindowWindowPtr(MyWindowPtr win);
short GetWindowKind(void *winWP);
void SetWindowKind(void *winWP, short kind);
void SetWindowMyWindowPtr(void *winWP, MyWindowPtr win);
void *GetMyWindowPrivateData(MyWindowPtr win);
void SetMyWindowPrivateData(MyWindowPtr win, void *data);

/* Window management (mywindow.c) */
MyWindowPtr GetNewMyWindow(short resId, void *wStorage, MyWindowPtr win,
                           void *behind, bool hBar, bool vBar,
                           short windowKind);
bool ShowMyWindow(void *winWP);
void ShowMyWindowBehind(void *winWP, void *behindWP);
void MySelectWindow(void *winWP);
void UserSelectWindow(void *winWP);
void UpdateMyWindow(void *winWP);
void InvalContent(MyWindowPtr win);
void MyDisposeWindow(void *winWP);
void ZeroWinFuncs(MyWindowPtr win);
bool IsMyWindow(void *winWP);
void SetTopMargin(MyWindowPtr win, short h);
void SetBotMargin(MyWindowPtr win, short h);
void MyWindowDidResize(MyWindowPtr win, void *oldContR);
void SetWTitle(void *winWP, unsigned char *title);
void *GetWindowList(void);
void *GetNextWindow(void *win);
void ReZoomMyWindow(void *winWP);
void SendBehind(void *winWP, void *behindWP);
bool CloseMyWindow(void *winWP);

/* Mailbox open/close */
int OpenMailbox(FSSpecPtr spec, bool showIt, TOCType * toc);
void InitMailboxWin(MyWindowPtr win, TOCType * toc, bool showIt);
GtkWidget *CreateMailboxPanel(TOCType *toc);
void OpenMBWin(void);

/* Mailbox function prototypes */
short FirstMsgSelected(TOCType * tocH);
unsigned char *GetMailboxName(TOCType * tocH, short sum, unsigned char *name);
int BoxFOpenLo(TOCType * tocH, short sumNum);
int BoxFOpen(TOCType * tocH);
void BoxFClose(TOCType * tocH, bool flush);
int AddMesgError(TOCType * tocH, short sum, unsigned char *errorStr,
                 int errorCode);
void NoteFreeSpace(TOCType * tocH);
short CountSelectedMessages(TOCType * tocH);
OSErr UpdateIMAPMailbox(TOCType * tocH);
void UsingWindow(GtkWidget *win);
void NotUsingWindow(GtkWidget *win);
TOCType * FindTOC(FSSpecPtr spec);

/* IsWindowVisible: GTK4 portable check — true if widget is non-null and visible
 */
static inline bool IsWindowVisible(void *win) {
  return win != NULL && gtk_widget_is_visible(GTK_WIDGET(win));
}

/* Search window — declared in searchwin.h */
bool IsSearchWindow(void *win);
void GetSearchTOC(MyWindowPtr win, TOCType * *tocH);

/* Mailbox/message utilities */
short FindSumBySerialNum(TOCType * tocH, long serialNum);
int GetMailbox(FSSpecPtr spec, bool showIt);
void DeleteMessageLo(TOCType * tocH, int sumNum, bool nuke);
OSErr HandPlusHand(Handle h1, Handle h2);
short FindDirLevel(short vRefNum, long dirID);
char *MailboxMenuFile(short mid, short item, char *name);
long CountFlaggedMessages(TOCType * tocH);
short GetSumColor(TOCType * tocH, short sumNum);
#ifndef SumColor
#define SumColor(sum) (((sum)->flags >> 14) & 0xf)
#endif
void SetSumColor(TOCType * tocH, short sumNum, short color);
int DeleteMesgError(TOCType * tocH, short sum);
void FixSpecUnread(FSSpecPtr spec, bool unread);
bool SaveMessageSum(void *sum, TOCType * *tocH);

#endif /* MAILBOX_H */
