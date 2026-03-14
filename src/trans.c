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
specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
 * trans.c - ETL (Eudora Translation Layer) for the GTK port
 *
 * Ported from MAC624/trans.c. The MIME data structure functions
 * (NewTLMIME, AddTLMIME, etc.) are fully ported. The plugin
 * loading/execution functions currently have no plugins to load
 * (the Mac Component Manager API doesn't exist here), so they
 * return "no translators" errors to let the built-in MIME
 * handling run. When a dlopen-based plugin loader is added,
 * these functions will call into it.
 */

#include "trans.h"
#include "emsapi-mac.h"
#include "fileutil.h"
#include "mydefs.h"
#include "StringUtil.h"
#include "util.h"

#define FILE_NUM 75

#ifdef ETL

/*======================================================================
 * MIME data structure management — fully ported from original
 *====================================================================*/

/* Forward declaration */
void DisposeTLMIMEParam(emsMIMEParamHandle param);

/**********************************************************************
 * NewTLMIME - Allocate a tlMIME structure
 **********************************************************************/
OSErr NewTLMIME(emsMIMEHandle *tlMIME)
{
	if ((*tlMIME = NewZH(emsMIMEtype)))
		(**tlMIME)->size = sizeof(emsMIMEtype);
	return(MemError());
}

/**********************************************************************
 * DisposeTLMIME - dispose of an emsMIME structure
 **********************************************************************/
void DisposeTLMIME(emsMIMEHandle tlMIME)
{
	if (tlMIME)
	{
		if (*tlMIME)
		{
			DisposeTLMIMEParam((*tlMIME)->params);
			DisposeTLMIMEParam((*tlMIME)->contentParams);
		}
		ZapHandle(tlMIME);
	}
}

/**********************************************************************
 * DisposeTLMIMEParam - dispose of an emsMIME parameter list
 **********************************************************************/
void DisposeTLMIMEParam(emsMIMEParamHandle param)
{
	if (param)
	{
		if (*param)
		{
			if ((*param)->next) DisposeTLMIMEParam((*param)->next);
			ZapHandle((*param)->value);
		}
		ZapHandle(param);
	}
}

/**********************************************************************
 * AddTLMIME - add type, subtype, param, version, or content-disposition
 **********************************************************************/
OSErr AddTLMIME(emsMIMEHandle tlMIME, short what, PStr name, PStr value)
{
	UHandle h2=nil;
	emsMIMEParamHandle h3=nil;
	OSErr err = noErr;

	switch(what)
	{
		case TLMIME_TYPE:
			g_strlcpy((*tlMIME)->mimeType, (const char *)name, sizeof((*tlMIME)->mimeType));
			break;

		case TLMIME_SUBTYPE:
			g_strlcpy((*tlMIME)->subType, (const char *)name, sizeof((*tlMIME)->subType));
			break;

		case TLMIME_CONTENTDISP_PARAM:
		case TLMIME_PARAM:
		{
			size_t valueLen = value ? strlen((const char *)value) : 0;
			emsMIMEParamHandle scan;

			/* already in the list? */
			for (scan=(what==TLMIME_PARAM ? (*tlMIME)->params : (*tlMIME)->contentParams); scan; scan=(*scan)->next)
			{
				if (StringSame(LDRef(scan)->name, (const char *)name))
				{
					UL(scan);
					if (value && valueLen > 0)
					{
						PtrPlusHand(value, (Handle)(*scan)->value, (long)valueLen);
						if (MemError()) return(MemError());
					}
					return(noErr);
				}
				UL(scan);
			}

			/* not in the list — make a new one */
			h3 = NewZH(emsMIMEparam);
			h2 = value ? NuHTempBetter((long)valueLen) : nil;
			if (h3 && (!value || h2))
			{
				g_strlcpy((*h3)->name, (const char *)name, sizeof((*h3)->name));
				if (h2 && valueLen > 0)
				{
					BMD(value, *h2, valueLen);
					(*h3)->value = (void **)h2;
				}
				if (what==TLMIME_PARAM)
				{
					emsMIMEParamHandle *tail = &(*tlMIME)->params;
					while (*tail) tail = &(**tail)->next;
					*tail = h3;
				}
				else
				{
					emsMIMEParamHandle *tail = &(*tlMIME)->contentParams;
					while (*tail) tail = &(**tail)->next;
					*tail = h3;
				}
				h2 = nil;
				h3 = nil;
			}
			else
				err = MemError();
			break;
		}

		case TLMIME_VERSION:
			g_strlcpy((*tlMIME)->mimeVersion, (const char *)name, sizeof((*tlMIME)->mimeVersion));
			break;

		case TLMIME_CONTENTDISP:
			g_strlcpy((*tlMIME)->contentDisp, (const char *)name, sizeof((*tlMIME)->contentDisp));
			break;
	}

	ZapHandle(h2);
	ZapHandle(h3);
	return(err);
}

/**********************************************************************
 * FlattenTLMIME - serialize a tlMIME structure into a single handle
 **********************************************************************/
OSErr FlattenTLMIME(emsMIMEHandle tlMIME, FlatTLMIMEHandle *flat)
{
	Accumulator a;
	OSErr err;
	emsMIMEParamHandle p;
	emsMIMEtype localType;
	emsMIMEparam localParam;
	short len;

	if (!tlMIME || !(*tlMIME)->mimeType[0] || !(*tlMIME)->subType[0]) return(fnfErr);

	localType = **tlMIME;

	if (!(err=AccuInit(&a)))
	{
		unsigned char typeLen = (unsigned char)strlen(localType.mimeType);
		if (!(err=AccuAddPtr(&a, &typeLen, 1)))
		if (!(err=AccuAddPtr(&a, localType.mimeType, typeLen)))
		{
			unsigned char subLen = (unsigned char)strlen(localType.subType);
			if (!(err=AccuAddPtr(&a, &subLen, 1)))
			if (!(err=AccuAddPtr(&a, localType.subType, subLen)))
			{
				for (p=(*tlMIME)->params; !err && p; p=(*p)->next)
				{
					localParam = **p;
					unsigned char nameLen = (unsigned char)strlen(localParam.name);
					if (!(err=AccuAddPtr(&a, &nameLen, 1)))
					if (!(err=AccuAddPtr(&a, localParam.name, nameLen)))
					{
						len = (short)GetHandleSize((Handle)(*p)->value);
						if (!(err=AccuAddPtr(&a, (void*)&len, 2)))
							err = AccuAddHandle(&a, (UHandle)(*p)->value);
					}
				}
			}
		}
	}

	if (err) AccuZap(&a);
	else
	{
		AccuTrim(&a);
		*flat = (FlatTLMIMEHandle)a.data;
	}

	return(err);
}

/**********************************************************************
 * UnflattenTLMIME - deserialize a handle back into a tlMIME structure
 **********************************************************************/
OSErr UnflattenTLMIME(FlatTLMIMEHandle flat, emsMIMEHandle *tlMIME)
{
	OSErr err = noErr;
	unsigned char name[64];
	unsigned char value[256];
	long offset=0;
	long len;
	short paramLen;

	if ((err = NewTLMIME(tlMIME))) return(err);

	len = GetHandleSize((Handle)flat);

	/* type */
	if (!err && offset<len)
	{
		unsigned char slen = ((unsigned char *)*flat)[offset];
		offset++;
		if (slen > 63) slen = 63;
		memcpy(name, (unsigned char *)*flat+offset, slen);
		name[slen] = '\0';
		offset += slen;
		err = AddTLMIME(*tlMIME, TLMIME_TYPE, name, nil);
	}
	else err = fnfErr;

	/* subtype */
	if (!err && offset<len)
	{
		unsigned char slen = ((unsigned char *)*flat)[offset];
		offset++;
		if (slen > 63) slen = 63;
		memcpy(name, (unsigned char *)*flat+offset, slen);
		name[slen] = '\0';
		offset += slen;
		err = AddTLMIME(*tlMIME, TLMIME_SUBTYPE, name, nil);
	}
	else err = fnfErr;

	/* params */
	while (!err && offset<len)
	{
		unsigned char slen = ((unsigned char *)*flat)[offset];
		offset++;
		if (slen > 63) slen = 63;
		memcpy(name, (unsigned char *)*flat+offset, slen);
		name[slen] = '\0';
		offset += slen;
		if (offset+2<=len)
		{
			paramLen = (((unsigned short)((unsigned char *)*flat)[offset])<<8)|((unsigned char *)*flat)[offset+1];
			if (paramLen > 255) paramLen = 255;
			memcpy(value, (unsigned char *)*flat+offset+2, paramLen);
			value[paramLen] = '\0';
			offset += paramLen+2;
			err = AddTLMIME(*tlMIME, TLMIME_PARAM, name, value);
		}
		else err = fnfErr;
	}

	if (err) ZapTLMIME(*tlMIME);
	return(err);
}

/*======================================================================
 * Address list management — ported from original
 *====================================================================*/

/**********************************************************************
 * ETLBuildAddrList - build a translation address list
 *
 * Full implementation requires SuckAddresses/ExpandAliases.
 * For now returns an empty but valid address list.
 **********************************************************************/
int ETLBuildAddrList(void **textIn, void **moreHeaders, HeaderDHandle hdh,
                     emsHeaderDataP addrList, short context)
{
	WriteZero(addrList, sizeof(emsHeaderData));
	addrList->size = sizeof(emsHeaderData);
	return(noErr);
}

/**********************************************************************
 * ETLDisposeAddrList - destroy a translation address list
 **********************************************************************/
static void ZapAddress(emsAddressH addr)
{
	emsAddressH next;

	while (addr)
	{
		if ((*addr)->address) DisposeHandle((Handle)(*addr)->address);
		if ((*addr)->realname) DisposeHandle((Handle)(*addr)->realname);
		next = (*addr)->next;
		DisposeHandle((Handle)addr);
		addr = next;
	}
}

void ETLDisposeAddrList(emsHeaderDataP addrList)
{
	if (addrList)
	{
		if (addrList->to) ZapAddress(*addrList->to);
		if (addrList->from) ZapAddress(*addrList->from);
		if (addrList->cc) ZapAddress(*addrList->cc);
		if (addrList->bcc) ZapAddress(*addrList->bcc);
		ZapHandle(addrList->subject);
		ZapHandle(addrList->rawHeaders);
	}
}

/*======================================================================
 * Plugin system — no plugins loaded yet (needs dlopen-based loader)
 *
 * These functions return "no translators" so the built-in MIME
 * handling in mime.c runs instead of trying to delegate to plugins.
 *====================================================================*/

/**********************************************************************
 * ETLInit - Initialize translator plugin system
 *
 * TODO: Scan plugin directory, dlopen each .so/.dylib, call
 * ems_plugin_init on each to register translators.
 **********************************************************************/
int ETLInit(void)
{
	/* No plugins loaded yet */
	return fnfErr;
}

/**********************************************************************
 * ETLCleanup - Shut down translator plugin system
 **********************************************************************/
void ETLCleanup(void)
{
	/* Nothing to clean up — no plugins loaded */
}

/**********************************************************************
 * ETLListAllTranslatorsLo - List available translators for a context
 **********************************************************************/
int ETLListAllTranslatorsLo(TLMHandle *translators, short context,
                            ModeTypeEnum forMode)
{
	if (translators) *translators = nil;
	return fnfErr; /* no translators */
}

/**********************************************************************
 * ETLCountTranslatorsLo - Count available translators
 **********************************************************************/
int ETLCountTranslatorsLo(short context, ModeTypeEnum forMode)
{
	return 0; /* zero translators */
}

/**********************************************************************
 * ETLCanTranslate - Check if any translator can handle this MIME type
 **********************************************************************/
int ETLCanTranslate(TLMHandle translators, short context,
                    emsMIMEHandle emsMIME, tlStringHandle *errorStr,
                    long *errCode, emsHeaderDataP addrList,
                    HeaderDHandle hdh)
{
	return -1; /* no translators available */
}

/**********************************************************************
 * ETLRemoveDeadTranslators - Remove translators that can't translate
 **********************************************************************/
int ETLRemoveDeadTranslators(TLMHandle translators)
{
	return 0;
}

/**********************************************************************
 * ETLInterpretFile - Interpret a MIME file using a translator
 *
 * Returns error so mime.c falls through to built-in MIME handling.
 **********************************************************************/
int ETLInterpretFile(short context, FSSpecPtr source, short resultRefN,
                     AccuPtr resultAcc, emsHeaderDataP addrList,
                     bool *dontSave)
{
	return fnfErr; /* no translators — use built-in handling */
}

/**********************************************************************
 * ETLDoAbout - Show translator about dialog
 **********************************************************************/
void ETLDoAbout(void)
{
}

/**********************************************************************
 * RecordTLID - Save translator ID to a file
 **********************************************************************/
int RecordTLID(FSSpecPtr spec, uLong id)
{
	return noErr; /* nothing to record without resource forks */
}

/**********************************************************************
 * TransRecvLine - Receive a line for translator use
 **********************************************************************/
int TransRecvLine(TransStream stream, UPtr line, long *size)
{
	return -1;
}

/**********************************************************************
 * ETLDisplayFile - Display a translated file
 **********************************************************************/
int ETLDisplayFile(FSSpecPtr spec, PETEHandle pte)
{
	return fnfErr;
}

/**********************************************************************
 * ETLAddIcons - Add translator icons to a window
 **********************************************************************/
int ETLAddIcons(MyWindowPtr win, short startNumber)
{
	return 0;
}

/**********************************************************************
 * ETLIconToID - Convert icon index to translator ID
 **********************************************************************/
long ETLIconToID(short which)
{
	return 0;
}

/**********************************************************************
 * ETLIconToDescriptions - Get descriptions for a translator icon
 **********************************************************************/
int ETLIconToDescriptions(short which, unsigned char *module,
                          unsigned char *translator)
{
	if (module) module[0] = '\0';
	if (translator) translator[0] = '\0';
	return fnfErr;
}

/**********************************************************************
 * ETLIDToIndex - Convert translator ID to list index
 **********************************************************************/
short ETLIDToIndex(long id)
{
	return -1;
}

/**********************************************************************
 * ETLID - Get combined module/translator ID
 **********************************************************************/
long ETLID(TLMHandle tl, short index)
{
	return 0;
}

/**********************************************************************
 * ETLIDToFileIcon - Get file icon for a translator
 **********************************************************************/
int ETLIDToFileIcon(long id, void ***suite)
{
	if (suite) *suite = nil;
	return fnfErr;
}

/**********************************************************************
 * ETLReadTL - Read translator info from a file
 **********************************************************************/
int ETLReadTL(FSSpecPtr spec, long *id)
{
	if (id) *id = 0;
	return fnfErr;
}

/**********************************************************************
 * ETLExists - Are any translators loaded?
 **********************************************************************/
bool ETLExists(void)
{
	return false;
}

/* ETLSendMessage — defined in mailxfer.c */

/**********************************************************************
 * ETLCanTransOut - Can any translator handle outgoing translation?
 **********************************************************************/
int ETLCanTransOut(MessHandle messH)
{
	return fnfErr;
}

/**********************************************************************
 * ETLTransOut - Translate an outgoing message
 **********************************************************************/
int ETLTransOut(MessHandle messH, emsMIMEHandle emsMIME, FSSpecPtr from,
                FSSpecPtr to)
{
	return fnfErr;
}

/**********************************************************************
 * ETLTransSelection - Translate a text selection
 **********************************************************************/
int ETLTransSelection(PETEHandle pte, HSPtr hs, short item)
{
	return fnfErr;
}

/**********************************************************************
 * ETLSpecial - Handle special translator menu item
 **********************************************************************/
int ETLSpecial(short item)
{
	return fnfErr;
}

/**********************************************************************
 * ETLEnableSpecialItems - Enable/disable special menu items
 **********************************************************************/
void ETLEnableSpecialItems(void)
{
}

/**********************************************************************
 * ETLAttach - Attach via translator
 **********************************************************************/
int ETLAttach(short item, MyWindowPtr win)
{
	return fnfErr;
}

/**********************************************************************
 * ETLDoSettings - Show translator settings
 **********************************************************************/
int ETLDoSettings(short item)
{
	return fnfErr;
}

/**********************************************************************
 * ETLNameAndIcon - Get name and icon for a translator
 **********************************************************************/
void ETLNameAndIcon(short i, unsigned char *name, void ***suite)
{
	if (name) name[0] = '\0';
	if (suite) *suite = nil;
}

/**********************************************************************
 * ETLSelect - Select/deselect a translator
 **********************************************************************/
int ETLSelect(short which, bool selecting, MessHandle messH)
{
	return fnfErr;
}

/**********************************************************************
 * ETLGetSystemPlugins - Discover system-level plugins
 **********************************************************************/
void ETLGetSystemPlugins(void)
{
}

/* ETLIdle — defined in mailxfer.c */

/**********************************************************************
 * GetCurrentPayMode - Get current payment mode
 *
 * Eudora had paid/free/adware modes. For the open-source port
 * we're always in "paid" (full feature) mode.
 **********************************************************************/
ModeTypeEnum GetCurrentPayMode(void)
{
	return kModePro;
}

/**********************************************************************
 * ETLEudoraModeNotification - Notify translators of mode change
 **********************************************************************/
void ETLEudoraModeNotification(ModeEventEnum modeEvent, ModeTypeEnum newMode)
{
}

/**********************************************************************
 * ETLDrawBoxTag - Draw mailbox translator tags
 **********************************************************************/
void ETLDrawBoxTag(TOCType *tocH, Rect *r)
{
}

/**********************************************************************
 * ETLAddBoxButtons - Add translator buttons to mailbox
 **********************************************************************/
void ETLAddBoxButtons(TOCType *tocH)
{
}

/**********************************************************************
 * ETLButtonHit - Handle translator button click
 **********************************************************************/
void ETLButtonHit(MyWindowPtr win, short item)
{
}

/**********************************************************************
 * ETLClickContextMenu - Handle translator context menu click
 **********************************************************************/
bool ETLClickContextMenu(MyWindowPtr win, Point pt, Rect *rSizeBox)
{
	return false;
}

/**********************************************************************
 * ETLHasMBoxContextFolder - Does mailbox have translator context folder?
 **********************************************************************/
bool ETLHasMBoxContextFolder(MyWindowPtr win)
{
	return false;
}

/**********************************************************************
 * ETLMBoxContextFolder - Get translator context folder
 **********************************************************************/
short ETLMBoxContextFolder(MyWindowPtr win, short *vRefNum, long *dirID)
{
	return fnfErr;
}

/**********************************************************************
 * ETLGetPluginFolderSpec - Get the plugin folder path
 *
 * TODO: Return ~/.eudora/plugins/ or similar
 **********************************************************************/
int ETLGetPluginFolderSpec(FSSpec *spec, short nameId)
{
	return fnfErr;
}

/**********************************************************************
 * ETLMenu2Icon - Get icon for a translator menu item
 **********************************************************************/
void **ETLMenu2Icon(short menu, short item)
{
	return nil;
}

/**********************************************************************
 * ETLAddToToolbar - Add translator items to toolbar
 **********************************************************************/
void ETLAddToToolbar(void)
{
}

/**********************************************************************
 * ETLBoxTagWidth - Get width of translator box tag
 **********************************************************************/
short ETLBoxTagWidth(MyWindowPtr win)
{
	return 0;
}

/**********************************************************************
 * ETLQueueMessage - Queue a message for translator processing
 **********************************************************************/
short ETLQueueMessage(MessHandle messH)
{
	return fnfErr;
}

/**********************************************************************
 * ETLDrain - Drain translator processing queue
 **********************************************************************/
long ETLDrain(void)
{
	return 0;
}

/*======================================================================
 * Plugin window support
 *====================================================================*/

bool IsPlugwindow(GtkWindow *theWindow)
{
	return false;
}

bool IsModalPlugwindow(GtkWindow *theWindow)
{
	return false;
}

bool IsNonModalPlugwindow(GtkWindow *theWindow)
{
	return false;
}

bool PlugwindowEventFilter(GdkEvent *event)
{
	return false;
}

void PlugwindowEnable(GtkWindow *theWindow, long *flags)
{
}

bool PlugwindowMenu(GtkWindow *theWindow, long select)
{
	return false;
}

bool PlugwindowClose(GtkWindow *theWindow)
{
	return false;
}

int32_t PlugwindowDrag(GtkWindow *theWindow, GdkDragAction action,
                       GdkDrag *drag)
{
	return 0;
}

void PlugwindowSendFakeEvent(GtkWindow *theWindow, uint32_t message,
                             GdkEventType what, GdkModifierType modifiers,
                             double x, double y)
{
}

void PlugwindowActivate(GtkWindow *theWindow, bool active)
{
}

void PlugwindowUpdate(GtkWindow *theWindow)
{
}

/*======================================================================
 * Import system
 *====================================================================*/

int ETLImport(long id, ImportOperationEnum what, void *params, void *results)
{
	return fnfErr;
}

int ETLQueryImporters(ImportAccountInfoH *results, long id, bool search)
{
	if (results) *results = nil;
	return fnfErr;
}

void **GetImporterAppIcon(long id)
{
	return nil;
}

void GetImporterName(long id, Str255 name)
{
	if (name) name[0] = '\0';
}

int ETLImportSignatures(ImportAccountInfoP account)
{
	return fnfErr;
}

int ETLImportAddresses(ImportAccountInfoP account)
{
	return fnfErr;
}

int ETLImportMail(ImportAccountInfoP account)
{
	return fnfErr;
}

int ETLImportSettings(ImportAccountInfoP account, ImportPersDataH *persData)
{
	if (persData) *persData = nil;
	return fnfErr;
}

/*======================================================================
 * Junk mail scoring
 *====================================================================*/

int ETLScoreJunk(TLMPtr thePlugin, emsTranslatorP transInfo,
                 emsJunkInfoP junkInfo, emsMessageInfoP message,
                 emsJunkScoreP junkScore, emsResultStatusP status)
{
	return fnfErr; /* no junk scoring plugins */
}

int ETLMarkJunk(TLMPtr thePlugin, emsTranslatorP transInfo,
                emsJunkInfoP junkInfo, emsMessageInfoP message,
                emsJunkScoreP junkScore, emsResultStatusP status)
{
	return fnfErr; /* no junk marking plugins */
}

/*======================================================================
 * Translator recording for messages
 *====================================================================*/

void AddTranslatorsFromPtr(MessHandle messH, unsigned char *text, long len)
{
}

void WriteTranslators(MessHandle messH)
{
}

#endif /* ETL */
