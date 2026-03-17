/* Copyright (c) 2017, Computer History Museum
   All rights reserved. */

/* Prevent legacy_shim.h from defining inline CanScoreJunk - real one is below */
#define CanScoreJunk CanScoreJunk
#include "mydefs.h"
#undef CanScoreJunk
#include <assert.h>
#include <errno.h>
// #include <glib.h> // Not available
// #include <gtk/gtk.h> // Not available
#include <stdbool.h>
#include <stdint.h> // For uint32_t

#include "StringUtil.h"
#include "fileutil.h"
#include "filters.h"
#include "filtrun.h"
#include "gtk_dialogs.h"
#include "imapdownload.h"
#include "imapmailboxes.h"
#include "junk.h"
// #include "mydefs.h" // Moved to top
#include "prefdefs.h"
#include "schizo.h"
#include "toc.h"
#include "boxact.h"

#ifndef JUNK
#define JUNK MBX_JUNK
#endif
#ifndef TRASH
#define TRASH MBX_TRASH
#endif

#include "util.h"

#include "emsapi-mac.h"
extern MenuHandle GetMHandle(short menuID);
#include "StrnDefs.h"
#include "imapnetlib.h"

#include "message.h"
#include "trans.h"
#include "pop.h"
/* MyRes.h must come before StringDefs.h (MyRes.h's enums use names that
   StringDefs.h defines as macros - include order matters) */
#include "MyRes.h"
/* StringDefs.h after MyRes.h to avoid macro/enum conflicts */
#include "StringDefs.h"
#include "progress.h"
#include "Globals.h"
#include "threading.h"

#ifndef ASSERT
#define ASSERT assert
#endif

/* MenuHandle defined as GMenuModel* in mailbox.h - do not redefine */

// Constants and Macros

// Hash is now a macro in message.h: #define Hash(s) HashWithSeed(s, 1)
/* HNoPurge/HPurge provided by legacy_shim.h as static inline */
int HandleHeadGetIdText(void **text, int head, void ***h);

// Forward declarations for functions defined in this file (not in headers)
// Modernized: Use FSSpec * instead of FSSpecPtr
extern FSSpec *Box2TOCSpec(FSSpec *boxSpec, FSSpec *tocSpec);
/* CacheMessage declared in imapdownload.h as int - do not redeclare as bool */
long BodyOffset(void **text);
void EnsureFromHash(TOCType * tocH, short sumNum);
bool WhiteListByMID(TOCType * tocH, short sumNum);

// UI/Menu functions stubs/declarations
void CycleBalls(void);
void CyclePendulum(void);
void GetItemCmd(void *m, int item, char *key);
void SetItemCmd(void *m, int item, char key);
int AdjustSpecialMenuItem(int item);
void SetMenuItemModifiers(void *m, int item, int mods);
void ClearPrefBit(short pref, long bit);
void SetPrefBit(short pref, long bit);
/* Win2MessH is a macro defined in message.h - not a function */
TOCType * Win2TOC(MyWindowPtr win);
bool MyWinHasSelection(MyWindowPtr win);
short GetWindowKind(void *winWP);

// Resource IDs for string resources
#define JUNK_MAILBOX_EMPTY_DAYS 1000
#define JUNK_MAILBOX_EMPTY_THRESH 1001
#define FILE_ALIAS_JUNK 1002
#define JUNK_EMPTY_WARNING 1003
#define JUNK_PREEXISTING_WARNING 1004
#define JUNK_PREEXISTING_FOLDER_WARNING 1005
#define JUNK_STR_RES 1006
#define JUNK_JUNK_IS_BAD_TRIM_DEST 1007
#define FILE_ALIAS_TRASH 1008
#define JUNK_CANT_ARCHIVE 1009
#define JUNK_PREEXISTING_RENAME_NAME 1010

#define TRIMMING_JUNK 1011
#define JUNK_MAILBOX_EMPTY_DEST 1012
#define MESSAGE_DELETE_ITEM 1013

short BoxSpecByName(TOCType * *tocH, const char *name);
int WriteTOC(TOCType * tocH);
extern bool AmQuitting;
/* optionKey/shiftKey are macros in legacy_shim.h, not variables */

typedef enum { kAutoScore, kUserMarkJunk, kUserMarkNotJunk } TJunkType;

void ScoreOneMessage(TLMHandle tList, TOCType * tocH, short sumNum,
                     TJunkType howToScore);
bool NagHitReturnItem(EventRecord *event, DialogPtr theDialog, short itemHit,
                      long dialogRefcon);
void MarkOneAsJunk(TLMHandle tList, TOCType * tocH, short sumNum, bool isJunk);
int JunkSetScoreLo(TOCType * tocH, short sumNum, short because, short score);

// UI/Menu functions stubs/declarations
#define JUNK_INTRO_DLOG 1234
#define JUNK_TRIM_INTERVAL 1235
#define MESSAGE_MENU 1236
#define SPECIAL_MENU 1237
#define SPECIAL_FILTER_ITEM 1

#define MESSAGE_JUNK_ITEM 2
#define MESSAGE_NOTJUNK_ITEM 3
#define kMenuOptionModifier 0x0800

void GetItemCmd(void *m, int item, char *key);
void SetItemCmd(void *m, int item, char key);
int AdjustSpecialMenuItem(int item);
void SetMenuItemModifiers(void *m, int item, int mods);
void ClearPrefBit(short pref, long bit);
void SetPrefBit(short pref, long bit);
/* Win2MessH is a macro in message.h - not a function */
TOCType * Win2TOC(MyWindowPtr win);
bool MyWinHasSelection(MyWindowPtr win);

bool CanScoreToc(TOCType * tocH);

// Whitelisting stubs
bool HashAppearsInAliasFile(uint32_t hash, void *file);
bool AppearsInAliasFile(const char *name, void *file);

// Iterative thingy stub
int DoIterativeThingyLo(TOCType * tocH, int item, int mods, int p4, bool p5);

#define kNoMessageId 0

/* OutgoingMIDList declared in Globals.h as MIDListType */

// IMAP support
int JunkIMAP(TOCType * tocH, short sumNum, bool isJunk, bool dontMove);
int ArchiveIMAPJunk(void);
void JunkMoveIMAPMessages(TOCType * tocH, bool isJunk);
/* CacheMessage declared as int in imapdownload.h - not bool */

long BodyOffset(void **text);
void **NewHandleClear(size_t size);

/************************************************************************
 * MoveToJunk -
 ************************************************************************/
int MoveToJunk(TOCType * source, short spamThresh, FilterPB *fpb) {
  int err = 0;
  TOCType * junk = GetJunkTOC();
  short found = 0;
  int i;

  if (junk == NULL)
    return paramErr;

  //	select all messages above the junk mailbox threshhold
  for (i = 0; i < source->count; i++) {
    if (source->sums[i].spamScore < spamThresh)
      BoxSetSummarySelected(source, i, false);
    else {
      BoxSetSummarySelected(source, i, true);
      found += 1;

      //	don't trust the date on junk
      if (!JunkPrefBelieveDate())
        source->sums[i].seconds = source->sums[i].arrivalSeconds;

      //	Clean up the server
      if (JunkPrefServerDel() && !(source->sums[i].flags & FLAG_SKIPPED))
        AddTSToPOPD(DELETE_ID, source, i, false);
    }
  }

  //	if we found any junk, move it to the junk mailbox
  if (found > 0) {
    FSSpec junkSpec = GetMailboxSpec(junk, -1);
    err = MoveSelectedMessagesLo(source, &junkSpec, false, false, false, false);

    //	Bookkeeping
    if (PrefIsSet(PREF_REPORT) && fpb != NULL)
      while (found-- > 0)
        AddSpecToList(&junkSpec, fpb->report);
    UseFeature(featureJunk);
  }

  return err;
}

/**********************************************************************
 * MoveToIMAPJunk - if an IMAP message is junk, get rid of it.
 *
 *	Note, this is different than MoveToJunk that is used on local
 *	mailboxes.  It must be able to move messages one at a time for
 *	incremental IMAP filtering.
 *
 *	Additionally, it does not move open spam messages, nor does it
 *	move deleted messages.
 **********************************************************************/
int MoveToIMAPJunk(TOCType * source, short sumNum, short spamThresh,
                   FilterPB *fpb) {
  int err = 0;
  TOCType * junkTocH = GetJunkTOC();
  short found = 0;
  short i;
  short start, end;

  // locate the proper junk mailbox.  Don't warn the user, we're already
  // warned them before the IMAP operation got underway.
  junkTocH = LocateIMAPJunkToc(source, true, true);

  // Sanity check
  if ((source->imapTOC == 0) || (junkTocH == NULL))
    return (paramErr);

  // count the messages to be moved
  if (sumNum == -1) {
    // do them all.
    start = 0;
    end = source->count;

    // count number of spam messages.  Ignore deleted and open messages.
    for (i = start; (i < end); i++)
      if ((source->sums[i].spamScore >= spamThresh) &&
          (source->sums[i].messH == NULL) &&
          ((source->sums[i].opts & OPT_DELETED) == 0))
        found++;
  } else {
    // just one message
    found = 1;
  }

  if (sumNum != -1) {
    // doing a single message.
    err = IMAPMoveMessageDuringFiltering(source, sumNum, junkTocH, false, fpb);
  } else {
    // doing a range of messages

    // junk all spam messages
    if (found > 0) {
      unsigned long *uid_array =
          (unsigned long *)calloc(found, sizeof(unsigned long));
      if (uid_array) {
        i = found - 1;
        for (sumNum = start; (sumNum < end) && i >= 0; sumNum++)
          if ((source->sums[sumNum].spamScore >= spamThresh) &&
              (source->sums[sumNum].messH == NULL) &&
              ((source->sums[sumNum].opts & OPT_DELETED) == 0)) {
            uid_array[i--] = (source->sums[sumNum].uidHash);

            // don't trust the date on junk
            if (!JunkPrefBelieveDate())
              source->sums[sumNum].seconds =
                  source->sums[sumNum].arrivalSeconds;
          }
        // Wrap in a portable Handle (void**) for backward compatibility
        void **uid_handle = (void **)malloc(sizeof(void *));
        if (uid_handle) {
          *uid_handle = malloc(found * sizeof(unsigned long));
          if (*uid_handle) {
            memcpy(*uid_handle, uid_array, found * sizeof(unsigned long));
            err = IMAPTransferMessages(source, junkTocH, uid_handle, false,
                                       false);
            free(*uid_handle);
          }
          free(uid_handle);
        }
        free(uid_array);
      }
    }
  }

  return err;
}

/**********************************************************************
 * FilterJunk - if a message is junk, get rid of it
 **********************************************************************/
int FilterJunk(TOCType * fromTocH) {
  int err = 0;

  JunkScoreBox(fromTocH, -1, -1, true);
  err = MoveToJunk(fromTocH, GetRLong(JUNK_MAILBOX_THRESHHOLD), NULL);

  return err;
}

//	Helper routine for "Junk" - Mark one message as junk
void MarkOneAsJunk(TLMHandle tList, TOCType * tocH, short sumNum, bool isJunk) {
  short spamScore = isJunk ? GetRLong(JUNK_XFER_SCORE) : 0;
  if (tList != NULL)
    ScoreOneMessage(tList, tocH, sumNum,
                    isJunk ? kUserMarkJunk : kUserMarkNotJunk);
  JunkSetScore(tocH, sumNum, JUNK_BECAUSE_USER,
               !isJunk || spamScore ? spamScore
                                    : tocH->sums[sumNum].spamScore);
  //	don't trust the date on junk
  if (isJunk && !JunkPrefBelieveDate())
    tocH->sums[sumNum].seconds = tocH->sums[sumNum].arrivalSeconds;

  if (!tocH->imapTOC && JunkPrefServerDel() &&
      !(tocH->sums[sumNum].flags & FLAG_SKIPPED))
    AddTSToPOPD(DELETE_ID, tocH, sumNum, false);
}

/************************************************************************
 * Junk - mark selected messages as junk (or not)
 ************************************************************************/
// no error handling yet
// no easyopen yet
int Junk(TOCType * tocH, short sumNum, bool isJunk, bool ezOpen) {
  FilterPB fpb;
  FSSpec junkSpec;
  TOCType * junkTocH;
  TLMHandle tList = NULL;
  short i;
  TOCType * realTOC;
  short realSum;

  UseFeature(featureJunk);

  // Properly handle IMAP mailboxes ...
  if (tocH->imapTOC)
    return (JunkIMAP(tocH, sumNum, isJunk, false));

  // Porperly handle IMAP messages in virtual TOCs ...
  if (tocH->virtualTOC) {
    // prrocess all IMAP messages in this virtual toc
    for (i = tocH->count; i--;)
      if (tocH->sums[i].selected) {
        realTOC = GetRealTOC(tocH, i, &realSum);
        if (realTOC)
          JunkIMAP(realTOC, realSum, isJunk, true);
      }

    // move all the processed IMAP messages
    JunkMoveIMAPMessages(tocH, isJunk);
  }

  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);

  //	Looking for selected messages
  if (sumNum < 0) {
    // mark the selected ones with the chosen score
    for (i = tocH->count; i--;)
      if (tocH->sums[i].selected) {
        MarkOneAsJunk(tList, tocH, i, isJunk);
        if (!isJunk) {
          // redate if need be
          if (tocH->sums[i].seconds == tocH->sums[i].arrivalSeconds)
            RedateTS(tocH, i);

          // put back on server if need be
          PushPers(PERS_FORCE(TS_TO_PERS(tocH, i)));
          if (PrefIsSet(PREF_LMOS))
            RemIdFromPOPD(PERS_POPD_TYPE(CurPers), DELETE_ID,
                          tocH->sums[i].uidHash);
          PopPers();
        }
      }

    // if they're being marked not junk...
    if (!isJunk) {
      bool filter = true;

      // Whitelist them?
      if (JunkPrefNotJunkWhite())
        WhiteListTS(tocH, -1);

      // Before filtering, adjust the selection to only messages
      // actually in the junk mailbox, if it's a virtual TOC
      if (tocH->virtualTOC)
        for (i = tocH->count - 1; i >= 0; i--) {
          FSSpec spec;
          if (tocH->sums[sumNum].selected) {
            if (tocH->sums[sumNum].flags & FLAG_OUT)
              BoxSetSummarySelected(tocH, i, false);
            else {
              spec = GetMailboxSpec(tocH, i);
              if (!SpecIsJunkSpec(&spec))
                BoxSetSummarySelected(tocH, i, false);
            }
          }
        }
      else if (!BoxIsJunkBox(tocH))
        filter = false;

      if (filter) {
        // filter them
        FilterSelectedMessages(flkIncoming, tocH, &fpb);
        FilterPostprocess(flkIncoming, &fpb);
      }
    }
    // if they're being marked junk, transfer them to junk
    else if (!BoxIsJunkBox(tocH)) {
      if (tocH->which != JUNK && (junkTocH = GetJunkTOC())) {
        junkSpec = GetMailboxSpec(junkTocH, -1);
        MoveSelectedMessages(tocH, &junkSpec, false);
      }
    }
  } else {
    //	do just the one message
    MarkOneAsJunk(tList, tocH, sumNum, isJunk);

    // if it's not junk...
    if (!isJunk) {
      if (!isJunk) {
        // fix the date?
        if (tocH->sums[sumNum].seconds ==
            tocH->sums[sumNum].arrivalSeconds)
          RedateTS(tocH, sumNum);

        // put back on server if need be
        PushPers(PERS_FORCE(TS_TO_PERS(tocH, sumNum)));
        if (PrefIsSet(PREF_LMOS))
          RemIdFromPOPD(PERS_POPD_TYPE(CurPers), DELETE_ID,
                        tocH->sums[sumNum].uidHash);
        PopPers();
      }

      // Whitelist it?
      if (JunkPrefNotJunkWhite())
        WhiteListTS(tocH, sumNum);

      // Filter it
      FilterMessage(flkIncoming, tocH, sumNum);
    }
    // if it is junk, move it
    else if (ezOpen)
      TransferMenuChoice(TRANSFER_MENU, MAILBOX_JUNK_ITEM, tocH, sumNum, 0,
                         false);
    else if (tocH->which != JUNK && (junkTocH = GetJunkTOC())) {
      junkSpec = GetMailboxSpec(junkTocH, -1);
      MoveMessageLo(tocH, sumNum, &junkSpec, false, false, true);
    }
  }
  // lies, lies, lies
  if (tList != NULL)
    free((void **)tList);
  return 0;
}

/************************************************************************
 * JunkIMAP - mark selected IMAP messages as junk (or not)
 ************************************************************************/
int JunkIMAP(TOCType * tocH, short sumNum, bool isJunk, bool dontMove) {
  int err = 0;
  (void)err;
  TLMHandle tList = NULL;
  TOCType * destTocH = NULL;
  PersHandle imapPers = TOCToPers(tocH);
  MailboxNodeHandle imapDest = NULL;
  bool filteringUnderway = IMAPFilteringUnderway();
  long uid = tocH->sums[sumNum].uidHash;
  short i, count;

  // sanity check
  if (!tocH || !imapPers)
    return (paramErr);

  // build list of translators ...
  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);

  //	Looking for selected messages
  if (sumNum < 0) {
    // mark the selected ones with the chosen score
    for (i = tocH->count; i--;)
      if (tocH->sums[i].selected)
        MarkOneAsJunk(tList, tocH, i, isJunk);

    // determine where to move them
    if (!isJunk) {
      // Whitelist them?
      if (JunkPrefNotJunkWhite())
        WhiteListTS(tocH, -1);

      if (BoxIsJunkBox(tocH)) {
        // Put the message back into the proper inbox.
        if ((imapDest = LocateInboxForPers(imapPers)) != NULL)
          destTocH = TOCBySpec(&imapDest->mailboxSpec);
      }
    }
    // if they're being marked junk, transfer them to junk
    else if (!BoxIsJunkBox(tocH)) {
      // This is an IMAP mailbox we're junking from.  Get the proper JUNK
      // mailbox
      imapDest = GetIMAPJunkMailbox(imapPers, true, false);
      if (imapDest && (imapDest != TOCToMbox(tocH)))
        destTocH = TOCBySpec(&imapDest->mailboxSpec);
    }

    if (!dontMove) {
      // build a list of the currently selected, non-deleted messages and move
      // them. if we're not junking, the selection may have expanded!
      count = 0;
      for (i = 0; (i < tocH->count); i++)
        if ((tocH->sums[i].selected) &&
            ((tocH->sums[i].opts & OPT_DELETED) == 0))
          count++;

      if (count > 0) {
        unsigned long *uid_array =
            (unsigned long *)calloc(count, sizeof(unsigned long));
        if (!uid_array) {
          // Clean up and return error
          if (tList)
            free((void **)tList);
          return memFullErr;
        }

        // Fill the uids array
        int uid_index = count - 1;
        for (i = 0; i < tocH->count && uid_index >= 0; i++) {
          if ((tocH->sums[i].selected) &&
              ((tocH->sums[i].opts & OPT_DELETED) == 0)) {
            uid_array[uid_index--] = (tocH->sums[i].uidHash);

            // also mark the messages as not junk to catch the expanded
            // selection
            if (!isJunk)
              MarkOneAsJunk(tList, tocH, i, false);
          }
        }

        // actually move them now - note: IMAPTransferMessages expects void**
        // for now
        // TODO: refactor IMAPTransferMessages to accept array pointer
        if (destTocH) {
          // Temporarily wrap in void** for backward compatibility
          void **uid_handle = (void **)malloc(sizeof(void *));
          if (uid_handle) {
            *uid_handle = malloc(count * sizeof(unsigned long));
            if (*uid_handle) {
              memcpy(*uid_handle, uid_array, count * sizeof(unsigned long));
              err = IMAPTransferMessages(tocH, destTocH, uid_handle, false,
                                         false);
              free(*uid_handle);
            }
            free(uid_handle);
          }
        }

        free(uid_array);
      }
    }
  } else {
    //	do just the one message
    MarkOneAsJunk(tList, tocH, sumNum, isJunk);

    if (!dontMove) {
      // if it's not junk...
      if (!isJunk) {
        // Whitelist it?
        if (JunkPrefNotJunkWhite())
          WhiteListTS(tocH, sumNum);

        // Find the proper Inbox to move this messgae back to
        if ((imapDest = LocateInboxForPers(imapPers)) != NULL)
          destTocH = TOCBySpec(&imapDest->mailboxSpec);

        // Move the message
        if (destTocH)
          err = IMAPTransferMessage(tocH, destTocH, uid, false, false);
      } else if (!BoxIsJunkBox(tocH)) {
        imapDest = GetIMAPJunkMailbox(imapPers, true, false);
        if (imapDest) {
          if ((destTocH = TOCBySpec(&imapDest->mailboxSpec)) != NULL) {
            // this gets a little tricky with IMAP filtering ...
            if (filteringUnderway) {
              IMAPStopFiltering(false);

              // copy the message from this mailbox to the destination
              if (IMAPTransferMessage(tocH, destTocH, uid, true, true) == 0) {
                // then mark it as deleted.  We'll expunge when filtering is
                // complete
                IMAPMarkMessageAsDeleted(tocH, uid, false);
                if (FancyTrashForThisPers(tocH))
                  FlagForExpunge(tocH);
              }
              IMAPStartFiltering(tocH, true);
            } else {
              MoveMessageLo(tocH, sumNum, &imapDest->mailboxSpec, false,
                            false, true);
            }
          }
        }
      }
    }
  }
  // lies, lies, lies
  if (tList != NULL)
    free((void **)tList);
  return 0;
}

/************************************************************************
 * ArchiveJunk - destroy the junk that's there
 ************************************************************************/
int ArchiveJunk(TOCType * tocH) {
  long i;
  uLong threshTime =
      GMTDateTime() - GetRLong(JUNK_MAILBOX_EMPTY_DAYS) * 24 * 3600;
  uLong threshScore = GetRLong(JUNK_MAILBOX_EMPTY_THRESH);
  long count = 0;
  short button;
  char dest[256];
  bool trash;
  bool nuke;
  FSSpec spec;

  if (!tocH)
    return fnfErr;

  SetPrefLong(PREF_LAST_JUNK_TRIM, GMTDateTime() / 3600);

  for (i = 0; i < tocH->count; i++) {
    if (tocH->sums[i].arrivalSeconds < threshTime &&
        tocH->sums[i].spamScore >= threshScore) {
      BoxSetSummarySelected(tocH, i, true);
      count++;
    } else
      BoxSetSummarySelected(tocH, i, false);
  }

  if (count) {
    // warning?
    if (JunkPrefBoxArchiveWarning()) {
      button = ComposeStdAlert(Note, JUNK_EMPTY_WARNING, FILE_ALIAS_JUNK, count,
                               FILE_ALIAS_JUNK, JUNK_MAILBOX_EMPTY_DAYS);
      if (CommandPeriod || button == kAlertStdAlertCancelButton)
        return userCancelled;
      if (button == kAlertStdAlertOtherButton)
        SetPrefLong(PREF_JUNK_MAILBOX, GetPrefLong(PREF_JUNK_MAILBOX) |
                                           bJunkPrefBoxArchiveWarning);
    }

    // progress dialog
    OpenProgress();
    ProgressMessage(kpTitle, ComposeRString(dest, TRIMMING_JUNK,
                                           FILE_ALIAS_JUNK));

    // where are we going?
    GetRString((char *)dest, JUNK_MAILBOX_EMPTY_DEST);
    if (EqualStrRes((char *)dest, FILE_ALIAS_JUNK) ||
        EqualStrRes((char *)dest, JUNK_STR_RES)) {
      if (kAlertStdAlertOKButton ==
          ComposeStdAlert(Stop, JUNK_JUNK_IS_BAD_TRIM_DEST, FILE_ALIAS_JUNK,
                          FILE_ALIAS_TRASH)) {
        SetPref(JUNK_MAILBOX_EMPTY_DEST, "");
        GetRString((char *)dest, TRASH);
      } else
        return dupFNErr;
    }

    trash = EqualStrRes((char *)dest, TRASH) ||
            EqualStrRes((char *)dest, FILE_ALIAS_TRASH);
    nuke = (AmQuitting && trash && PrefIsSet(PREF_AUTO_EMPTY)) ||
           StringSame("-", (const char *)dest);

    // if we're quitting and the trash will be emptied, we can just nuke now
    if (nuke)
      DoIterativeThingyLo(tocH, MESSAGE_DELETE_ITEM, optionKey | shiftKey, 0,
                          false);
    else if (trash)
      DoIterativeThingyLo(tocH, MESSAGE_DELETE_ITEM, 0, 0, false);
    else if (!BoxSpecByName((TOCType * *)&spec, (const char *)dest))
      MoveSelectedMessages(tocH, &spec, false);
    else
      ComposeStdAlert(Caution, JUNK_CANT_ARCHIVE, FILE_ALIAS_JUNK,
                      FILE_ALIAS_JUNK, dest);

    // we may be on the hairy edge of life.  Write the new toc if so
    if (AmQuitting)
      WriteTOC(tocH);
  }

  // void** IMAP Junk mailboxes as well
  ArchiveIMAPJunk();

  return 0;
}

/************************************************************************
 * ArchiveIMAPJunk - destroy junk in all IMAP Junk Mailboxes
 ************************************************************************/
int ArchiveIMAPJunk(void) {
  long i;
  uLong threshTime;
  uLong threshScore;
  long count = 0;
  short button;
  char dest[256];
  bool trash;
  bool nuke;
  bool archiveIMAP;
  FSSpec spec;
  TOCType *tocH, *toTocH;
  MailboxNodeHandle junkMBox;
  bool bWarned = false;

  long sumNum;
  MailboxNodeHandle destMBox;
  PersHandle destPers;

  PushPers(CurPers);
  for (CurPers = PersList; CurPers; CurPers = CurPers->next) {
    if (IsIMAPPers(CurPers)) {
      threshTime =
          GMTDateTime() - GetRLong(JUNK_MAILBOX_EMPTY_DAYS) * 24 * 3600;
      threshScore = GetRLong(JUNK_MAILBOX_EMPTY_THRESH);

      SetPrefLong(PREF_LAST_JUNK_TRIM, GMTDateTime() / 3600);

      // Find this personality's junk mailbox
      if ((junkMBox = GetIMAPJunkMailbox(CurPers, false, true))) {
        spec = junkMBox->mailboxSpec;
        if ((tocH = TOCBySpec(&spec))) {
          // count the number of messages to move.  Ignore deleted messages
          for (i = 0; i < tocH->count; i++)
            if ((tocH->sums[i].arrivalSeconds < threshTime) &&
                (tocH->sums[i].spamScore >= threshScore) &&
                ((tocH->sums[i].opts & OPT_DELETED) == 0))
              count++;

          if (count) {
            // warning?
            if (!bWarned && JunkPrefBoxArchiveWarning()) {
              button = ComposeStdAlert(Note, JUNK_EMPTY_WARNING,
                                       FILE_ALIAS_JUNK, count, FILE_ALIAS_JUNK,
                                       JUNK_MAILBOX_EMPTY_DAYS);
              if (CommandPeriod || button == kAlertStdAlertCancelButton)
                return userCancelled;
              if (button == kAlertStdAlertOtherButton)
                SetPrefLong(PREF_JUNK_MAILBOX, GetPrefLong(PREF_JUNK_MAILBOX) |
                                                   bJunkPrefBoxArchiveWarning);
              bWarned = true;
            }

            // warn the user, we may have to go online for this

            // progress dialog
            OpenProgress();
            ProgressMessage(kpTitle, ComposeRString(
                                         dest, TRIMMING_JUNK, FILE_ALIAS_JUNK));

            // where are we going?
            toTocH = NULL;
            GetRString((char *)dest, JUNK_MAILBOX_EMPTY_DEST);
            trash = EqualStrRes((char *)dest, TRASH) ||
                    EqualStrRes((char *)dest, FILE_ALIAS_TRASH);
            nuke = (AmQuitting && trash && PrefIsSet(PREF_AUTO_EMPTY)) ||
                   StringSame("-", (const char *)dest);

            archiveIMAP = false;
            if (!nuke && !trash) {
              toTocH = NULL;
              if (!BoxSpecByName((TOCType * *)&spec, (const char *)dest)) {
                FSSpec imapSpec;
                imapSpec = spec;
                imapSpec.parID = SpecDirId(&spec);

                // Look in the local IMAP tree first for the destination mailbox
                destMBox = LocateNodeBySpec(CurPers->mailboxTree, &imapSpec);

                // If not found, look in all IMAP trees for the destination
                // mailbox
                if (destMBox == NULL)
                  LocateNodeBySpecInAllPersTrees(&imapSpec, &destMBox,
                                                 &destPers);

                if (destMBox) {
                  toTocH = TOCBySpec(&destMBox->mailboxSpec);
                  archiveIMAP = true;
                } else {
                  // If still not found, use the local mailbox ...
                  toTocH = TOCBySpec(&spec);
                }
              }
            }

            // Actually archive messages now ...
            if (nuke || trash || (archiveIMAP && toTocH)) {
              // build a list of UIDs to transfer ...
              unsigned long *uid_array =
                  (unsigned long *)calloc(count, sizeof(unsigned long));
              if (uid_array) {
                int uid_index = count - 1;
                for (sumNum = 0; sumNum < tocH->count && uid_index >= 0;
                     sumNum++)
                  if ((tocH->sums[sumNum].arrivalSeconds < threshTime) &&
                      (tocH->sums[sumNum].spamScore >= threshScore) &&
                      ((tocH->sums[sumNum].opts & OPT_DELETED) == 0))
                    uid_array[uid_index--] = (tocH->sums[sumNum].uidHash);

                // Wrap in void** for backward compatibility
                void **uid_handle = NULL;
                if (PtrToHand(uid_array, &uid_handle,
                              count * sizeof(unsigned long)) == 0) {
                  if (archiveIMAP)
                    IMAPTransferMessages(tocH, toTocH, uid_handle, false, true);
                  else
                    IMAPDeleteMessages(tocH, uid_handle, nuke, true, false,
                                       true);
                  free(uid_handle);
                }
                free(uid_array);
              }
            } else if (toTocH) {
              // we're archiving to a local mailbox
              IMAPStartFiltering(tocH,
                                 true); // display progress like we're filtering
              for (sumNum = 0; sumNum < tocH->count && count; sumNum++)
                if ((tocH->sums[sumNum].arrivalSeconds < threshTime) &&
                    (tocH->sums[sumNum].spamScore >= threshScore) &&
                    ((tocH->sums[sumNum].opts & OPT_DELETED) == 0))
                  IMAPMoveMessageDuringFiltering(tocH, sumNum, toTocH, false,
                                                 NULL);
              IMAPStopFiltering(true);
            } else
              ComposeStdAlert(Caution, JUNK_CANT_ARCHIVE, FILE_ALIAS_JUNK,
                              FILE_ALIAS_JUNK, dest);
          }

          // We're done with this Junk mailbox
          tocH = NULL;
        }
      }
      // else
      // junk mailbox could not be found.
    }
  }
  PopPers();

  return 0;
}

/************************************************************************
 * SpecIsJunkSpec is a spec the junk mailbox?
 ************************************************************************/
bool SpecIsJunkSpec(FSSpecPtr spec) {
  return IsRoot(spec->path) && EqualStrRes(spec->name, JUNK);
}

/************************************************************************
 * BoxIsJunkBox - is a tocH the junk mailbox?
 ************************************************************************/
bool BoxIsJunkBox(TOCType * tocH) {
  return tocH && (tocH->which == JUNK ||
                  (tocH->imapTOC && IsIMAPJunkMailbox(TOCToMbox(tocH))));
}

/************************************************************************
 * PreexistingJunkWarning - warn the user about their existing junk mailbox
 *   I know it doesn't check for errors.  There's not much we could do
 *   except fail to launch, and that seems more drastic than just letting
 *   things proceed.
 ************************************************************************/
void PreexistingJunkWarning(FSSpecPtr spec) {
  bool folder = g_file_test(spec->path, G_FILE_TEST_IS_DIR);
  short template =
      folder ? JUNK_PREEXISTING_FOLDER_WARNING : JUNK_PREEXISTING_WARNING;
  short button = ComposeStdAlert(Note, template, JUNK, JUNK);
  FSSpec newSpec;

  if (folder) {
    newSpec = *spec;
    GetRString(newSpec.name, JUNK_PREEXISTING_RENAME_NAME);
    UniqueSpec(&newSpec, 100);
    // Standard rename takes full paths
    rename(spec->path, newSpec.path);
    int fd = creat(spec->path, 0644);
    if (fd >= 0)
      close(fd);
  } else if (button == kAlertStdAlertCancelButton) {
    FSSpec tocSpec;
    FSSpec newTOCSpec;

    // rename the mailbox
    newSpec = *spec;
    GetRString(newSpec.name, JUNK_PREEXISTING_RENAME_NAME);
    UniqueSpec(&newSpec, 100);

    int fd = creat(newSpec.path, 0644);
    if (fd >= 0)
      close(fd);

    // Exchange files: portable way is rename/rename/rename or link/unlink
    // For now, let's just use rename if the destination doesn't exist
    rename(spec->path, newSpec.path); // simplified

    // rename the toc file, if any
    Box2TOCSpec(spec, &tocSpec);
    if (g_file_test(tocSpec.path, G_FILE_TEST_EXISTS)) {
      Box2TOCSpec(&newSpec, &newTOCSpec);
      rename(tocSpec.path, newTOCSpec.path);
    }
  } else if (CommandPeriod || button == kAlertStdAlertOtherButton) {
    // disable the feature
    SetPrefLong(PREF_JUNK_MAILBOX,
                GetPrefLong(PREF_JUNK_MAILBOX) | bJunkPrefBoxHold);
  }
  // if the user says ok, we need do nothing special

  // Do not pass this way again
  SetPrefLong(PREF_JUNK_MAILBOX,
              GetPrefLong(PREF_JUNK_MAILBOX) | bJunkPrefBoxExistWarning);
}

/************************************************************************
 * JunkTOCCleanse - do some stuff to the junk box on open
 ************************************************************************/
void JunkTOCCleanse(TOCType * tocH) {
  short i = tocH->count;
  short spamXferScore = GetRLong(JUNK_XFER_SCORE);

  while (i--) {
    if (tocH->sums[i].spamBecause == 0)
      JunkSetScore(tocH, i, JUNK_BECAUSE_XFER, spamXferScore);
  }
}

/************************************************************************
 * JunkRescan - rescan a mailbox for junk (not the junk mailbox)
 ************************************************************************/
short JunkRescanBox(TOCType * tocH) {
  int err = 0;
  short sumNum;
  uLong spamThresh = GetRLong(JUNK_MAILBOX_THRESHHOLD);
  TLMHandle tList = NULL;

  UseFeature(featureJunk);
  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);
  if (tList != NULL) { // we have at least one plugin!
    ASSERT(tocH != NULL);
    ASSERT(tocH != GetJunkTOC());

    // rescore any we need to
    for (sumNum = tocH->count; sumNum--;) {
      CycleBalls();

      //	Save off the selected flag
      tocH->sums[sumNum].spareShort = tocH->sums[sumNum].selected;

      //	If it's never been scored, or scored by a plugin, rescore it
      if (tocH->sums[sumNum].spamScore == -1 ||
          tocH->sums[sumNum].spamBecause == JUNK_BECAUSE_PLUG)
        ScoreOneMessage(tList, tocH, sumNum, kAutoScore);
    }

    //	transfer the rescored ones
    err = MoveToJunk(tocH, spamThresh, NULL);

    //	restore the selection
    for (sumNum = tocH->count; sumNum--;)
      BoxSetSummarySelected(tocH, sumNum, tocH->sums[sumNum].spareShort);

    free((void **)tList);
  }

  return err;
}

/************************************************************************
 * JunkRescan - rescan a the junk mailbox for junk
 ************************************************************************/
int JunkRescanJunkMailbox() {
  FilterPB fpb;
  short sumNum;
  uLong spamThresh = GetRLong(JUNK_MAILBOX_THRESHHOLD);
  TOCType * tocH = GetJunkTOC();
  TLMHandle tList = NULL;

  //	Bail if there isn't a junk mailbox
  if (tocH == NULL)
    return paramErr;

  UseFeature(featureJunk);
  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);

  if (tList != NULL) { // we have at least one plugin!
                       //	rescore any we need to
    for (sumNum = tocH->count; sumNum--;) {
      CycleBalls();

      //	Save off the selected flag
      tocH->sums[sumNum].spareShort = tocH->sums[sumNum].selected;

      //	If it's never been scored, or scored by a plugin, rescore it
      if (tocH->sums[sumNum].spamScore == -1 ||
          tocH->sums[sumNum].spamBecause == JUNK_BECAUSE_PLUG)
        ScoreOneMessage(tList, tocH, sumNum, kAutoScore);

      BoxSetSummarySelected(tocH, sumNum,
                            tocH->sums[sumNum].spamScore < spamThresh);
    }

    FilterSelectedMessages(flkIncoming, tocH, &fpb);
    FilterPostprocess(flkIncoming, &fpb);

    // restore the selection
    for (sumNum = tocH->count; sumNum--;)
      BoxSetSummarySelected(tocH, sumNum, tocH->sums[sumNum].spareShort);

    free((void **)tList);
  }

  return 0;
}

/**********************************************************************
 * JunkSetScore - set a message's junk score,
 * 				handle virtual TOCs, too
 **********************************************************************/
int JunkSetScore(TOCType * tocH, short sumNum, short because, short score) {
  TOCType * realTOC;
  short realSum;

  JunkSetScoreLo(tocH, sumNum, because, score);
  realTOC = GetRealTOC(tocH, sumNum, &realSum);
  if (realTOC && realTOC != tocH) {
    // do real mailbox also if working in virtual mailbox
    JunkSetScoreLo(realTOC, realSum, because, score);
    tocH = realTOC;
    sumNum = realSum;
  }
  SearchUpdateSum(tocH, sumNum, tocH, tocH->sums[sumNum].serialNum, false,
                  false); //	Notify search window
  return 0;
}

/************************************************************************
 * JunkSetScoreLo - change the score of some junk
 ************************************************************************/
int JunkSetScoreLo(TOCType * tocH, short sumNum, short because, short score) {
  short junkThresh = GetRLong(JUNK_MAILBOX_THRESHHOLD);
  bool nowJunk = score >= junkThresh;
  bool wasJunk = tocH->sums[sumNum].spamScore >= junkThresh;
  short oldBecause = tocH->sums[sumNum].spamBecause;

  ASSERT(sumNum >= 0 && sumNum < tocH->count);
  if (sumNum < 0 || sumNum >= tocH->count)
    return fnfErr;

  tocH->sums[sumNum].spamScore = score;
  tocH->sums[sumNum].spamBecause = because;
  InvalTocBox(tocH, sumNum, blJunk);

  // Update stats here
  if (because == JUNK_BECAUSE_WHITE)
    UpdateNumStatWithTime(kStatWhiteList, 1, tocH->sums[sumNum].seconds);
  else if (because == JUNK_BECAUSE_PLUG) {
    if (nowJunk)
      UpdateNumStatWithTime(kStatScoredJunk, 1, tocH->sums[sumNum].seconds);
    else
      UpdateNumStatWithTime(kStatScoredNotJunk, 1,
                            tocH->sums[sumNum].seconds);
  } else if (because == JUNK_BECAUSE_USER) {
    if (nowJunk && !wasJunk) {
      // something missed it.  What?
      if (oldBecause == JUNK_BECAUSE_WHITE)
        UpdateNumStatWithTime(kStatFalseWhiteList, 1,
                              tocH->sums[sumNum].seconds);
      else if (oldBecause == JUNK_BECAUSE_PLUG)
        UpdateNumStatWithTime(kStatFalseNegatives, 1,
                              tocH->sums[sumNum].seconds);
    } else if (!nowJunk && wasJunk) {
      // Did we misjunk it?
      if (oldBecause == JUNK_BECAUSE_PLUG) {
        UpdateNumStatWithTime(kStatFalsePositives, 1,
                              tocH->sums[sumNum].seconds);
      }
    }
  }
  return 0;
}

/************************************************************************
 * JunkIntro - introduce the junk feature
 ************************************************************************/
#define jiNoItem 2
short JunkIntro(void) {
  extern ModalFilterYDUPP DlgFilterUPP;
  short item = 0;

  if (ETLCountTranslators(EMSF_JUNK_MAIL)) {
    SetPrefLong(PREF_JUNK_MAILBOX,
                GetPrefLong(PREF_JUNK_MAILBOX) | bJunkPrefNeedIntro);
    Nag(JUNK_INTRO_DLOG, NULL, NagHitReturnItem, DlgFilterUPP, false,
        (uLong)&item);
    if (CommandPeriod || item == jiNoItem)
      SetPrefLong(PREF_JUNK_MAILBOX,
                  GetPrefLong(PREF_JUNK_MAILBOX) | bJunkPrefBoxHold);
    ASSERT(item != 0);
  }

  return item;
}

/************************************************************************
 * NagHitReturnItem - hit proc that stuffs the item hit into the refcon, used as
 *pointer to short
 ************************************************************************/
bool NagHitReturnItem(EventRecord *event, DialogPtr theDialog, short itemHit,
                      long dialogRefcon) {
  *(short *)dialogRefcon = itemHit;
  return (true);
}

/************************************************************************
 * JunkTrimOK - is it time to trim junk?
 ************************************************************************/
bool JunkTrimOK(void) {
  long daysSince =
      (GMTDateTime() / 3600 - GetPrefLong(PREF_LAST_JUNK_TRIM) + 8) / 24;
  short interval = GetRLong(JUNK_TRIM_INTERVAL);

  // If we're quitting, follow the interval strictly
  // we also follow the interval strictly if the user has a hyper interval set
  if (AmQuitting || GetRLong(JUNK_MAILBOX_EMPTY_DAYS) == 1)
    return interval && daysSince >= interval;

  // If the user might be in the middle of something, don't be quite so cheeky
  else
    return interval && daysSince >= 3 * interval;
}

/************************************************************************
 * JunkReassignKeys - reassign command keys from filter to junk
 ************************************************************************/
void JunkReassignKeys(bool switchem) {
  MenuHandle messageMH = GetMHandle(MESSAGE_MENU);
  MenuHandle specialMH = GetMHandle(SPECIAL_MENU);
  char key;

  if (switchem) {
    GetItemCmd(specialMH, AdjustSpecialMenuItem(SPECIAL_FILTER_ITEM), &key);
    if (key) {
      SetItemCmd(specialMH, AdjustSpecialMenuItem(SPECIAL_FILTER_ITEM), 0);
      SetItemCmd(messageMH, MESSAGE_JUNK_ITEM, key);
      SetItemCmd(messageMH, MESSAGE_NOTJUNK_ITEM, key);
      SetMenuItemModifiers(messageMH, MESSAGE_NOTJUNK_ITEM,
                           kMenuOptionModifier);
    }
  } else {
    GetItemCmd(messageMH, MESSAGE_JUNK_ITEM, &key);
    if (key) {
      SetItemCmd(messageMH, MESSAGE_JUNK_ITEM, 0);
      SetItemCmd(specialMH, AdjustSpecialMenuItem(SPECIAL_FILTER_ITEM), key);
      SetItemCmd(messageMH, MESSAGE_NOTJUNK_ITEM, 0);
    }
  }
  if (switchem)
    ClearPrefBit(PREF_JUNK_MAILBOX, bJunkPrefSwitchCmdJ);
  else
    SetPrefBit(PREF_JUNK_MAILBOX, bJunkPrefSwitchCmdJ);
}

/************************************************************************
 * JunkItemEnable - should the junk menu item be enabled?
 ************************************************************************/
bool JunkItemsEnable(MyWindowPtr win, bool not) {
  WindowPtr winWP = GetMyWindowWindowPtr(win);
  TOCType * tocH;
  short sumNum;
  FSSpec spec;

  if (!win || !winWP)
    return false;
  Zero(spec);

  switch (GetWindowKind(winWP)) {
  case CBOX_WIN:
    return false;
  case COMP_WIN:
    return false;
  case MESS_WIN:
    if (JunkPrefAlwaysEnable())
      return true;
    tocH = Win2MessH(win)->tocH;
    return not == BoxIsJunkBox(tocH);

  case MBOX_WIN:
    tocH = Win2TOC(win);
    if (!(/*win->hasSelection ||*/ MyWinHasSelection(win)))
      return false;
    if (JunkPrefAlwaysEnable())
      return true;
    if (!tocH->virtualTOC)
      return not == BoxIsJunkBox(tocH);

    // the non-trivial case here
    for (sumNum = tocH->count - 1; sumNum >= 0; sumNum--) {
      if (tocH->sums[sumNum].selected) {
        if (tocH->sums[sumNum].flags & FLAG_OUT)
          return false;
        spec = GetMailboxSpec(tocH, sumNum);
        if (not!= SpecIsJunkSpec(&spec))
          return false;
      }
    }

    // if we found any selected messages, we'll have
    // put their spec in here, and so we should enable
    return *spec.name != 0;
  }

  // failure
  return false;
}

#pragma mark ** Call Junk Plugins **

/************************************************************************
 * ScoreOneMessage -
 * Score a message using each of the installed plugins
 * Store the high score (and the signature of the plugin)
 * into the TOC for the mailbox.
 ************************************************************************/
void ScoreOneMessage(TLMHandle tList, TOCType * tocH, short sumNum,
                     TJunkType howToScore) {
  int err = 0;
  short highScore = -2;
  // short highSig = 0; // Unused
  void **text;

  ASSERT(tList != NULL);
  ASSERT(HandleCount(tList) > 0);

  // don't score plugins if we've been asked not to
  if (!CanScoreToc(tocH))
    return;

  // Load the message ...
  if (tocH->imapTOC)
    CacheIMAPMessageForSpamWatch(tocH, sumNum);
  else
    CacheMessage(tocH, sumNum);

  if ((text = tocH->sums[sumNum].cache)) {
    void **headers, **body;
    body = NULL;
    char *ret;
    long bodyOffset, textSz, hdrSz, envSz;

    ASSERT(*text != NULL);
    //	Split the message into headers and body
    //	Error detection here would be a good thing!!

    //	The first line is always a UUCP envelope, followed by a <CR>
    //	It's not part of the message, so we strip it out.
    ret = strchr(*text, '\015');
    ASSERT(ret != NULL);
    envSz = ret + 1 - (char *)*text; // how much of the buffer to throw away

    //	The headers are seperated from the body by a <CR><CR> sequence
    bodyOffset = BodyOffset(text); // first byte of the body
    textSz = GetHandleSize(text);  // total size of the message
    hdrSz = bodyOffset - envSz;    // bytes in the header
    if (bodyOffset == textSz) {
      //	couldn't find "\n\n" - empty message body?
      body = NULL;
    } else {
      long bodySz = textSz - bodyOffset;
      ASSERT(bodySz > 0);
      body = NewHandleClear(bodySz + 1);
      BlockMoveData(*text + bodyOffset, *body, bodySz);
    }

    ASSERT(hdrSz > 0);
    headers = NewHandleClear(hdrSz + 1);
    BlockMoveData(*text + envSz, *headers, hdrSz);

    //	For each translator in the list, score the message
    if (err == 0) {
      emsJunkScore junkScore;
      emsResultStatus junkStatus;
      emsMessageInfo messageInfo;
      emsHeaderData messageHdr;
      emsMIMEtype mimeInfo;
      emsTranslator transInfo;
      emsJunkInfo junkInfo;
      TLMPtr aModule;
      short i, numPlugins;

      Zero(messageHdr);
      messageHdr.size = sizeof(messageHdr);
      Zero(messageInfo);
      messageInfo.size = sizeof(messageInfo);
      Zero(mimeInfo);
      mimeInfo.size = sizeof(mimeInfo);
      Zero(junkInfo);
      junkInfo.size = sizeof(junkInfo);
      messageInfo.fromAddressStatus = emsAddressNotChecked; // !!! for now
      messageInfo.header = &messageHdr;
      messageInfo.textType = &mimeInfo;
      messageInfo.text = body;

      junkInfo.context = EMSFJUNK_SCORE_ON_ARRIVAL;

      messageHdr.rawHeaders = headers;
      //	Need to fill in more of mimeInfo and messageInfo
      //	And the fromAddressStatus, too!
      HLock((void **)tList);

      numPlugins = HandleCount(tList) - 1; // don't call the sentinel!
      for (i = 0; i < numPlugins; ++i) {
        aModule = &tList[i];
        Zero(junkScore);
        junkScore.size = sizeof(junkScore);
        Zero(junkStatus);
        junkStatus.size = sizeof(junkStatus);
        Zero(transInfo);
        transInfo.size = sizeof(transInfo);

        transInfo.id = aModule->id;

        if (howToScore != kAutoScore) {
          //	Fill in the old score info here
          junkScore.score = tocH->sums[sumNum].spamScore;
          junkInfo.context = howToScore == kUserMarkJunk
                                 ? EMSFJUNK_MARK_IS_JUNK
                                 : EMSFJUNK_MARK_NOT_JUNK;
          junkInfo.pluginID = tocH->sums[sumNum].spamBecause;
          (void)ETLMarkJunk(aModule, &transInfo, &junkInfo, &messageInfo,
                            &junkScore, &junkStatus);
        } else {
          if (0 ==
              (err = ETLScoreJunk(aModule, &transInfo, &junkInfo, &messageInfo,
                                  &junkScore, &junkStatus))) {
            if (junkScore.score > highScore && highScore != -1) {
              highScore = junkScore.score;
              // highSig = transInfo.id; // !!! Is this right ??
            } else if (junkScore.score == -1) {
              highScore = -1;
            }
          }
        }
      }
      HUnlock((void **)tList);
    }

    if (body != NULL)
      free(body);
    if (headers != NULL)
      free(headers);
  }

  // We want to record scores of zero or -1 that come out
  // of plug-ins.  The only time we don't put a score
  // on a message is when no plug-in ran successfully on it
  if (highScore > -2 && howToScore == kAutoScore)
    // we're not recording id's right now
    // JunkSetScore ( tocH, sumNum, highSig, highScore );
    JunkSetScore(tocH, sumNum, JUNK_BECAUSE_PLUG, MAX(0, highScore));

  // cleanup after CacheIMAPMessage.
  // Zap the cache, unless this message still needs to be filtered.
  if (tocH->imapTOC)
    if ((tocH->sums[sumNum].flags & FLAG_UNFILTERED) == 0)
      free(tocH->sums[sumNum].cache);
}

/************************************************************************
 * JunkScoreBox - Score all the messages in a mailbox
 ************************************************************************/
void JunkScoreBox(TOCType * tocH, short first, short last, bool rescore) {
  TLMHandle tList = NULL;
  short i;
  bool whiteCheck = JunkPrefWhiteList();
  bool midCheck = JunkPrefWhiteListReplies();

  ASSERT(tocH != NULL);
  UseFeature(featureJunk);
  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);

  if (first < 0)
    first = 0;
  if (last < 0 || last >= tocH->count)
    last = tocH->count - 1;

  //	We want to run through all these messages even if we have no junk
  // plugins 	because we might want to mark some for whitelisting.
  for (i = first; i <= last; ++i)
    if (rescore || !tocH->sums[i].spamBecause) {
      if (midCheck) {
        if (WhiteListByMID(tocH, i)) {
          JunkSetScore(tocH, i, JUNK_BECAUSE_WHITE, 0);
          continue;
        }
      }
      if (whiteCheck) {
        EnsureFromHash(tocH, i);
        if (tocH->sums[i].fromHash != kNoMessageId)
          if (HashAppearsInAliasFile(tocH->sums[i].fromHash, NULL)) {
            JunkSetScore(tocH, i, JUNK_BECAUSE_WHITE, 0);
            continue;
          }
      }
      //	Otherwise
      CycleBalls();
      if (tList != NULL)
        ScoreOneMessage(tList, tocH, i, kAutoScore);
    }

  if (tList != NULL)
    free((void **)tList);
}

/************************************************************************
 * JunkScoreIMAPBox - Score the messages in an IMAP mailbox.
 *
 * Like JunkScoreBox, but handles IMAP nuances.  Specify first and last
 *	for a range to junk, (-1) for each to score the entire mailbox, and
 *	bUnfilteredOnly to do unfiltered only.
 *
 * Very rarely would one want to score an entire IMAP mailbox, certainly
 *	not after a mailcheck as we do in POP.  This routine is cirtually
 *	identical to its POP equivalent, but allows for scoring of messages
 *	about to be filtered as well.
 ************************************************************************/
void JunkScoreIMAPBox(TOCType * tocH, short first, short last,
                      bool bUnfilteredOnly) {
  TLMHandle tList = NULL;
  short i;
  bool whiteCheck = JunkPrefWhiteList();

  UseFeature(featureJunk);
  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);
  if (tList != NULL) {
    if (first < 0)
      first = 0;
    if (last < 0 || last >= tocH->count)
      last = tocH->count - 1;

    for (i = first; i <= last; ++i) {

      // only do unfiltered messages if the caller has requested that
      if (bUnfilteredOnly && ((tocH->sums[i].flags & FLAG_UNFILTERED) == 0))
        continue;

      if (whiteCheck) {
        EnsureFromHash(tocH, i);
        if (tocH->sums[i].fromHash != kNoMessageId)
          if (HashAppearsInAliasFile(tocH->sums[i].fromHash, NULL)) {
            JunkSetScore(tocH, i, JUNK_BECAUSE_WHITE, 0);
            continue;
          }
      }
      //	Otherwise
      CycleBalls();
      ScoreOneMessage(tList, tocH, i, false);
    }

    free((void **)tList);
  }
}

/************************************************************************
 * JunkScoreSelected - Score the selected messages in a mailbox
 ************************************************************************/
void JunkScoreSelected(TOCType * tocH) {
  TLMHandle tList = NULL;
  short i;
  bool whiteCheck = JunkPrefWhiteList();

  ASSERT(tocH != NULL);
  UseFeature(featureJunk);
  (void)ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);

  //	We want to run through all these messages even if we have no junk
  // plugins 	because we might want to mark some for whitelisting.
  for (i = 0; i < tocH->count; ++i) {
    if (tocH->sums[i].selected) {
      if (whiteCheck) {
        EnsureFromHash(tocH, i);
        if (tocH->sums[i].fromHash != kNoMessageId)
          if (HashAppearsInAliasFile(tocH->sums[i].fromHash, NULL)) {
            JunkSetScore(tocH, i, JUNK_BECAUSE_WHITE, 0);
            continue;
          }
      }
      //	Otherwise
      CycleBalls();
      if (tList != NULL)
        ScoreOneMessage(tList, tocH, i, kAutoScore);
    }
  }

  if (tList != NULL)
    free((void **)tList);
}

//	Can we actually score mail?
//	Really a shorthand for "are there any plugins that can score mail
// installed"?
bool CanScoreJunk() {
  TLMHandle tList = NULL;
  int err = ETLListAllTranslators(&tList, EMSF_JUNK_MAIL);
  if (tList != NULL)
    free((void **)tList);
  return err == 0;
}

/************************************************************************
 * CanScoreToc - Can we score this tocH?
 *	false 	if it's an IMAP mailbox and bJunkPrefIMAPNoRunPlugins is set
 *	true 	in all other cases
 ************************************************************************/
bool CanScoreToc(TOCType * tocH) {
  bool bScore = true;
  PersHandle pers;

  if (tocH == NULL)
    return (false);

  pers = TOCToPers(tocH);
  if ((pers != NULL) && IsIMAPPers(pers)) {
    PushPers(pers);
    bScore = !JunkPrefIMAPNoRunPlugins();
    PopPers();
  }

  return (bScore);
}

/************************************************************************
 * JunkMoveIMAPMessages - go through a virtual toc and move all selected
 *	IMAP messages to the junk or inbox folder
 *
 *	This is fairly tricky as
 *		(a) the virtual toc contains messages in different IMAP
 *mailboxes (b) the messages being moved may go to different destinations (c)
 *IMAP transfers suck in that they can take a long time (d) there may be POP
 *messages in the mailbox we need to ignore
 *
 *	This routine sets up one transfer operation per imap mailbox
 *	represented in the virtual toc.
 ************************************************************************/
void JunkMoveIMAPMessages(TOCType * tocH, bool isJunk) {
  short i, j;
  TOCType *realToc, *destTocH;
  short realSum;
  MailboxNodeHandle dest;
  PersHandle pers;
  Accumulator a;
  // char * uids; // Unused

  for (i = 0; i < tocH->count; i++) {
    if ((tocH->sums[i].selected) &&
        ((tocH->sums[i].opts & OPT_DELETED) == 0)) {
      // initialize
      destTocH = NULL;

      // is this selected message an IMAP message?
      realToc = GetRealTOC(tocH, i, &realSum);
      if (realToc && realToc->imapTOC) {
        pers = TOCToPers(realToc);
        if (pers) {
          // where is this particular IMAP message going?
          if (!isJunk) {
            // we're not junking something in the junk mailbox
            if (BoxIsJunkBox(realToc)) {
              // Put the message back into the proper inbox.
              if ((dest = LocateInboxForPers(pers)) != NULL)
                destTocH = TOCBySpec(&dest->mailboxSpec);
            }
          } else {
            // we're junking something in something other than the junk box
            if (!BoxIsJunkBox(realToc)) {
              // Get the proper JUNK mailbox
              dest = GetIMAPJunkMailbox(pers, true, false);
              if (dest && (dest != TOCToMbox(realToc)))
                destTocH = TOCBySpec(&dest->mailboxSpec);
            }
          }
        }
      }

      // build a list of messages to transfer
      AccuInit(&a);

      // properly handle all of the messages in this real toc
      for (j = i; j < tocH->count; j++) {
        if ((tocH->sums[j].selected) &&
            ((tocH->sums[j].opts & OPT_DELETED) == 0)) {
          TOCType * rt;
          short rs;

          rt = GetRealTOC(tocH, j, &rs);
          // is this message  in the same IMAP mailbox we're currently
          // considering?
          if (rt && (rt == realToc) && (rt->imapTOC)) {
            // are we moving this message?
            if (destTocH) {
              // add it to the list of messages to transfer
              AccuAddPtr(&a, &(rt->sums[rs].uidHash), sizeof(long));

              // invalidate the summary in the search window
              // invalid member access:
              // tocH->sums[j].u.virtualMess.virtualMBIdx = -1;
              InvalSum(tocH, j);
            }

            // do not reprocess this message
            tocH->sums[j].selected = false;
          }
        }
      }

      // move the messages if they're going anywhere.
      if (destTocH) {
        AccuTrim(&a);
        void *uidsH = NULL;
        if (PtrToHand(a.data, &uidsH, a.offset) == noErr) {
          IMAPTransferMessages(realToc, destTocH, uidsH, false, false);
          free(uidsH);
        }
      }
      free(a.data); a.data = NULL; a.offset = a.size = 0;
    }
  }
}

/************************************************************************
 * WhiteListByMID - are any of a message's references on a list of messages we
 *sent?
 ************************************************************************/
bool WhiteListByMID(TOCType * tocH, short sumNum) {
  void **text;

  if (SumFlagIsSet(tocH, sumNum, FLAG_KNOWS_ME))
    return true;
  if (CacheMessage(tocH, sumNum))
    return false;
  if (!OutgoingMIDList || !OutgoingMIDList->len)
    return false;

  if ((text = tocH->sums[sumNum].cache)) {
    void **references = NULL;
    HNoPurge(text);
    if (!HandleHeadGetIdText(text, HeaderStrn + REFERENCES_HEAD, &references)) {
      char **addresses = NULL;
      Tr(references, (unsigned char *)" \t\r\n", (unsigned char *)",,,,");
      if (!SuckAddresses(&addresses, references, false, false, false, NULL)) {
        uint32_t *mids = (uint32_t *)OutgoingMIDList->data;
        guint midCount = OutgoingMIDList->len;

        for (int i = 0; addresses[i]; i++) {
          uint32_t h = Hash((unsigned char *)addresses[i]);
          bool found = false;
          for (guint mi = 0; mi < midCount; mi++) {
            if (mids[mi] == h) { found = true; break; }
          }
          if (found) {
            SetSumFlag(tocH, sumNum, FLAG_KNOWS_ME);
            break;
          }
        }
        g_strfreev(addresses); addresses = NULL;
      }
      free(references);
    }
    HPurge(text);
  }

  // did we win?
  return SumFlagIsSet(tocH, sumNum, FLAG_KNOWS_ME);
}
