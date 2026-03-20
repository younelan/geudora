/* macmbx_build.c — Build TOC from mbox scan
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Scans mbox file for "From " separator lines, parses headers
 * (Date, From, Subject, Priority, Message-ID, Status, etc.),
 * builds per-message summaries.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <strings.h>

/* Portable strcasestr for strict C99 */
#ifndef _GNU_SOURCE
static const char *macmbx_strcasestr(const char *hay, const char *needle) {
  if (!hay || !needle) return NULL;
  size_t nlen = strlen(needle);
  if (nlen == 0) return hay;
  for (; *hay; hay++) {
    if (strncasecmp(hay, needle, nlen) == 0) return hay;
  }
  return NULL;
}
#define strcasestr macmbx_strcasestr
#endif

/* ================================================================
 * "From " line validation (ported from Eudora buildtoc.c)
 * ================================================================ */

bool macmbx_is_from_line(const char *line) {
  if (!line) return false;
  if (line[0] != 'F' || line[1] != 'r' || line[2] != 'o' || line[3] != 'm')
    return false;

  const char *lp = line + 4;
  if (*lp++ != ' ') return false;

  /* Skip return address (may contain quoted strings) */
  int quote = 0;
  while (*lp && (quote || *lp != ' ')) {
    if (*lp == '"') quote = !quote;
    lp++;
  }
  if (!*lp++) return false;
  while (*lp == ' ') lp++;

  /* Validate date portion: need day, month, year, time */
  int weekDay = 0, day = 0, month = 0, year = 0, tym = 0, other = 0;
  char scratch[256];
  int len = (int)strlen(lp);
  if (len >= (int)sizeof(scratch)) return false;
  strcpy(scratch, lp);

  static const char *month_names[] = {
    "jan","feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"
  };
  static const char *day_names[] = {
    "mon","tue","wed","thu","fri","sat","sun"
  };

  for (char *cp = strtok(scratch, " \t\r\n,"); cp; cp = strtok(NULL, " \t\r\n,")) {
    len = (int)strlen(cp);
    int num = atoi(cp);

    if (num < 24 && len >= 5 && cp[2] == ':' &&
        (len == 5 || (len == 8 && cp[5] == ':'))) {
      if (tym++) return false;
    } else if (!year && day && len == 2 && isdigit((unsigned char)cp[len-1])) {
      if (year++) return false;
    } else if (len <= 2 && num && num < 32) {
      if (day++) return false;
    } else if (len == 4 && num > 1900) {
      if (year++) return false;
    } else if (len == 3) {
      bool is_day = false, is_month = false;
      for (int i = 0; i < 7; i++)
        if (strcasecmp(cp, day_names[i]) == 0) { is_day = true; break; }
      for (int i = 0; i < 12; i++)
        if (strcasecmp(cp, month_names[i]) == 0) { is_month = true; break; }
      if (is_day) { if (weekDay++) return false; }
      else if (is_month) { if (month++) return false; }
      else other++;
    } else {
      other++;
    }
  }
  return (day && year && month && tym && other <= 2);
}

/* ================================================================
 * Date parsing helpers
 * ================================================================ */

static int month_num(const char *s) {
  static const char *m[] = {"jan","feb","mar","apr","may","jun",
                            "jul","aug","sep","oct","nov","dec"};
  char lower[4];
  for (int i = 0; i < 3 && s[i]; i++) lower[i] = tolower((unsigned char)s[i]);
  lower[3] = '\0';
  for (int i = 0; i < 12; i++)
    if (strcmp(lower, m[i]) == 0) return i;
  return -1;
}

static long parse_tz_offset(const char *s) {
  if (!s) return 0;
  while (*s == ' ' || *s == '\t') s++;
  if (*s == '+' || *s == '-') {
    int sign = (*s == '-') ? -1 : 1;
    s++;
    long val = strtol(s, NULL, 10);
    if (val > 2400 || val < -2400) return 0;
    return sign * ((val / 100) * 3600 + (val % 100) * 60);
  }
  /* Common symbolic timezones */
  if (strncasecmp(s, "GMT", 3) == 0 || strncasecmp(s, "UTC", 3) == 0) return 0;
  if (strncasecmp(s, "EST", 3) == 0) return -5*3600;
  if (strncasecmp(s, "EDT", 3) == 0) return -4*3600;
  if (strncasecmp(s, "CST", 3) == 0) return -6*3600;
  if (strncasecmp(s, "CDT", 3) == 0) return -5*3600;
  if (strncasecmp(s, "MST", 3) == 0) return -7*3600;
  if (strncasecmp(s, "MDT", 3) == 0) return -6*3600;
  if (strncasecmp(s, "PST", 3) == 0) return -8*3600;
  if (strncasecmp(s, "PDT", 3) == 0) return -7*3600;
  if (strncasecmp(s, "CET", 3) == 0) return 3600;
  if (strncasecmp(s, "CEST", 4) == 0) return 7200;
  return 0;
}

/* Parse RFC822/mbox date string to UTC seconds */
static uint32_t parse_date(const char *date, long *tz_secs) {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  if (tz_secs) *tz_secs = 0;
  if (!date || !*date) return 0;

  const char *s = date;
  /* Skip "day, " prefix */
  if (isalpha((unsigned char)s[0]) && isalpha((unsigned char)s[1]) &&
      isalpha((unsigned char)s[2])) {
    const char *comma = strchr(s, ',');
    if (comma && comma - s <= 10) s = comma + 1;
    else {
      /* Mbox format: "Wed Jun 14 12:36:18 2023" */
      s += 3;
    }
  }
  while (*s == ' ' || *s == '\t') s++;

  /* Try "dd Mon yyyy hh:mm:ss" */
  int day = 0, mon = -1, year = 0, h = 0, m = 0, sec = 0;

  if (isdigit((unsigned char)*s)) {
    day = (int)strtol(s, (char **)&s, 10);
    while (*s == ' ' || *s == '-') s++;
  }

  if (isalpha((unsigned char)*s)) {
    mon = month_num(s);
    while (isalpha((unsigned char)*s)) s++;
    while (*s == ' ' || *s == '-') s++;
  } else if (mon < 0 && day > 0 && day <= 12) {
    /* Maybe mm/dd/yyyy */
    mon = day - 1;
    day = 0;
  }

  if (isdigit((unsigned char)*s)) {
    if (!day) day = (int)strtol(s, (char **)&s, 10);
    else year = (int)strtol(s, (char **)&s, 10);
    while (*s == ' ' || *s == '-' || *s == '/') s++;
  }

  if (!year && isdigit((unsigned char)*s)) {
    year = (int)strtol(s, (char **)&s, 10);
    while (*s == ' ') s++;
  }

  /* Time */
  if (isdigit((unsigned char)*s)) {
    h = (int)strtol(s, (char **)&s, 10);
    if (*s == ':') { s++; m = (int)strtol(s, (char **)&s, 10); }
    if (*s == ':') { s++; sec = (int)strtol(s, (char **)&s, 10); }
    while (*s == ' ') s++;
  }

  /* Year after time (mbox format) */
  if (!year && isdigit((unsigned char)*s)) {
    year = (int)strtol(s, (char **)&s, 10);
  }

  if (year < 100) year += (year < 70) ? 2000 : 1900;
  if (mon < 0 || day < 1 || year < 1970) return 0;

  tm.tm_year = year - 1900;
  tm.tm_mon = mon;
  tm.tm_mday = day;
  tm.tm_hour = h;
  tm.tm_min = m;
  tm.tm_sec = sec;
  tm.tm_isdst = -1;

  time_t secs = mktime(&tm);
  if (secs == (time_t)-1) return 0;

  /* Timezone */
  while (*s == ' ') s++;
  long tz = parse_tz_offset(s);
  if (tz_secs) *tz_secs = tz;

  /* mktime gave local time — adjust to account for parsed tz */
  time_t now = time(NULL);
  long local_tz = (long)(mktime(gmtime(&now)) - now);
  /* Actually simpler: timegm if available, otherwise adjust */
  return (uint32_t)(secs + local_tz - tz);
}

/* ================================================================
 * Simple string hash (djb2)
 * ================================================================ */

static uint32_t hash_string(const char *s, int len) {
  uint32_t h = 5381;
  for (int i = 0; i < len; i++)
    h = ((h << 5) + h) + (unsigned char)s[i];
  return h ? h : 1;
}

/* ================================================================
 * Header value extraction
 * ================================================================ */

static void extract_header_value(const char *line, char *out, int outSz) {
  const char *colon = strchr(line, ':');
  if (!colon) { out[0] = '\0'; return; }
  colon++;
  while (*colon == ' ' || *colon == '\t') colon++;
  int len = (int)strlen(colon);
  while (len > 0 && (colon[len-1] == '\r' || colon[len-1] == '\n')) len--;
  if (len >= outSz) len = outSz - 1;
  memcpy(out, colon, len);
  out[len] = '\0';
}

/* Beautify a From address: "Name <addr>" → "Name", "<addr>" → "addr" */
static void beautify_from(char *from) {
  if (!from || !*from) return;
  /* Strip leading whitespace */
  char *s = from;
  while (*s == ' ' || *s == '\t') s++;

  char *lt = strchr(s, '<');
  char *gt = lt ? strchr(lt, '>') : NULL;
  if (lt && gt) {
    /* Check for display name before < */
    char *nameEnd = lt;
    while (nameEnd > s && (nameEnd[-1] == ' ' || nameEnd[-1] == '\t')) nameEnd--;
    if (nameEnd > s) {
      char *start = s;
      if (*start == '"' && nameEnd > start + 1 && nameEnd[-1] == '"') {
        start++; nameEnd--;
      }
      int len = (int)(nameEnd - start);
      if (len > 0) {
        memmove(from, start, len);
        from[len] = '\0';
        return;
      }
    }
    /* No name — use address */
    int len = (int)(gt - lt - 1);
    if (len > 0) {
      memmove(from, lt + 1, len);
      from[len] = '\0';
      return;
    }
  }
  if (s != from) memmove(from, s, strlen(s) + 1);
}

/* ================================================================
 * Build TOC from mbox
 * ================================================================ */

MacmbxTOC *macmbx_toc_build(const char *mbox_path) {
  if (!mbox_path) return NULL;

  FILE *f = fopen(mbox_path, "rb");
  if (!f) return NULL;

  MacmbxTOC *toc = (MacmbxTOC *)calloc(1, sizeof(MacmbxTOC));
  if (!toc) { fclose(f); return NULL; }

  snprintf(toc->mbox_path, sizeof(toc->mbox_path), "%s", mbox_path);
  char toc_path[PATH_MAX];
  snprintf(toc_path, sizeof(toc_path), "%s.toc", mbox_path);
  snprintf(toc->toc_path, sizeof(toc->toc_path), "%s", toc_path);
  toc->next_serial = 1;
  toc->lock_fd = -1;
  toc->major_version = MACMBX_TOC_MAJOR;
  toc->minor_version = MACMBX_TOC_MINOR;
  toc->dirty = true;

  int capacity = 64;
  toc->msgs = (MacmbxMsgSum *)calloc(capacity, sizeof(MacmbxMsgSum));
  if (!toc->msgs) { free(toc); fclose(f); return NULL; }
  toc->capacity = capacity;

  char line[4096];
  long offset = 0;
  MacmbxMsgSum cur;
  bool in_header = false;
  bool have_msg = false;
  int sender_priority = 99;

  memset(&cur, 0, sizeof(cur));

  while (fgets(line, sizeof(line), f)) {
    long line_start = offset;
    offset = ftell(f);

    if (macmbx_is_from_line(line)) {
      /* Save previous message if any */
      if (have_msg) {
        cur.length = line_start - cur.offset;
        if (!cur.from[0]) snprintf(cur.from, sizeof(cur.from), "unknown");
        beautify_from(cur.from);
        cur.serial_num = toc->next_serial++;
        if (toc->count >= toc->capacity) {
          toc->capacity *= 2;
          toc->msgs = (MacmbxMsgSum *)realloc(toc->msgs,
            toc->capacity * sizeof(MacmbxMsgSum));
        }
        toc->msgs[toc->count++] = cur;
      }

      /* Start new message */
      memset(&cur, 0, sizeof(cur));
      cur.offset = line_start;
      cur.state = MACMBX_UNREAD;
      cur.priority = 3;
      cur.spam_score = -1;
      cur.flags = MACMBX_FLAG_UTF8;
      sender_priority = 99;

      /* Extract sender and date from From line */
      char *sp = line + 5; /* skip "From " */
      char *ep = sp;
      while (*ep && *ep != ' ') ep++;
      int alen = (int)(ep - sp);
      if (alen >= (int)sizeof(cur.from)) alen = sizeof(cur.from) - 1;
      memcpy(cur.from, sp, alen);
      cur.from[alen] = '\0';

      while (*ep == ' ') ep++;
      /* Strip trailing CR/LF */
      char *end = ep + strlen(ep);
      while (end > ep && (end[-1] == '\r' || end[-1] == '\n')) end--;
      *end = '\0';

      long tz = 0;
      cur.seconds = parse_date(ep, &tz);
      cur.orig_zone = (int16_t)(tz / 60);
      cur.arrival = cur.seconds;

      in_header = true;
      have_msg = true;
      continue;
    }

    if (in_header) {
      if (line[0] == '\r' || line[0] == '\n') {
        /* Blank line = end of headers */
        in_header = false;
        cur.body_offset = (int)(offset - cur.offset);
        continue;
      }

      char val[256];

      if (strncasecmp(line, "Date:", 5) == 0) {
        extract_header_value(line, val, sizeof(val));
        long tz = 0;
        uint32_t secs = parse_date(val, &tz);
        if (secs) {
          cur.seconds = secs;
          cur.orig_zone = (int16_t)(tz / 60);
        }
      } else if (strncasecmp(line, "Subject:", 8) == 0) {
        extract_header_value(line, cur.subject, sizeof(cur.subject));
      } else if (strncasecmp(line, "From:", 5) == 0 && sender_priority > 1) {
        extract_header_value(line, cur.from, sizeof(cur.from));
        sender_priority = 1;
      } else if (strncasecmp(line, "Sender:", 7) == 0 && sender_priority > 2) {
        extract_header_value(line, cur.from, sizeof(cur.from));
        sender_priority = 2;
      } else if (strncasecmp(line, "Reply-To:", 9) == 0 && sender_priority > 3) {
        extract_header_value(line, cur.from, sizeof(cur.from));
        sender_priority = 3;
      } else if (strncasecmp(line, "X-Priority:", 11) == 0) {
        extract_header_value(line, val, sizeof(val));
        int pri = atoi(val);
        if (pri >= 1 && pri <= 5) cur.priority = (uint8_t)pri;
      } else if (strncasecmp(line, "Importance:", 11) == 0) {
        extract_header_value(line, val, sizeof(val));
        if (strcasestr(val, "high")) cur.priority = 1;
        else if (strcasestr(val, "low")) cur.priority = 5;
      } else if (strncasecmp(line, "Message-ID:", 11) == 0 ||
                 strncasecmp(line, "Message-Id:", 11) == 0) {
        extract_header_value(line, val, sizeof(val));
        char *id = val;
        if (*id == '<') id++;
        char *idEnd = id + strlen(id);
        if (idEnd > id && idEnd[-1] == '>') idEnd--;
        cur.msg_id_hash = hash_string(id, (int)(idEnd - id));
        if (!cur.uid_hash) cur.uid_hash = cur.msg_id_hash;
      } else if (strncasecmp(line, "In-Reply-To:", 12) == 0) {
        extract_header_value(line, val, sizeof(val));
        char *id = val;
        if (*id == '<') id++;
        char *idEnd = id + strlen(id);
        if (idEnd > id && idEnd[-1] == '>') idEnd--;
        if (idEnd > id) cur.in_reply_to_hash = hash_string(id, (int)(idEnd - id));
      } else if (strncasecmp(line, "Status:", 7) == 0) {
        extract_header_value(line, val, sizeof(val));
        if (strchr(val, 'R')) cur.state = MACMBX_READ;
      } else if (strncasecmp(line, "Content-Type:", 13) == 0) {
        if (strcasestr(line, "text/html")) cur.flags |= MACMBX_FLAG_HTML;
      } else if (strncasecmp(line, "Precedence:", 11) == 0) {
        extract_header_value(line, val, sizeof(val));
        if (strcasestr(val, "bulk") || strcasestr(val, "list") || strcasestr(val, "junk"))
          cur.flags |= MACMBX_FLAG_BULK;
      }
    }
  }

  /* Save last message */
  if (have_msg) {
    fseek(f, 0, SEEK_END);
    cur.length = ftell(f) - cur.offset;
    if (!cur.from[0]) snprintf(cur.from, sizeof(cur.from), "unknown");
    beautify_from(cur.from);
    cur.serial_num = toc->next_serial++;
    if (toc->count >= toc->capacity) {
      toc->capacity *= 2;
      toc->msgs = (MacmbxMsgSum *)realloc(toc->msgs,
        toc->capacity * sizeof(MacmbxMsgSum));
    }
    toc->msgs[toc->count++] = cur;
  }

  fclose(f);
  return toc;
}

/* ================================================================
 * Rebuild TOC with salvage from old
 * ================================================================ */

MacmbxTOC *macmbx_toc_rebuild(const char *mbox_path, MacmbxTOC *old) {
  MacmbxTOC *toc = macmbx_toc_build(mbox_path);
  if (!toc) return NULL;
  if (!old || old->count == 0) return toc;

  /* Salvage state from old TOC by matching offset+length or msg_id_hash */
  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum *n = &toc->msgs[i];

    /* Binary search in old by offset */
    int lo = 0, hi = old->count - 1;
    bool found = false;
    while (lo <= hi) {
      int mid = (lo + hi) / 2;
      if (old->msgs[mid].offset < n->offset) lo = mid + 1;
      else if (old->msgs[mid].offset > n->offset) hi = mid - 1;
      else {
        if (old->msgs[mid].length == n->length) {
          /* Match — salvage state, priority, flags */
          int bo = n->body_offset;
          uint32_t secs = n->seconds;
          long serial = n->serial_num;
          long offset = n->offset;
          long length = n->length;
          *n = old->msgs[mid];
          n->offset = offset;
          n->length = length;
          n->body_offset = bo;
          n->serial_num = serial;
          if (n->state != MACMBX_TIMED) n->seconds = secs;
          found = true;
        }
        break;
      }
    }

    /* If not found by offset, try msg_id_hash */
    if (!found && n->msg_id_hash) {
      for (int j = 0; j < old->count; j++) {
        if (old->msgs[j].msg_id_hash == n->msg_id_hash &&
            old->msgs[j].length == n->length) {
          n->state = old->msgs[j].state;
          n->priority = old->msgs[j].priority;
          n->flags = old->msgs[j].flags;
          break;
        }
      }
    }
  }

  toc->dirty = true;
  return toc;
}
