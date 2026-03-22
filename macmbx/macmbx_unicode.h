/*
 * macmbx_unicode.h — Portable Unicode and charset conversion for macmbx
 *
 * Standalone: no GLib, no Apple frameworks, no Eudora, no GTK.
 * Only standard C (stdio, stdlib, string, ctype, stdint, stdbool).
 *
 * Ported from original Eudora unicode.c (Apple TEC-based) to built-in
 * conversion tables for full portability.
 */

#ifndef MACMBX_UNICODE_H
#define MACMBX_UNICODE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Charset IDs ────────────────────────────────────────────────── */

enum {
    MACMBX_CHARSET_UNKNOWN    = 0,
    MACMBX_CHARSET_US_ASCII,
    MACMBX_CHARSET_UTF8,
    MACMBX_CHARSET_UTF16LE,
    MACMBX_CHARSET_UTF16BE,
    MACMBX_CHARSET_UTF32LE,
    MACMBX_CHARSET_UTF32BE,
    MACMBX_CHARSET_ISO_8859_1,
    MACMBX_CHARSET_ISO_8859_2,
    MACMBX_CHARSET_ISO_8859_3,
    MACMBX_CHARSET_ISO_8859_4,
    MACMBX_CHARSET_ISO_8859_5,
    MACMBX_CHARSET_ISO_8859_6,
    MACMBX_CHARSET_ISO_8859_7,
    MACMBX_CHARSET_ISO_8859_8,
    MACMBX_CHARSET_ISO_8859_9,
    MACMBX_CHARSET_ISO_8859_10,
    MACMBX_CHARSET_ISO_8859_11,
    MACMBX_CHARSET_ISO_8859_13,
    MACMBX_CHARSET_ISO_8859_14,
    MACMBX_CHARSET_ISO_8859_15,
    MACMBX_CHARSET_WINDOWS_1250,
    MACMBX_CHARSET_WINDOWS_1251,
    MACMBX_CHARSET_WINDOWS_1252,
    MACMBX_CHARSET_WINDOWS_1253,
    MACMBX_CHARSET_WINDOWS_1254,
    MACMBX_CHARSET_WINDOWS_1255,
    MACMBX_CHARSET_WINDOWS_1256,
    MACMBX_CHARSET_WINDOWS_1257,
    MACMBX_CHARSET_WINDOWS_1258,
    MACMBX_CHARSET_MAC_ROMAN,
    MACMBX_CHARSET_COUNT
};

/* ── UTF-8 codec ────────────────────────────────────────────────── */

/* Encode one codepoint to UTF-8.  Returns bytes written (0 on error). */
int  macmbx_utf8_encode(uint32_t codepoint, char *out, int maxLen);

/* Decode one codepoint from UTF-8.  Advances *next past the character.
 * Returns 0xFFFD on invalid sequence. */
uint32_t macmbx_utf8_decode(const char *utf8, const char **next);

/* Count codepoints in a UTF-8 buffer of byteLen bytes. */
int  macmbx_utf8_len(const char *utf8, int byteLen);

/* Find longest valid UTF-8 prefix (no split chars at end).
 * Equivalent to original GoodUTF8Len. */
int  macmbx_utf8_valid_len(const char *utf8, int byteLen);

/* Check if entire buffer is valid UTF-8. */
bool macmbx_utf8_validate(const char *utf8, int byteLen);

/* ── Charset conversion (replaces Apple TEC) ────────────────────── */

/* Convert from named charset to UTF-8.
 * Caller must free(*out) on success.  Returns 0 on success. */
int  macmbx_charset_to_utf8(const char *charset,
                            const char *in, int inLen,
                            char **out, int *outLen);

/* Convert from UTF-8 to named charset.
 * Caller must free(*out) on success.  Returns 0 on success. */
int  macmbx_utf8_to_charset(const char *charset,
                            const char *in, int inLen,
                            char **out, int *outLen);

/* Sniff encoding from BOM or byte patterns.  Returns charset ID. */
int  macmbx_charset_detect(const char *data, int len);

/* Get canonical charset name from ID. */
const char *macmbx_charset_name(int id);

/* ── RFC 2047 MIME encoded-word ─────────────────────────────────── */

/* Decode =?charset?B?...?= and =?charset?Q?...?= encoded words.
 * Returns malloc'd UTF-8 string (caller frees). */
char *macmbx_mime_decode_header(const char *header);

/* Encode UTF-8 string as RFC 2047 for use in email headers.
 * Returns malloc'd string (caller frees). */
char *macmbx_mime_encode_header(const char *utf8, const char *charset);

/* ── Convenience ────────────────────────────────────────────────── */

/* One-shot convert to UTF-8.  Returns malloc'd string (caller frees). */
char *macmbx_to_utf8(const char *data, int len, const char *charset);

/* Check if pure 7-bit ASCII. */
bool macmbx_is_ascii(const char *data, int len);

/* Alias for macmbx_utf8_validate. */
bool macmbx_is_utf8(const char *data, int len);

/* ── ISO-2022 ───────────────────────────────────────────────────── */

/* Strip ISO-2022 escape sequences (ESC $ B, ESC ( B, SI, SO, etc.).
 * Writes cleaned data to out. Returns bytes written. */
int  macmbx_clean_iso2022(const char *in, int inLen, char *out, int maxOut);

/* ── Content-Type charset parsing ────────────────────────────────── */

/* Extract charset from a Content-Type header or HTML meta tag.
 * Returns pointer to static buffer with charset name, or NULL. */
const char *macmbx_parse_charset(const char *content_type);

#ifdef __cplusplus
}
#endif

#endif /* MACMBX_UNICODE_H */
