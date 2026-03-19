/* Copyright (c) 2017, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

#include "TransStream.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdio.h>
#define FILE_NUM 100
// Copyright � 1997 by QUALCOMM Incorporated
/**********************************************************************
 * code to support multiple simultaneous network connections
 **********************************************************************/
 

/**********************************************************************
 * NewTransStream - allocate memory for a new TransStream
 **********************************************************************/
 
int NewTransStream(TransStream *newStream)
{
	int err = 0;

	if (!newStream) return EINVAL;

	*newStream = (TransStream) calloc(1, sizeof(TransStreamStruct));
	if (!*newStream) {
		err = errno ? errno : 1;
		*newStream = NULL;
		return err;
	}

	/* initialize socket to invalid */
	(*newStream)->sockfd = -1;

	return err;
}

/**********************************************************************
 * ZapTransStream - clean up and destroy a transstream.
 **********************************************************************/
void ZapTransStream(TransStream *theStream)
{
	if (!theStream || !*theStream) return;

	TransStream ts = *theStream;

	/* close socket if open */
	if (ts->sockfd >= 0) {
		close(ts->sockfd);
		ts->sockfd = -1;
	}

	/* free receive buffer */
	if (ts->RcvBuffer) {
		free(ts->RcvBuffer);
		ts->RcvBuffer = NULL;
	}

	free(ts);
	*theStream = NULL;
}

/**********************************************************************
 * StartStreamAudit - start auditing a network stream
 **********************************************************************/
void StartStreamAudit(TransStream theStream, StreamAuditTypeEnum what)
{
	if (theStream)
	{
		theStream->auditType = what;
		theStream->bytesTransferred = 0;
	}
}

/**********************************************************************
 * StopStreamAudit - stop auditing a stream
 **********************************************************************/
void StopStreamAudit(TransStream theStream)
{
	if (theStream)
	{
		theStream->auditType = kAuditNothing;
	}
}

/**********************************************************************
 * ReportStreamAudit - report audit results
 **********************************************************************/
long ReportStreamAudit(TransStream theStream)
{
	long byteMe = 0;
	
	if (theStream)
	{
		byteMe = theStream->bytesTransferred;
	}
	
	return (byteMe);
}

/*
 * ConnectTrans - resolve and connect to serverName:port with timeout (ms)
 * Returns 0 on success, errno-style error on failure.
 */
int ConnectTrans(TransStream stream, const char *serverName, long port, bool silently, unsigned long timeout)
{
	struct addrinfo hints, *res, *rp;
	char portstr[16];
	int sfd = -1;
	int err = 0;

	if (!serverName || !stream) return EINVAL;

	snprintf(stream->serverName, sizeof(stream->serverName), "%s", serverName);
	stream->port = (unsigned long)port;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snprintf(portstr, sizeof(portstr), "%ld", port);
	if ((err = getaddrinfo(serverName, portstr, &hints, &res)) != 0) {
		return EINVAL;
	}

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd < 0) continue;

		/* set non-blocking to implement timeout */
		int flags = fcntl(sfd, F_GETFL, 0);
		fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			/* connected immediately */
			fcntl(sfd, F_SETFL, flags); /* restore */
			break;
		}

		if (errno != EINPROGRESS) {
			close(sfd);
			sfd = -1;
			continue;
		}

		/* wait for socket writable or timeout */
		fd_set wfds;
		struct timeval tv;
		FD_ZERO(&wfds);
		FD_SET(sfd, &wfds);
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		int sel = select(sfd + 1, NULL, &wfds, NULL, &tv);
		if (sel > 0 && FD_ISSET(sfd, &wfds)) {
			int soerr = 0;
			socklen_t len = sizeof(soerr);
			if (getsockopt(sfd, SOL_SOCKET, SO_ERROR, &soerr, &len) < 0 || soerr != 0) {
				close(sfd);
				sfd = -1;
				continue;
			}
			/* success */
			fcntl(sfd, F_SETFL, flags);
			break;
		}

		/* timed out or error */
		close(sfd);
		sfd = -1;
	}

	freeaddrinfo(res);

	if (sfd < 0) return ETIMEDOUT;

	stream->sockfd = sfd;
	stream->Opening = 0;
	stream->DontWait = 0;
	stream->streamErr = 0;
	stream->RcvSpot = -1;
	/* Allocate receive buffer for NetRecvLine if not already present */
	if (!stream->RcvBuffer) {
		stream->RcvBuffer = (char *)malloc(4096);
	}

	return 0;
}

/*
 * SendTrans - variadic send helper. Accepts pairs (ptr, size) ending with a NULL pointer.
 * Returns 0 on success or errno on failure.
 */
int SendTrans(TransStream stream, const char *text, long size, ...)
{
	if (!stream) return EINVAL;
	if (stream->sockfd < 0) return ENOTCONN;

	va_list ap;
	const char *ptr = text;
	long len = size;
	ssize_t sent;
	int lastErr = 0;

	va_start(ap, size);
	while (ptr) {
		long toSend = len;
		const char *p = ptr;
		while (toSend > 0) {
			sent = send(stream->sockfd, p, (size_t)toSend, 0);
			if (sent < 0) {
				lastErr = errno;
				va_end(ap);
				return lastErr;
			}
			toSend -= sent;
			p += sent;
			stream->bytesTransferred += sent;
		}
		ptr = va_arg(ap, const char *);
		if (!ptr) break;
		len = va_arg(ap, long);
	}
	va_end(ap);
	return 0;
}

/*
 * RecvTrans - receive up to *size bytes into line. On success, *size is set to bytes received.
 * Returns 0 on success, errno on failure, or 1 on EOF.
 */
int RecvTrans(TransStream stream, char *line, long *size)
{
	if (!stream || !size || !line) return EINVAL;
	if (stream->sockfd < 0) return ENOTCONN;

	long want = *size;
	ssize_t r = recv(stream->sockfd, line, (size_t)want, 0);
	if (r < 0) return errno;
	if (r == 0) { *size = 0; return 1; }
	*size = (long)r;
	stream->bytesTransferred += r;
	return 0;
}