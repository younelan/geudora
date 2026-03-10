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
bool AmTempToc(TOCHandle tocH);
#else
#define GetInTOC() GetSpecialTOC(IN)
#define GetOutTOC() GetSpecialTOC(OUT)
#endif
/* Constants for Hash Checking */
#define kNeverHashed 0
#define kNoMessageId 0

/* StateEnum typedef for IMAP compatibility */
// typedef short StateEnum;

/* Forward declaration for legacy handle types */
// typedef struct mstruct **MessHandle;

/* Message summary structure - legacy compatible */
typedef struct {
  long offset;         /* Offset in mailbox file */
  long length;         /* Message length */
  short state;         /* Message state (read, etc) */
  short flags;         /* Message flags */
  short priority;      /* Priority */
  short opts;          /* Options */
  long seconds;        /* Date in seconds */
  short origZone;      /* Original time zone */
  uint32_t uidHash;    /* UIDL hash */
  uint32_t msgIdHash;  /* Message-ID hash */
  uint32_t fromHash;   /* From hash */
  long serialNum;      /* Message Serial Number */
  char from[64];       /* From address (PStr equivalent in size) */
  char subj[64];       /* Subject (PStr equivalent) */
  MessHandle messH;    /* Handle to message structure */
  void **cache;        /* Cached message content */
  long bodyOffset;     /* Added for buildtoc.obj */
  long popPersId;      /* Added for buildtoc.obj */
  long persId;         /* Added for buildtoc.obj */
  short tableId;       /* Added for mailxfer.c */
  long sigId;          /* Added for mailxfer.c */
  short spamScore;     /* Junk mail spam score */
  short spamBecause;   /* Reason for spam score */
  short spareShort;    /* Spare short for temporary flags */
  long arrivalSeconds; /* Message arrival time */
  bool selected;       /* Selection state */
  long subjId;         /* Subject ID for error handling */
  union {
    struct {
      short virtualMBIdx;
    } virtualMess;
    struct {
      long linkSerialNum;
    } linkMess;
  } u;
  void **mesgErrH; /* Message error handle */
} MessageSummary;

typedef MessageSummary *MSumPtr;
typedef MessageSummary MSumType; /* Legacy typedef */

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
  struct TOCType **next;

  /* Mailbox specification */
  struct {
    FSSpec spec;
    struct {
      long specListCount;
      FSSpecPtr *specList;
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
  struct TOCType **nextTOC; /* Linked list of TOCs */
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

  MessageSummary sums[1]; /* Variable length array of summaries */
} TOCType;

/* TOCHandle is defined in mailbox.h */

/* Mailbox type constants */
#define kResortWhenever 1
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
#define FLAG_HAS_ATT (1 << 8) /* Dummy value */

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
void InvalSum(TOCHandle tocH, short sumNum);
short FindSumByHash(TOCHandle tocH, uint32_t hash);
TOCHandle GetSpecialTOC(short nameId);
TOCHandle GetRealTOC(TOCHandle tocH, short sum, short *realSum);
TOCHandle LocateIMAPJunkToc(TOCHandle source, bool create, bool open);
FSSpec GetMailboxSpec(TOCHandle tocH, short num);
void BoxSetSummarySelected(TOCHandle tocH, short sum, bool select);
int GetPrefBit(short prefId, int bit);
int GetPrefBitNoDominant(short prefId, int bit);
#undef PrefIsSet
bool PrefIsSet(short prefId);
void UseFeature(short featureId);
/* AddTSToPOPD is a macro defined in pop.h */
/* AddSpecToList declared in filtrun.h */
void RedateTS(TOCHandle tocH, short sum);
void RemIdFromPOPD(uint32_t popdType, short deleteId, uint32_t uidHash);
int MoveSelectedMessagesLo(TOCHandle tocH, FSSpecPtr dest, bool a, bool b,
                           bool c, bool d);
int MoveSelectedMessages(TOCHandle tocH, FSSpecPtr dest, bool openIt);
int MoveMessageLo(TOCHandle tocH, int sumNum, FSSpecPtr dest, bool copy,
                  bool toTemp, bool holdOpen);
/* IMAP-specific functions are declared in imapdownload.h */
PersHandle TOCToPers(TOCHandle tocH);
MailboxNodeHandle TOCToMbox(TOCHandle tocH);
TOCHandle TOCBySpec(FSSpecPtr spec);
bool IMAPFilteringUnderway(void);
MailboxNodeHandle LocateInboxForPers(PersHandle pers);
MailboxNodeHandle GetIMAPJunkMailbox(PersHandle pers, bool create,
                                     bool switchTo);

TOCHandle toc_load(const char *path);
void toc_free(TOCHandle toc);
MessageSummary *toc_get_summaries(TOCHandle toc, int *count);
int toc_write(TOCHandle toc);
int toc_save(TOCHandle toc);
MessageSummary *toc_get_message(TOCHandle toc, uint32_t index);
uint32_t toc_get_message_count(TOCHandle toc);
int toc_get_unread_count(TOCHandle toc);

void TOCSetDirty(TOCHandle tocH, bool dirty);
OSErr TOCDates(FSSpecPtr spec, uLong *box, uLong *res, uLong *file);

#define OPT_INLINE_SIG 0x0100 /* Dummy value */

/* CopySum: portable summary copy (idx unused in GTK port) */
static inline void CopySum(MSumPtr from, MSumPtr to, short idx) {
  (void)idx;
  if (from && to) *to = *from;
}

#endif /* TOC_H */
