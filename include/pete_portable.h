/*
 * pete_portable.h — Portable PETE type definitions
 *
 * These match the original Mac PETE editor types from
 * Editor/Headers/pete.h and Include/peteglue.h.
 * Field names and layouts are kept identical so that existing code
 * (rich.c, message.c, comp.c, etc.) compiles without modification.
 */
#ifndef PETE_PORTABLE_H
#define PETE_PORTABLE_H

/* Mask legacy PETE header to prevent conflicts */
#ifndef PETE_H
#define PETE_H
#endif

#include <stdint.h>
#include "mailbox.h"  /* RGBColor, Handle, etc. */

/* ===== PETE Constants ===== */

/* Default paragraph and style references */
#define kPETEDefaultPara        (-3L)
#define kPETEDefaultStyle       (-3L)
#define kPETECurrentStyle       (-2L)
#define kPETECurrentSelection   (-1L)
#define kPETEInsertionPoint     (-1L)
#define kPETELastPara           (-1L)

/* Default font and size values */
#define kPETEDefaultFont        (-1)
#define kPETEDefaultFixed       (-2)
#define kPETEDefaultSize        (-1)
#define kPETERelativeSizeBase   (-64)

/* Shift amounts for label and lock fields */
#define kPETELabelShift         20
#define kPETELockShift          16

/* Valid bits for style properties */
#define peBoldValid             0x00000001L
#define peItalicValid           0x00000002L
#define peUnderlineValid        0x00000004L
#define peOutlineValid          0x00000008L
#define peShadowValid           0x00000010L
#define peCondenseValid         0x00000020L
#define peExtendValid           0x00000040L
#define peFaceValid             0x0000007FL
#define peFontValid             0x00000100L
#define peSizeValid             0x00000200L
#define peColorValid            0x00000400L
#define peLangValid             0x00000800L
#define peLockValid             0x000F1000L
#define peLabelValid            0xFFF02000L
#define peGraphicValid          0x00004000L
#define peGraphicColorChangeValid 0x00008000L
#define peAllValid              ((long)0xFFFF3F7FL)

/* Valid bits for paragraph properties */
#define peStartMarginValid      0x0001
#define peEndMarginValid        0x0002
#define peIndentValid           0x0004
#define peDirectionValid        0x0008
#define peJustificationValid    0x0010
#define peFlagsValid            0x0020
#define peTabsValid             0x0040
#define peQuoteLevelValid       0x0080
#define peSignedLevelValid      0x0100
#define peAllParaValid          0x01FF

/* ===== PETE Type Definitions ===== */

/* Main PETE instance and handle — in GTK, PETEHandle = GtkWidget* */
typedef void *PETEHandle;
typedef void *PETEInst;

/* PETE global instance — unused in GTK, passed as NULL */
#ifndef PETE
#define PETE ((PETEInst)NULL)
#endif

/* Text style — matches original Mac PETE struct layout */
typedef struct PETETextStyle {
	short tsFont;               /* Font ID */
	uint32_t tsFace;            /* Style face flags (bold, italic, etc.) */
	char filler;                /* Padding */
	short tsSize;               /* Font size */
	RGBColor tsColor;           /* Text color */
	int32_t tsLang;             /* Language code */
	unsigned short tsLock : 4;  /* Lock bits */
	unsigned short tsLabel : 12; /* Label for application use */
} PETETextStyle, *PETETextStylePtr;

/* Graphic style — same fields but graphicInfo instead of color */
typedef struct PETEGraphicInfo PETEGraphicInfo;
typedef PETEGraphicInfo *PETEGraphicInfoPtr;
typedef PETEGraphicInfo **PETEGraphicInfoHandle;

struct PETEGraphicInfo {
	void *itemProc;             /* Graphic handler callback */
	short width;                /* Width in pixels */
	short height;               /* Height in pixels */
	short descent;              /* Descent below baseline */
	uint32_t flags;             /* drawInWindow, cloneReplaceText, etc. */
	long privateType;           /* For application's use */
};

typedef struct PETEGraphicStyle {
	short tsFont;
	uint32_t tsFace;
	char filler;
	short tsSize;
	PETEGraphicInfoHandle graphicInfo;
	short filler0;
	int32_t tsLang;
	unsigned short tsLock : 4;
	unsigned short tsLabel : 12;
} PETEGraphicStyle, *PETEGraphicStylePtr;

/* Style info union */
typedef union PETEStyleInfo {
	PETETextStyle textStyle;
	PETEGraphicStyle graphicStyle;
} PETEStyleInfo, *PETEStyleInfoPtr;

/* Style entry — one run of styled text */
typedef struct PETEStyleEntry {
	long psStartChar;           /* Character offset where this style starts */
	long psGraphic;             /* 0 for text, 1 for graphic */
	PETEStyleInfo psStyle;      /* Union containing text or graphic style */
} PETEStyleEntry, *PETEStyleList, *PETEStyleEntryPtr, **PETEStyleListHandle;

/* Paragraph info — per-paragraph formatting */
typedef struct PETEParaInfo {
	long paraOffset;            /* Offset to paragraph start (set by GetParaInfo) */
	long paraLength;            /* Length of paragraph (set by GetParaInfo) */
	short startMargin;          /* Left margin in pixels */
	short filler1;
	short endMargin;            /* Right margin in pixels */
	short filler2;
	short indent;               /* First-line indent in pixels */
	short filler3;
	short direction;            /* Text direction */
	short justification;        /* Alignment: left, center, right, justify */
	uint8_t signedLevel : 4;    /* Signature level */
	uint8_t quoteLevel : 4;     /* Quote level */
	uint8_t paraFlags;          /* Paragraph flags */
	short tabCount;             /* Number of tab stops */
	short **tabHandle;          /* Handle to tab stop array */
} PETEParaInfo, *PETEParaInfoPtr;

/* Paragraph scrap — for clipboard/serialization */
typedef struct PETEParaScrapEntry {
	long paraLength;
	short startMargin;
	short filler1;
	short endMargin;
	short filler2;
	short indent;
	short filler3;
	short direction;
	short justification;
	uint8_t signedLevel : 4;
	uint8_t quoteLevel : 4;
	uint8_t paraFlags;
	short tabCount;
} PETEParaScrapEntry;
typedef PETEParaScrapEntry **PETEParaScrapHandle;

/* Document initialization info */
typedef struct PETEDocInitInfo {
	int32_t version;
	void *reserved;
} PETEDocInitInfo, *PETEDocInitInfoPtr;

/* Label style entry */
typedef struct PETELabelStyleEntry {
	unsigned short plLabel;
	unsigned short plValidBits;
	short plFont;
	uint32_t plFace;
	char filler;
	short plSize;
	RGBColor plColor;
	short plColorWeight;
} PETELabelStyleEntry, *PETELabelStylePtr, *PETELabelStyleList, **PETELabelStyleHandle;

/* PeteSaneMargin — paragraph margin info */
#ifndef PETE_SANE_MARGIN_DEFINED
#define PETE_SANE_MARGIN_DEFINED
typedef struct {
	long first;   /* First line margin */
	long second;  /* Subsequent lines margin */
	long right;   /* Right margin */
} PeteSaneMargin, *PSMPtr, **PSMHandle;
#endif

/* ===== PETE Enums ===== */

typedef enum {
	peeCut,
	peeCopy,
	peePaste,
	peeClear,
	peeEvent,
	peeUndo,
	peeCutPlain,
	peeCopyPlain,
	peePastePlain,
	peeLimit = 0x7FFF
} PETEEditEnum;

typedef enum {
	peSetDragContents,
	peGetDragContents,
	peProgressLoop,
	peDocChanged,
	peHasBeenCalled,
	peWordBreak,
	peIntelligentCut,
	peIntelligentPaste,
	peCallbackLimit = 0x7FFF
} PETECallbackEnum;

typedef enum {
	peGraphicDraw,
	peGraphicClone,
	peGraphicTest,
	peGraphicHit,
	peGraphicRemove,
	peGraphicResize,
	peGraphicRegion,
	peGraphicEvent,
	peGraphicInsert,
	peGraphicNewText
} PETEGraphicMessage;

typedef enum {
	peCantUndo,
	peUndoTyping,
	peUndoCut,
	peUndoPaste,
	peUndoClear,
	peUndoStyle,
	peUndoDrag,
	peUndoPara,
	peUndoCutPlain,
	peUndoStyleAndPara,
	peUndoLast = peUndoStyleAndPara,
	peUndoMaximum = 0x7FFF,
	peRedoTyping = -peUndoTyping,
	peRedoCut = -peUndoCut,
	peRedoPaste = -peUndoPaste,
	peRedoClear = -peUndoClear,
	peRedoStyle = -peUndoStyle,
	peRedoDrag = -peUndoDrag,
	peRedoPara = -peUndoPara,
	peRedoCutPlain = -peUndoCutPlain,
	peRedoStyleAndPara = -peUndoStyleAndPara,
	peRedoLast = -peUndoLast
} PETEUndoEnum;

typedef enum {
	peNoLock = 0x00,
	peModLock = 0x01,
	peClickAfterLock = 0x02,
	peClickBeforeLock = 0x04,
	peSelectLock = 0x08
} PETELockBits;

typedef enum {
	peTextOnly = 0x01,
	pePlainTextOnly = 0x02,
	peNoParaPaste = 0x04,
	peDiskList = 0x10,
	peSquareList = 0x20,
	peCircleList = 0x40,
	peListBits = 0x70
} PETEParaInfoFlags;

/* Graphic handler pointer */
typedef int (*PETEGraphicHandlerProcPtr)(void *ph,
                                         PETEGraphicInfoHandle graphic,
                                         long offset,
                                         PETEGraphicMessage message,
                                         void *data);

/* ===== PETE Function Declarations ===== */
/* These wrap gEditCtrl operations — implemented in peteglue.c */

int PETEGetParaInfo(PETEInst pi, PETEHandle pte, long paraIndex, PETEParaInfo *info);
int PETESetParaInfo(PETEInst pi, PETEHandle pte, long paraIndex, PETEParaInfo *info, long validBits);
int PETEGetStyle(PETEInst pi, PETEHandle pte, long offset, long *len, PETEStyleEntry *style);
int PETESetStyle(PETEInst pi, PETEHandle pte, long start, long stop, PETEStyleInfo *style, long validBits);
int PETESetTextStyle(PETEInst pi, PETEHandle pte, long start, long stop, PETETextStyle *style, long validBits);
int PETEInsertParaBreak(PETEInst pi, PETEHandle pte, long offset);
int PETESelectGraphic(PETEInst pi, PETEHandle pte, long offset);
int PETEGetParaIndex(PETEInst pi, PETEHandle pte, long offset, long *index);
int PETESetRecalcState(PETEInst pi, PETEHandle pte, int state);
long PETEGetRefCon(PETEInst pi, PETEHandle pte);
void PETESetRefCon(PETEInst pi, PETEHandle pte, long refCon);
void PETEMarkDocDirty(PETEInst pi, PETEHandle pte, int dirty);

/* Eudora peteglue convenience functions */
void PeteStyleAt(PETEHandle pte, long offset, PETEStyleEntry *style);
void PeteGetStyle(PETEHandle pte, long offset, long *runLen, PETEStyleEntry *style);
short PeteTextStyleDiff(PETETextStylePtr s1, PETETextStylePtr s2);
short PeteParaInfoDiff(PETEParaInfoPtr s1, PETEParaInfoPtr s2);
void PeteLabel(PETEHandle pte, long start, long stop, short label, short mask);
void PeteLock(PETEHandle pte, long start, long stop, short lock);
void PeteWrap(void *win, PETEHandle pte, int wrap);
int PeteInsertIntlText(PETEHandle pte, long *offset, void *text, long len,
                       long encoding1, void *converter, long encoding2,
                       int b1, int b2);
int PeteParaConvert(PETEHandle pte, long start, long end);

#endif /* PETE_PORTABLE_H */
