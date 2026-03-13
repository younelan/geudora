/* scripting_ae.c — Apple Events scripting backend for Eudora (macOS)
 *
 * Exposes Eudora's scripting API via Apple Events so AppleScript and
 * other macOS automation tools can manage filters, mailboxes, etc.
 *
 * This is the macOS-specific transport layer. It translates incoming
 * Apple Events into calls to the platform-neutral ScriptXxx() functions
 * defined in scripting.h and implemented in filters.c.
 *
 * Original Eudora (Mac) had these AE handlers mixed into filters.c.
 * This file separates the transport (AE) from the logic (CRUD).
 *
 * Build: only on macOS. Guard with #ifdef __APPLE__ or build-system
 * conditional.
 *
 * Copyright (c) 2017, Computer History Museum. All rights reserved.
 * See LICENSE for terms.
 */

#ifdef __APPLE__

#include "scripting.h"
#include <stdbool.h>
#include <string.h>

/* On macOS, Apple Events types come from Carbon/ApplicationServices.
 * For a GTK4 macOS build we'd use the Objective-C bridge or the
 * low-level AE C API via CoreServices. */
#include <Carbon/Carbon.h>

/* Eudora scripting dictionary four-char codes (from original aete resource) */
#define cEuFilter       'euFi'
#define pEuManual       'euMn'
#define pEuOutgoing     'euOg'
#define pEuConjunction  'euCj'
#define pEuSubject      'euSb'
#define pEuLabel        'euLb'
#define pEuPriority     'euPr'
#define pEuCopy         'euCp'
#define pEuFilterUse    'euFU'
#define pEuFilterHeader 'euFH'
#define pEuFilterVerb   'euFV'
#define pEuFilterValue  'euFl'

/*----------------------------------------------------------------------
 * Map Apple Events DescType property codes to ScriptPropertyID
 *--------------------------------------------------------------------*/
static ScriptPropertyID ae_prop_to_script(DescType prop)
{
  switch (prop) {
  case pName:           return kScriptPropName;
  case formUniqueID:    return kScriptPropId;
  case pEuFilterUse:    return kScriptPropLastMatch;
  case 'IN  ':          return kScriptPropIncoming;
  case pEuOutgoing:     return kScriptPropOutgoing;
  case pEuManual:       return kScriptPropManual;
  case pEuConjunction:  return kScriptPropConjunction;
  case 'euMB':          return kScriptPropTransferMailbox;
  case pEuCopy:         return kScriptPropCopyInstead;
  case pEuFilterHeader: return kScriptPropTermHeader;
  case pEuFilterVerb:   return kScriptPropTermVerb;
  case pEuFilterValue:  return kScriptPropTermValue;
  default:              return (ScriptPropertyID)-1;
  }
}

/*----------------------------------------------------------------------
 * AE handler: create filter
 *--------------------------------------------------------------------*/
static OSErr AECreateFilterHandler(const AppleEvent *event,
                                   AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  long newId = 0;
  DescType where;
  DescType junk;
  Size actualSize;
  int position = -1;  /* default: end */

  /* Check for position parameter */
  AEDesc insertLoc;
  if (AEGetParamDesc(event, keyAEInsertHere, typeWildCard, &insertLoc) == noErr) {
    if (AEGetKeyPtr(&insertLoc, keyAEPosition, typeEnumeration,
                     &junk, &where, sizeof(where), &actualSize) == noErr) {
      if (where == kAEBeginning) position = 0;
    }
    AEDisposeDesc(&insertLoc);
  }

  int err = ScriptCreateFilter(position, &newId);
  if (!err && reply)
    AEPutParamPtr(reply, keyAEResult, typeSInt64, &newId, sizeof(newId));

  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: delete filter
 *--------------------------------------------------------------------*/
static OSErr AEDeleteFilterHandler(const AppleEvent *event,
                                   AppleEvent *reply, SRefCon refcon)
{
  (void)reply; (void)refcon;
  SInt64 filterId;
  DescType junk;
  Size actualSize;

  if (AEGetParamPtr(event, keyDirectObject, typeSInt64, &junk,
                     &filterId, sizeof(filterId), &actualSize) != noErr)
    return errAEParamMissed;

  int err = ScriptDeleteFilter((long)filterId, true);
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: get filter property
 *--------------------------------------------------------------------*/
static OSErr AEGetFilterPropertyHandler(const AppleEvent *event,
                                        AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  SInt64 filterId;
  DescType aeProp;
  DescType junk;
  Size actualSize;

  if (AEGetParamPtr(event, keyDirectObject, typeSInt64, &junk,
                     &filterId, sizeof(filterId), &actualSize) != noErr)
    return errAEParamMissed;

  if (AEGetParamPtr(event, 'prop', typeType, &junk,
                     &aeProp, sizeof(aeProp), &actualSize) != noErr)
    return errAEParamMissed;

  ScriptPropertyID prop = ae_prop_to_script(aeProp);
  if ((int)prop < 0) return errAENoSuchObject;

  ScriptValue out;
  memset(&out, 0, sizeof(out));
  int err = ScriptGetFilterProperty((long)filterId, true, prop, &out);
  if (err) return errAEEventNotHandled;

  /* Put result into reply */
  if (reply) {
    switch (out.type) {
    case kScriptValString:
      AEPutParamPtr(reply, keyAEResult, typeUTF8Text,
                     out.u.str, strlen(out.u.str));
      break;
    case kScriptValLong: {
      SInt64 val = out.u.num;
      AEPutParamPtr(reply, keyAEResult, typeSInt64, &val, sizeof(val));
      break;
    }
    case kScriptValBool: {
      Boolean val = out.u.flag;
      AEPutParamPtr(reply, keyAEResult, typeBoolean, &val, sizeof(val));
      break;
    }
    default:
      break;
    }
  }
  return noErr;
}

/*----------------------------------------------------------------------
 * AE handler: count filters
 *--------------------------------------------------------------------*/
static OSErr AECountFiltersHandler(const AppleEvent *event,
                                   AppleEvent *reply, SRefCon refcon)
{
  (void)event; (void)refcon;
  long count = 0;
  ScriptCountFilters(&count);
  if (reply) {
    SInt32 c = (SInt32)count;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &c, sizeof(c));
  }
  return noErr;
}

/*======================================================================
 * ScriptingInit / ScriptingShutdown — Apple Events backend
 *====================================================================*/

int ScriptingInit(void)
{
  /* Install Apple Event handlers for filter scripting.
   * Event class 'euFi' (Eudora Filter), various event IDs. */
  AEInstallEventHandler('euFi', 'crea',
    NewAEEventHandlerUPP(AECreateFilterHandler), 0, false);
  AEInstallEventHandler('euFi', 'delo',
    NewAEEventHandlerUPP(AEDeleteFilterHandler), 0, false);
  AEInstallEventHandler('euFi', 'getd',
    NewAEEventHandlerUPP(AEGetFilterPropertyHandler), 0, false);
  AEInstallEventHandler('euFi', 'cntf',
    NewAEEventHandlerUPP(AECountFiltersHandler), 0, false);

  return 0;
}

void ScriptingShutdown(void)
{
  AERemoveEventHandler('euFi', 'crea', NULL, false);
  AERemoveEventHandler('euFi', 'delo', NULL, false);
  AERemoveEventHandler('euFi', 'getd', NULL, false);
  AERemoveEventHandler('euFi', 'cntf', NULL, false);
}

#else /* !__APPLE__ */

/* Not on macOS — provide empty implementations so the linker is happy
 * if this file gets compiled on other platforms. The real backend
 * for Linux is scripting_dbus.c. */
#include "scripting.h"

/* These are weak symbols; the D-Bus backend provides the real ones.
 * Only needed if both files are accidentally linked on the same
 * platform. */
int  __attribute__((weak)) ScriptingInit(void) { return 0; }
void __attribute__((weak)) ScriptingShutdown(void) {}

#endif /* __APPLE__ */
