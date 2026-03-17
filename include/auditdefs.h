#ifndef AUDITDEFS_H
#define AUDITDEFS_H

#include "mydefs.h"

int AuditShutdown(long faceTime, long rearTime, long connectTime, long totalTime);
int AuditTimestamp(long faceTime, long rearTime, long connectTime, long totalTime);
int AuditCheckStart(uLong sessionID, uLong personalityID, bool isAuto);
int AuditCheckDone(uLong sessionID, long messagesRcvd, long bytesRcvd);
int AuditSendStart(uLong sessionID, uLong personalityID, bool isAuto);
int AuditSendDone(uLong sessionID, long messagesSent, long bytesSent);
int AuditStartup(long platform, long version, long buildNumber);
int AuditConnect(bool connectionUp);

#endif /* AUDITDEFS_H */
