/* portable-compat.h shim */
#ifndef PORTABLE_COMPAT_H
#define PORTABLE_COMPAT_H

#include "mailbox.h"
#include <stdbool.h>
#include <stdint.h>

/* Basic Mac/Eudora types for portability - Rely on mailbox.h */
#ifndef STRING_HANDLE_DEFINED
#define STRING_HANDLE_DEFINED
typedef char *StringHandle;
#endif

typedef unsigned char Byte;

#ifndef noErr
#define noErr 0
#endif

#ifndef nil
#define nil 0
#endif

typedef int32_t SInt32;
typedef uint32_t UInt32;
typedef int16_t SInt16;
typedef uint16_t UInt16;
typedef int8_t SInt8;
typedef uint8_t UInt8;
typedef uint32_t ID;


#ifndef OSStatus
typedef int OSStatus;
#endif

#ifndef Boolean
typedef bool Boolean;
#endif

/* GTK/GDK Cross-Platform Abstraction Types for EMS API */
#ifdef __has_include
#if __has_include(<gtk/gtk.h>)
#include <gtk/gtk.h>
/* GdkDrag is only available in GTK 4.x, use void* for GTK 3.x */
#ifndef GDK_TYPE_DRAG
typedef void GdkDrag;
#endif
#else
typedef void GtkWindow;
typedef void GdkEvent;
typedef int GdkEventType;
typedef int GdkModifierType;
typedef int GdkDragAction;
typedef void GdkDrag;
#endif
#else
#include <gtk/gtk.h>
/* GdkDrag is only available in GTK 4.x, use void* for GTK 3.x */
#ifndef GDK_TYPE_DRAG
typedef void GdkDrag;
#endif
#endif

typedef GtkWindow *EUDORA_WindowPtr;
typedef GdkEvent *EUDORA_EventRecord;
typedef GdkEventType EUDORA_EventKind;
typedef GdkModifierType EUDORA_EventModifiers;
typedef GdkDragAction EUDORA_DragTrackingMessage;
typedef GdkDrag *EUDORA_DragReference;

typedef struct {
  double x;
  double y;
} EUDORA_Point;

#ifndef portable_compat_h
#define portable_compat_h

#define userCanceledErr -128

#endif

#ifndef pascal
#define pascal
#endif

#include <pthread.h>
typedef pthread_t ThreadID;

/* Eudora-specific stubs for compilation */
typedef void *FMBHandle;
typedef void *ICacheHandle;
/* MyOTTCPStreamHandle is defined in tcp.h - use that definition */
typedef void *NagStateHandle;
typedef void *FeatureRecHandle;

/* PGPRecvContextPtr is defined in pgpin.h - do not redefine here */
typedef void *ProxyHandle;

/* Forward declare Personality for PersHandle */
struct Personality;
typedef struct Personality *PersHandle;

typedef struct {
  uint32_t high;
  uint32_t low;
} ProcessSerialNumber;

typedef struct {
  short dummy;
} ScriptFontInfo;

typedef struct {
  short dummy;
} MStoreSubFile;

typedef struct {
  short dummy;
} NoAdsAuxRec;

typedef struct {
  short dummy;
} ICMPReport;

#ifndef NMREC_DEFINED
#define NMREC_DEFINED
typedef struct NMRec {
  short dummy;
} NMRec;
#endif

typedef struct {
  short dummy;
} ConnHandle;

#ifndef NSpare
#define NSpare 2
#endif

/* Stub for mail.h if not found in expected paths */
#ifndef MAIL_H
#include "mail.h"
#endif

#endif
