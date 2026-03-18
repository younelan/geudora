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

//depot/projects/eudora/mac/Current/rich.c#24 - edit change 4405 (text)
#include "rich.h"
#include "util.h"         /* StackInit, StackPush, StackPop, AccuAddStr, AccuAddChar, AccuAddPtr, ScriptVar */
#include "StringUtil.h"   /* ComposeRString */
#include "StringDefs.h"   /* MIME_RICH_ON */
#include "StrnDefs.h"     /* EnrichedStrn, EnrichedEnum */
#include "mime.h"         /* OffsetAndStyleHandle */
#include "fileutil.h"     /* MemError */
#include "comp.h"         /* CompHeadFind, HeadSpec */
#include "messact.h"      /* RelLine2Spec */
/* ColorParam declared below — color.h depends on mywindow.h which isn't ported yet */
#include "features.h"     /* HasFeature */

/* CycleBalls was a Mac UI feedback spinner — no-op in GTK */
static inline void CycleBalls(void) {}
#include "toc.h"          /* TOCType, UseFlowInExcerpt */

/* Font globals from Globals.h — declared here to avoid pulling in the entire header */
extern int FontID, FontSize, FixedID, FixedSize;
extern short *StdSizes;
/* Copyright (c) 1995 by QUALCOMM Incorporated */
#define FILE_NUM 76

/* Mac QuickDraw face style bits — used in tsFace field */
#ifndef bold
enum { bold = 1, italic = 2, underline = 4, outline = 8, shadow = 0x10, condense = 0x20, extend = 0x40 };
#endif

/* Mac Script Manager selector not yet in util.h */
#ifndef smScriptSysFondSize
#define smScriptSysFondSize 0x0408
#endif

/* Mac TextEdit alignment constants */
#ifndef teFlushDefault
enum { teFlushDefault = 0, teCenter = 1, teFlushRight = -1, teFlushLeft = -2 };
#endif

/* Mac error codes */
#ifndef unimpErr
#define unimpErr -4
#endif

/* IS_DEFAULT_COLOR — from original peteglue.h */
#ifndef IS_DEFAULT_COLOR
#define IS_DEFAULT_COLOR(c) (((c).red==0 && (c).green==0 && (c).blue==0) || ((c).red==1 && (c).green==1 && (c).blue==1))
#endif

/* Flow preferences — UseFlowInExcerpt already in toc.h as 0 */
#ifndef UseFlowIn
#define UseFlowIn false
#endif
#ifndef UseFlowOutExcerpt
#define UseFlowOutExcerpt false
#endif

/* pLinkLabel — label bit for URL links (from original peteglue.h) */
#ifndef pLinkLabel
#define pLinkLabel 0x10
#endif
#ifndef pSpellLabel
#define pSpellLabel 0x04
#endif

/* OPT_JUST_EXCERPT — message option flag */
#ifndef OPT_JUST_EXCERPT
#define OPT_JUST_EXCERPT (1<<21)
#endif

/* GetHandleSize without underscore — map to GetHandleSize_ */
#ifndef GetHandleSize
#define GetHandleSize(h) GetHandleSize_(h)
#endif

/* IntlConverter — Mac text encoding converter. GTK is always UTF-8.
   Provide a minimal typedef and no-op stubs. */
typedef struct { TextEncoding encoding; } IntlConverter;
static inline int CreateIntlConverter(IntlConverter *c, TextEncoding enc) { if (c) c->encoding = enc; return 0; }
static inline void DisposeIntlConverter(IntlConverter c) { (void)c; }
static inline int UpdateIntlConverter(IntlConverter *c, unsigned char *charset) { (void)c; (void)charset; return 0; }

/* PeteConvert2Marg — convert PETEParaInfo margins to PeteSaneMargin */
static inline void PeteConvert2Marg(PETEParaInfoPtr pinfo, PSMPtr marg)
{
	marg->first = pinfo->indent + pinfo->startMargin;
	marg->second = pinfo->startMargin;
	marg->right = pinfo->endMargin;
}

/* PeteConvertMarg — copy default para margins, then apply PeteSaneMargin to PETEParaInfo */
static inline void PeteConvertMarg(PETEHandle pte, long basePara, PSMPtr marg, PETEParaInfoPtr pinfo)
{
	/* Get base paragraph info if available */
	if (basePara >= 0 || basePara == kPETEDefaultPara)
		PETEGetParaInfo(PETE, pte, basePara, pinfo);
	pinfo->startMargin = marg->second;
	pinfo->indent = marg->first - marg->second;
	pinfo->endMargin = marg->right;
}

/* Forward declarations for functions not yet ported or in unincluded headers */
extern void GetFontName(short fontID, unsigned char *name);
extern int InsertHTMLLo(unsigned char * text, long *htmlOffset, long textLen, long *inOffset,
                        PETEHandle pte, TextEncoding encoding, long flags, StackHandle partRefStack);
extern bool EncodingError(int err);
extern TextEncoding CreateSystemRomanEncoding(void);
extern int StackQueue(void *what, StackHandle *pStack);
extern int StackTop(void *into, StackHandle stack);
extern int PETEInsertTextHandle(PETEInst pi, PETEHandle pte, long offset,
                                unsigned char * text, long len, long hOffset,
                                PETEStyleListHandle styles);
extern int PETESelect(PETEInst pi, PETEHandle pte, long start, long stop);
extern int PeteInsertHeader(PETEHandle pte, long *pOff, unsigned char * text, long len, long tOff);
extern int PeteEnsureBreakLo(PETEHandle pte, long offset, bool *did);
extern int PeteEnsureCrAndBreakLo(PETEHandle pte, long inOffset, long *newOffset, bool *did);
#define PeteEnsureBreak(pte, offset) PeteEnsureBreakLo(pte, offset, nil)
#define PeteEnsureCrAndBreak(pte, inOffset, newOffset) PeteEnsureCrAndBreakLo(pte, inOffset, newOffset, nil)
extern void PetePlainParaAtLo(PETEHandle pte, long start, long stop, long validBits);
#define PetePlainParaAt(pte, start, stop) PetePlainParaAtLo(pte, start, stop, peAllParaValid & ~peTabsValid)
extern long PeteParaAt(PETEHandle pte, long offset);
extern int PeteSetExcerptLevelAt(PETEHandle pte, long offset, short level);
extern bool PeteIsExcerptAt(PETEHandle pte, long offset);
extern int PeteGetStyleLo(PETEHandle pte, long offset, long *len, bool allowGraphics, PETEStyleEntry *style);
extern bool IsEmoticonStyle(PETEGraphicStylePtr gs);
extern void AttachNoteLo(const char *path, const char *theMessage);
extern int ColorParam(RGBColor *color, unsigned char *text);

int UnwindRich(short *counters,StackHandle openStack,StackHandle redoStack,AccuPtr enriched);
int RewindRich(StackHandle openStack,StackHandle redoStack,AccuPtr enriched);
int BuildEnrichedDirectives(PETETextStylePtr oldStyle,PETETextStylePtr newStyle,
															PETEParaInfoPtr oldInfo,PETEParaInfoPtr newInfo,
															StackHandle openStack,StackHandle redoStack,
															AccuPtr enriched);
char * BuildRichParaParam(char * directive,PSMPtr marg);
char * BuildRichFontParam(char * directive,short fontID);
char * BuildRichColorParam(char * directive,RGBColor *color);
void AddToStyle(EnrichedEnum cmd,bool neg,long offset,OffsetAndStyleHandle *styles,uLong valid);
int EnrichedToken(unsigned char * enriched,long maxLen,bool headers,short *cmdId,bool *neg,long *tStart,long *tStop);
int InsertEnriched(unsigned char * text, long *textOffset, long textLen, long *offset, bool unwrap, PETEHandle pte);
int InsertEnrichedLo(unsigned char * text, long *textOffset, long textLen, long *inOffset, bool unwrap, PETEHandle pte, TextEncoding encoding);
int InsertFlowed(unsigned char * text, long *textOffset, long textLen, long *inOffset, PETEHandle pte, bool delSP);
int InsertFlowedLo(unsigned char * text, long *textOffset, long textLen, long *inOffset, PETEHandle pte, bool delSP, TextEncoding encoding);
int InsertFixed(unsigned char * text, long *textOffset, long textLen, long *inOffset, PETEHandle pte, TextEncoding encoding);
#define FlushIntlText(pte, offset, converter) PeteInsertIntlText(pte, offset, nil, 0L, 0L, converter, kTextEncodingUnknown, false, true);
bool PartIsReferenced(uLong cID,uLong baseID,uLong sourceID,StackHandle partRefStack);
int FakeAttachment(PETEHandle pte,uLong *offset,char * spec);

#pragma segment Enriched

/**********************************************************************
 * Text/Enriched building
 **********************************************************************/
 
typedef struct
{
	EnrichedEnum directive;
	union
	{
		RGBColor color;
		PeteSaneMargin marg;
		short fontID;
	} param;
} DirectiveType,*DTPtr, *DTHandle;

#define ISSUE_DIRECTIVE																										\
	do {																																		\
		ComposeRString(directive,MIME_RICH_ON,EnrichedStrn+pushMe.directive);	\
		if (err=AccuAddStr(enriched,directive)) goto done;										\
		if (err=StackPush(&pushMe, &openStack)) goto done;											\
	} while (0)

/**********************************************************************
 * BuildEnriched - take a block of text and turn it into text/enriched
 **********************************************************************/
int BuildEnriched(AccuPtr enriched,PETEHandle pte,unsigned char * text,long len,long offset,PETEStyleListHandle pslh,bool xrich)
{
	long eSize = 0;
	long eOffset = 0;
	Byte c;
	int err = noErr;
	bool cr = False;
	PETEStyleEntry oldStyle, newStyle;
	PETEParaInfo oldInfo, newInfo;
	long oldPara = -1;
	long para;
	short paraDiff, styleDiff;
	StackHandle openStack=nil;
	StackHandle redoStack=nil;
	long runLen;
	char directive[256];
	DirectiveType pushMe;
	bool kkk;
	unsigned char * spot;
#ifdef DEBUG
	char scratch[256];
#endif
#ifdef NEVER
	EuStyleSheet ess;
#endif
	
	if (pte)
	{
		oldInfo.tabHandle = nil;
		PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&oldInfo);
		PeteStyleAt(pte,kPETEDefaultStyle,&oldStyle);
		PETEGetRawText(PETE,pte,&text);
	}
	if (!(err=StackInit(sizeof(DirectiveType),&openStack)))
	if (!(err=StackInit(sizeof(DirectiveType),&redoStack)))
	{
		if (xrich)
		{
			pushMe.directive = enXRich;
			ISSUE_DIRECTIVE;
		}
		while (offset<len)
		{
			if (pte)
			{
				/*
				 * first check for style changes
				 */
				PeteGetStyle(pte,offset,&runLen,&newStyle);
				runLen = MAX(runLen,1);	// we are very scared by very short runs
				if (newStyle.psStyle.textStyle.tsFont==FontID) newStyle.psStyle.textStyle.tsFont=kPETEDefaultFont;
#ifdef USEFIXEDDEFAULTFONT
				if (newStyle.psStyle.textStyle.tsFont==FixedID) newStyle.psStyle.textStyle.tsFont=kPETEDefaultFixed;
#endif
				if (newStyle.psStyle.textStyle.tsSize==0) newStyle.psStyle.textStyle.tsSize=ScriptVar(smScriptSysFondSize);
#ifdef USERELATIVESIZES
				if (((newStyle.psStyle.textStyle.tsSize==FontSize) && (newStyle.psStyle.textStyle.tsFont == kPETEDefaultFont)) ||
				    ((newStyle.psStyle.textStyle.tsSize==FixedSize) && (newStyle.psStyle.textStyle.tsFont == kPETEDefaultFixed)))
					newStyle.psStyle.textStyle.tsSize=kPETERelativeSizeBase;
#else
				if (newStyle.psStyle.textStyle.tsSize==FontSize) newStyle.psStyle.textStyle.tsSize=kPETEDefaultSize;
#endif
				runLen -= offset-newStyle.psStartChar;
#ifdef DEBUG
				{ size_t _mpl = (runLen); memcpy(scratch, text+offset, _mpl); ((char*)(scratch))[_mpl] = '\0'; }
#endif
				/*
				 * check for lily-white style run
				 */
				kkk = True;
				for (spot=text+offset;spot<text+offset+runLen;spot++)
				{
					if (!IsSpace(*spot)) {kkk=False;break;}
				}

				/*
				 * what's changed?
				 */
				styleDiff = PeteTextStyleDiff(&oldStyle.psStyle.textStyle,&newStyle.psStyle.textStyle);

				/*
				 * if all white and underline hasn't changed, pretend no change
				 */
				if (kkk && !(styleDiff&underline))
				{
					newStyle = oldStyle;
					styleDiff = 0;
				}

				PETEGetParaIndex(PETE,pte,offset,&para);
				if (oldPara!=para)
				{
					newInfo.tabHandle = nil;
					PETEGetParaInfo(PETE,pte,para,&newInfo);
					paraDiff = PeteParaInfoDiff(&oldInfo,&newInfo);
					oldPara = para;
				}
				else paraDiff = 0;
				
				/*
				 * anything changed?
				 */
				if (paraDiff || styleDiff)
				{
					if (err=BuildEnrichedDirectives(
						styleDiff?&oldStyle.psStyle.textStyle:nil,&newStyle.psStyle.textStyle,
						paraDiff?&oldInfo:nil,&newInfo,
						openStack,redoStack,
						enriched)) goto done;
					oldStyle = newStyle;
					oldInfo = newInfo;
					cr = False;
#ifdef NEVER
					if (RunType==Debugging)
					{
						Zero(ess);
						g_strlcpy((char *)(ess.styleName), (char *)(scratch), sizeof(ess.styleName));
						ess.textStyle = newStyle.psStyle.textStyle;
						ess.paraInfo = newInfo;
						ess.paraValid = peAllParaValid;
						ess.textValid = peAllValid;
						DebugStr(Style2String(&ess,scratch));
					}
#endif
				}
			}
			else
				runLen = len-offset;
			
			if (!pte)
			{
				pushMe.directive = enNoFill;
				ISSUE_DIRECTIVE;
				err = AccuAddChar(enriched,'\015');
				if (err) goto done;
			}
			
			runLen += offset;
			while (offset<runLen)
			{
				c = text[offset++];
				if (c=='\015' && pte)
				{
					if (!cr)
					{
						cr = True;
						if (err=AccuAddChar(enriched,c)) goto done;
					}
					if (err=AccuAddChar(enriched,c)) goto done;
				}
				else
				{
					cr = False;
					if (c=='<') if (err=AccuAddChar(enriched,c)) goto done;
					if (err=AccuAddChar(enriched,c)) goto done;
				}
			}
		}
	}
done:
	if (!err)
	{
		// trim off one hard linebreak
		if (enriched->offset && enriched->data[enriched->offset-1]=='\015')
			enriched->offset--;
		if (enriched->offset && enriched->data[enriched->offset-1]=='\015')
			enriched->offset--;
		// finish up the directives
		err = UnwindRich(nil,openStack,nil,enriched);
		if (err==fnfErr) err = 0;
		// make sure we end with hard linebreak
		while (enriched->offset<2) AccuAddChar(enriched,'\015');
		if (enriched->data[enriched->offset-1] != '\015')  AccuAddChar(enriched,'\015');
		if (enriched->data[enriched->offset-2] != '\015')  AccuAddChar(enriched,'\015');
	}
	free(openStack);
	free(redoStack);
	if (!err) AccuTrim(enriched);
	return(err);
}

/**********************************************************************
 * BuildEnrichedDirectives - build directives for moving from one style to another
 **********************************************************************/
int BuildEnrichedDirectives(PETETextStylePtr oldStyle,PETETextStylePtr newStyle,
															PETEParaInfoPtr oldInfo,PETEParaInfoPtr newInfo,
															StackHandle openStack,StackHandle redoStack,
															AccuPtr enriched)
{
	short styleDiff = oldStyle ? PeteTextStyleDiff(oldStyle,newStyle) : 0;
	short paraDiff = oldInfo ? PeteParaInfoDiff(oldInfo,newInfo) : 0;
	short counters[enPLeft];
	short i;
	int err = noErr;
	char directive[256];
	DirectiveType pushMe;
	short oldInc, newInc, normInc;
	PeteSaneMargin marg;
	
	/*
	 * undo section
	 */
	 
	// clear counters
	for (i=0;i<sizeof(counters)/sizeof(short);i++) counters[i]=0;
	
	// figure out what it is we have to undo
	if ((styleDiff && bold) && !(newStyle->tsFace&bold)) counters[enBold]=1;
	if ((styleDiff && italic) && !(newStyle->tsFace&italic)) counters[enItalic]=1;
	if ((styleDiff && underline) && !(newStyle->tsFace&underline)) counters[enUnderline]=1;
	if (styleDiff && peFontValid && oldStyle->tsFont!=kPETEDefaultFont)
	{
#ifdef USEFIXEDDEFAULTFONT
		if(oldStyle->tsFont == kPETEDefaultFixed)
			counters[enFixed]=1;
		else
#endif
			counters[enFont]=1;
	}
	if (styleDiff && peColorValid && !IS_DEFAULT_COLOR(oldStyle->tsColor)) counters[enColor]=1;
	if ((styleDiff & peSizeValid))
	{
		oldInc = FindSizeInc(oldStyle->tsSize);
		newInc = FindSizeInc(newStyle->tsSize);
		normInc = FindSizeInc(FontSize);
		if (oldInc==newInc) styleDiff &= ~peSizeValid;
		else if (oldInc<newInc)
		{
			if (oldInc<normInc)
			{
				while(oldInc<normInc && oldInc<newInc) {counters[enSmaller]++;oldInc++;}
			}
		}
		else if (oldInc>newInc)
		{
			if (oldInc>normInc)
			{
				while(oldInc>normInc && oldInc>newInc) {counters[enBigger]++;oldInc--;}
			}
		}
		if (oldInc==newInc) styleDiff &= ~peSizeValid;
	}
	
	if ((paraDiff && peQuoteLevelValid) && oldInfo->quoteLevel>newInfo->quoteLevel)
		counters[enExcerpt] = oldInfo->quoteLevel-newInfo->quoteLevel;
	
	if (paraDiff & peJustificationValid)
	{
		if (oldInfo->justification==teCenter)
			counters[enCenter] = 1;
		else if (oldInfo->justification==teFlushRight)
			counters[enRight] = 1;
		if (newInfo->justification==teFlushDefault || newInfo->justification==teFlushLeft)
			paraDiff &= ~peJustificationValid;
	}
	
	if (paraDiff & (peEndMarginValid|peStartMarginValid|peIndentValid))
	{
		PeteConvert2Marg(oldInfo,&marg);
		if (marg.first||marg.second||marg.right) counters[enPara]++;
	}
	
	//undo them
	if (err=UnwindRich(counters,openStack,redoStack,enriched)) goto done;
	
	//redo the ones we need to redo
	if (err=RewindRich(openStack,redoStack,enriched)) goto done;
	
	/*
	 * do section
	 */
	if (paraDiff & peQuoteLevelValid)
		for(i=oldInfo->quoteLevel;i<newInfo->quoteLevel;i++)
		{
			pushMe.directive = enExcerpt;
			ISSUE_DIRECTIVE;
		}
	

	if (paraDiff & peJustificationValid)
	{
		if (newInfo->justification==teCenter) pushMe.directive = enCenter;
		else pushMe.directive = enRight;
		ISSUE_DIRECTIVE;
	}

	if (paraDiff & (peEndMarginValid|peStartMarginValid|peIndentValid))
	{
		PeteConvert2Marg(newInfo,&marg);
		if (marg.first||marg.second||marg.right)
		{
			pushMe.directive = enPara;
			pushMe.param.marg = marg;
			ISSUE_DIRECTIVE;
			BuildRichParaParam(directive,&marg);
			AccuAddStr(enriched,directive);
		}
	}

	if ((styleDiff && bold) && (newStyle->tsFace&bold))
	{
		pushMe.directive = enBold;
		ISSUE_DIRECTIVE;
	}
	if ((styleDiff && italic) && (newStyle->tsFace&italic))
	{
		pushMe.directive = enItalic;
		ISSUE_DIRECTIVE;
	}
	if ((styleDiff && underline) && (newStyle->tsFace&underline))
	{
		pushMe.directive = enUnderline;
		ISSUE_DIRECTIVE;
	}
	if (styleDiff && peFontValid && newStyle->tsFont!=kPETEDefaultFont)
	{
#ifdef USEFIXEDDEFAULTFONT
		if(newStyle->tsFont==kPETEDefaultFixed)
		{
			pushMe.directive = enFixed;
			ISSUE_DIRECTIVE;
		}
		else
#endif
		{
			pushMe.directive = enFont;
			pushMe.param.fontID = newStyle->tsFont;
			ISSUE_DIRECTIVE;
			BuildRichFontParam(directive,newStyle->tsFont);
			AccuAddStr(enriched,directive);
		}
	}
	if (styleDiff && peColorValid && !IS_DEFAULT_COLOR(newStyle->tsColor))
	{
		pushMe.directive = enColor;
		pushMe.param.color = newStyle->tsColor;
		ISSUE_DIRECTIVE;
		BuildRichColorParam(directive,&newStyle->tsColor);
		AccuAddStr(enriched,directive);
	}

	if (styleDiff & peSizeValid)
	{
		//smaller?
		if (oldInc>newInc)
		{
			pushMe.directive = enSmaller;
			while(oldInc>newInc) {ISSUE_DIRECTIVE;oldInc--;}
		}
				
		//bigger?
		else if (oldInc<newInc)
		{
			pushMe.directive = enBigger;
			while(oldInc<newInc) {ISSUE_DIRECTIVE;oldInc++;}
		}
	}

done:
	return(err);
}

/**********************************************************************
 * BuildRichParaParam - build the font parameter
 **********************************************************************/
char * BuildRichParaParam(char * directive,PSMPtr marg)
{
	char guts[256];
	short i;
	
	*guts = 0;
	for (i=marg->right;i;i--)
	{
		PCatR(guts,EnrichedStrn+enPRight);
		PCatC(guts,',');
	}
	if (marg->first>marg->second)
	{
		for (i=marg->second;i;i--)
		{
			PCatR(guts,EnrichedStrn+enPLeft);
			PCatC(guts,',');
		}
		for (i=marg->first-marg->second;i;i--)
		{
			PCatR(guts,EnrichedStrn+enPIn);
			PCatC(guts,',');
		}
	}
	else
	{
		for (i=marg->second;i;i--)
		{
			PCatR(guts,EnrichedStrn+enPLeft);
			PCatC(guts,',');
		}
		for (i=marg->second-marg->first;i;i--)
		{
			PCatR(guts,EnrichedStrn+enPOut);
			PCatC(guts,',');
		}
	}
	if (*guts)
	{
		--*guts;
		ComposeRString(directive,MIME_RICH_PARAM,guts);
	}
	else *directive = 0;
	return(directive);
}

/**********************************************************************
 * BuildRichFontParam - build the font parameter
 **********************************************************************/
char * BuildRichFontParam(char * directive,short fontID)
{
	char scratch[256];
	
	GetFontName(fontID,scratch);
	PTr(scratch," ","_");
	ComposeRString(directive,MIME_RICH_PARAM,scratch);
	return(directive);
}

/**********************************************************************
 * BuildRichColorParam - build the font parameter
 **********************************************************************/
char * BuildRichColorParam(char * directive,RGBColor *color)
{
	char scratch[32];
	
	Bytes2Hex((void*)&color->red,2,scratch);
	Bytes2Hex((void*)&color->green,2,scratch+5);
	Bytes2Hex((void*)&color->blue,2,scratch+10);
	scratch[4] = scratch[9] = ',';
	scratch[14] = 0;
	ComposeRString(directive,MIME_RICH_PARAM,scratch);
	return(directive);
}

/**********************************************************************
 * UnwindRich - unwind richtext to remove certain directives
 **********************************************************************/
int UnwindRich(short *counters,StackHandle openStack,StackHandle redoStack,AccuPtr enriched)
{
	int err = fnfErr;
	DirectiveType top;
	char cmd[32];
	long counterCount=0;
	short i;
	
	if (counters)
	{
		counterCount = 0;
		for (i=0;i<enPLeft;i++) counterCount += counters[i];
		if (!counterCount) return(noErr);
	}
	
	while(!StackPop(&top,openStack))
	{
		ComposeRString(cmd,MIME_RICH_OFF,EnrichedStrn+top.directive);
		if (top.directive==enNoFill && (err=AccuAddChar(enriched,'\015'))) goto done;
		if (err=AccuAddStr(enriched,cmd)) goto done;
		if (counters && counters[top.directive])
		{
			counters[top.directive]--;
			if (!--counterCount) break;
		}
		else if (redoStack) err = StackPush(&top, &redoStack);
	}
done:
	return(err ? err : (counterCount?fnfErr:noErr));
}

/**********************************************************************
 * RewindRich - Reissue a set of directives
 **********************************************************************/
int RewindRich(StackHandle openStack,StackHandle redoStack,AccuPtr enriched)
{
	int err = noErr;
	DirectiveType top;
	char cmd[256];
	
	while(!err && !StackPop(&top,redoStack))
	{
		ComposeRString(cmd,MIME_RICH_ON,EnrichedStrn+top.directive);
		if (err=AccuAddStr(enriched,cmd)) goto done;
		switch(top.directive)
		{
			case enColor:
				BuildRichColorParam(cmd,&top.param.color);
				if (err=AccuAddStr(enriched,cmd)) goto done;
				break;
			case enPara:
				BuildRichParaParam(cmd,&top.param.marg);
				if (err=AccuAddStr(enriched,cmd)) goto done;
				break;
			case enFont:
				BuildRichFontParam(cmd,top.param.fontID);
				if (err=AccuAddStr(enriched,cmd)) goto done;
				break;
		}
		err = StackPush(&top, &openStack);
	}
done:
	return(err);
}

/**********************************************************************
 * Text/Enriched interpretation
 **********************************************************************/
#define enUnknown (0)
#define enText (-1)
#define enCR (-2)
#define enParamText (-3)
#define enDoubleLess (-4)
/************************************************************************
 * PeteRich - interpret rich text in a PETE record
 ************************************************************************/
int PeteRich(PETEHandle pte,long start,long stop,bool unwrap)
{
	unsigned char * text=nil;
	unsigned char * enriched=nil;
	long len;
	int err = noErr;
	
	/*
	 * grab the text/enriched
	 */
	PETEGetRawText(PETE,pte,&text);
	len = stop ? stop-start : GetHandleSize_(text)-start;
	enriched = malloc(len);
	if (!enriched) err = 0;
	else
	{
		BMD(text+start,enriched,len);
		
		/*
		 * now remove the rich text from the edit record
		 */
		PeteDelete(pte,start,start+len);
		
		/*
		 * now put it back, with interpretation
		 */
		err = InsertRich(enriched,0,-1,start,unwrap,pte,nil,false);
		free(enriched);
	}
	return(err);
}

/************************************************************************
 * InsertRich - insert some text that contains markup
 ************************************************************************/
int InsertRich(unsigned char * text,long textOffset,long textLen,long offset,bool unwrap,PETEHandle pte,StackHandle partStack,bool delSP)
{
	return InsertRichLo(text, textOffset, textLen, offset, false, unwrap, pte, partStack, nil, delSP);
}

/************************************************************************
 * InsertRichLo - insert some text that contains markup
 ************************************************************************/
int InsertRichLo(unsigned char * text,long textOffset,long textLen,long offset,bool headers,bool unwrap,PETEHandle pte,StackHandle partStack,MessHandle messH,bool delSP)
{
	short cmdId;
	bool neg;
	long tStart,tStop, tempiOffset, tempoOffset, tempStart;
	int err = noErr;
	char scratch[256];
	PartDesc pd;
	bool wasRel = False;
	bool paraMe = false;
	long realOffset;
	StackHandle partRefStack;
	
	if (!text) return(noErr);
	
	if (textLen==-1) textLen = GetHandleSize(text);
	
	if (textLen && text[textLen-1]=='\015') textLen--;	// trim one trailing newline
	
	tStart = tStop = textOffset;
	
	StackInit(sizeof(uLong),&partRefStack);
	
	while (!err && !(err=EnrichedToken(text,textLen,headers,&cmdId,&neg,&tStart,&tStop)))
	{
		TextEncoding encoding;
		
		encoding = kTextEncodingUnknown;
	TryItAgain :
		if(!neg) {
			if((tempiOffset = offset) == -1) PeteGetTextAndSelection(pte,nil,&tempiOffset,nil);
			tempStart = tStart;
			switch(cmdId) {
				default :
					goto JustText;
				case enXRich :
					err = InsertEnrichedLo(text,&tStart,textLen-tStart,&offset,unwrap,pte, encoding);
					break;
				case enXCharset :
					err = InsertFixed(text,&tStart,textLen-tStart,&offset,pte,encoding);
					break;
				case enXFlowed :
					err = InsertFlowedLo(text,&tStart,textLen-tStart,&offset,pte,delSP,encoding);
					break;
				case enXHTML :
				case enHTML :
					err = InsertHTMLLo(text,&tStart,textLen-tStart,&offset,pte,encoding,0,partRefStack);
			}
			if(EncodingError(err) && (encoding == kTextEncodingUnknown) && (messH != nil))
			{
				if(!messH->tocH->sums[messH->sumNum].mesgErrH) {
					AddMesgError(messH->tocH, messH->sumNum, GetRString(scratch,BAD_CHARSET_ERR), err);
				}
				if((tempoOffset = offset) == -1) PeteGetTextAndSelection(pte,nil,&tempoOffset,nil);
				offset = tempiOffset;
				PeteDelete(pte, tempiOffset, tempoOffset);
				encoding = CreateSystemRomanEncoding();
				tStart = tempStart;
				goto TryItAgain;
			}
			tStop = tStart;
			wasRel = false;
			paraMe = true;
		}
		else
		{
		JustText :
			{ size_t _mpl = (tStop-tStart); memcpy(scratch, text+tStart, _mpl); ((char*)(scratch))[_mpl] = '\0'; }
			if (!RelLine2Spec(scratch,&pd.spec,&pd.cid,&pd.relURL,&pd.absURL))
			{
				if (partStack && PartIsReferenced(pd.cid,pd.relURL,pd.absURL,partRefStack))
					StackQueue(&pd, &partStack);
				else
				{
					err = FakeAttachment(pte,&offset,&pd.spec);
					if (err) break;
					paraMe = false;
				}
				wasRel = True;
			}
			else if (!wasRel)
			{
				if (paraMe)	// make sure we start a new para?
				{
					if (offset==-1) PeteGetTextAndSelection(pte,nil,&realOffset,nil);
					else realOffset = offset;
				}
				
				if(headers)
				{
					err = PeteInsertHeader(pte,&offset,text,tStop-tStart,tStart);
					if(EncodingError(err)) err = noErr;
					else if(err) break;
					if((tStop-tStart == 2) && (text[tStart] == 13) && (text[tStart+1] == 13))
						headers = false;
				}
				else
				{
					err = PETEInsertTextHandle(PETE,pte,offset,text,tStop-tStart,tStart,nil);
					if(err) break;
					if (offset!=-1) offset += tStop-tStart;
				}
				
				// plain, please
				if (paraMe)
				{
					long oldLen = PeteLen(pte);
					PeteEnsureCrAndBreakLo(pte,realOffset,&realOffset,&paraMe);
					PetePlainParaAt(pte,realOffset,realOffset+tStop-tStart);
					if (offset==-1) PeteSelect(nil,pte,realOffset+tStop-tStart,realOffset+tStop-tStart);
					else offset += PeteLen(pte)-oldLen;
				}
			}
			else wasRel = false;	// skip only one token
			paraMe = false;
		}
	}
	
	free(partRefStack);
	return(err==eofErr ? noErr : err);
}

/************************************************************************
 * PartIsReferenced - is this part actually referenced by anything?
 ************************************************************************/
bool PartIsReferenced(uLong cID,uLong baseID,uLong sourceID,StackHandle partRefStack)
{
	uLong id;
	short item;
	
	if (!partRefStack) return false;
	for (item=0;item<partRefStack->elCount;item++)
	{
		StackItem(&id,item,partRefStack);
		if (id==cID || id==baseID || id==sourceID) return true;
	}
	return false;
}

/************************************************************************
 * FakeAttachment - an html part wasn't referenced by anything; pretend
 * it's an attachment
 ************************************************************************/
int FakeAttachment(PETEHandle pte,uLong *offset,char * spec)
{
	int err;
	char scratch[256];
	
	// snide comment
	GetRString(scratch,UNREFERENCED_PART);
	err = PeteInsertPtr(pte,*offset,scratch,strlen((const char*)scratch));
	if (err) return err;
	if (*offset!=-1) *offset += strlen((const char*)scratch);

	// attachment
	AttachNoteLo(spec,(const char *)scratch);
	err = PeteInsertPtr(pte,*offset,scratch,strlen((const char*)scratch));
	if (err) return err;
	if (*offset!=-1) *offset += strlen((const char*)scratch);
	
	return noErr;
}
			
/************************************************************************
 * InsertFlowed - insert some format=flowed, until we run out
 ************************************************************************/
int InsertFlowed(unsigned char * text, long *textOffset, long textLen, long *inOffset, PETEHandle pte, bool delSP)
{
	return InsertFlowedLo(text, textOffset, textLen, inOffset, pte, delSP, kTextEncodingUnknown);
}

/************************************************************************
 * InsertFlowed - insert some format=flowed, until we run out
 ************************************************************************/
int InsertFlowedLo(unsigned char * text, long *textOffset, long textLen, long *inOffset, PETEHandle pte, bool delSP, TextEncoding encoding)
{
	long tStart, tStop, cStart;
	char notFlowed[32], testMe[32];
	bool flowNext = false;
	short size;
	long offset = *inOffset;
	int err = noErr;
	bool interpret = UseFlowIn;
	bool excerpt = UseFlowInExcerpt;
	long quoteLevel, oldQuoteLevel=0;
	long lastCR;
	char sigSep[16];
#ifdef DEBUG
	char line[256];
#endif
	IntlConverter converter;
	
	err = CreateIntlConverter(&converter, encoding);
	if(err) return err;
	
	textLen += *textOffset;	// reset to absolute offset, please
	if (offset==-1) PeteGetTextAndSelection(pte,nil,&offset,nil);
	err = PeteEnsureCrAndBreak(pte,lastCR=offset,&offset);
	if(err) return err;
	
	ComposeRString(notFlowed,MIME_RICH_OFF,EnrichedStrn+enXFlowed);
	GetRString(sigSep,SIG_SEPARATOR);
	
	// skip token
	for (cStart = -1, tStart = *textOffset;tStart<textLen;tStart++)
	{
		if (text[tStart]==' ') cStart = tStart + 1;
		if (text[tStart]=='>') break;
	}
	if((cStart > 0) && (encoding == kTextEncodingUnknown))
	{
		{ size_t _mpl = ( tStart - cStart); memcpy(testMe,  text + cStart, _mpl); ((char*)(testMe))[_mpl] = '\0'; }
		UpdateIntlConverter(&converter, testMe);
	}
	tStart++;
	
	/*
	 * process input one line at a time
	 */
	while(!err && tStart<textLen)
	{
		// gather line
		for (tStop=tStart;tStop<textLen&text[tStop]!='\015';tStop++);
		
#ifdef DEBUG
		{ size_t _mpl = (tStop-tStart); memcpy(line, text+tStart, _mpl); ((char*)(line))[_mpl] = '\0'; }
#endif
		
		// is it the magic turn-off?
		if (tStop-tStart==*notFlowed)
		{
			{ size_t _mpl = (tStop-tStart); memcpy(testMe, text+tStart, _mpl); ((char*)(testMe))[_mpl] = '\0'; }
			if (StringSame(testMe,notFlowed))
			{
				if (flowNext)	// add back in newline we removed
				{
					err = FlushIntlText(pte, &offset, &converter);
					if(err) break;
					if (excerpt)
					{
						err = PeteEnsureBreak(pte,lastCR);
						if(err) break;
					}
					err = PeteInsertChar(pte,offset,'\015',nil);
					if(err) break;
					offset++;
					if (excerpt) err = PeteSetExcerptLevelAt(pte,lastCR=offset,quoteLevel);
				}
				break;
			}
		}
		
		if (interpret)
		{
			// figure current quotelevel
			for (quoteLevel=tStart;quoteLevel<tStop&text[quoteLevel]=='>';quoteLevel++);
			quoteLevel -= tStart;
			
			// if quotelevel changes, make hard newline
			if (oldQuoteLevel!=quoteLevel && flowNext)
			{
				err = FlushIntlText(pte, &offset, &converter);
				if(err) break;
				if (excerpt)
				{
					err = PeteEnsureBreak(pte,lastCR);
					if(err) break;
				}
				err = PeteInsertChar(pte,offset,'\015',nil);
				if(err) break;
				++offset;
				if (excerpt) err = PeteSetExcerptLevelAt(pte,lastCR=offset,oldQuoteLevel);
				flowNext = false;
			}
			oldQuoteLevel = quoteLevel;
			
			// undo space-stuffing?
			if (text[tStart]==' ') tStart++;
			// remove quotes
			else if ((excerpt||flowNext) && text[tStart]=='>')
			{
				while (tStart<tStop&text[tStart]=='>') tStart++;
				if (tStart<tStop && text[tStart]==' ') tStart++;	// more space-stuffing
			}
			
			// how about next time around?
			flowNext = false;
			if (tStart<tStop)
			{
				flowNext = text[tStop-1]==' ';
				if (flowNext && tStart<tStop-2)
					flowNext = text[tStop-3]!=' ' || text[tStop-2]!=' ';
				if (flowNext && tStop-tStart==*sigSep)
				{
					flowNext = 0!=memcmp(sigSep+1,text+tStart,*sigSep);
				}
			}
		}
		
		// how much do we insert?
		size = tStop-tStart;
		
		// include return if next line not flowed
		if (!flowNext && tStop<textLen) size++; // add return
		
		// trim space if we're doing that sort of thing
		if (flowNext && delSP && text[tStop-1]==' ') size--;
		
		// and insert
		if (size>0)
		{
			bool excerptChange;
			
			excerptChange = excerpt && !flowNext && quoteLevel!=PeteIsExcerptAt(pte,lastCR);
			err = PeteInsertIntlText(pte,&offset,text,tStart,tStart+size,&converter,kTextEncodingUnknown,false,excerptChange);
			if (!err && excerptChange)
			{
				err = PeteEnsureBreak(pte,lastCR);
				if(!err) err = PeteSetExcerptLevelAt(pte,offset,quoteLevel);
			}
			if (!flowNext) lastCR = offset;
		}
		
		tStart = tStop+1;
	}
	if (*inOffset!=-1) *inOffset = offset;	// record where we ended
	else PeteSelect(nil,pte,offset,offset);	// if at insertion point, make sure we end insertion point at end of text
	*textOffset = tStop;
	DisposeIntlConverter(converter);
	
	return(err);
}
			
int InsertFixed(unsigned char * text, long *textOffset, long textLen, long *inOffset, PETEHandle pte, TextEncoding encoding)
{
	long tStart, tStop, cStart, textStart;
	char notCharset[32], testMe[32];
	long offset = *inOffset;
	int err = noErr;
	IntlConverter converter;
	
	err = CreateIntlConverter(&converter, encoding);
	if(err) return err;
	
	textLen += *textOffset;	// reset to absolute offset, please
	if (offset==-1) PeteGetTextAndSelection(pte,nil,&offset,nil);
	PeteEnsureCrAndBreak(pte,offset,&offset);
	
	ComposeRString(notCharset,MIME_RICH_OFF,EnrichedStrn+enXCharset);
	
	// skip token
	for (cStart = -1, tStart = *textOffset;tStart<textLen;tStart++)
	{
		if (text[tStart]==' ') cStart = tStart + 1;
		if (text[tStart]=='>') break;
	}
	if((cStart > 0) && (encoding == kTextEncodingUnknown))
	{
		{ size_t _mpl = ( tStart - cStart); memcpy(testMe,  text + cStart, _mpl); ((char*)(testMe))[_mpl] = '\0'; }
		UpdateIntlConverter(&converter, testMe);
		if(err) goto DoDispose;
	}
	textStart = ++tStart;
	
	/*
	 * Look for end one line at a time
	 */
	while(tStart<textLen)
	{
		// gather line
		for (tStop=tStart;tStop<textLen&text[tStop]!='\015';tStop++);

		// is it the magic turn-off?
		if (tStop-tStart==*notCharset)
		{
			{ size_t _mpl = (tStop-tStart); memcpy(testMe, text+tStart, _mpl); ((char*)(testMe))[_mpl] = '\0'; }
			if (StringSame(testMe,notCharset)) break;
		}
		tStart = tStop + 1;
	}
	err = PeteInsertIntlText(pte,&offset,text,textStart,tStart,&converter,kTextEncodingUnknown,false,true);
	if (*inOffset!=-1) *inOffset = offset;	// record where we ended
	else PeteSelect(nil,pte,offset,offset);	// if at insertion point, make sure we end insertion point at end of text
	*textOffset = tStop;
DoDispose :
	DisposeIntlConverter(converter);
	return err;
}		

/************************************************************************
 * InsertEnriched - insert some text/enriched, until we run out
 ************************************************************************/
int InsertEnriched(unsigned char * text, long *textOffset, long textLen, long *inOffset, bool unwrap, PETEHandle pte)
{
	return InsertEnrichedLo(text,textOffset,textLen,inOffset,unwrap,pte,kTextEncodingUnknown);
}

/************************************************************************
 * InsertEnriched - insert some text/enriched, until we run out
 ************************************************************************/
int InsertEnrichedLo(unsigned char * text, long *textOffset, long textLen, long *inOffset, bool unwrap, PETEHandle pte, TextEncoding encoding)
{
	//variables related to the current command
	long origStart = *inOffset;
	long start;
	long tStart, tStop;
	short cmdId;
	bool neg;
	short lastCmd;
	//interpreter state variables
	short noFill = 0;
	short inRich = 0;
	short inParam = 0;
	short sizeInc = 0;
	short styleCounters[3];
	StackHandle fontStack=nil;
	StackHandle justStack=nil;
	StackHandle margStack=nil;
	StackHandle colorStack=nil;
	char paramText[256];
	//global prefs
	uLong validMask = ~GetPrefLong(PREF_INTERPRET_ENRICHED);
	uLong validParaMask = ~GetPrefLong(PREF_INTERPRET_PARA);
	//scratch variables
	int err = noErr;
	PETETextStyle style;
	char scratch[256];
	PETEParaInfo pinfo;
	PeteSaneMargin marg, curMarg;
	PETEStyleEntry pse;
	short exLevel = 0;
	long offset;
	bool cr, needSpace;
	long pIndex;
	unsigned char * origText;
	IntlConverter converter;
	
	err = CreateIntlConverter(&converter, encoding);
	if(err) return err;
	
	if (!(validParaMask&1)) validParaMask &= ~7L;	// if "margins" turned off, turn off all indents
	
	tStart = tStop = *textOffset;
	
	/*
	 * initialize state
	 */
	PeteGetTextAndSelection(pte,&origText,(origStart==-1)?&origStart:nil,nil);
	start = origStart;
	cr = (!start || origText[start-1]=='\015');
	needSpace = false;
	
	styleCounters[0] = styleCounters[1] = styleCounters[2] = 0;
	if (!(err=StackInit(sizeof(style.tsFont),&fontStack)) &&
			!(err=StackInit(sizeof(pinfo.justification),&justStack)) &&
			!(err=StackInit(sizeof(PeteSaneMargin),&margStack)) &&
			!(err=StackInit(sizeof(RGBColor),&colorStack)))
	{
		/*
		 * make sure we start at a paragraph
		 */
		PeteParaConvert(pte,start,start);
		PETESelect(PETE,pte,start,start);
		
		/*
		 * make the paragraph normal.
		 */
		Zero(marg);
		PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
		PeteGetTextAndSelection(pte,nil,nil,&offset);
		pIndex = PeteParaAt(pte,offset);
		PETESetParaInfo(PETE,pte,pIndex,&pinfo,
			peStartMarginValid|peIndentValid|peEndMarginValid);
			
		PETEGetStyle(PETE,pte,kPETEDefaultStyle,nil,&pse);
		PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,&pse.psStyle.textStyle,peAllValid);
		
		/*
		 * operate on tokens from the text/enriched
		 */
		tStart = tStop = *textOffset;
		while (!err && !(err=EnrichedToken(text,*textOffset+textLen,false,&cmdId,&neg,&tStart,&tStop)))
		{
#ifdef DEBUG
			{ size_t _mpl = (tStop-tStart); memcpy(scratch, text+tStart, _mpl); ((char*)(scratch))[_mpl] = '\0'; }
#endif				
			CycleBalls();
			if (!inRich && (cmdId!=enXRich || neg)) cmdId = enText;
			if (inParam && cmdId!=enParam) cmdId = enParamText;
			switch (cmdId)
			{
				case enXRich:
					if (neg) inRich = MAX(inRich-1,0);
					else inRich++;
					if (!inRich)
					{
						tStart = tStop;
						goto end;
					}
					break;
				case enCR:
					needSpace = false;
					if (!noFill)
					{
						if (unwrap && tStop-tStart==1)
						{
//							PeteInsertChar(pte,-1L,' ',nil);
							needSpace = true;
							cr = False;
						}
						else
						{
							if (unwrap) tStart++;
							for (;tStart<tStop;tStart++)
							{
								FlushIntlText(pte, nil, &converter);
								PeteInsertChar(pte,-1,'\015',nil);
								PeteGetTextAndSelection(pte,nil,&start,nil);
								PETEInsertParaBreak(PETE,pte,start);
								cr = True;
							}
						}
						break;
					}
					cr = True;		// Set cr to true when inserting a nofill cr - pr 9/24/96
					// fall through to enText if noFill
DoText :
				case enText:
					err = PeteInsertIntlText(pte, nil, text, tStart, tStop, &converter, kTextEncodingUnknown, needSpace, false);
					needSpace = false;
					if(cmdId == enText) cr = False;	// Reset cr to false after inserted text - pr 9/24/96
					break;
				case enDoubleLess:
//					err = PeteInsertChar(pte,-1L,'<',nil);
					cr = False;	// Reset cr to false after inserted text - pr 9/24/96
					++tStart;
					goto DoText;
//					break;
				case enUnderline:
				case enItalic:
				case enBold:
					if (neg)
						styleCounters[cmdId-1] = MAX(0,styleCounters[cmdId-1]-1);
					else
						styleCounters[cmdId-1]++;
					style.tsFace = styleCounters[cmdId-1] ? 1<<(cmdId-1) : 0;
					PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
												&style,(1<<(cmdId-1))&validMask);
					break;
				case enColor:
					if (neg) StackPop(nil,colorStack);	// drop top one
					if (StackTop(&style.tsColor,colorStack))
					{
						PeteStyleAt(pte,kPETEDefaultStyle,&pse);
						style.tsColor = pse.psStyle.textStyle.tsColor;
					}
					if (!neg) StackPush(&style.tsColor, &colorStack);
					PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
												&style,peColorValid&validMask);
					break;
				case enSmaller:
				case enBigger:
					if (cmdId==enSmaller&neg || cmdId==enBigger&!neg)
						sizeInc++;
					else
						sizeInc--;
#ifdef USERELATIVESIZES
					style.tsSize = kPETERelativeSizeBase+sizeInc;
#else
					style.tsSize = IncrementTextSize(FontSize,sizeInc);
#endif
					PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
												&style,peSizeValid&validMask);
					break;
				case enFont:
				case enFixed:
					if (neg)
					{
						StackPop(nil,fontStack);	//dump current one
						if (StackTop(&style.tsFont,fontStack)) style.tsFont = kPETEDefaultFont;
					}
					else
					{
						if (cmdId==enFont)
						{
							if (StackTop(&style.tsFont,fontStack))
								style.tsFont = kPETEDefaultFont;
						}
						else
							style.tsFont = GetFontID(GetRString(scratch,FIXED_FONT));
						StackPush(&style.tsFont, &fontStack);
					}
					PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
												&style,peFontValid&validMask);
					break;
				case enNoFill:
					/* Added to ensure break around nofill - pr 9/24/96 */
					PeteEnsureBreak(pte,origStart);
					if (!cr)
					{
						FlushIntlText(pte, nil, &converter);
						PeteInsertChar(pte,-1L,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);
						cr = True;
					}
					/* End added section */
					if (neg) noFill = MAX(noFill-1,0);
					else noFill++;
					break;
					
				case enLeft:
				case enCenter:
				case enRight:
					PeteEnsureBreak(pte,origStart);
					if (!cr)
					{
						FlushIntlText(pte, nil, &converter);
						PeteInsertChar(pte,-1L,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);
						cr = True;
					}
					Zero(pinfo);
					if (neg)
					{
						StackPop(nil,justStack);	// dump current one
						if (StackTop(&pinfo.justification,justStack)) pinfo.justification = teFlushDefault;
					}
					else
					{
						switch(cmdId)
						{
							case enLeft: pinfo.justification = teFlushLeft; break;
							case enCenter: pinfo.justification = teCenter; break;
							case enRight: pinfo.justification = teFlushRight; break;
						}
						StackPush(&pinfo.justification, &justStack);
					}
					PeteGetTextAndSelection(pte,nil,nil,&offset);
					pIndex = PeteParaAt(pte,offset);
					PETESetParaInfo(PETE,pte,pIndex,&pinfo,validParaMask&peJustificationValid);
					break;
				
				case enExcerpt:
					PeteEnsureBreak(pte,origStart);
					if (!cr)
					{
						FlushIntlText(pte, nil, &converter);
						PeteInsertChar(pte,-1L,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);
						cr = True;
					}
					Zero(pinfo);
					if (neg)
						exLevel = MAX(0,exLevel-1);	// dump current one
					else
						exLevel++;
					pinfo.quoteLevel = exLevel;
					PeteGetTextAndSelection(pte,nil,nil,&offset);
					pIndex = PeteParaAt(pte,offset);
					PETESetParaInfo(PETE,pte,pIndex,&pinfo,validParaMask&peQuoteLevelValid);
					break;
				
				case enPara:
					PeteEnsureBreak(pte,origStart);
					if (!cr)
					{
						FlushIntlText(pte, nil, &converter);
						PeteInsertChar(pte,-1L,'\015',nil);
						PeteGetTextAndSelection(pte,nil,&start,nil);
						PETEInsertParaBreak(PETE,pte,start);
						cr = True;
					}
					Zero(pinfo);
					if (neg) StackPop(nil,margStack);	// drop top one
					if (StackTop(&marg,margStack)) Zero(marg);
					if (!neg) StackPush(&marg, &margStack);
					PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
					PeteGetTextAndSelection(pte,nil,nil,&offset);
					pIndex = PeteParaAt(pte,offset);
					PETESetParaInfo(PETE,pte,pIndex,&pinfo,validParaMask&(peStartMarginValid|peIndentValid|peEndMarginValid));
					break;

				case enParam:
					if (neg)
					{
						inParam = MAX(0,inParam-1);
						switch(lastCmd)
						{
							case enFont:
								style.tsFont=GetFontID(paramText);
								if (style.tsFont==1)
								{
									PTr(paramText,"_"," ");
									style.tsFont = GetFontID(paramText);
								}
								if (style.tsFont!=1)
								{
									// *replace* the top font with the new one
									StackPop(nil,fontStack);
									StackPush(&style.tsFont, &fontStack);
									PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
											&style,peFontValid&validMask);
								}
								break;
							case enColor:
								if (!ColorParam(&style.tsColor,paramText))
								{
									// *replace* the top color with the new one
									StackPop(nil,colorStack);
									StackPush(&style.tsColor, &colorStack);
									PETESetTextStyle(PETE,pte,kPETECurrentStyle,kPETECurrentStyle,
											&style,peColorValid&validMask);
								}
								break;
							case enPara:
								PeteEnsureBreak(pte,origStart);
								Zero(pinfo);
								if (!ParaIndent2Margin(&marg,paramText))
								{
									// *replace* the top margin with the new one
									if (StackPop(&curMarg,margStack)) Zero(curMarg);
									marg.first += curMarg.first;
									marg.second += curMarg.second;
									marg.right += curMarg.right;
									PeteConvertMarg(pte,kPETEDefaultPara,&marg,&pinfo);
									StackPush(&marg, &margStack);
									PeteGetTextAndSelection(pte,nil,nil,&offset);
									pIndex = PeteParaAt(pte,offset);
									PETESetParaInfo(PETE,pte,pIndex,&pinfo,
										validParaMask&(peStartMarginValid|peIndentValid|peEndMarginValid));
								}
								break;
							case enXRich:
								if(encoding == kTextEncodingUnknown)
								{
									UpdateIntlConverter(&converter, paramText);
								}
						}
					}
					else inParam++;
					break;
				case enParamText:
					{ size_t _mpl = (tStop-tStart); memcpy(scratch, text+tStart, _mpl); ((char*)(scratch))[_mpl] = '\0'; }
					g_strlcat((char *)(paramText), (char *)(scratch), sizeof(paramText));
					break;
			}
			if (cmdId!=enParam && cmdId!=enParamText) lastCmd=cmdId;
			if (cmdId!=enParamText && !(cmdId==enParam && neg)) *paramText = 0;
		}
	}
end:
	*textOffset = tStart;
	if (*inOffset!=-1) PeteGetTextAndSelection(pte,nil,inOffset,nil);
	free(margStack);
	free(fontStack);
	free(justStack);
	free(colorStack);
	DisposeIntlConverter(converter);
	return err==eofErr ? noErr : err;
}

/**********************************************************************
 * ParaIndent2Margin - convert a paraindent command to one of my margin things
 **********************************************************************/
int ParaIndent2Margin(PSMPtr marg,char * string)
{
	char token[256];
	unsigned char * spot;
	int err = fnfErr;
	
	Zero(*marg);
	for (spot=string;PToken(string,token,&spot,",");)
	{
		err = noErr;
		switch(FindSTRNIndex(EnrichedStrn,token))
		{
			case enPLeft: marg->second++; marg->first++; break;
			case enPRight: marg->right++; break;
			case enPIn: marg->first++; break;
			case enPOut: marg->second++; break;
			default: return(fnfErr);
		}
	}
	return(noErr);
}

/**********************************************************************
 * EnrichedToken - get the next token from a text/enriched stream
 **********************************************************************/
int EnrichedToken(unsigned char * enriched,long maxLen,bool headers,short *cmdId,bool *neg,long *tStart,long *tStop)
{
	unsigned char *start, *stop, *end;
	char cmd[64];
	long len = GetHandleSize(enriched);
	
	if (maxLen<len) len = maxLen;
	
	
	*tStart = *tStop;
	start = enriched+*tStart;
	end = enriched+len;
	
	// end of text?
	if (*tStart>=len) {; return(eofErr);}
	
	// carriage return?
	if (*start=='\015')
	{
		for (stop=start+1;stop<end && *stop=='\015';stop++);
		*cmdId = enCR;
	}
	// other stuff
	else
	{
		for (stop=start+1; stop<end && (headers || *stop!='<'&*stop!='>') && (*stop!='\015' || headers&stop<end-1&IsWhite(stop[1]));stop++);
		if (*start=='<')
		{
			// "<<"
			if (*stop=='<' && stop-start==1)
			{
				stop++;
				*cmdId = enDoubleLess;
			}
			// a <>-delimited-thing
			else if (*stop=='>')
			{
				unsigned char * tempStop;
				
				for(tempStop=start+1;tempStop<stop&*tempStop!=' ';tempStop++);
				*neg = start[1]=='/';
				if (*neg) { size_t _mpl = (tempStop-start-2); memcpy(cmd, start+2, _mpl); ((char*)(cmd))[_mpl] = '\0'; }
				else { size_t _mpl = (tempStop-start-1); memcpy(cmd, start+1, _mpl); ((char*)(cmd))[_mpl] = '\0'; }
				stop++;
				*cmdId = FindSTRNIndex(EnrichedStrn,cmd);
			}
		}
		// some random text
		else
			*cmdId = enText;
	}
	*tStop = stop-enriched;
	return(noErr);
}

/**********************************************************************
 * 
 **********************************************************************/
void AddToStyle(EnrichedEnum cmd,bool neg,long offset,OffsetAndStyleHandle *styles,uLong valid)
{
	OffsetAndStyle current;
	short n;
	
	if (!styles || !*styles) return;
	
	n = HandleCount(*styles);
	
	/*
	 * fill in current style
	 */
	if (n==0)
	{
		Zero(current);
		current.style.tsSize = FontSize;
	}
	else
		current = (*styles)[n-1];
	
	/*
	 * modify
	 */
	if (neg && cmd==enBigger) cmd=enSmaller;
	else if (neg && cmd==enSmaller) cmd=enBigger;
	
	switch (cmd)
	{
		case enBold:
			current.validBits |= peBoldValid;
			if (neg) current.style.tsFace &= ~bold;
			else current.style.tsFace |= bold;
			break;
		case enItalic:
			current.validBits |= peItalicValid;
			if (neg) current.style.tsFace &= ~italic;
			else current.style.tsFace |= italic;
			break;
		case enUnderline:
			current.validBits |= peUnderlineValid;
			if (neg) current.style.tsFace &= ~underline;
			else current.style.tsFace |= underline;
			break;
		case enSmaller:
#ifdef USERELATIVESIZES
			if(current.style.tsSize < 0)
			{
				if(current.style.tsSize == kPETEDefaultSize)
					current.style.tsSize = kPETERelativeSizeBase;
				--current.style.tsSize;
			} else
#endif
			current.style.tsSize = IncrementTextSize(current.style.tsSize,-1);
			current.validBits |= peSizeValid;
			break;
		case enBigger:
#ifdef USERELATIVESIZES
			if(current.style.tsSize < 0)
			{
				if(current.style.tsSize == kPETEDefaultSize)
					current.style.tsSize = kPETERelativeSizeBase;
				++current.style.tsSize;
			} else
#endif
			current.style.tsSize = IncrementTextSize(current.style.tsSize,1);
			current.validBits |= peSizeValid;
			break;
		case enFixed:
		case enCenter:
		case enLeft:
		case enRight:
			return;
			break;
	}
	
	current.validBits &= valid;
	
	if (n && offset==current.offset) (*styles)[n-1] = current;
	else
	{
		current.offset = offset;
		void *newStyles = buf_append(*styles, &current, sizeof(current));
		if (!newStyles)
			free(*styles);
		else
			*styles = newStyles;
	}
}

/**********************************************************************
 * IncrementTextSize - make a font size smaller or larger
 **********************************************************************/
short IncrementTextSize(short size,short increment)
{
  short nSizes = HandleCount(StdSizes);
  short sizeIndex = FindSizeInc(size);

	sizeIndex += increment;
	sizeIndex = MIN(nSizes-1,sizeIndex);
	sizeIndex = MAX(0,sizeIndex);
	return(StdSizes[sizeIndex]);
}

/**********************************************************************
 * FindSizeInc - find which increment a size is
 **********************************************************************/
short FindSizeInc(short size)
{
  short nSizes = HandleCount(StdSizes);
	short *std;
	short sizeIndex;
	short tempSize;
	
	tempSize = size;
	if (tempSize < 0) tempSize=FontSize;
	
	sizeIndex = 0;
	for (std=StdSizes;std<StdSizes+nSizes;std++)
		if (tempSize>=*std)
		{
			sizeIndex = std-StdSizes;
			if (tempSize==*std) break;
		}
		else break;

#ifdef USERELATIVESIZES
	if ((size < 0) && (size != kPETEDefaultSize)) {
		sizeIndex += size - kPETERelativeSizeBase;
		if(sizeIndex < 0) sizeIndex = 0;
		if(sizeIndex >= nSizes) sizeIndex = nSizes - 1;
	}
#endif
	
	return(sizeIndex);
}

/**********************************************************************
 * SetMessRich - set the rich text flag for a message
 **********************************************************************/
bool SetMessRich(MessHandle messH)
{
	StyleLevelEnum result;
	HeadSpec hSpec;
	bool html = !PrefIsSet(PREF_SEND_ENRICHED_NEW) || MessOptIsSet(messH,OPT_HTML);
	
	if (!CompHeadFind(messH,0,&hSpec))
	{
		// No body.  Not good.
		result = false;
	}
	else
	{
		result=HasStyles(TheBody,hSpec.value,hSpec.stop,html);
	}
	
	if (result)
	{
		if (html)
		{
			SetMessOpt(messH,OPT_HTML);
			ClearMessFlag(messH,FLAG_RICH);
		}
		else
		{
			SetMessFlag(messH,FLAG_RICH);
			ClearMessOpt(messH,OPT_HTML);
		}
	}
	else
	{
		ClearMessFlag(messH,FLAG_RICH);
		ClearMessOpt(messH,OPT_HTML);
	}
	
	if (result==hasOnlyExcerpt && UseFlowOutExcerpt)
		SetMessOpt(messH,OPT_JUST_EXCERPT);
	else
		ClearMessOpt(messH,OPT_JUST_EXCERPT);

	return(result);
}

/**********************************************************************
 * HasStyles - does an edit record use multiple styles?
 **********************************************************************/
StyleLevelEnum HasStyles(PETEHandle pte,long from,long to,bool allowGraphics)
{
	PETEParaInfo defInfo, curInfo;
	PETEStyleEntry defStyle, curStyle;
	long para;
	long len;
	StyleLevelEnum result = hasNoStyle;
	short paraDiff;
		
	defInfo.tabHandle = curInfo.tabHandle = nil;
	PeteGetStyle(pte,kPETEDefaultStyle,nil,&defStyle);
	PETEGetParaInfo(PETE,pte,kPETEDefaultPara,&defInfo);
	
	for (;from<to;from=curStyle.psStartChar+MAX(1,len))
	{
		if (PETEGetParaIndex(PETE,pte,from,&para)) return(hasNoStyle);
		if (PETEGetParaInfo(PETE,pte,para,&curInfo)) return(hasNoStyle);
		if (paraDiff = PeteParaInfoDiff(&defInfo,&curInfo))
		{
			if (paraDiff==peQuoteLevelValid) result = hasOnlyExcerpt;
			else return(hasTonsOCrap);
		}
		if (PeteGetStyleLo(pte,from,&len,allowGraphics,&curStyle)) return(hasNoStyle);
		if (curStyle.psGraphic)
		{
			if (!IsEmoticonStyle(&curStyle.psStyle.graphicStyle)) return(hasTonsOCrap);	// graphics count
		}
		else
		{
			if (curStyle.psStyle.textStyle.tsLabel&pLinkLabel) return(hasTonsOCrap);	// links count
#ifdef WINTERTREE
			if (HasFeature (featureSpellChecking) && (curStyle.psStyle.textStyle.tsLabel&pSpellLabel)==pSpellLabel) continue; //skip spelling
#endif //WINTERTREE
			if (PeteTextStyleDiff(&defStyle.psStyle.textStyle,&curStyle.psStyle.textStyle)) return(hasTonsOCrap);
		}
	}
	return(result);
}

/************************************************************************
 * Style2String - encode a style in a string
 ************************************************************************/
char * Style2String(ESSPtr ess,char * string)
{
	char s[64];
	
	*string = 0;
	
	/*
	 * style name
	 */
	 
	// style name
	g_strlcpy((char *)(string), (char *)(ess->styleName), sizeof(string)); PCatC(string,',');
	
	/*
	 * text info
	 */
	
	// valid bits
	PXCat(string,ess->textValid); PCatC(string,',');
	
	// font name
	if ((ess->textValid && peFontValid) && ess->textStyle.tsFont!=kPETEDefaultFont)
	{
		GetFontName(ess->textStyle.tsFont,s);
		PCat(string,s);
	}
	PCatC(string,',');
	
	// font size
	if (ess->textValid & peFontValid)
	  PLCat(string,FindSizeInc(ess->textStyle.tsSize)-FindSizeInc(FontSize));
	PCatC(string,',');
	
	// style
	if (ess->textValid & peFaceValid)
	  PXWCat(string,ess->textStyle.tsFace);
	PCatC(string,',');
	
	// color
	if (ess->textValid & peColorValid)
	{
		PXWCat(string,ess->textStyle.tsColor.red);
		PCatC(string,',');
		PXWCat(string,ess->textStyle.tsColor.green);
		PCatC(string,',');
		PXWCat(string,ess->textStyle.tsColor.blue);
		PCatC(string,',');
	}
	else
	{
		PCatC(string,',');
		PCatC(string,',');
		PCatC(string,',');
	}
		
	// language
	if (ess->textValid & peLangValid) PLCat(string,ess->textStyle.tsLang);
	PCatC(string,',');
	
	// lock
	if (ess->textValid & peLockValid) PLCat(string,ess->textStyle.tsLang);
	PCatC(string,',');
	
	// do not save label
	
	/*
	 * paragraph info
	 */
	
	// valid bits
	PXWCat(string,ess->paraValid); PCatC(string,',');
	
	// margins
	if (ess->paraValid & peStartMarginValid) PLCat(string,ess->paraInfo.startMargin);
	PCatC(string,',');
	if (ess->paraValid & peEndMarginValid) PLCat(string,ess->paraInfo.endMargin);
	PCatC(string,',');
	if (ess->paraValid & peIndentValid) PLCat(string,ess->paraInfo.indent);
	PCatC(string,',');
	
	// direction
	if (ess->paraValid & peDirectionValid) PLCat(string,ess->paraInfo.direction);
	PCatC(string,',');
	
	// justification
	if (ess->paraValid & peJustificationValid) PLCat(string,ess->paraInfo.justification);
	PCatC(string,',');
	
	// quote level
	if (ess->paraValid & peQuoteLevelValid) PLCat(string,ess->paraInfo.quoteLevel);
	PCatC(string,',');
	
	// flags
	if (ess->paraValid & peFlagsValid) PXWCat(string,ess->paraInfo.paraFlags);
	PCatC(string,',');
	
	/*
	 * flags for us
	 */
	PLCat(string,ess->wholePara);
	PCatC(string,',');
	
	PLCat(string,ess->formatBar);
	PCatC(string,',');

	return string;
}

/************************************************************************
 * String2Style - decode a style from a string
 ************************************************************************/
int String2Style(ESSPtr ess,char * string)
{
	return unimpErr;
}
