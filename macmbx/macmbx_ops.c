/* macmbx_ops.c — Transfer, create, remove, rename, search, sort
 * Part of macmbx: standalone Eudora mbox storage library.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <strings.h>

#ifdef _WIN32
  #include <io.h>
  #include <direct.h>
  #define mkdir(p, m) _mkdir(p)
#else
  #include <unistd.h>
  #include <dirent.h>
#endif

/* ================================================================
 * Transfer
 * ================================================================ */

int macmbx_transfer(MacmbxTOC *src, int index, MacmbxTOC *dst, bool copy) {
  if (!src || !dst || index < 0 || index >= src->count) return -1;

  /* Read message from source */
  long msgLen = 0;
  char *msg = macmbx_read_message(src, index, &msgLen);
  if (!msg) return -1;

  /* Skip the "From " line in the read data — append will write its own */
  char *body = msg;
  long bodyLen = msgLen;
  if (strncmp(msg, "From ", 5) == 0) {
    char *nl = strchr(msg, '\n');
    if (nl) {
      body = nl + 1;
      bodyLen = msgLen - (body - msg);
    }
  }

  /* Append to destination */
  MacmbxMsgSum *srcSum = &src->msgs[index];
  int newIdx = macmbx_append_message(dst, body, bodyLen,
                                      srcSum->from,
                                      srcSum->state,
                                      srcSum->priority);
  free(msg);
  if (newIdx < 0) return -1;

  /* Copy metadata */
  dst->msgs[newIdx].flags = srcSum->flags & ~MACMBX_FLAG_DELETED;
  dst->msgs[newIdx].seconds = srcSum->seconds;
  dst->msgs[newIdx].orig_zone = srcSum->orig_zone;
  dst->msgs[newIdx].arrival = srcSum->arrival;
  dst->msgs[newIdx].msg_id_hash = srcSum->msg_id_hash;
  dst->msgs[newIdx].uid_hash = srcSum->uid_hash;
  dst->msgs[newIdx].spam_score = srcSum->spam_score;
  snprintf(dst->msgs[newIdx].from, sizeof(dst->msgs[newIdx].from), "%s", srcSum->from);
  snprintf(dst->msgs[newIdx].subject, sizeof(dst->msgs[newIdx].subject), "%s", srcSum->subject);
  dst->dirty = true;

  /* Mark source as deleted if move */
  if (!copy) {
    macmbx_delete_message(src, index);
  }

  return newIdx;
}

int macmbx_transfer_multi(MacmbxTOC *src, int *indices, int count,
                           MacmbxTOC *dst, bool copy) {
  if (!src || !dst || !indices || count <= 0) return -1;

  int transferred = 0;
  for (int i = 0; i < count; i++) {
    int res = macmbx_transfer(src, indices[i], dst, copy);
    if (res >= 0) transferred++;
  }
  return transferred;
}

/* ================================================================
 * Mailbox file operations
 * ================================================================ */

int macmbx_create(const char *mbox_path) {
  if (!mbox_path) return -1;

  /* Create empty mbox file */
  FILE *f = fopen(mbox_path, "wb");
  if (!f) return -1;
  fclose(f);

  /* Create empty TOC */
  MacmbxTOC toc;
  memset(&toc, 0, sizeof(toc));
  snprintf(toc.mbox_path, sizeof(toc.mbox_path), "%s", mbox_path);
  char toc_path[PATH_MAX];
  snprintf(toc_path, sizeof(toc_path), "%s.toc", mbox_path);
  snprintf(toc.toc_path, sizeof(toc.toc_path), "%s", toc_path);
  toc.next_serial = 1;
  toc.dirty = true;
  toc.msgs = NULL;
  macmbx_toc_save(&toc);

  return 0;
}

int macmbx_remove(const char *mbox_path) {
  if (!mbox_path) return -1;
  char toc_path[PATH_MAX];
  snprintf(toc_path, sizeof(toc_path), "%s.toc", mbox_path);
  remove(toc_path);
  return remove(mbox_path);
}

int macmbx_rename(const char *old_path, const char *new_path) {
  if (!old_path || !new_path) return -1;

  /* Rename mbox */
  if (rename(old_path, new_path) != 0) return -1;

  /* Rename .toc */
  char old_toc[PATH_MAX], new_toc[PATH_MAX];
  snprintf(old_toc, sizeof(old_toc), "%s.toc", old_path);
  snprintf(new_toc, sizeof(new_toc), "%s.toc", new_path);
  rename(old_toc, new_toc); /* OK if this fails — TOC can be rebuilt */

  return 0;
}

bool macmbx_is_mbox(const char *path) {
  if (!path) return false;
  struct stat st;
  if (stat(path, &st) != 0) return false;
  if (!S_ISREG(st.st_mode)) return false;
  if (st.st_size == 0) return true; /* empty mbox is valid */

  /* Check first line for "From " */
  FILE *f = fopen(path, "rb");
  if (!f) return false;
  char line[256];
  bool result = false;
  if (fgets(line, sizeof(line), f)) {
    result = macmbx_is_from_line(line);
  }
  fclose(f);
  return result;
}

int macmbx_list_mailboxes(const char *dir, char ***names) {
  if (!dir || !names) return 0;
  *names = NULL;

#ifdef _WIN32
  /* Windows: use FindFirstFile/FindNextFile */
  return 0; /* TODO: implement for Windows */
#else
  DIR *d = opendir(dir);
  if (!d) return 0;

  int count = 0, cap = 32;
  *names = (char **)calloc(cap, sizeof(char *));

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.') continue;

    /* Skip .toc files */
    size_t nlen = strlen(entry->d_name);
    if (nlen > 4 && strcmp(entry->d_name + nlen - 4, ".toc") == 0) continue;

    /* Check if it's a regular file */
    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
    struct stat st;
    if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;

    /* Add to list */
    if (count >= cap) {
      cap *= 2;
      *names = (char **)realloc(*names, cap * sizeof(char *));
    }
    (*names)[count++] = strdup(entry->d_name);
  }
  closedir(d);
  return count;
#endif
}

/* ================================================================
 * Search
 * ================================================================ */

/* Case-insensitive strstr */
static const char *ci_strstr(const char *hay, const char *needle) {
  if (!hay || !needle) return NULL;
  size_t nlen = strlen(needle);
  if (nlen == 0) return hay;
  size_t hlen = strlen(hay);
  if (hlen < nlen) return NULL;
  for (size_t i = 0; i <= hlen - nlen; i++) {
    bool match = true;
    for (size_t j = 0; j < nlen; j++) {
      if (tolower((unsigned char)hay[i+j]) != tolower((unsigned char)needle[j])) {
        match = false;
        break;
      }
    }
    if (match) return &hay[i];
  }
  return NULL;
}

int macmbx_search(MacmbxTOC *toc, const char *field, const char *pattern,
                   int **results) {
  if (!toc || !pattern || !results) return 0;
  *results = NULL;

  int count = 0, cap = 64;
  *results = (int *)calloc(cap, sizeof(int));

  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) continue;

    bool match = false;
    MacmbxMsgSum *msg = &toc->msgs[i];

    if (strcasecmp(field, "from") == 0) {
      match = ci_strstr(msg->from, pattern) != NULL;
    } else if (strcasecmp(field, "subject") == 0) {
      match = ci_strstr(msg->subject, pattern) != NULL;
    } else if (strcasecmp(field, "all") == 0) {
      /* Search from + subject in summary (fast) */
      match = ci_strstr(msg->from, pattern) != NULL ||
              ci_strstr(msg->subject, pattern) != NULL;
      /* If not found in summary, search full message (slow) */
      if (!match) {
        long len = 0;
        char *full = macmbx_read_message(toc, i, &len);
        if (full) {
          for (long j = 0; j <= len - (long)strlen(pattern); j++) {
            bool m = true;
            for (size_t k = 0; k < strlen(pattern); k++) {
              if (tolower((unsigned char)full[j+k]) != tolower((unsigned char)pattern[k])) {
                m = false; break;
              }
            }
            if (m) { match = true; break; }
          }
          free(full);
        }
      }
    } else if (strcasecmp(field, "body") == 0) {
      long len = 0;
      char *body = macmbx_read_body(toc, i, &len);
      if (body) {
        for (long j = 0; j <= len - (long)strlen(pattern); j++) {
          bool m = true;
          for (size_t k = 0; k < strlen(pattern); k++) {
            if (tolower((unsigned char)body[j+k]) != tolower((unsigned char)pattern[k])) {
              m = false; break;
            }
          }
          if (m) { match = true; break; }
        }
        free(body);
      }
    } else if (strncasecmp(field, "header:", 7) == 0) {
      const char *hdr_name = field + 7;
      char *val = macmbx_read_header_field(toc, i, hdr_name);
      if (val) {
        match = ci_strstr(val, pattern) != NULL;
        free(val);
      }
    }

    if (match) {
      if (count >= cap) {
        cap *= 2;
        *results = (int *)realloc(*results, cap * sizeof(int));
      }
      (*results)[count++] = i;
    }
  }
  return count;
}

/* ================================================================
 * Sort
 * ================================================================ */

static const char *sort_field;
static bool sort_ascending;

static int sort_cmp(const void *a, const void *b) {
  const MacmbxMsgSum *sa = (const MacmbxMsgSum *)a;
  const MacmbxMsgSum *sb = (const MacmbxMsgSum *)b;
  int result = 0;

  if (strcasecmp(sort_field, "date") == 0) {
    if (sa->seconds < sb->seconds) result = -1;
    else if (sa->seconds > sb->seconds) result = 1;
  } else if (strcasecmp(sort_field, "from") == 0) {
    result = strcasecmp(sa->from, sb->from);
  } else if (strcasecmp(sort_field, "subject") == 0) {
    /* Skip Re:/Fwd: for sorting */
    const char *a_subj = sa->subject;
    const char *b_subj = sb->subject;
    while ((*a_subj == 'R' || *a_subj == 'r') &&
           (a_subj[1] == 'E' || a_subj[1] == 'e') && a_subj[2] == ':') {
      a_subj += 3;
      while (*a_subj == ' ') a_subj++;
    }
    while ((*b_subj == 'R' || *b_subj == 'r') &&
           (b_subj[1] == 'E' || b_subj[1] == 'e') && b_subj[2] == ':') {
      b_subj += 3;
      while (*b_subj == ' ') b_subj++;
    }
    result = strcasecmp(a_subj, b_subj);
  } else if (strcasecmp(sort_field, "size") == 0) {
    if (sa->length < sb->length) result = -1;
    else if (sa->length > sb->length) result = 1;
  } else if (strcasecmp(sort_field, "state") == 0) {
    result = (int)sa->state - (int)sb->state;
  } else if (strcasecmp(sort_field, "priority") == 0) {
    result = (int)sa->priority - (int)sb->priority;
  } else if (strcasecmp(sort_field, "label") == 0) {
    int la = (sa->flags & MACMBX_FLAG_LABEL_MASK) >> MACMBX_FLAG_LABEL_SHIFT;
    int lb = (sb->flags & MACMBX_FLAG_LABEL_MASK) >> MACMBX_FLAG_LABEL_SHIFT;
    result = la - lb;
  }

  return sort_ascending ? result : -result;
}

int macmbx_sort(MacmbxTOC *toc, const char *field, bool ascending) {
  if (!toc || !field || toc->count <= 1) return 0;
  sort_field = field;
  sort_ascending = ascending;
  qsort(toc->msgs, toc->count, sizeof(MacmbxMsgSum), sort_cmp);
  toc->dirty = true;
  return 0;
}
