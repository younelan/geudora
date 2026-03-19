/* smtp.c — SMTP client library (RFC 5321)
 * Part of maillib: standalone, no Eudora/GTK dependency.
 */

#include "crispy_smtp.h"
#include "crispy_md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

/* SMTP command strings (RFC 5321) */
static const char *const smtp_cmds[] = {
  [SMTP_CMD_NONE]      = "",
  [SMTP_CMD_HELO]      = "HELO",
  [SMTP_CMD_EHLO]      = "EHLO",
  [SMTP_CMD_MAIL_FROM] = "MAIL FROM:",
  [SMTP_CMD_RCPT_TO]   = "RCPT TO:",
  [SMTP_CMD_DATA]      = "DATA",
  [SMTP_CMD_RSET]      = "RSET",
  [SMTP_CMD_QUIT]      = "QUIT",
  [SMTP_CMD_AUTH]      = "AUTH",
  [SMTP_CMD_STARTTLS]  = "STARTTLS",
  [SMTP_CMD_NOOP]      = "NOOP",
  [SMTP_CMD_VRFY]      = "VRFY",
  [SMTP_CMD_EXPN]      = "EXPN",
  [SMTP_CMD_HELP]      = "HELP",
};

const char *crispy_smtp_cmd_str(SmtpCmd cmd) {
  if (cmd < 0 || cmd >= SMTP_CMD_COUNT) return "";
  return smtp_cmds[cmd];
}

/* --- Debug helper --- */
static void dbg(SmtpSession *s, const char *fmt, ...) {
  if (!s->debug) return;
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  s->debug(buf, s->debug_userdata);
}

/* --- Internal helpers --- */

static int tp_send(SmtpSession *s, const char *data, long len) {
  if (!s->tp.send) return -1;
  return s->tp.send(s->tp.ctx, data, len);
}

static int tp_send_str(SmtpSession *s, const char *str) {
  return tp_send(s, str, (long)strlen(str));
}

/* Send a line (adds CRLF) */
static int send_line(SmtpSession *s, const char *line) {
  dbg(s, "C: %s", line);
  int err = tp_send_str(s, line);
  if (!err) err = tp_send(s, "\r\n", 2);
  return err;
}

/* Read one reply line. Returns reply code, or -1 on error.
 * Handles multi-line replies (code-SP vs code-DASH). */
static int read_reply_line(SmtpSession *s, char *buf, long bufSize,
                           bool *more) {
  long bytesRead = 0;
  *more = false;

  if (!s->tp.recv_line) return -1;
  int err = s->tp.recv_line(s->tp.ctx, buf, bufSize, &bytesRead);
  if (err || bytesRead < 3) return -1;

  /* Strip trailing CRLF */
  while (bytesRead > 0 && (buf[bytesRead-1] == '\r' || buf[bytesRead-1] == '\n'))
    buf[--bytesRead] = '\0';

  /* Parse 3-digit code */
  if (!isdigit((unsigned char)buf[0]) ||
      !isdigit((unsigned char)buf[1]) ||
      !isdigit((unsigned char)buf[2]))
    return -1;

  int code = (buf[0]-'0')*100 + (buf[1]-'0')*10 + (buf[2]-'0');
  *more = (bytesRead > 3 && buf[3] == '-');
  return code;
}

int crispy_smtp_read_reply(SmtpSession *s) {
  char line[512];
  bool more = true;
  int code = -1;

  s->last_reply[0] = '\0';

  while (more) {
    code = read_reply_line(s, line, sizeof(line), &more);
    if (code < 0) return -1;
    dbg(s, "S: %s", line);
    /* Keep last line text (after "NNN " or "NNN-") */
    if (strlen(line) > 4)
      snprintf(s->last_reply, sizeof(s->last_reply), "%s", line + 4);
  }

  s->last_code = code;
  return code;
}

/* Parse EHLO response for capabilities */
static void parse_ehlo_line(SmtpSession *s, const char *line) {
  /* line is the text after "250-" or "250 " */
  if (strncasecmp(line, "STARTTLS", 8) == 0)
    s->caps.starttls = true;
  else if (strncasecmp(line, "8BITMIME", 8) == 0)
    s->caps.eightbitmime = true;
  else if (strncasecmp(line, "PIPELINING", 10) == 0)
    s->caps.pipelining = true;
  else if (strncasecmp(line, "SIZE", 4) == 0) {
    const char *p = line + 4;
    while (*p == ' ') p++;
    s->caps.max_size = atol(p);
  }
  else if (strncasecmp(line, "AUTH", 4) == 0) {
    s->caps.has_auth = true;
    const char *p = line + 4;
    while (*p == ' ' || *p == '=') p++;
    snprintf(s->caps.auth_mechs, sizeof(s->caps.auth_mechs), "%s", p);
  }
}

/* Read EHLO response, parsing each capability line */
static int read_ehlo_reply(SmtpSession *s) {
  char line[512];
  bool more = true;
  int code = -1;
  bool first = true;

  memset(&s->caps, 0, sizeof(s->caps));

  while (more) {
    code = read_reply_line(s, line, sizeof(line), &more);
    if (code < 0) return -1;
    if (first) {
      first = false; /* skip greeting line */
    } else if (strlen(line) > 4) {
      parse_ehlo_line(s, line + 4);
    }
  }

  s->last_code = code;
  return code;
}

/* --- Session lifecycle --- */

void crispy_smtp_init(SmtpSession *s, SmtpTransport tp, const char *local_hostname) {
  memset(s, 0, sizeof(*s));
  s->tp = tp;
  if (local_hostname)
    snprintf(s->local_hostname, sizeof(s->local_hostname), "%s", local_hostname);
  else
    snprintf(s->local_hostname, sizeof(s->local_hostname), "localhost");
}

int crispy_smtp_connect(SmtpSession *s, const char *host, int port,
                 SmtpSecurity security) {
  if (!s->tp.connect) return -1;

  int err = s->tp.connect(s->tp.ctx, host, port);
  if (err) return err;
  s->connected = true;

  /* Read greeting */
  int code = crispy_smtp_read_reply(s);
  if (code < 0 || !SMTP_IS_OK(code)) return code > 0 ? code : -1;

  /* EHLO (fall back to HELO) */
  code = crispy_smtp_ehlo(s);
  if (code < 0) return -1;

  /* STARTTLS if requested */
  if (security == SMTP_STARTTLS && s->caps.starttls) {
    err = crispy_smtp_starttls(s);
    if (err) return err;
    /* Re-EHLO after TLS */
    code = crispy_smtp_ehlo(s);
    if (code < 0) return -1;
  }

  return SMTP_IS_OK(code) ? 0 : code;
}

int crispy_smtp_ehlo(SmtpSession *s) {
  char cmd[300];
  snprintf(cmd, sizeof(cmd), "EHLO %s", s->local_hostname);
  int err = send_line(s, cmd);
  if (err) return -1;

  int code = read_ehlo_reply(s);

  /* Fall back to HELO if EHLO fails */
  if (!SMTP_IS_OK(code)) {
    snprintf(cmd, sizeof(cmd), "HELO %s", s->local_hostname);
    err = send_line(s, cmd);
    if (err) return -1;
    code = crispy_smtp_read_reply(s);
  }

  return code;
}

int crispy_smtp_starttls(SmtpSession *s) {
  if (!s->tp.start_tls) return -1;

  int err = send_line(s, "STARTTLS");
  if (err) return -1;

  int code = crispy_smtp_read_reply(s);
  if (!SMTP_IS_OK(code)) return code > 0 ? code : -1;

  return s->tp.start_tls(s->tp.ctx);
}

int crispy_smtp_quit(SmtpSession *s) {
  send_line(s, "QUIT");
  int code = crispy_smtp_read_reply(s);
  return code;
}

void crispy_smtp_close(SmtpSession *s) {
  if (s->connected) {
    crispy_smtp_quit(s);
    if (s->tp.close)
      s->tp.close(s->tp.ctx);
    s->connected = false;
  }
  if (s->tp.destroy) {
    s->tp.destroy(s->tp.ctx);
    s->tp.ctx = NULL;
  }
}

/* --- Authentication --- */

/* Declared in smtp.h — implemented externally or here */
extern char *crispy_base64_encode(const char *in, long inLen, long *outLen);
extern char *crispy_base64_decode(const char *in, long inLen, long *outLen);

int crispy_smtp_auth_plain(SmtpSession *s, const char *user, const char *pass) {
  /* PLAIN: \0user\0pass base64-encoded */
  size_t uLen = strlen(user);
  size_t pLen = strlen(pass);
  size_t rawLen = 1 + uLen + 1 + pLen;
  char *raw = (char *)malloc(rawLen);
  if (!raw) return -1;

  raw[0] = '\0';
  memcpy(raw + 1, user, uLen);
  raw[1 + uLen] = '\0';
  memcpy(raw + 2 + uLen, pass, pLen);

  long b64Len;
  char *b64 = crispy_base64_encode(raw, (long)rawLen, &b64Len);
  free(raw);
  if (!b64) return -1;

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "AUTH PLAIN %s", b64);
  free(b64);

  int err = send_line(s, cmd);
  if (err) return -1;

  int code = crispy_smtp_read_reply(s);
  if (code == 235) s->authenticated = true;
  return SMTP_IS_OK(code) ? 0 : code;
}

int crispy_smtp_auth_login(SmtpSession *s, const char *user, const char *pass) {
  int err = send_line(s, "AUTH LOGIN");
  if (err) return -1;

  int code = crispy_smtp_read_reply(s);
  if (code != 334) return code > 0 ? code : -1;

  /* Send base64-encoded username */
  long b64Len;
  char *b64 = crispy_base64_encode(user, (long)strlen(user), &b64Len);
  if (!b64) return -1;
  err = send_line(s, b64);
  free(b64);
  if (err) return -1;

  code = crispy_smtp_read_reply(s);
  if (code != 334) return code > 0 ? code : -1;

  /* Send base64-encoded password */
  b64 = crispy_base64_encode(pass, (long)strlen(pass), &b64Len);
  if (!b64) return -1;
  err = send_line(s, b64);
  free(b64);
  if (err) return -1;

  code = crispy_smtp_read_reply(s);
  if (code == 235) s->authenticated = true;
  return SMTP_IS_OK(code) ? 0 : code;
}

int crispy_smtp_auth_begin(SmtpSession *s, const char *mech,
                    const char *initial_response) {
  char cmd[1024];
  if (initial_response)
    snprintf(cmd, sizeof(cmd), "AUTH %s %s", mech, initial_response);
  else
    snprintf(cmd, sizeof(cmd), "AUTH %s", mech);
  int err = send_line(s, cmd);
  if (err) return -1;
  return crispy_smtp_read_reply(s);
}

int crispy_smtp_auth_respond(SmtpSession *s, const char *response) {
  int err = send_line(s, response);
  if (err) return -1;
  return crispy_smtp_read_reply(s);
}

int crispy_smtp_auth_cancel(SmtpSession *s) {
  int err = send_line(s, "*");
  if (err) return -1;
  return crispy_smtp_read_reply(s);
}

int crispy_smtp_auth_cram_md5(SmtpSession *s, const char *user, const char *pass) {
  /* Step 1: send AUTH CRAM-MD5, get 334 + base64 challenge */
  int err = send_line(s, "AUTH CRAM-MD5");
  if (err) return -1;

  int code = crispy_smtp_read_reply(s);
  if (code != 334) return code > 0 ? code : -1;

  /* Step 2: decode the base64 challenge from last_reply */
  long chalLen;
  char *challenge = crispy_base64_decode(s->last_reply, (long)strlen(s->last_reply), &chalLen);
  if (!challenge) return -1;

  /* Step 3: HMAC-MD5(password, challenge) */
  char hex[33];
  crispy_hmac_md5_hex(pass, strlen(pass), challenge, chalLen, hex);
  free(challenge);

  /* Step 4: build "user hex" and base64-encode it */
  char response[512];
  snprintf(response, sizeof(response), "%s %s", user, hex);

  long b64Len;
  char *b64 = crispy_base64_encode(response, (long)strlen(response), &b64Len);
  if (!b64) return -1;

  err = send_line(s, b64);
  free(b64);
  if (err) return -1;

  code = crispy_smtp_read_reply(s);
  if (code == 235) s->authenticated = true;
  return SMTP_IS_OK(code) ? 0 : code;
}

/* --- Sending --- */

int crispy_smtp_command(SmtpSession *s, const char *cmd_line) {
  int err = send_line(s, cmd_line);
  if (err) return -1;
  return crispy_smtp_read_reply(s);
}

int crispy_smtp_mail_from(SmtpSession *s, const char *addr) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "MAIL FROM:<%s>", addr);
  return crispy_smtp_command(s, cmd);
}

int crispy_smtp_rcpt_to(SmtpSession *s, const char *addr) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "RCPT TO:<%s>", addr);
  return crispy_smtp_command(s, cmd);
}

int crispy_smtp_data_begin(SmtpSession *s) {
  return crispy_smtp_command(s, "DATA");
}

int crispy_smtp_data_send(SmtpSession *s, const char *data, long len) {
  return tp_send(s, data, len);
}

int crispy_smtp_data_end(SmtpSession *s) {
  int err = tp_send(s, "\r\n.\r\n", 5);
  if (err) return -1;
  return crispy_smtp_read_reply(s);
}

int crispy_smtp_rset(SmtpSession *s) {
  return crispy_smtp_command(s, "RSET");
}

/* High-level send: MAIL FROM, RCPT TO for each, DATA, message body */
int crispy_smtp_send(SmtpSession *s, const char *from,
              const char *rcpts[], const char *message, long msgLen) {
  if (msgLen < 0) msgLen = (long)strlen(message);

  /* RSET first to clear any previous state */
  int code = crispy_smtp_rset(s);
  if (!SMTP_IS_OK(code) && code != -1) return code;

  /* MAIL FROM */
  code = crispy_smtp_mail_from(s, from);
  if (!SMTP_IS_OK(code)) return code;

  /* RCPT TO for each recipient */
  for (int i = 0; rcpts[i]; i++) {
    code = crispy_smtp_rcpt_to(s, rcpts[i]);
    if (!SMTP_IS_OK(code)) return code;
  }

  /* DATA */
  code = crispy_smtp_data_begin(s);
  if (code != 354 && !SMTP_IS_OK(code)) return code;

  /* Send message body with dot-stuffing */
  int nlState = 2; /* start as if we just saw a newline */
  long outBufSize = msgLen * 2 + 16;
  char *stuffed = (char *)malloc(outBufSize);
  if (!stuffed) return -1;

  long stuffedLen = crispy_smtp_dot_stuff(message, msgLen, stuffed, "\r\n", &nlState);
  int err = crispy_smtp_data_send(s, stuffed, stuffedLen);
  free(stuffed);
  if (err) return -1;

  /* End DATA */
  code = crispy_smtp_data_end(s);
  return SMTP_IS_OK(code) ? 0 : code;
}

/* --- Utilities --- */

long crispy_smtp_dot_stuff(const char *in, long inLen, char *out,
                    const char *newline, int *nlState) {
  int state = *nlState;
  const char *end = in + inLen;
  int nlLen = (int)strlen(newline);
  char *orig = out;

  for (; in < end; in++) {
    if (*in == '.' && state == nlLen) {
      *out++ = '.'; /* stuff extra period */
      state = 0;
    } else {
      if (state == nlLen)
        state = 0;
      if (*in == newline[state])
        state++;
      else
        state = 0;
    }
    *out++ = *in;
  }

  *nlState = state;
  return (long)(out - orig);
}

static const char *day_names[] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char *month_names[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *crispy_smtp_format_date(char *buf, size_t bufSize, long utc_seconds,
                       long tz_offset_seconds) {
  time_t t = (time_t)utc_seconds;
  struct tm tm;
#ifdef _WIN32
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif

  /* Adjust for timezone */
  t += tz_offset_seconds;
#ifdef _WIN32
  gmtime_s(&tm, &t);
#else
  gmtime_r(&t, &tm);
#endif

  long tz_min = tz_offset_seconds / 60;
  bool neg = tz_min < 0;
  if (neg) tz_min = -tz_min;

  snprintf(buf, bufSize, "%s, %02d %s %04d %02d:%02d:%02d %c%02ld%02ld",
           day_names[tm.tm_wday], tm.tm_mday, month_names[tm.tm_mon],
           tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec,
           neg ? '-' : '+', tz_min / 60, tz_min % 60);
  return buf;
}

char *crispy_smtp_format_zone(char *buf, long tz_offset_seconds) {
  long tz_min = tz_offset_seconds / 60;
  bool neg = tz_min < 0;
  if (neg) tz_min = -tz_min;
  snprintf(buf, 8, "%c%02ld%02ld", neg ? '-' : '+', tz_min / 60, tz_min % 60);
  return buf;
}
