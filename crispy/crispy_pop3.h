/* pop3.h — POP3 client library (RFC 1939)
 * Part of maillib: standalone, no Eudora/GTK dependency.
 *
 * Simple API: connect, authenticate, list/fetch/delete messages, close.
 * Transport is pluggable — same SmtpTransport struct works here.
 */

#ifndef CRISPY_POP3_H
#define CRISPY_POP3_H

#include <stddef.h>
#include <stdbool.h>

/* Reuse the same transport abstraction */
#include "crispy_smtp.h" /* for SmtpTransport */

/* POP3 transport is the same interface */
typedef SmtpTransport Pop3Transport;

/* --- Security mode --- */
typedef enum {
  POP3_PLAIN    = 0,
  POP3_STLS     = 1,  /* upgrade via STLS */
  POP3_SSL      = 2,  /* implicit TLS (POP3S, port 995) */
} Pop3Security;

/* --- Message info from LIST/UIDL --- */
typedef struct Pop3MsgInfo {
  int number;         /* 1-based message number */
  long size;          /* message size in bytes */
  char uidl[128];     /* unique ID (from UIDL command) */
} Pop3MsgInfo;

/* --- Session --- */
/* Debug callback: called with "C: ..." for sent commands, "S: ..." for replies */
typedef void (*CrispyDebugFn)(const char *line, void *userdata);

typedef struct Pop3Session {
  Pop3Transport tp;
  int last_err;
  char last_reply[512];
  char greeting[512];   /* server greeting (for APOP) */
  bool connected;
  bool authenticated;
  int msg_count;        /* from STAT */
  long mailbox_size;    /* from STAT */
  CrispyDebugFn debug;
  void *debug_userdata;
} Pop3Session;

/* --- High-level API --- */

void crispy_pop3_init(Pop3Session *s, Pop3Transport tp);

/* Connect and optionally upgrade to TLS. Returns 0 on success. */
int crispy_pop3_connect(Pop3Session *s, const char *host, int port,
                 Pop3Security security);

/* Authenticate with USER/PASS. Returns 0 on success. */
int crispy_pop3_auth(Pop3Session *s, const char *user, const char *pass);

/* Authenticate with APOP (uses greeting timestamp). Returns 0 on success. */
int crispy_pop3_auth_apop(Pop3Session *s, const char *user, const char *pass);

/* Get message count and mailbox size (STAT). Returns 0 on success.
 * Results stored in s->msg_count and s->mailbox_size. */
int crispy_pop3_stat(Pop3Session *s);

/* Get info for all messages (LIST + UIDL).
 * Allocates array into *msgs (caller must free). Returns count, or -1. */
int crispy_pop3_list(Pop3Session *s, Pop3MsgInfo **msgs);

/* Retrieve message by number (1-based). Allocates into *out.
 * Returns message length, or -1 on error. Caller must free *out. */
long crispy_pop3_retr(Pop3Session *s, int msgNum, char **out);

/* Retrieve just headers + first N lines (TOP). Allocates into *out.
 * Returns length, or -1. Caller must free *out. */
long crispy_pop3_top(Pop3Session *s, int msgNum, int lines, char **out);

/* Mark message for deletion (DELE). Returns 0 on success. */
int crispy_pop3_dele(Pop3Session *s, int msgNum);

/* Unmark all deletions (RSET). Returns 0 on success. */
int crispy_pop3_rset(Pop3Session *s);

/* Close connection (QUIT — commits deletions). */
void crispy_pop3_close(Pop3Session *s);

/* --- Low-level API --- */

/* Send a POP3 command and read response.
 * Returns 0 for +OK, -1 for -ERR. */
int crispy_pop3_command(Pop3Session *s, const char *cmd);

/* Read a multi-line response (dot-terminated).
 * Allocates into *out. Returns total length, or -1. */
long crispy_pop3_read_multiline(Pop3Session *s, char **out);

#endif /* CRISPY_POP3_H */
