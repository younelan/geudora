/* crispy_encode.h — RFC 2047 encoded-word, charset conversion, QP encoding
 * Part of crispy: standalone mail library.
 */

#ifndef CRISPY_ENCODE_H
#define CRISPY_ENCODE_H

#include <stddef.h>
#include <stdbool.h>

/* --- RFC 2047 encoded-word --- */

/* Decode an RFC 2047 encoded header value.
 * Handles =?charset?B?...?= (base64) and =?charset?Q?...?= (QP).
 * Multiple encoded words separated by whitespace are concatenated.
 * Output is always UTF-8. Caller must free result.
 * Returns NULL on error (input returned as-is copy on decode failure). */
char *crispy_decode_header(const char *in);

/* Encode a UTF-8 string as RFC 2047 encoded-word(s) if it contains
 * non-ASCII characters. Returns malloc'd string.
 * If input is pure ASCII, returns a copy unchanged.
 * Uses base64 encoding (B) for reliability. */
char *crispy_encode_header(const char *utf8);

/* --- Charset conversion to UTF-8 --- */

/* Convert text from a named charset to UTF-8.
 * Supports: US-ASCII, UTF-8 (passthrough), ISO-8859-1, ISO-8859-15,
 *           Windows-1252, ISO-8859-2 through ISO-8859-9.
 * Returns malloc'd UTF-8 string. Caller must free.
 * Returns NULL on unsupported charset (caller should use input as-is). */
char *crispy_charset_to_utf8(const char *in, long inLen,
                             const char *charset);

/* Check if a charset name is supported. */
bool crispy_charset_supported(const char *charset);

/* --- Quoted-Printable encoding --- */

/* Encode data as Quoted-Printable (RFC 2045).
 * Wraps lines at 76 characters with soft line breaks (=\r\n).
 * Returns malloc'd string. Caller must free. */
char *crispy_qp_encode(const char *in, long inLen, long *outLen);

/* Decode Quoted-Printable data.
 * Returns malloc'd string. Caller must free. */
char *crispy_qp_decode(const char *in, long inLen, long *outLen);

#endif /* CRISPY_ENCODE_H */
