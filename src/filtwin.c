/* Copyright (c) 2017, Computer History Museum
All rights reserved. (BSD license — see original file for full text)
Filters window — ported from Mac Carbon/QuickDraw to GTK4.
Original: filtwin.c + filtmng.c persistence logic. */

#include "filters.h"
#include "filtrun.h"
#include "FiltDefs.h"
#include "features.h"
#include "mailbox.h"
#include "toc.h"
#include "junk.h"
#include "schizo.h"
#include "messact.h"
#include "nickmng.h"
#include "mydefs.h"
#include "fileutil.h"
#include "imapmailboxes.h"
#include "imapdownload.h"
#include "prefdefs.h"
#include "threading.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

/* Forward declarations for functions not yet ported to headers */
extern void SetState(TOCType *tocH, short sumNum, short state);
extern void SetPriority(TOCType *tocH, short sumNum, short priority);
extern short DoFordirectMessage(TOCType *tocH, short sumNum, short action,
                                unsigned char *addresses, bool now);
extern short DoReplyClosed(TOCType *tocH, short sumNum, bool all, bool self,
                           bool quote, bool redo, short item, bool vis,
                           bool station);
extern void PlayNamedSound(unsigned char *name);
extern FSSpec GetMailboxSpec(TOCType *tocH, short which);
extern TOCType *TOCBySpec(FSSpec *spec);
extern bool SameSpec(FSSpec *a, FSSpec *b);
extern void InvalSum(TOCType *tocH, short sumNum);
extern bool FetchAllIMAPAttachments(TOCType *tocH, short sumNum, bool fg);
extern void CacheRecentNickname(void *addr);
extern short PrintClosedMessage(TOCType *tocH, short sumNum, bool now);

/* Filter return codes */
#ifndef euFilterStop
#define euFilterStop 1
#define euFilterXfered 2
#endif

#ifndef TS_TO_PPERS
#define TS_TO_PPERS(toc, sum) (FindPersById((toc)->sums[sum].persId))
#endif

/* Global Filters handle — the in-memory filter database */
void *Filters = NULL;
short FiltersRefCount = 0;
Handle PreFilters = NULL;   /* filter rules generated externally (plugins) */
Handle PostFilters = NULL;  /* filter rules generated externally (plugins) */

/* Safe string copy into fixed-size buffer */
static void sstrncpy(char *dst, const char *src, size_t dstsize) {
  if (dstsize == 0) return;
  strncpy(dst, src, dstsize - 1);
  dst[dstsize - 1] = '\0';
}

/* Convert C string to Pascal string (only for calling legacy APIs) */
static void c_to_pascal(unsigned char *pstr, const char *cstr) {
  int len = strlen(cstr);
  if (len > 255) len = 255;
  pstr[0] = (unsigned char)len;
  memcpy(pstr + 1, cstr, len);
}

/************************************************************************
 * Filter keyword strings — maps FilterKeywordEnum to text for file I/O
 * Must match FiltDefs.h enum order exactly.
 ************************************************************************/
static const char *FiltKeywords[] = {
  "",            /* flkZero (0) */
  "none",        /* flkNone (1) */
  "",            /* flkDash1 */
  "status",      /* flkStatus */
  "priority",    /* flkPriority */
  "label",       /* flkLabel */
  "personality", /* flkPersonality */
  "subject",     /* flkSubject */
  "",            /* flkDash7 */
  "sound",       /* flkSound */
  "speak",       /* flkSpeak */
  "open",        /* flkOpenMessage */
  "print",       /* flkPrint */
  "addhistory",  /* flkAddHistory */
  "notifyUser",  /* flkNotifyUser */
  "",            /* flkDash14 */
  "forward",     /* flkForward */
  "redirect",    /* flkRedirect */
  "reply",       /* flkReply */
  "",            /* flkDash18 */
  "serverOpt",   /* flkServerOpts */
  "",            /* flkDash20 */
  "copy",        /* flkCopy */
  "transfer",    /* flkTransfer */
  "junk",        /* flkJunk */
  "mvattach",    /* flkMoveAttach */
  "",            /* flkDash25 */
  "stop",        /* flkStop */
  "rule",        /* flkRule */
  "incoming",    /* flkIncoming */
  "outgoing",    /* flkOutgoing */
  "manual",      /* flkManual */
  "header",      /* flkHeader */
  "verb",        /* flkVerb */
  "value",       /* flkValue */
  "conjunction", /* flkConjunction */
  "name",        /* flkName */
  "copyInstead", /* flkCopyInstead */
  "raise",       /* flkRaise */
  "lower",       /* flkLower */
  "id",          /* flkId */
};
#define NUM_FILT_KEYWORDS (sizeof(FiltKeywords) / sizeof(FiltKeywords[0]))

static const char *VerbStrings[] = {
  "",            /* 0 — not used */
  "contains",    /* mbmContains = 1 */
  "!contains",   /* mbmNotContains */
  "is",          /* mbmIs */
  "!is",         /* mbmIsnt */
  "starts",      /* mbmStarts */
  "ends",        /* mbmEnds */
  "appears",     /* mbmAppears */
  "!appears",    /* mbmNotAppears */
  "intersects",  /* mbmIntersects */
  "disjoint",    /* mbmNotIntersects */
  "intersectsFile", /* mbmIntersectsFile */
  "disjointFile",   /* mbmNotIntersectsFile */
  "regex",       /* mbmRegEx */
  "less",        /* mbmJunkLess */
  "greater",     /* mbmJunkMore */
};
#define NUM_VERB_STRINGS (sizeof(VerbStrings) / sizeof(VerbStrings[0]))

static const char *ConjStrings[] = {
  "",        /* 0 — not used */
  "ignore",  /* cjIgnore = 1 */
  "and",     /* cjAnd */
  "or",      /* cjOr */
  "unless",  /* cjUnless */
};
#define NUM_CONJ_STRINGS (sizeof(ConjStrings) / sizeof(ConjStrings[0]))

/************************************************************************
 * FRInit — initialize a FilterRecord to defaults
 ************************************************************************/
static void FRInit(FilterRecord *fr) {
  memset(fr, 0, sizeof(*fr));
  fr->conjunction = cjIgnore;
  fr->terms[0].verb = mbmContains;
  fr->terms[1].verb = mbmContains;
}

/************************************************************************
 * Filter ID management
 ************************************************************************/
static long gNextFilterId = 1;

static long FilterNewId(void) { return gNextFilterId++; }

/************************************************************************
 * Global filter array — replaces Mac Handle-based storage
 ************************************************************************/
int gNFilters = 0;
FilterRecord *gFilterArray = NULL;

static int GetNFilters(void) { return gNFilters; }

static FilterRecord *GetFR(int idx) {
  if (idx < 0 || idx >= gNFilters) return NULL;
  return &gFilterArray[idx];
}

/************************************************************************
 * Lookup functions for file I/O
 ************************************************************************/
static FilterKeywordEnum LookupKeyword(const char *word) {
  for (int i = 1; i < (int)NUM_FILT_KEYWORDS; i++) {
    if (FiltKeywords[i][0] && strcmp(FiltKeywords[i], word) == 0)
      return (FilterKeywordEnum)i;
  }
  return flkZero;
}

static MatchEnum LookupVerb(const char *word) {
  for (int i = 1; i < (int)NUM_VERB_STRINGS; i++) {
    if (strcmp(VerbStrings[i], word) == 0)
      return (MatchEnum)i;
  }
  return mbmContains;
}

static ConjunctionEnum LookupConj(const char *word) {
  for (int i = 1; i < (int)NUM_CONJ_STRINGS; i++) {
    if (strcmp(ConjStrings[i], word) == 0)
      return (ConjunctionEnum)i;
  }
  return cjIgnore;
}

/* StudyFilter — preprocess filter (compile regex, set header IDs) */
static void StudyFilter(FilterRecord *fr) {
  for (int t = 0; t < 2; t++) {
    MatchTerm *mt = &fr->terms[t];
    mt->headerID = 0;
    if (mt->header[0]) {
      if (mt->verb == mbmRegEx && mt->value[0]) {
        if (mt->regex) regfree(mt->regex);
        else mt->regex = malloc(sizeof(regex_t));
        if (mt->regex) {
          if (regcomp(mt->regex, mt->value, REG_EXTENDED | REG_NOSUB) != 0) {
            free(mt->regex);
            mt->regex = NULL;
          }
        }
      }
    }
  }
}

/* Append action to end of linked list */
static void AppendAction(FActionHandle *list, FActionHandle fa) {
  FActionHandle *tail = list;
  while (*tail) tail = &((**tail)->next);
  *tail = fa;
}

/* Create a new FAction node */
static FActionHandle NewAction(FilterKeywordEnum act) {
  FActionHandle fa = (FActionHandle)calloc(1, sizeof(void *));
  if (!fa) return NULL;
  *fa = (FAction *)calloc(1, sizeof(FAction));
  if (!*fa) { free(fa); return NULL; }
  (*fa)->action = act;
  (*fa)->next = NULL;
  return fa;
}

/* AppendFilter — add a filter to the global array */
static int AppendFilter(FilterRecord *fr) {
  StudyFilter(fr);

  /* Fill out actions to MAX_ACTIONS */
  int na = 0;
  for (FActionHandle fa = fr->actions; fa; fa = (*fa)->next) na++;
  while (na < MAX_ACTIONS) {
    FActionHandle fa = NewAction(flkNone);
    if (!fa) break;
    AppendAction(&fr->actions, fa);
    na++;
  }

  gNFilters++;
  gFilterArray = realloc(gFilterArray, gNFilters * sizeof(FilterRecord));
  if (!gFilterArray) { gNFilters = 0; return -1; }
  gFilterArray[gNFilters - 1] = *fr;
  return 0;
}

/* Get the filters file path */
static const char *GetFiltersPath(void) {
  static char path[1024] = {0};
  if (!path[0]) {
    const char *home = g_get_home_dir();
    snprintf(path, sizeof(path), "%s/.local/share/geudora/Filters", home);
  }
  return path;
}

/* ReadFilters — read filter database from text file */
int ReadFilters(void) {
  const char *path = GetFiltersPath();
  FILE *fp = fopen(path, "r");
  if (!fp) return 0; /* no file = no filters, not an error */

  FilterRecord fr;
  FRInit(&fr);
  int term = 0;
  char line[1024];

  while (fgets(line, sizeof(line), fp)) {
    int len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
      line[--len] = 0;
    if (len == 0 || line[0] == '#') continue;

    char *space = strchr(line, ' ');
    char keyword[64] = {0};
    char value[960] = {0};
    if (space) {
      int kwlen = space - line;
      if (kwlen > 63) kwlen = 63;
      memcpy(keyword, line, kwlen);
      keyword[kwlen] = 0;
      sstrncpy(value, space + 1, sizeof(value));
    } else {
      sstrncpy(keyword, line, sizeof(keyword));
    }

    FilterKeywordEnum key = LookupKeyword(keyword);
    if (key == flkZero) continue;

    if (key == flkRaise) { key = flkPriority; strcpy(value, "7"); }
    if (key == flkLower) { key = flkPriority; strcpy(value, "8"); }

    /* Check if it's an action keyword */
    FActionProc *fap = (FActionProc *)FATable(key);
    if (fap) {
      FActionHandle fa = NewAction(key);
      if (fa) {
        /* Pass the value as a plain C string to faeRead */
        (*fap)(faeRead, fa, NULL, value[0] ? value : NULL);
        AppendAction(&fr.actions, fa);
      }
      continue;
    }

    /* Non-action keywords */
    switch (key) {
    case flkRule:
      if (fr.name[0]) AppendFilter(&fr);
      FRInit(&fr);
      term = 0;
      sstrncpy(fr.name, value, sizeof(fr.name));
      break;
    case flkIncoming:
      fr.incoming = true;
      break;
    case flkOutgoing:
      fr.outgoing = true;
      break;
    case flkManual:
      fr.manual = true;
      break;
    case flkId:
      fr.fu.id = atol(value);
      if (fr.fu.id >= gNextFilterId) gNextFilterId = fr.fu.id + 1;
      break;
    case flkHeader:
      sstrncpy(fr.terms[term].header, value, sizeof(fr.terms[term].header));
      break;
    case flkVerb:
      fr.terms[term].verb = LookupVerb(value);
      break;
    case flkValue:
      sstrncpy(fr.terms[term].value, value, sizeof(fr.terms[term].value));
      break;
    case flkConjunction:
      fr.conjunction = LookupConj(value);
      if (term == 0) term = 1;
      break;
    case flkCopyInstead: {
      FActionHandle last = NULL;
      for (FActionHandle a = fr.actions; a; a = (*a)->next)
        if ((*a)->action == flkTransfer) last = a;
      if (last) (*last)->action = flkCopy;
      break;
    }
    default:
      break;
    }
  }
  if (fr.name[0]) AppendFilter(&fr);

  fclose(fp);
  g_print("ReadFilters: loaded %d filters from %s\n", gNFilters, path);
  return 0;
}

/* FWriteStr — write "keyword value\n" to file (C string value) */
static int FWriteStr(FILE *fp, FilterKeywordEnum flk, const char *val) {
  if (!val || !val[0]) return 0;
  if ((int)flk >= (int)NUM_FILT_KEYWORDS) return -1;
  fprintf(fp, "%s %s\n", FiltKeywords[flk], val);
  return 0;
}

/* FWriteBool — write keyword line only if value is true */
short FWriteBool(short refN, FilterKeywordEnum flk, bool value) {
  (void)refN;
  return 0;
}

static int FWriteBoolFP(FILE *fp, FilterKeywordEnum flk, bool value) {
  if (!value) return 0;
  if ((int)flk >= (int)NUM_FILT_KEYWORDS) return -1;
  fprintf(fp, "%s\n", FiltKeywords[flk]);
  return 0;
}

static int FWriteEnum(FILE *fp, FilterKeywordEnum flk, short e) {
  if ((int)flk >= (int)NUM_FILT_KEYWORDS) return -1;
  const char *evalStr = "";
  if (flk == flkVerb && e > 0 && e < (int)NUM_VERB_STRINGS)
    evalStr = VerbStrings[e];
  else if (flk == flkConjunction && e > 0 && e < (int)NUM_CONJ_STRINGS)
    evalStr = ConjStrings[e];
  fprintf(fp, "%s %s\n", FiltKeywords[flk], evalStr);
  return 0;
}

/* WriteFilter — write a single filter to file */
static int WriteFilter(FILE *fp, FilterRecord *fr) {
  FWriteStr(fp, flkRule, fr->name);
  if (!fr->fu.id) fr->fu.id = FilterNewId();
  {
    char idstr[32];
    snprintf(idstr, sizeof(idstr), "%ld", fr->fu.id);
    FWriteStr(fp, flkId, idstr);
  }
  FWriteBoolFP(fp, flkIncoming, fr->incoming);
  FWriteBoolFP(fp, flkOutgoing, fr->outgoing);
  FWriteBoolFP(fp, flkManual, fr->manual);
  FWriteStr(fp, flkHeader, fr->terms[0].header);
  FWriteEnum(fp, flkVerb, fr->terms[0].verb);
  FWriteStr(fp, flkValue, fr->terms[0].value);
  if (fr->conjunction && fr->conjunction != cjIgnore) {
    FWriteEnum(fp, flkConjunction, fr->conjunction);
    FWriteStr(fp, flkHeader, fr->terms[1].header);
    FWriteEnum(fp, flkVerb, fr->terms[1].verb);
    FWriteStr(fp, flkValue, fr->terms[1].value);
  }
  for (FActionHandle fa = fr->actions; fa; fa = (*fa)->next) {
    FilterKeywordEnum act = (*fa)->action;
    if (act == flkNone || act == flkZero) continue;
    if ((int)act < (int)NUM_FILT_KEYWORDS && FiltKeywords[act][0])
      fprintf(fp, "%s\n", FiltKeywords[act]);
  }
  return 0;
}

/* SaveFilters — save all filters to disk */
int SaveFilters(void) {
  const char *path = GetFiltersPath();

  char dir[1024];
  snprintf(dir, sizeof(dir), "%s/.local/share/geudora", g_get_home_dir());
  g_mkdir_with_parents(dir, 0755);

  char tmppath[1080];
  snprintf(tmppath, sizeof(tmppath), "%s~", path);
  FILE *fp = fopen(tmppath, "w");
  if (!fp) {
    g_print("SaveFilters: cannot open %s: %s\n", tmppath, strerror(errno));
    return -1;
  }

  for (int i = 0; i < gNFilters; i++)
    WriteFilter(fp, &gFilterArray[i]);

  fclose(fp);
  rename(tmppath, path);
  g_print("SaveFilters: saved %d filters to %s\n", gNFilters, path);
  return 0;
}

/* RegenerateFilters — load filters from disk if not already loaded */
bool RegenerateFilters(void) {
  if (gNFilters > 0) {
    FiltersRefCount++;
    return false;
  }
  int err = ReadFilters();
  if (!err) FiltersRefCount++;
  return (err != 0);
}

/************************************************************************
 * FAflk* - Filter Action Functions
 * Each handles multiple callTypes:
 *   faeDo    — execute the action on a message
 *   faeInit  — create GTK widgets for editing the action
 *   faeRead  — load action data from file (dataPtr is C string or NULL)
 *   faeWrite — save action data to file
 *   faeSave  — copy widget state to action data
 *   faeClose — destroy GTK widgets
 ************************************************************************/

/*---------- FAflkNone ----------*/
short FAflkNone(FACallEnum callType, FActionHandle action, Rect *r,
                void *dataPtr) {
  (void)callType; (void)action; (void)r; (void)dataPtr;
  return 0;
}

/*---------- FAflkStop ----------*/
short FAflkStop(FACallEnum callType, FActionHandle action, Rect *r,
                void *dataPtr) {
  (void)action; (void)r; (void)dataPtr;
  if (callType == faeDo) return euFilterStop;
  return 0;
}

/*---------- FAflkJunk ----------*/
short FAflkJunk(FACallEnum callType, FActionHandle action, Rect *r,
                void *dataPtr) {
  (void)action; (void)r;
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeDo:
    if (HasFeature(featureJunk)) {
      TOCType *junkTOC = NULL;
      if (fpb->tocH->imapTOC)
        junkTOC = LocateIMAPJunkToc(fpb->tocH, true, true);
      else
        junkTOC = GetSpecialTOC(MBX_JUNK);

      if (junkTOC) {
        int err;
        UseFeature(featureJunk);
        err = Junk(fpb->tocH, fpb->sumNum, true, false);
        if (!err && (fpb->tocH != junkTOC)) {
          fpb->spec = GetMailboxSpec(junkTOC, -1);
          fpb->xferred = true;
          fpb->dontUser = true;
          return euFilterXfered;
        }
        if (err) return euFilterStop;
      }
    }
    return 0;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkPrint ----------*/
short FAflkPrint(FACallEnum callType, FActionHandle action, Rect *r,
                 void *dataPtr) {
  (void)action; (void)r;
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  if (callType == faeDo) {
    if (HasFeature(featureFilterPrint)) {
      UseFeature(featureFilterPrint);
      if (fpb->tocH->imapTOC) {
        bool filtering = IMAPFilteringUnderway();
        if (filtering) IMAPStopFiltering(false);
        EnsureMsgDownloaded(fpb->tocH, fpb->sumNum, false);
        if (filtering) IMAPStartFiltering(fpb->tocH, true);
      }
      fpb->print = true;
    }
  }
  return 0;
}

/*---------- FAflkTransfer / FAflkCopy ----------*/
typedef struct {
  FSSpec spec;
  bool brandNew;
  GtkWidget *button;
} FDTransfer;

short FAflkTransfer(FACallEnum callType, FActionHandle action, Rect *r,
                    void *dataPtr) {
  (void)r;
  int err = 0;
  FDTransfer *data = (FDTransfer *)((*action)->data
                                      ? *((*action)->data)
                                      : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;
  bool copy = ((*action)->action == flkCopy);

  switch (callType) {
  case faeRead: {
    FDTransfer *nd = calloc(1, sizeof(FDTransfer));
    if (!nd) return -1;
    if (dataPtr) {
      const char *path = (const char *)dataPtr;
      sstrncpy(nd->spec.name, path, sizeof(nd->spec.name));
      sstrncpy(nd->spec.path, path, sizeof(nd->spec.path));
    } else {
      nd->brandNew = true;
    }
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDTransfer **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *label = data->spec.name[0] ? data->spec.name : "(choose mailbox)";
      data->button = gtk_button_new_with_label(label);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->button = NULL;
    break;
  case faeSave:
    break;
  case faeDo:
    if (!data || !data->spec.name[0]) return 0;
    {
      FSSpec curSpec = GetMailboxSpec(fpb->tocH, -1);
      if (SameSpec(&curSpec, &data->spec)) return euFilterStop;

      if (!copy && fpb->openMessage)
        fpb->tocH->sums[fpb->sumNum].opts |= OPT_OPEN;

      if (!copy && fpb->print) {
        short oldstat = fpb->tocH->sums[fpb->sumNum].state;
        if (fpb->tocH->imapTOC) {
          bool filtering = IMAPFilteringUnderway();
          if (filtering) IMAPStopFiltering(false);
          EnsureMsgDownloaded(fpb->tocH, fpb->sumNum, false);
          if (filtering) IMAPStartFiltering(fpb->tocH, true);
        }
        PrintClosedMessage(fpb->tocH, fpb->sumNum, true);
        SetState(fpb->tocH, fpb->sumNum, oldstat);
      }

      fpb->tocH->sums[fpb->sumNum].flags |= FLAG_SKIPWARN;

      TOCType *toTocH = TOCBySpec(&data->spec);
      if (fpb->tocH->imapTOC || (toTocH && toTocH->imapTOC)) {
        bool filtering = IMAPFilteringUnderway();
        if (filtering) IMAPStopFiltering(false);
        /* IMAP transfer */
        if (filtering) IMAPStartFiltering(fpb->tocH, true);
      }

      if (!copy) {
        fpb->xferred = true;
        if (fpb->tocH->imapTOC) fpb->xferredFromIMAP = true;
        fpb->spec = data->spec;
        err = euFilterXfered;
      }
    }
    if (err && err != euFilterXfered) err = euFilterStop;
    break;
  default:
    break;
  }
  return err;
}

short FAflkCopy(FACallEnum callType, FActionHandle action, Rect *r,
                void *dataPtr) {
  return FAflkTransfer(callType, action, r, dataPtr);
}

/*---------- FAflkMoveAttach ----------*/
typedef struct {
  FSSpec spec;
  GtkWidget *button;
} FDMoveAttach;

short FAflkMoveAttach(FACallEnum callType, FActionHandle action, Rect *r,
                      void *dataPtr) {
  (void)r;
  FDMoveAttach *data = (FDMoveAttach *)((*action)->data
                                          ? *((*action)->data)
                                          : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDMoveAttach *nd = calloc(1, sizeof(FDMoveAttach));
    if (!nd) return -1;
    if (dataPtr) {
      const char *path = (const char *)dataPtr;
      sstrncpy(nd->spec.name, path, sizeof(nd->spec.name));
    }
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDMoveAttach **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *label = data->spec.name[0] ? data->spec.name : "(choose folder)";
      data->button = gtk_button_new_with_label(label);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->button = NULL;
    break;
  case faeDo:
    if (!data) break;
    if (fpb->tocH->sums[fpb->sumNum].flags & FLAG_HAS_ATT) {
      if (fpb->tocH->imapTOC) {
        bool filtering = IMAPFilteringUnderway();
        if (filtering) IMAPStopFiltering(false);
        EnsureMsgDownloaded(fpb->tocH, fpb->sumNum, false);
        if (FetchAllIMAPAttachments(fpb->tocH, fpb->sumNum, true)) {
          g_print("FAflkMoveAttach: move attachments for sum=%d\n", fpb->sumNum);
        }
        if (filtering) IMAPStartFiltering(fpb->tocH, true);
      } else {
        g_print("FAflkMoveAttach: move attachments for sum=%d\n", fpb->sumNum);
      }
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkStatus ----------*/
typedef struct {
  short status;
  GtkWidget *dropdown;
} FDStatus;

short FAflkStatus(FACallEnum callType, FActionHandle action, Rect *r,
                  void *dataPtr) {
  (void)r;
  FDStatus *data = (FDStatus *)((*action)->data
                                  ? *((*action)->data)
                                  : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDStatus *nd = calloc(1, sizeof(FDStatus));
    if (!nd) return -1;
    if (dataPtr) nd->status = atoi((const char *)dataPtr);
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDStatus **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *states[] = {"Unread", "Read", "Replied", "Forwarded",
                              "Redirected", "Sent", "Queued", "Timed", NULL};
      data->dropdown = gtk_drop_down_new_from_strings(states);
      if (data->status >= 0 && data->status < 8)
        gtk_drop_down_set_selected(GTK_DROP_DOWN(data->dropdown), data->status);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->dropdown = NULL;
    break;
  case faeSave:
    if (data && data->dropdown)
      data->status = gtk_drop_down_get_selected(GTK_DROP_DOWN(data->dropdown));
    break;
  case faeDo:
    if (data) SetState(fpb->tocH, fpb->sumNum, data->status);
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkSubject ----------*/
typedef struct {
  char subject[256];
  GtkWidget *entry;
} FDSubject;

short FAflkSubject(FACallEnum callType, FActionHandle action, Rect *r,
                   void *dataPtr) {
  (void)r;
  FDSubject *data = (FDSubject *)((*action)->data
                                    ? *((*action)->data)
                                    : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDSubject *nd = calloc(1, sizeof(FDSubject));
    if (!nd) return -1;
    if (dataPtr) sstrncpy(nd->subject, (const char *)dataPtr, sizeof(nd->subject));
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDSubject **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      data->entry = gtk_entry_new();
      gtk_editable_set_text(GTK_EDITABLE(data->entry), data->subject);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->entry = NULL;
    break;
  case faeSave:
    if (data && data->entry) {
      const char *text = gtk_editable_get_text(GTK_EDITABLE(data->entry));
      sstrncpy(data->subject, text, sizeof(data->subject));
    }
    break;
  case faeDo:
    if (data && data->subject[0]) {
      /* NonSequitur expects Pascal string — convert at boundary */
      unsigned char ps[256];
      c_to_pascal(ps, data->subject);
      NonSequitur(ps, fpb->tocH, fpb->sumNum);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkLabel ----------*/
typedef struct {
  long color;
  GtkWidget *dropdown;
} FDLabel;

short FAflkLabel(FACallEnum callType, FActionHandle action, Rect *r,
                 void *dataPtr) {
  (void)r;
  FDLabel *data = (FDLabel *)((*action)->data
                                ? *((*action)->data)
                                : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDLabel *nd = calloc(1, sizeof(FDLabel));
    if (!nd) return -1;
    if (dataPtr) nd->color = atol((const char *)dataPtr);
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDLabel **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *labels[] = {"None", "Label 1", "Label 2", "Label 3",
                              "Label 4", "Label 5", "Label 6", "Label 7", NULL};
      data->dropdown = gtk_drop_down_new_from_strings(labels);
      if (data->color >= 0 && data->color < 8)
        gtk_drop_down_set_selected(GTK_DROP_DOWN(data->dropdown), data->color);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->dropdown = NULL;
    break;
  case faeSave:
    if (data && data->dropdown)
      data->color = gtk_drop_down_get_selected(GTK_DROP_DOWN(data->dropdown));
    break;
  case faeDo:
    if (HasFeature(featureFilterLabel) && data) {
      UseFeature(featureFilterLabel);
      SetSumColor(fpb->tocH, fpb->sumNum, (short)data->color);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkPersonality ----------*/
typedef struct {
  long persId;
  GtkWidget *dropdown;
} FDPers;

short FAflkPersonality(FACallEnum callType, FActionHandle action, Rect *r,
                       void *dataPtr) {
  (void)r;
  FDPers *data = (FDPers *)((*action)->data
                               ? *((*action)->data)
                               : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDPers *nd = calloc(1, sizeof(FDPers));
    if (!nd) return -1;
    nd->persId = 0;
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDPers **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *pers[] = {"Dominant", NULL};
      data->dropdown = gtk_drop_down_new_from_strings(pers);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->dropdown = NULL;
    break;
  case faeDo:
    if (HasFeature(featureMultiplePersonalities) && data) {
      PersHandle pers = FindPersById(data->persId);
      if (pers && pers != PersList)
        UseFeature(featureFilterPersonality);
      if (pers)
        SetPers(fpb->tocH, fpb->sumNum, pers, false);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkSound ----------*/
typedef struct {
  char name[256];
  GtkWidget *entry;
} FDSound;

short FAflkSound(FACallEnum callType, FActionHandle action, Rect *r,
                 void *dataPtr) {
  (void)r;
  FDSound *data = (FDSound *)((*action)->data
                                ? *((*action)->data)
                                : NULL);

  switch (callType) {
  case faeRead: {
    FDSound *nd = calloc(1, sizeof(FDSound));
    if (!nd) return -1;
    if (dataPtr) sstrncpy(nd->name, (const char *)dataPtr, sizeof(nd->name));
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDSound **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      data->entry = gtk_entry_new();
      gtk_editable_set_text(GTK_EDITABLE(data->entry), data->name);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->entry = NULL;
    break;
  case faeSave:
    if (data && data->entry) {
      const char *text = gtk_editable_get_text(GTK_EDITABLE(data->entry));
      sstrncpy(data->name, text, sizeof(data->name));
    }
    break;
  case faeDo:
    if (HasFeature(featureFilterSound) && data && data->name[0]) {
      UseFeature(featureFilterSound);
      /* PlayNamedSound expects Pascal string — convert at boundary */
      unsigned char ps[256];
      c_to_pascal(ps, data->name);
      PlayNamedSound(ps);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkOpenMessage ----------*/
typedef struct {
  long flags;
  GtkWidget *chk_mailbox;
  GtkWidget *chk_message;
} FDOpen;

short FAflkOpenMessage(FACallEnum callType, FActionHandle action, Rect *r,
                       void *dataPtr) {
  (void)r;
  FDOpen *data = (FDOpen *)((*action)->data
                              ? *((*action)->data)
                              : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDOpen *nd = calloc(1, sizeof(FDOpen));
    if (!nd) return -1;
    if (dataPtr) nd->flags = atol((const char *)dataPtr);
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDOpen **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      data->chk_mailbox = gtk_check_button_new_with_label("Open Mailbox");
      data->chk_message = gtk_check_button_new_with_label("Open Message");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(data->chk_mailbox),
                                  0 != (data->flags & afbOpenMailbox));
      gtk_check_button_set_active(GTK_CHECK_BUTTON(data->chk_message),
                                  0 != (data->flags & afbOpenMessage));
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) { data->chk_mailbox = NULL; data->chk_message = NULL; }
    break;
  case faeSave:
    if (data) {
      data->flags = 0;
      if (data->chk_mailbox && gtk_check_button_get_active(GTK_CHECK_BUTTON(data->chk_mailbox)))
        data->flags |= afbOpenMailbox;
      if (data->chk_message && gtk_check_button_get_active(GTK_CHECK_BUTTON(data->chk_message)))
        data->flags |= afbOpenMessage;
    }
    break;
  case faeDo:
    if (HasFeature(featureFilterOpen) && data) {
      UseFeature(featureFilterOpen);
      fpb->openMailbox = 0 != (data->flags & afbOpenMailbox);
      fpb->openMessage = 0 != (data->flags & afbOpenMessage);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkPriority ----------*/
typedef struct {
  long prior;
  GtkWidget *dropdown;
} FDPrior;

short FAflkPriority(FACallEnum callType, FActionHandle action, Rect *r,
                    void *dataPtr) {
  (void)r;
  FDPrior *data = (FDPrior *)((*action)->data
                                ? *((*action)->data)
                                : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDPrior *nd = calloc(1, sizeof(FDPrior));
    if (!nd) return -1;
    if (dataPtr) nd->prior = atol((const char *)dataPtr);
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDPrior **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *pris[] = {"Highest", "High", "Normal", "Low", "Lowest",
                            "Raise", "Lower", NULL};
      data->dropdown = gtk_drop_down_new_from_strings(pris);
      int idx = 2;
      if (data->prior >= 1 && data->prior <= 5) idx = data->prior - 1;
      else if (data->prior == 7) idx = 5;
      else if (data->prior == 8) idx = 6;
      gtk_drop_down_set_selected(GTK_DROP_DOWN(data->dropdown), idx);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->dropdown = NULL;
    break;
  case faeSave:
    if (data && data->dropdown) {
      int idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(data->dropdown));
      if (idx >= 0 && idx <= 4) data->prior = idx + 1;
      else if (idx == 5) data->prior = 7;
      else if (idx == 6) data->prior = 8;
    }
    break;
  case faeDo:
    if (data) {
      short newPrior = NewPrior((short)data->prior,
                                fpb->tocH->sums[fpb->sumNum].priority);
      SetPriority(fpb->tocH, fpb->sumNum, newPrior);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkForward ----------*/
typedef struct {
  char addresses[256];
  GtkWidget *entry;
} FDForward;

short FAflkForward(FACallEnum callType, FActionHandle action, Rect *r,
                   void *dataPtr) {
  (void)r;
  FDForward *data = (FDForward *)((*action)->data
                                    ? *((*action)->data)
                                    : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDForward *nd = calloc(1, sizeof(FDForward));
    if (!nd) return -1;
    if (dataPtr) sstrncpy(nd->addresses, (const char *)dataPtr, sizeof(nd->addresses));
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDForward **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      data->entry = gtk_entry_new();
      gtk_editable_set_text(GTK_EDITABLE(data->entry), data->addresses);
      gtk_entry_set_placeholder_text(GTK_ENTRY(data->entry), "email@example.com");
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->entry = NULL;
    break;
  case faeSave:
    if (data && data->entry) {
      const char *text = gtk_editable_get_text(GTK_EDITABLE(data->entry));
      sstrncpy(data->addresses, text, sizeof(data->addresses));
    }
    break;
  case faeDo:
    if (HasFeature(featureFilterForward) && data) {
      if ((fpb->tocH->sums[fpb->sumNum].opts & OPT_BULK) &&
          !PrefIsSet(PREF_BOMBS_AWAY))
        return 0;
      /* DoFordirectMessage expects Pascal string — convert at boundary */
      unsigned char ps[256];
      c_to_pascal(ps, data->addresses);
      DoFordirectMessage(fpb->tocH, fpb->sumNum, (*action)->action, ps, true);
      UseFeature(featureFilterForward);
    }
    break;
  default:
    break;
  }
  return 0;
}

short FAflkRedirect(FACallEnum callType, FActionHandle action, Rect *r,
                    void *dataPtr) {
  return FAflkForward(callType, action, r, dataPtr);
}

/*---------- FAflkReply ----------*/
typedef struct {
  short templateIdx;
  GtkWidget *dropdown;
} FDReply;

short FAflkReply(FACallEnum callType, FActionHandle action, Rect *r,
                 void *dataPtr) {
  (void)r;
  FDReply *data = (FDReply *)((*action)->data
                                ? *((*action)->data)
                                : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDReply *nd = calloc(1, sizeof(FDReply));
    if (!nd) return -1;
    nd->templateIdx = 0;
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDReply **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      const char *tmpls[] = {"(no template)", NULL};
      data->dropdown = gtk_drop_down_new_from_strings(tmpls);
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) data->dropdown = NULL;
    break;
  case faeDo:
    if (HasFeature(featureFilterReply)) {
      if ((fpb->tocH->sums[fpb->sumNum].opts & OPT_BULK) &&
          !PrefIsSet(PREF_BOMBS_AWAY))
        return 0;
      UseFeature(featureFilterReply);
      DoReplyClosed(fpb->tocH, fpb->sumNum, false, false, false, true, 0,
                    true, true);
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkNotifyUser ----------*/
typedef struct {
  long flags;
  GtkWidget *chk_user;
  GtkWidget *chk_report;
} FDNotify;

short FAflkNotifyUser(FACallEnum callType, FActionHandle action, Rect *r,
                      void *dataPtr) {
  (void)r;
  FDNotify *data = (FDNotify *)((*action)->data
                                  ? *((*action)->data)
                                  : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDNotify *nd = calloc(1, sizeof(FDNotify));
    if (!nd) return -1;
    if (dataPtr) nd->flags = atol((const char *)dataPtr);
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDNotify **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      data->chk_user = gtk_check_button_new_with_label("Open Filters Report");
      data->chk_report = gtk_check_button_new_with_label("Generate Filter Report");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(data->chk_user),
                                  0 != (data->flags & afbUser));
      gtk_check_button_set_active(GTK_CHECK_BUTTON(data->chk_report),
                                  0 != (data->flags & afbReport));
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) { data->chk_user = NULL; data->chk_report = NULL; }
    break;
  case faeSave:
    if (data) {
      data->flags = 0;
      if (data->chk_user && gtk_check_button_get_active(GTK_CHECK_BUTTON(data->chk_user)))
        data->flags |= afbUser;
      if (data->chk_report && gtk_check_button_get_active(GTK_CHECK_BUTTON(data->chk_report)))
        data->flags |= afbReport;
    }
    break;
  case faeDo:
    if (data) {
      if (0 == (data->flags & afbUser))
        fpb->dontUser = true;
      fpb->doReport = 0 != (data->flags & afbReport);
      fpb->dontReport = !fpb->doReport;
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkServerOpts ----------*/
typedef struct {
  long flags;
  GtkWidget *chk_fetch;
  GtkWidget *chk_trash;
} FDSOpt;

short FAflkServerOpts(FACallEnum callType, FActionHandle action, Rect *r,
                      void *dataPtr) {
  (void)r;
  FDSOpt *data = (FDSOpt *)((*action)->data
                               ? *((*action)->data)
                               : NULL);
  FilterPBPtr fpb = (FilterPBPtr)dataPtr;

  switch (callType) {
  case faeRead: {
    FDSOpt *nd = calloc(1, sizeof(FDSOpt));
    if (!nd) return -1;
    if (dataPtr) nd->flags = atol((const char *)dataPtr);
    (*action)->data = calloc(1, sizeof(void *));
    if ((*action)->data) *(FDSOpt **)(*action)->data = nd;
    break;
  }
  case faeInit:
    if (data) {
      data->chk_fetch = gtk_check_button_new_with_label("Fetch Message");
      data->chk_trash = gtk_check_button_new_with_label("Delete from Server");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(data->chk_fetch),
                                  0 != (data->flags & afbFetch));
      gtk_check_button_set_active(GTK_CHECK_BUTTON(data->chk_trash),
                                  0 != (data->flags & afbTrash));
    }
    break;
  case faeClose:
  case faeDispose:
    if (data) { data->chk_fetch = NULL; data->chk_trash = NULL; }
    break;
  case faeSave:
    if (data) {
      data->flags = 0;
      if (data->chk_fetch && gtk_check_button_get_active(GTK_CHECK_BUTTON(data->chk_fetch)))
        data->flags |= afbFetch;
      if (data->chk_trash && gtk_check_button_get_active(GTK_CHECK_BUTTON(data->chk_trash)))
        data->flags |= afbTrash;
    }
    break;
  case faeDo:
    if (HasFeature(featureFilterServerOptions) && data) {
      UseFeature(featureFilterServerOptions);
      long flag = data->flags;
      long uidHash = fpb->tocH->sums[fpb->sumNum].uidHash;
      PersHandle pers = TS_TO_PPERS(fpb->tocH, fpb->sumNum);

      if (fpb->tocH->imapTOC) {
        bool filtering = IMAPFilteringUnderway();
        if (filtering) IMAPStopFiltering(false);
        if (flag & afbFetch) {
          if (!IMAPMessageDownloaded(fpb->tocH, fpb->sumNum) &&
              !IMAPMessageBeingDownloaded(fpb->tocH, fpb->sumNum))
            UIDDownloadMessage(fpb->tocH, uidHash, true, true);
        } else if (flag & afbTrash) {
          IMAPDeleteMessageDuringFiltering(fpb->tocH, pers, uidHash);
        }
        if (filtering) IMAPStartFiltering(fpb->tocH, true);
      } else {
        if (flag & afbTrash)
          fpb->tocH->sums[fpb->sumNum].flags |= FLAG_SKIPWARN;
      }
    }
    break;
  default:
    break;
  }
  return 0;
}

/*---------- FAflkSpeak ----------*/
short FAflkSpeak(FACallEnum callType, FActionHandle action, Rect *r,
                 void *dataPtr) {
  (void)action; (void)r; (void)dataPtr;
  if (callType == faeDo)
    g_print("Filter action: Speak (not supported on this platform)\n");
  return 0;
}

/*---------- FAflkAddHistory ----------*/
short FAflkAddHistory(FACallEnum callType, FActionHandle action, Rect *r,
                      void *dataPtr) {
  (void)action; (void)r;

  if (callType == faeDo) {
    if (HasFeature(featureFilterAddHistory)) {
      FilterPBPtr fpb = (FilterPBPtr)dataPtr;
      if (fpb) {
        void **addresses = NULL;
        UseFeature(featureFilterAddHistory);
        if (!GatherBoxAddresses(fpb->tocH, 0, fpb->sumNum, fpb->sumNum,
                                &addresses, false)) {
          if (addresses)
            CacheRecentNickname(*addresses);
        }
      }
    }
  }
  return 0;
}

/************************************************************************
 * GTK4 Filter Window UI
 * Ported from Mac filtwin.c — provides the Filters window where users
 * can view, create, edit, and delete message filters.
 * Layout: toolbar | left=filter list | right=match+actions detail
 ************************************************************************/

static GtkWidget *filter_window = NULL;
static GtkWidget *filter_list_box = NULL;
static GtkWidget *detail_box = NULL;

static int gSelectedFilter = -1;
static bool gCurrentDirty = false;

/* Detail panel widgets */
static GtkWidget *chk_incoming = NULL;
static GtkWidget *chk_outgoing = NULL;
static GtkWidget *chk_manual = NULL;
static GtkWidget *header_entry1 = NULL;
static GtkWidget *verb_drop1 = NULL;
static GtkWidget *value_entry1 = NULL;
static GtkWidget *conj_drop = NULL;
static GtkWidget *header_entry2 = NULL;
static GtkWidget *verb_drop2 = NULL;
static GtkWidget *value_entry2 = NULL;

typedef struct {
  GtkWidget *type_drop;
  GtkWidget *value_box;
} ActionRow;

static ActionRow action_rows[MAX_ACTIONS];

static const char *verb_ui_strings[] = {
    "contains", "doesn't contain", "is",         "isn't",
    "starts with", "ends with",    "appears",    "doesn't appear",
    "intersects", "doesn't intersect", "intersects file", "doesn't intersect file",
    "matches regex", "junk score less than", "junk score more than", NULL};

static const char *action_ui_strings[] = {
    "None",          "Transfer To",     "Copy To",
    "Set Status",    "Set Priority",    "Set Label",
    "Set Personality", "Play Sound",    "Open Message",
    "Print",         "Forward To",      "Redirect To",
    "Reply With",    "Server Options",  "Mark as Junk",
    "Move Attachments", "Stop",         NULL};

static FilterKeywordEnum action_idx_to_fk[] = {
    flkNone, flkTransfer, flkCopy, flkStatus, flkPriority, flkLabel,
    flkPersonality, flkSound, flkOpenMessage, flkPrint, flkForward,
    flkRedirect, flkReply, flkServerOpts, flkJunk, flkMoveAttach, flkStop};
#define NUM_ACTION_TYPES (sizeof(action_idx_to_fk) / sizeof(action_idx_to_fk[0]))

static int fk_to_action_idx(FilterKeywordEnum fk) {
  for (int i = 0; i < (int)NUM_ACTION_TYPES; i++)
    if (action_idx_to_fk[i] == fk) return i;
  return 0;
}

/* Forward declarations */
static void SaveCurrentFilter(void);
static void DisplaySelectedFilter(void);
static void PopulateFilterList(void);
static void FiltersSetGreys(void);

/* Callback: filter list selection changed */
static void on_filter_selected(GtkListBox *box, GtkListBoxRow *row,
                                gpointer user_data) {
  (void)box; (void)user_data;
  SaveCurrentFilter();
  gSelectedFilter = row ? gtk_list_box_row_get_index(row) : -1;
  DisplaySelectedFilter();
  FiltersSetGreys();
  gCurrentDirty = false;
}

/* Callback: "New" button */
static void on_new_filter(GtkButton *btn, gpointer user_data) {
  (void)btn; (void)user_data;
  SaveCurrentFilter();

  FilterRecord fr;
  FRInit(&fr);
  fr.incoming = true;
  fr.fu.id = FilterNewId();
  sstrncpy(fr.name, "Untitled", sizeof(fr.name));

  for (int i = 0; i < MAX_ACTIONS; i++) {
    FActionHandle fa = NewAction(flkNone);
    if (!fa) break;
    AppendAction(&fr.actions, fa);
  }

  int n = gNFilters;
  gNFilters++;
  gFilterArray = realloc(gFilterArray, gNFilters * sizeof(FilterRecord));
  gFilterArray[n] = fr;

  PopulateFilterList();
  gSelectedFilter = n;
  gtk_list_box_select_row(GTK_LIST_BOX(filter_list_box),
    gtk_list_box_get_row_at_index(GTK_LIST_BOX(filter_list_box), n));
  DisplaySelectedFilter();
  gCurrentDirty = true;
}

/* Callback: "Remove" button */
static void on_remove_filter(GtkButton *btn, gpointer user_data) {
  (void)btn; (void)user_data;
  if (gSelectedFilter < 0 || gSelectedFilter >= gNFilters) return;

  for (int i = gSelectedFilter; i < gNFilters - 1; i++)
    gFilterArray[i] = gFilterArray[i + 1];
  gNFilters--;
  if (gNFilters == 0) { free(gFilterArray); gFilterArray = NULL; }
  else gFilterArray = realloc(gFilterArray, gNFilters * sizeof(FilterRecord));

  if (gSelectedFilter >= gNFilters) gSelectedFilter = gNFilters - 1;

  PopulateFilterList();
  if (gSelectedFilter >= 0) {
    gtk_list_box_select_row(GTK_LIST_BOX(filter_list_box),
      gtk_list_box_get_row_at_index(GTK_LIST_BOX(filter_list_box), gSelectedFilter));
  }
  DisplaySelectedFilter();
}

/* Callback: "Duplicate" button */
static void on_dup_filter(GtkButton *btn, gpointer user_data) {
  (void)btn; (void)user_data;
  if (gSelectedFilter < 0 || gSelectedFilter >= gNFilters) return;
  SaveCurrentFilter();

  FilterRecord orig = gFilterArray[gSelectedFilter];
  FilterRecord dup;
  memcpy(&dup, &orig, sizeof(dup));
  dup.fu.id = FilterNewId();

  /* Deep copy actions */
  dup.actions = NULL;
  for (FActionHandle fa = orig.actions; fa; fa = (*fa)->next) {
    FActionHandle newfa = NewAction((*fa)->action);
    if (!newfa) break;
    AppendAction(&dup.actions, newfa);
  }

  int n = gNFilters;
  gNFilters++;
  gFilterArray = realloc(gFilterArray, gNFilters * sizeof(FilterRecord));
  gFilterArray[n] = dup;

  PopulateFilterList();
  gSelectedFilter = n;
  gtk_list_box_select_row(GTK_LIST_BOX(filter_list_box),
    gtk_list_box_get_row_at_index(GTK_LIST_BOX(filter_list_box), n));
  DisplaySelectedFilter();
}

/* Callback: "Save" button */
static void on_save_filters(GtkButton *btn, gpointer user_data) {
  (void)btn; (void)user_data;
  SaveCurrentFilter();
  SaveFilters();
}

/* Save the current filter's UI state back to the FilterRecord */
static void SaveCurrentFilter(void) {
  if (gSelectedFilter < 0 || gSelectedFilter >= gNFilters) return;
  FilterRecord *fr = &gFilterArray[gSelectedFilter];

  fr->incoming = gtk_check_button_get_active(GTK_CHECK_BUTTON(chk_incoming));
  fr->outgoing = gtk_check_button_get_active(GTK_CHECK_BUTTON(chk_outgoing));
  fr->manual = gtk_check_button_get_active(GTK_CHECK_BUTTON(chk_manual));

  /* Match term 1 */
  const char *h1 = gtk_editable_get_text(GTK_EDITABLE(header_entry1));
  sstrncpy(fr->terms[0].header, h1, sizeof(fr->terms[0].header));
  int vidx1 = gtk_drop_down_get_selected(GTK_DROP_DOWN(verb_drop1));
  fr->terms[0].verb = (vidx1 >= 0 && vidx1 < (int)mbmLimit - 1) ? vidx1 + 1 : mbmContains;
  const char *v1 = gtk_editable_get_text(GTK_EDITABLE(value_entry1));
  sstrncpy(fr->terms[0].value, v1, sizeof(fr->terms[0].value));

  /* Conjunction */
  int cidx = gtk_drop_down_get_selected(GTK_DROP_DOWN(conj_drop));
  fr->conjunction = (cidx >= 0 && cidx < (int)cjLimit - 1) ? cidx + 1 : cjIgnore;

  /* Match term 2 */
  const char *h2 = gtk_editable_get_text(GTK_EDITABLE(header_entry2));
  sstrncpy(fr->terms[1].header, h2, sizeof(fr->terms[1].header));
  int vidx2 = gtk_drop_down_get_selected(GTK_DROP_DOWN(verb_drop2));
  fr->terms[1].verb = (vidx2 >= 0 && vidx2 < (int)mbmLimit - 1) ? vidx2 + 1 : mbmContains;
  const char *v2 = gtk_editable_get_text(GTK_EDITABLE(value_entry2));
  sstrncpy(fr->terms[1].value, v2, sizeof(fr->terms[1].value));

  /* Update actions from dropdowns */
  FActionHandle fa = fr->actions;
  for (int i = 0; i < MAX_ACTIONS && fa; i++, fa = (*fa)->next) {
    int aidx = gtk_drop_down_get_selected(GTK_DROP_DOWN(action_rows[i].type_drop));
    if (aidx >= 0 && aidx < (int)NUM_ACTION_TYPES)
      (*fa)->action = action_idx_to_fk[aidx];
  }

  /* Build filter name from first term */
  if (fr->terms[0].header[0] || fr->terms[0].value[0])
    snprintf(fr->name, sizeof(fr->name), "%s: %s",
             fr->terms[0].header, fr->terms[0].value);
  else
    sstrncpy(fr->name, "Untitled", sizeof(fr->name));

  gCurrentDirty = false;
}

/* Display the selected filter in the detail panel */
static void DisplaySelectedFilter(void) {
  FilterRecord fr;
  if (gSelectedFilter >= 0 && gSelectedFilter < gNFilters)
    fr = gFilterArray[gSelectedFilter];
  else
    FRInit(&fr);

  gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_incoming), fr.incoming);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_outgoing), fr.outgoing);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_manual), fr.manual);

  /* Match term 1 */
  gtk_editable_set_text(GTK_EDITABLE(header_entry1), fr.terms[0].header);
  int vidx = (fr.terms[0].verb >= mbmContains && fr.terms[0].verb < mbmLimit)
                 ? fr.terms[0].verb - 1 : 0;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(verb_drop1), vidx);
  gtk_editable_set_text(GTK_EDITABLE(value_entry1), fr.terms[0].value);

  /* Conjunction */
  int cidx = (fr.conjunction >= cjIgnore && fr.conjunction < cjLimit)
                 ? fr.conjunction - 1 : 0;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(conj_drop), cidx);

  /* Match term 2 */
  gtk_editable_set_text(GTK_EDITABLE(header_entry2), fr.terms[1].header);
  vidx = (fr.terms[1].verb >= mbmContains && fr.terms[1].verb < mbmLimit)
             ? fr.terms[1].verb - 1 : 0;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(verb_drop2), vidx);
  gtk_editable_set_text(GTK_EDITABLE(value_entry2), fr.terms[1].value);

  /* Actions */
  FActionHandle fa = fr.actions;
  for (int i = 0; i < MAX_ACTIONS; i++) {
    FilterKeywordEnum act = flkNone;
    if (fa) {
      act = (*fa)->action;
      fa = (*fa)->next;
    }
    gtk_drop_down_set_selected(GTK_DROP_DOWN(action_rows[i].type_drop),
                               fk_to_action_idx(act));
  }

  /* Enable/disable second match row based on conjunction */
  bool hasTwo = (fr.conjunction != cjIgnore);
  gtk_widget_set_sensitive(header_entry2, hasTwo);
  gtk_widget_set_sensitive(verb_drop2, hasTwo);
  gtk_widget_set_sensitive(value_entry2, hasTwo);
}

/* Populate the filter list from gFilterArray */
static void PopulateFilterList(void) {
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(filter_list_box)) != NULL)
    gtk_list_box_remove(GTK_LIST_BOX(filter_list_box), child);

  for (int i = 0; i < gNFilters; i++) {
    GtkWidget *label = gtk_label_new(gFilterArray[i].name);
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_list_box_append(GTK_LIST_BOX(filter_list_box), label);
  }
}

/* Enable/disable controls based on selection state */
static void FiltersSetGreys(void) {
  bool hasSel = (gSelectedFilter >= 0 && gSelectedFilter < gNFilters);
  gtk_widget_set_sensitive(chk_incoming, hasSel);
  gtk_widget_set_sensitive(chk_outgoing, hasSel);
  gtk_widget_set_sensitive(chk_manual, hasSel);
  gtk_widget_set_sensitive(header_entry1, hasSel);
  gtk_widget_set_sensitive(verb_drop1, hasSel);
  gtk_widget_set_sensitive(value_entry1, hasSel);
  gtk_widget_set_sensitive(conj_drop, hasSel);
  for (int i = 0; i < MAX_ACTIONS; i++)
    gtk_widget_set_sensitive(action_rows[i].type_drop, hasSel);
}

/* Build one match-term row: [header entry] [verb dropdown] [value entry] */
static GtkWidget *build_match_row(GtkWidget **out_header, GtkWidget **out_verb,
                                  GtkWidget **out_value) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  *out_header = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(*out_header), "Header");
  gtk_widget_set_size_request(*out_header, 100, -1);
  gtk_box_append(GTK_BOX(row), *out_header);

  *out_verb = gtk_drop_down_new_from_strings(verb_ui_strings);
  gtk_box_append(GTK_BOX(row), *out_verb);

  *out_value = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(*out_value), "Value");
  gtk_widget_set_hexpand(*out_value, TRUE);
  gtk_box_append(GTK_BOX(row), *out_value);

  return row;
}

/************************************************************************
 * OpenFiltersWindow — open/create the Filters window
 ************************************************************************/
void OpenFiltersWindow(GtkWindow *parent) {
  if (filter_window) {
    gtk_window_present(GTK_WINDOW(filter_window));
    return;
  }

  RegenerateFilters();

  filter_window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(filter_window), "Filters");
  gtk_window_set_default_size(GTK_WINDOW(filter_window), 800, 550);
  if (parent)
    gtk_window_set_transient_for(GTK_WINDOW(filter_window), parent);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child(GTK_WINDOW(filter_window), vbox);

  /* ===== Toolbar ===== */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(toolbar, 8);
  gtk_widget_set_margin_end(toolbar, 8);
  gtk_widget_set_margin_top(toolbar, 4);
  gtk_widget_set_margin_bottom(toolbar, 4);

  GtkWidget *new_btn = gtk_button_new_with_label("New");
  GtkWidget *remove_btn = gtk_button_new_with_label("Remove");
  GtkWidget *dup_btn = gtk_button_new_with_label("Duplicate");
  GtkWidget *save_btn = gtk_button_new_with_label("Save");
  g_signal_connect(new_btn, "clicked", G_CALLBACK(on_new_filter), NULL);
  g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_remove_filter), NULL);
  g_signal_connect(dup_btn, "clicked", G_CALLBACK(on_dup_filter), NULL);
  g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_filters), NULL);
  gtk_box_append(GTK_BOX(toolbar), new_btn);
  gtk_box_append(GTK_BOX(toolbar), remove_btn);
  gtk_box_append(GTK_BOX(toolbar), dup_btn);
  gtk_box_append(GTK_BOX(toolbar), save_btn);
  gtk_box_append(GTK_BOX(vbox), toolbar);

  /* ===== Paned: left = filter list, right = detail ===== */
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(paned), 220);
  gtk_widget_set_vexpand(paned, TRUE);

  /* --- Filter list (left) --- */
  GtkWidget *list_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  filter_list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(filter_list_box),
                                  GTK_SELECTION_SINGLE);
  g_signal_connect(filter_list_box, "row-selected",
                   G_CALLBACK(on_filter_selected), NULL);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll),
                                filter_list_box);
  gtk_paned_set_start_child(GTK_PANED(paned), list_scroll);
  gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
  gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);

  /* --- Detail panel (right) --- */
  GtkWidget *detail_scroll = gtk_scrolled_window_new();
  detail_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(detail_box, 12);
  gtk_widget_set_margin_end(detail_box, 12);
  gtk_widget_set_margin_top(detail_box, 8);
  gtk_widget_set_margin_bottom(detail_box, 8);

  /* -- Incoming/Outgoing/Manual checkboxes -- */
  GtkWidget *type_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  chk_incoming = gtk_check_button_new_with_label("Incoming");
  chk_outgoing = gtk_check_button_new_with_label("Outgoing");
  chk_manual = gtk_check_button_new_with_label("Manual");
  gtk_box_append(GTK_BOX(type_row), chk_incoming);
  gtk_box_append(GTK_BOX(type_row), chk_outgoing);
  gtk_box_append(GTK_BOX(type_row), chk_manual);
  gtk_box_append(GTK_BOX(detail_box), type_row);

  /* -- Match section -- */
  GtkWidget *match_frame = gtk_frame_new("Match");
  GtkWidget *match_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(match_box, 8);
  gtk_widget_set_margin_end(match_box, 8);
  gtk_widget_set_margin_top(match_box, 4);
  gtk_widget_set_margin_bottom(match_box, 8);

  GtkWidget *row1 = build_match_row(&header_entry1, &verb_drop1, &value_entry1);
  gtk_box_append(GTK_BOX(match_box), row1);

  const char *conjs[] = {"ignore", "and", "or", "unless", NULL};
  conj_drop = gtk_drop_down_new_from_strings(conjs);
  gtk_box_append(GTK_BOX(match_box), conj_drop);

  GtkWidget *row2 = build_match_row(&header_entry2, &verb_drop2, &value_entry2);
  gtk_box_append(GTK_BOX(match_box), row2);

  gtk_frame_set_child(GTK_FRAME(match_frame), match_box);
  gtk_box_append(GTK_BOX(detail_box), match_frame);

  /* -- Action section -- */
  GtkWidget *action_frame = gtk_frame_new("Actions");
  GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(action_box, 8);
  gtk_widget_set_margin_end(action_box, 8);
  gtk_widget_set_margin_top(action_box, 4);
  gtk_widget_set_margin_bottom(action_box, 8);

  for (int i = 0; i < MAX_ACTIONS; i++) {
    GtkWidget *act_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    action_rows[i].type_drop = gtk_drop_down_new_from_strings(action_ui_strings);
    action_rows[i].value_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_hexpand(action_rows[i].value_box, TRUE);
    gtk_box_append(GTK_BOX(act_row), action_rows[i].type_drop);
    gtk_box_append(GTK_BOX(act_row), action_rows[i].value_box);
    gtk_box_append(GTK_BOX(action_box), act_row);
  }

  gtk_frame_set_child(GTK_FRAME(action_frame), action_box);
  gtk_box_append(GTK_BOX(detail_box), action_frame);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(detail_scroll), detail_box);
  gtk_paned_set_end_child(GTK_PANED(paned), detail_scroll);
  gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
  gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);

  gtk_box_append(GTK_BOX(vbox), paned);

  PopulateFilterList();
  FiltersSetGreys();

  g_signal_connect_swapped(filter_window, "destroy",
                           G_CALLBACK(g_nullify_pointer), &filter_window);

  gtk_window_present(GTK_WINDOW(filter_window));
}
