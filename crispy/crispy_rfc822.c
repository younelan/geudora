/* crispy_rfc822.c — RFC822 message utilities
 * Part of crispy: standalone mail library.
 * Ported from CrispinIMAP rfc822.c + mail.c, rewritten clean.
 *
 * Portable: POSIX + Windows. No Eudora dependency.
 */

#include "crispy_rfc822.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* RFC822 special characters */
static const char rfc822_specials[] = "()<>@,;:\\\"[].";
/* tspecials for MIME parameter quoting (RFC 2045) */
static const char *rfc822_tspecials_str = "()<>@,;:\\\"[]/?=";

static const char *months_short[] = {
  "Jan","Feb","Mar","Apr","May","Jun",
  "Jul","Aug","Sep","Oct","Nov","Dec"
};

static const char *days_short[] = {
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

/* ================================================================
 * String helpers
 * ================================================================ */

static char *my_strdup(const char *s) {
  if (!s) return NULL;
  return strdup(s);
}

/* Append string to a dynamically growing buffer */
typedef struct {
  char *data;
  size_t len;
  size_t cap;
} StrBuf;

static void sb_init(StrBuf *sb) {
  sb->cap = 256;
  sb->data = (char *)malloc(sb->cap);
  sb->data[0] = '\0';
  sb->len = 0;
}

static void sb_append(StrBuf *sb, const char *s) {
  size_t slen = strlen(s);
  if (sb->len + slen + 1 > sb->cap) {
    while (sb->len + slen + 1 > sb->cap) sb->cap *= 2;
    sb->data = (char *)realloc(sb->data, sb->cap);
  }
  memcpy(sb->data + sb->len, s, slen);
  sb->len += slen;
  sb->data[sb->len] = '\0';
}

static void sb_appendc(StrBuf *sb, char c) {
  if (sb->len + 2 > sb->cap) {
    sb->cap *= 2;
    sb->data = (char *)realloc(sb->data, sb->cap);
  }
  sb->data[sb->len++] = c;
  sb->data[sb->len] = '\0';
}

static char *sb_detach(StrBuf *sb) {
  char *r = sb->data;
  sb->data = NULL;
  sb->len = sb->cap = 0;
  return r;
}

/* ================================================================
 * RFC822 comment and whitespace skipping
 * ================================================================ */

const char *crispy_rfc822_skip_comment(const char *s) {
  if (!s || *s != '(') return s;
  s++; /* skip '(' */
  int depth = 1;
  while (*s && depth > 0) {
    if (*s == '(') depth++;
    else if (*s == ')') depth--;
    else if (*s == '\\' && s[1]) s++; /* skip escaped char */
    if (depth > 0) s++;
  }
  if (*s == ')') s++;
  return s;
}

void crispy_rfc822_skipws(const char **s) {
  while (**s) {
    if (**s == ' ' || **s == '\t' || **s == '\r' || **s == '\n') {
      (*s)++;
    } else if (**s == '(') {
      *s = crispy_rfc822_skip_comment(*s);
    } else {
      break;
    }
  }
}

/* ================================================================
 * RFC822 quoting
 * ================================================================ */

char *crispy_rfc822_quote(const char *src) {
  if (!src) return NULL;
  /* Check if quoting needed */
  if (!strpbrk(src, rfc822_specials) && !strpbrk(src, " \t")) {
    return strdup(src);
  }
  /* Need to quote */
  size_t len = strlen(src);
  char *out = (char *)malloc(len * 2 + 3);
  if (!out) return NULL;
  size_t o = 0;
  out[o++] = '"';
  for (size_t i = 0; i < len; i++) {
    if (src[i] == '"' || src[i] == '\\') out[o++] = '\\';
    out[o++] = src[i];
  }
  out[o++] = '"';
  out[o] = '\0';
  return out;
}

char *crispy_rfc822_unquote(char *src) {
  if (!src) return NULL;
  char *dst = src;
  char *s = src;
  while (*s) {
    if (*s == '"') { s++; continue; }
    if (*s == '\\' && s[1]) { s++; }
    *dst++ = *s++;
  }
  *dst = '\0';
  return src;
}

/* ================================================================
 * RFC822 address parsing
 *
 * Handles: "Name <user@host>", "user@host", "<user@host>",
 *          "Name <user@host>, Name2 <user2@host2>",
 *          group syntax "Group: addr1, addr2 ;"
 * ================================================================ */

/* Parse a phrase (display name) — sequence of words, quoted strings, comments */
static const char *parse_phrase(const char *s, char *buf, size_t bufsz) {
  size_t o = 0;
  crispy_rfc822_skipws(&s);
  if (!*s || *s == '<' || *s == '@' || *s == ',' || *s == ';') return NULL;

  bool in_quote = false;
  const char *start = s;
  while (*s) {
    if (*s == '"') {
      in_quote = !in_quote;
      s++;
    } else if (in_quote) {
      if (*s == '\\' && s[1]) {
        if (o < bufsz - 1) buf[o++] = s[1];
        s += 2;
      } else {
        if (o < bufsz - 1) buf[o++] = *s;
        s++;
      }
    } else if (*s == '<' || *s == ',' || *s == ';' || *s == ':') {
      break;
    } else if (*s == '(') {
      s = crispy_rfc822_skip_comment(s);
    } else if (*s == ' ' || *s == '\t') {
      if (o < bufsz - 1) buf[o++] = ' ';
      while (*s == ' ' || *s == '\t') s++;
    } else {
      if (o < bufsz - 1) buf[o++] = *s;
      s++;
    }
  }
  /* Trim trailing space */
  while (o > 0 && buf[o-1] == ' ') o--;
  buf[o] = '\0';
  return (s > start && o > 0) ? s : NULL;
}

/* Parse addr-spec: localpart@domain */
static const char *parse_addrspec(const char *s, char *mailbox, size_t mbsz,
                                   char *host, size_t hsz,
                                   const char *default_host) {
  crispy_rfc822_skipws(&s);
  size_t mo = 0, ho = 0;
  /* Local part — may be quoted */
  if (*s == '"') {
    s++;
    while (*s && *s != '"') {
      if (*s == '\\' && s[1]) s++;
      if (mo < mbsz - 1) mailbox[mo++] = *s;
      s++;
    }
    if (*s == '"') s++;
  } else {
    while (*s && *s != '@' && *s != '>' && *s != ',' && *s != ';' &&
           *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') {
      if (mo < mbsz - 1) mailbox[mo++] = *s;
      s++;
    }
  }
  mailbox[mo] = '\0';
  /* Domain */
  if (*s == '@') {
    s++;
    while (*s && *s != '>' && *s != ',' && *s != ';' &&
           *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n' && *s != ')') {
      if (ho < hsz - 1) host[ho++] = *s;
      s++;
    }
  } else if (default_host) {
    snprintf(host, hsz, "%s", default_host);
    ho = strlen(host);
  }
  host[ho] = '\0';
  return (mo > 0) ? s : NULL;
}

/* Parse route-addr: <@route:user@host> or <user@host> */
static const char *parse_routeaddr(const char *s, ImapAddress *adr,
                                    const char *default_host) {
  crispy_rfc822_skipws(&s);
  if (*s != '<') return NULL;
  s++; /* skip '<' */
  crispy_rfc822_skipws(&s);

  /* Check for route: @domain1,@domain2: */
  if (*s == '@') {
    StrBuf route;
    sb_init(&route);
    while (*s == '@') {
      if (route.len > 0) sb_appendc(&route, ',');
      sb_appendc(&route, '@');
      s++;
      while (*s && *s != ',' && *s != ':') { sb_appendc(&route, *s); s++; }
      if (*s == ',') s++;
    }
    if (*s == ':') s++;
    adr->adl = sb_detach(&route);
  }

  char mb[512], host[512];
  s = parse_addrspec(s, mb, sizeof(mb), host, sizeof(host), default_host);
  if (!s) return NULL;
  adr->mailbox = my_strdup(mb);
  adr->host = my_strdup(host);

  crispy_rfc822_skipws(&s);
  if (*s == '>') s++;
  return s;
}

ImapAddress *crispy_rfc822_parse_mailbox(const char *text, const char *default_host) {
  if (!text || !*text) return NULL;
  const char *s = text;
  crispy_rfc822_skipws(&s);
  if (!*s) return NULL;

  ImapAddress *adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
  if (!adr) return NULL;

  /* Try route-addr first: <user@host> */
  if (*s == '<') {
    s = parse_routeaddr(s, adr, default_host);
    if (!s) { free(adr); return NULL; }
    return adr;
  }

  /* Try "phrase <route-addr>" */
  char phrase[1024] = "";
  const char *after_phrase = parse_phrase(s, phrase, sizeof(phrase));
  if (after_phrase) {
    crispy_rfc822_skipws(&after_phrase);
    if (*after_phrase == '<') {
      adr->name = my_strdup(phrase);
      after_phrase = parse_routeaddr(after_phrase, adr, default_host);
      if (!after_phrase) { crispy_imap_free_addresses(adr); return NULL; }
      return adr;
    }
  }

  /* Plain addr-spec: user@host */
  char mb[512], host[512];
  s = parse_addrspec(s, mb, sizeof(mb), host, sizeof(host), default_host);
  if (!s || !mb[0]) { free(adr); return NULL; }
  adr->mailbox = my_strdup(mb);
  adr->host = my_strdup(host);
  return adr;
}

ImapAddress *crispy_rfc822_parse_adrlist(const char *text, const char *default_host) {
  if (!text || !*text) return NULL;
  ImapAddress *head = NULL, *tail = NULL;
  const char *s = text;

  while (s && *s) {
    crispy_rfc822_skipws(&s);
    if (!*s) break;

    /* Check for group syntax "Name:" */
    const char *colon = NULL;
    {
      const char *t = s;
      char tmp[1024];
      const char *after = parse_phrase(t, tmp, sizeof(tmp));
      if (after) {
        crispy_rfc822_skipws(&after);
        if (*after == ':') colon = after;
      }
    }

    if (colon) {
      /* Group: skip name and ':', parse addresses until ';' */
      s = colon + 1;
      while (s && *s && *s != ';') {
        crispy_rfc822_skipws(&s);
        if (!*s || *s == ';') break;

        /* Save position, try parsing a mailbox */
        ImapAddress *adr = NULL;
        const char *saved = s;

        /* Try "phrase <route-addr>" */
        char phrase[1024] = "";
        const char *ap = parse_phrase(s, phrase, sizeof(phrase));
        if (ap) {
          crispy_rfc822_skipws(&ap);
          if (*ap == '<') {
            adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
            if (phrase[0]) adr->name = my_strdup(phrase);
            ap = parse_routeaddr(ap, adr, default_host);
            if (ap) s = ap; else { crispy_imap_free_addresses(adr); adr = NULL; s = saved; }
          }
        }
        if (!adr && *s == '<') {
          adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
          const char *ap2 = parse_routeaddr(s, adr, default_host);
          if (ap2) s = ap2; else { free(adr); adr = NULL; }
        }
        if (!adr) {
          char mb[512], host[512];
          const char *ap3 = parse_addrspec(s, mb, sizeof(mb), host, sizeof(host), default_host);
          if (ap3 && mb[0]) {
            adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
            adr->mailbox = my_strdup(mb);
            adr->host = my_strdup(host);
            s = ap3;
          } else {
            s++; /* skip unparseable char */
          }
        }
        if (adr) {
          if (!head) head = adr; else tail->next = adr;
          tail = adr;
        }
        crispy_rfc822_skipws(&s);
        if (*s == ',') s++;
      }
      if (*s == ';') s++;
      crispy_rfc822_skipws(&s);
      if (*s == ',') s++;
      continue;
    }

    /* Regular address */
    ImapAddress *adr = NULL;
    const char *saved = s;

    /* Try "phrase <route-addr>" */
    char phrase[1024] = "";
    const char *ap = parse_phrase(s, phrase, sizeof(phrase));
    if (ap) {
      crispy_rfc822_skipws(&ap);
      if (*ap == '<') {
        adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
        if (phrase[0]) adr->name = my_strdup(phrase);
        ap = parse_routeaddr(ap, adr, default_host);
        if (ap) s = ap; else { crispy_imap_free_addresses(adr); adr = NULL; s = saved; }
      }
    }
    if (!adr && *s == '<') {
      adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
      const char *ap2 = parse_routeaddr(s, adr, default_host);
      if (ap2) s = ap2; else { free(adr); adr = NULL; }
    }
    if (!adr) {
      char mb[512], host[512];
      const char *ap3 = parse_addrspec(s, mb, sizeof(mb), host, sizeof(host), default_host);
      if (ap3 && mb[0]) {
        adr = (ImapAddress *)calloc(1, sizeof(ImapAddress));
        adr->mailbox = my_strdup(mb);
        adr->host = my_strdup(host);
        s = ap3;
      } else {
        break; /* can't parse further */
      }
    }
    if (adr) {
      if (!head) head = adr; else tail->next = adr;
      tail = adr;
    }
    crispy_rfc822_skipws(&s);
    if (*s == ',') s++;
  }
  return head;
}

/* ================================================================
 * RFC822 address formatting
 * ================================================================ */

void crispy_rfc822_address(char *buf, size_t bufsize, ImapAddress *adr) {
  buf[0] = '\0';
  if (!adr || !adr->mailbox) return;
  if (adr->host && adr->host[0] && adr->host[0] != '@') {
    snprintf(buf, bufsize, "%s@%s", adr->mailbox, adr->host);
  } else {
    snprintf(buf, bufsize, "%s", adr->mailbox);
  }
}

char *crispy_rfc822_format_address(ImapAddress *adr) {
  if (!adr) return NULL;
  char addr[512];
  crispy_rfc822_address(addr, sizeof(addr), adr);

  if (adr->name && adr->name[0]) {
    char *quoted = crispy_rfc822_quote(adr->name);
    size_t len = strlen(quoted) + strlen(addr) + 4;
    char *result = (char *)malloc(len);
    snprintf(result, len, "%s <%s>", quoted, addr);
    free(quoted);
    return result;
  }
  return strdup(addr);
}

char *crispy_rfc822_write_address(ImapAddress *adr) {
  if (!adr) return strdup("");
  StrBuf sb;
  sb_init(&sb);
  bool first = true;
  while (adr) {
    if (!first && adr->mailbox) sb_append(&sb, ", ");
    if (adr->name && adr->name[0]) {
      char *q = crispy_rfc822_quote(adr->name);
      sb_append(&sb, q);
      free(q);
      sb_append(&sb, " <");
      char addr[512];
      crispy_rfc822_address(addr, sizeof(addr), adr);
      sb_append(&sb, addr);
      sb_append(&sb, ">");
    } else if (adr->mailbox) {
      char addr[512];
      crispy_rfc822_address(addr, sizeof(addr), adr);
      sb_append(&sb, addr);
    }
    first = false;
    adr = adr->next;
  }
  return sb_detach(&sb);
}

ImapAddress *crispy_rfc822_copy_adrlist(ImapAddress *adr) {
  ImapAddress *head = NULL, *tail = NULL;
  while (adr) {
    ImapAddress *copy = (ImapAddress *)calloc(1, sizeof(ImapAddress));
    copy->name = my_strdup(adr->name);
    copy->adl = my_strdup(adr->adl);
    copy->mailbox = my_strdup(adr->mailbox);
    copy->host = my_strdup(adr->host);
    if (!head) head = copy; else tail->next = copy;
    tail = copy;
    adr = adr->next;
  }
  return head;
}

/* ================================================================
 * RFC822 header building
 * ================================================================ */

static const char *body_type_names[] = {
  "text", "multipart", "message", "application",
  "audio", "image", "video", "model", "other"
};

static const char *encoding_names[] = {
  "7bit", "8bit", "binary", "base64", "quoted-printable", "other"
};

char *crispy_rfc822_body_header(ImapBodyPart *body) {
  if (!body) return strdup("");
  StrBuf sb;
  sb_init(&sb);

  /* Content-Type */
  const char *type = (body->type <= IMAP_TYPE_OTHER) ?
    body_type_names[body->type] : "application";
  const char *subtype = body->subtype ? body->subtype : "octet-stream";
  char ct[512];
  snprintf(ct, sizeof(ct), "Content-Type: %s/%s", type, subtype);
  sb_append(&sb, ct);

  /* Parameters */
  ImapParam *p = body->params;
  while (p) {
    if (p->name && p->value) {
      sb_append(&sb, "; ");
      sb_append(&sb, p->name);
      sb_appendc(&sb, '=');
      /* Quote value if it contains tspecials */
      if (strpbrk(p->value, rfc822_tspecials_str)) {
        sb_appendc(&sb, '"');
        /* Escape backslashes and quotes within value */
        for (const char *v = p->value; *v; v++) {
          if (*v == '"' || *v == '\\') sb_appendc(&sb, '\\');
          sb_appendc(&sb, *v);
        }
        sb_appendc(&sb, '"');
      } else {
        sb_append(&sb, p->value);
      }
    }
    p = p->next;
  }
  if (!body->params && body->type == IMAP_TYPE_TEXT)
    sb_append(&sb, "; charset=US-ASCII");
  sb_append(&sb, "\r\n");

  /* Content-Transfer-Encoding */
  if (body->encoding > IMAP_ENC_7BIT && body->encoding <= IMAP_ENC_OTHER) {
    char enc[128];
    snprintf(enc, sizeof(enc), "Content-Transfer-Encoding: %s\r\n",
             encoding_names[body->encoding]);
    sb_append(&sb, enc);
  }

  /* Content-ID */
  if (body->id) {
    char id[512];
    snprintf(id, sizeof(id), "Content-ID: %s\r\n", body->id);
    sb_append(&sb, id);
  }

  /* Content-Description */
  if (body->description) {
    char desc[512];
    snprintf(desc, sizeof(desc), "Content-Description: %s\r\n", body->description);
    sb_append(&sb, desc);
  }

  /* Content-MD5 */
  if (body->md5) {
    char md5[128];
    snprintf(md5, sizeof(md5), "Content-MD5: %s\r\n", body->md5);
    sb_append(&sb, md5);
  }

  /* Content-Disposition */
  if (body->disposition.type) {
    char disp[512];
    snprintf(disp, sizeof(disp), "Content-Disposition: %s", body->disposition.type);
    sb_append(&sb, disp);
    ImapParam *dp = body->disposition.params;
    while (dp) {
      if (dp->name && dp->value) {
        char dparam[256];
        snprintf(dparam, sizeof(dparam), "; %s=\"%s\"", dp->name, dp->value);
        sb_append(&sb, dparam);
      }
      dp = dp->next;
    }
    sb_append(&sb, "\r\n");
  }

  /* Content-Language */
  if (body->language) {
    char lang[256];
    snprintf(lang, sizeof(lang), "Content-Language: %s\r\n", body->language);
    sb_append(&sb, lang);
  }

  return sb_detach(&sb);
}

char *crispy_rfc822_build_header(ImapEnvelope *env, ImapBodyPart *body) {
  if (!env) return strdup("\r\n");
  StrBuf sb;
  sb_init(&sb);

  /* Date */
  if (env->date) {
    sb_append(&sb, "Date: ");
    sb_append(&sb, env->date);
    sb_append(&sb, "\r\n");
  }

  /* From */
  if (env->from) {
    char *addr = crispy_rfc822_write_address(env->from);
    sb_append(&sb, "From: ");
    sb_append(&sb, addr);
    sb_append(&sb, "\r\n");
    free(addr);
  }

  /* Sender (only if different from From) */
  if (env->sender) {
    char *addr = crispy_rfc822_write_address(env->sender);
    sb_append(&sb, "Sender: ");
    sb_append(&sb, addr);
    sb_append(&sb, "\r\n");
    free(addr);
  }

  /* Reply-To */
  if (env->reply_to) {
    char *addr = crispy_rfc822_write_address(env->reply_to);
    sb_append(&sb, "Reply-To: ");
    sb_append(&sb, addr);
    sb_append(&sb, "\r\n");
    free(addr);
  }

  /* Subject */
  if (env->subject) {
    sb_append(&sb, "Subject: ");
    sb_append(&sb, env->subject);
    sb_append(&sb, "\r\n");
  }

  /* To */
  if (env->to) {
    char *addr = crispy_rfc822_write_address(env->to);
    sb_append(&sb, "To: ");
    sb_append(&sb, addr);
    sb_append(&sb, "\r\n");
    free(addr);
  } else if (env->bcc && !env->cc) {
    sb_append(&sb, "To: undisclosed recipients: ;\r\n");
  }

  /* Cc */
  if (env->cc) {
    char *addr = crispy_rfc822_write_address(env->cc);
    sb_append(&sb, "Cc: ");
    sb_append(&sb, addr);
    sb_append(&sb, "\r\n");
    free(addr);
  }

  /* In-Reply-To */
  if (env->in_reply_to) {
    sb_append(&sb, "In-Reply-To: ");
    sb_append(&sb, env->in_reply_to);
    sb_append(&sb, "\r\n");
  }

  /* Message-ID */
  if (env->message_id) {
    sb_append(&sb, "Message-ID: ");
    sb_append(&sb, env->message_id);
    sb_append(&sb, "\r\n");
  }

  /* MIME headers from body */
  if (body) {
    sb_append(&sb, "MIME-Version: 1.0\r\n");
    char *bh = crispy_rfc822_body_header(body);
    sb_append(&sb, bh);
    free(bh);
  }

  /* Terminating blank line */
  sb_append(&sb, "\r\n");
  return sb_detach(&sb);
}

/* ================================================================
 * RFC822 message output
 * ================================================================ */

char *crispy_rfc822_build_message(ImapEnvelope *env, ImapBodyPart *body,
                                   const char *body_text, long body_len) {
  char *headers = crispy_rfc822_build_header(env, body);
  if (body_len < 0) body_len = body_text ? (long)strlen(body_text) : 0;
  size_t hlen = strlen(headers);
  char *msg = (char *)malloc(hlen + body_len + 1);
  if (msg) {
    memcpy(msg, headers, hlen);
    if (body_text && body_len > 0) memcpy(msg + hlen, body_text, body_len);
    msg[hlen + body_len] = '\0';
  }
  free(headers);
  return msg;
}

/* ================================================================
 * Date parsing
 *
 * Accepts: "dd-Mon-yyyy hh:mm:ss +zzzz" (IMAP INTERNALDATE)
 *          "Mon, dd Mon yyyy hh:mm:ss +zzzz" (RFC822)
 *          "mm/dd/yyyy" (simple date)
 *          "dd Mon yyyy hh:mm:ss +zzzz"
 * ================================================================ */

static int month_from_name(const char *name) {
  for (int i = 0; i < 12; i++) {
    if (strncasecmp(name, months_short[i], 3) == 0) return i + 1;
  }
  return 0;
}

bool crispy_rfc822_parse_date(const char *str, CrispyDate *d) {
  if (!str || !d) return false;
  memset(d, 0, sizeof(*d));
  const char *s = str;

  /* Skip leading whitespace */
  while (*s == ' ' || *s == '\t') s++;

  /* Skip day-of-week if present: "Mon, " or "Monday, " */
  if (isalpha((unsigned char)*s)) {
    const char *t = s;
    while (isalpha((unsigned char)*t)) t++;
    while (*t == ' ' || *t == '\t') t++;
    if (*t == ',') {
      s = t + 1;
      while (*s == ' ' || *s == '\t') s++;
    } else if (*t == '-' || isdigit((unsigned char)*t)) {
      /* Not a day name, might be month name in "dd-Mon-yyyy" — reset */
    } else {
      s = t; /* skip the alpha prefix */
    }
  }

  /* Try "dd-Mon-yyyy" or "dd Mon yyyy" */
  if (isdigit((unsigned char)*s)) {
    int n1 = (int)strtol(s, (char **)&s, 10);

    if (*s == '-' || *s == ' ') {
      char sep = *s;
      s++;
      if (isalpha((unsigned char)*s)) {
        /* dd-Mon-yyyy or dd Mon yyyy */
        d->day = n1;
        d->month = month_from_name(s);
        while (isalpha((unsigned char)*s)) s++;
        if (*s == sep || *s == ' ' || *s == '-') s++;
        d->year = (int)strtol(s, (char **)&s, 10);
        if (d->year < 100) d->year += (d->year < 70) ? 2000 : 1900;
      } else if (isdigit((unsigned char)*s) && sep == '/') {
        /* mm/dd/yyyy */
        d->month = n1;
        d->day = (int)strtol(s, (char **)&s, 10);
        if (*s == '/') s++;
        d->year = (int)strtol(s, (char **)&s, 10);
        if (d->year < 100) d->year += (d->year < 70) ? 2000 : 1900;
      }
    } else if (*s == '/') {
      /* mm/dd/yyyy */
      s++;
      d->month = n1;
      d->day = (int)strtol(s, (char **)&s, 10);
      if (*s == '/') s++;
      d->year = (int)strtol(s, (char **)&s, 10);
      if (d->year < 100) d->year += (d->year < 70) ? 2000 : 1900;
    }
  }

  if (d->year == 0) return false; /* couldn't parse date */

  /* Time: hh:mm:ss or hh:mm */
  while (*s == ' ' || *s == '\t') s++;
  if (isdigit((unsigned char)*s)) {
    d->hours = (int)strtol(s, (char **)&s, 10);
    if (*s == ':') {
      s++;
      d->minutes = (int)strtol(s, (char **)&s, 10);
    }
    if (*s == ':') {
      s++;
      d->seconds = (int)strtol(s, (char **)&s, 10);
    }
  }

  /* Timezone */
  while (*s == ' ' || *s == '\t') s++;
  if (*s == '+' || *s == '-') {
    int sign = (*s == '-') ? -1 : 1;
    s++;
    int tz = (int)strtol(s, (char **)&s, 10);
    if (tz > 99) {
      /* +0200 format */
      d->tz_offset_min = sign * ((tz / 100) * 60 + (tz % 100));
    } else {
      d->tz_offset_min = sign * tz * 60;
    }
  } else if (isalpha((unsigned char)*s)) {
    /* Symbolic timezone */
    if (strncasecmp(s, "GMT", 3) == 0 || strncasecmp(s, "UTC", 3) == 0)
      d->tz_offset_min = 0;
    else if (strncasecmp(s, "EST", 3) == 0) d->tz_offset_min = -300;
    else if (strncasecmp(s, "EDT", 3) == 0) d->tz_offset_min = -240;
    else if (strncasecmp(s, "CST", 3) == 0) d->tz_offset_min = -360;
    else if (strncasecmp(s, "CDT", 3) == 0) d->tz_offset_min = -300;
    else if (strncasecmp(s, "MST", 3) == 0) d->tz_offset_min = -420;
    else if (strncasecmp(s, "MDT", 3) == 0) d->tz_offset_min = -360;
    else if (strncasecmp(s, "PST", 3) == 0) d->tz_offset_min = -480;
    else if (strncasecmp(s, "PDT", 3) == 0) d->tz_offset_min = -420;
    else if (strncasecmp(s, "CET", 3) == 0) d->tz_offset_min = 60;
    else if (strncasecmp(s, "CEST", 4) == 0) d->tz_offset_min = 120;
    else if (strncasecmp(s, "JST", 3) == 0) d->tz_offset_min = 540;
  }

  return (d->month >= 1 && d->month <= 12 && d->day >= 1 && d->day <= 31);
}

char *crispy_rfc822_format_imap_date(char *buf, size_t bufsize, CrispyDate *d) {
  int tzh = abs(d->tz_offset_min) / 60;
  int tzm = abs(d->tz_offset_min) % 60;
  snprintf(buf, bufsize, "%02d-%s-%04d %02d:%02d:%02d %c%02d%02d",
           d->day, months_short[d->month - 1], d->year,
           d->hours, d->minutes, d->seconds,
           d->tz_offset_min >= 0 ? '+' : '-', tzh, tzm);
  return buf;
}

char *crispy_rfc822_format_date(char *buf, size_t bufsize, CrispyDate *d) {
  /* Calculate day of week (Zeller-like) */
  int m = d->month, y = d->year;
  if (m <= 2) { m += 9; y--; } else m -= 3;
  int dow = (d->day + 2 + (7 + 31 * m) / 12 + y + y/4 + y/400 - y/100) % 7;

  int tzh = abs(d->tz_offset_min) / 60;
  int tzm = abs(d->tz_offset_min) % 60;
  snprintf(buf, bufsize, "%s, %02d %s %04d %02d:%02d:%02d %c%02d%02d",
           days_short[dow], d->day, months_short[d->month - 1], d->year,
           d->hours, d->minutes, d->seconds,
           d->tz_offset_min >= 0 ? '+' : '-', tzh, tzm);
  return buf;
}

char *crispy_rfc822_format_cdate(char *buf, size_t bufsize, CrispyDate *d) {
  int m = d->month, y = d->year;
  if (m <= 2) { m += 9; y--; } else m -= 3;
  int dow = (d->day + 2 + (7 + 31 * m) / 12 + y + y/4 + y/400 - y/100) % 7;

  snprintf(buf, bufsize, "%s %s %2d %02d:%02d:%02d %04d\n",
           days_short[dow], months_short[d->month - 1], d->day,
           d->hours, d->minutes, d->seconds, d->year);
  return buf;
}

/* ================================================================
 * BODYSTRUCTURE section navigation
 * ================================================================ */

ImapBodyPart *crispy_rfc822_sub_body(ImapBodyPart *body, const char *section) {
  if (!body) return NULL;
  if (!section || !*section) return body;

  const char *s = section;
  ImapBodyPart *b = body;

  while (*s) {
    if (!isdigit((unsigned char)*s)) return NULL;
    unsigned long idx = strtoul(s, (char **)&s, 10);
    if (*s == '.') s++;

    if (b->type == IMAP_TYPE_MULTIPART) {
      /* Find the idx-th child */
      ImapBodyPart *child = b->subparts;
      for (unsigned long i = 1; child && i < idx; i++)
        child = child->next;
      if (!child) return NULL;
      b = child;
    } else if (b->type == IMAP_TYPE_MESSAGE && b->nested_body && idx == 1) {
      /* message/rfc822 — descend into nested body */
      b = b->nested_body;
    } else if (idx == 1) {
      /* Non-multipart, section 1 = the body itself */
    } else {
      return NULL;
    }

    /* If more sections, check we can descend */
    if (*s) {
      if (b->type == IMAP_TYPE_MULTIPART) continue;
      if (b->type == IMAP_TYPE_MESSAGE && b->nested_body) {
        b = b->nested_body;
        continue;
      }
      return NULL;
    }
  }
  return b;
}

/* ================================================================
 * Subject stripping for threading
 * ================================================================ */

const char *crispy_rfc822_skip_re(const char *subject) {
  if (!subject) return NULL;
  const char *s = subject;
  while (1) {
    while (*s == ' ' || *s == '\t') s++;
    if ((s[0] == 'R' || s[0] == 'r') &&
        (s[1] == 'E' || s[1] == 'e') &&
        s[2] == ':') {
      s += 3;
    } else if ((s[0] == 'R' || s[0] == 'r') &&
               (s[1] == 'E' || s[1] == 'e') &&
               s[2] == '[' && isdigit((unsigned char)s[3])) {
      /* Re[2]: style */
      s += 3;
      while (isdigit((unsigned char)*s)) s++;
      if (*s == ']') s++;
      if (*s == ':') s++;
    } else {
      break;
    }
  }
  return s;
}

const char *crispy_rfc822_skip_fwd(const char *subject) {
  if (!subject) return NULL;
  const char *s = subject;
  while (1) {
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '(' &&
        (s[1] == 'F' || s[1] == 'f') &&
        (s[2] == 'W' || s[2] == 'w') &&
        (s[3] == 'D' || s[3] == 'd') &&
        s[4] == ')') {
      s += 5;
    } else if (strncasecmp(s, "fwd:", 4) == 0) {
      s += 4;
    } else if (strncasecmp(s, "[fwd:", 5) == 0) {
      /* [Fwd: ...] — skip to matching ] */
      const char *end = strchr(s, ']');
      if (end) s = end + 1; else break;
    } else {
      break;
    }
  }
  return s;
}

const char *crispy_rfc822_base_subject(const char *subject) {
  if (!subject) return NULL;
  const char *s = subject;
  const char *prev;
  do {
    prev = s;
    s = crispy_rfc822_skip_re(s);
    s = crispy_rfc822_skip_fwd(s);
  } while (s != prev);
  return s;
}

/* ================================================================
 * IMAP sequence set parsing
 * ================================================================ */

int crispy_rfc822_parse_sequence(const char *seqset, unsigned long max_uid,
                                  unsigned long **uids) {
  if (!seqset || !uids) return 0;
  *uids = NULL;
  int count = 0, cap = 128;
  *uids = (unsigned long *)calloc(cap, sizeof(unsigned long));

  const char *s = seqset;
  while (*s) {
    while (*s == ' ') s++;
    if (!*s) break;

    unsigned long start, end;
    if (*s == '*') {
      start = max_uid;
      s++;
    } else {
      start = strtoul(s, (char **)&s, 10);
    }

    if (*s == ':') {
      s++;
      if (*s == '*') {
        end = max_uid;
        s++;
      } else {
        end = strtoul(s, (char **)&s, 10);
      }
    } else {
      end = start;
    }

    /* Ensure start <= end */
    if (start > end) { unsigned long tmp = start; start = end; end = tmp; }

    /* Add range to array */
    for (unsigned long u = start; u <= end; u++) {
      if (count >= cap) {
        cap *= 2;
        *uids = (unsigned long *)realloc(*uids, cap * sizeof(unsigned long));
      }
      (*uids)[count++] = u;
    }

    if (*s == ',') s++;
  }
  return count;
}
