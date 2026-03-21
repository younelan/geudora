/* macmbx_store.c — Mailbox directory manager
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Manages a hierarchy of mailboxes and folders rooted at a base path.
 * Portable: POSIX + Windows.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
  #include <io.h>
  #include <direct.h>
  #include <windows.h>
  #define mkdir_p(p) _mkdir(p)
  #define DIRSEP '\\'
#else
  #include <unistd.h>
  #include <dirent.h>
  #include <fcntl.h>
  #include <sys/file.h>
  #define mkdir_p(p) mkdir(p, 0755)
  #define DIRSEP '/'
#endif

/* Event emit functions (defined in macmbx.c) */
extern void macmbx_emit_path(MacmbxEventType type, const char *path);
extern void macmbx_emit_rename(MacmbxEventType type, const char *old_path,
                                const char *new_path);

/* ================================================================
 * Helpers
 * ================================================================ */

static bool is_dir(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_file(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool is_toc_file(const char *name) {
  size_t len = strlen(name);
  return len > 4 && strcmp(name + len - 4, ".toc") == 0;
}

static bool is_lock_file(const char *name) {
  size_t len = strlen(name);
  return len > 5 && strcmp(name + len - 5, ".lock") == 0;
}

static bool is_temp_file(const char *name) {
  size_t len = strlen(name);
  return len > 4 && strcmp(name + len - 4, ".tmp") == 0;
}

/* Build relative path from base */
static void rel_path(const char *base, const char *full, char *rel, size_t sz) {
  size_t blen = strlen(base);
  if (strncmp(full, base, blen) == 0) {
    const char *r = full + blen;
    if (*r == '/' || *r == '\\') r++;
    snprintf(rel, sz, "%s", r);
  } else {
    snprintf(rel, sz, "%s", full);
  }
}

/* ================================================================
 * Node management
 * ================================================================ */

static MacmbxNode *node_new(const char *name, const char *path,
                             MacmbxNodeType type) {
  MacmbxNode *n = (MacmbxNode *)calloc(1, sizeof(MacmbxNode));
  if (!n) return NULL;
  snprintf(n->name, sizeof(n->name), "%s", name);
  snprintf(n->path, sizeof(n->path), "%s", path);
  n->type = type;
  n->unread = -1;
  n->total = -1;
  if (type == MACMBX_NODE_MAILBOX)
    n->mbox_type = macmbx_detect_type(path);
  return n;
}

void macmbx_node_free(MacmbxNode *node) {
  if (!node) return;
  macmbx_node_free(node->children);
  macmbx_node_free(node->next);
  free(node);
}

/* Add a child node to a parent, sorted alphabetically.
 * Special mailboxes (In/Out/Trash/Junk) sort first. */
static void node_add_child(MacmbxNode *parent, MacmbxNode *child) {
  child->parent = parent;
  child->next = NULL;

  if (!parent->children) {
    parent->children = child;
    return;
  }

  /* Special mailboxes always go first */
  int child_special = (child->type == MACMBX_NODE_MAILBOX && child->mbox_type != MACMBX_TYPE_NORMAL) ? 1 : 0;

  MacmbxNode **pp = &parent->children;
  while (*pp) {
    MacmbxNode *cur = *pp;
    int cur_special = (cur->type == MACMBX_NODE_MAILBOX && cur->mbox_type != MACMBX_TYPE_NORMAL) ? 1 : 0;

    if (child_special && !cur_special) break; /* insert before non-special */
    if (!child_special && cur_special) { pp = &cur->next; continue; }

    /* Folders before mailboxes within same priority */
    if (child->type == MACMBX_NODE_FOLDER && cur->type == MACMBX_NODE_MAILBOX && !cur_special) break;
    if (child->type == MACMBX_NODE_MAILBOX && cur->type == MACMBX_NODE_FOLDER) { pp = &cur->next; continue; }

    /* Alphabetical within same type */
    if (strcasecmp(child->name, cur->name) < 0) break;
    pp = &cur->next;
  }
  child->next = *pp;
  *pp = child;
}

/* ================================================================
 * Directory scanning — build tree from filesystem
 * ================================================================ */

static MacmbxNode *scan_directory(const char *dir_path, const char *dir_name);

#ifndef _WIN32
static MacmbxNode *scan_directory(const char *dir_path, const char *dir_name) {
  DIR *d = opendir(dir_path);
  if (!d) return NULL;

  MacmbxNode *folder = node_new(dir_name, dir_path, MACMBX_NODE_FOLDER);
  if (!folder) { closedir(d); return NULL; }

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (entry->d_name[0] == '.') continue;
    if (is_toc_file(entry->d_name)) continue;
    if (is_lock_file(entry->d_name)) continue;
    if (is_temp_file(entry->d_name)) continue;

    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", dir_path, entry->d_name);

    if (is_dir(full)) {
      /* Subfolder */
      MacmbxNode *sub = scan_directory(full, entry->d_name);
      if (sub) node_add_child(folder, sub);
    } else if (is_file(full)) {
      /* Mailbox file */
      MacmbxNode *mbox = node_new(entry->d_name, full, MACMBX_NODE_MAILBOX);
      if (mbox) node_add_child(folder, mbox);
    }
  }
  closedir(d);
  return folder;
}
#else
static MacmbxNode *scan_directory(const char *dir_path, const char *dir_name) {
  char search[PATH_MAX];
  snprintf(search, sizeof(search), "%s\\*", dir_path);
  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(search, &fd);
  if (h == INVALID_HANDLE_VALUE) return NULL;

  MacmbxNode *folder = node_new(dir_name, dir_path, MACMBX_NODE_FOLDER);
  if (!folder) { FindClose(h); return NULL; }

  do {
    if (fd.cFileName[0] == '.') continue;
    if (is_toc_file(fd.cFileName)) continue;
    if (is_lock_file(fd.cFileName)) continue;
    if (is_temp_file(fd.cFileName)) continue;

    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s\\%s", dir_path, fd.cFileName);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      MacmbxNode *sub = scan_directory(full, fd.cFileName);
      if (sub) node_add_child(folder, sub);
    } else {
      MacmbxNode *mbox = node_new(fd.cFileName, full, MACMBX_NODE_MAILBOX);
      if (mbox) node_add_child(folder, mbox);
    }
  } while (FindNextFileA(h, &fd));
  FindClose(h);
  return folder;
}
#endif

/* ================================================================
 * Store lifecycle
 * ================================================================ */

MacmbxStore *macmbx_store_open(const char *root_path) {
  if (!root_path) return NULL;

  /* Create directory if needed */
  if (!is_dir(root_path)) {
    if (mkdir_p(root_path) != 0 && errno != EEXIST) return NULL;
  }

  MacmbxStore *store = (MacmbxStore *)calloc(1, sizeof(MacmbxStore));
  if (!store) return NULL;

  snprintf(store->root_path, sizeof(store->root_path), "%s", root_path);
  snprintf(store->base_path, sizeof(store->base_path), "%s", root_path);
  store->lock_fd = -1;

  /* Scan directory tree */
  store->root = scan_directory(root_path, "");
  return store;
}

void macmbx_store_close(MacmbxStore *store) {
  if (!store) return;
  macmbx_store_flush(store);
  macmbx_store_unlock(store);
  macmbx_node_free(store->root);
  free(store);
}

int macmbx_store_refresh(MacmbxStore *store) {
  if (!store) return -1;
  macmbx_node_free(store->root);
  store->root = scan_directory(store->root_path, "");
  return store->root ? 0 : -1;
}

/* ================================================================
 * Tree navigation
 * ================================================================ */

MacmbxNode *macmbx_store_root(MacmbxStore *store) {
  if (!store || !store->root) return NULL;
  return store->root->children;
}

static MacmbxNode *find_in_tree(MacmbxNode *node, const char *base,
                                 const char *target) {
  if (!node || !target) return NULL;

  for (MacmbxNode *n = node; n; n = n->next) {
    char nodeRel[PATH_MAX];
    rel_path(base, n->path, nodeRel, sizeof(nodeRel));
    if (strcmp(nodeRel, target) == 0) return n;
    if (n->children) {
      MacmbxNode *found = find_in_tree(n->children, base, target);
      if (found) return found;
    }
  }
  return NULL;
}

MacmbxNode *macmbx_store_find(MacmbxStore *store, const char *rel_path) {
  if (!store || !store->root || !rel_path) return NULL;
  return find_in_tree(store->root->children, store->base_path, rel_path);
}

static MacmbxNode *find_by_name(MacmbxNode *node, const char *name) {
  for (MacmbxNode *n = node; n; n = n->next) {
    if (strcasecmp(n->name, name) == 0) return n;
    if (n->children) {
      MacmbxNode *found = find_by_name(n->children, name);
      if (found) return found;
    }
  }
  return NULL;
}

MacmbxNode *macmbx_store_find_by_name(MacmbxStore *store, const char *name) {
  if (!store || !store->root || !name) return NULL;
  return find_by_name(store->root->children, name);
}

static MacmbxNode *find_special(MacmbxNode *node, MacmbxType type) {
  for (MacmbxNode *n = node; n; n = n->next) {
    if (n->type == MACMBX_NODE_MAILBOX && n->mbox_type == type) return n;
    if (n->children) {
      MacmbxNode *found = find_special(n->children, type);
      if (found) return found;
    }
  }
  return NULL;
}

MacmbxNode *macmbx_store_find_special(MacmbxStore *store, MacmbxType type) {
  if (!store || !store->root) return NULL;
  return find_special(store->root->children, type);
}

static int count_nodes(MacmbxNode *node, MacmbxNodeType type) {
  int n = 0;
  for (MacmbxNode *c = node; c; c = c->next) {
    if (c->type == type) n++;
    if (c->children) n += count_nodes(c->children, type);
  }
  return n;
}

int macmbx_store_count_mailboxes(MacmbxStore *store) {
  if (!store || !store->root) return 0;
  return count_nodes(store->root->children, MACMBX_NODE_MAILBOX);
}

int macmbx_store_count_folders(MacmbxStore *store) {
  if (!store || !store->root) return 0;
  return count_nodes(store->root->children, MACMBX_NODE_FOLDER);
}

/* ================================================================
 * Mailbox/folder creation
 * ================================================================ */

static void resolve_parent(MacmbxStore *store, const char *parent_path,
                            char *full_parent, size_t sz) {
  if (parent_path && parent_path[0])
    snprintf(full_parent, sz, "%s/%s", store->base_path, parent_path);
  else
    snprintf(full_parent, sz, "%s", store->base_path);
}

MacmbxNode *macmbx_store_create_mailbox(MacmbxStore *store,
                                         const char *parent_path,
                                         const char *name) {
  if (!store || !name || !name[0]) return NULL;

  char parent_full[PATH_MAX];
  resolve_parent(store, parent_path, parent_full, sizeof(parent_full));

  /* Ensure parent directory exists */
  if (!is_dir(parent_full)) {
    if (mkdir_p(parent_full) != 0) return NULL;
  }

  char mbox_path[PATH_MAX];
  snprintf(mbox_path, sizeof(mbox_path), "%s/%s", parent_full, name);

  if (macmbx_create(mbox_path) != 0) return NULL;

  macmbx_emit_path(MACMBX_EVENT_MAILBOX_CREATED, mbox_path);
  macmbx_store_refresh(store);

  /* Find and return the new node */
  char rel[PATH_MAX];
  if (parent_path && parent_path[0])
    snprintf(rel, sizeof(rel), "%s/%s", parent_path, name);
  else
    snprintf(rel, sizeof(rel), "%s", name);
  return macmbx_store_find(store, rel);
}

MacmbxNode *macmbx_store_create_folder(MacmbxStore *store,
                                        const char *parent_path,
                                        const char *name) {
  if (!store || !name || !name[0]) return NULL;

  char parent_full[PATH_MAX];
  resolve_parent(store, parent_path, parent_full, sizeof(parent_full));

  char dir_path[PATH_MAX];
  snprintf(dir_path, sizeof(dir_path), "%s/%s", parent_full, name);

  if (mkdir_p(dir_path) != 0 && errno != EEXIST) return NULL;

  macmbx_emit_path(MACMBX_EVENT_FOLDER_CREATED, dir_path);
  macmbx_store_refresh(store);

  char rel[PATH_MAX];
  if (parent_path && parent_path[0])
    snprintf(rel, sizeof(rel), "%s/%s", parent_path, name);
  else
    snprintf(rel, sizeof(rel), "%s", name);
  return macmbx_store_find(store, rel);
}

/* ================================================================
 * Delete, rename, move
 * ================================================================ */

int macmbx_store_delete(MacmbxStore *store, const char *rel_path) {
  if (!store || !rel_path) return -1;

  MacmbxNode *node = macmbx_store_find(store, rel_path);
  if (!node) return -1;

  if (node->type == MACMBX_NODE_FOLDER) {
    /* Only delete empty folders */
    if (node->children) return -1;
    if (rmdir(node->path) != 0) return -1;
    macmbx_emit_path(MACMBX_EVENT_FOLDER_DELETED, node->path);
  } else {
    if (macmbx_remove(node->path) != 0) return -1;
    macmbx_emit_path(MACMBX_EVENT_MAILBOX_DELETED, node->path);
  }

  macmbx_store_refresh(store);
  return 0;
}

int macmbx_store_rename(MacmbxStore *store, const char *rel_path,
                         const char *new_name) {
  if (!store || !rel_path || !new_name) return -1;

  MacmbxNode *node = macmbx_store_find(store, rel_path);
  if (!node) return -1;

  /* Build new path: same parent directory, new name */
  char new_path[PATH_MAX];
  char *last_sep = strrchr(node->path, '/');
#ifdef _WIN32
  char *last_bsep = strrchr(node->path, '\\');
  if (last_bsep && (!last_sep || last_bsep > last_sep)) last_sep = last_bsep;
#endif
  if (last_sep) {
    size_t prefix_len = last_sep - node->path + 1;
    memcpy(new_path, node->path, prefix_len);
    snprintf(new_path + prefix_len, sizeof(new_path) - prefix_len, "%s", new_name);
  } else {
    snprintf(new_path, sizeof(new_path), "%s", new_name);
  }

  char old_path_copy[PATH_MAX];
  snprintf(old_path_copy, sizeof(old_path_copy), "%s", node->path);

  if (node->type == MACMBX_NODE_MAILBOX) {
    if (macmbx_rename(node->path, new_path) != 0) return -1;
  } else {
    if (rename(node->path, new_path) != 0) return -1;
  }

  macmbx_emit_rename(MACMBX_EVENT_MAILBOX_RENAMED, old_path_copy, new_path);
  macmbx_store_refresh(store);
  return 0;
}

int macmbx_store_move(MacmbxStore *store, const char *rel_path,
                       const char *new_parent) {
  if (!store || !rel_path) return -1;

  MacmbxNode *node = macmbx_store_find(store, rel_path);
  if (!node) return -1;

  char dst_dir[PATH_MAX];
  resolve_parent(store, new_parent, dst_dir, sizeof(dst_dir));

  if (!is_dir(dst_dir)) return -1;

  char new_path[PATH_MAX];
  snprintf(new_path, sizeof(new_path), "%s/%s", dst_dir, node->name);

  {
    char old_path_copy[PATH_MAX];
    snprintf(old_path_copy, sizeof(old_path_copy), "%s", node->path);

    if (node->type == MACMBX_NODE_MAILBOX) {
      if (macmbx_rename(node->path, new_path) != 0) return -1;
    } else {
      if (rename(node->path, new_path) != 0) return -1;
    }
    macmbx_emit_rename(MACMBX_EVENT_MAILBOX_MOVED, old_path_copy, new_path);
  }

  macmbx_store_refresh(store);
  return 0;
}

/* ================================================================
 * Open mailbox through store
 * ================================================================ */

MacmbxTOC *macmbx_store_open_mailbox(MacmbxStore *store, const char *rel_path) {
  if (!store || !rel_path) return NULL;
  MacmbxNode *node = macmbx_store_find(store, rel_path);
  if (!node || node->type != MACMBX_NODE_MAILBOX) return NULL;
  return macmbx_toc_open(node->path);
}

/* ================================================================
 * Batch operations
 * ================================================================ */

int macmbx_store_flush(MacmbxStore *store) {
  if (!store) return 0;
  return macmbx_registry_flush();
}

static int compact_tree(MacmbxNode *node) {
  int count = 0;
  for (MacmbxNode *n = node; n; n = n->next) {
    if (n->type == MACMBX_NODE_MAILBOX) {
      MacmbxTOC *toc = macmbx_registry_find(n->path);
      if (toc && macmbx_count_deleted(toc) > 0) {
        if (macmbx_compact(toc) == 0) count++;
      }
    }
    if (n->children) count += compact_tree(n->children);
  }
  return count;
}

int macmbx_store_compact_all(MacmbxStore *store) {
  if (!store || !store->root) return 0;
  return compact_tree(store->root->children);
}

static void update_counts_tree(MacmbxNode *node) {
  for (MacmbxNode *n = node; n; n = n->next) {
    if (n->type == MACMBX_NODE_MAILBOX) {
      /* Try to get counts from registry first (fast) */
      MacmbxTOC *toc = macmbx_registry_find(n->path);
      if (toc) {
        n->total = toc->count;
        n->unread = macmbx_count_unread(toc);
      } else {
        /* Peek the TOC header */
        int count = 0;
        if (macmbx_toc_peek(n->path, &count, NULL, NULL) == 0) {
          n->total = count;
          /* Can't get unread without loading full TOC */
          n->unread = -1;
        }
      }
    }
    if (n->children) update_counts_tree(n->children);
  }
}

void macmbx_store_update_counts(MacmbxStore *store) {
  if (!store || !store->root) return;
  update_counts_tree(store->root->children);
}

/* ================================================================
 * Store locking
 * ================================================================ */

int macmbx_store_lock(MacmbxStore *store) {
  if (!store || store->lock_fd >= 0) return 0;
#ifdef _WIN32
  return 0;
#else
  char lock_path[PATH_MAX];
  snprintf(lock_path, sizeof(lock_path), "%s/.store.lock", store->base_path);
  int fd = open(lock_path, O_CREAT | O_RDWR, 0644);
  if (fd < 0) return -1;
  if (flock(fd, LOCK_EX | LOCK_NB) != 0) { close(fd); return -1; }
  store->lock_fd = fd;
  return 0;
#endif
}

void macmbx_store_unlock(MacmbxStore *store) {
  if (!store || store->lock_fd < 0) return;
#ifndef _WIN32
  flock(store->lock_fd, LOCK_UN);
  close(store->lock_fd);
  char lock_path[PATH_MAX];
  snprintf(lock_path, sizeof(lock_path), "%s/.store.lock", store->base_path);
  remove(lock_path);
#endif
  store->lock_fd = -1;
}

/* ================================================================
 * Flat enumeration
 * ================================================================ */

static void collect_paths(MacmbxNode *node, const char *base,
                           MacmbxNodeType type,
                           char ***paths, int *count, int *cap) {
  for (MacmbxNode *n = node; n; n = n->next) {
    if (n->type == type) {
      if (*count >= *cap) {
        *cap *= 2;
        *paths = (char **)realloc(*paths, *cap * sizeof(char *));
      }
      char rel[PATH_MAX];
      rel_path(base, n->path, rel, sizeof(rel));
      (*paths)[(*count)++] = strdup(rel);
    }
    if (n->children) collect_paths(n->children, base, type, paths, count, cap);
  }
}

int macmbx_store_list_mailboxes(MacmbxStore *store, char ***paths) {
  if (!store || !store->root || !paths) return 0;
  *paths = NULL;
  int count = 0, cap = 32;
  *paths = (char **)calloc(cap, sizeof(char *));
  collect_paths(store->root->children, store->base_path,
                MACMBX_NODE_MAILBOX, paths, &count, &cap);
  return count;
}

int macmbx_store_list_folders(MacmbxStore *store, char ***paths) {
  if (!store || !store->root || !paths) return 0;
  *paths = NULL;
  int count = 0, cap = 16;
  *paths = (char **)calloc(cap, sizeof(char *));
  collect_paths(store->root->children, store->base_path,
                MACMBX_NODE_FOLDER, paths, &count, &cap);
  return count;
}

/* ================================================================
 * Root directory accessor
 * ================================================================ */

const char *macmbx_store_root_dir(MacmbxStore *store) {
  return store ? store->root_path : NULL;
}
