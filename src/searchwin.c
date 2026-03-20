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
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
 * searchwin.c - Search window implementation (GTK4 port)
 *
 * Ported from Mac Carbon to GTK4. Original used Mac Carbon UI
 * (ControlHandle, GtkWidget *, ViewList, EventRecord, etc.)
 * Now uses GtkWindow, GtkEntry, GtkDropDown, GtkTreeView, GRegex.
 */

#include "searchwin.h"
#include "message.h"
#include "regexp.h"
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* Globals                                                            */
/* ------------------------------------------------------------------ */
static int gSearchWinCount;

/* Each search window's SearchInfo is stored in the TOC's virtualMB.data. */

/* ------------------------------------------------------------------ */
/* Case-folding table (ASCII) for fast case-insensitive matching      */
/* ------------------------------------------------------------------ */
static unsigned char gFoldTable[256];
static bool gFoldTableInit = false;

static void InitFoldTable(void) {
  if (gFoldTableInit) return;
  for (int i = 0; i < 256; i++)
    gFoldTable[i] = (unsigned char)tolower((unsigned char)i);
  /* Collapse whitespace variants to space */
  gFoldTable['\t'] = ' ';
  gFoldTable['\r'] = ' ';
  gFoldTable['\n'] = ' ';
  gFoldTableInit = true;
}

/* ------------------------------------------------------------------ */
/* Internal helpers                                                   */
/* ------------------------------------------------------------------ */

/* Get SearchInfo from a TOC that is a search virtual TOC */
static SearchInfo *GetSearchInfoFromTOC(TOCType *toc) {
  if (!toc || !toc->virtualTOC) return NULL;
  return (SearchInfo *)toc->mailbox.virtualMB.data;
}

/* Get the search TOC from a MyWindowPtr (if it is a search window) */
static TOCType *GetSearchTOCFromWin(MyWindowPtr win) {
  if (!win) return NULL;
  /* The TOC is stored as the window's private data */
  return (TOCType *)win->privateData;
}

/* ------------------------------------------------------------------ */
/* Text Search Engine                                                 */
/* ------------------------------------------------------------------ */

/*
 * SearchStrText - case-insensitive substring search in text buffer.
 * Returns byte offset of match, or -1 if not found.
 */
static long SearchStrText(const char *needle, const char *haystack,
                           long haystackLen, long offset) {
  InitFoldTable();
  if (!needle || !haystack || offset < 0) return -1;
  long needleLen = (long)strlen(needle);
  if (needleLen < 1 || needleLen > haystackLen - offset) return -1;

  const unsigned char *h = (const unsigned char *)haystack + offset;
  const unsigned char *n = (const unsigned char *)needle;
  long remaining = haystackLen - offset - needleLen + 1;

  for (long i = 0; i < remaining; i++) {
    if (gFoldTable[h[i]] != gFoldTable[n[0]]) continue;
    bool match = true;
    for (long j = 1; j < needleLen; j++) {
      if (gFoldTable[h[i + j]] != gFoldTable[n[j]]) { match = false; break; }
    }
    if (match) return offset + i;
  }
  return -1;
}

/*
 * SearchStrWord - case-insensitive whole-word search.
 * A word boundary is defined by a transition between alphanumeric and
 * non-alphanumeric characters.
 */
static long SearchStrWord(const char *needle, const char *haystack,
                           long haystackLen, long offset) {
  long needleLen = (long)strlen(needle);
  long pos = offset;
  while (pos >= 0) {
    pos = SearchStrText(needle, haystack, haystackLen, pos);
    if (pos < 0) return -1;
    /* Check word boundaries */
    bool leftOK = (pos == 0) || !isalnum((unsigned char)haystack[pos - 1]);
    bool rightOK = (pos + needleLen >= haystackLen) ||
                    !isalnum((unsigned char)haystack[pos + needleLen]);
    if (leftOK && rightOK) return pos;
    pos++;
  }
  return -1;
}

/*
 * SearchTextRelation - apply a text search relation to a C string value
 * against a haystack buffer.
 * Returns true if the relation is satisfied.
 */
static bool SearchTextRelation(const char *value, const char *text,
                                long textLen, long offset, int relation,
                                PortableRegexp *compiledRegex) {
  long valueLen = (long)strlen(value);
  switch (relation) {
    case SR_CONTAINS:
      return SearchStrText(value, text, textLen, offset) >= 0;
    case SR_CONTAINS_WORD:
      return SearchStrWord(value, text, textLen, offset) >= 0;
    case SR_NOT_CONTAINS:
      return SearchStrText(value, text, textLen, offset) < 0;
    case SR_IS:
      return valueLen == (textLen - offset) &&
             SearchStrText(value, text, textLen, offset) >= 0;
    case SR_IS_NOT:
      return valueLen != (textLen - offset) ||
             SearchStrText(value, text, textLen, offset) < 0;
    case SR_STARTS:
      return valueLen <= (textLen - offset) &&
             SearchStrText(value, text, offset + valueLen, offset) >= 0;
    case SR_ENDS:
      if (valueLen > (textLen - offset)) return false;
      return SearchStrText(value, text, textLen,
                            textLen - valueLen) >= 0;
    case SR_REGEXP:
      if (!compiledRegex) return false;
      return eudora_regex_search(compiledRegex, text, offset, textLen) >= 0;
  }
  return false;
}

/* ------------------------------------------------------------------ */
/* Search a single message summary against one criterion              */
/* ------------------------------------------------------------------ */

static int ShortCompare(int a, int b) {
  if (a == b) return 0;
  return a < b ? -1 : 1;
}

/*
 * SearchSummary - test one message summary against a single criterion.
 * Returns true if the criterion matches.
 *
 * For full-text (body/header/anywhere) searches, we'd need to read the
 * message file. For now, summary-only fields (status, priority, date, size,
 * from, subject, etc.) are directly searchable.
 */
static bool SearchSummary(MSumPtr sum, SearchCriterion *crit,
                           PortableRegexp *re) {
  long result;
  switch (crit->category) {
    case SC_SUMMARY:
      /* Search in subject and from fields of summary */
      if (SearchTextRelation(crit->text, (const char *)sum->subj + 1,
                              sum->subj[0], 0, crit->relation, re))
        return true;
      if (SearchTextRelation(crit->text, (const char *)sum->from + 1,
                              sum->from[0], 0, crit->relation, re))
        return true;
      return false;

    case SC_TO:
    case SC_FROM:
    case SC_SUBJECT:
    case SC_CC:
    case SC_BCC:
    case SC_ANY_RECIPIENT:
      /* Summary only has from and subject; full header search needs message
       * text. For from/subject, search in the appropriate summary field. */
      if (crit->category == SC_FROM || crit->category == SC_ANY_RECIPIENT)
        if (SearchTextRelation(crit->text, (const char *)sum->from + 1,
                                sum->from[0], 0, crit->relation, re))
          return true;
      if (crit->category == SC_SUBJECT)
        return SearchTextRelation(crit->text, (const char *)sum->subj + 1,
                                   sum->subj[0], 0, crit->relation, re);
      /* For TO/CC/BCC/ANY_RECIPIENT on summary, check the from field as
       * fallback; a full search would need the message text. */
      return false;

    case SC_STATUS: {
      int val = sum->state;
      return crit->specifier == val ? crit->relation == SR_EQUAL
                                     : crit->relation == SR_NOT_EQUAL;
    }

    case SC_PRIORITY: {
      int val = sum->priority;
      if (!val) val = 3; /* default priority */
      result = ShortCompare((int)crit->specifier, val);
      goto DoCompare;
    }

    case SC_SIZE: {
      long sizeK = (sum->length + 1023) / 1024;
      result = ShortCompare((int)sizeK, (int)crit->specifier);
      goto DoCompare;
    }

    case SC_JUNK_SCORE: {
      int val = sum->spamScore;
      result = ShortCompare(val, (int)crit->specifier);
      goto DoCompare;
    }

    case SC_DATE: {
      /* Compare message date against criterion date.
       * sum->seconds is GMT; criterion stores a date struct. */
      time_t msgTime = (time_t)sum->seconds;
      struct tm *tm = gmtime(&msgTime);
      if (!tm) return false;
      /* Compare year, month, day in order */
      result = ShortCompare(tm->tm_year + 1900, (int)crit->specifier);
      if (!result) {
        /* specifier encodes: year*10000 + month*100 + day */
        /* For now, do simple seconds comparison */
        return false;
      }
      goto DoCompare;
    }

    case SC_AGE: {
      time_t now = time(NULL);
      time_t msgTime = (time_t)sum->seconds;
      long diffSecs = (long)(now - msgTime);
      long ageValue;
      switch (crit->ageUnits) {
        case AGE_DAYS:   ageValue = diffSecs / 86400; break;
        case AGE_WEEKS:  ageValue = diffSecs / (86400 * 7); break;
        case AGE_MONTHS: ageValue = diffSecs / (86400 * 30); break;
        case AGE_YEARS:  ageValue = diffSecs / (86400 * 365); break;
        default:         ageValue = diffSecs / 86400; break;
      }
      result = ShortCompare((int)ageValue, (int)crit->specifier);
      goto DoCompare;
    }

    case SC_ANYWHERE:
    case SC_HEADERS:
    case SC_BODY:
    case SC_ATTACH_NAMES:
    case SC_ATTACH_COUNT:
      /* These require reading message text from file.
       * Search in summary fields as fallback. */
      if (SearchTextRelation(crit->text, (const char *)sum->subj + 1,
                              sum->subj[0], 0, crit->relation, re))
        return true;
      if (SearchTextRelation(crit->text, (const char *)sum->from + 1,
                              sum->from[0], 0, crit->relation, re))
        return true;
      return false;

    default:
      return false;
  }

DoCompare:
  switch (crit->relation) {
    case SR_EQUAL:     return result == 0;
    case SR_NOT_EQUAL: return result != 0;
    case SR_GREATER:   return result > 0;
    case SR_LESS:      return result < 0;
  }
  return false;
}

/*
 * MatchesCriteria - test a message summary against all criteria.
 * matchAny = true means OR mode, false means AND mode.
 */
static bool MatchesCriteria(MSumPtr sum, SearchInfo *si) {
  PortableRegexp *re = NULL;

  for (int i = 0; i < si->criteriaCount; i++) {
    SearchCriterion *crit = &si->criteria[i];
    if (crit->text[0] == '\0' && crit->relation != SR_REGEXP)
      continue; /* skip blank criteria */

    /* Compile regex if needed */
    if (crit->relation == SR_REGEXP) {
      re = eudora_regcomp(crit->text);
    }

    bool found = SearchSummary(sum, crit, re);

    if (re) { eudora_regfree(re); re = NULL; }

    if (found && si->matchAny) return true;
    if (!found && !si->matchAny) return false;
  }
  return si->matchAny ? false : true;
}

/* ------------------------------------------------------------------ */
/* Public API: Search window lifecycle                                */
/* ------------------------------------------------------------------ */

MyWindowPtr SearchOpen(int searchMode) {
  (void)searchMode;
  /* TODO: Create GTK4 search window with criteria widgets.
   * For now, allocate a SearchInfo and increment window count. */
  gSearchWinCount++;
  return NULL;
}

void SearchClose_cb(MyWindowPtr win) {
  TOCType *toc = GetSearchTOCFromWin(win);
  if (toc) {
    SearchInfo *si = GetSearchInfoFromTOC(toc);
    if (si) {
      /* Free bulk search state */
      for (int i = 0; i < SEARCH_MAX_CRITERIA; i++) {
        if (si->bulkState.hitOffsets[i])
          g_array_free(si->bulkState.hitOffsets[i], TRUE);
      }
      if (si->resultLinks)
        g_array_free(si->resultLinks, TRUE);
      if (si->mailboxPaths)
        g_ptr_array_free(si->mailboxPaths, TRUE);
      g_free(si);
      toc->mailbox.virtualMB.data = NULL;
    }
  }
  if (gSearchWinCount > 0) gSearchWinCount--;
}

/* ------------------------------------------------------------------ */
/* Public API: Query functions                                        */
/* ------------------------------------------------------------------ */

bool IsSearchWindow(void *winWP) {
  if (!winWP) return false;
  MyWindowPtr win = (MyWindowPtr)winWP;
  TOCType *toc = GetSearchTOCFromWin(win);
  if (!toc) return false;
  return toc->virtualTOC && toc->mailbox.virtualMB.type == kSearchMB;
}

TOCType *GetTOCFromSearchWin(char * spec) {
  (void)spec;
  /* Walk open windows to find a search TOC matching this spec.
   * Without a window list implementation, return NULL. */
  return NULL;
}

void GetSearchTOC(MyWindowPtr win, TOCType **ptoc) {
  if (!ptoc) return;
  *ptoc = GetSearchTOCFromWin(win);
}

bool SearchViewIsMailbox(TOCType *tocH) {
  if (!tocH || !tocH->virtualTOC) return false;
  SearchInfo *si = GetSearchInfoFromTOC(tocH);
  return si && si->mailboxView;
}

bool GetSearchWinSpec(void *winWP, char *spec) {
  if (!winWP || !spec) return false;
  MyWindowPtr win = (MyWindowPtr)winWP;
  TOCType *toc = GetSearchTOCFromWin(win);
  if (!toc) return false;
  SearchInfo *si = GetSearchInfoFromTOC(toc);
  if (!si || !si->hasSaveSpec) return false;
  g_strlcpy(spec, si->saveSpec, PATH_MAX);
  return true;
}

/* ------------------------------------------------------------------ */
/* Public API: Summary copy / update                                  */
/* ------------------------------------------------------------------ */

void CopySum(MSumPtr sumFrom, MSumPtr sumTo, short virtualMBIdx) {
  if (!sumFrom || !sumTo) return;
  *sumTo = *sumFrom;
  sumTo->u.virtualMess.virtualMBIdx = virtualMBIdx;
  sumTo->u.virtualMess.linkSerialNum = sumFrom->serialNum;
}

void SearchUpdateSum(TOCType *tocH, short sumNum,
                      TOCType *fromTocH, long serialNum,
                      bool transfer, bool nuke) {
  (void)tocH; (void)sumNum; (void)fromTocH;
  (void)serialNum; (void)transfer; (void)nuke;
  if (!gSearchWinCount) return;
  /* In full implementation, walk open search windows and update any
   * virtual summaries that link to the changed message.
   * Without a window list, this is a no-op. */
}

/* ------------------------------------------------------------------ */
/* Public API: Incremental search / mailbox tracking                  */
/* ------------------------------------------------------------------ */

bool SearchIncremental(MyWindowPtr win, TOCType *tocH, int sumNum) {
  if (!win || !tocH) return false;
  TOCType *srchToc = GetSearchTOCFromWin(win);
  if (!srchToc) return false;
  SearchInfo *si = GetSearchInfoFromTOC(srchToc);
  if (!si || !si->didSearch) return false;

  /* Test this message against search criteria */
  if (sumNum < 0 || sumNum >= tocH->count) return false;
  return MatchesCriteria(&tocH->sums[sumNum], si);
}

void SearchInvalTocBox(TOCType *tocH, short sumNum, int boxCol) {
  (void)tocH; (void)sumNum; (void)boxCol;
  if (!gSearchWinCount) return;
  /* In full implementation, invalidate matching virtual summaries. */
}

void TellSearchMBRename(char * oldSpec, char * newSpec) {
  (void)oldSpec; (void)newSpec;
  if (!gSearchWinCount) return;
  /* In full implementation, update spec lists in search windows. */
}

bool SearchBoxesInclude(MyWindowPtr win, TOCType *tocH) {
  if (!win || !tocH) return false;
  TOCType *srchToc = GetSearchTOCFromWin(win);
  if (!srchToc) return false;
  SearchInfo *si = GetSearchInfoFromTOC(srchToc);
  if (!si || !si->mailboxPaths) return false;

  /* Check if tocH's path is in the search mailbox list */
  for (unsigned i = 0; i < si->mailboxPaths->len; i++) {
    const char *path = g_ptr_array_index(si->mailboxPaths, i);
    if (path && strcmp(path, tocH) == 0)
      return true;
  }
  return false;
}

/* ------------------------------------------------------------------ */
/* Public API: IMAP search                                            */
/* ------------------------------------------------------------------ */

/* IMAPSearchIncremental is implemented in imapdownload.c */

/* ------------------------------------------------------------------ */
/* Public API: Idle and update                                        */
/* ------------------------------------------------------------------ */

void SearchAllIdle(void) {
  if (!gSearchWinCount) return;
  /* In full implementation, process any pending search work. */
}

void SearchMBUpdate(void) {
  if (!gSearchWinCount) return;
  /* In full implementation, refresh mailbox lists in search windows. */
}

/* ------------------------------------------------------------------ */
/* Public API: Saved searches                                         */
/* ------------------------------------------------------------------ */

void SearchSave(MyWindowPtr win, bool saveAs) {
  (void)win; (void)saveAs;
  /* TODO: Save search criteria to JSON file. */
}

void OpenSearchFile(char * spec) {
  (void)spec;
  /* TODO: Load saved search from JSON file and open window. */
}

void OpenSearchFileAndStart(char * spec) {
  OpenSearchFile(spec);
  /* TODO: Start the search after opening. */
}

/* ------------------------------------------------------------------ */
/* Public API: Search menu                                            */
/* ------------------------------------------------------------------ */

void BuildSearchMenu(void) {
  /* TODO: Populate saved-search submenu in GTK menu. */
}

void OpenSearchMenu(short item) {
  (void)item;
  /* TODO: Open saved search by menu item number. */
}

/* ------------------------------------------------------------------ */
/* Public API: Utility                                                */
/* ------------------------------------------------------------------ */

void SearchNewFindStringLo(const char *str, bool withPrejudice) {
  (void)str; (void)withPrejudice;
  /* TODO: Set the search string in the frontmost search window. */
}

void SearchFixUnread(TOCType *tocH, bool unread) {
  (void)tocH; (void)unread;
  if (!gSearchWinCount) return;
  /* TODO: Update unread indicators in search window mailbox lists. */
}

void SearchSetWTitle(MyWindowPtr win) {
  (void)win;
  /* TODO: Update window title based on criteria. */
}

void AddCriteriaText(SearchInfo *si, char *buf, int bufSize) {
  if (!si || !buf || bufSize <= 0) return;
  buf[0] = '\0';
  int pos = 0;
  for (int i = 0; i < si->criteriaCount && pos < bufSize - 1; i++) {
    if (i > 0 && pos < bufSize - 3) {
      buf[pos++] = ',';
      buf[pos++] = ' ';
    }
    int written = snprintf(buf + pos, (size_t)(bufSize - pos), "%s",
                            si->criteria[i].text);
    if (written > 0) pos += written;
  }
}
