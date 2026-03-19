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
  c->ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (!c->ssl_ctx) return -1;

  /* Minimum TLS 1.2 */
  SSL_CTX_set_min_proto_version(c->ssl_ctx, TLS1_2_VERSION);

  /* Load system CA certificates */
  SSL_CTX_set_default_verify_paths(c->ssl_ctx);

  /* If we have a pre-trusted cert for this host, load it */
  if (c->trusted_pem) {
    BIO *bio = BIO_new_mem_buf(c->trusted_pem, -1);
    if (bio) {
      X509 *trusted = PEM_read_bio_X509(bio, NULL, NULL, NULL);
      if (trusted) {
        X509_STORE *store = SSL_CTX_get_cert_store(c->ssl_ctx);
        X509_STORE_add_cert(store, trusted);
        X509_free(trusted);
      }
      BIO_free(bio);
    }
  }

  /* Enable verification */
  SSL_CTX_set_verify(c->ssl_ctx, SSL_VERIFY_PEER, NULL);

  c->ssl = SSL_new(c->ssl_ctx);
  if (!c->ssl) return -1;

  SSL_set_fd(c->ssl, c->sockfd);
  SSL_set_tlsext_host_name(c->ssl, c->hostname);
  SSL_set1_host(c->ssl, c->hostname);

  if (SSL_connect(c->ssl) <= 0) {
    unsigned long err = ERR_peek_last_error();
    const char *reason = ERR_reason_error_string(err);

    /* Handshake failed — try the callback if we have one */
    if (c->cert_cb) {
      char *pem = get_peer_cert_pem(c->ssl);
      bool accepted = c->cert_cb(c->hostname,
                                  reason ? reason : "handshake failed",
                                  pem, c->cert_userdata);
      free(pem);
      if (accepted) {
        /* User accepted — reconnect without strict verification */
        SSL_free(c->ssl);
        SSL_CTX_free(c->ssl_ctx);
        c->ssl = NULL;
        c->ssl_ctx = SSL_CTX_new(TLS_client_method());
        SSL_CTX_set_min_proto_version(c->ssl_ctx, TLS1_2_VERSION);
        c->ssl = SSL_new(c->ssl_ctx);
        SSL_set_fd(c->ssl, c->sockfd);
        SSL_set_tlsext_host_name(c->ssl, c->hostname);
        if (SSL_connect(c->ssl) <= 0) return -1;
        return 0;
      }
    }
    fprintf(stderr, "SSL: %s\n", reason ? reason : "handshake failed");
    return -1;
  }

  /* Verify cert post-handshake */
  long verify_result = SSL_get_verify_result(c->ssl);
  if (verify_result != X509_V_OK) {
    const char *errStr = X509_verify_cert_error_string(verify_result);

    if (c->cert_cb) {
      char *pem = get_peer_cert_pem(c->ssl);
      bool accepted = c->cert_cb(c->hostname, errStr, pem, c->cert_userdata);
      free(pem);
      if (accepted) return 0; /* user accepted */
    }

    fprintf(stderr, "SSL cert: %s\n", errStr);
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

char *crispy_cert_info(const char *cert_pem) {
  if (!cert_pem) return NULL;

  BIO *bio = BIO_new_mem_buf(cert_pem, -1);
  if (!bio) return NULL;

  X509 *cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
  BIO_free(bio);
  if (!cert) return strdup("(invalid certificate)");

  /* Extract info */
  char *info = (char *)malloc(2048);
  if (!info) { X509_free(cert); return NULL; }
  int pos = 0;

  /* Subject */
  char subj[256] = "";
  X509_NAME_oneline(X509_get_subject_name(cert), subj, sizeof(subj));
  pos += snprintf(info + pos, 2048 - pos, "Subject: %s\n", subj);

  /* Issuer */
  char issuer[256] = "";
  X509_NAME_oneline(X509_get_issuer_name(cert), issuer, sizeof(issuer));
  pos += snprintf(info + pos, 2048 - pos, "Issuer:  %s\n", issuer);

  /* Validity */
  BIO *tbio = BIO_new(BIO_s_mem());
  if (tbio) {
    ASN1_TIME_print(tbio, X509_get0_notBefore(cert));
    char timebuf[64] = "";
    BIO_read(tbio, timebuf, sizeof(timebuf) - 1);
    pos += snprintf(info + pos, 2048 - pos, "Valid from: %s\n", timebuf);

    BIO_reset(tbio);
    ASN1_TIME_print(tbio, X509_get0_notAfter(cert));
    memset(timebuf, 0, sizeof(timebuf));
    BIO_read(tbio, timebuf, sizeof(timebuf) - 1);
    pos += snprintf(info + pos, 2048 - pos, "Valid to:   %s\n", timebuf);
    BIO_free(tbio);
  }

  /* SHA-256 fingerprint */
  unsigned char fp[32];
  unsigned int fpLen = sizeof(fp);
  X509_digest(cert, EVP_sha256(), fp, &fpLen);
  pos += snprintf(info + pos, 2048 - pos, "SHA-256:    ");
  for (unsigned int i = 0; i < fpLen; i++)
    pos += snprintf(info + pos, 2048 - pos, "%s%02X", i ? ":" : "", fp[i]);
  pos += snprintf(info + pos, 2048 - pos, "\n");

  X509_free(cert);
  return info;
}
