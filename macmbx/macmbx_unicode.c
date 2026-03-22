/* macmbx_unicode.c — Portable Unicode and charset conversion
 * Part of macmbx: standalone mail data management library.
 *
 * Replaces Apple TEC with built-in conversion tables.
 * Standalone: no GLib, no Apple frameworks, no GTK.
 */

#include "macmbx_unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ================================================================
 * UTF-8 codec
 * ================================================================ */

int macmbx_utf8_encode(uint32_t cp, char *out, int maxLen) {
  if (cp < 0x80) {
    if (maxLen < 1) return 0;
    out[0] = (char)cp;
    return 1;
  } else if (cp < 0x800) {
    if (maxLen < 2) return 0;
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  } else if (cp < 0x10000) {
    if (maxLen < 3) return 0;
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  } else if (cp <= 0x10FFFF) {
    if (maxLen < 4) return 0;
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
  }
  return 0;
}

uint32_t macmbx_utf8_decode(const char *utf8, const char **next) {
  const unsigned char *p = (const unsigned char *)utf8;
  uint32_t cp;
  int trail;

  if (p[0] < 0x80) { cp = p[0]; trail = 0; }
  else if ((p[0] & 0xE0) == 0xC0) { cp = p[0] & 0x1F; trail = 1; }
  else if ((p[0] & 0xF0) == 0xE0) { cp = p[0] & 0x0F; trail = 2; }
  else if ((p[0] & 0xF8) == 0xF0) { cp = p[0] & 0x07; trail = 3; }
  else { if (next) *next = (const char *)p + 1; return 0xFFFD; }

  for (int i = 0; i < trail; i++) {
    if ((p[1 + i] & 0xC0) != 0x80) {
      if (next) *next = (const char *)p + 1;
      return 0xFFFD;
    }
    cp = (cp << 6) | (p[1 + i] & 0x3F);
  }

  /* Overlong and surrogate checks */
  if ((trail == 1 && cp < 0x80) || (trail == 2 && cp < 0x800) ||
      (trail == 3 && cp < 0x10000) || (cp >= 0xD800 && cp <= 0xDFFF) ||
      cp > 0x10FFFF) {
    if (next) *next = (const char *)p + 1;
    return 0xFFFD;
  }

  if (next) *next = (const char *)p + 1 + trail;
  return cp;
}

int macmbx_utf8_len(const char *utf8, int byteLen) {
  if (!utf8) return 0;
  int count = 0;
  const char *end = utf8 + byteLen;
  while (utf8 < end) {
    macmbx_utf8_decode(utf8, &utf8);
    count++;
  }
  return count;
}

int macmbx_utf8_valid_len(const char *utf8, int byteLen) {
  if (!utf8 || byteLen <= 0) return 0;
  int last_good = 0, pos = 0;
  const unsigned char *p = (const unsigned char *)utf8;
  while (pos < byteLen) {
    int seq_len;
    if (p[pos] < 0x80) seq_len = 1;
    else if ((p[pos] & 0xE0) == 0xC0) seq_len = 2;
    else if ((p[pos] & 0xF0) == 0xE0) seq_len = 3;
    else if ((p[pos] & 0xF8) == 0xF0) seq_len = 4;
    else break; /* invalid lead byte */
    if (pos + seq_len > byteLen) break; /* incomplete at end */
    bool valid = true;
    for (int i = 1; i < seq_len; i++)
      if ((p[pos + i] & 0xC0) != 0x80) { valid = false; break; }
    if (!valid) break;
    pos += seq_len;
    last_good = pos;
  }
  return last_good;
}

bool macmbx_utf8_validate(const char *utf8, int byteLen) {
  return macmbx_utf8_valid_len(utf8, byteLen) == byteLen;
}

/* ================================================================
 * Charset conversion tables
 * ================================================================ */

/* Windows-1252: 0x80-0x9F special mappings (the rest = identity to Unicode) */
static const uint16_t win1252_80_9f[32] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,
};

/* Mac Roman: 0x80-0xFF → Unicode */
static const uint16_t mac_roman[128] = {
  0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
  0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
  0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
  0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
  0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
  0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
  0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
  0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
  0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
  0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
  0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
  0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
  0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
  0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
  0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
  0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7,
};

/* ISO-8859-2 (Latin-2 / Central European) 0xA0-0xFF */
static const uint16_t iso8859_2[96] = {
  0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7,
  0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B,
  0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7,
  0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C,
  0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
  0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
  0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
  0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
  0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
  0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
};

/* ISO-8859-15 (Latin-9, replaces 8 chars from Latin-1) 0xA0-0xFF */
static const uint16_t iso8859_15[96] = {
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7,
  0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7,
  0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,
};

/* Windows-1251 (Cyrillic) 0x80-0xFF */
static const uint16_t win1251[128] = {
  0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
  0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
  0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
  0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
  0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
  0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
  0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
  0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
  0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
  0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
  0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

/* ISO-8859-3 (Latin-3 / South European) 0xA0-0xFF */
static const uint16_t iso8859_3[96] = {
  0x00A0, 0x0126, 0x02D8, 0x00A3, 0x00A4, 0x0000, 0x0124, 0x00A7,
  0x00A8, 0x0130, 0x015E, 0x011E, 0x0134, 0x00AD, 0x0000, 0x017B,
  0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x0125, 0x00B7,
  0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, 0x0000, 0x017C,
  0x00C0, 0x00C1, 0x00C2, 0x0000, 0x00C4, 0x010A, 0x0108, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x0000, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7,
  0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x016C, 0x015C, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x0000, 0x00E4, 0x010B, 0x0109, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x0000, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7,
  0x011D, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9,
};

/* ISO-8859-4 (Latin-4 / Baltic) 0xA0-0xFF */
static const uint16_t iso8859_4[96] = {
  0x00A0, 0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7,
  0x00A8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF,
  0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4, 0x0129, 0x013C, 0x02C7,
  0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E, 0x014B,
  0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A,
  0x0110, 0x0145, 0x014C, 0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x0168, 0x016A, 0x00DF,
  0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B,
  0x0111, 0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9,
};

/* ISO-8859-5 (Cyrillic) 0xA0-0xFF */
static const uint16_t iso8859_5[96] = {
  0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
  0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F,
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
  0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
  0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
  0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
  0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
  0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457,
  0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F,
};

/* ISO-8859-6 (Arabic) 0xA0-0xFF */
static const uint16_t iso8859_6[96] = {
  0x00A0, 0x0000, 0x0000, 0x0000, 0x00A4, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x060C, 0x00AD, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x061B, 0x0000, 0x0000, 0x0000, 0x061F,
  0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
  0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
  0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637,
  0x0638, 0x0639, 0x063A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647,
  0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F,
  0x0650, 0x0651, 0x0652, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* ISO-8859-7 (Greek) 0xA0-0xFF */
static const uint16_t iso8859_7[96] = {
  0x00A0, 0x2018, 0x2019, 0x00A3, 0x20AC, 0x20AF, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x037A, 0x00AB, 0x00AC, 0x00AD, 0x0000, 0x2015,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x0385, 0x0386, 0x00B7,
  0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E, 0x038F,
  0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
  0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x0000, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
  0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
  0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
  0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
  0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
  0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x0000,
};

/* ISO-8859-8 (Hebrew) 0xA0-0xFF */
static const uint16_t iso8859_8[96] = {
  0x00A0, 0x0000, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2017,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
  0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
  0x05E8, 0x05E9, 0x05EA, 0x0000, 0x0000, 0x200E, 0x200F, 0x0000,
};

/* ISO-8859-9 (Latin-5 / Turkish) 0xA0-0xFF
 * Same as Latin-1 except: D0→011E, DD→0130, DE→015E, F0→011F, FD→0131, FE→015F */
static const uint16_t iso8859_9[96] = {
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF,
};

/* ISO-8859-10 (Latin-6 / Nordic) 0xA0-0xFF */
static const uint16_t iso8859_10[96] = {
  0x00A0, 0x0104, 0x0112, 0x0122, 0x012A, 0x0128, 0x0136, 0x00A7,
  0x013B, 0x0110, 0x0160, 0x0166, 0x017D, 0x00AD, 0x016A, 0x014A,
  0x00B0, 0x0105, 0x0113, 0x0123, 0x012B, 0x0129, 0x0137, 0x00B7,
  0x013C, 0x0111, 0x0161, 0x0167, 0x017E, 0x2015, 0x016B, 0x014B,
  0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x00CF,
  0x00D0, 0x0145, 0x014C, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0168,
  0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
  0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x00EF,
  0x00F0, 0x0146, 0x014D, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0169,
  0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x0138,
};

/* ISO-8859-11 (Thai) 0xA0-0xFF */
static const uint16_t iso8859_11[96] = {
  0x00A0, 0x0E01, 0x0E02, 0x0E03, 0x0E04, 0x0E05, 0x0E06, 0x0E07,
  0x0E08, 0x0E09, 0x0E0A, 0x0E0B, 0x0E0C, 0x0E0D, 0x0E0E, 0x0E0F,
  0x0E10, 0x0E11, 0x0E12, 0x0E13, 0x0E14, 0x0E15, 0x0E16, 0x0E17,
  0x0E18, 0x0E19, 0x0E1A, 0x0E1B, 0x0E1C, 0x0E1D, 0x0E1E, 0x0E1F,
  0x0E20, 0x0E21, 0x0E22, 0x0E23, 0x0E24, 0x0E25, 0x0E26, 0x0E27,
  0x0E28, 0x0E29, 0x0E2A, 0x0E2B, 0x0E2C, 0x0E2D, 0x0E2E, 0x0E2F,
  0x0E30, 0x0E31, 0x0E32, 0x0E33, 0x0E34, 0x0E35, 0x0E36, 0x0E37,
  0x0E38, 0x0E39, 0x0E3A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0E3F,
  0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0x0E45, 0x0E46, 0x0E47,
  0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E, 0x0E4F,
  0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54, 0x0E55, 0x0E56, 0x0E57,
  0x0E58, 0x0E59, 0x0E5A, 0x0E5B, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* ISO-8859-13 (Latin-7 / Baltic Rim) 0xA0-0xFF */
static const uint16_t iso8859_13[96] = {
  0x00A0, 0x201D, 0x00A2, 0x00A3, 0x00A4, 0x201E, 0x00A6, 0x00A7,
  0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x201C, 0x00B5, 0x00B6, 0x00B7,
  0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
  0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112,
  0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
  0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7,
  0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
  0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113,
  0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
  0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7,
  0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x2019,
};

/* ISO-8859-14 (Latin-8 / Celtic) 0xA0-0xFF */
static const uint16_t iso8859_14[96] = {
  0x00A0, 0x1E02, 0x1E03, 0x00A3, 0x010A, 0x010B, 0x1E0A, 0x00A7,
  0x1E80, 0x00A9, 0x1E82, 0x1E0B, 0x1EF2, 0x00AD, 0x00AE, 0x0178,
  0x1E1E, 0x1E1F, 0x0120, 0x0121, 0x1E40, 0x1E41, 0x00B6, 0x1E56,
  0x1E81, 0x1E57, 0x1E83, 0x1E60, 0x1EF3, 0x1E84, 0x1E85, 0x1E61,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x0174, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x1E6A,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x0176, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x0175, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x1E6B,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x0177, 0x00FF,
};

/* Windows-1250 (Central European) 0x80-0xFF */
static const uint16_t win1250[128] = {
  0x20AC, 0x0081, 0x201A, 0x0083, 0x201E, 0x2026, 0x2020, 0x2021,
  0x0088, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x0098, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,
  0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
  0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
  0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
  0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
  0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
  0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
  0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
  0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
  0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
  0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,
};

/* Windows-1253 (Greek) 0x80-0xFF */
static const uint16_t win1253[128] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x0088, 0x2030, 0x008A, 0x2039, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x0098, 0x2122, 0x009A, 0x203A, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x0385, 0x0386, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x0000, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x2015,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x00B5, 0x00B6, 0x00B7,
  0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E, 0x038F,
  0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
  0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
  0x03A0, 0x03A1, 0x0000, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
  0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
  0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
  0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
  0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
  0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x0000,
};

/* Windows-1254 (Turkish) 0x80-0xFF */
static const uint16_t win1254[128] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x008E, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x009E, 0x0178,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
  0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
  0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF,
};

/* Windows-1255 (Hebrew) 0x80-0xFF */
static const uint16_t win1255[128] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x008A, 0x2039, 0x008C, 0x008D, 0x008E, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x009A, 0x203A, 0x009C, 0x009D, 0x009E, 0x009F,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AA, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7,
  0x05B8, 0x05B9, 0x0000, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05F0, 0x05F1, 0x05F2, 0x05F3,
  0x05F4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
  0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
  0x05E8, 0x05E9, 0x05EA, 0x0000, 0x0000, 0x200E, 0x200F, 0x0000,
};

/* Windows-1256 (Arabic) 0x80-0xFF */
static const uint16_t win1256[128] = {
  0x20AC, 0x067E, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x0679, 0x2039, 0x0152, 0x0686, 0x0698, 0x0688,
  0x06AF, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x06A9, 0x2122, 0x0691, 0x203A, 0x0153, 0x200C, 0x200D, 0x06BA,
  0x00A0, 0x060C, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x06BE, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x061B, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x061F,
  0x06C1, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
  0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
  0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x00D7,
  0x0637, 0x0638, 0x0639, 0x063A, 0x0640, 0x0641, 0x0642, 0x0643,
  0x00E0, 0x0644, 0x00E2, 0x0645, 0x0646, 0x0647, 0x0648, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0649, 0x064A, 0x00EE, 0x00EF,
  0x064B, 0x064C, 0x064D, 0x064E, 0x00F4, 0x064F, 0x0650, 0x00F7,
  0x0651, 0x00F9, 0x0652, 0x00FB, 0x00FC, 0x200E, 0x200F, 0x06D2,
};

/* Windows-1257 (Baltic) 0x80-0xFF */
static const uint16_t win1257[128] = {
  0x20AC, 0x0081, 0x201A, 0x0083, 0x201E, 0x2026, 0x2020, 0x2021,
  0x0088, 0x2030, 0x008A, 0x2039, 0x008C, 0x00A8, 0x02C7, 0x00B8,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x0098, 0x2122, 0x009A, 0x203A, 0x009C, 0x00AF, 0x02DB, 0x009F,
  0x00A0, 0x0000, 0x00A2, 0x00A3, 0x00A4, 0x0000, 0x00A6, 0x00A7,
  0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
  0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112,
  0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
  0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7,
  0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
  0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113,
  0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
  0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7,
  0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x02D9,
};

/* Windows-1258 (Vietnamese) 0x80-0xFF */
static const uint16_t win1258[128] = {
  0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
  0x02C6, 0x2030, 0x008A, 0x2039, 0x0152, 0x008D, 0x008E, 0x008F,
  0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x02DC, 0x2122, 0x009A, 0x203A, 0x0153, 0x009D, 0x009E, 0x0178,
  0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
  0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
  0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
  0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
  0x00C0, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
  0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x0300, 0x00CD, 0x00CE, 0x00CF,
  0x0110, 0x00D1, 0x0309, 0x00D3, 0x00D4, 0x01A0, 0x00D6, 0x00D7,
  0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x01AF, 0x0303, 0x00DF,
  0x00E0, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
  0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x0301, 0x00ED, 0x00EE, 0x00EF,
  0x0111, 0x00F1, 0x0323, 0x00F3, 0x00F4, 0x01A1, 0x00F6, 0x00F7,
  0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x01B0, 0x20AB, 0x00FF,
};

/* Get the conversion table for a charset.
 * Returns pointer to 128-entry (0x80-0xFF) or 96-entry (0xA0-0xFF) table.
 * Sets *base to the starting byte (0x80 or 0xA0). */
static const uint16_t *get_charset_table(int id, int *base, int *count) {
  switch (id) {
  case MACMBX_CHARSET_WINDOWS_1252:
    /* 0x00-0x7F = ASCII, 0x80-0x9F = special, 0xA0-0xFF = identity */
    *base = 0x80; *count = 32;
    return win1252_80_9f; /* only 0x80-0x9F; 0xA0+ is identity */
  case MACMBX_CHARSET_MAC_ROMAN:
    *base = 0x80; *count = 128;
    return mac_roman;
  case MACMBX_CHARSET_ISO_8859_2:
    *base = 0xA0; *count = 96;
    return iso8859_2;
  case MACMBX_CHARSET_ISO_8859_15:
    *base = 0xA0; *count = 96;
    return iso8859_15;
  case MACMBX_CHARSET_WINDOWS_1251:
    *base = 0x80; *count = 128;
    return win1251;
  case MACMBX_CHARSET_ISO_8859_3:
    *base = 0xA0; *count = 96;
    return iso8859_3;
  case MACMBX_CHARSET_ISO_8859_4:
    *base = 0xA0; *count = 96;
    return iso8859_4;
  case MACMBX_CHARSET_ISO_8859_5:
    *base = 0xA0; *count = 96;
    return iso8859_5;
  case MACMBX_CHARSET_ISO_8859_6:
    *base = 0xA0; *count = 96;
    return iso8859_6;
  case MACMBX_CHARSET_ISO_8859_7:
    *base = 0xA0; *count = 96;
    return iso8859_7;
  case MACMBX_CHARSET_ISO_8859_8:
    *base = 0xA0; *count = 96;
    return iso8859_8;
  case MACMBX_CHARSET_ISO_8859_9:
    *base = 0xA0; *count = 96;
    return iso8859_9;
  case MACMBX_CHARSET_ISO_8859_10:
    *base = 0xA0; *count = 96;
    return iso8859_10;
  case MACMBX_CHARSET_ISO_8859_11:
    *base = 0xA0; *count = 96;
    return iso8859_11;
  case MACMBX_CHARSET_ISO_8859_13:
    *base = 0xA0; *count = 96;
    return iso8859_13;
  case MACMBX_CHARSET_ISO_8859_14:
    *base = 0xA0; *count = 96;
    return iso8859_14;
  case MACMBX_CHARSET_WINDOWS_1250:
    *base = 0x80; *count = 128;
    return win1250;
  case MACMBX_CHARSET_WINDOWS_1253:
    *base = 0x80; *count = 128;
    return win1253;
  case MACMBX_CHARSET_WINDOWS_1254:
    *base = 0x80; *count = 128;
    return win1254;
  case MACMBX_CHARSET_WINDOWS_1255:
    *base = 0x80; *count = 128;
    return win1255;
  case MACMBX_CHARSET_WINDOWS_1256:
    *base = 0x80; *count = 128;
    return win1256;
  case MACMBX_CHARSET_WINDOWS_1257:
    *base = 0x80; *count = 128;
    return win1257;
  case MACMBX_CHARSET_WINDOWS_1258:
    *base = 0x80; *count = 128;
    return win1258;
  default:
    *base = 0; *count = 0;
    return NULL;
  }
}

/* Convert a single byte from a charset to Unicode codepoint */
static uint32_t byte_to_unicode(int charset_id, unsigned char byte) {
  if (byte < 0x80) return byte; /* ASCII is universal */

  int base, count;
  const uint16_t *table = get_charset_table(charset_id, &base, &count);

  if (charset_id == MACMBX_CHARSET_WINDOWS_1252) {
    /* Win-1252: 0x80-0x9F from table, 0xA0-0xFF = identity */
    if (byte >= 0x80 && byte <= 0x9F)
      return win1252_80_9f[byte - 0x80];
    return byte; /* 0xA0-0xFF = U+00A0-U+00FF */
  }

  if (charset_id == MACMBX_CHARSET_ISO_8859_1 ||
      charset_id == MACMBX_CHARSET_US_ASCII) {
    return byte; /* identity mapping */
  }

  if (table && byte >= (unsigned)base && byte < (unsigned)(base + count))
    return table[byte - base];

  /* Fallback for charsets without tables: identity (ISO-8859-x where x > 2) */
  return byte;
}

/* ================================================================
 * Charset name resolution
 * ================================================================ */

static int ci_strcmp(const char *a, const char *b) {
  while (*a && *b) {
    char ca = (char)tolower((unsigned char)*a);
    char cb = (char)tolower((unsigned char)*b);
    if (ca != cb) return ca - cb;
    a++; b++;
  }
  return (unsigned char)*a - (unsigned char)*b;
}

/* Normalize charset name: strip hyphens/underscores for comparison */
static void normalize_charset(const char *name, char *buf, int bufLen) {
  int j = 0;
  for (int i = 0; name[i] && j < bufLen - 1; i++) {
    char c = (char)tolower((unsigned char)name[i]);
    if (c != '-' && c != '_' && c != ' ')
      buf[j++] = c;
  }
  buf[j] = '\0';
}

static int charset_name_to_id(const char *name) {
  if (!name || !name[0]) return MACMBX_CHARSET_UNKNOWN;
  char norm[64];
  normalize_charset(name, norm, sizeof(norm));

  if (!strcmp(norm, "usascii") || !strcmp(norm, "ascii")) return MACMBX_CHARSET_US_ASCII;
  if (!strcmp(norm, "utf8")) return MACMBX_CHARSET_UTF8;
  if (!strcmp(norm, "utf16le")) return MACMBX_CHARSET_UTF16LE;
  if (!strcmp(norm, "utf16be")) return MACMBX_CHARSET_UTF16BE;
  if (!strcmp(norm, "utf16")) return MACMBX_CHARSET_UTF16BE; /* default BE */
  if (!strcmp(norm, "utf32le")) return MACMBX_CHARSET_UTF32LE;
  if (!strcmp(norm, "utf32be") || !strcmp(norm, "utf32")) return MACMBX_CHARSET_UTF32BE;

  /* ISO-8859-x */
  if (!strcmp(norm, "iso88591") || !strcmp(norm, "latin1")) return MACMBX_CHARSET_ISO_8859_1;
  if (!strcmp(norm, "iso88592") || !strcmp(norm, "latin2")) return MACMBX_CHARSET_ISO_8859_2;
  if (!strcmp(norm, "iso88593")) return MACMBX_CHARSET_ISO_8859_3;
  if (!strcmp(norm, "iso88594")) return MACMBX_CHARSET_ISO_8859_4;
  if (!strcmp(norm, "iso88595")) return MACMBX_CHARSET_ISO_8859_5;
  if (!strcmp(norm, "iso88596")) return MACMBX_CHARSET_ISO_8859_6;
  if (!strcmp(norm, "iso88597")) return MACMBX_CHARSET_ISO_8859_7;
  if (!strcmp(norm, "iso88598")) return MACMBX_CHARSET_ISO_8859_8;
  if (!strcmp(norm, "iso88599") || !strcmp(norm, "latin5")) return MACMBX_CHARSET_ISO_8859_9;
  if (!strcmp(norm, "iso885910")) return MACMBX_CHARSET_ISO_8859_10;
  if (!strcmp(norm, "iso885911")) return MACMBX_CHARSET_ISO_8859_11;
  if (!strcmp(norm, "iso885913")) return MACMBX_CHARSET_ISO_8859_13;
  if (!strcmp(norm, "iso885914")) return MACMBX_CHARSET_ISO_8859_14;
  if (!strcmp(norm, "iso885915") || !strcmp(norm, "latin9")) return MACMBX_CHARSET_ISO_8859_15;

  /* Windows codepages */
  if (!strcmp(norm, "windows1250") || !strcmp(norm, "cp1250")) return MACMBX_CHARSET_WINDOWS_1250;
  if (!strcmp(norm, "windows1251") || !strcmp(norm, "cp1251")) return MACMBX_CHARSET_WINDOWS_1251;
  if (!strcmp(norm, "windows1252") || !strcmp(norm, "cp1252")) return MACMBX_CHARSET_WINDOWS_1252;
  if (!strcmp(norm, "windows1253") || !strcmp(norm, "cp1253")) return MACMBX_CHARSET_WINDOWS_1253;
  if (!strcmp(norm, "windows1254") || !strcmp(norm, "cp1254")) return MACMBX_CHARSET_WINDOWS_1254;
  if (!strcmp(norm, "windows1255") || !strcmp(norm, "cp1255")) return MACMBX_CHARSET_WINDOWS_1255;
  if (!strcmp(norm, "windows1256") || !strcmp(norm, "cp1256")) return MACMBX_CHARSET_WINDOWS_1256;
  if (!strcmp(norm, "windows1257") || !strcmp(norm, "cp1257")) return MACMBX_CHARSET_WINDOWS_1257;
  if (!strcmp(norm, "windows1258") || !strcmp(norm, "cp1258")) return MACMBX_CHARSET_WINDOWS_1258;

  if (!strcmp(norm, "macroman") || !strcmp(norm, "macintosh") ||
      !strcmp(norm, "xmacroman")) return MACMBX_CHARSET_MAC_ROMAN;

  return MACMBX_CHARSET_UNKNOWN;
}

const char *macmbx_charset_name(int id) {
  switch (id) {
  case MACMBX_CHARSET_US_ASCII:      return "US-ASCII";
  case MACMBX_CHARSET_UTF8:          return "UTF-8";
  case MACMBX_CHARSET_UTF16LE:       return "UTF-16LE";
  case MACMBX_CHARSET_UTF16BE:       return "UTF-16BE";
  case MACMBX_CHARSET_UTF32LE:       return "UTF-32LE";
  case MACMBX_CHARSET_UTF32BE:       return "UTF-32BE";
  case MACMBX_CHARSET_ISO_8859_1:    return "ISO-8859-1";
  case MACMBX_CHARSET_ISO_8859_2:    return "ISO-8859-2";
  case MACMBX_CHARSET_ISO_8859_3:    return "ISO-8859-3";
  case MACMBX_CHARSET_ISO_8859_4:    return "ISO-8859-4";
  case MACMBX_CHARSET_ISO_8859_5:    return "ISO-8859-5";
  case MACMBX_CHARSET_ISO_8859_6:    return "ISO-8859-6";
  case MACMBX_CHARSET_ISO_8859_7:    return "ISO-8859-7";
  case MACMBX_CHARSET_ISO_8859_8:    return "ISO-8859-8";
  case MACMBX_CHARSET_ISO_8859_9:    return "ISO-8859-9";
  case MACMBX_CHARSET_ISO_8859_10:   return "ISO-8859-10";
  case MACMBX_CHARSET_ISO_8859_11:   return "ISO-8859-11";
  case MACMBX_CHARSET_ISO_8859_13:   return "ISO-8859-13";
  case MACMBX_CHARSET_ISO_8859_14:   return "ISO-8859-14";
  case MACMBX_CHARSET_ISO_8859_15:   return "ISO-8859-15";
  case MACMBX_CHARSET_WINDOWS_1250:  return "Windows-1250";
  case MACMBX_CHARSET_WINDOWS_1251:  return "Windows-1251";
  case MACMBX_CHARSET_WINDOWS_1252:  return "Windows-1252";
  case MACMBX_CHARSET_WINDOWS_1253:  return "Windows-1253";
  case MACMBX_CHARSET_WINDOWS_1254:  return "Windows-1254";
  case MACMBX_CHARSET_WINDOWS_1255:  return "Windows-1255";
  case MACMBX_CHARSET_WINDOWS_1256:  return "Windows-1256";
  case MACMBX_CHARSET_WINDOWS_1257:  return "Windows-1257";
  case MACMBX_CHARSET_WINDOWS_1258:  return "Windows-1258";
  case MACMBX_CHARSET_MAC_ROMAN:     return "macintosh";
  default: return "unknown";
  }
}

/* ================================================================
 * Charset conversion: to UTF-8
 * ================================================================ */

int macmbx_charset_to_utf8(const char *charset, const char *in, int inLen,
                            char **out, int *outLen) {
  if (!in || inLen < 0 || !out || !outLen) return -1;

  int id = charset_name_to_id(charset);

  /* UTF-8 passthrough */
  if (id == MACMBX_CHARSET_UTF8) {
    *out = (char *)malloc(inLen + 1);
    memcpy(*out, in, inLen);
    (*out)[inLen] = '\0';
    *outLen = inLen;
    return 0;
  }

  /* UTF-16 */
  if (id == MACMBX_CHARSET_UTF16LE || id == MACMBX_CHARSET_UTF16BE) {
    /* Allocate worst case: each UTF-16 pair -> 4 UTF-8 bytes */
    char *buf = (char *)malloc(inLen * 2 + 1);
    int bpos = 0;
    bool le = (id == MACMBX_CHARSET_UTF16LE);
    int start = 0;
    /* Strip BOM if present */
    if (inLen >= 2) {
      uint16_t bom;
      if (le) bom = (unsigned char)in[0] | ((unsigned char)in[1] << 8);
      else    bom = ((unsigned char)in[0] << 8) | (unsigned char)in[1];
      if (bom == 0xFEFF) start = 2;
    }
    for (int i = start; i + 1 < inLen; i += 2) {
      uint16_t w;
      if (le) w = (unsigned char)in[i] | ((unsigned char)in[i+1] << 8);
      else    w = ((unsigned char)in[i] << 8) | (unsigned char)in[i+1];
      uint32_t cp = w;
      /* Handle surrogate pairs */
      if (w >= 0xD800 && w <= 0xDBFF && i + 3 < inLen) {
        uint16_t w2;
        if (le) w2 = (unsigned char)in[i+2] | ((unsigned char)in[i+3] << 8);
        else    w2 = ((unsigned char)in[i+2] << 8) | (unsigned char)in[i+3];
        if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
          cp = 0x10000 + ((w - 0xD800) << 10) + (w2 - 0xDC00);
          i += 2;
        }
      }
      bpos += macmbx_utf8_encode(cp, buf + bpos, 4);
    }
    buf[bpos] = '\0';
    *out = buf;
    *outLen = bpos;
    return 0;
  }

  /* UTF-32 */
  if (id == MACMBX_CHARSET_UTF32LE || id == MACMBX_CHARSET_UTF32BE) {
    char *buf = (char *)malloc(inLen + 1); /* worst case: inLen/4 codepoints * 4 bytes */
    int bpos = 0;
    bool le = (id == MACMBX_CHARSET_UTF32LE);
    int start = 0;
    /* Strip BOM if present */
    if (inLen >= 4) {
      uint32_t bom;
      if (le) bom = (unsigned char)in[0] | ((unsigned char)in[1] << 8) |
                     ((unsigned char)in[2] << 16) | ((unsigned char)in[3] << 24);
      else    bom = ((unsigned char)in[0] << 24) | ((unsigned char)in[1] << 16) |
                     ((unsigned char)in[2] << 8) | (unsigned char)in[3];
      if (bom == 0xFEFF) start = 4;
    }
    for (int i = start; i + 3 < inLen; i += 4) {
      uint32_t cp;
      if (le) cp = (unsigned char)in[i] | ((unsigned char)in[i+1] << 8) |
                    ((unsigned char)in[i+2] << 16) | ((unsigned char)in[i+3] << 24);
      else    cp = ((unsigned char)in[i] << 24) | ((unsigned char)in[i+1] << 16) |
                    ((unsigned char)in[i+2] << 8) | (unsigned char)in[i+3];
      if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = 0xFFFD;
      bpos += macmbx_utf8_encode(cp, buf + bpos, 4);
    }
    buf[bpos] = '\0';
    *out = buf;
    *outLen = bpos;
    return 0;
  }

  /* Single-byte charsets */
  /* Worst case: each byte → 3 UTF-8 bytes */
  char *buf = (char *)malloc(inLen * 3 + 1);
  int bpos = 0;
  for (int i = 0; i < inLen; i++) {
    uint32_t cp = byte_to_unicode(id, (unsigned char)in[i]);
    bpos += macmbx_utf8_encode(cp, buf + bpos, 4);
  }
  buf[bpos] = '\0';
  *out = buf;
  *outLen = bpos;
  return 0;
}

/* ================================================================
 * Charset conversion: from UTF-8
 * ================================================================ */

int macmbx_utf8_to_charset(const char *charset, const char *in, int inLen,
                            char **out, int *outLen) {
  if (!in || inLen < 0 || !out || !outLen) return -1;

  int id = charset_name_to_id(charset);

  /* UTF-8 passthrough */
  if (id == MACMBX_CHARSET_UTF8) {
    *out = (char *)malloc(inLen + 1);
    memcpy(*out, in, inLen);
    (*out)[inLen] = '\0';
    *outLen = inLen;
    return 0;
  }

  /* UTF-16 */
  if (id == MACMBX_CHARSET_UTF16LE || id == MACMBX_CHARSET_UTF16BE) {
    /* Worst case: each codepoint -> 4 bytes (surrogate pair) */
    char *buf = (char *)malloc(inLen * 2 + 1);
    int bpos = 0;
    bool le = (id == MACMBX_CHARSET_UTF16LE);
    const char *p = in, *end_p = in + inLen;
    while (p < end_p) {
      uint32_t cp = macmbx_utf8_decode(p, &p);
      if (cp < 0x10000) {
        if (le) { buf[bpos++] = (char)(cp & 0xFF); buf[bpos++] = (char)((cp >> 8) & 0xFF); }
        else    { buf[bpos++] = (char)((cp >> 8) & 0xFF); buf[bpos++] = (char)(cp & 0xFF); }
      } else {
        cp -= 0x10000;
        uint16_t hi = 0xD800 + (uint16_t)(cp >> 10);
        uint16_t lo = 0xDC00 + (uint16_t)(cp & 0x3FF);
        if (le) {
          buf[bpos++] = (char)(hi & 0xFF); buf[bpos++] = (char)((hi >> 8) & 0xFF);
          buf[bpos++] = (char)(lo & 0xFF); buf[bpos++] = (char)((lo >> 8) & 0xFF);
        } else {
          buf[bpos++] = (char)((hi >> 8) & 0xFF); buf[bpos++] = (char)(hi & 0xFF);
          buf[bpos++] = (char)((lo >> 8) & 0xFF); buf[bpos++] = (char)(lo & 0xFF);
        }
      }
    }
    *out = buf;
    *outLen = bpos;
    return 0;
  }

  /* UTF-32 */
  if (id == MACMBX_CHARSET_UTF32LE || id == MACMBX_CHARSET_UTF32BE) {
    char *buf = (char *)malloc(inLen * 4 + 1);
    int bpos = 0;
    bool le = (id == MACMBX_CHARSET_UTF32LE);
    const char *p = in, *end_p = in + inLen;
    while (p < end_p) {
      uint32_t cp = macmbx_utf8_decode(p, &p);
      if (le) {
        buf[bpos++] = (char)(cp & 0xFF);
        buf[bpos++] = (char)((cp >> 8) & 0xFF);
        buf[bpos++] = (char)((cp >> 16) & 0xFF);
        buf[bpos++] = (char)((cp >> 24) & 0xFF);
      } else {
        buf[bpos++] = (char)((cp >> 24) & 0xFF);
        buf[bpos++] = (char)((cp >> 16) & 0xFF);
        buf[bpos++] = (char)((cp >> 8) & 0xFF);
        buf[bpos++] = (char)(cp & 0xFF);
      }
    }
    *out = buf;
    *outLen = bpos;
    return 0;
  }

  /* For single-byte charsets: build reverse table, then convert */
  /* Allocate output (one byte per codepoint, worst case = inLen) */
  char *buf = (char *)malloc(inLen + 1);
  int bpos = 0;
  const char *p = in, *end = in + inLen;

  while (p < end) {
    uint32_t cp = macmbx_utf8_decode(p, &p);
    if (cp < 0x80) {
      buf[bpos++] = (char)cp;
    } else {
      /* Try to find byte in charset table */
      bool found = false;
      int base, count;
      const uint16_t *table = get_charset_table(id, &base, &count);

      if (id == MACMBX_CHARSET_WINDOWS_1252) {
        /* Check 0x80-0x9F special range */
        for (int i = 0; i < 32; i++) {
          if (win1252_80_9f[i] == cp) { buf[bpos++] = (char)(0x80 + i); found = true; break; }
        }
        /* 0xA0-0xFF is identity for Latin-1 range */
        if (!found && cp >= 0xA0 && cp <= 0xFF) { buf[bpos++] = (char)cp; found = true; }
      } else if (id == MACMBX_CHARSET_ISO_8859_1) {
        if (cp <= 0xFF) { buf[bpos++] = (char)cp; found = true; }
      } else if (table) {
        for (int i = 0; i < count; i++) {
          if (table[i] == cp) { buf[bpos++] = (char)(base + i); found = true; break; }
        }
      }
      if (!found) buf[bpos++] = '?'; /* unmappable */
    }
  }

  buf[bpos] = '\0';
  *out = buf;
  *outLen = bpos;
  return 0;
}

/* ================================================================
 * Charset detection
 * ================================================================ */

int macmbx_charset_detect(const char *data, int len) {
  if (!data || len < 1) return MACMBX_CHARSET_UNKNOWN;
  const unsigned char *p = (const unsigned char *)data;

  /* BOM detection */
  if (len >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) return MACMBX_CHARSET_UTF8;
  if (len >= 4 && p[0] == 0xFF && p[1] == 0xFE && p[2] == 0 && p[3] == 0) return MACMBX_CHARSET_UTF32LE;
  if (len >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 0xFE && p[3] == 0xFF) return MACMBX_CHARSET_UTF32BE;
  if (len >= 2 && p[0] == 0xFF && p[1] == 0xFE) return MACMBX_CHARSET_UTF16LE;
  if (len >= 2 && p[0] == 0xFE && p[1] == 0xFF) return MACMBX_CHARSET_UTF16BE;

  /* Try UTF-8 validation */
  if (macmbx_utf8_validate(data, len)) {
    /* Check if there are any high bytes — if not, it's ASCII */
    bool has_high = false;
    for (int i = 0; i < len; i++)
      if (p[i] >= 0x80) { has_high = true; break; }
    return has_high ? MACMBX_CHARSET_UTF8 : MACMBX_CHARSET_US_ASCII;
  }

  /* Has high bytes but not valid UTF-8 → likely Windows-1252 */
  return MACMBX_CHARSET_WINDOWS_1252;
}

/* ================================================================
 * RFC 2047 MIME encoded-word
 * ================================================================ */

/* Inline base64 decoder for RFC 2047 B-encoding */
static int b64_decode(const char *in, int inLen, char *out, int maxOut) {
  static const int8_t b64[256] = {
    [0 ... 255] = -1,
    ['A'] = 0, ['B'] = 1, ['C'] = 2, ['D'] = 3, ['E'] = 4, ['F'] = 5,
    ['G'] = 6, ['H'] = 7, ['I'] = 8, ['J'] = 9, ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17,
    ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25,
    ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29, ['e'] = 30, ['f'] = 31,
    ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37,
    ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41, ['q'] = 42, ['r'] = 43,
    ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49,
    ['y'] = 50, ['z'] = 51,
    ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57,
    ['6'] = 58, ['7'] = 59, ['8'] = 60, ['9'] = 61,
    ['+'] = 62, ['/'] = 63,
  };
  int o = 0, acc = 0, bits = 0;
  for (int i = 0; i < inLen && o < maxOut; i++) {
    int v = b64[(unsigned char)in[i]];
    if (v < 0) { if (in[i] == '=') break; continue; }
    acc = (acc << 6) | v;
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out[o++] = (char)((acc >> bits) & 0xFF);
    }
  }
  return o;
}

/* Q-encoding decoder for RFC 2047 */
static int q_decode(const char *in, int inLen, char *out, int maxOut) {
  int o = 0;
  for (int i = 0; i < inLen && o < maxOut; i++) {
    if (in[i] == '_') {
      out[o++] = ' ';
    } else if (in[i] == '=' && i + 2 < inLen) {
      int hi = -1, lo = -1;
      char c1 = in[i+1], c2 = in[i+2];
      if (c1 >= '0' && c1 <= '9') hi = c1 - '0';
      else if (c1 >= 'A' && c1 <= 'F') hi = c1 - 'A' + 10;
      else if (c1 >= 'a' && c1 <= 'f') hi = c1 - 'a' + 10;
      if (c2 >= '0' && c2 <= '9') lo = c2 - '0';
      else if (c2 >= 'A' && c2 <= 'F') lo = c2 - 'A' + 10;
      else if (c2 >= 'a' && c2 <= 'f') lo = c2 - 'a' + 10;
      if (hi >= 0 && lo >= 0) {
        out[o++] = (char)((hi << 4) | lo);
        i += 2;
      } else {
        out[o++] = in[i];
      }
    } else {
      out[o++] = in[i];
    }
  }
  return o;
}

char *macmbx_mime_decode_header(const char *header) {
  if (!header) return NULL;
  int hlen = (int)strlen(header);
  /* Worst case: output ≤ input length */
  char *result = (char *)malloc(hlen * 4 + 1);
  int rpos = 0;
  bool last_was_encoded = false;

  const char *p = header;
  while (*p) {
    /* Look for =?charset?encoding?text?= */
    if (p[0] == '=' && p[1] == '?') {
      const char *cs_start = p + 2;
      const char *cs_end = strchr(cs_start, '?');
      if (cs_end && cs_end[1] && cs_end[2] == '?') {
        char encoding = (char)toupper((unsigned char)cs_end[1]);
        const char *text_start = cs_end + 3;
        const char *text_end = strstr(text_start, "?=");
        if (text_end && (encoding == 'B' || encoding == 'Q')) {
          /* Skip whitespace between consecutive encoded words */
          if (last_was_encoded) {
            /* already skipped */
          }

          /* Extract charset */
          int cs_len = (int)(cs_end - cs_start);
          char cs[64];
          if (cs_len >= (int)sizeof(cs)) cs_len = sizeof(cs) - 1;
          memcpy(cs, cs_start, cs_len);
          cs[cs_len] = '\0';

          /* Decode text */
          int text_len = (int)(text_end - text_start);
          char *decoded = (char *)malloc(text_len + 1);
          int dec_len;
          if (encoding == 'B')
            dec_len = b64_decode(text_start, text_len, decoded, text_len);
          else
            dec_len = q_decode(text_start, text_len, decoded, text_len);

          /* Convert to UTF-8 */
          char *utf8 = NULL;
          int utf8_len = 0;
          macmbx_charset_to_utf8(cs, decoded, dec_len, &utf8, &utf8_len);
          free(decoded);

          if (utf8) {
            memcpy(result + rpos, utf8, utf8_len);
            rpos += utf8_len;
            free(utf8);
          }

          p = text_end + 2;
          last_was_encoded = true;

          /* Skip whitespace between consecutive encoded words (RFC 2047 §6.2) */
          if (last_was_encoded) {
            const char *ws = p;
            while (*ws == ' ' || *ws == '\t' || *ws == '\r' || *ws == '\n') ws++;
            if (ws[0] == '=' && ws[1] == '?') p = ws;
          }
          continue;
        }
      }
    }
    last_was_encoded = false;
    result[rpos++] = *p++;
  }
  result[rpos] = '\0';
  return result;
}

char *macmbx_mime_encode_header(const char *utf8, const char *charset) {
  if (!utf8) return NULL;
  if (!charset) charset = "UTF-8";
  int len = (int)strlen(utf8);

  /* Check if encoding is needed */
  if (macmbx_is_ascii(utf8, len))
    return strdup(utf8);

  /* Use B-encoding (base64) — simpler for arbitrary charsets */
  static const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  /* Allocate: =?charset?B?...?= overhead + base64 expansion */
  int max_out = (int)strlen(charset) + 10 + (len * 4 / 3) + 10;
  char *result = (char *)malloc(max_out);
  int rpos = snprintf(result, max_out, "=?%s?B?", charset);

  /* Base64 encode */
  int i;
  for (i = 0; i + 2 < len; i += 3) {
    unsigned int v = ((unsigned char)utf8[i] << 16) |
                     ((unsigned char)utf8[i+1] << 8) |
                     (unsigned char)utf8[i+2];
    result[rpos++] = b64chars[(v >> 18) & 0x3F];
    result[rpos++] = b64chars[(v >> 12) & 0x3F];
    result[rpos++] = b64chars[(v >> 6) & 0x3F];
    result[rpos++] = b64chars[v & 0x3F];
  }
  if (i < len) {
    unsigned int v = (unsigned char)utf8[i] << 16;
    if (i + 1 < len) v |= (unsigned char)utf8[i+1] << 8;
    result[rpos++] = b64chars[(v >> 18) & 0x3F];
    result[rpos++] = b64chars[(v >> 12) & 0x3F];
    result[rpos++] = (i + 1 < len) ? b64chars[(v >> 6) & 0x3F] : '=';
    result[rpos++] = '=';
  }

  result[rpos++] = '?';
  result[rpos++] = '=';
  result[rpos] = '\0';
  return result;
}

/* ================================================================
 * Convenience
 * ================================================================ */

char *macmbx_to_utf8(const char *data, int len, const char *charset) {
  char *out = NULL;
  int outLen = 0;
  if (macmbx_charset_to_utf8(charset, data, len, &out, &outLen) != 0)
    return NULL;
  return out;
}

bool macmbx_is_ascii(const char *data, int len) {
  if (!data) return true;
  for (int i = 0; i < len; i++)
    if ((unsigned char)data[i] >= 0x80) return false;
  return true;
}

bool macmbx_is_utf8(const char *data, int len) {
  return macmbx_utf8_validate(data, len);
}

/* ================================================================
 * ISO-2022 escape sequence stripping
 * ================================================================ */

int macmbx_clean_iso2022(const char *in, int inLen, char *out, int maxOut) {
  if (!in || inLen <= 0 || !out || maxOut <= 0) return 0;
  int o = 0;
  for (int i = 0; i < inLen && o < maxOut - 1; i++) {
    if ((unsigned char)in[i] == 0x1B && i + 1 < inLen) {
      /* ESC sequence — skip ESC and designator bytes */
      i++; /* skip ESC */
      /* ESC $ B, ESC $ @, ESC ( B, ESC ( J, ESC ( I, etc. */
      if (in[i] == '$' || in[i] == '(') {
        i++; /* skip $ or ( */
        /* The final byte is the next char, skip it too */
        continue;
      }
      /* ESC . A, ESC . F etc (96-char sets) */
      if (in[i] == '.' || in[i] == '-' || in[i] == ')') {
        i++; /* skip final byte */
        continue;
      }
      /* Unknown ESC sequence, skip just the ESC */
      i--; /* re-examine this byte */
      continue;
    }
    /* SI (0x0F) and SO (0x0E) shift codes — strip them */
    if ((unsigned char)in[i] == 0x0E || (unsigned char)in[i] == 0x0F)
      continue;
    out[o++] = in[i];
  }
  if (o < maxOut) out[o] = '\0';
  return o;
}

/* ================================================================
 * Content-Type charset extraction
 * ================================================================ */

const char *macmbx_parse_charset(const char *content_type) {
  if (!content_type) return NULL;
  static char result[64];
  const char *p;

  /* Try "charset=" in Content-Type header */
  p = content_type;
  while (*p) {
    /* Case-insensitive search for "charset" */
    if (tolower((unsigned char)p[0]) == 'c' &&
        tolower((unsigned char)p[1]) == 'h' &&
        tolower((unsigned char)p[2]) == 'a' &&
        tolower((unsigned char)p[3]) == 'r' &&
        tolower((unsigned char)p[4]) == 's' &&
        tolower((unsigned char)p[5]) == 'e' &&
        tolower((unsigned char)p[6]) == 't') {
      p += 7;
      /* Skip whitespace and = */
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '=') {
        p++;
        while (*p == ' ' || *p == '\t') p++;
        /* Strip optional quotes */
        char quote = 0;
        if (*p == '"' || *p == '\'') { quote = *p; p++; }
        int j = 0;
        while (*p && j < (int)sizeof(result) - 1) {
          if (quote && *p == quote) break;
          if (!quote && (*p == ';' || *p == ' ' || *p == '\t' || *p == '"' || *p == '>')) break;
          result[j++] = *p++;
        }
        result[j] = '\0';
        if (j > 0) return result;
      }
    }
    p++;
  }

  /* Try HTML <meta charset="..."> */
  p = content_type;
  while (*p) {
    if (p[0] == '<' && tolower((unsigned char)p[1]) == 'm' &&
        tolower((unsigned char)p[2]) == 'e' &&
        tolower((unsigned char)p[3]) == 't' &&
        tolower((unsigned char)p[4]) == 'a') {
      /* Scan within this tag for charset= */
      const char *tag_end = strchr(p, '>');
      if (!tag_end) break;
      const char *inner = p + 5;
      while (inner < tag_end) {
        if (tolower((unsigned char)inner[0]) == 'c' &&
            tolower((unsigned char)inner[1]) == 'h' &&
            tolower((unsigned char)inner[2]) == 'a' &&
            tolower((unsigned char)inner[3]) == 'r' &&
            tolower((unsigned char)inner[4]) == 's' &&
            tolower((unsigned char)inner[5]) == 'e' &&
            tolower((unsigned char)inner[6]) == 't') {
          inner += 7;
          while (inner < tag_end && (*inner == ' ' || *inner == '\t')) inner++;
          if (*inner == '=') {
            inner++;
            while (inner < tag_end && (*inner == ' ' || *inner == '\t')) inner++;
            char quote = 0;
            if (*inner == '"' || *inner == '\'') { quote = *inner; inner++; }
            int j = 0;
            while (inner < tag_end && j < (int)sizeof(result) - 1) {
              if (quote && *inner == quote) break;
              if (!quote && (*inner == ';' || *inner == ' ' || *inner == '\t' ||
                             *inner == '"' || *inner == '>')) break;
              result[j++] = *inner++;
            }
            result[j] = '\0';
            if (j > 0) return result;
          }
        }
        inner++;
      }
      p = tag_end;
    }
    p++;
  }

  return NULL;
}
