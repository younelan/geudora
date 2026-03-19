/* crispy_imap.c — IMAP4rev1 client (RFC 3501)
 * Part of crispy: standalone, no Eudora dependency.
 *
 * Full IMAP4rev1 implementation:
 *   Connection:    CAPABILITY, LOGIN, AUTHENTICATE (PLAIN, XOAUTH2, CRAM-MD5),
 *                  STARTTLS, LOGOUT
 *   Mailbox:       LIST, LSUB, SELECT, EXAMINE, STATUS, CREATE, DELETE,
 *                  RENAME, SUBSCRIBE, UNSUBSCRIBE, CHECK, CLOSE
 *   Fetch:         UID FETCH (FLAGS, BODY.PEEK[], BODY.PEEK[HEADER],
 *                  BODY.PEEK[TEXT], BODY.PEEK[section], HEADER.FIELDS,
 *                  RFC822.SIZE, INTERNALDATE, ENVELOPE, BODYSTRUCTURE)
 *   Modify:        UID STORE, UID COPY, UID MOVE, EXPUNGE, APPEND
 *   Search:        UID SEARCH
 *   Session:       NOOP, IDLE (RFC 2177)
 *
 * Portable: POSIX + Windows (via #ifdef).
 */

#include "crispy_imap.h"
#include "crispy_smtp.h"  /* for crispy_base64_encode/decode */
#include "crispy_md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define CRISPY_POLL WSAPoll
  typedef WSAPOLLFD crispy_pollfd;
#else
  #include <poll.h>
  #include <unistd.h>
  #define CRISPY_POLL poll
  typedef struct pollfd crispy_pollfd;
#endif

/* Forward declarations */
static bool needs_literal(const char *str);
static int send_with_literal(ImapSession *s, const char *prefix, const char *arg);
void crispy_imap_cache_clear(ImapSession *s);

/* ================================================================
 * Debug
 * ================================================================ */

static void dbg(ImapSession *s, const char *fmt, ...) {
  if (!s->debug) return;
  char buf[4096];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  s->debug(buf, s->debug_userdata);
}

/* ================================================================
 * Transport helpers
 * ================================================================ */

static int tp_send(ImapSession *s, const char *data, long len) {
  if (!s->tp.send) return -1;
  return s->tp.send(s->tp.ctx, data, len);
}

static int send_line(ImapSession *s, const char *line) {
  dbg(s, "C: %s", line);
  int err = tp_send(s, line, (long)strlen(line));
  if (!err) err = tp_send(s, "\r\n", 2);
  return err;
}

/* Read one line (up to \n). Returns malloc'd string, caller frees. */
char *crispy_imap_readline(ImapSession *s) {
  if (!s->tp.recv_line) return NULL;
  char buf[8192];
  long bytesRead = 0;
  int err = s->tp.recv_line(s->tp.ctx, buf, sizeof(buf), &bytesRead);
  if (err || bytesRead <= 0) return NULL;
  /* Strip CRLF */
  while (bytesRead > 0 && (buf[bytesRead-1] == '\r' || buf[bytesRead-1] == '\n'))
    buf[--bytesRead] = '\0';
  buf[bytesRead] = '\0';
  dbg(s, "S: %s", buf);
  return strdup(buf);
}

/* Read exactly 'size' bytes of literal data. Returns malloc'd buffer. */
static char *read_literal(ImapSession *s, long size) {
  if (size <= 0) return NULL;
  char *data = (char *)malloc(size + 1);
  if (!data) return NULL;
  long got = 0;
  while (got < size) {
    char buf[8192];
    long want = size - got;
    if (want > (long)sizeof(buf)) want = sizeof(buf);
    long n = want;
    int err = s->tp.recv_line(s->tp.ctx, buf, want, &n);
    if (err || n <= 0) { free(data); return NULL; }
    memcpy(data + got, buf, n);
    got += n;
  }
  data[size] = '\0';
  return data;
}

/* ================================================================
 * Tag management
 * ================================================================ */

static void next_tag(ImapSession *s) {
  s->tag_num++;
  snprintf(s->tag, sizeof(s->tag), "A%04d", s->tag_num);
}

/* ================================================================
 * Core command/response engine
 * ================================================================ */

/* Track unsolicited mailbox updates (EXISTS, RECENT, EXPUNGE, BYE)
 * that can arrive in any untagged response. */
static void track_mailbox_updates(ImapSession *s, const char *untagged) {
  long num;
  if (sscanf(untagged, "%ld EXISTS", &num) == 1) {
    s->selected.exists = num;
    dbg(s, "Mailbox update: %ld EXISTS", num);
  } else if (sscanf(untagged, "%ld RECENT", &num) == 1) {
    s->selected.recent = num;
  } else if (sscanf(untagged, "%ld EXPUNGE", &num) == 1) {
    if (s->selected.exists > 0) s->selected.exists--;
    /* Update seqno map: remove the expunged entry, decrement higher seqnos */
    for (int i = 0; i < s->cache.count; i++) {
      if (s->cache.entries[i].seqno == (unsigned long)num) {
        /* Free cached data */
        free(s->cache.entries[i].internaldate);
        crispy_imap_free_envelope(s->cache.entries[i].envelope);
        /* Remove by shifting */
        memmove(&s->cache.entries[i], &s->cache.entries[i+1],
                (s->cache.count - i - 1) * sizeof(ImapCacheEntry));
        s->cache.count--;
        i--;
      } else if (s->cache.entries[i].seqno > (unsigned long)num) {
        s->cache.entries[i].seqno--;
      }
    }
    dbg(s, "Mailbox update: msg %ld EXPUNGED, now %ld", num, s->selected.exists);
  } else if (strncasecmp(untagged, "BYE ", 4) == 0) {
    s->bye_received = true;
    s->connected = false;
    dbg(s, "Server BYE: %s", untagged + 4);
  }
}

/* Parse response code from tagged reply: OK [CODE ...] text */
static void parse_response_code(ImapSession *s, const char *reply) {
  s->last_rcode = IMAP_RCODE_NONE;
  /* Skip OK/NO/BAD */
  const char *p = reply;
  while (*p && *p != ' ' && *p != '[') p++;
  while (*p == ' ') p++;
  if (*p != '[') return;
  p++; /* skip [ */

  if (strncasecmp(p, "ALERT", 5) == 0) {
    s->last_rcode = IMAP_RCODE_ALERT;
    /* Extract alert text after ] */
    char *close = strchr(p, ']');
    if (close) {
      close++;
      while (*close == ' ') close++;
      snprintf(s->last_alert, sizeof(s->last_alert), "%s", close);
    }
    dbg(s, "ALERT: %s", s->last_alert);
  } else if (strncasecmp(p, "TRYCREATE", 9) == 0) {
    s->last_rcode = IMAP_RCODE_TRYCREATE;
  } else if (strncasecmp(p, "PARSE", 5) == 0) {
    s->last_rcode = IMAP_RCODE_PARSE;
  } else if (strncasecmp(p, "READ-ONLY", 9) == 0) {
    s->last_rcode = IMAP_RCODE_READ_ONLY;
  } else if (strncasecmp(p, "READ-WRITE", 10) == 0) {
    s->last_rcode = IMAP_RCODE_READ_WRITE;
  } else if (strncasecmp(p, "APPENDUID", 9) == 0) {
    s->last_rcode = IMAP_RCODE_APPENDUID;
  } else if (strncasecmp(p, "COPYUID", 7) == 0) {
    s->last_rcode = IMAP_RCODE_COPYUID;
  } else if (strncasecmp(p, "UIDVALIDITY", 11) == 0) {
    s->last_rcode = IMAP_RCODE_UIDVALIDITY;
  } else if (strncasecmp(p, "UIDNEXT", 7) == 0) {
    s->last_rcode = IMAP_RCODE_UIDNEXT;
  } else if (strncasecmp(p, "UNSEEN", 6) == 0) {
    s->last_rcode = IMAP_RCODE_UNSEEN;
  } else if (strncasecmp(p, "PERMANENTFLAGS", 14) == 0) {
    s->last_rcode = IMAP_RCODE_PERMANENTFLAGS;
  } else if (strncasecmp(p, "REFERRAL ", 9) == 0) {
    s->last_rcode = IMAP_RCODE_REFERRAL;
    p += 9;
    char *close = strchr(p, ']');
    if (close) {
      size_t len = close - p;
      if (len >= sizeof(s->last_referral)) len = sizeof(s->last_referral) - 1;
      memcpy(s->last_referral, p, len);
      s->last_referral[len] = '\0';
    }
  } else {
    s->last_rcode = IMAP_RCODE_OTHER;
  }
}

/* Send tagged command, read all responses until tagged reply.
 * Discards untagged responses (stores last one in s->reply). */
int crispy_imap_command(ImapSession *s, const char *cmd) {
  if (s->bye_received) return IMAP_ERR;
  next_tag(s);
  char full[4096];
  snprintf(full, sizeof(full), "%s %s", s->tag, cmd);
  if (send_line(s, full)) return IMAP_ERR;

  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;

    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      char *status = line + tagLen + 1;
      snprintf(s->reply, sizeof(s->reply), "%s", status);
      parse_response_code(s, status);
      int result;
      if (strncasecmp(status, "OK", 2) == 0)       result = IMAP_OK;
      else if (strncasecmp(status, "NO", 2) == 0)   result = IMAP_NO;
      else if (strncasecmp(status, "BAD", 3) == 0)  result = IMAP_BAD;
      else result = IMAP_ERR;
      free(line);
      return result;
    }

    /* Untagged response */
    if (line[0] == '*' && line[1] == ' ') {
      track_mailbox_updates(s, line + 2);
      snprintf(s->reply, sizeof(s->reply), "%s", line + 2);
    }

    free(line);
    if (s->bye_received) return IMAP_ERR;
  }
}

/* Send command and collect all untagged responses into a growing array. */
int crispy_imap_command_collect(ImapSession *s, const char *cmd,
                                 char ***lines, int *lineCount) {
  if (s->bye_received) return IMAP_ERR;
  next_tag(s);
  char full[4096];
  snprintf(full, sizeof(full), "%s %s", s->tag, cmd);
  if (send_line(s, full)) return IMAP_ERR;

  int cap = 64;
  *lines = (char **)calloc(cap, sizeof(char *));
  *lineCount = 0;

  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;

    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      parse_response_code(s, line + tagLen + 1);
      int result;
      if (strncasecmp(line + tagLen + 1, "OK", 2) == 0)  result = IMAP_OK;
      else if (strncasecmp(line + tagLen + 1, "NO", 2) == 0) result = IMAP_NO;
      else result = IMAP_BAD;
      free(line);
      return result;
    }

    if (line[0] == '*' && line[1] == ' ') {
      track_mailbox_updates(s, line + 2);
      if (*lineCount >= cap) {
        cap *= 2;
        *lines = (char **)realloc(*lines, cap * sizeof(char *));
      }
      (*lines)[(*lineCount)++] = strdup(line + 2);
    }
    free(line);
  }
}

static void free_collected(char **lines, int count) {
  if (!lines) return;
  for (int i = 0; i < count; i++) free(lines[i]);
  free(lines);
}

/* ================================================================
 * Fetch with literal support — sends command, reads responses
 * including {NNN} literals, returns all response lines with
 * literals expanded inline.
 * ================================================================ */

static int fetch_collect(ImapSession *s, const char *cmd,
                          char ***lines, int *lineCount) {
  next_tag(s);
  char full[4096];
  snprintf(full, sizeof(full), "%s %s", s->tag, cmd);
  if (send_line(s, full)) return IMAP_ERR;

  int cap = 64;
  *lines = (char **)calloc(cap, sizeof(char *));
  *lineCount = 0;

  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;

    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      int result;
      if (strncasecmp(line + tagLen + 1, "OK", 2) == 0) result = IMAP_OK;
      else if (strncasecmp(line + tagLen + 1, "NO", 2) == 0) result = IMAP_NO;
      else result = IMAP_BAD;
      free(line);
      return result;
    }

    /* Check for literal {NNN} at end of line */
    char *lbrace = strrchr(line, '{');
    if (lbrace) {
      char *rbrace = strchr(lbrace, '}');
      if (rbrace && (rbrace[1] == '\0' || rbrace[1] == '\r')) {
        long litSize = strtol(lbrace + 1, NULL, 10);
        if (litSize > 0) {
          char *litData = read_literal(s, litSize);
          if (litData) {
            /* Combine line prefix + literal data */
            size_t prefixLen = lbrace - line;
            size_t totalLen = prefixLen + litSize + 1;
            char *combined = (char *)malloc(totalLen + 1);
            if (combined) {
              memcpy(combined, line, prefixLen);
              combined[prefixLen] = '\n'; /* separator */
              memcpy(combined + prefixLen + 1, litData, litSize);
              combined[totalLen] = '\0';
            }
            free(litData);

            /* Read the closing line (usually just ")") and append */
            char *closeLine = crispy_imap_readline(s);
            if (closeLine && combined) {
              size_t clLen = strlen(closeLine);
              char *full2 = (char *)realloc(combined, totalLen + clLen + 2);
              if (full2) {
                full2[totalLen] = '\n';
                memcpy(full2 + totalLen + 1, closeLine, clLen);
                full2[totalLen + 1 + clLen] = '\0';
                combined = full2;
                totalLen = totalLen + 1 + clLen;
              }
            }
            free(closeLine);

            if (*lineCount >= cap) {
              cap *= 2;
              *lines = (char **)realloc(*lines, cap * sizeof(char *));
            }
            if (line[0] == '*' && line[1] == ' ') {
              /* Store without "* " prefix in the combined data */
              (*lines)[(*lineCount)++] = combined;
            } else {
              (*lines)[(*lineCount)++] = combined;
            }
            free(line);
            continue;
          }
        }
      }
    }

    if (line[0] == '*' && line[1] == ' ') {
      if (*lineCount >= cap) {
        cap *= 2;
        *lines = (char **)realloc(*lines, cap * sizeof(char *));
      }
      (*lines)[(*lineCount)++] = strdup(line + 2);
    }
    free(line);
  }
}

/* ================================================================
 * Parse helpers
 * ================================================================ */

static void parse_capabilities(ImapSession *s, const char *line) {
  /* Detect protocol level */
  if (strcasestr(line, "IMAP4rev2"))      s->level = IMAP_LEVEL_IMAP4REV2;
  else if (strcasestr(line, "IMAP4rev1")) s->level = IMAP_LEVEL_IMAP4REV1;
  else if (strcasestr(line, "IMAP4"))     s->level = IMAP_LEVEL_IMAP4;
  else if (strcasestr(line, "IMAP2bis"))  s->level = IMAP_LEVEL_IMAP2BIS;
  else if (strcasestr(line, "IMAP2"))     s->level = IMAP_LEVEL_IMAP2;

  if (strcasestr(line, "STARTTLS"))    s->cap_starttls = true;
  if (strcasestr(line, "IDLE"))        s->cap_idle = true;
  if (strcasestr(line, "UIDPLUS"))     s->cap_uidplus = true;
  if (strcasestr(line, "MOVE"))        s->cap_move = true;
  if (strcasestr(line, "LITERAL+"))    s->cap_literal_plus = true;
  if (strcasestr(line, "SORT"))        s->cap_sort = true;
  if (strcasestr(line, "THREAD"))      s->cap_thread = true;
  if (strcasestr(line, "NAMESPACE"))   s->cap_namespace = true;
  if (strcasestr(line, "CONDSTORE"))   s->cap_condstore = true;
  if (strcasestr(line, "CHILDREN"))    s->cap_children = true;
  if (strcasestr(line, " ID"))         s->cap_id = true;
  if (strcasestr(line, "QUOTA"))       s->cap_quota = true;
  if (strcasestr(line, " ACL"))        s->cap_acl = true;
  /* Collect AUTH mechanisms */
  const char *auth = line;
  s->cap_auth[0] = '\0';
  while ((auth = strcasestr(auth, "AUTH=")) != NULL) {
    auth += 5;
    const char *end = auth;
    while (*end && *end != ' ' && *end != ']') end++;
    if (s->cap_auth[0])
      strncat(s->cap_auth, " ", sizeof(s->cap_auth) - strlen(s->cap_auth) - 1);
    size_t len = end - auth;
    if (len > sizeof(s->cap_auth) - strlen(s->cap_auth) - 2) break;
    strncat(s->cap_auth, auth, len);
    auth = end;
  }
}

static ImapFlags parse_flags(const char *str) {
  ImapFlags f = {0};
  if (strcasestr(str, "\\Seen"))     f.seen = true;
  if (strcasestr(str, "\\Answered")) f.answered = true;
  if (strcasestr(str, "\\Flagged"))  f.flagged = true;
  if (strcasestr(str, "\\Deleted"))  f.deleted = true;
  if (strcasestr(str, "\\Draft"))    f.draft = true;
  if (strcasestr(str, "\\Recent"))   f.recent = true;
  return f;
}

static char *flags_to_str(ImapFlags f, char *buf, size_t sz) {
  buf[0] = '\0';
  if (f.seen)     strlcat(buf, "\\Seen ", sz);
  if (f.answered) strlcat(buf, "\\Answered ", sz);
  if (f.flagged)  strlcat(buf, "\\Flagged ", sz);
  if (f.deleted)  strlcat(buf, "\\Deleted ", sz);
  if (f.draft)    strlcat(buf, "\\Draft ", sz);
  size_t len = strlen(buf);
  if (len > 0 && buf[len-1] == ' ') buf[len-1] = '\0';
  return buf;
}

/* ================================================================
 * IMAP S-expression parser for ENVELOPE and BODYSTRUCTURE
 *
 * IMAP uses parenthesized lists with quoted strings, literals, and NIL.
 * This parser walks a cursor through the response text.
 * ================================================================ */

/* Skip whitespace */
static void skip_ws(const char **p) {
  while (**p == ' ' || **p == '\t' || **p == '\r' || **p == '\n') (*p)++;
}

/* Parse a quoted string or NIL. Returns malloc'd string or NULL for NIL.
 * Advances *p past the parsed token. */
static char *parse_nstring(const char **p) {
  skip_ws(p);
  if (strncasecmp(*p, "NIL", 3) == 0 &&
      ((*p)[3] == ' ' || (*p)[3] == ')' || (*p)[3] == '\0')) {
    *p += 3;
    return NULL;
  }
  if (**p == '"') {
    (*p)++; /* skip opening quote */
    const char *start = *p;
    /* Find closing quote, handling escaped chars */
    while (**p && (**p != '"' || (*(*p - 1) == '\\' && *(*p - 2) != '\\'))) {
      (*p)++;
    }
    size_t len = *p - start;
    char *str = (char *)malloc(len + 1);
    if (str) {
      /* Unescape: remove backslash escapes */
      size_t j = 0;
      for (size_t i = 0; i < len; i++) {
        if (start[i] == '\\' && i + 1 < len) {
          str[j++] = start[i + 1];
          i++;
        } else {
          str[j++] = start[i];
        }
      }
      str[j] = '\0';
    }
    if (**p == '"') (*p)++; /* skip closing quote */
    return str;
  }
  /* Atom (unquoted token) */
  if (**p != '(' && **p != ')') {
    const char *start = *p;
    while (**p && **p != ' ' && **p != ')' && **p != '(' && **p != '\r' && **p != '\n')
      (*p)++;
    size_t len = *p - start;
    if (len == 3 && strncasecmp(start, "NIL", 3) == 0) return NULL;
    char *str = (char *)malloc(len + 1);
    if (str) { memcpy(str, start, len); str[len] = '\0'; }
    return str;
  }
  return NULL;
}

/* Parse a number atom. Returns the number, 0 if NIL. Advances *p. */
static unsigned long parse_number(const char **p) {
  skip_ws(p);
  if (strncasecmp(*p, "NIL", 3) == 0) { *p += 3; return 0; }
  unsigned long n = strtoul(*p, (char **)p, 10);
  return n;
}

/* Skip one S-expression element (atom, string, or nested parens) */
static void skip_sexp(const char **p) {
  skip_ws(p);
  if (**p == '(') {
    int depth = 1;
    (*p)++;
    while (**p && depth > 0) {
      if (**p == '(') depth++;
      else if (**p == ')') depth--;
      else if (**p == '"') {
        (*p)++;
        while (**p && **p != '"') {
          if (**p == '\\') (*p)++;
          (*p)++;
        }
      }
      if (**p) (*p)++;
    }
  } else if (**p == '"') {
    (*p)++;
    while (**p && **p != '"') {
      if (**p == '\\') (*p)++;
      (*p)++;
    }
    if (**p == '"') (*p)++;
  } else {
    /* Atom or NIL */
    while (**p && **p != ' ' && **p != ')' && **p != '(') (*p)++;
  }
}

/* ================================================================
 * Address parsing for ENVELOPE
 * Format: ((personal adl mailbox host) (personal adl mailbox host) ...)
 * ================================================================ */

static ImapAddress *parse_address(const char **p) {
  skip_ws(p);
  if (**p != '(') return NULL;
  (*p)++; /* skip ( */
  ImapAddress *a = (ImapAddress *)calloc(1, sizeof(ImapAddress));
  if (!a) return NULL;
  a->name = parse_nstring(p);
  a->adl = parse_nstring(p);
  a->mailbox = parse_nstring(p);
  a->host = parse_nstring(p);
  skip_ws(p);
  if (**p == ')') (*p)++;
  return a;
}

static ImapAddress *parse_address_list(const char **p) {
  skip_ws(p);
  if (strncasecmp(*p, "NIL", 3) == 0 &&
      ((*p)[3] == ' ' || (*p)[3] == ')' || (*p)[3] == '\0')) {
    *p += 3;
    return NULL;
  }
  if (**p != '(') return NULL;
  (*p)++; /* skip outer ( */

  ImapAddress *head = NULL, *tail = NULL;
  while (**p && **p != ')') {
    skip_ws(p);
    if (**p == ')') break;
    ImapAddress *a = parse_address(p);
    if (a) {
      if (!head) head = a;
      else tail->next = a;
      tail = a;
    }
    skip_ws(p);
  }
  if (**p == ')') (*p)++;
  return head;
}

/* ================================================================
 * ENVELOPE parser
 * Format: (date subject from sender reply-to to cc bcc in-reply-to message-id)
 * ================================================================ */

static ImapEnvelope *parse_envelope_sexp(const char **p) {
  skip_ws(p);
  if (**p != '(') return NULL;
  (*p)++;

  ImapEnvelope *env = (ImapEnvelope *)calloc(1, sizeof(ImapEnvelope));
  if (!env) return NULL;

  env->date       = parse_nstring(p);
  env->subject    = parse_nstring(p);
  env->from       = parse_address_list(p);
  env->sender     = parse_address_list(p);
  env->reply_to   = parse_address_list(p);
  env->to         = parse_address_list(p);
  env->cc         = parse_address_list(p);
  env->bcc        = parse_address_list(p);
  env->in_reply_to = parse_nstring(p);
  env->message_id  = parse_nstring(p);

  skip_ws(p);
  if (**p == ')') (*p)++;
  return env;
}

/* ================================================================
 * MIME parameter list parser
 * Format: (name1 value1 name2 value2 ...) or NIL
 * ================================================================ */

static ImapParam *parse_param_list(const char **p) {
  skip_ws(p);
  if (strncasecmp(*p, "NIL", 3) == 0 &&
      ((*p)[3] == ' ' || (*p)[3] == ')' || (*p)[3] == '\0')) {
    *p += 3;
    return NULL;
  }
  if (**p != '(') return NULL;
  (*p)++;

  ImapParam *head = NULL, *tail = NULL;
  while (**p && **p != ')') {
    skip_ws(p);
    if (**p == ')') break;
    char *name = parse_nstring(p);
    char *value = parse_nstring(p);
    if (name) {
      ImapParam *param = (ImapParam *)calloc(1, sizeof(ImapParam));
      if (param) {
        param->name = name;
        param->value = value;
        if (!head) head = param;
        else tail->next = param;
        tail = param;
      } else {
        free(name); free(value);
      }
    } else {
      free(value);
    }
    skip_ws(p);
  }
  if (**p == ')') (*p)++;
  return head;
}

/* ================================================================
 * Disposition parser
 * Format: (type (params)) or NIL
 * ================================================================ */

static ImapDisposition parse_disposition_sexp(const char **p) {
  ImapDisposition d = {0};
  skip_ws(p);
  if (strncasecmp(*p, "NIL", 3) == 0 &&
      ((*p)[3] == ' ' || (*p)[3] == ')' || (*p)[3] == '\0')) {
    *p += 3;
    return d;
  }
  if (**p != '(') return d;
  (*p)++;
  d.type = parse_nstring(p);
  d.params = parse_param_list(p);
  skip_ws(p);
  if (**p == ')') (*p)++;
  return d;
}

/* ================================================================
 * Body type/encoding name → enum
 * ================================================================ */

static ImapBodyType body_type_from_str(const char *s) {
  if (!s) return IMAP_TYPE_OTHER;
  if (strcasecmp(s, "TEXT") == 0)        return IMAP_TYPE_TEXT;
  if (strcasecmp(s, "MULTIPART") == 0)   return IMAP_TYPE_MULTIPART;
  if (strcasecmp(s, "MESSAGE") == 0)     return IMAP_TYPE_MESSAGE;
  if (strcasecmp(s, "APPLICATION") == 0) return IMAP_TYPE_APPLICATION;
  if (strcasecmp(s, "AUDIO") == 0)       return IMAP_TYPE_AUDIO;
  if (strcasecmp(s, "IMAGE") == 0)       return IMAP_TYPE_IMAGE;
  if (strcasecmp(s, "VIDEO") == 0)       return IMAP_TYPE_VIDEO;
  if (strcasecmp(s, "MODEL") == 0)       return IMAP_TYPE_MODEL;
  return IMAP_TYPE_OTHER;
}

static ImapEncoding encoding_from_str(const char *s) {
  if (!s) return IMAP_ENC_OTHER;
  if (strcasecmp(s, "7BIT") == 0)              return IMAP_ENC_7BIT;
  if (strcasecmp(s, "8BIT") == 0)              return IMAP_ENC_8BIT;
  if (strcasecmp(s, "BINARY") == 0)            return IMAP_ENC_BINARY;
  if (strcasecmp(s, "BASE64") == 0)            return IMAP_ENC_BASE64;
  if (strcasecmp(s, "QUOTED-PRINTABLE") == 0)  return IMAP_ENC_QUOTED_PRINTABLE;
  return IMAP_ENC_OTHER;
}

/* ================================================================
 * BODYSTRUCTURE recursive parser
 *
 * Non-multipart body:
 *   (type subtype (params) id description encoding size [lines]
 *    [md5 [disposition [language [location]]]])
 *
 * Multipart body:
 *   (part1 part2 ... subtype [(params) [disposition [language [location]]]])
 *
 * message/rfc822 body:
 *   ("MESSAGE" "RFC822" (params) id description encoding size
 *    envelope bodystructure lines [md5 [disp [lang [loc]]]])
 * ================================================================ */

static ImapBodyPart *parse_bodystructure(const char **p, const char *prefix);

static ImapBodyPart *parse_body_single(const char **p, const char *section) {
  ImapBodyPart *bp = (ImapBodyPart *)calloc(1, sizeof(ImapBodyPart));
  if (!bp) return NULL;
  snprintf(bp->section, sizeof(bp->section), "%s", section ? section : "1");

  /* type subtype */
  char *typeStr = parse_nstring(p);
  bp->subtype = parse_nstring(p);
  bp->type = body_type_from_str(typeStr);
  free(typeStr);

  /* params */
  bp->params = parse_param_list(p);

  /* id, description, encoding */
  bp->id = parse_nstring(p);
  bp->description = parse_nstring(p);
  char *encStr = parse_nstring(p);
  bp->encoding = encoding_from_str(encStr);
  free(encStr);

  /* size in bytes */
  bp->size_bytes = parse_number(p);

  /* For text types, there's also a line count */
  if (bp->type == IMAP_TYPE_TEXT) {
    bp->size_lines = parse_number(p);
  }

  /* For MESSAGE/RFC822: envelope, bodystructure, lines */
  if (bp->type == IMAP_TYPE_MESSAGE && bp->subtype &&
      strcasecmp(bp->subtype, "RFC822") == 0) {
    bp->nested_env = parse_envelope_sexp(p);
    bp->nested_body = parse_bodystructure(p, section);
    bp->size_lines = parse_number(p);
  }

  /* Extension data (optional): md5, disposition, language, location */
  skip_ws(p);
  if (**p != ')') {
    bp->md5 = parse_nstring(p);
    skip_ws(p);
    if (**p != ')') {
      bp->disposition = parse_disposition_sexp(p);
      skip_ws(p);
      if (**p != ')') {
        bp->language = parse_nstring(p);
        skip_ws(p);
        if (**p != ')') {
          bp->location = parse_nstring(p);
          /* Skip any further extensions */
          skip_ws(p);
          while (**p && **p != ')') skip_sexp(p);
        }
      }
    }
  }

  return bp;
}

static ImapBodyPart *parse_bodystructure(const char **p, const char *prefix) {
  skip_ws(p);
  if (**p != '(') return NULL;
  (*p)++;

  skip_ws(p);
  /* If next char is '(' it's multipart (each part starts with '(') */
  if (**p == '(') {
    ImapBodyPart *bp = (ImapBodyPart *)calloc(1, sizeof(ImapBodyPart));
    if (!bp) return NULL;
    bp->type = IMAP_TYPE_MULTIPART;
    snprintf(bp->section, sizeof(bp->section), "%s", prefix ? prefix : "");

    /* Parse child parts */
    ImapBodyPart *lastChild = NULL;
    int partNum = 1;
    while (**p == '(') {
      char childSection[64];
      if (prefix && prefix[0])
        snprintf(childSection, sizeof(childSection), "%s.%d", prefix, partNum);
      else
        snprintf(childSection, sizeof(childSection), "%d", partNum);

      ImapBodyPart *child = parse_bodystructure(p, childSection);
      if (child) {
        if (!bp->subparts) bp->subparts = child;
        else lastChild->next = child;
        lastChild = child;
      }
      partNum++;
      skip_ws(p);
    }

    /* After all parts: subtype */
    bp->subtype = parse_nstring(p);

    /* Extension data: params, disposition, language, location */
    skip_ws(p);
    if (**p != ')') {
      bp->params = parse_param_list(p);
      skip_ws(p);
      if (**p != ')') {
        bp->disposition = parse_disposition_sexp(p);
        skip_ws(p);
        if (**p != ')') {
          bp->language = parse_nstring(p);
          skip_ws(p);
          if (**p != ')') {
            bp->location = parse_nstring(p);
            skip_ws(p);
            while (**p && **p != ')') skip_sexp(p);
          }
        }
      }
    }

    skip_ws(p);
    if (**p == ')') (*p)++;
    return bp;
  }

  /* Single body part */
  ImapBodyPart *bp = parse_body_single(p, prefix);
  skip_ws(p);
  if (**p == ')') (*p)++;
  return bp;
}

/* ================================================================
 * Memory management
 * ================================================================ */

void crispy_imap_free_params(ImapParam *p) {
  while (p) {
    ImapParam *next = p->next;
    free(p->name);
    free(p->value);
    free(p);
    p = next;
  }
}

void crispy_imap_free_disposition(ImapDisposition *d) {
  if (!d) return;
  free(d->type);
  crispy_imap_free_params(d->params);
  d->type = NULL;
  d->params = NULL;
}

void crispy_imap_free_addresses(ImapAddress *a) {
  while (a) {
    ImapAddress *next = a->next;
    free(a->name);
    free(a->adl);
    free(a->mailbox);
    free(a->host);
    free(a);
    a = next;
  }
}

void crispy_imap_free_envelope(ImapEnvelope *e) {
  if (!e) return;
  free(e->date);
  free(e->subject);
  crispy_imap_free_addresses(e->from);
  crispy_imap_free_addresses(e->sender);
  crispy_imap_free_addresses(e->reply_to);
  crispy_imap_free_addresses(e->to);
  crispy_imap_free_addresses(e->cc);
  crispy_imap_free_addresses(e->bcc);
  free(e->in_reply_to);
  free(e->message_id);
  free(e);
}

void crispy_imap_free_bodypart(ImapBodyPart *bp) {
  if (!bp) return;
  free(bp->subtype);
  crispy_imap_free_params(bp->params);
  free(bp->id);
  free(bp->description);
  free(bp->md5);
  crispy_imap_free_disposition(&bp->disposition);
  free(bp->language);
  free(bp->location);
  crispy_imap_free_envelope(bp->nested_env);
  crispy_imap_free_bodypart(bp->nested_body);
  /* Free siblings */
  ImapBodyPart *child = bp->subparts;
  while (child) {
    ImapBodyPart *next = child->next;
    crispy_imap_free_bodypart(child);
    child = next;
  }
  free(bp);
}

void crispy_imap_fetch_result_free(ImapFetchResult *r) {
  if (!r) return;
  free(r->internaldate);
  crispy_imap_free_envelope(r->envelope);
  crispy_imap_free_bodypart(r->bodystructure);
  free(r->headers);
  free(r->body);
  free(r->full);
  memset(r, 0, sizeof(*r));
}

/* ================================================================
 * Connection
 * ================================================================ */

/* ================================================================
 * Modified UTF-7 (RFC 3501 section 5.1.3)
 *
 * IMAP uses a modified form of UTF-7 for mailbox names:
 *   - Printable ASCII (0x20-0x7e) except '&' is literal
 *   - '&' is encoded as "&-"
 *   - Non-ASCII is encoded as &<modified-base64>-
 *   - Modified base64 uses ',' instead of '/' and has no padding
 * ================================================================ */

static const char mutf7_b64[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,";

static int mutf7_b64_val(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == ',') return 63;
  return -1;
}

char *crispy_imap_utf8_to_mutf7(const char *utf8) {
  if (!utf8) return NULL;
  size_t len = strlen(utf8);
  /* Worst case: every char needs encoding */
  char *out = (char *)malloc(len * 4 + 1);
  if (!out) return NULL;
  size_t o = 0;

  size_t i = 0;
  while (i < len) {
    unsigned char c = (unsigned char)utf8[i];
    if (c >= 0x20 && c <= 0x7e && c != '&') {
      out[o++] = c;
      i++;
    } else if (c == '&') {
      out[o++] = '&'; out[o++] = '-';
      i++;
    } else {
      /* Collect non-ASCII run, convert to UTF-16BE, then modified base64 */
      out[o++] = '&';
      unsigned short utf16[256];
      int u16count = 0;
      while (i < len) {
        unsigned char ch = (unsigned char)utf8[i];
        if (ch >= 0x20 && ch <= 0x7e) break;
        unsigned int codepoint = 0;
        if (ch < 0x80) { codepoint = ch; i++; }
        else if ((ch & 0xE0) == 0xC0) {
          codepoint = (ch & 0x1F) << 6;
          if (i + 1 < len) codepoint |= ((unsigned char)utf8[i+1] & 0x3F);
          i += 2;
        } else if ((ch & 0xF0) == 0xE0) {
          codepoint = (ch & 0x0F) << 12;
          if (i + 1 < len) codepoint |= ((unsigned char)utf8[i+1] & 0x3F) << 6;
          if (i + 2 < len) codepoint |= ((unsigned char)utf8[i+2] & 0x3F);
          i += 3;
        } else if ((ch & 0xF8) == 0xF0) {
          codepoint = (ch & 0x07) << 18;
          if (i + 1 < len) codepoint |= ((unsigned char)utf8[i+1] & 0x3F) << 12;
          if (i + 2 < len) codepoint |= ((unsigned char)utf8[i+2] & 0x3F) << 6;
          if (i + 3 < len) codepoint |= ((unsigned char)utf8[i+3] & 0x3F);
          i += 4;
        } else { i++; continue; }

        if (codepoint < 0x10000) {
          if (u16count < 256) utf16[u16count++] = (unsigned short)codepoint;
        } else {
          /* Surrogate pair */
          codepoint -= 0x10000;
          if (u16count < 255) {
            utf16[u16count++] = 0xD800 | (codepoint >> 10);
            utf16[u16count++] = 0xDC00 | (codepoint & 0x3FF);
          }
        }
      }
      /* Encode UTF-16BE bytes as modified base64 */
      unsigned char bytes[512];
      int bcount = 0;
      for (int j = 0; j < u16count && bcount < 510; j++) {
        bytes[bcount++] = (utf16[j] >> 8) & 0xFF;
        bytes[bcount++] = utf16[j] & 0xFF;
      }
      /* Base64 encode without padding */
      int bi = 0;
      while (bi < bcount) {
        int b0 = bytes[bi++];
        int b1 = (bi < bcount) ? bytes[bi++] : -1;
        int b2 = (bi < bcount) ? bytes[bi++] : -1;
        out[o++] = mutf7_b64[b0 >> 2];
        out[o++] = mutf7_b64[((b0 & 3) << 4) | (b1 >= 0 ? (b1 >> 4) : 0)];
        if (b1 >= 0) out[o++] = mutf7_b64[((b1 & 0xF) << 2) | (b2 >= 0 ? (b2 >> 6) : 0)];
        if (b2 >= 0) out[o++] = mutf7_b64[b2 & 0x3F];
      }
      out[o++] = '-';
    }
  }
  out[o] = '\0';
  return out;
}

char *crispy_imap_mutf7_to_utf8(const char *mutf7) {
  if (!mutf7) return NULL;
  size_t len = strlen(mutf7);
  char *out = (char *)malloc(len * 4 + 1);
  if (!out) return NULL;
  size_t o = 0;

  size_t i = 0;
  while (i < len) {
    if (mutf7[i] != '&') {
      out[o++] = mutf7[i++];
    } else {
      i++; /* skip '&' */
      if (i < len && mutf7[i] == '-') {
        out[o++] = '&';
        i++;
      } else {
        /* Decode modified base64 until '-' */
        unsigned char bytes[512];
        int bcount = 0;
        int accum = 0, bits = 0;
        while (i < len && mutf7[i] != '-') {
          int val = mutf7_b64_val(mutf7[i]);
          if (val < 0) { i++; continue; }
          accum = (accum << 6) | val;
          bits += 6;
          if (bits >= 8) {
            bits -= 8;
            if (bcount < 512) bytes[bcount++] = (accum >> bits) & 0xFF;
          }
          i++;
        }
        if (i < len && mutf7[i] == '-') i++;

        /* Decode UTF-16BE to UTF-8 */
        for (int j = 0; j + 1 < bcount; j += 2) {
          unsigned int cp = (bytes[j] << 8) | bytes[j+1];
          if (cp >= 0xD800 && cp <= 0xDBFF && j + 3 < bcount) {
            unsigned int lo = (bytes[j+2] << 8) | bytes[j+3];
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
              cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
              j += 2;
            }
          }
          if (cp < 0x80) {
            out[o++] = (char)cp;
          } else if (cp < 0x800) {
            out[o++] = 0xC0 | (cp >> 6);
            out[o++] = 0x80 | (cp & 0x3F);
          } else if (cp < 0x10000) {
            out[o++] = 0xE0 | (cp >> 12);
            out[o++] = 0x80 | ((cp >> 6) & 0x3F);
            out[o++] = 0x80 | (cp & 0x3F);
          } else {
            out[o++] = 0xF0 | (cp >> 18);
            out[o++] = 0x80 | ((cp >> 12) & 0x3F);
            out[o++] = 0x80 | ((cp >> 6) & 0x3F);
            out[o++] = 0x80 | (cp & 0x3F);
          }
        }
      }
    }
  }
  out[o] = '\0';
  return out;
}

/* ================================================================
 * Connection
 * ================================================================ */

void crispy_imap_init(ImapSession *s, ImapTransport tp) {
  memset(s, 0, sizeof(*s));
  s->tp = tp;
}

int crispy_imap_connect(ImapSession *s, const char *host, int port,
                        ImapSecurity security) {
  if (!s->tp.connect) return IMAP_ERR;

  /* For implicit TLS (port 993), upgrade before any IMAP traffic */
  if (security == IMAP_SSL && s->tp.start_tls) {
    /* Connect TCP first */
    int err = s->tp.connect(s->tp.ctx, host, port);
    if (err) return IMAP_ERR;
    /* Upgrade to TLS immediately */
    err = s->tp.start_tls(s->tp.ctx);
    if (err) return IMAP_ERR;
  } else {
    int err = s->tp.connect(s->tp.ctx, host, port);
    if (err) return IMAP_ERR;
  }
  s->connected = true;
  s->bye_received = false;

  /* Save connection params for reconnect */
  snprintf(s->reconnect_host, sizeof(s->reconnect_host), "%s", host);
  s->reconnect_port = port;
  s->reconnect_security = security;

  /* Read greeting */
  char *greeting = crispy_imap_readline(s);
  if (!greeting) return IMAP_ERR;
  if (strncmp(greeting, "* OK", 4) != 0 && strncmp(greeting, "* PREAUTH", 9) != 0) {
    free(greeting);
    return IMAP_ERR;
  }
  if (strncmp(greeting, "* PREAUTH", 9) == 0) s->authenticated = true;
  if (strcasestr(greeting, "CAPABILITY")) parse_capabilities(s, greeting);
  free(greeting);

  /* Always send CAPABILITY to get full capability set */
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, "CAPABILITY", &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "CAPABILITY ", 11) == 0)
        parse_capabilities(s, lines[i]);
    }
    /* Also check the tagged OK reply for capabilities */
    parse_capabilities(s, s->reply);
  }
  free_collected(lines, lineCount);

  /* STARTTLS if requested */
  if (security == IMAP_STARTTLS) {
    if (!s->cap_starttls) {
      dbg(s, "Server does not support STARTTLS");
      return IMAP_ERR;
    }
    res = crispy_imap_command(s, "STARTTLS");
    if (res != IMAP_OK) return res;
    if (!s->tp.start_tls) return IMAP_ERR;
    int err = s->tp.start_tls(s->tp.ctx);
    if (err) return IMAP_ERR;
    /* Re-CAPABILITY after TLS — capabilities may change */
    lines = NULL; lineCount = 0;
    res = crispy_imap_command_collect(s, "CAPABILITY", &lines, &lineCount);
    if (res == IMAP_OK) {
      /* Reset caps before re-parsing */
      s->cap_starttls = false; s->cap_idle = false;
      s->cap_uidplus = false; s->cap_move = false;
      s->cap_literal_plus = false; s->cap_sort = false;
      s->cap_thread = false; s->cap_namespace = false;
      s->cap_condstore = false; s->cap_children = false;
      s->cap_auth[0] = '\0';
      for (int i = 0; i < lineCount; i++) {
        if (strncasecmp(lines[i], "CAPABILITY ", 11) == 0)
          parse_capabilities(s, lines[i]);
      }
      parse_capabilities(s, s->reply);
    }
    free_collected(lines, lineCount);
  }

  return IMAP_OK;
}

int crispy_imap_login(ImapSession *s, const char *user, const char *pass) {
  /* Quote user and pass — escape any backslashes and quotes */
  char quotedUser[512], quotedPass[512];
  size_t j = 0;
  for (size_t i = 0; user[i] && j < sizeof(quotedUser) - 2; i++) {
    if (user[i] == '"' || user[i] == '\\') quotedUser[j++] = '\\';
    quotedUser[j++] = user[i];
  }
  quotedUser[j] = '\0';

  j = 0;
  for (size_t i = 0; pass[i] && j < sizeof(quotedPass) - 2; i++) {
    if (pass[i] == '"' || pass[i] == '\\') quotedPass[j++] = '\\';
    quotedPass[j++] = pass[i];
  }
  quotedPass[j] = '\0';

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "LOGIN \"%s\" \"%s\"", quotedUser, quotedPass);
  int res = crispy_imap_command(s, cmd);
  if (res == IMAP_OK) {
    s->authenticated = true;
    /* Server may send updated capabilities after login */
    if (strcasestr(s->reply, "CAPABILITY")) parse_capabilities(s, s->reply);
  }
  return res;
}

int crispy_imap_auth_plain(ImapSession *s, const char *user, const char *pass) {
  /* PLAIN SASL: \0user\0pass */
  size_t uLen = strlen(user), pLen = strlen(pass);
  size_t rawLen = 1 + uLen + 1 + pLen;
  char *raw = (char *)malloc(rawLen);
  if (!raw) return IMAP_ERR;
  raw[0] = '\0';
  memcpy(raw + 1, user, uLen);
  raw[1 + uLen] = '\0';
  memcpy(raw + 2 + uLen, pass, pLen);

  long b64Len;
  char *b64 = crispy_base64_encode(raw, (long)rawLen, &b64Len);
  free(raw);
  if (!b64) return IMAP_ERR;

  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "AUTHENTICATE PLAIN %s", b64);
  free(b64);

  int res = crispy_imap_command(s, cmd);
  if (res == IMAP_OK) s->authenticated = true;
  return res;
}

int crispy_imap_auth_xoauth2(ImapSession *s, const char *user, const char *token) {
  /* XOAUTH2: user=<user>\x01auth=Bearer <token>\x01\x01 */
  size_t uLen = strlen(user), tLen = strlen(token);
  size_t rawLen = 5 + uLen + 1 + 12 + tLen + 2;
  char *raw = (char *)malloc(rawLen + 1);
  if (!raw) return IMAP_ERR;
  int pos = 0;
  memcpy(raw + pos, "user=", 5); pos += 5;
  memcpy(raw + pos, user, uLen); pos += uLen;
  raw[pos++] = '\x01';
  memcpy(raw + pos, "auth=Bearer ", 12); pos += 12;
  memcpy(raw + pos, token, tLen); pos += tLen;
  raw[pos++] = '\x01'; raw[pos++] = '\x01';

  long b64Len;
  char *b64 = crispy_base64_encode(raw, (long)pos, &b64Len);
  free(raw);
  if (!b64) return IMAP_ERR;

  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "AUTHENTICATE XOAUTH2 %s", b64);
  free(b64);

  int res = crispy_imap_command(s, cmd);
  if (res == IMAP_OK) s->authenticated = true;
  return res;
}

int crispy_imap_auth_cram_md5(ImapSession *s, const char *user, const char *pass) {
  /* Step 1: initiate CRAM-MD5 */
  next_tag(s);
  char full[256];
  snprintf(full, sizeof(full), "%s AUTHENTICATE CRAM-MD5", s->tag);
  if (send_line(s, full)) return IMAP_ERR;

  /* Server sends + <base64 challenge> */
  char *contLine = crispy_imap_readline(s);
  if (!contLine || contLine[0] != '+') { free(contLine); return IMAP_ERR; }
  char *chalB64 = contLine + 1;
  while (*chalB64 == ' ') chalB64++;

  /* Decode challenge */
  long chalLen;
  char *challenge = crispy_base64_decode(chalB64, (long)strlen(chalB64), &chalLen);
  free(contLine);
  if (!challenge) return IMAP_ERR;

  /* HMAC-MD5(password, challenge) */
  char hex[33];
  crispy_hmac_md5_hex(pass, strlen(pass), challenge, chalLen, hex);
  free(challenge);

  /* Build "user hex" and base64-encode */
  char response[512];
  snprintf(response, sizeof(response), "%s %s", user, hex);
  long b64Len;
  char *b64 = crispy_base64_encode(response, (long)strlen(response), &b64Len);
  if (!b64) return IMAP_ERR;

  /* Send response */
  if (send_line(s, b64)) { free(b64); return IMAP_ERR; }
  free(b64);

  /* Read tagged response */
  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      int res;
      if (strncasecmp(line + tagLen + 1, "OK", 2) == 0) res = IMAP_OK;
      else res = IMAP_NO;
      free(line);
      if (res == IMAP_OK) s->authenticated = true;
      return res;
    }
    free(line);
  }
}

void crispy_imap_close(ImapSession *s) {
  if (s->connected) {
    crispy_imap_command(s, "LOGOUT");
    if (s->tp.close) s->tp.close(s->tp.ctx);
    s->connected = false;
  }
  if (s->tp.destroy) { s->tp.destroy(s->tp.ctx); s->tp.ctx = NULL; }
}

/* ================================================================
 * Mailbox operations
 * ================================================================ */

/* Shared parser for LIST and LSUB responses */
static int parse_list_response(const char *prefix, char **lines, int lineCount,
                                ImapListEntry **entries, int *count) {
  size_t prefixLen = strlen(prefix);
  *entries = NULL;
  *count = 0;
  if (lineCount <= 0) return IMAP_OK;

  *entries = (ImapListEntry *)calloc(lineCount, sizeof(ImapListEntry));

  for (int i = 0; i < lineCount; i++) {
    char *l = lines[i];
    if (strncasecmp(l, prefix, prefixLen) != 0) continue;
    l += prefixLen;

    ImapListEntry *e = &(*entries)[*count];

    /* Parse attributes: (\Noselect \HasChildren ...) */
    if (*l == '(') {
      l++;
      char *end = strchr(l, ')');
      if (end) {
        char attrs[512];
        size_t alen = end - l;
        if (alen >= sizeof(attrs)) alen = sizeof(attrs) - 1;
        memcpy(attrs, l, alen); attrs[alen] = '\0';
        if (strcasestr(attrs, "\\Noselect"))      e->noselect = true;
        if (strcasestr(attrs, "\\Noinferiors"))    e->noinferiors = true;
        if (strcasestr(attrs, "\\HasChildren"))    e->has_children = true;
        if (strcasestr(attrs, "\\HasNoChildren"))  e->has_no_children = true;
        if (strcasestr(attrs, "\\Marked"))         e->marked = true;
        if (strcasestr(attrs, "\\Unmarked"))       e->unmarked = true;
        l = end + 1;
      }
    }

    /* Parse delimiter */
    while (*l == ' ') l++;
    if (*l == '"') {
      l++;
      e->delimiter = *l;
      l++;
      if (*l == '"') l++;
    } else if (strncasecmp(l, "NIL", 3) == 0) {
      e->delimiter = '\0';
      l += 3;
    }

    /* Parse name */
    while (*l == ' ') l++;
    if (*l == '"') {
      l++;
      char *end = strchr(l, '"');
      if (end) {
        size_t nlen = end - l;
        if (nlen >= sizeof(e->name)) nlen = sizeof(e->name) - 1;
        memcpy(e->name, l, nlen); e->name[nlen] = '\0';
      }
    } else {
      /* Unquoted name — take everything to end of line */
      size_t nlen = strlen(l);
      if (nlen >= sizeof(e->name)) nlen = sizeof(e->name) - 1;
      memcpy(e->name, l, nlen); e->name[nlen] = '\0';
      /* Trim trailing whitespace */
      while (nlen > 0 && (e->name[nlen-1] == ' ' || e->name[nlen-1] == '\r'))
        e->name[--nlen] = '\0';
    }

    (*count)++;
  }
  return IMAP_OK;
}

int crispy_imap_list(ImapSession *s, const char *ref, const char *pattern,
                     ImapListEntry **entries, int *count) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "LIST \"%s\" \"%s\"",
           ref ? ref : "", pattern ? pattern : "*");

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  *entries = NULL; *count = 0;
  if (res == IMAP_OK)
    parse_list_response("LIST ", lines, lineCount, entries, count);

  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_lsub(ImapSession *s, const char *ref, const char *pattern,
                     ImapListEntry **entries, int *count) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "LSUB \"%s\" \"%s\"",
           ref ? ref : "", pattern ? pattern : "*");

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  *entries = NULL; *count = 0;
  if (res == IMAP_OK)
    parse_list_response("LSUB ", lines, lineCount, entries, count);

  free_collected(lines, lineCount);
  return res;
}

static int select_or_examine(ImapSession *s, const char *mailbox, bool rdonly) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "%s \"%s\"", rdonly ? "EXAMINE" : "SELECT", mailbox);

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  if (res == IMAP_OK) {
    crispy_imap_cache_clear(s);
    memset(&s->selected, 0, sizeof(s->selected));
    snprintf(s->selected.name, sizeof(s->selected.name), "%s", mailbox);
    s->selected.read_only = rdonly;

    for (int i = 0; i < lineCount; i++) {
      char *l = lines[i];
      long num;
      if (sscanf(l, "%ld EXISTS", &num) == 1) {
        s->selected.exists = num;
      } else if (sscanf(l, "%ld RECENT", &num) == 1) {
        s->selected.recent = num;
      } else if (strstr(l, "UIDVALIDITY")) {
        char *p = strstr(l, "UIDVALIDITY");
        p += 11;
        while (*p == ' ' || *p == ']') p++;
        s->selected.uidvalidity = strtoul(p, NULL, 10);
      } else if (strstr(l, "UIDNEXT")) {
        char *p = strstr(l, "UIDNEXT");
        p += 7;
        while (*p == ' ' || *p == ']') p++;
        s->selected.uidnext = strtoul(p, NULL, 10);
      } else if (strstr(l, "UNSEEN")) {
        char *p = strstr(l, "UNSEEN");
        p += 6;
        while (*p == ' ' || *p == ']') p++;
        s->selected.unseen = strtol(p, NULL, 10);
      }
      /* PERMANENTFLAGS */
      if (strcasestr(l, "PERMANENTFLAGS")) {
        char *pf = strcasestr(l, "PERMANENTFLAGS");
        if (pf) {
          if (strcasestr(pf, "\\Seen"))     s->selected.perm_seen = true;
          if (strcasestr(pf, "\\Answered")) s->selected.perm_answered = true;
          if (strcasestr(pf, "\\Flagged"))  s->selected.perm_flagged = true;
          if (strcasestr(pf, "\\Deleted"))  s->selected.perm_deleted = true;
          if (strcasestr(pf, "\\Draft"))    s->selected.perm_draft = true;
          if (strcasestr(pf, "\\*"))        s->selected.perm_custom = true;
        }
      }
    }
    /* Check if read-only from tagged reply */
    if (strcasestr(s->reply, "[READ-ONLY]")) s->selected.read_only = true;
  }
  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_select(ImapSession *s, const char *mailbox) {
  return select_or_examine(s, mailbox, false);
}

int crispy_imap_examine(ImapSession *s, const char *mailbox) {
  return select_or_examine(s, mailbox, true);
}

int crispy_imap_status(ImapSession *s, const char *mailbox,
                       long *messages, long *recent, long *unseen,
                       unsigned long *uidvalidity, unsigned long *uidnext) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd),
           "STATUS \"%s\" (MESSAGES RECENT UNSEEN UIDVALIDITY UIDNEXT)", mailbox);
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      char *l = lines[i];
      char *p;
      if ((p = strcasestr(l, "MESSAGES")))    { p += 8;  while (*p == ' ') p++; if (messages) *messages = strtol(p, NULL, 10); }
      if ((p = strcasestr(l, "RECENT")))      { p += 6;  while (*p == ' ') p++; if (recent) *recent = strtol(p, NULL, 10); }
      if ((p = strcasestr(l, "UNSEEN")))      { p += 6;  while (*p == ' ') p++; if (unseen) *unseen = strtol(p, NULL, 10); }
      if ((p = strcasestr(l, "UIDVALIDITY"))) { p += 11; while (*p == ' ') p++; if (uidvalidity) *uidvalidity = strtoul(p, NULL, 10); }
      if ((p = strcasestr(l, "UIDNEXT")))     { p += 7;  while (*p == ' ') p++; if (uidnext) *uidnext = strtoul(p, NULL, 10); }
    }
  }
  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_create(ImapSession *s, const char *mailbox) {
  return send_with_literal(s, "CREATE", mailbox);
}

int crispy_imap_delete(ImapSession *s, const char *mailbox) {
  return send_with_literal(s, "DELETE", mailbox);
}

int crispy_imap_rename(ImapSession *s, const char *from, const char *to) {
  if (needs_literal(from) || needs_literal(to)) {
    /* Both may need literals — use raw command for simplicity */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "RENAME \"%s\" \"%s\"", from, to);
    return crispy_imap_command(s, cmd);
  }
  char cmd[512]; snprintf(cmd, sizeof(cmd), "RENAME \"%s\" \"%s\"", from, to);
  return crispy_imap_command(s, cmd);
}

int crispy_imap_subscribe(ImapSession *s, const char *mailbox) {
  return send_with_literal(s, "SUBSCRIBE", mailbox);
}

int crispy_imap_unsubscribe(ImapSession *s, const char *mailbox) {
  return send_with_literal(s, "UNSUBSCRIBE", mailbox);
}

int crispy_imap_check(ImapSession *s) {
  return crispy_imap_command(s, "CHECK");
}

int crispy_imap_close_mailbox(ImapSession *s) {
  int res = crispy_imap_command(s, "CLOSE");
  if (res == IMAP_OK) memset(&s->selected, 0, sizeof(s->selected));
  return res;
}

/* ================================================================
 * NAMESPACE (RFC 2342)
 * Response: * NAMESPACE ((prefix delim)) ((prefix delim)) ((prefix delim))
 * Three groups: personal, other users, shared — each NIL or list.
 * ================================================================ */

static void parse_namespace_group(const char **p, ImapNamespaceEntry *entries,
                                   int *count, int max) {
  *count = 0;
  skip_ws(p);
  if (strncasecmp(*p, "NIL", 3) == 0 &&
      ((*p)[3] == ' ' || (*p)[3] == ')' || (*p)[3] == '\0')) {
    *p += 3;
    return;
  }
  if (**p != '(') return;
  (*p)++; /* outer ( */
  while (**p && **p != ')' && *count < max) {
    skip_ws(p);
    if (**p != '(') break;
    (*p)++; /* inner ( */
    char *prefix = parse_nstring(p);
    char *delim = parse_nstring(p);
    if (prefix) {
      snprintf(entries[*count].prefix, sizeof(entries[*count].prefix), "%s", prefix);
      entries[*count].delimiter = (delim && delim[0]) ? delim[0] : '\0';
      (*count)++;
    }
    free(prefix);
    free(delim);
    /* Skip any extension data within this namespace entry */
    skip_ws(p);
    while (**p && **p != ')') skip_sexp(p);
    if (**p == ')') (*p)++;
    skip_ws(p);
  }
  if (**p == ')') (*p)++; /* close outer */
}

int crispy_imap_namespace(ImapSession *s, ImapNamespace *ns) {
  if (!s->cap_namespace) {
    dbg(s, "Server does not support NAMESPACE");
    return IMAP_NO;
  }
  memset(ns, 0, sizeof(*ns));

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, "NAMESPACE", &lines, &lineCount);

  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "NAMESPACE ", 10) == 0) {
        const char *cursor = lines[i] + 10;
        parse_namespace_group(&cursor, ns->personal, &ns->personal_count, 8);
        parse_namespace_group(&cursor, ns->other, &ns->other_count, 8);
        parse_namespace_group(&cursor, ns->shared, &ns->shared_count, 8);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return res;
}

/* ================================================================
 * Literal argument sending — for mailbox names with special chars.
 * If a string contains chars that can't be safely quoted
 * (\r, \n, ", \), send it as a literal {NNN}\r\n<data>.
 * ================================================================ */

static bool needs_literal(const char *str) {
  for (const char *p = str; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (c < 0x20 || c == '"' || c == '\\' || c == '{' || c > 0x7e)
      return true;
  }
  return false;
}

/* Send a command that includes a mailbox name which may need literal encoding.
 * fmt should contain exactly one %s for the mailbox name.
 * Example: send_with_literal(s, "SELECT %s", "My \"Folder\"") */
static int send_with_literal(ImapSession *s, const char *prefix,
                              const char *arg) {
  if (!needs_literal(arg)) {
    /* Safe to quote normally */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s \"%s\"", prefix, arg);
    return crispy_imap_command(s, cmd);
  }
  /* Send as literal */
  next_tag(s);
  long argLen = (long)strlen(arg);
  char full[512];
  snprintf(full, sizeof(full), "%s %s {%ld}", s->tag, prefix, argLen);
  if (send_line(s, full)) return IMAP_ERR;

  /* Wait for continuation */
  char *cont = crispy_imap_readline(s);
  if (!cont || cont[0] != '+') { free(cont); return IMAP_ERR; }
  free(cont);

  /* Send literal data */
  tp_send(s, arg, argLen);
  tp_send(s, "\r\n", 2);

  /* Read tagged response */
  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      parse_response_code(s, line + tagLen + 1);
      int res;
      if (strncasecmp(line + tagLen + 1, "OK", 2) == 0) res = IMAP_OK;
      else if (strncasecmp(line + tagLen + 1, "NO", 2) == 0) res = IMAP_NO;
      else res = IMAP_BAD;
      free(line);
      return res;
    }
    if (line[0] == '*' && line[1] == ' ')
      track_mailbox_updates(s, line + 2);
    free(line);
  }
}

/* ================================================================
 * ID (RFC 2971) — client/server identification
 * ================================================================ */

int crispy_imap_id(ImapSession *s, const char *name, const char *version) {
  char cmd[512];
  if (name && version)
    snprintf(cmd, sizeof(cmd), "ID (\"name\" \"%s\" \"version\" \"%s\")", name, version);
  else
    snprintf(cmd, sizeof(cmd), "ID NIL");
  return crispy_imap_command(s, cmd);
}

/* ================================================================
 * QUOTA (RFC 2087)
 * ================================================================ */

static void parse_quota_response(const char *line, ImapQuota *q) {
  /* Format: QUOTA "root" (STORAGE usage limit) */
  const char *p = strcasestr(line, "QUOTA ");
  if (!p) return;
  p += 6;
  /* Parse root name */
  if (*p == '"') {
    p++;
    const char *end = strchr(p, '"');
    if (end) {
      size_t len = end - p;
      if (len >= sizeof(q->root)) len = sizeof(q->root) - 1;
      memcpy(q->root, p, len); q->root[len] = '\0';
      p = end + 1;
    }
  }
  /* Parse (STORAGE usage limit) */
  char *stor = strcasestr(p, "STORAGE ");
  if (stor) {
    stor += 8;
    q->usage = strtoul(stor, (char **)&stor, 10);
    while (*stor == ' ') stor++;
    q->limit = strtoul(stor, NULL, 10);
  }
}

int crispy_imap_getquotaroot(ImapSession *s, const char *mailbox,
                              ImapQuota *quota) {
  memset(quota, 0, sizeof(*quota));
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "GETQUOTAROOT \"%s\"", mailbox);
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "QUOTA ", 6) == 0)
        parse_quota_response(lines[i], quota);
    }
  }
  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_getquota(ImapSession *s, const char *root,
                          ImapQuota *quota) {
  memset(quota, 0, sizeof(*quota));
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "GETQUOTA \"%s\"", root);
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "QUOTA ", 6) == 0)
        parse_quota_response(lines[i], quota);
    }
  }
  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_setquota(ImapSession *s, const char *root,
                          unsigned long limit_kb) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "SETQUOTA \"%s\" (STORAGE %lu)", root, limit_kb);
  return crispy_imap_command(s, cmd);
}

/* ================================================================
 * ACL (RFC 4314)
 * ================================================================ */

int crispy_imap_getacl(ImapSession *s, const char *mailbox,
                        ImapACLEntry **entries, int *count) {
  *entries = NULL; *count = 0;
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "GETACL \"%s\"", mailbox);
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "ACL ", 4) == 0) {
        /* Format: ACL "mailbox" identifier1 rights1 identifier2 rights2 ... */
        char *p = lines[i] + 4;
        /* Skip mailbox name */
        if (*p == '"') { p++; p = strchr(p, '"'); if (p) p++; }
        else { while (*p && *p != ' ') p++; }
        /* Count pairs */
        int cap = 16;
        *entries = (ImapACLEntry *)calloc(cap, sizeof(ImapACLEntry));
        while (p && *p) {
          while (*p == ' ') p++;
          if (!*p) break;
          /* identifier */
          char *idStart = p;
          if (*p == '"') {
            p++; char *end = strchr(p, '"');
            if (end) { p = end + 1; }
          } else {
            while (*p && *p != ' ') p++;
          }
          size_t idLen = p - idStart;
          /* rights */
          while (*p == ' ') p++;
          char *rStart = p;
          while (*p && *p != ' ') p++;

          if (*count >= cap) {
            cap *= 2;
            *entries = (ImapACLEntry *)realloc(*entries, cap * sizeof(ImapACLEntry));
          }
          ImapACLEntry *e = &(*entries)[*count];
          /* Copy identifier (strip quotes) */
          if (*idStart == '"') { idStart++; idLen -= 2; }
          if (idLen >= sizeof(e->identifier)) idLen = sizeof(e->identifier) - 1;
          memcpy(e->identifier, idStart, idLen); e->identifier[idLen] = '\0';
          /* Copy rights */
          size_t rLen = p - rStart;
          if (rLen >= sizeof(e->rights)) rLen = sizeof(e->rights) - 1;
          memcpy(e->rights, rStart, rLen); e->rights[rLen] = '\0';
          (*count)++;
        }
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_setacl(ImapSession *s, const char *mailbox,
                        const char *identifier, const char *rights) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "SETACL \"%s\" \"%s\" %s", mailbox, identifier, rights);
  return crispy_imap_command(s, cmd);
}

int crispy_imap_deleteacl(ImapSession *s, const char *mailbox,
                           const char *identifier) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "DELETEACL \"%s\" \"%s\"", mailbox, identifier);
  return crispy_imap_command(s, cmd);
}

int crispy_imap_listrights(ImapSession *s, const char *mailbox,
                            const char *identifier,
                            char *rights, size_t rightsSize) {
  rights[0] = '\0';
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "LISTRIGHTS \"%s\" \"%s\"", mailbox, identifier);
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "LISTRIGHTS ", 11) == 0) {
        /* Format: LISTRIGHTS "mailbox" "identifier" required optional... */
        char *p = lines[i] + 11;
        /* Skip mailbox and identifier */
        for (int skip = 0; skip < 2; skip++) {
          while (*p == ' ') p++;
          if (*p == '"') { p++; p = strchr(p, '"'); if (p) p++; }
          else { while (*p && *p != ' ') p++; }
        }
        while (*p == ' ') p++;
        snprintf(rights, rightsSize, "%s", p);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return res;
}

int crispy_imap_myrights(ImapSession *s, const char *mailbox,
                          char *rights, size_t rightsSize) {
  rights[0] = '\0';
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "MYRIGHTS \"%s\"", mailbox);
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "MYRIGHTS ", 9) == 0) {
        /* Format: MYRIGHTS "mailbox" rights */
        char *p = lines[i] + 9;
        /* Skip mailbox name */
        if (*p == '"') { p++; p = strchr(p, '"'); if (p) p++; }
        else { while (*p && *p != ' ') p++; }
        while (*p == ' ') p++;
        snprintf(rights, rightsSize, "%s", p);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return res;
}

/* ================================================================
 * Literal fetch helper — sends a FETCH command and reads the
 * response including {NNN} literal data. Returns malloc'd buffer.
 * ================================================================ */

static char *fetch_literal_response(ImapSession *s, const char *cmd,
                                     long *outLen) {
  next_tag(s);
  char full[512];
  snprintf(full, sizeof(full), "%s %s", s->tag, cmd);
  if (send_line(s, full)) return NULL;

  char *result = NULL;
  long resultLen = 0;

  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) break;

    /* Check for literal {NNN} in untagged FETCH response */
    if (line[0] == '*') {
      char *lbrace = strrchr(line, '{');
      if (lbrace) {
        char *rbrace = strchr(lbrace, '}');
        if (rbrace) {
          long litSize = strtol(lbrace + 1, NULL, 10);
          free(line);
          if (litSize > 0) {
            result = read_literal(s, litSize);
            resultLen = litSize;
            /* Read closing paren/continuation line */
            char *close = crispy_imap_readline(s);
            free(close);
          }
          continue;
        }
      }
    }

    /* Tagged response — done */
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      free(line);
      break;
    }
    free(line);
  }

  if (outLen) *outLen = resultLen;
  return result;
}

/* ================================================================
 * Message fetch operations
 * ================================================================ */

int crispy_imap_fetch_uids(ImapSession *s, unsigned long **uids) {
  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, "UID SEARCH ALL", &lines, &lineCount);

  *uids = NULL;
  int count = 0;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "SEARCH", 6) == 0) {
        char *p = lines[i] + 6;
        int cap = 128;
        *uids = (unsigned long *)calloc(cap, sizeof(unsigned long));
        while (*p) {
          while (*p == ' ') p++;
          if (!*p) break;
          unsigned long uid = strtoul(p, &p, 10);
          if (uid == 0) break;
          if (count >= cap) {
            cap *= 2;
            *uids = (unsigned long *)realloc(*uids, cap * sizeof(unsigned long));
          }
          (*uids)[count++] = uid;
        }
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return count;
}

int crispy_imap_fetch_flags(ImapSession *s, const char *uid_set,
                            unsigned long **uids, ImapFlags **flags,
                            int *count) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "UID FETCH %s (UID FLAGS)", uid_set);

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  *uids = NULL; *flags = NULL; *count = 0;
  if (res == IMAP_OK && lineCount > 0) {
    *uids = (unsigned long *)calloc(lineCount, sizeof(unsigned long));
    *flags = (ImapFlags *)calloc(lineCount, sizeof(ImapFlags));
    for (int i = 0; i < lineCount; i++) {
      char *uidp = strcasestr(lines[i], "UID ");
      char *flagp = strcasestr(lines[i], "FLAGS ");
      if (uidp) {
        uidp += 4;
        (*uids)[*count] = strtoul(uidp, NULL, 10);
      }
      if (flagp) (*flags)[*count] = parse_flags(flagp);
      if (uidp) (*count)++;
    }
  }
  free_collected(lines, lineCount);
  return res;
}

/* Helper: does server support BODY.PEEK[]? (IMAP4rev1+) */
static bool has_body_peek(ImapSession *s) {
  return s->level >= IMAP_LEVEL_IMAP4REV1 || s->level == IMAP_LEVEL_UNKNOWN;
}

char *crispy_imap_fetch_message(ImapSession *s, unsigned long uid,
                                long *outLen) {
  char cmd[256];
  if (has_body_peek(s))
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu BODY.PEEK[]", uid);
  else
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu RFC822", uid);
  return fetch_literal_response(s, cmd, outLen);
}

char *crispy_imap_fetch_headers(ImapSession *s, unsigned long uid) {
  char cmd[256];
  if (has_body_peek(s))
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu BODY.PEEK[HEADER]", uid);
  else
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu RFC822.HEADER", uid);
  long len = 0;
  return fetch_literal_response(s, cmd, &len);
}

char *crispy_imap_fetch_header_fields(ImapSession *s, unsigned long uid,
                                       const char *fields) {
  char cmd[1024];
  if (has_body_peek(s))
    snprintf(cmd, sizeof(cmd),
             "UID FETCH %lu BODY.PEEK[HEADER.FIELDS (%s)]", uid, fields);
  else
    snprintf(cmd, sizeof(cmd),
             "UID FETCH %lu RFC822.HEADER.LINES (%s)", uid, fields);
  long len = 0;
  return fetch_literal_response(s, cmd, &len);
}

char *crispy_imap_fetch_body(ImapSession *s, unsigned long uid,
                             long *outLen) {
  char cmd[256];
  if (has_body_peek(s))
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu BODY.PEEK[TEXT]", uid);
  else
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu RFC822.TEXT", uid);
  return fetch_literal_response(s, cmd, outLen);
}

/* --- Explicit legacy fetch forms --- */

char *crispy_imap_fetch_rfc822(ImapSession *s, unsigned long uid,
                                long *outLen) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu RFC822", uid);
  return fetch_literal_response(s, cmd, outLen);
}

char *crispy_imap_fetch_rfc822_header(ImapSession *s, unsigned long uid) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu RFC822.HEADER", uid);
  long len = 0;
  return fetch_literal_response(s, cmd, &len);
}

char *crispy_imap_fetch_rfc822_text(ImapSession *s, unsigned long uid,
                                     long *outLen) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu RFC822.TEXT", uid);
  return fetch_literal_response(s, cmd, outLen);
}

ImapBodyPart *crispy_imap_fetch_body_structure_legacy(ImapSession *s,
                                                       unsigned long uid) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu (BODY)", uid);

  char **lines = NULL;
  int lineCount = 0;
  int res = fetch_collect(s, cmd, &lines, &lineCount);

  ImapBodyPart *bp = NULL;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      /* Look for "BODY (" but not "BODYSTRUCTURE" */
      char *p = lines[i];
      char *found = NULL;
      while ((p = strcasestr(p, "BODY ")) != NULL) {
        /* Make sure it's not BODYSTRUCTURE */
        if (strncasecmp(p, "BODYSTRUCTURE", 13) != 0) {
          found = p + 5;
          break;
        }
        p += 5;
      }
      if (found) {
        const char *cursor = found;
        bp = parse_bodystructure(&cursor, NULL);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return bp;
}

char *crispy_imap_fetch_section(ImapSession *s, unsigned long uid,
                                const char *section, long *outLen) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu BODY.PEEK[%s]", uid, section);
  return fetch_literal_response(s, cmd, outLen);
}

char *crispy_imap_fetch_partial(ImapSession *s, unsigned long uid,
                                 unsigned long offset, unsigned long count,
                                 long *outLen) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu BODY.PEEK[]<%lu.%lu>",
           uid, offset, count);
  return fetch_literal_response(s, cmd, outLen);
}

long crispy_imap_fetch_size(ImapSession *s, unsigned long uid) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu (RFC822.SIZE)", uid);

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  long size = -1;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      char *p = strcasestr(lines[i], "RFC822.SIZE ");
      if (p) {
        p += 12;
        size = strtol(p, NULL, 10);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return size;
}

char *crispy_imap_fetch_date(ImapSession *s, unsigned long uid) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu (INTERNALDATE)", uid);

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  char *date = NULL;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      char *p = strcasestr(lines[i], "INTERNALDATE ");
      if (p) {
        p += 13;
        while (*p == ' ') p++;
        if (*p == '"') {
          p++;
          char *end = strchr(p, '"');
          if (end) {
            size_t len = end - p;
            date = (char *)malloc(len + 1);
            if (date) { memcpy(date, p, len); date[len] = '\0'; }
          }
        }
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return date;
}

ImapEnvelope *crispy_imap_fetch_envelope(ImapSession *s, unsigned long uid) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "UID FETCH %lu (ENVELOPE)", uid);

  char **lines = NULL;
  int lineCount = 0;
  int res = fetch_collect(s, cmd, &lines, &lineCount);

  ImapEnvelope *env = NULL;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      char *p = strcasestr(lines[i], "ENVELOPE ");
      if (p) {
        p += 9;
        const char *cursor = p;
        env = parse_envelope_sexp(&cursor);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return env;
}

ImapBodyPart *crispy_imap_fetch_structure(ImapSession *s, unsigned long uid) {
  /* Use BODYSTRUCTURE for IMAP4rev1+, fallback to BODY for older */
  bool use_bodystructure = has_body_peek(s);
  char cmd[256];
  if (use_bodystructure)
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu (BODYSTRUCTURE)", uid);
  else
    snprintf(cmd, sizeof(cmd), "UID FETCH %lu (BODY)", uid);

  char **lines = NULL;
  int lineCount = 0;
  int res = fetch_collect(s, cmd, &lines, &lineCount);

  ImapBodyPart *bp = NULL;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (use_bodystructure) {
        char *p = strcasestr(lines[i], "BODYSTRUCTURE ");
        if (p) {
          p += 14;
          const char *cursor = p;
          bp = parse_bodystructure(&cursor, NULL);
          break;
        }
      } else {
        /* BODY response — find "BODY (" but not "BODYSTRUCTURE" */
        char *p = lines[i];
        while ((p = strcasestr(p, "BODY ")) != NULL) {
          if (strncasecmp(p, "BODYSTRUCTURE", 13) != 0) {
            const char *cursor = p + 5;
            bp = parse_bodystructure(&cursor, NULL);
            break;
          }
          p += 5;
        }
        if (bp) break;
      }
    }
  }
  free_collected(lines, lineCount);
  return bp;
}

/* ================================================================
 * Overview fetch — combined UID, FLAGS, SIZE, DATE, ENVELOPE
 * ================================================================ */

int crispy_imap_fetch_overview(ImapSession *s, const char *uid_set,
                                ImapFetchResult **results, int *count) {
  char cmd[1024];
  snprintf(cmd, sizeof(cmd),
           "UID FETCH %s (UID FLAGS RFC822.SIZE INTERNALDATE ENVELOPE BODYSTRUCTURE)",
           uid_set);

  char **lines = NULL;
  int lineCount = 0;
  int res = fetch_collect(s, cmd, &lines, &lineCount);

  *results = NULL;
  *count = 0;
  if (res != IMAP_OK || lineCount <= 0) {
    free_collected(lines, lineCount);
    return res;
  }

  *results = (ImapFetchResult *)calloc(lineCount, sizeof(ImapFetchResult));

  for (int i = 0; i < lineCount; i++) {
    char *l = lines[i];
    ImapFetchResult *r = &(*results)[*count];

    /* Parse sequence number */
    if (isdigit((unsigned char)l[0])) {
      r->seqno = strtoul(l, NULL, 10);
    }

    /* UID */
    char *p = strcasestr(l, "UID ");
    if (p) {
      p += 4;
      r->uid = strtoul(p, NULL, 10);
    }

    /* FLAGS */
    p = strcasestr(l, "FLAGS ");
    if (p) r->flags = parse_flags(p);

    /* RFC822.SIZE */
    p = strcasestr(l, "RFC822.SIZE ");
    if (p) {
      p += 12;
      r->size = strtoul(p, NULL, 10);
    }

    /* INTERNALDATE */
    p = strcasestr(l, "INTERNALDATE ");
    if (p) {
      p += 13;
      while (*p == ' ') p++;
      if (*p == '"') {
        p++;
        char *end = strchr(p, '"');
        if (end) {
          size_t dlen = end - p;
          r->internaldate = (char *)malloc(dlen + 1);
          if (r->internaldate) { memcpy(r->internaldate, p, dlen); r->internaldate[dlen] = '\0'; }
        }
      }
    }

    /* ENVELOPE */
    p = strcasestr(l, "ENVELOPE ");
    if (p) {
      p += 9;
      const char *cursor = p;
      r->envelope = parse_envelope_sexp(&cursor);
    }

    /* BODYSTRUCTURE */
    p = strcasestr(l, "BODYSTRUCTURE ");
    if (p) {
      p += 14;
      const char *cursor = p;
      r->bodystructure = parse_bodystructure(&cursor, NULL);
    }

    if (r->uid > 0) (*count)++;
  }
  free_collected(lines, lineCount);
  return res;
}

/* ================================================================
 * Message modification operations
 * ================================================================ */

int crispy_imap_store_flags(ImapSession *s, const char *uid_set,
                            const char *action, ImapFlags flags) {
  char flagStr[128];
  flags_to_str(flags, flagStr, sizeof(flagStr));
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "UID STORE %s %s (%s)", uid_set, action, flagStr);
  return crispy_imap_command(s, cmd);
}

int crispy_imap_copy(ImapSession *s, const char *uid_set, const char *dest) {
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "UID COPY %s \"%s\"", uid_set, dest);
  return crispy_imap_command(s, cmd);
}

int crispy_imap_move(ImapSession *s, const char *uid_set, const char *dest) {
  if (s->cap_move) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "UID MOVE %s \"%s\"", uid_set, dest);
    return crispy_imap_command(s, cmd);
  }
  /* Fallback: COPY + store \Deleted + EXPUNGE */
  int res = crispy_imap_copy(s, uid_set, dest);
  if (res != IMAP_OK) return res;
  ImapFlags delFlag = {0};
  delFlag.deleted = true;
  res = crispy_imap_store_flags(s, uid_set, "+FLAGS", delFlag);
  if (res != IMAP_OK) return res;
  return crispy_imap_expunge(s);
}

int crispy_imap_expunge(ImapSession *s) {
  return crispy_imap_command(s, "EXPUNGE");
}

int crispy_imap_append(ImapSession *s, const char *mailbox,
                       ImapFlags flags, const char *date,
                       const char *message, long msgLen) {
  if (msgLen < 0) msgLen = (long)strlen(message);

  char flagStr[128] = "";
  if (flags.seen || flags.answered || flags.flagged || flags.deleted || flags.draft) {
    char tmp[128];
    flags_to_str(flags, tmp, sizeof(tmp));
    snprintf(flagStr, sizeof(flagStr), " (%s)", tmp);
  }

  char dateStr[128] = "";
  if (date && date[0]) {
    snprintf(dateStr, sizeof(dateStr), " \"%s\"", date);
  }

  char cmd[512];
  snprintf(cmd, sizeof(cmd), "APPEND \"%s\"%s%s {%ld}",
           mailbox, flagStr, dateStr, msgLen);

  next_tag(s);
  char full[512];
  snprintf(full, sizeof(full), "%s %s", s->tag, cmd);
  if (send_line(s, full)) return IMAP_ERR;

  /* Server should respond with + to accept literal */
  char *cont = crispy_imap_readline(s);
  if (!cont || cont[0] != '+') { free(cont); return IMAP_ERR; }
  free(cont);

  /* Send message data */
  tp_send(s, message, msgLen);
  tp_send(s, "\r\n", 2);

  /* Read tagged response */
  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      int res = strncasecmp(line + tagLen + 1, "OK", 2) == 0 ? IMAP_OK : IMAP_NO;
      free(line);
      return res;
    }
    free(line);
  }
}

/* ================================================================
 * Search
 * ================================================================ */

int crispy_imap_search(ImapSession *s, const char *criteria,
                       unsigned long **uids) {
  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "UID SEARCH %s", criteria);

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  *uids = NULL;
  int count = 0;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "SEARCH", 6) == 0) {
        char *p = lines[i] + 6;
        int cap = 128;
        *uids = (unsigned long *)calloc(cap, sizeof(unsigned long));
        while (*p) {
          while (*p == ' ') p++;
          if (!*p) break;
          unsigned long uid = strtoul(p, &p, 10);
          if (uid == 0) break;
          if (count >= cap) {
            cap *= 2;
            *uids = (unsigned long *)realloc(*uids, cap * sizeof(unsigned long));
          }
          (*uids)[count++] = uid;
        }
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return count;
}

/* ================================================================
 * SORT (RFC 5256)
 * ================================================================ */

int crispy_imap_sort(ImapSession *s, const char *criteria,
                     const char *charset, const char *search,
                     unsigned long **uids) {
  if (!s->cap_sort) {
    dbg(s, "Server does not support SORT");
    *uids = NULL;
    return -1;
  }

  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "UID SORT (%s) %s %s",
           criteria, charset ? charset : "UTF-8", search ? search : "ALL");

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  *uids = NULL;
  int count = 0;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "SORT", 4) == 0) {
        char *p = lines[i] + 4;
        int cap = 128;
        *uids = (unsigned long *)calloc(cap, sizeof(unsigned long));
        while (*p) {
          while (*p == ' ') p++;
          if (!*p) break;
          unsigned long uid = strtoul(p, &p, 10);
          if (uid == 0) break;
          if (count >= cap) {
            cap *= 2;
            *uids = (unsigned long *)realloc(*uids, cap * sizeof(unsigned long));
          }
          (*uids)[count++] = uid;
        }
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return count;
}

/* ================================================================
 * THREAD (RFC 5256)
 * ================================================================ */

char *crispy_imap_thread(ImapSession *s, const char *algorithm,
                          const char *charset, const char *search) {
  if (!s->cap_thread) {
    dbg(s, "Server does not support THREAD");
    return NULL;
  }

  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "UID THREAD %s %s %s",
           algorithm ? algorithm : "REFERENCES",
           charset ? charset : "UTF-8",
           search ? search : "ALL");

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  char *result = NULL;
  if (res == IMAP_OK) {
    for (int i = 0; i < lineCount; i++) {
      if (strncasecmp(lines[i], "THREAD ", 7) == 0) {
        result = strdup(lines[i] + 7);
        break;
      }
    }
  }
  free_collected(lines, lineCount);
  return result;
}

/* ================================================================
 * UIDPLUS (RFC 4315) — parse APPENDUID/COPYUID from tagged reply
 * ================================================================ */

unsigned long crispy_imap_last_append_uid(ImapSession *s) {
  /* Look for [APPENDUID uidvalidity uid] in reply */
  char *p = strcasestr(s->reply, "APPENDUID ");
  if (!p) return 0;
  p += 10;
  /* Skip uidvalidity */
  strtoul(p, &p, 10);
  while (*p == ' ') p++;
  return strtoul(p, NULL, 10);
}

unsigned long crispy_imap_last_copy_uidvalidity(ImapSession *s) {
  /* Look for [COPYUID uidvalidity srcset destset] in reply */
  char *p = strcasestr(s->reply, "COPYUID ");
  if (!p) return 0;
  p += 8;
  return strtoul(p, NULL, 10);
}

/* ================================================================
 * SCRAM-SHA-256 auth for IMAP (RFC 7677)
 * Reuses the exchange logic from crispy_auth.c via callbacks.
 * ================================================================ */

static char *imap_sasl_read_continuation(ImapSession *s) {
  char *line = crispy_imap_readline(s);
  if (!line) return NULL;
  /* Expect "+ <base64>" */
  if (line[0] == '+') {
    char *data = line + 1;
    while (*data == ' ') data++;
    char *result = strdup(data);
    free(line);
    return result;
  }
  free(line);
  return NULL;
}

int crispy_imap_auth_scram_sha256(ImapSession *s, const char *user, const char *pass) {
  /* Generate client nonce */
  char cnonce[25];
  {
    unsigned char rnd[16];
    crispy_md5(user, strlen(user), rnd);
    long b64Len;
    char *b64 = crispy_base64_encode((char *)rnd, 16, &b64Len);
    if (b64) { snprintf(cnonce, sizeof(cnonce), "%s", b64); free(b64); }
    else snprintf(cnonce, sizeof(cnonce), "crispy%lx", (unsigned long)time(NULL));
  }

  /* Client-first-message-bare: n=user,r=cnonce */
  char cfmb[512];
  snprintf(cfmb, sizeof(cfmb), "n=%s,r=%s", user, cnonce);

  /* Client-first-message: n,,<cfmb> (GS2 header + bare) */
  char cfm[512];
  snprintf(cfm, sizeof(cfm), "n,,%s", cfmb);

  /* Base64 encode and send AUTHENTICATE SCRAM-SHA-256 <b64> */
  long b64Len;
  char *b64 = crispy_base64_encode(cfm, (long)strlen(cfm), &b64Len);
  if (!b64) return IMAP_ERR;

  next_tag(s);
  char full[1024];
  snprintf(full, sizeof(full), "%s AUTHENTICATE SCRAM-SHA-256 %s", s->tag, b64);
  free(b64);
  if (send_line(s, full)) return IMAP_ERR;

  /* Read server-first-message (+ <base64>) */
  char *sfmB64 = imap_sasl_read_continuation(s);
  if (!sfmB64) return IMAP_ERR;

  long sfmLen;
  char *sfm = crispy_base64_decode(sfmB64, (long)strlen(sfmB64), &sfmLen);
  free(sfmB64);
  if (!sfm) return IMAP_ERR;

  /* Parse r=nonce,s=salt,i=iterations */
  char *rnonce = NULL, *saltB64 = NULL;
  int iters = 4096;
  {
    char *sfmCopy = strdup(sfm);
    char *tok = strtok(sfmCopy, ",");
    while (tok) {
      if (strncmp(tok, "r=", 2) == 0) rnonce = strdup(tok + 2);
      else if (strncmp(tok, "s=", 2) == 0) saltB64 = strdup(tok + 2);
      else if (strncmp(tok, "i=", 2) == 0) iters = atoi(tok + 2);
      tok = strtok(NULL, ",");
    }
    free(sfmCopy);
  }
  if (!rnonce || !saltB64) { free(rnonce); free(saltB64); free(sfm); return IMAP_ERR; }

  /* Decode salt */
  long saltLen;
  unsigned char *salt = (unsigned char *)crispy_base64_decode(saltB64, (long)strlen(saltB64), &saltLen);
  free(saltB64);
  if (!salt) { free(rnonce); free(sfm); return IMAP_ERR; }

  /* SaltedPassword = PBKDF2(password, salt, iterations, 32) */
  unsigned char saltedPw[32];
  crispy_pbkdf2_sha256(pass, strlen(pass), salt, saltLen, iters, saltedPw, 32);
  free(salt);

  /* ClientKey = HMAC(SaltedPassword, "Client Key") */
  unsigned char clientKey[32];
  crispy_hmac_sha256(saltedPw, 32, "Client Key", 10, clientKey);

  /* StoredKey = SHA256(ClientKey) */
  unsigned char storedKey[32];
  crispy_sha256(clientKey, 32, storedKey);

  /* Client-final-message-without-proof: c=biws,r=<rnonce> */
  char cfmwp[512];
  snprintf(cfmwp, sizeof(cfmwp), "c=biws,r=%s", rnonce);

  /* AuthMessage = cfmb,sfm,cfmwp */
  char authMsg[2048];
  snprintf(authMsg, sizeof(authMsg), "%s,%s,%s", cfmb, sfm, cfmwp);
  free(sfm);

  /* ClientSignature = HMAC(StoredKey, AuthMessage) */
  unsigned char clientSig[32];
  crispy_hmac_sha256(storedKey, 32, authMsg, strlen(authMsg), clientSig);

  /* ClientProof = ClientKey XOR ClientSignature */
  unsigned char proof[32];
  for (int i = 0; i < 32; i++) proof[i] = clientKey[i] ^ clientSig[i];

  /* Base64 encode proof */
  char *proofB64 = crispy_base64_encode((char *)proof, 32, &b64Len);
  if (!proofB64) { free(rnonce); return IMAP_ERR; }
  free(rnonce);

  /* Client-final-message: cfmwp,p=proof */
  char finalMsg[1024];
  snprintf(finalMsg, sizeof(finalMsg), "%s,p=%s", cfmwp, proofB64);
  free(proofB64);

  /* Send client-final base64 */
  b64 = crispy_base64_encode(finalMsg, (long)strlen(finalMsg), &b64Len);
  if (!b64) return IMAP_ERR;
  if (send_line(s, b64)) { free(b64); return IMAP_ERR; }
  free(b64);

  /* Read server-final (+ <base64 with v=verifier>) — we skip verification for now */
  char *svB64 = imap_sasl_read_continuation(s);
  free(svB64);

  /* Send empty line to complete */
  send_line(s, "");

  /* Read tagged response */
  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      int res = strncasecmp(line + tagLen + 1, "OK", 2) == 0 ? IMAP_OK : IMAP_NO;
      free(line);
      if (res == IMAP_OK) s->authenticated = true;
      return res;
    }
    free(line);
  }
}

/* ================================================================
 * DIGEST-MD5 auth for IMAP (RFC 2831)
 * ================================================================ */

int crispy_imap_auth_digest_md5(ImapSession *s, const char *user, const char *pass) {
  /* Step 1: initiate DIGEST-MD5 */
  next_tag(s);
  char full[256];
  snprintf(full, sizeof(full), "%s AUTHENTICATE DIGEST-MD5", s->tag);
  if (send_line(s, full)) return IMAP_ERR;

  /* Server sends + <base64 challenge> */
  char *chalB64 = imap_sasl_read_continuation(s);
  if (!chalB64) return IMAP_ERR;

  long chalLen;
  char *challenge = crispy_base64_decode(chalB64, (long)strlen(chalB64), &chalLen);
  free(chalB64);
  if (!challenge) return IMAP_ERR;

  /* Parse challenge: realm, nonce, qop */
  char *realm = NULL, *nonce = NULL;
  {
    char *chalCopy = strdup(challenge);
    char *tok = strtok(chalCopy, ",");
    while (tok) {
      while (*tok == ' ') tok++;
      if (strncmp(tok, "realm=", 6) == 0) {
        char *v = tok + 6;
        if (*v == '"') { v++; char *q = strchr(v, '"'); if (q) *q = '\0'; }
        realm = strdup(v);
      } else if (strncmp(tok, "nonce=", 6) == 0) {
        char *v = tok + 6;
        if (*v == '"') { v++; char *q = strchr(v, '"'); if (q) *q = '\0'; }
        nonce = strdup(v);
      }
      tok = strtok(NULL, ",");
    }
    free(chalCopy);
  }
  free(challenge);
  if (!nonce) { free(realm); return IMAP_ERR; }
  if (!realm) realm = strdup("");

  /* Compute DIGEST-MD5 response */
  char cnonce[33];
  crispy_md5_hex(user, strlen(user), cnonce);

  /* HA1 = MD5(user:realm:pass) */
  char a1[512];
  snprintf(a1, sizeof(a1), "%s:%s:%s", user, realm, pass);
  unsigned char ha1_raw[16];
  crispy_md5(a1, strlen(a1), ha1_raw);
  char ha1[33];
  for (int i = 0; i < 16; i++) {
    ha1[i*2]   = "0123456789abcdef"[ha1_raw[i] >> 4];
    ha1[i*2+1] = "0123456789abcdef"[ha1_raw[i] & 0xf];
  }
  ha1[32] = '\0';

  /* HA2 = MD5("AUTHENTICATE:imap/<realm>") */
  char digestUri[256];
  snprintf(digestUri, sizeof(digestUri), "imap/%s", realm);
  char a2[512];
  snprintf(a2, sizeof(a2), "AUTHENTICATE:%s", digestUri);
  char ha2[33];
  crispy_md5_hex(a2, strlen(a2), ha2);

  /* response = MD5(ha1:nonce:00000001:cnonce:auth:ha2) */
  char respInput[1024];
  snprintf(respInput, sizeof(respInput), "%s:%s:00000001:%s:auth:%s",
           ha1, nonce, cnonce, ha2);
  char respHash[33];
  crispy_md5_hex(respInput, strlen(respInput), respHash);

  /* Build response */
  char resp[2048];
  snprintf(resp, sizeof(resp),
           "username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\","
           "nc=00000001,qop=auth,digest-uri=\"%s\",response=%s",
           user, realm, nonce, cnonce, digestUri, respHash);
  free(realm);
  free(nonce);

  /* Base64 encode and send */
  long b64Len;
  char *b64 = crispy_base64_encode(resp, (long)strlen(resp), &b64Len);
  if (!b64) return IMAP_ERR;
  if (send_line(s, b64)) { free(b64); return IMAP_ERR; }
  free(b64);

  /* Server sends + <rspauth> — we respond with empty line */
  char *rspauth = imap_sasl_read_continuation(s);
  free(rspauth);
  send_line(s, "");

  /* Read tagged response */
  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      snprintf(s->reply, sizeof(s->reply), "%s", line + tagLen + 1);
      int res = strncasecmp(line + tagLen + 1, "OK", 2) == 0 ? IMAP_OK : IMAP_NO;
      free(line);
      if (res == IMAP_OK) s->authenticated = true;
      return res;
    }
    free(line);
  }
}

/* ================================================================
 * Session management
 * ================================================================ */

int crispy_imap_noop(ImapSession *s) {
  return crispy_imap_command(s, "NOOP");
}

/* IDLE (RFC 2177) — wait for server push notifications.
 * Uses poll() for proper timeout support. Falls back to NOOP
 * if server doesn't support IDLE. */
int crispy_imap_idle(ImapSession *s, int timeout_ms) {
  if (!s->cap_idle) return crispy_imap_noop(s);

  next_tag(s);
  char full[64];
  snprintf(full, sizeof(full), "%s IDLE", s->tag);
  if (send_line(s, full)) return IMAP_ERR;

  /* Server sends + continuation to confirm IDLE mode */
  char *cont = crispy_imap_readline(s);
  if (!cont || cont[0] != '+') { free(cont); return IMAP_ERR; }
  free(cont);

  /* Wait for server push with timeout using poll().
   * We need the underlying socket fd from the transport.
   * If transport exposes get_fd, use poll(). Otherwise blocking read. */
  if (s->tp.get_fd && timeout_ms > 0) {
    int fd = s->tp.get_fd(s->tp.ctx);
    if (fd >= 0) {
      crispy_pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLIN;
      pfd.revents = 0;
      int ret = CRISPY_POLL(&pfd, 1, timeout_ms);
      if (ret <= 0) {
        /* Timeout or error — send DONE immediately */
        send_line(s, "DONE");
        while (1) {
          char *line = crispy_imap_readline(s);
          if (!line) return IMAP_ERR;
          size_t tagLen = strlen(s->tag);
          if (strncmp(line, s->tag, tagLen) == 0) {
            int res = strncasecmp(line + tagLen + 1, "OK", 2) == 0 ? IMAP_OK : IMAP_NO;
            free(line);
            return res;
          }
          /* Drain any untagged responses */
          if (line[0] == '*' && line[1] == ' ')
            snprintf(s->reply, sizeof(s->reply), "%s", line + 2);
          free(line);
        }
      }
      /* poll returned > 0: data available, fall through to read */
    }
  }

  /* Read untagged update(s) from server */
  char *update = crispy_imap_readline(s);
  if (update) {
    if (update[0] == '*' && update[1] == ' ')
      snprintf(s->reply, sizeof(s->reply), "%s", update + 2);
    else
      snprintf(s->reply, sizeof(s->reply), "%s", update);
    free(update);
  }

  /* Send DONE to exit IDLE */
  send_line(s, "DONE");

  /* Read any remaining untagged responses and the tagged reply */
  while (1) {
    char *line = crispy_imap_readline(s);
    if (!line) return IMAP_ERR;
    size_t tagLen = strlen(s->tag);
    if (strncmp(line, s->tag, tagLen) == 0 && line[tagLen] == ' ') {
      int res = strncasecmp(line + tagLen + 1, "OK", 2) == 0 ? IMAP_OK : IMAP_NO;
      free(line);
      return res;
    }
    /* Store untagged responses */
    if (line[0] == '*' && line[1] == ' ')
      snprintf(s->reply, sizeof(s->reply), "%s", line + 2);
    free(line);
  }
}

/* ================================================================
 * Message cache
 * ================================================================ */

static void cache_ensure_capacity(ImapCache *c, int needed) {
  if (needed <= c->capacity) return;
  int newCap = c->capacity ? c->capacity * 2 : 256;
  while (newCap < needed) newCap *= 2;
  c->entries = (ImapCacheEntry *)realloc(c->entries,
                                          newCap * sizeof(ImapCacheEntry));
  memset(&c->entries[c->capacity], 0,
         (newCap - c->capacity) * sizeof(ImapCacheEntry));
  c->capacity = newCap;
}

void crispy_imap_cache_clear(ImapSession *s) {
  for (int i = 0; i < s->cache.count; i++) {
    free(s->cache.entries[i].internaldate);
    crispy_imap_free_envelope(s->cache.entries[i].envelope);
  }
  free(s->cache.entries);
  memset(&s->cache, 0, sizeof(s->cache));
}

ImapCacheEntry *crispy_imap_cache_lookup(ImapSession *s, unsigned long uid) {
  for (int i = 0; i < s->cache.count; i++) {
    if (s->cache.entries[i].uid == uid)
      return &s->cache.entries[i];
  }
  return NULL;
}

ImapCacheEntry *crispy_imap_cache_lookup_seqno(ImapSession *s, unsigned long seqno) {
  for (int i = 0; i < s->cache.count; i++) {
    if (s->cache.entries[i].seqno == seqno)
      return &s->cache.entries[i];
  }
  return NULL;
}

unsigned long crispy_imap_seqno_to_uid(ImapSession *s, unsigned long seqno) {
  ImapCacheEntry *e = crispy_imap_cache_lookup_seqno(s, seqno);
  return e ? e->uid : 0;
}

unsigned long crispy_imap_uid_to_seqno(ImapSession *s, unsigned long uid) {
  ImapCacheEntry *e = crispy_imap_cache_lookup(s, uid);
  return e ? e->seqno : 0;
}

void crispy_imap_cache_update_flags(ImapSession *s, unsigned long uid,
                                     ImapFlags flags) {
  ImapCacheEntry *e = crispy_imap_cache_lookup(s, uid);
  if (e) e->flags = flags;
}

int crispy_imap_cache_populate(ImapSession *s) {
  /* Fetch UID, FLAGS, RFC822.SIZE for all messages */
  char cmd[256];
  snprintf(cmd, sizeof(cmd),
           "UID FETCH 1:* (UID FLAGS RFC822.SIZE)");

  char **lines = NULL;
  int lineCount = 0;
  int res = crispy_imap_command_collect(s, cmd, &lines, &lineCount);

  crispy_imap_cache_clear(s);
  if (res != IMAP_OK || lineCount <= 0) {
    free_collected(lines, lineCount);
    return res;
  }

  cache_ensure_capacity(&s->cache, lineCount);

  for (int i = 0; i < lineCount; i++) {
    char *l = lines[i];
    ImapCacheEntry *e = &s->cache.entries[s->cache.count];
    memset(e, 0, sizeof(*e));

    /* Parse sequence number from "N FETCH ..." */
    if (isdigit((unsigned char)l[0])) {
      e->seqno = strtoul(l, NULL, 10);
    }

    char *p = strcasestr(l, "UID ");
    if (p) {
      p += 4;
      e->uid = strtoul(p, NULL, 10);
    }

    p = strcasestr(l, "FLAGS ");
    if (p) e->flags = parse_flags(p);

    p = strcasestr(l, "RFC822.SIZE ");
    if (p) {
      p += 12;
      e->size = strtoul(p, NULL, 10);
    }

    if (e->uid > 0) s->cache.count++;
  }

  free_collected(lines, lineCount);
  dbg(s, "Cache populated: %d messages", s->cache.count);
  return IMAP_OK;
}

/* ================================================================
 * Auto-reconnect
 * ================================================================ */

void crispy_imap_enable_reconnect(ImapSession *s,
                                   const char *user, const char *pass) {
  snprintf(s->reconnect_user, sizeof(s->reconnect_user), "%s", user);
  snprintf(s->reconnect_pass, sizeof(s->reconnect_pass), "%s", pass);
  s->reconnect_enabled = true;
}

void crispy_imap_disable_reconnect(ImapSession *s) {
  s->reconnect_enabled = false;
  memset(s->reconnect_user, 0, sizeof(s->reconnect_user));
  memset(s->reconnect_pass, 0, sizeof(s->reconnect_pass));
}

int crispy_imap_reconnect(ImapSession *s) {
  if (!s->reconnect_host[0] || !s->reconnect_user[0]) return IMAP_ERR;

  char saved_mailbox[256];
  snprintf(saved_mailbox, sizeof(saved_mailbox), "%s", s->selected.name);

  dbg(s, "Reconnecting to %s:%d...", s->reconnect_host, s->reconnect_port);

  /* Close existing connection without LOGOUT (it's dead) */
  if (s->tp.close) s->tp.close(s->tp.ctx);
  s->connected = false;
  s->authenticated = false;
  s->bye_received = false;

  /* Need a fresh transport context — destroy and recreate */
  if (s->tp.destroy) {
    s->tp.destroy(s->tp.ctx);
    s->tp.ctx = NULL;
  }

  /* Reconnect */
  int res = crispy_imap_connect(s, s->reconnect_host,
                                 s->reconnect_port,
                                 s->reconnect_security);
  if (res != IMAP_OK) {
    dbg(s, "Reconnect failed: connect error");
    return res;
  }

  /* Re-authenticate */
  res = crispy_imap_login(s, s->reconnect_user, s->reconnect_pass);
  if (res != IMAP_OK) {
    dbg(s, "Reconnect failed: login error");
    return res;
  }

  /* Re-select previously selected mailbox */
  if (saved_mailbox[0]) {
    res = crispy_imap_select(s, saved_mailbox);
    if (res != IMAP_OK) {
      dbg(s, "Reconnect failed: select %s error", saved_mailbox);
      return res;
    }
  }

  dbg(s, "Reconnected successfully");
  return IMAP_OK;
}
