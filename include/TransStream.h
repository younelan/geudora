/* TransStream.h - POSIX/OpenSSL backed TransStream definition
   This header provides a concrete TransStream structure so networking
   functions can be rewritten function-by-function to use POSIX APIs.
*/

#ifndef TRANSSTREAM_H
#define TRANSSTREAM_H

/* Keep this header self-contained and avoid pulling in the large
   `tcp.h` header here so callers can include `TransStream.h` when
   porting functions incrementally. */

typedef enum {
    kAuditNothing = 0,
    kAuditBytesReceived,
    kAuditBytesSent
} StreamAuditTypeEnum;

/* Concrete POSIX/OpenSSL-backed TransStream structure. Use simple
   C types so callers can be ported without depending on Carbon/OT. */
typedef struct TransStreamStruct {
    int sockfd;                 /* POSIX socket descriptor, -1 when unused */
    void *ctx;                  /* OpenSSL ctx (NULL when unused) */
    void *ssl;                  /* OpenSSL ssl object (NULL when unused) */

    unsigned char *RcvBuffer;
    int RcvSize;
    int RcvSpot;
    
    long ESSLSetting; // Back-compat field

    unsigned char BeSilent;
    unsigned char Opening;
    unsigned char DontWait;

    int streamErr;

    unsigned long port;
    unsigned char serverName[256];
    unsigned char localHostName[256];

    StreamAuditTypeEnum auditType;
    long bytesTransferred;
} TransStreamStruct;

#ifndef TRANSSTREAM_PTR_DEFINED
typedef TransStreamStruct *TransStream;
#define TRANSSTREAM_PTR_DEFINED 1
#endif
typedef TransStreamStruct *TransStreamPtr;

#define IsSendAudit(s) ((s) && (s)->auditType == kAuditBytesSent)
#define IsRecAudit(s) ((s) && (s)->auditType == kAuditBytesReceived)

/* Core TransStream helpers */
int NewTransStream(TransStream *newStream);    /* create a TransStream */
void ZapTransStream(TransStream *theStream);   /* destroy a TransStream */

/* Basic POSIX-backed network operations (minimal implementations)
   Signatures match existing callers in the tree. 
   Note: These may be overridden by macros in mydefs.h for legacy compatibility */
#ifdef SendTrans
#undef SendTrans
#endif
#ifdef RecvTrans
#undef RecvTrans
#endif
int ConnectTrans(TransStream stream, unsigned char *serverName, long port, int silently, unsigned long timeout);
int SendTrans(TransStream stream, unsigned char *text, long size, ...);
int RecvTrans(TransStream stream, unsigned char *line, long *size);

void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what);
void StopStreamAudit(TransStream theStream);
long ReportStreamAudit(TransStream theStream);

#define ShouldUseSSL(ts) ((ts) && (ts)->ctx != NULL)

#endif /* TRANSSTREAM_H */