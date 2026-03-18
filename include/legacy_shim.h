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
#include <time.h>

/* Portable legacy typedefs to support non-Carbon builds. These are kept
   minimal and guarded so they won't conflict with system headers on
   platforms where the original Mac types exist. */
#include <glib.h>

/* Legacy Mac types (Point, Rect, Handle, FSSpec, etc.) are defined in
   mailbox.h. legacy_shim.h should only contain minimal stubs for types not
   provided by the project's own compatibility headers. */

#ifndef bool
#define bool bool
#endif

#ifndef GraphicsImportComponent
typedef void *GraphicsImportComponent;
#endif

#ifndef Movie
typedef void *Movie;
#endif

#ifndef MovieController
typedef void *MovieController;
#endif

#ifndef PMPrintContext
typedef void *PMPrintContext;
#endif

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
#endif
#ifndef ControlUserPaneBackgroundUPP
typedef void *ControlUserPaneBackgroundUPP;
#endif
/* legacy_shim intentionally omits dialog/Navigation UPP typedefs; those
   are provided by Navigation.h shim for non-Apple builds to avoid
   conflicting with system Carbon definitions on Apple platforms. */

static inline void *NewMenu(short id, const char *title) { return NULL; }
static inline void *IsMessWindow(void *win) { return NULL; }
static inline void GetWTitle(void *win, char *title) {
  if (title)
    *title = 0;
}
void SetWTitle(void *winWP, const char *title);
static inline void InsertMenu(void *mh, short item) {}

static inline void DeleteMenu(short id) {}
static inline void DisposeMenu(void *mh) {}
/* Mac resource manager stubs — resources don't exist on GTK/Linux, all return
 * NULL */
void *Get1Resource(uint32_t type, short id);
void *Get1IndResource(uint32_t type, short index);
void *GetNamedResource(uint32_t type, const unsigned char *name);
static inline bool AnalDoIncoming(void) { return false; }
static inline void AnalBox(void *toc, short start, short end) {}
#ifndef OPT_FETCH_ATTACHMENTS
#define OPT_FETCH_ATTACHMENTS 0x1000
#endif
#ifndef MAX_BOX_NAME
#define MAX_BOX_NAME 32
#endif
static inline void *NuPtrClear(size_t size) { return calloc(1, size); }
static inline void AddIMAPXfUndoUIDs(void *tocH, void *toTocH, void *h,
                                     bool flag) {}

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

int PtrAndHand(const void *ptr, void **hand, long size);
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

#ifndef RGBCOLOR_DEFINED
#define RGBCOLOR_DEFINED
typedef struct RGBColor {
  unsigned short red, green, blue;
} RGBColor;
#endif

#ifndef PicHandle
typedef void *PicHandle;
#endif

#ifndef void *
typedef void *FMBHandle;
#endif

}

}

void ComputeLocalDate(void *sum, unsigned char *dateStr);

static inline unsigned char *CompCurAddr(void *win, unsigned char *addr) {
  if (addr)
    addr[0] = 0;
  return addr;
}

#define shortDate 1

static inline void DateString(long secs, int fmt, unsigned char *str, void *p) {
  if (str) {
    str[0] = 4;
    memcpy(str + 1, "Date", 4);
  }
}

/* Core memory/file APIs (MemError, GetHandleSize, etc.) are provided by
 * the project's authoritative headers (for example `include/fileutil.h`).
 * Stubbing them here causes conflicting declarations. Do not define them
 * in this shim; let the canonical headers be authoritative. */

/* Hash() is defined in message.h as:
 *   #define Hash(s) HashWithSeed(s, 1)
 *   #define HashWithSeed(s, seed) HashWithSeedLo(s, strlen(s), seed)
 * Do NOT redefine it here — use the real C-string-compatible version.

/* File IO stubs removed: the project's `include/fileutil.h` is
 * authoritative for file I/O APIs such as SetFPos, SetEOF, file_read,
 * file_write, CopyFBytes, etc. Do not provide conflicting stubs here. */

int ReallyDoAnAlert(int templ, int which);
/* #define Caution 1 */

#ifndef LOG_PROG
#define LOG_PROG 16
#endif

/* Pete functions now declared in pete_portable.h, implemented in peteglue.c.
 * Only functions NOT yet in pete_portable.h or peteglue.h stay as stubs here: */
static inline void PeteKillUndo(void *pte) {}
static inline void PetePlain(void *pte, long start, long end, long flags) {}
static inline void PetePlainPara(void *pte, long para) {}

static inline void PeteSmallParas(void *pte) {}
static inline void PeteTrimTrailingReturns(void *pte, bool b) {}

static inline bool Black(void *color) { return false; }
static inline bool EqualString(void *s1, void *s2, bool caseSens, bool diac) {
  (void)diac;
  if (caseSens)
    return strcmp((const char *)s1, (const char *)s2) == 0;
  return strcasecmp((const char *)s1, (const char *)s2) == 0;
}

static inline void ApplyDefaultStationery(void *win, bool b1, bool b2) {}

static inline void ApplyIndexStationery(void *win, int which, bool b1,
                                        bool b2) {}

static inline void *GetMyWindowCGrafPtr(void *win) { return NULL; }

static inline void SetControlValue(void *c, int v) {}
static inline void SetControlVisibility(void *c, bool v1, bool v2) {}

static inline void HiliteControl(void *c, int p) {}
static inline bool PositionPrefsTitle(bool save, void *win) { return true; }
#ifndef PROGRESS
#endif

static inline void CheckBox(void *win, bool checked) {}
static inline void *MyFrontNonFloatingWindow(void) { return 0; }
#define FrontWindow_ MyFrontNonFloatingWindow

typedef struct {
  unsigned char errorStr[256];
  uint32_t uidHash;
  short errorCode;
} MesgErrorType;
typedef MesgErrorType *mesgErrorPtr;
typedef MesgErrorType *mesgErrorHandle;

/* static inline int CopyToOut(void *tocH, int n, void *toTocH) { return 0; }
#define MAX_MESSAGES_PER_MAILBOX 10000
/* #define Stop 1 */

/* static inline long GetMessageLength(void *tocH, int sumNum) { return 0; }
static inline int ReadMessage(void *tocH, int sumNum, unsigned char *buffer) {
  return 0;
}
static inline void DBNoteUIDHash(unsigned long hash, unsigned long uid) {}
#define SEND_ITEM 100
#define SAVE_ITEM 101
#ifndef PREF_188
#define PREF_188 188
#endif
static inline bool GoOnline(void) { return true; }
/* NoteFreeSpace is provided by the project's TOC implementation; do not
  define a stub that conflicts with its signature. If a stub is required
  for standalone builds, provide it under a unique name. */
/* NoteFreeSpace is provided by the project's TOC implementation; do not
  provide a conflicting stub here. */
/* AddMesgError is implemented in the real mailbox code; do not stub here
  to avoid conflicting declarations. If a shim is required later, provide
  it under a different name or behind a project-specific #ifdef. */

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
typedef void *RgnHandle;
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

/* HomeResFile: returns the refnum of the file a resource lives in.
 * In the GTK port resources are in a GResource bundle or in-memory;
 * return a non-zero sentinel so callers that check "if (HomeResFile(s))"
 * proceed with the resource they already have. */

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

/* ProxifyStr: post-process a retrieved string (used for personality proxies).
 * Identity function — personalities are not proxied in the GTK port. */
#ifndef ProxifyStr
static inline unsigned char *ProxifyStr(unsigned char *s, short idx) {
  return s;
}
#endif

#ifndef Fixed
typedef long Fixed;
#endif

/* StyledLineBreak: Mac TextEdit line-break algorithm. GTK text uses Pango.
 * Return kTextUsedWholeString to indicate we consumed the whole buffer. */
#ifndef StyledLineBreak
#include <stdint.h>
static inline StyledLineBreakCode StyledLineBreak(const char *text, long len,
                                                  long textStart, long textEnd,
                                                  long flags, long *textWidth,
                                                  long *offset) {
  if (offset)
    *offset = textEnd;
  return kLineBreakAtWord;
}
#endif

#ifndef GetScriptVariable
static inline long GetScriptVariable(short script, short selector) { return 0; }
#endif

#ifndef NewFaceMeasure
static inline void * NewFaceMeasure(void) { return NULL; }
static inline void FaceMeasureBegin(void * fmb) {}
static inline void DisposeFaceMeasure(void * fmb) {}
static inline void FaceMeasureReport(void * fmb, long *timePos, void *unk2,
                                     void *unk3, void *unk4) {
  if (timePos)
    *timePos = 0;
}
static inline void FaceMeasureReset(void * fmb) {}
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

void NumToString(long n, char *s);

/* GTK port: Accu string wrappers deleted because they conflict with the
 * actual OS IMAP os_unix.h */

#ifndef CanScoreJunk
static inline bool CanScoreJunk(void) { return false; }
#endif

enum { longDate = 1 };
enum { tokDecPoint = 1, tokThousands = 2 };

extern short InBG;
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

typedef struct {
} Itl1ExtRec;

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

/* RANDOM_FAILURE: debug macro to test random allocation failures.
 * Disabled in production builds. */
#ifndef RANDOM_FAILURE
#define RANDOM_FAILURE /* no-op */
#endif

/* TempNewHandle: allocate from temp zone. Falls back to malloc.
 * Use void** to avoid dependency on void *typedef ordering. */
#ifndef TempNewHandle
static inline void *TempNewHandle(long size, int *err) {
  void **h = (void **)malloc(sizeof(void *));
  if (h) {
    *h = malloc((size_t)size);
    if (!*h) {
      free(h);
      h = NULL;
    }
  }
  if (err)
    *err = h ? 0 : -108; /* ENOMEM */
  return h;
}
#endif

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
#endif
#ifndef autoKeyMask
#endif

#ifndef updateMask
#define updateMask 0x0040
#define activMask 0x0100
#define osMask 0x0C00
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
