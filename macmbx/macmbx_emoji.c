/* macmbx_emoji.c — Emoji lookup and text replacement
 * Part of macmbx: standalone mail data management library.
 *
 * ASCII smiley ↔ Unicode emoji conversion table + replace functions.
 * No UI dependency — pure text transform.
 */

#include "macmbx.h"
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * Emoji table — sorted longest-ASCII-first for greedy matching
 * ================================================================ */

typedef struct {
  const char *ascii;
  const char *emoji;
  const char *name;
  int ascii_len;
  int emoji_len;
} EmojiEntry;

static EmojiEntry emoji_table[] = {
  /* Longer patterns first to avoid partial matches */
  {">:-(",  "\xF0\x9F\x98\xA0",         "angry",        0, 0},
  {">:(",   "\xF0\x9F\x98\xA0",         "angry",        0, 0},
  {"O:-)",  "\xF0\x9F\x98\x87",         "angel",        0, 0},
  {"O:)",   "\xF0\x9F\x98\x87",         "angel",        0, 0},
  {":'-(",  "\xF0\x9F\x98\xA2",         "cry",          0, 0},
  {":'(",   "\xF0\x9F\x98\xA2",         "cry",          0, 0},
  {"</3",   "\xF0\x9F\x92\x94",         "broken heart", 0, 0},
  {":-)",   "\xF0\x9F\x98\x80",         "smile",        0, 0},
  {";-)",   "\xF0\x9F\x98\x89",         "wink",         0, 0},
  {":-D",   "\xF0\x9F\x98\x83",         "big grin",     0, 0},
  {":-P",   "\xF0\x9F\x98\x9B",         "tongue",       0, 0},
  {":-p",   "\xF0\x9F\x98\x9B",         "tongue",       0, 0},
  {":-(",   "\xF0\x9F\x98\x9E",         "sad",          0, 0},
  {":-/",   "\xF0\x9F\x98\x95",         "confused",     0, 0},
  {":-|",   "\xF0\x9F\x98\x90",         "neutral",      0, 0},
  {":-O",   "\xF0\x9F\x98\xAE",         "surprised",    0, 0},
  {":-o",   "\xF0\x9F\x98\xAE",         "surprised",    0, 0},
  {":-*",   "\xF0\x9F\x98\x98",         "kiss",         0, 0},
  {":-S",   "\xF0\x9F\x98\x96",         "confounded",   0, 0},
  {":-s",   "\xF0\x9F\x98\x96",         "confounded",   0, 0},
  {":-X",   "\xF0\x9F\xA4\x90",         "mute",         0, 0},
  {":^)",   "\xF0\x9F\x98\x8F",         "smirk",        0, 0},
  {"B-)",   "\xF0\x9F\x98\x8E",         "cool",         0, 0},
  {":)",    "\xF0\x9F\x98\x80",         "smile",        0, 0},
  {";)",    "\xF0\x9F\x98\x89",         "wink",         0, 0},
  {":D",    "\xF0\x9F\x98\x83",         "big grin",     0, 0},
  {":P",    "\xF0\x9F\x98\x9B",         "tongue",       0, 0},
  {":p",    "\xF0\x9F\x98\x9B",         "tongue",       0, 0},
  {":(",    "\xF0\x9F\x98\x9E",         "sad",          0, 0},
  {":/",    "\xF0\x9F\x98\x95",         "confused",     0, 0},
  {":|",    "\xF0\x9F\x98\x90",         "neutral",      0, 0},
  {":O",    "\xF0\x9F\x98\xAE",         "surprised",    0, 0},
  {":o",    "\xF0\x9F\x98\xAE",         "surprised",    0, 0},
  {":*",    "\xF0\x9F\x98\x98",         "kiss",         0, 0},
  {":S",    "\xF0\x9F\x98\x96",         "confounded",   0, 0},
  {":s",    "\xF0\x9F\x98\x96",         "confounded",   0, 0},
  {":X",    "\xF0\x9F\xA4\x90",         "mute",         0, 0},
  {"B)",    "\xF0\x9F\x98\x8E",         "cool",         0, 0},
  {"XD",    "\xF0\x9F\x98\x86",         "laughing",     0, 0},
  {"xD",    "\xF0\x9F\x98\x86",         "laughing",     0, 0},
  {"<3",    "\xE2\x9D\xA4\xEF\xB8\x8F", "heart",       0, 0},
};

#define EMOJI_COUNT (sizeof(emoji_table) / sizeof(emoji_table[0]))

static bool initted = false;

static void ensure_init(void) {
  if (initted) return;
  for (int i = 0; i < (int)EMOJI_COUNT; i++) {
    emoji_table[i].ascii_len = (int)strlen(emoji_table[i].ascii);
    emoji_table[i].emoji_len = (int)strlen(emoji_table[i].emoji);
  }
  initted = true;
}

/* Check if character before position is a word char (letter/digit) */
static bool is_word_char(const char *text, int pos) {
  if (pos <= 0) return false;
  unsigned char c = (unsigned char)text[pos - 1];
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

/* ================================================================
 * Public API
 * ================================================================ */

const MacmbxEmoji *macmbx_emoji_table(int *count) {
  ensure_init();
  if (count) *count = (int)EMOJI_COUNT;
  return (const MacmbxEmoji *)emoji_table;
}

const char *macmbx_emoji_lookup(const char *ascii) {
  if (!ascii) return NULL;
  ensure_init();
  for (int i = 0; i < (int)EMOJI_COUNT; i++) {
    if (strcmp(emoji_table[i].ascii, ascii) == 0)
      return emoji_table[i].emoji;
  }
  return NULL;
}

const char *macmbx_emoji_reverse(const char *emoji) {
  if (!emoji) return NULL;
  ensure_init();
  for (int i = 0; i < (int)EMOJI_COUNT; i++) {
    if (strcmp(emoji_table[i].emoji, emoji) == 0)
      return emoji_table[i].ascii;
  }
  return NULL;
}

char *macmbx_emoji_replace(const char *text) {
  if (!text) return NULL;
  ensure_init();

  long len = (long)strlen(text);
  /* Worst case: every char expands to 4-byte emoji */
  long cap = len * 4 + 1;
  char *out = (char *)malloc(cap);
  if (!out) return strdup(text);
  long o = 0;

  for (long i = 0; i < len; ) {
    /* Don't replace inside words — smiley must be at word boundary */
    bool at_boundary = !is_word_char(text, (int)i);

    if (at_boundary) {
      bool matched = false;
      for (int e = 0; e < (int)EMOJI_COUNT; e++) {
        int alen = emoji_table[e].ascii_len;
        if (i + alen <= len && memcmp(text + i, emoji_table[e].ascii, alen) == 0) {
          /* Check that smiley ends at word boundary too */
          long after = i + alen;
          bool end_boundary = (after >= len) ||
            (!(text[after] >= 'a' && text[after] <= 'z') &&
             !(text[after] >= 'A' && text[after] <= 'Z') &&
             !(text[after] >= '0' && text[after] <= '9'));
          /* Short smileys like :) :( need stricter boundary — skip if
             followed by path-like chars to avoid mangling URLs */
          if (alen <= 2 && after < len && (text[after] == '/' || text[after] == '\\'))
            end_boundary = false;

          if (end_boundary) {
            int elen = emoji_table[e].emoji_len;
            if (o + elen >= cap) { cap *= 2; out = realloc(out, cap); }
            memcpy(out + o, emoji_table[e].emoji, elen);
            o += elen;
            i += alen;
            matched = true;
            break;
          }
        }
      }
      if (matched) continue;
    }

    if (o + 1 >= cap) { cap *= 2; out = realloc(out, cap); }
    out[o++] = text[i++];
  }
  out[o] = '\0';
  return out;
}

char *macmbx_emoji_strip(const char *text) {
  if (!text) return NULL;
  ensure_init();

  long len = (long)strlen(text);
  long cap = len * 4 + 1; /* smileys are longer than emoji */
  char *out = (char *)malloc(cap);
  if (!out) return strdup(text);
  long o = 0;

  for (long i = 0; i < len; ) {
    bool matched = false;
    for (int e = 0; e < (int)EMOJI_COUNT; e++) {
      int elen = emoji_table[e].emoji_len;
      if (i + elen <= len && memcmp(text + i, emoji_table[e].emoji, elen) == 0) {
        /* Replace emoji with ASCII */
        int alen = emoji_table[e].ascii_len;
        if (o + alen >= cap) { cap *= 2; out = realloc(out, cap); }
        memcpy(out + o, emoji_table[e].ascii, alen);
        o += alen;
        i += elen;
        matched = true;
        break;
      }
    }
    if (!matched) {
      if (o + 1 >= cap) { cap *= 2; out = realloc(out, cap); }
      out[o++] = text[i++];
    }
  }
  out[o] = '\0';
  return out;
}
