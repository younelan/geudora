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

/*----------------------------------------------------------------------
 * AE handler: count personalities
 *--------------------------------------------------------------------*/
#define cEuPersonality  'euPe'

static OSErr AECountPersonalitiesHandler(const AppleEvent *event,
                                          AppleEvent *reply, SRefCon refcon)
{
  (void)event; (void)refcon;
  long count = 0;
  ScriptCountPersonalities(&count);
  if (reply) {
    SInt32 c = (SInt32)count;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &c, sizeof(c));
  }
  return noErr;
}

/*----------------------------------------------------------------------
 * AE handler: create personality
 *--------------------------------------------------------------------*/
static OSErr AECreatePersonalityHandler(const AppleEvent *event,
                                         AppleEvent *reply, SRefCon refcon)
{
  (void)event; (void)refcon;
  long newIndex = 0;
  int err = ScriptCreatePersonality(&newIndex);
  if (!err && reply) {
    SInt32 idx = (SInt32)newIndex;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &idx, sizeof(idx));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: get personality property
 *--------------------------------------------------------------------*/
static OSErr AEGetPersPropertyHandler(const AppleEvent *event,
                                       AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  SInt32 index;
  DescType aeProp;
  DescType junk;
  Size actualSize;

  if (AEGetParamPtr(event, keyDirectObject, typeSInt32, &junk,
                     &index, sizeof(index), &actualSize) != noErr)
    return errAEParamMissed;

  if (AEGetParamPtr(event, 'prop', typeType, &junk,
                     &aeProp, sizeof(aeProp), &actualSize) != noErr)
    return errAEParamMissed;

  /* Map AE property to ScriptPropertyID */
  ScriptPropertyID prop;
  if (aeProp == pName) prop = kScriptPropName;
  else if (aeProp == formUniqueID) prop = kScriptPropId;
  else return errAENoSuchObject;

  ScriptValue out;
  memset(&out, 0, sizeof(out));
  int err = ScriptGetPersonalityProperty((long)index, prop, &out);
  if (err) return errAEEventNotHandled;

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
    default:
      break;
    }
  }
  return noErr;
}

/*----------------------------------------------------------------------
 * AE handler: set personality property
 *--------------------------------------------------------------------*/
static OSErr AESetPersPropertyHandler(const AppleEvent *event,
                                       AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  SInt32 index;
  DescType aeProp;
  DescType junk;
  Size actualSize;

  if (AEGetParamPtr(event, keyDirectObject, typeSInt32, &junk,
                     &index, sizeof(index), &actualSize) != noErr)
    return errAEParamMissed;

  if (AEGetParamPtr(event, 'prop', typeType, &junk,
                     &aeProp, sizeof(aeProp), &actualSize) != noErr)
    return errAEParamMissed;

  /* Map AE property to ScriptPropertyID */
  ScriptPropertyID prop;
  if (aeProp == pName) prop = kScriptPropName;
  else return errAENoSuchObject;

  /* Get the value to set */
  ScriptValue val;
  memset(&val, 0, sizeof(val));

  if (prop == kScriptPropName) {
    char buf[256];
    if (AEGetParamPtr(event, 'data', typeUTF8Text, &junk,
                       buf, sizeof(buf) - 1, &actualSize) != noErr)
      return errAEParamMissed;
    buf[actualSize < 255 ? actualSize : 255] = '\0';
    val = ScriptString(buf);
  }

  int err = ScriptSetPersonalityProperty((long)index, prop, &val);
  if (!err && reply) {
    Boolean success = true;
    AEPutParamPtr(reply, keyAEResult, typeBoolean, &success, sizeof(success));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: delete personality
 *--------------------------------------------------------------------*/
static OSErr AEDeletePersonalityHandler(const AppleEvent *event,
                                         AppleEvent *reply, SRefCon refcon)
{
  (void)reply; (void)refcon;
  SInt32 index;
  DescType junk;
  Size actualSize;

  if (AEGetParamPtr(event, keyDirectObject, typeSInt32, &junk,
                     &index, sizeof(index), &actualSize) != noErr)
    return errAEParamMissed;

  int err = ScriptDeletePersonality((long)index);
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: check mail
 *--------------------------------------------------------------------*/
static OSErr AECheckMailHandler(const AppleEvent *event,
                                 AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  DescType junk;
  Size actualSize;
  Boolean doCheck = true, doSend = false;

  /* Optional parameters for check/send flags */
  AEGetParamPtr(event, 'chck', typeBoolean, &junk,
                 &doCheck, sizeof(doCheck), &actualSize);
  AEGetParamPtr(event, 'send', typeBoolean, &junk,
                 &doSend, sizeof(doSend), &actualSize);

  int err = ScriptCheckMail(doCheck, doSend);
  if (reply) {
    Boolean success = (err == 0);
    AEPutParamPtr(reply, keyAEResult, typeBoolean, &success, sizeof(success));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE helper: extract mailbox path + message index from event params
 *--------------------------------------------------------------------*/
static int ae_get_mailbox_and_index(const AppleEvent *event,
                                     char *mailbox, size_t mailboxSz,
                                     SInt32 *index)
{
  DescType junk;
  Size actualSize;

  if (AEGetParamPtr(event, 'mbox', typeUTF8Text, &junk,
                     mailbox, mailboxSz - 1, &actualSize) != noErr)
    return -1;
  mailbox[actualSize < (Size)(mailboxSz - 1) ? actualSize : mailboxSz - 1] = '\0';

  if (AEGetParamPtr(event, 'midx', typeSInt32, &junk,
                     index, sizeof(*index), &actualSize) != noErr)
    return -1;

  return 0;
}

/*----------------------------------------------------------------------
 * AE handler: count messages in a mailbox
 *--------------------------------------------------------------------*/
static OSErr AECountMessagesHandler(const AppleEvent *event,
                                     AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  DescType junk;
  Size actualSize;
  char mailbox[1024];

  if (AEGetParamPtr(event, 'mbox', typeUTF8Text, &junk,
                     mailbox, sizeof(mailbox) - 1, &actualSize) != noErr)
    return errAEParamMissed;
  mailbox[actualSize < (Size)(sizeof(mailbox) - 1) ? actualSize : sizeof(mailbox) - 1] = '\0';

  long count = 0;
  int err = ScriptCountMessages(mailbox, &count);
  if (!err && reply) {
    SInt32 c = (SInt32)count;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &c, sizeof(c));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: get message property
 *--------------------------------------------------------------------*/
static OSErr AEGetMessagePropertyHandler(const AppleEvent *event,
                                          AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  char mailbox[1024];
  SInt32 index;
  DescType aeProp, junk;
  Size actualSize;

  if (ae_get_mailbox_and_index(event, mailbox, sizeof(mailbox), &index))
    return errAEParamMissed;

  if (AEGetParamPtr(event, 'prop', typeType, &junk,
                     &aeProp, sizeof(aeProp), &actualSize) != noErr)
    return errAEParamMissed;

  /* Map AE property to ScriptPropertyID */
  ScriptPropertyID prop;
  switch (aeProp) {
  case 'euPY': prop = kScriptPropPriority; break;
  case 'euST': prop = kScriptPropStatus; break;
  case 'euSe': prop = kScriptPropSender; break;
  case 'euDa': prop = kScriptPropDate; break;
  case 'euSu': prop = kScriptPropSubject; break;
  case 'euSi': prop = kScriptPropSize; break;
  case 'euOu': prop = kScriptPropIsOutgoing; break;
  case 'eLbl': prop = kScriptPropLabel; break;
  case 'eWrp': prop = kScriptPropWrap; break;
  case 'eCpy': prop = kScriptPropKeepCopy; break;
  case 'eRRR': prop = kScriptPropReturnReceipt; break;
  default:     return errAENoSuchObject;
  }

  ScriptValue out;
  memset(&out, 0, sizeof(out));
  int err = ScriptGetMessageProperty(mailbox, (long)index, prop, &out);
  if (err) return errAEEventNotHandled;

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
 * AE handler: set message property
 *--------------------------------------------------------------------*/
static OSErr AESetMessagePropertyHandler(const AppleEvent *event,
                                          AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  char mailbox[1024];
  SInt32 index;
  DescType aeProp, junk;
  Size actualSize;

  if (ae_get_mailbox_and_index(event, mailbox, sizeof(mailbox), &index))
    return errAEParamMissed;

  if (AEGetParamPtr(event, 'prop', typeType, &junk,
                     &aeProp, sizeof(aeProp), &actualSize) != noErr)
    return errAEParamMissed;

  ScriptPropertyID prop;
  switch (aeProp) {
  case 'euPY': prop = kScriptPropPriority; break;
  case 'euST': prop = kScriptPropStatus; break;
  case 'euSu': prop = kScriptPropSubject; break;
  case 'eLbl': prop = kScriptPropLabel; break;
  case 'eWrp': prop = kScriptPropWrap; break;
  case 'eCpy': prop = kScriptPropKeepCopy; break;
  case 'eRRR': prop = kScriptPropReturnReceipt; break;
  default:     return errAENoSuchObject;
  }

  ScriptValue val;
  memset(&val, 0, sizeof(val));

  /* Try string first, then integer, then boolean */
  char buf[256];
  SInt64 numVal;
  Boolean boolVal;
  if (AEGetParamPtr(event, 'data', typeUTF8Text, &junk,
                     buf, sizeof(buf) - 1, &actualSize) == noErr) {
    buf[actualSize < 255 ? actualSize : 255] = '\0';
    val = ScriptString(buf);
  } else if (AEGetParamPtr(event, 'data', typeSInt64, &junk,
                            &numVal, sizeof(numVal), &actualSize) == noErr) {
    val = ScriptLong((long)numVal);
  } else if (AEGetParamPtr(event, 'data', typeBoolean, &junk,
                            &boolVal, sizeof(boolVal), &actualSize) == noErr) {
    val = ScriptBool(boolVal);
  } else {
    return errAEParamMissed;
  }

  int err = ScriptSetMessageProperty(mailbox, (long)index, prop, &val);
  if (!err && reply) {
    Boolean success = true;
    AEPutParamPtr(reply, keyAEResult, typeBoolean, &success, sizeof(success));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: create message (compose)
 *--------------------------------------------------------------------*/
static OSErr AECreateMessageHandler(const AppleEvent *event,
                                     AppleEvent *reply, SRefCon refcon)
{
  (void)event; (void)refcon;
  long newIndex = 0;
  int err = ScriptCreateMessage(&newIndex);
  if (!err && reply) {
    SInt32 idx = (SInt32)newIndex;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &idx, sizeof(idx));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: reply to message
 *--------------------------------------------------------------------*/
static OSErr AEReplyMessageHandler(const AppleEvent *event,
                                    AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  char mailbox[1024];
  SInt32 index;
  DescType junk;
  Size actualSize;
  Boolean replyAll = false, includeSelf = false, quoteText = true;

  if (ae_get_mailbox_and_index(event, mailbox, sizeof(mailbox), &index))
    return errAEParamMissed;

  AEGetParamPtr(event, 'eRAl', typeBoolean, &junk,
                 &replyAll, sizeof(replyAll), &actualSize);
  AEGetParamPtr(event, 'eSlf', typeBoolean, &junk,
                 &includeSelf, sizeof(includeSelf), &actualSize);
  AEGetParamPtr(event, 'eQTx', typeBoolean, &junk,
                 &quoteText, sizeof(quoteText), &actualSize);

  long newIndex = 0;
  int err = ScriptReplyMessage(mailbox, (long)index,
                                replyAll, includeSelf, quoteText, &newIndex);
  if (!err && reply) {
    SInt32 idx = (SInt32)newIndex;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &idx, sizeof(idx));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: forward message
 *--------------------------------------------------------------------*/
static OSErr AEForwardMessageHandler(const AppleEvent *event,
                                      AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  char mailbox[1024];
  SInt32 index;

  if (ae_get_mailbox_and_index(event, mailbox, sizeof(mailbox), &index))
    return errAEParamMissed;

  long newIndex = 0;
  int err = ScriptForwardMessage(mailbox, (long)index, &newIndex);
  if (!err && reply) {
    SInt32 idx = (SInt32)newIndex;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &idx, sizeof(idx));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: redirect message
 *--------------------------------------------------------------------*/
static OSErr AERedirectMessageHandler(const AppleEvent *event,
                                       AppleEvent *reply, SRefCon refcon)
{
  (void)refcon;
  char mailbox[1024];
  SInt32 index;

  if (ae_get_mailbox_and_index(event, mailbox, sizeof(mailbox), &index))
    return errAEParamMissed;

  long newIndex = 0;
  int err = ScriptRedirectMessage(mailbox, (long)index, &newIndex);
  if (!err && reply) {
    SInt32 idx = (SInt32)newIndex;
    AEPutParamPtr(reply, keyAEResult, typeSInt32, &idx, sizeof(idx));
  }
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: queue message for sending
 *--------------------------------------------------------------------*/
static OSErr AEQueueMessageHandler(const AppleEvent *event,
                                    AppleEvent *reply, SRefCon refcon)
{
  (void)reply; (void)refcon;
  DescType junk;
  Size actualSize;
  SInt32 index;

  if (AEGetParamPtr(event, 'midx', typeSInt32, &junk,
                     &index, sizeof(index), &actualSize) != noErr)
    return errAEParamMissed;

  int err = ScriptQueueMessage((long)index);
  return err ? errAEEventNotHandled : noErr;
}

/*----------------------------------------------------------------------
 * AE handler: move/copy message between mailboxes
 *--------------------------------------------------------------------*/
static OSErr AEMoveMessageHandler(const AppleEvent *event,
                                   AppleEvent *reply, SRefCon refcon)
{
  (void)reply; (void)refcon;
  DescType junk;
  Size actualSize;
  char fromBox[1024], toBox[1024];
  SInt32 index;
  Boolean copy = false;

  if (AEGetParamPtr(event, 'frmb', typeUTF8Text, &junk,
                     fromBox, sizeof(fromBox) - 1, &actualSize) != noErr)
    return errAEParamMissed;
  fromBox[actualSize < (Size)(sizeof(fromBox) - 1) ? actualSize : sizeof(fromBox) - 1] = '\0';

  if (AEGetParamPtr(event, 'midx', typeSInt32, &junk,
                     &index, sizeof(index), &actualSize) != noErr)
    return errAEParamMissed;

  if (AEGetParamPtr(event, 'tomb', typeUTF8Text, &junk,
                     toBox, sizeof(toBox) - 1, &actualSize) != noErr)
    return errAEParamMissed;
  toBox[actualSize < (Size)(sizeof(toBox) - 1) ? actualSize : sizeof(toBox) - 1] = '\0';

  AEGetParamPtr(event, 'copy', typeBoolean, &junk,
                 &copy, sizeof(copy), &actualSize);

  int err = ScriptMoveMessage(fromBox, (long)index, toBox, copy);
  return err ? errAEEventNotHandled : noErr;
}

/*======================================================================
 * ScriptingInit / ScriptingShutdown — Apple Events backend
 *====================================================================*/

int ScriptingInit(void)
{
  /* Filter scripting — event class 'euFi' */
  AEInstallEventHandler('euFi', 'crea',
    NewAEEventHandlerUPP(AECreateFilterHandler), 0, false);
  AEInstallEventHandler('euFi', 'delo',
    NewAEEventHandlerUPP(AEDeleteFilterHandler), 0, false);
  AEInstallEventHandler('euFi', 'getd',
    NewAEEventHandlerUPP(AEGetFilterPropertyHandler), 0, false);
  AEInstallEventHandler('euFi', 'cntf',
    NewAEEventHandlerUPP(AECountFiltersHandler), 0, false);

  /* Personality scripting — event class 'euPe' */
  AEInstallEventHandler('euPe', 'cntP',
    NewAEEventHandlerUPP(AECountPersonalitiesHandler), 0, false);
  AEInstallEventHandler('euPe', 'crea',
    NewAEEventHandlerUPP(AECreatePersonalityHandler), 0, false);
  AEInstallEventHandler('euPe', 'getd',
    NewAEEventHandlerUPP(AEGetPersPropertyHandler), 0, false);
  AEInstallEventHandler('euPe', 'setd',
    NewAEEventHandlerUPP(AESetPersPropertyHandler), 0, false);
  AEInstallEventHandler('euPe', 'delo',
    NewAEEventHandlerUPP(AEDeletePersonalityHandler), 0, false);

  /* Mail transfer scripting — event class 'euMl' */
  AEInstallEventHandler('euMl', 'chck',
    NewAEEventHandlerUPP(AECheckMailHandler), 0, false);

  /* Message scripting — event class 'euMS' */
  AEInstallEventHandler('euMS', 'cntM',
    NewAEEventHandlerUPP(AECountMessagesHandler), 0, false);
  AEInstallEventHandler('euMS', 'getd',
    NewAEEventHandlerUPP(AEGetMessagePropertyHandler), 0, false);
  AEInstallEventHandler('euMS', 'setd',
    NewAEEventHandlerUPP(AESetMessagePropertyHandler), 0, false);
  AEInstallEventHandler('euMS', 'crea',
    NewAEEventHandlerUPP(AECreateMessageHandler), 0, false);
  AEInstallEventHandler('euMS', 'eRep',
    NewAEEventHandlerUPP(AEReplyMessageHandler), 0, false);
  AEInstallEventHandler('euMS', 'eFwd',
    NewAEEventHandlerUPP(AEForwardMessageHandler), 0, false);
  AEInstallEventHandler('euMS', 'eRdr',
    NewAEEventHandlerUPP(AERedirectMessageHandler), 0, false);
  AEInstallEventHandler('euMS', 'eQue',
    NewAEEventHandlerUPP(AEQueueMessageHandler), 0, false);
  AEInstallEventHandler('euMS', 'move',
    NewAEEventHandlerUPP(AEMoveMessageHandler), 0, false);

  return 0;
}

void ScriptingShutdown(void)
{
  /* Filter handlers */
  AERemoveEventHandler('euFi', 'crea', NULL, false);
  AERemoveEventHandler('euFi', 'delo', NULL, false);
  AERemoveEventHandler('euFi', 'getd', NULL, false);
  AERemoveEventHandler('euFi', 'cntf', NULL, false);

  /* Personality handlers */
  AERemoveEventHandler('euPe', 'cntP', NULL, false);
  AERemoveEventHandler('euPe', 'crea', NULL, false);
  AERemoveEventHandler('euPe', 'getd', NULL, false);
  AERemoveEventHandler('euPe', 'setd', NULL, false);
  AERemoveEventHandler('euPe', 'delo', NULL, false);

  /* Mail transfer handlers */
  AERemoveEventHandler('euMl', 'chck', NULL, false);

  /* Message handlers */
  AERemoveEventHandler('euMS', 'cntM', NULL, false);
  AERemoveEventHandler('euMS', 'getd', NULL, false);
  AERemoveEventHandler('euMS', 'setd', NULL, false);
  AERemoveEventHandler('euMS', 'crea', NULL, false);
  AERemoveEventHandler('euMS', 'eRep', NULL, false);
  AERemoveEventHandler('euMS', 'eFwd', NULL, false);
  AERemoveEventHandler('euMS', 'eRdr', NULL, false);
  AERemoveEventHandler('euMS', 'eQue', NULL, false);
  AERemoveEventHandler('euMS', 'move', NULL, false);
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
