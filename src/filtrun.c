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

/************************************************************************
 * filtrun.c — Filter runtime engine
 * Ported from Mac Carbon/QuickDraw to GTK4/POSIX.
 * Copyright (C) 1994 QUALCOMM Incorporated
 ************************************************************************/

#include "filtrun.h"
#include "Globals.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "StrnDefs.h"
#include "StringUtil.h"
#include "FiltDefs.h"
#include "features.h"
#include "progress.h"
#include "log.h"
#include "messact.h"
#include "message.h"
#include "mailxfer.h"
#include "imapdownload.h"
#include "imapmailboxes.h"
#include "fileutil.h"
#include "junk.h"
#include "util.h"
#include "threading.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>


#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif

#define FILE_NUM 69
#define MAX_FILTER_PASS 10

/* Identifiers from original Mac headers not yet in the GTK port headers */
#ifndef OPT_NOTIFY
#define OPT_NOTIFY 0x0004
#endif
#ifndef eManFilter
#define eManFilter 7
#endif
#ifndef kNotEnoughTime
#define kNotEnoughTime (-2)
#endif
#ifndef MINI_MASK
#define MINI_MASK 0xFFFF
#endif

/* FiltersDecRef — decrement filter reference count; defined in mailxfer.c */
extern void FiltersDecRef(void);

/* IsDelivery — check if spec is a delivery mailbox */
extern bool IsDelivery(const char *path);

/* External declarations for functions not provided by the included headers */
extern void SetState(TOCType *tocH, short sumNum, short state);
extern short PrintClosedMessage(TOCType *tocH, short sumNum, bool now);
extern void MakeMessTitle(unsigned char *title, TOCType *tocH, int sumNum, bool b);
extern int OpenFilterMessages(char * spec);
extern void NotifyHelpers(int code, int event, TOCType *tocH);
extern void CheckSLIP(void);
extern void PlaySoundId(short id);
/* GMTDateTime and LocalDateTime are declared in include/util.h
 * (LocalDateTime maps to MyLocalDateTime with a fixed-width type).
 * Remove local conflicting declarations here.
 */
extern void *PERS_FORCE(void *pers);
extern void *TS_TO_PERS(TOCType *tocH, short sumNum);
extern short Prior2Display(short priority);
extern void PriorityHeader(char *s, short priority);
extern char *FindHeaderString(char *text, char *headerName, long *size, bool bodyToo);
extern void *OpenText(void *a, void *b, void *c, void *behindWP, bool d, char *title, bool e, bool f);
extern int ExpandAliases(void **h, void *raw, int n, bool deep);
extern bool ValidHash(unsigned long h);
extern bool HashAppearsInAliasFile(unsigned long hash, const char *file);
extern bool AppearsInAliasFile(const char *addr, const char *file);
/* GetWindowList, GetNextWindow, CloseMyWindow, ReZoomMyWindow, SendBehind
 * declared in mailbox.h (real implementations in mywindow.c) */
extern bool IsKnownWindowMyWindow(void *winWP);
extern short GetWindowKind(void *winWP);

/* Forward declarations for internal functions */
static bool DoesIntersectNick(char **nickAddresses, void **nickExpanded, char *spot, long len);
static bool DoesIntersectNickFile(const char *file, char *spot, long len);
static bool TermDateMatch(MTPtr mt, TOCType *tocH, short sumNum);
static bool TermJunkMatch(MTPtr mt, TOCType *tocH, short sumNum);
static bool TermPersMatch(MTPtr mt, TOCType *tocH, short sumNum);
static bool TermPriorMatch(MTPtr mt, TOCType *tocH, short sumNum);
static bool AnyFilters(FilterKeywordEnum fType);
static bool AnyFiltersLo(FilterKeywordEnum fType, FilterRecord *array, int count);
static bool RightFilterType(FilterKeywordEnum fType, short filter);
static bool FilterMatch(short filter, TOCType *tocH, short sumNum, FilterPBPtr fpb);
static int TakeFilterAction(short filter, FilterPBPtr fpb, bool noXfer);
static bool TermMatch(MTPtr mt, TOCType *tocH, short sumNum, FilterPBPtr fpb);
static bool TermPtrMatch(MTPtr mt, char *spot, char *end);
static void Filter1Postprocess(FilterKeywordEnum fType, FilterPBPtr fpb);
static bool TermExpMatch(MTPtr mt, char *spot, char *end, char **cache);
static void FiltLogMatch(short filter, TOCType *tocH, short sumNum);
static uLong FilterLastMatch(short filter);
static bool FromIntersectNickFile(MTPtr mt, TOCType *tocH, short sumNum);
static bool FromIntersectNickFileMatch(MTPtr mt, TOCType *tocH, short sumNum);

int FGlobalErr;

/*
 * Current filter set being iterated. Set before filter iteration loops
 * to support the three-pass architecture (PreFilters, main, PostFilters).
 */
static FilterRecord *gCurFilters = NULL;
static int gCurNFilters = 0;

/* All string functions (GetRString, ComposeRString, EqualStrRes,
 * FindHeaderString, PPtrFindSub, PPtrMatchLWSP) now operate on
 * C strings — no Pascal conversion needed. */

/* Helper: get Handle-based filter array + count for Pre/PostFilters */
static FilterRecord *HandleToFilterArray(void *h) {
	return (FilterRecord *)h;
}
static int HandleToFilterCount(void *h) {
	if (!h) return 0;
	return (int)(malloc_size(h) / sizeof(FilterRecord));
}

/* Safe string copy */
static void sstrncpy(char *dst, const char *src, size_t dstsize) {
	if (dstsize == 0) return;
	strncpy(dst, src, dstsize - 1);
	dst[dstsize - 1] = '\0';
}


/**********************************************************************
 * Filter1PostProcess - postprocessing of a single filter
 **********************************************************************/
static void Filter1Postprocess(FilterKeywordEnum fType, FilterPBPtr fpb)
{
	FSSpec spec;
	bool openPref = !PrefIsSet(PREF_NO_OPEN_IN);
	bool openBox = (openPref && !fpb->dontUser && fType==flkIncoming) || fpb->openMailbox;
	short which;

	if (fpb->xferred)
	{
		g_strlcpy(spec, fpb->spec, sizeof(spec));
		which = 0;
		if (!fpb->xferredFromIMAP)
		{
			if (IsRoot(spec))
			{
				if (EqualStrRes((unsigned char *)spec_name(spec),IN)) which = IN;
				else if (EqualStrRes((unsigned char *)spec_name(spec),OUT)) which = OUT;
				else if (EqualStrRes((unsigned char *)spec_name(spec),TRASH)) which = TRASH;
			}
		}
	}
	else
	{
		GetMailboxSpec(fpb->tocH,-1, spec);
		which = fpb->tocH->which;
		if (which==IN_TEMP)
		{
			TOCType *tocH = GetRealInTOC();
			which = IN;
			if (!tocH) return;
			GetMailboxSpec(tocH,-1, spec);
		}
			if (IsDelivery(spec))
		{
			unsigned char inName[256];
			GetRString((char *)inName, IN);
			spec_make(MailRoot.path, (char *)inName, &spec);
		}
	}

	/* IMAP - make sure we open/report the visible TOC, not the hidden one */
	if (fpb->tocH->imapTOC)
		GetRealIMAPSpec(spec, &spec);

	if (spec_name(spec)[0])
	{
		/* do we need to open the mailbox? */
		if (openBox && (fpb->xferred || !openPref))
			AddSpecToList(&spec, fpb->mailbox);

		/* do we need to open the message? */
		if (fpb->openMessage)
		{
			if (!IsIMAPMailboxFile(&spec) || !fpb->xferred)
				AddSpecToList(&spec, fpb->message);
			if (!fpb->xferred)
				fpb->tocH->sums[fpb->sumNum].opts |= OPT_OPEN;
		}

		/* do we need to do the report? */
		if ((fpb->doReport || (PrefIsSet(PREF_REPORT) && !which)) && !fpb->dontReport)
			AddSpecToList(&spec, fpb->report);
	}

	/* how about normal notification? */
	if (fpb->xferred || fpb->dontUser)
	{
		fpb->notify--;
		if (fpb->dontUser || (which==TRASH)) fpb->doNotifyThing--;
		if (!fpb->xferred)
			fpb->tocH->sums[fpb->sumNum].opts &= ~OPT_NOTIFY;
	}
	else
		fpb->tocH->sums[fpb->sumNum].opts |= OPT_NOTIFY;

	/* printing? */
	if (fpb->print && !fpb->xferred)
	{
		short oldstat = fpb->tocH->sums[fpb->sumNum].state;
		PrintClosedMessage(fpb->tocH, fpb->sumNum, true);
		SetState(fpb->tocH, fpb->sumNum, oldstat);
	}
}

/**********************************************************************
 * FilterPostProcess - take all the final actions needed after filtering
 **********************************************************************/
void FilterPostprocess(FilterKeywordEnum fType, FilterPBPtr fpb)
{
	(void)fType;
	CSpec cspec;
	short n;
	short i;
	void *behindWP = NULL;

	if (fpb->ccAddresses) { g_strfreev(fpb->ccAddresses); fpb->ccAddresses = NULL; }
	if (fpb->bccAddresses) { g_strfreev(fpb->bccAddresses); fpb->bccAddresses = NULL; }
	if (fpb->toAddresses) { g_strfreev(fpb->toAddresses); fpb->toAddresses = NULL; }

	/* first, we make the filter report */
	if (fpb->report && CSpecCount(fpb->report))
	{
		GenSpecWindow(fpb->report);
		g_array_free(fpb->report, TRUE);
		fpb->report = NULL;
	}

	/* Expunge any IMAP mailbox that may have been touched during filtering */
	IMAPPostFilterExpunge();

	/* now, we open mailboxes */
	if (fpb->mailbox)
	{
		TOCType *tocH = GetSpecialTOC(IN);
		void *tocWinWP = NULL;

		behindWP = OpenBehindMePlease();
		if (tocH && tocH->win)
		{
			tocWinWP = GetMyWindowWindowPtr(tocH->win);
			if (!PrefIsSet(PREF_NO_OPEN_IN) && fpb->notify && fType==flkIncoming && IsWindowVisible(tocWinWP))
			{
				if (!behindWP)
				{
					gtk_window_present(GTK_WINDOW(tocWinWP));
					behindWP = GetMyWindowWindowPtr(tocH->win);
				}
				else SendBehind(tocWinWP, behindWP);
			}
		}

		n = CSpecCount(fpb->mailbox);
		for (i = 0; i < n; i++)
		{
			cspec = CSpecAt(fpb->mailbox, i);
			if ((tocH = TOCBySpec(&cspec.spec)))
			{
				ShowBoxAt(tocH, tocH->previewPTE ? -1 : 0, behindWP);
				if (PrefIsSet(PREF_ZOOM_OPEN))
					ReZoomMyWindow(GetMyWindowWindowPtr(tocH->win));
				if (!behindWP && tocWinWP)
					UpdateMyWindow(tocWinWP);
				behindWP = GetMyWindowWindowPtr(tocH->win);
			}
		}
	}

	/* now, we open messages */
	if (fpb->message)
	{
		n = CSpecCount(fpb->message);
		while (n--)
		{
			cspec = CSpecAt(fpb->message, n);
			OpenFilterMessages(&cspec.spec);
		}
	}

	/* and sounds */
	if (fpb->sounds)
	{
		n = (fpb->sounds ? malloc_size(fpb->sounds) / sizeof(*(fpb->sounds)) : 0);
		while (n--)
			PlaySoundId(fpb->sounds[n]);
	}

	/* Resynchronize any IMAP mailbox that may have been touched during filtering */
	if (!gSkipIMAPBoxes)
	{
		IMAPStopFiltering(true);
		IMAPPostFilterResync();
	}

	/* done */
	if (fpb->message) { g_array_free(fpb->message, TRUE); fpb->message = NULL; }
	if (fpb->mailbox) { g_array_free(fpb->mailbox, TRUE); fpb->mailbox = NULL; }
	if (fpb->report) { /* already freed above in some paths, but safe */ g_array_free(fpb->report, TRUE); fpb->report = NULL; }
	free(fpb->sounds);
}

/**********************************************************************
 * InitFPB - initialize an FPB
 **********************************************************************/
int InitFPB(FilterPBPtr fpb, bool zapAddrs, bool listsToo)
{
	FilterPB saveFPB = *fpb;

	if (zapAddrs)
	{
		if (fpb->toAddresses) { g_strfreev(fpb->toAddresses); fpb->toAddresses = NULL; }
		if (fpb->ccAddresses) { g_strfreev(fpb->ccAddresses); fpb->ccAddresses = NULL; }
		if (fpb->bccAddresses) { g_strfreev(fpb->bccAddresses); fpb->bccAddresses = NULL; }
	}

	memset(fpb, 0, sizeof(*fpb));

	/* get the header names */
	{
		char tmp[256];
		GetRString(tmp, HEADER_STRN+TO_HEAD);
		g_strlcpy(fpb->to, tmp, sizeof(fpb->to));
		GetRString(tmp, HEADER_STRN+CC_HEAD);
		g_strlcpy(fpb->cc, tmp, sizeof(fpb->cc));
		GetRString(tmp, HEADER_STRN+BCC_HEAD);
		g_strlcpy(fpb->bcc, tmp, sizeof(fpb->bcc));
	}

	if (listsToo)
	{
		/* Initialize GLib-based CSpec lists */
		fpb->message = g_array_new(FALSE, FALSE, sizeof(CSpec));
		fpb->mailbox = g_array_new(FALSE, FALSE, sizeof(CSpec));
		fpb->report = g_array_new(FALSE, FALSE, sizeof(CSpec));
		if (!fpb->message || !fpb->mailbox || !fpb->report)
			return(0);
	}
	else
	{
		fpb->message = saveFPB.message;
		fpb->mailbox = saveFPB.mailbox;
		fpb->report = saveFPB.report;
		fpb->sounds = saveFPB.sounds;
		fpb->doNotifyThing = saveFPB.doNotifyThing;
		fpb->notify = saveFPB.notify;
	}

	return 0;
}

/************************************************************************
 * FilterSelectedMessage - filter the selection from a mailbox
 ************************************************************************/
int FilterSelectedMessages(FilterKeywordEnum fType, TOCType *tocH, FilterPBPtr fpb)
{
	short err = 0;
	short sumNum;
	short countWas;
	short lastSelected = -1;
	short initialCount = tocH->count;
	bool isOut = tocH->which==OUT;
	uLong pTicks = 0;
	long count;
	TOCType *realTocH;
	short realSumNum;
	long realSerialNum;
	FSSpec inSpec;
	TOCType *inTocH;
	bool justFakingIt = false;
	bool bHidden = false;

	if (fType==flkIncoming)
	{
		if (!(inTocH = GetRealInTOC())) return(ECANCELED);
		GetMailboxSpec(inTocH, -1, inSpec);
	}

	if ((err = InitFPB(fpb, false, true))) return(WarnUser(MEM_ERR, err));

	if (!PrefIsSet(PREF_MA))
	{
		if ((err = RegenerateFilters())) return(err);
		justFakingIt = fType==flkIncoming && !AnyFilters(fType);
		if (justFakingIt || AnyFilters(fType))
		{
			if (tocH->imapTOC) IMAPStartManualFiltering();
			count = CountSelectedMessages(tocH);
			if (count > 10) OpenProgress();
			ProgressMessageR(kpTitle, FILTERING);
			ProgressMessageR(kpSubTitle, LEFT_TO_PROCESS);
			Progress(NoBar, count, NULL, NULL, NULL);
			for (sumNum = 0; sumNum < tocH->count; sumNum++)
			{
				if (count > 1) MiniEvents();
				if (CommandPeriod) break;
				if (tocH->sums[sumNum].selected)
				{
					fpb->notify++;
					if (!(--count%10) || TickCount()-pTicks > 30)
					{
						Progress(NoBar, count, NULL, NULL, NULL);
						pTicks = TickCount();
					}
					lastSelected = sumNum;
					countWas = tocH->count;

					/* load up cache */
					if ((realTocH = GetRealTOC(tocH, sumNum, &realSumNum)))
					{
						realSerialNum = realTocH->sums[realSumNum].serialNum;

						CacheMessage(realTocH, realSumNum);

						if (!realTocH->imapTOC)
						{
							if (!realTocH->sums[realSumNum].cache)
							{
								WarnUser(MEM_ERR, 0);
								err = 1;
							}
						}

						/* do the filtering */
						if (!err && !justFakingIt)
							err = FilterMessageLo(fType, realTocH, realSumNum, fpb, false);

						if (!err && fType==flkIncoming)
						{
							if (!(err = MoveMessageLo(realTocH, realSumNum, &inSpec, false, false, true)))
								err = euFilterXfered;
						}

						/* Hide the message if it was deleted */
						if (tocH->imapTOC)
							bHidden = ShowHideFilteredSummary(tocH, sumNum);

						if (bHidden || ((err==euFilterXfered) && !tocH->imapTOC && !tocH->virtualTOC))
						{
							sumNum--;
							InvalContent(tocH->win);
							UpdateMyWindow(GetMyWindowWindowPtr(tocH->win));
						}

						if (err==euFilterXfered)
						{
							if (realTocH->imapTOC)
								tocH->sums[sumNum].u.virtualMess.virtualMBIdx = -1;
							if (!bHidden)
								InvalSum(tocH, sumNum);
						}
						else
						{
							if (realTocH->sums[realSumNum].cache)
								/* no-op: heap memory doesn't need purge */;
						}
					}
					if (err==euFilterStop || err==euFilterXfered) err = 0;
					if (err || CommandPeriod) break;
				}
			}

			IMAPStopFiltering(true);
			CloseProgress();
		}
		FiltersDecRef();
	}
	NotifyHelpers(0, eManFilter, tocH);
	if (initialCount > tocH->count && !CommandPeriod)
		BoxSelectAfter(tocH->win, lastSelected);
	return(err);
}

/************************************************************************
 * FilterFlaggedMessages - filter the flagged messages in a mailbox
 ************************************************************************/
int FilterFlaggedMessages(FilterKeywordEnum fType, TOCType *tocH, FilterPBPtr fpb)
{
	short err = 0;
	short sumNum;
	short countWas;
	short lastSelected = -1;
	short initialCount = tocH->count;
	bool isOut = tocH->which==OUT;
	uLong pTicks = 0;
	long count = CountFlaggedMessages(tocH);

	fpb->doNotifyThing = fpb->notify = count;

	if ((err = RegenerateFilters())) return(err);
	if (AnyFilters(fType))
	{
		if (count > 5) OpenProgress();
		ProgressMessageR(kpTitle, FILTERING);
		ProgressMessageR(kpSubTitle, LEFT_TO_PROCESS);
		Progress(NoBar, count, NULL, NULL, NULL);
		fpb->doNotifyThing = fpb->notify = 0;
		for (sumNum = 0; sumNum < tocH->count; sumNum++)
		{
			if (count > 1) MiniEvents();
			if (CommandPeriod) break;
			if (tocH->sums[sumNum].flags & FLAG_UNFILTERED)
			{
				fpb->notify++;
				fpb->doNotifyThing++;
				if (!(--count%10) || TickCount()-pTicks > 30)
				{
					Progress(NoBar, count, NULL, NULL, NULL);
					pTicks = TickCount();
				}
				lastSelected = sumNum;
				countWas = tocH->count;

				/* load up cache */
				CacheMessage(tocH, sumNum);

				if (!tocH->imapTOC)
				{
					if (!tocH->sums[sumNum].cache)
					{
						WarnUser(MEM_ERR, 0);
						err = 1;
					}
					else
						/* no-op: heap memory */;
				}

				/* do the filtering */
				if (!err) err = FilterMessageLo(fType, tocH, sumNum, fpb, false);

				/* clean up after */
				if (err==euFilterStop || err==euFilterXfered) err = 0;
				if (err || CommandPeriod) break;
			}
		}
		IMAPStopFiltering(true);
		CloseProgress();
	}
	FiltersDecRef();
	NotifyHelpers(0, eManFilter, tocH);
	if (initialCount > tocH->count && !CommandPeriod)
		BoxSelectAfter(tocH->win, lastSelected);
	return(err);
}

/************************************************************************
 * FilterIMAPTocIncrementally - spam score and filter messages in
 *	an IMAP mailbox a chunk at a time.
 ************************************************************************/
int FilterIMAPTocIncrementally(TOCType *tocH, FilterPBPtr fpb, bool noXfer)
{
	short err = 0;
	short sumNum;
	short filterHogTicks = GetRLong(FILTER_HOG_TICKS);
	uLong startTick = TickCount();
	bool dirty = false;
	long spamThresh = GetRLong(JUNK_MAILBOX_THRESHHOLD);
	FSSpec mailboxspec;

	mailboxspec[0] = '\0';

	if ((err = RegenerateFilters())) return(err);

	for (sumNum = 0; sumNum < tocH->count; sumNum++)
	{
		if ((tocH->sums[sumNum].flags & FLAG_UNFILTERED) == 0)
			continue;

		if (EventPending())
			return kNotEnoughTime;
		if (TickCount() - startTick > filterHogTicks)
			if (dirty) return kNotEnoughTime;

		if (CommandPeriod) break;
		CheckSLIP();
		fpb->doNotifyThing++;
		fpb->notify++;

		/* score the message */
		if (HasFeature(featureJunk) && false)
		{
			JunkScoreIMAPBox(tocH, sumNum, sumNum, false);

			if (JunkPrefBoxHold() && (tocH->sums[sumNum].spamScore >= spamThresh))
			{
				tocH->sums[sumNum].flags &= ~FLAG_UNFILTERED;
				free(tocH->sums[sumNum].cache);
				fpb->doNotifyThing--;
				err = euFilterXfered;
			}
		}

		/* do the filtering */
		if (!err)
		{
			if (tocH->sums[sumNum].messH == NULL)
			{
				err = FilterMessageLo(flkIncoming, tocH, sumNum, fpb, noXfer);
				tocH->sums[sumNum].flags &= ~FLAG_UNFILTERED;
			}
		}
		dirty = true;

		/* clean up after */
		if (err != euFilterXfered)
		{
			if (spec_name(mailboxspec)[0] == 0)
				GetRealIMAPSpec(tocH->mailbox.spec, &mailboxspec);

			if (spec_name(mailboxspec)[0])
			{
				if (!PrefIsSet(PREF_NO_OPEN_IN))
					AddSpecToList(&mailboxspec, fpb->mailbox);
			}

			if (tocH->sums[sumNum].cache)
				free(tocH->sums[sumNum].cache);
		}

		if (ShowHideFilteredSummary(tocH, sumNum))
			sumNum--;

		if (err==euFilterStop || err==euFilterXfered) err = 0;
		if (CommandPeriod) break;
	}

	if (CommandPeriod)
		IMAPFilteringCancelled(true);

	if (err)
	{
		Aprintf(OK_ALRT, Note, THREAD_PUNT_FILTER_ERR, err);
		NeedToFilterIMAP = false;
	}
	else
	{
		SetIMAPMailboxNeeds(TOCToMbox(tocH), kNeedsFilter, false);

		PushPers(CurPers);
		CurPers = TOCToPers(tocH);
		if (CurPers && JunkPrefBoxHold() && !JunkPrefIMAPNoRunPlugins())
			MoveToIMAPJunk(tocH, -1, spamThresh, fpb);
		PopPers();
	}

	CloseProgress();
	FiltersDecRef();

	return(err);
}

/************************************************************************
 * GenSpecWindow - open the spec window (filter report)
 ************************************************************************/
void GenSpecWindow(CSpecHandle specList)
{
	short n;
	short i;
	char s[256];
	char date[64];
	void *win = NULL;
	void *frontWin;
	void *winWP;
	void *frontWinWP;
	FSSpec spec;
	long len;

	if (!specList || !CSpecCount(specList)) return;

	TimeString(LocalDateTime(), false, date, NULL);
	GetRString(s, SPEC_TITLE);

	/* find topmost filter window */
	frontWinWP = GetWindowList();
	frontWin = GetWindowMyWindowPtr(frontWinWP);

	/* Look for existing text window for filter report */
	for (winWP = frontWinWP; winWP; winWP = GetNextWindow(winWP))
	{
		void *w = GetWindowMyWindowPtr(winWP);
		if (IsKnownWindowMyWindow(winWP) && GetWindowKind(winWP)==TEXT_WIN && w)
			break;
	}

	if (!winWP)
	{
		win = OpenText(NULL, NULL, NULL, NULL, false, s, true, false);
	}

	if (win)
	{
		/* TODO: full GTK text window implementation for filter report */
		/* For now the structure is preserved; actual display needs GTK widgets */
		ComposeRString(s, SPEC_INTRO, date);
		n = CSpecCount(specList);
		for (i = 0; i < n; i++)
		{
			g_strlcpy(spec, CSpecAt(specList, i).spec, sizeof(spec));
			ComposeRString(s, SPEC_FMT, spec_name(spec), CSpecAt(specList, i).count);
		}
	}
}

/************************************************************************
 * FilterMessagesFrom - filter messages after a particular spot
 ************************************************************************/
int FilterMessagesFrom(FilterKeywordEnum fType, TOCType *tocH, short startWith, FilterPBPtr fpb, bool noXfer)
{
	short err;
	short sumNum;
	short countWas;
	void *win;
	bool isOut = tocH->which==OUT;
	short count;
	short filterHogTicks = GetRLong(FILTER_HOG_TICKS);
	uLong startTick = TickCount();
	bool noInterruptions = false;
	bool deliveryBatch = fType==flkDelivery;
	bool dirty = false;
	FSSpec inSpec;
	bool isTempIn = tocH->which==IN_TEMP;
	TOCType *realInTocH = GetRealInTOC();

	if (!realInTocH) return(1);
	if (tocH->imapTOC) return(1);

	GetMailboxSpec(realInTocH, -1, inSpec);

	if (deliveryBatch)
		count = 0;  /* incremental filtering */
	else
		count = tocH->count - startWith;

	if ((err = RegenerateFilters())) return(err);

	if (fType==flkDelivery)
		fType = flkIncoming;

	if (AnyFilters(fType))
	{
		if (!deliveryBatch || noInterruptions)
			OpenProgress();
		ProgressR(NoBar, count, FILTERING, LEFT_TO_FILTER, NULL);
		for (sumNum = startWith; sumNum < tocH->count; sumNum++)
		{
			if (deliveryBatch && !noInterruptions)
			{
				if (EventPending())
					return kNotEnoughTime;
				if (TickCount() - startTick > filterHogTicks)
				{
					if (dirty) return kNotEnoughTime;
				}
			}
			if (!deliveryBatch)
				MiniEvents();
			if (CommandPeriod) break;
			CheckSLIP();
			fpb->doNotifyThing++;
			fpb->notify++;
			countWas = tocH->count;
			Progress(NoBar, count--, NULL, NULL, NULL);

			/* load up message or cache */
			if (isOut)
			{
				win = GetAMessage(tocH, sumNum, NULL, NULL, false);
				if (!win) err = 1;
			}
			else
			{
				CacheMessage(tocH, sumNum);
				if (!tocH->sums[sumNum].cache)
				{
					WarnUser(MEM_ERR, 0);
					err = 1;
				}
				else
					/* no-op: heap memory */;
			}

			/* do the filtering */
			if (!err) err = FilterMessageLo(fType, tocH, sumNum, fpb, noXfer);
			dirty = true;

			/* clean up after */
			if (err==euFilterXfered)
				sumNum--;
			else
			{
				if (isOut)
				{
					void *winWP = GetMyWindowWindowPtr(win);
					if (winWP && !IsWindowVisible(winWP)) CloseMyWindow(winWP);
				}
				else
				{
					if ((deliveryBatch || isTempIn) && realInTocH)
					{
						if (!(err = MoveMessageLo(tocH, sumNum, &inSpec, false, false, true)))
							sumNum--;
						else
						{
							WarnUser(WRITE_MBOX, err);
							break;
						}
					}
				}
			}
			if (err==euFilterStop || err==euFilterXfered) err = 0;
			if (CommandPeriod) break;
		}
	}
	/* just transfer messages to in box */
	else
	{
		if ((deliveryBatch || isTempIn) && realInTocH)
		{
			count = tocH->count;
			if (!deliveryBatch)
				OpenProgress();
			ProgressR(NoBar, count, MOVING_MESSAGES_TO_IN, LEFT_TO_MOVE, NULL);
			for (sumNum = 0; sumNum < tocH->count; sumNum++)
			{
				fpb->doNotifyThing++;
				fpb->notify++;
				if (!noInterruptions)
				{
					if (EventPending())
						return kNotEnoughTime;
					if (TickCount() - startTick > filterHogTicks)
					{
						if (dirty) return kNotEnoughTime;
					}
				}
				Progress(NoBar, count--, NULL, NULL, NULL);
				if (CommandPeriod) break;
				if (!(err = MoveMessageLo(tocH, sumNum, &inSpec, false, false, true)))
					sumNum--;
				else
				{
					WarnUser(WRITE_MBOX, err);
					break;
				}
				dirty = true;
			}
		}
		else if (realInTocH)
		{
			fpb->doNotifyThing = count;
			fpb->notify = count;
		}
	}
	if (deliveryBatch)
	{
		if (err)
		{
			Aprintf(OK_ALRT, Note, THREAD_PUNT_FILTER_ERR, err);
			NeedToFilterIn = false;
		}
	}
	CloseProgress();

	if (realInTocH) BoxFClose(realInTocH, true);
	FiltersDecRef();
	return(err);
}

/**********************************************************************
 * FilterMessage - filter a single message, including postprocessing
 **********************************************************************/
int FilterMessage(FilterKeywordEnum fType, TOCType *tocH, short sumNum)
{
	FilterPB fpb;
	int err;

	InitFPB(&fpb, false, true);
	err = FilterMessageLo(fType, tocH, sumNum, &fpb, false);
	if (fType==flkIncoming && err != euFilterXfered)
		TransferMenuChoice(TRANSFER_MENU, TRANSFER_IN_ITEM, tocH, sumNum, 0, false);
	FilterPostprocess(fType, &fpb);

	/* Hide the message if it was deleted from an IMAP mailbox */
	if ((fType == flkManual) && tocH->imapTOC)
		ShowHideFilteredSummary(tocH, sumNum);

	return(err);
}


/************************************************************************
 * FilterMessageLo - filter a message; needs to be setup first
 ************************************************************************/
int FilterMessageLo(FilterKeywordEnum fType, TOCType *tocH, short sumNum, FilterPBPtr fpb, bool noXfer)
{
	short err = 0;
	bool done = false;
	char title[256];
	short oldCount = tocH->count;
	bool oldSensitive = Sensitive;
	short f;
	short n;
	bool openIncomingErr = false;
	bool unjunking = fType==flkIncoming && tocH->which==JUNK;

	Sensitive = false;  /* do all filtering insensitively */

	NukeXfUndo();
	MakeMessTitle(title, tocH, sumNum, true);
	ProgressMessage(kpMessage, title);
	CycleBalls();

	if ((err = RegenerateFilters())) return(err);
	CacheMessage(tocH, sumNum);
	if (!tocH->imapTOC)
	{
		if (!tocH->sums[sumNum].cache)
		{
			WarnUser(GENERAL, 0);
			return(1);
		}
	}

	openIncomingErr = (fType==flkIncoming && PrefIsSet(PREF_OPEN_IN_ERR_MESS)
		& (tocH->which==IN || tocH->which==IN_TEMP)
		& tocH->sums[sumNum].state==MESG_ERR);

	if (AnyFilters(fType) || openIncomingErr)
	{
		/* Three-pass filter architecture: PreFilters, main filters, PostFilters */
		FilterRecord *passArrays[3];
		int passCounts[3];
		short filterIdx;

		passArrays[0] = HandleToFilterArray(PreFilters);
		passCounts[0] = HandleToFilterCount(PreFilters);
		passArrays[1] = gFilterArray;
		passCounts[1] = gNFilters;
		passArrays[2] = HandleToFilterArray(PostFilters);
		passCounts[2] = HandleToFilterCount(PostFilters);

		InitFPB(fpb, true, false);
		fpb->tocH = tocH;
		fpb->sumNum = sumNum;
		fpb->openMessage = openIncomingErr;

		for (filterIdx = 0; filterIdx < 3 && !err; filterIdx++)
		{
			gCurFilters = passArrays[filterIdx];
			gCurNFilters = passCounts[filterIdx];
			if (!gCurFilters || !gCurNFilters) continue;

			n = gCurNFilters;
			for (f = 0; f < n && !FGlobalErr; f++)
			{
				if (RightFilterType(fType, f))
				{
					MiniEvents();
					if (CommandPeriod) break;
					CycleBalls();
					if (FilterMatch(f, tocH, sumNum, fpb) && !FGlobalErr)
					{
						err = TakeFilterAction(f, fpb, noXfer);
						if (err) break;
					}
				}
			}
		}
		/* Restore to main filters */
		gCurFilters = gFilterArray;
		gCurNFilters = gNFilters;

		Filter1Postprocess(fType, fpb);
	}
	FiltersDecRef();

	/* Kill the message cache if we filled it with minimal headers for IMAP filtering */
	if (oldCount==tocH->count && tocH->sums[sumNum].cache && tocH->sums[sumNum].offset < 0)
	{
		free(tocH->sums[sumNum].cache);
		tocH->sums[sumNum].cache = NULL;
	}

	Sensitive = oldSensitive;

	if (err==euFilterStop && unjunking) err = 0;

	return(err);
}

/************************************************************************
 * AddSpecToList - add an FSSpec to a list of FSSpecs
 ************************************************************************/
void AddSpecToList(char * spec, CSpecHandle specList)
{
	short n;
	CSpec cspec;
	if (!specList) return;

	if (IsRoot(spec) && EqualStrRes(spec_name(spec), TRASH)) return;

	n = CSpecCount(specList);
	while (n--)
		if (SameSpec(&CSpecAt(specList, n).spec, spec))
		{
			CSpecAt(specList, n).count++;
			return;
		}

	g_strlcpy(cspec.spec, spec, sizeof(cspec.spec));
	cspec.count = 1;
	CSpecAppend(specList, cspec);
}

/************************************************************************
 * TermMatch - does a message match a term?
 ************************************************************************/
static bool TermMatch(MTPtr mt, TOCType *tocH, short sumNum, FilterPBPtr fpb)
{
	char *text;
	long bodyOffset;
	char *end;
	char *spot;
	char *hEnd;
	bool match = false;
	long size;
	bool hasColon;
	bool foundHeader = false;
	char *addr_cache = NULL;

	/* Set things up to do filtering to or in an IMAP mailbox */
	if (!IMAPStartFiltering(tocH, (tocH->imapTOC && (tocH->sums[sumNum].offset==-1))))
	{
		IMAPError(kIMAPSearching, kIMAPSelectMailboxErr, errIMAPSearchMailboxErr);
		CommandPeriod = true;
		return false;
	}

	/* if the message to be filtered has not been downloaded */
	if (tocH->imapTOC && (tocH->sums[sumNum].offset==-1))
	{
		if (mt->headerID != FILTER_BODY)
		{
			if (!tocH->sums[sumNum].cache)
			{
				void *hCache;
				if ((hCache = IMAPFetchMessageHeadersForFiltering(tocH, sumNum)) != NULL)
				{
					tocH->sums[sumNum].cache = hCache;
				}
			}

			if (tocH->imapTOC && !tocH->sums[sumNum].cache)
			{
				return false;
			}

			if (tocH->sums[sumNum].cache)
			{
				text = (char *)tocH->sums[sumNum].cache;
				bodyOffset = strlen((char *)tocH->sums[sumNum].cache);
			}
			else
			{
				text = NULL;
			}
		}
	}
	else
	{
		if (!tocH->sums[sumNum].cache)
		{
			return false;
		}

		text = (char *)tocH->sums[sumNum].cache;
		bodyOffset = tocH->sums[sumNum].bodyOffset;
	}

	/* make sure we've got a valid cache to work with */
	if (tocH->imapTOC && !tocH->sums[sumNum].cache)
	{
		return false;
	}

	if (mt->header[0] && mt->headerID != FILTER_BODY)	/* we DO have a header to look for */
	{
		if (!strcasecmp(mt->header, "date"))
			match = TermDateMatch(mt, tocH, sumNum);
		else if (HasFeature(featureJunk) && EqualStrRes(mt->header, FiltMetaEnglishStrn+fmeJunk))
		{
			UseFeature(featureJunk);
			match = TermJunkMatch(mt, tocH, sumNum);
		}
		else if (EqualStrRes(mt->header, FiltMetaEnglishStrn+fmePersonality))
		{
			match = TermPersMatch(mt, tocH, sumNum);
		}
		else if (FromIntersectNickFile(mt, tocH, sumNum))
		{
			match = FromIntersectNickFileMatch(mt, tocH, sumNum);
		}
		/* if this is an IMAP message that hasn't been downloaded, look on the server */
		else if (tocH->imapTOC && (tocH->sums[sumNum].offset==-1) && !text)
		{
			match = IMAPTermMatch(mt, &tocH->sums[sumNum]);
		}
		else
		{
			hasColon = NULL != strchr(mt->header, ':');
			end = text + bodyOffset;
			size = end - text;
			for (spot = FindHeaderString(text, mt->header, &size, false);
					 spot;
					 spot = FindHeaderString(spot, mt->header, &size, false))
			{
				/* we found one */
				foundHeader = true;
				switch (mt->verb)
				{
					case mbmAppears: match = true; goto done; break;
					case mbmNotAppears: match = false; goto done; break;
				}

				/* find last char of header */
				for (hEnd = spot; hEnd < end; hEnd++)
					if (hEnd[0]=='\015' && !IsWhite(hEnd[1])) break;

				/* find first char of header value */
				if (!hasColon)
				{
					while (*spot != ':' && spot < hEnd) spot++;
					if (*spot == ':') spot++;
				}
				while (spot < hEnd && IsWhite(*spot)) spot++;

				/* match */
				match = TermPtrMatch(mt, spot, hEnd);
				if (!match && (tocH->which==OUT || (tocH->sums[sumNum].flags && FLAG_OUT)))
				{
					char *hnStart, *hnEnd;
					short hid;
					char *addr_cache_local = NULL;
					char headerName[64];

					for (hnEnd = spot; hnEnd > text && *hnEnd != ':'; hnEnd--);
					for (hnStart = hnEnd; hnStart > text && hnStart[-1] != '\015'; hnStart--);

					/* Extract header name as C string */
					{
						int hnLen = (int)(hnEnd - hnStart + 1);
						if (hnLen > 63) hnLen = 63;
						memcpy(headerName, hnStart, hnLen);
						headerName[hnLen] = '\0';
					}

					if (strcasecmp(headerName, fpb->cc) == 0) hid = CC_HEAD;
					else if (strcasecmp(headerName, fpb->bcc) == 0) hid = BCC_HEAD;
					else if (strcasecmp(headerName, fpb->to) == 0) hid = TO_HEAD;
					else hid = 0;

					if (hid)
					{
						switch(hid)
						{
							case CC_HEAD: addr_cache_local = fpb->ccAddresses ? *fpb->ccAddresses : NULL; break;
							case TO_HEAD: addr_cache_local = fpb->toAddresses ? *fpb->toAddresses : NULL; break;
							case BCC_HEAD: addr_cache_local = fpb->bccAddresses ? *fpb->bccAddresses : NULL; break;
							default: addr_cache_local = NULL; break;
						}
						match = TermExpMatch(mt, spot, hEnd, &addr_cache_local);
					}
				}

				if (match)
				{
					switch (mt->verb)
					{
						case mbmIsnt:
						case mbmNotContains:
						case mbmNotIntersects:
						case mbmNotIntersectsFile:
							match = false;
					}
					goto done;
				}
				spot = hEnd + 1;
				size = end - spot;
			}
			match = mt->verb==mbmIsnt || mt->verb==mbmNotContains || mt->verb==mbmNotAppears
				|| mt->verb==mbmNotIntersects || mt->verb==mbmNotIntersectsFile;
		}
	}
	else
	{
		/* Match against message body */
		if (tocH->imapTOC && (tocH->sums[sumNum].offset==-1))
		{
			match = IMAPTermMatch(mt, &tocH->sums[sumNum]);
		}
		else
		{
			spot = text + bodyOffset + 1;
			end = text + strlen((char *)tocH->sums[sumNum].cache);
			match = TermPtrMatch(mt, spot, end);
			switch (mt->verb)
			{
				case mbmIsnt:
				case mbmNotContains:
				case mbmNotIntersects:
				case mbmNotIntersectsFile:
					match = !match;
			}
		}
	}

done:
	return(match);
}

/**********************************************************************
 * TermExpMatch - match expanded addresses
 **********************************************************************/
static bool TermExpMatch(MTPtr mt, char *spot, char *end, char **cache)
{
	char **raw = NULL;
	char *expanded_str = NULL;
	bool result = false;

	/* Use cached expansion if available */
	if (cache && *cache && **cache)
		expanded_str = *cache;

	if (!expanded_str)
	{
		if (!SuckPtrAddresses(&raw, spot, end - spot, true, false, false, NULL))
		{
			/* ExpandAliases is a stub; join raw addresses with commas */
			if (raw && raw[0])
				expanded_str = g_strjoinv(",", raw);
		}
	}

	if (expanded_str)
	{
		result = TermPtrMatch(mt, expanded_str, expanded_str + strlen(expanded_str));
		if (cache)
		{
			if (*cache != expanded_str)
			{
				free(*cache);
				*cache = expanded_str;
			}
			expanded_str = NULL;
		}
	}

	g_strfreev(raw);
	if (expanded_str)
		free(expanded_str);
	return(result);
}

/************************************************************************
 * DoesIntersectNick - does a string intersect a nickname?
 ************************************************************************/
static bool DoesIntersectNick(char **nickAddresses, void **nickExpanded, char *spot, long len)
{
	char **addresses = NULL;
	bool match = false;
	(void)nickExpanded;  /* ExpandAliases is a stub, nickExpanded always NULL */

	int err = SuckPtrAddresses(&addresses, spot, len, false, false, false, NULL);
	if (err > 0 || err == EINVAL) return false;
	if (!(FGlobalErr = err))
	{
		for (int i = 0; addresses[i]; i++)
			for (int j = 0; nickAddresses[j]; j++)
				if ((match = StringSame(nickAddresses[j], addresses[i]))) goto done;
	}
done:
	g_strfreev(addresses);
	return(match);
}

/************************************************************************
 * FromIntersectNickFile - Is this the special case of matching from address
 *  against nickname file?
 ************************************************************************/
static bool FromIntersectNickFile(MTPtr mt, TOCType *tocH, short sumNum)
{
	return (mt->verb==mbmIntersectsFile || mt->verb==mbmNotIntersectsFile)
		& !strcasecmp(mt->header, "from")
		& ValidHash(tocH->sums[sumNum].fromHash);
}

/************************************************************************
 * FromIntersectNickFileMatch - Special case routine for matching from address
 *  against nickname file
 ************************************************************************/
static bool FromIntersectNickFileMatch(MTPtr mt, TOCType *tocH, short sumNum)
{
	const char *file = NULL;
	bool match;

	if (mt->value[0] && !EqualStrRes(mt->value, ANY_ALIAS_FILE))
		file = mt->value;

	match = HashAppearsInAliasFile(tocH->sums[sumNum].fromHash, file);

	if (mt->verb==mbmNotIntersectsFile) match = !match;

	return match;
}

/************************************************************************
 * DoesIntersectNickFile - does a string intersect a nickname file?
 ************************************************************************/
static bool DoesIntersectNickFile(const char *file, char *spot, long len)
{
	char **addresses = NULL;
	const char *fileArg = NULL;
	bool match = false;
	int err;

	if (file && file[0] && !EqualStrRes((char *)file, ANY_ALIAS_FILE))
		fileArg = file;

	err = SuckPtrAddresses(&addresses, spot, len, false, false, false, NULL);
	if (err > 0 || err == EINVAL) return false;
	if (!(FGlobalErr = err))
	{
		for (int i = 0; addresses[i]; i++)
			if ((match = AppearsInAliasFile(addresses[i], fileArg))) break;
	}
	g_strfreev(addresses);
	return(match);
}

/************************************************************************
 * TermPtrMatch - does the term match a particular string?
 * mt->value is a C string; spot/end point into raw message text
 ************************************************************************/
static bool TermPtrMatch(MTPtr mt, char *spot, char *end)
{
	bool match = false;
	long valLen = (long)strlen(mt->value);

	switch(mt->verb)
	{
		case mbmRegEx:
			if (!mt->regex)
			{
				/* mt->value is already a C string — use directly with regcomp */
				mt->regex = (regex_t *)malloc(sizeof(regex_t));
				if (mt->regex && regcomp(mt->regex, mt->value, REG_EXTENDED|REG_ICASE|REG_NOSUB) != 0)
				{
					free(mt->regex);
					mt->regex = NULL;
				}
			}
			if (mt->regex)
			{
				/* Need null-terminated copy for regexec */
				long textLen = end - spot;
				char *tmp = malloc(textLen + 1);
				if (tmp)
				{
					memcpy(tmp, spot, textLen);
					tmp[textLen] = '\0';
					match = (regexec(mt->regex, tmp, 0, NULL, 0) == 0);
					free(tmp);
				}
			}
			break;

		case mbmNotContains:
		case mbmContains:
			match = !PrefIsSet(PREF_NO_FILT_LWSP)
				? PPtrMatchLWSP(mt->value, spot, end-spot, false, false)
				: NULL != PPtrFindSub(mt->value, spot, end-spot);
			break;

		case mbmIsnt:
		case mbmIs:
			if (!PrefIsSet(PREF_NO_FILT_LWSP))
				match = PPtrMatchLWSP(mt->value, spot, end-spot, true, true);
			else if (end-spot != valLen) match = false;
			else match = 0==strncasecmp(mt->value, (const char *)spot, valLen);
			break;

		case mbmStarts:
			if (!PrefIsSet(PREF_NO_FILT_LWSP))
				match = PPtrMatchLWSP(mt->value, spot, end-spot, true, false);
			else if (end-spot < valLen) match = false;
			else match = 0==strncasecmp(mt->value, (const char *)spot, valLen);
			break;

		case mbmEnds:
			if (!PrefIsSet(PREF_NO_FILT_LWSP))
				match = PPtrMatchLWSP(mt->value, spot, end-spot, false, true);
			else if (end-spot < valLen) match = false;
			else match = 0==strncasecmp(mt->value, (const char *)(end-valLen), valLen);
			break;

		case mbmNotAppears:
		case mbmAppears:
			match = true;
			break;

		case mbmNotIntersects:
		case mbmIntersects:
		{
			/* cache nickname expansion */
			if (!mt->nickExpanded || !*mt->nickExpanded || !mt->nickAddresses)
			{
				free(mt->nickExpanded);
				if (mt->nickAddresses) { g_strfreev((char **)mt->nickAddresses); mt->nickAddresses = NULL; }
				if (!(FGlobalErr = SuckPtrAddresses((char ***)&mt->nickAddresses,
						(const char *)mt->value, valLen, false, false, false, NULL)))
					FGlobalErr = ExpandAliases(&mt->nickExpanded, mt->nickAddresses, 0, false);
			}

			if (!mt->nickExpanded || !mt->nickAddresses)
				return false;

			match = DoesIntersectNick((char **)mt->nickAddresses, mt->nickExpanded, spot, end-spot);
		}
			break;

		case mbmNotIntersectsFile:
		case mbmIntersectsFile:
		{
			match = DoesIntersectNickFile(mt->value, spot, end-spot);
		}
			break;

		default:
			break;
	}
	return(match);
}

/************************************************************************
 * TermDateMatch - does the date of a message match a term?
 ************************************************************************/
static bool TermDateMatch(MTPtr mt, TOCType *tocH, short sumNum)
{
	char s[256];
	bool match;

	ComputeLocalDate(&tocH->sums[sumNum], (unsigned char *)s);
	match = TermPtrMatch(mt, s, s + strlen(s));
	if (mt->verb==mbmIsnt || mt->verb==mbmNotContains) match = !match;
	return(match);
}

/************************************************************************
 * TermPersMatch - does the personality of a message match a term?
 ************************************************************************/
static bool TermPersMatch(MTPtr mt, TOCType *tocH, short sumNum)
{
	char s[256];
	bool match;

	/* Personality matching — get personality name as C string.
	 * PERS_FORCE/TS_TO_PERS use opaque types; for now use dominant personality. */
	GetRString(s, DOMINANT);
	match = TermPtrMatch(mt, s, s + strlen(s));
	if (mt->verb==mbmIsnt || mt->verb==mbmNotContains) match = !match;
	return(match);
}

/************************************************************************
 * TermJunkMatch - does the junk score of a message match a term?
 ************************************************************************/
static bool TermJunkMatch(MTPtr mt, TOCType *tocH, short sumNum)
{
	long num = atol(mt->value);

	switch (mt->verb)
	{
		case mbmIs: return tocH->sums[sumNum].spamScore == num;
		case mbmIsnt: return tocH->sums[sumNum].spamScore != num;
		case mbmJunkMore: return tocH->sums[sumNum].spamScore > num;
		case mbmJunkLess: return tocH->sums[sumNum].spamScore < num;
		default: return false;
	}
}

/************************************************************************
 * TermPriorMatch - does the priority of a message match a term?
 ************************************************************************/
static bool TermPriorMatch(MTPtr mt, TOCType *tocH, short sumNum)
{
	char s[256];
	bool match;
	short priority;

	priority = tocH->sums[sumNum].priority;
	priority = Prior2Display(priority);
	if (priority==3) return(mt->verb==mbmIsnt || mt->verb==mbmNotContains);
	PriorityHeader(s, priority);
	match = TermPtrMatch(mt, s, s + strlen((const char *)s));
	if (mt->verb==mbmIsnt || mt->verb==mbmNotContains) match = !match;
	return(match);
}

/************************************************************************
 * AnyFiltersLo - do any filters match the given filtertype?
 ************************************************************************/
static bool AnyFiltersLo(FilterKeywordEnum fType, FilterRecord *array, int count)
{
	FilterRecord *saveCur = gCurFilters;
	int saveCount = gCurNFilters;
	bool result = false;
	short f;

	gCurFilters = array;
	gCurNFilters = count;
	for (f = 0; f < count; f++)
	{
		if (RightFilterType(fType, f))
		{
			result = true;
			break;
		}
	}
	gCurFilters = saveCur;
	gCurNFilters = saveCount;
	return result;
}

/**********************************************************************
 * HaveManualFilters - does the user have any manual filters
 **********************************************************************/
bool HaveManualFilters(void)
{
	bool result;

	if (RegenerateFilters()) return false;

	result = AnyFilters(flkManual);

	FiltersDecRef();

	return result;
}

/************************************************************************
 * AnyFilters - do any filters match the given filtertype?
 ************************************************************************/
static bool AnyFilters(FilterKeywordEnum fType)
{
	return AnyFiltersLo(fType, gFilterArray, gNFilters)
		|| AnyFiltersLo(fType, HandleToFilterArray(PreFilters), HandleToFilterCount(PreFilters))
		|| AnyFiltersLo(fType, HandleToFilterArray(PostFilters), HandleToFilterCount(PostFilters));
}

/************************************************************************
 * RightFilterType - is this filter of the right type
 ************************************************************************/
static bool RightFilterType(FilterKeywordEnum fType, short filter)
{
	switch(fType)
	{
		case flkIncoming: return(gCurFilters[filter].incoming);
		case flkOutgoing: return(gCurFilters[filter].outgoing);
		case flkManual: return(gCurFilters[filter].manual);
	}
	return false;
}

/************************************************************************
 * FilterMatchHi - does a message match a filter?
 ************************************************************************/
bool FilterMatchHi(short f, TOCType *tocH, short sumNum)
{
	FilterPB fpb;
	void *cache;
	bool match = false;

	CacheMessage(tocH, sumNum);
	cache = tocH->sums[sumNum].cache;
	if (cache)
	{
		memset(&fpb, 0, sizeof(fpb));

		/* Set current filter set to main filters for this lookup */
		gCurFilters = gFilterArray;
		gCurNFilters = gNFilters;

		match = FilterMatch(f, tocH, sumNum, &fpb);

		if (fpb.ccAddresses) { g_strfreev(fpb.ccAddresses); fpb.ccAddresses = NULL; }
		if (fpb.bccAddresses) { g_strfreev(fpb.bccAddresses); fpb.bccAddresses = NULL; }
		if (fpb.toAddresses) { g_strfreev(fpb.toAddresses); fpb.toAddresses = NULL; }

		if (tocH->sums[sumNum].cache && tocH->sums[sumNum].offset < 0)
		{
			free(tocH->sums[sumNum].cache);
			tocH->sums[sumNum].cache = NULL;
		}
	}
	return(match);
}

/************************************************************************
 * FilterMatch - does a message match a filter?
 ************************************************************************/
static bool FilterMatch(short filter, TOCType *tocH, short sumNum, FilterPBPtr fpb)
{
	MatchTerm term;
	bool match;
	bool result;

	term = gCurFilters[filter].terms[0];
	match = term.header[0] ? TermMatch(&term, tocH, sumNum, fpb) : false;
	gCurFilters[filter].terms[0] = term;  /* store back any cached info */

	if (FGlobalErr) result = false;
	else
	{
		term = gCurFilters[filter].terms[1];
		if (!term.header[0]) result = match;
		else
			switch(gCurFilters[filter].conjunction)
			{
				case cjIgnore:
					result = match;
					break;

				case cjAnd:
					if (!match) result = false;
					else result = TermMatch(&term, tocH, sumNum, fpb);
					break;

				case cjOr:
					if (match) result = true;
					else result = TermMatch(&term, tocH, sumNum, fpb);
					break;

				case cjUnless:
					if (!match) result = false;
					else result = !TermMatch(&term, tocH, sumNum, fpb);
					break;

				default:
					result = false;
					break;
			}
		gCurFilters[filter].terms[1] = term;  /* store back any cached info */
	}

	if (result && (LogLevel && LOG_FILT))
		FiltLogMatch(filter, tocH, sumNum);

	/* Purge the message cache if we filled it with minimal headers for IMAP filtering */
	if (result && tocH->sums[sumNum].cache && tocH->sums[sumNum].offset < 0)
	{
		free(tocH->sums[sumNum].cache);
		tocH->sums[sumNum].cache = NULL;
	}

	return(result);
}

/**********************************************************************
 * FiltLogMatch - log a filter match
 **********************************************************************/
static void FiltLogMatch(short filter, TOCType *tocH, short sumNum)
{
	char title[256];

	MakeMessTitle(title, tocH, sumNum, true);
	ComposeLogR(LOG_FILT, NULL, FILT_LOG_FMT, gCurFilters[filter].name, title);
}

/************************************************************************
 * NonSequitur - change the subject
 * Takes a C string subject
 ************************************************************************/
void NonSequitur(char *subject, TOCType *tocH, short sumNum)
{
	char newSub[256];
	char oldSub[128];
	char replace[64];
	char brackets[32];
	char *ampr;

	GetRString(brackets, SUBJ_TRIM_STR);
	GetRString(replace, SUBJ_REPLACE);
	g_strlcpy(oldSub, tocH->sums[sumNum].subj, sizeof(oldSub));

	{
		short subjLen = strlen(subject);
		if ((ampr = PPtrFindSub(replace, subject, subjLen)))
		{
			short prefixLen = ampr - subject;
			memcpy(newSub, subject, prefixLen);
			newSub[prefixLen] = '\0';
			if (StartsWith(subject, brackets))
			{
				short brackLen = strlen(brackets);
				memmove(newSub, newSub + brackLen, strlen(newSub) - brackLen + 1);
				TrimSquares(oldSub, true, true);
				TrimAllWhite(oldSub);
				TrimInternalWhite(oldSub);
			}
			g_strlcat(newSub, oldSub, sizeof(newSub));
			short replLen = strlen(replace);
			char remainder[256];
			short remainLen = subjLen - (ampr - subject) - replLen;
			if (remainLen > 0) {
				memcpy(remainder, ampr + replLen, remainLen);
				remainder[remainLen] = '\0';
				g_strlcat(newSub, remainder, sizeof(newSub));
			}
		}
		else g_strlcpy(newSub, subject, sizeof(newSub));
	}
	SetSubject(tocH, sumNum, newSub);
}


/**********************************************************************
 * Filter usage persistence — file-based replacement for Mac resources
 *
 * Mac original used GetResource_/AddMyResource_/ChangedResource with
 * resource type 'FU  ' to persist filter usage timestamps.
 *
 * GTK port: store as a simple binary file (filter_usage.dat) in the
 * user's data directory. Each record is a FilterUse struct.
 **********************************************************************/
static char *FilterUsagePath(void)
{
	const char *data_home = g_get_user_data_dir();
	return g_build_filename(data_home, "geudora", "filter_usage.dat", NULL);
}

static uLong FilterLastMatch(short filter)
{
	uLong when = 0;
	long id = gCurFilters[filter].fu.id;
	if (!id) return 0;

	char *path = FilterUsagePath();
	FILE *f = fopen(path, "rb");
	g_free(path);
	if (!f) return 0;

	FilterUse fu;
	while (fread(&fu, sizeof(fu), 1, f) == 1)
	{
		if (fu.id == id)
		{
			when = fu.lastMatch;
			break;
		}
	}
	fclose(f);
	return when;
}

/**********************************************************************
 * FilterLastMatchHi - return the time a filter was last used, using cache
 **********************************************************************/
uLong FilterLastMatchHi(short filter)
{
	uLong secs;

	/* Ensure gCurFilters points at main filters for this lookup */
	gCurFilters = gFilterArray;
	gCurNFilters = gNFilters;

	if (gCurFilters[filter].fu.lastMatch)
		return(gCurFilters[filter].fu.lastMatch);
	if ((secs = FilterLastMatch(filter)))
		gCurFilters[filter].fu.lastMatch = secs;
	return(secs);
}

/**********************************************************************
 * FilterNoteMatch - note that a filter has matched
 *
 * Reads the filter usage file, updates or appends the record for
 * this filter, and rewrites the file.
 **********************************************************************/
int FilterNoteMatch(short filter, long secs)
{
	long id = gCurFilters[filter].fu.id;
	gCurFilters[filter].fu.lastMatch = secs;

	if (!id) return 0;

	char *path = FilterUsagePath();

	/* Read existing records */
	FilterUse *records = NULL;
	size_t count = 0;
	FILE *f = fopen(path, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		long fileSize = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (fileSize > 0)
		{
			count = (size_t)fileSize / sizeof(FilterUse);
			records = (FilterUse *)malloc(count * sizeof(FilterUse));
			if (records)
				count = fread(records, sizeof(FilterUse), count, f);
			else
				count = 0;
		}
		fclose(f);
	}

	/* Update existing or append new */
	bool found = false;
	for (size_t i = 0; i < count; i++)
	{
		if (records[i].id == id)
		{
			records[i].lastMatch = secs;
			found = true;
			break;
		}
	}

	if (!found)
	{
		FilterUse *grown = (FilterUse *)realloc(records, (count + 1) * sizeof(FilterUse));
		if (!grown)
		{
			free(records);
			g_free(path);
			return -108; /* ENOMEM */
		}
		records = grown;
		records[count].id = id;
		records[count].lastMatch = secs;
		count++;
	}

	/* Write back */
	f = fopen(path, "wb");
	if (f)
	{
		fwrite(records, sizeof(FilterUse), count, f);
		fclose(f);
	}

	free(records);
	g_free(path);
	return 0;
}


/************************************************************************
 * TakeFilterAction - take an action that has been deemed necessary
 ************************************************************************/
static int TakeFilterAction(short filter, FilterPBPtr fpb, bool noXfer)
{
	short err = 0;
	FActionHandle fa;
	short pass;

	FilterNoteMatch(filter, GMTDateTime());

	for (pass = 0; pass < MAX_FILTER_PASS && !err; pass++)
	{
		for (fa = gCurFilters[filter].actions; !err && fa; fa = fa->next)
		{
			if (FAPass(fa->action)==pass && !(noXfer && fa->action==flkTransfer))
			{
				err = CallAction(faeDo, fa, NULL, fpb);
			}
		}
	}

	return(err);
}
