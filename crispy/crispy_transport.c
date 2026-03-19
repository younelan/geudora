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

typedef struct {
  int sockfd;
  SSL_CTX *ssl_ctx;
  SSL *ssl;
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

  /* If port is 465 or 993 or 995, do immediate TLS */
  if (port == 465 || port == 993 || port == 995) {
    c->ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (!c->ssl_ctx) return -1;
    c->ssl = SSL_new(c->ssl_ctx);
    if (!c->ssl) return -1;
    SSL_set_fd(c->ssl, c->sockfd);
    SSL_set_tlsext_host_name(c->ssl, host);
    if (SSL_connect(c->ssl) <= 0) return -1;
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

  c->ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (!c->ssl_ctx) return -1;

  c->ssl = SSL_new(c->ssl_ctx);
  if (!c->ssl) return -1;

  SSL_set_fd(c->ssl, c->sockfd);
  if (SSL_connect(c->ssl) <= 0) return -1;

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
