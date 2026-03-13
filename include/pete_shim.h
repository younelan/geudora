/* Minimal pete.h stub providing small subset of legacy types
   required by the non-UI build of mbx_lib. This is intentionally tiny
   and safe for portable builds. */
#ifndef PETE_SHIM_H
#define PETE_SHIM_H

#include <stdint.h>

/* If a portable PETE shim is available in Include/, prefer it. */
#if defined(__has_include)
# if __has_include("pete_portable.h")
#  include "pete_portable.h"
#  define PETE_SHIM_HAVE_PORTABLE 1
# endif
#endif

/* Prefer project's PETE header when available (unless a portable shim
   is present). When compiling on Apple, tell the PETE header to use
   system (Carbon) types to avoid collisions by defining
   `PETE_USE_SYSTEM_TYPES`. */
#if !defined(PETE_SHIM_HAVE_PORTABLE)
# if defined(__has_include)
#  if __has_include("../Editor/Headers/pete.h")
#   include "../Editor/Headers/pete.h"
#  elif __has_include("Editor/Headers/pete.h")
#   include "Editor/Headers/pete.h"
#  else
#   define PETE_SHIM_NO_REAL_PETE
#  endif
# else
#  define PETE_SHIM_NO_REAL_PETE
# endif
#endif

/* Provide minimal, non-conflicting fallbacks for PETE symbols when the
   real PETE header is not used. On Apple platforms we avoid defining
   Mac/Carbon core typedefs (Handle, Boolean, etc.) so the system
   headers can provide them instead. */
#if defined(PETE_SHIM_NO_REAL_PETE)
/* PETE handle types (opaque) */
/* For gEditCtrl port: PETEHandle is just the GtkWidget containing the editor */
/* Forward-declare GtkWidget so this header can be included without pulling
 * the full GTK headers. The real build will include <gtk/gtk.h> where
 * necessary. */
typedef struct _GtkWidget GtkWidget;
typedef GtkWidget *PETEHandle;
typedef struct mstruct *MessHandle;

/* On non-Apple platforms provide a few legacy Mac types used by older
   code; on Apple rely on the system headers to supply these. */
#if !defined(__APPLE__)
typedef struct FSSpec { int32_t _dummy; } FSSpec;
typedef FSSpec *FSSpecPtr;
typedef char Str31[32];
#endif

/* Minimal opaque PETE structures used by peteglue.h */
typedef struct PETEGraphicInfo PETEGraphicInfo, *PETEGraphicInfoPtr, **PETEGraphicInfoHandle;
typedef struct PETEStyleEntry PETEStyleEntry, *PETEStyleList, *PETEStyleEntryPtr, **PETEStyleListHandle;
typedef struct PETEParaInfo PETEParaInfo, *PETEParaInfoPtr;
typedef struct PETEDocInitInfo PETEDocInitInfo, *PETEDocInitInfoPtr;
typedef void *PETEParaScrapHandle;

#if 0
/* PGP helper enum stub -- disabled because the project defines it elsewhere. */
/* typedef int PGPEnum; */
#endif
#endif

#endif /* PETE_SHIM_H */
