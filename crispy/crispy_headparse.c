/* headparse.c — RFC 822 header parsing for raw message text
 * Part of maillib: standalone, no external dependencies.
 */

#include "crispy_headparse.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Standard RFC 2822 header name strings (with colon+space) */
static const char *const rfc2822_headers[MAIL_HEAD_COUNT] = {
  [MAIL_HEAD_TO]                        = "To: ",
  [MAIL_HEAD_FROM]                      = "From: ",
  [MAIL_HEAD_SUBJECT]                   = "Subject: ",
  [MAIL_HEAD_CC]                        = "Cc: ",
  [MAIL_HEAD_BCC]                       = "Bcc: ",
  [MAIL_HEAD_REPLY_TO]                  = "Reply-To: ",
  [MAIL_HEAD_DATE]                      = "Date: ",
  [MAIL_HEAD_MESSAGE_ID]                = "Message-Id: ",
  [MAIL_HEAD_IN_REPLY_TO]               = "In-Reply-To: ",
  [MAIL_HEAD_REFERENCES]                = "References: ",
  [MAIL_HEAD_CONTENT_TYPE]              = "Content-Type: ",
  [MAIL_HEAD_CONTENT_TRANSFER_ENCODING] = "Content-Transfer-Encoding: ",
  [MAIL_HEAD_CONTENT_DISPOSITION]       = "Content-Disposition: ",
  [MAIL_HEAD_MIME_VERSION]              = "MIME-Version: ",
  [MAIL_HEAD_RETURN_PATH]               = "Return-Path: ",
  [MAIL_HEAD_RECEIVED]                  = "Received: ",
  [MAIL_HEAD_X_MAILER]                  = "X-Mailer: ",
  [MAIL_HEAD_X_SENDER]                  = "X-Sender: ",
  [MAIL_HEAD_X_PRIORITY]                = "X-Priority: ",
};

const char *crispy_header_name(int id) {
  if (id < 0 || id >= MAIL_HEAD_COUNT) return NULL;
  return rfc2822_headers[id];
}

/* Case-insensitive prefix match */
static int hp_strncasecmp(const char *a, const char *b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    int ca = tolower((unsigned char)a[i]);
    int cb = tolower((unsigned char)b[i]);
    if (ca != cb) return ca - cb;
    if (ca == 0) return 0;
  }
  return 0;
}

MailHeadSpec *crispy_headparse_find(const char *text, const char *name,
                             MailHeadSpec *hs) {
  if (!text || !name || !hs) return NULL;
  memset(hs, 0, sizeof(*hs));

  size_t nameLen = strlen(name);
  const char *p = text;

  while (*p) {
    if (hp_strncasecmp(p, name, nameLen) == 0) {
      const char *valStart = p + nameLen;
      while (*valStart == ' ' || *valStart == '\t') valStart++;

      const char *valEnd = valStart;
      while (*valEnd) {
        if (*valEnd == '\r' || *valEnd == '\n') {
          const char *next = valEnd;
          if (*next == '\r') next++;
          if (*next == '\n') next++;
          if (*next == ' ' || *next == '\t') {
            valEnd = next;
            continue;
          }
          break;
        }
        valEnd++;
      }

      hs->start = hs->offset = (long)(p - text);
      hs->value = (long)(valStart - text);
      hs->stop = (long)(valEnd - text);
      hs->length = (long)(valEnd - p);
      return hs;
    }

    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;
    if (*p == '\r' || *p == '\n' || *p == '\0') break;
  }
  return NULL;
}

int crispy_headparse_get_value(const char *text, const MailHeadSpec *hs, char **out) {
  if (!text || !hs || !out) return -1;
  *out = NULL;

  long valLen = hs->stop - hs->value;
  if (valLen <= 0) return -1;

  *out = (char *)malloc(valLen + 1);
  if (!*out) return -1;
  memcpy(*out, text + hs->value, valLen);
  (*out)[valLen] = '\0';
  return 0;
}

int crispy_headparse_get_by_id(const char *text, int id, char **out) {
  if (!text || !out) return -1;
  *out = NULL;

  const char *name = crispy_header_name(id);
  if (!name) return -1;

  MailHeadSpec hs;
  if (!crispy_headparse_find(text, name, &hs))
    return -1;

  return crispy_headparse_get_value(text, &hs, out);
}

char *crispy_headparse_get_by_id_buf(const char *text, int id, char *val,
                              size_t valSize) {
  if (!text || !val || !valSize) return val;
  val[0] = '\0';

  const char *name = crispy_header_name(id);
  if (!name) return val;

  MailHeadSpec hs;
  if (crispy_headparse_find(text, name, &hs)) {
    long len = hs.stop - hs.value;
    if (len > (long)(valSize - 1)) len = (long)(valSize - 1);
    if (len > 0)
      memcpy(val, text + hs.value, len);
    val[len] = '\0';
  }
  return val;
}
