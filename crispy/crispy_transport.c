/* transport_posix.c — POSIX socket + OpenSSL transport for maillib
 * Standalone: only depends on POSIX sockets and OpenSSL.
 */

#include "crispy_transport.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define close_socket closesocket
  typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <netdb.h>
  #include <unistd.h>
  #define close_socket close
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/bn.h>

typedef struct {
  int sockfd;
  SSL_CTX *ssl_ctx;
  SSL *ssl;
  char hostname[256];
  /* Certificate callback */
  CrispyCertCallback cert_cb;
  void *cert_userdata;
  char *trusted_pem;  /* pre-trusted cert PEM for this host */
  /* Line-reading buffer */
  char readbuf[4096];
  int readpos;
  int readlen;
} PosixCtx;

static PosixCtx *ctx_new(void) {
  PosixCtx *c = (PosixCtx *)calloc(1, sizeof(PosixCtx));
  if (c) c->sockfd = -1;
  return c;
}

/* --- Get server cert as PEM string --- */

static char *get_peer_cert_pem(SSL *ssl) {
  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) return NULL;

  BIO *bio = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(bio, cert);

  char *data = NULL;
  long len = BIO_get_mem_data(bio, &data);
  char *pem = (char *)malloc(len + 1);
  if (pem) { memcpy(pem, data, len); pem[len] = '\0'; }

  BIO_free(bio);
  X509_free(cert);
  return pem;
}

/* --- SSL setup with certificate verification + callback --- */

static int setup_ssl(PosixCtx *c) {
  /* Step 1: Connect WITHOUT verification to get the server cert */
  c->ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (!c->ssl_ctx) return -1;
  SSL_CTX_set_min_proto_version(c->ssl_ctx, TLS1_2_VERSION);

  c->ssl = SSL_new(c->ssl_ctx);
  if (!c->ssl) return -1;
  SSL_set_fd(c->ssl, c->sockfd);
  SSL_set_tlsext_host_name(c->ssl, c->hostname);

  if (SSL_connect(c->ssl) <= 0) {
    unsigned long err = ERR_peek_last_error();
    fprintf(stderr, "SSL handshake failed: %s\n",
            ERR_reason_error_string(err));
    return -1;
  }

  /* Step 2: Get the server cert */
  char *pem = get_peer_cert_pem(c->ssl);

  /* Step 3: Verify the cert */
  /* Check against system CA store + trusted store */
  X509 *cert = SSL_get_peer_certificate(c->ssl);
  bool verified = false;

  if (cert) {
    /* Build a verification context with system CAs + trusted cert */
    X509_STORE *store = X509_STORE_new();
    X509_STORE_set_default_paths(store);

    if (c->trusted_pem) {
      BIO *tbio = BIO_new_mem_buf(c->trusted_pem, -1);
      if (tbio) {
        X509 *tcert = PEM_read_bio_X509(tbio, NULL, NULL, NULL);
        if (tcert) { X509_STORE_add_cert(store, tcert); X509_free(tcert); }
        BIO_free(tbio);
      }
    }

    X509_STORE_CTX *vctx = X509_STORE_CTX_new();
    X509_STORE_CTX_init(vctx, store, cert, NULL);
    verified = (X509_verify_cert(vctx) == 1);
    const char *errStr = verified ? NULL :
        X509_verify_cert_error_string(X509_STORE_CTX_get_error(vctx));

    X509_STORE_CTX_free(vctx);
    X509_STORE_free(store);
    X509_free(cert);

    /* Step 4: If not verified, prompt the user */
    if (!verified && c->cert_cb && pem) {
      verified = c->cert_cb(c->hostname,
                             errStr ? errStr : "certificate verify failed",
                             pem, c->cert_userdata);
    }
  }

  free(pem);

  if (!verified) {
    SSL_shutdown(c->ssl);
    SSL_free(c->ssl); c->ssl = NULL;
    SSL_CTX_free(c->ssl_ctx); c->ssl_ctx = NULL;
    return -1;
  }

  return 0;
}

/* --- Transport callbacks --- */

static int posix_connect(void *vctx, const char *host, int port) {
  PosixCtx *c = (PosixCtx *)vctx;

  struct addrinfo hints, *res, *rp;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  char portstr[16];
  snprintf(portstr, sizeof(portstr), "%d", port);

  int err = getaddrinfo(host, portstr, &hints, &res);
  if (err) return -1;

  for (rp = res; rp; rp = rp->ai_next) {
    c->sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (c->sockfd < 0) continue;
    if (connect(c->sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;
    close_socket(c->sockfd);
    c->sockfd = -1;
  }
  freeaddrinfo(res);

  if (c->sockfd < 0) return -1;

  /* Save hostname for SNI and cert verification */
  snprintf(c->hostname, sizeof(c->hostname), "%s", host);

  /* If port is 465 or 993 or 995, do immediate TLS */
  if (port == 465 || port == 993 || port == 995) {
    int err = setup_ssl(c);
    if (err) return err;
  }

  return 0;
}

static int posix_send(void *vctx, const char *data, long len) {
  PosixCtx *c = (PosixCtx *)vctx;
  const char *p = data;
  long remaining = len;

  while (remaining > 0) {
    long sent;
    if (c->ssl) {
      sent = SSL_write(c->ssl, p, (int)remaining);
      if (sent <= 0) return -1;
    } else {
      sent = send(c->sockfd, p, (size_t)remaining, 0);
      if (sent <= 0) return -1;
    }
    p += sent;
    remaining -= sent;
  }
  return 0;
}

/* Buffered line reader — reads until \n */
static int posix_recv_line(void *vctx, char *buf, long bufSize,
                           long *bytesRead) {
  PosixCtx *c = (PosixCtx *)vctx;
  long out = 0;

  while (out < bufSize - 1) {
    /* Refill internal buffer if empty */
    if (c->readpos >= c->readlen) {
      int n;
      if (c->ssl) {
        n = SSL_read(c->ssl, c->readbuf, sizeof(c->readbuf));
      } else {
        n = (int)recv(c->sockfd, c->readbuf, sizeof(c->readbuf), 0);
      }
      if (n <= 0) {
        if (out > 0) break; /* return what we have */
        return -1;
      }
      c->readpos = 0;
      c->readlen = n;
    }

    char ch = c->readbuf[c->readpos++];
    buf[out++] = ch;
    if (ch == '\n') break;
  }

  buf[out] = '\0';
  *bytesRead = out;
  return 0;
}

static int posix_start_tls(void *vctx) {
  PosixCtx *c = (PosixCtx *)vctx;

  if (c->ssl) return 0; /* already TLS */

  int err = setup_ssl(c);
  if (err) return err;

  /* Clear read buffer — TLS has its own framing */
  c->readpos = 0;
  c->readlen = 0;

  return 0;
}

static void posix_close(void *vctx) {
  PosixCtx *c = (PosixCtx *)vctx;
  if (c->ssl) {
    SSL_shutdown(c->ssl);
  }
  if (c->sockfd >= 0) {
    close_socket(c->sockfd);
    c->sockfd = -1;
  }
}

static void posix_destroy(void *vctx) {
  PosixCtx *c = (PosixCtx *)vctx;
  if (!c) return;
  if (c->ssl) SSL_free(c->ssl);
  if (c->ssl_ctx) SSL_CTX_free(c->ssl_ctx);
  free(c->trusted_pem);
  free(c);
}

static int posix_get_fd(void *vctx) {
  PosixCtx *c = (PosixCtx *)vctx;
  return c ? c->sockfd : -1;
}

/* --- Public --- */

SmtpTransport crispy_transport_new(void) {
  PosixCtx *c = ctx_new();
  SmtpTransport tp = {
    .ctx       = c,
    .connect   = posix_connect,
    .send      = posix_send,
    .recv_line = posix_recv_line,
    .start_tls = posix_start_tls,
    .close     = posix_close,
    .destroy   = posix_destroy,
    .get_fd    = posix_get_fd,
  };
  return tp;
}

void crispy_transport_set_cert_callback(SmtpTransport *tp,
                                        CrispyCertCallback cb,
                                        void *userdata) {
  PosixCtx *c = (PosixCtx *)tp->ctx;
  if (c) {
    c->cert_cb = cb;
    c->cert_userdata = userdata;
  }
}

void crispy_transport_load_trusted(SmtpTransport *tp, char *pem) {
  PosixCtx *c = (PosixCtx *)tp->ctx;
  if (c) {
    free(c->trusted_pem);
    c->trusted_pem = pem; /* takes ownership */
  }
}

/* --- Certificate store --- */

static char *cert_path(const char *certDir, const char *host,
                       char *buf, size_t bufSize) {
  snprintf(buf, bufSize, "%s/%s.pem", certDir, host);
  return buf;
}

static char *read_file_str(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *data = (char *)malloc(len + 1);
  if (data) { long rd = (long)fread(data, 1, len, f); data[rd] = '\0'; }
  fclose(f);
  return data;
}

int crispy_cert_store_save(const char *certDir, const char *host,
                           const char *cert_pem) {
  if (!certDir || !host || !cert_pem) return -1;
  char path[512];
  cert_path(certDir, host, path, sizeof(path));
  FILE *f = fopen(path, "w");
  if (!f) return -1;
  fputs(cert_pem, f);
  fclose(f);
  return 0;
}

char *crispy_cert_store_load(const char *certDir, const char *host) {
  if (!certDir || !host) return NULL;
  char path[512];
  cert_path(certDir, host, path, sizeof(path));
  return read_file_str(path);
}

int crispy_cert_store_delete(const char *certDir, const char *host) {
  if (!certDir || !host) return -1;
  char path[512];
  cert_path(certDir, host, path, sizeof(path));
  return remove(path);
}

char **crispy_cert_store_list(const char *certDir, int *count) {
  if (count) *count = 0;
  if (!certDir) return NULL;

  /* Open directory and find *.pem files */
  char **list = (char **)calloc(128, sizeof(char *));
  if (!list) return NULL;
  int n = 0;

#ifdef _WIN32
  /* TODO: Windows directory listing */
#else
  #include <dirent.h>
  DIR *dir = opendir(certDir);
  if (!dir) { free(list); return NULL; }

  struct dirent *ent;
  while ((ent = readdir(dir)) && n < 127) {
    size_t len = strlen(ent->d_name);
    if (len > 4 && strcmp(ent->d_name + len - 4, ".pem") == 0) {
      /* Strip .pem to get hostname */
      list[n] = strndup(ent->d_name, len - 4);
      n++;
    }
  }
  closedir(dir);
#endif

  list[n] = NULL;
  if (count) *count = n;
  return list;
}

/* Helper: extract readable name fields from X509_NAME */
static int format_x509_name(X509_NAME *name, char *buf, size_t bufSize) {
  int pos = 0;
  struct { int nid; const char *label; } fields[] = {
    { NID_commonName, "CN" },
    { NID_organizationName, "O" },
    { NID_organizationalUnitName, "OU" },
    { NID_localityName, "L" },
    { NID_stateOrProvinceName, "ST" },
    { NID_countryName, "C" },
    { 0, NULL }
  };

  for (int i = 0; fields[i].label; i++) {
    char val[256] = "";
    int idx = X509_NAME_get_text_by_NID(name, fields[i].nid, val, sizeof(val));
    if (idx >= 0 && val[0]) {
      if (pos > 0) pos += snprintf(buf + pos, bufSize - pos, "\n");
      pos += snprintf(buf + pos, bufSize - pos, "  %-4s %s", fields[i].label, val);
    }
  }
  return pos;
}

char *crispy_cert_info(const char *cert_pem) {
  if (!cert_pem) return NULL;

  BIO *bio = BIO_new_mem_buf(cert_pem, -1);
  if (!bio) return NULL;

  X509 *cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
  BIO_free(bio);
  if (!cert) return strdup("(invalid certificate)");

  char *info = (char *)malloc(4096);
  if (!info) { X509_free(cert); return NULL; }
  int pos = 0;
  int sz = 4096;

  /* Subject */
  pos += snprintf(info + pos, sz - pos, "Subject:\n");
  pos += format_x509_name(X509_get_subject_name(cert), info + pos, sz - pos);
  pos += snprintf(info + pos, sz - pos, "\n\n");

  /* Issuer */
  pos += snprintf(info + pos, sz - pos, "Issuer:\n");
  pos += format_x509_name(X509_get_issuer_name(cert), info + pos, sz - pos);
  pos += snprintf(info + pos, sz - pos, "\n\n");

  /* Subject Alternative Names */
  GENERAL_NAMES *sans = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
  if (sans) {
    pos += snprintf(info + pos, sz - pos, "Alt Names:\n");
    for (int i = 0; i < sk_GENERAL_NAME_num(sans); i++) {
      GENERAL_NAME *gen = sk_GENERAL_NAME_value(sans, i);
      if (gen->type == GEN_DNS) {
        unsigned char *dns = NULL;
        ASN1_STRING_to_UTF8(&dns, gen->d.dNSName);
        if (dns) {
          pos += snprintf(info + pos, sz - pos, "  DNS  %s\n", dns);
          OPENSSL_free(dns);
        }
      } else if (gen->type == GEN_IPADD) {
        /* IP address */
        ASN1_OCTET_STRING *ip = gen->d.iPAddress;
        if (ip->length == 4) {
          pos += snprintf(info + pos, sz - pos, "  IP   %d.%d.%d.%d\n",
                          ip->data[0], ip->data[1], ip->data[2], ip->data[3]);
        }
      }
    }
    GENERAL_NAMES_free(sans);
    pos += snprintf(info + pos, sz - pos, "\n");
  }

  /* Validity */
  BIO *tbio = BIO_new(BIO_s_mem());
  if (tbio) {
    char timebuf[64] = "";
    ASN1_TIME_print(tbio, X509_get0_notBefore(cert));
    BIO_read(tbio, timebuf, sizeof(timebuf) - 1);
    pos += snprintf(info + pos, sz - pos, "Valid From:  %s\n", timebuf);

    BIO_reset(tbio);
    memset(timebuf, 0, sizeof(timebuf));
    ASN1_TIME_print(tbio, X509_get0_notAfter(cert));
    BIO_read(tbio, timebuf, sizeof(timebuf) - 1);
    pos += snprintf(info + pos, sz - pos, "Valid Until: %s\n\n", timebuf);
    BIO_free(tbio);
  }

  /* Serial number */
  ASN1_INTEGER *serial = X509_get_serialNumber(cert);
  if (serial) {
    BIGNUM *bn = ASN1_INTEGER_to_BN(serial, NULL);
    if (bn) {
      char *hex = BN_bn2hex(bn);
      if (hex) {
        pos += snprintf(info + pos, sz - pos, "Serial:  %s\n", hex);
        OPENSSL_free(hex);
      }
      BN_free(bn);
    }
  }

  /* SHA-256 fingerprint — wrapped at 24 bytes per line */
  unsigned char fp[32];
  unsigned int fpLen = sizeof(fp);
  X509_digest(cert, EVP_sha256(), fp, &fpLen);
  pos += snprintf(info + pos, sz - pos, "\nSHA-256 Fingerprint:\n  ");
  for (unsigned int i = 0; i < fpLen; i++) {
    pos += snprintf(info + pos, sz - pos, "%s%02X", i ? ":" : "", fp[i]);
    if (i == 15) /* wrap after 16 bytes */
      pos += snprintf(info + pos, sz - pos, "\n  ");
  }
  pos += snprintf(info + pos, sz - pos, "\n");

  X509_free(cert);
  return info;
}
