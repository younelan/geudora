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

#ifndef HEADER_H
#define HEADER_H

#include <stdbool.h>
#include "mydefs.h"
#include "mailbox.h"
#include "trans.h"
#include "StrnDefs.h"

/*
 * states of the header converter
 */
typedef enum
{
	ExpectHeaderName,		/* next token shd be a header name */
	ExpectColon,				/* next token shd be a colon */
	ExpectText,					/* we're looking for an unstructured field */
	ExpectType,					/* content type */
	ExpectSlash,				/* separator */
	ExpectSubType,			/* content subtype */
	ExpectSem,					/* expecting semicolon (or end) */
	ExpectAttribute,		/* expecting attribute name */
	ExpectEqual,				/* expecting equal sign */
	ExpectValue,				/* expecting attribute value */
	ExpectEnco,					/* expecting encoding */
	ExpectVersion,			/* expecting MIME version number */
	ExpectDisposition,		/* expecting content disposition */
	ExpectStructuredValue		/* expecting a structured field value */
} HeaderStateEnum;


/*
 * structure that describes a header
 */
/* AssocArray may be defined in util.h; avoid duplicate typedefs */
#ifndef AAS_TYPEDEF_DONE
typedef struct AssocArray AssocArray, *AAPtr, *AAHandle;
#define AAS_TYPEDEF_DONE
#endif

typedef struct HeaderDesc
{
	HeaderStateEnum state;	/* state of header converter */
	InterestHeadEnum hFound;/* header we're working on now */
	char contentType[32];			/* MIME content type */
	char contentSubType[32];		/* MIME content subtype */
	char contentEnco[32];			/* MIME content type */
	char status[16];						/* status header */
	char subj[64];							/* subject */
	char who[64];							/* sender */
	char attributeName[32];		/* name of attribute being collected */
	char mimeVersion[16];			/* mime version string */
	AAHandle contentAttributes;	/* attributes from the content-type header */
	AAHandle funFields;				/* fields we'd like to keep an eye on */
	Accumulator fullHeaders;// all the headers for this message
	emsMIMEHandle tlMIME;		// for translation api
	long diskStart;					/* where header starts on disk */
	long diskEnd;						/* where header ends on disk */
	bool grokked;				/* did we find understand it all? */
	bool isMIME;					/* is MIME */
	bool hasRich;				/* has richtext */
	bool hasHTML;				/* has richtext */
	bool hasFlow;				/* has format=flow */
	bool hasCharset;				/* has a non-ASCII charset */
	bool hasMDN;				/* has MDN request */
	short xlateResID;				/* resource id of translit table to use for display */
	bool eightBit;				/* quoted-printable cte? */
	bool isPartial;			/* is this a MIME-partial? */
	bool isAttach;				/* is the content-disposition attachment? */
	bool foundRecvd;			/* do we have Recvd already? */
	bool foundMID;				/* do we have Message-id already? */
	uLong msgIdHash;				/* hash of same */
	bool relatedPart;		/* subpart of multipart/related */
	uLong mhtmlID;			/* part number of the mhtml part */
	uLong relURLHash;			/* hash of relative url for content-location */
	uLong absURLHash;			/* hash of absolute url for content-location */
	uLong cidHash;	/* hash of content-id */
	uLong gmtSecs;
	char summaryInfo[128];		// message summary information
	short depth;					// depth of MIME structure
} HeaderDesc, *HeaderDPtr, *HeaderDHandle;

int ReadHeader(TransStream stream,HeaderDHandle hdh, long estSize, short refN, bool isDigest);
int ReadHeaderLo(TransStream stream,HeaderDHandle hdh, long estSize, short refN, bool isDigest, short funFieldsID, short funFieldsLimit);
HeaderDHandle NewHeaderDesc(HeaderDHandle parentHDH);
void DisposeHeaderDesc(HeaderDHandle hdh);
short ViewTable(HeaderDHandle hdh);
int ParseAHeader(char * h, HeaderDHandle *hdhp);
int ParseAHeaderLo(char * h, HeaderDHandle *hdhp, short funFieldsID, short funFieldsLimit);
#define ZapHeaderDesc(hdh) do{DisposeHeaderDesc(hdh);hdh=nil;}while(0)
bool HeaderDescInAddrBook(HeaderDHandle hdh);

#endif
