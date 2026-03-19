/* smtp.h — SMTP client library (RFC 5321)
 * Part of maillib: standalone, no Eudora/GTK dependency.
 *
 * Simple API: connect, authenticate, send messages, close.
 * Transport is pluggable — provide your own socket/TLS callbacks,
 * or use the built-in POSIX/OpenSSL implementation.
 */

#ifndef CRISPY_SMTP_H
#define CRISPY_SMTP_H

#include <stddef.h>
#include <stdbool.h>

/* --- Security mode --- */
typedef enum {
  SMTP_PLAIN    = 0,  /* no encryption */
  SMTP_STARTTLS = 1,  /* upgrade via STARTTLS */
  SMTP_SSL      = 2,  /* implicit TLS (SMTPS) */
} SmtpSecurity;

/* --- SMTP command IDs (RFC 5321) --- */
typedef enum {
  SMTP_CMD_NONE = 0,
  SMTP_CMD_HELO,
  SMTP_CMD_EHLO,
  SMTP_CMD_MAIL_FROM,
  SMTP_CMD_RCPT_TO,
  SMTP_CMD_DATA,
  SMTP_CMD_RSET,
  SMTP_CMD_QUIT,
  SMTP_CMD_AUTH,
  SMTP_CMD_STARTTLS,
  SMTP_CMD_NOOP,
  SMTP_CMD_VRFY,
  SMTP_CMD_EXPN,
  SMTP_CMD_HELP,
  SMTP_CMD_COUNT
} SmtpCmd;

const char *crispy_smtp_cmd_str(SmtpCmd cmd);

/* --- SMTP reply codes --- */
#define SMTP_REPLY_CLASS(code)  ((code) / 100)
#define SMTP_IS_OK(code)        (SMTP_REPLY_CLASS(code) == 2)
#define SMTP_IS_TEMP_ERR(code)  (SMTP_REPLY_CLASS(code) == 4)
#define SMTP_IS_PERM_ERR(code)  (SMTP_REPLY_CLASS(code) == 5)

/* --- Transport abstraction --- */
/* Implement these to plug in your own I/O (POSIX, GIO, libcurl, etc.).
 * All return 0 on success, nonzero on error. */
typedef struct SmtpTransport {
  void *ctx;
  int (*connect)(void *ctx, const char *host, int port);
  int (*send)(void *ctx, const char *data, long len);
  int (*recv_line)(void *ctx, char *buf, long bufSize, long *bytesRead);
  int (*start_tls)(void *ctx);
  void (*close)(void *ctx);
  void (*destroy)(void *ctx);  /* free the ctx */
} SmtpTransport;

/* --- EHLO capabilities (parsed from server response) --- */
typedef struct SmtpCaps {
  bool starttls;
  bool eightbitmime;
  bool pipelining;
  bool has_auth;
  long max_size;
  char auth_mechs[256];
} SmtpCaps;

/* --- Session --- */
/* Debug callback — shared between SMTP and POP3 */
#ifndef CRISPY_DEBUG_FN_DEFINED
#define CRISPY_DEBUG_FN_DEFINED
typedef void (*CrispyDebugFn)(const char *line, void *userdata);
#endif

typedef struct SmtpSession {
  SmtpTransport tp;
  SmtpCaps caps;
  int last_code;
  char last_reply[512];
  char local_hostname[256];
  bool connected;
  bool authenticated;
  CrispyDebugFn debug;
  void *debug_userdata;
} SmtpSession;

/* --- High-level API --- */

/* Create a session. Does not connect yet. */
void crispy_smtp_init(SmtpSession *s, SmtpTransport tp, const char *local_hostname);

/* Connect to server, do EHLO, optionally STARTTLS. Returns 0 on success. */
int crispy_smtp_connect(SmtpSession *s, const char *host, int port,
                 SmtpSecurity security);

/* Authenticate (PLAIN, LOGIN, or CRAM-MD5). Returns 0 on success. */
int crispy_smtp_auth_plain(SmtpSession *s, const char *user, const char *pass);
int crispy_smtp_auth_login(SmtpSession *s, const char *user, const char *pass);

/* Send a complete message.
 * from: envelope sender (e.g. "user@example.com")
 * rcpts: NULL-terminated array of recipient addresses
 * message: raw RFC 5322 message (headers + body)
 * msgLen: length of message, or -1 for strlen
 * Returns 0 on success. */
int crispy_smtp_send(SmtpSession *s, const char *from,
              const char *rcpts[], const char *message, long msgLen);

/* Close connection gracefully (QUIT). */
void crispy_smtp_close(SmtpSession *s);

/* --- Low-level API (for custom flows) --- */

int crispy_smtp_ehlo(SmtpSession *s);
int crispy_smtp_starttls(SmtpSession *s);
int crispy_smtp_mail_from(SmtpSession *s, const char *addr);
int crispy_smtp_rcpt_to(SmtpSession *s, const char *addr);
int crispy_smtp_data_begin(SmtpSession *s);
int crispy_smtp_data_send(SmtpSession *s, const char *data, long len);
int crispy_smtp_data_end(SmtpSession *s);
int crispy_smtp_rset(SmtpSession *s);
int crispy_smtp_quit(SmtpSession *s);

/* Send raw command, read reply. Returns reply code. */
int crispy_smtp_command(SmtpSession *s, const char *cmd_line);

/* Read one reply from server. Returns reply code. */
int crispy_smtp_read_reply(SmtpSession *s);

/* --- Utilities --- */

/* Dot-stuff text for DATA transmission. out must be >= 2*inLen.
 * Returns bytes written. nlState tracks newline position across calls. */
long crispy_smtp_dot_stuff(const char *in, long inLen, char *out,
                    const char *newline, int *nlState);

/* Format RFC 5322 date. buf >= 64 bytes. */
char *crispy_smtp_format_date(char *buf, size_t bufSize, long utc_seconds,
                       long tz_offset_seconds);

/* Format timezone, e.g. "+0200". buf >= 8 bytes. */
char *crispy_smtp_format_zone(char *buf, long tz_offset_seconds);

/* Base64 encode/decode (for SASL). Caller frees output. */
char *crispy_base64_encode(const char *in, long inLen, long *outLen);
char *crispy_base64_decode(const char *in, long inLen, long *outLen);

#endif /* CRISPY_SMTP_H */
