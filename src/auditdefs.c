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

#include <glib.h>
#include "auditdefs.h"

#define LOG_DOMAIN "eudora.audit"

int AuditShutdown(long faceTime, long rearTime, long connectTime, long totalTime)
{
    g_debug(LOG_DOMAIN ": shutdown face=%ld rear=%ld connect=%ld total=%ld",
            faceTime, rearTime, connectTime, totalTime);
    return 0;
}

int AuditTimestamp(long faceTime, long rearTime, long connectTime, long totalTime)
{
    g_debug(LOG_DOMAIN ": timestamp face=%ld rear=%ld connect=%ld total=%ld",
            faceTime, rearTime, connectTime, totalTime);
    return 0;
}

int AuditCheckStart(uLong sessionID, uLong personalityID, bool isAuto)
{
    g_debug(LOG_DOMAIN ": check-start session=%lu pers=%lu auto=%d",
            sessionID, personalityID, isAuto);
    return 0;
}

int AuditCheckDone(uLong sessionID, long messagesRcvd, long bytesRcvd)
{
    g_debug(LOG_DOMAIN ": check-done session=%lu msgs=%ld bytes=%ld",
            sessionID, messagesRcvd, bytesRcvd);
    return 0;
}

int AuditSendStart(uLong sessionID, uLong personalityID, bool isAuto)
{
    g_debug(LOG_DOMAIN ": send-start session=%lu pers=%lu auto=%d",
            sessionID, personalityID, isAuto);
    return 0;
}

int AuditSendDone(uLong sessionID, long messagesSent, long bytesSent)
{
    g_debug(LOG_DOMAIN ": send-done session=%lu msgs=%ld bytes=%ld",
            sessionID, messagesSent, bytesSent);
    return 0;
}

int AuditStartup(long platform, long version, long buildNumber)
{
    g_debug(LOG_DOMAIN ": startup platform=%ld version=%ld build=%ld",
            platform, version, buildNumber);
    return 0;
}

int AuditConnect(bool connectionUp)
{
    g_debug(LOG_DOMAIN ": connect up=%d", connectionUp);
    return 0;
}
