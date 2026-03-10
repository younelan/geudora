/*
 * boxact.c — Mailbox window actions, GTK4 port.
 *
 * Original Mac Eudora: ~5700 lines of Carbon/QuickDraw mailbox window code.
 * This port replaces all Mac drawing, drag, and control APIs with GTK4
 * equivalents, while preserving the logical operations (selection, sorting,
 * status, opening, etc.).
 *
 * TOCHandle → TOCType *  (direct pointer, no double-deref)
 * PStr → unsigned char * (Pascal strings)
 * EventRecord * → void * (opaque event)
 * OSErr → int
 * PETEHandle → GtkWidget *
 */

#include "boxact.h"
#include "mailbox.h"
#include "message.h"
#include "toc.h"
#include "searchwin.h"
#include "MyRes.h"
#include "StringUtil.h"
#include "fileutil.h"
#include "legacy_shim.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>

/* ---- Macros from original Mac headers ---- */
#ifndef Prior2Display
#define Prior2Display(p) ((p) ? (MIN((p), 200) + 20) / 40 : 3)
#endif
#ifndef Display2Prior
#define Display2Prior(p) ((p) * 40)
#endif
#ifndef MAX_BOX_NAME
#define MAX_BOX_NAME 31
#endif
#ifndef FLAG_SKIPPED
#define FLAG_SKIPPED (1 << 5)
#endif
#ifndef FLAG_OUT
#define FLAG_OUT (1 << 1)
#endif
#ifndef FLAG_HAS_ATT
#define FLAG_HAS_ATT (1 << 0)
#endif
#ifndef OPT_JUSTSUB
#define OPT_JUSTSUB (1 << 3)
#endif
#ifndef OPT_DELETED
#define OPT_DELETED (1 << 11)
#endif
#ifndef OPT_IMAP_SENT
#define OPT_IMAP_SENT (1 << 15)
#endif
#ifndef OPT_WILL_SEL
#define OPT_WILL_SEL (1 << 13)
#endif

#ifndef K
#define K *1024L
#endif

#define kLargeUniqueValue 0x7f7f7f7f

#ifndef SORT_ASCEND
#define SORT_ASCEND  1
#define SORT_DESCEND 2
#endif

/* kDontResort, kNoSlowResort, kResortNow, kResortWhenever defined in toc.h */

/* Sort menu item IDs — from MyRes.h or wherever they're defined */
#ifndef SORT_STATUS_ITEM
#define SORT_STATUS_ITEM      1
#define SORT_JUNK_ITEM        2
#define SORT_PRIORITY_ITEM    3
#define SORT_SENDER_ITEM      4
#define SORT_TIME_ITEM        5
#define SORT_MAILBOX_ITEM     6
#define SORT_LABEL_ITEM       7
#define SORT_SIZE_ITEM        8
#define SORT_ATTACHMENTS_ITEM 9
#define SORT_SUBJECT_ITEM     10
#define SORT_MOOD_ITEM        11
#endif

/* BoxLines column enums — from mailbox.h */
#ifndef blStat
#define blStat    1
#define blJunk    2
#define blPrior   3
#define blAttach  4
#define blFrom    5
#define blDate    6
#define blSize    7
#define blLabel   8
#define blSubject 9
#define blServer  10
#define blMailbox 11
#define blAnal    12
#endif
#ifndef BoxLinesLimit
#define BoxLinesLimit 13
#endif

/* Extern declarations for functions in other modules */
extern void SetState(TOCType *tocH, int sumNum, int state);
extern short BoxNextSelected(TOCType *tocH, short afterNum);
extern short LastMsgSelected(TOCType *tocH);

/* Comparator function type */
typedef int (*SumCompareFn)(MSumPtr, MSumPtr);

/* ---- Module globals ---- */
static TOCType *gSortTOC;

/* ---- Forward declarations ---- */
static void BoxCenter(MyWindowPtr win, short mNum);
static short BoxCountSelected(TOCType *tocH);

/* Sorting comparators */
static int SumTimeCompare(MSumPtr sum1, MSumPtr sum2);
static int SumStatCompare(MSumPtr sum1, MSumPtr sum2);
static int SumPriorCompare(MSumPtr sum1, MSumPtr sum2);
static int SumSubjCompare(MSumPtr sum1, MSumPtr sum2);
static int SumFromCompare(MSumPtr sum1, MSumPtr sum2);
static int SumSizeCompare(MSumPtr sum1, MSumPtr sum2);
static int SumAttCompare(MSumPtr sum1, MSumPtr sum2);
static int SumLabelCompare(MSumPtr sum1, MSumPtr sum2);
static int SumJunkCompare(MSumPtr sum1, MSumPtr sum2);
static int SumAnalCompare(MSumPtr sum1, MSumPtr sum2);
static int SumSelectCompare(MSumPtr sum1, MSumPtr sum2);
static int SumSubjIdCompare(MSumPtr sum1, MSumPtr sum2);
static int SumTimeFuzzCompare(MSumPtr sum1, MSumPtr sum2);
static int SumSizeKCompare(MSumPtr sum1, MSumPtr sum2);
static int SumOffsetCompare(MSumPtr sum1, MSumPtr sum2);

static int RevSumTimeCompare(MSumPtr s1, MSumPtr s2);
static int RevSumStatCompare(MSumPtr s1, MSumPtr s2);
static int RevSumPriorCompare(MSumPtr s1, MSumPtr s2);
static int RevSumSubjCompare(MSumPtr s1, MSumPtr s2);
static int RevSumFromCompare(MSumPtr s1, MSumPtr s2);
static int RevSumSizeCompare(MSumPtr s1, MSumPtr s2);
static int RevSumAttCompare(MSumPtr s1, MSumPtr s2);
static int RevSumLabelCompare(MSumPtr s1, MSumPtr s2);
static int RevSumJunkCompare(MSumPtr s1, MSumPtr s2);
static int RevSumAnalCompare(MSumPtr s1, MSumPtr s2);
static int RevSumOffsetCompare(MSumPtr s1, MSumPtr s2);

static SumCompareFn MBCompareTable[BoxLinesLimit + 2];
static int MBResortCompare(MSumPtr s1, MSumPtr s2);
static SumCompareFn FindMBSort(short item, bool reverse);
static void SwapSum(MSumPtr sum1, MSumPtr sum2);
static void SortTOC(TOCType *tocH, bool reverse, SumCompareFn compare);

static short BoxLine2Item(short line);
static short BoxItem2Line(short item);
static void MBRemoveSort(TOCType *tocH, short index);
static void MBAddSort(TOCType *tocH, short index, long sortOrder);
static bool MBIsSticky(TOCType *tocH);
static void MBSortHit(TOCType *tocH, short index, bool reverse, bool extend);

#define NSORT 6
#define MBGetSort(tocH, index) (tocH)->sorts[(index)-1]

/***********************************************************************
 * Item2Status - turn a menu item into a status value
 ***********************************************************************/
short Item2Status(short item)
{
    switch (item) {
    case statmUnread:     return UNREAD;
    case statmRead:       return READ;
    case statmReplied:    return REPLIED;
    case statmForwarded:  return FORWARDED;
    case statmRedirected: return REDIST;
    case statmUnsendable: return UNSENDABLE;
    case statmSendable:   return SENDABLE;
    case statmQueued:     return QUEUED;
    case statmTimed:      return TIMED;
    case statmUnsent:     return UNSENT;
    case statmSent:       return SENT;
    case statmMesgError:  return MESG_ERR;
    case statmRecovered:  return REBUILT;
    default:              return 0;
    }
}

/***********************************************************************
 * Status2Item - turn a status value into a menu item
 ***********************************************************************/
short Status2Item(short status)
{
    /* Note: toc.h #defines TIMED=4 and MESG_ERR=6 which shadow the
     * mailbox.h enum values REDIST=4 and SENDABLE=6.  The on-disk
     * message state uses the toc.h values. */
    switch (status) {
    case UNREAD:     return statmUnread;
    case READ:       return statmRead;
    case REPLIED:    return statmReplied;
    case FORWARDED:  return statmForwarded;
    case TIMED:      return statmTimed;       /* == 4, shadows REDIST */
    case UNSENDABLE: return statmUnsendable;
    case MESG_ERR:   return statmMesgError;   /* == 6, shadows SENDABLE */
    case QUEUED:     return statmQueued;
    case UNSENT:     return statmUnsent;
    case SENT:       return statmSent;
    case REBUILT:    return statmRecovered;
    default:         return 0;
    }
}

/***********************************************************************
 * SelectBoxRange - make a particular range in a mailbox the selection.
 *
 * In GTK4 the visual inversion is handled by the GtkListView selection
 * model — we just update the data model here.
 ***********************************************************************/
void SelectBoxRange(TOCType *tocH, int start, int end, bool cmd,
                    int eStart, int eEnd)
{
    int sNum;
    int r1, r2;

    if (!tocH || !tocH->count) return;

    MyWindowPtr win = tocH->win;

    TOCSetDirty(tocH, true);

    /* normalise ranges */
    if (end < start) { r1 = start; start = end; end = r1; }
    if (eEnd < eStart) { r1 = eStart; eStart = eEnd; eEnd = r1; }

    r1 = start < tocH->count ? start : tocH->count;
    r2 = end < tocH->count ? end : tocH->count - 1;
    if (r1 < 0) r1 = 0;
    if (r2 < 0) r2 = 0;
    if (win) win->hasSelection = false;

    if (cmd) {
        /* cmd-click: toggle selection in [r1..r2], leave others alone */
        for (sNum = 0; sNum < r1; sNum++)
            if (tocH->sums[sNum].selected) {
                if (win) win->hasSelection = true;
                break;
            }

        for (sNum = r1; sNum <= r2; sNum++)
            if (sNum < eStart || sNum > eEnd) {
                tocH->sums[sNum].selected = !tocH->sums[sNum].selected;
                if (win)
                    win->hasSelection = win->hasSelection || tocH->sums[sNum].selected;
            }

        if (win && !win->hasSelection)
            for (sNum = r2 + 1; sNum < tocH->count; sNum++)
                if (tocH->sums[sNum].selected) {
                    win->hasSelection = true;
                    break;
                }
    } else {
        /* normal click: select [r1..r2], deselect everything else */
        for (sNum = 0; sNum < r1; sNum++)
            tocH->sums[sNum].selected = false;

        for (sNum = r1; sNum <= r2; sNum++)
            tocH->sums[sNum].selected = true;
        if (win) win->hasSelection = (r1 <= r2);

        for (sNum = r2 + 1; sNum < tocH->count; sNum++)
            tocH->sums[sNum].selected = false;
    }

    tocH->updateBoxSizes = true;
    tocH->conConMultiScan = true;
}

/***********************************************************************
 * BoxSetSummarySelected - make sure a summary is selected or not
 ***********************************************************************/
void BoxSetSummarySelected(TOCType *tocH, short sumNum, bool selected)
{
    if (!tocH || sumNum < 0 || sumNum >= tocH->count) return;
    if (tocH->sums[sumNum].selected != selected) {
        tocH->sums[sumNum].selected = selected;
        InvalSum(tocH, sumNum);
        tocH->updateBoxSizes = true;
        tocH->conConMultiScan = true;
    }
}

/***********************************************************************
 * BoxActivate - handle activate/deactivate for a mailbox window
 ***********************************************************************/
void BoxActivate(MyWindowPtr win)
{
    /* In GTK4, activation is handled by the window manager.
     * This is kept for interface compatibility. */
    (void)win;
}

/***********************************************************************
 * BoxListFocus - focus on the list (vs preview)
 ***********************************************************************/
void BoxListFocus(TOCType *tocH, bool focus)
{
    if (!tocH) return;
    if (focus != tocH->listFocus) {
        tocH->listFocus = focus;
        if (focus) tocH->searchFocus = false;
    }
}

/***********************************************************************
 * BoxMenu - handle a menu choice for a mailbox
 *
 * GTK4 port: simplified — many Mac-specific menu hierarchies are
 * replaced by GAction dispatching. This preserves the logic structure.
 ***********************************************************************/
bool BoxMenu(MyWindowPtr win, int menu, int item, short modifiers)
{
    /* TODO: port menu dispatch as menus are brought online */
    (void)win; (void)menu; (void)item; (void)modifiers;
    return false;
}

/***********************************************************************
 * BoxFind - find text in summaries
 ***********************************************************************/
bool BoxFind(MyWindowPtr win, unsigned char *what)
{
    if (!win || !what || !*what) return false;

    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH || !tocH->count) return false;

    short start;
    for (start = tocH->count - 1; start >= 0; start--)
        if (tocH->sums[start].selected) break;
    start++;

    bool wrapped = false;
    for (short sumNum = start; sumNum != start || !wrapped; sumNum++) {
        if (sumNum >= tocH->count) {
            sumNum = 0;
            wrapped = true;
            if (!start) break;
        }
        MSumPtr sum = &tocH->sums[sumNum];
        if (strcasestr(sum->from, (const char *)what) ||
            strcasestr(sum->subj, (const char *)what)) {
            SelectBoxRange(tocH, sumNum, sumNum, false, -1, -1);
            BoxCenterSelection(win);
            return true;
        }
    }
    return false;
}

/***********************************************************************
 * BoxClose - close a mailbox window
 ***********************************************************************/
bool BoxClose(MyWindowPtr win)
{
    if (!win) return true;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return true;

    /* Write dirty TOC */
    if (tocH->dirty)
        toc_save(tocH);

    /* Close associated message windows */
    for (short sumNum = tocH->count - 1; sumNum >= 0; sumNum--) {
        if (tocH->sums[sumNum].messH) {
            MessHandle messH = tocH->sums[sumNum].messH;
            MyWindowPtr messWin = (*messH)->win;
            if (messWin && !messWin->isDirty) {
                CloseMyWindow(messWin);
            }
        }
    }

    return true;
}

/***********************************************************************
 * BoxOpen - open selected messages from a mailbox window
 ***********************************************************************/
void BoxOpen(MyWindowPtr win)
{
    if (!win) return;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return;

    /* Walk selected entries, open message windows */
    for (int sum = 0; sum < tocH->count; sum++) {
        if (tocH->sums[sum].selected) {
            if (tocH->sums[sum].messH) {
                MessHandle messH = tocH->sums[sum].messH;
                MyWindowPtr messWin = (*messH)->win;
                if (messWin) {
                    /* Show and raise the window */
                    if (messWin->window)
                        gtk_window_present(GTK_WINDOW(messWin->window));
                }
            } else {
                /* Open message from disk */
                TOCType *realTOC = GetRealTOC(tocH, sum, NULL);
                if (!realTOC) realTOC = tocH;
                MyWindowPtr w = GetAMessage(realTOC, sum, NULL, NULL, true);
                if (!w) break;
                if (tocH != realTOC)
                    BeenThereDoneThat(tocH, sum);
            }
        }
    }
}

/***********************************************************************
 * BoxKey - handle keystrokes in a mailbox window
 ***********************************************************************/
bool BoxKey(MyWindowPtr win, void *eventPtr)
{
    /* TODO: port keyboard navigation as mailbox UI comes online */
    (void)win; (void)eventPtr;
    return false;
}

/***********************************************************************
 * BoxCenter - center a mailbox around a given line
 ***********************************************************************/
static void BoxCenter(MyWindowPtr win, short mNum)
{
    /* In GTK4, scrolling to a row is done via GtkScrolledWindow.
     * TODO: implement when GtkListView is wired up */
    (void)win; (void)mNum;
}

/***********************************************************************
 * BoxSelectAfter - select the message on or after a given message,
 * if no messages are already selected
 ***********************************************************************/
void BoxSelectAfter(MyWindowPtr win, short mNum)
{
    if (!win) return;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return;

    if (mNum >= 0 && BoxNextSelected(tocH, -1) < 0) {
        if (tocH->count > 0) {
            win->hasSelection = true;
            if (mNum >= tocH->count) mNum = tocH->count - 1;
            tocH->sums[mNum].selected = true;
            BoxCenter(win, mNum);
        }
    }
}

/***********************************************************************
 * BoxCenterSelection - center a selection in a mailbox window
 ***********************************************************************/
void BoxCenterSelection(MyWindowPtr win)
{
    if (!win) return;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return;

    int top, bottom;
    for (top = 0; top < tocH->count; top++)
        if (tocH->sums[top].selected) break;
    for (bottom = tocH->count - 1; bottom >= 0; bottom--)
        if (tocH->sums[bottom].selected) break;
    if (top <= bottom) BoxCenter(win, (top + bottom) / 2);
}

/***********************************************************************
 * BoxPosition - save/restore window position
 ***********************************************************************/
int BoxPosition(MyWindowPtr win)
{
    /* GTK4 window positioning: GtkWindow remembers its own state. */
    (void)win;
    return 0;
}

/***********************************************************************
 * MakeMessFileName - make a default filename for save-as from subject
 ***********************************************************************/
void MakeMessFileName(TOCType *tocH, short sumNum, unsigned char *name)
{
    if (!tocH || sumNum < 0 || sumNum >= tocH->count) {
        name[0] = 0;
        return;
    }

    const char *subj = tocH->sums[sumNum].subj;
    short len = (short)strlen(subj);
    if (len > MAX_BOX_NAME) len = MAX_BOX_NAME;

    memcpy(name, subj, len);
    name[len] = 0;

    /* Replace colons (Mac path sep) and slashes (Unix path sep) with dashes */
    for (short i = 0; i < len; i++) {
        if (name[i] == ':' || name[i] == '/') name[i] = '-';
    }
}

/***********************************************************************
 * BoxHelp - balloon help for mailboxes (no-op in GTK4, use tooltips)
 ***********************************************************************/
void BoxHelp(MyWindowPtr win, Point mouse)
{
    (void)win; (void)mouse;
}

/***********************************************************************
 * BoxDidResize - handle resize of mailbox window
 ***********************************************************************/
void BoxDidResize(MyWindowPtr win, Rect *oldContR)
{
    /* GTK4 handles layout automatically via GtkBox/GtkPaned.
     * We still call RedoTOC to update internal state. */
    if (!win) return;
    TOCType *tocH = (TOCType *)win->privateData;
    if (tocH) RedoTOC(tocH);
    (void)oldContR;
}

/***********************************************************************
 * BoxGonnaShow - prepare to show a mailbox window
 ***********************************************************************/
int BoxGonnaShow(MyWindowPtr win)
{
    if (!win) return -1;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return -1;

    tocH->listFocus = true;
    tocH->conConMultiScan = true;

    if (tocH->resort) MBResort(tocH);
    BoxInitialSelection(tocH);

    return 0;
}

/***********************************************************************
 * BoxInitialSelection - make an initial selection in a mailbox
 ***********************************************************************/
void BoxInitialSelection(TOCType *tocH)
{
    if (!tocH || !tocH->count) return;
    /* Default: select last message (newest) */
    SelectBoxRange(tocH, tocH->count - 1, tocH->count - 1, false, 0, 0);
}

/***********************************************************************
 * SetPriority - set a message's priority, handle virtual TOCs
 ***********************************************************************/
void SetPriority(TOCType *tocH, short sumNum, short priority)
{
    short realSum = -1;
    TOCType *realTOC;

    if (!tocH || sumNum < 0 || sumNum >= tocH->count) return;

    /* Set in current TOC */
    short dp = Prior2Display(priority);
    if (dp != Prior2Display(tocH->sums[sumNum].priority))
        InvalTocBox(tocH, sumNum, blPrior);

    tocH->sums[sumNum].priority = priority;
    TOCSetDirty(tocH, true);

    /* If virtual TOC, set in real TOC too */
    realTOC = GetRealTOC(tocH, sumNum, &realSum);
    if (realTOC && realTOC != tocH) {
        realTOC->sums[realSum].priority = priority;
        TOCSetDirty(realTOC, true);
        InvalTocBox(realTOC, realSum, blPrior);
    }

    SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum,
                    false, false);
}

/***********************************************************************
 * InvalTocBox - invalidate one area of a mailbox window
 ***********************************************************************/
void InvalTocBox(TOCType *tocH, short sumNum, short box)
{
    /* In GTK4, invalidation triggers a redraw via
     * gtk_widget_queue_draw on the relevant list row.
     * For now we just mark the need for update. */
    if (!tocH) return;

    if (MBGetSort(tocH, box))
        tocH->resort = (tocH->resort > kNoSlowResort) ? tocH->resort : kNoSlowResort;

    if (sumNum >= 0)
        SearchInvalTocBox(tocH, sumNum, box);
}

/* RedoTOC — real implementation in toc.c */

/***********************************************************************
 * RedoAllTOCs - fix all TOCs (e.g., after date format change)
 ***********************************************************************/
void RedoAllTOCs(void)
{
    /* TODO: iterate open TOC windows when window list is available */
}

/***********************************************************************
 * CheckSortItems - check the appropriate items in the sort menu
 ***********************************************************************/
void CheckSortItems(MyWindowPtr win)
{
    /* GTK4: sort state is reflected in column header widgets */
    (void)win;
}

/***********************************************************************
 * MenuItem2Handle - convert a menu item to a handle
 *
 * In GTK4, menus use GAction strings not handles. This returns NULL
 * since the callers that use it (forward/redirect) need porting too.
 ***********************************************************************/
Handle MenuItem2Handle(short menu, short item)
{
    (void)menu; (void)item;
    return NULL;
}

/***********************************************************************
 * ServerMenuChoice - choose an item from the server menu
 ***********************************************************************/
void ServerMenuChoice(TOCType *tocH, short sumNum, short item,
                      bool shiftPressed)
{
    if (!tocH) return;

    if (tocH->imapTOC) {
        /* IMAP server menu choices */
        switch (item) {
        case 1: /* delete */
            /* TODO: IMAPDeleteMessages */
            break;
        case 2: /* remove cache */
            /* TODO: IMAPRemoveSelectedCachedContents */
            break;
        case 3: /* fetch message */
            /* TODO: IMAPFetchSelectedMessages */
            break;
        case 4: /* fetch attachments */
            /* TODO: IMAPFetchSelectedMessages with attachments */
            break;
        }
    } else {
        /* POP server menu choices */
        switch (item) {
        case 1: /* none */
            InvalTocBox(tocH, sumNum, blServer);
            break;
        case 2: /* fetch */
            InvalTocBox(tocH, sumNum, blServer);
            break;
        case 3: /* delete */
            InvalTocBox(tocH, sumNum, blServer);
            break;
        case 4: /* both */
            InvalTocBox(tocH, sumNum, blServer);
            break;
        }
    }
    (void)shiftPressed;
}

/***********************************************************************
 * BeenThereDoneThat - mark messages as read
 ***********************************************************************/
void BeenThereDoneThat(TOCType *tocH, short sumNum)
{
    if (!tocH) return;

    if (sumNum < 0) {
        /* Mark all selected messages as read */
        for (short s = tocH->count - 1; s >= 0; s--)
            if (tocH->sums[s].selected)
                BeenThereDoneThat(tocH, s);
    } else {
        if (sumNum >= tocH->count) return;
        if (tocH->sums[sumNum].state == UNREAD)
            SetState(tocH, sumNum, READ);

        if (tocH->virtualTOC) {
            short realSum = -1;
            TOCType *realTOC = GetRealTOC(tocH, sumNum, &realSum);
            if (realTOC && realSum >= 0 &&
                realTOC->sums[realSum].state == UNREAD)
                SetState(realTOC, realSum, READ);
        }
    }
}

/***********************************************************************
 * BoxButton - handle a hit in the box buttons
 ***********************************************************************/
bool BoxButton(MyWindowPtr win, GtkWidget *widget, GdkEvent *event)
{
    /* GTK4: button clicks are handled by GtkButton signal handlers */
    (void)win; (void)widget; (void)event;
    return false;
}

/***********************************************************************
 * BoxScroll - scrolling callback
 ***********************************************************************/
bool BoxScroll(MyWindowPtr win, short h, short v)
{
    (void)win; (void)h; (void)v;
    return true;
}

/***********************************************************************
 * BoxHasSelection - is there a selection?
 ***********************************************************************/
bool BoxHasSelection(MyWindowPtr win)
{
    if (!win) return false;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return false;
    return (LastMsgSelected(tocH) >= 0);
}

/***********************************************************************
 * BoxIdle - idle routine for mailboxes
 ***********************************************************************/
void BoxIdle(MyWindowPtr win)
{
    if (!win) return;
    TOCType *tocH = (TOCType *)win->privateData;
    if (!tocH) return;

    /* Attend to any pending updates */
    if (tocH->updateBoxSizes)
        tocH->updateBoxSizes = false;
}

/***********************************************************************
 * BoxUpdate - handle an update event for a mailbox window
 * In GTK4, the GtkListView handles rendering. This is a no-op.
 ***********************************************************************/
void BoxUpdate(MyWindowPtr win)
{
    (void)win;
}

/***********************************************************************
 * BoxClick - handle a click in the content region
 * In GTK4, clicks are handled by GtkGesture controllers.
 ***********************************************************************/
void BoxClick(MyWindowPtr win, void *event)
{
    (void)win; (void)event;
}

/***********************************************************************
 * BoxSelectSame - select all messages with the same <something>
 ***********************************************************************/
void BoxSelectSame(TOCType *tocH, short item, short clickedSum)
{
    SumCompareFn compare = NULL;

    if (!tocH) return;

    switch (item) {
    case SORT_STATUS_ITEM:      compare = SumStatCompare; break;
    case SORT_JUNK_ITEM:        compare = SumJunkCompare; break;
    case SORT_PRIORITY_ITEM:    compare = SumPriorCompare; break;
    case SORT_SUBJECT_ITEM:     compare = SumSubjCompare; break;
    case SORT_SENDER_ITEM:      compare = SumFromCompare; break;
    case SORT_TIME_ITEM:        compare = SumTimeFuzzCompare; break;
    case SORT_SIZE_ITEM:        compare = SumSizeKCompare; break;
    case SORT_ATTACHMENTS_ITEM: compare = SumAttCompare; break;
    case SORT_LABEL_ITEM:       compare = SumLabelCompare; break;
    case SORT_MOOD_ITEM:        compare = SumAnalCompare; break;
    default: return;
    }

    gSortTOC = tocH;

    /* Phase 1: find candidates matching any selected summary */
    for (short selected = 0; selected < tocH->count; selected++) {
        if (tocH->sums[selected].selected) {
            tocH->sums[selected].spareShort2 = 0;
            for (short candidate = 0; candidate < tocH->count; candidate++) {
                if (candidate != selected &&
                    !(tocH->sums[candidate].opts & OPT_WILL_SEL)) {
                    tocH->sums[candidate].spareShort2 = 0;
                    if (!compare(&tocH->sums[selected],
                                 &tocH->sums[candidate]))
                        tocH->sums[candidate].opts |= OPT_WILL_SEL;
                }
            }
        }
    }

    /* Phase 2: select them */
    for (short candidate = 0; candidate < tocH->count; candidate++) {
        if (tocH->sums[candidate].opts & OPT_WILL_SEL) {
            if (!tocH->sums[candidate].selected)
                SelectBoxRange(tocH, candidate, candidate, true, -1, -1);
            tocH->sums[candidate].opts &= ~OPT_WILL_SEL;
        }
    }

    /* Phase 3: sort selected together if they're not contiguous */
    short last = -1;
    bool need = false;
    for (short candidate = 0; candidate < tocH->count; candidate++) {
        if (tocH->sums[candidate].selected) {
            if (last != -1 && last != candidate - 1) {
                need = true;
                break;
            }
            last = candidate;
        }
    }

    if (need) {
        for (short candidate = 0; candidate < tocH->count; candidate++) {
            if (tocH->sums[candidate].selected)
                tocH->sums[candidate].spareShort = 1;
            else
                tocH->sums[candidate].spareShort =
                    (candidate < clickedSum) ? 0 : 2;
        }
        SortTOC(tocH, false, SumSelectCompare);
    }
}

/***********************************************************************
 * SubjCompare - compare two subjects after trimming Re:/Fwd:/whitespace
 ***********************************************************************/
int SubjCompare(unsigned char *in1, unsigned char *in2)
{
    /* Simple C-string subject comparison with Re:/Fwd: stripping */
    const char *s1 = (const char *)in1;
    const char *s2 = (const char *)in2;

    /* Skip Re:/Fwd: prefixes */
    while (*s1) {
        if ((s1[0] == 'R' || s1[0] == 'r') &&
            (s1[1] == 'E' || s1[1] == 'e') && s1[2] == ':') {
            s1 += 3;
            while (*s1 == ' ') s1++;
        } else if ((s1[0] == 'F' || s1[0] == 'f') &&
                   (s1[1] == 'W' || s1[1] == 'w') &&
                   (s1[2] == 'D' || s1[2] == 'd') && s1[3] == ':') {
            s1 += 4;
            while (*s1 == ' ') s1++;
        } else break;
    }
    while (*s2) {
        if ((s2[0] == 'R' || s2[0] == 'r') &&
            (s2[1] == 'E' || s2[1] == 'e') && s2[2] == ':') {
            s2 += 3;
            while (*s2 == ' ') s2++;
        } else if ((s2[0] == 'F' || s2[0] == 'f') &&
                   (s2[1] == 'W' || s2[1] == 'w') &&
                   (s2[2] == 'D' || s2[2] == 'd') && s2[3] == ':') {
            s2 += 4;
            while (*s2 == ' ') s2++;
        } else break;
    }

    return strcasecmp(s1, s2);
}

/* ================================================================== */
/*                        SORTING COMPARATORS                         */
/* ================================================================== */

static int SumTimeCompare(MSumPtr sum1, MSumPtr sum2)
{
    if ((unsigned long)sum1->seconds > (unsigned long)sum2->seconds) return 1;
    if (sum1->seconds == sum2->seconds) return 0;
    return -1;
}

static int SumStatCompare(MSumPtr sum1, MSumPtr sum2)
{
    static unsigned char table[] = {1,2,3,4,5,6,8,7,10,11,9,12,13,14,15,16,17};
    short s1 = (sum1->state == 255) ? 255 :
               (sum1->state < 0 || (unsigned)sum1->state >= sizeof(table)) ? 100 :
               table[sum1->state];
    short s2 = (sum2->state == 255) ? 255 :
               (sum2->state < 0 || (unsigned)sum2->state >= sizeof(table)) ? 100 :
               table[sum2->state];
    return s1 - s2;
}

static int SumPriorCompare(MSumPtr sum1, MSumPtr sum2)
{
    short p1 = sum1->priority ? sum1->priority : Display2Prior(3);
    short p2 = sum2->priority ? sum2->priority : Display2Prior(3);
    return p1 - p2;
}

static int SumSubjCompare(MSumPtr sum1, MSumPtr sum2)
{
    return SubjCompare((unsigned char *)sum1->subj, (unsigned char *)sum2->subj);
}

static int SumFromCompare(MSumPtr sum1, MSumPtr sum2)
{
    return strcasecmp(sum1->from, sum2->from);
}

#define DisplayLength(sum) ((sum)->length - (sum)->bodyOffset + 1023)
#define EffectiveLength(sum) \
    (((sum)->flags & FLAG_SKIPPED) ? -1 K : \
     (((sum)->opts & OPT_JUSTSUB) ? -2 K : DisplayLength(sum)))

static int SumSizeCompare(MSumPtr sum1, MSumPtr sum2)
{
    long l1 = EffectiveLength(sum1);
    long l2 = EffectiveLength(sum2);
    return (l1 > l2) ? 1 : (l1 < l2) ? -1 : 0;
}

static int SumSizeKCompare(MSumPtr sum1, MSumPtr sum2)
{
    long l1 = EffectiveLength(sum1) / (1 K);
    long l2 = EffectiveLength(sum2) / (1 K);
    return (l1 > l2) ? 1 : (l1 < l2) ? -1 : 0;
}

static int SumAttCompare(MSumPtr sum1, MSumPtr sum2)
{
    return (int)(sum1->flags & FLAG_HAS_ATT) - (int)(sum2->flags & FLAG_HAS_ATT);
}

static int SumLabelCompare(MSumPtr sum1, MSumPtr sum2)
{
    return SumColor(sum1) - SumColor(sum2);
}

static int SumJunkCompare(MSumPtr sum1, MSumPtr sum2)
{
    return sum1->spamScore - sum2->spamScore;
}

static int SumAnalCompare(MSumPtr sum1, MSumPtr sum2)
{
    if ((unsigned)sum1->score > (unsigned)sum2->score) return 1;
    if (sum1->score == sum2->score) return 0;
    return -1;
}


static int SumSelectCompare(MSumPtr sum1, MSumPtr sum2)
{
    if (sum1->length == kLargeUniqueValue) return 1;
    if (sum2->length == kLargeUniqueValue) return -1;
    long res = (long)sum1->spareShort - (long)sum2->spareShort;
    if (!res) res = sum1->spareShort2 - sum2->spareShort2;
    return res;
}

static int SumSubjIdCompare(MSumPtr sum1, MSumPtr sum2)
{
    return sum1->subjId - sum2->subjId;
}

static int SumTimeFuzzCompare(MSumPtr sum1, MSumPtr sum2)
{
    /* Simple: compare dates as seconds */
    return SumTimeCompare(sum1, sum2);
}

static int SumOffsetCompare(MSumPtr sum1, MSumPtr sum2)
{
    if ((unsigned long)sum1->offset > (unsigned long)sum2->offset) return 1;
    if (sum1->offset == sum2->offset) return 0;
    return -1;
}

/* Reverse comparators */
static int RevSumTimeCompare(MSumPtr s1, MSumPtr s2) { return -SumTimeCompare(s1, s2); }
static int RevSumStatCompare(MSumPtr s1, MSumPtr s2) { return -SumStatCompare(s1, s2); }
static int RevSumPriorCompare(MSumPtr s1, MSumPtr s2) { return -SumPriorCompare(s1, s2); }
static int RevSumSubjCompare(MSumPtr s1, MSumPtr s2) { return -SumSubjCompare(s1, s2); }
static int RevSumFromCompare(MSumPtr s1, MSumPtr s2) { return -SumFromCompare(s1, s2); }
static int RevSumSizeCompare(MSumPtr s1, MSumPtr s2) { return -SumSizeCompare(s1, s2); }
static int RevSumAttCompare(MSumPtr s1, MSumPtr s2) { return -SumAttCompare(s1, s2); }
static int RevSumLabelCompare(MSumPtr s1, MSumPtr s2) { return -SumLabelCompare(s1, s2); }
static int RevSumJunkCompare(MSumPtr s1, MSumPtr s2) { return -SumJunkCompare(s1, s2); }
static int RevSumAnalCompare(MSumPtr s1, MSumPtr s2) { return -SumAnalCompare(s1, s2); }
static int RevSumOffsetCompare(MSumPtr s1, MSumPtr s2) { return -SumOffsetCompare(s1, s2); }

/* ================================================================== */
/*                          SORTING ENGINE                            */
/* ================================================================== */

static SumCompareFn FindMBSort(short item, bool reverse)
{
    switch (item) {
    case SORT_STATUS_ITEM:      return reverse ? RevSumStatCompare : SumStatCompare;
    case SORT_JUNK_ITEM:        return reverse ? RevSumJunkCompare : SumJunkCompare;
    case SORT_PRIORITY_ITEM:    return reverse ? RevSumPriorCompare : SumPriorCompare;
    case SORT_SUBJECT_ITEM:     return reverse ? RevSumSubjCompare : SumSubjCompare;
    case SORT_SENDER_ITEM:      return reverse ? RevSumFromCompare : SumFromCompare;
    case SORT_TIME_ITEM:        return reverse ? RevSumTimeCompare : SumTimeCompare;
    case SORT_SIZE_ITEM:        return reverse ? RevSumSizeCompare : SumSizeCompare;
    case SORT_ATTACHMENTS_ITEM: return reverse ? RevSumAttCompare : SumAttCompare;
    case SORT_LABEL_ITEM:       return reverse ? RevSumLabelCompare : SumLabelCompare;
    case SORT_MOOD_ITEM:        return reverse ? RevSumAnalCompare : SumAnalCompare;
    default:                    return NULL;
    }
}

static void SwapSum(MSumPtr sum1, MSumPtr sum2)
{
    MSumType temp = *sum1;
    *sum1 = *sum2;
    *sum2 = temp;
}

/* qsort-compatible wrapper around the active comparator */
static SumCompareFn gActiveCompare;
static int qsort_wrapper(const void *a, const void *b)
{
    return gActiveCompare((MSumPtr)a, (MSumPtr)b);
}

static void SortTOC(TOCType *tocH, bool reverse, SumCompareFn compare)
{
    if (!tocH || tocH->count <= 1) return;

    MSumPtr sums = tocH->sums;
    int count = tocH->count;

    /* Tag original positions for stability */
    for (int i = 0; i < count; i++)
        sums[i].spareShort2 = i;

    gSortTOC = tocH;
    gActiveCompare = compare;
    qsort(sums, count, sizeof(MSumType), qsort_wrapper);

    /* Update back-pointers from messages to their sum index */
    for (int i = 0; i < count; i++)
        if (sums[i].messH)
            (*(MessHandle)sums[i].messH)->sumNum = i;

    TOCSetDirty(tocH, true);
    (void)reverse; /* reverse is encoded in the comparators */
}

static int MBResortCompare(MSumPtr s1, MSumPtr s2)
{
    if (s1->length == kLargeUniqueValue) return 1;
    if (s2->length == kLargeUniqueValue) return -1;

    long res = 0;
    for (int i = 0; MBCompareTable[i] && !res; i++)
        res = MBCompareTable[i](s1, s2);
    return res ? res : (s1->spareShort2 - s2->spareShort2);
}

static short BoxLine2Item(short line)
{
    switch (line) {
    case blStat:    return SORT_STATUS_ITEM;
    case blJunk:    return SORT_JUNK_ITEM;
    case blPrior:   return SORT_PRIORITY_ITEM;
    case blFrom:    return SORT_SENDER_ITEM;
    case blDate:    return SORT_TIME_ITEM;
    case blMailbox: return SORT_MAILBOX_ITEM;
    case blLabel:   return SORT_LABEL_ITEM;
    case blSize:    return SORT_SIZE_ITEM;
    case blAttach:  return SORT_ATTACHMENTS_ITEM;
    case blSubject: return SORT_SUBJECT_ITEM;
    case blAnal:    return SORT_MOOD_ITEM;
    default:        return 0;
    }
}

static short BoxItem2Line(short item)
{
    switch (item) {
    case SORT_STATUS_ITEM:      return blStat;
    case SORT_JUNK_ITEM:        return blJunk;
    case SORT_PRIORITY_ITEM:    return blPrior;
    case SORT_SENDER_ITEM:      return blFrom;
    case SORT_TIME_ITEM:        return blDate;
    case SORT_MAILBOX_ITEM:     return blMailbox;
    case SORT_LABEL_ITEM:       return blLabel;
    case SORT_SIZE_ITEM:        return blSize;
    case SORT_ATTACHMENTS_ITEM: return blAttach;
    case SORT_SUBJECT_ITEM:     return blSubject;
    case SORT_MOOD_ITEM:        return blAnal;
    default:                    return 0;
    }
}

static bool MBIsSticky(TOCType *tocH)
{
    if (!tocH) return false;
    for (short i = 1; i <= NSORT; i++)
        if (MBGetSort(tocH, i)) return true;
    return false;
}

static void MBRemoveSort(TOCType *tocH, short index)
{
    long oldSort = MBGetSort(tocH, index);
    if (!oldSort) return;

    MBGetSort(tocH, index) = 0;
    InvalTocBox(tocH, -2, index);

    for (short i = 1; i <= NSORT; i++)
        if (MBGetSort(tocH, i) > oldSort)
            MBGetSort(tocH, i) -= 4;

    tocH->resort = kResortNow;
    TOCSetDirty(tocH, true);
}

static void MBAddSort(TOCType *tocH, short index, long sortOrder)
{
    long max = 0;
    for (short i = 1; i <= NSORT; i++)
        if (MBGetSort(tocH, i) > max) max = MBGetSort(tocH, i);

    max = (((max >> 2) + 1) << 2) + sortOrder;
    MBGetSort(tocH, index) = max;
    tocH->resort = kResortNow;
    TOCSetDirty(tocH, true);
}

static void MBSortHit(TOCType *tocH, short index, bool reverse, bool extend)
{
    long sort = MBGetSort(tocH, index);

    if (!extend)
        for (short i = 1; i <= NSORT; i++)
            MBRemoveSort(tocH, i);

    if (!sort) {
        sort = reverse ? SORT_DESCEND : SORT_ASCEND;
        MBAddSort(tocH, index, sort);
    } else if (reverse) {
        long sortOrder = sort & 0x3;
        sortOrder = (sortOrder == SORT_ASCEND) ? SORT_DESCEND : SORT_ASCEND;
        MBGetSort(tocH, index) = (sort & ~0x3) | sortOrder;
        tocH->resort = kResortNow;
    } else if (extend) {
        MBRemoveSort(tocH, index);
    }
}

/***********************************************************************
 * MBResort - resort a mailbox using its stored sort criteria
 ***********************************************************************/
void MBResort(TOCType *tocH)
{
    if (!tocH) return;

    long max = 0;
    short n;

    for (short i = 1; i <= NSORT; i++)
        if (MBGetSort(tocH, i) > max) max = MBGetSort(tocH, i);

    bool reverse = (SORT_DESCEND == (max & 3));
    max >>= 2;
    if (max > BoxLinesLimit) max = BoxLinesLimit;

    if (max) {
        for (n = 1; n <= max; n++)
            for (short i = 1; i <= NSORT; i++)
                if ((tocH->sorts[i - 1] >> 2) == n) {
                    MBCompareTable[n - 1] = FindMBSort(
                        BoxLine2Item(i),
                        (MBGetSort(tocH, i) & 3) == SORT_DESCEND);
                    tocH->lastSort = BoxLine2Item(i);
                    break;
                }
        MBCompareTable[n - 1] = NULL;
        SortTOC(tocH, reverse, MBResortCompare);
        TOCSetDirty(tocH, true);
    }

    tocH->resort = kDontResort;

    if (tocH->win)
        BoxCenterSelection(tocH->win);
}

/***********************************************************************
 * BoxCountSelected - how many messages are selected
 ***********************************************************************/
static short BoxCountSelected(TOCType *tocH)
{
    short count = 0;
    if (!tocH) return 0;
    for (short i = tocH->count - 1; i >= 0; i--)
        if (tocH->sums[i].selected) count++;
    return count;
}
