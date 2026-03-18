/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
 * tcp.c - POSIX BSD socket implementation of Eudora's TCP layer.
 *
 * Replaces the original Mac Open Transport (OT) networking code.
 * All public function signatures are preserved so callers are unchanged.
 *
 * RAS/PPP/dial-up connection checks are STUBBED – we assume the host
 * is always online (no modem or dial-up involved).
 *
 * TransStream->sockfd is the POSIX file descriptor for each connection.
 * No OT intermediate structs (MyOTTCPStream, EndpointRef, etc.) are used.
 */

#include "tcp.h"
#include "Globals.h" /* PrefIsSet, SettingsRefN, etc. */
#include "MyRes.h" /* FNAME_STRN, OPEN_ERR_ALRT, BIG_OK_ALRT, TIMEOUT_ALRT, Caution */
#include "StringDefs.h" /* OPEN_ERR_ALRT, BIG_OK_ALRT, Caution, FNAME_STRN, etc. */
#include "StringUtil.h"  /* MyStringToNum, ComposeRString, etc. */
#include "gtk_dialogs.h" /* SetPref, AlertStr, ResetAlertStage */
#include "log.h"         /* ComposeLogS, LOG_PROTO, LOG_TRANS, etc. */
#include "mydefs.h"      /* NumToString, commandTimeout */
#include "threading.h"
#define FILE_NUM 37

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* ─── Globals ────────────────────────────────────────────────────────────────
 */

#define RECV_BUFFER_SIZE 4096

bool gUseOT = true; /* always true – kept for callers that guard on it */
bool OTinitialized = true;

/* PPP/RAS stub state – always pretend we are connected */
MyOTPPPInfoStruct MyOTPPPInfo = {false, kPPPUp};
bool needPPPConnection = false;
bool gUpdateTPWindow = false;
unsigned long connectionSelection = kOtherSelected;

/* Pending-close queue (streams waiting for the remote FIN).
 * Declared extern in Globals.h — not static here. */
MyOTTCPStreamPtr pendingCloses = NULL;

/* ─── Error reporting macros ─────────────────────────────────────────────────
 */
#define TcpTrouble(stream, which, err)                                         \
  TT(stream, which, err, FNAME_STRN + FILE_NUM, __LINE__)
#define OTTCPError(stream, general, specific)                                  \
  OTTE(stream, general, specific, FNAME_STRN + FILE_NUM, __LINE__)

/* commandTimeout: used as a stream error code when poll() times out */
#ifndef commandTimeout
#define commandTimeout (-1201)
#endif

/* ResetAlertStage: Mac Dialog Manager call — no-op on POSIX/GTK */
#ifndef ResetAlertStage
#define ResetAlertStage() ((void)0)
#endif

/* ─── Forward declarations ────────────────────────────────────────────────────
 */
int TT(TransStream stream, int which, int theErr, int file, int line);
int OTTE(TransStream stream, int general, int specific, short file,
           short line);
static void DestroyMyTStream(MyOTTCPStreamPtr myStream);
static void EnqueueMyTStream(MyOTTCPStreamPtr myStream);
static short WaitForChars(TransStream stream, long timeoutSecs,
                          char *line, long *size);
uint32_t RandomAddr(uint32_t *addrs);
void NoteAddrGoodness(struct hostInfo *hip, uint32_t addr, short err);

/* ─── Utility ─────────────────────────────────────────────────────────────────
 */

/* Convert a timeout in seconds to milliseconds for poll(). */
static int MillisToTimeout(long secs) {
  if (secs <= 0)
    return -1;
  if (secs > 3600)
    secs = 3600;
  return (int)(secs * 1000);
}

/* ─── RandomAddr / NoteAddrGoodness ───────────────────────────────────────────
 */

uint32_t RandomAddr(uint32_t *addrs) {
  short count;
  if (!PrefIsSet(PREF_DNS_BALANCE))
    return addrs[0];
  for (count = NUM_ALT_ADDRS; count; count--)
    if (addrs[count - 1])
      break;
  return addrs[count < 2 ? 0 : TickCount() % count];
}

void NoteAddrGoodness(struct hostInfo *hip, uint32_t addr, short err) {
  short count, i;
  if (!PrefIsSet(PREF_DNS_BALANCE))
    return;
  for (count = NUM_ALT_ADDRS; count; count--)
    if (hip->addr[count - 1] == addr)
      break;
  if (err) {
    for (i = count; i < NUM_ALT_ADDRS; i++)
      hip->addr[i - 1] = hip->addr[i];
    hip->addr[NUM_ALT_ADDRS - 1] = 0;
    if (count == 1)
      *hip->cname = 0;
  } else {
    memset(hip->addr, 0, sizeof(uint32_t) * NUM_ALT_ADDRS);
    hip->addr[0] = addr;
  }
}

/* ─── SplitPort
 * ──────────────────────────────────────────────────────────────── */
bool SplitPort(char *host, long *port) {
  char *colon;
  long localPort;

  if ((colon = strchr(host, ':')) != NULL) {
    *colon = 0;  /* null-terminate host at the colon */
    MyStringToNum(colon + 1, &localPort);
    assert(localPort > 0);
    if (localPort > 0 && port)
      *port = localPort;
    return true;
  }
  return false;
}

/* ─── TT (TcpTrouble)
 * ────────────────────────────────────────────────────────── */

int TT(TransStream stream, int which, int theErr, int file, int line) {
  if (stream == NULL ||
      (!stream->BeSilent &&
       (!CommandPeriod || (stream->Opening && !PrefIsSet(PREF_OFFLINE) &&
                           !PrefIsSet(PREF_NO_OFF_OFFER))))) {
    char message[256], tcpMessage[256], debugStr[256], rawNumber[256];
    short realRef = SettingsRefN;

    /* Format error number as C string for alert display */
    snprintf(rawNumber, sizeof(rawNumber), "%d", theErr);
    SettingsRefN = GetMainGlobalSettingsRefN();
    GetRString(message, which);
    OTErrorToString(theErr, tcpMessage);
    ComposeRString(debugStr, FILE_LINE_FMT, file, line);
    SettingsRefN = realRef;

    MyParamText(message, rawNumber, tcpMessage, debugStr);
    if (stream == NULL || stream->Opening) {
      if (2 == ReallyDoAnAlert(OPEN_ERR_ALRT, Caution))
        SetPref(PREF_OFFLINE, YesStr);
    } else {
      ReallyDoAnAlert(BIG_OK_ALRT, Caution);
    }
  }
  return stream ? (stream->streamErr = theErr) : theErr;
}

/* ─── OT init stubs
 * ──────────────────────────────────────────────────────────── */

void OTInitOpenTransport(void) {
  /* No-op on POSIX. Mark as initialised so callers proceed. */
  gUseOT = true;
  OTinitialized = true;
}

void OTCleanUpAfterOpenTransport(void) { /* No-op on POSIX. */ }

/* ─── OTTCPConnectTrans
 * ───────────────────────────────────────────────────────── */

int OTTCPConnectTrans(TransStream stream, const char *serverName,
                        long port, bool silently, uint32_t timeout) {
  char hostName[256];
  char portStr[16];
  struct addrinfo hints, *res = NULL, *rp;
  int sockfd = -1;
  int gaierr;
  int err = 0;
  char scratch[256];

  assert(stream);
  assert(stream->sockfd < 0); /* must not already have a socket */

  stream->streamErr = 0;
  stream->Opening = true;
  stream->BeSilent = (unsigned char)silently;

  /* serverName is already a C string */
  g_strlcpy(hostName, serverName, sizeof(hostName));
  snprintf(portStr, sizeof(portStr), "%ld", port);

  /* Progress feedback */
  GetRString(scratch, DNR_LOOKUP);
  strncat(scratch, serverName, sizeof(scratch) - strlen(scratch) - 1);
  ProgressMessage(kpSubTitle, scratch);

  /* DNS resolution */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  gaierr = getaddrinfo(hostName, portStr, &hints, &res);
  if (gaierr != 0) {
    ComposeLogS(LOG_PROTO, NULL, "DNS lookup of '%s' failed: %s", hostName,
                gai_strerror(gaierr));
    err = stream->streamErr = errDNR;
    if (!silently)
      OTTCPError(stream, errDNR, errDNR);
    goto done;
  }

  /* Log resolved addresses */
  if (LogLevel & LOG_PROTO) {
    ComposeLogS(LOG_PROTO, NULL, "DNS Lookup of \"%s\"", hostName);
    struct addrinfo *p;
    for (p = res; p; p = p->ai_next) {
      char ip[INET_ADDRSTRLEN];
      struct sockaddr_in *in4 = (struct sockaddr_in *)p->ai_addr;
      inet_ntop(AF_INET, &in4->sin_addr, ip, sizeof(ip));
      ComposeLogS(LOG_PROTO, NULL, "    %s", ip);
    }
  }

  ProgressMessageR(kpSubTitle, PREPARING_CONNECTION);

  /* Try each address until one connects */
  for (rp = res; rp; rp = rp->ai_next) {
    char ip[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)rp->ai_addr;
    inet_ntop(AF_INET, &in4->sin_addr, ip, sizeof(ip));
    ComposeLogS(LOG_PROTO, NULL, "Connecting to %s:%ld", ip, port);

    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd < 0)
      continue;

#ifdef SO_NOSIGPIPE
    {
      int on = 1;
      setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
    }
#endif

    /* Non-blocking connect with poll() timeout */
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
      break; /* immediate connect */
    }

    if (errno == EINPROGRESS) {
      struct pollfd pfd = {sockfd, POLLOUT, 0};
      int pret =
          poll(&pfd, 1, MillisToTimeout(timeout > 0 ? (long)timeout : 30));
      if (pret > 0 && (pfd.revents && POLLOUT)) {
        int soerr = 0;
        socklen_t solen = sizeof(soerr);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &soerr, &solen);
        if (soerr == 0)
          break; /* connected */
      }
    }

    close(sockfd);
    sockfd = -1;
    if (CommandPeriod) {
      err = stream->streamErr = userCancelled;
      goto done;
    }
  }
  freeaddrinfo(res);
  res = NULL;

  if (sockfd < 0) {
    ComposeLogS(LOG_PROTO, NULL, "Connection to %s:%ld failed", hostName,
                port);
    err = stream->streamErr = errOpenStream;
    if (!silently)
      OTTCPError(stream, errOpenStream, err);
    goto done;
  }

  /* Back to blocking mode */
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) & ~O_NONBLOCK);

  stream->sockfd = sockfd;
  stream->RcvSpot = -1;
  /* Allocate receive buffer for NetRecvLine if not already present */
  if (!stream->RcvBuffer) {
    stream->RcvBuffer = (char *)malloc(RECV_BUFFER_SIZE);
  }
  gActiveConnections++;

  {
    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    char ip[INET_ADDRSTRLEN];
    getpeername(sockfd, (struct sockaddr *)&peer, &plen);
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
    ComposeLogS(LOG_PROTO, NULL, "Connected to %s:%ld", ip, port);
  }

done:
  if (res)
    freeaddrinfo(res);
  stream->Opening = false;
  return stream->streamErr;
}

/* ─── OTTCPSendTrans
 * ─────────────────────────────────────────────────────────── */

int OTTCPSendTrans(TransStream stream, const char *text, long size, ...) {
  va_list extra;
  const char *buf;
  long remaining;

  assert(stream);
  assert(stream->sockfd >= 0);

  stream->streamErr = 0;
  if (CommandPeriod)
    return userCancelled;
  if (size == 0)
    return 0;

  va_start(extra, size);
  buf = text;
  remaining = size;

  do {
    while (remaining > 0) {
      ssize_t sent;
      if (CommandPeriod) {
        stream->streamErr = userCancelled;
        goto done;
      }

#ifdef MSG_NOSIGNAL
      sent = send(stream->sockfd, buf, (size_t)remaining, MSG_NOSIGNAL);
#else
      sent = send(stream->sockfd, buf, (size_t)remaining, 0);
#endif

      if (sent > 0) {
        if (LogLevel && LOG_TRANS && !stream->streamErr)
          CarefulLog(LOG_TRANS, LOG_SENT, buf, (long)sent);
        CycleBalls();
        remaining -= (long)sent;
        buf += sent;
        if (IsSendAudit(stream))
          stream->bytesTransferred += sent;
      } else if (sent == 0) {
        stream->streamErr = errLostConnection;
        break;
      } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          struct pollfd pfd = {stream->sockfd, POLLOUT, 0};
          if (poll(&pfd, 1, 60000) <= 0) {
            stream->streamErr = commandTimeout;
            break;
          }
          /* retry on next iteration */
        } else if (errno == EPIPE || errno == ECONNRESET) {
          stream->streamErr = errLostConnection;
          break;
        } else {
          stream->streamErr = errMiscSend;
          break;
        }
      }
    }

    if (!stream->streamErr) {
      buf = va_arg(extra, const char *);
      if (buf)
        remaining = va_arg(extra, long);
    }
  } while (!stream->streamErr && buf);

done:
  va_end(extra);
  if (stream->streamErr && stream->streamErr != commandTimeout &&
      stream->streamErr != userCancelled) {
    TcpTrouble(stream, stream->streamErr, stream->streamErr);
  }
  return stream->streamErr;
}

/* ─── OTTCPRecvTrans
 * ─────────────────────────────────────────────────────────── */

int OTTCPRecvTrans(TransStream stream, char *line, long *size) {
  char scratch[32];
  long timeout =
      InAThread() ? GetRLong(THREAD_RECV_TIMEOUT) : GetRLong(RECV_TIMEOUT);

  assert(stream);
  assert(stream->sockfd >= 0);

  stream->streamErr = 0;

  do {
    stream->streamErr = WaitForChars(stream, timeout, line, size);
  } while (stream->streamErr == commandTimeout &&
           (AlertStr(TIMEOUT_ALRT, Caution,
                     GetRString(scratch, InAThread() ? THREAD_RECV_TIMEOUT
                                                     : RECV_TIMEOUT)),
            false) /* AlertStr is void; never retry — break out on timeout */);

  if (stream->streamErr == 0) {
    if (*size)
      ResetAlertStage();
    if (*size && (LogLevel && LOG_TRANS))
      CarefulLog(LOG_TRANS, LOG_GOT, line, *size);
    if (*size && IsRecAudit(stream))
      stream->bytesTransferred += *size;
  } else if (stream->streamErr != commandTimeout &&
             stream->streamErr != userCancelled) {
    TcpTrouble(stream, stream->streamErr, stream->streamErr);
  }
  return stream->streamErr;
}

/* ─── WaitForChars (internal)
 * ────────────────────────────────────────────────── */

static short WaitForChars(TransStream stream, long timeoutSecs,
                          char *line, long *size) {
  struct pollfd pfd;

  if (CommandPeriod)
    return userCancelled;

  pfd.fd = stream->sockfd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  {
    int pret = poll(&pfd, 1, MillisToTimeout(timeoutSecs));
    if (CommandPeriod)
      return userCancelled;
    if (pret == 0)
      return commandTimeout;
    if (pret < 0)
      return errMiscRec;
  }

  if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
    return errLostConnection;

  if (pfd.revents & POLLIN) {
    ssize_t got = recv(stream->sockfd, line, (size_t)*size, 0);
    if (got > 0) {
      *size = (long)got;
      return 0;
    }
    if (got == 0)
      return errLostConnection;
    return errMiscRec;
  }
  return errMiscRec;
}

/* ─── OTTCPDisTrans
 * ──────────────────────────────────────────────────────────── */

int OTTCPDisTrans(TransStream stream) {
  if (!stream || stream->sockfd < 0)
    return 0;
  stream->streamErr = 0;
  /* Send FIN; keep socket open so we can still receive the remote's FIN */
  shutdown(stream->sockfd, SHUT_WR);
  return stream->streamErr;
}

/* ─── OTTCPDestroyTrans
 * ──────────────────────────────────────────────────────── */

int OTTCPDestroyTrans(TransStream stream) {
  if (!stream)
    return 0;

  if (stream->sockfd >= 0) {
    close(stream->sockfd);
    stream->sockfd = -1;
    if (gActiveConnections)
      gActiveConnections--;
  }
  if (stream->RcvBuffer) {
    free(stream->RcvBuffer);
    stream->RcvBuffer = NULL;
  }
  return stream->streamErr;
}

/* ─── OTTCPTransError / OTTCPSilenceTrans
 * ────────────────────────────────────── */

int OTTCPTransError(TransStream stream) {
  assert(stream);
  return stream->streamErr;
}

void OTTCPSilenceTrans(TransStream stream, bool silence) {
  assert(stream);
  stream->BeSilent = silence;
}

/* ─── OTTCPWhoAmI ─────────────────────────────────────────────────────────────
 * ────────────────────────────────────────────────────────────────── */

char *OTTCPWhoAmI(TransStream stream, char *who) {
  char hostname[256];
  size_t len;
  (void)stream;

  if (!*MyHostname) {
    struct addrinfo hints, *res = NULL;
    if (gethostname(hostname, sizeof(hostname)) != 0)
      return who;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if (getaddrinfo(hostname, NULL, &hints, &res) == 0 && res) {
      if (res->ai_canonname)
        strncpy(hostname, res->ai_canonname, sizeof(hostname) - 1);
      freeaddrinfo(res);
    }

    ComposeRString(who, TCP_ME, hostname);
    SetPref(PREF_LASTHOST, who);
    strcpy(MyHostname, who);
  }
  strcpy(who, MyHostname);

  len = strlen(who);
  if (len > 0 && who[len - 1] == '.')
    who[len - 1] = '\0';
  return who;
}

/* ─── DNSHostid
 * ──────────────────────────────────────────────────────────────── */

int DNSHostid(uint32_t *dnsAddr) {
  *dnsAddr = 0;
#if defined(__APPLE__) || defined(__linux__)
  {
    extern int res_init(void);
    /* _res is provided by resolv.h on both platforms */
#include <resolv.h>
    if (res_init() == 0 && _res.nscount > 0) {
      *dnsAddr = (uint32_t)_res.nsaddr_list[0].sin_addr.s_addr;
    }
  }
#endif
  return 0;
}

/* ─── OTMyHostid / GetMyHostid
 * ───────────────────────────────────────────────── */

int OTMyHostid(uint32_t *myAddr, uint32_t *myMask) {
  struct ifaddrs *ifap = NULL, *ifa;
  *myAddr = 0;
  *myMask = 0;
  if (getifaddrs(&ifap) != 0)
    return memFullErr;
  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    struct sockaddr_in *sa, *nm;
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
      continue;
    sa = (struct sockaddr_in *)ifa->ifa_addr;
    if (ntohl(sa->sin_addr.s_addr) == INADDR_LOOPBACK)
      continue;
    *myAddr = (uint32_t)sa->sin_addr.s_addr;
    if (ifa->ifa_netmask) {
      nm = (struct sockaddr_in *)ifa->ifa_netmask;
      *myMask = (uint32_t)nm->sin_addr.s_addr;
    }
    break;
  }
  freeifaddrs(ifap);
  return 0;
}

int GetMyHostid(uint32_t *addr, uint32_t *mask) {
  return OTMyHostid(addr, mask);
}

/* ─── OTGetHostByName
 * ────────────────────────────────────────────────────────── */

int OTGetHostByName(InetDomainName hostName, InetHostInfo *hostInfoPtr) {
  struct addrinfo hints, *res = NULL, *rp;
  int rc, count = 0;

  memset(hostInfoPtr, 0, sizeof(*hostInfoPtr));
  strncpy(spec_name(hostInfoPtr), hostName, sizeof(spec_name(hostInfoPtr)) - 1);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  rc = getaddrinfo(hostName, NULL, &hints, &res);
  if (rc != 0) {
    if (LogLevel & LOG_PROTO) {
      ComposeLogS(LOG_PROTO, NULL, "DNS Lookup of \"%s\" failed: %s", hostName,
                  gai_strerror(rc));
    }
    return errDNR;
  }

  for (rp = res; rp && count < NUM_ALT_ADDRS; rp = rp->ai_next) {
    struct sockaddr_in *in4 = (struct sockaddr_in *)rp->ai_addr;
    hostInfoPtr->addrs[count++] = (InetHost)in4->sin_addr.s_addr;
  }
  freeaddrinfo(res);

  if (LogLevel & LOG_PROTO) {
    int i;
    ComposeLogS(LOG_PROTO, NULL, "DNS Lookup of \"%s\"", hostName);
    for (i = 0; i < count; i++) {
      char ip[INET_ADDRSTRLEN];
      struct in_addr ia;
      ia.s_addr = hostInfoPtr->addrs[i];
      inet_ntop(AF_INET, &ia, ip, sizeof(ip));
      ComposeLogS(LOG_PROTO, NULL, "    %s (%d)", ip, i + 1);
    }
  }
  return 0;
}

/* ─── OTGetHostByAddr
 * ────────────────────────────────────────────────────────── */

int OTGetHostByAddr(InetHost addr, InetHostInfo *domainNamePtr) {
  struct sockaddr_in sa;
  char host[NI_MAXHOST];
  size_t len;

  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = (in_addr_t)addr;

  if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host), NULL,
                  0, NI_NAMEREQD) != 0)
    return errDNR;

  len = strlen(host);
  if (len > 0 && host[len - 1] == '.')
    host[len - 1] = '\0';

  strncpy(spec_name(domainNamePtr), host, sizeof(spec_name(domainNamePtr)) - 1);
  domainNamePtr->addrs[0] = addr;
  return 0;
}

/* ─── OTRandomAddr
 * ───────────────────────────────────────────────────────────── */

InetHost OTRandomAddr(InetHostInfo *host) {
  short count;
  if (!PrefIsSet(PREF_DNS_BALANCE))
    return host->addrs[0];
  for (count = NUM_ALT_ADDRS; count; count--)
    if (host->addrs[count - 1])
      break;
  return host->addrs[count < 2 ? 0 : TickCount() % count];
}

/* ─── OTGetDomainMX
 * ──────────────────────────────────────────────────────────── */

/*
 * MX lookup stub – Eudora falls back gracefully to a direct connection
 * when no MX records are returned, so we simply return errNoMXRecords.
 */
int OTGetDomainMX(InetDomainName hostName, InetMailExchange *MXPtr,
                    short *numMX) {
  (void)hostName;
  (void)MXPtr;
  (void)numMX;
  return errNoMXRecords;
}

/* ─── GetPreferredMX
 * ─────────────────────────────────────────────────────────── */

void GetPreferredMX(InetDomainName preferredName, InetMailExchange *MXPtr,
                    short numMX) {
  short lowestPref, preferredHost;
  short count = numMX - 1;
  size_t len;

  preferredHost = count;
  lowestPref = MXPtr[count].preference;
  for (; count >= 0; count--) {
    if (MXPtr[count].preference < lowestPref) {
      lowestPref = MXPtr[count].preference;
      preferredHost = count;
    }
  }
  strncpy(preferredName, MXPtr[preferredHost].exchange,
          sizeof(InetDomainName) - 1);
  len = strlen(preferredName);
  if (len > 0 && preferredName[len - 1] == '.')
    preferredName[len - 1] = '\0';
}

/* ─── OTFlushInput
 * ───────────────────────────────────────────────────────────── */

void OTFlushInput(TransStream stream, uint32_t timeout) {
  char junk[256];
  long got;
  if (!stream || stream->sockfd < 0)
    return;
  do {
    got = sizeof(junk);
    if (WaitForChars(stream, (long)timeout, junk, &got))
      break;
    if (LogLevel && LOG_TRANS && !stream->streamErr && got > 0)
      CarefulLog(LOG_TRANS, LOG_FLUSHED, junk, got);
  } while (got > 0);
}

/* ─── Error messages table
 * ───────────────────────────────────────────────────── */

static short errorMessages[errMyLastOTErr - errOTInitFailed] = {
    OT_INIT_ERR,           /* errOTInitFailed   */
    OT_INIT_ERR,           /* errOTInetSvcs     */
    BIND_ERR,              /* errDNR            */
    0,                     /* errNoMXRecords    */
    TCP_TROUBLE,           /* errCreateStream   */
    NO_SMTP_SERVER,        /* errOpenStream     */
    TCP_TROUBLE,           /* errLostConnection */
    TCP_TROUBLE,           /* errMiscRec        */
    TCP_TROUBLE,           /* errMiscSend       */
    OT_DIALUP_CONNECT_ERR, /* errPPPConnect     */
    0,                     /* errPPPPrefNotFound */
    0,                     /* errPPPStateUnknown */
    OT_MISSING_LIBRARY,    /* errOTMissingLib   */
};

static void OTTELo(int generalError, int specificError, char *message) {
  (void)specificError;
  if (generalError >= errOTInitFailed && generalError < errMyLastOTErr)
    GetRString(message, errorMessages[generalError - errOTInitFailed]);
  else
    GetRString(message, TCP_TROUBLE);
}

int OTTE(TransStream stream, int generalError, int specificError,
           short file, short line) {
  if (generalError != 0 && (stream == NULL || !stream->BeSilent) &&
      !AmQuitting &&
      (!CommandPeriod || stream == NULL ||
       (stream->Opening && !PrefIsSet(PREF_OFFLINE) &&
        !PrefIsSet(PREF_NO_OFF_OFFER)))) {
    char message[256], tcpMessage[256], debugStr[256], rawNumber[256];
    short realRef = SettingsRefN;

    tcpMessage[0] = 0;
    snprintf(rawNumber, sizeof(rawNumber), "%d", specificError);
    SettingsRefN = GetMainGlobalSettingsRefN();
    OTErrorToString(specificError, tcpMessage);
    OTTELo(generalError, specificError, message);
    ComposeRString(debugStr, FILE_LINE_FMT, file, line);
    SettingsRefN = realRef;

    MyParamText(message, rawNumber, tcpMessage, debugStr);
    if (stream == NULL || stream->Opening) {
      if (2 == ReallyDoAnAlert(OPEN_ERR_ALRT, Caution))
        SetPref(PREF_OFFLINE, YesStr);
    } else {
      ReallyDoAnAlert(BIG_OK_ALRT, Caution);
    }
  }
  return generalError;
}

/* ─── OTErrorToString
 * ────────────────────────────────────────────────────────── */

void OTErrorToString(short specificError, char *tcpMessage) {
  tcpMessage[0] = 0;

  if (specificError >= errOTInitFailed && specificError < errMyLastOTErr) {
    switch (specificError) {
    case errOTInetSvcs:
      GetRString(tcpMessage, OT_INET_SVCS_ERR);
      break;
    case errLostConnection:
      GetRString(tcpMessage, TCP_TROUBLE);
      break;
    case errPPPStateUnknown:
      GetRString(tcpMessage, OT_PPP_STATE_ERR);
      break;
    case errPPPPrefNotFound:
      GetRString(tcpMessage, OT_TCPIP_PREF_ERR);
      break;
    case errOTMissingLib:
      GetRString(tcpMessage, OT_MISSING_LIBRARY);
      break;
    default:
      GetRString(tcpMessage, OT_UNKNOWN_ERR);
      break;
    }
  } else if (specificError < 0) {
    /* Map POSIX errno (stored as negative) to a readable string */
    const char *msg = strerror(-specificError);
    if (!msg)
      msg = "Unknown network error";
    strcpy(tcpMessage, msg);
  }
}

/* ─── Pending-close queue helpers
 * ────────────────────────────────────────────── */

/*
 * MyOTTCPStream / pendingCloses: we keep the queue structure for orderly
 * disconnect tracking (streams where we sent FIN but haven't seen the remote
 * FIN yet).  Each node just holds a socket fd + age for timeout.
 */

static void DestroyMyTStream(MyOTTCPStreamPtr myStream) {
  if (!myStream)
    return;
  if (myStream->sockfd >= 0) {
    close(myStream->sockfd);
    myStream->sockfd = -1;
  }
  if (gActiveConnections)
    gActiveConnections--;
}

static void EnqueueMyTStream(MyOTTCPStreamPtr myStream) {
  MyOTTCPStreamPtr scan;
  assert(myStream);
  myStream->next = NULL;
  myStream->prev = NULL;
  if (!pendingCloses) {
    pendingCloses = myStream;
    return;
  }
  for (scan = pendingCloses; scan->next; scan = scan->next)
    ;
  scan->next = myStream;
  myStream->prev = scan;
}

void KillDeadMyTStreams(bool destroy) {
  MyOTTCPStreamPtr scan = pendingCloses, kill;

  while (scan) {
    if (scan->sockfd >= 0) {
      struct pollfd pfd = {scan->sockfd, POLLIN, 0};
      if (poll(&pfd, 1, 0) > 0) {
        ssize_t got =
            recv(scan->sockfd, scan->dummyBuffer, sizeof(scan->dummyBuffer), 0);
        if (got <= 0) {
          scan->otherSideClosed = true;
          scan->releaseMe = true;
        } else {
          scan->age = TickCount();
        }
      } else if (pfd.revents & (POLLHUP | POLLERR)) {
        scan->otherSideClosed = true;
        scan->releaseMe = true;
      }
    }
    if ((TickCount() - scan->age) > 3600)
      scan->releaseMe = true;

    if (destroy || scan->releaseMe) {
      kill = scan;
      scan = scan->next;
      if (kill == pendingCloses)
        pendingCloses = kill->next;
      if (kill->prev)
        kill->prev->next = kill->next;
      if (kill->next)
        kill->next->prev = kill->prev;
      DestroyMyTStream(kill);
      free(kill);
    } else {
      scan = scan->next;
    }
  }
}

/* ─── OTVerifyOpen (RAS/PPP stub)
 * ────────────────────────────────────────────── */

int OTVerifyOpen(TransStream stream) {
  /* Stub: no dial-up/PPP/RAS — assume always connected. */
  if (stream)
    stream->streamErr = 0;
  needPPPConnection = false;
  MyOTPPPInfo.state = kPPPUp;
  MyOTPPPInfo.weConnectedPPP = false;
  return 0;
}

/* ─── GetHostByAddr / GetHostByName
 * ──────────────────────────────────────────── */

int GetHostByAddr(struct hostInfo *hostInfoPtr, long addr) {
  /* Detect private/NAT ranges and return a literal string */
  if ((addr & 0xff000000) == 0x0A000000 || (addr & 0xfff00000) == 0xAC100000 ||
      (addr & 0xffff0000) == 0xC0A80000) {
    char literal[32];
    ComposeRString(literal, NAT_FMT, addr);
    strcpy(hostInfoPtr->cname, literal);
    hostInfoPtr->addr[0] = (unsigned long)addr;
    hostInfoPtr->rtnCode = 0;
    return 0;
  }

  {
    InetHostInfo domainName;
    short count;
    int err = OTGetHostByAddr((InetHost)addr, &domainName);
    if (err == 0) {
      strcpy(hostInfoPtr->cname, domainName.name);
      for (count = 0; count < NUM_ALT_ADDRS; count++)
        hostInfoPtr->addr[count] = domainName.addrs[count];
      hostInfoPtr->rtnCode = 0;
    }
    return err;
  }
}

int GetHostByName(const char *name, struct hostInfo **hostInfoPtr) {
  static struct hostInfo trickCaller;
  InetHostInfo domainName;
  InetDomainName hostName;
  short count;
  int err;

  strcpy(hostName, name);
  err = OTGetHostByName(hostName, &domainName);
  if (err == 0) {
    *hostInfoPtr = &trickCaller;
    strcpy(trickCaller.cname, domainName.name);
    for (count = 0; count < NUM_ALT_ADDRS; count++)
      trickCaller.addr[count] = domainName.addrs[count];
    trickCaller.rtnCode = 0;
  }
  return err;
}

/* ─── PPP/RAS stubs
 * ──────────────────────────────────────────────────────────── */

int SelectedConnectionMode(unsigned long *sel, bool forceRead) {
  (void)forceRead;
  if (sel)
    *sel = kOtherSelected;
  return 0;
}

bool CanCheckPPPState(void) { return false; }
bool CanChangePPPState(void) { return false; }
bool PPPDown(void) { return false; }
bool PPPIsMostDefinitelyUpAndRunning(void) { return true; }
int OTConnectForLink(void) { return 0; }
bool UserIdle(uint32_t ticks) {
  (void)ticks;
  return false;
}

/* ─── Connection method monitoring ────────────────────────────────────────────
 */

void UpdateCachedConnectionMethodInfo(void) {
  /* No-op – always a direct POSIX socket connection */
}

bool NeedToUpdateTP(void) {
  bool u = gUpdateTPWindow;
  gUpdateTPWindow = false;
  return u;
}

bool AutoCheckOKWithDBRead(bool updatePers) {
  (void)updatePers;
  return AutoCheckOK();
}

/* ─── TcpFastFlush
 * ───────────────────────────────────────────────────────────── */

void TcpFastFlush(bool destroy) {
  static bool flushing = false;
  if (flushing)
    return;
  flushing = true;
  KillDeadMyTStreams(destroy);
  flushing = false;
}

/* ─── NetRecvLine — line-buffered receive using RcvBuffer
 * Reads from the network via RecvTrans, buffers data, and returns
 * one line at a time (terminated by \r from \n in the stream).
 * Ported from Mac ph.c, adapted for direct pointer (no Handle).
 * ──────────────────────────────────────────────────────────────── */

int NetRecvLine(TransStream stream, char *line, long *size) {
  long bSize = *size;
  char *anchor, *end;
  char c;

  if (!stream->RcvBuffer)
    return (CommandPeriod ? userCancelled : memFullErr);

  *size = 0;
  anchor = line;
  end = line + bSize - 1;

  while (anchor < end) {
    if (stream->RcvSpot >= 0) {
      /* There are buffered chars — scan for newline */
      char *rPtr = stream->RcvBuffer + stream->RcvSpot;
      for (c = *rPtr++; anchor < end; c = *rPtr++) {
        if (c && c != '\015') {
          *anchor++ = c;
          if (c == '\012') {
            anchor[-1] = '\015'; /* normalize LF → CR */
            break;
          }
        }
      }
      if (c != '\012')
        rPtr--; /* back up — didn't find newline */
      stream->RcvSpot = (int)(rPtr - stream->RcvBuffer);
      if (stream->RcvSpot > stream->RcvSize)
        anchor--; /* newline was sentinel */
      if (stream->RcvSpot >= stream->RcvSize)
        stream->RcvSpot = -1; /* buffer exhausted */
      if ((anchor > line && anchor[-1] == '\015') || anchor >= end) {
        *size = anchor - line;
        *anchor = 0;
        return 0;
      }
    } else {
      /* Need more data from network */
      long count = RECV_BUFFER_SIZE - 1;
      int err = OTTCPRecvTrans(stream, stream->RcvBuffer, &count);
      if (count > 0) {
        stream->RcvBuffer[count] = '\012'; /* sentinel */
        stream->RcvSize = (int)count;
        stream->RcvSpot = 0;
      }
      if (err) {
        *size = anchor - line;
        line[*size] = 0;
        return err;
      }
    }
  }
  *size = anchor - line;
  *anchor = 0;
  return 0;
}

/* ─── OTTCPTrans — TransVector for POSIX TCP transport
 * Order: vConnectTrans, vSendTrans, vRecvTrans, vDisTrans, vDestroyTrans,
 *        vTransError, vSilenceTrans, vSendWDS, vWhoAmI, vRecvLine,
 *        vAsyncSendTrans
 * ──────────────────────────────────────────────────────────────── */

TransVector OTTCPTrans = {
    (int (*)(TransStream, const char *, long, bool, unsigned long))OTTCPConnectTrans,
    (int (*)(TransStream, const char *, long, ...))OTTCPSendTrans,
    OTTCPRecvTrans,
    OTTCPDisTrans,     OTTCPDestroyTrans, OTTCPTransError,
    (void (*)(TransStream, bool))OTTCPSilenceTrans,
    NULL,              OTTCPWhoAmI,
    NetRecvLine,       NULL
};

/* ─── GetTCPTrans
 * ────────────────────────────────────────────────────────────── */

TransVector GetTCPTrans(void) {
  TransVector theTrans = OTTCPTrans;
#ifdef ESSL
  extern TransVector ESSLSetupVector(TransVector);
  return ESSLSetupVector(theTrans);
#else
  return theTrans;
#endif
}

/* ─── CheckConnectionSettings
 * ────────────────────────────────────────────────── */

int CheckConnectionSettings(const char *host, long port,
                               char *errorMessage) {
  int err = 0;
  TransStream stream = NULL;
  bool oldPref = PrefIsSet(PREF_IGNORE_PPP);

  if (0 == (err = NewTransStream(&stream))) {
    SetPref(PREF_IGNORE_PPP, YesStr);
    err = ConnectTrans(stream, (char *)host, port, true, GetRLong(SHORT_OPEN_TIMEOUT));
    SetPref(PREF_IGNORE_PPP, oldPref ? YesStr : NoStr);

    if (0 != err && errorMessage != NULL) {
      short realRef = SettingsRefN;
      SettingsRefN = GetMainGlobalSettingsRefN();
      OTTELo(errOpenStream, stream->streamErr, errorMessage);
      SettingsRefN = realRef;
    }

    if (0 == err)
      DestroyTrans(stream);
    ZapTransStream(&stream);
  }
  return err;
}
