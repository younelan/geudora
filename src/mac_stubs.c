#include <stdbool.h>
#include <gtk/gtk.h>
#include "mailbox.h"
#include "schizo.h"

void ActiveTicks() {}
int AddInlineSig(void* messH) {
  return 0;
}
void AddSpecToList(void* spec, void* specList) {
}
int AddTLMIME(void* emsMIME, short what, unsigned char *name, unsigned char *value) {
  return 0;
}
void AddXfUndo(void* tocH, void* trashTOC, int unused) {
}
void AdjustSpecialMenuItem() {}
void AliasRefCount() {}
void Aliases() {}
void AnyTOCDirty() {}
void AppendMenu() {}
void AttFolderSpec() {}
bool AttIsSelected(void* win, void* pte, long startWith, long endWith, short what, long *start, long *stop) {
  return false;
}
int AttLine2Spec(unsigned char *line, void* spec, bool wantToOpen) {
  return 0;
}
void AttachOptNumber() {}
void AttachSelect(void* messH) {
}
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
void BeenThereDoneThat(void* tocH, short sumNum) {
}
void BeginPGP(void* pgpc) {
}
void BoxCenterSelection() {}
void BoxCount() {}
int BoxFOpen(TOCType *tocH) {
  return 0;
}
void BoxMap() {}
void BoxPreviewProfile() {}
void BoxSelectAfter(void* win, short sumNum) {
}
void BoxSetSummarySelected(void* tocH, short sum, bool select) {
}
void BoxWidths() {}
void BugFlags() {}
void BuildEnriched() {}
void BuildHTML() {}
void BuildStationeryList() {
}
void CacheRecentNickname(unsigned char *name) {
}
void Cell1Rect() {}
bool CheckAddNotifyControls(void* win, void* messH) {
  return false;
}
void CheckNow() {}
void CheckOnIdle() {}
void CheckThreadError() {}
void CheckThreadRunning() {}
void ClearPrefBit() {}
/* CloseMyWindow — real inline in legacy_shim.h */
void CompAttachSpec(void* win, void* spec) {
}
void CompDelAttachment(void* messH, void* where) {
}
int CompGatherRecipientAddresses(void* messH, bool wantComments) {
  return 0;
}
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
void CrLf() {}
void CurTrans() {}
void CurrentAttFolderSpec() {}
void CyclePendulum() {
}
void DarkenColor() {}
void DefPosition() {}
void DisposeTLMIME(void* emsMIME) {
}
void* DoComposeNew(int type) {
  return NULL;
}
void DontTranslate() {}
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
int ExportHTMLSum(void* tocH, short sumNum) {
  return 0;
}
short EzOpenFind(void* tocH, short origSum) {
  return 0;
}
void FAflkAddHistory() {}
void FAflkCopy() {}
void FAflkForward() {}
void FAflkJunk() {}
void FAflkLabel() {}
void FAflkMoveAttach() {}
void FAflkNone() {}
void FAflkNotifyUser() {}
void FAflkOpenMessage() {}
void FAflkPersonality() {}
void FAflkPrint() {}
void FAflkPriority() {}
void FAflkRedirect() {}
void FAflkReply() {}
void FAflkServerOpts() {}
void FAflkSound() {}
void FAflkSpeak() {}
void FAflkStatus() {}
void FAflkStop() {}
void FAflkSubject() {}
void FAflkTransfer() {}
void FakeTabs() {}
void Fcc(void* messH, void* box) {
}
void FigureZoom() {}
int FilterFlaggedMessages(void* fType, void* tocH, void* fpb) {
  return 0;
}
int FilterMessage(void* fType, void* tocH, short sumNum) {
  return 0;
}
int FilterMessagesFrom(void* fType, void* tocH, short startWith, void* fpb, bool noXfer) {
  return 0;
}
void FilterPostprocess(void* fType, void* fpb) {
}
int FilterSelectedMessages(void* fType, void* tocH, void* fpb) {
  return 0;
}
void FindControl() {}
void* FindControlByRefCon(void* win, long refCon) {
  return NULL;
}
short FindFolder(short vRef, uint32_t type, bool create, int *foundVRef, long *foundDirID) {
  return 0;
}
void FindListView() {}
/* FindTOCSpot — real implementation in buildtoc.c */
int FlattenTLMIME(void* emsMIME, void* flat) {
  return 0;
}
void ForceSend() {}
void FrameRect() {}
void Fwd() {}
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
void GetPassStuff(unsigned char *persName, unsigned char *uName, unsigned char *hName) {
}
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
void GetSMTPInfo() {}
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
void GrowZoning() {}
void HRename() {}
void HTMLPostamble() {}
void HTMLPreamble() {}
void HTMLSignature() {}
int HandleHeadGetIdText(char *textIn, short id, char **text) {
  return 0;
}
void HandleHeadGetPStr() {}
void HashAppearsInAliasFile() {}
void HideControl(void* ctl) {
}
void HideDialogItem() {}
void HiliteButtonOne() {}
void HiliteOddReply(void* messH) {
}
void IMAPAccuAddPtr() {}
void IMAPAccuInit() {}
void IMAPAccuZap() {}
void ImportErr() {}
int InitFPB(void* fpb, bool zapAddrs, bool listsToo) {
  return 0;
}
int InsertCommaIfNeedBe(void* pte, void* hs) {
  return 0;
}
void InsertWin() {}
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
void LastCheckTime() {}
void LastContigSpace() {}
void LastTotalSpace() {}
void LeftRimWidth() {}
void LogRefN() {}
void LogTicks() {}
void LooseTrans() {}
void MBTickle() {}
void MailRoot() {}
void MainEvent() {}
short MatchAlias(FSSpecPtr spec, long flags, ...) {
  return 0;
}
void MemCanFail() {}
void MemLastFailed() {}
int Menu2Label(short menu) {
  return 0;
}
void MenuItemIsSeparator() {}
bool MessApp1(void* win, void *event) {
  return false;
}
bool MessClose(void* win) {
  return false;
}
void MessCursor(Point mouse) {
}
bool MessFind(void* win, unsigned char *what) {
  return false;
}
int MessGonnaShow(void* win) {
  return 0;
}
void MessIBarUpdate(void* messH) {
}
void MessList() {}
bool MessMenu(void* win, int menu, int item, short modifiers) {
  return false;
}
int MessSaveSub(void* messH) {
  return 0;
}
short MessWi(void* win) {
  return 0;
}
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
void NeedToFilterIMAP() {}
void NeedToFilterIn() {}
void NeedToFilterOut() {}
void NeedToNotify() {}
void NewError() {}
void NewHandleClear() {}
void NewIconButton() {}
void NewLine() {}
short NewPrior(short item, short prior) {
  return 0;
}
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
void NoSaves() {}
void NukeXfUndo() {
}
void OFwd() {}
void OTTCPTrans() {}
void OffsetWindow() {}
void OnBatteriesX() {}
void OpenAddrErrs() {}
void OpenOtherURLPtr() {}
void OutgoingMIDList() {}
void P1() {}
void P2() {}
void P3() {}
void P4() {}
short PBGetCatInfoSync(void *pb) {
  return 0;
}
void PETE() {}
void ParseProtocolFromURLPtr() {}
void ParseURL() {}
/* PersList - NOT a stub. PersList is a macro in threading.h:
   #define PersList (CurThreadGlobals->tPersList)
   Having a function with this name shadows the macro and causes crashes. */
void PeteCleanList() {}
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
int QueueMessage(void* tocH, short sumNum, int when, int flags, bool b1, bool b2) {
  return 0;
}
void Re() {}
void* ReReadPGPClearText(void* stream, short refN, void* buf, long bSize, void* spec) {
  return NULL;
}
/* ReadSum — real implementation in buildtoc.c */
int RecordTLID(void* spec, void* id) {
  return 0;
}
int RelLine2Spec(unsigned char *line, void* spec, void* cid, void* relURL, void* absURL) {
  return 0;
}
void* RemSpoolFolder(long uidHash) {
  return NULL;
}
void RemindSortLinkWin() {
}
void RemoveUTF8FromSum() {}
void RichSignature() {}
bool SaveComp(void* win) {
  return false;
}
bool SaveMessHi(void* win, bool closing) {
  return false;
}
void ScrollIt() {}
void SearchPtrPtr() {}
void SearchStrPtr() {}
void SecondsToDate() {}
void SelectBoxRange(void* tocH, short from, short to, bool extend, short oFrom, short oTo) {
}
void SelectDialogItemText() {}
void SendBehind() {}
void SendImmediately() {}
void SendQueue() {}
void SendThreadError() {}
void SendThreadRunning() {}
void ServerMenuChoice(void* tocH, short sumNum, int choice, bool shift) {
}
void SetControlMaximum(void* cntl, short max) {
}
void SetDItemState() {}
void SetItemCmd() {}
void SetMenuItemCommandID() {}
void SetMenuItemHierarchicalMenu() {}
void SetMenuItemModifiers() {}
void SetMessTable(void* tocH, short sumNum, short newId) {
}
void SetMyCursor() {}
void SetPrefBit() {}
void SetPrefText() {}
void SetPriority(void* tocH, short sumNum, int priority) {
}
/* SetRect — real inline in legacy_shim.h */
void SetResInfo(void **res, short id, unsigned char *name) {
}
void SetSig(void* tocH, short sumNum, int sigId) {
}
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
void ShowMessageSeparator(void* pte, bool center) {
}
/* ShowMyWindow - real impl in mywindow.c */
/* ShowMyWindowBehind - real impl in mywindow.c */
void ShowWindow() {}
void SigStyled() {}
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
int gActiveConnections = 0;
void* iBeamCursor = NULL;

void gedit_document_insert_markup(void* self, int offset, const char* markup) {}
void geditctrl_set_editable(void* ctrl, int editable) {}
void geditctrl_set_rich_text(void* ctrl, int offset, int is_rich) {}
int pstrincmp(const unsigned char *s1, const unsigned char *s2) { return 0; }

/* toc_free, toc_get_message, toc_get_message_count, toc_get_summaries,
   toc_get_unread_count, toc_load, toc_save — real implementations in toc.c */

void TextFace(int f) {}
void TextFont(int f) {}
void TextSize(int s) {}
void ThreadYieldTicks(int t) {}
int TitleBarHeight(void* w) { return 0; }
void TrackControl(void* c, void* pt, void* a) {}
void TransferMenuChoice(void) {}
void TransmitMessageForSpool(void) {}
void UUPCDry(void) {}
void UUPCPrime(void) {}
void UUPCWriteAddr(void) {}
/* UpdateMyWindow - real impl in mywindow.c */
void UpdateSum(void* m) {}
void UpdateTaskProgress(void* t) {}
bool UseFlowOut(void) { return false; }
bool UserHasValidPaidModeRegcode(void) { return false; }
/* UserSelectWindow - real impl in mywindow.c */
bool WNE(int e, void* m, int t) { return false; }
void* Win2TOC(void* w) { return NULL; }
void WrapWrong(void) {}
void YesStr(void) {}
void eSignature(void) {}
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
