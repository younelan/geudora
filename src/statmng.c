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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "statmng.h"

/* orphan brace removed */
static short InBG = 0;
#include "statwin.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "fileutil.h"
#include "graph.h"
#include "junk.h"
#include "mailbox.h" /* For Accumulator struct definition */
#include "toc.h"
#include "sendmail.h"
#include "util.h"

/* Stubs for Mac-specific platform functions not in GTK port */
static void AccuIndent(AccuPtr a) { (void)a; }
static int AccuAddTag(AccuPtr a, unsigned char *tag, int close) {
  (void)a;
  (void)tag;
  (void)close;
  return 0;
}
static void SecondsToDate(long secs, DateTimeRec *d) {
  time_t t = (time_t)secs;
  struct tm *tm = localtime(&t);
  if (tm && d) {
    d->year = tm->tm_year + 1900;
    d->month = tm->tm_mon + 1;
    d->day = tm->tm_mday;
    d->hour = tm->tm_hour;
    d->minute = tm->tm_min;
    d->second = tm->tm_sec;
    d->dayOfWeek = tm->tm_wday + 1;
  }
}

/* Stubs for Mac/XML platform functions not available in GTK port */
static void XMLIncIndent(void) {}
static void XMLDecIndent(void) {}
static int AccuAddCRLF(AccuPtr a) {
  (void)a;
  return 0;
}
/* AccuAddTagLine(accu, str, wantsSpace) -- 3-arg variant used in statmng */
static int AccuAddTagLine(AccuPtr a, unsigned char *s, int sp) {
  (void)a;
  (void)s;
  (void)sp;
  return 0;
}
/* AccuAddXMLObjectInt(accu, tag, val, wantsCRLF) -- 4-arg */
static int AccuAddXMLObjectInt(AccuPtr a, unsigned char *tag, long val,
                               int cr) {
  (void)a;
  (void)tag;
  (void)val;
  (void)cr;
  return 0;
}
/* AccuAddXMLObject(accu, tag, val, wantsCRLF) -- 4-arg */
static int AccuAddXMLObject(AccuPtr a, unsigned char *tag, unsigned char *val,
                            int cr) {
  (void)a;
  (void)tag;
  (void)val;
  (void)cr;
  return 0;
}
#define FILE_NUM 134
/* Copyright (c) 2000 by Qualcomm, Inc */

enum {
  kStatCount = kBeginShortStats,
  kShortStatCount = kEndStats - kBeginShortStats,
  kSaveStateSecs = 15 * 60, //	Save playlist state every 15 minutes
  kStatFileVersion = 2,
  kDayStatCount = 24,
  kWeekStatCount = 7,
  kMonthStatCount = 31,
  kYearStatCount = 12,
  kAutoUpdateTicks =
      120 //	Auto udpate display 2 seconds after last change in stats
};
enum { kFacetimeOther, kFacetimeRead, kFacetimeWrite };
enum { kThisStat, kLastStat, kAveStat, kTotalStat };

/************************************************************************
 * data
 ************************************************************************/
typedef struct {
  long day[kDayStatCount];
  long week[kWeekStatCount];
  long month[kMonthStatCount];
  long year[kYearStatCount];
} PeriodicStats;

typedef struct {
  PeriodicStats current;
  PeriodicStats last;
  PeriodicStats average;
  long total;
} NumericStats;

typedef struct {
  long day;
  long week;
  long month;
  long year;
} ShortPeriodicStats;

typedef struct {
  ShortPeriodicStats current;
  ShortPeriodicStats last;
  ShortPeriodicStats average;
  long total;
} ShortNumericStats;

typedef struct {
  short version;
  long startTime;     //	When did we start collecting stats?
  long junkStartTime; //	When did we start collecting junk stats?
  long currentTime;
  long unused1; //	totalDays;
  long unused2; //	totalWeeks;
  long unused3; //	totalMonths;
  long unused4; //	totalYears;
  NumericStats numStats[kStatCount];
  ShortNumericStats shortStats[kShortStatCount];
} StatData, *StatDataPtr, *StatDataHandle;

typedef struct {
  short rThis, rLast;
  char sProjected[33];
  StatTimePeriod period;
  short rPeriod;
  bool showAverage;
} ComposeStatsData;

/************************************************************************
 * globals
 ************************************************************************/
static StatDataHandle gStatData;
static struct tm gCurrentTime;
static bool gStatsDirty;
static void * gFaceMeasure;
static short gFacetimeMode;
static uLong gAutoUpdateTicks;
static short dataCountTab[] = {kDayStatCount, kWeekStatCount, kMonthStatCount,
                               kYearStatCount};
static short kwdStrTab[] = {kStatReceivedMail,
                            kStatSentMail,
                            kStatFaceTime,
                            kStatScoredJunk,
                            kStatScoredNotJunk,
                            kStatWhiteList,
                            kStatFalsePositives,
                            kStatFalseNegatives,
                            kStatFalseWhiteList,
                            0,
                            0,
                            0,
                            0,
                            kStatReceivedAttach,
                            kStatSentAttach,
                            kStatReadMsg,
                            0,
                            kStatForwardMsg,
                            kStatReplyMsg,
                            kStatRedirectMsg,
                            kStatFaceTimeRead,
                            kStatFaceTimeCompose,
                            kStatFaceTimeOther};
#define CalcedStat(i) (i >= kStatJunkTotal && i <= kStatJunkPercent)

/************************************************************************
 * prototypes
 ************************************************************************/
static int ReadStatData(void);
static int WriteStatData(void);
static void CheckTimeChange(void);
static int LoadStats(void);
static int SaveStats(bool force);
static void UpdateFacetime(void);
static void UpdateFacetimeLo(void);
static void GetStatTotals(NumericStats *stats, StatTimePeriod period,
                          uLong values[], uLong startTime);
void GetStatTotalPercents(NumericStats *statNumerator,
                          NumericStats *statDenominator, StatTimePeriod period,
                          uLong values[]);
static long SumArray(long *values, short len);
#define SumArrayHi(a) SumArray(a, sizeof(a) / sizeof(a[0]))
static char *GetGraphPict(NumericStats *stats, StatTimePeriod period,
                          bool seconds, bool showAverage, char * sThis,
                          char * sLast, char * sAverage);
static void *GetXLabels(StatTimePeriod period);
static unsigned char * RightAlign7(uLong value, unsigned char * s);
static unsigned char * RightAlign7Dec(uLong value, unsigned char * s);
static unsigned char * RightAlign(uLong value, unsigned char * s, short minLen, short decPlace);
static void AddToAve(uLong *to, uLong *from, short length);
static void UpdateNumStatLo(StatType which, long value);
static int GetAbbrevNames(void **monthNames, void **dayNames);
static void MoveNames(char names[16], long count, long abbrLen, char * dest);
static void ComposeNumStats(AccuPtr a, StatType type, short strType,
                            ComposeStatsData *data);
static void ComposeShortStats(AccuPtr a, StatType type, short strType,
                              ComposeStatsData *data, short rObject,
                              bool percent);
static void ComposeUsageActivities(AccuPtr a, ComposeStatsData *data);
static bool InitGraphPict(GraphData *data, SeriesInfo *theSeries,
                          StatTimePeriod period, char * *labels);
static char *FinishGraphPict(GraphData *data);
static void ComposeFRR(AccuPtr a, ComposeStatsData *data);
void ComposeJunkJunk(AccuPtr a, ComposeStatsData *data);
static uLong CalcElapsedUnits(StatTimePeriod period);
static void AccuAddNumStats(AccuPtr a, short keyword, PeriodicStats *stats);
static void AccuAddArray(AccuPtr a, short keyword, uLong *stats, short count);
static void AccuAddShortStats(AccuPtr a, short keyword,
                              ShortPeriodicStats *stats);
static void ParseStatFile(void *hStatXML);
static short WordStrn(short rStrn, long value);
void UpdateCalcStats(void);
typedef enum { opPlus, opMinus, opPercent } OpType;
void NumericStatOperation(StatType destination, StatType increment, OpType op);
void PeriodicStatOperation(PeriodicStats *destP, PeriodicStats *incP,
                           OpType op);
void LongArrayOperation(long *array, long *sub, short n, OpType op);

/************************************************************************
 * InitStats - initialize statistics
 ************************************************************************/
void InitStats(void) {
  gFaceMeasure = NULL;
  gFacetimeMode = kFacetimeOther;
  LoadStats();
}

/************************************************************************
 * ShutdownStats - shutdown statistics
 ************************************************************************/
void ShutdownStats(void) {
  if (!gStatData)
    return;
  UpdateFacetime();
  SaveStats(true);
  free(gStatData);
  gFaceMeasure = nil;
}

/************************************************************************
 * StatsIdle - idle time processing for statistics
 ************************************************************************/
void StatsIdle(void) {
  static uLong nextIdle, nextSaveStatus;
  short facetimeMode;
  GtkWidget * winWP;
  uLong seconds;

  if (!gStatData || InBG)
    return;

  if (nextIdle && TickCount() < nextIdle)
    return; //	Once a second is sufficient
  nextIdle = TickCount() + 60;

  facetimeMode = kFacetimeOther;
  if ((winWP = MyFrontWindow()) && IsMyWindow(winWP)) {
    TOCType * tocH;

    switch (GetWindowKind(winWP)) {
    case COMP_WIN:
      facetimeMode = kFacetimeWrite;
      break;

    case MESS_WIN:
      facetimeMode = kFacetimeRead;
      break;

    case MBOX_WIN:
    case CBOX_WIN:
      tocH = (TOCType *)NULL;
      if (tocH->previewPTE)
        facetimeMode = kFacetimeRead;
      break;
    }
  }

  if (facetimeMode != gFacetimeMode) {
    UpdateFacetime();
    gFacetimeMode = facetimeMode;
  }

  //	Save stats on disk every 15 minutes in case of system failure
  seconds = LocalDateTime();
  if (nextSaveStatus) {
    if (seconds >= nextSaveStatus) {
      SaveStats(false);
      nextSaveStatus = seconds + kSaveStateSecs;
    }
  } else
    nextSaveStatus = seconds + kSaveStateSecs;

  if (gAutoUpdateTicks && (TickCount() >= gAutoUpdateTicks)) {
    RedisplayStats();
    gAutoUpdateTicks = 0;
  }
}

/************************************************************************
 * UpdateNumStat - update a statistic
 ************************************************************************/
void UpdateNumStat(StatType which, long value) {
  CheckTimeChange();
  UpdateNumStatLo(which, value);
}

/************************************************************************
 * UpdateNumStatWithTime - update a statistic specifying the time
 ************************************************************************/
void UpdateNumStatWithTime(StatType which, long value, uLong seconds) {
  uLong curSeconds;
  NumericStats *pNum = nil;
  ShortNumericStats *pShort = nil;

  if (!gStatData)
    return;

  CheckTimeChange();
  curSeconds = LocalDateTime();
  if (seconds > curSeconds)
    //	If time is in future, just use current time
    UpdateNumStatLo(which, value);
  else if (value) {
    StatDataPtr pData;
    struct tm dt_buf; struct tm *_dtp;
    short dYear, dMonth, dWeek, dDay;

    { time_t _ts = (time_t)seconds; _dtp = localtime(&_ts); if(_dtp) dt_buf = *_dtp; }
    dDay = gCurrentTime.tm_yday - dt_buf.tm_yday;
    dWeek = ((gCurrentTime.tm_yday + 7 - gCurrentTime.tm_wday) / 7) - ((dt_buf.tm_yday + 7 - dt_buf.tm_wday) / 7);
    dMonth = (gCurrentTime.tm_mon + 1) - (dt_buf.tm_mon + 1);
    dYear = (gCurrentTime.tm_year + 1900) - (dt_buf.tm_year + 1900);

    pData = gStatData;

    if (which < kStatCount) {
      pNum = &pData->numStats[which];
      if (dYear == 0) {
        // current year
        pNum->current.year[(dt_buf.tm_mon + 1) - 1] += value;
        if (dMonth == 0)
          pNum->current.month[dt_buf.tm_mday - 1] += value; // current month
        else {
          if (dMonth == 1)
            pNum->last.month[dt_buf.tm_mday - 1] += value;  // last month
          pNum->average.month[dt_buf.tm_mday - 1] += value; // add to average
        }
        if (dWeek == 0)
          pNum->current.week[dt_buf.tm_wday - 1] += value; // current week
        else {
          if (dWeek == 1)
            pNum->last.week[dt_buf.tm_wday - 1] += value;  // last week
          pNum->average.week[dt_buf.tm_wday - 1] += value; // add to average
        }
        if (dDay == 0)
          pNum->current.day[dt_buf.tm_hour] += value; // today
        else {
          if (dDay == 1)
            pNum->last.day[dt_buf.tm_hour] += value;  // yesterday
          pNum->average.day[dt_buf.tm_hour] += value; // add to average
        }
      } else {
        if (dYear == 1) {
          // last year
          pNum->last.year[(dt_buf.tm_mon + 1) - 1] += value;
          if ((dt_buf.tm_mon + 1) == 12 && (gCurrentTime.tm_mon + 1) == 1)
            pNum->last.month[dt_buf.tm_mday - 1] += value; // last month
          if (((dt_buf.tm_yday + 7 - dt_buf.tm_wday) / 7) == 52 && ((gCurrentTime.tm_yday + 7 - gCurrentTime.tm_wday) / 7) == 1)
            pNum->last.week[dt_buf.tm_wday - 1] += value; // last week
        }
        pNum->average.year[(dt_buf.tm_mon + 1) - 1] += value; // add to average
      }
      pNum->total += value;
    } else if (which < kEndStats) {
      pShort = &pData->shortStats[which - kBeginShortStats - 1];
      if (dYear == 0) {
        // current year
        pShort->current.year += value;
        if (dMonth == 0)
          pShort->current.month += value; // current month
        else {
          if (dMonth == 1)
            pShort->last.month += value; // last month
          pShort->average.month += value;
        }
        if (dWeek == 0)
          pShort->current.week += value; // current week
        else {
          if (dWeek == 1)
            pShort->last.week += value; // last week
          pShort->average.week += value;
        }
        if (dDay == 0)
          pShort->current.day += value; // today
        else {
          if (dDay == 1)
            pShort->last.day += value; // yesterday
          pShort->average.day += value;
        }
      } else {
        if (dYear == 1) {
          // last year
          pShort->last.year += value;
          if ((dt_buf.tm_mon + 1) == 12 && (gCurrentTime.tm_mon + 1) == 1)
            pShort->last.month += value; // last month
          if (((dt_buf.tm_yday + 7 - dt_buf.tm_wday) / 7) == 52 && ((gCurrentTime.tm_yday + 7 - gCurrentTime.tm_wday) / 7) == 1)
            pShort->last.week += value; // last week
        }
        pShort->average.year += value;
      }
      pShort->total += value;
    }
    gStatsDirty = true;
    gAutoUpdateTicks = TickCount() + kAutoUpdateTicks;
  }
}

/************************************************************************
 * UpdateNumStatLo - update a statistic
 ************************************************************************/
static void UpdateNumStatLo(StatType which, long value) {
  StatDataPtr pData;
  NumericStats *pNum = nil;
  ShortNumericStats *pShort = nil;

  if (LoadStats())
    return;

  if (value) {
    pData = gStatData;

    if (which < kStatCount) {
      pNum = &pData->numStats[which];
      pNum->current.day[gCurrentTime.tm_hour] += value;
      pNum->current.week[gCurrentTime.tm_wday - 1] += value;
      pNum->current.month[gCurrentTime.tm_mday - 1] += value;
      pNum->current.year[(gCurrentTime.tm_mon + 1) - 1] += value;
      pNum->total += value;
    } else if (which < kEndStats) {
      pShort = &pData->shortStats[which - kBeginShortStats - 1];
      pShort->current.day += value;
      pShort->current.week += value;
      pShort->current.month += value;
      pShort->current.year += value;
      pShort->total += value;
    }
    gStatsDirty = true;
    gAutoUpdateTicks = TickCount() + kAutoUpdateTicks;
  }
}

/************************************************************************
 * CheckTimeChange - see if we have gone to a new time period
 ************************************************************************/
void CheckTimeChange(void) {
  struct tm dt_buf, dtStart_buf; struct tm *_dtp;
  enum {
    kNewYear = 0x01,
    kSkipYear = 0x02,
    kNewMonth = 0x04,
    kSkipMonth = 0x08,
    kNewWeek = 0x10,
    kSkipWeek = 0x20,
    kNewDay = 0x40,
    kSkipDay = 0x80,
    kStartYear = 0x100,
    kStartMonth = 0x200,
    kStartWeek = 0x400,
    kStartDay = 0x800
  } timeFlags;

  if (!gStatData)
    return;

  { time_t _ts = (time_t)gStatData->startTime; _dtp = localtime(&_ts); if(_dtp) dtStart_buf = *_dtp; }
  timeFlags = 0;

  if (dt_buf.tm_hour != gCurrentTime.tm_hour)
    // facetime needs to be updated at least every hour
    UpdateFacetimeLo();

  if ((dt_buf.tm_year + 1900) > (gCurrentTime.tm_year + 1900)) {
    //	Happy New Year!
    timeFlags |= kNewYear + kNewMonth + kNewDay;
    if ((dt_buf.tm_year + 1900) - (gCurrentTime.tm_year + 1900) > 1)
      // Skipped a year
      timeFlags |= kSkipYear + kSkipMonth + kSkipWeek + kSkipDay;
    if (dtStart_buf.tm_year == (gCurrentTime.tm_year + 1900))
      // don't include very first month in average
      timeFlags |= kStartYear;
  }

  if ((dt_buf.tm_mon + 1) > (gCurrentTime.tm_mon + 1) ||
      (dt_buf.tm_year + 1900) > (gCurrentTime.tm_year + 1900)) {
    // New month
    timeFlags |= kNewMonth + kNewDay;
    if ((dt_buf.tm_mon + 1) - (gCurrentTime.tm_mon + 1) > 1)
      // Skipped a month
      timeFlags |= kSkipMonth + kSkipWeek + kSkipDay;
    if (dtStart_buf.tm_year == (gCurrentTime.tm_year + 1900) &&
        dtStart_buf.tm_mon == (gCurrentTime.tm_mon + 1))
      // don't include very first day in average
      timeFlags |= kStartMonth;
  }

  if (((dt_buf.tm_yday + 7 - dt_buf.tm_wday) / 7) > ((gCurrentTime.tm_yday + 7 - gCurrentTime.tm_wday) / 7) ||
      (dt_buf.tm_year + 1900) > (gCurrentTime.tm_year + 1900)) {
    // New week
    timeFlags |= kNewWeek + kNewDay;
    if (((dt_buf.tm_yday + 7 - dt_buf.tm_wday) / 7) - ((gCurrentTime.tm_yday + 7 - gCurrentTime.tm_wday) / 7) > 1)
      // Skipped a week
      timeFlags |= kSkipWeek + kSkipDay;
    if (dtStart_buf.tm_year == (gCurrentTime.tm_year + 1900) &&
        dtStart_buf.tm_yday == ((gCurrentTime.tm_yday + 7 - gCurrentTime.tm_wday) / 7))
      // don't include very first day in average
      timeFlags |= kStartWeek;
  }

  if (dt_buf.tm_yday > gCurrentTime.tm_yday ||
      (dt_buf.tm_year + 1900) > (gCurrentTime.tm_year + 1900)) {
    // New day
    timeFlags |= kNewDay;
    if (dt_buf.tm_yday - gCurrentTime.tm_yday > 1)
      // Skipped a day
      timeFlags |= kSkipDay;
    if (dtStart_buf.tm_year == (gCurrentTime.tm_year + 1900) &&
        dtStart_buf.tm_yday == gCurrentTime.tm_yday)
      // don't include very first hour in average
      timeFlags |= kStartDay;
  }

  if (timeFlags) {
    short i;
    NumericStats *pStats;
    ShortNumericStats *pShortStats;


    //  Do numeric stats
    for (i = 0, pStats = gStatData->numStats; i < kStatCount;
         i++, pStats++) {
      //	Do year
      if (timeFlags & kNewYear) {
        memmove(pStats->last.year, pStats->current.year, sizeof(pStats->current.year));
        if (timeFlags & kStartYear)
          // don't use first partial month
          pStats->current.year[dtStart_buf.tm_mon - 1] = 0;
        AddToAve((uLong *)pStats->average.year, (uLong *)pStats->current.year,
                 kYearStatCount);
        Zero(pStats->current.year);
        if (timeFlags & kSkipYear)
          Zero(pStats->last.year);
      }
      //	Do month
      if (timeFlags & kNewMonth) {
        memmove(pStats->last.month, pStats->current.month, sizeof(pStats->current.month));
        if (timeFlags & kStartMonth)
          // don't use first partial day
          pStats->current.month[dtStart_buf.tm_mday - 1] = 0;
        AddToAve((uLong *)pStats->average.month, (uLong *)pStats->current.month,
                 kMonthStatCount);
        Zero(pStats->current.month);
        if (timeFlags & kSkipMonth)
          Zero(pStats->last.month);
      }
      //	Do week
      if (timeFlags & kNewWeek) {
        memmove(pStats->last.week, pStats->current.week, sizeof(pStats->current.week));
        if (timeFlags & kStartWeek)
          // don't use first partial day
          pStats->current.week[dtStart_buf.tm_wday - 1] = 0;
        AddToAve((uLong *)pStats->average.week, (uLong *)pStats->current.week,
                 kWeekStatCount);
        Zero(pStats->current.week);
        if (timeFlags & kSkipWeek)
          Zero(pStats->last.week);
      }
      //	Do day
      if (timeFlags & kNewDay) {
        memmove(pStats->last.day, pStats->current.day, sizeof(pStats->current.day));
        if (timeFlags & kStartDay)
          // don't use first partial hour
          pStats->current.day[dtStart_buf.tm_hour] = 0;
        AddToAve((uLong *)pStats->average.day, (uLong *)pStats->current.day,
                 kDayStatCount);
        Zero(pStats->current.day);
        if (timeFlags & kSkipDay)
          Zero(pStats->last.day);
      }
    }

    //  Do short stats
    for (i = 0, pShortStats = gStatData->shortStats; i < kShortStatCount;
         i++, pShortStats++) {
      //	Do year
      if (timeFlags & kNewYear) {
        if (timeFlags & kSkipYear)
          pShortStats->current.year = 0;
        pShortStats->last.year = pShortStats->current.year;
        pShortStats->current.year = 0;
      }
      //	Do month
      if (timeFlags & kNewMonth) {
        if (timeFlags & kSkipMonth)
          pShortStats->current.month = 0;
        pShortStats->last.month = pShortStats->current.month;
        pShortStats->current.month = 0;
      }
      //	Do week
      if (timeFlags & kNewWeek) {
        if (timeFlags & kSkipWeek)
          pShortStats->current.week = 0;
        pShortStats->last.week = pShortStats->current.week;
        pShortStats->current.week = 0;
      }
      //	Do day
      if (timeFlags & kNewDay) {
        if (timeFlags & kSkipDay)
          pShortStats->current.day = 0;
        pShortStats->last.day = pShortStats->current.day;
        pShortStats->current.day = 0;
      }
    }
    gAutoUpdateTicks = TickCount() + kAutoUpdateTicks;
  }

  /* gCurrentTime already set above */
}

/************************************************************************
 * AddToAve - add current stats to average
 ************************************************************************/
static void AddToAve(uLong *to, uLong *from, short length) {
  while (length--)
    *to++ += *from++;
}

/************************************************************************
 * LoadStats - load stats from file in XML format
 ************************************************************************/
int LoadStats(void) {
  int err;
  FSSpec spec, renameSpec;
  char s[256];
  void *hStatXML;
  struct tm dt_buf; struct tm *_dtp;

  if (gStatData)
    return 0; //	Already got it

  gStatData = calloc(1, sizeof(StatData));
  if (!gStatData)
    return 0;

  gStatData->startTime = gStatData->junkStartTime =
      gStatData->currentTime =
          LocalDateTime(); // in case we can't get any saved stats

  spec_for(Root.path, (const char *)GetRString(s, STATISTICS_FILE), &spec);
  err = Snarf(&spec, &hStatXML, 0);
  if (!err) {
    ParseStatFile(hStatXML);
    free(hStatXML);
  } else if (!FSpExists(&spec)) {
    //	There was a problem loading the stats data. Either the file doesn't
    // exist yet 	or there was a file error. In the latter case, rename
    // the bad file so we have 	a backup and so we can create a new one.
    g_strlcpy(renameSpec, spec, sizeof(renameSpec));
    g_strlcat((char *)spec_name(renameSpec), ".bad", sizeof(spec_name(renameSpec)));
    UniqueSpec(&renameSpec, 31);
    MyFSpRename(&spec, spec_name(renameSpec));
  }

  { time_t _ts = (time_t)gStatData->currentTime; _dtp = localtime(&_ts); if(_dtp) dt_buf = *_dtp; }
  CheckTimeChange();

  return err;
}

/************************************************************************
 * SaveStats - save stats to file in XML format
 ************************************************************************/
int SaveStats(bool force) {
  int err;
  FSSpec spec;
  char s[256], sValue[256];
  Accumulator a;
  StatDataPtr pStatData;
  short i;

  if (!gStatData || !gStatsDirty)
    return 0;

  if (!force && !DiskSpunUp())
    return 0; // if the disk isn't spun up, bail

  CheckTimeChange();
  gStatData->currentTime = LocalDateTime();
  pStatData = gStatData;

  // start XML
  AccuInit(&a);
  AccuAddStr(&a, GetRString(s, StatXMLStrn + ksStatXMLVersion));
  AccuAddCRLF(&a);
  AccuAddTagLine(&a, GetRString(s, StatXMLStrn + ksStatDocEntry), false);
  XMLIncIndent();

  // initial data
  AccuAddXMLObjectInt(&a,
                      (unsigned char *)GetRString((unsigned char *)s,
                                                  StatXMLStrn + ksStatVersion),
                      kStatFileVersion, false); //      version
  AccuAddXMLObject(
      &a,
      (unsigned char *)GetRString((unsigned char *)s,
                                  StatXMLStrn + ksStatStartTime),
      (unsigned char *)R822Date(sValue, pStatData->startTime - ZoneSecs()),
      false); //   start time
  AccuAddXMLObject(
      &a,
      (unsigned char *)GetRString((unsigned char *)s,
                                  StatXMLStrn + ksStatJunkStartTime),
      (unsigned char *)R822Date(sValue, pStatData->junkStartTime - ZoneSecs()),
      false); //     junk start time
  AccuAddXMLObject(
      &a,
      (unsigned char *)GetRString((unsigned char *)s,
                                  StatXMLStrn + ksStatCurrentTime),
      (unsigned char *)R822Date(sValue, pStatData->currentTime - ZoneSecs()),
      false); //     current time
  AccuAddCRLF(&a);

  // number stats
  for (i = 0; i < kEndStats; i++) {
    if (i == kBeginShortStats || i == kStatUnused || CalcedStat(i))
      continue;
    AccuAddTagLine(&a, GetRString(s, StatXMLStrn + kwdStrTab[i]), false);
    XMLIncIndent();
    if (i < kBeginShortStats) {
      // number stats
      AccuAddNumStats(&a, StatXMLStrn + ksStatCurrent,
                      &pStatData->numStats[i].current);
      AccuAddNumStats(&a, StatXMLStrn + ksStatPrevious,
                      &pStatData->numStats[i].last);
      AccuAddNumStats(&a, StatXMLStrn + ksStatAveSum,
                      &pStatData->numStats[i].average);
      AccuAddXMLObjectInt(&a,
                          (unsigned char *)GetRString(
                              (unsigned char *)s, StatXMLStrn + ksStatTotal),
                          pStatData->numStats[i].total, false);
    } else {
      // short number stats
      AccuAddShortStats(
          &a, StatXMLStrn + ksStatCurrent,
          &pStatData->shortStats[i - kBeginShortStats - 1].current);
      AccuAddShortStats(&a, StatXMLStrn + ksStatPrevious,
                        &pStatData->shortStats[i - kBeginShortStats - 1].last);
      AccuAddShortStats(
          &a, StatXMLStrn + ksStatAveSum,
          &pStatData->shortStats[i - kBeginShortStats - 1].average);
      AccuAddXMLObjectInt(&a,
                          (unsigned char *)GetRString(
                              (unsigned char *)s, StatXMLStrn + ksStatTotal),
                          pStatData->shortStats[i - kBeginShortStats - 1].total,
                          false);
    }
    XMLDecIndent();
    AccuAddTagLine(&a, GetRString(s, StatXMLStrn + kwdStrTab[i]), true);
    AccuAddCRLF(&a);
  }

  // end XML
  XMLDecIndent();
  AccuAddTagLine(&a, GetRString(s, StatXMLStrn + ksStatDocEntry), true);
  AccuTrim(&a);

  // save out to file
  spec_for(Root.path,
               (const char *)GetRString((unsigned char *)s, STATISTICS_FILE),
               &spec);
  err = Blat(&spec, a.data, false);
  free(a.data); a.data = NULL; a.offset = a.size = 0;

  gStatsDirty = false;
  return err;
}

/************************************************************************
 * ResetStatistics - throw away the stats and start all over
 ************************************************************************/
void ResetStatistics(void) {
  FSSpec spec;
  char s[256];

  ShutdownStats();
  spec_for(Root.path, (const char *)GetRString(s, STATISTICS_FILE), &spec);
  FSpTrash(&spec);
  InitStats();
}

/************************************************************************
 * AccuAddShortStats - save short stats
 ************************************************************************/
static void AccuAddShortStats(AccuPtr a, short keyword,
                              ShortPeriodicStats *stats) {
  char s[64];

  AccuAddTagLine(a, (unsigned char *)GetRString((unsigned char *)s, keyword),
                 false);
  XMLIncIndent();
  AccuAddXMLObjectInt(
      a,
      (unsigned char *)GetRString((unsigned char *)s, StatXMLStrn + ksStatDay),
      stats->day, true);
  AccuAddXMLObjectInt(
      a,
      (unsigned char *)GetRString((unsigned char *)s, StatXMLStrn + ksStatWeek),
      stats->week, true);
  AccuAddXMLObjectInt(a,
                      (unsigned char *)GetRString((unsigned char *)s,
                                                  StatXMLStrn + ksStatMonth),
                      stats->month, true);
  AccuAddXMLObjectInt(
      a,
      (unsigned char *)GetRString((unsigned char *)s, StatXMLStrn + ksStatYear),
      stats->year, true);
  XMLDecIndent();
  AccuAddTagLine(a, (unsigned char *)GetRString((unsigned char *)s, keyword),
                 true);
}

/************************************************************************
 * AccuAddNumStats - save number stats
 ************************************************************************/
static void AccuAddNumStats(AccuPtr a, short keyword, PeriodicStats *stats) {
  char s[64];

  AccuAddTagLine(a, (unsigned char *)GetRString((unsigned char *)s, keyword),
                 false);
  XMLIncIndent();
  AccuAddArray(a, StatXMLStrn + ksStatDay, (uLong *)stats->day, kDayStatCount);
  AccuAddArray(a, StatXMLStrn + ksStatWeek, (uLong *)stats->week,
               kWeekStatCount);
  AccuAddArray(a, StatXMLStrn + ksStatMonth, (uLong *)stats->month,
               kMonthStatCount);
  AccuAddArray(a, StatXMLStrn + ksStatYear, (uLong *)stats->year,
               kYearStatCount);
  XMLDecIndent();
  AccuAddTagLine(a, (unsigned char *)GetRString((unsigned char *)s, keyword),
                 true);
}

/************************************************************************
 * AccuAddArray - save an array of data
 ************************************************************************/
static void AccuAddArray(AccuPtr a, short keyword, uLong *stats, short count) {
  char s[64];
  short i;

  AccuIndent(a);
  AccuAddTag(a, (unsigned char *)GetRString((unsigned char *)s, keyword),
             false);
  for (i = 0; i < count; i++) {
    if (i)
      AccuAddChar(a, ' '); // delimiter
    sprintf(s, "%ld", (long)(stats[i]));
    AccuAddStr(a, (unsigned char *)s);
  }
  AccuAddTag(a, (unsigned char *)GetRString((unsigned char *)s, keyword), true);
  AccuAddCRLF(a);
}

/************************************************************************
 * UpdateFacetime - udpate facetime stats
 ************************************************************************/
static void UpdateFacetime(void) {
  CheckTimeChange();
  UpdateFacetimeLo();
}

/************************************************************************
 * UpdateFacetimeLo - udpate facetime stats
 ************************************************************************/
static void UpdateFacetimeLo(void) {
  long faceTime;
  if (faceTime > 0) {
    UpdateNumStatLo(kStatFaceTime, faceTime);
    UpdateNumStatLo(gFacetimeMode == kFacetimeOther  ? kStatFaceTimeOther
                    : gFacetimeMode == kFacetimeRead ? kStatFaceTimeRead
                                                     : kStatFaceTimeCompose,
                    faceTime);
  }
}

/************************************************************************
 * SumArray - return sum of long array
 ************************************************************************/
static long SumArray(long *values, short len) {
  short i;
  long sum = 0;

  for (i = 0; i < len; i++)
    sum += *values++;
  return sum;
}

/************************************************************************
 * GetStatTotals - get some totals
 ************************************************************************/
static void GetStatTotals(NumericStats *stats, StatTimePeriod period,
                          uLong values[], uLong startTime) {
  float dayAve, secs;

  secs = LocalDateTime() - startTime;
  //	average is times 10 so we can do a single decimal place
  dayAve = secs ? 10 * 24.0 * 60.0 * 60.0 * (float)stats->total / secs : 0;
  switch (period) {
  case kStatDay:
    values[kThisStat] = SumArray(stats->current.day, kDayStatCount);
    values[kLastStat] = SumArray(stats->last.day, kDayStatCount);
    values[kAveStat] = dayAve;
    break;

  case kStatWeek:
    values[kThisStat] = SumArray(stats->current.week, kWeekStatCount);
    values[kLastStat] = SumArray(stats->last.week, kWeekStatCount);
    values[kAveStat] = 7.0 * dayAve;
    break;

  case kStatMonth:
    values[kThisStat] = SumArray(stats->current.month, kMonthStatCount);
    values[kLastStat] = SumArray(stats->last.month, kMonthStatCount);
    values[kAveStat] = 30.4375 * dayAve;
    break;

  case kStatYear:
    values[kThisStat] = SumArray(stats->current.year, kYearStatCount);
    values[kLastStat] = SumArray(stats->last.year, kYearStatCount);
    values[kAveStat] = 365.25 * dayAve;
    break;
  }
  values[kTotalStat] = stats->total;
}

/************************************************************************
 * GetStatTotalPercents - get some totals
 ************************************************************************/
void GetStatTotalPercents(NumericStats *statNumerator,
                          NumericStats *statDenominator, StatTimePeriod period,
                          uLong values[]) {
  switch (period) {
  case kStatDay:
    values[kThisStat] =
        (100 * SumArray(statNumerator->current.day, kDayStatCount)) /
        SumArray(statDenominator->current.day, kDayStatCount);
    values[kLastStat] =
        (100 * SumArray(statNumerator->last.day, kDayStatCount)) /
        SumArray(statDenominator->last.day, kDayStatCount);
    break;

  case kStatWeek:
    values[kThisStat] =
        (100 * SumArray(statNumerator->current.week, kDayStatCount)) /
        SumArray(statDenominator->current.week, kDayStatCount);
    values[kLastStat] =
        (100 * SumArray(statNumerator->last.week, kDayStatCount)) /
        SumArray(statDenominator->last.week, kDayStatCount);
    break;

  case kStatMonth:
    values[kThisStat] =
        (100 * SumArray(statNumerator->current.month, kDayStatCount)) /
        SumArray(statDenominator->current.month, kDayStatCount);
    values[kLastStat] =
        (100 * SumArray(statNumerator->last.month, kDayStatCount)) /
        SumArray(statDenominator->last.month, kDayStatCount);
    break;

  case kStatYear:
    values[kThisStat] =
        (100 * SumArray(statNumerator->current.year, kDayStatCount)) /
        SumArray(statDenominator->current.year, kDayStatCount);
    values[kLastStat] =
        (100 * SumArray(statNumerator->last.year, kDayStatCount)) /
        SumArray(statDenominator->last.year, kDayStatCount);
    break;
  }
  values[kTotalStat] = (100 * statNumerator->total) / statDenominator->total;
}

/************************************************************************
 * GetStatsAsText - return stats in HTML format
 ************************************************************************/
void *GetStatsAsText(StatTimePeriod period, bool extended) {
  Accumulator a;
  ComposeStatsData data;
  char s[256];
  char sTime[33];
  short rPeriodTab[] = {sStatDay, sStatWeek, sStatMonth, sStatYear};
  short rThisTab[] = {sStatToday, sStatThisWeek, sStatThisMonth, sStatThisYear};
  short rLastTab[] = {sStatYesterday, sStatLastWeek, sStatLastMonth,
                      sStatLastYear};
  uLong secondsTab[] = {24 * 60 * 60, 7 * 24 * 60 * 60, 31 * 24 * 60 * 60,
                        365 * 24 * 60 * 60};
  bool oldShowAve;
  bool canScoreJunk = false;
  data.period = period;
  data.rPeriod = StatHTMLStrn + rPeriodTab[period];
  data.rThis = StatHTMLStrn + rThisTab[period];
  data.rLast = StatHTMLStrn + rLastTab[period];

  //	Don't display averages until we have statistics for 24 hours
  data.showAverage = LocalDateTime() > gStatData->startTime + 60 * 60 * 24;

  if (AccuInit(&a)) {
    return nil;
  }

  if (!data.showAverage ||
      ((LocalDateTime() - gStatData->startTime) < secondsTab[period]))
    GetRString(data.sProjected, StatHTMLStrn + sStatProjected);
  else
    data.sProjected[0] = 0;

  UpdateCalcStats();

  AccuAddRes(&a, StatHTMLStrn + sStatHTMLBegin); //	begin HTML
  if (!extended) {
    ComposeNumStats(&a, kStatReceivedMail, sStatRecdEmail,
                    &data);                      //	Received Email
    AccuAddRes(&a, StatHTMLStrn + sStatDivider); //	divider

    if (canScoreJunk) {
      oldShowAve = data.showAverage;
      data.showAverage = false;
      ComposeNumStats(&a, kStatJunkPercent, sStatJunkPercent,
                      &data); //	Junk; calculated
      data.showAverage = oldShowAve;
      AccuAddRes(&a, StatHTMLStrn + sStatDivider); //	divider
    }

    ComposeNumStats(&a, kStatSentMail, sStatSentEmail,
                    &data);                       //	Sent Email
    AccuAddRes(&a, StatHTMLStrn + sStatDivider);  //	divider
    ComposeNumStats(&a, kStatFaceTime, 0, &data); //	Usage
    AccuAddRes(&a, StatHTMLStrn + sStatDivider);  //	divider
  } else {
    //	More stats
    ComposeNumStats(&a, kStatReceivedMail, sStatRecdEmail,
                    &data); //	Received Email
    ComposeShortStats(&a, kStatReadMsg, sStatMsgsRead, &data,
                      StatHTMLStrn + sStatMsgZero, true); //	Read mail
    ComposeShortStats(&a, kStatReceivedAttach, sStatAttachRecv, &data,
                      StatHTMLStrn + sStatAttachZero,
                      false);                    //	Received attachments
    AccuAddRes(&a, StatHTMLStrn + sStatDivider); //	divider

    if (canScoreJunk) {
      oldShowAve = data.showAverage;
      data.showAverage = false;
      ComposeNumStats(&a, kStatJunkPercent, sStatJunkPercent,
                      &data); //	Junk; calculated
      data.showAverage = oldShowAve;
      ComposeJunkJunk(&a, &data);
      AccuAddRes(&a, StatHTMLStrn + sStatDivider); //	divider
    }

    ComposeNumStats(&a, kStatSentMail, sStatSentEmail,
                    &data); //	Sent Email
    ComposeFRR(&a, &data);  //	Forward, Reply, Redirect
    ComposeShortStats(&a, kStatSentAttach, sStatAttachSent, &data,
                      StatHTMLStrn + sStatAttachZero,
                      false);                     //	Sent attachments
    AccuAddRes(&a, StatHTMLStrn + sStatDivider);  //	divider
    ComposeNumStats(&a, kStatFaceTime, 0, &data); //	Usage
    ComposeUsageActivities(&a, &data);            //	reading, composing, other
    AccuAddRes(&a, StatHTMLStrn + sStatDivider);  //	divider
  }

  //	Since when?
  TimeString(gStatData->startTime, false, sTime, nil);
  AccuComposeR(&a, StatHTMLStrn + sStatSince, s, sTime);

  //	Current time
  TimeString(LocalDateTime(), false, sTime, nil);
  AccuComposeR(&a, StatHTMLStrn + sStatCurrentTime, s, sTime);

  AccuAddRes(&a, StatHTMLStrn + sStatHTMLEnd); //	end HTML
  return a.data;
}

/************************************************************************
 * ComposeFRR - show forward, reply, redirect
 ************************************************************************/
static void ComposeFRR(AccuPtr a, ComposeStatsData *data) {
  uLong secs;
  char sThisVal[33], sLastVal[33], sAveVal[33], sTotalVal[33];
  char sThisPeriod[64], sLastPeriod[64];
  ShortNumericStats *sStats;
  float dayAve;
  void *hPict;
  GraphData graphData;
  SeriesInfo theSeries[3];
  char * labels[3];
  uLong thisSeries[3], lastSeries[3], aveSeries[3], totalSeries;
  short i;

  for (i = 0; i < 3; i++) {
    sStats =
        &gStatData->shortStats[kStatForwardMsg - kBeginShortStats - 1 + i];
    secs = LocalDateTime() - gStatData->startTime;
    dayAve = secs ? 10 * 24.0 * 60.0 * 60.0 * (float)sStats->total / secs : 0;
    switch (data->period) {
    case kStatDay:
      thisSeries[i] = sStats->current.day;
      lastSeries[i] = sStats->last.day;
      aveSeries[i] = dayAve;
      break;

    case kStatWeek:
      thisSeries[i] = sStats->current.week;
      lastSeries[i] = sStats->last.week;
      aveSeries[i] = dayAve * 7.0;
      break;

    case kStatMonth:
      thisSeries[i] = sStats->current.month;
      lastSeries[i] = sStats->last.month;
      aveSeries[i] = dayAve * 30.4375;
      break;

    case kStatYear:
      thisSeries[i] = sStats->current.year;
      lastSeries[i] = sStats->last.year;
      aveSeries[i] = dayAve * 365.25;
      break;
    }
    totalSeries = sStats->total;
    RightAlign7(thisSeries[i], sThisVal);
    RightAlign7(lastSeries[i], sLastVal);
    RightAlign7Dec(aveSeries[i], sAveVal);
    if (!data->showAverage)
      memset(sAveVal + 1, ' ', *sAveVal); // replace with spaces
    RightAlign7(totalSeries, sTotalVal);
    AccuComposeR(a, StatHTMLStrn + sStatFRR, StatHTMLStrn + sStatForward + i,
                 sThisVal, WordStrn(StatHTMLStrn + sStatMsgZero, thisSeries[i]),
                 data->rThis, sLastVal,
                 WordStrn(StatHTMLStrn + sStatMsgZero, lastSeries[i]),
                 data->rLast, sAveVal, data->rPeriod, data->sProjected,
                 sTotalVal, WordStrn(StatHTMLStrn + sStatMsgZero, totalSeries));
  }

  // add a graph
  labels[0] = GetRString(sThisPeriod, data->rThis);
  labels[1] = GetRString(sLastPeriod, data->rLast);
  labels[2] = GetRString(sAveVal, StatHTMLStrn + sStatAverage);
  Zero(theSeries);
  if (InitGraphPict(&graphData, theSeries, kStatFaceTime, labels)) {
    graphData.dataCount = 3;
    graphData.type = kBarGraph;
    graphData.xLabels = GetResource('STR#', FRRStrn);
    theSeries[0].data = thisSeries;
    theSeries[1].data = lastSeries;
    theSeries[2].data = aveSeries;
    theSeries[2].divider = 10; // increase average graph resolution
    if (!CalcElapsedUnits(data->period))
      graphData.seriesCount = 2; //	not enough data to do average yet

    char *b64 = FinishGraphPict(&graphData);
    if (b64) {
      AccuAddStr(a, (unsigned char *)"<br><img src=\"data:image/png;base64,");
      AccuAddStr(a, (unsigned char *)b64);
      AccuAddStr(a, (unsigned char *)"\"><br>\n");
      g_free(b64);
    }
  }
}

/************************************************************************
 * ComposeJunkJunk - show more junk stats
 ************************************************************************/
void ComposeJunkJunk(AccuPtr a, ComposeStatsData *data) {
  char sThis[33], sLast[33];
  uLong junkTotal, total, falseNeg, falsePos, falseWhite;
  uLong lastJunkTotal, lastTotal, lastFalseNeg, lastFalsePos, lastFalseWhite;

  switch (data->period) {
  case kStatDay:
    junkTotal = SumArrayHi(gStatData->numStats[kStatJunkTotal].current.day);
    total = SumArrayHi(gStatData->numStats[kStatTotal].current.day);
    falseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].current.day);
    falsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].current.day);
    falseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].current.day);
    lastJunkTotal = SumArrayHi(gStatData->numStats[kStatJunkTotal].last.day);
    lastTotal = SumArrayHi(gStatData->numStats[kStatTotal].last.day);
    lastFalseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].last.day);
    lastFalsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].last.day);
    lastFalseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].last.day);
    break;

  case kStatWeek:
    junkTotal = SumArrayHi(gStatData->numStats[kStatJunkTotal].current.week);
    total = SumArrayHi(gStatData->numStats[kStatTotal].current.week);
    falseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].current.week);
    falsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].current.week);
    falseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].current.week);
    lastJunkTotal =
        SumArrayHi(gStatData->numStats[kStatJunkTotal].last.week);
    lastTotal = SumArrayHi(gStatData->numStats[kStatTotal].last.week);
    lastFalseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].last.week);
    lastFalsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].last.week);
    lastFalseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].last.week);
    break;

  case kStatMonth:
    junkTotal =
        SumArrayHi(gStatData->numStats[kStatJunkTotal].current.month);
    total = SumArrayHi(gStatData->numStats[kStatTotal].current.month);
    falseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].current.month);
    falsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].current.month);
    falseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].current.month);
    lastJunkTotal =
        SumArrayHi(gStatData->numStats[kStatJunkTotal].last.month);
    lastTotal = SumArrayHi(gStatData->numStats[kStatTotal].last.month);
    lastFalseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].last.month);
    lastFalsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].last.month);
    lastFalseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].last.month);
    break;

  case kStatYear:
    junkTotal = SumArrayHi(gStatData->numStats[kStatJunkTotal].current.year);
    total = SumArrayHi(gStatData->numStats[kStatTotal].current.year);
    falseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].current.year);
    falsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].current.year);
    falseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].current.year);
    lastJunkTotal =
        SumArrayHi(gStatData->numStats[kStatJunkTotal].last.year);
    lastTotal = SumArrayHi(gStatData->numStats[kStatTotal].last.year);
    lastFalseNeg =
        SumArrayHi(gStatData->numStats[kStatFalseNegatives].last.year);
    lastFalsePos =
        SumArrayHi(gStatData->numStats[kStatFalsePositives].last.year);
    lastFalseWhite =
        SumArrayHi(gStatData->numStats[kStatFalseWhiteList].last.year);
    break;
  }

  // no divide by zero
  if (!total)
    total = 1;
  if (!lastTotal)
    lastTotal = 1;

  AccuComposeR(a, StatHTMLStrn + sStatMsgAndPercent,
               StatHTMLStrn + sStatJunkJunk, // Header for section
               RightAlign7(junkTotal, sThis),
               WordStrn(StatHTMLStrn + sStatMsgZero, junkTotal),
               (100 * junkTotal) / total, data->rThis, // this year's info
               RightAlign7(lastJunkTotal, sLast),
               WordStrn(StatHTMLStrn + sStatMsgZero, lastJunkTotal),
               (100 * lastJunkTotal) / lastTotal,
               data->rLast); // last year's info
  AccuComposeR(a, StatHTMLStrn + sStatMsgAndPercent,
               StatHTMLStrn + sStatJunkFalseNeg, // Header for section
               RightAlign7(falseNeg, sThis),
               WordStrn(StatHTMLStrn + sStatMsgZero, falseNeg),
               (100 * falseNeg) / total, data->rThis, // this year's info
               RightAlign7(lastFalseNeg, sLast),
               WordStrn(StatHTMLStrn + sStatMsgZero, lastFalseNeg),
               (100 * lastFalseNeg) / lastTotal,
               data->rLast); // last year's info
  AccuComposeR(a, StatHTMLStrn + sStatMsgAndPercent,
               StatHTMLStrn + sStatJunkFalsePos, // Header for section
               RightAlign7(falsePos, sThis),
               WordStrn(StatHTMLStrn + sStatMsgZero, falsePos),
               (100 * falsePos) / total, data->rThis, // this year's info
               RightAlign7(lastFalsePos, sLast),
               WordStrn(StatHTMLStrn + sStatMsgZero, lastFalsePos),
               (100 * lastFalsePos) / lastTotal,
               data->rLast); // last year's info
  if (JunkPrefWhiteList())
    AccuComposeR(a, StatHTMLStrn + sStatMsgAndPercent,
                 StatHTMLStrn + sStatJunkFalseWhite, // Header for section
                 RightAlign7(falseWhite, sThis),
                 WordStrn(StatHTMLStrn + sStatMsgZero, falseWhite),
                 (100 * falseWhite) / total, data->rThis, // this year's info
                 RightAlign7(lastFalseWhite, sLast),
                 WordStrn(StatHTMLStrn + sStatMsgZero, lastFalseWhite),
                 (100 * lastFalseWhite) / lastTotal,
                 data->rLast); // last year's info
  AccuComposeR(a, StatHTMLStrn + sStatPercent,
               StatHTMLStrn + sStatAccuracy, // Header for section
               RightAlign7((100 * (total - falsePos - falseNeg - falseWhite)) /
                               (total - falseWhite),
                           sThis),
               data->rThis, // this year's info
               RightAlign7((100 * (lastTotal - lastFalsePos - lastFalseNeg -
                                   lastFalseWhite)) /
                               (lastTotal - lastFalseWhite),
                           sLast),
               data->rLast); // last year's info
}

/************************************************************************
 * ComposeUsageActivities - show reading, composing, other percentages
 ************************************************************************/
static void ComposeUsageActivities(AccuPtr a, ComposeStatsData *data) {
  uLong read, compose, other, total, readPercent, composePercent, otherPercent;
  char sRead[33], sCompose[33], sOther[33];
  void *hPict;
  GraphData graphData;
  SeriesInfo theSeries[3];
  char * labels[3];
  uLong series[3];

  switch (data->period) {
  case kStatDay:
    read = (gStatData)
               ->shortStats[kStatFaceTimeRead - kBeginShortStats - 1]
               .current.day;
    compose = (gStatData)
                  ->shortStats[kStatFaceTimeCompose - kBeginShortStats - 1]
                  .current.day;
    other = (gStatData)
                ->shortStats[kStatFaceTimeOther - kBeginShortStats - 1]
                .current.day;
    break;

  case kStatWeek:
    read = (gStatData)
               ->shortStats[kStatFaceTimeRead - kBeginShortStats - 1]
               .current.week;
    compose = (gStatData)
                  ->shortStats[kStatFaceTimeCompose - kBeginShortStats - 1]
                  .current.week;
    other = (gStatData)
                ->shortStats[kStatFaceTimeOther - kBeginShortStats - 1]
                .current.week;
    break;

  case kStatMonth:
    read = (gStatData)
               ->shortStats[kStatFaceTimeRead - kBeginShortStats - 1]
               .current.month;
    compose = (gStatData)
                  ->shortStats[kStatFaceTimeCompose - kBeginShortStats - 1]
                  .current.month;
    other = (gStatData)
                ->shortStats[kStatFaceTimeOther - kBeginShortStats - 1]
                .current.month;
    break;

  case kStatYear:
    read = (gStatData)
               ->shortStats[kStatFaceTimeRead - kBeginShortStats - 1]
               .current.year;
    compose = (gStatData)
                  ->shortStats[kStatFaceTimeCompose - kBeginShortStats - 1]
                  .current.year;
    other = (gStatData)
                ->shortStats[kStatFaceTimeOther - kBeginShortStats - 1]
                .current.year;
    break;
  }
  total = read + compose + other;
  readPercent = read * 100 / total;
  composePercent = compose * 100 / total;
  otherPercent = 100 - readPercent - composePercent;
  RightAlign7(readPercent, sRead);
  RightAlign7(composePercent, sCompose);
  RightAlign7(otherPercent, sOther);
  AccuComposeR(a, StatHTMLStrn + sStatUsageAct, data->rThis, sRead, sCompose,
               sOther);

  // add a graph
  Zero(theSeries);
  labels[0] = GetRString(sRead, StatHTMLStrn + sStatReading);
  labels[1] = GetRString(sCompose, StatHTMLStrn + sStatComposing);
  labels[2] = GetRString(sOther, StatHTMLStrn + sStatOther);
  if (InitGraphPict(&graphData, theSeries, kStatFaceTime, labels)) {
    graphData.type = kPieGraph;
    graphData.dataCount = 3;

    theSeries[0].data = series;
    series[0] = readPercent;
    series[1] = composePercent;
    series[2] = otherPercent;

    char *b64 = FinishGraphPict(&graphData);
    if (b64) {
      AccuAddStr(a, (unsigned char *)"<br><img src=\"data:image/png;base64,");
      AccuAddStr(a, (unsigned char *)b64);
      AccuAddStr(a, (unsigned char *)"\"><br>\n");
      g_free(b64);
    }
  }
}

/************************************************************************
 * ComposeShortStats - compose a set of statistics
 ************************************************************************/
static void ComposeShortStats(AccuPtr a, StatType type, short strType,
                              ComposeStatsData *data, short rObject,
                              bool percent) {
  uLong thisVal, lastVal, aveVal;
  char sThisVal[33], sLastVal[33], sAveVal[33], sTotalVal[33];
  float dayAve, secs, recvMailAve;
  ShortNumericStats *stats;
  NumericStats *recvMailStats;

  secs = LocalDateTime() - gStatData->startTime;
  //	average is times 10 so we can do a single decimal place
  stats = &gStatData->shortStats[type - kBeginShortStats - 1];
  dayAve = secs ? 10 * 24.0 * 60.0 * 60.0 * (float)stats->total / secs : 0;
  if (type == kStatReadMsg) {
    recvMailStats = &gStatData->numStats[kStatReceivedMail];
    recvMailAve =
        secs ? 24.0 * 60.0 * 60.0 * (float)recvMailStats->total / secs : 0;
    aveVal = dayAve * 100 / recvMailAve;
  }
  switch (data->period) {
  case kStatDay:
    thisVal = stats->current.day;
    lastVal = stats->last.day;
    if (percent) {
      thisVal =
          thisVal * 100 / SumArray(recvMailStats->current.day, kDayStatCount);
      lastVal =
          lastVal * 100 / SumArray(recvMailStats->last.day, kDayStatCount);
    } else
      aveVal = dayAve;
    break;

  case kStatWeek:
    thisVal = stats->current.week;
    lastVal = stats->last.week;
    if (percent) {
      thisVal =
          thisVal * 100 / SumArray(recvMailStats->current.week, kWeekStatCount);
      lastVal =
          lastVal * 100 / SumArray(recvMailStats->last.week, kWeekStatCount);
    } else
      aveVal = 7.0 * dayAve;
    break;

  case kStatMonth:
    thisVal = stats->current.month;
    lastVal = stats->last.month;
    if (percent) {
      thisVal = thisVal * 100 /
                SumArray(recvMailStats->current.month, kMonthStatCount);
      lastVal =
          lastVal * 100 / SumArray(recvMailStats->last.month, kMonthStatCount);
    } else
      aveVal = 30.4375 * dayAve;
    break;

  case kStatYear:
    thisVal = stats->current.year;
    lastVal = stats->last.year;
    if (percent) {
      thisVal =
          thisVal * 100 / SumArray(recvMailStats->current.year, kYearStatCount);
      lastVal =
          lastVal * 100 / SumArray(recvMailStats->last.year, kYearStatCount);
    } else
      aveVal = 365.25 * dayAve;
    break;
  }
  if (percent) {
    //	make sure average values don't exceen 100%
    if (thisVal > 100)
      thisVal = 100;
    if (lastVal > 100)
      lastVal = 100;
    if (aveVal > 1000)
      aveVal = 1000; // average is times 10
  }
  RightAlign7(thisVal, sThisVal);
  RightAlign7(lastVal, sLastVal);
  RightAlign7Dec(aveVal, sAveVal);
  if (!data->showAverage)
    memset(sAveVal + 1, ' ', *sAveVal); // replace with spaces
  RightAlign7(stats->total, sTotalVal);
  if (percent) {
    AccuComposeR(a, StatHTMLStrn + strType, sThisVal,
                 WordStrn(rObject, percent ? 2 : thisVal), data->rThis,
                 sLastVal, WordStrn(rObject, percent ? 2 : lastVal),
                 data->rLast, sAveVal, sTotalVal,
                 WordStrn(rObject, stats->total));
  } else {
    AccuComposeR(a, StatHTMLStrn + strType, sThisVal,
                 WordStrn(rObject, percent ? 2 : thisVal), data->rThis,
                 sLastVal, WordStrn(rObject, percent ? 2 : lastVal),
                 data->rLast, sAveVal, data->rPeriod, data->sProjected,
                 sTotalVal, WordStrn(rObject, stats->total));
  }
}

/************************************************************************
 * ComposeNumStats - compose a set of statistics
 ************************************************************************/
static void ComposeNumStats(AccuPtr a, StatType type, short strType,
                            ComposeStatsData *data) {
  uLong values[4];
  void *hPict;
  char sThisVal[33], sLastVal[33], sAveVal[33], sTotalVal[33];
  char sThisPeriod[64], sLastPeriod[64];

  if (type == kStatJunkPercent)
    GetStatTotalPercents(&gStatData->numStats[kStatJunkTotal],
                         &gStatData->numStats[kStatTotal], data->period,
                         values);
  else
    GetStatTotals(&gStatData->numStats[type], data->period, values,
                  type != kStatJunkPercent ? gStatData->startTime
                                           : gStatData->junkStartTime);

  if (type == kStatFaceTime) {
    //	Usage
    RightAlign7Dec(values[kThisStat] / 360, sThisVal);
    RightAlign7Dec(values[kLastStat] / 360, sLastVal);
    RightAlign7Dec(values[kAveStat] / 3600, sAveVal);
    if (!data->showAverage)
      memset(sAveVal + 1, ' ', *sAveVal); // replace with spaces
    RightAlign7Dec(values[kTotalStat] / 360, sTotalVal);
    AccuComposeR(a, StatHTMLStrn + sStatUsageEntry, sThisVal, data->rThis,
                 sLastVal, data->rLast, sAveVal, data->rPeriod,
                 data->sProjected, sTotalVal);
  } else {
    short fmt = data->showAverage ? sStatMsgsEntry : sStatNoAveEntry;
    short unit = type == kStatJunkPercent ? sStatPercentZero : sStatMsgZero;
    RightAlign7(values[kThisStat], sThisVal);
    RightAlign7(values[kLastStat], sLastVal);
    RightAlign7Dec(values[kAveStat], sAveVal);
    RightAlign7(values[kTotalStat], sTotalVal);
    AccuComposeR(a, StatHTMLStrn + fmt, StatHTMLStrn + strType, sThisVal,
                 WordStrn(StatHTMLStrn + unit, values[kThisStat]), data->rThis,
                 sLastVal, WordStrn(StatHTMLStrn + unit, values[kLastStat]),
                 data->rLast, sAveVal, data->rPeriod, data->sProjected,
                 sTotalVal, WordStrn(StatHTMLStrn + unit, values[kTotalStat]));
  }

  // add a graph
  char *b64 = GetGraphPict(&gStatData->numStats[type], data->period,
                           type == kStatFaceTime, data->showAverage,
                           GetRString(sThisPeriod, data->rThis),
                           GetRString(sLastPeriod, data->rLast),
                           GetRString(sAveVal, StatHTMLStrn + sStatAverage));
  if (b64) {
    AccuAddStr(a, (unsigned char *)"<br><img src=\"data:image/png;base64,");
    AccuAddStr(a, (unsigned char *)b64);
    AccuAddStr(a, (unsigned char *)"\"><br>\n");
    g_free(b64);
  }
}

/************************************************************************
 * WordStrn - return correct singular or plural form of string resource
 ************************************************************************/
static short WordStrn(short rStrn, long value) {
  if (value == 0)
    return rStrn;
  else if (value == 1)
    return rStrn + 1;
  else
    return rStrn + 2;
}

/************************************************************************
 * RightAlign7 - right align with 7 chars in field
 ************************************************************************/
static unsigned char * RightAlign7(uLong value, unsigned char * s) {
  return RightAlign(value, s, 7, 0);
}

/************************************************************************
 * RightAlign7Dec - right align with 7 chars in field and use decimal place
 ************************************************************************/
static unsigned char * RightAlign7Dec(uLong value, unsigned char * s) {
  return RightAlign(value, s, 7, 1);
}

/************************************************************************
 * RightAlign - right align with X chars in field, and add any commas
 ************************************************************************/
static unsigned char * RightAlign(uLong value, unsigned char * s, short minLen, short decPlace) {
  short len;
  char cTokThousands, cDecPoint;

  if (decPlace && value >= 100) {
    // number is big enough we aren't going to worry about decimal
    while (decPlace > 0) {
      value /= 10;
      decPlace--;
    }
  }
  sprintf(s, "%ld", (long)(value));
  if (decPlace && (cDecPoint = GetIntlNumberPart(1))) {
    if (*s == 1)
      PInsertC(s, sizeof(s), '0', s + 1);
    PInsertC(s, sizeof(s), cDecPoint, s + *s - decPlace + 1);
  }

  //  Put in some commas if large number
  len = *s;
  if ((cTokThousands = GetIntlNumberPart(2))) {
    if (len > 9)
      PInsertC(s, 32, cTokThousands, s + *s - 8);
    if (len > 6)
      PInsertC(s, 32, cTokThousands, s + *s - 5);
    if (len > 3)
      PInsertC(s, 32, cTokThousands, s + *s - 2);
  }
  while (*s < minLen)
    PInsertC(s, 32, ' ', s + 1);
  return s;
}

/************************************************************************
 * GetGraphPict - make a graph
 ************************************************************************/
static char *GetGraphPict(NumericStats *stats, StatTimePeriod period,
                          bool seconds, bool showAverage, char * sThis,
                          char * sLast, char * sAverage) {
  uLong aveVals[31], *aveRaw;
  short i;
  GraphData data;
  SeriesInfo theSeries[3];
  char * labels[3];
  DateTimeRec dtStart, dtNow;
  uLong elapsedUnits;
  bool doAverage = true;
  short compare1;
  labels[0] = sThis;
  labels[1] = sLast;
  labels[2] = sAverage;
  if (InitGraphPict(&data, theSeries, period, labels)) {
    data.xLabels = GetXLabels(period);
    if (seconds) {
      data.divider = 360; // 60*60/10
      data.decPlace = 1;
    }

    theSeries[0].type = GetRLong(STAT_CURRENT_TYPE);
    theSeries[1].type = GetRLong(STAT_PREVIOUS_TYPE);
    theSeries[2].type = GetRLong(STAT_AVERAGE_TYPE);

    SecondsToDate(LocalDateTime(), &dtNow);
    SecondsToDate(gStatData->startTime, &dtStart);
    dtNow.minute = dtNow.second = 0;
    dtStart.minute = dtStart.second = 0;

    elapsedUnits = CalcElapsedUnits(period);

    switch (period) {
    case kStatDay:
      theSeries[0].data = stats->current.day;
      theSeries[1].data = stats->last.day;
      aveRaw = stats->average.day;
      compare1 = dtStart.hour + 1;
      theSeries[0].dataCount = dtNow.hour + 1;
      break;
    case kStatWeek:
      theSeries[0].data = stats->current.week;
      theSeries[1].data = stats->last.week;
      aveRaw = stats->average.week;
      compare1 = dtStart.dayOfWeek - 1;
      theSeries[0].dataCount = dtNow.dayOfWeek;
      break;
    case kStatMonth:
      theSeries[0].data = stats->current.month;
      theSeries[1].data = stats->last.month;
      aveRaw = stats->average.month;
      compare1 = dtStart.day - 1;
      theSeries[0].dataCount = dtNow.day;
      break;
    case kStatYear:
      theSeries[0].data = stats->current.year;
      theSeries[1].data = stats->last.year;
      aveRaw = stats->average.year;
      compare1 = dtStart.month - 1;
      theSeries[0].dataCount = dtNow.month;
      break;
    }

    // calculate averages
    if (elapsedUnits && showAverage) {
      short divider;

      for (i = 0; i < dataCountTab[period]; i++) {
        divider = elapsedUnits;
        if (i < compare1 && divider > 1)
          divider--;
        aveVals[i] = (float)aveRaw[i] * 10.0 / divider;
      }
      theSeries[2].data = aveVals;
      theSeries[2].divider = 10; // increase graph resolution
    } else
      data.seriesCount = 2; //	not enough data to do average yet

    return FinishGraphPict(&data);
  }
  return NULL;
}

/************************************************************************
 * CalcElapsedUnits - how many time periods since beginning of stats?
 ************************************************************************/
static uLong CalcElapsedUnits(StatTimePeriod period) {
  uLong startSecs, nowSecs, elapsedSecs;
  DateTimeRec dtStart, dtNow;
  uLong elapsedUnits;

  SecondsToDate(LocalDateTime(), &dtNow);
  SecondsToDate(gStatData->startTime, &dtStart);
  dtNow.minute = dtNow.second = 0;
  dtStart.minute = dtStart.second = 0;

  switch (period) {
  case kStatDay:
    dtStart.hour++;
    elapsedSecs = nowSecs > startSecs ? nowSecs - startSecs : 0;
    elapsedUnits = elapsedSecs / (24 * 60 * 60);
    break;
  case kStatWeek:
    dtNow.hour = dtStart.hour = 0;
    dtStart.day++;
    elapsedSecs = nowSecs > startSecs ? nowSecs - startSecs : 0;
    elapsedUnits = elapsedSecs / (7 * 24 * 60 * 60);
    break;
  case kStatMonth:
    elapsedUnits = 12 * (dtNow.year - dtStart.year);
    dtStart.year = dtNow.year;
    if (TimeCompare(&dtNow, &dtStart) < 0) {
      elapsedUnits -= 12;
      dtNow.month += 12;
    }
    if (dtNow.month != dtStart.month)
      elapsedUnits += dtNow.month - dtStart.month - 1;
    break;
  case kStatYear:
    elapsedUnits = dtNow.year - dtStart.year;
    dtStart.year = dtNow.year;
    if (TimeCompare(&dtNow, &dtStart) < 0)
      elapsedUnits--;
    break;
  }

  return elapsedUnits;
}

/************************************************************************
 * InitGraphPict - make a graph
 ************************************************************************/
static bool InitGraphPict(GraphData *data, SeriesInfo *theSeries,
                          StatTimePeriod period, char * *labels) {
  Zero(*data);
  SetRect(&data->bounds, 0, 0, GetRLong(STAT_GRAPH_WIDTH),
          GetRLong(STAT_GRAPH_HEIGHT));
  data->type = kLineGraph;
  data->seriesCount = 3;
  data->dataCount = dataCountTab[period];
  data->series = theSeries;
  data->xLabels = GetXLabels(period);
  data->legend = 1;

  GetRColor(&theSeries[0].color, STAT_CURRENT_COLOR);
  theSeries[0].dataCount = data->dataCount;
  theSeries[0].label = (unsigned char *)labels[0];
  theSeries[0].divider = 0;

  GetRColor(&theSeries[1].color, STAT_PREVIOUS_COLOR);
  theSeries[1].dataCount = data->dataCount;
  theSeries[1].label = (unsigned char *)labels[1];
  theSeries[1].divider = 0;

  GetRColor(&theSeries[2].color, STAT_AVERAGE_COLOR);
  theSeries[2].dataCount = data->dataCount;
  theSeries[2].label = (unsigned char *)labels[2];
  theSeries[2].divider = 0;

  if (period == kStatDay) {
    data->largeTickInterval = 4;
    data->medTickInterval = 1;
  } else if (period == kStatMonth) {
    data->largeTickInterval = 5;
    data->medTickInterval = 1;
  } else {
    data->largeTickInterval = data->medTickInterval = 0;
  }

  return true;
}

/************************************************************************
 * FinishGraphPict - finish up graph PICT
 ************************************************************************/
static cairo_status_t write_to_gstring(void *closure, const unsigned char *data,
                                       unsigned int length) {
  g_string_append_len((GString *)closure, (const gchar *)data, length);
  return CAIRO_STATUS_SUCCESS;
}

static char *FinishGraphPict(GraphData *data) {
  int w = data->bounds.right - data->bounds.left;
  int h = data->bounds.bottom - data->bounds.top;
  if (w <= 0 || h <= 0)
    return NULL;

  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  cairo_t *cr = cairo_create(surf);

  // white background
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);

  DrawGraph(cr, data);
  cairo_destroy(cr);

  GString *out = g_string_new("");
  cairo_surface_write_to_png_stream(surf, write_to_gstring, out);
  cairo_surface_destroy(surf);

  gchar *b64 = g_base64_encode((guchar *)out->str, out->len);
  g_string_free(out, TRUE);
  return b64;
}

/************************************************************************
 * GetXLabels - generate the X-axis labels
 ************************************************************************/
static void *GetXLabels(StatTimePeriod period) {
  static void *labelList[4];
  char s[256];
  int err = 0;

  if (!labelList[period]) {
    // need to generate stringlist
    Accumulator a;
    short i;

    switch (period) {
    case kStatDay:
      labelList[period] = GetResource('STR#', StatHoursStrn);
      break;

    case kStatMonth:
      if (AccuInit(&a))
        return nil;
      AccuAddPtr(&a, dataCountTab + period, sizeof(short)); //	count
      for (i = 1; i <= dataCountTab[period]; i++) {
        if (i % 5) //	display every 5th value
          *s = 0;
        else
          sprintf(s, "%ld", (long)(i));
        if ((err = AccuAddPtr(&a, &s, *s + 1)))
          break;
      }
      if (err) {
        free(a.data); a.data = NULL; a.offset = a.size = 0;
      }
      else {
        AccuTrim(&a);
        labelList[period] = a.data;
      }
      break;

    case kStatWeek:
    case kStatYear:
      //	This is Pete's suggested way to get these
      //	Get short names for days of week or months
      GetAbbrevNames(&labelList[kStatYear], &labelList[kStatWeek]);
      break;
    }
  }
  return labelList[period];
}

/************************************************************************
 * GetAbbrevNames - get abbreviated month and weekday names
 ************************************************************************/
static int GetAbbrevNames(void **monthNames, void **dayNames) {
  const char *m[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const char *d[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

  *monthNames = malloc(2 + 12 * 4);
  *dayNames = malloc(2 + 7 * 4);

  if (!*monthNames || !*dayNames)
    return ENOMEM;

  *(short *)*monthNames = 12;
  char *ptr = (char *)*monthNames + 2;
  for (int i = 0; i < 12; i++) {
    int l = strlen(m[i]);
    *ptr++ = l;
    memcpy(ptr, m[i], l);
    ptr += l;
  }

  *(short *)*dayNames = 7;
  ptr = (char *)*dayNames + 2;
  for (int i = 0; i < 7; i++) {
    int l = strlen(d[i]);
    *ptr++ = l;
    memcpy(ptr, d[i], l);
    ptr += l;
  }

  return 0;
}

/************************************************************************
 * SaveStats - save stats to file in XML format
 ************************************************************************/
static void ParseStatFile(void *hStatXML) {
  // FIXME: Port Eudora XML stats parser to GMarkup
  // The Mac version used TokenInfo and GetNextToken from xml.h which is
  // unported
}

/************************************************************************
 * GetIntlNumberPart - get an international number part
 ************************************************************************/
char GetIntlNumberPart(short charToken) {
  // Use U.S. defaults since Mac number parts unported
  switch (charToken) {
  case 1:
    return '.';
  case 2:
    return ',';
  }
  return 0;
}

/************************************************************************
 * UpdateCalcStats - update calculated status
 ************************************************************************/
void UpdateCalcStats(void) {
  long oldTotal;

  // Take the ones we score as junk...
  gStatData->numStats[kStatJunkTotal] =
      gStatData->numStats[kStatScoredJunk];

  // Subtract the false positives (they weren't really junk)
  NumericStatOperation(kStatJunkTotal, kStatFalsePositives, opMinus);

  // Add the false negatives (they were junk, but the scoring missed them)
  NumericStatOperation(kStatJunkTotal, kStatFalseNegatives, opPlus);

  // Add the junk that was on the white list and so didn't get recognized as
  // junk
  NumericStatOperation(kStatJunkTotal, kStatFalseWhiteList, opPlus);

  // the result is all the junk we got!

  // Take the ones we didn't score as junk...
  gStatData->numStats[kStatTotal] =
      gStatData->numStats[kStatScoredNotJunk];

  // Add the whitelist
  NumericStatOperation(kStatTotal, kStatWhiteList, opPlus);

  // Add the scored junk
  NumericStatOperation(kStatTotal, kStatScoredJunk, opPlus);

  // The result is all eligible mail

  // Compute percentages
  gStatData->numStats[kStatJunkPercent] =
      gStatData->numStats[kStatJunkTotal];
  oldTotal = gStatData->numStats[kStatJunkPercent].total;
  NumericStatOperation(kStatJunkPercent, kStatTotal, opPercent);
  gStatData->numStats[kStatJunkPercent].total = oldTotal;
}

/************************************************************************
 * NumericStatOperation - add or subtract one numeric stat from another
 ************************************************************************/
void NumericStatOperation(StatType destination, StatType increment, OpType op) {
  PeriodicStatOperation(&gStatData->numStats[destination].current,
                        &gStatData->numStats[increment].current, op);
  PeriodicStatOperation(&gStatData->numStats[destination].last,
                        &gStatData->numStats[increment].last, op);
  PeriodicStatOperation(&gStatData->numStats[destination].average,
                        &gStatData->numStats[increment].average, op);

  switch (op) {
  case opPlus:
    gStatData->numStats[destination].total +=
        gStatData->numStats[increment].total;
    break;
  case opMinus:
    gStatData->numStats[destination].total -=
        gStatData->numStats[increment].total;
    break;
  case opPercent:
    gStatData->numStats[destination].total =
        gStatData->numStats[increment].total
            ? (100 * gStatData->numStats[destination].total) /
                  gStatData->numStats[increment].total
            : 0;
    break;
  }
}

/************************************************************************
 * PeriodicStatOperation - add or subtract one periodic stat from another
 ************************************************************************/
void PeriodicStatOperation(PeriodicStats *destP, PeriodicStats *incP,
                           OpType op) {
  LongArrayOperation(destP->day, incP->day,
                     sizeof(incP->day) / sizeof(incP->day[0]), op);
  LongArrayOperation(destP->week, incP->week,
                     sizeof(incP->week) / sizeof(incP->week[0]), op);
  LongArrayOperation(destP->month, incP->month,
                     sizeof(incP->month) / sizeof(incP->month[0]), op);
  LongArrayOperation(destP->year, incP->year,
                     sizeof(incP->year) / sizeof(incP->year[0]), op);
}

/************************************************************************
 * LongArrayOperation - add or subtract one array of longs from another
 ************************************************************************/
void LongArrayOperation(long *array, long *sub, short n, OpType op) {
  switch (op) {
  case opPlus:
    while (n-- > 0)
      *array++ += *sub++;
    break;
  case opMinus:
    while (n-- > 0)
      *array++ -= *sub++;
    break;
  case opPercent:
    for (; n-- > 0; array++, sub++)
      *array = (100 * (*array)) / (*sub);
    break;
  }
}

/************************************************************************
 * Accessor functions for statwin.c GTK display
 ************************************************************************/
bool StatDataLoaded(void) {
  return gStatData != NULL;
}

long StatGetTotal(StatType which) {
  if (!gStatData) return 0;
  StatDataPtr p = gStatData;
  if (which < kStatCount)
    return p->numStats[which].total;
  else if (which > kBeginShortStats && which < kEndStats)
    return p->shortStats[which - kBeginShortStats - 1].total;
  return 0;
}

/* Get current-period value for a numeric stat */
long StatGetCurrentPeriod(StatType which, StatTimePeriod period) {
  if (!gStatData || which >= kStatCount) return 0;
  NumericStats *ns = &gStatData->numStats[which];
  switch (period) {
  case kStatDay:   return SumArrayHi(ns->current.day);
  case kStatWeek:  return SumArrayHi(ns->current.week);
  case kStatMonth: return SumArrayHi(ns->current.month);
  case kStatYear:  return SumArrayHi(ns->current.year);
  }
  return 0;
}

long StatGetLastPeriod(StatType which, StatTimePeriod period) {
  if (!gStatData || which >= kStatCount) return 0;
  NumericStats *ns = &gStatData->numStats[which];
  switch (period) {
  case kStatDay:   return SumArrayHi(ns->last.day);
  case kStatWeek:  return SumArrayHi(ns->last.week);
  case kStatMonth: return SumArrayHi(ns->last.month);
  case kStatYear:  return SumArrayHi(ns->last.year);
  }
  return 0;
}

/* Get current-period value for a short stat */
long StatGetShortCurrent(StatType which, StatTimePeriod period) {
  if (!gStatData || which <= kBeginShortStats || which >= kEndStats) return 0;
  ShortNumericStats *ss = &gStatData->shortStats[which - kBeginShortStats - 1];
  switch (period) {
  case kStatDay:   return ss->current.day;
  case kStatWeek:  return ss->current.week;
  case kStatMonth: return ss->current.month;
  case kStatYear:  return ss->current.year;
  }
  return 0;
}

long StatGetShortLast(StatType which, StatTimePeriod period) {
  if (!gStatData || which <= kBeginShortStats || which >= kEndStats) return 0;
  ShortNumericStats *ss = &gStatData->shortStats[which - kBeginShortStats - 1];
  switch (period) {
  case kStatDay:   return ss->last.day;
  case kStatWeek:  return ss->last.week;
  case kStatMonth: return ss->last.month;
  case kStatYear:  return ss->last.year;
  }
  return 0;
}

long StatGetStartTime(void) {
  if (!gStatData) return 0;
  return gStatData->startTime;
}

/* Get hourly breakdown for today (24 values) for a numeric stat */
void StatGetHourlyData(StatType which, long out[24]) {
  memset(out, 0, 24 * sizeof(long));
  if (!gStatData || which >= kStatCount) return;
  memcpy(out, gStatData->numStats[which].current.day, 24 * sizeof(long));
}

/* Get daily breakdown for this week (7 values) */
void StatGetWeeklyData(StatType which, long out[7]) {
  memset(out, 0, 7 * sizeof(long));
  if (!gStatData || which >= kStatCount) return;
  memcpy(out, gStatData->numStats[which].current.week, 7 * sizeof(long));
}