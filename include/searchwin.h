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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. */

#ifndef SEARCHWIN_H
#define SEARCHWIN_H

#include "mailbox.h"
#include "toc.h"
#include <gtk/gtk.h>
#include <stdbool.h>

/* Search criteria categories */
enum {
  SC_ANYWHERE = 1, SC_HEADERS, SC_BODY, SC_ATTACH_NAMES, SC_DIV1,
  SC_SUMMARY, SC_STATUS, SC_JUNK_SCORE, SC_PRIORITY,
  SC_ATTACH_COUNT, SC_LABEL, SC_DATE, SC_SIZE, SC_AGE,
  SC_PERSONALITY, SC_DIV2,
  SC_TO, SC_FROM, SC_SUBJECT, SC_CC, SC_BCC, SC_ANY_RECIPIENT,
  SC_LIMIT
};

/* Search relations for text */
enum { SR_CONTAINS = 1, SR_CONTAINS_WORD, SR_NOT_CONTAINS,
       SR_IS, SR_IS_NOT, SR_STARTS, SR_ENDS, SR_REGEXP };

/* Search relations for comparison */
enum { SR_EQUAL = 1, SR_NOT_EQUAL, SR_GREATER, SR_LESS };

/* Match mode */
enum { MATCH_ALL = 1, MATCH_ANY };

/* Tab mode */
enum { TAB_MAILBOXES = 1, TAB_RESULTS };

/* Age units for SC_AGE criterion */
enum { AGE_DAYS = 0, AGE_WEEKS, AGE_MONTHS, AGE_YEARS };

/* Bulk search I/O buffer size */
#define BULK_SEARCH_BUF_SIZE (32 * 1024)
#define BULK_SEARCH_NBUFS 2

/* Saved search folder name */
#define SEARCH_FOLDER_NAME "Search Folder"
#define SEARCH_FILE_EXT ".esj"

#define SEARCH_MAX_CRITERIA 16

/* Single search criterion */
typedef struct {
  int category;       /* SC_ANYWHERE..SC_ANY_RECIPIENT */
  int relation;       /* SR_CONTAINS..SR_REGEXP or SR_EQUAL..SR_LESS */
  char text[256];     /* search text (C string) */
  long specifier;     /* numeric specifier for status/priority/etc */
  int ageUnits;       /* AGE_DAYS..AGE_YEARS for SC_AGE */
} SearchCriterion;

/* Search result entry - links a virtual summary to its source */
typedef struct {
  int sourceMBIdx;    /* index into specList */
  long linkSerial;    /* serialNum of source message */
  int sourceSumNum;   /* sumNum in source TOC */
} SearchResultLink;

/* Bulk search buffer for fast file I/O */
typedef struct {
  char *buf;          /* buffer data */
  long offset;        /* file offset of this buffer */
  long size;          /* bytes read into buffer */
  bool free;          /* buffer available for use */
} BulkSearchBuf;

/* Bulk search state for one mailbox file */
typedef struct {
  FILE *fp;                        /* open mailbox file */
  long fileLen;                    /* total file size */
  long readPos;                    /* current read position */
  BulkSearchBuf bufs[BULK_SEARCH_NBUFS];
  bool active;                     /* bulk search in progress */
  /* Per-criterion hit tracking: offsets where each criterion matched */
  GArray *hitOffsets[SEARCH_MAX_CRITERIA]; /* arrays of long */
  int nCriteriaTracked;
} BulkSearchState;

/* Main search window state */
typedef struct SearchInfo {
  int criteriaCount;
  SearchCriterion criteria[SEARCH_MAX_CRITERIA];
  bool matchAny;            /* true = match any, false = match all */
  bool searching;           /* actively searching */
  bool didSearch;           /* has searched at least once */
  bool mailboxView;         /* showing mailbox tab vs results tab */
  long nHits;               /* number of hits found */
  bool dirty;               /* unsaved changes */

  /* Saved file */
  char saveSpec[PATH_MAX];           /* spec of saved search file (if any) */
  bool hasSaveSpec;          /* true if saveSpec is valid */

  /* Search progress tracking */
  int currentBox;           /* current mailbox index being searched */
  int currentMsg;           /* current message index in current mailbox */
  GPtrArray *mailboxPaths;  /* array of mailbox file paths to search */
  MacmbxTOC * currentTOC;     /* TOC currently being searched */
  bool currentTOCOpened;    /* we opened it (need to close) */

  /* Bulk search state */
  BulkSearchState bulkState;
  bool noBulkSearch;         /* disable bulk search (for incremental) */

  /* Result links */
  GArray *resultLinks;      /* array of SearchResultLink */

  /* GTK widgets */
  GtkWidget *notebook;      /* GtkNotebook (Mailboxes | Results) */
  GtkWidget *mbTreeView;    /* mailbox selection tree */
  GtkWidget *resultView;    /* results GtkTreeView */
  GtkWidget *resultStore;   /* GtkListStore for results */
  GtkWidget *searchBtn;     /* Search / Stop button */
  GtkWidget *moreBtn;       /* More Criteria button */
  GtkWidget *fewerBtn;      /* Fewer Criteria button */
  GtkWidget *matchCombo;    /* All/Any combo */
  GtkWidget *criteriaBox;   /* GtkBox holding criteria rows */
  GtkWidget *statusLabel;   /* "Searching mailbox X..." label */

  /* Per-criterion GTK widgets */
  struct {
    GtkWidget *row;         /* container for this criterion */
    GtkWidget *catCombo;    /* category combo */
    GtkWidget *relCombo;    /* relation combo */
    GtkWidget *entry;       /* text entry (or NULL) */
    GtkWidget *specCombo;   /* specifier combo (or NULL) */
  } criteriaWidgets[SEARCH_MAX_CRITERIA];
} SearchInfo;

/* Search window lifecycle */
MyWindowPtr SearchOpen(int searchMode);
void        SearchClose_cb(MyWindowPtr win);
GtkWidget  *CreateSearchPanel(void);

/* Externally-used functions */
bool        IsSearchWindow(void *winWP);
MacmbxTOC *   GetTOCFromSearchWin(char * spec);
void        GetSearchTOC(MyWindowPtr win, MacmbxTOC * *ptoc);
void        SearchUpdateSum(MacmbxTOC * tocH, short sumNum,
                            MacmbxTOC * fromTocH, long serialNum,
                            bool transfer, bool nuke);
void        CopySum(MacmbxMsgSum * sumFrom, MacmbxMsgSum * sumTo, short virtualMBIdx);
void        IMAPSearchIncremental(MailboxNodeHandle mbox);
void        SearchAllIdle(void);
void        SearchMBUpdate(void);

/* Incremental search / mailbox tracking */
bool        SearchIncremental(MyWindowPtr win, MacmbxTOC * tocH, int sumNum);
void        SearchInvalTocBox(MacmbxTOC * tocH, short sumNum, int boxCol);
void        TellSearchMBRename(char * oldSpec, char * newSpec);
bool        SearchBoxesInclude(MyWindowPtr win, MacmbxTOC * tocH);

/* Saved search files */
void        SearchSave(MyWindowPtr win, bool saveAs);
void        OpenSearchFile(char * spec);
void        OpenSearchFileAndStart(char * spec);

/* Search menu */
void        BuildSearchMenu(void);
void        OpenSearchMenu(short item);

/* Utility */
void        SearchNewFindStringLo(const char *str, bool withPrejudice);
void        SearchFixUnread(MacmbxTOC * tocH, bool unread);
bool        GetSearchWinSpec(void *winWP, char *spec);
void        SearchSetWTitle(MyWindowPtr win);
void        AddCriteriaText(SearchInfo *si, char *buf, int bufSize);

/* Query */
bool        SearchViewIsMailbox(MacmbxTOC * tocH);

#endif /* SEARCHWIN_H */
