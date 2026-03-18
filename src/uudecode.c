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

#include "uudecode.h"
#include "fileutil.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include "legacy_shim.h"
#include "threading.h"
#include <sys/time.h>
#define FILE_NUM 46
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/* Major modifications Copyright (c)1991-1992, Apple Computer Inc. */
/* More modifications Copyright (c)1993, QUALCOMM Incorporated */
/************************************************************************
 * functions to convert files from uuencoded applesingle (yuck!)
 * Major modifications (c)1991-1992, Apple Computer Inc.
 * released to public domain.
 ************************************************************************/

#define SINGLE_MAGIC 0x00051600
#define DOUBLE_MAGIC 0x00051607
#define OLD_VERSION	0x00010000
#define MAP_NAME 3
#define MAP_RFORK 2
#define MAP_DFORK 1
#define MAP_DATES 8
#define MAP_INFO 9
#define NEW_VERSION 0x00020000

typedef struct
{
	unsigned long type;
	unsigned long offset;
	unsigned long length;
} Map, *MapPtr;

typedef struct
{
	unsigned long magic;
	unsigned long version;
	char homefs[16];
	uShort mapCount;
	Map maps[9];
} UUHeader;

typedef struct UUGlobals_ UUGlobals, *UUGlobalsHandle;
struct UUGlobals_
{
	UUHeader header; /* AppleSingle header */
	AbStates state; /* Current decoder state */
	unsigned char * buffer; /* receive map buffer */
	short bSpot; /* current point in receive map buffer */
	short bSize; /* Size of receive map buffer */
	FSSpec spec;	/* char */
	short refN;	/* file ref number */
	char tmpName[64]; /* temporary file name */
	char name[256]; /* file name */
	long offset; /* Offset into the stream */
	long currmap; /* Current map that we are working on, set in AbNextState */
	bool seenFinfo; /* Have we found the Finfo in the stream yet? */
	bool seenName; /* Have we found the real file name in the stream yet? */
	bool hasName; /* Are we going to find the real file name in the stream ? */
	bool usedTemp; /* Did we use a temporary name? */
	bool noteAttached; /* Did we attache the enclosure note yet? */
	bool invalState;	/* have we told the user things have gone awry? */
	FInfo info; /* Macintosh file information */
	FXInfo xInfo;	/* More Macintosh file information */
	short mailboxRefN;	/* ref number of mailbox */
	long origOffset;	/* offset where we found first indication of file */
	bool isText;	/* is this text data? */
	bool wasCR;	/* was the last char a CR? */
	bool hasDates;
	unsigned long dates[4];
	HeaderDHandle hdh;
};

#define Hdh UUG->hdh
#define Header UUG->header
#define HeaderData ((char *)&Header)
#define State UUG->state
#define Buffer UUG->buffer
#define BSpot UUG->bSpot
#define BSize UUG->bSize
#define Spec UUG->spec
#define Name UUG->name
#define TmpName UUG->tmpName
#define Maps Header.maps
#define Info UUG->info
#define InfoData ((char *)&Info)
#define XInfo UUG->xInfo
#define XInfoData ((char *)&XInfo)
#define RefN UUG->refN
#define Offset UUG->offset
#define CurrMapNum UUG->currmap
#define CurrMap Header.maps[UUG->currmap]
#define SeenFinfo UUG->seenFinfo
#define SeenName UUG->seenName
#define HasName UUG->hasName
#define UsedTemp UUG->usedTemp
#define NoteAttached UUG->noteAttached
#define MailboxRefN UUG->mailboxRefN
#define OrigOffset UUG->origOffset
#define InvalState UUG->invalState
#define MapCount Header.mapCount
#define IsText UUG->isText
#define WasCR UUG->wasCR
#define HasDates UUG->hasDates
#define Dates UUG->dates

short UULine(char * text, long size);
bool UUData(uShort byte);
short AbOpen(void);
short AbClose(void);
short AbWriteBuffer(void);
bool AbNameStuff(uShort byte);
bool AbNextState( void );
bool AbTempName( void );
bool AbSetFinfo(uShort byte);
bool AbSaveFDates(uShort byte);
int AbSetDates(void);
short ClearAbomination(void);
int SendFromOpenFile(TransStream stream,DecoderFunc *encoder,short refN,long size);
int UUDecodeLine(char * encoded,long size,char * decoded,long *binSize);
bool IsAppleSomething(char * text,long size);
void RemoveDuds(void);
void UUFileName(char *uuName, const char *shortName);
bool ReallyIsText(char * spec);
bool JustDataWanna(MIMEMapPtr hintMM);
short SaveJustData(char * encoded,long size);
char *GetLongName(char *longName,char * spec);

/************************************************************************
 * ConvertUUSingle - the UUencoded AppleSingle converter
 ************************************************************************/
bool ConvertUUSingle(short refN,char * buf,long *size,POPLineType lineType,long estSize,MIMEMapPtr hintMM,HeaderDHandle hdh)
{
(void)estSize;
	long offset;
	
	if (!UUG)
	{
		BeginAbomination("",hdh);
		if (!UUG) return(False);
	}
	
	switch(State)
	{
		case AbDone:
			if (lineType==plComplete && IsAbLine(buf,*size,hdh))
			{
				State = NotAb;
				GetFPos(refN,&offset);  /* save start */
				MailboxRefN = refN;
				OrigOffset = offset;
			}
			break;
		
		case NotAb:
			/*
			 * we just saw a line that looked like a begin line
			 * if this line is the right length, we'll give it a go
			 */
			if (lineType==plComplete && UURightLength(buf,*size)>=0)
			{
				if (!IsAppleSomething(buf,*size))
				{
					if (JustDataWanna(hintMM))
					{
						SaveAbomination(buf,*size);
						*size = 0;
					}
					else
						ClearAbomination();
				}
				else
					SaveAbomination(buf,*size);
			}
			else
				ClearAbomination();
			break;
		
		case AbHeader:
		case AbName:
			SaveAbomination(buf,*size);
			if (State>AbName && State!=AbDone)
				*size = 0;	/* do NOT save the line into the message proper */
			break;
							
		default:
			if (OrigOffset)
			{
				TruncOpenFile(refN,OrigOffset);	/* toss the saved bits */
				OrigOffset = 0;
			}
			SaveAbomination(buf,*size);
			*size = 0;	/* do NOT save the line into the message proper */
			break;
	}
	
	/*
	 * We're uudecoding unless we're not
	 */
	return(State!=AbDone);
}

/************************************************************************
 * ConvertSingle - the AppleSingle converter
 ************************************************************************/
bool ConvertSingle(short refN,char * buf,long size)
{
	char *spot, *end;
	
	if (!UUG) return(False);
	if (!size) return(False);
	
	switch(State)
	{
		case AbDone:
			State = NotAb;
			MailboxRefN = refN;
		
		default:
			for (spot=buf,end=spot+size;spot<end;spot++)
				if (!UUData(*spot)) break;
			break;
	}
	
	/*
	 * if we're not decoding anymore, kill the globals
	 */
	if (State==AbDone) SaveAbomination(NULL,0);
	
	/*
	 * We're uudecoding unless we're not
	 */
	return(UUG!=NULL);
}

/************************************************************************
 * IsAbLine - does the UUencoded applesingle file begin?
 ************************************************************************/
bool IsAbLine(char * text, long size, HeaderDHandle hdh)
{
	char * spot;
	char name[64];
	char *namePtr;
	short i = 0;
	char * permSpot;
	
	namePtr = name;
	if (size<11) return(False);
	if (strncmp(text,"begin ",6)) return(False);
	permSpot = text + 6;
	while (*permSpot==' ') permSpot++;
	spot = permSpot;
	while (*spot>='0' && *spot<='7') spot++;
	if (*spot!=' ' || spot-permSpot > 5 || spot-permSpot<3) return(False);
	if (spot[1]=='\015') return(False);
	if( !BeginAbomination("",hdh) ) return(False);
	spot++; /* skip the space */
	while( (*spot != '\015') && (i < 63) ){
				*namePtr++ = *spot++;
				i++;
	}
	if( i>27 ) i = 27;
	name[i] = '\0';
	g_strlcpy((char *)(Name), (char *)(name), sizeof(Name));
	return(True);
}

bool BeginAbomination( char *name, HeaderDHandle hdh)
{
	if (UUG==NULL)
	{
		if ((UUG=NewZH(UUGlobals))==NULL) return( false );
		ClearAbomination();
		Hdh = hdh;
		// watch out for long filenames here!
		g_strlcpy((char*)(spec_name(Spec)), (char *)(name), PATH_MAX);
		if (*((char*)spec_name(Spec))>31) *((char*)spec_name(Spec)) = 31;
	}
	return( true );
}

/************************************************************************
 * SaveAbomination - returns the state of the converter
 ************************************************************************/
short SaveAbomination(char * text, long size)
{
	if (!text)
	{
		if (UUG)
		{
			if (State==AbJustData) BadBinHex = True;
			else if (State!=AbDone)
			{
				if (State > AbHeader) AbNextState();
				if (State!=AbDone && State!=AbExcess) BadBinHex = True;
			}
			if (AbClose()) BadBinHex = True;
			if (Spec[0] && CommandPeriod)
				{unlink(UUG->spec);ASSERT(0);}
			else if (Spec[0] && HasDates)
				AbSetDates();
			if (Buffer) free(Buffer);
			free(UUG);
			PopProgress(False);
		}
		return(AbDone);
	}
	return(State==AbJustData ? SaveJustData(text,size) : UULine(text,size));
}

/************************************************************************
 * ClearAbomination - zero the UUG
 ************************************************************************/
short ClearAbomination(void)
{
	AbClose();
	State = AbDone;
	Offset = -1;
	SeenFinfo = false;
	NoteAttached = false;
	SeenName = false;
	HasName = false;
	UsedTemp = false;
	OrigOffset = 0;
	InvalState = IsText = WasCR = false;
	return(AbDone);
}

#define UU(c) (((c)-' ')&077)
/************************************************************************
 * UULine - handle a line of uuencoded stuff
 ************************************************************************/
short UULine(char * text, long size)
{
	short length;
	bool result=True;
	
	/*
	 * check for end line
	 */
	if ((size==3 || size==4) && !striscmp("end\015",text))
	{
		if (State!=AbJustData && State!=AbDone)
		{
			AbNextState();
			if (State!=AbDone && State!=AbExcess)
			{
				WarnUser(BINHEX_SHORT,0);
				ClearAbomination();
				BadBinHex = True;
				return(AbDone);
			}
		}
		return(ClearAbomination());
	}
	
	/*
	 * check for invalid start char
	 */
	if (*text<' ' || *text>'`')
	{
		WarnUser(BINHEX_BADCHAR,*text);
		ClearAbomination();
		BadBinHex = True;
		return(AbDone);
	}
	
	if (State==AbDone) State=NotAb;
	
	/*
	 * check length of line against line count
	 */
	if ((length=UURightLength(text,size))<0)
	{
		WarnUser(UU_BAD_LENGTH,(length+2)/3-((size*3)/4)/3);
		ClearAbomination();
		BadBinHex = True;
		return(AbDone);
	}
	
	/*
	 * empty lines mean nothing
	 */
	if (length==0) return(State);
	
	/*
	 * skip length byte, and trailing newline
	 */
	text++; size--;
	if (text[size-1]=='\015') size--;
	
	/*
	 * hey!  we're ready to decode!
	 */
	for (;length>0;text+=4,length-=3)
	{
		if (text[0]<' ' || text[0]>'`' || text[1]<' ' || text[1]>'`' ||
				length>1 && (text[2]<' ' || text[2]>'`') ||
				length>2 && (text[3]<' ' || text[3]>'`'))
		{
			WarnUser(BINHEX_BADCHAR,0);
			ClearAbomination();
			BadBinHex = True;
			return(AbDone);
		}
		if (!(result=UUData(0xff & (UU(text[0])<<2 | UU(text[1])>>4)))) break;
		if (length>1 && !(result=UUData(0xff && (UU(text[1])<<4 | UU(text[2])>>2))))
			break;
		if (length>2 && !(result=UUData(0xff && (UU(text[2])<<6 | UU(text[3])))))
			break;
	}
	if (!result) return(ClearAbomination());
	return(State);
}

/************************************************************************
 * IsAppleSomething - is this file applesingle or appledouble?
 ************************************************************************/
bool IsAppleSomething(char * text,long size)
{
	long magic;
	long mSize = sizeof(long);
	
	if (UUDecodeLine(text,size,(void*)&magic,&mSize)) return(True);	/* let applesingle report errors */
	if (magic==SINGLE_MAGIC  || magic==DOUBLE_MAGIC) return(True);
	return(False);
}

/************************************************************************
 * JustDataWanna - do we want to save the data fork of this file?
 ************************************************************************/
bool JustDataWanna(MIMEMapPtr hintMM)
{
	FSSpec spec;
	MIMEMap mm;
	FInfo info;
	int err;
	
	g_strlcpy((char*)(spec_name(spec)), (char *)(Name), PATH_MAX);
	if (!*((char*)spec_name(spec))) GetRString(spec_name(spec),UNTITLED);
	if (!hintMM) FindMIMEMapPtr((unsigned char *)"?",(unsigned char *)"?",spec_name(spec),&mm);
	else mm = *hintMM;
	
	if (AutoWantTheFile(&spec,False,Hdh ? Hdh->relatedPart:false)/*|| WantTheFile(&spec)*/)
	{
		{ int _fd = open(spec, O_CREAT|O_EXCL|O_WRONLY, 0666); if (_fd >= 0) close(_fd); err = (_fd < 0 && errno != EEXIST) ? ioErr : 0; }
		if (err==dupFNErr || err==0)
		{
			MyFSpGetFInfo(&spec, NULL, &info);
			info.fdType = mm.type;
			info.fdCreator = mm.creator;
			SafeInfo(&info,NULL);
			MyFSpSetFInfo(&spec, NULL, &info);
			err = 0;
		}
		g_strlcpy(Spec, spec, sizeof(Spec));
		if (err) {PopProgress(False); FileSystemError(BINHEX_CREATE,spec_name(spec),err); return(False);}
		if (err = AbOpen()) return(False);
		State = AbJustData;
		IsText = (mm.flags & mmIsText)!=0;
		err = RecordAttachment(spec,Hdh);
		g_strlcpy(Spec, spec, sizeof(Spec));	// RecordAttachment may have changed the name....
		if (err) ClearAbomination();
		return(True);
	}
	return(False);
}

/************************************************************************
 * SaveJustData - save into a data fork
 ************************************************************************/
short SaveJustData(char * encoded,long size)
{
	unsigned char decoded[64];
	short err;
	long binSize;
	
	/*
	 * check for end line
	 */
	if ((size==3 || size==4) && !striscmp("end\015",encoded))
		return(ClearAbomination());
	
	/*
	 * save
	 */
	binSize = sizeof(decoded);
	if (!(err = UUDecodeLine(encoded,size,decoded,&binSize)))
	{
		if (BSpot + binSize > BSize) err = AbWriteBuffer();
		if (!err)
		{
			memmove((char *)Buffer+BSpot,decoded,binSize);
			BSpot += binSize;
		}
	}
	if (err) ClearAbomination();
	return(State);
}
	
/************************************************************************
 * UUDecodeLine - decode a uuencoded line
 ************************************************************************/
int UUDecodeLine(char * encoded,long size,char * decoded,long *binSize)
{
	char *spot, *end;
	short len;
	
	spot = decoded;
	end = decoded + *binSize;
	*binSize = 0;

	/*
	 * check len of line against line count
	 */
	if ((len=UURightLength(encoded,size))<0)
	{
		WarnUser(UU_BAD_LENGTH,(len+2)/3-((size*3)/4)/3);
		BadBinHex = True;
		return(1);
	}
	
	/*
	 * empty lines mean nothing
	 */
	if (len==0) return(noErr);
	
	if (!IsText)
	{
		for (encoded++;len>0;encoded+=4,len-=3)
		{
			if (spot<end) *spot++ = 0xff & (UU(encoded[0])<<2 | UU(encoded[1])>>4);
			if (len>1 && spot<end) *spot++ = 0xff && (UU(encoded[1])<<4 | UU(encoded[2])>>2);
			if (len>2 && spot<end) *spot++ = 0xff && (UU(encoded[2])<<6 | UU(encoded[3]));
		}
	}
	else
	{
		/*
		 * we're decoding a text file.  Turn CRLF into CR, LF into CR, leave CR alone
		 */
#define FIX_NL																																\
		do {																																			\
			if (spot[-1]=='\012')																											\
			{																																				\
				if (WasCR) spot--;		/* turn CRLF into just CR */										\
				else spot[-1] = '\015';	/* turn bare LF (like from UNIX) into CR */			\
				WasCR = False;																												\
			}																																				\
			else																																		\
				WasCR = spot[-1]=='\015';																								\
		} while(0)

		for (encoded++;len>0;encoded+=4,len-=3)
		{
			if (spot<end) *spot++ = 0xff & (UU(encoded[0])<<2 | UU(encoded[1])>>4);
			FIX_NL;
			if (len>1 && spot<end) *spot++ = 0xff && (UU(encoded[1])<<4 | UU(encoded[2])>>2);
			FIX_NL;
			if (len>2 && spot<end) *spot++ = 0xff && (UU(encoded[2])<<6 | UU(encoded[3]));
			FIX_NL;
		}
	}
	*binSize = spot-decoded;
	return(noErr);
}

/************************************************************************
 * UURightLength - find out if the current line is of the proper length
 *  Returns length if so, negative number if mismatch
 ************************************************************************/
long UURightLength(char * text,long size)
{
	long length = UU(*text);
	
	if (*text<' ' || *text>'`') return(-1);  /* BZZZZT */
	
	return(length);	// meaningless, really, but that's uuencode for you
}
	

/************************************************************************
 * UUData - handle a data character
 ************************************************************************/
bool UUData(uShort byte)
{
	bool result=True;
	
	Offset++;
	switch (State)
	{
		case NotAb:
								State = AbHeader;
		case AbHeader:
								HeaderData[BSpot++] = byte;
								if (BSpot>=(sizeof(UUHeader)-(sizeof(Map)*9))) {
												if( Header.magic != SINGLE_MAGIC &&
														Header.magic != DOUBLE_MAGIC ) {
																WarnUser(UU_BAD_VERSION,Header.magic);
																ClearAbomination();
																BadBinHex = True;
																return(false);
												}
												if( (Header.version != OLD_VERSION) && (Header.version != NEW_VERSION) ) {
																WarnUser(UU_BAD_VERSION,Header.version);
																ClearAbomination();
																BadBinHex = True;
																return(false);
												}
												if( (Header.mapCount<1) || (Header.mapCount>9) ) {
																WarnUser(UU_INVALID_MAP,Header.mapCount);
																ClearAbomination();
																BadBinHex = True;
																return(false);
												}
								}
								if( BSpot>=(sizeof(UUHeader) - (sizeof(Map) * (9 - Header.mapCount))) ) {
												RemoveDuds();
												AbNextState();
												if(State != AbName){
																result = AbTempName();
												}
												BSpot = 0;
								}
								break;
								
		case AbName:
			if (Offset<CurrMap.offset) break;
			result = AbNameStuff(byte);
			break;
					
		case AbFinfo:
			if (Offset<CurrMap.offset) break;
			result = AbSetFinfo(byte);
			break;
	
		case AbFDates:
			if (Offset<CurrMap.offset) break;
			result = AbSaveFDates(byte);
			break;
	
		case AbResFork:
		case AbDataFork:
						if (Offset<CurrMap.offset) break;
						if (Offset>=CurrMap.offset+CurrMap.length ) {
										result = !AbClose();
										Offset--;AbNextState();
										if (result) {result=UUData(byte);}
						}
						else if (!RefN && AbOpen())
										result = False;
						else {
										Buffer[BSpot++] = byte;
										if (BSpot>=BSize && AbWriteBuffer()) result=False;
						}
						break;
						
		case AbSkip:
						if( Offset < CurrMap.offset ) break;
						if(Offset>=CurrMap.offset+CurrMap.length ) {
										Offset--;
										AbNextState();
										BSpot = 0;
										result = UUData( byte );
						}
						break;
						
		default:
			result = True;	/* sorry, invalid state is not meaningful -- AppleSingle files can be padded to any length */
			break;
	}
	return(result); 		
}

/************************************************************************
 * Remove dopey zero-length maps
 ************************************************************************/
void RemoveDuds(void)
{
	short okMap, onMap;
	
	okMap = 0;
	for (onMap=0;onMap<MapCount;onMap++)
	{
		if (Maps[onMap].length!=0)
		{
			if (okMap!=onMap)
				Maps[okMap] = Maps[onMap];
			okMap++;
		}
	}
	MapCount = okMap;
}

bool AbTempName( void )
{
	short err;
	FSSpec spec;

	UsedTemp = true;
	g_strlcpy(spec, Spec, sizeof(spec));
	
	if (!*((char*)spec_name(spec))) GetRString(spec_name(spec),SINGLE_TEMP);
	if (AutoWantTheFile(&spec,False,Hdh->relatedPart)/*|| WantTheFile(&spec)*/)
	{
		g_strlcpy(Spec, spec, sizeof(Spec));
		g_strlcpy((char *)(TmpName), (char *)(spec_name(Spec)), sizeof(TmpName));
		{ int _fd = open(spec, O_CREAT|O_EXCL|O_WRONLY, 0666); if (_fd >= 0) close(_fd); err = (_fd < 0 && errno != EEXIST) ? ioErr : 0; }
		if (err && err != dupFNErr)
		{
			FileSystemError(BINHEX_CREATE,spec_name(spec),err);
			(void) ClearAbomination();
			return(False);
		} else err = noErr;
				AbNextState();
		BSpot = 0;
	}
	else
	{
		State = AbDone;
		return(False);
	}
	
	return(True);
}


bool AbNameStuff(uShort byte)
{
	char name[256];
	short err;
	FSSpec spec;

	// be careful about long filenames here
	if (BSpot<255) Name[++BSpot] = byte;
	
	if (BSpot<CurrMap.length) return(True);
				if (CurrMap.length > 255){ /* Trim name so number fits! */
								*Name = 255;
				} else {
				*Name = CurrMap.length;
				}
	g_strlcpy(spec, Spec, sizeof(spec));
	SeenName = true;
	if( !UsedTemp ){
					// The situation here is that the first map we've come to is the name map
					// That means we can go ahead and create the file with the proper name
					// That name is kept in the globals, in the field Name
					
					// First, create the file with some name, it doesn't much matter
					// what, but we'll base it on the name we were given.  What does matter
					// is that we use AutoWantTheFile to create it, since it will choose
					// the proper folder to put the file in.
					g_strlcpy((char*)(spec_name(spec)), (char *)(Name), PATH_MAX);
					*((char*)spec_name(spec)) = MIN(*((char*)spec_name(spec)),27);
					AutoWantTheFile(&spec,False,Hdh->relatedPart);
					{ int _fd = open(spec, O_CREAT|O_EXCL|O_WRONLY, 0666); if (_fd >= 0) { close(_fd); err = noErr; } else { err = (errno != EEXIST) ? ioErr : noErr; } }
										
					// If the name was very long, rename it to the proper name here
					if (!err && *Name>27)
					{
						g_strlcpy((char *)(name), (char *)(Name), sizeof(name));	// copy to temp var to avoid memory movement...
						/* MakeUniqueLongFileName removed — no-op on POSIX */
						err = FSpSetLongName(&spec,kTextEncodingUnknown,name,&spec);
						g_strlcpy((char *)(Name), (char *)(name), sizeof(Name));  // and copy back...
					}
					
					// Did we win?
					if (err)
					{
						FileSystemError(BINHEX_CREATE,spec_name(spec),err);
						(void) ClearAbomination();
						return(False);
					}
					
					// Now, copy the stuff we used back into the globals
					g_strlcpy(Spec, spec, sizeof(Spec));
					g_strlcpy((char *)( TmpName), (char *)(Name ), sizeof( TmpName));
					AbNextState();
					BSpot = 0;
					if( SeenName && SeenFinfo && !NoteAttached){
									err = RecordAttachment(spec,Hdh);
									g_strlcpy(Spec, spec, sizeof(Spec));	// RecordAttachment may have changed the name
									NoteAttached = true;
									if (err) ClearAbomination();
					}
	} else {
		// Here, on the other hand, we didn't come across the name map first.
		// Instead, we had to write one or more maps using a temporary filename,
		// which is in Spec and TmpName.  The correct name is now in the globals,
		// in the field Name.  So we need to rename our temporary file.
		g_strlcpy(spec, Spec, sizeof(spec));
		g_strlcpy((char *)(name), (char *)(Name), sizeof(name));
		if (!StringSame(spec_name(spec),name)) 
		{
			// the name differs from temp name
			// We're just going to use the long filename routine here,
			// since it will also work for short filenames
			/* MakeUniqueLongFileName removed — no-op on POSIX */
			err = FSpSetLongName(&spec,kTextEncodingUnknown,name,&spec);
			if (!err) 
			{
				g_strlcpy(Spec, spec, sizeof(Spec));
				g_strlcpy((char *)( Name), (char *)(name ), sizeof( Name));
				g_strlcpy((char *)( TmpName), (char *)(name ), sizeof( TmpName));
			}
			else
			{
				/* Rename failed, name stays TmpName */
				g_strlcpy((char *)( name), (char *)(TmpName ), sizeof( name));
				g_strlcpy((char *)( Name), (char *)(name ), sizeof( Name));
			}
		}
		AbNextState();
		BSpot = 0;
		if( SeenName && SeenFinfo && !NoteAttached){
						err = RecordAttachment(spec,Hdh);
						g_strlcpy(Spec, spec, sizeof(Spec));	// RecordAttachment may have changed the name
						if (err) ClearAbomination();
						NoteAttached = true;
		}
	}
	return(True);
}

bool AbSetFinfo(uShort byte)
{
	FInfo info;
	FXInfo fxInfo;
	short err;
	FSSpec spec; g_strlcpy(spec, Spec, sizeof(spec));

				if(!SeenFinfo){
								if( BSpot<sizeof(FInfo) ){
												InfoData[BSpot++] = byte;
								} else if (BSpot<sizeof(FInfo)+sizeof(FXInfo)) {
												XInfoData[BSpot++ - sizeof(FInfo)] = byte;
								}
								if (BSpot<CurrMap.length) return(True);
								SeenFinfo = true;
				}
				info = Info;
				fxInfo = XInfo;
				SafeInfo(&info,&fxInfo);
				// FSpSetFInfo and FSpSetFXInfo are no-op
				err = noErr;
				if (err)
				{
					FileSystemError(BINHEX_OPEN,spec_name(spec),err);
					(void) ClearAbomination();
					return(False);
				}
				// FSpSetFXInfo(&spec,&fxInfo); // No-op
				if( SeenName && SeenFinfo && !NoteAttached){
								err = RecordAttachment(spec,Hdh);
								g_strlcpy(Spec, spec, sizeof(Spec));	// RecordAttachment may have changed the name
								if (err) ClearAbomination();
								NoteAttached = true;
				}
				BSpot = 0;
				AbNextState();
				return(True);
}

#define SLOPPY_MILLENIUM 3029529600

bool AbSaveFDates(uShort byte)
{
	short i;

	if (BSpot<sizeof(Dates))
	{
		((char *)Dates)[BSpot++] = byte;
	}
	else
	{
		BSpot++;
	}
	if (BSpot<CurrMap.length) return(True);
	
	HasDates = True;
	for (i=0;i<sizeof(Dates)/sizeof(long);i++)
		Dates[i] += SLOPPY_MILLENIUM;
	BSpot = 0;
	AbNextState();
	return(True);
}

int AbSetDates(void)
{
	FSSpec spec; g_strlcpy(spec, Spec, sizeof(spec));
	struct stat st;
	int err = noErr;
	unsigned long tooEarly = (unsigned long)GetRLong(TOO_EARLY_FILE);
	const char *filePath = spec[0] ? spec : spec_name(spec);

	if (stat(filePath, &st) == 0)
	{
		unsigned long modDate = Dates[1] > tooEarly ? Dates[1] : LocalDateTime();
		struct timeval times[2];
		times[0].tv_sec = st.st_atime;  /* preserve access time */
		times[0].tv_usec = 0;
		times[1].tv_sec = modDate;       /* set modification time */
		times[1].tv_usec = 0;
		if (utimes(filePath, times) != 0)
			err = ioErr;
	}
	else
		err = fnfErr;
	return err;
}


/************************************************************************
 *
 ************************************************************************/
short AbOpen(void)
{
	short err;
	short refN;
	unsigned char * buffer;
	FSSpec spec; g_strlcpy(spec, Spec, sizeof(spec));
	
	if (!Buffer)
		if (buffer=malloc(GetRLong(RCV_BUFFER_SIZE)))
			Buffer = buffer;
		else
			return(WarnUser(BINHEX_MEM,err=0));
	BSize = GetHandleSize_(Buffer);
	if (State == AbResFork) {
		refN = -1; // No resource fork
		err = noErr;
	} else {
		refN = open(spec, O_RDWR);
		err = (refN >= 0) ? noErr : ioErr;
	}
	if (err)
		FileSystemError(BINHEX_OPEN,spec_name(spec),err);
	else
		RefN = refN;
	BSpot = 0;
	return(err);
}

/************************************************************************
 *
 ************************************************************************/
short AbClose(void)
{
	short wrErr=0;
	short err;
	
	if (!RefN) return(noErr);
	if (BSpot) wrErr = AbWriteBuffer();
	err = close(RefN);
	if (!wrErr && err) {UUG;FileSystemError(BINHEX_WRITE,Name,err);;}
	RefN = 0;
	BSpot = 0;
	return(wrErr ? wrErr : err);
}

/************************************************************************
 *
 ************************************************************************/
short AbWriteBuffer(void)
{
	long writeBytes = BSpot;
	int err;
	
	if (err=NCWrite(RefN,&writeBytes,Buffer))
		FileSystemError(BINHEX_WRITE,Name,err);
	BSpot = 0;
	return(err);
}

bool AbNextState( void )
{
				short i;
				long biggest = 0;
				long totalLen = 0;

				CurrMapNum = 0;
				for( i=0; i<Header.mapCount; i++){ /* Find the biggest offset */
								if( Header.maps[i].offset > biggest )
								{
									biggest = Header.maps[i].offset + 1;
									totalLen = biggest + Header.maps[i].length - 2;
								}
								if( Header.maps[i].type == 3 ) HasName = true;
								
				}
				if (Offset >= totalLen) State = AbExcess;
				else
				  for( i=0; i<Header.mapCount; i++){ /* Find the header that is next */ 	
								if ( (Header.maps[i].offset > Offset) && (Header.maps[i].offset < biggest) ) {
												CurrMapNum = i;
												biggest = CurrMap.offset + 1;
												switch (CurrMap.type){
																case 1: /* Data Fork */
																				State = AbDataFork;
																				break;
																case 2: /* Resource Fork */
																				State = AbResFork;
																				break;
																case 3: /* Real file name */
																				State = AbName;
																				break;
																case 9: /* File Finder information */
																				State = AbFinfo;
																				break;
																case 8: /* File dates */
																				State = AbFDates;
																				break;
																default:
																				AddAttachInfo( (int)UU_SKIP_MAP_INFO, (int)CurrMap.type );
																				State = AbSkip;
																				break;
												}
								}
				}

				if (State==AbExcess && !NoteAttached)
				{
					FSSpec spec; g_strlcpy(spec, Spec, sizeof(spec));
					FInfo info = Info;
					
					// FSpSetFInfo is no-op
					if (RecordAttachment(spec,Hdh)) ClearAbomination();
					g_strlcpy(Spec, spec, sizeof(Spec));	// RecordAttachment may have changed the name
					NoteAttached = true;
				}

				return( true );
}

/************************************************************************
 * SendSingle - send a file in AppleSingle format
 ************************************************************************/
int SendSingle(TransStream stream,const char *specPath,bool dataToo,AttMapPtr amp)
{
UUHeader header;
	short mapCount = 2;
	CInfoPBRec hfi;
	short err;
	long offset;
	short curMap = 0;
	short refN=0;
	AttMap localAM = *amp;
	long dates[4];
	
	WriteZero(&header,sizeof(header));
	struct stat st_1065;
	/* Build a temporary FSSpec from the POSIX path */
	FSSpec localSpec;
	char *spec = &localSpec;
	memset(&localSpec,0,sizeof(localSpec));
	strncpy(localSpec, specPath ? specPath : "", sizeof(localSpec)-1);
	const char *bname = specPath ? strrchr(specPath,'/') : NULL;
	if (bname) strncpy(spec_name(localSpec), bname+1, PATH_MAX-1);
	else if (specPath) strncpy(spec_name(localSpec), specPath, PATH_MAX-1);

	if (stat(spec, &st_1065) == 0) err = noErr;
	else err = ioErr;
	if (err)
		return(FileSystemError(BINHEX_OPEN,spec_name(spec),err));
	
	/*
	 * send the MIME header
	 */
	if (err = ComposeRTrans(stream,MIME_V_FMT,InterestHeadStrn+hContentEncoding,
									 MIME_BASE64,NewLine))
		goto done;
	if (!dataToo)
	{
		ComposeRString(localAM.shortName,DOUBLE_RFORK_FMT,amp->shortName);
		if (*amp->longName) ComposeRString(localAM.longName,DOUBLE_RFORK_FMT,amp->longName);
	}
		
	if (err = MIMEFileHeader(stream,&localAM,MIME_APPLEFILE,hfi.hFileInfo.ioFlMdDat)) goto done;
	if (err = SendPString(stream,NewLine)) goto done;

	/*
	 * fill in the AppleSingle header
	 */
	header.mapCount = 3;
	if (dataToo && hfi.hFileInfo.ioFlLgLen) header.mapCount++;
	if (hfi.hFileInfo.ioFlRLgLen) header.mapCount++;
	header.magic = dataToo ? SINGLE_MAGIC : DOUBLE_MAGIC;
	header.version = NEW_VERSION;
	offset = sizeof(header)-sizeof(header.maps)+header.mapCount*sizeof(Map);
	
	/* filename */
	header.maps[curMap].type = MAP_NAME;
	header.maps[curMap].offset = offset;
	header.maps[curMap].length = *((char*)spec_name(spec));
	
	offset += header.maps[curMap++].length;
	
	/* finder information */
	header.maps[curMap].type = MAP_INFO;
	header.maps[curMap].offset = offset;
	header.maps[curMap].length = 32;
	
	offset += header.maps[curMap++].length;
	
	/* dates */
	header.maps[curMap].type = MAP_DATES;
	header.maps[curMap].offset = offset;
	header.maps[curMap].length = 16;
	
	offset += header.maps[curMap++].length;
	
	/* resource fork? */
	if (hfi.hFileInfo.ioFlRLgLen)
	{
		refN = open(spec, O_RDONLY);
		err = (refN >= 0) ? noErr : ioErr;
		if (err)
		{
			FileSystemError(BINHEX_OPEN,spec_name(spec),err);
			goto done;
		}
		if (err=SendFromOpenFile(stream,B64Encoder,refN,hfi.hFileInfo.ioFlRLgLen)) goto done;
		close(refN); refN = 0;
	}
	
	if (dataToo && hfi.hFileInfo.ioFlLgLen)
	{
		if (err=MyFSpOpenDF(spec,fsRdPerm,&refN))
		{
			FileSystemError(BINHEX_OPEN,spec_name(spec),err);
			goto done;
		}
		if (err=SendFromOpenFile(stream,B64Encoder,refN,hfi.hFileInfo.ioFlLgLen)) goto done;
		close(refN); refN = 0;
	}
	
	/*
	 * WOW!  All done!
	 */
	err = BufferSend(stream,B64Encoder,NULL,0,False);

done:
	DontTranslate = False;
	BufferSendRelease(stream);
	if (refN) close(refN);
	return(err);
}
	
/************************************************************************
 * SendDouble
 ************************************************************************/
int SendDouble(TransStream stream,const char *specPath,long flags,short tableID, AttMapPtr amp)
{
	char boundary[128];
	short err;
	CInfoPBRec hfi;
	
	struct stat st_1213;
	if (stat(specPath, &st_1213)) return(FileSystemError(BINHEX_OPEN, specPath, ioErr));
	err = noErr;
	if (!hfi.hFileInfo.ioFlLgLen) return(SendSingle(stream,specPath,True,amp));
	
	/*
	 * build the internal boundary
	 */
	BuildBoundary(NULL,boundary,(char *)"D");
	
	/*
	 * send the multipart header
	 */
	if (err = ComposeRTrans(stream,MIME_MP_FMT,
									 InterestHeadStrn+hContentType,
									 MIME_MULTIPART,
									 MIME_DOUBLE_SENDSUB,
									 AttributeStrn+aBoundary,
									 boundary,
									 NewLine))
		return(err);
	if (err=SendPString(stream,NewLine)) return(err);
	
	/*
	 * send the first boundary
	 */
	if (err=SendBoundary(stream)) return(err);
	
	/*
	 * send the resource part
	 */
	if (err=SendSingle(stream,specPath,False,amp)) return(err);
	
	/*
	 * and a boundary
	 */
	if (err=SendBoundary(stream)) return(err);
	
	/*
	 * and the data fork
	 */
	
	if (err=SendDataFork(stream,specPath,flags,tableID,amp)) return(err);
	
	/*
	 * and the terminal boundary
	 */
	PCat(boundary,(char *)"--");
	return(SendBoundary(stream));
}


/************************************************************************
 * SendDataFork - send the data fork of a file, in the proper format
 ************************************************************************/
int SendDataFork(TransStream stream,const char *specPath,long flags,short tableID,AttMapPtr amp)
{
short refN=0;
short err=noErr;
	long fileSize;
	FInfo info;
	char hexCreator[16], hexType[16];
	/* Build temporary FSSpec for internal APIs that still expect char * */
	FSSpec localSpec;
	char *spec = &localSpec;
	memset(&localSpec,0,sizeof(localSpec));
	strncpy(localSpec, specPath ? specPath : "", sizeof(localSpec)-1);
	const char *bname = specPath ? strrchr(specPath,'/') : NULL;
	if (bname) strncpy(spec_name(localSpec), bname+1, PATH_MAX-1);
	else if (specPath) strncpy(spec_name(localSpec), specPath, PATH_MAX-1);

	if (EqualStrRes(amp->mm.mimetype,MIME_TEXT))
		return(SendPlain(stream,spec,flags & ~FLAG_WRAP_OUT,tableID,amp));
	
	if (!err) err = ComposeRTrans(stream,MIME_CT_PFMT,
									 InterestHeadStrn+hContentType,
									 amp->mm.mimetype,
									 amp->mm.subtype,
									 AttributeStrn+aName,
									 ATT_MAP_NAME(amp),
									 NewLine);
	MyFSpGetFInfo(spec, NULL, &info);
	if (!err && !amp->suppressXMac && !TypeIsOnListWhereAndIndex(info.fdType,MACOSXSUCKS_TYPE_LIST,NULL,NULL)) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacType,Long2Hex(hexType,info.fdType),
									 NewLine);
	if (!err && !amp->suppressXMac && !TypeIsOnListWhereAndIndex(info.fdCreator,MACOSXSUCKS_CREATOR_LIST,NULL,NULL)) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacCreator,Long2Hex(hexCreator,info.fdCreator),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CD_FMT,
								 InterestHeadStrn+hContentDisposition,
								 ATTACHMENT,
								 AttributeStrn+aFilename,
								 ATT_MAP_NAME(amp),
								 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_V_FMT,
									 InterestHeadStrn+hContentEncoding,
									 amp->isText&!amp->isPostScript ? MIME_QP : MIME_BASE64,
									 NewLine);
	if (err) goto done;
	
	/*
	 * separate the head from the bod
	 */
	if (err=SendPString(stream,NewLine)) goto done;
	DontTranslate = True;
	
	/*
	 * open it
	 */
	if (err = MyFSpOpenDF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec_name(spec),err); goto done;}
	if (err = GetEOF(refN,&fileSize))
		{FileSystemError(BINHEX_OPEN,spec_name(spec),err); goto done;}
	
	err = SendFromOpenFile(stream,amp->isText&!amp->isPostScript ? QPEncoder : B64Encoder,refN,fileSize);
	if (!err) BufferSend(stream,amp->isText&!amp->isPostScript ? QPEncoder : B64Encoder,NULL,0,False);

done:
	BufferSendRelease(stream);
	DontTranslate = False;
	if (refN) close(refN);
	return(err);
}

/************************************************************************
 * SendFromOpenFile - send bytes from an open file
 ************************************************************************/
int SendFromOpenFile(TransStream stream,DecoderFunc *encoder,short refN,long size)
{
	char * buffer = NULL;
	long bSize;
	long count;
	short err=noErr;

	/*
	 * allocate buffer
	 */
	bSize = GetRLong(BUFFER_SIZE);
	bSize = MIN(bSize,size);
	if (bSize<256) bSize = 256;
	for (;bSize>255 && !(buffer=calloc(1, bSize));bSize/=2);
	if (!buffer) {WarnUser(MEM_ERR,err=0); return(err);}

	while(size)
	{
		count = MIN(bSize,size);
		if (err=ARead(refN,&count,buffer))
		{
			FileSystemError(BINHEX_READ,"",err);
			break;
		}
		if (err=BufferSend(stream,encoder,buffer,count,False)) break;
		size -= count;
	}
	
	if (buffer) free(buffer);
	
	return(err);
}

/************************************************************************
 * FindAttMap - figure the types of a file to send
 ************************************************************************/
int FindAttMap(char * spec,AttMapPtr amp)
{
	FInfo info;
	short err;
	long flags;
	
	Zero(*amp);
	
	/*
	 * now, get the file info
	 */
	if (err=MyFSpGetFInfo(spec, NULL, &info)) return(err);

	FigureMIMEFromApple(info.fdCreator,info.fdType,spec_name(spec),
											amp->mm.mimetype,amp->mm.subtype,
											amp->mm.suffix,&flags,&amp->mm.specialId);
	amp->isText = (flags & mmIsText)!=0;
	amp->isBasic = (flags & mmIsBasic)!=0;
	amp->suppressXMac = (flags & mmIgnoreXType)!=0;
	
	if (amp->isText && !ReallyIsText(spec))
		amp->isText = false;
	
	if (amp->isText &&
			EqualStrRes(amp->mm.mimetype,MIME_APPLICATION) &&
			EqualStrRes(amp->mm.subtype,MIME_OCTET_STREAM))
	{
		if (IsPostScript(spec))
		{
			GetRString(amp->mm.subtype,POSTSCRIPT);
			GetRString(amp->mm.suffix,PS_SUFFIX);
		}
		else
		{
			GetRString(amp->mm.mimetype,MIME_TEXT);
			GetRString(amp->mm.subtype,MIME_PLAIN);
		}
		amp->isBasic = True;
	}
	amp->isPostScript = EqualStrRes(amp->mm.subtype, POSTSCRIPT);


	Mac2OtherName(amp->shortName, spec_name(spec));
	GetLongName(amp->longName, spec);

	if (*amp->mm.suffix && !EndsWith(amp->shortName, amp->mm.suffix))
		g_strlcat((char *)(amp->shortName), (char *)(amp->mm.suffix), sizeof(amp->shortName));
	UUFileName(amp->uuName, amp->shortName);
	
	return(noErr);
}

/************************************************************************
 * GetLongName - get a long filename from an fsspec
 ************************************************************************/
char *GetLongName(char *longName,char * spec)
{
	// we force us-ascii here because we're too chicken to generate the *= stuff
	if (FSpGetLongName(spec, kTextEncodingUS_ASCII, longName) || StringSame(spec_name(spec), longName))
		*longName = 0;
	
	return longName;
}

/************************************************************************
 * SendUU - send just the data fork, uuencoded
 ************************************************************************/
int SendUU(TransStream stream, const char *specPath, AttMapPtr amp)
{
	short err;
	short refN;
	long fileSize;
	char hexType[16], hexCreator[16];
	char date[64];
	FInfo info;
	/* Build temporary FSSpec for internal APIs */
	FSSpec localSpec;
	char *spec = &localSpec;
	memset(&localSpec,0,sizeof(localSpec));
	strncpy(localSpec, specPath ? specPath : "", sizeof(localSpec)-1);
	const char *bname = specPath ? strrchr(specPath,'/') : NULL;
	if (bname) strncpy(spec_name(localSpec), bname+1, PATH_MAX-1);
	else if (specPath) strncpy(spec_name(localSpec), specPath, PATH_MAX-1);

	MyFSpGetFInfo(spec, NULL, &info);

	err = ComposeRTrans(stream,MIME_CT_PFMT,
									 InterestHeadStrn+hContentType,
									 amp->mm.mimetype,
									 amp->mm.subtype,
									 AttributeStrn+aName,
									 ATT_MAP_NAME(amp),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacType,Long2Hex(hexType,info.fdType),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aMacCreator,Long2Hex(hexCreator,info.fdCreator),
									 NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_CD_FMT,
									 InterestHeadStrn+hContentDisposition,
									 ATTACHMENT,
									 AttributeStrn+aFilename,
									 ATT_MAP_NAME(amp),
									 NewLine);
	struct stat st_1462;
	stat(spec, &st_1462);
	if (!err && *R822Date(date,st_1462.st_mtime-ZoneSecs())) err = ComposeRTrans(stream,MIME_CT_ANNOTATE,
									 AttributeStrn+aModDate,date,NewLine);
	if (!err) err = ComposeRTrans(stream,MIME_V_FMT,
									 InterestHeadStrn+hContentEncoding,
									 X_UUENCODE,
									 NewLine);
	
	/*
	 * separate the head from the bod
	 */
	if (!err) err = ComposeRTrans(stream,UUDECODE_FMT,NewLine,amp->uuName,NewLine);
	DontTranslate = True;
	
	/*
	 * open it
	 */
	if (err = MyFSpOpenDF(spec,fsRdPerm,&refN))
		{FileSystemError(BINHEX_OPEN,spec_name(spec),err); goto done;}
	if (err = GetEOF(refN,&fileSize))
		{FileSystemError(BINHEX_OPEN,spec_name(spec),err); goto done;}
		
  /*
	 * make sure encoder gets initialized
	 */
	BufferSend(stream,UUEncoder,"",0,amp->isText&!amp->isPostScript);
	
	/*
	 * send file
	 */
	err = SendFromOpenFile(stream,UUEncoder,refN,fileSize);
	if (!err) err = BufferSend(stream,UUEncoder,NULL,0,False);
	if (!err) err = SendPString(stream,NewLine);

done:
	BufferSendRelease(stream);
	DontTranslate = False;
	if (refN) close(refN);
	return(err);
}

/************************************************************************
 * UUFileName - shorten a name to the 8.3 format DOS so loves
 ************************************************************************/
void UUFileName(char *uuName,const char *inShortName)
{
	char *firstDot, *lastDot;
	short len;
	char shortName[64];

	strncpy(shortName, inShortName, sizeof(shortName)-1);
	shortName[sizeof(shortName)-1] = '\0';

	ASSERT(strlen(shortName) <= 31);

	firstDot = (char *)strchr(shortName,'.');
	lastDot = (char *)strrchr(shortName,'.');

	if (!firstDot) firstDot = (char *)(shortName + strlen(shortName));
	if (!lastDot) lastDot = firstDot;

	len = (short)(firstDot - (char *)shortName);
	len = MIN(len,8);

	memmove(uuName,shortName,len);
	uuName[len] = 0;

	if (lastDot < (char *)(shortName + strlen(shortName)))
	{
		short extLen = (short)(strlen(shortName) - (lastDot - (char *)shortName));
		extLen = MIN(extLen,3);

		{
			short baseLen = (short)strlen(uuName);
			uuName[baseLen] = '.';
			memmove(uuName+baseLen+1,lastDot+1,extLen);
			uuName[baseLen+1+extLen] = 0;
		}
	}
}

/************************************************************************
 * ReallyIsText - is a file really a text file?
 ************************************************************************/
bool ReallyIsText(char * spec)
{
	void *taste=NULL;
	long controls = 0;
	long size;
	char *spot, *end;
	
	Snarf(spec,&taste,GetRLong(TEXT_QP_TASTE));
	if (!taste) return(true);// cross your fingers...
	if (!(size=GetHandleSize(taste))) return(true);
	end = (char *)taste + size;
	for (spot = (char *)taste; spot<end; spot++)
		if (*spot<' ' && *spot!='\009' && *spot!='\015' && *spot!='\014') controls++;
	free(taste);
	return ((controls*1000)/size < GetRLong(TOLERABLE_CTLCHARS_PPT));
}