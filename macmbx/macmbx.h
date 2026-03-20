/* macmbx.h — Eudora mbox + TOC mailbox storage library
 *
 * Binary-compatible with Eudora's .toc format:
 *   76-byte TOCDiskHeader + 224-byte MSumDisk per message.
 *
 * Standalone, portable mailbox management:
 *   - Unix mbox format read/write/append
 *   - Binary TOC (Table of Contents) — Eudora-compatible on disk
 *   - Build TOC from mbox scan (parse "From " lines + headers)
 *   - Validate and repair corrupt TOCs
 *   - Compact (remove deleted messages, rewrite mbox + TOC atomically)
 *   - Transfer messages between mailboxes
 *   - Create/delete/rename mailboxes
 *   - File locking for concurrent access
 *   - Search and sort
 *
 * No GLib, no GTK, no Eudora dependency. Pure C99 + POSIX.
 */

#ifndef MACMBX_H
#define MACMBX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#ifdef _WIN32
  #ifndef PATH_MAX
    #define PATH_MAX 260
  #endif
#endif

/* ================================================================
 * Message state (matches Eudora StateEnum)
 * ================================================================ */

typedef enum {
  MACMBX_UNREAD     = 0,
  MACMBX_READ       = 1,
  MACMBX_REPLIED    = 2,
  MACMBX_FORWARDED  = 3,
  MACMBX_REDIRECTED = 4,
  MACMBX_QUEUED     = 5,
  MACMBX_SENT       = 6,
  MACMBX_UNSENT     = 7,
  MACMBX_TIMED      = 8,
  MACMBX_SENDABLE   = 9,
  MACMBX_REBUILT    = 10,
} MacmbxState;

/* ================================================================
 * Special mailbox types (matches Eudora which field)
 * ================================================================ */

typedef enum {
  MACMBX_TYPE_NORMAL   = 0,
  MACMBX_TYPE_IN       = 1,
  MACMBX_TYPE_OUT      = 2,
  MACMBX_TYPE_TRASH    = 3,
  MACMBX_TYPE_JUNK     = 4,
  MACMBX_TYPE_IN_TEMP  = 11,
  MACMBX_TYPE_OUT_TEMP = 12,
} MacmbxType;

/* ================================================================
 * Message flags (bitfield — matches Eudora flags field)
 * ================================================================ */

#define MACMBX_FLAG_DELETED    (1U << 0)
#define MACMBX_FLAG_READ       (1U << 1)
#define MACMBX_FLAG_ANSWERED   (1U << 2)
#define MACMBX_FLAG_FLAGGED    (1U << 3)
#define MACMBX_FLAG_DRAFT      (1U << 4)
#define MACMBX_FLAG_ATTACHMENT (1U << 5)
#define MACMBX_FLAG_HTML       (1U << 6)
#define MACMBX_FLAG_BULK       (1U << 7)
#define MACMBX_FLAG_UTF8       (1U << 8)
#define MACMBX_FLAG_KEEP_COPY  (1U << 9)
#define MACMBX_FLAG_RR         (1U << 10)
#define MACMBX_FLAG_CAN_ENC    (1U << 11)
#define MACMBX_FLAG_LABEL_MASK (7U << 12)
#define MACMBX_FLAG_LABEL_SHIFT 12
#define MACMBX_FLAG_BX_TEXT    (1U << 15)
#define MACMBX_FLAG_WRAP_OUT   (1U << 16)

/* Message options (opts field) */
#define MACMBX_OPT_DELETED     (1U << 0)
#define MACMBX_OPT_HTML        (1U << 1)
#define MACMBX_OPT_FLOW        (1U << 2)
#define MACMBX_OPT_CHARSET     (1U << 3)
#define MACMBX_OPT_ATT_DEL     (1U << 6)
#define MACMBX_OPT_BULK        (1U << 4)
#define MACMBX_OPT_ORPHAN_ATT  (1U << 9)

/* ================================================================
 * On-disk TOC format — binary compatible with Eudora
 * TOCDiskHeader: 76 bytes, MSumDisk: 224 bytes
 * ================================================================ */

#define MACMBX_TOC_MAJOR  1
#define MACMBX_TOC_MINOR  9

#ifdef _MSC_VER
  #pragma pack(push, 1)
  #define MACMBX_PACKED
#else
  #define MACMBX_PACKED __attribute__((packed))
#endif

typedef struct MACMBX_PACKED {
  int32_t majorVersion;
  int32_t minorVersion;
  int16_t count;
  int16_t which;
  int32_t boxSize;
  int32_t writeDate;
  int32_t nextSerialNum;
  int32_t sort;
  int32_t lastSort;
  int32_t pluginKey;
  int32_t pluginValue;
  int32_t previewHi;
  int32_t unreadBase;
  int32_t sorts[6];
  int32_t needsCompact;
} MacmbxDiskHeader;  /* 76 bytes */

typedef struct MACMBX_PACKED {
  int32_t  offset;
  int32_t  length;
  int32_t  bodyOffset;
  int32_t  state;
  int32_t  spamBits;         /* spamScore:8 + spamBecause:3 + spare:21 */
  uint32_t arrivalSeconds;
  uint32_t mesgErrH;         /* always 0 on disk */
  uint32_t fromHash;
  uint32_t spare[3];
  int32_t  serialNum;
  uint32_t seconds;
  uint32_t flags;
  int16_t  savedPos[4];      /* window position rect */
  uint8_t  priority;
  uint8_t  origPriority;
  int16_t  tableId;
  int16_t  scoreBits;        /* score:4 + outType:4 + unused:8 */
  int16_t  spareShort2;
  int16_t  sumRandBytes;
  int16_t  origZone;
  uint32_t sigId;
  char     from[48];
  uint32_t popPersId;
  uint32_t persId;
  int32_t  msgIdHash;
  int16_t  subjId;
  int16_t  spareShort;
  char     subj[60];
  uint32_t opts;
  uint32_t uidHash;
  uint32_t cache;            /* always 0 on disk */
  uint8_t  selected;
  uint8_t  _pad[3];
  uint32_t messH;            /* always 0 on disk */
} MacmbxDiskSum;  /* 224 bytes */

#ifdef _MSC_VER
  #pragma pack(pop)
#endif

#define MACMBX_DISK_HDR_SIZE  76
#define MACMBX_DISK_SUM_SIZE 224
#define MACMBX_DISK_SIZE(count) \
  ((long)MACMBX_DISK_HDR_SIZE + (long)(count) * (long)MACMBX_DISK_SUM_SIZE)

/* ================================================================
 * In-memory message summary (richer than disk format)
 * ================================================================ */

typedef struct {
  long offset;
  long length;
  int  body_offset;
  uint8_t state;              /* MacmbxState */
  uint8_t priority;           /* 1-5 */
  uint8_t origPriority;
  int8_t  spam_score;         /* -1 = unscored */
  uint8_t spam_because;       /* source of spam score */
  uint32_t flags;
  uint32_t opts;
  uint32_t seconds;           /* Date: as UTC seconds */
  uint32_t arrival;           /* arrival time */
  uint32_t from_hash;
  int32_t  msg_id_hash;
  uint32_t uid_hash;
  long serial_num;
  int16_t  orig_zone;         /* timezone in minutes */
  uint32_t sig_id;
  uint32_t pop_pers_id;       /* personality that received */
  uint32_t pers_id;           /* personality that sent */
  int16_t  subj_id;
  int16_t  table_id;
  int16_t  score;             /* text analysis */
  int16_t  out_type;          /* forward/reply/redirect */
  int16_t  saved_pos[4];      /* window position */
  char from[48];
  char subject[60];
  /* Attachment detection */
  bool has_attachment;
  /* Threading (in-memory only, not on disk) */
  int32_t in_reply_to_hash;  /* hash of In-Reply-To Message-ID */
} MacmbxMsgSum;

/* ================================================================
 * In-memory TOC
 * ================================================================ */

typedef struct MacmbxTOC MacmbxTOC;
typedef struct MacmbxStore MacmbxStore;

struct MacmbxTOC {
  char mbox_path[PATH_MAX];
  char toc_path[PATH_MAX];
  int count;
  int capacity;
  long next_serial;
  int16_t which;              /* MacmbxType */
  bool dirty;
  bool being_written;         /* reentrant write protection */
  int lock_fd;                /* file lock descriptor, -1 if unlocked */
  /* Disk header fields preserved for round-trip */
  int32_t major_version;
  int32_t minor_version;
  int32_t box_size;
  int32_t write_date;
  int32_t sort_order;
  int32_t last_sort;
  int32_t plugin_key;
  int32_t plugin_value;
  int32_t preview_hi;
  int32_t unread_base;
  int32_t sorts[6];
  int32_t needs_compact;
  MacmbxMsgSum *msgs;
  /* Registry linkage */
  MacmbxTOC *next;
};

/* ================================================================
 * Event callbacks — optional, register to get notified of changes.
 * All callbacks receive a void *ctx (user data) as last argument.
 * NULL callbacks are silently ignored.
 * ================================================================ */

typedef enum {
  MACMBX_EVENT_NEW_MAIL,        /* message(s) appended to mailbox */
  MACMBX_EVENT_DELETED,         /* message marked deleted */
  MACMBX_EVENT_UNDELETED,       /* message unmarked */
  MACMBX_EVENT_STATE_CHANGED,   /* message state changed (read, replied, etc.) */
  MACMBX_EVENT_FLAGS_CHANGED,   /* message flags changed */
  MACMBX_EVENT_COMPACTED,       /* mailbox compacted, indices shifted */
  MACMBX_EVENT_TOC_SAVED,       /* TOC written to disk */
  MACMBX_EVENT_TOC_REBUILT,     /* TOC rebuilt from mbox scan */
  MACMBX_EVENT_MAILBOX_CREATED, /* new mailbox created (store) */
  MACMBX_EVENT_MAILBOX_DELETED, /* mailbox removed (store) */
  MACMBX_EVENT_MAILBOX_RENAMED, /* mailbox renamed (store) */
  MACMBX_EVENT_MAILBOX_MOVED,   /* mailbox moved to new parent (store) */
  MACMBX_EVENT_FOLDER_CREATED,  /* new folder created (store) */
  MACMBX_EVENT_FOLDER_DELETED,  /* folder removed (store) */
  MACMBX_EVENT_ERROR,           /* error occurred */
  MACMBX_EVENT_STATUS,          /* informational status message */
} MacmbxEventType;

/* Event data — union of possible payloads */
typedef struct {
  MacmbxEventType type;
  MacmbxTOC *toc;               /* which mailbox (NULL for store events) */
  int index;                     /* message index (-1 if N/A) */
  int count;                     /* count (e.g. new messages appended) */
  const char *path;              /* path (for store events) */
  const char *old_path;          /* old path (for rename/move) */
  const char *message;           /* text (for error/status) */
} MacmbxEvent;

/* Callback function type */
typedef void (*MacmbxEventFn)(const MacmbxEvent *event, void *ctx);

/* Register a global event handler. Multiple can be registered.
 * Returns a handle (>= 0) for later unregister, or -1 on error. */
int macmbx_on(MacmbxEventFn fn, void *ctx);

/* Unregister by handle. */
void macmbx_off(int handle);

/* Unregister all handlers. */
void macmbx_off_all(void);

/* ================================================================
 * TOC lifecycle
 * ================================================================ */

MacmbxTOC *macmbx_toc_open(const char *mbox_path);
int         macmbx_toc_save(MacmbxTOC *toc);
void        macmbx_toc_close(MacmbxTOC *toc);
MacmbxTOC *macmbx_toc_build(const char *mbox_path);
MacmbxTOC *macmbx_toc_rebuild(const char *mbox_path, MacmbxTOC *old);
bool        macmbx_toc_valid(MacmbxTOC *toc);

/* Peek: read just the header without loading summaries.
 * Fills count, which, boxSize. Returns 0 on success. */
int macmbx_toc_peek(const char *mbox_path, int *count, int *which,
                     long *box_size);

/* ================================================================
 * TOC validation and repair
 * ================================================================ */

/* Validate TOC integrity: check offsets, lengths, strings.
 * Returns 0 if OK, negative error code if corrupt. */
#define MACMBX_ERR_CORRUPT    -1
#define MACMBX_ERR_MISMATCH   -2
#define MACMBX_ERR_BAD_VERSION -3

int macmbx_toc_validate(MacmbxTOC *toc);

/* Repair a TOC: fix string overflows, zero-length messages,
 * bad offsets, assign serial numbers. Modifies toc in place. */
void macmbx_toc_repair(MacmbxTOC *toc);

/* ================================================================
 * File locking
 * ================================================================ */

/* Lock a mailbox for exclusive access. Returns 0 on success. */
int macmbx_lock(MacmbxTOC *toc);

/* Unlock a previously locked mailbox. */
void macmbx_unlock(MacmbxTOC *toc);

/* ================================================================
 * TOC registry — track open TOCs, prevent double-open
 * ================================================================ */

/* Find an already-open TOC by path. Returns NULL if not open. */
MacmbxTOC *macmbx_registry_find(const char *mbox_path);

/* Flush (save) all dirty TOCs. Returns count saved. */
int macmbx_registry_flush(void);

/* Close all open TOCs. */
void macmbx_registry_close_all(void);

/* ================================================================
 * Message access
 * ================================================================ */

char *macmbx_read_message(MacmbxTOC *toc, int index, long *outLen);
char *macmbx_read_headers(MacmbxTOC *toc, int index);
char *macmbx_read_body(MacmbxTOC *toc, int index, long *outLen);
char *macmbx_read_header_field(MacmbxTOC *toc, int index, const char *field);

/* ================================================================
 * Message modification
 * ================================================================ */

int     macmbx_append_message(MacmbxTOC *toc, const char *message, long len,
                               const char *sender, uint8_t state, uint8_t priority);
int     macmbx_delete_message(MacmbxTOC *toc, int index);
int     macmbx_undelete_message(MacmbxTOC *toc, int index);
int     macmbx_set_state(MacmbxTOC *toc, int index, uint8_t state);
int     macmbx_set_flags(MacmbxTOC *toc, int index, uint32_t flags);
int     macmbx_clear_flags(MacmbxTOC *toc, int index, uint32_t flags);
int     macmbx_set_priority(MacmbxTOC *toc, int index, uint8_t priority);
int     macmbx_set_label(MacmbxTOC *toc, int index, uint8_t label);
uint8_t macmbx_get_label(MacmbxTOC *toc, int index);

/* ================================================================
 * Transfer between mailboxes
 * ================================================================ */

int macmbx_transfer(MacmbxTOC *src, int index, MacmbxTOC *dst, bool copy);
int macmbx_transfer_multi(MacmbxTOC *src, int *indices, int count,
                           MacmbxTOC *dst, bool copy);

/* ================================================================
 * Compact
 * ================================================================ */

int  macmbx_compact(MacmbxTOC *toc);
int  macmbx_count_deleted(MacmbxTOC *toc);
long macmbx_reclaimable(MacmbxTOC *toc);

/* ================================================================
 * Mailbox file operations
 * ================================================================ */

int  macmbx_create(const char *mbox_path);
int  macmbx_remove(const char *mbox_path);
int  macmbx_rename(const char *old_path, const char *new_path);
bool macmbx_is_mbox(const char *path);
int  macmbx_list_mailboxes(const char *dir, char ***names);

/* Detect special mailbox type from filename. */
MacmbxType macmbx_detect_type(const char *mbox_path);

/* ================================================================
 * Mbox utilities
 * ================================================================ */

bool macmbx_is_from_line(const char *line);
void macmbx_write_from_line(char *buf, size_t bufsz, const char *sender);

/* Detect if message has attachments from its headers. */
bool macmbx_detect_attachment(const char *headers);

/* ================================================================
 * Search and sort
 * ================================================================ */

int  macmbx_search(MacmbxTOC *toc, const char *field, const char *pattern,
                    int **results);
int  macmbx_sort(MacmbxTOC *toc, const char *field, bool ascending);

/* ================================================================
 * Statistics
 * ================================================================ */

int  macmbx_count_unread(MacmbxTOC *toc);
int  macmbx_count_flagged(MacmbxTOC *toc);
long macmbx_total_size(MacmbxTOC *toc);

/* ================================================================
 * Message threading
 * ================================================================ */

/* Thread node — represents a message in a thread tree */
typedef struct MacmbxThread MacmbxThread;
struct MacmbxThread {
  int index;                  /* message index in TOC (-1 for dummy root) */
  MacmbxThread *parent;
  MacmbxThread *child;        /* first child */
  MacmbxThread *next;         /* next sibling */
  int depth;                  /* nesting depth (0 = root) */
};

/* Build thread tree for a mailbox.
 * Uses In-Reply-To and msg_id_hash to link parent/child.
 * Returns array of root threads (top-level messages/conversations).
 * Caller must free with macmbx_threads_free(). */
MacmbxThread *macmbx_build_threads(MacmbxTOC *toc);

/* Get a flat ordered list of message indices in threaded order.
 * Walks the thread tree depth-first. Returns count, allocates *indices.
 * Caller frees *indices. */
int macmbx_thread_flatten(MacmbxThread *threads, int **indices, int **depths);

/* Free thread tree. */
void macmbx_threads_free(MacmbxThread *threads);

/* Find the thread containing a message index. Returns root of that thread. */
MacmbxThread *macmbx_thread_find(MacmbxThread *threads, int index);

/* Count conversations (top-level threads). */
int macmbx_thread_count(MacmbxThread *threads);

/* ================================================================
 * Message deduplication
 * ================================================================ */

/* Check if a message already exists in a TOC by Message-ID hash.
 * Returns the index if found, -1 if not a duplicate. */
int macmbx_find_duplicate(MacmbxTOC *toc, int32_t msg_id_hash);

/* Check if a message (raw RFC822) is a duplicate.
 * Extracts Message-ID, hashes it, checks TOC.
 * Returns index of existing message, -1 if not duplicate. */
int macmbx_is_duplicate(MacmbxTOC *toc, const char *message, long len);

/* Append message only if not a duplicate. Returns new index, or
 * existing index (negative) if duplicate. -1 on error. */
int macmbx_append_unique(MacmbxTOC *toc, const char *message, long len,
                          const char *sender, uint8_t state, uint8_t priority);

/* ================================================================
 * Import / Export
 * ================================================================ */

/* Export a single message as .eml file. Returns 0 on success. */
int macmbx_export_eml(MacmbxTOC *toc, int index, const char *eml_path);

/* Export multiple messages to a directory as individual .eml files.
 * Filenames are based on serial number: "00001.eml", "00002.eml", etc. */
int macmbx_export_eml_multi(MacmbxTOC *toc, int *indices, int count,
                              const char *dir_path);

/* Export entire mailbox as .eml files to a directory. */
int macmbx_export_all_eml(MacmbxTOC *toc, const char *dir_path);

/* Import a single .eml file into a mailbox. Returns new message index. */
int macmbx_import_eml(MacmbxTOC *toc, const char *eml_path);

/* Import all .eml files from a directory. Returns count imported. */
int macmbx_import_eml_dir(MacmbxTOC *toc, const char *dir_path);

/* Import from maildir format (cur/, new/, tmp/ structure).
 * Returns count imported. */
int macmbx_import_maildir(MacmbxTOC *toc, const char *maildir_path);

/* Import from another mbox file. Returns count imported.
 * Optionally dedup by Message-ID. */
int macmbx_import_mbox(MacmbxTOC *toc, const char *mbox_path, bool dedup);

/* ================================================================
 * Filters — rule-based message routing
 *
 * Eudora Filters file compatible format.
 * Pure logic: match conditions, produce actions.
 * ================================================================ */

/* Match verb — how to compare */
typedef enum {
  MACMBX_VERB_CONTAINS      = 1,
  MACMBX_VERB_NOT_CONTAINS  = 2,
  MACMBX_VERB_IS            = 3,
  MACMBX_VERB_IS_NOT        = 4,
  MACMBX_VERB_STARTS_WITH   = 5,
  MACMBX_VERB_ENDS_WITH     = 6,
  MACMBX_VERB_APPEARS       = 7,  /* any header contains */
  MACMBX_VERB_NOT_APPEARS   = 8,
  MACMBX_VERB_REGEX         = 13,
  MACMBX_VERB_JUNK_LESS     = 14,
  MACMBX_VERB_JUNK_MORE     = 15,
  /* Date comparison */
  MACMBX_VERB_DATE_BEFORE   = 16,
  MACMBX_VERB_DATE_AFTER    = 17,
  MACMBX_VERB_DATE_IS       = 18,
  /* Priority comparison */
  MACMBX_VERB_PRIORITY_IS   = 19,
  MACMBX_VERB_PRIORITY_ABOVE= 20,  /* higher priority = lower number */
  MACMBX_VERB_PRIORITY_BELOW= 21,
} MacmbxVerb;

/* Conjunction between conditions */
typedef enum {
  MACMBX_CONJ_NONE = 0,     /* single condition */
  MACMBX_CONJ_AND  = 1,     /* both must match */
  MACMBX_CONJ_OR   = 2,     /* either matches */
  MACMBX_CONJ_UNLESS = 3,   /* first AND NOT second */
} MacmbxConjunction;

/* Condition — one match clause */
typedef struct {
  char header[64];           /* header field name, e.g. "From:", "Subject:", "Any:" */
  MacmbxVerb verb;
  char value[256];           /* value to match against */
} MacmbxCondition;

/* Action type */
typedef enum {
  MACMBX_ACT_NONE       = 0,
  MACMBX_ACT_STATUS     = 1, /* set message state */
  MACMBX_ACT_PRIORITY   = 2, /* set priority */
  MACMBX_ACT_LABEL      = 3, /* set label color */
  MACMBX_ACT_SUBJECT    = 4, /* modify subject */
  MACMBX_ACT_SOUND      = 5, /* play sound (name stored, execution via callback) */
  MACMBX_ACT_OPEN       = 6, /* open message */
  MACMBX_ACT_PRINT      = 7, /* print message */
  MACMBX_ACT_FORWARD    = 8, /* forward to address */
  MACMBX_ACT_REDIRECT   = 9, /* redirect to address */
  MACMBX_ACT_REPLY      = 10,/* auto-reply */
  MACMBX_ACT_COPY       = 11,/* copy to mailbox */
  MACMBX_ACT_TRANSFER   = 12,/* move to mailbox */
  MACMBX_ACT_DELETE      = 13,/* mark deleted */
  MACMBX_ACT_JUNK       = 14,/* mark as junk */
  MACMBX_ACT_NOTIFY     = 15,/* notify user */
  MACMBX_ACT_STOP       = 16,/* stop processing further rules */
  MACMBX_ACT_SERVER_DELETE = 17, /* delete from server */
  MACMBX_ACT_SERVER_FETCH  = 18, /* fetch from server */
  MACMBX_ACT_CALLBACK   = 99,/* custom action via callback */
} MacmbxActionType;

/* Action — one operation to perform */
typedef struct {
  MacmbxActionType type;
  int int_value;             /* state, priority, label, junk score */
  char str_value[PATH_MAX];  /* mailbox path, sound name, address, etc. */
} MacmbxAction;

/* Maximum conditions and actions per rule */
#define MACMBX_MAX_CONDITIONS 2
#define MACMBX_MAX_ACTIONS    5

/* When the rule applies */
#define MACMBX_WHEN_INCOMING  (1 << 0)
#define MACMBX_WHEN_OUTGOING  (1 << 1)
#define MACMBX_WHEN_MANUAL    (1 << 2)

/* Filter rule */
typedef struct {
  char name[128];            /* rule display name */
  int id;                    /* unique rule ID */
  uint8_t when;              /* MACMBX_WHEN_* flags */
  MacmbxConjunction conjunction;
  MacmbxCondition conditions[MACMBX_MAX_CONDITIONS];
  int condition_count;
  MacmbxAction actions[MACMBX_MAX_ACTIONS];
  int action_count;
} MacmbxRule;

/* Filter set — collection of rules */
typedef struct {
  MacmbxRule *rules;
  int count;
  int capacity;
  char path[PATH_MAX];       /* file path for save */
} MacmbxFilterSet;

/* Filter result — what happened when applying a rule */
typedef struct {
  bool matched;
  bool stopped;              /* hit a STOP action */
  bool transferred;          /* message was moved */
  bool copied;               /* message was copied */
  bool deleted;              /* message was marked deleted */
  char transfer_dest[PATH_MAX]; /* destination mailbox if transferred */
  int new_state;             /* -1 = unchanged */
  int new_priority;          /* -1 = unchanged */
  int new_label;             /* -1 = unchanged */
} MacmbxFilterResult;

/* Callback for custom actions (sound, open, print, forward, etc.) */
typedef void (*MacmbxFilterActionFn)(MacmbxTOC *toc, int index,
                                      const MacmbxAction *action, void *ctx);

/* --- Filter set lifecycle --- */

MacmbxFilterSet *macmbx_filter_new(void);
void macmbx_filter_free(MacmbxFilterSet *fs);

/* Load from Eudora Filters file. Returns NULL on error. */
MacmbxFilterSet *macmbx_filter_load(const char *path);

/* Save to Eudora Filters file. Returns 0 on success. */
int macmbx_filter_save(MacmbxFilterSet *fs);

/* --- Rule management --- */

/* Add a rule. Returns index. */
int macmbx_filter_add_rule(MacmbxFilterSet *fs, const MacmbxRule *rule);

/* Remove a rule by index. */
int macmbx_filter_remove_rule(MacmbxFilterSet *fs, int index);

/* Move a rule (reorder). */
int macmbx_filter_move_rule(MacmbxFilterSet *fs, int from, int to);

/* Get rule by index. */
MacmbxRule *macmbx_filter_get_rule(MacmbxFilterSet *fs, int index);

/* --- Matching (pure logic, no side effects) --- */

/* Test if a rule matches a message. Returns true if conditions match. */
bool macmbx_filter_match(MacmbxRule *rule, MacmbxTOC *toc, int index);

/* --- Execution --- */

/* Apply all matching rules to a message.
 * store: needed for transfer/copy actions (may be NULL if no transfers).
 * action_fn: callback for custom actions (may be NULL).
 * Returns result struct describing what happened. */
MacmbxFilterResult macmbx_filter_apply(MacmbxFilterSet *fs,
                                        MacmbxTOC *toc, int index,
                                        MacmbxStore *store,
                                        MacmbxFilterActionFn action_fn,
                                        void *action_ctx);

/* Apply filters to all messages in a TOC.
 * when: MACMBX_WHEN_INCOMING, OUTGOING, or MANUAL.
 * no_xfer: if true, skip transfer/copy actions (for preview/test).
 * Returns count of messages matched. */
int macmbx_filter_apply_all(MacmbxFilterSet *fs, MacmbxTOC *toc,
                              uint8_t when, MacmbxStore *store,
                              MacmbxFilterActionFn action_fn,
                              void *action_ctx);

/* Apply filters to selected messages only (by index array).
 * indices must be sorted ascending. */
int macmbx_filter_apply_selected(MacmbxFilterSet *fs, MacmbxTOC *toc,
                                   int *indices, int count,
                                   uint8_t when, bool no_xfer,
                                   MacmbxStore *store,
                                   MacmbxFilterActionFn action_fn,
                                   void *action_ctx);

/* Apply all rules to one message with noXfer option.
 * If no_xfer is true, TRANSFER and COPY actions are skipped. */
MacmbxFilterResult macmbx_filter_apply_ex(MacmbxFilterSet *fs,
                                           MacmbxTOC *toc, int index,
                                           bool no_xfer,
                                           MacmbxStore *store,
                                           MacmbxFilterActionFn action_fn,
                                           void *action_ctx);

/* ================================================================
 * Junk mail management
 *
 * Core junk logic: score, mark, move, archive, rescan.
 * Actual scoring is pluggable via callback — register your own
 * Bayesian classifier, rspamd client, or any scoring engine.
 * ================================================================ */

/* Spam score sources (matches Eudora spamBecause field) */
#define MACMBX_JUNK_BECAUSE_NOT_JUNK  0
#define MACMBX_JUNK_BECAUSE_PLUG      1  /* scored by plugin/classifier */
#define MACMBX_JUNK_BECAUSE_USER      2  /* manually marked by user */
#define MACMBX_JUNK_BECAUSE_XFER      3  /* moved to junk mailbox */
#define MACMBX_JUNK_BECAUSE_WHITELIST 4  /* whitelisted sender */

/* Score action — what the caller asked for */
typedef enum {
  MACMBX_SCORE_AUTO,         /* automatic scoring on receive */
  MACMBX_SCORE_RESCAN,       /* rescan existing message */
  MACMBX_SCORE_USER_JUNK,    /* user marked as junk (train) */
  MACMBX_SCORE_USER_NOT_JUNK,/* user marked as not-junk (train) */
} MacmbxScoreAction;

/* Scoring callback — called to get a spam score for a message.
 * toc + index identify the message. headers and body are provided
 * for convenience (may be NULL if not available, scorer should
 * read from toc in that case).
 * Returns score 0-100 (0=ham, 100=spam), or -1 to skip. */
typedef int (*MacmbxJunkScoreFn)(MacmbxTOC *toc, int index,
                                  const char *headers, const char *body,
                                  MacmbxScoreAction action, void *ctx);

/* Whitelist callback — returns true if sender should never be junk.
 * from: the From field from the summary. */
typedef bool (*MacmbxWhitelistFn)(const char *from, void *ctx);

/* Junk configuration */
typedef struct {
  int threshold;              /* score >= threshold is junk (default 50) */
  int archive_days;           /* delete junk older than N days (0=never) */
  int archive_threshold;      /* only archive if score >= this (default 50) */
  bool believe_date;          /* trust Date: header on junk (default false) */
  bool server_delete;         /* delete from server when junked */
  MacmbxJunkScoreFn score_fn; /* scoring callback (NULL = no scoring) */
  void *score_ctx;
  MacmbxWhitelistFn whitelist_fn; /* whitelist callback (NULL = no whitelist) */
  void *whitelist_ctx;
} MacmbxJunkConfig;

/* Initialize junk config with defaults. */
void macmbx_junk_config_init(MacmbxJunkConfig *cfg);

/* Register a scoring callback. */
void macmbx_junk_set_scorer(MacmbxJunkConfig *cfg,
                             MacmbxJunkScoreFn fn, void *ctx);

/* Register a whitelist callback. */
void macmbx_junk_set_whitelist(MacmbxJunkConfig *cfg,
                                MacmbxWhitelistFn fn, void *ctx);

/* --- Scoring --- */

/* Score a single message. Updates spam_score and spam_because.
 * Returns the score (0-100) or -1 if not scored. */
int macmbx_junk_score(MacmbxJunkConfig *cfg, MacmbxTOC *toc, int index);

/* Score all unscored messages in a mailbox.
 * Returns count scored. */
int macmbx_junk_score_box(MacmbxJunkConfig *cfg, MacmbxTOC *toc);

/* Rescore all messages (even previously scored). */
int macmbx_junk_rescore_box(MacmbxJunkConfig *cfg, MacmbxTOC *toc);

/* --- Mark as junk / not-junk --- */

/* Mark a message as junk. Sets score, optionally trains scorer.
 * If store is non-NULL and junk mailbox exists, moves message there. */
int macmbx_junk_mark(MacmbxJunkConfig *cfg, MacmbxTOC *toc, int index,
                      bool is_junk, MacmbxStore *store);

/* Set spam score and source directly. */
int macmbx_junk_set_score(MacmbxTOC *toc, int index,
                           int8_t score, uint8_t because);

/* --- Move to junk --- */

/* Move all messages above threshold to junk mailbox.
 * Returns count moved. */
int macmbx_junk_move_spam(MacmbxJunkConfig *cfg, MacmbxTOC *toc,
                            MacmbxStore *store);

/* --- Archive (delete old junk) --- */

/* Delete junk messages older than cfg->archive_days.
 * toc should be the Junk mailbox.
 * Returns count deleted. */
int macmbx_junk_archive(MacmbxJunkConfig *cfg, MacmbxTOC *toc);

/* --- Utilities --- */

/* Check if a TOC is the Junk mailbox. */
bool macmbx_is_junk_mailbox(MacmbxTOC *toc);

/* Cleanse junk TOC on load (set scores for unscored messages). */
void macmbx_junk_toc_cleanse(MacmbxJunkConfig *cfg, MacmbxTOC *toc);

/* ================================================================
 * Nicknames / Address Book
 *
 * Eudora nickname file compatible format:
 *   alias nickname addr1,addr2,...
 *   note nickname <first:John><last:Doe><note:text>
 *
 * Multiple address book files in a directory.
 * ================================================================ */

/* Note field (tagged key-value pair) */
typedef struct MacmbxNoteField {
  char key[64];
  char *value;                /* malloc'd */
  struct MacmbxNoteField *next;
} MacmbxNoteField;

/* Single nickname entry */
typedef struct {
  char name[256];             /* nickname (short name) */
  char *addresses;            /* comma-separated address list, malloc'd */
  MacmbxNoteField *notes;     /* linked list of note fields */
  uint32_t name_hash;         /* hash for fast lookup */
  uint32_t addr_hash;         /* hash of first address for whitelist */
  bool deleted;
  bool dirty;
} MacmbxNickname;

/* Address book (one file) */
typedef struct {
  char name[256];             /* display name (filename) */
  char path[PATH_MAX];        /* full path */
  MacmbxNickname *entries;
  int count;
  int capacity;
  bool dirty;
} MacmbxAddressBook;

/* Address book collection (all books in Nicknames/ folder) */
typedef struct {
  char dir_path[PATH_MAX];    /* Nicknames directory */
  MacmbxAddressBook *books;
  int count;
  int capacity;
} MacmbxAddressBooks;

/* --- Lifecycle --- */

/* Open all address books from a Nicknames directory.
 * Creates directory if needed. */
MacmbxAddressBooks *macmbx_nick_open(const char *nick_dir);

/* Close and free all address books. Does NOT save. */
void macmbx_nick_close(MacmbxAddressBooks *abs);

/* Save all dirty address books. Returns 0 on success. */
int macmbx_nick_save(MacmbxAddressBooks *abs);

/* Save a single address book. */
int macmbx_nick_save_book(MacmbxAddressBook *book);

/* --- Address book management --- */

/* Create a new empty address book. Returns pointer. */
MacmbxAddressBook *macmbx_nick_create_book(MacmbxAddressBooks *abs,
                                             const char *name);

/* Remove an address book (deletes file). */
int macmbx_nick_remove_book(MacmbxAddressBooks *abs, int book_index);

/* Get address book by index. */
MacmbxAddressBook *macmbx_nick_get_book(MacmbxAddressBooks *abs, int index);

/* Find address book by name. */
MacmbxAddressBook *macmbx_nick_find_book(MacmbxAddressBooks *abs,
                                           const char *name);

/* --- Nickname CRUD --- */

/* Add a nickname. Returns index in book. */
int macmbx_nick_add(MacmbxAddressBook *book, const char *name,
                     const char *addresses);

/* Remove a nickname by index. */
int macmbx_nick_remove(MacmbxAddressBook *book, int index);

/* Find nickname by name. Returns index or -1. */
int macmbx_nick_find(MacmbxAddressBook *book, const char *name);

/* Find nickname by name across all books. Returns book index in *book_idx. */
int macmbx_nick_find_all(MacmbxAddressBooks *abs, const char *name,
                           int *book_idx);

/* Get/set addresses. */
const char *macmbx_nick_get_addresses(MacmbxAddressBook *book, int index);
int macmbx_nick_set_addresses(MacmbxAddressBook *book, int index,
                                const char *addresses);

/* Rename a nickname. */
int macmbx_nick_rename(MacmbxAddressBook *book, int index, const char *new_name);

/* --- Note fields (first, last, phone, email, note, picture, etc.) --- */

/* Get a note field value. Returns NULL if not set. Do not free. */
const char *macmbx_nick_get_field(MacmbxAddressBook *book, int index,
                                    const char *key);

/* Set a note field value. Creates field if not exists. */
int macmbx_nick_set_field(MacmbxAddressBook *book, int index,
                            const char *key, const char *value);

/* Remove a note field. */
int macmbx_nick_remove_field(MacmbxAddressBook *book, int index,
                               const char *key);

/* --- Lookup / Search --- */

/* Check if an email address appears in any nickname (any book).
 * Case-insensitive substring match. For whitelist/filter use. */
bool macmbx_nick_contains_address(MacmbxAddressBooks *abs,
                                    const char *email);

/* Check if an address hash exists in any book. Fast lookup. */
bool macmbx_nick_contains_hash(MacmbxAddressBooks *abs, uint32_t addr_hash);

/* Expand a nickname to its addresses. Returns malloc'd string or NULL. */
char *macmbx_nick_expand(MacmbxAddressBooks *abs, const char *name);

/* Search nicknames by name or address pattern.
 * Returns count, allocates *results as array of {book_idx, nick_idx} pairs. */
int macmbx_nick_search(MacmbxAddressBooks *abs, const char *pattern,
                         int **results);

/* --- Import / Export --- */

/* Import from Eudora nickname file into an existing book.
 * Merges: existing nicknames are updated, new ones added. */
int macmbx_nick_import_eudora(MacmbxAddressBook *book, const char *eudora_file);

/* Export a book to Eudora nickname file format. */
int macmbx_nick_export_eudora(MacmbxAddressBook *book, const char *eudora_file);

/* Import from vCard (.vcf) file. Returns count imported. */
int macmbx_nick_import_vcard(MacmbxAddressBook *book, const char *vcf_path);

/* Export a single nickname to vCard format. Returns malloc'd string. */
char *macmbx_nick_export_vcard(MacmbxAddressBook *book, int index);

/* --- Autocomplete --- */

/* Autocomplete result */
typedef struct {
  int book_idx;
  int nick_idx;
  const char *name;          /* nickname (do not free) */
  const char *addresses;     /* address list (do not free) */
  const char *display;       /* "First Last <addr>" or "nick <addr>" (do not free) */
} MacmbxCompleteResult;

/* Autocomplete: find nicknames matching a prefix.
 * Matches against nickname name, first+last name, and email addresses.
 * Results sorted: exact prefix on name first, then first/last, then address.
 * Returns count, allocates *results. Caller frees *results (but not internal pointers).
 * max_results: limit output (0 = unlimited). */
int macmbx_nick_complete(MacmbxAddressBooks *abs, const char *prefix,
                          MacmbxCompleteResult **results, int max_results);

/* --- Utility --- */

/* Hash a string (for address comparison). */
uint32_t macmbx_nick_hash(const char *s);

/* Free note field list. */
void macmbx_nick_free_notes(MacmbxNoteField *notes);

/* ================================================================
 * Signatures — plain text signature blocks
 *
 * Stored as individual text files in a Signatures/ directory.
 * "Standard" and "Alternate" are the default two.
 * ================================================================ */

typedef struct {
  char name[256];
  char path[PATH_MAX];
  char *content;              /* malloc'd text, NULL if not loaded */
  bool dirty;
} MacmbxSignature;

typedef struct {
  char dir_path[PATH_MAX];
  MacmbxSignature *sigs;
  int count;
  int capacity;
} MacmbxSignatures;

/* Lifecycle */
MacmbxSignatures *macmbx_sig_open(const char *sig_dir);
void macmbx_sig_close(MacmbxSignatures *sigs);
int macmbx_sig_save(MacmbxSignatures *sigs);

/* CRUD */
int macmbx_sig_add(MacmbxSignatures *sigs, const char *name, const char *content);
int macmbx_sig_remove(MacmbxSignatures *sigs, int index);
int macmbx_sig_rename(MacmbxSignatures *sigs, int index, const char *new_name);

/* Access */
const char *macmbx_sig_get(MacmbxSignatures *sigs, int index);
int macmbx_sig_set(MacmbxSignatures *sigs, int index, const char *content);
int macmbx_sig_find(MacmbxSignatures *sigs, const char *name);
int macmbx_sig_count(MacmbxSignatures *sigs);

/* Get Standard (index 0) or Alternate (index 1) signature */
const char *macmbx_sig_standard(MacmbxSignatures *sigs);
const char *macmbx_sig_alternate(MacmbxSignatures *sigs);

/* ================================================================
 * Stationery — message templates
 *
 * Stored as individual text files in a Stationery/ directory.
 * Each file contains a complete RFC822 message (headers + body)
 * used as a template for new messages.
 * ================================================================ */

typedef struct {
  char name[256];
  char path[PATH_MAX];
  char *content;              /* malloc'd full message, NULL if not loaded */
  long content_len;
  bool dirty;
} MacmbxStationery;

typedef struct {
  char dir_path[PATH_MAX];
  MacmbxStationery *items;
  int count;
  int capacity;
} MacmbxStationerySet;

/* Lifecycle */
MacmbxStationerySet *macmbx_stat_open(const char *stat_dir);
void macmbx_stat_close(MacmbxStationerySet *ss);
int macmbx_stat_save(MacmbxStationerySet *ss);

/* CRUD */
int macmbx_stat_add(MacmbxStationerySet *ss, const char *name,
                     const char *message, long len);
int macmbx_stat_remove(MacmbxStationerySet *ss, int index);
int macmbx_stat_rename(MacmbxStationerySet *ss, int index, const char *new_name);

/* Access */
const char *macmbx_stat_get(MacmbxStationerySet *ss, int index, long *outLen);
int macmbx_stat_set(MacmbxStationerySet *ss, int index,
                     const char *message, long len);
int macmbx_stat_find(MacmbxStationerySet *ss, const char *name);
int macmbx_stat_count(MacmbxStationerySet *ss);

/* Create a new message from a stationery template.
 * Returns malloc'd RFC822 message with template headers/body. Caller frees. */
char *macmbx_stat_new_message(MacmbxStationerySet *ss, int index, long *outLen);

/* Save current compose message as stationery. */
int macmbx_stat_save_from_message(MacmbxStationerySet *ss, const char *name,
                                    const char *message, long len);

/* ================================================================
 * Emoji — ASCII smiley ↔ Unicode emoji conversion
 * ================================================================ */

typedef struct {
  const char *ascii;          /* ":-)  " */
  const char *emoji;          /* UTF-8 emoji bytes */
  const char *name;           /* "smile" */
} MacmbxEmoji;

/* Get the full emoji table. */
const MacmbxEmoji *macmbx_emoji_table(int *count);

/* Look up ASCII → emoji. Returns NULL if not found. */
const char *macmbx_emoji_lookup(const char *ascii);

/* Look up emoji → ASCII. Returns NULL if not found. */
const char *macmbx_emoji_reverse(const char *emoji);

/* Replace all ASCII smileys with emoji in text.
 * Respects word boundaries (won't mangle URLs or code).
 * Returns malloc'd string. Caller frees. */
char *macmbx_emoji_replace(const char *text);

/* Replace all emoji with ASCII smileys in text.
 * Returns malloc'd string. Caller frees. */
char *macmbx_emoji_strip(const char *text);

/* ================================================================
 * Disk format conversion (for binary compatibility)
 * ================================================================ */

void macmbx_disk_to_sum(const MacmbxDiskSum *disk, MacmbxMsgSum *mem);
void macmbx_sum_to_disk(const MacmbxMsgSum *mem, MacmbxDiskSum *disk);

/* ================================================================
 * MacmbxStore — mailbox directory manager
 *
 * Manages a hierarchy of mailboxes and folders rooted at a base path.
 * Handles enumeration, creation, deletion, renaming, locking, and
 * batch operations across the entire mailbox tree.
 *
 * Structure on disk:
 *   base/
 *     In              (mbox file)
 *     In.toc          (TOC file)
 *     Out
 *     Out.toc
 *     Trash
 *     My Folder/      (subfolder directory)
 *       Work
 *       Work.toc
 *       Personal/     (nested subfolder)
 *         Family
 *         Family.toc
 * ================================================================ */

/* Node type in the mailbox tree */
typedef enum {
  MACMBX_NODE_MAILBOX = 0,
  MACMBX_NODE_FOLDER  = 1,
} MacmbxNodeType;

/* A node in the mailbox tree (mailbox or folder) */
typedef struct MacmbxNode MacmbxNode;
struct MacmbxNode {
  char name[256];             /* display name */
  char path[PATH_MAX];        /* full filesystem path */
  MacmbxNodeType type;
  MacmbxType mbox_type;       /* special type if mailbox (In/Out/Trash/Junk) */
  int unread;                 /* cached unread count (-1 = unknown) */
  int total;                  /* cached message count (-1 = unknown) */
  /* Tree links */
  MacmbxNode *children;       /* first child (for folders) */
  MacmbxNode *next;           /* next sibling */
  MacmbxNode *parent;         /* parent folder */
};

/* The store — root of the mailbox hierarchy */
struct MacmbxStore {
  char base_path[PATH_MAX];   /* root mailbox directory */
  MacmbxNode *root;           /* tree of nodes */
  int lock_fd;                /* store-level lock */
};

/* ----------------------------------------------------------------
 * Store lifecycle
 * ---------------------------------------------------------------- */

/* Open a mailbox store rooted at the given directory.
 * Scans the directory tree and builds the node hierarchy.
 * Creates the directory if it doesn't exist. */
MacmbxStore *macmbx_store_open(const char *base_path);

/* Close the store: flush all dirty TOCs, release locks, free tree. */
void macmbx_store_close(MacmbxStore *store);

/* Rescan the directory tree (after external changes). */
int macmbx_store_refresh(MacmbxStore *store);

/* ----------------------------------------------------------------
 * Tree navigation
 * ---------------------------------------------------------------- */

/* Get the root node list (top-level mailboxes and folders). */
MacmbxNode *macmbx_store_root(MacmbxStore *store);

/* Find a node by path relative to base (e.g. "My Folder/Work").
 * Returns NULL if not found. */
MacmbxNode *macmbx_store_find(MacmbxStore *store, const char *rel_path);

/* Find a node by display name (searches recursively).
 * Returns first match or NULL. */
MacmbxNode *macmbx_store_find_by_name(MacmbxStore *store, const char *name);

/* Find a special mailbox (In, Out, Trash, Junk). */
MacmbxNode *macmbx_store_find_special(MacmbxStore *store, MacmbxType type);

/* Count total mailboxes in the store (recursive). */
int macmbx_store_count_mailboxes(MacmbxStore *store);

/* Count total folders in the store (recursive). */
int macmbx_store_count_folders(MacmbxStore *store);

/* ----------------------------------------------------------------
 * Mailbox operations (through store)
 * ---------------------------------------------------------------- */

/* Create a new mailbox. parent_path is relative (NULL = top level).
 * Returns the new node or NULL on error. */
MacmbxNode *macmbx_store_create_mailbox(MacmbxStore *store,
                                         const char *parent_path,
                                         const char *name);

/* Create a new folder. parent_path is relative (NULL = top level). */
MacmbxNode *macmbx_store_create_folder(MacmbxStore *store,
                                        const char *parent_path,
                                        const char *name);

/* Delete a mailbox or empty folder. Fails if folder has children. */
int macmbx_store_delete(MacmbxStore *store, const char *rel_path);

/* Rename a mailbox or folder. */
int macmbx_store_rename(MacmbxStore *store, const char *rel_path,
                         const char *new_name);

/* Move a mailbox or folder to a new parent.
 * new_parent is relative path (NULL = top level). */
int macmbx_store_move(MacmbxStore *store, const char *rel_path,
                       const char *new_parent);

/* Open a mailbox TOC by relative path.
 * Returns the TOC (registered in global registry). */
MacmbxTOC *macmbx_store_open_mailbox(MacmbxStore *store, const char *rel_path);

/* ----------------------------------------------------------------
 * Batch operations
 * ---------------------------------------------------------------- */

/* Flush all dirty TOCs in the store. Returns count saved. */
int macmbx_store_flush(MacmbxStore *store);

/* Compact all mailboxes that need it. Returns count compacted. */
int macmbx_store_compact_all(MacmbxStore *store);

/* Update unread/total counts for all nodes. */
void macmbx_store_update_counts(MacmbxStore *store);

/* ----------------------------------------------------------------
 * Store locking
 * ---------------------------------------------------------------- */

/* Lock the entire store (exclusive access). */
int macmbx_store_lock(MacmbxStore *store);

/* Unlock the store. */
void macmbx_store_unlock(MacmbxStore *store);

/* ----------------------------------------------------------------
 * Enumeration (flat list for iteration)
 * ---------------------------------------------------------------- */

/* Get a flat list of all mailbox paths. Returns count, allocates *paths.
 * Each path is relative to base. Caller frees each string + array. */
int macmbx_store_list_mailboxes(MacmbxStore *store, char ***paths);

/* Get a flat list of all folder paths. */
int macmbx_store_list_folders(MacmbxStore *store, char ***paths);

/* Free the node tree (internal use). */
void macmbx_node_free(MacmbxNode *node);

#endif /* MACMBX_H */
