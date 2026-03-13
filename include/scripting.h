/* scripting.h — Platform-independent scripting abstraction for Eudora
 *
 * This header defines the scripting API that external automation systems
 * can call to manipulate Eudora objects (filters, mailboxes, messages).
 *
 * Platform backends implement the transport layer:
 *   - macOS: Apple Events (scripting_ae.c)
 *   - Linux: D-Bus (scripting_dbus.c)
 *   - Windows: COM automation (scripting_com.c)
 *
 * The CRUD operations below are platform-neutral. Each backend translates
 * incoming scripting requests into calls to these functions, and formats
 * return values back into the platform's scripting protocol.
 *
 * Copyright (c) 2017, Computer History Museum. All rights reserved.
 * See LICENSE for terms.
 */

#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <stdbool.h>

/*======================================================================
 * Scripting property IDs — platform-neutral equivalents of Mac DescType
 *
 * These map 1:1 to the original Apple Events property codes from the
 * Mac Eudora scripting dictionary, but are plain enums so they work
 * everywhere.
 *====================================================================*/
typedef enum {
  /* Filter properties */
  kScriptPropName           = 1,
  kScriptPropId             = 2,
  kScriptPropLastMatch      = 3,
  kScriptPropIncoming       = 4,
  kScriptPropOutgoing       = 5,
  kScriptPropManual         = 6,
  kScriptPropConjunction    = 7,
  kScriptPropTransferMailbox = 8,
  kScriptPropCopyInstead    = 9,

  /* Match term properties */
  kScriptPropTermHeader     = 20,
  kScriptPropTermVerb       = 21,
  kScriptPropTermValue      = 22,

  /* Message properties */
  kScriptPropPriority       = 30,
  kScriptPropStatus         = 31,
  kScriptPropSender         = 32,
  kScriptPropDate           = 33,
  kScriptPropSubject        = 34,
  kScriptPropSize           = 35,
  kScriptPropIsOutgoing     = 36,
  kScriptPropBody           = 37,
  kScriptPropLabel          = 38,
  kScriptPropWrap           = 39,
  kScriptPropKeepCopy       = 40,
  kScriptPropReturnReceipt  = 41,
  kScriptPropSignature      = 42,
} ScriptPropertyID;

/*======================================================================
 * Scripting value — a tagged union for passing data in/out
 *====================================================================*/
typedef enum {
  kScriptValNone,
  kScriptValString,
  kScriptValLong,
  kScriptValBool,
} ScriptValType;

typedef struct {
  ScriptValType type;
  union {
    char    str[256];
    long    num;
    bool    flag;
  } u;
} ScriptValue;

/* Convenience constructors */
static inline ScriptValue ScriptString(const char *s) {
  ScriptValue v;
  v.type = kScriptValString;
  if (s) {
    int i;
    for (i = 0; i < 255 && s[i]; i++) v.u.str[i] = s[i];
    v.u.str[i] = '\0';
  } else {
    v.u.str[0] = '\0';
  }
  return v;
}

static inline ScriptValue ScriptLong(long n) {
  ScriptValue v;
  v.type = kScriptValLong;
  v.u.num = n;
  return v;
}

static inline ScriptValue ScriptBool(bool b) {
  ScriptValue v;
  v.type = kScriptValBool;
  v.u.flag = b;
  return v;
}

/*======================================================================
 * Filter scripting API — platform-neutral CRUD operations
 *
 * These operate on the in-memory filter list (gFilterArray / Filters
 * handle). They call RegenerateFilters / SaveFilters as needed.
 *
 * Return 0 on success, negative on error.
 *====================================================================*/

/* Count total filters */
int ScriptCountFilters(long *count);

/* Check if a filter exists by 1-based index or by unique ID */
bool ScriptFilterExists(long idOrIndex, bool byId);

/* Create a new filter at the given 0-based position (-1 = end).
 * Returns the new filter's unique ID in *outId. */
int ScriptCreateFilter(int position, long *outId);

/* Delete a filter by 1-based index or unique ID */
int ScriptDeleteFilter(long idOrIndex, bool byId);

/* Get a filter property */
int ScriptGetFilterProperty(long idOrIndex, bool byId,
                            ScriptPropertyID prop, ScriptValue *out);

/* Set a filter property */
int ScriptSetFilterProperty(long idOrIndex, bool byId,
                            ScriptPropertyID prop, const ScriptValue *in);

/* Get a match term property (termIndex is 0 or 1) */
int ScriptGetTermProperty(long idOrIndex, bool byId, int termIndex,
                          ScriptPropertyID prop, ScriptValue *out);

/* Set a match term property (termIndex is 0 or 1) */
int ScriptSetTermProperty(long idOrIndex, bool byId, int termIndex,
                          ScriptPropertyID prop, const ScriptValue *in);

/*======================================================================
 * Personality scripting API — platform-neutral CRUD operations
 *
 * These operate on the personality list (PersList).
 * Return 0 on success, negative on error.
 *====================================================================*/

/* Count personalities */
int ScriptCountPersonalities(long *count);

/* Get a personality property by 1-based index or name */
int ScriptGetPersonalityProperty(long index, ScriptPropertyID prop,
                                  ScriptValue *out);

/* Set a personality property by 1-based index */
int ScriptSetPersonalityProperty(long index, ScriptPropertyID prop,
                                  const ScriptValue *in);

/* Create a new personality. Returns new 1-based index in *outIndex. */
int ScriptCreatePersonality(long *outIndex);

/* Delete a personality by 1-based index (cannot delete dominant) */
int ScriptDeletePersonality(long index);

/*======================================================================
 * Mail transfer scripting API
 *
 * Trigger check/send from external scripting.
 *====================================================================*/

/* Trigger a mail check and/or send */
int ScriptCheckMail(bool check, bool send);

/*======================================================================
 * Message scripting API — platform-neutral operations
 *
 * Messages are identified by mailbox path + 0-based index.
 * Return 0 on success, negative on error.
 *====================================================================*/

/* Count messages in a mailbox */
int ScriptCountMessages(const char *mailboxPath, long *count);

/* Get a message property */
int ScriptGetMessageProperty(const char *mailboxPath, long index,
                              ScriptPropertyID prop, ScriptValue *out);

/* Set a message property */
int ScriptSetMessageProperty(const char *mailboxPath, long index,
                              ScriptPropertyID prop, const ScriptValue *in);

/* Create a new outgoing message (compose). Returns index in *outIndex. */
int ScriptCreateMessage(long *outIndex);

/* Reply to a message. replyAll/includeSelf/quoteText are options.
 * Returns the new message's index in *outIndex. */
int ScriptReplyMessage(const char *mailboxPath, long index,
                        bool replyAll, bool includeSelf, bool quoteText,
                        long *outIndex);

/* Forward a message. Returns the new message's index in *outIndex. */
int ScriptForwardMessage(const char *mailboxPath, long index,
                          long *outIndex);

/* Redirect a message. Returns the new message's index in *outIndex. */
int ScriptRedirectMessage(const char *mailboxPath, long index,
                           long *outIndex);

/* Queue a message for sending (message must be in Out mailbox) */
int ScriptQueueMessage(long index);

/* Move/copy a message between mailboxes.
 * If copy is true, copies instead of moving. */
int ScriptMoveMessage(const char *fromMailbox, long index,
                       const char *toMailbox, bool copy);

/*======================================================================
 * Backend lifecycle — called from main() or the application init
 *====================================================================*/

/* Initialize the platform scripting backend (register D-Bus object,
 * install AE handlers, etc.) */
int ScriptingInit(void);

/* Shut down the scripting backend */
void ScriptingShutdown(void);

#endif /* SCRIPTING_H */
