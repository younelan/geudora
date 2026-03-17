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

/************************************************************************
 * Download a URL using libcurl (GTK port)
 * Originally used Mac OpenTransport API
 ************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <string.h>
#include <stdio.h>
#include "downloadurl.h"
#include "buildversion.h"

// Thread parameter struct
typedef struct
{
	char url[1024];
	FSSpec destSpec;
	FILE *destFile;
	long refCon;
	void (*FinishFunc)(long,int,DownloadInfo*);
	pthread_t threadID;
	HTTPinfo HTTPstuff;
	bool aborted;
	bool completedDownload;
	int error;
	char checksum[256];
} URLParms, *URLParmsPtr, *URLParmsHandle;

// Active downloads list
static URLParmsHandle activeDownloads[32] = {0};
static pthread_mutex_t downloadsMutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static void *DownloadURLThread(void *threadParameter);
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userp);

/************************************************************************
 * HeaderCallback - libcurl callback for parsing HTTP headers
 ************************************************************************/
static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userp)
{
	URLParmsHandle threadData = (URLParmsHandle)userp;
	size_t numbytes = size * nitems;
	
	// Look for "Checksum:" header
	if (numbytes > 10 && strncasecmp(buffer, "Checksum:", 9) == 0) {
		char *value = buffer + 9;
		// Skip whitespace
		while (*value == ' ' || *value == '\t') value++;
		// Copy checksum value, removing trailing CR/LF
		size_t len = 0;
		while (len < 255 && value[len] && value[len] != '\r' && value[len] != '\n') {
			threadData->checksum[len] = value[len];
			len++;
		}
		threadData->checksum[len] = '\0';
	}
	
	return numbytes;
}

/************************************************************************
 * WriteCallback - libcurl callback for writing downloaded data
 ************************************************************************/
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	URLParmsHandle threadData = (URLParmsHandle)userp;
	
	if (threadData->aborted)
		return 0;
	
	return fwrite(contents, size, nmemb, threadData->destFile);
}

/************************************************************************
 * DownloadURLThread - thread function for downloading URL
 ************************************************************************/
static void *DownloadURLThread(void *threadParameter)
{
	URLParmsHandle threadData = (URLParmsHandle)threadParameter;
	CURL *curl;
	CURLcode res;
	DownloadInfo info;
	struct curl_slist *headers = NULL;
	char headerBuf[256];
	
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, threadData->url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, threadData);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, threadData);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Eudora/6.2.4 (GTK)");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		
		// Add custom headers if specified
		if (threadData->HTTPstuff.sContentType) {
			snprintf(headerBuf, sizeof(headerBuf), "ContentType: %c", threadData->HTTPstuff.sContentType);
			headers = curl_slist_append(headers, headerBuf);
		}
		if (threadData->HTTPstuff.sMessageType) {
			snprintf(headerBuf, sizeof(headerBuf), "MessageType: %c", threadData->HTTPstuff.sMessageType);
			headers = curl_slist_append(headers, headerBuf);
		}
		if (threadData->HTTPstuff.sCheckSum) {
			snprintf(headerBuf, sizeof(headerBuf), "Checksum: %c", threadData->HTTPstuff.sCheckSum);
			headers = curl_slist_append(headers, headerBuf);
		}
		if (headers) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}
		
		// POST support
		if (threadData->HTTPstuff.post && threadData->HTTPstuff.hRequestData) {
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, threadData->HTTPstuff.hRequestData);
		}
		
		res = curl_easy_perform(curl);
		threadData->error = (res == CURLE_OK) ? 0 : -1;
		
		if (headers) {
			curl_slist_free_all(headers);
		}
		curl_easy_cleanup(curl);
	} else {
		threadData->error = -1;
	}
	
	fclose(threadData->destFile);
	threadData->completedDownload = true;
	
	// Call finish callback
	if (threadData->FinishFunc) {
		g_strlcpy(info.spec, threadData->destSpec, sizeof(info.spec));
		// Use extracted checksum if available, otherwise 0
		info.checksum = threadData->checksum[0] ? atoi(threadData->checksum) : 0;
		threadData->FinishFunc(threadData->refCon, threadData->error, &info);
	}
	
	// Unregister download
	pthread_mutex_lock(&downloadsMutex);
	for (int i = 0; i < 32; i++) {
		if (activeDownloads[i] == threadData) {
			activeDownloads[i] = NULL;
			break;
		}
	}
	pthread_mutex_unlock(&downloadsMutex);
	
	// Cleanup
	free(threadData);
	
	return NULL;
}

/************************************************************************
 * DownloadURL - download file specified by URL, supports HTTP/HTTPS
 ************************************************************************/
int DownloadURL(const char *urlString, char *destSpec,long refCon,void (*FinishFunc)(long,int,DownloadInfo*),long *pReference,HTTPinfo *pHTTPstuff)
{
	URLParmsHandle threadData;
	FILE *destFile;
	
	// Open destination file
	destFile = fopen((const char *)spec_name(destSpec), "wb");
	if (!destFile) {
		return -1;
	}
	
	// Allocate thread data
	threadData = (URLParmsHandle)calloc(1, sizeof(URLParms));
	if (!threadData) {
		fclose(destFile);
		return -1;
	}
	
	// Setup thread data
	strncpy(threadData->url, urlString, sizeof(threadData->url) - 1);
	g_strlcpy(threadData->destSpec, destSpec, sizeof(threadData->destSpec));
	threadData->destFile = destFile;
	threadData->refCon = refCon;
	threadData->FinishFunc = FinishFunc;
	if (pHTTPstuff)
		threadData->HTTPstuff = *pHTTPstuff;
	
	// Register download
	pthread_mutex_lock(&downloadsMutex);
	for (int i = 0; i < 32; i++) {
		if (!activeDownloads[i]) {
			activeDownloads[i] = threadData;
			break;
		}
	}
	pthread_mutex_unlock(&downloadsMutex);
	
	// Start download thread
	if (pthread_create(&threadData->threadID, NULL, DownloadURLThread, threadData) != 0) {
		fclose(destFile);
		free(threadData);
		return -1;
	}
	
	pthread_detach(threadData->threadID);
	*pReference = (long)threadData;
	return 0;
}

/************************************************************************
 * URLDownloadAbort - abort a download in progress
 ************************************************************************/
void URLDownloadAbort(long urlRef)
{
	URLParmsHandle threadData = (URLParmsHandle)urlRef;
	if (threadData) {
		threadData->aborted = true;
	}
}

/************************************************************************
 * DownloadURLOK - check if download functionality is available
 ************************************************************************/
bool DownloadURLOK(void)
{
	return true;  // libcurl should always be available
}
