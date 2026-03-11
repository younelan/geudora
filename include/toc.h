/* Copyright (c) 2017, Computer History Museum
   All rights reserved. */

#ifndef TOC_H
#define TOC_H

#include "mailbox.h"
#include "schizo.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Copyright (c) 1992-1995 by QUALCOMM Incorporated */

/* Legacy state constants */
#define UNREAD 0
#define READ 1
#define SENT 2
#define REPLIED 3
#define TIMED 4
#define MESG_ERR 6
#define REBUILT 100

#ifdef THREADING_ON
#define GetTempInTOC() GetSpecialTOC(IN_TEMP)
#define GetTempOutTOC() GetSpecialTOC(OUT_TEMP)
#define GetRealInTOC() GetSpecialTOC(IN)
#define GetRealOutTOC() GetSpecialTOC(OUT)
#define GetInTOC() (InAThread() ? GetTempInTOC() : GetRealInTOC())
#define GetOutTOC() (InAThread() ? GetTempOutTOC() : GetRealOutTOC())
bool AmTempToc(TOCType * tocH);
#else
#define GetInTOC() GetSpecialTOC(IN)
#define GetOutTOC() GetSpecialTOC(OUT)
#endif
/* Constants for Hash Checking */
#define kNeverHashed 0
#define kNoMessageId 0

/* VirtualMessData — must be no larger than sizeof(Rect) = 8 bytes on 32-bit.
   On 64-bit, long is 8 bytes so the union will be larger. */
typedef struct {
  long linkSerialNum;    /* Serial # link to original message */
  short virtualMBIdx;    /* FSSpec index for virtual mailboxes */
} VirtualMessData;

/*
 * MSumType — matches original Eudora Mac field order exactly.
 * See MAC624/Include/mailbox.h lines 91-134.
 */
typedef struct {
  long offset;              /* byte offset in file */
  long length;              /* length of message, in bytes */
  int bodyOffset;           /* byte where the body begins, relative to offset */
  StateEnum state;          /* current state of the message */
  long spamScore:8;         /* spam score */
  unsigned long spamBecause:3; /* where the spam score came from */
  long spare21:21;          /* spare bits */
  unsigned long arrivalSeconds; /* when the message arrived at this machine */
  void **mesgErrH;          /* mesgErrorHandle — message error handle */
  unsigned long fromHash;   /* hash of the from address */
  unsigned long spare[3];
  long serialNum;           /* unique message serial number */
  unsigned long seconds;    /* the value of seconds represented by the date field */
  unsigned long flags;      /* some binary values */
  union {
    Rect savedPos;          /* saved window position */
    VirtualMessData virtualMess;
  } u;
  Byte priority;            /* display as 1-5, keep as 1-200 */
  Byte origPriority;
  short tableId;            /* resid of xlate table to use */
  short score:4;            /* for the text analysis engine */
  short outType:4;          /* for statistics: forward, reply, redirect */
  short unused:8;           /* take it if you need it */
  short spareShort2;
  short sumRandBytes;       /* bytes for various uses */
  short origZone;           /* message's original timezone */
  unsigned long sigId;      /* fileid of signature; 0 for main sig, 1 for alternate */
  char from[48];            /* from address */
  unsigned long popPersId;  /* personality id it came from */
  unsigned long persId;     /* the personality id */
  long msgIdHash;           /* hash of the message-id */
  short subjId;             /* subject id */
  short spareShort;
  char subj[60];            /* subject */
  unsigned long opts;
  unsigned long uidHash;    /* hash of message id */
  Handle cache;             /* cache of message text */
  bool selected;            /* is it selected? */
  MessHandle messH;         /* message structure (and window) if any */
} MSumType, *MSumPtr;

typedef MSumType MessageSummary; /* Alias used by ported code */

/* TOC (Table of Contents) structure - legacy compatible */
typedef struct TOCType {
  char path[PATH_MAX]; /* Mailbox file path */
  short refN;          /* Ref number */
  short count;         /* Number of messages */
  short which;         /* Mailbox type (IN, OUT, etc) */
  bool dirty;          /* Needs to be written */
  bool durty;          /* Legacy typo compatibility */
  long used;           /* Bytes used */
  long total;          /* Total bytes */
  long updateID;       /* Update ID */
  void **imapTOC;      /* IMAP info */
  struct TOCType *next;

  /* Mailbox specification */
  struct {
    FSSpec spec;
    struct {
      long specListCount;
      FSSpecPtr *specList;
      void *data;   /* search window private data (SearchInfo*) */
      short type;   /* virtual mailbox type (0 = none, kSearchMB = search) */
    } virtualMB;
  } mailbox; /* Mailbox file specification */

  /* Window and UI fields */
  MyWindowPtr win; /* Window displaying this TOC */
  bool drawer;     /* Drawer flag */
  void *drawerWin; /* Drawer window (stub) */

  /* Preview and interaction tracking */
  long previewID;     /* Preview pane ID */
  void *previewPTE;   /* Preview text editor (stub) */
  long lastSameTicks; /* Last same tick count */
  long mouseTicks;    /* Mouse tick count */
  struct {
    short h, v;
  } mouseSpot;     /* Mouse position */
  long userActive; /* User activity flag */

  /* Storage and size tracking */
  long volumeFree;     /* Free space on volume */
  long usedK;          /* KB used */
  long totalK;         /* Total KB */
  bool updateBoxSizes; /* Need to update box sizes */
  long boxSize;        /* Mailbox file size */
  long writeDate;      /* Last write date */

  /* Unread message tracking */
  long unread; /* Number of unread messages */

  /* Thread safety */
  short beingWritten; /* Being written flag */

  /* Internal use */
  long internalUseOnly; /* Internal use only */

  /* IMAP-specific fields */
  MailboxNodeHandle imapMBH; /* IMAP mailbox node handle */

  /* Fields required by buildtoc.c and other legacy modules */
  long sort;                /* Sort order */
  long lastSort;            /* Previous sort order */
  long majorVersion;        /* Major version number */
  long minorVersion;        /* Minor version number */
  long needsCompact;        /* Needs compaction flag */
  struct TOCType *nextTOC; /* Linked list of TOCs */
  long pluginKey;           /* Plugin key */
  long pluginValue;         /* Plugin value */
  short updateError;        /* Update error code */
  long previewLo;           /* Preview low range */
  long previewHi;           /* Preview high range */
  long nextSerialNum;       /* Next serial number to assign */
  long ezOpenSerialNum;     /* Added for message.c compatibility */
  long unreadBase;          /* Unread base count */
  bool reallyDirty;         /* Added for buildtoc.obj */
  long sorts[6];            /* Added for buildtoc.obj (guess size) */
  bool virtualTOC;          /* Is this a virtual/IMAP TOC */
  long imapMessagesWaiting; /* IMAP messages waiting to download */
  bool analScanned;         /* Moodmail analysis scanned flag */
  short resort;             /* Added for message.c */
  long needRedo;            /* Added for message.c */
  bool conConMultiScan;     /* Added for mailbox.c */
  bool listFocus;           /* List has focus (vs preview) */
  bool searchFocus;         /* Search field has focus */
  long maxValid;            /* Max valid summary index */

  MessageSummary sums[1]; /* Variable length array of summaries */
} TOCType;

/* TOCType * is defined in mailbox.h */

/* Virtual mailbox types */
#define kSearchMB 1

/* Mailbox type constants */
#define kDontResort   0
#define kNoSlowResort 1
#define kResortNow    2
#define kResortWhenever 3
#define MBX_IN 1
#define MBX_OUT 2
#define MBX_TRASH 3
#define MBX_JUNK 4
#define MBX_IN_TEMP 11
#define MBX_OUT_TEMP 12

/* Message flags */
#define FLAG_READ 0x0001
#define MSG_FLAG_DELETED 0x0002
#define FLAG_SKIPPED (1 << 24)
#define FLAG_OUT (1 << 5)

/* Message options */
#define OPT_DELETED 0x0001
#define OPT_HTML 0x0002
#define OPT_EMSR_DELETE_REQUESTED 11042
#define OPT_ORPHAN_ATT (1 << 9)
#define OPT_FLOW 0x0004
#define OPT_CHARSET 0x0008

/* Message flags */
#define FLAG_READ 0x0001
#define MSG_FLAG_DELETED 0x0002
#define FLAG_SKIPPED (1 << 24)
#define FLAG_OUT (1 << 5)
#define FLAG_FIXED_WIDTH (1 << 16)
#define FLAG_RICH (1 << 17)
#define FLAG_SHOW_ALL (1 << 30)
/* FLAG_HAS_ATT defined in mailbox.h */

/* Global preferences/settings stubs */
#define UseFlowInExcerpt 0

/* Menu constants */

/* Local preference bit IDs - not the same as global PREF_ enum in prefdefs.h */
#define TOC_PREF_JUNK_MAILBOX 1
#define TOC_PREF_REPORT 2
#define TOC_PREF_LMOS 3
#define DELETE_ID 3

/* EMS Constants */
#define EMSF_JUNK_MAIL_ID 10

/* Feature IDs */
#define featureJunk 1

/* Function prototypes */
void InvalSum(TOCType * tocH, short sumNum);
short FindSumByHash(TOCType * tocH, uint32_t hash);
TOCType * GetSpecialTOC(short nameId);
TOCType * GetRealTOC(TOCType * tocH, short sum, short *realSum);
TOCType * LocateIMAPJunkToc(TOCType * source, bool create, bool open);
FSSpec GetMailboxSpec(TOCType * tocH, short num);
void BoxSetSummarySelected(TOCType * tocH, short sum, bool select);
int GetPrefBit(short prefId, int bit);
int GetPrefBitNoDominant(short prefId, int bit);
#undef PrefIsSet
bool PrefIsSet(short prefId);
void UseFeature(short featureId);
/* AddTSToPOPD is a macro defined in pop.h */
/* AddSpecToList declared in filtrun.h */
void RedateTS(TOCType * tocH, short sum);
void RemIdFromPOPD(uint32_t popdType, short deleteId, uint32_t uidHash);
int MoveSelectedMessagesLo(TOCType * tocH, FSSpecPtr dest, bool a, bool b,
                           bool c, bool d);
int MoveSelectedMessages(TOCType * tocH, FSSpecPtr dest, bool openIt);
int MoveMessageLo(TOCType * tocH, int sumNum, FSSpecPtr dest, bool copy,
                  bool toTemp, bool holdOpen);
/* IMAP-specific functions are declared in imapdownload.h */
PersHandle TOCToPers(TOCType * tocH);
MailboxNodeHandle TOCToMbox(TOCType * tocH);
TOCType * TOCBySpec(FSSpecPtr spec);
bool IMAPFilteringUnderway(void);
MailboxNodeHandle LocateInboxForPers(PersHandle pers);
MailboxNodeHandle GetIMAPJunkMailbox(PersHandle pers, bool create,
                                     bool switchTo);

TOCType * toc_load(const char *path);
void toc_free(TOCType * toc);
MessageSummary *toc_get_summaries(TOCType * toc, int *count);
int toc_write(TOCType * toc);
int toc_save(TOCType * toc);
MessageSummary *toc_get_message(TOCType * toc, uint32_t index);
uint32_t toc_get_message_count(TOCType * toc);
int toc_get_unread_count(TOCType * toc);

void TOCSetDirty(TOCType * tocH, bool dirty);
OSErr TOCDates(FSSpecPtr spec, uLong *box, uLong *res, uLong *file);

#define OPT_INLINE_SIG 0x0100 /* Dummy value */

/* CopySum — real implementation in searchwin.c */
void CopySum(MSumPtr from, MSumPtr to, short idx);

#endif /* TOC_H */
