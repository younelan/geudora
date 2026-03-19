/* headparse.h — RFC 822 header parsing for raw message text
 * Part of maillib: standalone library, no Eudora/GTK dependency.
 */

#ifndef CRISPY_HEADPARSE_H
#define CRISPY_HEADPARSE_H

#include <stddef.h>
#include <stdbool.h>

/* Standard RFC 2822 header IDs */
enum {
  MAIL_HEAD_TO = 0,
  MAIL_HEAD_FROM,
  MAIL_HEAD_SUBJECT,
  MAIL_HEAD_CC,
  MAIL_HEAD_BCC,
  MAIL_HEAD_REPLY_TO,
  MAIL_HEAD_DATE,
  MAIL_HEAD_MESSAGE_ID,
  MAIL_HEAD_IN_REPLY_TO,
  MAIL_HEAD_REFERENCES,
  MAIL_HEAD_CONTENT_TYPE,
  MAIL_HEAD_CONTENT_TRANSFER_ENCODING,
  MAIL_HEAD_CONTENT_DISPOSITION,
  MAIL_HEAD_MIME_VERSION,
  MAIL_HEAD_RETURN_PATH,
  MAIL_HEAD_RECEIVED,
  MAIL_HEAD_X_MAILER,
  MAIL_HEAD_X_SENDER,
  MAIL_HEAD_X_PRIORITY,
  MAIL_HEAD_COUNT
};

/* Returns the RFC header name string for a given ID, e.g. "Subject: "
 * Returns NULL for invalid IDs. */
const char *mail_header_name(int id);

/* Header position descriptor */
typedef struct MailHeadSpec {
  long offset;  /* start offset of the header line in text */
  long length;  /* total length of header (name + value) */
  long stop;    /* end offset */
  long value;   /* offset where the value starts (after "Name: ") */
  long start;   /* alias for offset */
  short index;  /* header index (caller-defined) */
} MailHeadSpec;

/* Find a header by name in raw RFC 822 text.
 * name should include the colon, e.g. "Subject: " or "To:"
 * Returns hs on success, NULL if not found. */
MailHeadSpec *headparse_find(const char *text, const char *name,
                             MailHeadSpec *hs);

/* Extract header value text. Allocates into *out (caller must free).
 * Returns 0 on success, -1 on failure. */
int headparse_get_value(const char *text, const MailHeadSpec *hs, char **out);

/* Find header by standard ID, extract value.
 * Returns 0 on success, -1 on failure. */
int headparse_get_by_id(const char *text, int id, char **out);

/* Find header by standard ID, copy into buffer.
 * Returns val (always). */
char *headparse_get_by_id_buf(const char *text, int id, char *val,
                              size_t valSize);

#endif /* CRISPY_HEADPARSE_H */
