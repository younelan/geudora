#ifndef AUDITDEFS_H
#define AUDITDEFS_H

#include "mydefs.h"

OSErr AuditShutdown(long faceTime, long rearTime, long connectTime, long totalTime);
OSErr AuditTimestamp(long faceTime, long rearTime, long connectTime, long totalTime);
OSErr AuditCheckStart(uLong sessionID, uLong personalityID, bool isAuto);
OSErr AuditCheckDone(uLong sessionID, long messagesRcvd, long bytesRcvd);
OSErr AuditSendStart(uLong sessionID, uLong personalityID, bool isAuto);
OSErr AuditSendDone(uLong sessionID, long messagesSent, long bytesSent);
OSErr AuditStartup(long platform, long version, long buildNumber);
OSErr AuditConnect(bool connectionUp);

#endif /* AUDITDEFS_H */
