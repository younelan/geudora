/* Minimal legacy shims for QuickTime, Printing, and other Carbon-era types
   used by the Eudora codebase. These are intentionally small opaque
   placeholders to allow a GTK-first build; behavior must be implemented
   later using platform-appropriate APIs if runtime functionality is needed. */

#ifndef LEGACY_SHIM_H
#define LEGACY_SHIM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Portable legacy typedefs to support non-Carbon builds. These are kept
   minimal and guarded so they won't conflict with system headers on
   platforms where the original Mac types exist. */
#include <glib.h>

/* Legacy Mac types (Point, Rect, Handle, FSSpec, etc.) are defined in
   mailbox.h. legacy_shim.h should only contain minimal stubs for types not
   provided by the project's own compatibility headers. */

#ifndef Boolean
#define Boolean bool
#endif

/* Handle typedef — matches mailbox.h definition; guarded so no conflict */
#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED
typedef void **Handle;
typedef unsigned char **UHandle;
typedef char *Ptr;
typedef unsigned char *PStr;
typedef unsigned char *UPtr;
#endif

/* QuickTime placeholders */
#ifndef GraphicsImportComponent
typedef void *GraphicsImportComponent;
#endif

#ifndef Movie
typedef void *Movie;
#endif

#ifndef MovieController
typedef void *MovieController;
#endif

/* Printing Manager placeholder */
#ifndef PMPrintContext
typedef void *PMPrintContext;
#endif

/* PenState placeholder used by old QuickDraw-based drawing utilities */
#ifndef PenState
typedef struct {
  int32_t dummy;
} PenState;
#endif
/* Opaque handles for Mac UI types. These will be replaced by GTK widgets in
 * ported code. */
#ifndef ControlBackgroundPtr
typedef void *ControlBackgroundPtr;
#endif
#ifndef ControlHandle
typedef void *ControlHandle;
#endif
#ifndef ControlUserPaneBackgroundUPP
typedef void *ControlUserPaneBackgroundUPP;
#endif
/* legacy_shim intentionally omits dialog/Navigation UPP typedefs; those
   are provided by Navigation.h shim for non-Apple builds to avoid
   conflicting with system Carbon definitions on Apple platforms. */

static inline void *NewMenu(short id, const char *title) { return NULL; }
static inline void *IsMessWindow(void *win) { return NULL; }
static inline void GetWTitle(void *win, unsigned char *title) {
  if (title)
    *title = 0;
}
/* SetWTitle - real impl in mywindow.c */
void SetWTitle(void *winWP, unsigned char *title);
static inline void InsertMenu(void *mh, short item) {}
static inline void SetDItemValue(void *dp, short item, short value) {}
static inline short GetDItemValue(void *dp, short item) { return 0; }
/* SetState implemented in mailbox.c */
static inline void DeleteMenu(short id) {}
static inline void DisposeMenu(void *mh) {}
static inline short CurResFile(void) { return 0; }
static inline void UseResFile(short refN) { (void)refN; }
static inline void DetachResource(void *h) { (void)h; }
/* Mac resource manager stubs — resources don't exist on GTK/Linux, all return
 * NULL */
Handle Get1Resource(uint32_t type, short id);
Handle Get1IndResource(uint32_t type, short index);
Handle GetNamedResource(uint32_t type, const unsigned char *name);
/* SaveAbomination is defined in uudecode.h - do not stub here */
static inline bool AnalDoIncoming(void) { return false; }
static inline void AnalBox(void *toc, short start, short end) {}
/* HasStubFileAttachment in imapdownload.h */
#ifndef OPT_FETCH_ATTACHMENTS
#define OPT_FETCH_ATTACHMENTS 0x1000
#endif
#ifndef MAX_BOX_NAME
#define MAX_BOX_NAME 32
#endif
/* Legacy style macros moved to mailbox.h or replaced by GTK attributes */
static inline void *NuPtrClear(size_t size) { return calloc(1, size); }
/* SetSumColor provided by message.c or other modules */
/* static inline void SetSumColor(void *tocH, int sumNum, int color) {} */
/* BoxSelectAfter provided by message.c or other modules */
/* static inline void BoxSelectAfter(void *tocH, int sumNum) {} */
/* MBResort — real implementation in boxact.c */
static inline void AddIMAPXfUndoUIDs(void *tocH, void *toTocH, void *h,
                                     bool flag) {}

#define kStatReceivedMail_SHIM 0
#define ksStatReceivedMail_SHIM 0
#ifndef POINT_DEFINED
#define POINT_DEFINED
typedef struct Point {
  short v;
  short h;
} Point;
#endif

#ifndef RECT_DEFINED
#define RECT_DEFINED
typedef struct Rect {
  short top;
  short left;
  short bottom;
  short right;
} Rect;
#endif

static inline void GetWindowStructureBounds(void *win, Rect *r) {
  if (r)
    memset(r, 0, sizeof(Rect));
}
static inline short MyGetWindowTitleWidth(void *win) { return 0; }
static inline long PopUpMenuSelect(void *mh, short top, short left,
                                   short item) {
  return 0;
}

/* PtrAndHand: append ptr[0..size-1] to a Handle; Hand must be malloc'd */
/* Declared here; implementation forward-declared so compilers find it */
int PtrAndHand(const void *ptr, void **hand, long size);
/* BoxSelectSame — real implementation in boxact.c */
/* MBOpenFolder - real impl in mbwin.c */
void MBOpenFolder(void *hStringList, bool isIMAP);

#ifndef optionKey
#define optionKey 0x0800
#endif
#ifndef shiftKey
#define shiftKey 0x0200
#endif
#ifndef cmdKey
#define cmdKey 0x0100
#endif
#ifndef controlKey
#define controlKey 0x1000
#endif
#ifndef alphaLock
#define alphaLock 0x0400
#endif

struct EventRecord {
  short what;
  unsigned long message;
  unsigned long when;
  Point where;
  short modifiers;
};
typedef struct EventRecord EventRecord;

#ifndef Str32
typedef unsigned char Str32[33];
#endif

#ifndef LongDateRec
typedef struct {
  union {
    struct {
      long lHigh;
      long lLow;
    } hl;
    long long dl;
    long long c;
  };
} LongDateCvt;

typedef struct {
  union {
    struct {
      short era;
      short year;
      short month;
      short day;
      short hour;
      short minute;
      short second;
      short dayOfWeek;
      short dayOfYear;
      short weekOfYear;
      short pm;
      short res1;
      short res2;
      short res3;
    } ld;
    short list[14];
    long long dl;
  };
} LongDateRec;
#endif

/* GTK Port: DateTimeRec and SecondsToDate removed to avoid util.h override */

#ifndef RGBCOLOR_DEFINED
#define RGBCOLOR_DEFINED
typedef struct RGBColor {
  unsigned short red, green, blue;
} RGBColor;
#endif

#ifndef PicHandle
typedef void **PicHandle;
#endif

#ifndef FMBHandle
typedef void **FMBHandle;
#endif

static inline void SetHandleSize(void **h, long size) {
  if (h && *h) {
    void *resized = realloc(*h, size);
    if (resized)
      *h = resized;
  }
}
static inline void PtoCcpy(char *cStr, const unsigned char *pStr) {
  if (!pStr || !cStr)
    return;
  long len = pStr[0];
  memmove(cStr, pStr + 1, len);
  memmove(cStr, pStr + 1, len);
  cStr[len] = '\0';
}

static inline void PCopy_SHIM(unsigned char *dst, const unsigned char *src) {
  if (dst && src) {
    long len = src[0];
    memmove(dst, src, len + 1);
  }
}

/* PCopy provided by StringUtil.h */

/* Stubs for missing legacy functions */
static inline void ComputeLocalDate(void *sum, unsigned char *dateStr) {
  if (dateStr) {
    /* valid pascal string "Date" */
    dateStr[0] = 4;
    memcpy(dateStr + 1, "Date", 4);
  }
}

static inline void TimeString(long secs, bool b, unsigned char *str, void *p) {
  if (str) {
    str[0] = 4;
    memcpy(str + 1, "Time", 4);
  }
}

static inline void ShortAddr(unsigned char *src, unsigned char *dst) {
  if (src != dst)
    PCopy_SHIM(dst, src);
}

/*
static inline unsigned char *CompCurAddr(void *win, unsigned char *addr) {
  if (addr)
    addr[0] = 0;
  return addr;
}
*/

/* peModLock — now in pete_portable.h */
#define shortDate 1

static inline void DateString(long secs, int fmt, unsigned char *str, void *p) {
  if (str) {
    str[0] = 4;
    memcpy(str + 1, "Date", 4);
  }
}

/* utl_PlugParams declared in utl.h — real implementation */
#ifndef BMD
#define BMD(s, d, l) memmove(d, s, l)
#endif
static inline void CycleBalls(void) {}
static inline void HPurge(void *h) {}
static inline void HNoPurge(void *h) {}

/* Core memory/file APIs (MemError, GetHandleSize, etc.) are provided by
 * the project's authoritative headers (for example `include/fileutil.h`).
 * Stubbing them here causes conflicting declarations. Do not define them
 * in this shim; let the canonical headers be authoritative. */

static inline uint32_t Hash(unsigned char *pStr) {
  uint32_t hash = 0;
  if (!pStr)
    return 0;
  int len = pStr[0];
  for (int i = 1; i <= len; i++)
    hash = (hash << 5) - hash + pStr[i];
  return hash;
}

/* GetMailboxName implemented in mailbox.c */

/* File IO stubs removed: the project's `include/fileutil.h` is
 * authoritative for file I/O APIs such as SetFPos, SetEOF, ARead,
 * AWrite, CopyFBytes, etc. Do not provide conflicting stubs here. */

/* ReallyDoAnAlert implemented in shame.c */
int ReallyDoAnAlert(int templ, int which);
/* #define Caution 1 */

/* Logging: real implementations are in log.c/log.h — no stubs here */
#ifndef LOG_PROG
#define LOG_PROG 16
#endif

/* PETE Stubs */
/* PETEHandle, kPETELastPara, peModLock etc. — all in pete_portable.h now */

/* Pete functions now declared in pete_portable.h, implemented in peteglue.c.
 * Only functions NOT yet in pete_portable.h or peteglue.h stay as stubs here: */
static inline void PeteKillUndo(void *pte) {}
static inline void PetePlain(void *pte, long start, long end, long flags) {}
static inline void PetePlainPara(void *pte, long para) {}
static inline int PETEInsertPara(void *glob, void *pte, long para, void *style,
                                 void *text, void *range) {
  return 0;
}
static inline void PeteSmallParas(void *pte) {}
static inline void PeteTrimTrailingReturns(void *pte, bool b) {}

/* Color/Graphics Stubs */

/* Color/Graphics Stubs */
static inline bool Black(void *color) { return false; }
// static inline void BeautifyFrom(void *s) {}

/* String Stubs */
static inline bool EqualString(void *s1, void *s2, bool caseSens, bool diac) {
  return false;
}
// static inline void *FindHeaderString(void *text, void *headerName, long
// *size,
//                                      bool bodyToo) {
//   return NULL;
// }

/* Message/Window Stubs */

/* Message/Window Stubs */
static inline void ReZoomMyWindow(void *w) {}
/* InvalContent - real impl in mywindow.c */
// static inline void ShowMessageSeparator(void *pte, bool center) {}
static inline bool CloseMyWindow(void *w) { return true; }

/* UpdateMyWindow - real impl in mywindow.c */
// static inline void EnsureMessNewline(void *messH) {}
static inline void ApplyDefaultStationery(void *win, bool b1, bool b2) {}

static inline void ApplyIndexStationery(void *win, int which, bool b1,
                                        bool b2) {}

/* Util Stubs */
/* AccuZap is provided by CrispinIMAP as IMAPAccuZap — do not stub */
static inline void PushGWorld(void) {}
static inline void PopGWorld(void) {}
static inline void SetPort(void *p) {}
static inline void *GetMyWindowCGrafPtr(void *win) { return NULL; }
static inline void InvalWindowRect(void *win, void *r) {}
static inline void SetControlValue(void *c, int v) {}
static inline void SetControlVisibility(void *c, bool v1, bool v2) {}
static inline void ShowControl(void *c) {}
static inline void HiliteControl(void *c, int p) {}
static inline bool PositionPrefsTitle(bool save, void *win) { return true; }
#ifndef PROGRESS
#endif

/* UI Stubs */
/* IsRoot provided by fileutil.h - do not stub here. */
/* In the GTK port all managed windows are app windows */
/* IsMyWindow - real impl in mywindow.c */
static inline void CheckBox(void *win, bool checked) {}
static inline void *MyFrontNonFloatingWindow(void) { return 0; }
#define FrontWindow_ MyFrontNonFloatingWindow
static inline void *GetNextWindow(void *win) { return NULL; }
/* GetMHandle is provided by gtk_menus.h as MenuHandle GetMHandle(short) */

/* File IO Stubs */

/* VolumeMargin provided by fileutil.h */
/* CountSelectedMessages is implemented in mailbox.c */
// static inline int CountSelectedMessages(void *tocH) { return 0; }

/* Types */
typedef struct {
  unsigned char errorStr[256];
  uint32_t uidHash;
  short errorCode;
} MesgErrorType;
typedef MesgErrorType *mesgErrorPtr;
typedef MesgErrorType **mesgErrorHandle;

/* Missing Legacy Functions */
/* FindTOCSpot is in buildtoc.h - don't stub it */

/* static inline int CopyToOut(void *tocH, int n, void *toTocH) { return 0; }
 */
/* AWrite, AlertStr, CopyFBytes provided by fileutil.h - no shim here. */

#define MAX_MESSAGES_PER_MAILBOX 10000
/* FLAG_SKIPWARN defined in mailbox.h */
/* #define Stop 1 */

/* Comp.c Stubs */
/* static inline long GetMessageLength(void *tocH, int sumNum) { return 0; }
 */
/*
static inline int ReadMessage(void *tocH, int sumNum, unsigned char *buffer) {
  return 0;
}
*/
static inline void DBNoteUIDHash(unsigned long hash, unsigned long uid) {}
#define SEND_ITEM 100
#define SAVE_ITEM 101
#ifndef PREF_188
#define PREF_188 188
#endif
static inline bool GoOnline(void) { return true; }
static inline int PtrPlusHand_(void *ptr, void *h, long size) { return 0; }
/* NoteFreeSpace is provided by the project's TOC implementation; do not
  define a stub that conflicts with its signature. If a stub is required
  for standalone builds, provide it under a unique name. */
/* NoteFreeSpace is provided by the project's TOC implementation; do not
  provide a conflicting stub here. */
/* UpdateIMAPMailbox is implemented in mailbox.c */
/* AddMesgError is implemented in the real mailbox code; do not stub here
  to avoid conflicting declarations. If a shim is required later, provide
  it under a different name or behind a project-specific #ifdef. */

/* ---- Mac Control Manager portability ---- */
/* GetControlOwner: return the window owning a control handle.
 * In GTK controls are GtkWidget children; return NULL as void* (callers
 * pass result to GetWindowMyWindowPtr which accepts void*). */
#ifndef GetControlOwner
static inline void *GetControlOwner(void *cntl) { return NULL; }
#endif
#ifndef GetControlBounds
static inline void GetControlBounds(void *cntl, Rect *r) {
  memset(r, 0, sizeof(Rect));
}
#endif
#ifndef GetControlTitle
static inline void GetControlTitle(void *cntl, unsigned char *title) {
  title[0] = 0;
}
#endif
#ifndef GetBestControlRect
static inline void GetBestControlRect(void *cntl, Rect *r, short *base) {
  memset(r, 0, sizeof(Rect));
  *base = 0;
}
#endif
#ifndef MoveMyCntl
static inline void MoveMyCntl(void *cntl, short x, short y, short w, short h) {}
#endif
#ifndef SizeControl
static inline void SizeControl(void *cntl, short w, short h) {}
#endif
#ifndef ControlIsUgly
static inline int ControlIsUgly(void *cntl) { return 0; }
#endif
#ifndef StringWidth
static inline short StringWidth(const unsigned char *s) {
  return s ? s[0] * 7 : 0;
}
#endif
#ifndef IsMenuItemEnabled
static inline int IsMenuItemEnabled(void *mh, short item) { return 1; }
#endif

/* ChangedResource: in the GTK port, resource handles are in-memory.
 * Marking as changed is a no-op; persistence is handled at higher levels. */
#ifndef ChangedResource
static inline void ChangedResource(void *h) {}
#endif

/* OffsetRect: shift a Rect by (dh, dv) */
#ifndef OffsetRect
static inline void OffsetRect(Rect *r, short dh, short dv) {
  r->left += dh;
  r->right += dh;
  r->top += dv;
  r->bottom += dv;
}
#endif

/* Region handle type and stubs — ShowDragRectHilite is the only user in util.c.
 * GTK drag-and-drop provides its own highlight; these are thin no-ops. */
#ifndef RGNHANDLE_DEFINED
#define RGNHANDLE_DEFINED
typedef void **RgnHandle;
#endif
#ifndef NewRgn
#include <stdlib.h>
static inline RgnHandle NewRgn(void) {
  return (RgnHandle)malloc(sizeof(void *));
}
static inline void DisposeRgn(RgnHandle rgn) { free(rgn); }
static inline void RectRgn(RgnHandle rgn, Rect *r) {}
static inline void ShowDragHilite(void *drag, RgnHandle rgn, int inside) {}
#endif

/* ---- Mac Resource Manager portability ---- */

/* HomeResFile: returns the refnum of the file a resource lives in.
 * In the GTK port resources are in a GResource bundle or in-memory;
 * return a non-zero sentinel so callers that check "if (HomeResFile(s))"
 * proceed with the resource they already have. */
#ifndef HomeResFile
static inline short HomeResFile(void *h) { return 1; }
#endif

/* GetIndString: legacy no-op stub kept for any stray callers.
 * Real string lookup goes through string_table_lookup() in GetRStringLo. */
#ifndef GetIndString
static inline void GetIndString(unsigned char *str, short strListID,
                                short index) {
  if (str)
    str[0] = 0;
}
#endif

/* GetString: get an 'STR ' resource by ID.
 * Wraps GetResource_ with the 'STR ' four-char code. */
#ifndef GetString
#define GetString(id) GetResource_('STR ', (id))
#endif

/* ProxifyStr: post-process a retrieved string (used for personality proxies).
 * Identity function — personalities are not proxied in the GTK port. */
#ifndef ProxifyStr
static inline unsigned char *ProxifyStr(unsigned char *s, short idx) {
  return s;
}
#endif

/* Fixed: 16.16 fixed-point type used by QuickDraw / Mac typography. */
#ifndef Fixed
typedef long Fixed;
#endif

/* GetPrefLong declared as a function in gtk_dialogs.h */

/* StyledLineBreak: Mac TextEdit line-break algorithm. GTK text uses Pango.
 * Return kTextUsedWholeString to indicate we consumed the whole buffer. */
#ifndef StyledLineBreak
#include <stdint.h>
typedef long StyledLineBreakCode;
#define kLineBreakInWord 0
#define kLineBreakAtWord 1
#define kLineBreakOverflow 2
static inline StyledLineBreakCode StyledLineBreak(const char *text, long len,
                                                  long textStart, long textEnd,
                                                  long flags, long *textWidth,
                                                  long *offset) {
  if (offset)
    *offset = textEnd;
  return kLineBreakAtWord;
}
#endif

/* GetScriptVariable: Mac Script Manager query. Return 0 for all queries. */
#ifndef GetScriptVariable
static inline long GetScriptVariable(short script, short selector) { return 0; }
#endif

/* Missing Mac UI and Date Translation Stubs */
#ifndef NewFaceMeasure
static inline FMBHandle NewFaceMeasure(void) { return NULL; }
static inline void FaceMeasureBegin(FMBHandle fmb) {}
static inline void DisposeFaceMeasure(FMBHandle fmb) {}
static inline void FaceMeasureReport(FMBHandle fmb, long *timePos, void *unk2,
                                     void *unk3, void *unk4) {
  if (timePos)
    *timePos = 0;
}
static inline void FaceMeasureReset(FMBHandle fmb) {}
#endif

#ifndef GetWindowPrivateData
static inline void *GetWindowPrivateData(void *w) { return NULL; }
#endif

#ifndef LongSecondsToDate
static inline void LongSecondsToDate(long long *secs, LongDateRec *d) {
  if (d)
    memset(d, 0, sizeof(LongDateRec));
}
#endif

/* NumToString is defined in stringutil.c */
void NumToString(long n, unsigned char *s);

/* GTK port: Accu string wrappers deleted because they conflict with the
 * actual OS IMAP os_unix.h */

#ifndef CanScoreJunk
static inline bool CanScoreJunk(void) { return false; }
#endif

/* R822Date is implemented in sendmail.c */
/* ZoneSecs is implemented in util.c */

/* DiskSpunUp is implemented in fileutil.c */

enum { longDate = 1 };
enum { tokDecPoint = 1, tokThousands = 2 };

extern short InBG;
/* RedisplayStats is now a real function in statwin.c */
static inline void GetDateTime(long *time) {
  if (time)
    *time = 0;
}

typedef struct {
  Rect srcRect;
  long hRes;
  long vRes;
  short version;
  short reserved1;
  short reserved2;
} OpenCPicParams;

static inline void DateToSeconds(void *d, unsigned long *secs) {
  if (secs)
    *secs = 0;
}

static inline void *GetStatWin(void) { return 0; }
static inline void SetRect(Rect *r, short left, short top, short right,
                           short bottom) {}
static inline PicHandle OpenCPicture(OpenCPicParams *p) { return 0; }
static inline void ClipRect(Rect *r) {}

static inline void ClosePicture(void) {}

typedef struct {
} Itl1ExtRec;
/* AccuAddPtr removed to avoid IMAP conflicts */

/* Dprintf: implemented in shame.c */

/* ---- Mac Event / Memory Manager portability ---- */

/* MightSwitch/AfterSwitch: cooperative threading hints. GTK uses GLib's
 * main loop; these are no-ops in the ported version. */
#ifndef MightSwitch
static inline void MightSwitch(void) {}
static inline void AfterSwitch(void) {}
#endif

/* EventAvail: peek at the event queue. GTK handles events via its main loop.
 * Return 0 (no event) as a safe default. */
#ifndef EventAvail
static inline int EventAvail(short mask, void *event) { return 0; }
#endif

/* PurgeSpace: Mac Memory Manager — reports purge space.
 * In the GTK port memory is managed by glibc/GLib. Return plentiful space. */
#ifndef PurgeSpace
static inline void PurgeSpace(long *total, long *contig) {
  if (total)
    *total = 64 * 1024 * 1024;
  if (contig)
    *contig = 64 * 1024 * 1024;
}
#endif

/* RANDOM_FAILURE: debug macro to test random allocation failures.
 * Disabled in production builds. */
#ifndef RANDOM_FAILURE
#define RANDOM_FAILURE /* no-op */
#endif

/* ---- Mac Memory Manager: zone/handle operations ---- */
/* MoveHHi: move handle to top of heap zone. GTK/glibc handles its own heap. */
#ifndef MoveHHi
static inline void MoveHHi(void *h) {}
#endif

/* TempNewHandle: allocate from temp zone. Falls back to malloc.
 * Use void** to avoid dependency on Handle typedef ordering. */
#ifndef TempNewHandle
static inline void **TempNewHandle(long size, int *err) {
  void **h = (void **)malloc(sizeof(void *));
  if (h) {
    *h = malloc((size_t)size);
    if (!*h) {
      free(h);
      h = NULL;
    }
  }
  if (err)
    *err = h ? 0 : -108; /* memFullErr */
  return h;
}
#endif

/* LastContigSpace / LastTotalSpace: declared in Globals.h. */

/* Event filter masks used in MyOSEventAvail / MiniEventsLo */
#ifndef mUpMask
#define mUpMask 0x0010
#endif
#ifndef mDownMask
#define mDownMask 0x0008
#endif
#ifndef keyDownMask
#define keyDownMask 0x0004
#endif
#ifndef keyUpMask
#define keyUpMask 0x0008
#endif
#ifndef autoKeyMask
#define autoKeyMask 0x0040
#endif

#ifndef updateMask
#define updateMask 0x0040
#define activMask 0x0100
#define osMask 0x0C00
#endif
#ifndef HGetState
static inline signed char HGetState(void *h) { return 0; }
#endif
#ifndef HSetState
static inline void HSetState(void *h, signed char s) {
  (void)h;
  (void)s;
}
#endif
#ifndef RemoveResource
static inline void RemoveResource(void *h) {}
#endif

#ifndef AddDragItemFlavor
#define flavorNotSaved 0x0001
static inline int AddDragItemFlavor(void *drag, unsigned int item,
                                    unsigned int flavor, const void *data,
                                    unsigned long dlen, int flags) {
  return 0;
}
#endif
#ifndef CGrafPtr
typedef void *CGrafPtr;
#endif

uint32_t TickCount(void);

#endif /* LEGACY_SHIM_H */
