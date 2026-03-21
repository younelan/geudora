/* macmbx_thread.c — Message threading, dedup, import/export
 * Part of macmbx: standalone Eudora mbox storage library.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <io.h>
  #include <direct.h>
#else
  #include <unistd.h>
  #include <dirent.h>
#endif

/* ================================================================
 * Hash helper (same as in macmbx_build.c)
 * ================================================================ */

static int32_t hash_str(const char *s, int len) {
  uint32_t h = 5381;
  for (int i = 0; i < len; i++)
    h = ((h << 5) + h) + (unsigned char)s[i];
  return h ? (int32_t)h : 1;
}

/* Extract Message-ID hash from raw headers */
static int32_t extract_msgid_hash(const char *msg, long len) {
  const char *p = msg;
  const char *end = msg + len;
  while (p < end) {
    if (strncasecmp(p, "Message-ID:", 11) == 0 ||
        strncasecmp(p, "Message-Id:", 11) == 0) {
      p += 11;
      while (p < end && (*p == ' ' || *p == '\t')) p++;
      const char *id = p;
      if (*id == '<') id++;
      const char *idEnd = id;
      while (idEnd < end && *idEnd != '>' && *idEnd != '\r' && *idEnd != '\n')
        idEnd++;
      if (idEnd > id) return hash_str(id, (int)(idEnd - id));
      break;
    }
    /* Skip to next line */
    while (p < end && *p != '\n') p++;
    if (p < end) p++;
    /* End of headers */
    if (p < end && (*p == '\r' || *p == '\n')) break;
  }
  return 0;
}

/* ================================================================
 * Message threading
 *
 * Algorithm (simplified JWZ threading):
 * 1. Build hash table: msg_id_hash -> thread node
 * 2. For each message with in_reply_to_hash, link child->parent
 * 3. Collect root nodes (no parent)
 * 4. Sort roots by date
 * ================================================================ */

#define THREAD_HASH_SIZE 4096

typedef struct ThreadEntry {
  int32_t hash;
  MacmbxThread *node;
  struct ThreadEntry *next;
} ThreadEntry;

static ThreadEntry **thread_table_new(void) {
  return (ThreadEntry **)calloc(THREAD_HASH_SIZE, sizeof(ThreadEntry *));
}

static void thread_table_free(ThreadEntry **table) {
  if (!table) return;
  for (int i = 0; i < THREAD_HASH_SIZE; i++) {
    ThreadEntry *e = table[i];
    while (e) { ThreadEntry *next = e->next; free(e); e = next; }
  }
  free(table);
}

static MacmbxThread *thread_table_find(ThreadEntry **table, int32_t hash) {
  if (!hash) return NULL;
  int bucket = ((uint32_t)hash) % THREAD_HASH_SIZE;
  for (ThreadEntry *e = table[bucket]; e; e = e->next) {
    if (e->hash == hash) return e->node;
  }
  return NULL;
}

static void thread_table_insert(ThreadEntry **table, int32_t hash,
                                 MacmbxThread *node) {
  if (!hash) return;
  int bucket = ((uint32_t)hash) % THREAD_HASH_SIZE;
  ThreadEntry *e = (ThreadEntry *)calloc(1, sizeof(ThreadEntry));
  e->hash = hash;
  e->node = node;
  e->next = table[bucket];
  table[bucket] = e;
}

static MacmbxThread *thread_new(int index) {
  MacmbxThread *t = (MacmbxThread *)calloc(1, sizeof(MacmbxThread));
  t->index = index;
  return t;
}

static void thread_add_child(MacmbxThread *parent, MacmbxThread *child) {
  child->parent = parent;
  child->depth = parent->depth + 1;
  child->next = parent->child;
  parent->child = child;
}

MacmbxThread *macmbx_build_threads(MacmbxTOC *toc) {
  if (!toc || toc->count == 0) return NULL;

  ThreadEntry **table = thread_table_new();
  MacmbxThread **nodes = (MacmbxThread **)calloc(toc->count, sizeof(MacmbxThread *));

  /* Pass 1: create nodes, index by msg_id_hash */
  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    nodes[i] = thread_new(i);
    if (toc->msgs[i].msg_id_hash)
      thread_table_insert(table, toc->msgs[i].msg_id_hash, nodes[i]);
  }

  /* Pass 2: link children to parents via in_reply_to_hash */
  for (int i = 0; i < toc->count; i++) {
    if (!nodes[i]) continue;
    int32_t parent_hash = toc->msgs[i].in_reply_to_hash;
    if (parent_hash) {
      MacmbxThread *parent = thread_table_find(table, parent_hash);
      if (parent && parent != nodes[i]) {
        thread_add_child(parent, nodes[i]);
      }
    }
  }

  /* Pass 3: collect roots (nodes without parents) */
  MacmbxThread *roots = NULL;
  MacmbxThread *lastRoot = NULL;
  for (int i = 0; i < toc->count; i++) {
    if (!nodes[i]) continue;
    if (!nodes[i]->parent) {
      if (!roots) roots = nodes[i];
      else lastRoot->next = nodes[i];
      lastRoot = nodes[i];
      nodes[i]->next = NULL; /* will be set by next root */
    }
  }
  /* Ensure last root's next is NULL */
  if (lastRoot) lastRoot->next = NULL;

  thread_table_free(table);
  free(nodes);
  return roots;
}

/* Flatten thread tree depth-first */
static void flatten_walk(MacmbxThread *t, int **indices, int **depths,
                          int *count, int *cap) {
  for (MacmbxThread *n = t; n; n = n->next) {
    if (*count >= *cap) {
      *cap *= 2;
      *indices = (int *)realloc(*indices, *cap * sizeof(int));
      *depths = (int *)realloc(*depths, *cap * sizeof(int));
    }
    (*indices)[*count] = n->index;
    (*depths)[*count] = n->depth;
    (*count)++;
    if (n->child) flatten_walk(n->child, indices, depths, count, cap);
  }
}

int macmbx_thread_flatten(MacmbxThread *threads, int **indices, int **depths) {
  if (!threads || !indices || !depths) return 0;
  int count = 0, cap = 256;
  *indices = (int *)calloc(cap, sizeof(int));
  *depths = (int *)calloc(cap, sizeof(int));
  flatten_walk(threads, indices, depths, &count, &cap);
  return count;
}

static void free_thread_children(MacmbxThread *t) {
  if (!t) return;
  free_thread_children(t->child);
  free_thread_children(t->next);
  free(t);
}

void macmbx_threads_free(MacmbxThread *threads) {
  free_thread_children(threads);
}

static MacmbxThread *find_in_thread(MacmbxThread *t, int index) {
  for (MacmbxThread *n = t; n; n = n->next) {
    if (n->index == index) return n;
    if (n->child) {
      MacmbxThread *found = find_in_thread(n->child, index);
      if (found) return found;
    }
  }
  return NULL;
}

MacmbxThread *macmbx_thread_find(MacmbxThread *threads, int index) {
  MacmbxThread *node = find_in_thread(threads, index);
  if (!node) return NULL;
  /* Walk up to root */
  while (node->parent) node = node->parent;
  return node;
}

int macmbx_thread_count(MacmbxThread *threads) {
  int count = 0;
  for (MacmbxThread *t = threads; t; t = t->next) count++;
  return count;
}

/* ================================================================
 * Message deduplication
 * ================================================================ */

int macmbx_find_duplicate(MacmbxTOC *toc, int32_t msg_id_hash) {
  if (!toc || !msg_id_hash) return -1;
  for (int i = 0; i < toc->count; i++) {
    if (!(toc->msgs[i].flags & MACMBX_FLAG_DELETED) &&
        toc->msgs[i].msg_id_hash == msg_id_hash)
      return i;
  }
  return -1;
}

int macmbx_is_duplicate(MacmbxTOC *toc, const char *message, long len) {
  if (!toc || !message) return -1;
  if (len < 0) len = (long)strlen(message);
  int32_t hash = extract_msgid_hash(message, len);
  if (!hash) return -1;
  return macmbx_find_duplicate(toc, hash);
}

int macmbx_append_unique(MacmbxTOC *toc, const char *message, long len,
                          const char *sender, uint8_t state, uint8_t priority) {
  if (!toc || !message) return -1;
  if (len < 0) len = (long)strlen(message);

  int existing = macmbx_is_duplicate(toc, message, len);
  if (existing >= 0) return -(existing + 1); /* negative = duplicate */

  return macmbx_append_message(toc, message, len, sender, state, priority);
}

/* ================================================================
 * Export
 * ================================================================ */

int macmbx_export_eml(MacmbxTOC *toc, int index, const char *eml_path) {
  if (!toc || !eml_path || index < 0 || index >= toc->count) return -1;

  long len = 0;
  char *msg = macmbx_read_message(toc, index, &len);
  if (!msg) return -1;

  /* Skip "From " line if present */
  char *body = msg;
  long bodyLen = len;
  if (len > 5 && strncmp(msg, "From ", 5) == 0) {
    char *nl = memchr(msg, '\n', len);
    if (nl) {
      body = nl + 1;
      bodyLen = len - (body - msg);
    }
  }

  FILE *f = fopen(eml_path, "wb");
  if (!f) { free(msg); return -1; }
  fwrite(body, 1, bodyLen, f);
  fclose(f);
  free(msg);
  return 0;
}

int macmbx_export_eml_multi(MacmbxTOC *toc, int *indices, int count,
                              const char *dir_path) {
  if (!toc || !indices || !dir_path) return -1;

  int exported = 0;
  for (int i = 0; i < count; i++) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%05ld.eml",
             dir_path, toc->msgs[indices[i]].serial_num);
    if (macmbx_export_eml(toc, indices[i], path) == 0) exported++;
  }
  return exported;
}

int macmbx_export_all_eml(MacmbxTOC *toc, const char *dir_path) {
  if (!toc || !dir_path) return -1;

  /* Create directory if needed */
  struct stat st;
  if (stat(dir_path, &st) != 0) {
#ifdef _WIN32
    _mkdir(dir_path);
#else
    mkdir(dir_path, 0755);
#endif
  }

  int exported = 0;
  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%05ld.eml", dir_path, toc->msgs[i].serial_num);
    if (macmbx_export_eml(toc, i, path) == 0) exported++;
  }
  return exported;
}

/* ================================================================
 * Import
 * ================================================================ */

/* Read entire file into malloc'd buffer */
static char *read_file(const char *path, long *outLen) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc(len + 1);
  if (!buf) { fclose(f); return NULL; }
  long got = (long)fread(buf, 1, len, f);
  fclose(f);
  buf[got] = '\0';
  if (outLen) *outLen = got;
  return buf;
}

int macmbx_import_eml(MacmbxTOC *toc, const char *eml_path) {
  if (!toc || !eml_path) return -1;

  long len = 0;
  char *msg = read_file(eml_path, &len);
  if (!msg) return -1;

  /* Extract sender from From: header */
  char sender[256] = "unknown";
  const char *p = msg;
  while (p < msg + len) {
    if (strncasecmp(p, "From:", 5) == 0) {
      p += 5;
      while (*p == ' ' || *p == '\t') p++;
      const char *end = p;
      while (end < msg + len && *end != '\r' && *end != '\n') end++;
      size_t slen = end - p;
      if (slen >= sizeof(sender)) slen = sizeof(sender) - 1;
      memcpy(sender, p, slen);
      sender[slen] = '\0';
      break;
    }
    while (p < msg + len && *p != '\n') p++;
    if (p < msg + len) p++;
    if (p < msg + len && (*p == '\r' || *p == '\n')) break;
  }

  int idx = macmbx_append_message(toc, msg, len, sender, MACMBX_UNREAD, 3);
  free(msg);
  return idx;
}

int macmbx_import_eml_dir(MacmbxTOC *toc, const char *dir_path) {
  if (!toc || !dir_path) return -1;
  int imported = 0;

#ifdef _WIN32
  char search[PATH_MAX];
  snprintf(search, sizeof(search), "%s\\*.eml", dir_path);
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(search, &fd);
  if (h == INVALID_HANDLE_VALUE) return 0;
  do {
    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s\\%s", dir_path, fd.cFileName);
    if (macmbx_import_eml(toc, full) >= 0) imported++;
  } while (FindNextFileA(h, &fd));
  FindClose(h);
#else
  DIR *d = opendir(dir_path);
  if (!d) return -1;
  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    size_t nlen = strlen(entry->d_name);
    if (nlen < 5) continue;
    if (strcasecmp(entry->d_name + nlen - 4, ".eml") != 0) continue;
    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", dir_path, entry->d_name);
    if (macmbx_import_eml(toc, full) >= 0) imported++;
  }
  closedir(d);
#endif
  return imported;
}

int macmbx_import_maildir(MacmbxTOC *toc, const char *maildir_path) {
  if (!toc || !maildir_path) return -1;
  int imported = 0;

  /* Maildir has cur/, new/, tmp/ subdirectories */
  const char *subdirs[] = {"new", "cur", NULL};
  for (int s = 0; subdirs[s]; s++) {
    char subpath[PATH_MAX];
    snprintf(subpath, sizeof(subpath), "%s/%s", maildir_path, subdirs[s]);

#ifdef _WIN32
    char search[PATH_MAX];
    snprintf(search, sizeof(search), "%s\\*", subpath);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search, &fd);
    if (h == INVALID_HANDLE_VALUE) continue;
    do {
      if (fd.cFileName[0] == '.') continue;
      char full[PATH_MAX];
      snprintf(full, sizeof(full), "%s\\%s", subpath, fd.cFileName);
      long len = 0;
      char *msg = read_file(full, &len);
      if (msg) {
        uint8_t state = (s == 0) ? MACMBX_UNREAD : MACMBX_READ;
        /* Check maildir flags: filename may end with :2,S for Seen */
        if (strstr(fd.cFileName, ":2,") && strchr(fd.cFileName, 'S'))
          state = MACMBX_READ;
        macmbx_append_message(toc, msg, len, NULL, state, 3);
        free(msg);
        imported++;
      }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *d = opendir(subpath);
    if (!d) continue;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_name[0] == '.') continue;
      char full[PATH_MAX];
      snprintf(full, sizeof(full), "%s/%s", subpath, entry->d_name);
      struct stat st;
      if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
      long len = 0;
      char *msg = read_file(full, &len);
      if (msg) {
        uint8_t state = (s == 0) ? MACMBX_UNREAD : MACMBX_READ;
        /* Check maildir flags */
        char *flags = strstr(entry->d_name, ":2,");
        if (flags && strchr(flags, 'S')) state = MACMBX_READ;
        macmbx_append_message(toc, msg, len, NULL, state, 3);
        free(msg);
        imported++;
      }
    }
    closedir(d);
#endif
  }
  return imported;
}

int macmbx_import_mbox(MacmbxTOC *toc, const char *mbox_path, bool dedup) {
  if (!toc || !mbox_path) return -1;

  /* Build a temporary TOC from the source mbox */
  MacmbxTOC *src = macmbx_toc_build(mbox_path);
  if (!src) return -1;

  int imported = 0;
  for (int i = 0; i < src->count; i++) {
    long len = 0;
    char *msg = macmbx_read_message(src, i, &len);
    if (!msg) continue;

    /* Skip "From " line */
    char *body = msg;
    long bodyLen = len;
    if (len > 5 && strncmp(msg, "From ", 5) == 0) {
      char *nl = memchr(msg, '\n', len);
      if (nl) { body = nl + 1; bodyLen = len - (body - msg); }
    }

    if (dedup) {
      int idx = macmbx_append_unique(toc, body, bodyLen,
                                      src->msgs[i].from,
                                      src->msgs[i].state,
                                      src->msgs[i].priority);
      if (idx >= 0) imported++;
    } else {
      int idx = macmbx_append_message(toc, body, bodyLen,
                                       src->msgs[i].from,
                                       src->msgs[i].state,
                                       src->msgs[i].priority);
      if (idx >= 0) imported++;
    }
    free(msg);
  }

  macmbx_toc_close(src);
  return imported;
}

/* ================================================================
 * Batch export — single file per mailbox (Unix mbox format)
 * ================================================================ */

int macmbx_export_mbox(MacmbxTOC *toc, const char *output_path) {
  if (!toc || !output_path) return -1;
  FILE *out = fopen(output_path, "wb");
  if (!out) return -1;

  int exported = 0;
  for (int i = 0; i < toc->count; i++) {
    long len = 0;
    char *msg = macmbx_read_message(toc, i, &len);
    if (!msg) continue;

    /* Ensure starts with From line */
    if (len > 5 && strncmp(msg, "From ", 5) != 0) {
      char from_line[256];
      macmbx_write_from_line(from_line, sizeof(from_line),
                              toc->msgs[i].from[0] ? toc->msgs[i].from : "unknown");
      fputs(from_line, out);
    }
    fwrite(msg, 1, len, out);
    /* Ensure ends with newline */
    if (len > 0 && msg[len - 1] != '\n') fputc('\n', out);
    free(msg);
    exported++;
  }

  fclose(out);
  return exported;
}

/* Mozilla flag mapping */
#define MOZ_FLAG_READ     0x0001
#define MOZ_FLAG_REPLIED  0x0002
#define MOZ_FLAG_MARKED   0x0004
#define MOZ_FLAG_EXPUNGED 0x0008
#define MOZ_FLAG_FORWARDED 0x1000

static unsigned long macmbx_state_to_mozilla(uint8_t state, uint8_t priority) {
  unsigned long flags = 0;
  switch (state) {
    case 2: flags |= MOZ_FLAG_READ; break;                     /* READ */
    case 3: flags |= MOZ_FLAG_READ | MOZ_FLAG_REPLIED; break;  /* REPLIED */
    case 8: flags |= MOZ_FLAG_READ | MOZ_FLAG_FORWARDED; break;/* FORWARDED */
    default: break;
  }
  /* Mozilla stores priority in bits 13-15 */
  if (priority > 0 && priority <= 5)
    flags |= ((unsigned long)(priority)) << 13;
  return flags;
}

int macmbx_export_mbox_mozilla(MacmbxTOC *toc, const char *output_path) {
  if (!toc || !output_path) return -1;
  FILE *out = fopen(output_path, "wb");
  if (!out) return -1;

  int exported = 0;
  for (int i = 0; i < toc->count; i++) {
    long len = 0;
    char *msg = macmbx_read_message(toc, i, &len);
    if (!msg) continue;

    MacmbxMsgSum *sum = &toc->msgs[i];

    /* From line */
    if (len < 5 || strncmp(msg, "From ", 5) != 0) {
      char from_line[256];
      macmbx_write_from_line(from_line, sizeof(from_line),
                              sum->from[0] ? sum->from : "unknown");
      fputs(from_line, out);
    }

    /* Find end of first line (From line) and insert Mozilla headers after it */
    char *nl = strchr(msg, '\n');
    if (nl) {
      fwrite(msg, 1, nl - msg + 1, out);
      /* Insert Mozilla status headers */
      unsigned long moz_flags = macmbx_state_to_mozilla(sum->state, sum->priority);
      fprintf(out, "X-Mozilla-Status: %04lx\r\n", moz_flags & 0xFFFF);
      fprintf(out, "X-Mozilla-Status2: %08lx\r\n", 0UL);
      /* Write rest of message */
      fwrite(nl + 1, 1, len - (nl - msg + 1), out);
    } else {
      fwrite(msg, 1, len, out);
    }

    if (len > 0 && msg[len - 1] != '\n') fputc('\n', out);
    free(msg);
    exported++;
  }

  fclose(out);
  return exported;
}

int macmbx_export_store(MacmbxStore *store, const char *output_dir) {
  if (!store || !output_dir) return -1;

  /* Create output directory */
  struct stat st;
  if (stat(output_dir, &st) != 0)
    mkdir(output_dir, 0755);

  MacmbxNode *root = macmbx_store_root(store);
  int exported = 0;

  for (MacmbxNode *n = root; n; n = n->next) {
    if (n->type == MACMBX_NODE_MAILBOX) {
      MacmbxTOC *toc = macmbx_toc_open(n->path);
      if (toc && toc->count > 0) {
        char out_path[PATH_MAX];
        snprintf(out_path, sizeof(out_path), "%s/%s.mbox", output_dir, n->name);
        macmbx_export_mbox(toc, out_path);
        exported++;
      }
    }
    /* TODO: recurse into folders */
  }

  return exported;
}

/* ================================================================
 * Signature import
 * ================================================================ */

int macmbx_sig_import_dir(const char *sig_dir, const char *import_from) {
  if (!sig_dir || !import_from) return -1;

  DIR *d = opendir(import_from);
  if (!d) return -1;

  /* Create sig_dir if needed */
  struct stat st;
  if (stat(sig_dir, &st) != 0)
    mkdir(sig_dir, 0755);

  int imported = 0;
  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.') continue;

    char src_path[PATH_MAX], dst_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s/%s", import_from, entry->d_name);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", sig_dir, entry->d_name);

    /* Only import regular files */
    struct stat fst;
    if (stat(src_path, &fst) != 0 || !S_ISREG(fst.st_mode)) continue;

    /* Read source */
    FILE *in = fopen(src_path, "rb");
    if (!in) continue;
    char *buf = (char *)malloc(fst.st_size + 1);
    if (!buf) { fclose(in); continue; }
    size_t nread = fread(buf, 1, fst.st_size, in);
    fclose(in);
    buf[nread] = '\0';

    /* Write to sig_dir */
    FILE *out = fopen(dst_path, "wb");
    if (out) {
      fwrite(buf, 1, nread, out);
      fclose(out);
      imported++;
    }
    free(buf);
  }

  closedir(d);
  return imported;
}
