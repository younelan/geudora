/* imap_callbacks.h — Pluggable callback interface for CrispinIMAP
 *
 * CrispinIMAP requires mm_* callback functions. This header defines
 * a callback struct that apps can fill to receive IMAP events.
 *
 * Default implementations are provided in imap_callbacks.c —
 * they collect results into arrays (pull API) or forward to
 * registered callbacks (push API). No Eudora dependency.
 *
 * Usage:
 *   CrispyImapCallbacks cb = {0};
 *   cb.on_log = my_log_func;
 *   cb.on_list = my_list_func;
 *   cb.ctx = my_app_data;
 *   crispy_imap_set_callbacks(&cb);
 *
 *   // Now call mail_list(), mail_fetch_*, etc. — callbacks fire
 */

#ifndef IMAP_CALLBACKS_H
#define IMAP_CALLBACKS_H

#include <stdbool.h>

/* Forward declarations — actual types in mail.h */
struct MAILSTREAM;
struct MESSAGECACHE;
struct NETMBX;
struct MAILSTATUS;

/* --- Callback function types --- */

/* Auth: provide credentials. If NULL, uses crispy_imap_set_credentials(). */
typedef void (*ImapLoginFn)(const char *host, char *user, char *pwd,
                            int trial, void *ctx);

/* Logging */
typedef void (*ImapLogFn)(const char *msg, int level, void *ctx);

/* Mailbox list item from LIST/LSUB */
typedef void (*ImapListFn)(const char *mailbox, char delimiter,
                           long attributes, void *ctx);

/* STATUS response */
typedef void (*ImapStatusFn)(const char *mailbox, long messages, long recent,
                             long unseen, unsigned long uidnext,
                             unsigned long uidvalidity, void *ctx);

/* SEARCH result — called once per matching UID */
typedef void (*ImapSearchedFn)(unsigned long uid, void *ctx);

/* EXISTS — message count changed */
typedef void (*ImapExistsFn)(long count, void *ctx);

/* EXPUNGE — message removed */
typedef void (*ImapExpungedFn)(unsigned long msgno, void *ctx);

/* FLAGS — message flags changed (uid, seen, deleted, flagged, answered, draft, recent, size) */
typedef void (*ImapFlagsFn)(unsigned long uid, bool seen, bool deleted,
                            bool flagged, bool answered, bool draft,
                            bool recent, unsigned long size, void *ctx);

/* --- Callback struct --- */

typedef struct CrispyImapCallbacks {
  ImapLoginFn    on_login;
  ImapLogFn      on_log;
  ImapListFn     on_list;
  ImapStatusFn   on_status;
  ImapSearchedFn on_searched;
  ImapExistsFn   on_exists;
  ImapExpungedFn on_expunged;
  ImapFlagsFn    on_flags;
  void *ctx;  /* passed to all callbacks */
} CrispyImapCallbacks;

/* --- API --- */

/* Register callbacks. Pass NULL to reset to defaults.
 * Can be called multiple times. */
void crispy_imap_set_callbacks(const CrispyImapCallbacks *cb);

/* Set credentials for mm_login (used when on_login is NULL). */
void crispy_imap_set_credentials(const char *user, const char *password);

/* Get last error string (from mm_log with error level). */
const char *crispy_imap_last_error(void);

/* --- Pull API helpers (results collected by default callbacks) --- */

/* Get collected LIST results. Returns count. Caller must free *names.
 * Each entry is: names[i], delimiters[i], attrs[i]. */
int crispy_imap_get_list_results(char ***names, char **delimiters,
                                 long **attrs);

/* Get collected SEARCH results. Returns count. Caller must free *uids. */
int crispy_imap_get_search_results(unsigned long **uids);

/* Clear collected results (call before each new LIST/SEARCH). */
void crispy_imap_clear_results(void);

#endif /* IMAP_CALLBACKS_H */
