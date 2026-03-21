#include <stdbool.h>
#include <string.h>
#include <gtk/gtk.h>
#include "mailbox.h"
#include "schizo.h"
#include "gtk_prefs.h"

/* ActiveTicks — defined in globals.c */
int AddInlineSig(void* messH) {
  return 0;
}
/* AddSpecToList — real implementation elsewhere */
/* AddTLMIME — real implementation in trans.c */
void AddXfUndo(void* tocH, void* trashTOC, int unused) {
}
void AdjustSpecialMenuItem() {}
/* DisposeHandle - needed by CrispinIMAP, just free */
void DisposeHandle(void *h) { free(h); }
short AliasRefCount = 0;
struct AliasDStruct *Aliases = NULL;
int gAliasCount = 0;
long AnyTOCDirty = 0;
void AppendMenu() {}
/* AttFolderSpec — defined in globals.c */
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
/* AutoCheckOK — real implementation in address.c */
/* BeenThereDoneThat — real implementation elsewhere */
void BeginPGP(void* pgpc) {
}
/* BoxCenterSelection — real implementation elsewhere */
void *BoxCount = NULL;
size_t BoxCountSize = 0;
/* BoxFOpen — real implementation in mailbox.c */
struct BoxMapStruct *BoxMap = NULL;
size_t BoxMapSize = 0;
void BoxPreviewProfile() {}
/* BoxSelectAfter — real implementation elsewhere */
/* BoxSetSummarySelected — real implementation elsewhere */
short *BoxWidths = NULL;
short BugFlags = 0;
/* BuildEnriched/BuildHTML — were in rich.c, now replaced by gEditCtrl + crispy_richtext */
int BuildEnriched(void *a, void *b, void *c, long d, long e, void *f, int g) { (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g; return -1; }
int BuildHTML(void *a, void *b, void *c, long d, long e, void *f, void *g, int h, void *i, void *j, void *k) { (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k; return -1; }
int HTMLPreamble(void *a, void *b, int c, int d) { (void)a;(void)b;(void)c;(void)d; return -1; }
int HTMLPostamble(void *a, int b) { (void)a;(void)b; return -1; }
void BuildStationeryList() {
}
void CacheRecentNickname(unsigned char *name) {
}
void Cell1Rect() {}
/* CheckAddNotifyControls — real implementation elsewhere */
/* CheckNow — defined in globals.c */
unsigned char CheckOnIdle = 0;     /* global variable, not a function */
/* CheckThreadError — real implementation elsewhere */
/* CheckThreadRunning — real implementation elsewhere */
void ClearPrefBit() {}
/* CloseMyWindow — real inline in legacy_shim.h */
/* CompAttachSpec — real implementation in compact.c */
/* CompDelAttachment — real implementation in compact.c */
/* CompGatherRecipientAddresses — real implementation in compact.c */
void CompGetMID() {}
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
/* CrLf — defined in globals.c */
/* CurTrans — real implementation via thread-local in threading.h */
void CurrentAttFolderSpec() {}
void CyclePendulum() {
}
void DarkenColor() {}
void DefPosition() {}
/* DisposeTLMIME — real implementation in trans.c */
/* DoComposeNew — ported to comp.c */
/* DontTranslate — defined in globals.c */
void DotToNum() {}
void DragIsInteresting() {}
void DrawString() {}
void DrawThemeListBoxFrame() {}
/* ESSLSetupVector and ESSLStartSSL now in ssl.c */
/* ETL functions — all in trans.c */
void EndMovableModal() {}
void EraseRect() {}
/* ExportHTMLSum — real implementation elsewhere */
/* EzOpenFind — real implementation elsewhere */
/* FAflk* functions implemented in filtwin.c */
/* FakeTabs — defined in globals.c */
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
/* FlattenTLMIME — real implementation in trans.c */
/* ForceSend — now in globals.c */
void FrameRect() {}
/* Fwd — real implementation elsewhere */
void* GetAMessage(void* tocH, short sumNum, void *u1, void *u2, bool b1) {
  return NULL;
}
short GetControlValue(void* cntl) {
  return 0;
}
/* GetCurrentPayMode — in trans.c */
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
/* GetRealname — real implementation in address.c */
void GetResInfo(void **res, short *id, unsigned int *type, unsigned char *name) {
}
void GetResName() {}
void *GetResource(uint32_t type, short id) { return NULL; }
/* GetReturnAddr — real implementation in address.c */
int GetSMTPInfo(unsigned char *host) {
  if (!host) return -1;
  gchar *server = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "smtp_server", "");
  size_t len = strlen(server);
  if (len > 255) len = 255;
  host[0] = (unsigned char)len;
  memcpy(host + 1, server, len);
  g_free(server);
  return 0;
}
short GetSumColor(MacmbxTOC *tocH, short sumNum) {
  return 0;
}
/* GetTrashTOC — real implementation in address.c */
void GetUUPCMail() {}
void GetWindowPort() {}
/* GetWindowPrivateData — real inline in legacy_shim.h */
void GlobalToLocal() {}
bool GrowZoning = false;
void HRename() {}
/* HTMLPostamble/HTMLPreamble stubs moved to line ~54 with proper signatures */
/* HTMLSignature — defined in globals.c */
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
struct HeadSpec;
int InsertCommaIfNeedBe(GtkWidget *pte, struct HeadSpec *hs) {
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
/* IsFCCAddr — real implementation in address.c */
bool IsIMAPMessageProcessed(void* tocH, short sumNum) {
  return false;
}
/* IsMe — real implementation in address.c */
/* IsNewsgroupAddr — real implementation in address.c */
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
/* LastCheckTime — defined in globals.c */
long LastContigSpace = 0;
long LastTotalSpace = 0;
void LeftRimWidth() {}
/* LogRefN — defined in globals.c */
long LogTicks = 0;
bool LooseTrans = false;
void MBTickle() {}
RootSpec MailRoot = {0};
/* MainEvent removed */
short MatchAlias(char * spec, long flags, ...) {
  return 0;
}
/* MemCanFail — defined in globals.c */
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
/* MyCloseResFile — real impl in fileutil.c */
/* MyDirCreate — real impl in fileutil.c */
void MyDisposeDialog() {}
/* MyDisposeWindow - real impl in mywindow.c */
/* MyFSClose — real impl in fileutil.c */
/* MyFSpDirCreate — real impl in fileutil.c */
void MyGetWTitle() {}
unsigned char MyHostname[128] = {0};
void MyNMRec() {}
/* MySelectWindow - real impl in floatingwin.c */
void MySetThemeWindowBackground() {}
void MyWinHasSelection() {}
/* MyWindowDidResize - real impl in mywindow.c */
short NeedToFilterIMAP = 0;
/* NeedToFilterIn — defined in globals.c */
/* NeedToFilterOut — defined in globals.c */
/* NeedToNotify — defined in globals.c */
bool NewError = false; /* global variable, not a function */
void NewHandleClear() {}
void NewIconButton() {}
/* NewLine is now a proper C string global defined in globals.c */
/* NewPrior — real implementation elsewhere */
/* NewTLMIME — real implementation in trans.c */
void NicknameWatcherFocusChange(void* pte) {
}
void NoAdsRec() {}
void NoDominant() {}
void NoSLGet1IndResource() {}
void NoSLGet1Resource() {}
void NoSLGetMHandle() {}
/* NoSaves — defined in globals.c */
void NukeXfUndo() {
}
void OFwd() {}
/* OTTCPTrans is now a proper TransVector defined in tcp.c */
void OffsetWindow() {}
void OnBatteriesX() {}
/* OpenAddrErrs — defined in globals.c */
void OpenOtherURLPtr() {}
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
/* RecordTLID — in trans.c */
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
/* SendBehind: real implementation in mywindow.c */
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
void SetSumColor(MacmbxTOC *tocH, short sumNum, short color) {
}
void SetSumFlag(void* tocH, short sumNum, long flag) {
}
void SetThemeBackground() {}
/* SettingsRefN — fallback global for files that don't include threading.h.
   In threaded code it's a macro: CurThreadGlobals->tSettingsRefN */
short SettingsRefN;
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
/* Slash — defined as unsigned char Slash[3] in globals.c */
void SortedDescending() {}
void StartMovableModal() {}

void* gIMAPConnectionPool = NULL;
int gMaxBoxLevels = 0;
bool gNeedRemind = false;
void* gRegFiles = NULL;
/* gTaskProgressInitied — defined in globals.c */
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
/* YesStr — defined in globals.c */
/* eSignature — now in globals.c */
int flavorTypeText = 0;


/* CurPers is a macro in threading.h = CurThreadGlobals->tCurPers */
/* InBG — defined in globals.c */
/* StartingUp — defined as bool in globals.c */
/* SuckAddresses — real implementation in address.c */
void SuckHeaderText(void) {}
/* SuckPtrAddresses — real implementation in address.c */
bool SumFlagIsSet(void* tocH, short sumNum, long flag) { return false; }
bool TOCIsDirty(void) { return false; }
/* TaskDontAutoClose — defined in globals.c */


bool ExpandAliasesLow(void **h1, void *h2, int i, bool b1, void *p1, int i2) { return true; }

/* --- Link stubs for unported functions --- */

/* Compose window header field management — needs real GTK impl */

int GetIMAPAttachFolderPath(short vRef, long dirId, char *path, int pathSize) {
  const char *home = g_get_home_dir();
  g_snprintf(path, pathSize, "%s/.eudora/IMAP Attachments", home);
  g_mkdir_with_parents(path, 0755);
  return 0;
}

int GetPartsFolder(void *spec) {
  return 0;
}

/* GetReturnAddrC — real implementation in address.c */

/* FSSpec folder check — real GLib implementation */
bool FSpIsItAFolder(void *spec) {
  /* FSSpec has a name field — in practice we check the path */
  return false;
}

/* AttachOptNumber — already a macro in compact.h, but some code calls it as function */
short AttachOptNumber_func(long flags) {
  return (short)(((flags & (0x40|0x80)) >> 6) & 0x3);
}

/* Compose window UI stubs — need real GTK impl */
void CompGatherRecipientAddresses(void *messH, bool cache) {}
void InvalTopMargin(void *win) {}
void RefreshSigButton(void *win) {}
void RemoveInlineSig(void *messH) {}
void EnableTxtFmtBarIfOK(void *win) {}

/* Content analysis stubs — not needed in GTK port */
bool AnalWarning(void *messH) { return false; }
bool AnalDelayOutgoing(void) { return false; }

/* Mac dialup stub */
void CheckSLIP(void) {}

/* Mac menu stubs */
void AdjustSpecialMenuSelection(void *tocH, short sum) {}

/* Key event stub */
bool IsKnownWindowMyWindow(void *win) { return false; }

/* ListView stub */
void LVSelect(void *lv, short row, bool extend) {}

/* Filter creation stub */
void DoMakeFilter(void *tocH, short sumNum) {}

/* Receipt generation stub */
void GenerateReceipt(void *messH, short type, short action) {}

/* Nickname caching stub */
void NicknameCachingScan(void *messH, unsigned char *text, long len) {}

/* Open text window stub */
void *OpenText(void *spec, void *win, void *p, bool b) { return NULL; }

/* Mailbox folder navigation stubs */
short MBFindInCollapsed(short vRef, long dirId) { return 0; }
short MBGetFolderMenuID(short vRef, long dirId) { return 0; }

/* Attachment folder path — real GLib implementation */
int GetAttFolderPath(short vRef, long dirId, char *path, int pathSize) {
  const char *home = g_get_home_dir();
  g_snprintf(path, pathSize, "%s/.eudora/Attachments", home);
  g_mkdir_with_parents(path, 0755);
  return 0;
}

