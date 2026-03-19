/* crispy_encode.c — RFC 2047, charset conversion, QP encode/decode
 * Part of crispy: standalone, no external dependencies.
 */

#include "crispy_encode.h"
#include "crispy_smtp.h" /* for crispy_base64_encode/decode */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ================================================================
 * Charset conversion tables
 * ================================================================ */

/* Windows-1252 — superset of ISO-8859-1, differs in 0x80–0x9F range.
 * Maps each byte 0x80–0x9F to its Unicode codepoint. */
static const unsigned int cp1252_map[32] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, /* 80-87 */
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F, /* 88-8F */
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, /* 90-97 */
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, /* 98-9F */
};

/* ISO-8859-15 — differs from 8859-1 at these positions */
static const struct { unsigned char byte; unsigned int cp; } iso15_diff[] = {
  { 0xA4, 0x20AC }, /* Euro sign */
  { 0xA6, 0x0160 }, { 0xA8, 0x0161 },
  { 0xB4, 0x017D }, { 0xB8, 0x017E },
  { 0xBC, 0x0152 }, { 0xBD, 0x0153 }, { 0xBE, 0x0178 },
  { 0, 0 }
};

/* Encode a single Unicode codepoint as UTF-8, return bytes written (1-4) */
static int utf8_encode_cp(unsigned int cp, char *out) {
  if (cp < 0x80) {
    out[0] = (char)cp;
    return 1;
  } else if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  } else if (cp < 0x10000) {
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  } else {
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
  }
}

/* ================================================================
 * Charset conversion
 * ================================================================ */

/* ISO-8859-1 to UTF-8 (identity for 0x00–0x7F, 2-byte for 0x80–0xFF) */
static char *latin1_to_utf8(const char *in, long inLen) {
  char *out = (char *)malloc(inLen * 2 + 1);
  if (!out) return NULL;
  long o = 0;
  for (long i = 0; i < inLen; i++) {
    unsigned char c = (unsigned char)in[i];
    o += utf8_encode_cp(c, out + o);
  }
  out[o] = '\0';
  return out;
}

/* Windows-1252 to UTF-8 */
static char *cp1252_to_utf8(const char *in, long inLen) {
  char *out = (char *)malloc(inLen * 3 + 1);
  if (!out) return NULL;
  long o = 0;
  for (long i = 0; i < inLen; i++) {
    unsigned char c = (unsigned char)in[i];
    unsigned int cp;
    if (c >= 0x80 && c <= 0x9F)
      cp = cp1252_map[c - 0x80];
    else
      cp = c;
    o += utf8_encode_cp(cp, out + o);
  }
  out[o] = '\0';
  return out;
}

/* ISO-8859-15 to UTF-8 */
static char *iso15_to_utf8(const char *in, long inLen) {
  char *out = (char *)malloc(inLen * 3 + 1);
  if (!out) return NULL;
  long o = 0;
  for (long i = 0; i < inLen; i++) {
    unsigned char c = (unsigned char)in[i];
    unsigned int cp = c; /* default: same as Latin-1 */
    for (int j = 0; iso15_diff[j].byte; j++) {
      if (c == iso15_diff[j].byte) { cp = iso15_diff[j].cp; break; }
    }
    o += utf8_encode_cp(cp, out + o);
  }
  out[o] = '\0';
  return out;
}

/* ISO-8859-2 through 8859-9 — all are single-byte with 0x80-0xFF mapped
 * to specific Unicode ranges. For simplicity, treat them like Latin-1
 * (correct for most common characters, close enough for display).
 * Full tables would be 128 entries each — add as needed. */

bool crispy_charset_supported(const char *charset) {
  if (!charset) return false;
  return (strcasecmp(charset, "UTF-8") == 0 ||
          strcasecmp(charset, "US-ASCII") == 0 ||
          strcasecmp(charset, "ASCII") == 0 ||
          strcasecmp(charset, "ISO-8859-1") == 0 ||
          strcasecmp(charset, "latin1") == 0 ||
          strcasecmp(charset, "ISO-8859-15") == 0 ||
          strcasecmp(charset, "latin9") == 0 ||
          strcasecmp(charset, "Windows-1252") == 0 ||
          strcasecmp(charset, "CP1252") == 0);
}

char *crispy_charset_to_utf8(const char *in, long inLen,
                             const char *charset) {
  if (!in || !charset) return NULL;

  /* UTF-8 and ASCII: passthrough */
  if (strcasecmp(charset, "UTF-8") == 0 ||
      strcasecmp(charset, "US-ASCII") == 0 ||
      strcasecmp(charset, "ASCII") == 0) {
    char *out = (char *)malloc(inLen + 1);
    if (out) { memcpy(out, in, inLen); out[inLen] = '\0'; }
    return out;
  }

  /* Windows-1252 / CP1252 */
  if (strcasecmp(charset, "Windows-1252") == 0 ||
      strcasecmp(charset, "CP1252") == 0)
    return cp1252_to_utf8(in, inLen);

  /* ISO-8859-15 / Latin-9 */
  if (strcasecmp(charset, "ISO-8859-15") == 0 ||
      strcasecmp(charset, "latin9") == 0)
    return iso15_to_utf8(in, inLen);

  /* ISO-8859-1 / Latin-1 (and fallback for other 8859 variants) */
  if (strcasecmp(charset, "ISO-8859-1") == 0 ||
      strcasecmp(charset, "latin1") == 0 ||
      strncasecmp(charset, "ISO-8859-", 9) == 0)
    return latin1_to_utf8(in, inLen);

  return NULL; /* unsupported */
}

/* ================================================================
 * Quoted-Printable encode/decode
 * ================================================================ */

static const char hex_chars[] = "0123456789ABCDEF";

char *crispy_qp_encode(const char *in, long inLen, long *outLen) {
  /* Worst case: every byte becomes =XX (3x), plus soft breaks */
  char *out = (char *)malloc(inLen * 3 + (inLen / 25) * 3 + 16);
  if (!out) return NULL;
  long o = 0;
  int lineLen = 0;

  for (long i = 0; i < inLen; i++) {
    unsigned char c = (unsigned char)in[i];

    /* Pass through printable ASCII (33-126) except = */
    if ((c >= 33 && c <= 126 && c != '=') ||
        (c == '\t') ||
        (c == ' ' && i + 1 < inLen && in[i+1] != '\r' && in[i+1] != '\n')) {
      /* Soft line break if line would exceed 76 */
      if (lineLen >= 75) {
        out[o++] = '='; out[o++] = '\r'; out[o++] = '\n';
        lineLen = 0;
      }
      out[o++] = (char)c;
      lineLen++;
    }
    /* Preserve CRLF as literal */
    else if (c == '\r' && i + 1 < inLen && in[i+1] == '\n') {
      out[o++] = '\r'; out[o++] = '\n';
      i++; /* skip \n */
      lineLen = 0;
    }
    else if (c == '\n') {
      out[o++] = '\r'; out[o++] = '\n';
      lineLen = 0;
    }
    /* Encode everything else as =XX */
    else {
      if (lineLen >= 73) { /* need 3 chars + possible soft break */
        out[o++] = '='; out[o++] = '\r'; out[o++] = '\n';
        lineLen = 0;
      }
      out[o++] = '=';
      out[o++] = hex_chars[c >> 4];
      out[o++] = hex_chars[c & 0x0F];
      lineLen += 3;
    }
  }

  out[o] = '\0';
  if (outLen) *outLen = o;
  return out;
}

char *crispy_qp_decode(const char *in, long inLen, long *outLen) {
  char *out = (char *)malloc(inLen + 1);
  if (!out) return NULL;
  long o = 0;

  for (long i = 0; i < inLen; i++) {
    if (in[i] == '=' && i + 2 < inLen) {
      if (in[i+1] == '\r' || in[i+1] == '\n') {
        /* Soft line break */
        i++;
        if (i < inLen && in[i] == '\r' && i + 1 < inLen && in[i+1] == '\n')
          i++;
      } else if (isxdigit((unsigned char)in[i+1]) &&
                 isxdigit((unsigned char)in[i+2])) {
        char hex[3] = { in[i+1], in[i+2], 0 };
        out[o++] = (char)strtol(hex, NULL, 16);
        i += 2;
      } else {
        out[o++] = in[i];
      }
    } else {
      out[o++] = in[i];
    }
  }
  out[o] = '\0';
  if (outLen) *outLen = o;
  return out;
}

/* ================================================================
 * RFC 2047 encoded-word decode
 * ================================================================ */

/* Decode a single =?charset?encoding?text?= token.
 * Returns malloc'd UTF-8 string. */
static char *decode_one_word(const char *start, const char *end) {
  /* Format: =?charset?E?encoded_text?= where E is B or Q */
  const char *p = start + 2; /* skip =? */
  const char *charset_end = strchr(p, '?');
  if (!charset_end || charset_end >= end) return NULL;

  char charset[64];
  long csLen = (long)(charset_end - p);
  if (csLen >= (long)sizeof(charset)) csLen = sizeof(charset) - 1;
  memcpy(charset, p, csLen);
  charset[csLen] = '\0';

  p = charset_end + 1;
  if (p >= end) return NULL;
  char encoding = (char)toupper((unsigned char)*p);
  p++;
  if (*p != '?') return NULL;
  p++;

  /* Find closing ?= */
  const char *text_end = end - 2; /* before ?= */
  long textLen = (long)(text_end - p);
  if (textLen < 0) return NULL;

  /* Decode */
  char *decoded = NULL;
  long decodedLen = 0;

  if (encoding == 'B') {
    decoded = crispy_base64_decode(p, textLen, &decodedLen);
  } else if (encoding == 'Q') {
    /* Q encoding is like QP but _ means space */
    char *tmp = (char *)malloc(textLen + 1);
    if (!tmp) return NULL;
    memcpy(tmp, p, textLen);
    for (long i = 0; i < textLen; i++)
      if (tmp[i] == '_') tmp[i] = ' ';
    tmp[textLen] = '\0';
    decoded = crispy_qp_decode(tmp, textLen, &decodedLen);
    free(tmp);
  } else {
    return NULL;
  }

  if (!decoded) return NULL;

  /* Convert charset to UTF-8 */
  if (strcasecmp(charset, "UTF-8") == 0 ||
      strcasecmp(charset, "US-ASCII") == 0) {
    return decoded; /* already UTF-8 */
  }

  char *utf8 = crispy_charset_to_utf8(decoded, decodedLen, charset);
  free(decoded);
  if (!utf8) {
    /* Unsupported charset — return raw decoded as best effort */
    utf8 = (char *)malloc(decodedLen + 1);
    if (utf8) { memcpy(utf8, decoded, decodedLen); utf8[decodedLen] = '\0'; }
  }
  return utf8;
}

char *crispy_decode_header(const char *in) {
  if (!in) return NULL;

  long inLen = (long)strlen(in);
  /* Quick check: no encoded words? */
  if (!strstr(in, "=?")) return strdup(in);

  /* Build output */
  char *out = (char *)malloc(inLen * 4 + 1); /* generous */
  if (!out) return NULL;
  long o = 0;
  const char *p = in;
  const char *end = in + inLen;
  bool prevWasEncoded = false;

  while (p < end) {
    /* Look for =? */
    const char *ewStart = strstr(p, "=?");
    if (!ewStart) {
      /* Copy rest */
      long rest = (long)(end - p);
      memcpy(out + o, p, rest);
      o += rest;
      break;
    }

    /* Copy text before encoded word (unless it's just whitespace between
     * two encoded words — RFC 2047 says to skip that) */
    if (ewStart > p) {
      if (prevWasEncoded) {
        /* Check if text between is only whitespace */
        bool onlyWS = true;
        for (const char *c = p; c < ewStart; c++)
          if (*c != ' ' && *c != '\t' && *c != '\r' && *c != '\n')
            { onlyWS = false; break; }
        if (!onlyWS) {
          memcpy(out + o, p, ewStart - p);
          o += ewStart - p;
        }
      } else {
        memcpy(out + o, p, ewStart - p);
        o += ewStart - p;
      }
    }

    /* Find closing ?= */
    const char *ewEnd = NULL;
    const char *q = ewStart + 2;
    int qmarks = 0;
    while (q < end) {
      if (*q == '?') {
        qmarks++;
        if (qmarks >= 3 && q + 1 < end && q[1] == '=') {
          ewEnd = q + 2;
          break;
        }
      }
      q++;
    }

    if (!ewEnd) {
      /* Malformed — copy as-is */
      memcpy(out + o, ewStart, end - ewStart);
      o += end - ewStart;
      break;
    }

    char *decoded = decode_one_word(ewStart, ewEnd);
    if (decoded) {
      long dlen = (long)strlen(decoded);
      /* Grow output if needed */
      if (o + dlen >= inLen * 4) {
        char *newOut = (char *)realloc(out, (o + dlen) * 2 + 1);
        if (newOut) out = newOut;
      }
      memcpy(out + o, decoded, dlen);
      o += dlen;
      free(decoded);
      prevWasEncoded = true;
    } else {
      /* Decode failed — copy raw */
      long ewLen = (long)(ewEnd - ewStart);
      memcpy(out + o, ewStart, ewLen);
      o += ewLen;
      prevWasEncoded = false;
    }

    p = ewEnd;
  }

  out[o] = '\0';
  return out;
}

/* ================================================================
 * RFC 2047 encoded-word encode
 * ================================================================ */

char *crispy_encode_header(const char *utf8) {
  if (!utf8) return NULL;

  /* Check if encoding is needed */
  bool needsEncoding = false;
  for (const char *p = utf8; *p; p++) {
    if ((unsigned char)*p > 126 || (unsigned char)*p < 32) {
      needsEncoding = true;
      break;
    }
  }

  if (!needsEncoding) return strdup(utf8);

  /* Encode as =?UTF-8?B?...?= in chunks of ~45 bytes (fits in 76-char line) */
  long inLen = (long)strlen(utf8);
  /* Worst case: each 45-byte chunk becomes ~76 chars + overhead */
  long maxOut = (inLen / 45 + 1) * 80 + 64;
  char *out = (char *)malloc(maxOut);
  if (!out) return NULL;
  long o = 0;

  const char *p = utf8;
  long remaining = inLen;
  bool first = true;

  while (remaining > 0) {
    /* Find a chunk boundary that doesn't split a UTF-8 sequence */
    long chunk = remaining > 45 ? 45 : remaining;
    while (chunk < remaining && chunk > 0 &&
           ((unsigned char)p[chunk] & 0xC0) == 0x80)
      chunk--; /* back up to start of UTF-8 char */
    if (chunk == 0) chunk = remaining; /* safety */

    long b64Len;
    char *b64 = crispy_base64_encode(p, chunk, &b64Len);
    if (!b64) { free(out); return NULL; }

    if (!first) {
      out[o++] = '\r'; out[o++] = '\n'; out[o++] = ' ';
    }
    o += snprintf(out + o, maxOut - o, "=?UTF-8?B?%s?=", b64);
    free(b64);

    p += chunk;
    remaining -= chunk;
    first = false;
  }

  out[o] = '\0';
  return out;
}
