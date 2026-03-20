/* macmbx_spell.c — Spell checking with pluggable backend
 * Part of macmbx: standalone mail data management library.
 *
 * Built-in: hash table dictionary + Levenshtein edit distance suggestions.
 */

#include "macmbx_spell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* ================================================================
 * Hash table dictionary
 * ================================================================ */

#define DICT_HASH_SIZE 65536  /* 64K buckets */

typedef struct DictEntry {
  char *word;
  bool user_word;             /* from user dictionary */
  struct DictEntry *next;
} DictEntry;

struct MacmbxSpell {
  DictEntry *buckets[DICT_HASH_SIZE];
  int word_count;
  /* User dictionary */
  char user_dict_path[1024];
  /* Pluggable backend */
  MacmbxSpellCheckFn check_fn;
  MacmbxSpellSuggestFn suggest_fn;
  void *backend_ctx;
  /* Options */
  bool ignore_caps;
  bool ignore_mixed_case;
  bool ignore_digits;
  int min_length;
};

static unsigned int dict_hash(const char *word) {
  unsigned int h = 5381;
  while (*word) {
    h = ((h << 5) + h) + (unsigned char)tolower(*word);
    word++;
  }
  return h % DICT_HASH_SIZE;
}

static DictEntry *dict_find(MacmbxSpell *sp, const char *word) {
  unsigned int h = dict_hash(word);
  for (DictEntry *e = sp->buckets[h]; e; e = e->next)
    if (strcasecmp(e->word, word) == 0) return e;
  return NULL;
}

static int dict_insert(MacmbxSpell *sp, const char *word, bool user_word) {
  if (dict_find(sp, word)) return 0; /* already present */
  unsigned int h = dict_hash(word);
  DictEntry *e = (DictEntry *)malloc(sizeof(DictEntry));
  if (!e) return -1;
  /* Store lowercase */
  e->word = strdup(word);
  if (e->word) for (char *p = e->word; *p; p++) *p = tolower((unsigned char)*p);
  e->user_word = user_word;
  e->next = sp->buckets[h];
  sp->buckets[h] = e;
  sp->word_count++;
  return 1;
}

static int dict_remove(MacmbxSpell *sp, const char *word) {
  unsigned int h = dict_hash(word);
  DictEntry **pp = &sp->buckets[h];
  while (*pp) {
    if (strcasecmp((*pp)->word, word) == 0) {
      DictEntry *e = *pp;
      *pp = e->next;
      free(e->word); free(e);
      sp->word_count--;
      return 0;
    }
    pp = &(*pp)->next;
  }
  return -1;
}

/* ================================================================
 * Levenshtein edit distance
 * ================================================================ */

static int levenshtein(const char *s, int slen, const char *t, int tlen) {
  if (slen == 0) return tlen;
  if (tlen == 0) return slen;
  /* Use two rows instead of full matrix */
  int *prev = (int *)malloc((tlen + 1) * sizeof(int));
  int *curr = (int *)malloc((tlen + 1) * sizeof(int));
  if (!prev || !curr) { free(prev); free(curr); return 999; }

  for (int j = 0; j <= tlen; j++) prev[j] = j;

  for (int i = 1; i <= slen; i++) {
    curr[0] = i;
    for (int j = 1; j <= tlen; j++) {
      int cost = (tolower((unsigned char)s[i-1]) == tolower((unsigned char)t[j-1])) ? 0 : 1;
      int del = prev[j] + 1;
      int ins = curr[j-1] + 1;
      int sub = prev[j-1] + cost;
      curr[j] = del < ins ? (del < sub ? del : sub) : (ins < sub ? ins : sub);
    }
    int *tmp = prev; prev = curr; curr = tmp;
  }
  int result = prev[tlen];
  free(prev); free(curr);
  return result;
}

/* ================================================================
 * Lifecycle
 * ================================================================ */

MacmbxSpell *macmbx_spell_new(void) {
  MacmbxSpell *sp = (MacmbxSpell *)calloc(1, sizeof(MacmbxSpell));
  if (!sp) return NULL;
  sp->ignore_caps = true;
  sp->ignore_mixed_case = true;
  sp->ignore_digits = true;
  sp->min_length = 2;
  return sp;
}

void macmbx_spell_free(MacmbxSpell *sp) {
  if (!sp) return;
  for (int i = 0; i < DICT_HASH_SIZE; i++) {
    DictEntry *e = sp->buckets[i];
    while (e) { DictEntry *next = e->next; free(e->word); free(e); e = next; }
  }
  free(sp);
}

/* ================================================================
 * Dictionary loading
 * ================================================================ */

int macmbx_spell_load_dict(MacmbxSpell *sp, const char *path) {
  if (!sp || !path) return -1;
  FILE *f = fopen(path, "r");
  if (!f) return -1;

  int loaded = 0;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    /* Strip trailing whitespace */
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n' ||
                        line[len-1] == ' ')) line[--len] = '\0';
    if (len == 0 || line[0] == '#') continue;

    /* Some dictionary files have encoding markers or affix info — skip */
    if (strchr(line, '/')) {
      /* Hunspell format: word/flags — take just the word */
      char *slash = strchr(line, '/');
      *slash = '\0';
      len = strlen(line);
    }

    if (len > 0 && len < 128) {
      if (dict_insert(sp, line, false) > 0) loaded++;
    }
  }
  fclose(f);
  return loaded;
}

int macmbx_spell_load_system_dict(MacmbxSpell *sp) {
  if (!sp) return 0;

  /* Try platform-specific paths first, then common ones */
  static const char *paths[] = {
#ifdef _WIN32
    /* Windows: Hunspell shipped with LibreOffice, Firefox, or user-installed */
    "C:\\Program Files\\LibreOffice\\share\\extensions\\dict-en\\en_US.dic",
    "C:\\Program Files (x86)\\LibreOffice\\share\\extensions\\dict-en\\en_US.dic",
    "C:\\hunspell\\en_US.dic",
#elif defined(__APPLE__)
    /* macOS */
    "/Library/Spelling/en",
    "/usr/share/dict/words",
    "/opt/homebrew/share/hunspell/en_US.dic",
    "/usr/local/share/hunspell/en_US.dic",
#else
    /* Linux / FreeBSD / other Unix */
    "/usr/share/dict/words",
    "/usr/share/dict/american-english",
    "/usr/share/dict/british-english",
    "/usr/share/hunspell/en_US.dic",
    "/usr/share/myspell/dicts/en_US.dic",
    "/usr/share/myspell/en_US.dic",
    "/usr/local/share/hunspell/en_US.dic",
#endif
    NULL
  };

  for (int i = 0; paths[i]; i++) {
    int loaded = macmbx_spell_load_dict(sp, paths[i]);
    if (loaded > 0) return loaded;
  }

  /* Last resort: try relative to executable or current directory */
  int loaded = macmbx_spell_load_dict(sp, "dictionaries/en_US.dic");
  if (loaded > 0) return loaded;
  loaded = macmbx_spell_load_dict(sp, "en_US.dic");
  return loaded > 0 ? loaded : 0;
}

int macmbx_spell_add_word(MacmbxSpell *sp, const char *word) {
  if (!sp || !word) return -1;
  return dict_insert(sp, word, true);
}

int macmbx_spell_remove_word(MacmbxSpell *sp, const char *word) {
  if (!sp || !word) return -1;
  return dict_remove(sp, word);
}

int macmbx_spell_load_user_dict(MacmbxSpell *sp, const char *path) {
  if (!sp || !path) return -1;
  snprintf(sp->user_dict_path, sizeof(sp->user_dict_path), "%s", path);
  FILE *f = fopen(path, "r");
  if (!f) return 0;
  int loaded = 0;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) line[--len] = '\0';
    if (len > 0 && dict_insert(sp, line, true) > 0) loaded++;
  }
  fclose(f);
  return loaded;
}

int macmbx_spell_save_user_dict(MacmbxSpell *sp, const char *path) {
  if (!sp) return -1;
  const char *p = path ? path : sp->user_dict_path;
  if (!p[0]) return -1;
  FILE *f = fopen(p, "w");
  if (!f) return -1;
  for (int i = 0; i < DICT_HASH_SIZE; i++) {
    for (DictEntry *e = sp->buckets[i]; e; e = e->next) {
      if (e->user_word) fprintf(f, "%s\n", e->word);
    }
  }
  fclose(f);
  return 0;
}

int macmbx_spell_word_count(MacmbxSpell *sp) {
  return sp ? sp->word_count : 0;
}

/* ================================================================
 * Pluggable backend
 * ================================================================ */

void macmbx_spell_set_backend(MacmbxSpell *sp,
                                MacmbxSpellCheckFn check_fn,
                                MacmbxSpellSuggestFn suggest_fn,
                                void *ctx) {
  if (!sp) return;
  sp->check_fn = check_fn;
  sp->suggest_fn = suggest_fn;
  sp->backend_ctx = ctx;
}

/* ================================================================
 * Options
 * ================================================================ */

/* ================================================================
 * Replacement / Auto-correct
 * ================================================================ */

char *macmbx_spell_replace(const char *text, int offset, int length,
                             const char *replacement) {
  if (!text || !replacement) return text ? strdup(text) : NULL;
  long tlen = (long)strlen(text);
  long rlen = (long)strlen(replacement);
  long newlen = tlen - length + rlen;
  char *out = (char *)malloc(newlen + 1);
  if (!out) return strdup(text);
  memcpy(out, text, offset);
  memcpy(out + offset, replacement, rlen);
  memcpy(out + offset + rlen, text + offset + length, tlen - offset - length);
  out[newlen] = '\0';
  return out;
}

char *macmbx_spell_apply_fixes(const char *text, long len,
                                 MacmbxSpellFix *fixes, int fix_count) {
  if (!text || !fixes || fix_count <= 0) return text ? strdup(text) : NULL;
  if (len < 0) len = (long)strlen(text);

  /* Calculate output size */
  long newlen = len;
  for (int i = 0; i < fix_count; i++)
    newlen += (long)strlen(fixes[i].replacement) - fixes[i].length;

  char *out = (char *)malloc(newlen + 1);
  if (!out) return strdup(text);

  long src = 0, dst = 0;
  for (int i = 0; i < fix_count; i++) {
    /* Copy text before this fix */
    long gap = fixes[i].offset - src;
    if (gap > 0) { memcpy(out + dst, text + src, gap); dst += gap; }
    /* Insert replacement */
    long rlen = (long)strlen(fixes[i].replacement);
    memcpy(out + dst, fixes[i].replacement, rlen);
    dst += rlen;
    src = fixes[i].offset + fixes[i].length;
  }
  /* Copy remaining text */
  if (src < len) { memcpy(out + dst, text + src, len - src); dst += len - src; }
  out[dst] = '\0';
  return out;
}

char *macmbx_spell_auto_correct(MacmbxSpell *sp, const char *text, long len) {
  if (!sp || !text) return text ? strdup(text) : NULL;
  if (len < 0) len = (long)strlen(text);

  MacmbxSpellError *errors = NULL;
  int count = macmbx_spell_scan(sp, text, len, &errors);
  if (count <= 0) { free(errors); return strdup(text); }

  /* Build fixes array — only apply confident corrections (first suggestion) */
  MacmbxSpellFix *fixes = (MacmbxSpellFix *)calloc(count, sizeof(MacmbxSpellFix));
  int nfixes = 0;
  for (int i = 0; i < count; i++) {
    if (errors[i].result.suggestion_count > 0) {
      fixes[nfixes].offset = errors[i].offset;
      fixes[nfixes].length = errors[i].length;
      fixes[nfixes].replacement = errors[i].result.suggestions[0];
      nfixes++;
    }
  }

  char *result = macmbx_spell_apply_fixes(text, len, fixes, nfixes);
  free(fixes);
  free(errors);
  return result;
}

/* ================================================================
 * Options
 * ================================================================ */

void macmbx_spell_set_ignore_caps(MacmbxSpell *sp, bool ignore) { if (sp) sp->ignore_caps = ignore; }
void macmbx_spell_set_ignore_mixed_case(MacmbxSpell *sp, bool ignore) { if (sp) sp->ignore_mixed_case = ignore; }
void macmbx_spell_set_ignore_digits(MacmbxSpell *sp, bool ignore) { if (sp) sp->ignore_digits = ignore; }
void macmbx_spell_set_min_length(MacmbxSpell *sp, int min_len) { if (sp) sp->min_length = min_len; }

/* ================================================================
 * Checking
 * ================================================================ */

/* Should we skip this word based on options? */
static bool should_skip(MacmbxSpell *sp, const char *word, int len) {
  if (len < sp->min_length) return true;

  /* All caps */
  if (sp->ignore_caps) {
    bool all_caps = true;
    for (int i = 0; i < len; i++)
      if (islower((unsigned char)word[i])) { all_caps = false; break; }
    if (all_caps && len > 1) return true;
  }

  /* Mixed case (camelCase, iPhone) */
  if (sp->ignore_mixed_case) {
    bool has_upper = false, has_lower = false;
    for (int i = 1; i < len; i++) {
      if (isupper((unsigned char)word[i])) has_upper = true;
      if (islower((unsigned char)word[i])) has_lower = true;
    }
    if (has_upper && has_lower && isupper((unsigned char)word[0])) {
      /* Could be a name — only skip if uppercase appears mid-word */
      for (int i = 2; i < len; i++)
        if (isupper((unsigned char)word[i])) return true;
    }
  }

  /* Contains digits */
  if (sp->ignore_digits) {
    for (int i = 0; i < len; i++)
      if (isdigit((unsigned char)word[i])) return true;
  }

  return false;
}

bool macmbx_spell_check(MacmbxSpell *sp, const char *word) {
  if (!sp || !word || !*word) return true;

  /* External backend first */
  if (sp->check_fn) return sp->check_fn(word, sp->backend_ctx);

  /* Built-in dictionary */
  return dict_find(sp, word) != NULL;
}

/* Find suggestions using Levenshtein distance */
static int builtin_suggest(MacmbxSpell *sp, const char *word,
                            char suggestions[][64], int max) {
  int wlen = (int)strlen(word);
  if (wlen > 60 || wlen < 2) return 0;

  /* Collect candidates with edit distance <= 2 */
  typedef struct { char w[64]; int dist; } Cand;
  Cand *cands = (Cand *)malloc(64 * sizeof(Cand));
  if (!cands) return 0;
  int ncands = 0;
  int max_dist = (wlen <= 4) ? 1 : 2;

  for (int i = 0; i < DICT_HASH_SIZE && ncands < 64; i++) {
    for (DictEntry *e = sp->buckets[i]; e && ncands < 64; e = e->next) {
      int elen = (int)strlen(e->word);
      /* Quick length filter — edit distance can't exceed length difference */
      if (abs(elen - wlen) > max_dist) continue;
      int dist = levenshtein(word, wlen, e->word, elen);
      if (dist > 0 && dist <= max_dist) {
        snprintf(cands[ncands].w, 64, "%s", e->word);
        cands[ncands].dist = dist;
        ncands++;
      }
    }
  }

  /* Sort by distance, then alphabetically */
  for (int i = 0; i < ncands - 1; i++) {
    for (int j = i + 1; j < ncands; j++) {
      if (cands[j].dist < cands[i].dist ||
          (cands[j].dist == cands[i].dist && strcmp(cands[j].w, cands[i].w) < 0)) {
        Cand tmp = cands[i]; cands[i] = cands[j]; cands[j] = tmp;
      }
    }
  }

  int count = ncands < max ? ncands : max;
  for (int i = 0; i < count; i++)
    snprintf(suggestions[i], 64, "%s", cands[i].w);

  free(cands);
  return count;
}

MacmbxSpellResult macmbx_spell_check_full(MacmbxSpell *sp, const char *word) {
  MacmbxSpellResult r = {0};
  if (!sp || !word) { r.correct = true; return r; }

  r.correct = macmbx_spell_check(sp, word);
  if (!r.correct) {
    if (sp->suggest_fn) {
      r.suggestion_count = sp->suggest_fn(word, r.suggestions,
                                            MACMBX_SPELL_MAX_SUGGESTIONS,
                                            sp->backend_ctx);
    } else {
      r.suggestion_count = builtin_suggest(sp, word, r.suggestions,
                                             MACMBX_SPELL_MAX_SUGGESTIONS);
    }
  }
  return r;
}

/* ================================================================
 * Text scanning
 * ================================================================ */

/* Check if position looks like it's inside a URL or email */
static bool in_url_or_email(const char *text, int pos) {
  /* Look backwards for "://" or "@" */
  for (int i = pos; i > 0 && i > pos - 50; i--) {
    if (text[i] == ' ' || text[i] == '\n' || text[i] == '\r') break;
    if (text[i] == '/' && i > 1 && text[i-1] == '/' && text[i-2] == ':') return true;
    if (text[i] == '@') return true;
  }
  /* Look forward for common URL patterns */
  for (int i = pos; text[i] && i < pos + 20; i++) {
    if (text[i] == ' ' || text[i] == '\n') break;
    if (text[i] == '@') return true;
    if (text[i] == '/' && i + 1 < pos + 20 && text[i+1] == '/') return true;
  }
  return false;
}

int macmbx_spell_scan(MacmbxSpell *sp, const char *text, long len,
                        MacmbxSpellError **errors) {
  if (!sp || !text || !errors) return 0;
  if (len < 0) len = (long)strlen(text);
  *errors = NULL;

  int count = 0, cap = 32;
  *errors = (MacmbxSpellError *)calloc(cap, sizeof(MacmbxSpellError));

  long i = 0;
  while (i < len) {
    /* Skip non-alpha */
    while (i < len && !isalpha((unsigned char)text[i])) i++;
    if (i >= len) break;

    /* Collect word */
    long start = i;
    while (i < len && (isalpha((unsigned char)text[i]) || text[i] == '\'' || text[i] == '-'))
      i++;
    /* Strip trailing apostrophe/hyphen */
    while (i > start && (text[i-1] == '\'' || text[i-1] == '-')) i--;

    int wlen = (int)(i - start);
    if (wlen <= 0) continue;

    char word[128];
    if (wlen >= (int)sizeof(word)) continue;
    memcpy(word, text + start, wlen);
    word[wlen] = '\0';

    /* Skip based on options */
    if (should_skip(sp, word, wlen)) continue;

    /* Skip URLs and email addresses */
    if (in_url_or_email(text, (int)start)) continue;

    /* Check */
    if (!macmbx_spell_check(sp, word)) {
      if (count >= cap) {
        cap *= 2;
        *errors = (MacmbxSpellError *)realloc(*errors, cap * sizeof(MacmbxSpellError));
      }
      MacmbxSpellError *err = &(*errors)[count];
      snprintf(err->word, sizeof(err->word), "%s", word);
      err->offset = (int)start;
      err->length = wlen;
      err->result = macmbx_spell_check_full(sp, word);
      count++;
    }
  }
  return count;
}
