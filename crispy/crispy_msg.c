/* crispy_msg.c — RFC 5322 message construction and parsing
 * Part of crispy: standalone mail library.
 */

#include "crispy_msg.h"
#include "crispy_smtp.h"      /* for crispy_base64_encode, crispy_smtp_format_date */
#include "crispy_headparse.h"
#include "crispy_encode.h"
#include "crispy_md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

/* --- Growing buffer --- */
typedef struct {
  char *data;
  long len;
  long cap;
} Buf;

static void buf_init(Buf *b) { memset(b, 0, sizeof(*b)); }

static int buf_grow(Buf *b, long need) {
  if (b->len + need <= b->cap) return 0;
  long newcap = b->cap ? b->cap * 2 : 4096;
  while (newcap < b->len + need) newcap *= 2;
  char *p = (char *)realloc(b->data, newcap);
  if (!p) return -1;
  b->data = p;
  b->cap = newcap;
  return 0;
}

static int buf_add(Buf *b, const char *s, long len) {
  if (buf_grow(b, len)) return -1;
  memcpy(b->data + b->len, s, len);
  b->len += len;
  return 0;
}

static int buf_str(Buf *b, const char *s) {
  return buf_add(b, s, (long)strlen(s));
}

static int buf_fmt(Buf *b, const char *fmt, ...) {
  char tmp[1024];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  return buf_add(b, tmp, n);
}

/* --- MIME type detection --- */

static const struct { const char *ext; const char *type; } mime_table[] = {
  { ".txt",  "text/plain" },
  { ".html", "text/html" },
  { ".htm",  "text/html" },
  { ".css",  "text/css" },
  { ".csv",  "text/csv" },
  { ".xml",  "text/xml" },
  { ".json", "application/json" },
  { ".pdf",  "application/pdf" },
  { ".zip",  "application/zip" },
  { ".gz",   "application/gzip" },
  { ".tar",  "application/x-tar" },
  { ".doc",  "application/msword" },
  { ".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
  { ".xls",  "application/vnd.ms-excel" },
  { ".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
  { ".ppt",  "application/vnd.ms-powerpoint" },
  { ".png",  "image/png" },
  { ".jpg",  "image/jpeg" },
  { ".jpeg", "image/jpeg" },
  { ".gif",  "image/gif" },
  { ".bmp",  "image/bmp" },
  { ".svg",  "image/svg+xml" },
  { ".ico",  "image/x-icon" },
  { ".webp", "image/webp" },
  { ".mp3",  "audio/mpeg" },
  { ".wav",  "audio/wav" },
  { ".mp4",  "video/mp4" },
  { ".avi",  "video/x-msvideo" },
  { ".mov",  "video/quicktime" },
  { ".eml",  "message/rfc822" },
  { NULL, NULL }
};

const char *crispy_mime_type(const char *filename) {
  if (!filename) return "application/octet-stream";
  const char *dot = strrchr(filename, '.');
  if (!dot) return "application/octet-stream";
  for (int i = 0; mime_table[i].ext; i++)
    if (strcasecmp(dot, mime_table[i].ext) == 0)
      return mime_table[i].type;
  return "application/octet-stream";
}

/* --- Boundary and Message-ID generation --- */

char *crispy_mime_boundary(char *buf, size_t bufSize) {
  snprintf(buf, bufSize, "----=_CrispyBoundary_%lx_%x",
           (unsigned long)time(NULL), (unsigned)rand());
  return buf;
}

char *crispy_msg_gen_id(char *buf, size_t bufSize, const char *domain) {
  if (!domain) domain = "crispy.local";
  /* Hash time + random for a unique ID */
  char seed[64];
  snprintf(seed, sizeof(seed), "%lx.%x.%d",
           (unsigned long)time(NULL), (unsigned)rand(), (int)getpid());
  char hex[33];
  crispy_md5_hex(seed, strlen(seed), hex);
  snprintf(buf, bufSize, "<%s@%s>", hex, domain);
  return buf;
}

/* --- Read file into malloc'd buffer --- */

static char *read_file(const char *path, long *outLen) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *data = (char *)malloc(len);
  if (data) {
    long rd = (long)fread(data, 1, len, f);
    if (outLen) *outLen = rd;
  }
  fclose(f);
  return data;
}

/* --- Extract domain from email address --- */

static const char *addr_domain(const char *addr) {
  const char *at = strchr(addr, '@');
  if (at) return at + 1;
  /* Check for <addr> format */
  const char *lt = strchr(addr, '<');
  if (lt) {
    at = strchr(lt, '@');
    if (at) return at + 1;
  }
  return "localhost";
}

/* Strip domain of trailing > */
static void clean_domain(const char *addr, char *domain, size_t sz) {
  const char *d = addr_domain(addr);
  snprintf(domain, sz, "%s", d);
  char *gt = strchr(domain, '>');
  if (gt) *gt = '\0';
}

/* --- Extract bare address from "Name <addr>" --- */

static void extract_addr(const char *full, char *addr, size_t sz) {
  const char *lt = strchr(full, '<');
  if (lt) {
    const char *gt = strchr(lt, '>');
    if (gt) {
      size_t len = (size_t)(gt - lt - 1);
      if (len >= sz) len = sz - 1;
      memcpy(addr, lt + 1, len);
      addr[len] = '\0';
      return;
    }
  }
  /* No angle brackets — use as-is, trimmed */
  while (*full == ' ') full++;
  snprintf(addr, sz, "%s", full);
  char *end = addr + strlen(addr) - 1;
  while (end > addr && *end == ' ') *end-- = '\0';
}

/* --- Encode attachment as base64 and write to buffer --- */

static int write_attachment(Buf *b, const CrispyAttachment *att,
                            const char *boundary) {
  const char *data = att->data;
  long dataLen = att->data_len;
  char *fileData = NULL;

  /* Read from file if no inline data */
  if (!data && att->path) {
    fileData = read_file(att->path, &dataLen);
    if (!fileData) return -1;
    data = fileData;
  }
  if (!data) return -1;

  /* Determine filename and MIME type */
  const char *fname = att->filename;
  if (!fname && att->path) {
    fname = strrchr(att->path, '/');
    if (fname) fname++; else fname = att->path;
  }
  if (!fname) fname = "attachment";

  const char *mime = att->mime_type;
  if (!mime) mime = crispy_mime_type(fname);

  /* MIME part header */
  buf_fmt(b, "\r\n--%s\r\n", boundary);
  if (att->is_inline && att->content_id) {
    buf_fmt(b, "Content-Type: %s; name=\"%s\"\r\n", mime, fname);
    buf_str(b, "Content-Transfer-Encoding: base64\r\n");
    buf_fmt(b, "Content-ID: <%s>\r\n", att->content_id);
    buf_str(b, "Content-Disposition: inline\r\n");
  } else {
    buf_fmt(b, "Content-Type: %s; name=\"%s\"\r\n", mime, fname);
    buf_str(b, "Content-Transfer-Encoding: base64\r\n");
    buf_fmt(b, "Content-Disposition: attachment; filename=\"%s\"\r\n", fname);
  }
  buf_str(b, "\r\n");

  /* Base64 encode in 76-char lines */
  long b64Len;
  char *b64 = crispy_base64_encode(data, dataLen, &b64Len);
  free(fileData);
  if (!b64) return -1;

  /* Write in 76-char lines */
  for (long i = 0; i < b64Len; i += 76) {
    long lineLen = b64Len - i;
    if (lineLen > 76) lineLen = 76;
    buf_add(b, b64 + i, lineLen);
    buf_str(b, "\r\n");
  }

  free(b64);
  return 0;
}

/* --- Build message --- */

char *crispy_msg_build(const CrispyMsg *msg, long *outLen) {
  if (!msg || !msg->from || !msg->to) return NULL;

  Buf b;
  buf_init(&b);

  /* Date */
  char date[64];
  crispy_smtp_format_date(date, sizeof(date), (long)time(NULL),
                          msg->tz_offset);
  buf_fmt(&b, "Date: %s\r\n", date);

  /* From / To / Cc / Bcc */
  buf_fmt(&b, "From: %s\r\n", msg->from);
  buf_fmt(&b, "To: %s\r\n", msg->to);
  if (msg->cc && *msg->cc)
    buf_fmt(&b, "Cc: %s\r\n", msg->cc);
  /* BCC not written to headers (envelope only) */

  /* Subject — encode if non-ASCII */
  if (msg->subject) {
    char *encSubj = crispy_encode_header(msg->subject);
    buf_fmt(&b, "Subject: %s\r\n", encSubj ? encSubj : msg->subject);
    free(encSubj);
  } else {
    buf_str(&b, "Subject: \r\n");
  }

  /* Message-ID */
  char msgid[128];
  if (msg->message_id) {
    buf_fmt(&b, "Message-ID: %s\r\n", msg->message_id);
  } else {
    char domain[128];
    clean_domain(msg->from, domain, sizeof(domain));
    crispy_msg_gen_id(msgid, sizeof(msgid), domain);
    buf_fmt(&b, "Message-ID: %s\r\n", msgid);
  }

  /* In-Reply-To / References */
  if (msg->in_reply_to)
    buf_fmt(&b, "In-Reply-To: %s\r\n", msg->in_reply_to);
  if (msg->references)
    buf_fmt(&b, "References: %s\r\n", msg->references);
  if (msg->reply_to)
    buf_fmt(&b, "Reply-To: %s\r\n", msg->reply_to);

  /* Priority */
  if (msg->priority > 0 && msg->priority != 3)
    buf_fmt(&b, "X-Priority: %d\r\n", msg->priority);

  /* X-Mailer */
  buf_fmt(&b, "X-Mailer: %s\r\n",
          msg->x_mailer ? msg->x_mailer : "Crispy Mail Library");

  buf_str(&b, "MIME-Version: 1.0\r\n");

  /* Decide structure:
   * - Plain only, no attachments → simple text/plain
   * - Plain + HTML, no attachments → multipart/alternative
   * - Attachments → multipart/mixed (with alternative inside if both texts)
   */
  bool has_html = msg->body_html && *msg->body_html;
  bool has_attach = msg->attachment_count > 0;
  char boundary_mixed[48] = "";
  char boundary_alt[48] = "";

  if (has_attach) {
    crispy_mime_boundary(boundary_mixed, sizeof(boundary_mixed));
    buf_fmt(&b, "Content-Type: multipart/mixed;\r\n boundary=\"%s\"\r\n",
            boundary_mixed);
    buf_str(&b, "\r\n");
    buf_str(&b, "This is a multi-part message in MIME format.\r\n");

    if (has_html) {
      /* mixed { alternative { plain, html }, attachments } */
      crispy_mime_boundary(boundary_alt, sizeof(boundary_alt));
      buf_fmt(&b, "\r\n--%s\r\n", boundary_mixed);
      buf_fmt(&b, "Content-Type: multipart/alternative;\r\n boundary=\"%s\"\r\n",
              boundary_alt);

      /* Plain part */
      buf_fmt(&b, "\r\n--%s\r\n", boundary_alt);
      buf_str(&b, "Content-Type: text/plain; charset=UTF-8\r\n");
      buf_str(&b, "Content-Transfer-Encoding: 8bit\r\n\r\n");
      buf_str(&b, msg->body_plain ? msg->body_plain : "");
      buf_str(&b, "\r\n");

      /* HTML part */
      buf_fmt(&b, "\r\n--%s\r\n", boundary_alt);
      buf_str(&b, "Content-Type: text/html; charset=UTF-8\r\n");
      buf_str(&b, "Content-Transfer-Encoding: 8bit\r\n\r\n");
      buf_str(&b, msg->body_html);
      buf_str(&b, "\r\n");

      buf_fmt(&b, "\r\n--%s--\r\n", boundary_alt);
    } else {
      /* mixed { plain, attachments } */
      buf_fmt(&b, "\r\n--%s\r\n", boundary_mixed);
      buf_str(&b, "Content-Type: text/plain; charset=UTF-8\r\n");
      buf_str(&b, "Content-Transfer-Encoding: 8bit\r\n\r\n");
      buf_str(&b, msg->body_plain ? msg->body_plain : "");
      buf_str(&b, "\r\n");
    }

    /* Attachments */
    for (int i = 0; i < msg->attachment_count; i++)
      write_attachment(&b, &msg->attachments[i], boundary_mixed);

    buf_fmt(&b, "\r\n--%s--\r\n", boundary_mixed);

  } else if (has_html) {
    /* alternative { plain, html } */
    crispy_mime_boundary(boundary_alt, sizeof(boundary_alt));
    buf_fmt(&b, "Content-Type: multipart/alternative;\r\n boundary=\"%s\"\r\n",
            boundary_alt);
    buf_str(&b, "\r\n");

    /* Plain */
    buf_fmt(&b, "\r\n--%s\r\n", boundary_alt);
    buf_str(&b, "Content-Type: text/plain; charset=UTF-8\r\n");
    buf_str(&b, "Content-Transfer-Encoding: 8bit\r\n\r\n");
    buf_str(&b, msg->body_plain ? msg->body_plain : "");
    buf_str(&b, "\r\n");

    /* HTML */
    buf_fmt(&b, "\r\n--%s\r\n", boundary_alt);
    buf_str(&b, "Content-Type: text/html; charset=UTF-8\r\n");
    buf_str(&b, "Content-Transfer-Encoding: 8bit\r\n\r\n");
    buf_str(&b, msg->body_html);
    buf_str(&b, "\r\n");

    buf_fmt(&b, "\r\n--%s--\r\n", boundary_alt);

  } else {
    /* Simple plain text */
    buf_str(&b, "Content-Type: text/plain; charset=UTF-8\r\n");
    buf_str(&b, "Content-Transfer-Encoding: 8bit\r\n\r\n");
    buf_str(&b, msg->body_plain ? msg->body_plain : "");
    buf_str(&b, "\r\n");
  }

  /* Null-terminate */
  buf_add(&b, "\0", 1);
  b.len--; /* don't count null in length */

  if (outLen) *outLen = b.len;
  return b.data;
}

/* --- Extract recipients --- */

static int split_addrs(const char *list, char **out, int maxOut) {
  if (!list || !*list) return 0;
  int count = 0;
  const char *p = list;
  int depth = 0; /* track <...> nesting */

  while (*p && count < maxOut) {
    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;

    const char *start = p;
    while (*p) {
      if (*p == '<') depth++;
      else if (*p == '>') depth--;
      else if (*p == ',' && depth == 0) break;
      p++;
    }

    /* Extract and trim this address */
    long len = (long)(p - start);
    while (len > 0 && (start[len-1] == ' ' || start[len-1] == '\t')) len--;

    if (len > 0) {
      char full[512];
      if (len >= (long)sizeof(full)) len = sizeof(full) - 1;
      memcpy(full, start, len);
      full[len] = '\0';

      char addr[256];
      extract_addr(full, addr, sizeof(addr));
      if (*addr) {
        out[count] = strdup(addr);
        count++;
      }
    }

    if (*p == ',') p++;
  }
  return count;
}

char **crispy_msg_recipients(const CrispyMsg *msg, int *count) {
  char **list = (char **)calloc(128, sizeof(char *));
  if (!list) return NULL;
  int n = 0;

  n += split_addrs(msg->to, list + n, 128 - n);
  n += split_addrs(msg->cc, list + n, 128 - n);
  n += split_addrs(msg->bcc, list + n, 128 - n);

  list[n] = NULL; /* null-terminate */
  if (count) *count = n;
  return list;
}

/* --- MIME part header helpers --- */

/* Extract a header value from a MIME part header block */
static char *part_header_value(const char *headers, long headLen,
                               const char *name) {
  MailHeadSpec hs;
  /* Temporarily null-terminate */
  char *tmp = (char *)malloc(headLen + 1);
  if (!tmp) return NULL;
  memcpy(tmp, headers, headLen);
  tmp[headLen] = '\0';

  char *val = NULL;
  if (crispy_headparse_find(tmp, name, &hs))
    crispy_headparse_get_value(tmp, &hs, &val);
  free(tmp);
  return val;
}

/* Extract a parameter from a header value, e.g. boundary from
 * "multipart/mixed; boundary=\"abc\"" */
static char *extract_param(const char *value, const char *param) {
  if (!value || !param) return NULL;
  size_t plen = strlen(param);
  const char *p = value;

  while ((p = strcasestr(p, param)) != NULL) {
    p += plen;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '=') continue;
    p++;
    while (*p == ' ' || *p == '\t') p++;

    char *result;
    if (*p == '"') {
      p++;
      const char *end = strchr(p, '"');
      if (!end) return NULL;
      result = (char *)malloc(end - p + 1);
      memcpy(result, p, end - p);
      result[end - p] = '\0';
    } else {
      const char *end = p;
      while (*end && *end != ';' && *end != ' ' && *end != '\t' &&
             *end != '\r' && *end != '\n')
        end++;
      result = (char *)malloc(end - p + 1);
      memcpy(result, p, end - p);
      result[end - p] = '\0';
    }
    return result;
  }
  return NULL;
}

/* Find body start (after \r\n\r\n or \n\n) within a region */
static const char *find_body(const char *data, const char *end) {
  const char *p = data;
  while (p < end - 1) {
    if (p[0] == '\r' && p[1] == '\n' && p + 3 < end &&
        p[2] == '\r' && p[3] == '\n')
      return p + 4;
    if (p[0] == '\n' && p[1] == '\n')
      return p + 2;
    p++;
  }
  return NULL;
}

/* Parse a single MIME part (headers + body) into a CrispyMsgPart */
static int parse_one_part(const char *partStart, long partLen,
                          CrispyMsgPart *out) {
  memset(out, 0, sizeof(*out));
  const char *partEnd = partStart + partLen;

  const char *body = find_body(partStart, partEnd);
  long headLen = body ? (long)(body - partStart) : partLen;
  if (!body) body = partStart; /* no headers, all body */

  long bodyLen = (long)(partEnd - body);

  /* Parse Content-Type */
  char *ct = part_header_value(partStart, headLen, "Content-Type: ");
  if (ct) {
    /* Extract just the type, e.g. "text/plain" from "text/plain; charset=UTF-8" */
    char *semi = strchr(ct, ';');
    if (semi) *semi = '\0';
    /* Trim whitespace */
    char *e = ct + strlen(ct) - 1;
    while (e > ct && (*e == ' ' || *e == '\r' || *e == '\n')) *e-- = '\0';
    out->mime_type = strdup(ct);

    /* Extract charset */
    if (semi) *semi = ';'; /* restore for param extraction */
    out->charset = extract_param(ct, "charset");
    char *name = extract_param(ct, "name");
    if (name) { out->filename = name; out->is_attachment = true; }
    free(ct);
  } else {
    out->mime_type = strdup("text/plain");
  }

  /* Parse Content-Transfer-Encoding */
  char *cte = part_header_value(partStart, headLen,
                                "Content-Transfer-Encoding: ");

  /* Parse Content-Disposition */
  char *cd = part_header_value(partStart, headLen,
                               "Content-Disposition: ");
  if (cd) {
    if (strncasecmp(cd, "attachment", 10) == 0) {
      out->is_attachment = true;
      if (!out->filename)
        out->filename = extract_param(cd, "filename");
    }
    free(cd);
  }

  /* Parse Content-ID */
  char *cid = part_header_value(partStart, headLen, "Content-ID: ");
  if (cid) {
    /* Strip angle brackets */
    if (cid[0] == '<') {
      char *gt = strchr(cid, '>');
      if (gt) *gt = '\0';
      out->content_id = strdup(cid + 1);
      free(cid);
    } else {
      out->content_id = cid;
    }
  }

  /* Decode body */
  if (cte && strcasecmp(cte, "base64") == 0) {
    out->data = crispy_base64_decode(body, bodyLen, &out->data_len);
  } else if (cte && strcasecmp(cte, "quoted-printable") == 0) {
    out->data = crispy_qp_decode(body, bodyLen, &out->data_len);
  } else {
    /* 7bit, 8bit, binary — copy as-is */
    out->data = (char *)malloc(bodyLen + 1);
    if (out->data) {
      memcpy(out->data, body, bodyLen);
      out->data[bodyLen] = '\0';
      out->data_len = bodyLen;
    }
  }

  /* Convert charset to UTF-8 for text parts */
  if (out->data && out->charset && out->mime_type &&
      strncasecmp(out->mime_type, "text/", 5) == 0 &&
      strcasecmp(out->charset, "UTF-8") != 0 &&
      strcasecmp(out->charset, "US-ASCII") != 0) {
    char *utf8 = crispy_charset_to_utf8(out->data, out->data_len, out->charset);
    if (utf8) {
      free(out->data);
      out->data = utf8;
      out->data_len = (long)strlen(utf8);
      free(out->charset);
      out->charset = strdup("UTF-8");
    }
  }

  free(cte);
  return 0;
}

/* Recursively parse multipart MIME, appending parts to a growing array */
static int parse_multipart(const char *body, long bodyLen,
                           const char *boundary,
                           CrispyMsgPart **parts, int *count, int *cap) {
  size_t blen = strlen(boundary);
  const char *end = body + bodyLen;
  const char *p = body;

  /* Find first boundary */
  while (p < end) {
    if (p[0] == '-' && p[1] == '-' && strncmp(p + 2, boundary, blen) == 0) {
      p += 2 + blen;
      /* Skip to end of line */
      while (p < end && *p != '\n') p++;
      if (p < end) p++;
      break;
    }
    while (p < end && *p != '\n') p++;
    if (p < end) p++;
  }

  /* Parse each part between boundaries */
  while (p < end) {
    /* Find next boundary */
    const char *partStart = p;
    const char *partEnd = NULL;

    while (p < end) {
      if (p[0] == '-' && p[1] == '-' && strncmp(p + 2, boundary, blen) == 0) {
        partEnd = p;
        /* Check for closing boundary (--boundary--) */
        p += 2 + blen;
        /* Skip to end of line */
        while (p < end && *p != '\n') p++;
        if (p < end) p++;
        break;
      }
      while (p < end && *p != '\n') p++;
      if (p < end) p++;
    }
    if (!partEnd) partEnd = end;

    /* Strip trailing \r\n before boundary */
    while (partEnd > partStart &&
           (partEnd[-1] == '\r' || partEnd[-1] == '\n'))
      partEnd--;

    long partLen = (long)(partEnd - partStart);
    if (partLen <= 0) continue;

    /* Check if this part is itself multipart */
    char *subCt = part_header_value(partStart, partLen, "Content-Type: ");
    if (subCt && strncasecmp(subCt, "multipart/", 10) == 0) {
      char *subBoundary = extract_param(subCt, "boundary");
      if (subBoundary) {
        const char *subBody = find_body(partStart, partEnd);
        if (subBody) {
          parse_multipart(subBody, (long)(partEnd - subBody),
                          subBoundary, parts, count, cap);
        }
        free(subBoundary);
      }
      free(subCt);
      continue;
    }
    free(subCt);

    /* Parse this part */
    if (*count >= *cap) {
      *cap = *cap ? *cap * 2 : 8;
      *parts = (CrispyMsgPart *)realloc(*parts, *cap * sizeof(CrispyMsgPart));
    }
    parse_one_part(partStart, partLen, &(*parts)[*count]);
    (*count)++;
  }

  return 0;
}

/* --- Parse received message --- */

int crispy_msg_parse(const char *raw, long rawLen, CrispyMsgParsed *out) {
  if (!raw || !out) return -1;
  memset(out, 0, sizeof(*out));

  const char *end = raw + rawLen;

  /* Find header/body boundary */
  const char *bodyStart = find_body(raw, end);

  /* Parse standard headers with RFC 2047 decoding */
  MailHeadSpec hs;
  char *rawval;

  #define PARSE_HEADER_DECODED(name, field) \
    if (crispy_headparse_find(raw, name, &hs) && \
        crispy_headparse_get_value(raw, &hs, &rawval) == 0) { \
      out->field = crispy_decode_header(rawval); \
      free(rawval); \
    }

  PARSE_HEADER_DECODED("From: ", from)
  PARSE_HEADER_DECODED("To: ", to)
  PARSE_HEADER_DECODED("Cc: ", cc)
  PARSE_HEADER_DECODED("Subject: ", subject)
  PARSE_HEADER_DECODED("Date: ", date)
  PARSE_HEADER_DECODED("Message-Id: ", message_id)
  PARSE_HEADER_DECODED("In-Reply-To: ", in_reply_to)

  #undef PARSE_HEADER_DECODED

  /* Check if this is multipart */
  char *ct = NULL;
  if (crispy_headparse_find(raw, "Content-Type: ", &hs))
    crispy_headparse_get_value(raw, &hs, &ct);

  if (ct && strncasecmp(ct, "multipart/", 10) == 0) {
    /* Multipart message — extract boundary and parse parts */
    char *boundary = extract_param(ct, "boundary");
    if (boundary && bodyStart) {
      int cap = 0;
      parse_multipart(bodyStart, (long)(end - bodyStart), boundary,
                      &out->parts, &out->part_count, &cap);
      free(boundary);
    }
  } else if (bodyStart && bodyStart < end) {
    /* Single-part message — decode based on Content-Transfer-Encoding */
    out->parts = (CrispyMsgPart *)calloc(1, sizeof(CrispyMsgPart));
    parse_one_part(raw, rawLen, &out->parts[0]);
    out->part_count = 1;
  }

  free(ct);

  /* Set convenience pointers to first text/plain and text/html parts */
  for (int i = 0; i < out->part_count; i++) {
    if (!out->parts[i].is_attachment && out->parts[i].mime_type) {
      if (!out->body_plain && strcasecmp(out->parts[i].mime_type, "text/plain") == 0) {
        out->body_plain = out->parts[i].data;
        out->body_plain_len = out->parts[i].data_len;
      }
      if (!out->body_html && strcasecmp(out->parts[i].mime_type, "text/html") == 0) {
        out->body_html = out->parts[i].data;
        out->body_html_len = out->parts[i].data_len;
      }
    }
  }

  return 0;
}

void crispy_msg_parsed_free(CrispyMsgParsed *parsed) {
  if (!parsed) return;
  free(parsed->from);
  free(parsed->to);
  free(parsed->cc);
  free(parsed->subject);
  free(parsed->date);
  free(parsed->message_id);
  free(parsed->in_reply_to);
  for (int i = 0; i < parsed->part_count; i++) {
    free(parsed->parts[i].mime_type);
    free(parsed->parts[i].charset);
    free(parsed->parts[i].filename);
    free(parsed->parts[i].content_id);
    free(parsed->parts[i].data);
  }
  free(parsed->parts);
  memset(parsed, 0, sizeof(*parsed));
}
