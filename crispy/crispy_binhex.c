/* crispy_binhex.c — BinHex 4.0 decode
 * Part of crispy: standalone, no Eudora dependency.
 *
 * BinHex 4.0 format:
 *   (This file must be converted with BinHex 4.0)
 *   :<encoded data using 6-to-8 bit table>:
 *
 * Structure after decoding:
 *   1 byte  filename length
 *   N bytes filename
 *   1 byte  version (0)
 *   4 bytes type
 *   4 bytes creator
 *   2 bytes flags
 *   4 bytes data fork length
 *   4 bytes resource fork length
 *   2 bytes header CRC
 *   N bytes data fork
 *   2 bytes data CRC
 *   N bytes resource fork
 *   2 bytes resource CRC
 *
 * We extract the data fork only (resource fork is Mac-specific).
 */

#include "crispy_binhex.h"
#include <stdlib.h>
#include <string.h>

/* BinHex 6-to-8 bit decode table */
static const signed char bh_decode[256] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,-1,-1,-1,
  12,13,14,15,16,17,18,-1,19,20,-1,-1,-1,-1,-1,-1,
  21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,-1,
  36,37,38,39,40,41,42,-1,43,44,45,46,-1,-1,-1,-1,
  47,48,49,50,51,52,53,-1,54,55,56,57,58,-1,59,60,
  61,62,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

bool crispy_binhex_detect(const char *data, long len) {
  if (!data || len < 45) return false;
  return (memmem(data, len, "(This file must be converted with BinHex", 40) != NULL);
}

/* Decode BinHex 6-bit encoding to raw bytes, handling RLE */
static char *bh_raw_decode(const char *encoded, long encLen, long *rawLen) {
  /* First pass: 6-to-8 decode */
  long maxRaw = (encLen * 3) / 4 + 4;
  unsigned char *raw = (unsigned char *)malloc(maxRaw);
  if (!raw) return NULL;
  long ri = 0;

  unsigned int accum = 0;
  int bits = 0;

  for (long i = 0; i < encLen; i++) {
    unsigned char c = (unsigned char)encoded[i];
    if (c == '\r' || c == '\n' || c == ' ' || c == '\t') continue;
    if (c == ':') break; /* end marker */

    signed char val = bh_decode[c];
    if (val < 0) continue; /* skip invalid */

    accum = (accum << 6) | (unsigned int)val;
    bits += 6;
    while (bits >= 8) {
      bits -= 8;
      if (ri < maxRaw) raw[ri++] = (unsigned char)((accum >> bits) & 0xFF);
    }
  }

  /* Second pass: RLE decode (0x90 = repeat marker) */
  long maxExp = ri * 4 + 16;
  unsigned char *expanded = (unsigned char *)malloc(maxExp);
  if (!expanded) { free(raw); return NULL; }
  long ei = 0;

  for (long i = 0; i < ri; i++) {
    if (raw[i] == 0x90) {
      if (i + 1 < ri && raw[i + 1] == 0x00) {
        /* Literal 0x90 */
        if (ei < maxExp) expanded[ei++] = 0x90;
        i++;
      } else if (i + 1 < ri) {
        /* Repeat previous byte raw[i+1]-1 more times */
        unsigned char count = raw[i + 1];
        unsigned char prev = (ei > 0) ? expanded[ei - 1] : 0;
        /* Grow if needed */
        if (ei + count > maxExp) {
          maxExp = ei + count + 1024;
          unsigned char *tmp = (unsigned char *)realloc(expanded, maxExp);
          if (!tmp) { free(raw); free(expanded); return NULL; }
          expanded = tmp;
        }
        for (int j = 1; j < count; j++)
          expanded[ei++] = prev;
        i++;
      }
    } else {
      if (ei >= maxExp) {
        maxExp *= 2;
        unsigned char *tmp = (unsigned char *)realloc(expanded, maxExp);
        if (!tmp) { free(raw); free(expanded); return NULL; }
        expanded = tmp;
      }
      expanded[ei++] = raw[i];
    }
  }

  free(raw);
  if (rawLen) *rawLen = ei;
  return (char *)expanded;
}

char *crispy_binhex_decode(const char *data, long len, long *outLen,
                           char **outFilename) {
  if (!data || len < 50) return NULL;
  if (outLen) *outLen = 0;
  if (outFilename) *outFilename = NULL;

  /* Find the colon that starts encoded data */
  const char *start = NULL;
  const char *p = data;
  const char *end = data + len;

  /* Skip header line */
  while (p < end) {
    if (*p == ':') { start = p + 1; break; }
    p++;
  }
  if (!start) return NULL;

  /* Find ending colon */
  long encLen = 0;
  const char *q = start;
  while (q < end) {
    if (*q == ':') { encLen = (long)(q - start); break; }
    q++;
  }
  if (encLen == 0) return NULL;

  /* Decode */
  long rawLen = 0;
  char *raw = bh_raw_decode(start, encLen, &rawLen);
  if (!raw || rawLen < 22) { free(raw); return NULL; }

  /* Parse header */
  unsigned char nameLen = (unsigned char)raw[0];
  if (nameLen == 0 || nameLen > 63 || 1 + nameLen + 20 > rawLen) {
    free(raw);
    return NULL;
  }

  if (outFilename) {
    *outFilename = (char *)malloc(nameLen + 1);
    memcpy(*outFilename, raw + 1, nameLen);
    (*outFilename)[nameLen] = '\0';
  }

  long hdrOff = 1 + nameLen + 1; /* name + version byte */
  /* type(4) + creator(4) + flags(2) = 10 bytes */
  hdrOff += 10;

  /* Data fork length (4 bytes big-endian) */
  unsigned long dfLen = ((unsigned char)raw[hdrOff] << 24) |
                        ((unsigned char)raw[hdrOff+1] << 16) |
                        ((unsigned char)raw[hdrOff+2] << 8) |
                        ((unsigned char)raw[hdrOff+3]);
  hdrOff += 4;

  /* Resource fork length (skip) */
  hdrOff += 4;

  /* Header CRC (skip) */
  hdrOff += 2;

  /* Data fork starts here */
  if (hdrOff + (long)dfLen > rawLen) {
    /* Truncated — return what we have */
    dfLen = rawLen - hdrOff;
    if ((long)dfLen <= 0) { free(raw); return NULL; }
  }

  char *out = (char *)malloc(dfLen);
  if (!out) { free(raw); return NULL; }
  memcpy(out, raw + hdrOff, dfLen);

  free(raw);
  if (outLen) *outLen = (long)dfLen;
  return out;
}

/* ================================================================
 * BinHex 4.0 Encode
 * ================================================================ */

/* BinHex 6-to-8 encode table (same as original Eudora) */
static const unsigned char bh_encode[64] = {
  0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x30, 0x31, 0x32,
  0x33, 0x34, 0x35, 0x36, 0x38, 0x39, 0x40, 0x41,
  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
  0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x50, 0x51, 0x52,
  0x53, 0x54, 0x55, 0x56, 0x58, 0x59, 0x5a, 0x5b,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x68,
  0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x70, 0x71, 0x72
};

/* CRC-16 for BinHex */
static unsigned short bh_crc(unsigned short crc, unsigned char c) {
  int i;
  unsigned long temp = (unsigned long)crc;
  for (i = 0; i < 8; i++) {
    temp <<= 1;
    if (c & 0x80) temp |= 1;
    c <<= 1;
    if (temp & 0x10000) temp ^= 0x10211;
  }
  return (unsigned short)(temp & 0xFFFF);
}

/* Encoder state */
typedef struct {
  char *out;
  long len;
  long cap;
  int state86;     /* 6-bit accumulator state (0-2) */
  int savedBits;
  int lineLen;
  unsigned short crc;
} BHEnc;

static void bh_grow(BHEnc *e, long need) {
  if (e->len + need > e->cap) {
    e->cap = (e->cap + need) * 2;
    e->out = (char *)realloc(e->out, e->cap);
  }
}

static void bh_emit_char(BHEnc *e, unsigned char encoded) {
  bh_grow(e, 3);
  e->out[e->len++] = (char)encoded;
  e->lineLen++;
  if (e->lineLen >= 64) {
    e->out[e->len++] = '\r';
    e->out[e->len++] = '\n';
    e->lineLen = 0;
  }
}

/* Encode one 8-bit byte to 6-bit BinHex output */
static void bh_encode_byte(BHEnc *e, unsigned char c) {
  switch (e->state86) {
    case 0:
      bh_emit_char(e, bh_encode[c >> 2]);
      e->savedBits = (c & 0x03) << 4;
      e->state86 = 1;
      break;
    case 1:
      bh_emit_char(e, bh_encode[e->savedBits | (c >> 4)]);
      e->savedBits = (c & 0x0F) << 2;
      e->state86 = 2;
      break;
    case 2:
      bh_emit_char(e, bh_encode[e->savedBits | (c >> 6)]);
      bh_emit_char(e, bh_encode[c & 0x3F]);
      e->state86 = 0;
      break;
  }
}

/* Encode byte with RLE and CRC */
static void bh_code(BHEnc *e, unsigned char c) {
  bh_encode_byte(e, c);
  if (c == 0x90) bh_encode_byte(e, 0x00); /* escape literal 0x90 */
  e->crc = bh_crc(e->crc, c);
}

static void bh_code_short(BHEnc *e, unsigned short v) {
  bh_code(e, (unsigned char)(v >> 8));
  bh_code(e, (unsigned char)(v & 0xFF));
}

static void bh_code_long(BHEnc *e, unsigned long v) {
  bh_code(e, (unsigned char)((v >> 24) & 0xFF));
  bh_code(e, (unsigned char)((v >> 16) & 0xFF));
  bh_code(e, (unsigned char)((v >> 8) & 0xFF));
  bh_code(e, (unsigned char)(v & 0xFF));
}

char *crispy_binhex_encode(const char *filename, const char *data,
                           long dataLen, long *outLen) {
  if (!filename || !data || dataLen < 0) return NULL;

  BHEnc e;
  memset(&e, 0, sizeof(e));
  e.cap = dataLen * 2 + 1024;
  e.out = (char *)malloc(e.cap);
  if (!e.out) return NULL;

  /* Header line */
  const char *hdr = "(This file must be converted with BinHex 4.0)\r\n:";
  long hdrLen = (long)strlen(hdr);
  memcpy(e.out, hdr, hdrLen);
  e.len = hdrLen;
  e.lineLen = 1; /* after the colon */

  /* File info header */
  int nameLen = (int)strlen(filename);
  if (nameLen > 63) nameLen = 63;

  /* Filename (length-prefixed) */
  bh_code(&e, (unsigned char)nameLen);
  for (int i = 0; i < nameLen; i++)
    bh_code(&e, (unsigned char)filename[i]);
  bh_code(&e, 0); /* version */

  /* Type and creator — generic for portable */
  bh_code_long(&e, 0x3F3F3F3F); /* '????' type */
  bh_code_long(&e, 0x3F3F3F3F); /* '????' creator */
  bh_code_short(&e, 0);          /* flags */

  /* Fork lengths */
  bh_code_long(&e, (unsigned long)dataLen); /* data fork */
  bh_code_long(&e, 0);                      /* resource fork (none) */

  /* Header CRC */
  {
    unsigned short headerCrc = e.crc;
    e.crc = bh_crc(e.crc, 0);
    e.crc = bh_crc(e.crc, 0);
    headerCrc = e.crc;
    bh_code_short(&e, headerCrc);
    e.crc = 0;
  }

  /* Data fork */
  for (long i = 0; i < dataLen; i++)
    bh_code(&e, (unsigned char)data[i]);

  /* Data fork CRC */
  {
    e.crc = bh_crc(e.crc, 0);
    e.crc = bh_crc(e.crc, 0);
    unsigned short dataCrc = e.crc;
    bh_code_short(&e, dataCrc);
    e.crc = 0;
  }

  /* Empty resource fork CRC */
  {
    e.crc = bh_crc(e.crc, 0);
    e.crc = bh_crc(e.crc, 0);
    unsigned short resCrc = e.crc;
    bh_code_short(&e, resCrc);
  }

  /* Flush remaining bits */
  if (e.state86) bh_encode_byte(&e, 0);

  /* End marker */
  bh_grow(&e, 4);
  e.out[e.len++] = ':';
  e.out[e.len++] = '\r';
  e.out[e.len++] = '\n';
  e.out[e.len] = '\0';

  if (outLen) *outLen = e.len;
  return e.out;
}
