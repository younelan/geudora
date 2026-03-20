/* macmbx_spell.h — Spell checking with pluggable backend
 * Part of macmbx: standalone mail data management library.
 *
 * Built-in spell checker: hash table dictionary + Levenshtein suggestions.
 * Pluggable: register hunspell/aspell/system callback to override.
 * User dictionary: per-user word list, persisted to file.
 *
 * No UI dependency. Portable C99.
 */

#ifndef MACMBX_SPELL_H
#define MACMBX_SPELL_H

#include <stddef.h>
#include <stdbool.h>

/* ================================================================
 * Spell check result
 * ================================================================ */

#define MACMBX_SPELL_MAX_SUGGESTIONS 10

typedef struct {
  bool correct;                                 /* word is known */
  char suggestions[MACMBX_SPELL_MAX_SUGGESTIONS][64]; /* suggested corrections */
  int suggestion_count;
} MacmbxSpellResult;

/* Result for scanning a block of text */
typedef struct {
  char word[128];            /* the misspelled word */
  int offset;                /* byte offset in text */
  int length;                /* word length */
  MacmbxSpellResult result;  /* suggestions */
} MacmbxSpellError;

/* ================================================================
 * Pluggable callback — override built-in with external engine
 * ================================================================ */

/* Check if a word is correct. Return true if known. */
typedef bool (*MacmbxSpellCheckFn)(const char *word, void *ctx);

/* Get suggestions for a misspelled word. Return count, fill suggestions. */
typedef int (*MacmbxSpellSuggestFn)(const char *word,
                                     char suggestions[][64], int max,
                                     void *ctx);

/* ================================================================
 * Spell checker instance
 * ================================================================ */

typedef struct MacmbxSpell MacmbxSpell;

/* ================================================================
 * Lifecycle
 * ================================================================ */

/* Create a new spell checker. Uses built-in engine by default. */
MacmbxSpell *macmbx_spell_new(void);

/* Free spell checker and all dictionaries. */
void macmbx_spell_free(MacmbxSpell *sp);

/* ================================================================
 * Dictionary management
 * ================================================================ */

/* Load a word list file (one word per line).
 * Can be called multiple times to load multiple dictionaries.
 * Returns count of words loaded, or -1 on error. */
int macmbx_spell_load_dict(MacmbxSpell *sp, const char *path);

/* Load the system dictionary (tries common paths).
 * Returns count loaded, 0 if not found. */
int macmbx_spell_load_system_dict(MacmbxSpell *sp);

/* Add a single word to the active dictionary. */
int macmbx_spell_add_word(MacmbxSpell *sp, const char *word);

/* Remove a word from the dictionary. */
int macmbx_spell_remove_word(MacmbxSpell *sp, const char *word);

/* Load user dictionary (personal word list). */
int macmbx_spell_load_user_dict(MacmbxSpell *sp, const char *path);

/* Save user dictionary. */
int macmbx_spell_save_user_dict(MacmbxSpell *sp, const char *path);

/* Get dictionary word count. */
int macmbx_spell_word_count(MacmbxSpell *sp);

/* ================================================================
 * Pluggable backend
 * ================================================================ */

/* Register an external spell check engine (hunspell, aspell, etc.).
 * When set, built-in dictionary is bypassed for check/suggest.
 * User dictionary add/remove still work (forwarded to callback if set).
 * Pass NULL to revert to built-in. */
void macmbx_spell_set_backend(MacmbxSpell *sp,
                                MacmbxSpellCheckFn check_fn,
                                MacmbxSpellSuggestFn suggest_fn,
                                void *ctx);

/* ================================================================
 * Checking
 * ================================================================ */

/* Check a single word. Returns true if correct. */
bool macmbx_spell_check(MacmbxSpell *sp, const char *word);

/* Check a word and get suggestions if misspelled. */
MacmbxSpellResult macmbx_spell_check_full(MacmbxSpell *sp, const char *word);

/* Scan a block of text. Returns count of misspelled words.
 * Allocates *errors array. Caller frees.
 * Skips: URLs, email addresses, numbers, words in user dict. */
int macmbx_spell_scan(MacmbxSpell *sp, const char *text, long len,
                        MacmbxSpellError **errors);

/* ================================================================
 * Replacement / Auto-correct
 * ================================================================ */

/* Replace a misspelled word at offset in text. Returns new malloc'd text. */
char *macmbx_spell_replace(const char *text, int offset, int length,
                             const char *replacement);

/* Replace all misspelled words with first suggestion automatically.
 * Returns new malloc'd text. Caller frees.
 * Only replaces words where suggestion confidence is high (distance=1). */
char *macmbx_spell_auto_correct(MacmbxSpell *sp, const char *text, long len);

/* Apply specific replacements: array of {offset, length, replacement}.
 * Replacements must be sorted by offset ascending.
 * Returns new malloc'd text. Caller frees. */
typedef struct {
  int offset;
  int length;
  const char *replacement;
} MacmbxSpellFix;

char *macmbx_spell_apply_fixes(const char *text, long len,
                                 MacmbxSpellFix *fixes, int fix_count);

/* ================================================================
 * Options
 * ================================================================ */

/* Ignore words in ALL CAPS. Default: true. */
void macmbx_spell_set_ignore_caps(MacmbxSpell *sp, bool ignore);

/* Ignore words with mixed case (e.g. "iPhone"). Default: true. */
void macmbx_spell_set_ignore_mixed_case(MacmbxSpell *sp, bool ignore);

/* Ignore words containing digits. Default: true. */
void macmbx_spell_set_ignore_digits(MacmbxSpell *sp, bool ignore);

/* Minimum word length to check. Default: 2. */
void macmbx_spell_set_min_length(MacmbxSpell *sp, int min_len);

#endif /* MACMBX_SPELL_H */
