/* pop3.c — POP3 client library (RFC 1939)
 * Part of maillib: standalone, no Eudora/GTK dependency.
 */

#include "crispy_pop3.h"
#include "crispy_md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* --- Debug helper --- */
static void dbg(Pop3Session *s, const char *fmt, ...) {
  if (!s->debug) return;
  char buf[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  s->debug(buf, s->debug_userdata);
}

/* --- Internal helpers --- */

static int tp_send(Pop3Session *s, const char *data, long len) {
  if (!s->tp.send) return -1;
  return s->tp.send(s->tp.ctx, data, len);
}

static int send_line(Pop3Session *s, const char *line) {
  dbg(s, "C: %s", line);
  int err = tp_send(s, line, (long)strlen(line));
  if (!err) err = tp_send(s, "\r\n", 2);
  return err;
}

/* Read one response line. Returns 0 for +OK, -1 for -ERR or error. */
static int read_response(Pop3Session *s) {
  long bytesRead = 0;
  if (!s->tp.recv_line) return -1;

  int err = s->tp.recv_line(s->tp.ctx, s->last_reply,
                            sizeof(s->last_reply), &bytesRead);
  if (err || bytesRead < 1) return -1;

  /* Strip trailing CRLF */
  while (bytesRead > 0 &&
         (s->last_reply[bytesRead-1] == '\r' ||
          s->last_reply[bytesRead-1] == '\n'))
    s->last_reply[--bytesRead] = '\0';

  dbg(s, "S: %s", s->last_reply);

  if (strncmp(s->last_reply, "+OK", 3) == 0) return 0;
  if (strncmp(s->last_reply, "-ERR", 4) == 0) return -1;
  return -1; /* unexpected */
}

/* --- Session lifecycle --- */

void crispy_pop3_init(Pop3Session *s, Pop3Transport tp) {
  memset(s, 0, sizeof(*s));
  s->tp = tp;
}

int crispy_pop3_connect(Pop3Session *s, const char *host, int port,
                 Pop3Security security) {
  if (!s->tp.connect) return -1;

  int err = s->tp.connect(s->tp.ctx, host, port);
  if (err) return err;
  s->connected = true;

  /* Read greeting */
  err = read_response(s);
  if (err) return -1;

  /* Save greeting for APOP */
  snprintf(s->greeting, sizeof(s->greeting), "%s", s->last_reply);

  /* STLS if requested */
  if (security == POP3_STLS) {
    if (!s->tp.start_tls) return -1;
    err = crispy_pop3_command(s, "STLS");
    if (err) return -1;
    err = s->tp.start_tls(s->tp.ctx);
    if (err) return err;
  }

  return 0;
}

int crispy_pop3_command(Pop3Session *s, const char *cmd) {
  int err = send_line(s, cmd);
  if (err) return -1;
  return read_response(s);
}

void crispy_pop3_close(Pop3Session *s) {
  if (s->connected) {
    crispy_pop3_command(s, "QUIT");
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

int crispy_pop3_auth(Pop3Session *s, const char *user, const char *pass) {
  char cmd[512];

  snprintf(cmd, sizeof(cmd), "USER %s", user);
  int err = crispy_pop3_command(s, cmd);
  if (err) return -1;

  snprintf(cmd, sizeof(cmd), "PASS %s", pass);
  err = crispy_pop3_command(s, cmd);
  if (err) return -1;

  s->authenticated = true;
  return 0;
}

int crispy_pop3_auth_apop(Pop3Session *s, const char *user, const char *pass) {
  /* Find <timestamp> in greeting */
  char *start = strchr(s->greeting, '<');
  char *end = start ? strchr(start, '>') : NULL;
  if (!start || !end) return -1;

  /* APOP: MD5(timestamp + password) */
  size_t tsLen = (size_t)(end - start + 1);
  size_t pLen = strlen(pass);
  char *concat = (char *)malloc(tsLen + pLen + 1);
  if (!concat) return -1;
  memcpy(concat, start, tsLen);
  memcpy(concat + tsLen, pass, pLen);
  concat[tsLen + pLen] = '\0';

  char hex[33];
  crispy_md5_hex(concat, tsLen + pLen, hex);
  free(concat);

  char cmd[512];
  snprintf(cmd, sizeof(cmd), "APOP %s %s", user, hex);
  int err = crispy_pop3_command(s, cmd);
  if (!err) s->authenticated = true;
  return err;
}

/* --- Message operations --- */

int crispy_pop3_stat(Pop3Session *s) {
  int err = crispy_pop3_command(s, "STAT");
  if (err) return -1;

  /* Parse "+OK count size" */
  int count = 0;
  long size = 0;
  if (sscanf(s->last_reply + 3, "%d %ld", &count, &size) >= 1) {
    s->msg_count = count;
    s->mailbox_size = size;
  }
  return 0;
}

long crispy_pop3_read_multiline(Pop3Session *s, char **out) {
  *out = NULL;
  long capacity = 4096;
  long used = 0;
  char *buf = (char *)malloc(capacity);
  if (!buf) return -1;

  char line[2048];
  long bytesRead;

  while (1) {
    if (!s->tp.recv_line) { free(buf); return -1; }
    int err = s->tp.recv_line(s->tp.ctx, line, sizeof(line), &bytesRead);
    if (err) { free(buf); return -1; }

    /* Strip trailing CRLF */
    while (bytesRead > 0 &&
           (line[bytesRead-1] == '\r' || line[bytesRead-1] == '\n'))
      bytesRead--;
    line[bytesRead] = '\0';

    /* Dot-only line terminates */
    if (bytesRead == 1 && line[0] == '.')
      break;

    /* De-stuff leading dot */
    const char *src = line;
    if (bytesRead >= 2 && line[0] == '.' && line[1] == '.')
      src = line + 1;

    long srcLen = (long)strlen(src);

    /* Grow buffer if needed */
    while (used + srcLen + 3 > capacity) {
      capacity *= 2;
      char *newBuf = (char *)realloc(buf, capacity);
      if (!newBuf) { free(buf); return -1; }
      buf = newBuf;
    }

    memcpy(buf + used, src, srcLen);
    used += srcLen;
    buf[used++] = '\r';
    buf[used++] = '\n';
  }

  buf[used] = '\0';
  *out = buf;
  return used;
}

int crispy_pop3_list(Pop3Session *s, Pop3MsgInfo **msgs) {
  *msgs = NULL;

  /* Get count first */
  int err = crispy_pop3_stat(s);
  if (err) return -1;
  if (s->msg_count == 0) return 0;

  Pop3MsgInfo *list = (Pop3MsgInfo *)calloc(s->msg_count, sizeof(Pop3MsgInfo));
  if (!list) return -1;

  /* LIST */
  err = crispy_pop3_command(s, "LIST");
  if (err) { free(list); return -1; }

  char line[256];
  long bytesRead;
  int idx = 0;

  /* Read LIST multiline response until dot terminator */
  for (;;) {
    err = s->tp.recv_line(s->tp.ctx, line, sizeof(line), &bytesRead);
    if (err) break;

    while (bytesRead > 0 &&
           (line[bytesRead-1] == '\r' || line[bytesRead-1] == '\n'))
      bytesRead--;
    line[bytesRead] = '\0';

    if (bytesRead == 1 && line[0] == '.') break;

    int num = 0;
    long sz = 0;
    if (idx < s->msg_count && sscanf(line, "%d %ld", &num, &sz) >= 1) {
      list[idx].number = num;
      list[idx].size = sz;
      idx++;
    }
  }

  /* UIDL (optional — server may not support it) */
  err = crispy_pop3_command(s, "UIDL");
  if (!err) {
    idx = 0;
    for (;;) {
      err = s->tp.recv_line(s->tp.ctx, line, sizeof(line), &bytesRead);
      if (err) break;

      while (bytesRead > 0 &&
             (line[bytesRead-1] == '\r' || line[bytesRead-1] == '\n'))
        bytesRead--;
      line[bytesRead] = '\0';

      if (bytesRead == 1 && line[0] == '.') break;

      int num = 0;
      char uidl[128] = {0};
      if (sscanf(line, "%d %127s", &num, uidl) >= 2) {
        /* Find matching entry */
        for (int i = 0; i < s->msg_count; i++) {
          if (list[i].number == num) {
            snprintf(list[i].uidl, sizeof(list[i].uidl), "%s", uidl);
            break;
          }
        }
      }
      idx++;
    }
  }

  *msgs = list;
  return s->msg_count;
}

long crispy_pop3_retr(Pop3Session *s, int msgNum, char **out) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "RETR %d", msgNum);

  int err = crispy_pop3_command(s, cmd);
  if (err) return -1;

  return crispy_pop3_read_multiline(s, out);
}

long crispy_pop3_top(Pop3Session *s, int msgNum, int lines, char **out) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "TOP %d %d", msgNum, lines);

  int err = crispy_pop3_command(s, cmd);
  if (err) return -1;

  return crispy_pop3_read_multiline(s, out);
}

int crispy_pop3_dele(Pop3Session *s, int msgNum) {
  char cmd[64];
  snprintf(cmd, sizeof(cmd), "DELE %d", msgNum);
  return crispy_pop3_command(s, cmd);
}

int crispy_pop3_rset(Pop3Session *s) {
  return crispy_pop3_command(s, "RSET");
}
