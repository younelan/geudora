/* crispy_auth.c — SASL authentication mechanisms for SMTP and POP3
 * Part of crispy: standalone, uses OpenSSL for crypto.
 *
 * Implements: CRAM-MD5, XOAUTH2, SCRAM-SHA-256, DIGEST-MD5
 */

#include "crispy_smtp.h"
#include "crispy_pop3.h"
#include "crispy_md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Forward declarations for send_line/read helpers */
extern int crispy_smtp_read_reply(SmtpSession *s);

/* ================================================================
 * XOAUTH2 (RFC 7628 / Google extension)
 * Format: "user=<user>\x01auth=Bearer <token>\x01\x01"
 * ================================================================ */

static char *build_xoauth2_string(const char *user, const char *token,
                                   long *outLen) {
  /* user=<user>\1auth=Bearer <token>\1\1 */
  size_t uLen = strlen(user);
  size_t tLen = strlen(token);
  size_t rawLen = 5 + uLen + 1 + 12 + tLen + 2;
  char *raw = (char *)malloc(rawLen + 1);
  if (!raw) return NULL;

  int pos = 0;
  memcpy(raw + pos, "user=", 5); pos += 5;
  memcpy(raw + pos, user, uLen); pos += uLen;
  raw[pos++] = '\x01';
  memcpy(raw + pos, "auth=Bearer ", 12); pos += 12;
  memcpy(raw + pos, token, tLen); pos += tLen;
  raw[pos++] = '\x01';
  raw[pos++] = '\x01';
  raw[pos] = '\0';

  long b64Len;
  char *b64 = crispy_base64_encode(raw, (long)pos, &b64Len);
  free(raw);
  if (outLen) *outLen = b64Len;
  return b64;
}

int crispy_smtp_auth_xoauth2(SmtpSession *s, const char *user,
                              const char *token) {
  long b64Len;
  char *b64 = build_xoauth2_string(user, token, &b64Len);
  if (!b64) return -1;

  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "AUTH XOAUTH2 %s", b64);
  free(b64);

  int code = crispy_smtp_command(s, cmd);
  if (code == 235) s->authenticated = true;
  return SMTP_IS_OK(code) ? 0 : code;
}

int crispy_pop3_auth_xoauth2(Pop3Session *s, const char *user,
                              const char *token) {
  long b64Len;
  char *b64 = build_xoauth2_string(user, token, &b64Len);
  if (!b64) return -1;

  char cmd[2048];
  snprintf(cmd, sizeof(cmd), "AUTH XOAUTH2 %s", b64);
  free(b64);

  int err = crispy_pop3_command(s, cmd);
  if (!err) s->authenticated = true;
  return err;
}

/* ================================================================
 * POP3 CRAM-MD5
 * Same as SMTP CRAM-MD5 but with POP3 AUTH command
 * ================================================================ */

int crispy_pop3_auth_cram_md5(Pop3Session *s, const char *user,
                               const char *pass) {
  /* Step 1: send AUTH CRAM-MD5, expect + challenge */
  int err = crispy_pop3_command(s, "AUTH CRAM-MD5");
  if (err) return -1;

  /* The +OK response should contain a base64 challenge after the + */
  char *plus = strchr(s->last_reply, '+');
  if (!plus) return -1;
  plus++; /* skip + */
  while (*plus == ' ') plus++;

  /* Step 2: decode challenge */
  long chalLen;
  char *challenge = crispy_base64_decode(plus, (long)strlen(plus), &chalLen);
  if (!challenge) return -1;

  /* Step 3: HMAC-MD5(password, challenge) */
  char hex[33];
  crispy_hmac_md5_hex(pass, strlen(pass), challenge, chalLen, hex);
  free(challenge);

  /* Step 4: build "user hex" and base64-encode */
  char response[512];
  snprintf(response, sizeof(response), "%s %s", user, hex);

  long b64Len;
  char *b64 = crispy_base64_encode(response, (long)strlen(response), &b64Len);
  if (!b64) return -1;

  err = crispy_pop3_command(s, b64);
  free(b64);
  if (!err) s->authenticated = true;
  return err;
}

/* ================================================================
 * SCRAM-SHA-256 (RFC 7677)
 *
 * Client-first: n,,n=user,r=client-nonce
 * Server-first: r=combined-nonce,s=salt,i=iterations
 * Client-final: c=biws,r=combined-nonce,p=proof
 * Server-final: v=verifier
 * ================================================================ */

static void xor_bytes(unsigned char *a, const unsigned char *b, size_t len) {
  for (size_t i = 0; i < len; i++) a[i] ^= b[i];
}

static int scram_sha256_exchange(
    /* send_cmd: sends a line and reads reply. Returns reply code (SMTP) or 0/-1 (POP). */
    int (*send_cmd)(void *session, const char *line),
    int (*read_reply)(void *session, char *reply, size_t replySize),
    void *session,
    const char *user, const char *pass,
    int success_code) {

  /* Generate client nonce */
  char cnonce[25];
  {
    unsigned char rnd[18];
    crispy_md5(user, strlen(user), rnd); /* not truly random but unique enough */
    long b64Len;
    char *b64 = crispy_base64_encode((char *)rnd, 16, &b64Len);
    if (b64) { snprintf(cnonce, sizeof(cnonce), "%s", b64); free(b64); }
    else snprintf(cnonce, sizeof(cnonce), "crispy%lx", (unsigned long)time(NULL));
  }

  /* Client-first-message-bare: n=user,r=cnonce */
  char cfmb[512];
  snprintf(cfmb, sizeof(cfmb), "n=%s,r=%s", user, cnonce);

  /* Client-first-message: n,,<cfmb> */
  char cfm[512];
  snprintf(cfm, sizeof(cfm), "n,,%s", cfmb);

  /* Base64 and send */
  long b64Len;
  char *b64 = crispy_base64_encode(cfm, (long)strlen(cfm), &b64Len);
  if (!b64) return -1;

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "AUTH SCRAM-SHA-256 %s", b64);
  free(b64);

  int code = send_cmd(session, cmd);
  if (code != 334 && code != 0) return code ? code : -1;

  /* Read server-first-message */
  char sfmReply[1024] = {0};
  read_reply(session, sfmReply, sizeof(sfmReply));

  /* Decode server-first */
  long sfmLen;
  char *sfm = crispy_base64_decode(sfmReply, (long)strlen(sfmReply), &sfmLen);
  if (!sfm) return -1;

  /* Parse r=nonce,s=salt,i=iterations */
  char *rnonce = NULL, *saltB64 = NULL;
  int iters = 4096;
  {
    char *tok = strtok(sfm, ",");
    while (tok) {
      if (strncmp(tok, "r=", 2) == 0) rnonce = tok + 2;
      else if (strncmp(tok, "s=", 2) == 0) saltB64 = tok + 2;
      else if (strncmp(tok, "i=", 2) == 0) iters = atoi(tok + 2);
      tok = strtok(NULL, ",");
    }
  }
  if (!rnonce || !saltB64) { free(sfm); return -1; }

  /* Decode salt */
  long saltLen;
  unsigned char *salt = (unsigned char *)crispy_base64_decode(saltB64,
      (long)strlen(saltB64), &saltLen);
  if (!salt) { free(sfm); return -1; }

  /* SaltedPassword = PBKDF2(password, salt, iterations, 32) */
  unsigned char saltedPw[32];
  crispy_pbkdf2_sha256(pass, strlen(pass), salt, saltLen, iters, saltedPw, 32);
  free(salt);

  /* ClientKey = HMAC(SaltedPassword, "Client Key") */
  unsigned char clientKey[32];
  crispy_hmac_sha256(saltedPw, 32, "Client Key", 10, clientKey);

  /* StoredKey = SHA256(ClientKey) */
  unsigned char storedKey[32];
  crispy_sha256(clientKey, 32, storedKey);

  /* Client-final-message-without-proof: c=biws,r=<rnonce> */
  char cfmwp[512];
  snprintf(cfmwp, sizeof(cfmwp), "c=biws,r=%s", rnonce);

  /* AuthMessage = cfmb,sfm,cfmwp */
  /* Note: sfm was modified by strtok, reconstruct from reply */
  char authMsg[2048];
  long sfmRawLen;
  char *sfmRaw = crispy_base64_decode(sfmReply, (long)strlen(sfmReply), &sfmRawLen);
  snprintf(authMsg, sizeof(authMsg), "%s,%s,%s", cfmb,
           sfmRaw ? sfmRaw : "", cfmwp);
  free(sfmRaw);
  free(sfm);

  /* ClientSignature = HMAC(StoredKey, AuthMessage) */
  unsigned char clientSig[32];
  crispy_hmac_sha256(storedKey, 32, authMsg, strlen(authMsg), clientSig);

  /* ClientProof = ClientKey XOR ClientSignature */
  unsigned char proof[32];
  memcpy(proof, clientKey, 32);
  xor_bytes(proof, clientSig, 32);

  /* Base64 encode proof */
  char *proofB64 = crispy_base64_encode((char *)proof, 32, &b64Len);
  if (!proofB64) return -1;

  /* Client-final-message: cfmwp,p=proof */
  char finalMsg[1024];
  snprintf(finalMsg, sizeof(finalMsg), "%s,p=%s", cfmwp, proofB64);
  free(proofB64);

  /* Send client-final */
  b64 = crispy_base64_encode(finalMsg, (long)strlen(finalMsg), &b64Len);
  if (!b64) return -1;

  code = send_cmd(session, b64);
  free(b64);

  return (code == success_code || code == 0) ? 0 : (code > 0 ? code : -1);
}

/* SMTP SCRAM-SHA-256 wrappers */
static int smtp_scram_send(void *s, const char *line) {
  return crispy_smtp_command((SmtpSession *)s, line);
}
static int smtp_scram_reply(void *s, char *buf, size_t sz) {
  SmtpSession *ss = (SmtpSession *)s;
  snprintf(buf, sz, "%s", ss->last_reply);
  return 0;
}

int crispy_smtp_auth_scram_sha256(SmtpSession *s, const char *user,
                                   const char *pass) {
  int err = scram_sha256_exchange(smtp_scram_send, smtp_scram_reply,
                                  s, user, pass, 235);
  if (!err) s->authenticated = true;
  return err;
}

/* POP3 SCRAM-SHA-256 wrappers */
static int pop3_scram_send(void *s, const char *line) {
  return crispy_pop3_command((Pop3Session *)s, line);
}
static int pop3_scram_reply(void *s, char *buf, size_t sz) {
  Pop3Session *ps = (Pop3Session *)s;
  /* Extract base64 from "+OK <data>" or "+ <data>" */
  char *p = ps->last_reply;
  if (strncmp(p, "+OK ", 4) == 0) p += 4;
  else if (strncmp(p, "+ ", 2) == 0) p += 2;
  snprintf(buf, sz, "%s", p);
  return 0;
}

int crispy_pop3_auth_scram_sha256(Pop3Session *s, const char *user,
                                   const char *pass) {
  int err = scram_sha256_exchange(pop3_scram_send, pop3_scram_reply,
                                  s, user, pass, 0);
  if (!err) s->authenticated = true;
  return err;
}

/* ================================================================
 * DIGEST-MD5 (RFC 2831) — deprecated but some servers still use it
 * ================================================================ */

int crispy_smtp_auth_digest_md5(SmtpSession *s, const char *user,
                                 const char *pass) {
  /* Step 1: initiate */
  int code = crispy_smtp_command(s, "AUTH DIGEST-MD5");
  if (code != 334) return code > 0 ? code : -1;

  /* Step 2: decode challenge */
  long chalLen;
  char *challenge = crispy_base64_decode(s->last_reply,
      (long)strlen(s->last_reply), &chalLen);
  if (!challenge) return -1;

  /* Parse challenge: realm, nonce, qop, charset, algorithm */
  char *realm = NULL, *nonce = NULL, *qop = NULL;
  {
    char *chalCopy = strdup(challenge);
    char *tok = strtok(chalCopy, ",");
    while (tok) {
      while (*tok == ' ') tok++;
      if (strncmp(tok, "realm=", 6) == 0) {
        realm = tok + 6;
        if (*realm == '"') { realm++; char *q = strchr(realm, '"'); if (q) *q = '\0'; }
      } else if (strncmp(tok, "nonce=", 6) == 0) {
        nonce = tok + 6;
        if (*nonce == '"') { nonce++; char *q = strchr(nonce, '"'); if (q) *q = '\0'; }
      } else if (strncmp(tok, "qop=", 4) == 0) {
        qop = tok + 4;
        if (*qop == '"') { qop++; char *q = strchr(qop, '"'); if (q) *q = '\0'; }
      }
      tok = strtok(NULL, ",");
    }

    if (!nonce) { free(chalCopy); free(challenge); return -1; }
    if (!realm) realm = "";
    if (!qop) qop = "auth";

    /* Compute response:
     * HA1 = MD5(user:realm:pass)
     * HA2 = MD5("AUTHENTICATE:smtp/<realm>")
     * response = MD5(HA1_hex:nonce:00000001:cnonce:auth:HA2_hex) */
    char cnonce[33];
    crispy_md5_hex(user, strlen(user), cnonce);

    char a1[512];
    snprintf(a1, sizeof(a1), "%s:%s:%s", user, realm, pass);
    unsigned char ha1_raw[16];
    crispy_md5(a1, strlen(a1), ha1_raw);
    char ha1[33];
    for (int i = 0; i < 16; i++) { ha1[i*2] = "0123456789abcdef"[ha1_raw[i]>>4]; ha1[i*2+1] = "0123456789abcdef"[ha1_raw[i]&0xf]; }
    ha1[32] = '\0';

    char digestUri[256];
    snprintf(digestUri, sizeof(digestUri), "smtp/%s", realm);

    char a2[512];
    snprintf(a2, sizeof(a2), "AUTHENTICATE:%s", digestUri);
    char ha2[33];
    crispy_md5_hex(a2, strlen(a2), ha2);

    char respInput[1024];
    snprintf(respInput, sizeof(respInput), "%s:%s:00000001:%s:auth:%s",
             ha1, nonce, cnonce, ha2);
    char respHash[33];
    crispy_md5_hex(respInput, strlen(respInput), respHash);

    /* Build response string */
    char resp[2048];
    snprintf(resp, sizeof(resp),
             "username=\"%s\",realm=\"%s\",nonce=\"%s\",cnonce=\"%s\","
             "nc=00000001,qop=auth,digest-uri=\"%s\",response=%s",
             user, realm, nonce, cnonce, digestUri, respHash);

    free(chalCopy);
    free(challenge);

    /* Base64 encode and send */
    long b64Len;
    char *b64 = crispy_base64_encode(resp, (long)strlen(resp), &b64Len);
    if (!b64) return -1;

    code = crispy_smtp_command(s, b64);
    free(b64);
  }

  /* Step 3: server sends rspauth — we send empty response */
  if (code == 334) {
    code = crispy_smtp_command(s, "");
  }

  if (code == 235) s->authenticated = true;
  return SMTP_IS_OK(code) ? 0 : code;
}
