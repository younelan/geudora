/* crispy_md5.c — MD5 hash using OpenSSL EVP
 * Part of crispy: standalone.
 */

#include "crispy_md5.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <string.h>

static const char hex_chars[] = "0123456789abcdef";

void crispy_md5(const void *data, size_t len, unsigned char digest[16]) {
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) { memset(digest, 0, 16); return; }

  EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
  EVP_DigestUpdate(ctx, data, len);

  unsigned int md_len = 16;
  EVP_DigestFinal_ex(ctx, digest, &md_len);
  EVP_MD_CTX_free(ctx);
}

char *crispy_md5_hex(const void *data, size_t len, char buf[33]) {
  unsigned char digest[16];
  crispy_md5(data, len, digest);

  for (int i = 0; i < 16; i++) {
    buf[i*2]     = hex_chars[digest[i] >> 4];
    buf[i*2 + 1] = hex_chars[digest[i] & 0x0F];
  }
  buf[32] = '\0';
  return buf;
}

void crispy_hmac_md5(const void *key, size_t keyLen,
                     const void *data, size_t dataLen,
                     unsigned char digest[16]) {
  unsigned int md_len = 16;
  HMAC(EVP_md5(), key, (int)keyLen, (const unsigned char *)data, dataLen,
       digest, &md_len);
}

char *crispy_hmac_md5_hex(const void *key, size_t keyLen,
                          const void *data, size_t dataLen,
                          char buf[33]) {
  unsigned char digest[16];
  crispy_hmac_md5(key, keyLen, data, dataLen, digest);

  for (int i = 0; i < 16; i++) {
    buf[i*2]     = hex_chars[digest[i] >> 4];
    buf[i*2 + 1] = hex_chars[digest[i] & 0x0F];
  }
  buf[32] = '\0';
  return buf;
}
