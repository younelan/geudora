#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include "mailbox.h"
#include "schizo.h"
#include "gtk_prefs.h"

long ActiveTicks = 0;
int AddInlineSig(void* messH) {
  return 0;
}
/* AddSpecToList — real implementation elsewhere */
int AddTLMIME(void* emsMIME, short what, unsigned char *name, unsigned char *value) {
  return 0;
}
void AddXfUndo(void* tocH, void* trashTOC, int unused) {
}
void AdjustSpecialMenuItem() {}
short AliasRefCount = 0;
struct AliasDStruct **Aliases = NULL;
long AnyTOCDirty = 0;
void AppendMenu() {}
FSSpec AttFolderSpec = {0};
/* AttIsSelected — real implementation elsewhere */
/* AttLine2Spec — real implementation elsewhere */
/* AttachOptNumber — macro in compact.h */
/* AttachSelect — real implementation in compact.c */
void AuditHit() {}
void AuditPersCreate(uint32_t hash) {
}
void AuditPersDelete(uint32_t persId) {
}
void AuditPersRename(uint32_t oldId, uint32_t newHash) {
}
bool AutoCheckOK() {
  return false;
}
/* BeenThereDoneThat — real implementation elsewhere */
void BeginPGP(void* pgpc) {
}
/* BoxCenterSelection — real implementation elsewhere */
void *BoxCount = NULL; /* void ** Handle */
/* BoxFOpen — real implementation in mailbox.c */
struct BoxMapStruct **BoxMap = NULL;
void BoxPreviewProfile() {}
/* BoxSelectAfter — real implementation elsewhere */
/* BoxSetSummarySelected — real implementation elsewhere */
void *BoxWidths = NULL; /* short ** Handle */
short BugFlags = 0;
/* BuildEnriched — real implementation elsewhere */
void BuildHTML() {}
void BuildStationeryList() {
}
void CacheRecentNickname(unsigned char *name) {
}
void Cell1Rect() {}
/* CheckAddNotifyControls — real implementation elsewhere */
bool CheckNow = false; /* global variable, not a function */
Byte CheckOnIdle = 0;     /* global variable, not a function */
/* CheckThreadError — real implementation elsewhere */
/* CheckThreadRunning — real implementation elsewhere */
void ClearPrefBit() {}
/* CloseMyWindow — real inline in legacy_shim.h */
/* CompAttachSpec — real implementation in compact.c */
/* CompDelAttachment — real implementation in compact.c */
/* CompGatherRecipientAddresses — real implementation in compact.c */
void CompGetMID() {}
int CompHeadAppendPtr(void* pte, void* hSpec, char *text, long size) {
  return 0;
}
void* CompHeadFindStr(void* messH, char *name, void* hSpec) {
  return NULL;
}
int CompHeadGetStrLo(void* messH, short index, char *string, short size) {
  return 0;
}
int CompHeadPrependPtr(void* pte, void* hSpec, char *text, long size) {
  return 0;
}
int CompHeadSet(void* pte, void* hSpec, char *text) {
  return 0;
}
int CompHeadSetPtr(void* pte, void* hSpec, char *text, long size) {
  return 0;
}
int ConConMess(void* messH, void* pte, void *profile, void *a, void *b) {
  return 0;
}
void ConConMultiple(void* tocH, void* pte, void *profile, int rule, void *a, void *b) {
}
void ConfigFontSetup() {}
void ControlHi() {}
void ConvertExcerpt() {}
bool ConvertPGP(short refN, void* buf, long *size, void* lineType, long estSize, void* pgpc) {
  return false;
}
Byte CrLf[3] = {0};
void CurTrans() {}
void CurrentAttFolderSpec() {}
void CyclePendulum() {
}
void DarkenColor() {}
void DefPosition() {}
void DisposeTLMIME(void* emsMIME) {
}
/* DoComposeNew — ported to comp.c */
bool DontTranslate = false; /* global variable, not a function */
void DotToNum() {}
void DragIsInteresting() {}
void DrawString() {}
void DrawThemeListBoxFrame() {}
void* ESSLSetupVector(void* theTrans) {
  return NULL;
}
void* ESSLStartSSL(void* stream) {
  return NULL;
}
int ETLBuildAddrList(void **textIn, void **moreHeaders, void* hdh, void* addrList, short context) {
  return 0;
}
int ETLCanTranslate(void* translators, short context, void* emsMIME, void* errorStr, long *errCode, void* addrList, void* hdh) {
  return 0;
}
int ETLCountTranslatorsLo(short context, void* forMode) {
  return 0;
}
void ETLDisposeAddrList(void* addrList) {
}
int ETLGetPluginFolderSpec(void* spec, short nameId) {
  return 0;
}
long ETLID(void* tl, short index) {
  return 0;
}
int ETLInterpretFile(short context, void* source, short resultRefN, void* resultAcc, void* addrList, bool *dontSave) {
  return 0;
}
int ETLListAllTranslatorsLo(void* translators, short context, void* forMode) {
  return 0;
}
int ETLMarkJunk(void* thePlugin, void* transInfo, void* junkInfo, void* message, void* junkScore, void* status) {
  return 0;
}
int ETLScoreJunk(void* thePlugin, void* transInfo, void* junkInfo, void* message, void* junkScore, void* status) {
  return 0;
}
void EndMovableModal() {}
void EraseRect() {}
/* ExportHTMLSum — real implementation elsewhere */
/* EzOpenFind — real implementation elsewhere */
/* FAflk* functions implemented in filtwin.c */
short FakeTabs = 0;
/* Fcc — real implementation elsewhere */
void FigureZoom() {}
/* FilterFlaggedMessages — real implementation elsewhere */
/* FilterMessage — real implementation elsewhere */
/* FilterMessagesFrom — real implementation elsewhere */
/* FilterPostprocess — real implementation elsewhere */
/* FilterSelectedMessages — real implementation elsewhere */
void FindControl() {}
void* FindControlByRefCon(void* win, long refCon) {
  return NULL;
}
short FindFolder(short vRef, uint32_t type, bool create, int *foundVRef, long *foundDirID) {
  return 0;
}
/* FindListView — real implementation elsewhere */
/* FindTOCSpot — real implementation in buildtoc.c */
int FlattenTLMIME(void* emsMIME, void* flat) {
  return 0;
}
/* ForceSend — now in globals.c */
void FrameRect() {}
/* Fwd — real implementation elsewhere */
void* GetAMessage(void* tocH, short sumNum, void *u1, void *u2, bool b1) {
  return NULL;
}
short GetControlValue(void* cntl) {
  return 0;
}
void* GetCurrentPayMode() {
  return NULL;
}
void GetDIText() {}
void GetDItemState() {}
void GetDialogWindow() {}
void GetItemCmd(void* mh, short item, short *cmd) {
}
void GetMenuHandle() {}
void GetMenuItemText() {}
/* GetNamedResource — real inline in legacy_shim.h */
void GetNewMyDialog() {}
/* GetNewMyWindow - real impl in mywindow.c */
/* GetPassStuff — ported to util.c, no longer a stub */
void GetPortBounds() {}
int GetPrefBit(short prefId, int bit) {
  return 0;
}
int GetPrefBitNoDominant(short prefId, int bit) {
  return 0;
}
void GetPrefNoDominant(unsigned char *buf, short prefId) {
}
void GetPrefString() {}
int GetRHeaderAnywhere(void* messH, short header, char **text) {
  return 0;
}
void GetRealname() {}
void GetResInfo(void **res, short *id, unsigned int *type, unsigned char *name) {
}
void GetResName() {}
void *GetResource(uint32_t type, short id) { return NULL; }
void* GetReturnAddr(void* addr, bool wantDefault) {
  return NULL;
}
OSErr GetSMTPInfo(unsigned char *host) {
  if (!host) return -1;
  gchar *server = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "smtp_server", "");
  size_t len = strlen(server);
  if (len > 255) len = 255;
  host[0] = (unsigned char)len;
  memcpy(host + 1, server, len);
  g_free(server);
  return 0;
}
short GetSumColor(TOCType *tocH, short sumNum) {
  return 0;
}
void* GetTrashTOC() {
  return NULL;
}
void GetUUPCMail() {}
void GetWindowPort() {}
/* GetWindowPrivateData — real inline in legacy_shim.h */
void GlobalToLocal() {}
bool GrowZoning = false;
void HRename() {}
void HTMLPostamble() {}
void HTMLPreamble() {}
void *HTMLSignature = NULL; /* void ** Handle */
int HandleHeadGetIdText(char *textIn, short id, char **text) {
  return 0;
}
void HandleHeadGetPStr() {}
void HashAppearsInAliasFile() {}
void HideControl(void* ctl) {
}
void HideDialogItem() {}
void HiliteButtonOne() {}
/* HiliteOddReply — real implementation elsewhere */
void IMAPAccuAddPtr() {}
void IMAPAccuInit() {}
void IMAPAccuZap() {}
void ImportErr() {}
/* InitFPB — real implementation elsewhere */
int InsertCommaIfNeedBe(void* pte, void* hs) {
  return 0;
}
MyWindowPtr InsertWin = NULL;
void InvalBoxSizeBox(void *wp) {
}
/* InvalContent - real impl in mywindow.c */
void InvalidListView() {}
void InvalidatePasswords(bool pwGood, bool auxpwGood, bool all) {
}
void InvertRect() {}
void IsAdInPlaylist() {}
bool IsAllLWSPMess(void* messH) {
  return false;
}
void IsFCCAddr() {}
bool IsIMAPMessageProcessed(void* tocH, short sumNum) {
  return false;
}
bool IsMe(char *address) {
  return false;
}
void IsNewsgroupAddr() {}
void IsQueuedState() {}
/* IsWindowVisible — real inline in mailbox.h */
void LVActivate() {}
void LVCalcSize() {}
void LVClick() {}
void LVCountSelection() {}
void LVDispose() {}
void LVDrag() {}
void LVDraw() {}
void LVGetItem() {}
void LVKey() {}
void LVMaxSize() {}
void LVNewWithDetails() {}
void LVSelectAll() {}
void LVSize() {}
uint32_t LastCheckTime = 0;
long LastContigSpace = 0;
long LastTotalSpace = 0;
void LeftRimWidth() {}
short LogRefN = 0; /* global variable, not a function */
long LogTicks = 0;
bool LooseTrans = false;
void MBTickle() {}
RootSpec MailRoot = {0};
EventRecord MainEvent = {0};
short MatchAlias(FSSpecPtr spec, long flags, ...) {
  return 0;
}
bool MemCanFail = false; /* global variable, not a function */
long MemLastFailed = 0;
int Menu2Label(short menu) {
  return 0;
}
void MenuItemIsSeparator() {}
/* MessApp1 — real implementation elsewhere */
/* MessClose — real implementation elsewhere */
/* MessCursor — real implementation elsewhere */
/* MessFind — real implementation elsewhere */
/* MessGonnaShow — real implementation elsewhere */
/* MessIBarUpdate — real implementation elsewhere */
/* MessList — now in globals.c */
/* MessMenu — real implementation elsewhere */
/* MessSaveSub — real implementation elsewhere */
/* MessWi — real implementation elsewhere */
void MiniEvents() {
}
bool Mom(short button, short item, short pref, short warning, short verb) {
  return false;
}
void MovableModalDialog() {}
void MoveTo() {}
void MovingAttachments(void* tocH, short sumNum, bool a, bool b, bool c, bool d) {
}
bool MultiMessageOpOK(int warnType, int count) {
  return false;
}
void MyBalloon() {}
void MyCloseResFile(short refN) {
}
int MyDirCreate(short vRefNum, long parentDirID, const char *directoryName, long *createdDirID) {
  return 0;
}
void MyDisposeDialog() {}
/* MyDisposeWindow - real impl in mywindow.c */
int MyFSClose(short refN) {
  return 0;
}
int MyFSpDirCreate(void* spec, void* scriptTag, long *createdDirID) {
  return 0;
}
void MyGetWTitle() {}
void MyHostname() {}
void MyNMRec() {}
/* MySelectWindow - real impl in floatingwin.c */
void MySetThemeWindowBackground() {}
void MyWinHasSelection() {}
/* MyWindowDidResize - real impl in mywindow.c */
short NeedToFilterIMAP = 0; /* global variable, not a function */
short NeedToFilterIn = 0; /* global variable, not a function */
void NeedToFilterOut() {}
void NeedToNotify() {}
bool NewError = false; /* global variable, not a function */
void NewHandleClear() {}
void NewIconButton() {}
void NewLine() {}
/* NewPrior — real implementation elsewhere */
int NewTLMIME(void* emsMIME) {
  return 0;
}
void NicknameWatcherFocusChange(void* pte) {
}
void NoAdsRec() {}
void NoDominant() {}
void NoSLGet1IndResource() {}
void NoSLGet1Resource() {}
void NoSLGetMHandle() {}
bool NoSaves = false; /* global variable, not a function */
void NukeXfUndo() {
}
void OFwd() {}
void OTTCPTrans() {}
void OffsetWindow() {}
void OnBatteriesX() {}
bool OpenAddrErrs = false; /* global variable, not a function */
void OpenOtherURLPtr() {}
void OutgoingMIDList() {}
void P1() {}
void P2() {}
void P3() {}
void P4() {}
short PBGetCatInfoSync(void *pb) {
  return 0;
}
/* PETE — now a macro in pete_portable.h */
void ParseProtocolFromURLPtr() {}
void ParseURL() {}
/* PersList - NOT a stub. PersList is a macro in threading.h:
   #define PersList (CurThreadGlobals->tPersList)
   Having a function with this name shadows the macro and causes crashes. */
/* PeteCleanList — real implementation elsewhere */
void PeteSelectedString() {}
void PlotIconID() {}
void PlotIconSuite() {}
void PopCursor() {}
void PositionBevelButtons() {}
bool PrefIsSetOrNot(int pref, int modifiers, int mask) {
  return false;
}
void Prior2Display() {}
void PtInRect() {}
void PushCursor() {}
/* QueueMessage — real implementation in compact.c */
/* Re — real implementation elsewhere */
void* ReReadPGPClearText(void* stream, short refN, void* buf, long bSize, void* spec) {
  return NULL;
}
/* ReadSum — real implementation in buildtoc.c */
int RecordTLID(void* spec, void* id) {
  return 0;
}
/* RelLine2Spec — real implementation elsewhere */
void* RemSpoolFolder(long uidHash) {
  return NULL;
}
void RemindSortLinkWin() {
}
void RemoveUTF8FromSum() {}
/* RichSignature — now in globals.c */
bool SaveComp(void* win) {
  return false;
}
/* SaveMessHi — real implementation elsewhere */
void ScrollIt() {}
void SearchPtrPtr() {}
void SearchStrPtr() {}
void SecondsToDate() {}
/* SelectBoxRange — real implementation elsewhere */
void SelectDialogItemText() {}
void SendBehind() {}
/* SendImmediately, SendQueue, SendThreadRunning are global variables — not stubs */
/* SendThreadError — real implementation elsewhere */
/* ServerMenuChoice — real implementation elsewhere */
void SetControlMaximum(void* cntl, short max) {
}
void SetDItemState() {}
void SetItemCmd() {}
void SetMenuItemCommandID() {}
void SetMenuItemHierarchicalMenu() {}
void SetMenuItemModifiers() {}
/* SetMessTable — real implementation elsewhere */
void SetMyCursor() {}
void SetPrefBit() {}
void SetPrefText() {}
/* SetPriority — real implementation elsewhere */
/* SetRect — real inline in legacy_shim.h */
void SetResInfo(void **res, short id, unsigned char *name) {
}
/* SetSig — real implementation in compact.c */
void SetStrOverride(short strn, const char *str) {
}
void SetSumColor(TOCType *tocH, short sumNum, short color) {
}
void SetSumFlag(void* tocH, short sumNum, long flag) {
}
void SetThemeBackground() {}
void SettingsRefN() {}
void ShowBoxSizes(void* win) {
}
/* ShowMessageSeparator — real implementation elsewhere */
/* ShowMyWindow - real impl in mywindow.c */
/* ShowMyWindowBehind - real impl in mywindow.c */
void ShowWindow() {}
bool SigStyled = false; /* global variable, not a function */
int SigValidate(short sigId) {
  return 0;
}
void Slash() {}
void SortedDescending() {}
void StartMovableModal() {}

void* gIMAPConnectionPool = NULL;
int gMaxBoxLevels = 0;
bool gNeedRemind = false;
void* gRegFiles = NULL;
bool gTaskProgressInitied = false;
int nagState = 0;
int g16bitSubMenuIDs = 0;
/* gActiveConnections — real implementation elsewhere */
void* iBeamCursor = NULL;

/* gedit_document_insert_markup, geditctrl_set_editable, geditctrl_set_rich_text
   implemented in gEditCtrl/geditctrl-glue.c */
int pstrincmp(const unsigned char *s1, const unsigned char *s2) { return 0; }

/* toc_free, toc_get_message, toc_get_message_count, toc_get_summaries,
   toc_get_unread_count, toc_load, toc_save — real implementations in toc.c */

void TextFace(int f) {}
void TextFont(int f) {}
void TextSize(int s) {}
void ThreadYieldTicks(int t) {}
int TitleBarHeight(void* w) { return 0; }
void TrackControl(void* c, void* pt, void* a) {}
/* TransferMenuChoice — real implementation elsewhere */
void TransmitMessageForSpool(void) {}
void UUPCDry(void) {}
void UUPCPrime(void) {}
void UUPCWriteAddr(void) {}
/* UpdateMyWindow - real impl in mywindow.c */
void UpdateSum(void* m) {}
/* UpdateTaskProgress — real impl in taskProgress.c */
bool UseFlowOut(void) { return false; }
bool UserHasValidPaidModeRegcode(void) { return false; }
/* UserSelectWindow - real impl in mywindow.c */
bool WNE(int e, void* m, int t) { return false; }
void* Win2TOC(void* w) { return NULL; }
void WrapWrong(void) {}
void YesStr(void) {}
/* eSignature — now in globals.c */
int flavorTypeText = 0;


/* CurPers is a macro in threading.h = CurThreadGlobals->tCurPers */
short InBG = 0;
bool StartingUp(void) { return false; }
int SuckAddresses(void ***addr, void **text, bool b1, bool b2, bool b3, void *p) { return 0; }
void SuckHeaderText(void) {}
int SuckPtrAddresses(void ***addr, void *text, long size, bool b1, bool b2, bool b3, void *p) { return 0; }
bool SumFlagIsSet(void* tocH, short sumNum, long flag) { return false; }
bool TOCIsDirty(void) { return false; }
void TaskDontAutoClose(void) {}


bool ExpandAliasesLow(void **h1, void *h2, int i, bool b1, void *p1, int i2) { return true; }

/* --- Link stubs for unported functions --- */

/* Compose window header field management — needs real GTK impl */
void CompHeadActivate(void *pte) {}
short CompHeadCurrent(void *win) { return 0; }
void CompSwitchFields(void *win, bool forward) {}
void CompGatherRecipientAddresses(void *messH, bool cache) {}

/* Compose window UI — needs real GTK impl */
void InvalTopMargin(void *win) {}
void RefreshSigButton(void *win) {}
void RemoveInlineSig(void *messH) {}
void EnableTxtFmtBarIfOK(void *win) {}

/* Translator/plugin system — needs real port eventually */
long ETLIconToID(void *tl, short context, short index) { return 0; }
void AddTranslatorsFromPtr(void *messH, unsigned char *text, long len) {}
void WriteTranslators(void *messH) {}

/* Content analysis (moodwatch) — not needed in GTK port */
bool AnalWarning(void *messH) { return false; }
bool AnalDelayOutgoing(void) { return false; }

/* Mac dialup/PPP — not needed */
void CheckSLIP(void) {}

/* Mac menu manager — needs GTK menu port */
void AdjustSpecialMenuSelection(void *tocH, short sum) {}

/* Mac key event — replace with GDK */
short UnadornKey(short key, short modifiers) { return key; }

/* Mac window list — replace with GTK window tracking */
void *GetWindowList(void) { return NULL; }
bool IsKnownWindowMyWindow(void *win) { return false; }

/* Mac graphics — not needed */
void DisplayGetGraphics(void *pte) {}

/* Mac ListView — needs GTK TreeView port */
void LVSelect(void *lv, short row, bool extend) {}

/* Mail filter creation — needs real port */
void DoMakeFilter(void *tocH, short sumNum) {}

/* Receipt generation — needs real port */
void GenerateReceipt(void *messH, short type, short action) {}

/* Nickname caching — needs real port */
void NicknameCachingScan(void *messH, unsigned char *text, long len) {}

/* Open text window — needs GTK impl */
void *OpenText(void *spec, void *win, void *p, bool b) { return NULL; }

/* Mailbox folder navigation — needs GTK menu port */
short MBFindInCollapsed(short vRef, long dirId) { return 0; }
short MBGetFolderMenuID(short vRef, long dirId) { return 0; }

/* Attachment/parts folder paths — real GLib implementation */
int GetAttFolderPath(short vRef, long dirId, char *path, int pathSize) {
  const char *home = g_get_home_dir();
  g_snprintf(path, pathSize, "%s/.eudora/Attachments", home);
  g_mkdir_with_parents(path, 0755);
  return 0;
}

int GetIMAPAttachFolderPath(short vRef, long dirId, char *path, int pathSize) {
  const char *home = g_get_home_dir();
  g_snprintf(path, pathSize, "%s/.eudora/IMAP Attachments", home);
  g_mkdir_with_parents(path, 0755);
  return 0;
}

int GetPartsFolder(void *spec) {
  return 0;
}

/* Return address — needs real port from prefs */
unsigned char *GetReturnAddrC(unsigned char *addr) {
  if (addr) addr[0] = 0;
  return addr;
}

/* FSSpec folder check — real GLib implementation */
bool FSpIsItAFolder(void *spec) {
  /* FSSpec has a name field — in practice we check the path */
  return false;
}

/* AttachOptNumber — already a macro in compact.h, but some code calls it as function */
short AttachOptNumber_func(long flags) {
  return (short)(((flags & (0x40|0x80)) >> 6) & 0x3);
}
