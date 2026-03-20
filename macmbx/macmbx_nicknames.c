/* macmbx_nicknames.c — Nickname / Address Book management
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Eudora nickname file format compatible:
 *   alias nickname addr1,addr2,...
 *   alias "nick with spaces" addr1,addr2,...
 *   note nickname <key:value><key:value>...
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
#else
  #include <unistd.h>
  #include <dirent.h>
#endif

/* ================================================================
 * Hash
 * ================================================================ */

uint32_t macmbx_nick_hash(const char *s) {
  if (!s) return 0;
  uint32_t h = 5381;
  while (*s) {
    h = ((h << 5) + h) + (unsigned char)tolower(*s);
    s++;
  }
  return h ? h : 1;
}

/* ================================================================
 * Note fields
 * ================================================================ */

void macmbx_nick_free_notes(MacmbxNoteField *notes) {
  while (notes) {
    MacmbxNoteField *next = notes->next;
    free(notes->value);
    free(notes);
    notes = next;
  }
}

static MacmbxNoteField *note_find(MacmbxNoteField *notes, const char *key) {
  for (MacmbxNoteField *f = notes; f; f = f->next) {
    if (strcasecmp(f->key, key) == 0) return f;
  }
  return NULL;
}

static MacmbxNoteField *note_add(MacmbxNoteField **head, const char *key,
                                   const char *value) {
  MacmbxNoteField *f = (MacmbxNoteField *)calloc(1, sizeof(MacmbxNoteField));
  if (!f) return NULL;
  snprintf(f->key, sizeof(f->key), "%s", key);
  f->value = value ? strdup(value) : NULL;
  f->next = *head;
  *head = f;
  return f;
}

/* Find unescaped character, skipping \-escaped ones */
static const char *find_unescaped(const char *p, char c) {
  while (*p) {
    if (*p == '\\' && p[1]) { p += 2; continue; }
    if (*p == c) return p;
    p++;
  }
  return NULL;
}

/* Unescape \< \> \\ in a value string, in place */
static void unescape_value(char *s) {
  char *dst = s;
  while (*s) {
    if (*s == '\\' && (s[1] == '<' || s[1] == '>' || s[1] == '\\')) {
      *dst++ = s[1];
      s += 2;
    } else {
      *dst++ = *s++;
    }
  }
  *dst = '\0';
}

/* Escape < > \ in a value string. Returns malloc'd string. */
static char *escape_value(const char *s) {
  if (!s) return strdup("");
  size_t len = strlen(s);
  char *out = (char *)malloc(len * 2 + 1);
  if (!out) return strdup("");
  size_t o = 0;
  for (size_t i = 0; i < len; i++) {
    if (s[i] == '<' || s[i] == '>' || s[i] == '\\') out[o++] = '\\';
    out[o++] = s[i];
  }
  out[o] = '\0';
  return out;
}

/* Parse notes string: "<first:John><last:Doe><note:some \> text>" */
static MacmbxNoteField *parse_notes(const char *s) {
  if (!s) return NULL;
  MacmbxNoteField *head = NULL;
  const char *p = s;
  while (*p) {
    if (*p == '<') {
      p++;
      const char *colon = strchr(p, ':');
      const char *close = find_unescaped(p, '>');
      if (colon && close && colon < close) {
        char key[64];
        size_t klen = colon - p;
        if (klen >= sizeof(key)) klen = sizeof(key) - 1;
        memcpy(key, p, klen); key[klen] = '\0';

        const char *vstart = colon + 1;
        size_t vlen = close - vstart;
        char *value = (char *)malloc(vlen + 1);
        if (value) {
          memcpy(value, vstart, vlen); value[vlen] = '\0';
          unescape_value(value);
        }

        MacmbxNoteField *f = (MacmbxNoteField *)calloc(1, sizeof(MacmbxNoteField));
        if (f) {
          snprintf(f->key, sizeof(f->key), "%s", key);
          f->value = value;
          f->next = head;
          head = f;
        } else {
          free(value);
        }
        p = close + 1;
      } else {
        p++;
      }
    } else {
      p++;
    }
  }
  return head;
}

/* Serialize notes to string: "<first:John><last:Doe>" with escaping */
static char *notes_to_string(MacmbxNoteField *notes) {
  if (!notes) return strdup("");
  /* First pass: compute size with escaping */
  size_t len = 0;
  for (MacmbxNoteField *f = notes; f; f = f->next) {
    char *escaped = escape_value(f->value);
    len += strlen(f->key) + strlen(escaped) + 3; /* <key:escaped> */
    free(escaped);
  }
  char *buf = (char *)malloc(len + 1);
  if (!buf) return strdup("");
  size_t pos = 0;
  for (MacmbxNoteField *f = notes; f; f = f->next) {
    char *escaped = escape_value(f->value);
    pos += (size_t)sprintf(buf + pos, "<%s:%s>", f->key, escaped);
    free(escaped);
  }
  return buf;
}

/* ================================================================
 * Nickname helpers
 * ================================================================ */

static void free_nickname(MacmbxNickname *n) {
  free(n->addresses);
  macmbx_nick_free_notes(n->notes);
  memset(n, 0, sizeof(*n));
}

static void ensure_capacity(MacmbxAddressBook *book, int needed) {
  if (needed <= book->capacity) return;
  int newCap = book->capacity ? book->capacity * 2 : 32;
  while (newCap < needed) newCap *= 2;
  book->entries = (MacmbxNickname *)realloc(book->entries,
    newCap * sizeof(MacmbxNickname));
  book->capacity = newCap;
}

/* Compute address hash from first address in comma-separated list */
static uint32_t hash_first_address(const char *addresses) {
  if (!addresses || !*addresses) return 0;
  char first[256];
  const char *comma = strchr(addresses, ',');
  size_t len = comma ? (size_t)(comma - addresses) : strlen(addresses);
  if (len >= sizeof(first)) len = sizeof(first) - 1;
  memcpy(first, addresses, len); first[len] = '\0';
  /* Trim whitespace */
  char *s = first;
  while (*s == ' ') s++;
  char *e = s + strlen(s);
  while (e > s && e[-1] == ' ') e--;
  *e = '\0';
  /* Lowercase for consistent hashing */
  for (char *p = s; *p; p++) *p = tolower((unsigned char)*p);
  return macmbx_nick_hash(s);
}

/* ================================================================
 * File I/O — read Eudora nickname file
 * ================================================================ */

static int read_nick_file(MacmbxAddressBook *book) {
  FILE *f = fopen(book->path, "r");
  if (!f) return -1;

  char line[4096];
  while (fgets(line, sizeof(line), f)) {
    /* Strip trailing whitespace */
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n' ||
                        line[len-1] == ' ')) line[--len] = '\0';
    if (len == 0) continue;

    /* Parse command: "alias" or "note" */
    bool is_alias = (strncasecmp(line, "alias ", 6) == 0);
    bool is_note = (strncasecmp(line, "note ", 5) == 0);
    if (!is_alias && !is_note) continue;

    char *p = line + (is_alias ? 6 : 5);
    while (*p == ' ') p++;

    /* Parse nickname (may be quoted) */
    char nick[256];
    if (*p == '"') {
      p++;
      char *end = strchr(p, '"');
      if (!end) continue;
      size_t nlen = end - p;
      if (nlen >= sizeof(nick)) nlen = sizeof(nick) - 1;
      memcpy(nick, p, nlen); nick[nlen] = '\0';
      p = end + 1;
    } else {
      char *space = strchr(p, ' ');
      if (!space) {
        snprintf(nick, sizeof(nick), "%s", p);
        p = p + strlen(p);
      } else {
        size_t nlen = space - p;
        if (nlen >= sizeof(nick)) nlen = sizeof(nick) - 1;
        memcpy(nick, p, nlen); nick[nlen] = '\0';
        p = space;
      }
    }
    while (*p == ' ') p++;

    /* Find or create the nickname entry */
    int idx = macmbx_nick_find(book, nick);
    if (idx < 0) {
      ensure_capacity(book, book->count + 1);
      idx = book->count++;
      memset(&book->entries[idx], 0, sizeof(MacmbxNickname));
      snprintf(book->entries[idx].name, sizeof(book->entries[idx].name),
               "%s", nick);
      book->entries[idx].name_hash = macmbx_nick_hash(nick);
    }

    if (is_alias) {
      /* Rest of line is addresses */
      free(book->entries[idx].addresses);
      book->entries[idx].addresses = strdup(p);
      book->entries[idx].addr_hash = hash_first_address(p);
    } else {
      /* Rest of line is notes in <key:value> format */
      macmbx_nick_free_notes(book->entries[idx].notes);
      book->entries[idx].notes = parse_notes(p);
    }
  }
  fclose(f);
  book->dirty = false;
  return 0;
}

/* Write Eudora nickname file */
int macmbx_nick_save_book(MacmbxAddressBook *book) {
  if (!book || !book->dirty) return 0;

  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s.tmp", book->path);
  FILE *f = fopen(tmp, "w");
  if (!f) return -1;

  /* Write all alias lines first, then all note lines (Eudora convention) */
  for (int i = 0; i < book->count; i++) {
    MacmbxNickname *n = &book->entries[i];
    if (n->deleted || !n->addresses || !n->addresses[0]) continue;
    /* Quote name if it contains spaces */
    if (strchr(n->name, ' '))
      fprintf(f, "alias \"%s\" %s\n", n->name, n->addresses);
    else
      fprintf(f, "alias %s %s\n", n->name, n->addresses);
  }

  for (int i = 0; i < book->count; i++) {
    MacmbxNickname *n = &book->entries[i];
    if (n->deleted || !n->notes) continue;
    char *noteStr = notes_to_string(n->notes);
    if (noteStr && noteStr[0]) {
      if (strchr(n->name, ' '))
        fprintf(f, "note \"%s\" %s\n", n->name, noteStr);
      else
        fprintf(f, "note %s %s\n", n->name, noteStr);
    }
    free(noteStr);
  }

  fclose(f);
  if (rename(tmp, book->path) != 0) { remove(tmp); return -1; }
  book->dirty = false;
  return 0;
}

/* ================================================================
 * Lifecycle
 * ================================================================ */

MacmbxAddressBooks *macmbx_nick_open(const char *nick_dir) {
  if (!nick_dir) return NULL;

  /* Create directory if needed */
  struct stat st;
  if (stat(nick_dir, &st) != 0) {
#ifdef _WIN32
    _mkdir(nick_dir);
#else
    mkdir(nick_dir, 0755);
#endif
  }

  MacmbxAddressBooks *abs = (MacmbxAddressBooks *)calloc(1, sizeof(MacmbxAddressBooks));
  if (!abs) return NULL;
  snprintf(abs->dir_path, sizeof(abs->dir_path), "%s", nick_dir);
  abs->capacity = 8;
  abs->books = (MacmbxAddressBook *)calloc(abs->capacity, sizeof(MacmbxAddressBook));

  /* Scan directory for nickname files */
#ifdef _WIN32
  /* TODO: Windows FindFirstFile */
#else
  DIR *d = opendir(nick_dir);
  if (d) {
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_name[0] == '.') continue;
      /* Skip .toc and other non-nickname files */
      size_t nlen = strlen(entry->d_name);
      if (nlen > 4 && strcmp(entry->d_name + nlen - 4, ".toc") == 0) continue;
      if (nlen > 4 && strcmp(entry->d_name + nlen - 4, ".tmp") == 0) continue;
      if (nlen > 5 && strcmp(entry->d_name + nlen - 5, ".lock") == 0) continue;

      char full[PATH_MAX];
      snprintf(full, sizeof(full), "%s/%s", nick_dir, entry->d_name);
      if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;

      if (abs->count >= abs->capacity) {
        abs->capacity *= 2;
        abs->books = (MacmbxAddressBook *)realloc(abs->books,
          abs->capacity * sizeof(MacmbxAddressBook));
      }
      MacmbxAddressBook *book = &abs->books[abs->count];
      memset(book, 0, sizeof(*book));
      snprintf(book->name, sizeof(book->name), "%s", entry->d_name);
      snprintf(book->path, sizeof(book->path), "%s", full);
      read_nick_file(book);
      abs->count++;
    }
    closedir(d);
  }
#endif

  return abs;
}

void macmbx_nick_close(MacmbxAddressBooks *abs) {
  if (!abs) return;
  for (int b = 0; b < abs->count; b++) {
    for (int i = 0; i < abs->books[b].count; i++)
      free_nickname(&abs->books[b].entries[i]);
    free(abs->books[b].entries);
  }
  free(abs->books);
  free(abs);
}

int macmbx_nick_save(MacmbxAddressBooks *abs) {
  if (!abs) return -1;
  int err = 0;
  for (int b = 0; b < abs->count; b++) {
    if (abs->books[b].dirty) {
      if (macmbx_nick_save_book(&abs->books[b]) != 0) err = -1;
    }
  }
  return err;
}

/* ================================================================
 * Book management
 * ================================================================ */

MacmbxAddressBook *macmbx_nick_create_book(MacmbxAddressBooks *abs,
                                             const char *name) {
  if (!abs || !name) return NULL;
  if (abs->count >= abs->capacity) {
    abs->capacity *= 2;
    abs->books = (MacmbxAddressBook *)realloc(abs->books,
      abs->capacity * sizeof(MacmbxAddressBook));
  }
  MacmbxAddressBook *book = &abs->books[abs->count];
  memset(book, 0, sizeof(*book));
  snprintf(book->name, sizeof(book->name), "%s", name);
  snprintf(book->path, sizeof(book->path), "%s/%s", abs->dir_path, name);
  book->dirty = true;
  abs->count++;
  /* Create empty file */
  FILE *f = fopen(book->path, "w");
  if (f) fclose(f);
  return book;
}

int macmbx_nick_remove_book(MacmbxAddressBooks *abs, int book_index) {
  if (!abs || book_index < 0 || book_index >= abs->count) return -1;
  MacmbxAddressBook *book = &abs->books[book_index];
  remove(book->path);
  for (int i = 0; i < book->count; i++) free_nickname(&book->entries[i]);
  free(book->entries);
  memmove(&abs->books[book_index], &abs->books[book_index + 1],
          (abs->count - book_index - 1) * sizeof(MacmbxAddressBook));
  abs->count--;
  return 0;
}

MacmbxAddressBook *macmbx_nick_get_book(MacmbxAddressBooks *abs, int index) {
  if (!abs || index < 0 || index >= abs->count) return NULL;
  return &abs->books[index];
}

MacmbxAddressBook *macmbx_nick_find_book(MacmbxAddressBooks *abs,
                                           const char *name) {
  if (!abs || !name) return NULL;
  for (int i = 0; i < abs->count; i++) {
    if (strcasecmp(abs->books[i].name, name) == 0) return &abs->books[i];
  }
  return NULL;
}

/* ================================================================
 * Nickname CRUD
 * ================================================================ */

int macmbx_nick_add(MacmbxAddressBook *book, const char *name,
                     const char *addresses) {
  if (!book || !name) return -1;
  ensure_capacity(book, book->count + 1);
  int idx = book->count++;
  MacmbxNickname *n = &book->entries[idx];
  memset(n, 0, sizeof(*n));
  snprintf(n->name, sizeof(n->name), "%s", name);
  n->addresses = addresses ? strdup(addresses) : NULL;
  n->name_hash = macmbx_nick_hash(name);
  n->addr_hash = hash_first_address(addresses);
  book->dirty = true;
  return idx;
}

int macmbx_nick_remove(MacmbxAddressBook *book, int index) {
  if (!book || index < 0 || index >= book->count) return -1;
  book->entries[index].deleted = true;
  book->dirty = true;
  return 0;
}

int macmbx_nick_find(MacmbxAddressBook *book, const char *name) {
  if (!book || !name) return -1;
  uint32_t hash = macmbx_nick_hash(name);
  for (int i = 0; i < book->count; i++) {
    if (!book->entries[i].deleted && book->entries[i].name_hash == hash &&
        strcasecmp(book->entries[i].name, name) == 0)
      return i;
  }
  return -1;
}

int macmbx_nick_find_all(MacmbxAddressBooks *abs, const char *name,
                           int *book_idx) {
  if (!abs || !name) return -1;
  for (int b = 0; b < abs->count; b++) {
    int idx = macmbx_nick_find(&abs->books[b], name);
    if (idx >= 0) {
      if (book_idx) *book_idx = b;
      return idx;
    }
  }
  return -1;
}

const char *macmbx_nick_get_addresses(MacmbxAddressBook *book, int index) {
  if (!book || index < 0 || index >= book->count) return NULL;
  return book->entries[index].addresses;
}

int macmbx_nick_set_addresses(MacmbxAddressBook *book, int index,
                                const char *addresses) {
  if (!book || index < 0 || index >= book->count) return -1;
  free(book->entries[index].addresses);
  book->entries[index].addresses = addresses ? strdup(addresses) : NULL;
  book->entries[index].addr_hash = hash_first_address(addresses);
  book->dirty = true;
  return 0;
}

int macmbx_nick_rename(MacmbxAddressBook *book, int index, const char *new_name) {
  if (!book || index < 0 || index >= book->count || !new_name) return -1;
  snprintf(book->entries[index].name, sizeof(book->entries[index].name),
           "%s", new_name);
  book->entries[index].name_hash = macmbx_nick_hash(new_name);
  book->dirty = true;
  return 0;
}

/* ================================================================
 * Note fields
 * ================================================================ */

const char *macmbx_nick_get_field(MacmbxAddressBook *book, int index,
                                    const char *key) {
  if (!book || index < 0 || index >= book->count || !key) return NULL;
  MacmbxNoteField *f = note_find(book->entries[index].notes, key);
  return f ? f->value : NULL;
}

int macmbx_nick_set_field(MacmbxAddressBook *book, int index,
                            const char *key, const char *value) {
  if (!book || index < 0 || index >= book->count || !key) return -1;
  MacmbxNoteField *f = note_find(book->entries[index].notes, key);
  if (f) {
    free(f->value);
    f->value = value ? strdup(value) : NULL;
  } else {
    note_add(&book->entries[index].notes, key, value);
  }
  book->dirty = true;
  return 0;
}

int macmbx_nick_remove_field(MacmbxAddressBook *book, int index,
                               const char *key) {
  if (!book || index < 0 || index >= book->count || !key) return -1;
  MacmbxNoteField **pp = &book->entries[index].notes;
  while (*pp) {
    if (strcasecmp((*pp)->key, key) == 0) {
      MacmbxNoteField *f = *pp;
      *pp = f->next;
      free(f->value);
      free(f);
      book->dirty = true;
      return 0;
    }
    pp = &(*pp)->next;
  }
  return -1;
}

/* ================================================================
 * Lookup / Search
 * ================================================================ */

/* Case-insensitive strstr */
static bool ci_contains(const char *hay, const char *needle) {
  if (!hay || !needle) return false;
  size_t nlen = strlen(needle);
  if (nlen == 0) return true;
  for (; *hay; hay++) {
    if (strncasecmp(hay, needle, nlen) == 0) return true;
  }
  return false;
}

bool macmbx_nick_contains_address(MacmbxAddressBooks *abs,
                                    const char *email) {
  if (!abs || !email) return false;
  for (int b = 0; b < abs->count; b++) {
    for (int i = 0; i < abs->books[b].count; i++) {
      MacmbxNickname *n = &abs->books[b].entries[i];
      if (n->deleted || !n->addresses) continue;
      if (ci_contains(n->addresses, email)) return true;
    }
  }
  return false;
}

bool macmbx_nick_contains_hash(MacmbxAddressBooks *abs, uint32_t addr_hash) {
  if (!abs || !addr_hash) return false;
  for (int b = 0; b < abs->count; b++) {
    for (int i = 0; i < abs->books[b].count; i++) {
      if (!abs->books[b].entries[i].deleted &&
          abs->books[b].entries[i].addr_hash == addr_hash)
        return true;
    }
  }
  return false;
}

char *macmbx_nick_expand(MacmbxAddressBooks *abs, const char *name) {
  if (!abs || !name) return NULL;
  int book_idx;
  int idx = macmbx_nick_find_all(abs, name, &book_idx);
  if (idx < 0) return NULL;
  const char *addr = abs->books[book_idx].entries[idx].addresses;
  return addr ? strdup(addr) : NULL;
}

int macmbx_nick_search(MacmbxAddressBooks *abs, const char *pattern,
                         int **results) {
  if (!abs || !pattern || !results) return 0;
  *results = NULL;
  int count = 0, cap = 32;
  *results = (int *)calloc(cap, sizeof(int) * 2); /* pairs: book_idx, nick_idx */

  for (int b = 0; b < abs->count; b++) {
    for (int i = 0; i < abs->books[b].count; i++) {
      MacmbxNickname *n = &abs->books[b].entries[i];
      if (n->deleted) continue;
      bool match = ci_contains(n->name, pattern) ||
                   ci_contains(n->addresses, pattern);
      if (!match) {
        /* Search note fields */
        for (MacmbxNoteField *f = n->notes; f && !match; f = f->next) {
          if (ci_contains(f->value, pattern)) match = true;
        }
      }
      if (match) {
        if (count * 2 + 1 >= cap) {
          cap *= 2;
          *results = (int *)realloc(*results, cap * sizeof(int));
        }
        (*results)[count * 2] = b;
        (*results)[count * 2 + 1] = i;
        count++;
      }
    }
  }
  return count;
}

/* ================================================================
 * Autocomplete
 * ================================================================ */

/* Match priority: lower = better */
#define MATCH_NAME_PREFIX    1
#define MATCH_FIRSTLAST      2
#define MATCH_ADDR_PREFIX    3
#define MATCH_NAME_CONTAINS  4
#define MATCH_ADDR_CONTAINS  5

typedef struct {
  MacmbxCompleteResult result;
  int priority;
  char display_buf[320]; /* "First Last <addr>" */
} CompleteEntry;

static bool ci_prefix(const char *s, const char *prefix) {
  return s && prefix && strncasecmp(s, prefix, strlen(prefix)) == 0;
}

int macmbx_nick_complete(MacmbxAddressBooks *abs, const char *prefix,
                          MacmbxCompleteResult **results, int max_results) {
  if (!abs || !prefix || !results) return 0;
  *results = NULL;
  if (!*prefix) return 0;

  int count = 0, cap = 32;
  CompleteEntry *entries = (CompleteEntry *)calloc(cap, sizeof(CompleteEntry));
  if (!entries) return 0;

  for (int b = 0; b < abs->count; b++) {
    for (int i = 0; i < abs->books[b].count; i++) {
      MacmbxNickname *n = &abs->books[b].entries[i];
      if (n->deleted) continue;

      int priority = 0;

      /* Check nickname prefix */
      if (ci_prefix(n->name, prefix)) {
        priority = MATCH_NAME_PREFIX;
      }
      /* Check first+last name */
      if (!priority) {
        MacmbxNoteField *ff = note_find(n->notes, "first");
        MacmbxNoteField *lf = note_find(n->notes, "last");
        if (ff && ff->value && ci_prefix(ff->value, prefix)) {
          priority = MATCH_FIRSTLAST;
        } else if (lf && lf->value && ci_prefix(lf->value, prefix)) {
          priority = MATCH_FIRSTLAST;
        } else if (ff && lf && ff->value && lf->value) {
          /* Try "First Last" combined */
          char fullname[256];
          snprintf(fullname, sizeof(fullname), "%s %s", ff->value, lf->value);
          if (ci_prefix(fullname, prefix)) priority = MATCH_FIRSTLAST;
        }
      }
      /* Check address prefix */
      if (!priority && n->addresses && ci_prefix(n->addresses, prefix)) {
        priority = MATCH_ADDR_PREFIX;
      }
      /* Check name contains (not just prefix) */
      if (!priority && ci_contains(n->name, prefix)) {
        priority = MATCH_NAME_CONTAINS;
      }
      /* Check address contains */
      if (!priority && n->addresses && ci_contains(n->addresses, prefix)) {
        priority = MATCH_ADDR_CONTAINS;
      }

      if (!priority) continue;

      if (count >= cap) {
        cap *= 2;
        entries = (CompleteEntry *)realloc(entries, cap * sizeof(CompleteEntry));
      }

      CompleteEntry *e = &entries[count];
      e->priority = priority;
      e->result.book_idx = b;
      e->result.nick_idx = i;
      e->result.name = n->name;
      e->result.addresses = n->addresses;

      /* Build display string: "First Last <first_addr>" or "nick <first_addr>" */
      MacmbxNoteField *ff = note_find(n->notes, "first");
      MacmbxNoteField *lf = note_find(n->notes, "last");
      const char *first_addr = n->addresses;
      char addr_part[256] = "";
      if (first_addr) {
        const char *comma = strchr(first_addr, ',');
        size_t alen = comma ? (size_t)(comma - first_addr) : strlen(first_addr);
        if (alen >= sizeof(addr_part)) alen = sizeof(addr_part) - 1;
        memcpy(addr_part, first_addr, alen);
        addr_part[alen] = '\0';
      }

      if (ff && ff->value && lf && lf->value) {
        snprintf(e->display_buf, sizeof(e->display_buf), "%s %s <%s>",
                 ff->value, lf->value, addr_part);
      } else if (ff && ff->value) {
        snprintf(e->display_buf, sizeof(e->display_buf), "%s <%s>",
                 ff->value, addr_part);
      } else {
        snprintf(e->display_buf, sizeof(e->display_buf), "%s <%s>",
                 n->name, addr_part);
      }
      e->result.display = e->display_buf;
      count++;
    }
  }

  /* Sort by priority (stable within same priority) */
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (entries[j].priority < entries[i].priority) {
        CompleteEntry tmp = entries[i];
        entries[i] = entries[j];
        entries[j] = tmp;
      }
    }
  }

  /* Limit results */
  if (max_results > 0 && count > max_results) count = max_results;

  /* Copy to output */
  *results = (MacmbxCompleteResult *)calloc(count, sizeof(MacmbxCompleteResult));
  if (!*results) { free(entries); return 0; }
  for (int i = 0; i < count; i++) {
    (*results)[i] = entries[i].result;
    /* display pointer needs to point into the result's own storage —
       but we're about to free entries. Copy display into a stable place.
       Since we can't malloc per-result (caller frees array only),
       we'll embed display in the addresses field... no, let's just
       allocate the display strings. */
  }

  /* Actually, let's allocate display strings that the caller can free */
  /* Re-do: allocate a flat buffer for all display strings */
  size_t total_display = 0;
  for (int i = 0; i < count; i++)
    total_display += strlen(entries[i].display_buf) + 1;

  char *display_block = (char *)malloc(total_display);
  if (display_block) {
    size_t offset = 0;
    for (int i = 0; i < count; i++) {
      size_t dlen = strlen(entries[i].display_buf);
      memcpy(display_block + offset, entries[i].display_buf, dlen + 1);
      (*results)[i].display = display_block + offset;
      offset += dlen + 1;
    }
  }

  free(entries);
  return count;
}

/* ================================================================
 * Import / Export — Eudora format
 * ================================================================ */

int macmbx_nick_import_eudora(MacmbxAddressBook *book, const char *eudora_file) {
  if (!book || !eudora_file) return -1;
  /* Save original path, temporarily point to eudora file, read, restore */
  char saved[PATH_MAX];
  snprintf(saved, sizeof(saved), "%s", book->path);
  snprintf(book->path, sizeof(book->path), "%s", eudora_file);
  int err = read_nick_file(book);
  snprintf(book->path, sizeof(book->path), "%s", saved);
  book->dirty = true;
  return err;
}

int macmbx_nick_export_eudora(MacmbxAddressBook *book, const char *eudora_file) {
  if (!book || !eudora_file) return -1;
  char saved[PATH_MAX];
  snprintf(saved, sizeof(saved), "%s", book->path);
  snprintf(book->path, sizeof(book->path), "%s", eudora_file);
  book->dirty = true;
  int err = macmbx_nick_save_book(book);
  snprintf(book->path, sizeof(book->path), "%s", saved);
  return err;
}

/* ================================================================
 * Import / Export — vCard (.vcf) via macmbx_vcard parser
 * ================================================================ */

#include "macmbx_vcard.h"

int macmbx_nick_import_vcard(MacmbxAddressBook *book, const char *vcf_path) {
  if (!book || !vcf_path) return -1;

  MacmbxVcard **cards = NULL;
  int card_count = macmbx_vcard_parse_file(vcf_path, &cards);
  if (card_count <= 0) return 0;

  int imported = 0;
  for (int c = 0; c < card_count; c++) {
    MacmbxVcard *vc = cards[c];
    if (!vc) continue;

    /* Determine nickname */
    char nick[256] = "";
    const char *fn = macmbx_vcard_fn(vc);
    const char *given = macmbx_vcard_given(vc);
    const char *family = macmbx_vcard_family(vc);
    const char *email = macmbx_vcard_email(vc);

    if (fn && fn[0]) snprintf(nick, sizeof(nick), "%s", fn);
    else if (given || family) snprintf(nick, sizeof(nick), "%s%s%s",
      given ? given : "", (given && family) ? " " : "", family ? family : "");
    else if (email) snprintf(nick, sizeof(nick), "%s", email);
    else { macmbx_vcard_free(vc); continue; }

    /* Collect all emails */
    const char *emails[16];
    int email_count = macmbx_vcard_emails(vc, emails, 16);
    char addrs[1024] = "";
    for (int e = 0; e < email_count; e++) {
      if (e > 0) strncat(addrs, ",", sizeof(addrs) - strlen(addrs) - 1);
      strncat(addrs, emails[e], sizeof(addrs) - strlen(addrs) - 1);
    }

    int idx = macmbx_nick_add(book, nick, addrs[0] ? addrs : NULL);
    if (idx >= 0) {
      if (given) macmbx_nick_set_field(book, idx, "first", given);
      if (family) macmbx_nick_set_field(book, idx, "last", family);

      /* Phone — try types */
      const char *ph;
      if ((ph = macmbx_vcard_phone(vc, "HOME")))
        macmbx_nick_set_field(book, idx, "phone", ph);
      else if ((ph = macmbx_vcard_phone(vc, NULL)))
        macmbx_nick_set_field(book, idx, "phone", ph);
      if ((ph = macmbx_vcard_phone(vc, "WORK")))
        macmbx_nick_set_field(book, idx, "phone2", ph);
      if ((ph = macmbx_vcard_phone(vc, "CELL")))
        macmbx_nick_set_field(book, idx, "mobile", ph);
      if ((ph = macmbx_vcard_phone(vc, "FAX")))
        macmbx_nick_set_field(book, idx, "fax", ph);

      /* Organization, title */
      const char *org = macmbx_vcard_org(vc);
      if (org) macmbx_nick_set_field(book, idx, "company", org);
      const char *title = macmbx_vcard_get(vc, "TITLE");
      if (title) macmbx_nick_set_field(book, idx, "title", title);

      /* URL */
      const char *url = macmbx_vcard_get(vc, "URL");
      if (url) macmbx_nick_set_field(book, idx, "web", url);

      /* Note */
      const char *note = macmbx_vcard_get(vc, "NOTE");
      if (note) macmbx_nick_set_field(book, idx, "note", note);

      /* Address — ADR: PO;Ext;Street;City;State;Zip;Country */
      const char *street = macmbx_vcard_get_part(vc, "ADR", 2);
      const char *city = macmbx_vcard_get_part(vc, "ADR", 3);
      const char *state = macmbx_vcard_get_part(vc, "ADR", 4);
      const char *zip = macmbx_vcard_get_part(vc, "ADR", 5);
      const char *country = macmbx_vcard_get_part(vc, "ADR", 6);
      if (street && street[0]) macmbx_nick_set_field(book, idx, "address", street);
      if (city && city[0]) macmbx_nick_set_field(book, idx, "city", city);
      if (state && state[0]) macmbx_nick_set_field(book, idx, "state", state);
      if (zip && zip[0]) macmbx_nick_set_field(book, idx, "zip", zip);
      if (country && country[0]) macmbx_nick_set_field(book, idx, "country", country);

      imported++;
    }
    macmbx_vcard_free(vc);
  }
  free(cards);
  return imported;
}

char *macmbx_nick_export_vcard(MacmbxAddressBook *book, int index) {
  if (!book || index < 0 || index >= book->count) return NULL;
  MacmbxNickname *n = &book->entries[index];
  if (n->deleted) return NULL;

  MacmbxVcard *vc = macmbx_vcard_new();
  if (!vc) return NULL;

  /* FN + N */
  const char *first = macmbx_nick_get_field(book, index, "first");
  const char *last = macmbx_nick_get_field(book, index, "last");
  if (first || last) {
    char fn[256];
    snprintf(fn, sizeof(fn), "%s%s%s", first ? first : "",
             (first && last) ? " " : "", last ? last : "");
    macmbx_vcard_add(vc, "FN", fn);
    const char *n_parts[] = { last ? last : "", first ? first : "", "", "", "" };
    macmbx_vcard_add_structured(vc, "N", n_parts, 5);
  } else {
    macmbx_vcard_add(vc, "FN", n->name);
  }

  /* EMAIL(s) */
  if (n->addresses) {
    char *addrs = strdup(n->addresses);
    char *tok = strtok(addrs, ",");
    while (tok) {
      while (*tok == ' ') tok++;
      macmbx_vcard_add_typed(vc, "EMAIL", tok, "INTERNET");
      tok = strtok(NULL, ",");
    }
    free(addrs);
  }

  /* Phones with types */
  const char *v;
  if ((v = macmbx_nick_get_field(book, index, "phone")))
    macmbx_vcard_add_typed(vc, "TEL", v, "HOME,VOICE");
  if ((v = macmbx_nick_get_field(book, index, "phone2")))
    macmbx_vcard_add_typed(vc, "TEL", v, "WORK,VOICE");
  if ((v = macmbx_nick_get_field(book, index, "mobile")))
    macmbx_vcard_add_typed(vc, "TEL", v, "CELL");
  if ((v = macmbx_nick_get_field(book, index, "fax")))
    macmbx_vcard_add_typed(vc, "TEL", v, "FAX");

  /* Org, title, URL, note */
  if ((v = macmbx_nick_get_field(book, index, "company"))) macmbx_vcard_add(vc, "ORG", v);
  if ((v = macmbx_nick_get_field(book, index, "title"))) macmbx_vcard_add(vc, "TITLE", v);
  if ((v = macmbx_nick_get_field(book, index, "web"))) macmbx_vcard_add(vc, "URL", v);
  if ((v = macmbx_nick_get_field(book, index, "note"))) macmbx_vcard_add(vc, "NOTE", v);

  /* Address — ADR: PO;Ext;Street;City;State;Zip;Country */
  const char *addr = macmbx_nick_get_field(book, index, "address");
  const char *city = macmbx_nick_get_field(book, index, "city");
  const char *state = macmbx_nick_get_field(book, index, "state");
  const char *zip = macmbx_nick_get_field(book, index, "zip");
  const char *country = macmbx_nick_get_field(book, index, "country");
  if (addr || city || state || zip || country) {
    const char *adr_parts[] = { "", "", addr ? addr : "", city ? city : "",
      state ? state : "", zip ? zip : "", country ? country : "" };
    macmbx_vcard_add_structured(vc, "ADR", adr_parts, 7);
  }

  char *result = macmbx_vcard_serialize(vc);
  macmbx_vcard_free(vc);
  return result;
}
