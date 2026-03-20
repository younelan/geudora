/* macmbx_filter.c — Filter rule engine
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Eudora Filters file compatible.
 * Pure match + execute: no UI dependencies.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>

#ifndef _WIN32
  #include <regex.h>
  #define MACMBX_HAVE_REGEX 1
#endif

/* ================================================================
 * String matching
 * ================================================================ */

static bool ci_contains(const char *hay, const char *needle) {
  if (!hay || !needle || !*needle) return !needle || !*needle;
  size_t nlen = strlen(needle);
  for (; *hay; hay++) {
    if (strncasecmp(hay, needle, nlen) == 0) return true;
  }
  return false;
}

static bool ci_equals(const char *a, const char *b) {
  return a && b && strcasecmp(a, b) == 0;
}

static bool ci_starts(const char *s, const char *prefix) {
  return s && prefix && strncasecmp(s, prefix, strlen(prefix)) == 0;
}

static bool ci_ends(const char *s, const char *suffix) {
  if (!s || !suffix) return false;
  size_t slen = strlen(s), plen = strlen(suffix);
  if (plen > slen) return false;
  return strncasecmp(s + slen - plen, suffix, plen) == 0;
}

static bool match_verb(const char *text, MacmbxVerb verb, const char *value) {
  switch (verb) {
  case MACMBX_VERB_CONTAINS:     return ci_contains(text, value);
  case MACMBX_VERB_NOT_CONTAINS: return !ci_contains(text, value);
  case MACMBX_VERB_IS:           return ci_equals(text, value);
  case MACMBX_VERB_IS_NOT:       return !ci_equals(text, value);
  case MACMBX_VERB_STARTS_WITH:  return ci_starts(text, value);
  case MACMBX_VERB_ENDS_WITH:    return ci_ends(text, value);
  case MACMBX_VERB_APPEARS:      return text && text[0];
  case MACMBX_VERB_NOT_APPEARS:  return !text || !text[0];
#ifdef MACMBX_HAVE_REGEX
  case MACMBX_VERB_REGEX:        return false; /* use match_verb_cond for regex */
#else
  case MACMBX_VERB_REGEX:        return ci_contains(text, value);
#endif
  case MACMBX_VERB_JUNK_LESS:    return false; /* needs spam score context */
  case MACMBX_VERB_JUNK_MORE:    return false;
  default: return false;
  }
}

/* Regex-aware match — uses cached compiled regex from condition */
static bool match_verb_cond(const char *text, MacmbxCondition *cond) {
  if (cond->verb != MACMBX_VERB_REGEX)
    return match_verb(text, cond->verb, cond->value);
#ifdef MACMBX_HAVE_REGEX
  if (cond->compiled_regex) {
    return regexec((regex_t *)cond->compiled_regex, text ? text : "",
                   0, NULL, 0) == 0;
  }
  /* Fallback: compile on the fly */
  regex_t re;
  if (regcomp(&re, cond->value, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
    return false;
  bool m = regexec(&re, text ? text : "", 0, NULL, 0) == 0;
  regfree(&re);
  return m;
#else
  return ci_contains(text, cond->value);
#endif
}

/* ================================================================
 * Date parsing for filter comparison
 * Parses "dd/mm/yyyy" or "yyyy-mm-dd" or days-ago number.
 * Returns UTC seconds, 0 on failure.
 * ================================================================ */

static uint32_t parse_filter_date(const char *s) {
  if (!s || !*s) return 0;
  /* Try as number of days ago */
  if (isdigit((unsigned char)s[0]) && !strchr(s, '/') && !strchr(s, '-')) {
    int days = atoi(s);
    return (uint32_t)(time(NULL) - days * 86400);
  }
  /* Try mm/dd/yyyy */
  int m = 0, d = 0, y = 0;
  if (sscanf(s, "%d/%d/%d", &m, &d, &y) == 3) {
    struct tm tm = {0};
    tm.tm_year = (y < 100 ? y + 2000 : y) - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_isdst = -1;
    time_t t = mktime(&tm);
    return t > 0 ? (uint32_t)t : 0;
  }
  /* Try yyyy-mm-dd */
  if (sscanf(s, "%d-%d-%d", &y, &m, &d) == 3) {
    struct tm tm = {0};
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_isdst = -1;
    time_t t = mktime(&tm);
    return t > 0 ? (uint32_t)t : 0;
  }
  return 0;
}

/* Compare dates: strip time, compare date-only (within same day) */
static int date_day_cmp(uint32_t a, uint32_t b) {
  uint32_t day_a = a / 86400;
  uint32_t day_b = b / 86400;
  if (day_a < day_b) return -1;
  if (day_a > day_b) return 1;
  return 0;
}

/* ================================================================
 * Condition matching
 * ================================================================ */

static bool match_condition(MacmbxCondition *cond, MacmbxTOC *toc, int index) {
  if (!cond->header[0]) return false;
  MacmbxMsgSum *msg = &toc->msgs[index];

  /* --- From: (fast, from summary) --- */
  if (strcasecmp(cond->header, "From:") == 0 || strcasecmp(cond->header, "From") == 0)
    return match_verb_cond(msg->from, cond);

  /* --- Subject: (fast, from summary) --- */
  if (strcasecmp(cond->header, "Subject:") == 0 || strcasecmp(cond->header, "Subject") == 0)
    return match_verb_cond(msg->subject, cond);

  /* --- Date: comparison --- */
  if (strcasecmp(cond->header, "Date:") == 0 || strcasecmp(cond->header, "Date") == 0) {
    uint32_t target = parse_filter_date(cond->value);
    if (!target) return false;
    switch (cond->verb) {
    case MACMBX_VERB_DATE_BEFORE: return date_day_cmp(msg->seconds, target) < 0;
    case MACMBX_VERB_DATE_AFTER:  return date_day_cmp(msg->seconds, target) > 0;
    case MACMBX_VERB_DATE_IS:     return date_day_cmp(msg->seconds, target) == 0;
    case MACMBX_VERB_IS:          return date_day_cmp(msg->seconds, target) == 0;
    case MACMBX_VERB_JUNK_LESS:   return msg->seconds < target; /* before */
    case MACMBX_VERB_JUNK_MORE:   return msg->seconds > target; /* after */
    default: return false;
    }
  }

  /* --- Priority: comparison --- */
  if (strcasecmp(cond->header, "Priority:") == 0 || strcasecmp(cond->header, "Priority") == 0) {
    int target = atoi(cond->value);
    switch (cond->verb) {
    case MACMBX_VERB_PRIORITY_IS:    return msg->priority == target;
    case MACMBX_VERB_PRIORITY_ABOVE: return msg->priority < target; /* lower number = higher priority */
    case MACMBX_VERB_PRIORITY_BELOW: return msg->priority > target;
    case MACMBX_VERB_IS:             return msg->priority == target;
    case MACMBX_VERB_JUNK_LESS:      return msg->priority < target;
    case MACMBX_VERB_JUNK_MORE:      return msg->priority > target;
    default: return false;
    }
  }

  /* --- Junk: score --- */
  if (strcasecmp(cond->header, "Junk:") == 0 || strcasecmp(cond->header, "Junk") == 0) {
    int threshold = atoi(cond->value);
    if (cond->verb == MACMBX_VERB_JUNK_LESS) return msg->spam_score < threshold;
    if (cond->verb == MACMBX_VERB_JUNK_MORE) return msg->spam_score > threshold;
    return false;
  }

  /* --- Personality: match by personality ID or name --- */
  if (strcasecmp(cond->header, "Personality:") == 0 || strcasecmp(cond->header, "Personality") == 0) {
    /* Match by pers_id (numeric) or by name (string).
     * Value can be: numeric ID, or account name to compare.
     * For numeric: pop_pers_id is a hash; for name: caller should
     * set the value to the hash or name. We try both. */
    char pers_str[32];
    snprintf(pers_str, sizeof(pers_str), "%u", msg->pop_pers_id);
    if (match_verb(pers_str, cond->verb, cond->value)) return true;
    /* Also try pers_id (sending personality) */
    snprintf(pers_str, sizeof(pers_str), "%u", msg->pers_id);
    return match_verb(pers_str, cond->verb, cond->value);
  }

  /* --- Body: search message body text --- */
  if (strcasecmp(cond->header, "Body:") == 0 || strcasecmp(cond->header, "Body") == 0 ||
      strcasecmp(cond->header, "<<Body>>") == 0) {
    long bodyLen = 0;
    char *body = macmbx_read_body(toc, index, &bodyLen);
    bool result = body && match_verb_cond(body, cond);
    free(body);
    return result;
  }

  /* --- Any: match against all summary fields + To + Cc + body --- */
  if (strcasecmp(cond->header, "Any:") == 0 || strcasecmp(cond->header, "Any") == 0) {
    if (match_verb_cond(msg->from, cond)) return true;
    if (match_verb_cond(msg->subject, cond)) return true;
    char *to = macmbx_read_header_field(toc, index, "To");
    bool m = to && match_verb_cond(to, cond);
    free(to);
    if (m) return true;
    char *cc = macmbx_read_header_field(toc, index, "Cc");
    m = cc && match_verb_cond(cc, cond);
    free(cc);
    if (m) return true;
    /* Search body as last resort */
    long bodyLen = 0;
    char *body = macmbx_read_body(toc, index, &bodyLen);
    m = body && match_verb_cond(body, cond);
    free(body);
    return m;
  }

  /* --- Generic header — read from mbox --- */
  char field[64];
  snprintf(field, sizeof(field), "%s", cond->header);
  size_t flen = strlen(field);
  if (flen > 0 && field[flen-1] == ':') field[flen-1] = '\0';

  char *val = macmbx_read_header_field(toc, index, field);
  bool result = match_verb_cond(val, cond);
  free(val);
  return result;
}

/* ================================================================
 * Rule matching
 * ================================================================ */

bool macmbx_filter_match(MacmbxRule *rule, MacmbxTOC *toc, int index) {
  if (!rule || !toc || index < 0 || index >= toc->count) return false;
  if (rule->condition_count == 0) return false;

  bool first = match_condition(&rule->conditions[0], toc, index);

  if (rule->condition_count == 1) return first;

  bool second = match_condition(&rule->conditions[1], toc, index);

  switch (rule->conjunction) {
  case MACMBX_CONJ_AND:    return first && second;
  case MACMBX_CONJ_OR:     return first || second;
  case MACMBX_CONJ_UNLESS: return first && !second;
  default:                 return first;
  }
}

/* ================================================================
 * Execution
 * ================================================================ */

static void init_result(MacmbxFilterResult *r) {
  memset(r, 0, sizeof(*r));
  r->new_state = -1;
  r->new_priority = -1;
  r->new_label = -1;
}

/* Action pass assignment (matches Eudora's multi-pass order):
 * Pass 0: metadata changes (status, priority, label, junk)
 * Pass 1: transfer/copy/delete + stop + callbacks */
static int action_pass(MacmbxActionType type) {
  switch (type) {
  case MACMBX_ACT_STATUS:
  case MACMBX_ACT_PRIORITY:
  case MACMBX_ACT_LABEL:
  case MACMBX_ACT_JUNK:
  case MACMBX_ACT_SUBJECT:
    return 0;
  default:
    return 1;
  }
}

#define MACMBX_MAX_PASS 2

static void execute_action(MacmbxAction *act, MacmbxTOC *toc, int index,
                            bool no_xfer, MacmbxStore *store,
                            MacmbxFilterResult *result,
                            MacmbxFilterActionFn fn, void *ctx) {
  switch (act->type) {
  case MACMBX_ACT_STATUS:
    macmbx_set_state(toc, index, (uint8_t)act->int_value);
    result->new_state = act->int_value;
    break;

  case MACMBX_ACT_PRIORITY:
    macmbx_set_priority(toc, index, (uint8_t)act->int_value);
    result->new_priority = act->int_value;
    break;

  case MACMBX_ACT_LABEL:
    macmbx_set_label(toc, index, (uint8_t)act->int_value);
    result->new_label = act->int_value;
    break;

  case MACMBX_ACT_DELETE:
    if (!no_xfer) {
      macmbx_delete_message(toc, index);
      result->deleted = true;
    }
    break;

  case MACMBX_ACT_JUNK:
    macmbx_set_label(toc, index, 0);
    toc->msgs[index].spam_score = (int8_t)act->int_value;
    toc->dirty = true;
    break;

  case MACMBX_ACT_TRANSFER:
    if (no_xfer) break;
    if (store && act->str_value[0]) {
      MacmbxTOC *dst = macmbx_store_open_mailbox(store, act->str_value);
      if (dst) {
        macmbx_transfer(toc, index, dst, false);
        result->transferred = true;
        snprintf(result->transfer_dest, sizeof(result->transfer_dest),
                 "%s", act->str_value);
      }
    }
    break;

  case MACMBX_ACT_COPY:
    if (no_xfer) break;
    if (store && act->str_value[0]) {
      MacmbxTOC *dst = macmbx_store_open_mailbox(store, act->str_value);
      if (dst) {
        macmbx_transfer(toc, index, dst, true);
        result->copied = true;
      }
    }
    break;

  case MACMBX_ACT_STOP:
    result->stopped = true;
    break;

  case MACMBX_ACT_SOUND:
  case MACMBX_ACT_OPEN:
  case MACMBX_ACT_PRINT:
  case MACMBX_ACT_FORWARD:
  case MACMBX_ACT_REDIRECT:
  case MACMBX_ACT_REPLY:
  case MACMBX_ACT_NOTIFY:
  case MACMBX_ACT_CALLBACK:
    if (fn) fn(toc, index, act, ctx);
    break;

  default:
    break;
  }
}

/* Run all actions for a matched rule using multi-pass execution:
 * pass 0 = metadata, pass 1 = transfers/callbacks. */
static void run_rule_actions(MacmbxRule *rule, MacmbxTOC *toc, int index,
                              bool no_xfer, MacmbxStore *store,
                              MacmbxFilterResult *result,
                              MacmbxFilterActionFn fn, void *ctx) {
  for (int pass = 0; pass < MACMBX_MAX_PASS && !result->stopped && !result->transferred; pass++) {
    for (int a = 0; a < rule->action_count; a++) {
      if (action_pass(rule->actions[a].type) != pass) continue;
      execute_action(&rule->actions[a], toc, index, no_xfer, store,
                     result, fn, ctx);
      if (result->stopped || result->transferred) break;
    }
  }
}

MacmbxFilterResult macmbx_filter_apply(MacmbxFilterSet *fs,
                                        MacmbxTOC *toc, int index,
                                        MacmbxStore *store,
                                        MacmbxFilterActionFn action_fn,
                                        void *action_ctx) {
  return macmbx_filter_apply_ex(fs, toc, index, false, store,
                                 action_fn, action_ctx);
}

MacmbxFilterResult macmbx_filter_apply_ex(MacmbxFilterSet *fs,
                                           MacmbxTOC *toc, int index,
                                           bool no_xfer,
                                           MacmbxStore *store,
                                           MacmbxFilterActionFn action_fn,
                                           void *action_ctx) {
  MacmbxFilterResult result;
  init_result(&result);
  if (!fs || !toc || index < 0 || index >= toc->count) return result;

  for (int r = 0; r < fs->count; r++) {
    MacmbxRule *rule = &fs->rules[r];
    if (!macmbx_filter_match(rule, toc, index)) continue;

    result.matched = true;
    run_rule_actions(rule, toc, index, no_xfer, store, &result,
                     action_fn, action_ctx);
    if (result.stopped || result.transferred) break;
  }
  return result;
}

int macmbx_filter_apply_all(MacmbxFilterSet *fs, MacmbxTOC *toc,
                              uint8_t when, MacmbxStore *store,
                              MacmbxFilterActionFn action_fn,
                              void *action_ctx) {
  if (!fs || !toc) return 0;
  int matched = 0;

  /* Process backwards so transfers don't invalidate indices */
  for (int i = toc->count - 1; i >= 0; i--) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) continue;

    MacmbxFilterResult result;
    init_result(&result);

    for (int r = 0; r < fs->count; r++) {
      MacmbxRule *rule = &fs->rules[r];
      if (!(rule->when & when)) continue;
      if (!macmbx_filter_match(rule, toc, i)) continue;

      result.matched = true;
      run_rule_actions(rule, toc, i, false, store, &result,
                       action_fn, action_ctx);
      if (result.stopped || result.transferred) break;
    }
    if (result.matched) matched++;
  }
  return matched;
}

int macmbx_filter_apply_selected(MacmbxFilterSet *fs, MacmbxTOC *toc,
                                   int *indices, int count,
                                   uint8_t when, bool no_xfer,
                                   MacmbxStore *store,
                                   MacmbxFilterActionFn action_fn,
                                   void *action_ctx) {
  if (!fs || !toc || !indices || count <= 0) return 0;
  int matched = 0;

  /* Process backwards so transfers don't invalidate earlier indices */
  for (int i = count - 1; i >= 0; i--) {
    int idx = indices[i];
    if (idx < 0 || idx >= toc->count) continue;
    if (toc->msgs[idx].flags & MACMBX_FLAG_DELETED) continue;

    MacmbxFilterResult result;
    init_result(&result);

    for (int r = 0; r < fs->count; r++) {
      MacmbxRule *rule = &fs->rules[r];
      if (when && !(rule->when & when)) continue;
      if (!macmbx_filter_match(rule, toc, idx)) continue;

      result.matched = true;
      run_rule_actions(rule, toc, idx, no_xfer, store, &result,
                       action_fn, action_ctx);
      if (result.stopped || result.transferred) break;
    }
    if (result.matched) matched++;
  }
  return matched;
}

/* ================================================================
 * Filter set management
 * ================================================================ */

MacmbxFilterSet *macmbx_filter_new(void) {
  MacmbxFilterSet *fs = (MacmbxFilterSet *)calloc(1, sizeof(MacmbxFilterSet));
  if (!fs) return NULL;
  fs->capacity = 16;
  fs->next_id = 1;
  fs->rules = (MacmbxRule *)calloc(fs->capacity, sizeof(MacmbxRule));
  return fs;
}

void macmbx_filter_free(MacmbxFilterSet *fs) {
  if (!fs) return;
#ifdef MACMBX_HAVE_REGEX
  /* Free cached compiled regexes */
  for (int r = 0; r < fs->count; r++) {
    for (int c = 0; c < fs->rules[r].condition_count; c++) {
      if (fs->rules[r].conditions[c].compiled_regex) {
        regfree((regex_t *)fs->rules[r].conditions[c].compiled_regex);
        free(fs->rules[r].conditions[c].compiled_regex);
      }
    }
  }
#endif
  free(fs->rules);
  free(fs);
}

int macmbx_filter_add_rule(MacmbxFilterSet *fs, const MacmbxRule *rule) {
  if (!fs || !rule) return -1;
  if (fs->count >= fs->capacity) {
    fs->capacity *= 2;
    fs->rules = (MacmbxRule *)realloc(fs->rules, fs->capacity * sizeof(MacmbxRule));
  }
  fs->rules[fs->count] = *rule;
  /* Auto-assign ID if not set */
  if (fs->rules[fs->count].id <= 0)
    fs->rules[fs->count].id = fs->next_id++;
  else if (fs->rules[fs->count].id >= fs->next_id)
    fs->next_id = fs->rules[fs->count].id + 1;
  return fs->count++;
}

int macmbx_filter_remove_rule(MacmbxFilterSet *fs, int index) {
  if (!fs || index < 0 || index >= fs->count) return -1;
  memmove(&fs->rules[index], &fs->rules[index+1],
          (fs->count - index - 1) * sizeof(MacmbxRule));
  fs->count--;
  return 0;
}

int macmbx_filter_move_rule(MacmbxFilterSet *fs, int from, int to) {
  if (!fs || from < 0 || from >= fs->count || to < 0 || to >= fs->count)
    return -1;
  MacmbxRule tmp = fs->rules[from];
  if (from < to) {
    memmove(&fs->rules[from], &fs->rules[from+1],
            (to - from) * sizeof(MacmbxRule));
  } else {
    memmove(&fs->rules[to+1], &fs->rules[to],
            (from - to) * sizeof(MacmbxRule));
  }
  fs->rules[to] = tmp;
  return 0;
}

MacmbxRule *macmbx_filter_get_rule(MacmbxFilterSet *fs, int index) {
  if (!fs || index < 0 || index >= fs->count) return NULL;
  return &fs->rules[index];
}

int macmbx_filter_find_by_id(MacmbxFilterSet *fs, int id) {
  if (!fs || id <= 0) return -1;
  for (int i = 0; i < fs->count; i++)
    if (fs->rules[i].id == id) return i;
  return -1;
}

MacmbxRule *macmbx_filter_get_by_id(MacmbxFilterSet *fs, int id) {
  int idx = macmbx_filter_find_by_id(fs, id);
  return idx >= 0 ? &fs->rules[idx] : NULL;
}

int macmbx_filter_remove_by_id(MacmbxFilterSet *fs, int id) {
  int idx = macmbx_filter_find_by_id(fs, id);
  if (idx < 0) return -1;
  return macmbx_filter_remove_rule(fs, idx);
}

void macmbx_filter_compile(MacmbxFilterSet *fs) {
#ifdef MACMBX_HAVE_REGEX
  if (!fs) return;
  for (int r = 0; r < fs->count; r++) {
    for (int c = 0; c < fs->rules[r].condition_count; c++) {
      MacmbxCondition *cond = &fs->rules[r].conditions[c];
      if (cond->verb == MACMBX_VERB_REGEX && cond->value[0]) {
        /* Free old compiled regex if any */
        if (cond->compiled_regex) {
          regfree((regex_t *)cond->compiled_regex);
          free(cond->compiled_regex);
        }
        cond->compiled_regex = malloc(sizeof(regex_t));
        if (cond->compiled_regex) {
          if (regcomp((regex_t *)cond->compiled_regex, cond->value,
                      REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) {
            free(cond->compiled_regex);
            cond->compiled_regex = NULL;
          }
        }
      }
    }
  }
#else
  (void)fs;
#endif
}

/* ================================================================
 * File I/O — Eudora Filters format
 *
 * Format:
 *   rule <name>
 *   id <n>
 *   incoming
 *   outgoing
 *   manual
 *   header <Header:>
 *   verb <contains|!contains|is|!is|starts|ends|regex|...>
 *   value <text>
 *   conjunction <and|or|unless>
 *   header <Header:>      (second condition)
 *   verb <...>
 *   value <...>
 *   status <n>
 *   priority <n>
 *   label <n>
 *   transfer <mailbox>
 *   copy <mailbox>
 *   sound <name>
 *   forward <address>
 *   redirect <address>
 *   stop
 *   (blank line or next "rule" starts new rule)
 * ================================================================ */

static MacmbxVerb parse_verb(const char *s) {
  if (strcasecmp(s, "contains") == 0)  return MACMBX_VERB_CONTAINS;
  if (strcasecmp(s, "!contains") == 0) return MACMBX_VERB_NOT_CONTAINS;
  if (strcasecmp(s, "is") == 0)        return MACMBX_VERB_IS;
  if (strcasecmp(s, "!is") == 0)       return MACMBX_VERB_IS_NOT;
  if (strcasecmp(s, "starts") == 0)    return MACMBX_VERB_STARTS_WITH;
  if (strcasecmp(s, "ends") == 0)      return MACMBX_VERB_ENDS_WITH;
  if (strcasecmp(s, "appears") == 0)   return MACMBX_VERB_APPEARS;
  if (strcasecmp(s, "!appears") == 0)  return MACMBX_VERB_NOT_APPEARS;
  if (strcasecmp(s, "regex") == 0)     return MACMBX_VERB_REGEX;
  if (strcasecmp(s, "less") == 0)      return MACMBX_VERB_JUNK_LESS;
  if (strcasecmp(s, "greater") == 0)   return MACMBX_VERB_JUNK_MORE;
  if (strcasecmp(s, "before") == 0)    return MACMBX_VERB_DATE_BEFORE;
  if (strcasecmp(s, "after") == 0)     return MACMBX_VERB_DATE_AFTER;
  if (strcasecmp(s, "dateIs") == 0)    return MACMBX_VERB_DATE_IS;
  if (strcasecmp(s, "priorIs") == 0)   return MACMBX_VERB_PRIORITY_IS;
  if (strcasecmp(s, "priorAbove") == 0) return MACMBX_VERB_PRIORITY_ABOVE;
  if (strcasecmp(s, "priorBelow") == 0) return MACMBX_VERB_PRIORITY_BELOW;
  return MACMBX_VERB_CONTAINS;
}

static const char *verb_to_str(MacmbxVerb v) {
  switch (v) {
  case MACMBX_VERB_CONTAINS:     return "contains";
  case MACMBX_VERB_NOT_CONTAINS: return "!contains";
  case MACMBX_VERB_IS:           return "is";
  case MACMBX_VERB_IS_NOT:       return "!is";
  case MACMBX_VERB_STARTS_WITH:  return "starts";
  case MACMBX_VERB_ENDS_WITH:    return "ends";
  case MACMBX_VERB_APPEARS:      return "appears";
  case MACMBX_VERB_NOT_APPEARS:  return "!appears";
  case MACMBX_VERB_REGEX:        return "regex";
  case MACMBX_VERB_JUNK_LESS:      return "less";
  case MACMBX_VERB_JUNK_MORE:      return "greater";
  case MACMBX_VERB_DATE_BEFORE:    return "before";
  case MACMBX_VERB_DATE_AFTER:     return "after";
  case MACMBX_VERB_DATE_IS:        return "dateIs";
  case MACMBX_VERB_PRIORITY_IS:    return "priorIs";
  case MACMBX_VERB_PRIORITY_ABOVE: return "priorAbove";
  case MACMBX_VERB_PRIORITY_BELOW: return "priorBelow";
  default: return "contains";
  }
}

MacmbxFilterSet *macmbx_filter_load(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) return NULL;

  MacmbxFilterSet *fs = macmbx_filter_new();
  if (!fs) { fclose(f); return NULL; }
  snprintf(fs->path, sizeof(fs->path), "%s", path);

  char line[1024];
  MacmbxRule cur;
  memset(&cur, 0, sizeof(cur));
  bool in_rule = false;
  int cond_idx = 0;  /* which condition we're building */

  while (fgets(line, sizeof(line), f)) {
    /* Strip trailing whitespace */
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n' ||
                        line[len-1] == ' ')) line[--len] = '\0';
    if (len == 0) continue;

    char *key = line;
    char *val = strchr(line, ' ');
    if (val) { *val = '\0'; val++; while (*val == ' ') val++; }

    if (strcasecmp(key, "rule") == 0) {
      /* Save previous rule */
      if (in_rule && cur.condition_count > 0)
        macmbx_filter_add_rule(fs, &cur);
      memset(&cur, 0, sizeof(cur));
      in_rule = true;
      cond_idx = 0;
      if (val) snprintf(cur.name, sizeof(cur.name), "%s", val);
    } else if (strcasecmp(key, "id") == 0) {
      if (val) cur.id = atoi(val);
    } else if (strcasecmp(key, "incoming") == 0) {
      cur.when |= MACMBX_WHEN_INCOMING;
    } else if (strcasecmp(key, "outgoing") == 0) {
      cur.when |= MACMBX_WHEN_OUTGOING;
    } else if (strcasecmp(key, "manual") == 0) {
      cur.when |= MACMBX_WHEN_MANUAL;
    } else if (strcasecmp(key, "header") == 0) {
      if (cond_idx < MACMBX_MAX_CONDITIONS && val) {
        snprintf(cur.conditions[cond_idx].header,
                 sizeof(cur.conditions[cond_idx].header), "%s", val);
        if (cond_idx >= cur.condition_count) cur.condition_count = cond_idx + 1;
      }
    } else if (strcasecmp(key, "verb") == 0) {
      if (cond_idx < MACMBX_MAX_CONDITIONS && val)
        cur.conditions[cond_idx].verb = parse_verb(val);
    } else if (strcasecmp(key, "value") == 0) {
      if (cond_idx < MACMBX_MAX_CONDITIONS && val) {
        snprintf(cur.conditions[cond_idx].value,
                 sizeof(cur.conditions[cond_idx].value), "%s", val);
        cond_idx++; /* move to next condition slot */
      }
    } else if (strcasecmp(key, "conjunction") == 0) {
      if (val) {
        if (strcasecmp(val, "and") == 0)    cur.conjunction = MACMBX_CONJ_AND;
        else if (strcasecmp(val, "or") == 0) cur.conjunction = MACMBX_CONJ_OR;
        else if (strcasecmp(val, "unless") == 0) cur.conjunction = MACMBX_CONJ_UNLESS;
      }
    } else if (strcasecmp(key, "status") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_STATUS;
        cur.actions[cur.action_count].int_value = atoi(val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "priority") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_PRIORITY;
        cur.actions[cur.action_count].int_value = atoi(val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "label") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_LABEL;
        cur.actions[cur.action_count].int_value = atoi(val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "transfer") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_TRANSFER;
        snprintf(cur.actions[cur.action_count].str_value, PATH_MAX, "%s", val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "copy") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_COPY;
        snprintf(cur.actions[cur.action_count].str_value, PATH_MAX, "%s", val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "sound") == 0) {
      if (cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_SOUND;
        if (val) snprintf(cur.actions[cur.action_count].str_value, PATH_MAX, "%s", val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "forward") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_FORWARD;
        snprintf(cur.actions[cur.action_count].str_value, PATH_MAX, "%s", val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "redirect") == 0) {
      if (val && cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_REDIRECT;
        snprintf(cur.actions[cur.action_count].str_value, PATH_MAX, "%s", val);
        cur.action_count++;
      }
    } else if (strcasecmp(key, "junk") == 0) {
      if (cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_JUNK;
        cur.actions[cur.action_count].int_value = val ? atoi(val) : 100;
        cur.action_count++;
      }
    } else if (strcasecmp(key, "notifyUser") == 0) {
      if (cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_NOTIFY;
        cur.action_count++;
      }
    } else if (strcasecmp(key, "open") == 0) {
      if (cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_OPEN;
        cur.action_count++;
      }
    } else if (strcasecmp(key, "print") == 0) {
      if (cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_PRINT;
        cur.action_count++;
      }
    } else if (strcasecmp(key, "stop") == 0) {
      if (cur.action_count < MACMBX_MAX_ACTIONS) {
        cur.actions[cur.action_count].type = MACMBX_ACT_STOP;
        cur.action_count++;
      }
    }
  }

  /* Save last rule */
  if (in_rule && cur.condition_count > 0)
    macmbx_filter_add_rule(fs, &cur);

  fclose(f);
  macmbx_filter_compile(fs);
  return fs;
}

int macmbx_filter_save(MacmbxFilterSet *fs) {
  if (!fs || !fs->path[0]) return -1;

  FILE *f = fopen(fs->path, "w");
  if (!f) return -1;

  for (int r = 0; r < fs->count; r++) {
    MacmbxRule *rule = &fs->rules[r];

    fprintf(f, "rule %s\n", rule->name);
    if (rule->id) fprintf(f, "id %d\n", rule->id);
    if (rule->when & MACMBX_WHEN_INCOMING) fprintf(f, "incoming\n");
    if (rule->when & MACMBX_WHEN_OUTGOING) fprintf(f, "outgoing\n");
    if (rule->when & MACMBX_WHEN_MANUAL)   fprintf(f, "manual\n");

    /* First condition */
    if (rule->condition_count >= 1) {
      fprintf(f, "header %s\n", rule->conditions[0].header);
      fprintf(f, "verb %s\n", verb_to_str(rule->conditions[0].verb));
      fprintf(f, "value %s\n", rule->conditions[0].value);
    }

    /* Conjunction + second condition */
    if (rule->condition_count >= 2) {
      switch (rule->conjunction) {
      case MACMBX_CONJ_AND:    fprintf(f, "conjunction and\n"); break;
      case MACMBX_CONJ_OR:     fprintf(f, "conjunction or\n"); break;
      case MACMBX_CONJ_UNLESS: fprintf(f, "conjunction unless\n"); break;
      default: break;
      }
      fprintf(f, "header %s\n", rule->conditions[1].header);
      fprintf(f, "verb %s\n", verb_to_str(rule->conditions[1].verb));
      fprintf(f, "value %s\n", rule->conditions[1].value);
    }

    /* Actions */
    for (int a = 0; a < rule->action_count; a++) {
      MacmbxAction *act = &rule->actions[a];
      switch (act->type) {
      case MACMBX_ACT_STATUS:    fprintf(f, "status %d\n", act->int_value); break;
      case MACMBX_ACT_PRIORITY:  fprintf(f, "priority %d\n", act->int_value); break;
      case MACMBX_ACT_LABEL:     fprintf(f, "label %d\n", act->int_value); break;
      case MACMBX_ACT_TRANSFER:  fprintf(f, "transfer %s\n", act->str_value); break;
      case MACMBX_ACT_COPY:      fprintf(f, "copy %s\n", act->str_value); break;
      case MACMBX_ACT_SOUND:     fprintf(f, "sound %s\n", act->str_value); break;
      case MACMBX_ACT_FORWARD:   fprintf(f, "forward %s\n", act->str_value); break;
      case MACMBX_ACT_REDIRECT:  fprintf(f, "redirect %s\n", act->str_value); break;
      case MACMBX_ACT_JUNK:      fprintf(f, "junk %d\n", act->int_value); break;
      case MACMBX_ACT_NOTIFY:    fprintf(f, "notifyUser\n"); break;
      case MACMBX_ACT_OPEN:      fprintf(f, "open\n"); break;
      case MACMBX_ACT_PRINT:     fprintf(f, "print\n"); break;
      case MACMBX_ACT_DELETE:    fprintf(f, "status %d\n", MACMBX_READ); break; /* delete via status */
      case MACMBX_ACT_STOP:      fprintf(f, "stop\n"); break;
      default: break;
      }
    }
  }
  fclose(f);
  return 0;
}
