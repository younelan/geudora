/* Minimal Navigation.h shim for builds without macOS Navigation Services.
   This defines just enough types to satisfy references in navUtils.h and
   related code. It is intentionally minimal and portable.
*/

/* Navigation shim: always provide a minimal, portable set of typedefs
   for GTK/non-Carbon builds. This forces a consistent, Carbon-free
   configuration even on macOS builds. */
#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <stdint.h>
#include <stdbool.h>

/* Minimal fallback types */
typedef struct StandardFileReply { int16_t good; void *replyData; } StandardFileReply;

/* Provide both CInfoPB and CInfoPBRec aliases used across the codebase. */
typedef struct CInfoPBRec CInfoPBRec;
typedef struct CInfoPBRec CInfoPB;
typedef CInfoPBRec *CInfoPBPtr;

typedef void *NavDialogRef;
typedef void *NavCBRecPtr;
typedef int32_t NavEventCallbackMessage;
typedef int32_t NavFilterModes;
typedef void *NavTypeListHandle;

/* Provide THz here for headers that expect it when Navigation services are absent */
#ifndef THz
typedef void *THz;
#endif

/* Common file-navigation/finder types missing on non-Carbon builds */
typedef void *SFTypeList;

typedef void *DlgHookYDProcPtr;
typedef void *DlgHookYDUPP;
typedef void *ModalFilterYDUPP;
typedef int (*FileFilterProcPtr)(void *item, void *info);

#endif /* NAVIGATION_H */
