/* crispy_md5.h — MD5 hash (RFC 1321)
 * Part of crispy: standalone, uses OpenSSL EVP for the actual hash.
 *
 * Used for APOP authentication, CRAM-MD5, and Message-ID generation.
 */

#ifndef CRISPY_MD5_H
#define CRISPY_MD5_H

#include <stddef.h>

/* Compute MD5 hash of data. digest must be >= 16 bytes. */
void crispy_md5(const void *data, size_t len, unsigned char digest[16]);

/* Compute MD5 and return as 32-char hex string. buf must be >= 33 bytes. */
char *crispy_md5_hex(const void *data, size_t len, char buf[33]);

/* HMAC-MD5 (RFC 2104) — used for CRAM-MD5 auth.
 * digest must be >= 16 bytes. */
void crispy_hmac_md5(const void *key, size_t keyLen,
                     const void *data, size_t dataLen,
                     unsigned char digest[16]);

/* HMAC-MD5 as 32-char hex string. buf must be >= 33 bytes. */
char *crispy_hmac_md5_hex(const void *key, size_t keyLen,
                          const void *data, size_t dataLen,
                          char buf[33]);

#endif /* CRISPY_MD5_H */
