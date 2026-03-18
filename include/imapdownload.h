/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/* Copyright (c) 1998 by QUALCOMM Incorporated */

#ifndef IMAPDOWNLOAD_H
#define IMAPDOWNLOAD_H

/* Forward declarations for structures */
struct PETEInst;
struct IMAPSC;
struct LocalFlagChange;

#include "filters.h"
#include "imapnetlib.h"
#include "mailbox.h"
#include "toc.h"
#include "trans.h"
#include <stdbool.h>

// must have already included imapnetlib.h

/**********************************************************************
 *	imapDownload.c
 *
 *		This file contains the functions that download messages and
 *	message bits from the server.
 **********************************************************************/

/* Type definitions for IMAP operations */

#include "task_types.h"

/* CStr is defined in mydefs.h */

/* BoxCountHandle - handle to list of mailbox specs for search */
#ifndef PETE_H
typedef void *PETEInst;
#endif
typedef void *BoxCountHandle;

// resource where we put IMAP index
#define INDEX_RES_TYPE 'IIND'
#define INDEX_RES_ID 128

// special values of offset field in summary
#define imapNeedToDownload -1

// flags for special header combinations
typedef enum {
  kNone = 0,
  kBody,
  kAnyRecipient,
  kAllHeaders,
  kAnyWhere
} HeaderCombEnum;

// flags to tell empty trash routine what to do
typedef enum {
  kEmptyActiveTrashes = 0,
  kEmptyAutoCheckTrashes,
  kEmptyAllTrashes
} IMAPEmptyTrashEnum;

// UIDNode struct.  Used for lists of UIDs.

// IndexStruct.  Used to locate messages within a temporary IMAP message file
typedef struct IndexStruct IndexStruct, *IndexStructPtr;
struct IndexStruct {
  unsigned long uid;
  long offset;
  long length;
};

// AttachmentStubStruct.  Used to save enough information to fecth an attachment
// later
typedef struct AttachmentStubStruct AttachmentStubStruct, *AttachmentStubPtr,
    *AttachmentStubHandle;
struct AttachmentStubStruct {
  unsigned long persID;    // personality ID of owning personality
  unsigned long uid;       // uid of message
  char section[255];       // section string
  unsigned long sizeBytes; // body size in bytes
  unsigned long sizeLines; // body size in lines
};

// IMAPSCStruct - one for each header/value pair to be searched
typedef struct IMAPSCStruct IMAPSCStruct, *IMAPSCPtr, *IMAPSCHandle;
struct IMAPSCStruct {
  char string[256];                    // String to search for
  HeaderCombEnum headerCombination; // nothing special, body, or all headers
  short headerName;                 // string id of header to search
};

// IMAPSResultStruct - structure returned for each match
typedef struct IMAPSResultStruct IMAPSResultStruct, *IMAPSResultPtr,
    *IMAPSResultHandle;
struct IMAPSResultStruct {
  short box;    // index into BoxCount of match
  long uidHash; // the matching summary
};

// special value indicating all messages in lcoal cache need to be trashed
#define MSUM_DELETE_ALL (void *)(-1)
#define SEARCH_WINDOW (MailboxNodeHandle)(-1)

// DeliveryStruct.  Used to keep track of messages to be delivered to all
// opening IMAP mailboxes
typedef struct DeliveryNode DeliveryNode, *DeliveryNodePtr;
typedef DeliveryNode *DeliveryNodeHandle; /* was **; now direct pointer */
struct DeliveryNode {
  TOCType * toc; // toc to identify this deliverynode

  bool finished; // set to true once finished
  bool aborted;  // set to true by the main thread to abort the download

  MailboxNodeHandle mailbox; // handle to the IMAP mailbox we're updating
  void *ta;                 // stores list of summaries to be (a)dded
  void *td;                 // stores list of summaries to be (d)eleted
  void *tu;                 // stores list of summaries to be (u)pdated
  void *tc;                 // stores list of summaries to be (c)opied

  IMAPSResultHandle results; // stores list of IMAP search results
  short threadCount;         // number of threads adding results to this node

  bool filter; // set this flag if this mailbox needs to have filters run on it

  bool cleanupAttachments; // set this flag to true when updating a mailbox
                           // after a transfer so we go clean up attachments if
                           // we ought to.

  DeliveryNodeHandle next;
};

// UpdateNode.  Used to keep a list of windows waiting to be updated
typedef struct UpdateNode UpdateNode, *UpdateNodePtr;
typedef UpdateNode *UpdateNodeHandle;
struct UpdateNode {
  FSSpec mailboxSpec;       // the mailbox this message lives in
  unsigned long uid;        // uid of the message
  FSSpecHandle attachSpecs; // specs pointing to the attachments that have been
                            // downloaded

  UpdateNodeHandle next;
};

// IMAPAppendStruct.  Contains the information necessary to upload a POP message
// to an IMAP server
typedef struct IMAPAppendStruct IMAPAppendStruct, *IMAPAppendPtr,
    *IMAPAppendHandle;
struct IMAPAppendStruct {
  FSSpec spoolSpec;        // the spooled message
  StateEnum fromState;     // the state of the original message
  unsigned long fromFlags; // the flags from the original message

  bool transferred; // set to true when this message has been successfully
                    // transferred
  long serialNum;   // the serial number of the original POP message
  FSSpec mailbox;   // the POP mailbox this message came from
};

// UIDCopyStruct. Contains source mailbox, old UID, and new UID.  Used for main
// thread copys after a UIDPLUS response
typedef struct UIDCopyStruct UIDCopyStruct, *UIDCopyPtr;
struct UIDCopyStruct {
  FSSpec toSpec;   // destination mailbox
  void *hOldSums; // a copy of the summaries transferred.
  void *hNewUIDs; // list of new UIDs from UIDPLUS response.
  bool copy;       // true if this was a copy
};

// global flag to turn off progress messages
extern bool gFilteringUnderway;

// macros to use for progress
#define CAN_PROGRESS (!(gFilteringUnderway && !InAThread()))
#define PROGRESS_START                                                         \
  if (CAN_PROGRESS)                                                            \
  OpenProgress()
#define PROGRESS_MESSAGE(t, m)                                                 \
  if (CAN_PROGRESS)                                                            \
  ProgressMessage(t, m)
#define PROGRESS_MESSAGER(t, r)                                                \
  if (CAN_PROGRESS)                                                            \
  ProgressMessageR(t, r)
#define PROGRESS_BAR(a, b, c, d, e)                                            \
  if (CAN_PROGRESS)                                                            \
    Progress(a, b, c, d, e);
#define BYTE_PROGRESS(a, b, c)                                                 \
  if (CAN_PROGRESS)                                                            \
    ByteProgress(a, b, c);
#define PROGRESS_END                                                           \
  if (CAN_PROGRESS)                                                            \
  CloseProgress()

// Note: if we're currently filtering, and the main thread makes a PROGRESS
// call, skip it. Otherwise, we're a foreground non-filtering task, or a
// background task that can display progress.

// resynch and related routines
MailboxNodeHandle FetchNewMessages(TOCType * tocH, bool fetchFlags,
                                   bool sameThread, bool filter,
                                   bool isAutoCheck);
MailboxNodeHandle DoFetchNewMessages(char * mailboxSpec, bool fetchFlags,
                                     bool isAutoCheck);
bool IMAPDelivery(TOCType * inToc, void **toAdd, void **toUpdate,
                  void **toDelete, void **toCopy, bool *filter,
                  IMAPSResultHandle *results, MailboxNodeHandle *mbox,
                  bool *checkAttachments);
void IMAPAbortResync(TOCType * toc);
int RsyncCurPersInbox(void);
int IMAPProcessMailboxes(FSSpecHandle mailboxes, TaskKindEnum task);
bool DoIMAPProcessMailboxes(FSSpecHandle mailboxes, TaskKindEnum task);
bool ResyncCurrentIMAPMailbox(void);
bool UpdatableIMAPState(StateEnum state);
int SaveMinimalHeader(MAILSTREAM *stream);

// Message downloading routines
bool DoDownloadMessages(TOCType * toc, GArray *uids, bool attachmentsToo);
int UIDDownloadMessage(TOCType * inToc, unsigned long uid,
                         bool forceForeground, bool attachmentsToo);
int UIDDownloadMessages(TOCType * inToc, GArray *uids, bool forceForeground,
                          bool attachmentsToo);
bool EnsureMsgDownloaded(TOCType * tocH, int sumNum, bool attachmentsToo);
int CacheMessage(TOCType * tocH, short sumNum);

bool IMAPMessagesWaiting(TOCType * tocH, char * spoolSpec);
void IMAPAbortMessageFetch(TOCType * tocH, short sumNum);
void IMAPRemoveCachedContents(TOCType * tocH, short sumNum);
void IMAPRemoveSelectedCachedContents(TOCType * tocH);
void IMAPFetchSelectedMessages(TOCType * tocH, bool attach);
int IMAPTransferLocalCache(TOCType * fromTocH, MSumPtr pOrigSum,
                             TOCType * toTocH, long newUid, bool copy);

// mailbox polling
void IMAPPoll(PersHandle pers);
void IMAPPollMailboxes(MailboxNodeHandle tree);
void IMAPPollMailboxTree(IMAPStreamPtr imapStream, MailboxNodeHandle tree,
                         long numToPoll, long *remaining);

// Attachment downloading routines
bool IsIMAPAttachmentStub(char * spec);
unsigned long DownloadIMAPAttachment(char * spec, MailboxNodeHandle mailbox,
                                     bool forceForeground);
MailboxNodeHandle PETEHandleToMailboxNode(PETEHandle pte);
unsigned long DoDownloadIMAPAttachments(FSSpecHandle attachments,
                                        MailboxNodeHandle mailbox);
bool CanFetchAttachment(char * spec);
void UpdateIMAPWindows(void);
bool FetchAllIMAPAttachments(TOCType * toc, short sumNum, bool forceForeground);
bool FetchAllIMAPAttachmentsBySpec(char * spec, MailboxNodeHandle mailbox,
                                   bool forceForeground);
bool HasStubFileAttachment(TOCType * tocH, short sumNum);
int FetchIMAPAttachment(PETEHandle pte, char * spec, bool forceForeground);
void RedisplayIMAPMessage(MyWindowPtr win);

// functions to determine the state of a given message in an IMAP mailbox
bool IMAPMessageDownloaded(TOCType * t, short s);
bool IMAPMessageBeingDownloaded(TOCType * t, short s);

// Message transfer
int IMAPTransferMessage(TOCType * fromTocH, TOCType * toTocH,
                          unsigned long uid, bool copy, bool forceForeground);
int IMAPTransferMessages(TOCType * fromTocH, TOCType * toTocH, GArray *uids,
                           bool copy, bool forceForeground);
int DoTransferMessages(TOCType * fromTocH, TOCType * toTocH, GArray *uids,
                         bool copy);
int IMAPTransferMessagesToServer(TOCType * fromTocH, TOCType * toTocH,
                                   void *sumNums, bool copy,
                                   bool forceForeground);
int IMAPTransferMessageToServer(TOCType * tocH, TOCType * toTocH,
                                  short sumNum, bool copy,
                                  bool forceForeground);
int DoTransferMessagesToServer(TOCType * toTocH, IMAPAppendHandle spoolData,
                                 bool copy, bool forceForeground);
void CleanUpAttachmentsAfterIMAPTransfer(TOCType * tocH, short sumNum);
char *FlagsString(char **flags, bool seen, bool deleted, bool flagged,
                  bool answered, bool draft, bool recent, bool sent);
void UpdatePOPMailboxes(void);
int IMAPTransferMessagesFromSearchWindow(TOCType * fromTocH, TOCType * toTocH,
                                           bool copy);
int IMAPMoveIMAPMessages(TOCType * fromTocH, TOCType * toTocH, bool copy);

// Message deletion
bool IMAPDeleteMessage(TOCType * tocH, unsigned long uid, bool nuke,
                       bool expunge, bool undelete);
bool IMAPDeleteMessages(TOCType * tocH, GArray *uids, bool nuke, bool expunge,
                        bool undelete, bool forceForeground);
bool DoDeleteMessage(TOCType * tocH, GArray *uids, bool nuke, bool expunge,
                     bool undelete);
bool IMAPRemoveDeletedMessages(TOCType * tocH);
bool DoExpungeMailbox(TOCType * tocH);
bool DoExpungeMailboxLo(TOCType * tocH, bool bCheckLocal);
bool IMAPEmptyPersTrash(void);
bool EmptyImapTrashes(IMAPEmptyTrashEnum which);
bool IMAPMarkMessageAsDeleted(TOCType * tocH, unsigned long uid, bool undelete);
bool IMAPDeleteMessageDuringFiltering(TOCType * tocH, PersHandle pers,
                                      unsigned long uid);
bool IMAPDeleteMessagesFromSearchWindow(TOCType * tocH);

// offline commands
int PerformQueuedCommands(PersHandle pers, IMAPStreamPtr imapStream,
                            bool progress);
void ExecuteAllPendingIMAPCommands(void);
int QueueMessFlagChange(TOCType * tocH, short sumNum, StateEnum state,
                          bool bTrashed);
bool PendingMessFlagChange(unsigned long uid, MailboxNodeHandle mbox);

// some ordered UID list functions
void UID_LL_OrderedInsert(UIDNodeHandle *head, UIDNodeHandle *item, bool isNew);
void UID_LL_Zap(UIDNodeHandle *list);

// Error reporting.
int IMAPError(short operation, short explanation, int err);
void IMAPAlert(IMAPStreamPtr stream, TaskKindEnum taskKind);
void IMAPWarnings(void);
void IMAPSpamWatchSupported(bool bSupported, bool bWarnIfNeeded);
void IMAPResetSpamSupportPrefs(void);

// Searching
bool IMAPSearch(TOCType * searchWin, BoxCountHandle boxesToSearch,
                IMAPSCHandle searchCriteria, bool matchAll);
bool DoIMAPServerSearch(TOCType * searchWin, BoxCountHandle specs,
                        void *specsToSeach, IMAPSCHandle searchCriteria,
                        bool matchAll, long firstUID);
void IMAPAbortSearch(TOCType * searchWin);
bool IMAPSearchMailbox(TOCType * searchWin, BoxCountHandle boxesToSearch,
                       MailboxNodeHandle boxToSearch,
                       IMAPSCHandle searchCriteria, bool matchAlltocH,
                       long firstUid);
void UpdateIncrementalIMAPSearches(void);
void IMAPProccessBoxesMainThread(bool bResync, bool bExpunge, bool bSearch);
#define IMAPUpdateIncrementalSearches()                                        \
  IMAPProccessBoxesMainThread(false, false, true)

// Filtering
bool IMAPStartFiltering(TOCType * tocToFilter, bool connect);
void IMAPStartManualFiltering(void);
bool IMAPTermMatch(MTPtr mt, MSumPtr sum);
void IMAPStopFiltering(bool reallyDone);
#define IMAPPostFilterResync() IMAPProccessBoxesMainThread(true, false, false)
#define IMAPPostFilterExpunge() IMAPProccessBoxesMainThread(false, true, false)
bool IMAPFilteringUnderway(void);
void FlagForResync(TOCType * tocH);
void FlagForExpunge(TOCType * tocH);
void IMAPTocHBusy(TOCType * tocH, bool busy);
void ResyncOpenMailboxes(PersHandle pers);
void ResetFilterFlags(TOCType * tocH);
void *IMAPFetchMessageHeadersForFiltering(TOCType * tocH, short sumNum);
void GetNextWaitingIMAPToc(TOCType * *toc);
int IMAPMoveMessageDuringFiltering(TOCType * fromTocH, short sumNum,
                                     TOCType * toTocH, bool copy,
                                     FilterPBPtr fpb);
int CacheIMAPMessageForSpamWatch(TOCType * tocH, short sumNum);
int IMAPFilterProgress(TOCType * tocH);
int DoIMAPFilterProgress(void);
void IMAPFilteringCancelled(bool bOverride);

// Miscellaneous utility functions
// Miscellaneous utility functions
DeliveryNodeHandle FindNodeByToc(TOCType * toc);
int pstrincmp(unsigned char * ps, const char *cs, short n);
bool IsIMAPOperationUnderway(TaskKindEnum task);
bool IsIMAPMailboxBusy(TOCType * tocH);
bool IMAPDoQuit(void);

#ifdef DEBUG
// Debugging
void IMAPCollectFlags(void);
#endif

// Anthony Roybal's growing mailbox problem
void MarkAsProcessed(char *spec);
bool HasBeenProcessed(char *spec);

/* SearchUpdateSum — real implementation in searchwin.c */
void SearchUpdateSum(TOCType * toc, short sum, TOCType * newToc,
                     long newSerial, bool b1, bool b2);

/* InvalTocBox declared in boxact.h */

#ifndef STATMNG_H /* statmng.c provides the real UpdateNumStatWithTime */
static inline void UpdateNumStatWithTime(int stat, long count,
                                         long time) { (void)stat; (void)count; (void)time; }
#endif

void IMAPSearchIncremental(MailboxNodeHandle mbox);
int AddTaskErrorsS(const char *error, const char *explanation,
                     TaskKindEnum taskKind, long persId);

#endif // IMAPDOWNLOAD_H