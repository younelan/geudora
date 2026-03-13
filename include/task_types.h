#ifndef TASK_TYPES_H
#define TASK_TYPES_H

#ifndef TASK_KIND_ENUM_DEFINED
#define TASK_KIND_ENUM_DEFINED
typedef enum {
  UndefinedTask = 0,
  CheckingTask,        // pop check mail
  SendingTask,         // smtp send mail
  IMAPResyncTask,      // imap resync
  IMAPFetchingTask,    // imap fetch
  IMAPDeleteTask,      // imap delete
  IMAPUndeleteTask,    // imap undelete
  IMAPTransferTask,    // imap transfer task
  IMAPExpungeTask,     // imap expunge
  IMAPMailboxList,     // imap list command - not currently threaded.
  IMAPAttachmentFetch, // imap attachment fetch
  IMAPSearchTask,      // imap search
  IMAPAppendTask,      // imap append
  IMAPMultResyncTask,  // imap multiple resync
  IMAPMultExpungeTask, // imap multuple expunge
  IMAPUploadTask,      // POP->IMAP message transfer
  IMAPPollingTask,     // imap polling task
  IMAPFilterTask,      // imap filtering of incoming messages
  IMAPAlertTask = 1000 // imap ALERT recevied
} TaskKindEnum;
#endif

/* Forward declarations for threading types used in progress visibility */
typedef struct threadData_ *threadDataHandle;

#endif
