/*
 * Portable PETE shim
 * Minimal, non-Carbon PETE type definitions to allow building the
 * project in a portable (GTK-friendly) environment. Keep structs
 * intentionally small and opaque where possible.
 */
#ifndef PETE_PORTABLE_H
#define PETE_PORTABLE_H

/* Mask legacy PETE header to prevent conflicts */
#ifndef PETE_H
#define PETE_H
#endif

#include <stdint.h>

/* Basic PETE graphic info (opaque-enough for compilation). */
typedef struct PETEGraphicInfo {
  int32_t width;
  int32_t height;
  int32_t depth;
  void *platform; /* platform-specific data if needed */
} PETEGraphicInfo, *PETEGraphicInfoPtr, **PETEGraphicInfoHandle;

/* Main PETE Text Handle (opaque handle) */
typedef void *PETEHandle;

/* Text style used by PETE - minimalist fields. */
typedef struct PETETextStyle {
  int32_t font;
  int32_t size;
  int32_t face;
  int32_t color;
} PETETextStyle, *PETETextStylePtr;

/* Style list / entry representation (linked list). */
/* Style list / entry representation (linked list). */
typedef struct PETEParaInfo PETEParaInfo; /* Forward decl */

typedef struct PETEStyleEntry {
  struct {
    PETETextStyle textStyle;
    /* Graphic info would go here if needed */
  } psStyle;

  long psStartChar;
  long psEndChar;
  struct PETEStyleEntry *next;
} PETEStyleEntry, *PETEStyleList, *PETEStyleEntryPtr, **PETEStyleListHandle;

#define peLabelValid 1
#define peAllValid -1

/* Paragraph information used by PETE. */
typedef struct PETEParaInfo {
  int32_t leftIndent;
  int32_t rightIndent;
  int32_t spacingBefore;
  int32_t spacingAfter;
} PETEParaInfo, *PETEParaInfoPtr;

/* Document initialization info - keep minimal. */
typedef struct PETEDocInitInfo {
  int32_t version;
  void *reserved;
} PETEDocInitInfo, *PETEDocInitInfoPtr;

/* Misc handles */
typedef void *PETEParaScrapHandle;

/* Basic PETE enums and placeholders used by peteglue.h */
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

typedef void *PETEInst;

/* Graphic handler pointer placeholder */
typedef int (*PETEGraphicHandlerProcPtr)(void *ph,
                                         PETEGraphicInfoHandle graphic,
                                         long offset,
                                         PETEGraphicMessage message,
                                         void *data);

#endif /* PETE_PORTABLE_H */
