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

/* Copyright (c) 1995 by QUALCOMM Incorporated */
#ifndef TRANS_H
#define TRANS_H


#if defined(__has_include) && __has_include(<gtk/gtk.h>)
#include <gtk/gtk.h>
#endif
/* GTK types stubbed for non-GTK builds */
#ifndef GTK_MAJOR_VERSION
typedef void GtkWindow;
typedef void GdkEvent;
typedef void GdkDrag;
typedef int GdkDragAction;
typedef int GdkEventType;
typedef int GdkModifierType;
#endif

#include "TransStream.h"
#include "mailbox.h"
#include "mydefs.h"

/* EMS API types forward declarations */
typedef struct emsMIMEtype emsMIMEtype;
typedef struct emsMIMEparam emsMIMEparam;

/* Import types forward declarations */
typedef struct ImportAccountInfo *ImportAccountInfoP, *ImportAccountInfoH;
typedef struct ImportPersData *ImportPersDataH;

/*
 * Translator description structure
 */
typedef struct TLMaster {
  void *nameHandle;
  short module;
  short id;
  long flags;
  long type;
  short menuItem;
  int result;
  void **suite;
} TLMaster, *TLMPtr, *TLMHandle;

typedef emsMIMEtype *emsMIMEHandle;
typedef emsMIMEparam *emsMIMEParamHandle;
typedef void *FlatTLMIMEHandle;
typedef unsigned char *tlStringHandle;
typedef void *tlBufferHandle;
typedef struct mstruct *MessHandle;
#include "pete_portable.h"
/* PETEHandle is defined in pete_portable.h */
typedef struct HeadSpec *HSPtr;
typedef struct HeaderDesc *HeaderDPtr, *HeaderDHandle;

/* Forward declarations for EMS plugin types */
typedef struct emsHeaderData *emsHeaderDataP;
typedef struct emsTranslator *emsTranslatorP;
typedef struct emsJunkInfo *emsJunkInfoP;
typedef struct emsMessageInfo *emsMessageInfoP;
typedef struct emsJunkScore *emsJunkScoreP;
typedef struct emsResultStatus *emsResultStatusP;
typedef unsigned long uLong;
typedef enum {
  kModePayment,
  kModeFree,
  kModeAdWare,
  kModeSponsored,
  kModeLight,
  kModePro
} ModeTypeEnum;
typedef enum { kModeStarting, kModeStopping } ModeEventEnum;
typedef enum {
  kImportQueryOperation,
  kImportSettingsOperation,
  kImportSignaturesOperation,
  kImportAddressesOperation,
  kImportMailOperation
} ImportOperationEnum;

#define TOOLBAR_ICON_RTYPE 'TIcn'
#define TOOLBAR_ICON_RTYPE 'TIcn'
#define TOOLBAR_ICON_ID 1001
#define NO_TABLE 0
#define DEFAULT_TABLE 0

int ETLInit(void);     // Initialize the whole darn thing, including UI
void ETLCleanup(void); // Shut it all down
int ETLListAllTranslatorsLo(TLMHandle *translators, short context,
                            ModeTypeEnum forMode);
int ETLCountTranslatorsLo(short context, ModeTypeEnum forMode);
#define ETLListAllTranslators(translators, context)                            \
  ETLListAllTranslatorsLo(translators, context, GetCurrentPayMode())
#define ETLCountTranslators(context)                                           \
  ETLCountTranslatorsLo(context, GetCurrentPayMode())
int ETLCanTranslate(TLMHandle translators, short context, emsMIMEHandle emsMIME,
                    tlStringHandle *errorStr, long *errCode,
                    emsHeaderDataP addrList, HeaderDHandle hdh);
int ETLRemoveDeadTranslators(TLMHandle translators);
int ETLInterpretFile(short context, char * source, short resultRefN,
                     AccuPtr resultAcc, emsHeaderDataP addrList,
                     bool *dontSave);
void ETLDoAbout(void);
enum {
  TLMIME_TYPE,
  TLMIME_SUBTYPE,
  TLMIME_PARAM,
  TLMIME_VERSION,
  TLMIME_CONTENTDISP,
  TLMIME_CONTENTDISP_PARAM
};
int NewTLMIME(emsMIMEHandle *emsMIME);
void DisposeTLMIME(emsMIMEHandle emsMIME);
#define ZapTLMIME(m)                                                           \
  do {                                                                         \
    DisposeTLMIME(m);                                                          \
    m = nil;                                                                   \
  } while (0)
int RecordTLID(char * spec, uLong id);
int AddTLMIME(emsMIMEHandle emsMIME, short what, char *name,
              char *value);
int FlattenTLMIME(emsMIMEHandle emsMIME, FlatTLMIMEHandle *flat);
int UnflattenTLMIME(FlatTLMIMEHandle flat, emsMIMEHandle *tlMIME);
int TransRecvLine(TransStream stream, unsigned char * line, long *size);
int ETLDisplayFile(char * spec, PETEHandle pte);
int ETLAddIcons(MyWindowPtr win, short startNumber);
long ETLIconToID(short which);
int ETLIconToDescriptions(short which, unsigned char *module,
                          unsigned char *translator);
short ETLIDToIndex(long id);
short ETLSendMessage(TransStream stream, MessHandle messH, bool chatter,
                     bool sendDataCmd);
int ETLCanTransOut(MessHandle messH);
int ETLTransOut(MessHandle messH, emsMIMEHandle emsMIME, char * from,
                char * to);
int ETLTransSelection(PETEHandle pte, HSPtr hs, short item);
long ETLID(TLMHandle tl, short index);
int ETLIDToFileIcon(long id, void ***suite);
int ETLReadTL(char * spec, long *id);
bool ETLExists(void);
int ETLSpecial(short item);
void ETLEnableSpecialItems();
int ETLAttach(short item, MyWindowPtr win);
int ETLDoSettings(short item);
void ETLNameAndIcon(short i, unsigned char *name, void ***suite);
int ETLSelect(short which, bool selecting, MessHandle messH);
void ETLGetSystemPlugins(void);
int ETLBuildAddrList(void **textIn, void **moreHeaders, HeaderDHandle hdh,
                     emsHeaderDataP addrList, short context);
void ETLDisposeAddrList(emsHeaderDataP addrList);
void ETLIdle(long flags);
ModeTypeEnum GetCurrentPayMode();
void ETLEudoraModeNotification(ModeEventEnum modeEvent, ModeTypeEnum newMode);
void ETLDrawBoxTag(TOCType * tocH, Rect *r);
void ETLAddBoxButtons(TOCType * tocH);
void ETLButtonHit(MyWindowPtr win, short item);
bool ETLClickContextMenu(MyWindowPtr win, Point pt, Rect *rSizeBox);
bool ETLHasMBoxContextFolder(MyWindowPtr win);
short ETLMBoxContextFolder(MyWindowPtr win, short *vRefNum, long *dirID);
int ETLGetPluginFolderSpec(char *spec, short nameId);
void **ETLMenu2Icon(short menu, short item);
void ETLAddToToolbar(void);
short ETLBoxTagWidth(MyWindowPtr win);
short ETLQueueMessage(MessHandle messH);
long ETLDrain(void);

/* Plugin window support - GTK port */
bool IsPlugwindow(GtkWindow *theWindow);
bool IsModalPlugwindow(GtkWindow *theWindow);
bool IsNonModalPlugwindow(GtkWindow *theWindow);
bool PlugwindowEventFilter(GdkEvent *event);
void PlugwindowEnable(GtkWindow *theWindow, long *flags);
bool PlugwindowMenu(GtkWindow *theWindow, long select);
bool PlugwindowClose(GtkWindow *theWindow);
int32_t PlugwindowDrag(GtkWindow *theWindow, GdkDragAction action,
                       GdkDrag *drag);
void PlugwindowSendFakeEvent(GtkWindow *theWindow, uint32_t message,
                             GdkEventType what, GdkModifierType modifiers,
                             double x, double y);
void PlugwindowActivate(GtkWindow *theWindow, bool active);
void PlugwindowUpdate(GtkWindow *theWindow);
int ETLImport(long id, ImportOperationEnum what, void *params, void *results);
int ETLQueryImporters(ImportAccountInfoH *results, long id, bool search);
void **GetImporterAppIcon(long id);
void GetImporterName(long id, char name[256]);
int ETLImportSignatures(ImportAccountInfoP account);
int ETLImportAddresses(ImportAccountInfoP account);
int ETLImportMail(ImportAccountInfoP account);
int ETLImportSettings(ImportAccountInfoP account, ImportPersDataH *persData);
int ETLScoreJunk(TLMPtr thePlugin,
                 emsTranslatorP transInfo, /* In: Translator Info */
                 emsJunkInfoP junkInfo,    /* In: Junk information */
                 emsMessageInfoP message,  /* In: Message to score */
                 emsJunkScoreP junkScore,  /* Out: Junk score */
                 emsResultStatusP status   /* Out: Status information */
);

int ETLMarkJunk(TLMPtr thePlugin,
                emsTranslatorP transInfo, /* In: Translator Info */
                emsJunkInfoP junkInfo,    /* In: Junk information */
                emsMessageInfoP message,  /* In: Message to score */
                emsJunkScoreP junkScore,  /* Out: Junk score */
                emsResultStatusP status   /* Out: Status information */
);

/* TransVector definition presumed in mydefs.h */
extern TransVector CurTrans;

#endif
