/* crispy_uudecode.c — UUencode decode
 * Part of crispy: standalone, no Eudora dependency.
 *
 * Decodes standard UUencoded data:
 *   begin 644 filename
 *   M<encoded lines>
 *   `
 *   end
 */

#include "crispy_uudecode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEC(c) (((c) - ' ') & 077)

bool crispy_uudecode_detect(const char *data, long len) {
  if (!data || len < 10) return false;
  const char *p = data;
  const char *end = data + len;

  while (p < end - 10) {
    if (strncmp(p, "begin ", 6) == 0 && isdigit((unsigned char)p[6]))
      return true;
    /* Skip to next line */
    while (p < end && *p != '\n') p++;
    if (p < end) p++;
  }
  return false;
}

char *crispy_uudecode(const char *data, long len, long *outLen,
                      char **outFilename) {
  if (!data || len < 10) return NULL;
  if (outLen) *outLen = 0;
  if (outFilename) *outFilename = NULL;

  const char *p = data;
  const char *end = data + len;

  /* Find "begin NNN filename" line */
  while (p < end - 10) {
    if (strncmp(p, "begin ", 6) == 0 && isdigit((unsigned char)p[6])) {
      /* Skip "begin NNN " */
      const char *nameStart = p + 6;
      while (nameStart < end && isdigit((unsigned char)*nameStart)) nameStart++;
      while (nameStart < end && *nameStart == ' ') nameStart++;

      /* Extract filename */
      const char *nameEnd = nameStart;
      while (nameEnd < end && *nameEnd != '\n' && *nameEnd != '\r') nameEnd++;
      if (outFilename && nameEnd > nameStart) {
        long nlen = (long)(nameEnd - nameStart);
        *outFilename = (char *)malloc(nlen + 1);
        memcpy(*outFilename, nameStart, nlen);
        (*outFilename)[nlen] = '\0';
      }

      /* Skip to next line */
      p = nameEnd;
      while (p < end && (*p == '\r' || *p == '\n')) p++;
      break;
    }
    while (p < end && *p != '\n') p++;
    if (p < end) p++;
  }

  /* Decode lines */
  long capacity = len; /* generous estimate */
  char *out = (char *)malloc(capacity);
  if (!out) return NULL;
  long o = 0;

  while (p < end) {
    /* Check for "end" line */
    if (strncmp(p, "end", 3) == 0 &&
        (p[3] == '\n' || p[3] == '\r' || p[3] == '\0'))
      break;

    /* Empty or backtick line = end of data */
    if (*p == '`' || *p == '\n' || *p == '\r') {
      while (p < end && *p != '\n') p++;
      if (p < end) p++;
      continue;
    }

    /* First char = count of decoded bytes on this line */
    int n = DEC(*p);
    p++;

    while (n > 0 && p + 3 < end) {
      /* Grow buffer if needed */
      if (o + 4 > capacity) {
        capacity *= 2;
        char *tmp = (char *)realloc(out, capacity);
        if (!tmp) { free(out); return NULL; }
        out = tmp;
      }

      unsigned char c0 = DEC(p[0]);
      unsigned char c1 = DEC(p[1]);
      unsigned char c2 = DEC(p[2]);
      unsigned char c3 = DEC(p[3]);

      if (n >= 1) out[o++] = (char)((c0 << 2) | (c1 >> 4));
      if (n >= 2) out[o++] = (char)((c1 << 4) | (c2 >> 2));
      if (n >= 3) out[o++] = (char)((c2 << 6) | c3);

      p += 4;
      n -= 3;
    }

    /* Skip to next line */
    while (p < end && *p != '\n') p++;
    if (p < end) p++;
  }

  if (outLen) *outLen = o;
  return out;
}

/* ================================================================
 * UUencode
 * ================================================================ */

#define ENC(c) ((char)(((c) & 077) + ' '))

char *crispy_uuencode(const char *filename, const char *data,
                      long dataLen, long *outLen) {
  if (!filename || !data || dataLen < 0) return NULL;

  /* Output is ~37% larger than input + header/footer */
  long capacity = (dataLen * 4 / 3) + (dataLen / 45) * 2 + 256;
  char *out = (char *)malloc(capacity);
  if (!out) return NULL;
  long o = 0;

  /* "begin 644 filename\n" */
  o += snprintf(out + o, capacity - o, "begin 644 %s\n", filename);

  /* Encode 45 bytes per line */
  const unsigned char *src = (const unsigned char *)data;
  long remaining = dataLen;

  while (remaining > 0) {
    int n = (remaining > 45) ? 45 : (int)remaining;

    /* Grow if needed */
    if (o + n * 2 + 10 > capacity) {
      capacity *= 2;
      char *tmp = (char *)realloc(out, capacity);
      if (!tmp) { free(out); return NULL; }
      out = tmp;
    }

    /* Length character */
    out[o++] = ENC(n);

    /* Encode 3 bytes at a time into 4 characters */
    const unsigned char *p = src;
    int bytes_left = n;
    while (bytes_left > 0) {
      unsigned char c0 = p[0];
      unsigned char c1 = (bytes_left > 1) ? p[1] : 0;
      unsigned char c2 = (bytes_left > 2) ? p[2] : 0;

      out[o++] = ENC(c0 >> 2);
      out[o++] = ENC(((c0 << 4) & 060) | ((c1 >> 4) & 017));
      out[o++] = ENC(((c1 << 2) & 074) | ((c2 >> 6) & 03));
      out[o++] = ENC(c2 & 077);

      p += 3;
      bytes_left -= 3;
    }

    out[o++] = '\n';
    src += n;
    remaining -= n;
  }

  /* End: backtick (zero-length line) + "end" */
  o += snprintf(out + o, capacity - o, "`\nend\n");

  out[o] = '\0';
  if (outLen) *outLen = o;
  return out;
}
