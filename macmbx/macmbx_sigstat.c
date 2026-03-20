/* macmbx_sigstat.c — Signatures and Stationery management
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Signatures: plain text files in Signatures/ directory.
 * Stationery: RFC822 message templates in Stationery/ directory.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  #define mkdir_p(p) _mkdir(p)
#else
  #include <unistd.h>
  #include <dirent.h>
  #define mkdir_p(p) mkdir(p, 0755)
#endif

/* ================================================================
 * Shared helpers
 * ================================================================ */

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

static int write_file(const char *path, const char *data, long len) {
  if (len < 0) len = (long)strlen(data);
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  FILE *f = fopen(tmp, "wb");
  if (!f) return -1;
  if ((long)fwrite(data, 1, len, f) != len) { fclose(f); remove(tmp); return -1; }
  fclose(f);
  if (rename(tmp, path) != 0) { remove(tmp); return -1; }
  return 0;
}

static bool is_dir(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* ================================================================
 * Signatures
 * ================================================================ */

MacmbxSignatures *macmbx_sig_open(const char *sig_dir) {
  if (!sig_dir) return NULL;
  if (!is_dir(sig_dir)) {
    if (mkdir_p(sig_dir) != 0) return NULL;
  }

  MacmbxSignatures *sigs = (MacmbxSignatures *)calloc(1, sizeof(MacmbxSignatures));
  if (!sigs) return NULL;
  snprintf(sigs->dir_path, sizeof(sigs->dir_path), "%s", sig_dir);
  sigs->capacity = 16;
  sigs->sigs = (MacmbxSignature *)calloc(sigs->capacity, sizeof(MacmbxSignature));

  /* Ensure Standard and Alternate exist */
  char std_path[PATH_MAX], alt_path[PATH_MAX];
  snprintf(std_path, sizeof(std_path), "%s/Standard", sig_dir);
  snprintf(alt_path, sizeof(alt_path), "%s/Alternate", sig_dir);
  struct stat st;
  if (stat(std_path, &st) != 0) { FILE *f = fopen(std_path, "w"); if (f) fclose(f); }
  if (stat(alt_path, &st) != 0) { FILE *f = fopen(alt_path, "w"); if (f) fclose(f); }

  /* Scan directory — Standard first, Alternate second, then rest alphabetically */
#ifndef _WIN32
  /* Add Standard */
  snprintf(sigs->sigs[0].name, sizeof(sigs->sigs[0].name), "Standard");
  snprintf(sigs->sigs[0].path, sizeof(sigs->sigs[0].path), "%s", std_path);
  sigs->count = 1;

  /* Add Alternate */
  snprintf(sigs->sigs[1].name, sizeof(sigs->sigs[1].name), "Alternate");
  snprintf(sigs->sigs[1].path, sizeof(sigs->sigs[1].path), "%s", alt_path);
  sigs->count = 2;

  /* Add remaining signature files */
  DIR *d = opendir(sig_dir);
  if (d) {
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_name[0] == '.') continue;
      if (strcasecmp(entry->d_name, "Standard") == 0) continue;
      if (strcasecmp(entry->d_name, "Alternate") == 0) continue;
      size_t nlen = strlen(entry->d_name);
      if (nlen > 4 && strcmp(entry->d_name + nlen - 4, ".tmp") == 0) continue;

      char full[PATH_MAX];
      snprintf(full, sizeof(full), "%s/%s", sig_dir, entry->d_name);
      if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;

      if (sigs->count >= sigs->capacity) {
        sigs->capacity *= 2;
        sigs->sigs = (MacmbxSignature *)realloc(sigs->sigs,
          sigs->capacity * sizeof(MacmbxSignature));
      }
      MacmbxSignature *sig = &sigs->sigs[sigs->count];
      memset(sig, 0, sizeof(*sig));
      snprintf(sig->name, sizeof(sig->name), "%s", entry->d_name);
      snprintf(sig->path, sizeof(sig->path), "%s", full);
      sigs->count++;
    }
    closedir(d);
  }
#endif
  return sigs;
}

void macmbx_sig_close(MacmbxSignatures *sigs) {
  if (!sigs) return;
  for (int i = 0; i < sigs->count; i++) free(sigs->sigs[i].content);
  free(sigs->sigs);
  free(sigs);
}

int macmbx_sig_save(MacmbxSignatures *sigs) {
  if (!sigs) return -1;
  int err = 0;
  for (int i = 0; i < sigs->count; i++) {
    if (sigs->sigs[i].dirty && sigs->sigs[i].content) {
      if (write_file(sigs->sigs[i].path, sigs->sigs[i].content,
                     (long)strlen(sigs->sigs[i].content)) != 0)
        err = -1;
      else
        sigs->sigs[i].dirty = false;
    }
  }
  return err;
}

int macmbx_sig_add(MacmbxSignatures *sigs, const char *name, const char *content) {
  if (!sigs || !name) return -1;
  if (sigs->count >= sigs->capacity) {
    sigs->capacity *= 2;
    sigs->sigs = (MacmbxSignature *)realloc(sigs->sigs,
      sigs->capacity * sizeof(MacmbxSignature));
  }
  int idx = sigs->count++;
  MacmbxSignature *sig = &sigs->sigs[idx];
  memset(sig, 0, sizeof(*sig));
  snprintf(sig->name, sizeof(sig->name), "%s", name);
  snprintf(sig->path, sizeof(sig->path), "%s/%s", sigs->dir_path, name);
  sig->content = content ? strdup(content) : strdup("");
  sig->dirty = true;
  return idx;
}

int macmbx_sig_remove(MacmbxSignatures *sigs, int index) {
  if (!sigs || index < 0 || index >= sigs->count) return -1;
  remove(sigs->sigs[index].path);
  free(sigs->sigs[index].content);
  memmove(&sigs->sigs[index], &sigs->sigs[index + 1],
          (sigs->count - index - 1) * sizeof(MacmbxSignature));
  sigs->count--;
  return 0;
}

int macmbx_sig_rename(MacmbxSignatures *sigs, int index, const char *new_name) {
  if (!sigs || index < 0 || index >= sigs->count || !new_name) return -1;
  char new_path[PATH_MAX];
  snprintf(new_path, sizeof(new_path), "%s/%s", sigs->dir_path, new_name);
  if (rename(sigs->sigs[index].path, new_path) != 0) return -1;
  snprintf(sigs->sigs[index].name, sizeof(sigs->sigs[index].name), "%s", new_name);
  snprintf(sigs->sigs[index].path, sizeof(sigs->sigs[index].path), "%s", new_path);
  return 0;
}

const char *macmbx_sig_get(MacmbxSignatures *sigs, int index) {
  if (!sigs || index < 0 || index >= sigs->count) return NULL;
  if (!sigs->sigs[index].content) {
    sigs->sigs[index].content = read_file(sigs->sigs[index].path, NULL);
  }
  return sigs->sigs[index].content;
}

int macmbx_sig_set(MacmbxSignatures *sigs, int index, const char *content) {
  if (!sigs || index < 0 || index >= sigs->count) return -1;
  free(sigs->sigs[index].content);
  sigs->sigs[index].content = content ? strdup(content) : strdup("");
  sigs->sigs[index].dirty = true;
  return 0;
}

int macmbx_sig_find(MacmbxSignatures *sigs, const char *name) {
  if (!sigs || !name) return -1;
  for (int i = 0; i < sigs->count; i++) {
    if (strcasecmp(sigs->sigs[i].name, name) == 0) return i;
  }
  return -1;
}

int macmbx_sig_count(MacmbxSignatures *sigs) {
  return sigs ? sigs->count : 0;
}

const char *macmbx_sig_standard(MacmbxSignatures *sigs) {
  return macmbx_sig_get(sigs, 0);
}

const char *macmbx_sig_alternate(MacmbxSignatures *sigs) {
  return macmbx_sig_get(sigs, 1);
}

/* ================================================================
 * Stationery
 * ================================================================ */

MacmbxStationerySet *macmbx_stat_open(const char *stat_dir) {
  if (!stat_dir) return NULL;
  if (!is_dir(stat_dir)) {
    if (mkdir_p(stat_dir) != 0) return NULL;
  }

  MacmbxStationerySet *ss = (MacmbxStationerySet *)calloc(1, sizeof(MacmbxStationerySet));
  if (!ss) return NULL;
  snprintf(ss->dir_path, sizeof(ss->dir_path), "%s", stat_dir);
  ss->capacity = 16;
  ss->items = (MacmbxStationery *)calloc(ss->capacity, sizeof(MacmbxStationery));

#ifndef _WIN32
  DIR *d = opendir(stat_dir);
  if (d) {
    struct stat st;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_name[0] == '.') continue;
      size_t nlen = strlen(entry->d_name);
      if (nlen > 4 && strcmp(entry->d_name + nlen - 4, ".tmp") == 0) continue;

      char full[PATH_MAX];
      snprintf(full, sizeof(full), "%s/%s", stat_dir, entry->d_name);
      if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;

      if (ss->count >= ss->capacity) {
        ss->capacity *= 2;
        ss->items = (MacmbxStationery *)realloc(ss->items,
          ss->capacity * sizeof(MacmbxStationery));
      }
      MacmbxStationery *item = &ss->items[ss->count];
      memset(item, 0, sizeof(*item));
      snprintf(item->name, sizeof(item->name), "%s", entry->d_name);
      snprintf(item->path, sizeof(item->path), "%s", full);
      ss->count++;
    }
    closedir(d);
  }
#endif
  return ss;
}

void macmbx_stat_close(MacmbxStationerySet *ss) {
  if (!ss) return;
  for (int i = 0; i < ss->count; i++) free(ss->items[i].content);
  free(ss->items);
  free(ss);
}

int macmbx_stat_save(MacmbxStationerySet *ss) {
  if (!ss) return -1;
  int err = 0;
  for (int i = 0; i < ss->count; i++) {
    if (ss->items[i].dirty && ss->items[i].content) {
      if (write_file(ss->items[i].path, ss->items[i].content,
                     ss->items[i].content_len) != 0)
        err = -1;
      else
        ss->items[i].dirty = false;
    }
  }
  return err;
}

int macmbx_stat_add(MacmbxStationerySet *ss, const char *name,
                     const char *message, long len) {
  if (!ss || !name) return -1;
  if (len < 0 && message) len = (long)strlen(message);
  if (ss->count >= ss->capacity) {
    ss->capacity *= 2;
    ss->items = (MacmbxStationery *)realloc(ss->items,
      ss->capacity * sizeof(MacmbxStationery));
  }
  int idx = ss->count++;
  MacmbxStationery *item = &ss->items[idx];
  memset(item, 0, sizeof(*item));
  snprintf(item->name, sizeof(item->name), "%s", name);
  snprintf(item->path, sizeof(item->path), "%s/%s", ss->dir_path, name);
  if (message && len > 0) {
    item->content = (char *)malloc(len + 1);
    memcpy(item->content, message, len);
    item->content[len] = '\0';
    item->content_len = len;
  }
  item->dirty = true;
  return idx;
}

int macmbx_stat_remove(MacmbxStationerySet *ss, int index) {
  if (!ss || index < 0 || index >= ss->count) return -1;
  remove(ss->items[index].path);
  free(ss->items[index].content);
  memmove(&ss->items[index], &ss->items[index + 1],
          (ss->count - index - 1) * sizeof(MacmbxStationery));
  ss->count--;
  return 0;
}

int macmbx_stat_rename(MacmbxStationerySet *ss, int index, const char *new_name) {
  if (!ss || index < 0 || index >= ss->count || !new_name) return -1;
  char new_path[PATH_MAX];
  snprintf(new_path, sizeof(new_path), "%s/%s", ss->dir_path, new_name);
  if (rename(ss->items[index].path, new_path) != 0) return -1;
  snprintf(ss->items[index].name, sizeof(ss->items[index].name), "%s", new_name);
  snprintf(ss->items[index].path, sizeof(ss->items[index].path), "%s", new_path);
  return 0;
}

const char *macmbx_stat_get(MacmbxStationerySet *ss, int index, long *outLen) {
  if (!ss || index < 0 || index >= ss->count) return NULL;
  if (!ss->items[index].content) {
    ss->items[index].content = read_file(ss->items[index].path,
                                          &ss->items[index].content_len);
  }
  if (outLen) *outLen = ss->items[index].content_len;
  return ss->items[index].content;
}

int macmbx_stat_set(MacmbxStationerySet *ss, int index,
                     const char *message, long len) {
  if (!ss || index < 0 || index >= ss->count) return -1;
  if (len < 0 && message) len = (long)strlen(message);
  free(ss->items[index].content);
  if (message && len > 0) {
    ss->items[index].content = (char *)malloc(len + 1);
    memcpy(ss->items[index].content, message, len);
    ss->items[index].content[len] = '\0';
    ss->items[index].content_len = len;
  } else {
    ss->items[index].content = NULL;
    ss->items[index].content_len = 0;
  }
  ss->items[index].dirty = true;
  return 0;
}

int macmbx_stat_find(MacmbxStationerySet *ss, const char *name) {
  if (!ss || !name) return -1;
  for (int i = 0; i < ss->count; i++) {
    if (strcasecmp(ss->items[i].name, name) == 0) return i;
  }
  return -1;
}

int macmbx_stat_count(MacmbxStationerySet *ss) {
  return ss ? ss->count : 0;
}

char *macmbx_stat_new_message(MacmbxStationerySet *ss, int index, long *outLen) {
  long len = 0;
  const char *tmpl = macmbx_stat_get(ss, index, &len);
  if (!tmpl) return NULL;
  char *msg = (char *)malloc(len + 1);
  if (!msg) return NULL;
  memcpy(msg, tmpl, len);
  msg[len] = '\0';
  if (outLen) *outLen = len;
  return msg;
}

int macmbx_stat_save_from_message(MacmbxStationerySet *ss, const char *name,
                                    const char *message, long len) {
  if (!ss || !name || !message) return -1;
  int idx = macmbx_stat_find(ss, name);
  if (idx >= 0) {
    return macmbx_stat_set(ss, idx, message, len);
  }
  return macmbx_stat_add(ss, name, message, len);
}
