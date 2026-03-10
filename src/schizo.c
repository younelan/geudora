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

#include "schizo.h"
#include "StringUtil.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "gtk_dialogs.h"
#include "fileutil.h"
#include "util.h"
#include "acap.h"
#include "features.h"
#include "ends.h"
#include "Globals.h"
#include "threading.h"
#include "imapmailboxes.h"
#include "message.h"
#include "comp.h"
#include "sendmail.h"
#include "pop.h"
#include "gtk_menus.h"
#define FILE_NUM 82
/* Copyright (c) 1996 by QUALCOMM Incorporated */
static int PersCompare(PersHandle *p1, PersHandle *p2);
static void UpdatePersList(void);

#pragma segment Main
#ifdef APPLE_EVENTS
typedef struct
{
	PropToken property;
	PersHandle pers;
} PersSpecifier;
#endif

/**********************************************************************
 * PersAnyPasswords - do we have any passwords?
 **********************************************************************/
bool PersAnyPasswords(void)
{
	PersHandle pers;
	
	for (pers=PersList;pers;pers=(*pers)->next)
		if (*(*pers)->password || *(*pers)->secondPass) return(True);
	return(False);
}
#pragma segment Schizo

/**********************************************************************
 * PersFillPw - grab passwords for a personality
 **********************************************************************/
OSErr PersFillPw(PersHandle pers,uint32_t whichOnes)
{
	Str255 s;
	OSErr err = noErr;
	Str127 uName, hName, persName;
	
	*s = 0;
	
#ifdef HAVE_KEYCHAIN
	if (PrefIsSet(PREF_KEYCHAIN))
	{
		err = FindPersKCPassword(pers,s,sizeof(s));
		if (err==userCanceledErr) return err;
		PSCopy((*pers)->password,s);
	}
#endif //HAVE_KEYCHAIN

	if (!*(*pers)->password)
	{
		GetPassStuff(persName,uName,hName);
		GetPassword(persName,uName,hName,s,sizeof(s),ENTER);
		(*pers)->dirty = true;
		PSCopy((*pers)->password,s);
		if (*s)
		{
			if (PrefIsSet(PREF_SAVE_PASSWORD)) PersSavePw(pers);
			PSCopy((*pers)->password,s);	// savepw wipes it out; put it back, for now
		}
	}
	
	return(*(*pers)->password ? noErr:userCanceledErr);
}

/**********************************************************************
 * PersSavePw - save passwords for a personality
 **********************************************************************/
OSErr PersSavePw(PersHandle pers)
{
	OSErr err = noErr;
	Str31 pass, secondPass;
	Str255 scratch;
	
#ifdef HAVE_KEYCHAIN
	if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN))
	{
		err = UpdatePersKCPassword(pers);
		WriteZero((*pers)->password,sizeof((*pers)->password));
	}
	else
#endif //HAVE_KEYCHAIN
	if (pers==PersList)
	{
		PSCopy(pass,(*PersList)->password);
		PSCopy(secondPass,(*PersList)->secondPass);
 		if (!EqualString(pass,GetPref(scratch,PREF_PASS_TEXT),True,True) ||
	     	!EqualString(secondPass,GetPref(scratch,PREF_AUXPW),True,True))
		{
			SetPref(PREF_PASS_TEXT,pass);
			SetPref(PREF_AUXPW,secondPass);
			MyUpdateResFile(SettingsRefN);
		}
	}
	else
	{
		err = PersSave(pers);
	}
	return(err);
}

/**********************************************************************
 * PersSaveAll - save all personalities
 **********************************************************************/
OSErr PersSaveAll(void)
{
	OSErr err=noErr;
	PersHandle pers;
	bool multiplePersonalities = false;
	
	for (pers=PersList;pers;pers=(*pers)->next)
		if (err=PersSave(pers)) break;
	return(err);
}
	

/**********************************************************************
 * 
 **********************************************************************/
OSErr PersSave(PersHandle pers)
{
	OSErr err = noErr;
	Str63 scratch;
	PersHandle oldCur = CurPers;
	Str63 oldPass;
	
	*oldPass = 0;
	
	if ((*pers)->dirty && (*pers)->resId)
	{
		// swap personality into current
		CurPers = pers;
		// make sure we don't unintentionally save the password
		if (!PrefIsSet(PREF_SAVE_PASSWORD))
		{
			PSCopy(oldPass,(*pers)->password);
			WriteZero(LDRef(pers)->password,sizeof((*pers)->password));
			UL(pers);
		}
#ifdef HAVE_KEYCHAIN
		else if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN))
		{
			WriteZero((*pers)->password,sizeof((*pers)->password));
		}
#endif //HAVE_KEYCHAIN

		// remove old, add new
		ZapResource(PERS_RTYPE,(*pers)->resId);
		AddMyResource((Handle)pers,PERS_RTYPE,(*pers)->resId,"");
		if (!(err=ResError()))
		{
			err = MyUpdateResFile(SettingsRefN);
			DetachResource((Handle)pers);
		}
		if (!err) (*pers)->dirty = False;
		else
		{
			PSCopy(scratch,(*pers)->name);
			Aprintf(OK_ALRT,err,PERS_SAVE_ERR,scratch);
		}
		
		// put stuff back
		CurPers = oldCur;
		if (*oldPass) PSCopy((*pers)->password,oldPass);
	}
	return(err);
}

/**********************************************************************
 * PersNew - create a new personality
 **********************************************************************/
PersHandle PersNew(void)
{
	PersHandle pers, newPers=nil;
	short n;
	short resEnd;
	Str63 untitled;

	UseFeature (featureMultiplePersonalities);
		
	// find an unused name
	for (n=PersCount()+1;;n++)
	{
		GetRString(untitled,UNTITLED);
		PLCat(untitled,n);
		if (!FindPersByName(untitled)) break;
	}
	
	// Allocate the last couple bytes of resource id's
	n = 0;
	do
	{
		n++;
		Bytes2Hex(((unsigned char *)&n)+1,1,(unsigned char *)&resEnd);
		for (pers=(*PersList)->next;pers;pers=(*pers)->next)
			if ((*pers)->resEnd==resEnd) break;
	}
	while (pers);
	
	// out of personalities?
	if (n>100) {WarnUser(YOU_ARE_PSYCHO,0);return(nil);}
	
	// create the personality
	if (newPers=NewZH(Personality))
	{
		PersSetName(newPers,untitled);
		n = MyUniqueID(PERS_RTYPE);
		(*newPers)->resId = n;
		(*newPers)->resEnd = resEnd;
		PersSave(newPers);
		LL_Queue(PersList,newPers,(PersHandle));
		PersZapResources('STR ',resEnd);
		PushPers(CurPers);
		CurPers = newPers;
		SetStrOverride(PREF3_STRN+PREF_STUPID_USER%100,"");
		SetStrOverride(PREF3_STRN+PREF_STUPID_HOST%100,"");
		PopPers();
		UpdatePersList();
	}
	else
		WarnUser(MEM_ERR,MemError());
	return(newPers);
}

/**********************************************************************
 * PersDelete - kill a personality
 **********************************************************************/
OSErr PersDelete(PersHandle pers)
{
	if (HasFeature (featureMultiplePersonalities))
	{
		if (pers==PersList) return(errAENotModifiable);
		UseFeature (featureMultiplePersonalities);
		
#ifdef HAVE_KEYCHAIN
		if (KeychainAvailable() && PrefIsSet(PREF_KEYCHAIN))
		{
			DeletePersKCPassword(pers);
		}
#endif //HAVE_KEYCHAIN
		
		LL_Remove(PersList,pers,(PersHandle));
		
		ZapSettingsResource(PERS_RTYPE,(*pers)->resId);
		PersZapResources('STR ',(*pers)->resEnd);
		UpdatePersList();
		AuditPersDelete((*pers)->persId);
	}
	return(noErr);
}


/**********************************************************************
 * PersZapResources - zap resources for a personality
 **********************************************************************/
void PersZapResources(OSType type,short resEnd)
{
	Handle res;
	
	// Kill old resources
	type = PERS_TYPE(type,resEnd);
	while (res=Get1IndResource(type,1)) {RemoveResource(res); if (!ResError()) DisposeHandle(res);}
}

/**********************************************************************
 * PersType - find an alternate type for a resource based on the personality
 **********************************************************************/
OSType PersType(OSType theType,PersHandle pers)
{
	if (pers && (*pers)->persId)
		theType = (theType&0xffff0000) | (*pers)->resEnd;
	return(theType);
}

/**********************************************************************
 * FindPersById - find a personality, given a personality id
 **********************************************************************/
PersHandle FindPersById(uint32_t persId)
{
	PersHandle pers;
	for (pers=PersList;pers;pers=(*pers)->next)
		if ((*pers)->persId==persId) break;
	
	return(pers);
}

/**********************************************************************
 * FindPersById - find a personality, given a personality name
 **********************************************************************/
PersHandle FindPersByName(PStr name)
{
	if (EqualStrRes(name,DOMINANT)) return(PersList);
	return(FindPersById(Hash(name)));
}

#pragma segment Ends
/**********************************************************************
 * InitPersonalities - read personalities into the personality queue
 **********************************************************************/
void InitPersonalities(void)
{
	PersHandle pers = NewZH(Personality);
	Str63 dom;
	uint32_t ticks = TickCount();
	long hash;
	short n;
	bool multiplePersonalities = false;
	
	/*
	 * first, the dominant personality
	 */
	if (!pers) DieWithError(MEM_ERR,MemError());
	LL_Queue(PersList,pers,(PersHandle));
	GetRString(dom,DOMINANT);
	PSCopy((*pers)->name,dom);
#ifdef	IMAP
	(*pers)->mailboxTree = 0;
	(*pers)->imapRefresh = 0;
#endif
	
	CurPers = pers;
	
	/*
	 * Next, any others
	 */
	if (HasFeature (featureMultiplePersonalities)) {
		for (n=1;pers=(PersHandle)Get1IndResource(PERS_RTYPE,n);n++)
		{
			if (!*pers || !GetHandleSize(pers))
			{
				RemoveResource(pers);
				if (!ResError()) n--;	// removed one, retry
				ZapHandle(pers);
			}
			else if ((*pers)->version > PERS_VERS)
				ReleaseResource((Handle)pers);
			else
			{
				DetachResource((Handle)pers);
				SetHandleBig((Handle)pers,sizeof(Personality));
				if (MemError()) DieWithError(MEM_ERR,MemError());
				(*pers)->next = nil;
				LL_Queue(PersList,pers,(PersHandle));
				(*pers)->dirty = False;
				(*pers)->checkTicks = 0;
				(*pers)->proxy = nil;
#ifdef	IMAP
				(*pers)->mailboxTree = 0;
				(*pers)->imapRefresh = 0;
#endif
				// The popd resources shouldn't be here if this is
				// an imap personality.  Kill them.
				if (IsIMAPPers(pers))
				{
					ZapResource(PERS_POPD_TYPE(pers),POPD_ID);
					ZapResource(PERS_POPD_TYPE(pers),DELETE_ID);
					ZapResource(PERS_POPD_TYPE(pers),FETCH_ID);
				}
				
				if (pers!=PersList)
				{
					PSCopy(dom,(*pers)->name);
					hash = Hash(dom);
					ASSERT(hash==(*pers)->persId);
					(*pers)->persId = hash;
					
					// make sure we commit this particular setting
				 PushPers(pers);
					GetPrefNoDominant(dom,PREF_SUBMISSION_PORT);
					if (!*dom) SetPref(PREF_SUBMISSION_PORT,NoStr);
					PopPers();					
				}
			}
		}
		if (n > 1)
			UseFeature (featureMultiplePersonalities);
		UpdatePersList();
	}

	CurPers = PersList;	// let's make sure, shall we?
	
	return;
}

/**********************************************************************
 * PushPers - push current personality on the stack and set
 **********************************************************************/
void PushPers(PersHandle newCur)
{
	if (PersStack || !StackInit(sizeof(PersHandle),&PersStack))
		StackPush(&CurPers,PersStack);
	if (newCur) CurPers = newCur;
}

/**********************************************************************
 * PopPers - pop the top personality off the stack and set
 **********************************************************************/
void PopPers(void)
{
	ASSERT(PersStack && (*PersStack)->elCount);
	if (PersStack) StackPop(&CurPers,PersStack);
}

/**********************************************************************
 * PersSetName - set the name of a personality
 **********************************************************************/
OSErr PersSetName(PersHandle pers,PStr name)
{
	uint32_t hash = Hash(name);
	PersHandle oldPers;
	
	if (oldPers=FindPersByName(name))
	{
		if (pers==oldPers) return(noErr);
		else return(WarnUser(USED_PERSONALITY,dupFNErr));
	}
	PSCopy((*pers)->name,name);
	if ((*pers)->persId)
		AuditPersRename((*pers)->persId,hash);
	else
		AuditPersCreate(hash);
	(*pers)->persId = hash;
	(*pers)->dirty = True;
	if (HasFeature (featureMultiplePersonalities)) {
		if (pers != PersList)
			UseFeature (featureMultiplePersonalities);
		UpdatePersList();
	}
	
	// update IMAP mailboxes with new pers id
	if (IsIMAPPers(pers))
		IMAPPersIDChanged(pers, (*pers)->mailboxTree);	
	
	return(noErr);
}

#ifdef APPLE_EVENTS
/**********************************************************************
 * SetPersProperty - set a personality's properties via ae's
 **********************************************************************/
OSErr SetPersProperty(AEDescPtr token,AEDescPtr descP)
{
	short err=noErr;
	Str255 str;
	PersSpecifier persSpec;
	
	AEGetDescData(token,&persSpec,sizeof(persSpec));
	switch (persSpec.property.propertyId)
	{
		case pName:
			if (HasFeature (featureMultiplePersonalities) && persSpec.pers!=PersList)
			{
				UseFeature (featureMultiplePersonalities);
				GetAEPStr(str,descP);
				PersSetName(persSpec.pers,str);
				NotifyPersonalitiesWin();
			}
			else
				return(errAENotModifiable);
			break;
		
		case formUniqueID:
			return(errAENotModifiable);
			break;
		
		default:
			err = errAENoSuchObject;
			break;
	}
	return(err);
}

/**********************************************************************
 * 
 **********************************************************************/
OSErr GetPersProperty(AEDescPtr token,AppleEvent *reply,long refCon)
{
#pragma unused(refCon)
	OSErr err;
	Str255 s;
	PersSpecifier persSpec;
	
	AEGetDescData(token,&persSpec,sizeof(persSpec));	
	switch (persSpec.property.propertyId)
	{
		case pName:
			err = AEPutPStr(reply,keyAEResult,PCopy(s,(*persSpec.pers)->name));
			break;
		case formUniqueID:
			err = AEPutLong(reply,keyAEResult,(*persSpec.pers)->persId);
			break;
		default:
			err = errAENoSuchObject;
			break;
	}
	return(err);
}

/**********************************************************************
 * AECreatePersonality - create a personality
 **********************************************************************/
OSErr AECreatePersonality(DescType theClass,AEDescPtr inContainer,AppleEvent *event, AppleEvent *reply)
{
	OSErr err;
	PersHandle pers;
	long n;
	AEDesc obj;
	
	if (!HasFeature (featureMultiplePersonalities))
		return (paramErr);
		
	UseFeature (featureMultiplePersonalities);
	
	NullADList(&obj,nil);
	
	n = PersCount()+1;
	
	if (pers = PersNew())
	{
		err = AEPersObj(pers,&obj);
		if (!err) err = AEPutParamDesc(reply,keyAEResult,&obj);
		if (!err) NotifyPersonalitiesWin();
	}
	else err = MemError() ? MemError() : errAEEventNotHandled;
	
	DisposeADList(&obj,nil);
	return(err);
}

/************************************************************************
 * AEPersObj - build an object for a personality
 ************************************************************************/
OSErr AEPersObj(PersHandle pers,AEDescPtr obj)
{
	OSErr err;
	long n;
	AEDesc desc, root;
	
	if (!HasFeature (featureMultiplePersonalities))
		return (paramErr);
	
	if (pers != PersList)
		UseFeature (featureMultiplePersonalities);

	NullADList(&desc,obj,&root,nil);
	
	n = Pers2Index(pers);
		
	if (!(err = AECreateDesc(typeChar,(*pers)->name+1,*(*pers)->name,&desc)))
	{
		err = CreateObjSpecifier(cEuPersonality,&root,formName,&desc,False,obj);
	}
	
	DisposeADList(&desc,&root,nil);
	if (err) DisposeADList(obj,nil);
	return(err);
}

/************************************************************************
 * AESetPers - set a personality to an object
 ************************************************************************/
OSErr AESetPers(TOCType * tocH,short sumNum,AEDescPtr descP)
{
	AEDesc token;
	OSErr err;
	PersHandle	pers;

	if (!HasFeature (featureMultiplePersonalities))
		return (paramErr);
	
	err = AEResolve(descP,kAEIDoMinimum,&token);
	if (err) return(err);
	if (token.descriptorType!=cEuPersonality) return(errAEWrongDataType);
	AEGetDescData(&token,&pers,sizeof(pers));
	if (pers != PersList)
		UseFeature (featureMultiplePersonalities);
	return SetPers(tocH,sumNum,pers,True);
}
#endif /* APPLE_EVENTS */


/**********************************************************************
 * PersCount - how many personalities?
 **********************************************************************/
long PersCount(void)
{
	long n=0;
	PersHandle pers;
	
	for (pers=PersList;pers;pers=(*pers)->next) n++;
	return(n);
}


/**********************************************************************
 * DisposePersonalities - kill the current personality list
 **********************************************************************/
void DisposePersonalities(void)
{
	PersHandle pers;
	
	while (pers=PersList)
	{
		LL_Remove(PersList,pers,(PersHandle));
		ZapHandle(pers);
	}
	CurPers = PersList = nil;
}

/**********************************************************************
 * SetPers - set the personality of a message
 **********************************************************************/
OSErr SetPers(TOCType * tocH,short sumNum,PersHandle pers,bool stationery)
{
	WindowPtr	messWinWP;
	MessHandle messH = tocH->sums[sumNum].messH;
	bool opened = messH==nil;
	Str255 addr;
	OSErr err = noErr;
	uint32_t sigId;
	ControlHandle cntl;
	bool redirected = (tocH->sums[sumNum].opts & OPT_REDIRECTED)!=0;
	
	if (tocH->sums[sumNum].persId==(*pers)->persId) return(noErr);	// nothing to do

	if (pers!=PersList)
		UseFeature (featureMultiplePersonalities);
	
	tocH->sums[sumNum].persId = (*pers)->persId;
	TOCSetDirty(tocH,true);
	
	SetBGColorsByPers(messH);
	
#ifdef THREADING_ON
	if (tocH->which==OUT && (tocH->sums[sumNum].state!=SENT && tocH->sums[sumNum].state!=BUSY_SENDING))
#else
	if (tocH->which==OUT && tocH->sums[sumNum].state!=SENT)
#endif
	{
		if (stationery && !redirected)  // if stationery not allowed, don't copy sig
		{
			PushPers(pers);
			sigId = SigValidate(GetPrefLong(PREF_SIGFILE));
			PopPers();
			SetSig(tocH,sumNum,sigId);
		}
		
		if (opened)
		{
			(void) OpenComp(tocH,sumNum,nil,nil,False,False);
			messH = tocH->sums[sumNum].messH;
		}
		if (!messH) return(errAENoSuchObject);
		messWinWP = GetMyWindowWindowPtr((*messH)->win);
		PushPers(pers);
		GetReturnAddr(addr,True);
		PeteCalcOff(TheBody);
		if (redirected)
			err = RedirectAnnotation(messH);
		else
			err = SetMessText(messH,FROM_HEAD,addr+1,*addr);

		if (stationery) ApplyDefaultStationery((*messH)->win,False,False);
		PeteKillUndo(TheBody);
		PopPers();
		if (opened)
		{
			err = !SaveComp((*messH)->win) ? err : True;
			CloseMyWindow(messWinWP);
		}
		else
		{
			if (HasFeature (featureMultiplePersonalities)) {
				CheckNone(GetMHandle(PERS_HIER_MENU));
				if (cntl=FindControlByRefCon((*messH)->win,'pers'))
				{
					short item = pers==PersList ? 1 : Pers2Index(pers)+2;
					if (item!=GetControlValue(cntl))
					{
						SetControlMaximum(cntl,PersCount()+1);
						SetControlValue(cntl,item);
					}
				}
			}
		}
	}
	return(err);
}

/**********************************************************************
 * SetBGColorsByPers - set the bg colors by the personality of a message
 **********************************************************************/
void SetBGColorsByPers(MessHandle messH)
{
	if (messH)
	{
		RGBColor color;

		PushPers(PERS_FORCE(MESS_TO_PERS(messH)));
		GetRColor(&color,BACK_COLOR);
		PopPers();
		if (!SAME_COLOR(color,(*messH)->win->backColor))
		{
			char css[128];
			GtkCssProvider *provider;
			(*messH)->win->backColor = color;
			snprintf(css, sizeof(css),
				"textview { background-color: rgb(%u,%u,%u); }",
				color.red >> 8, color.green >> 8, color.blue >> 8);
			provider = gtk_css_provider_new();
			gtk_css_provider_load_from_string(provider, css);
			if ((*messH)->bodyPTE)
				gtk_style_context_add_provider(
					gtk_widget_get_style_context((*messH)->bodyPTE),
					GTK_STYLE_PROVIDER(provider),
					GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			if ((*messH)->subPTE)
				gtk_style_context_add_provider(
					gtk_widget_get_style_context((*messH)->subPTE),
					GTK_STYLE_PROVIDER(provider),
					GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			g_object_unref(provider);
		}
	}
}

/**********************************************************************
 * PersCheckTicks - find the earliest check interval
 **********************************************************************/
uint32_t PersCheckTicks(void)
{
	PersHandle pers;
	uint32_t ticks=0;
	
	for (pers=PersList;pers;pers=(*pers)->next)
		if ((*pers)->autoCheck)
		{
			if (!ticks) ticks = (*pers)->checkTicks+(*pers)->ivalTicks;
			else ticks = MIN(ticks,(*pers)->checkTicks+(*pers)->ivalTicks);
		}
	return(ticks);
}

/**********************************************************************
 * PersSkipNextCheck - skip the next check.
 **********************************************************************/
void PersSkipNextCheck(void)
{
	PersHandle pers;
	uint32_t ticks=0;
	
	// Reset all mail checks that were about to happen
	for (pers=PersList;pers;pers=(*pers)->next)
		if ((*pers)->autoCheck)
		{
			// if this check was about to happen, pretend it did.
			if ((*pers)->checkTicks+(*pers)->ivalTicks <= TickCount()+45*60)
				(*pers)->checkTicks = TickCount();
		}
}

/**********************************************************************
 * PersSetAutoCheck - set the autocheck flag
 **********************************************************************/
void PersSetAutoCheck(void)
{
	long ival;
	Str255 s;
	bool is;
	
	PushPers(CurPers);
	for (CurPers=PersList;CurPers;CurPers=(*CurPers)->next)
	{
		if (PrefIsSet(PREF_AUTO_CHECK) && (ival=GetPrefLong(PREF_INTERVAL)) && *GetPOPPref(s))
		{
			(*CurPers)->autoCheck = True;
			(*CurPers)->ivalTicks = TICKS2MINS * ival;
		}
		else
		{
			(*CurPers)->autoCheck = False;
			(*CurPers)->ivalTicks = 0;
		}
		is = *GetPOPPref(s) && s[1]=='!';
		(*CurPers)->uupcIn = is?1:0;
		is = *GetPref(s,PREF_SMTP) && s[1]=='!';
		(*CurPers)->uupcOut = is?1:0;
	}
	PopPers();
}

/**********************************************************************
 * Pers2Index - return the index of a personality
 **********************************************************************/
short Pers2Index(PersHandle goalPers)
{
	short index=0;
	PersHandle pers;
	
	if (!goalPers) return(-1);
	for (pers=PersList;pers && pers!=goalPers;pers=(*pers)->next) index++;
	return(pers?index:-1);
}

/**********************************************************************
 * Index2Pers - return the indexed personality
 **********************************************************************/
PersHandle Index2Pers(short n)
{
	PersHandle pers;
	
	for (pers=PersList;n-- && pers;pers=(*pers)->next);
	return(pers);
}
	

/************************************************************************
 * CheckPers - put a check next to the current message's personality
 ************************************************************************/
void CheckPers(MyWindowPtr win,bool all)
{
	WindowPtr	winWP = GetMyWindowWindowPtr(win);
	MenuHandle mh;
	MessHandle messH;
	TOCType * tocH;
	short n;
	bool out;
	short kind;
	PersHandle pers, messPers;
	
	//No personalities menu in Light
	if (!HasFeature (featureMultiplePersonalities))
		return;
	
	mh = GetMHandle(PERS_HIER_MENU);
	messH = win?(MessHandle) GetMyWindowPrivateData(win):nil;
	tocH = win?(TOCType *) GetMyWindowPrivateData(win):nil;
	out=False;
	kind = winWP ? GetWindowKind(winWP) : 0;
	
	/*
	 * uncheck old one
	 */
	CheckNone(mh);
	EnableItem(mh,0);
	if (all) return;
	
	if (IsMyWindow(winWP))
	{
		/*
		 * set n to current state
		 */
		n = 0;
		if (kind==MESS_WIN || kind==COMP_WIN)
		{
			messPers = PERS_FORCE(MESS_TO_PERS(messH));
			out = kind==COMP_WIN;
		}
		else if ((kind==MBOX_WIN || kind==CBOX_WIN) && win->hasSelection)
		{
			messPers = PERS_FORCE(FindPersById(tocH->sums[FirstMsgSelected(tocH)].persId));
			out = kind==CBOX_WIN;
		}
		else
		{
			DisableItem(mh,0);
			return;
		}
		
		/*
		 * check the appropriate item
		 */
		if (messPers==PersList) n = 1;
		else
		{
			n = 3;
			for (pers=(*PersList)->next;pers;pers=(*pers)->next)
				if (pers==messPers) break;
				else n++;
		}
		SetItemMark(mh,n,checkMark);
	}
	CalcMenuSize(mh);
}

/************************************************************************
 * UpdatePersList - sort personalities and rebuild personalities menu
 ************************************************************************/
static void UpdatePersList(void)
{
	PersHandle pers,**hPersList;
	short	count;
	
	//No personalities menu in Light
	if (!HasFeature (featureMultiplePersonalities))
		return;
	
	count = PersCount();
	if (count <= 2)
		;	//	Not enough items to sort
	else if (hPersList=NuHandleClear((count+2)*sizeof(PersHandle)))
	{
		PersHandle	*pPersList;
		short				idx;
		
		pPersList=LDRef(hPersList);
		for (pers=PersList;pers;pers=(*pers)->next)
			*pPersList++ = pers;
		*pPersList = nil;	//	Used later for rebuilding linked list
		/* Sort personalities 1..count-1 (index 0 = dominant, stays first) */
		qsort((PersHandle*)*hPersList + 1, count - 1, sizeof(PersHandle),
			(int(*)(const void*, const void*))PersCompare);
		
		//	Rebuild personalities list
		pPersList=*hPersList;
		for(idx=0;idx<count;idx++)
			(*pPersList[idx])->next = pPersList[idx+1];
	}
	BuildPersMenu();
}

/************************************************************************
 * PersCompare - compare two personality names
 ************************************************************************/
static int PersCompare(PersHandle *p1, PersHandle *p2)
{
	if (*p1==0) return 1;
	if (*p2==0) return -1;
	return(StringComp((**p1)->name,(**p2)->name));
}





static void PersChooseResponse(GtkDialog *dialog, gint response, gpointer data)
{
	gint *out = (gint *)data;
	*out = response;
	GMainLoop *loop = (GMainLoop *)g_object_get_data(G_OBJECT(dialog), "modal-loop");
	if (loop) g_main_loop_quit(loop);
}

/************************************************************************
 * PersChoose - choose a personality from a GTK dialog list
 ************************************************************************/
PersHandle PersChoose(unsigned char *prompt)
{
	PersHandle pers, result = nil;
	long count, i;
	PersHandle *indexed;
	GtkWidget *dialog, *content, *scrolled, *listbox;
	gint response;
	GtkListBoxRow *selected;

	if (!HasFeature(featureMultiplePersonalities))
		return PersList;

	count = PersCount();
	if (count == 1) return PersList;

	UseFeature(featureMultiplePersonalities);

	dialog = gtk_dialog_new_with_buttons(
		prompt ? (const char *)(prompt + 1) : "Choose Personality",
		NULL,
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		"Cancel", GTK_RESPONSE_CANCEL,
		"OK", GTK_RESPONSE_OK,
		NULL);

	scrolled = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 200);

	listbox = gtk_list_box_new();
	gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listbox);

	indexed = (PersHandle *)g_new0(gpointer, count);
	i = 0;
	for (pers = PersList; pers; pers = (*pers)->next)
	{
		char name[33];
		unsigned int len = (unsigned int)(*pers)->name[0];
		if (len > 32) len = 32;
		memcpy(name, (*pers)->name + 1, len);
		name[len] = '\0';
		GtkWidget *label = gtk_label_new(name);
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_widget_set_margin_start(label, 6);
		gtk_list_box_append(GTK_LIST_BOX(listbox), label);
		indexed[i++] = pers;
	}

	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_append(GTK_BOX(content), scrolled);
	{
		GMainLoop *loop = g_main_loop_new(NULL, FALSE);
		g_object_set_data(G_OBJECT(dialog), "modal-loop", loop);
		g_signal_connect(dialog, "response", G_CALLBACK(PersChooseResponse), &response);
		gtk_widget_show(dialog);
		g_main_loop_run(loop);
		g_main_loop_unref(loop);
	}

	if (response == GTK_RESPONSE_OK)
	{
		selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(listbox));
		if (selected)
		{
			int idx = gtk_list_box_row_get_index(selected);
			if (idx >= 0 && idx < count)
				result = indexed[idx];
		}
	}

	g_free(indexed);
	gtk_window_destroy(GTK_WINDOW(dialog));
	return result;
}
