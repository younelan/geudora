/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef TCP_H
#define TCP_H

#include "TransStream.h" /* TransStream, TransVector */
#include "mydefs.h"      /* TransVector and other Eudora types */
#include "portable-compat.h"

/* POSIX networking */
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * declarations for dealing with tcp streams
 ************************************************************************/

enum { NUM_ALT_ADDRS = 4 };

/* DNS result structure - replaces Mac OT InetHostInfo */
typedef char InetDomainName[256];
typedef uint32_t InetHost; /* IPv4 address in network byte order */
typedef uint16_t InetPort;

typedef struct InetHostInfo {
  char name[256];
  InetHost addrs[NUM_ALT_ADDRS];
} InetHostInfo;

/* MX record - simplified, used by OTGetDomainMX */
typedef struct InetMailExchange {
  uint16_t preference;
  char exchange[256];
} InetMailExchange;

struct hostInfo {
  long rtnCode;
  char cname[255];
  int8_t filler; /* Filler for proper byte alignment */
  unsigned long addr[NUM_ALT_ADDRS];
};
typedef struct hostInfo hostInfo;

typedef struct HIQ HostInfoQ, *HostInfoQPtr;
struct HIQ {
  HostInfoQPtr next;
  struct hostInfo hi;
  short done;
  bool inUse;
};

void TcpFastFlush(bool destroy);

int GetTCPStatus(TransStream stream, void *pb);
int GetHostByAddr(struct hostInfo *hi, long addr);
int GetHostByName(const char *name, struct hostInfo **hostInfoPtr);
int GetMyHostid(uint32_t *myAddr, uint32_t *myMask);
bool SplitPort(char *host, long *port);

#define NUM_MX 10 /* The number of MX records to look up for a host */

/* Error codes for TCP operations */
enum {
  errOTInitFailed = 700, /* TCP init failed */
  errOTInetSvcs,         /* failed to open internet services */
  errDNR,                /* DNS lookup error */
  errNoMXRecords,        /* no MX records found */
  errCreateStream,       /* failed to create stream struct */
  errOpenStream,         /* failed to establish connection */
  errLostConnection,     /* connection lost */
  errMiscRec,            /* receive error */
  errMiscSend,           /* send error */
  errPPPConnect,         /* dial-up connect error (stubbed/unused) */
  errPPPPrefNotFound,    /* PPP pref not found (stubbed/unused) */
  errPPPStateUnknown,    /* PPP state unknown (stubbed/unused) */
  errOTMissingLib,       /* missing library (stubbed/unused) */
  errMyLastOTErr
};

/* Connection method enum (PPP/SLIP/Other) - kept for callers */
enum { kPPPSelected, kMacSLIPSelected, kOtherSelected };

/* PPP state enum - kept for callers, always kPPPUp */
enum {
  kPPPLoaded = 0,
  kPPPNotLoaded,
  kPPPDown,
  kPPPClosing,
  kPPPUp,
  kPPPOpening
};

/* Per-connection POSIX TCP stream structure - replaces OT MyOTTCPStream */
typedef struct MTS MyOTTCPStream, *MyOTTCPStreamPtr;
struct MTS {
  int sockfd; /* POSIX socket fd, -1 if not open */

  bool weAreClosing;    /* true while shutting down gracefully */
  bool otherSideClosed; /* remote end closed */
  bool ourSideClosed;   /* we have called shutdown(SHUT_WR) */
  bool releaseMe;       /* safe to close()/free() */

  unsigned char dummyBuffer[256]; /* drain buffer during close */

  MyOTTCPStreamPtr next; /* orderly-close queue link */
  MyOTTCPStreamPtr prev;

  long age; /* TickCount() when queued for close */
};

/* PPP info stub - kept because globals reference it */
typedef struct MyOTPPPInfoStruct {
  bool weConnectedPPP;
  short state;
} MyOTPPPInfoStruct, *MyOTPPPInfoPtr;

/* Functions that were OT-specific but keep the same external name */
void OTInitOpenTransport(void);
void OTCleanUpAfterOpenTransport(void);

int OTTCPConnectTrans(TransStream stream, const char *serverName,
                         long port, bool silently, uint32_t timeout);
int OTTCPSendTrans(TransStream stream, const char *text, long size, ...);
int OTTCPRecvTrans(TransStream stream, char *line, long *size);
int OTTCPDisTrans(TransStream stream);
int OTTCPDestroyTrans(TransStream stream);
int OTTCPTransError(TransStream stream);
void OTTCPSilenceTrans(TransStream stream, bool silence);
char *OTTCPWhoAmI(TransStream stream, char *who);

void KillDeadMyTStreams(bool destroy);
void OTFlushInput(TransStream stream, uint32_t timeout);

/* DNS helpers */
int OTGetHostByName(InetDomainName hostName, InetHostInfo *hostInfoPtr);
int OTGetHostByAddr(InetHost addr, InetHostInfo *domainNamePtr);
InetHost OTRandomAddr(InetHostInfo *host);
int OTGetDomainMX(InetDomainName hostName, InetMailExchange *MXPtr,
                    short *numMX);
void GetPreferredMX(InetDomainName preferredName, InetMailExchange *MXPtr,
                    short numMX);

int DNSHostid(uint32_t *dnsAddr);
int OTMyHostid(uint32_t *myAddr, uint32_t *myMask);

/* Stubs for PPP/RAS/dial-up - always assume online */
int SelectedConnectionMode(unsigned long *connectionSelection,
                             bool forceRead);
bool NeedToUpdateTP(void);
bool AutoCheckOKWithDBRead(bool updatePers);
bool CanCheckPPPState(void);
bool CanChangePPPState(void);
bool PPPDown(void);
bool PPPIsMostDefinitelyUpAndRunning(void);
int OTConnectForLink(void);
bool UserIdle(uint32_t ticks);

void OTErrorToString(short specificError, char *tcpMessage);

TransVector GetTCPTrans(void);

int CheckConnectionSettings(const char *host, long port,
                               char *errorMessage);

#define ALMOST(x) (.95 * (x))

#endif /* TCP_H */
