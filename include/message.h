/* Copyright (c) 2017, Computer History Museum
   All rights reserved. */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "../gEditCtrl/geditctrl.h" /* For gTextviewCtrl / gEditCtrl */
#include "mailbox.h" /* Defines Handle, FSSpec, unsigned char *, etc. */
#include "mydefs.h"
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdint.h>
// Forward definition for GtkWidget to avoid include errors
// If GTK is included (e.g. from gtk/gtk.h or gtktypes.h), GtkWidget is already
// defined. If it's not, we need a forward declaration.
#ifndef GTK_TYPE_WIDGET
#ifndef _GTK_TYPES_H_ // Check for common GTK header guards if possible, or
                      // fallback
typedef struct _GtkWidget GtkWidget;
#endif
#endif

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Message descriptions */
#include "mailbox.h" /* Defines Handle, FSSpec, unsigned char *, etc. */

#define MAX_HEADER 64
#define WinPtr2MessH(aWindowPtr) ((MessHandle)GetWindowPrivateData(aWindowPtr))
#define Win2MessH(aMyWindowPtr)                                                \
  ((MessHandle)GetMyWindowPrivateData(aMyWindowPtr))
#define SumOf(mH) (&(mH)->tocH->sums[(mH)->sumNum])
#define BodyOf(mH) ((mH)->txes[BODY])
#define MessFlagIsSet(mH, f) (0 != (SumOf(mH)->flags & (f)))
#define SetMessFlag(mH, f)                                                     \
  do {                                                                         \
    SumOf(mH)->flags |= (f);                                                   \
    TOCSetDirty((mH)->tocH, true);                                             \
  } while (0)
#define MessOptIsSet(mH, f) (0 != (SumOf(mH)->opts & f))
#define SetMessOpt(mH, f)                                                      \
  do {                                                                         \
    SumOf(mH)->opts |= f;                                                      \
    TOCSetDirty((mH)->tocH, true);                                             \
  } while (0)
#define ClearMessOpt(mH, f)                                                    \
  do {                                                                         \
    SumOf(mH)->opts &= ~f;                                                     \
    TOCSetDirty((mH)->tocH, true);                                             \
  } while (0)
#define ClearMessFlag(mH, f)                                                   \
  do {                                                                         \
    SumOf(mH)->flags &= ~f;                                                    \
    TOCSetDirty((mH)->tocH, true);                                             \
  } while (0)
#define OldWin2Body(win) BodyOf(Win2MessH(win))
#define HeaderName(num)                                                        \
  (GetRString(scratch, HEADER_STRN + num), TrimWhite(scratch), (char *)scratch)
#define IsAddressHead(head)                                                    \
  (head == TO_HEAD || head == BCC_HEAD || head == CC_HEAD)
#define MessIsRich(mH)                                                         \
  (MessFlagIsSet((mH), FLAG_RICH) || MessOptIsSet((mH), OPT_HTML) ||           \
   UseFlowInExcerpt && MessOptIsSet((mH), OPT_FLOW))
#define TheBody (messH->bodyPTE)

/* Accumulator moved to mailbox.h */

typedef struct {
  long id;
  void *properties;
} TransInfo, *TransInfoPtr, *TransInfoHandle;

typedef struct {
  char spec[PATH_MAX];
  unsigned long cid;
  unsigned long absURL;
  unsigned long relURL;
} PartDesc, *PDPtr, *PDHandle;

/* Define MyWindow struct for gTextviewCtrl port */
typedef struct MyWindow {
  GtkWidget *pte; /* Editor Widget */
  bool (*close)(struct MyWindow *);
  GtkWidget *window; /* Main Window */
  void *privateData;
  bool (*menu)(struct MyWindow *, int, int, short);
  int (*gonnaShow)(struct MyWindow *);
  int (*position)(struct MyWindow *);
  void (*cursor)(Point);
  bool isDirty;
  bool hasSelection;

  bool (*button)(struct MyWindow *, GtkWidget *, GdkEvent *);
  bool (*app1)(struct MyWindow *, void *);
  bool (*find)(struct MyWindow *, char *);
  char *(*curAddr)(struct MyWindow *, char *);
  RGBColor backColor;                           /* Current background color */
  bool ro;                                      /* Read Only flag */
  bool saveSize;                                /* Saved size flag */
  int (*position_new)(bool, struct MyWindow *); /* Position function */
  bool dontControl;                             /* Added for comp.c */
  int label;                                    /* Added for comp.c */
  Rect contR;                                   /* Content rectangle */

  /* GTK port: Mac window manager callback fields used by Mac window files.
     These are stored as nullable function pointers; most are no-ops in GTK
     port. */
  void (*didResize)(struct MyWindow *, Rect *);
  void (*update)(struct MyWindow *);
  void (*click)(struct MyWindow *, void *);
  void (*bgClick)(struct MyWindow *, void *);
  void (*activate)(struct MyWindow *);
  bool (*key)(struct MyWindow *, void *);
  int (*drag)(struct MyWindow *, int, void *);
  void (*zoomSize)(struct MyWindow *, Rect *);
  void (*grow)(struct MyWindow *, Point *);
  void (*help)(struct MyWindow *, Point); /* takes Point, not void* */
  void (*idle)(struct MyWindow *);
  bool isActive;        /* Whether window is active */
  short topMargin;      /* Top margin of content area */
  Point minSize;        /* Minimum window size */
  void *pteList;        /* List of Pete editors in window */
  Rect sponsorAdRect;   /* Sponsor ad rectangle */
  bool sponsorAdExists; /* Whether a sponsor ad is shown */
  bool showsSponsorAd;  /* Whether this window shows ads */
} MyWindow, *MyWindowPtr;

/* GTK port: Concrete ListS struct for Mac ListHandle dereference patterns */
#ifndef LISTS_STRUCT_DEFINED
#define LISTS_STRUCT_DEFINED
struct ListS {
  Rect dataBounds; /* bounds of data cells */
  Rect visible;    /* visible cells */
  Point cellSize;  /* size of each cell */
};
/* ListHandle is typedef'd as GtkWidget* in task_ldef.h */
#ifndef LIST_HANDLE_DEFINED
#define LIST_HANDLE_DEFINED
typedef GtkWidget *ListHandle;
#endif
#endif

/* GTK port: icon plotting constants (Mac Icon Manager) */
#ifndef kCustomIconResource
#define kCustomIconResource (-16455)
#define atNone 0
#define atHorizontalCenter 4
#define ttNone 0
#define ttSelected 1
#endif

/**********************************************************************
 * structure to describe message
 **********************************************************************/
typedef struct mstruct MessType, *MessPtr;
typedef MessType *MessHandle;  /* was MessType** (Mac Handle); now direct pointer */
struct mstruct {
  TOCType * tocH;   /* the table of contents to which this message belongs */
  int sumNum;       /* the summary number of this message's summary */
  MyWindowPtr win;  /* window I'm displayed in */
  bool dirty;       /* whether or not message is dirty */
  bool forceUnread; /* don't set status to read on update */
  long weeded;      /* number of header bytes "weeded" out */
  ControlHandle sendButton;
  ControlHandle analControl;
  GtkWidget *bodyPTE;       /* internal editor widget (gTextviewCtrl) */
  GtkWidget *subPTE;        /* internal editor widget (gTextviewCtrl) */
  /* Compose header widgets — NULL for read-only messages */
  GtkWidget *headerWidgets[16]; /* indexed by TO_HEAD..PERSONA_HEAD */
  GtkWidget *headerGrid;        /* the grid containing all headers */
  TOCType * openedFromTocH; /* toc from which we were requested (links) */
  long openedFromSerialNum; // serial # of message from which we were requested
                            // (links)
  Accumulator extras;       /* extra header lines */
  MessHandle next;          /* next message in the list */
  long fieldDirty;          /* the dirty value when we entered this field */
  bool hasDelIcon;
  bool hasFetchIcon;
  Accumulator aSourceMID;
  uint32_t ezOpenSerialNum;
  TransInfoHandle hTranslators;
  int hTranslatorsCount;
  short nTransIcons;
  short sound;
  void *etlFiles;        /* buffer of attachment paths */
  size_t etlFilesSize;   /* tracked size of etlFiles buffer */
  void *hStationerySpec; /* FSSpecHandle stub */
  bool textFormatBarEnabled;
  void *persGraphic;
  bool openToEnd;
  Accumulator newsGroupAcc; // to hold newsgroup names
  bool redrawPersPopup;     // used if we try to draw the popup but fail
  bool dontActivate; // skip the activation of a particular field on showing
  long testSelStart, testSelEnd; // stash the selection for personality popup
  bool alreadyLeaving;           // flag to indicate we're in CompLeaving
};

#ifndef OPT_REDIRECTED
#define OPT_REDIRECTED 0x0010
#endif

/* Prototypes - Adapted for Portability */
void EnableMsgButtons(MyWindowPtr win, bool enable);
int SetMessText(MessHandle messH, short whichTXE, char *string,
                  long size);
int RedirectAnnotation(MessHandle messH);
int SigValidate(short sigId);
void SetSig(TOCType * tocH, short sumNum, int sigId);
ControlHandle FindControlByRefCon(MyWindowPtr win, long refCon);
short GetControlValue(ControlHandle cntl);
void SetControlMaximum(ControlHandle cntl, short max);
char *MessCurAddr(MyWindowPtr win, char *addr);

void FindFrom(char *who, GtkWidget *pte);
char *CurAddrSel(MyWindowPtr win, char *addr);

void QuoteLines(GtkWidget *pte, long from, long to, short pfid, long *qEnd);
char *HandleHeadGetPStr(char *text, short head, char *pStr);

int SuckAddresses(char ***addr, char **text, bool b1, bool b2, bool b3,
                  void *p);
int SuckPtrAddresses(char ***addr, const char *text, long size, bool b1, bool b2,
                     bool b3, void *p);
char *ShortAddr(char *shortAddr, const char *longAddr);
short CountAddresses(char **addresses, short atLeast);
bool IsMe(const char *address);
char *GetReturnAddr(char *addr, bool comments);
char *GetRealname(char *name);
void SetSumFlag(TOCType * tocH, short sumNum, long flag);
bool SumFlagIsSet(TOCType * tocH, short sumNum, long flag);

/* Hash function - portable version */
uLong HashWithSeedLo(char *s, uLong n, uLong seed);
#define HashWithSeed(s, seed)                                                  \
  HashWithSeedLo((char *)(s), strlen((char *)(s)), seed)
#define Hash(s) HashWithSeed(s, 1)

int TOCFindMessByMID(uLong mid, TOCType * tocH, long *sumNum);

int AppendMessage(TOCType * fromTocH, int fromN, TOCType ** toTocHP, bool copy,
                  bool toTemp, bool isIMAPtoPopTransfer);
MyWindowPtr GetAMessage(TOCType * tocH, short sumNum, void *u1, void *u2,
                        bool b1);
int EnsureMID(TOCType * tocH, short sumNum);
int SpoolMessage(MessHandle messH, char * theSpec, short refN);
long FindAnAttachment(void *text, long offset, char * spec, bool attach,
                      uLong *cid, uLong *relURL, uLong *absURL);
MyWindowPtr ReopenMessage(MyWindowPtr win);
int FileGraphicChangeGraphic(GtkWidget *pte, long offset, char * spec);
void BoxSelectAfter(MyWindowPtr win, short sumNum);
void Preview(TOCType * tocH, short sumNum);
void MovingAttachments(TOCType * tocH, short sumNum, bool attach, bool wipe,
                       bool toTrash, bool inPlace);
void NukeXfUndo(void);
int RemSpoolFolder(long uidHash);

#endif

void ComputeLocalDate(void *sum, char *dateStr);
