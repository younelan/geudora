/* imap_callbacks.c — Default mm_* implementations for standalone CrispinIMAP
 *
 * These provide the required mm_* callbacks that imap4r1.c and mail.c call.
 * Each checks if an app callback is registered and forwards to it.
 * Otherwise, collects results into internal arrays (pull API).
 *
 * No Eudora dependency. Links with libc-client.a.
 */

#include "Include/mail.h"
#include "Include/imap_callbacks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Registered callbacks --- */
static CrispyImapCallbacks g_cb = {0};

/* --- Credentials for mm_login --- */
static char g_user[256] = {0};
static char g_password[256] = {0};

/* --- Last error string --- */
static char g_last_error[1024] = {0};

/* --- Collected LIST results --- */
static char **g_list_names = NULL;
static char *g_list_delims = NULL;
static long *g_list_attrs = NULL;
static int g_list_count = 0;
static int g_list_cap = 0;

/* --- Collected SEARCH results --- */
static unsigned long *g_search_uids = NULL;
static int g_search_count = 0;
static int g_search_cap = 0;

/* --- API --- */

void crispy_imap_set_callbacks(const CrispyImapCallbacks *cb) {
  if (cb)
    g_cb = *cb;
  else
    memset(&g_cb, 0, sizeof(g_cb));
}

void crispy_imap_set_credentials(const char *user, const char *password) {
  if (user) { strncpy(g_user, user, sizeof(g_user) - 1); g_user[sizeof(g_user)-1] = '\0'; }
  else g_user[0] = '\0';
  if (password) { strncpy(g_password, password, sizeof(g_password) - 1); g_password[sizeof(g_password)-1] = '\0'; }
  else g_password[0] = '\0';
}

const char *crispy_imap_last_error(void) {
  return g_last_error;
}

void crispy_imap_clear_results(void) {
  { int i; for (i = 0; i < g_list_count; i++) free(g_list_names[i]); }
  free(g_list_names); g_list_names = NULL;
  free(g_list_delims); g_list_delims = NULL;
  free(g_list_attrs); g_list_attrs = NULL;
  g_list_count = g_list_cap = 0;

  free(g_search_uids); g_search_uids = NULL;
  g_search_count = g_search_cap = 0;
}

int crispy_imap_get_list_results(char ***names, char **delimiters, long **attrs) {
  if (names) *names = g_list_names;
  if (delimiters) *delimiters = g_list_delims;
  if (attrs) *attrs = g_list_attrs;
  int count = g_list_count;
  /* Detach — caller owns the arrays now */
  g_list_names = NULL; g_list_delims = NULL; g_list_attrs = NULL;
  g_list_count = g_list_cap = 0;
  return count;
}

int crispy_imap_get_search_results(unsigned long **uids) {
  if (uids) *uids = g_search_uids;
  int count = g_search_count;
  g_search_uids = NULL;
  g_search_count = g_search_cap = 0;
  return count;
}

/* --- mm_* implementations --- */

void mm_searched(MAILSTREAM *stream, unsigned long number) {
  (void)stream;
  if (g_cb.on_searched) {
    g_cb.on_searched(number, g_cb.ctx);
    return;
  }
  /* Default: collect into array */
  if (g_search_count >= g_search_cap) {
    g_search_cap = g_search_cap ? g_search_cap * 2 : 64;
    g_search_uids = realloc(g_search_uids, g_search_cap * sizeof(unsigned long));
  }
  g_search_uids[g_search_count++] = number;
}

void mm_exists(MAILSTREAM *stream, unsigned long number) {
  (void)stream;
  if (g_cb.on_exists)
    g_cb.on_exists((long)number, g_cb.ctx);
}

void mm_expunged(MAILSTREAM *stream, unsigned long number) {
  (void)stream;
  if (g_cb.on_expunged)
    g_cb.on_expunged(number, g_cb.ctx);
}

void mm_flags(MAILSTREAM *stream, unsigned long number) {
  (void)stream; (void)number;
  /* Flags updated in cache — nothing to forward unless app cares */
}

void mm_elt_flags(MAILSTREAM *stream, MESSAGECACHE *elt) {
  if (!stream || !elt) return;
  if (g_cb.on_flags) {
    g_cb.on_flags(elt->privat.uid,
                  elt->seen, elt->deleted, elt->flagged,
                  elt->answered, elt->draft, elt->recent,
                  elt->rfc822_size, g_cb.ctx);
    return;
  }
  /* Default: use OrderedInsert if available via stream */
  /* For standalone, just store in search results as a generic collector */
}

void mm_notify(MAILSTREAM *stream, char *string, long errflg) {
  (void)stream;
  mm_log(string, errflg);
}

void mm_list(MAILSTREAM *stream, int delimiter, char *mailbox, long attributes) {
  (void)stream;
  if (g_cb.on_list) {
    g_cb.on_list(mailbox, (char)delimiter, attributes, g_cb.ctx);
    return;
  }
  /* Default: collect into arrays */
  if (g_list_count >= g_list_cap) {
    g_list_cap = g_list_cap ? g_list_cap * 2 : 32;
    g_list_names = realloc(g_list_names, g_list_cap * sizeof(char *));
    g_list_delims = realloc(g_list_delims, g_list_cap * sizeof(char));
    g_list_attrs = realloc(g_list_attrs, g_list_cap * sizeof(long));
  }
  g_list_names[g_list_count] = strdup(mailbox ? mailbox : "");
  g_list_delims[g_list_count] = (char)delimiter;
  g_list_attrs[g_list_count] = attributes;
  g_list_count++;
}

void mm_lsub(MAILSTREAM *stream, int delimiter, char *mailbox, long attributes) {
  mm_list(stream, delimiter, mailbox, attributes);
}

void mm_status(MAILSTREAM *stream, char *mailbox, MAILSTATUS *status) {
  if (!status) return;
  if (g_cb.on_status) {
    g_cb.on_status(mailbox,
                   status->messages, status->recent, status->unseen,
                   status->uidnext, status->uidvalidity, g_cb.ctx);
  }
  /* Also update stream if selected */
  if (stream && mailbox) {
    stream->mailboxStatus.flags       = status->flags;
    stream->mailboxStatus.recent      = status->recent;
    stream->mailboxStatus.messages    = status->messages;
    stream->mailboxStatus.unseen      = status->unseen;
    stream->mailboxStatus.uidnext     = status->uidnext;
    stream->mailboxStatus.uidvalidity = status->uidvalidity;
  }
}

void mm_log(char *string, long errflg) {
  if (!string) return;
  if (g_cb.on_log) {
    g_cb.on_log(string, (int)errflg, g_cb.ctx);
    return;
  }
  /* Default: store error, print to stderr */
  if (errflg) {
    strncpy(g_last_error, string, sizeof(g_last_error) - 1);
    g_last_error[sizeof(g_last_error) - 1] = '\0';
    fprintf(stderr, "IMAP ERROR: %s\n", string);
  }
}

void mm_alert(MAILSTREAM *stream, char *string) {
  (void)stream;
  if (string) {
    if (g_cb.on_log)
      g_cb.on_log(string, 1, g_cb.ctx);
    else
      fprintf(stderr, "IMAP ALERT: %s\n", string);
  }
}

void mm_dlog(char *string) {
  if (string) {
    if (g_cb.on_log)
      g_cb.on_log(string, 0, g_cb.ctx);
  }
}

void mm_login(NETMBX *mb, char *user, char *pwd, long trial) {
  if (g_cb.on_login) {
    g_cb.on_login(mb ? mb->host : "", user, pwd, (int)trial, g_cb.ctx);
    return;
  }
  /* Default: use stored credentials */
  if (user) {
    if (g_user[0])
      strncpy(user, g_user, NETMAXUSER - 1);
    else if (mb && mb->user[0])
      strncpy(user, mb->user, NETMAXUSER - 1);
    user[NETMAXUSER - 1] = '\0';
  }
  if (pwd) {
    strncpy(pwd, g_password, 255);
    pwd[255] = '\0';
  }
}

void mm_critical(MAILSTREAM *stream)   { (void)stream; }
void mm_nocritical(MAILSTREAM *stream) { (void)stream; }

long mm_diskerror(MAILSTREAM *stream, long errcode, long serious) {
  (void)stream; (void)errcode; (void)serious;
  return 0;
}

void mm_fatal(char *string) {
  fprintf(stderr, "IMAP FATAL: %s\n", string ? string : "(null)");
}
