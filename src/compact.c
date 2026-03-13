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

/*
 * compact.c — composition window actions
 *
 * Ported from Mac Carbon/QuickDraw to GTK4.
 * This file provides queue management, attachment handling, signature
 * management, stationery, field navigation, translator support, and
 * other composition window utilities.
 *
 * The main composition window callbacks (CompClose, CompClick, CompMenu,
 * CompKey, CompButton, CompHelp, CompGonnaShow, CompDragHandler, CompIdle,
 * CompDidResize) live in comp.c.
 */

#include "mailbox.h"
#include "mydefs.h"
#include "message.h"
#include "toc.h"
#include "compact.h"
#include "comp.h"         /* CompHeadFind, CompHeadGetText, CompHeadAppendPtr, HeadSpec (via sendmail.h) */
#include "peteglue.h"     /* PeteIsDirty, PeteDelete, PeteInsertPtr, PeteSelect, PeteLen */
#include "legacy_shim.h"
#include "features.h"
#include "gtk_dialogs.h"
#include "prefdefs.h"
#include "util.h"
#include "StringUtil.h"   /* TrimWhite, ComposeRString, MakePStr */
#include "threading.h"    /* Must be before schizo.h for PersList/CurPers macros */
#include "schizo.h"       /* PushPers, PopPers, FindPersById, SetPers */
#include "nickmng.h"      /* historyAddressBook enum, FindAddressBookType */
#include "pop.h"          /* PERS_FORCE, MESS_TO_PERS */
#include "rich.h"         /* InsertRich */
#include "trans.h"        /* ETLIconToID */
#include "mailxfer.h"     /* XferMail */
#include "MyRes.h"        /* ATTACH_MENU */
#include "../gEditCtrl/geditctrl.h"
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define FILE_NUM 8

/*
 * Message flag bits — from original Mac mailbox.h
 * These control per-message options in the composition window icon bar.
 */
#ifndef FLAG_CAN_ENC
#define FLAG_CAN_ENC   (1L<<9)
#define FLAG_BX_TEXT   (1L<<2)
#define FLAG_WRAP_OUT  (1L<<3)
#define FLAG_KEEP_COPY (1L<<4)
#define FLAG_RR        (1L<<12)
#define FLAG_SIG       (1L<<21)
#define FLAG_OLD_SIG   (1L<<1)
#define FLAG_ICON_BAR  (1L<<30)
#endif

#ifndef OPT_COMP_TOOLBAR_VISIBLE
#define OPT_COMP_TOOLBAR_VISIBLE (1L<<13)
#endif
#ifndef OPT_BLOAT
#define OPT_BLOAT        (1L<<16)
#define OPT_STRIP        (1L<<17)
#define OPT_JUST_EXCERPT (1L<<21)
#define OPT_HAS_SPOOL   (1L<<15)
#endif

#ifndef SIG_NONE
#define SIG_NONE ((uLong)-1)
#endif

/* Style constants for send style warning */
#define ssLimit 0
#define ssBloat 1
#define ssStrip 2

#ifndef CANT_QUEUE
#define CANT_QUEUE 100
#endif

/* Icon bar flag bits array — matches original Mac order */
static long fBits[] = {
	-(long)OPT_COMP_TOOLBAR_VISIBLE,
	FLAG_CAN_ENC,
	FLAG_BX_TEXT,
	FLAG_WRAP_OUT,
	FLAG_KEEP_COPY,
	FLAG_RR
};

#define I_WIDTH 24
#define I_HEIGHT 24

/*
 * Forward declarations — only for functions not declared in any included header.
 * Functions declared in sendmail.h, comp.h, trans.h, mailxfer.h, rich.h,
 * schizo.h, pop.h are NOT re-declared here.
 */
extern bool IsQueued(TOCType *tocH, short sumNum);
extern void SetState(TOCType *tocH, short sumNum, short state);
extern void SetSendQueue(void);
extern uLong GMTDateTime(void);
extern long ZoneSecs(void);
/* FindAddressBookType, SaveIndNickFile declared in nickmng.h */
extern void EnableTxtFmtBarIfOK(MyWindowPtr win);
extern void AppendMessText(MessHandle messH, long offset, unsigned char *text, long len);
extern void InvalTopMargin(MyWindowPtr win);
extern void PlaceMessErrNote(MessHandle messH);
extern bool AnalWarning(MessHandle messH);
extern bool AnalDelayOutgoing(void);
extern void SumInfoCpy(MSumPtr dst, MSumPtr src);
extern void TextFindAndCopyHeader(unsigned char *text, long bodySpot, MessHandle messH, unsigned char *header, short head, short label);
extern int Snarf(FSSpec *spec, void **textH, long flags);
extern int SuckAddresses(void ***addr, void **text, bool b1, bool b2, bool b3, void *p);
extern void NicknameCachingScan(GtkWidget *pte, void *raw);
extern int ExpandAliases(void **h, void *raw, int n, bool deep);
extern void CommaList(Handle h);
extern int FSpIsItAFolder(FSSpec *spec);
extern long FSpFileSize(FSSpec *spec);
extern void FolderSizeHi(short vRefNum, long dirId, long *size);
extern long SpecDirId(FSSpec *spec);
extern void RefreshSigButton(MessHandle messH);
extern void CompSwitchFields(MessHandle messH, bool forward);
extern void MakeAttSubFolder(MessHandle messH, uLong hash, FSSpec *spec);
extern int FSpDupFolder(FSSpec *toSpec, FSSpec *fromSpec, bool replace, bool deep);
extern UPtr FindHeaderString(UPtr text, UPtr headerName, long *size, bool bodyToo);
extern bool UseInlineSig;
extern short pStationeryLabel;
/* historyAddressBook is an enum in nickmng.h */

/* M_T1 global needed by PERS_FORCE macro (from pop.h) */
extern uLong M_T1;

/* Mac ControlManager UI calls — no-ops in GTK4, UI handled by compose_window.c */
static inline void SetBevelIcon(ControlHandle c, short t, short a, short b, void *p) { (void)c; (void)t; (void)a; (void)b; (void)p; }
static inline void SetBevelMenuValue(ControlHandle c, short v) { (void)c; (void)v; }

#ifndef MAX_MESSAGE_SIZE
#define MAX_MESSAGE_SIZE 200
#endif
#ifndef HEADER_STRN
#define HEADER_STRN 1000
#endif

/* HeaderName replacement — Mac used resource strings; we use a static table */
static const char *CompHeaderNameStr(short num)
{
	static const char *names[] = {
		"",              /* 0 */
		"To:",           /* TO_HEAD = 1 */
		"From:",         /* FROM_HEAD = 2 */
		"Subject:",      /* SUBJ_HEAD = 3 */
		"Cc:",           /* CC_HEAD = 4 */
		"Bcc:",          /* BCC_HEAD = 5 */
		"Attachments:",  /* ATTACH_HEAD = 6 */
		"",              /* BODY_HEAD = 7 */
		"X-Translator:", /* TRANSLATOR_HEAD = 8 */
	};
	if (num >= 0 && num < (int)(sizeof(names)/sizeof(names[0])))
		return names[num];
	return "";
}
#undef HeaderName
#define HeaderName(num) ((unsigned char *)CompHeaderNameStr(num))

#ifndef PREF_NICK_CACHE
#define PREF_NICK_CACHE 300
#endif
#ifndef PREF_SEND_STYLE
#define PREF_SEND_STYLE 301
#endif
#ifndef PREF_WARN_RICH
#define PREF_WARN_RICH 302
#endif
#ifndef PREF_SUBJECT_WARNING
#define PREF_SUBJECT_WARNING 303
#endif
#ifndef PREF_AUTO_SEND
#define PREF_AUTO_SEND 304
#endif
#ifndef PREF_NICK_AUTO_EXPAND
#define PREF_NICK_AUTO_EXPAND 305
#endif
#ifndef OUT_TEMP
#define OUT_TEMP 0
#define OUT 1
#endif

#ifndef ANAL_DELAY_LEVEL
#define ANAL_DELAY_LEVEL 103
#define ANAL_DELAY_MINUTES 104
#endif
#ifndef SUBJECT_WARNING
#define SUBJECT_WARNING 105
#endif
#ifndef R_FMT
#define R_FMT 106
#endif
#ifndef QUEUE_BTN
#define QUEUE_BTN 107
#endif
#ifndef QUEUE_BUTTON
#define QUEUE_BUTTON 108
#endif
#ifndef SEND_BUTTON
#define SEND_BUTTON 109
#endif
#ifndef SAVE_ITEXT
#define SAVE_ITEXT 110
#endif
#ifndef COMP_WIN
#define COMP_WIN 5
#endif
#ifndef MESS_WIN
#define MESS_WIN 3
#endif

/* Head constants */
#ifndef TO_HEAD
#define TO_HEAD 1
#define FROM_HEAD 2
#define SUBJ_HEAD 3
#define CC_HEAD 4
#define BCC_HEAD 5
#define ATTACH_HEAD 6
#define BODY_HEAD 7
#define TRANSLATOR_HEAD 8
#endif
#ifndef flkManual
#define flkManual 1
#endif

#ifndef Mom
#define Mom(a,b,c,d,e) 0
#endif

/* Forward declaration for internal helper */
static int QueueSizeWarning(MessHandle messH);

/*
 * GTK4 synchronous dialog helper
 * Spins a nested main loop to get a synchronous response from GtkAlertDialog.
 */
typedef struct {
	GMainLoop *loop;
	int response;
} SyncAlertData;

static void sync_alert_response_cb(GObject *source, GAsyncResult *result, gpointer user_data)
{
	SyncAlertData *data = user_data;
	GError *error = NULL;
	data->response = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source), result, &error);
	if (error) {
		data->response = -1;
		g_error_free(error);
	}
	g_main_loop_quit(data->loop);
}

static int run_alert_sync(GtkWindow *parent, const char *message, const char * const *buttons)
{
	GtkAlertDialog *dialog = gtk_alert_dialog_new("%s", message);
	gtk_alert_dialog_set_buttons(dialog, buttons);
	gtk_alert_dialog_set_modal(dialog, TRUE);

	SyncAlertData data;
	data.loop = g_main_loop_new(NULL, FALSE);
	data.response = -1;

	gtk_alert_dialog_choose(dialog, parent, NULL, sync_alert_response_cb, &data);
	g_main_loop_run(data.loop);
	g_main_loop_unref(data.loop);
	g_object_unref(dialog);
	return data.response;
}

/*
 * GTK4 synchronous file dialog helper
 */
typedef struct {
	GMainLoop *loop;
	GFile *file;
} SyncFileData;

static void sync_file_open_cb(GObject *source, GAsyncResult *result, gpointer user_data)
{
	SyncFileData *data = user_data;
	GError *error = NULL;
	data->file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &error);
	if (error) {
		data->file = NULL;
		g_error_free(error);
	}
	g_main_loop_quit(data->loop);
}

/*
 * GTK4 synchronous custom dialog helper (for ModifyQueue)
 */
typedef struct {
	GMainLoop *loop;
	bool accepted;
	GtkWidget *radioNow;
	GtkWidget *radioQueue;
	GtkWidget *radioLater;
	GtkWidget *radioUnqueue;
	GtkWidget *timeEntry;
	GtkWidget *dateEntry;
} ModifyQueueData;

static void modq_ok_clicked(GtkButton *btn, gpointer user_data)
{
	(void)btn;
	ModifyQueueData *data = user_data;
	data->accepted = true;
	g_main_loop_quit(data->loop);
}

static void modq_cancel_clicked(GtkButton *btn, gpointer user_data)
{
	(void)btn;
	ModifyQueueData *data = user_data;
	data->accepted = false;
	g_main_loop_quit(data->loop);
}

static gboolean modq_close_request(GtkWindow *window, gpointer user_data)
{
	(void)window;
	ModifyQueueData *data = user_data;
	data->accepted = false;
	g_main_loop_quit(data->loop);
	return TRUE; /* prevent default close, we handle it */
}

/************************************************************************
 * QueueMessage - queue up a message for sending
 *
 * Original: compact.c:143-259
 * Handles rich text style warnings, subject warnings, content analysis,
 * translator checks, and timed/queued/immediate sending.
 ************************************************************************/
int QueueMessage(TOCType *tocH, short sumNum, SendTypeEnum st, long secs, bool noSpell, bool noAnalDelay)
{
	MessHandle messH = tocH->sums[sumNum].messH;
	int err = -1; /* userCancelled */
	short state = secs ? TIMED : QUEUED;
	long oldFlags, oldOpts;

	/* Cache recently used addresses — original Eudora feature */
	if (HasFeature(featureNicknameWatching) && messH && !PrefIsSet(PREF_NICK_CACHE)) {
		CompGatherRecipientAddresses(messH, true);
		short historyAB = FindAddressBookType(historyAddressBook);
		if (historyAB >= 0)
			SaveIndNickFile(historyAB, true);
	}

	/* Stationery: just save, don't queue */
	if (messH && messH->hStationerySpec) {
		if (SaveComp(messH->win))
			err = 0;
		return err;
	}

	if (!secs)
		secs = GMTDateTime();

	if (st != kEuSendNever && messH) {
		/* Figure out rich text situation */
		oldFlags = SumOf(messH)->flags;
		oldOpts = SumOf(messH)->opts;

		SetMessRich(messH);
		ClearMessOpt(messH, OPT_BLOAT);
		ClearMessOpt(messH, OPT_STRIP);

		bool stripWithPrejudice = MessOptIsSet(messH, OPT_JUST_EXCERPT);
		if (!stripWithPrejudice) {
			switch (GetPrefLong(PREF_SEND_STYLE)) {
				case ssBloat: SetMessOpt(messH, OPT_BLOAT); break;
				case ssStrip: SetMessOpt(messH, OPT_STRIP); break;
			}

			/* Ask user about rich text if needed */
			if (MessIsRich(messH) && PrefIsSet(PREF_WARN_RICH) && !MessOptIsSet(messH, OPT_BULK)) {
				/* Original called SendStyleWarning() which showed a Mac dialog.
				 * In GTK4, show an alert dialog with style choices. */
				const char *buttons[] = { "Cancel", "Send as-is", "Styled only", "Plain only", NULL };
				int choice = run_alert_sync(
					messH->win ? GTK_WINDOW(messH->win->window) : NULL,
					"This message contains styled text. How should it be sent?",
					buttons);
				switch (choice) {
					case 0: /* Cancel */
						return -1;
					case 2: /* Styled only = bloat */
						SetMessOpt(messH, OPT_BLOAT);
						ClearMessOpt(messH, OPT_STRIP);
						break;
					case 3: /* Plain only = strip */
						ClearMessOpt(messH, OPT_BLOAT);
						SetMessOpt(messH, OPT_STRIP);
						break;
					default: /* Send as-is */
						ClearMessOpt(messH, OPT_BLOAT);
						ClearMessOpt(messH, OPT_STRIP);
						break;
				}
			}
		}

		/* If flags changed, re-save */
		if (oldFlags != SumOf(messH)->flags || oldOpts != SumOf(messH)->opts)
			messH->win->isDirty = true;
	}

	if (st == kEuSendNever) {
		if (IsQueued(tocH, sumNum))
			SetState(tocH, sumNum, SENDABLE);
		err = 0;
	}
	else if (!messH ||
		(SumOf(messH)->length != 0 && !messH->win->isDirty)
		|| SaveComp(messH->win))
	{
		if (tocH->sums[sumNum].state == UNSENDABLE) {
			WarnUser(CANT_QUEUE, 0);
			err = CANT_QUEUE;
		}
		else {
			/* Check for empty subject */
			if (PrefIsSet(PREF_SUBJECT_WARNING) &&
				!*tocH->sums[sumNum].subj &&
				!Mom(QUEUE_BTN, 0, PREF_SUBJECT_WARNING, R_FMT, SUBJECT_WARNING))
			{
				err = CANT_QUEUE;
			}
			/* Check for offensive content */
			else if (messH && AnalWarning(messH)) {
				err = CANT_QUEUE;
			}
			/* Check message size */
			else if (messH && QueueSizeWarning(messH)) {
				err = CANT_QUEUE;
			}
			/* Check translators */
			else if (messH && messH->hTranslators) {
				/* ETL translator check — original called ETLCanTransOut/ETLQueueMessage.
				 * Translators that can't run now block queuing. */
				/* TODO: Port ETLCanTransOut/ETLQueueMessage when translator system is ready */
				err = CANT_QUEUE;
			}
			else if (!messH || CloseMyWindow(GetMyWindowWindowPtr(messH->win)))
			{
				/* Moodwatch queue delay */
				if (!noAnalDelay && AnalDelayOutgoing() && state != TIMED &&
					(st == kEuSendNow || st == kEuSendNext))
				{
					AnalBox(tocH, sumNum, sumNum);
					if (tocH->sums[sumNum].score > GetRLong(ANAL_DELAY_LEVEL)) {
						st = kEuSendLater;
						state = TIMED;
						secs += 60 * GetRLong(ANAL_DELAY_MINUTES);
					}
				}
				SetState(tocH, sumNum, state);
				TimeStamp(tocH, sumNum, secs, ZoneSecs());
				err = 0;
				if (st == kEuSendNow) {
					err = XferMail(false, true, false, false, true, 0);
				}
			}
		}
	}
	SetSendQueue();
	return err;
}

/************************************************************************
 * QueueSizeWarning - check message size and warn if excessive
 *
 * Original: compact.c:341-354
 * Uses Aprintf/YES_CANCEL_ALRT in original, ported to GtkAlertDialog.
 ************************************************************************/
static int QueueSizeWarning(MessHandle messH)
{
	uLong max = GetRLong(MAX_MESSAGE_SIZE);
	uLong size;

	if (max && max < (size = ApproxMessageSize(messH))) {
		char msg[256];
		snprintf(msg, sizeof(msg),
			"This message is approximately %lu KB, which exceeds the warning threshold of %lu KB. Send anyway?",
			size, max);
		const char *buttons[] = { "Send", "Cancel", NULL };
		int response = run_alert_sync(
			messH->win ? GTK_WINDOW(messH->win->window) : NULL,
			msg, buttons);
		if (response != 0) /* anything other than "Send" */
			return CANT_QUEUE;
	}
	return 0;
}

/************************************************************************
 * ApproxMessageSize - return approximate size of message, in K
 *
 * Original: compact.c:359-377
 * Calculates body size + base64-expanded attachment sizes.
 ************************************************************************/
uLong ApproxMessageSize(MessHandle messH)
{
	long size = (SumOf(messH)->length * 41) / 40;
	FSSpec spec;
	short index;
	long oneSize;

	for (index = 1; ; index++) {
		if (GetIndAttachment(messH, index, &spec, NULL))
			break;
		if (FSpIsItAFolder(&spec))
			FolderSizeHi(spec.vRefNum, SpecDirId(&spec), &oneSize);
		else
			oneSize = FSpFileSize(&spec);
		size += (4 * oneSize) / 3; /* base64 expansion */
	}

	return (size + 512) / 1024; /* RoundDiv(size, 1024) */
}

/************************************************************************
 * CountAttachments - count the attachments in a message
 *
 * Original: compact.c:382-393
 ************************************************************************/
short CountAttachments(MessHandle messH)
{
	short nAttach;
	FSSpec spec;

	for (nAttach = 0; GetIndAttachment(messH, nAttach + 1, &spec, NULL) == 0; nAttach++)
		;
	return nAttach;
}

/************************************************************************
 * CompAttach - attach a document to a message via file chooser
 *
 * Original: compact.c:395-398 (called CompAttachNav)
 * Ported: Uses GTK4 GtkFileDialog (async with sync main loop spin).
 ************************************************************************/
void CompAttach(MyWindowPtr win, bool insertDefault)
{
	GtkFileDialog *dialog = gtk_file_dialog_new();
	gtk_file_dialog_set_title(dialog, "Attach File");

	SyncFileData data;
	data.loop = g_main_loop_new(NULL, FALSE);
	data.file = NULL;

	gtk_file_dialog_open(dialog,
		win ? GTK_WINDOW(win->window) : NULL,
		NULL, sync_file_open_cb, &data);

	g_main_loop_run(data.loop);
	g_main_loop_unref(data.loop);
	g_object_unref(dialog);

	if (data.file) {
		char *path = g_file_get_path(data.file);
		if (path) {
			FSSpec spec;
			memset(&spec, 0, sizeof(spec));
			g_strlcpy((char *)spec.name, path, sizeof(spec.name));
			CompAttachSpec(win, &spec);
			g_free(path);
		}
		g_object_unref(data.file);
	}
}

/************************************************************************
 * CompAttachStd - standard attach (same as CompAttach for GTK port)
 *
 * Original: compact.c:403 — was same as CompAttach on Mac too.
 ************************************************************************/
void CompAttachStd(MyWindowPtr win, bool insertDefault)
{
	CompAttach(win, insertDefault);
}

/************************************************************************
 * CompReallyPreferBody - move focus to body if currently in header
 *
 * Original: compact.c:408-420
 ************************************************************************/
void CompReallyPreferBody(MyWindowPtr win)
{
	if (win && win->pte) {
		short cur = CompHeadCurrent(win->pte);
		if (cur != 0) {
			/* Currently in a header field, switch to body */
			HeadSpec hs;
			if (CompHeadFind(Win2MessH(win), 0, &hs)) {
				/* Focus the body editor */
				gtk_widget_grab_focus(win->pte);
			}
		}
	}
}

/************************************************************************
 * CompAttachSpec - attach a specific file to a comp window
 *
 * Original: compact.c:425-488
 * Builds attachment text from file path and appends to attachments header.
 ************************************************************************/
void CompAttachSpec(MyWindowPtr win, FSSpec *spec)
{
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	if (!messH || !win->pte)
		return;

	HeadSpec hs;
	if (!CompHeadFind(messH, ATTACH_HEAD, &hs))
		return;

	/* Build attachment text from file path */
	char attachText[1024];
	snprintf(attachText, sizeof(attachText), "%s", (char *)spec->name);

	/* Append to attachments header */
	GtkWidget *pte = TheBody ? TheBody : win->pte;
	if (hs.stop != hs.value)
		CompHeadAppendPtr(pte, &hs, " ", 1);
	CompHeadAppendPtr(pte, &hs, attachText, strlen(attachText));

	AttachSelect(messH);
	win->isDirty = true;
}

/************************************************************************
 * CompUnattach - remove all attachments
 *
 * Original: compact.c:490-497
 * Clears attachment header by inserting empty text at selection.
 ************************************************************************/
void CompUnattach(MyWindowPtr win)
{
	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	if (!messH)
		return;

	/* Original: PETEInsertTextPtr(PETE, TheBody, -1, "", 0, nil) — delete selection */
	GtkWidget *pte = TheBody ? TheBody : win->pte;
	PETEInsertTextPtr(NULL, pte, -1, "", 0, NULL);
	AttachSelect(messH);
	win->isDirty = true;
}

/************************************************************************
 * InTranslator - check if a translator ID is in the translator list
 *
 * Original: compact.c:502-515
 ************************************************************************/
bool InTranslator(TransInfoHandle translators, long id)
{
	if (!translators)
		return false;

	long size = GetHandleSize_((Handle)translators);
	short count = size / sizeof(TransInfo);
	for (short i = 0; i < count; i++) {
		if ((*translators)[i].id == id)
			return true;
	}
	return false;
}

/************************************************************************
 * AddMessTranslator - add a translator to a message
 *
 * Original: compact.c:517-545
 * Signature matches compact.h: void *properties (not Handle)
 ************************************************************************/
int AddMessTranslator(MessHandle messH, long which, void *properties)
{
	short n;
	long id = ETLIconToID(which);

	if (which == -1)
		return -1;

	if (messH->hTranslators)
		n = GetHandleSize_((Handle)messH->hTranslators) / sizeof(TransInfo);
	else
		n = 0;

	if (!n) {
		TransInfoHandle localHandle = (TransInfoHandle)NewHandle(sizeof(TransInfo));
		if (!localHandle)
			return -1;
		messH->hTranslators = localHandle;
	} else {
		SetHandleSize((Handle)messH->hTranslators, (n + 1) * sizeof(TransInfo));
	}

	(*messH->hTranslators)[n].id = id;
	(*messH->hTranslators)[n].properties = (Handle)properties;

	ControlHandle theCtl = FindControlByRefCon(messH->win, 0xff000000 | (which + ICON_BAR_NUM));
	if (theCtl)
		SetControlValue(theCtl, 1);
	messH->win->isDirty = true;
	return 0;
}

/************************************************************************
 * RemoveMessTranslator - remove a translator from a message
 *
 * Original: compact.c:550-577
 * Uses BMD (block move) in original; ported to memmove.
 ************************************************************************/
int RemoveMessTranslator(MessHandle messH, long which)
{
	short n, i;
	long id = ETLIconToID(which);

	if (messH->hTranslators)
		n = GetHandleSize_((Handle)messH->hTranslators) / sizeof(TransInfo);
	else
		n = 0;

	for (i = 0; i < n; i++) {
		if ((*messH->hTranslators)[i].id == id) {
			if ((*messH->hTranslators)[i].properties)
				DisposeHandle((*messH->hTranslators)[i].properties);
			if (n == 1) {
				DisposeHandle((Handle)messH->hTranslators);
				messH->hTranslators = NULL;
			} else {
				if (i != n - 1)
					memmove(&(*messH->hTranslators)[i],
						&(*messH->hTranslators)[i + 1],
						(n - i - 1) * sizeof(TransInfo));
				SetHandleSize((Handle)messH->hTranslators, (n - 1) * sizeof(TransInfo));
			}
			break;
		}
	}

	ControlHandle theCtl = FindControlByRefCon(messH->win, 0xff000000 | (which + ICON_BAR_NUM));
	if (theCtl)
		SetControlValue(theCtl, 0);
	messH->win->isDirty = true;
	return 0;
}

/************************************************************************
 * WarpQueue - adjust timing of queued messages
 *
 * Original: compact.c:582-600
 ************************************************************************/
void WarpQueue(uLong secs)
{
	TOCType *tocH = GetOutTOC();
	uLong now = GMTDateTime();

	if (tocH) {
		for (int i = 0; i < tocH->count; i++) {
			MSumPtr sum = &tocH->sums[i];
			if (sum->state == TIMED) {
				if (!secs)
					sum->seconds = 0;
				else if (sum->seconds < secs + now) {
					sum->seconds = now;
					TOCSetDirty(tocH, true);
				}
			}
		}
		SetSendQueue();
	}
}

/************************************************************************
 * AttachDoc - attach a document to the topmost message, or create new
 *
 * Original: compact.c:605-642
 ************************************************************************/
int AttachDoc(MyWindowPtr win, FSSpec *spec)
{
	MessHandle messH;

	if (win && win->window) {
		messH = (MessHandle)GetMyWindowPrivateData(win);
		if (!messH || SENT_OR_SENDING(SumOf(messH)->state)) {
			win = DoComposeNew(0);
			if (!win)
				return 1;
		}
	} else {
		win = DoComposeNew(0);
		if (!win)
			return 1;
	}

	CompAttachSpec(win, spec);
	return 0;
}

/************************************************************************
 * ApplyStationery - apply stationery to a composition window
 *
 * Original: compact.c:644-647
 ************************************************************************/
void ApplyStationery(MyWindowPtr win, FSSpec *spec, bool dontCleanse, bool personality)
{
	ApplyStationeryLo(win, spec, dontCleanse, personality, false);
}

/************************************************************************
 * ApplyStationeryLo - lower-level stationery application
 *
 * Original: compact.c:647-667
 * Reads stationery file via Snarf, then applies via ApplyStationeryHandle.
 ************************************************************************/
void ApplyStationeryLo(MyWindowPtr win, FSSpec *spec, bool dontCleanse, bool personality, bool editStationery)
{
	Handle textH = NULL;

	if (HasFeature(featureStationery))
		;  /* UseFeature call in original — feature gate */

	/* Read stationery file */
	if (Snarf(spec, (void **)&textH, 0))
		return;
	if (!textH || !*textH) {
		if (textH) DisposeHandle(textH);
		return;
	}

	/* Apply — dereference handle to get raw pointer + length per compact.h signature */
	unsigned char *text = (unsigned char *)*textH;
	long textLen = GetHandleSize_(textH);
	ApplyStationeryHandle(win, text, textLen, dontCleanse, personality, editStationery);

	DisposeHandle(textH);
}

/************************************************************************
 * GetStationerySum - get message summary from stationery text
 *
 * Original: compact.c:672-690
 * Signature matches compact.h: (unsigned char *text, long textLen, MSumPtr pSum)
 ************************************************************************/
int GetStationerySum(unsigned char *text, long textLen, MSumPtr pSum)
{
	if (!text || textLen <= 0)
		return -1;

	unsigned char *spot = text;
	unsigned char *end = text + textLen;
	unsigned char *nl;

	/* Skip to first space (past "X-Eudora-Stationery:" prefix) */
	while (spot < end && *spot != ' ')
		spot++;
	spot++;

	/* Find end of first line */
	for (nl = spot; nl < end && *nl != '\015' && *nl != '\n'; nl++)
		;

	if ((long)(nl - spot) != 2 * (long)sizeof(*pSum)) {
		WarnUser(0, 0); /* INVALID_STATIONERY */
		return -1;
	}

	Hex2Bytes(spot, nl - spot, (unsigned char *)pSum);
	return 0;
}

/************************************************************************
 * ApplyStationeryHandle - apply stationery text to a composition window
 *
 * Original: compact.c:695-868
 * This is the main stationery application function. It:
 * - Extracts summary info from the stationery header line
 * - Copies relevant summary fields (flags, options, personality)
 * - Copies headers (To, Cc, Bcc, Attachments, Subject with special handling)
 * - Applies body text (plain or HTML)
 * - Adds inline signature if configured
 *
 * Signature matches compact.h: (win, text, textLen, dontCleanse, personality, editStationery)
 ************************************************************************/
void ApplyStationeryHandle(MyWindowPtr win, unsigned char *text, long textLen, bool dontCleanse, bool personality, bool editStationery)
{
	MessageSummary oldSum, newSum;
	MessHandle messH;
	unsigned char scratch[256];
	unsigned char origSubj[256];
	unsigned char *spot, *end;
	long bodySpot;
	long size;
	unsigned char *subj;
	HeadSpec hs;
	PersHandle pers = NULL;
	short label = editStationery ? 0 : pStationeryLabel;

	if (!text || textLen <= 0 || !win)
		return;

	messH = Win2MessH(win);
	if (!messH)
		return;

	end = text + textLen;

	/* Fetch summary info from first line */
	if (GetStationerySum(text, textLen, &oldSum))
		return;

	/* Copy pertinent stuff */
	newSum = *SumOf(messH);
	SumInfoCpy(&newSum, &oldSum);
	if (editStationery) {
		/* Editing stationery: keep original uid */
		newSum.msgIdHash = oldSum.msgIdHash;
		newSum.uidHash = oldSum.uidHash;
	}
	newSum.seconds = GMTDateTime();
	newSum.state = UNSENDABLE;
	if (!(oldSum.flags & FLAG_OLD_SIG))
		newSum.sigId = SIG_NONE;
	*SumOf(messH) = newSum;

	/* Handle personality */
	if (personality && (pers = FindPersById(oldSum.persId))) {
		SetPers(messH->tocH, messH->sumNum, pers, false);
		PushPers(pers);
	}

	/* Find body start — double-newline separates headers from body */
	for (spot = text; spot < end - 1; spot++) {
		if (spot[0] == '\015' && spot[1] == '\015')
			break;
		if (spot[0] == '\n' && spot[1] == '\n')
			break;
	}
	bodySpot = (spot < end - 2) ? (spot - text + 2) : (end - text);
	spot = text;

	/* From header — if it's "me", set the From field */
	size = bodySpot;
	subj = (unsigned char *)FindHeaderString((UPtr)spot, HeaderName(FROM_HEAD), &size, false);
	if (subj && size) {
		MakePStr(scratch, subj, size);
		if (IsMe((char *)scratch))
			SetMessText(messH, FROM_HEAD, scratch, strlen((const char *)scratch));
	}

	/* Translator header */
	size = bodySpot;
	subj = (unsigned char *)FindHeaderString((UPtr)spot, HeaderName(TRANSLATOR_HEAD), &size, false);
	if (subj && size) {
		AddTranslatorsFromPtr(messH, (char *)subj, size);
	}

	/* Copy headers from stationery */
	TextFindAndCopyHeader(spot, bodySpot, messH, HeaderName(TO_HEAD), TO_HEAD, label);
	TextFindAndCopyHeader(spot, bodySpot, messH, HeaderName(CC_HEAD), CC_HEAD, label);
	TextFindAndCopyHeader(spot, bodySpot, messH, HeaderName(BCC_HEAD), BCC_HEAD, label);
	TextFindAndCopyHeader(spot, bodySpot, messH, HeaderName(ATTACH_HEAD), ATTACH_HEAD, label);

	/* Subject gets special handling — if comp already has a subject, combine them */
	CompHeadGetStr(messH, SUBJ_HEAD, (char *)origSubj);
	if (*origSubj) {
		size = bodySpot;
		subj = (unsigned char *)FindHeaderString((UPtr)spot, HeaderName(SUBJ_HEAD), &size, false);
		if (subj && size) {
			unsigned char sub[256];
			MakePStr(sub, subj, size);
			TrimWhite(sub);
			if (*sub) {
				unsigned char into[512];
				ComposeRString(into, R_FMT, sub, origSubj);
				SetMessText(messH, SUBJ_HEAD, into + 1, *into);
				if (CompHeadFind(messH, SUBJ_HEAD, &hs)) {
					geditctrl_set_label(TheBody, hs.value, hs.stop, label);
				}
			} else {
				*origSubj = 0;
			}
		} else {
			*origSubj = 0;
		}
	}
	if (!*origSubj)
		TextFindAndCopyHeader(spot, bodySpot, messH, HeaderName(SUBJ_HEAD), SUBJ_HEAD, label);

	/* Handle spool folder copy for stationery with attachments */
	if (!editStationery && (oldSum.opts & OPT_HAS_SPOOL)) {
		FSSpec toSpec, fromSpec;
		MakeAttSubFolder(messH, oldSum.uidHash, &fromSpec);
		MakeAttSubFolder(messH, newSum.uidHash, &toSpec);
		FSpDupFolder(&toSpec, &fromSpec, true, false);
	}

	/* Apply body text */
	if (spot + bodySpot < end) {
		long newBodySpot = PeteLen(TheBody);
		if (oldSum.opts & OPT_HTML) {
			/* For HTML stationery, use InsertRich to parse markup */
			/* Need to create a temporary handle for InsertRich */
			Handle bodyH = NewHandle(textLen - bodySpot);
			if (bodyH) {
				memcpy(*bodyH, text + bodySpot, textLen - bodySpot);
				InsertRich((UHandle)bodyH, 0, textLen - bodySpot, newBodySpot, false, TheBody, NULL, false);
				DisposeHandle(bodyH);
			}
			SetMessOpt(messH, OPT_HTML);
		} else {
			/* Plain text stationery — avoid bug #3596 from original */
			bool oldSigOpt = MessOptIsSet(messH, OPT_INLINE_SIG);
			ClearMessOpt(messH, OPT_INLINE_SIG);

			AppendMessText(messH, 0, spot + bodySpot, end - spot - bodySpot);

			if (oldSigOpt)
				SetMessOpt(messH, OPT_INLINE_SIG);

			if (oldSum.flags & FLAG_RICH) {
				geditctrl_set_rich_text(TheBody, newBodySpot, true);
				SetMessFlag(messH, FLAG_RICH);
			}
		}
		/* Label and lock the inserted body text */
		geditctrl_set_label(TheBody, newBodySpot, PeteLen(TheBody), label);
		geditctrl_lock_range(TheBody, newBodySpot, PeteLen(TheBody), 0);
	}

	win->isDirty = false;
	PeteCleanList(win->pte);

	if (UseInlineSig)
		AddInlineSig(messH);

	RefreshSigButton(messH);

	UpdateSum(messH, SumOf(messH)->offset, SumOf(messH)->length);

	/* Update window's idea of what color it is */
	messH->win->label = GetSumColor(messH->tocH, messH->sumNum);
	InvalTopMargin(win);
	CompIBarUpdate(messH);

	if (pers)
		PopPers();
}

/************************************************************************
 * CompIBarUpdate - update the icon bar controls to match message flags
 *
 * Original: compact.c:1737-1753
 ************************************************************************/
void CompIBarUpdate(MessHandle messH)
{
	if (!messH)
		return;

	for (short i = 0; i < ICON_BAR_NUM; i++) {
		ControlHandle cntl = FindControlByRefCon(messH->win, 0xff000000 | i);
		if (cntl) {
			if (fBits[i] < 0)
				SetControlValue(cntl, MessOptIsSet(messH, (-fBits[i])));
			else
				SetControlValue(cntl, MessFlagIsSet(messH, fBits[i]));
		}
	}
	RefreshSigButton(messH);
}

/************************************************************************
 * CompLeaving - handle leaving a header field (nick expansion, etc.)
 *
 * Original: compact.c:1815-1841
 * Key behavior: only expands nicknames if the field was actually changed
 * (comparing PeteIsDirty to saved fieldDirty value).
 ************************************************************************/
int CompLeaving(MessHandle messH, short head)
{
	short fDirty = PeteIsDirty(TheBody);
	int err = 0;
	MyWindowPtr win = messH->win;

	if (messH->alreadyLeaving)
		return 0;

	messH->alreadyLeaving = true;

	/* Only do nick expansion if the field actually changed */
	if (fDirty != messH->fieldDirty) {
		PushPers(PERS_FORCE(MESS_TO_PERS(messH)));

		if (IsAddressHead(head))
			err = NickExpandAndCacheHead(messH, head, false);

		UpdateSum(messH, SumOf(messH)->offset, SumOf(messH)->length);

		fDirty = PeteIsDirty(TheBody);
		messH->fieldDirty = fDirty;

		PopPers();
	}

	EnableTxtFmtBarIfOK(win);
	messH->alreadyLeaving = false;
	return err;
}

/************************************************************************
 * NickExpandAndCacheHead - expand nicknames and cache addresses
 *
 * Original: compact.c:1844-1900
 * Full logic: extracts text from header field, sucks addresses out,
 * caches nicknames, optionally expands aliases and replaces field text
 * if it changed, then gathers recipient addresses.
 ************************************************************************/
int NickExpandAndCacheHead(MessHandle messH, short head, bool cacheOnly)
{
	int err = -1;
	HeadSpec hs;
	char *text = NULL;
	Handle raw = NULL; /* BinAddrHandle in original */

	GtkWidget *pte = TheBody ? TheBody : messH->win->pte;
	if (!pte)
		return err;

	if (CompHeadFind(messH, head, &hs) && hs.stop - hs.value > 0) {
		if (CompHeadGetText(pte, &hs, &text) == 0 && text) {
			/* Convert text to Handle for SuckAddresses */
			Handle textH = NewHandle(strlen(text));
			if (textH) {
				memcpy(*textH, text, strlen(text));

				if (SuckAddresses(&raw, (void *)textH, true, true, false, NULL) == 0) {
					DisposeHandle(textH);
					textH = NULL;

					NicknameCachingScan(pte, raw);

					if (PrefIsSet(PREF_NICK_AUTO_EXPAND) && !cacheOnly) {
						Handle expanded = NULL;
						if (ExpandAliases((void **)&expanded, raw, 0, true) == 0 && expanded) {
							if (raw) {
								DisposeHandle((Handle)raw);
								raw = NULL;
							}
							CommaList(expanded);
							long len = GetHandleSize_(expanded);
							if (len > 0) {
								/* Check if expansion caused any changes.
								 * Don't replace text if no changes so selection doesn't change. */
								long selStart = 0, selEnd = 0;
								void *fieldTextH = NULL;
								PeteGetTextAndSelection(pte, &fieldTextH, &selStart, &selEnd);

								long fieldLen = hs.stop - hs.value;
								bool changed = false;
								if (fieldTextH) {
									if (fieldLen != len || memcmp(*expanded, (char *)*(Handle)fieldTextH + hs.value, fieldLen))
										changed = true;
								} else {
									changed = true;
								}

								if (changed) {
									/* Text has changed — replace field contents */
									PetePrepareUndo(pte, 0/*peCantUndo*/, hs.value, hs.stop, NULL, NULL);
									if (PeteDelete(pte, hs.value, hs.stop) == 0) {
										if (PeteInsertPtr(pte, hs.value, *expanded, len) == 0) {
											/* If previous selection was entire field, reselect entire field */
											bool selectAll = (selStart == hs.value && selEnd == hs.stop);
											if (CompHeadCurrent(pte) == head && CompHeadFind(messH, head, &hs))
												PeteSelect(messH->win, pte, selectAll ? hs.value : hs.stop, hs.stop);
										}
									}
									PeteFinishUndo(pte, 1/*peUndoPaste*/, hs.value, hs.value + len);
								}
							}
							CompGatherRecipientAddresses(messH, true);
							DisposeHandle(expanded);
						}
					} else {
						/* Just cache, no expand */
						CompGatherRecipientAddresses(messH, true);
					}
				} else {
					if (textH) DisposeHandle(textH);
				}
			}
			g_free(text);
			text = NULL;
		}
	}

	if (raw)
		DisposeHandle((Handle)raw);
	return err;
}

/************************************************************************
 * SetSig - set the signature of a message
 *
 * Original: compact.c:909-945
 * Handles inline signature replacement.
 ************************************************************************/
void SetSig(TOCType *tocH, short sumNum, int sigId)
{
	MessHandle messH = tocH->sums[sumNum].messH;

	if (sigId == -1)
		tocH->sums[sumNum].sigId = SIG_NONE;
	else
		tocH->sums[sumNum].sigId = sigId;

	TOCSetDirty(tocH, true);

	if (messH) {
		bool winDirtyWas = messH->win->isDirty;

		if (messH->hStationerySpec)
			messH->win->isDirty = true;

		/* Handle inline signature replacement */
		if (MessOptIsSet(messH, OPT_INLINE_SIG))
			RemoveInlineSig(messH);
		if (UseInlineSig)
			AddInlineSig(messH);
		else
			messH->win->isDirty = winDirtyWas;
	}
}

/************************************************************************
 * SetAttachmentType - set the type of an attachment
 *
 * Original: compact.c:950-965
 ************************************************************************/
void SetAttachmentType(TOCType *tocH, short sumNum, short type)
{
	MessHandle messH = tocH->sums[sumNum].messH;

	SetAOptNumber(tocH->sums[sumNum].flags, type);
	TOCSetDirty(tocH, true);

	if (messH) {
		ControlHandle cntl = FindControlByRefCon(messH->win, ATTACH_MENU);
		if (cntl) {
			SetBevelIcon(cntl, type, 0, 0, NULL);
			SetBevelMenuValue(cntl, type + 1);
			if (messH->hStationerySpec)
				messH->win->isDirty = true;
		}
	}
}

/************************************************************************
 * CompActivateAppropriate - activate the appropriate field
 *
 * Original: compact.c — activates To: field for new messages,
 * body for sent messages.
 ************************************************************************/
void CompActivateAppropriate(MessHandle messH)
{
	if (!messH)
		return;

	MyWindowPtr win = messH->win;
	if (!win || !win->pte)
		return;

	/* Activate the To: field by default, or body if message is being sent */
	HeadSpec hs;
	short targetHead = SENT_OR_SENDING(SumOf(messH)->state) ? 0 : TO_HEAD;

	if (CompHeadFind(messH, targetHead, &hs))
		CompHeadActivate(win->pte, &hs);
}

/************************************************************************
 * CompSetFormatBarIcon - set the format bar icon's value
 ************************************************************************/
void CompSetFormatBarIcon(MyWindowPtr win, bool visible)
{
	ControlHandle cntl = FindControlByRefCon(win, 0xff000000);
	if (cntl)
		SetControlValue(cntl, visible ? 1 : 0);
}

/************************************************************************
 * AddPriorityPopup - add the priority popup to the composition window
 *
 * In GTK4, priority is handled by compose_window.c UI layer.
 ************************************************************************/
int AddPriorityPopup(MessHandle messH)
{
	if (!messH || !messH->win)
		return -1;

	/* Priority is handled by compose_window.c GTK4 UI layer */
	return 0;
}

/************************************************************************
 * CompDelAttachment - delete selected attachment
 *
 * Original: compact.c:2880-2910
 * Deletes attachment text and cleans up surrounding double-spaces.
 ************************************************************************/
void CompDelAttachment(MessHandle messH, void *hsPtr)
{
	HSPtr hs = (HSPtr)hsPtr;
	GtkWidget *pte = TheBody ? TheBody : messH->win->pte;
	long sel;
	HeadSpec hSpec;

	if (!pte)
		return;

	if (hs) {
		PeteDelete(pte, hs->start, hs->stop);
		sel = hs->start;
	} else {
		/* Delete current selection */
		PETEInsertTextPtr(NULL, pte, -1, NULL, 0, NULL);
		long selEnd;
		PeteGetTextAndSelection(pte, NULL, &sel, &selEnd);
	}

	/* Clean up double-spaces around the deletion point */
	CompHeadFind(messH, ATTACH_HEAD, &hSpec);
	gchar *textStr = NULL;
	geditDocument *doc = geditctrl_get_document(pte);
	if (doc)
		textStr = gedit_document_get_text(doc);

	if (textStr) {
		long len = strlen(textStr);
		if (sel > hSpec.value && sel >= 2 && sel <= len &&
			textStr[sel - 1] == ' ' && textStr[sel - 2] == ' ')
		{
			PeteDelete(pte, sel - 1, sel);
			sel--;
			hSpec.stop--;
		}
		/* Re-read text after possible delete */
		g_free(textStr);
		textStr = gedit_document_get_text(doc);
		if (textStr) {
			len = strlen(textStr);
			if (sel < hSpec.stop && sel < len && sel > 0 &&
				textStr[sel] == ' ' && textStr[sel - 1] == ' ')
			{
				PeteDelete(pte, sel, sel + 1);
				hSpec.stop--;
			}
			g_free(textStr);
		}
	}

	if (!hs && hSpec.stop == hSpec.value)
		CompSwitchFields(messH, true);
}

/************************************************************************
 * AttachSelect - select attachment text in the attachment header
 *
 * Original: compact.c:2943-2961
 * Tries to select a graphic near the current cursor position.
 ************************************************************************/
void AttachSelect(MessHandle messH)
{
	if (!messH)
		return;

	long selStart = 0, selEnd = 0;
	HeadSpec hs;
	GtkWidget *pte = TheBody ? TheBody : messH->win->pte;

	if (PeteGetTextAndSelection(pte, NULL, &selStart, &selEnd))
		return;

	if (!CompHeadFind(messH, ATTACH_HEAD, &hs))
		return;

	/* If selection is outside attachment header, nothing to do */
	if (selEnd < hs.value || selStart > hs.stop)
		return;

	/* Try to select a graphic near the midpoint of selection */
	long mid = (selStart + selEnd) / 2;
	bool selected = false;

	/* In GTK port, select the text range around mid that represents an attachment */
	if (mid - 2 > hs.value) {
		geditctrl_select_range(pte, mid - 2, mid + 2);
		selected = true;
	} else if (mid + 4 < hs.stop) {
		geditctrl_select_range(pte, mid, mid + 4);
		selected = true;
	}

	if (!selected)
		CompSwitchFields(messH, true);
}

/************************************************************************
 * ForceCompWindowRecalcAndRedraw - force window recalculation and redraw
 ************************************************************************/
void ForceCompWindowRecalcAndRedraw(MyWindowPtr win)
{
	if (!win)
		return;

	MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
	if (messH)
		PlaceMessErrNote(messH);

	/* Queue a redraw of the window content */
	if (win->window)
		gtk_widget_queue_draw(win->window);
}

/************************************************************************
 * CompSendBtnUpdate - make sure name is correct on send button
 *
 * Original: compact.c — updates button text based on queue/send/save state.
 ************************************************************************/
void CompSendBtnUpdate(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	if (!messH || !messH->sendButton)
		return;

	const char *label;
	if (messH->hStationerySpec)
		label = "Save";
	else if (PrefIsSet(PREF_AUTO_SEND))
		label = "Send";
	else
		label = "Queue";

	/* Update button label if it's a GtkButton */
	GtkWidget *btn = (GtkWidget *)messH->sendButton;
	if (GTK_IS_BUTTON(btn))
		gtk_button_set_label(GTK_BUTTON(btn), label);
}

/************************************************************************
 * CompUpdateScore - update the content analysis score display
 ************************************************************************/
void CompUpdateScore(MyWindowPtr win)
{
	MessHandle messH = Win2MessH(win);
	if (!messH)
		return;

	if (SENT_OR_SENDING(SumOf(messH)->state))
		return;

	/* Score display is handled by the GTK4 compose window UI layer */
}

/************************************************************************
 * ModifyQueue - show dialog to change queue timing of a message
 *
 * Original: compact.c:3128-3226
 * Full implementation with radio buttons for Now/Queue/Later/Unqueue,
 * time/date entries for timed sending.
 * Ported to GTK4 using custom GtkWindow with nested main loop.
 ************************************************************************/
bool ModifyQueue(short *state, uLong *when, bool swap)
{
	/* Create dialog window */
	GtkWidget *window = gtk_window_new();
	gtk_window_set_title(GTK_WINDOW(window), "Change Queuing");
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_window_set_default_size(GTK_WINDOW(window), 350, 300);

	ModifyQueueData data;
	data.loop = g_main_loop_new(NULL, FALSE);
	data.accepted = false;

	/* Radio buttons for queue options */
	data.radioNow = gtk_check_button_new_with_label("Send Now");
	data.radioQueue = gtk_check_button_new_with_label("Queue");
	data.radioLater = gtk_check_button_new_with_label("Send Later");
	data.radioUnqueue = gtk_check_button_new_with_label("Don't Send");

	/* Group the radio buttons */
	gtk_check_button_set_group(GTK_CHECK_BUTTON(data.radioQueue), GTK_CHECK_BUTTON(data.radioNow));
	gtk_check_button_set_group(GTK_CHECK_BUTTON(data.radioLater), GTK_CHECK_BUTTON(data.radioNow));
	gtk_check_button_set_group(GTK_CHECK_BUTTON(data.radioUnqueue), GTK_CHECK_BUTTON(data.radioNow));

	/* Time/date entries for "Send Later" */
	data.timeEntry = gtk_entry_new();
	data.dateEntry = gtk_entry_new();

	/* Set defaults based on current state */
	uLong secs = *when ? *when + ZoneSecs() : (uLong)time(NULL);
	{
		time_t whenTime = (time_t)secs;
		struct tm *tm = localtime(&whenTime);
		if (tm) {
			char timeBuf[32], dateBuf[32];
			strftime(timeBuf, sizeof(timeBuf), "%H:%M", tm);
			strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", tm);
			gtk_editable_set_text(GTK_EDITABLE(data.timeEntry), timeBuf);
			gtk_editable_set_text(GTK_EDITABLE(data.dateEntry), dateBuf);
		}
	}

	if (*when) {
		gtk_check_button_set_active(GTK_CHECK_BUTTON(data.radioLater), true);
	} else if (swap) {
		if (PrefIsSet(PREF_AUTO_SEND))
			gtk_check_button_set_active(GTK_CHECK_BUTTON(data.radioQueue), true);
		else
			gtk_check_button_set_active(GTK_CHECK_BUTTON(data.radioNow), true);
	} else {
		if (*state == QUEUED)
			gtk_check_button_set_active(GTK_CHECK_BUTTON(data.radioQueue), true);
		else
			gtk_check_button_set_active(GTK_CHECK_BUTTON(data.radioUnqueue), true);
	}

	/* Build layout */
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_widget_set_margin_start(vbox, 12);
	gtk_widget_set_margin_end(vbox, 12);
	gtk_widget_set_margin_top(vbox, 12);
	gtk_widget_set_margin_bottom(vbox, 12);

	gtk_box_append(GTK_BOX(vbox), data.radioNow);
	gtk_box_append(GTK_BOX(vbox), data.radioQueue);
	gtk_box_append(GTK_BOX(vbox), data.radioLater);

	GtkWidget *timeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_margin_start(timeBox, 24);
	gtk_box_append(GTK_BOX(timeBox), gtk_label_new("Time:"));
	gtk_box_append(GTK_BOX(timeBox), data.timeEntry);
	gtk_box_append(GTK_BOX(timeBox), gtk_label_new("Date:"));
	gtk_box_append(GTK_BOX(timeBox), data.dateEntry);
	gtk_box_append(GTK_BOX(vbox), timeBox);

	gtk_box_append(GTK_BOX(vbox), data.radioUnqueue);

	/* OK / Cancel buttons */
	GtkWidget *btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_widget_set_halign(btnBox, GTK_ALIGN_END);
	gtk_widget_set_margin_top(btnBox, 12);

	GtkWidget *cancelBtn = gtk_button_new_with_label("Cancel");
	GtkWidget *okBtn = gtk_button_new_with_label("OK");
	gtk_box_append(GTK_BOX(btnBox), cancelBtn);
	gtk_box_append(GTK_BOX(btnBox), okBtn);
	gtk_box_append(GTK_BOX(vbox), btnBox);

	g_signal_connect(okBtn, "clicked", G_CALLBACK(modq_ok_clicked), &data);
	g_signal_connect(cancelBtn, "clicked", G_CALLBACK(modq_cancel_clicked), &data);
	g_signal_connect(window, "close-request", G_CALLBACK(modq_close_request), &data);

	gtk_window_set_child(GTK_WINDOW(window), vbox);
	gtk_window_present(GTK_WINDOW(window));

	/* Run nested main loop until user responds */
	g_main_loop_run(data.loop);
	g_main_loop_unref(data.loop);

	bool result = false;

	if (data.accepted) {
		result = true;
		if (gtk_check_button_get_active(GTK_CHECK_BUTTON(data.radioUnqueue))) {
			*state = SENDABLE;
			*when = 0;
		} else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(data.radioNow))) {
			*state = SENT; /* caller converts to QUEUED for immediate send */
			*when = 0;
		} else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(data.radioQueue))) {
			*state = QUEUED;
			*when = 0;
		} else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(data.radioLater))) {
			*state = TIMED;
			/* Parse time/date from entries */
			const char *timeStr = gtk_editable_get_text(GTK_EDITABLE(data.timeEntry));
			const char *dateStr = gtk_editable_get_text(GTK_EDITABLE(data.dateEntry));
			struct tm tm;
			memset(&tm, 0, sizeof(tm));
			time_t now = time(NULL);
			struct tm *nowTm = localtime(&now);
			if (nowTm)
				tm = *nowTm;

			/* Parse time HH:MM */
			if (timeStr && *timeStr) {
				int h = 0, m = 0;
				if (sscanf(timeStr, "%d:%d", &h, &m) >= 1) {
					tm.tm_hour = h;
					tm.tm_min = m;
					tm.tm_sec = 0;
				}
			}
			/* Parse date YYYY-MM-DD */
			if (dateStr && *dateStr) {
				int y = 0, mo = 0, d = 0;
				if (sscanf(dateStr, "%d-%d-%d", &y, &mo, &d) >= 3) {
					tm.tm_year = y - 1900;
					tm.tm_mon = mo - 1;
					tm.tm_mday = d;
				}
			}
			*when = (uLong)mktime(&tm);
		}
		SetSendQueue();
	}

	gtk_window_destroy(GTK_WINDOW(window));
	return result;
}

/************************************************************************
 * PlotFlag - draw a flag icon in a rectangle
 *
 * Original drew to a Mac GrafPort. In GTK4, flag rendering is handled
 * by the compose_window.c UI layer using GtkImage/CSS.
 ************************************************************************/
void PlotFlag(Rect *r, bool on, short which)
{
	(void)r;
	(void)on;
	(void)which;
}

/************************************************************************
 * DrawPopIBox - draw a popup icon box
 *
 * Original drew a sicn in a rect. In GTK4, handled by compose_window.c.
 ************************************************************************/
void DrawPopIBox(Rect *r, short sicnId)
{
	(void)r;
	(void)sicnId;
}

/************************************************************************
 * DrawShadowBox - draw a shadow box
 *
 * Original used QuickDraw. In GTK4, handled by CSS styling.
 ************************************************************************/
void DrawShadowBox(Rect *r)
{
	(void)r;
}

/************************************************************************
 * SaveStationeryStuff - save stationery header and change filetype
 *
 * Original: compact.c:3332-3378
 * Writes the X-Eudora-Stationery header (hex-encoded summary) and
 * translator info to the stationery file.
 ************************************************************************/
int SaveStationeryStuff(short refN, MessHandle messH)
{
	if (!HasFeature(featureStationery))
		return 0;

	MessageSummary sum = *SumOf(messH);

	/* Set signature flag for stationery persistence */
	if (sum.sigId == SIG_NONE)
		sum.flags &= ~FLAG_OLD_SIG;
	else
		sum.flags |= FLAG_OLD_SIG;

	/* Encode summary as hex */
	unsigned char hexBuf[sizeof(MessageSummary) * 2 + 2];
	Bytes2Hex((unsigned char *)&sum, sizeof(sum), hexBuf);
	long hexLen = sizeof(MessageSummary) * 2;

	/* Write "X-Eudora-Stationery: " prefix */
	unsigned char scratch[64];
	GetRString(scratch, HEADER_STRN);  /* X-Stuff header prefix */
	long prefixLen = strlen((char *)scratch);
	if (write(refN, scratch, prefixLen) != prefixLen)
		return -1;

	/* Write hex-encoded summary */
	if (write(refN, hexBuf, hexLen) != hexLen)
		return -1;

	/* Write newline */
	if (write(refN, "\n", 1) != 1)
		return -1;

	/* Write translator info if present */
	if (messH->hTranslators)
		WriteTranslators(refN, messH->hTranslators);

	return 0;
}

/************************************************************************
 * GatherRecipientAddresses - gather To, CC, and BCC addresses
 *
 * Original was in a different source file. This implements the compact.h
 * signature: (MessHandle messH, char **dest, bool wantComments)
 *
 * Accumulates addresses from all recipient headers into a single
 * comma-separated string allocated with malloc.
 ************************************************************************/
int GatherRecipientAddresses(MessHandle messH, char **dest, bool wantComments)
{
	int err = 0;
	short headers[] = {TO_HEAD, CC_HEAD, BCC_HEAD};
	size_t totalLen = 0;
	char *result = NULL;

	for (int i = 0; !err && i < 3; i++) {
		HeadSpec hs;
		if (CompHeadFind(messH, headers[i], &hs) && hs.stop > hs.value) {
			GtkWidget *pte = TheBody ? TheBody : messH->win->pte;
			char *text = NULL;
			if (CompHeadGetText(pte, &hs, &text) == 0 && text) {
				size_t textLen = strlen(text);
				if (textLen > 0) {
					if (result) {
						/* Append with comma separator */
						size_t newLen = totalLen + 2 + textLen;
						char *newResult = realloc(result, newLen + 1);
						if (newResult) {
							result = newResult;
							result[totalLen] = ',';
							result[totalLen + 1] = ' ';
							memcpy(result + totalLen + 2, text, textLen);
							totalLen = newLen;
							result[totalLen] = '\0';
						}
					} else {
						result = malloc(textLen + 1);
						if (result) {
							memcpy(result, text, textLen);
							totalLen = textLen;
							result[totalLen] = '\0';
						}
					}
				}
				g_free(text);
			}
		}
	}

	*dest = result;
	return err;
}
