/* Copyright (c) 2017, Computer History Museum
All rights reserved. */

#ifndef IMAPNETLIB_H
#define IMAPNETLIB_H

#include <stdbool.h>
#include <stdint.h>

#if defined(__APPLE__) && !defined(BUILDING_MBX_LIB)
#endif

#ifndef Boolean
typedef bool Boolean;
#endif

#ifndef OSErr
typedef int32_t OSErr;
#define OSErr OSErr
#endif

#ifndef noErr
#define noErr 0
#define False 0
#define True 1

#endif

#ifndef Str255
typedef unsigned char Str255[256];
#endif

#ifndef UPtr
typedef unsigned char *UPtr;
#endif

#ifndef Handle
typedef void **Handle;
#define Handle Handle
#endif

struct MailboxNode;
typedef struct MailboxNode **MailboxNodeHandle;

#ifndef TRANSSTREAM_H
#include "TransStream.h"
#endif

typedef struct FSSpec FSSpec, *FSSpecPtr;
typedef struct Personality **PersHandle;
typedef struct MailboxNode **MailboxNodeHandle;

typedef struct UIDNode UIDNode, *UIDNodePtr, **UIDNodeHandle;
struct UIDNode {
  struct UIDNode **next;
  unsigned long uid;
  unsigned int l_seen : 1;
  unsigned int l_deleted : 1;
  unsigned int l_flagged : 1;
  unsigned int l_answered : 1;
  unsigned int l_draft : 1;
  unsigned int l_recent : 1;
  unsigned int l_IsNew : 1; // added
  unsigned int l_sent : 1;
  unsigned int unUsed : 8;    // added
  unsigned int boxIndex : 16; // added
  unsigned long size;
};

typedef unsigned long UIDVALIDITY;
typedef unsigned long IMAPUID;

#define MAXLOGINTRIALS 1

#define TYPETEXT 0
#define TYPEMULTIPART 1
#define TYPEMESSAGE 2
#define TYPEAPPLICATION 3
#define TYPEAUDIO 4
#define TYPEIMAGE 5
#define TYPEVIDEO 6
#define TYPEMODEL 7
#define TYPEOTHER 8
#define TYPEMAX 15

#define ENC7BIT 0
#define ENC8BIT 1
#define ENCBINARY 2
#define ENCBASE64 3
#define ENCQUOTEDPRINTABLE 4
#define ENCOTHER 5
#define ENCMAX 10

#define LATT_NOINFERIORS (long)1
#define LATT_NOSELECT (long)2
#define LATT_MARKED (long)4
#define LATT_UNMARKED (long)8
#define LATT_NOT_IMAP (long)16
#define LATT_JUNK (long)32
#define LATT_HASNOCHILDREN (long)64
#define LATT_TRASH (long)512
#define LATT_ROOT (long)1024

struct IMAPMailboxAttributes;

#define IMAPBODY struct mail_bodystruct
#define MESSAGE struct mail_body_message
#define PARAMETER struct mail_body_parameter
#define PART struct mail_body_part
#define PARTTEXT struct mail_body_text
#define SIZEDTEXT struct mail_text

SIZEDTEXT {
  char *data;
  unsigned long size;
};

PARTTEXT {
  unsigned long offset;
  SIZEDTEXT text;
};

#define STRINGLIST struct string_list
STRINGLIST {
  SIZEDTEXT text;
  STRINGLIST *next;
};

#define ADDRESS struct mail_address
ADDRESS {
  char *personal;
  char *adl;
  char *mailbox;
  char *host;
  char *error;
  ADDRESS *next;
};

#define ENVELOPE struct mail_envelope
ENVELOPE {
  char *remail;
  ADDRESS *return_path;
  char *date;
  ADDRESS *from;
  ADDRESS *sender;
  ADDRESS *reply_to;
  char *subject;
  ADDRESS *to;
  ADDRESS *cc;
  ADDRESS *bcc;
  char *in_reply_to;
  char *message_id;
  char *newsgroups;
  char *followup_to;
  char *references;
};

IMAPBODY {
  unsigned short type;
  unsigned short encoding;
  char *subtype;
  PARAMETER *parameter;
  char *id;
  char *description;
  struct {
    char *type;
    PARAMETER *parameter;
  } disposition;
  STRINGLIST *language;
  PARTTEXT mime;
  PARTTEXT contents;
  union {
    PART *part;
    MESSAGE *msg;
  } nested;
  struct {
    unsigned long lines;
    unsigned long bytes;
  } size;
  char *md5;
};

PARAMETER {
  char *attribute;
  char *value;
  PARAMETER *next;
};

PART {
  IMAPBODY body;
  PART *next;
};

MESSAGE {
  ENVELOPE *env;
  IMAPBODY *body;
  PARTTEXT full;
  STRINGLIST *lines;
  PARTTEXT IMAPheader;
  PARTTEXT text;
};

#define STRINGDRIVER struct string_driver

typedef struct mailstring {
  void *data;
  unsigned long data1;
  unsigned long size;
  char *chunk;
  unsigned long chunksize;
  unsigned long offset;
  char *curpos;
  unsigned long cursize;
  STRINGDRIVER *dtb;
} STRING;

STRINGDRIVER {
  void (*init)(STRING *s, void *data, unsigned long size);
  char (*next)(STRING *s);
  void (*setpos)(STRING *s, unsigned long i);
};

typedef struct {
  Boolean DELETED;
  Boolean SEEN;
  Boolean FLAGGED;
  Boolean ANSWERED;
  Boolean DRAFT;
  Boolean RECENT;
} IMAPFLAGS;

typedef struct {
  IMAPFLAGS *Flags;
  char *InternalDate;
  ENVELOPE *Env;
  IMAPBODY *Body;
} IMAPFULL;

#include "mail.h"

#ifndef MAILSTREAM_DEFINED
#define MAILSTREAM_DEFINED
typedef struct mail_stream MAILSTREAM;
#endif

typedef struct IMAPStreamStruct {
  unsigned long currentMsgNum;
  unsigned long currentUID;
  unsigned long messageCount;
  unsigned long MessageSizeLimit;
  Str255 mailboxName;
  UIDVALIDITY uidvalidity;
  MAILSTREAM *mailStream;
  Str255 pServerName;
  unsigned long portNumber;
  MailboxNodeHandle mbox;
} IMAPStreamStruct, *IMAPStreamPtr;

enum {
  errIMAPOutOfMemory = 0,
  errIMAPNoServer,
  errIMAPNoMailstream,
  errIMAPNoAccount,
  errIMAPNoMailbox,
  errIMAPSelectMailbox,
  errIMAPMailboxNameInvalid,
  errIMAPCreateStream,
  errIMAPStreamIsLocked,
  errIMAPCreateMailbox,
  errIMAPDeleteMailbox,
  errIMAPRenameMailbox,
  errIMAPMoveMailbox,
  errIMAPNotConnected,
  errIMAPNoMessagesSpecified,
  errIMAPDeleteMessage,
  errIMAPUndeleteMessage,
  errIMAPCopyFailed,
  errNotIMAPPers,
  errNotIMAPMailboxErr,
  errIMAPListErr,
  errIMAPListInUse,
  errIMAPNoTrash,
  errIMAPStubFileBad,
  errIMAPCouldNotFetchPart,
  errIMAPBadEncodingErr,
  errIMAPSearchMailboxErr,
  errIMAPMailboxChangedErr,
  errIMAPReadOnlyStreamErr,
  errIMAPCantExpunge,
  errIMAPOneDownloadFailed,
  errIMAPNoJunk,
  errIMAPLastError
};

OSErr NewImapStream(IMAPStreamPtr *imapStream, UPtr ServerName,
                    unsigned long PortNum);
void ZapImapStream(IMAPStreamPtr *imapStream);
bool OpenControlStream(IMAPStreamPtr imapStream);
bool IMAPOpenMailbox(IMAPStreamPtr imapStream, const char *MailboxName,
                     bool readOnly);
bool CreateIMAPMailbox(IMAPStreamPtr imapStream, const char *mailboxName);
bool DeleteIMAPMailbox(IMAPStreamPtr imapStream, const char *mailboxName);
bool RenameIMAPMailbox(IMAPStreamPtr imapStream, const char *oldName,
                       const char *newName);
bool FetchMailboxAttributes(IMAPStreamPtr imapStream, const char *mailboxName);
bool MailboxAttributes(FSSpecPtr spec, struct IMAPMailboxAttributes *att);
void LocateNodeBySpecInAllPersTrees(FSSpecPtr spec, MailboxNodeHandle *node,
                                    PersHandle *pers);
bool FetchMailboxStatus(IMAPStreamPtr imapStream, const char *mailboxName,
                        long flags);
bool IMAPListUnSubscribed(IMAPStreamPtr imapStream, const char *pReference,
                          bool includeMailbox);
void Check(IMAPStreamPtr imapStream);
bool Noop(IMAPStreamPtr imapStream);
bool UIDDeleteMessages(IMAPStreamPtr imapStream, char *pList, bool Expunge);
bool UIDUnDeleteMessages(IMAPStreamPtr imapStream, char *pList);
OSErr UIDExpunge(IMAPStreamPtr imapStream, const char *pUidList);
Boolean Expunge(IMAPStreamPtr imapStream);
Boolean Logout(IMAPStreamPtr imapStream);
bool IsSelected(MAILSTREAM *imapStream);
bool IsConnected(MAILSTREAM *imapStream);
bool IsAuthenticated(MAILSTREAM *imapStream);
unsigned long GetMessageCount(IMAPStreamPtr imapStream);
unsigned long GetSTATUSMessageCount(IMAPStreamPtr imapStream);
bool UIDMarkAsReplied(IMAPStreamPtr imapStream, unsigned long uid);
ENVELOPE *UIDFetchEnvelope(IMAPStreamPtr imapStream, unsigned long uid);
IMAPBODY *UIDFetchStructure(IMAPStreamPtr imapStream, unsigned long uid);
void FreeBodyStructure(IMAPBODY *pBody);
bool UIDFetchFlags(IMAPStreamPtr imapStream, const char *pSequence);
bool FetchAllFlags(IMAPStreamPtr imapStream, UIDNodeHandle *uidList);
bool FetchFlags(IMAPStreamPtr imapStream, const char *sequence,
                UIDNodeHandle *uidList);
unsigned long UIDFetchLastUid(IMAPStreamPtr imapStream);
IMAPFULL *UIDFetchFast(IMAPStreamPtr imapStream, unsigned long uid);
IMAPFULL *UIDFetchAll(IMAPStreamPtr imapStream, unsigned long uid);
IMAPFULL *UIDFetchFull(IMAPStreamPtr imapStream, unsigned long uid);
char *UIDFetchInternalDate(IMAPStreamPtr imapStream, unsigned long uid);
bool UIDFetchHeader(IMAPStreamPtr imapStream, unsigned long uid, bool file);
bool UIDFetchMessage(IMAPStreamPtr imapStream, unsigned long uid, bool Peek);
bool UIDFetchMessageBody(IMAPStreamPtr imapStream, unsigned long uid,
                         bool Peek);
bool UIDFetchPartialMessage(IMAPStreamPtr imapStream, unsigned long uid,
                            unsigned long first, unsigned long nBytes,
                            bool Peek);
Boolean UIDFetchPartialMessageBody(IMAPStreamPtr imapStream, unsigned long uid,
                                   unsigned long first, unsigned long nBytes,
                                   bool Peek);
Boolean UIDFetchPartialBodyText(IMAPStreamPtr imapStream, unsigned long uid,
                                char *section, unsigned long first,
                                unsigned long nBytes, bool Peek, bool file);
unsigned long FetchUID(IMAPStreamPtr imapStream, unsigned long msgNum);
bool UIDFetchRFC822Header(IMAPStreamPtr imapStream, unsigned long uid,
                          char *sequence);
bool UIDFetchRFC822Text(IMAPStreamPtr imapStream, unsigned long uid,
                        char *sequence);
bool UIDFetchRFC822HeaderFields(IMAPStreamPtr imapStream, unsigned long uid,
                                char *sequence, char *Fields);
bool UIDFetchRFC822HeaderFieldsNot(IMAPStreamPtr imapStream, unsigned long uid,
                                   char *sequence, char *fields);
Boolean UIDFetchMimeHeader(IMAPStreamPtr imapStream, unsigned long uid,
                           char *sequence);
Boolean UIDFetchBodyText(IMAPStreamPtr imapStream, unsigned long uid,
                         char *sequence, bool PEEK);
Boolean UIDFetchBodyTextInChunks(IMAPStreamPtr imapStream, unsigned long uid,
                                 char *sequence, bool PEEK, long size);
bool UIDFetchPreamble(IMAPStreamPtr imapStream, unsigned long uid,
                      char *sequence);
bool UIDFetchTrailer(IMAPStreamPtr imapStream, unsigned long uid,
                     char *sequence);
bool UIDSaveFlags(IMAPStreamPtr imapStream, unsigned long uid, char *uidList,
                  IMAPFLAGS *Flags, bool Set, bool Silent);
bool UIDAddFlags(IMAPStreamPtr imapStream, unsigned long uid, IMAPFLAGS *Flags,
                 bool Silent);
bool UIDRemoveFlags(IMAPStreamPtr imapStream, unsigned long uid,
                    IMAPFLAGS *Flags, bool Silent);
bool UIDCopy(IMAPStreamPtr imapStream, char *pUidlist, char *pDestMailbox);
bool IMAPAppendMessage(IMAPStreamPtr imapStream, const char *Flags,
                       long seconds, STRING *pMsg);
bool UIDFetchPartialContentsToBuffer(IMAPStreamPtr imapStream,
                                     unsigned long uid, char *sequence,
                                     int first, unsigned long nBytes,
                                     char *buffer, unsigned long bufferSize,
                                     unsigned long *len);
Boolean UIDMessageIsMultipart(IMAPStreamPtr stream, unsigned long uid);
long UIDGetTime(IMAPStreamPtr stream, IMAPUID uid);
unsigned long GetRfc822Size(IMAPStreamPtr stream, IMAPUID uid);
UIDVALIDITY UIDValidity(IMAPStreamPtr stream);
bool IsReadOnly(MAILSTREAM *stream);
bool UIDFind(IMAPStreamPtr stream, const char *headerList, bool body, bool bNot,
             char *string, unsigned long firstUID, unsigned long lastUID,
             UIDNodeHandle *results);
bool FetchHeader(IMAPStreamPtr stream, unsigned long msgNum);
long FetchMIMEHeader(IMAPStreamPtr stream, unsigned long uid, char *section,
                     unsigned long flags);
void OrderedInsert(MAILSTREAM *mailStream, unsigned long uid, bool seen,
                   bool deleted, bool flagged, bool answered, bool draft,
                   bool recent, bool sent, unsigned long size);

/* Set credentials used by mm_login callback */
void imap_set_credentials(const char *user, const char *password);

#ifdef DEBUG
long LoMemCheck(void);
#endif
#endif // IMAPNETLIB_H
