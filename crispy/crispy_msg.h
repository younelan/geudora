/* crispy_msg.h — RFC 5322 message construction and parsing
 * Part of crispy: standalone mail library.
 *
 * Build complete MIME messages from parts (text, HTML, attachments).
 * Parse received messages into structured parts.
 */

#ifndef CRISPY_MSG_H
#define CRISPY_MSG_H

#include <stddef.h>
#include <stdbool.h>

/* --- Attachment --- */
typedef struct CrispyAttachment {
  const char *path;         /* file path (reads file at build time) */
  const char *filename;     /* display name (NULL = basename of path) */
  const char *mime_type;    /* e.g. "image/png" (NULL = auto-detect) */
  const char *data;         /* raw data (alternative to path) */
  long data_len;            /* length of raw data */
  bool is_inline;           /* inline (Content-ID) vs attachment */
  const char *content_id;   /* for inline references in HTML */
} CrispyAttachment;

/* --- Message to build --- */
typedef struct CrispyMsg {
  /* Envelope */
  const char *from;         /* "Name <addr>" or just "addr" */
  const char *to;           /* comma-separated */
  const char *cc;           /* comma-separated (NULL = none) */
  const char *bcc;          /* comma-separated (NULL = none) */
  const char *reply_to;     /* NULL = none */
  const char *subject;

  /* Body — provide one or both */
  const char *body_plain;   /* plain text body */
  const char *body_html;    /* HTML body (NULL = plain only) */

  /* Attachments */
  CrispyAttachment *attachments;
  int attachment_count;

  /* Options */
  const char *message_id;   /* NULL = auto-generate */
  const char *in_reply_to;  /* NULL = none */
  const char *references;   /* NULL = none */
  const char *x_mailer;     /* NULL = "Crispy Mail Library" */
  int priority;             /* 0=normal, 1=highest, 5=lowest */
  long tz_offset;           /* timezone offset in seconds */
} CrispyMsg;

/* --- Build --- */

/* Build a complete RFC 5322 message.
 * Returns malloc'd buffer (caller must free).
 * *outLen receives the length. Returns NULL on error. */
char *crispy_msg_build(const CrispyMsg *msg, long *outLen);

/* Extract envelope recipients from a built CrispyMsg.
 * Returns NULL-terminated malloc'd array of addresses.
 * Caller must free each string and the array. */
char **crispy_msg_recipients(const CrispyMsg *msg, int *count);

/* --- Parse (received messages) --- */

typedef struct CrispyMsgPart {
  char *mime_type;          /* e.g. "text/plain" */
  char *charset;            /* e.g. "UTF-8" */
  char *filename;           /* for attachments */
  char *content_id;         /* for inline parts */
  char *data;               /* decoded content */
  long data_len;
  bool is_attachment;
} CrispyMsgPart;

typedef struct CrispyMsgParsed {
  /* Headers (allocated strings, caller frees) */
  char *from;
  char *to;
  char *cc;
  char *subject;
  char *date;
  char *message_id;
  char *in_reply_to;

  /* Body parts */
  CrispyMsgPart *parts;
  int part_count;

  /* Convenience pointers into parts[] (not separately allocated) */
  const char *body_plain;   /* first text/plain part */
  long body_plain_len;
  const char *body_html;    /* first text/html part */
  long body_html_len;
} CrispyMsgParsed;

/* Parse a raw RFC 5322 message into structured parts.
 * Returns 0 on success. */
int crispy_msg_parse(const char *raw, long rawLen, CrispyMsgParsed *out);

/* Free a parsed message. */
void crispy_msg_parsed_free(CrispyMsgParsed *parsed);

/* --- MIME utilities --- */

/* Guess MIME type from filename extension. Returns static string. */
const char *crispy_mime_type(const char *filename);

/* Generate a unique boundary string. buf must be >= 48 bytes. */
char *crispy_mime_boundary(char *buf, size_t bufSize);

/* Generate a Message-ID. buf must be >= 128 bytes. */
char *crispy_msg_gen_id(char *buf, size_t bufSize, const char *domain);

#endif /* CRISPY_MSG_H */
