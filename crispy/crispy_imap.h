/* crispy_imap.h — IMAP4rev1 client library (RFC 3501)
 * Part of crispy: standalone mail library.
 *
 * Clean pull API: connect, authenticate, list folders, select,
 * fetch, store, copy, search, expunge, idle.
 * Uses crispy_transport for pluggable I/O.
 *
 * Portable: POSIX + Windows. No Eudora dependency.
 */

#ifndef CRISPY_IMAP_H
#define CRISPY_IMAP_H

#include <stddef.h>
#include <stdbool.h>
#include "crispy_smtp.h" /* for SmtpTransport, CrispyDebugFn */

typedef SmtpTransport ImapTransport;

/* --- Security mode --- */
typedef enum {
  IMAP_PLAIN    = 0,
  IMAP_STARTTLS = 1,
  IMAP_SSL      = 2,
} ImapSecurity;

/* --- Response status --- */
typedef enum {
  IMAP_OK  =  0,
  IMAP_NO  = -1,
  IMAP_BAD = -2,
  IMAP_ERR = -3,
} ImapStatus;

/* --- Protocol level (detected from CAPABILITY) --- */
typedef enum {
  IMAP_LEVEL_UNKNOWN  = 0,
  IMAP_LEVEL_IMAP2    = 2,  /* RFC 1176 — very old */
  IMAP_LEVEL_IMAP2BIS = 3,  /* draft — pre-standard */
  IMAP_LEVEL_IMAP4    = 4,  /* RFC 1730 */
  IMAP_LEVEL_IMAP4REV1 = 5, /* RFC 3501 — current standard */
  IMAP_LEVEL_IMAP4REV2 = 6, /* RFC 9051 — newest */
} ImapLevel;

/* --- Body content types (matching MIME) --- */
typedef enum {
  IMAP_TYPE_TEXT         = 0,
  IMAP_TYPE_MULTIPART   = 1,
  IMAP_TYPE_MESSAGE     = 2,
  IMAP_TYPE_APPLICATION = 3,
  IMAP_TYPE_AUDIO       = 4,
  IMAP_TYPE_IMAGE       = 5,
  IMAP_TYPE_VIDEO       = 6,
  IMAP_TYPE_MODEL       = 7,
  IMAP_TYPE_OTHER       = 8,
} ImapBodyType;

/* --- Transfer encodings --- */
typedef enum {
  IMAP_ENC_7BIT             = 0,
  IMAP_ENC_8BIT             = 1,
  IMAP_ENC_BINARY           = 2,
  IMAP_ENC_BASE64           = 3,
  IMAP_ENC_QUOTED_PRINTABLE = 4,
  IMAP_ENC_OTHER            = 5,
} ImapEncoding;

/* --- MIME parameter (linked list) --- */
typedef struct ImapParam {
  char *name;
  char *value;
  struct ImapParam *next;
} ImapParam;

/* --- Content disposition --- */
typedef struct {
  char *type;            /* "inline", "attachment", or NULL */
  ImapParam *params;     /* filename, size, etc. */
} ImapDisposition;

/* --- IMAP address --- */
typedef struct ImapAddress {
  char *name;            /* personal name (display) */
  char *adl;             /* at-domain-list (source route) */
  char *mailbox;         /* local-part */
  char *host;            /* domain */
  struct ImapAddress *next;
} ImapAddress;

/* --- Envelope (parsed from ENVELOPE fetch) --- */
typedef struct {
  char *date;
  char *subject;
  ImapAddress *from;
  ImapAddress *sender;
  ImapAddress *reply_to;
  ImapAddress *to;
  ImapAddress *cc;
  ImapAddress *bcc;
  char *in_reply_to;
  char *message_id;
} ImapEnvelope;

/* --- Body structure (recursive, from BODYSTRUCTURE fetch) --- */
typedef struct ImapBodyPart ImapBodyPart;
struct ImapBodyPart {
  ImapBodyType type;
  ImapEncoding encoding;
  char *subtype;          /* "plain", "html", "mixed", "alternative", etc. */
  ImapParam *params;      /* content-type params (charset, boundary, etc.) */
  char *id;               /* Content-ID */
  char *description;      /* Content-Description */
  char *md5;              /* MD5 checksum */
  ImapDisposition disposition;
  char *language;         /* language tag */
  char *location;         /* Content-Location */
  unsigned long size_bytes;
  unsigned long size_lines;  /* for text and message/rfc822 types */

  /* For multipart: linked list of child parts */
  ImapBodyPart *subparts;
  ImapBodyPart *next;     /* sibling in multipart */

  /* For message/rfc822: nested envelope and body */
  ImapEnvelope *nested_env;
  ImapBodyPart *nested_body;

  /* Section number for FETCH (e.g. "1.2.3") */
  char section[64];
};

/* --- NAMESPACE entry --- */
typedef struct {
  char prefix[256];
  char delimiter;
} ImapNamespaceEntry;

typedef struct {
  ImapNamespaceEntry personal[8];
  int personal_count;
  ImapNamespaceEntry other[8];
  int other_count;
  ImapNamespaceEntry shared[8];
  int shared_count;
} ImapNamespace;

/* --- Mailbox info (from SELECT) --- */
typedef struct {
  char name[256];
  long exists;
  long recent;
  long unseen;             /* first unseen seqno */
  unsigned long uidvalidity;
  unsigned long uidnext;
  bool read_only;
  /* PERMANENTFLAGS as individual bools */
  bool perm_seen;
  bool perm_answered;
  bool perm_flagged;
  bool perm_deleted;
  bool perm_draft;
  bool perm_custom;        /* server allows custom keywords */
} ImapMailboxInfo;

/* --- LIST/LSUB entry --- */
typedef struct {
  char name[256];
  char delimiter;
  bool noselect;
  bool noinferiors;
  bool has_children;
  bool has_no_children;
  bool marked;
  bool unmarked;
} ImapListEntry;

/* --- Message flags --- */
typedef struct {
  bool seen;
  bool answered;
  bool flagged;
  bool deleted;
  bool draft;
  bool recent;
} ImapFlags;

/* --- Fetch result per message --- */
typedef struct {
  unsigned long uid;
  unsigned long seqno;
  unsigned long size;      /* RFC822.SIZE */
  ImapFlags flags;
  char *internaldate;      /* malloc'd INTERNALDATE string, caller frees */
  ImapEnvelope *envelope;  /* parsed ENVELOPE, caller frees */
  ImapBodyPart *bodystructure; /* parsed BODYSTRUCTURE, caller frees */
  char *headers;           /* malloc'd raw headers, caller frees */
  char *body;              /* malloc'd body text, caller frees */
  long body_len;
  char *full;              /* malloc'd full RFC822, caller frees */
  long full_len;
} ImapFetchResult;

/* --- Response code (from tagged reply [CODE]) --- */
typedef enum {
  IMAP_RCODE_NONE       = 0,
  IMAP_RCODE_ALERT      = 1,  /* must display to user */
  IMAP_RCODE_TRYCREATE  = 2,  /* COPY/APPEND failed, create mailbox first */
  IMAP_RCODE_PARSE      = 3,  /* server couldn't parse message headers */
  IMAP_RCODE_READ_ONLY  = 4,
  IMAP_RCODE_READ_WRITE = 5,
  IMAP_RCODE_UIDVALIDITY= 6,
  IMAP_RCODE_UIDNEXT    = 7,
  IMAP_RCODE_UNSEEN     = 8,
  IMAP_RCODE_PERMANENTFLAGS = 9,
  IMAP_RCODE_APPENDUID  = 10,
  IMAP_RCODE_COPYUID    = 11,
  IMAP_RCODE_REFERRAL   = 12, /* server redirect */
  IMAP_RCODE_OTHER      = 99,
} ImapResponseCode;

/* --- Quota entry --- */
typedef struct {
  char root[256];
  unsigned long usage;       /* in KB */
  unsigned long limit;       /* in KB */
} ImapQuota;

/* --- ACL entry --- */
typedef struct {
  char identifier[256];      /* user or group name */
  char rights[64];           /* "lrswipkxtea" etc. */
} ImapACLEntry;

/* --- Cached message entry (for per-session message cache) --- */
typedef struct {
  unsigned long uid;
  unsigned long seqno;       /* current sequence number (updated on EXPUNGE) */
  unsigned long size;        /* RFC822.SIZE */
  ImapFlags flags;
  char *internaldate;        /* malloc'd, NULL if not cached */
  ImapEnvelope *envelope;    /* malloc'd, NULL if not cached */
} ImapCacheEntry;

/* --- Message cache --- */
typedef struct {
  ImapCacheEntry *entries;
  int count;
  int capacity;
} ImapCache;

/* --- Session --- */
typedef struct {
  ImapTransport tp;
  int tag_num;
  char tag[16];
  char reply[4096];          /* last untagged/tagged response text */
  ImapMailboxInfo selected;  /* currently selected mailbox */
  bool connected;
  bool authenticated;
  bool bye_received;         /* server sent * BYE */
  ImapLevel level;           /* detected protocol level */
  ImapResponseCode last_rcode; /* response code from last tagged reply */
  char last_alert[512];      /* last ALERT text (must display to user) */
  char last_referral[512];   /* last REFERRAL URL */
  /* Message cache + seqno↔UID map (per selected mailbox) */
  ImapCache cache;
  /* Auto-reconnect state */
  char reconnect_host[256];
  int reconnect_port;
  ImapSecurity reconnect_security;
  char reconnect_user[256];
  char reconnect_pass[512];
  bool reconnect_enabled;
  /* Capabilities */
  bool cap_starttls;
  bool cap_idle;
  bool cap_uidplus;
  bool cap_move;
  bool cap_literal_plus;
  bool cap_sort;
  bool cap_thread;
  bool cap_namespace;
  bool cap_condstore;
  bool cap_children;
  bool cap_id;
  bool cap_quota;
  bool cap_acl;
  char cap_auth[256];        /* space-separated AUTH mechs */
  /* Debug */
  CrispyDebugFn debug;
  void *debug_userdata;
} ImapSession;

/* ================================================================
 * Memory management — free functions for allocated types
 * ================================================================ */

void crispy_imap_free_params(ImapParam *p);
void crispy_imap_free_disposition(ImapDisposition *d);
void crispy_imap_free_addresses(ImapAddress *a);
void crispy_imap_free_envelope(ImapEnvelope *e);
void crispy_imap_free_bodypart(ImapBodyPart *bp);
void crispy_imap_fetch_result_free(ImapFetchResult *r);

/* ================================================================
 * Modified UTF-7 mailbox name encoding (RFC 3501 section 5.1.3)
 * IMAP requires non-ASCII mailbox names in modified UTF-7.
 * ================================================================ */

/* Encode UTF-8 mailbox name to IMAP modified UTF-7. Caller frees. */
char *crispy_imap_utf8_to_mutf7(const char *utf8);

/* Decode IMAP modified UTF-7 mailbox name to UTF-8. Caller frees. */
char *crispy_imap_mutf7_to_utf8(const char *mutf7);

/* ================================================================
 * Connection
 * ================================================================ */

void crispy_imap_init(ImapSession *s, ImapTransport tp);

int crispy_imap_connect(ImapSession *s, const char *host, int port,
                        ImapSecurity security);

int crispy_imap_login(ImapSession *s, const char *user, const char *pass);

/* SASL authentication mechanisms */
int crispy_imap_auth_plain(ImapSession *s, const char *user, const char *pass);
int crispy_imap_auth_xoauth2(ImapSession *s, const char *user, const char *token);
int crispy_imap_auth_cram_md5(ImapSession *s, const char *user, const char *pass);
int crispy_imap_auth_scram_sha256(ImapSession *s, const char *user, const char *pass);
int crispy_imap_auth_digest_md5(ImapSession *s, const char *user, const char *pass);

void crispy_imap_close(ImapSession *s);

/* ================================================================
 * Mailbox operations
 * ================================================================ */

/* LIST — list mailboxes matching pattern. Allocates *entries, caller frees. */
int crispy_imap_list(ImapSession *s, const char *ref, const char *pattern,
                     ImapListEntry **entries, int *count);

/* LSUB — list subscribed mailboxes. Same API as LIST. */
int crispy_imap_lsub(ImapSession *s, const char *ref, const char *pattern,
                     ImapListEntry **entries, int *count);

int crispy_imap_select(ImapSession *s, const char *mailbox);
int crispy_imap_examine(ImapSession *s, const char *mailbox);

/* STATUS — check mailbox without selecting. Any out-param may be NULL. */
int crispy_imap_status(ImapSession *s, const char *mailbox,
                       long *messages, long *recent, long *unseen,
                       unsigned long *uidvalidity, unsigned long *uidnext);

int crispy_imap_create(ImapSession *s, const char *mailbox);
int crispy_imap_delete(ImapSession *s, const char *mailbox);
int crispy_imap_rename(ImapSession *s, const char *from, const char *to);
int crispy_imap_subscribe(ImapSession *s, const char *mailbox);
int crispy_imap_unsubscribe(ImapSession *s, const char *mailbox);

/* CHECK — request checkpoint of currently selected mailbox. */
int crispy_imap_check(ImapSession *s);

/* CLOSE — close selected mailbox (expunges deleted). */
int crispy_imap_close_mailbox(ImapSession *s);

/* NAMESPACE — discover personal, other users, shared namespaces. */
int crispy_imap_namespace(ImapSession *s, ImapNamespace *ns);

/* ID (RFC 2971) — identify client to server.
 * name/version may be NULL to send NIL. Server ID stored in s->reply. */
int crispy_imap_id(ImapSession *s, const char *name, const char *version);

/* ================================================================
 * QUOTA (RFC 2087)
 * ================================================================ */

/* GETQUOTAROOT — get quota for a mailbox. Returns quota info. */
int crispy_imap_getquotaroot(ImapSession *s, const char *mailbox,
                              ImapQuota *quota);

/* GETQUOTA — get quota for a named root. */
int crispy_imap_getquota(ImapSession *s, const char *root,
                          ImapQuota *quota);

/* SETQUOTA — set quota limit (admin only). */
int crispy_imap_setquota(ImapSession *s, const char *root,
                          unsigned long limit_kb);

/* ================================================================
 * ACL (RFC 4314)
 * ================================================================ */

/* GETACL — get access control list for a mailbox.
 * Allocates *entries, caller frees. */
int crispy_imap_getacl(ImapSession *s, const char *mailbox,
                        ImapACLEntry **entries, int *count);

/* SETACL — set rights for an identifier on a mailbox.
 * rights: "lrswipkxtea" or "+rw" or "-w" */
int crispy_imap_setacl(ImapSession *s, const char *mailbox,
                        const char *identifier, const char *rights);

/* DELETEACL — remove all rights for an identifier. */
int crispy_imap_deleteacl(ImapSession *s, const char *mailbox,
                           const char *identifier);

/* LISTRIGHTS — list rights available to an identifier. */
int crispy_imap_listrights(ImapSession *s, const char *mailbox,
                            const char *identifier,
                            char *rights, size_t rightsSize);

/* MYRIGHTS — get the caller's rights on a mailbox. */
int crispy_imap_myrights(ImapSession *s, const char *mailbox,
                          char *rights, size_t rightsSize);

/* ================================================================
 * Message fetch operations (all UID-based)
 * ================================================================ */

/* Fetch UIDs for all messages. Returns count, allocates *uids. */
int crispy_imap_fetch_uids(ImapSession *s, unsigned long **uids);

/* Fetch flags for UID set (e.g. "1:*"). Allocates arrays. */
int crispy_imap_fetch_flags(ImapSession *s, const char *uid_set,
                            unsigned long **uids, ImapFlags **flags,
                            int *count);

/* Fetch full RFC822 message by UID. Returns malloc'd buffer. */
char *crispy_imap_fetch_message(ImapSession *s, unsigned long uid,
                                long *outLen);

/* Fetch headers only by UID. Returns malloc'd string. */
char *crispy_imap_fetch_headers(ImapSession *s, unsigned long uid);

/* Fetch specific header fields by UID.
 * fields: space-separated, e.g. "From Subject Date Message-ID"
 * Returns malloc'd string containing only requested headers. */
char *crispy_imap_fetch_header_fields(ImapSession *s, unsigned long uid,
                                       const char *fields);

/* Fetch body text only by UID. Returns malloc'd buffer. */
char *crispy_imap_fetch_body(ImapSession *s, unsigned long uid,
                             long *outLen);

/* Fetch a specific MIME section by UID (e.g. "1.2", "2.1.MIME").
 * Returns malloc'd buffer. */
char *crispy_imap_fetch_section(ImapSession *s, unsigned long uid,
                                const char *section, long *outLen);

/* Partial fetch — download byte range of a message.
 * BODY.PEEK[]<offset.count>. Returns malloc'd buffer. */
char *crispy_imap_fetch_partial(ImapSession *s, unsigned long uid,
                                 unsigned long offset, unsigned long count,
                                 long *outLen);

/* Fetch RFC822.SIZE for a UID. Returns size or -1 on error. */
long crispy_imap_fetch_size(ImapSession *s, unsigned long uid);

/* --- Legacy fetch forms (for IMAP2/4 compatibility) --- */

/* RFC822 — fetch full message (marks as \Seen, unlike BODY.PEEK[]).
 * Use crispy_imap_fetch_message() for non-destructive fetch. */
char *crispy_imap_fetch_rfc822(ImapSession *s, unsigned long uid,
                                long *outLen);

/* RFC822.HEADER — fetch headers only (legacy form). */
char *crispy_imap_fetch_rfc822_header(ImapSession *s, unsigned long uid);

/* RFC822.TEXT — fetch body only (legacy form, marks \Seen). */
char *crispy_imap_fetch_rfc822_text(ImapSession *s, unsigned long uid,
                                     long *outLen);

/* BODY (non-STRUCTURE) — simplified structure for IMAP4 (RFC 1730).
 * Returns same tree as BODYSTRUCTURE but without extension data. */
ImapBodyPart *crispy_imap_fetch_body_structure_legacy(ImapSession *s,
                                                       unsigned long uid);

/* Fetch INTERNALDATE for a UID. Returns malloc'd string or NULL. */
char *crispy_imap_fetch_date(ImapSession *s, unsigned long uid);

/* Fetch parsed ENVELOPE for a UID. Returns malloc'd struct, caller frees. */
ImapEnvelope *crispy_imap_fetch_envelope(ImapSession *s, unsigned long uid);

/* Fetch parsed BODYSTRUCTURE for a UID. Returns malloc'd tree, caller frees. */
ImapBodyPart *crispy_imap_fetch_structure(ImapSession *s, unsigned long uid);

/* Fetch overview: UID, FLAGS, SIZE, INTERNALDATE, ENVELOPE for a UID set.
 * Returns count, allocates *results array (caller frees each + array). */
int crispy_imap_fetch_overview(ImapSession *s, const char *uid_set,
                                ImapFetchResult **results, int *count);

/* ================================================================
 * Message modification operations (all UID-based)
 * ================================================================ */

/* Store flags. action: "+FLAGS", "-FLAGS", or "FLAGS".
 * uid_set: IMAP UID sequence like "100" or "1:*". */
int crispy_imap_store_flags(ImapSession *s, const char *uid_set,
                            const char *action, ImapFlags flags);

/* COPY UIDs to another mailbox.
 * If dest_uids is non-NULL and server supports UIDPLUS,
 * returns the destination UIDs (COPYUID response). */
int crispy_imap_copy(ImapSession *s, const char *uid_set,
                     const char *dest);

/* MOVE UIDs (native MOVE or COPY+DELETE+EXPUNGE fallback). */
int crispy_imap_move(ImapSession *s, const char *uid_set,
                     const char *dest);

/* EXPUNGE deleted messages in selected mailbox. */
int crispy_imap_expunge(ImapSession *s);

/* APPEND a message to a mailbox with optional flags and date.
 * date may be NULL. If new_uid is non-NULL and server supports UIDPLUS,
 * returns the UID assigned to the appended message. */
int crispy_imap_append(ImapSession *s, const char *mailbox,
                       ImapFlags flags, const char *date,
                       const char *message, long msgLen);

/* Parse UIDPLUS COPYUID/APPENDUID from last reply.
 * Returns the new UID(s) if available, 0 otherwise. */
unsigned long crispy_imap_last_append_uid(ImapSession *s);
unsigned long crispy_imap_last_copy_uidvalidity(ImapSession *s);

/* ================================================================
 * Search
 * ================================================================ */

/* UID SEARCH. Returns count, allocates *uids (caller frees). */
int crispy_imap_search(ImapSession *s, const char *criteria,
                       unsigned long **uids);

/* SORT (RFC 5256). criteria: sort keys like "DATE", "SUBJECT", "FROM", "SIZE".
 * charset: e.g. "UTF-8". search: search criteria or "ALL".
 * Returns count, allocates *uids (caller frees).
 * Returns -1 if server doesn't support SORT. */
int crispy_imap_sort(ImapSession *s, const char *criteria,
                     const char *charset, const char *search,
                     unsigned long **uids);

/* THREAD (RFC 5256). algorithm: "REFERENCES" or "ORDEREDSUBJECT".
 * charset: e.g. "UTF-8". search: search criteria or "ALL".
 * Returns thread structure as malloc'd string (caller frees).
 * The format is IMAP THREAD response: (uid1 uid2)(uid3 (uid4 uid5))... */
char *crispy_imap_thread(ImapSession *s, const char *algorithm,
                          const char *charset, const char *search);

/* ================================================================
 * Session management
 * ================================================================ */

/* NOOP — keep-alive + get pending updates. */
int crispy_imap_noop(ImapSession *s);

/* IDLE — wait for server push. Returns when server sends update
 * or timeout_ms expires (0 = wait indefinitely). */
int crispy_imap_idle(ImapSession *s, int timeout_ms);

/* ================================================================
 * Low-level
 * ================================================================ */

/* Send raw command, read tagged response. Returns IMAP_OK/NO/BAD. */
int crispy_imap_command(ImapSession *s, const char *cmd);

/* Send raw command, collect all untagged responses.
 * Returns IMAP_OK/NO/BAD. Allocates *lines array (caller frees). */
int crispy_imap_command_collect(ImapSession *s, const char *cmd,
                                 char ***lines, int *lineCount);

/* Read one line from server. Returns malloc'd string (caller frees). */
char *crispy_imap_readline(ImapSession *s);

/* ================================================================
 * Message cache — per-session UID→flags/size/envelope cache.
 * Populated automatically by fetch_overview/fetch_flags.
 * Invalidated on SELECT of a different mailbox.
 * ================================================================ */

/* Look up a cached entry by UID. Returns pointer or NULL. */
ImapCacheEntry *crispy_imap_cache_lookup(ImapSession *s, unsigned long uid);

/* Look up a cached entry by sequence number. Returns pointer or NULL. */
ImapCacheEntry *crispy_imap_cache_lookup_seqno(ImapSession *s, unsigned long seqno);

/* Get UID for a sequence number. Returns 0 if not mapped. */
unsigned long crispy_imap_seqno_to_uid(ImapSession *s, unsigned long seqno);

/* Get sequence number for a UID. Returns 0 if not mapped. */
unsigned long crispy_imap_uid_to_seqno(ImapSession *s, unsigned long uid);

/* Populate cache: fetch UID, FLAGS, SIZE for all messages in selected mailbox.
 * Call after SELECT. Builds the seqno↔UID map. */
int crispy_imap_cache_populate(ImapSession *s);

/* Update flags in cache for a UID (after STORE). */
void crispy_imap_cache_update_flags(ImapSession *s, unsigned long uid,
                                     ImapFlags flags);

/* Invalidate (clear) the cache. Called automatically on SELECT. */
void crispy_imap_cache_clear(ImapSession *s);

/* ================================================================
 * Auto-reconnect — reconnect + re-authenticate + re-select
 * on connection drop. Caller must enable and provide credentials.
 * ================================================================ */

/* Enable auto-reconnect. Stores credentials for re-login.
 * security/host/port are saved from the initial connect call. */
void crispy_imap_enable_reconnect(ImapSession *s,
                                   const char *user, const char *pass);

/* Disable auto-reconnect. */
void crispy_imap_disable_reconnect(ImapSession *s);

/* Attempt reconnect now. Returns IMAP_OK on success.
 * Re-authenticates and re-selects the previously selected mailbox. */
int crispy_imap_reconnect(ImapSession *s);

#endif /* CRISPY_IMAP_H */
