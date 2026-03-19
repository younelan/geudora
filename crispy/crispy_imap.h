/* crispy_imap.h — IMAP4rev1 client library (RFC 3501)
 * Part of crispy: standalone mail library.
 *
 * Simple API: connect, authenticate, list folders, select, fetch, store,
 * copy, search, expunge. Transport is pluggable (same as SMTP/POP3).
 */

#ifndef CRISPY_IMAP_H
#define CRISPY_IMAP_H

#include <stddef.h>
#include <stdbool.h>
#include "crispy_smtp.h" /* for SmtpTransport, SmtpSecurity, CrispyDebugFn */

/* Reuse same transport */
typedef SmtpTransport ImapTransport;
typedef SmtpSecurity ImapSecurity;

/* --- IMAP response status --- */
typedef enum {
  IMAP_OK = 0,
  IMAP_NO,          /* command failed (server said NO) */
  IMAP_BAD,         /* protocol error (server said BAD) */
  IMAP_BYE,         /* server closing connection */
  IMAP_ERR = -1,    /* transport/parse error */
} ImapStatus;

/* --- Mailbox info (from SELECT/STATUS) --- */
typedef struct ImapMailbox {
  char name[256];
  long exists;             /* number of messages */
  long recent;             /* number of recent messages */
  long unseen;             /* first unseen message number */
  unsigned long uidvalidity;
  unsigned long uidnext;
  bool read_only;
} ImapMailbox;

/* --- Mailbox list entry (from LIST) --- */
typedef struct ImapListEntry {
  char name[256];
  char delimiter;
  bool noselect;
  bool noinferiors;
  bool marked;
  bool has_children;
} ImapListEntry;

/* --- Message flags --- */
typedef struct ImapFlags {
  bool seen;
  bool answered;
  bool flagged;
  bool deleted;
  bool draft;
  bool recent;
} ImapFlags;

/* --- Envelope (parsed headers) --- */
typedef struct ImapEnvelope {
  char from[256];
  char to[512];
  char cc[512];
  char subject[256];
  char date[64];
  char message_id[256];
  char in_reply_to[256];
} ImapEnvelope;

/* --- Fetch result --- */
typedef struct ImapFetchResult {
  unsigned long uid;
  long size;
  ImapFlags flags;
  ImapEnvelope envelope;
  char *headers;     /* raw headers (malloc'd, caller frees) */
  char *body;        /* body text (malloc'd, caller frees) */
  long body_len;
  char *full;        /* full RFC822 message (malloc'd) */
  long full_len;
} ImapFetchResult;

/* --- Session --- */
typedef struct ImapSession {
  ImapTransport tp;
  int tag_counter;
  char last_tag[16];
  char last_reply[1024];
  ImapMailbox selected;    /* currently selected mailbox */
  bool connected;
  bool authenticated;
  bool has_idle;
  bool has_uidplus;
  bool has_move;
  CrispyDebugFn debug;
  void *debug_userdata;
} ImapSession;

/* --- High-level API --- */

void crispy_imap_init(ImapSession *s, ImapTransport tp);

/* Connect, do CAPABILITY, optionally STARTTLS. Returns 0 on success. */
int crispy_imap_connect(ImapSession *s, const char *host, int port,
                        ImapSecurity security);

/* Authenticate with LOGIN. Returns 0 on success. */
int crispy_imap_login(ImapSession *s, const char *user, const char *pass);

/* Close connection (LOGOUT). */
void crispy_imap_close(ImapSession *s);

/* --- Mailbox operations --- */

/* List mailboxes. Returns count, allocates array into *entries.
 * Caller must free the array. */
int crispy_imap_list(ImapSession *s, const char *ref, const char *pattern,
                     ImapListEntry **entries);

/* Select a mailbox. Fills s->selected. Returns 0 on success. */
int crispy_imap_select(ImapSession *s, const char *mailbox);

/* Examine (read-only select). Returns 0 on success. */
int crispy_imap_examine(ImapSession *s, const char *mailbox);

/* Get mailbox status without selecting. Returns 0 on success. */
int crispy_imap_status(ImapSession *s, const char *mailbox,
                       long *messages, long *unseen, unsigned long *uidvalidity);

/* Create a mailbox. Returns 0 on success. */
int crispy_imap_create(ImapSession *s, const char *mailbox);

/* Delete a mailbox. Returns 0 on success. */
int crispy_imap_delete(ImapSession *s, const char *mailbox);

/* Rename a mailbox. Returns 0 on success. */
int crispy_imap_rename(ImapSession *s, const char *from, const char *to);

/* Subscribe/unsubscribe. */
int crispy_imap_subscribe(ImapSession *s, const char *mailbox);
int crispy_imap_unsubscribe(ImapSession *s, const char *mailbox);

/* --- Message operations --- */

/* Fetch UIDs in the mailbox. Returns count, allocates into *uids.
 * Caller must free. */
int crispy_imap_fetch_uids(ImapSession *s, unsigned long **uids);

/* Fetch flags for a set of UIDs.
 * uid_set is IMAP sequence like "1:*" or "100,200,300".
 * Returns count, allocates into *results. Caller must free. */
int crispy_imap_fetch_flags(ImapSession *s, const char *uid_set,
                            unsigned long **uids, ImapFlags **flags);

/* Fetch envelope (headers) for a UID. Returns 0 on success. */
int crispy_imap_fetch_envelope(ImapSession *s, unsigned long uid,
                               ImapEnvelope *env);

/* Fetch headers as raw text for a UID. Returns malloc'd string. */
char *crispy_imap_fetch_headers(ImapSession *s, unsigned long uid);

/* Fetch full RFC822 message for a UID. Returns malloc'd buffer.
 * *outLen receives length. */
char *crispy_imap_fetch_message(ImapSession *s, unsigned long uid,
                                long *outLen);

/* Fetch body text only for a UID. Returns malloc'd buffer. */
char *crispy_imap_fetch_body(ImapSession *s, unsigned long uid,
                             long *outLen);

/* Fetch a specific body section (e.g., "1.2" for an attachment).
 * Returns malloc'd buffer. */
char *crispy_imap_fetch_section(ImapSession *s, unsigned long uid,
                                const char *section, long *outLen);

/* Store flags on messages.
 * action: "+FLAGS" to add, "-FLAGS" to remove, "FLAGS" to replace.
 * uid_set: IMAP UID sequence. */
int crispy_imap_store_flags(ImapSession *s, const char *uid_set,
                            const char *action, const ImapFlags *flags);

/* Copy messages to another mailbox. */
int crispy_imap_copy(ImapSession *s, const char *uid_set,
                     const char *dest_mailbox);

/* Move messages (COPY + mark deleted). */
int crispy_imap_move(ImapSession *s, const char *uid_set,
                     const char *dest_mailbox);

/* Expunge deleted messages. */
int crispy_imap_expunge(ImapSession *s);

/* Append a message to a mailbox. */
int crispy_imap_append(ImapSession *s, const char *mailbox,
                       const ImapFlags *flags, const char *message,
                       long msgLen);

/* Search messages. Returns count, allocates UIDs into *results. */
int crispy_imap_search(ImapSession *s, const char *criteria,
                       unsigned long **results);

/* NOOP (keep-alive, also fetches pending updates). */
int crispy_imap_noop(ImapSession *s);

/* IDLE (wait for server push notifications).
 * timeout_ms: max wait time. Returns when server sends update or timeout. */
int crispy_imap_idle(ImapSession *s, int timeout_ms);

#endif /* CRISPY_IMAP_H */
