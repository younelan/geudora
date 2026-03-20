/* macmbx.c — TOC lifecycle, binary compat I/O, locking, registry, validation
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Binary-compatible with Eudora .toc format:
 *   76-byte TOCDiskHeader + 224-byte MSumDisk per message.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#ifdef _WIN32
  #include <io.h>
  #include <windows.h>
#else
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/file.h>
#endif

/* ================================================================
 * Event dispatch
 * ================================================================ */

#define MACMBX_MAX_HANDLERS 16

typedef struct {
  MacmbxEventFn fn;
  void *ctx;
  bool active;
} MacmbxHandler;

static MacmbxHandler g_handlers[MACMBX_MAX_HANDLERS];
static int g_handler_count = 0;

int macmbx_on(MacmbxEventFn fn, void *ctx) {
  if (!fn) return -1;
  /* Find a free slot */
  for (int i = 0; i < MACMBX_MAX_HANDLERS; i++) {
    if (!g_handlers[i].active) {
      g_handlers[i].fn = fn;
      g_handlers[i].ctx = ctx;
      g_handlers[i].active = true;
      if (i >= g_handler_count) g_handler_count = i + 1;
      return i;
    }
  }
  return -1;
}

void macmbx_off(int handle) {
  if (handle >= 0 && handle < MACMBX_MAX_HANDLERS) {
    g_handlers[handle].active = false;
    g_handlers[handle].fn = NULL;
    g_handlers[handle].ctx = NULL;
  }
}

void macmbx_off_all(void) {
  memset(g_handlers, 0, sizeof(g_handlers));
  g_handler_count = 0;
}

void macmbx_emit(const MacmbxEvent *event) {
  for (int i = 0; i < g_handler_count; i++) {
    if (g_handlers[i].active && g_handlers[i].fn)
      g_handlers[i].fn(event, g_handlers[i].ctx);
  }
}

void macmbx_emit_simple(MacmbxEventType type, MacmbxTOC *toc, int index) {
  MacmbxEvent ev = {0};
  ev.type = type;
  ev.toc = toc;
  ev.index = index;
  macmbx_emit(&ev);
}

void macmbx_emit_path(MacmbxEventType type, const char *path) {
  MacmbxEvent ev = {0};
  ev.type = type;
  ev.path = path;
  macmbx_emit(&ev);
}

void macmbx_emit_rename(MacmbxEventType type, const char *old_path,
                         const char *new_path) {
  MacmbxEvent ev = {0};
  ev.type = type;
  ev.old_path = old_path;
  ev.path = new_path;
  macmbx_emit(&ev);
}

void macmbx_emit_message(MacmbxEventType type, const char *msg) {
  MacmbxEvent ev = {0};
  ev.type = type;
  ev.message = msg;
  macmbx_emit(&ev);
}

/* Portable strcasestr */
#if !defined(_GNU_SOURCE) && !defined(__APPLE__)
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
 * TOC registry — global linked list of open TOCs
 * ================================================================ */

static MacmbxTOC *g_registry = NULL;

static void registry_add(MacmbxTOC *toc) {
  toc->next = g_registry;
  g_registry = toc;
}

static void registry_remove(MacmbxTOC *toc) {
  MacmbxTOC **pp = &g_registry;
  while (*pp) {
    if (*pp == toc) { *pp = toc->next; toc->next = NULL; return; }
    pp = &(*pp)->next;
  }
}

MacmbxTOC *macmbx_registry_find(const char *mbox_path) {
  if (!mbox_path) return NULL;
  for (MacmbxTOC *t = g_registry; t; t = t->next) {
    if (strcmp(t->mbox_path, mbox_path) == 0) return t;
  }
  return NULL;
}

int macmbx_registry_flush(void) {
  int saved = 0;
  for (MacmbxTOC *t = g_registry; t; t = t->next) {
    if (t->dirty) {
      if (macmbx_toc_save(t) == 0) saved++;
    }
  }
  return saved;
}

void macmbx_registry_close_all(void) {
  while (g_registry) {
    MacmbxTOC *t = g_registry;
    g_registry = t->next;
    t->next = NULL;
    if (t->dirty) macmbx_toc_save(t);
    macmbx_unlock(t);
    free(t->msgs);
    free(t);
  }
}

/* ================================================================
 * Helpers
 * ================================================================ */

static void toc_path_from_mbox(const char *mbox, char *toc, size_t sz) {
  snprintf(toc, sz, "%s.toc", mbox);
}

static int file_info(const char *path, long *size, uint32_t *mtime) {
  struct stat st;
  if (stat(path, &st) != 0) return -1;
  if (size) *size = (long)st.st_size;
  if (mtime) *mtime = (uint32_t)st.st_mtime;
  return 0;
}

static const char *path_basename(const char *path) {
  const char *s = strrchr(path, '/');
#ifdef _WIN32
  const char *b = strrchr(path, '\\');
  if (b && (!s || b > s)) s = b;
#endif
  return s ? s + 1 : path;
}

static void toc_ensure_capacity(MacmbxTOC *toc, int needed) {
  if (needed <= toc->capacity) return;
  int newCap = toc->capacity ? toc->capacity * 2 : 64;
  while (newCap < needed) newCap *= 2;
  toc->msgs = (MacmbxMsgSum *)realloc(toc->msgs, newCap * sizeof(MacmbxMsgSum));
  toc->capacity = newCap;
}

/* ================================================================
 * Disk ↔ Memory conversion (binary-compatible with Eudora)
 * ================================================================ */

void macmbx_disk_to_sum(const MacmbxDiskSum *d, MacmbxMsgSum *s) {
  memset(s, 0, sizeof(*s));
  s->offset = d->offset;
  s->length = d->length;
  s->body_offset = d->bodyOffset;
  s->state = (uint8_t)d->state;
  s->spam_score = (int8_t)(d->spamBits & 0xFF);
  s->spam_because = (d->spamBits >> 8) & 0x7;
  s->arrival = d->arrivalSeconds;
  s->from_hash = d->fromHash;
  s->serial_num = d->serialNum;
  s->seconds = d->seconds;
  s->flags = d->flags;
  memcpy(s->saved_pos, d->savedPos, sizeof(s->saved_pos));
  s->priority = d->priority;
  s->origPriority = d->origPriority;
  s->table_id = d->tableId;
  s->score = d->scoreBits & 0xF;
  s->out_type = (d->scoreBits >> 4) & 0xF;
  s->orig_zone = d->origZone;
  s->sig_id = d->sigId;
  strncpy(s->from, d->from, sizeof(s->from) - 1);
  s->from[sizeof(s->from) - 1] = '\0';
  s->pop_pers_id = d->popPersId;
  s->pers_id = d->persId;
  s->msg_id_hash = d->msgIdHash;
  s->subj_id = d->subjId;
  strncpy(s->subject, d->subj, sizeof(s->subject) - 1);
  s->subject[sizeof(s->subject) - 1] = '\0';
  s->opts = d->opts;
  s->uid_hash = d->uidHash;
  /* Detect attachment from flags/opts */
  s->has_attachment = (s->flags & MACMBX_FLAG_ATTACHMENT) != 0;
}

void macmbx_sum_to_disk(const MacmbxMsgSum *s, MacmbxDiskSum *d) {
  memset(d, 0, sizeof(*d));
  d->offset = (int32_t)s->offset;
  d->length = (int32_t)s->length;
  d->bodyOffset = (int32_t)s->body_offset;
  d->state = (int32_t)s->state;
  d->spamBits = ((uint32_t)(s->spam_score & 0xFF)) |
                ((uint32_t)(s->spam_because & 0x7) << 8);
  d->arrivalSeconds = s->arrival;
  d->mesgErrH = 0;
  d->fromHash = s->from_hash;
  d->serialNum = (int32_t)s->serial_num;
  d->seconds = s->seconds;
  d->flags = s->flags;
  memcpy(d->savedPos, s->saved_pos, sizeof(d->savedPos));
  d->priority = s->priority;
  d->origPriority = s->origPriority;
  d->tableId = s->table_id;
  d->scoreBits = (s->score & 0xF) | ((s->out_type & 0xF) << 4);
  d->origZone = s->orig_zone;
  d->sigId = s->sig_id;
  strncpy(d->from, s->from, sizeof(d->from) - 1);
  d->from[sizeof(d->from) - 1] = '\0';
  d->popPersId = s->pop_pers_id;
  d->persId = s->pers_id;
  d->msgIdHash = s->msg_id_hash;
  d->subjId = s->subj_id;
  strncpy(d->subj, s->subject, sizeof(d->subj) - 1);
  d->subj[sizeof(d->subj) - 1] = '\0';
  d->opts = s->opts;
  d->uidHash = s->uid_hash;
  d->cache = 0;
  d->selected = 0;
  d->messH = 0;
}

/* ================================================================
 * TOC binary I/O — Eudora-compatible format
 * ================================================================ */

static MacmbxTOC *toc_load_eudora(const char *toc_path) {
  FILE *f = fopen(toc_path, "rb");
  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fileSize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (fileSize < MACMBX_DISK_HDR_SIZE) { fclose(f); return NULL; }

  MacmbxDiskHeader hdr;
  if (fread(&hdr, sizeof(hdr), 1, f) != 1) { fclose(f); return NULL; }

  int count = hdr.count;
  if (count < 0 || count > 100000) { fclose(f); return NULL; }

  long expectedSize = MACMBX_DISK_SIZE(count);
  if (fileSize < expectedSize) { fclose(f); return NULL; }

  MacmbxTOC *toc = (MacmbxTOC *)calloc(1, sizeof(MacmbxTOC));
  if (!toc) { fclose(f); return NULL; }

  toc->count = count;
  toc->capacity = count > 0 ? count : 16;
  toc->lock_fd = -1;

  /* Copy header fields */
  toc->major_version = hdr.majorVersion;
  toc->minor_version = hdr.minorVersion;
  toc->which = hdr.which;
  toc->box_size = hdr.boxSize;
  toc->write_date = hdr.writeDate;
  toc->next_serial = hdr.nextSerialNum;
  toc->sort_order = hdr.sort;
  toc->last_sort = hdr.lastSort;
  toc->plugin_key = hdr.pluginKey;
  toc->plugin_value = hdr.pluginValue;
  toc->preview_hi = hdr.previewHi;
  toc->unread_base = hdr.unreadBase;
  memcpy(toc->sorts, hdr.sorts, sizeof(toc->sorts));
  toc->needs_compact = hdr.needsCompact;

  toc->msgs = (MacmbxMsgSum *)calloc(toc->capacity, sizeof(MacmbxMsgSum));
  if (!toc->msgs) { free(toc); fclose(f); return NULL; }

  MacmbxDiskSum diskSum;
  for (int i = 0; i < count; i++) {
    if (fread(&diskSum, sizeof(diskSum), 1, f) != 1) {
      free(toc->msgs); free(toc); fclose(f); return NULL;
    }
    macmbx_disk_to_sum(&diskSum, &toc->msgs[i]);
  }
  fclose(f);
  return toc;
}

int macmbx_toc_save(MacmbxTOC *toc) {
  if (!toc) return -1;
  if (toc->being_written) return -1;
  toc->being_written = true;

  long mbox_size = 0;
  file_info(toc->mbox_path, &mbox_size, NULL);
  toc->box_size = (int32_t)(mbox_size + 1);
  toc->write_date = (int32_t)time(NULL);
  toc->unread_base = (int32_t)toc->count;
  toc->major_version = MACMBX_TOC_MAJOR;
  toc->minor_version = MACMBX_TOC_MINOR;

  /* Write to temp, then rename */
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof(tmp), "%s.tmp", toc->toc_path);
  FILE *f = fopen(tmp, "wb");
  if (!f) { toc->being_written = false; return -1; }

  MacmbxDiskHeader hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.majorVersion = toc->major_version;
  hdr.minorVersion = toc->minor_version;
  hdr.count = (int16_t)toc->count;
  hdr.which = toc->which;
  hdr.boxSize = toc->box_size;
  hdr.writeDate = toc->write_date;
  hdr.nextSerialNum = (int32_t)toc->next_serial;
  hdr.sort = toc->sort_order;
  hdr.lastSort = toc->last_sort;
  hdr.pluginKey = toc->plugin_key;
  hdr.pluginValue = toc->plugin_value;
  hdr.previewHi = toc->preview_hi;
  hdr.unreadBase = toc->unread_base;
  memcpy(hdr.sorts, toc->sorts, sizeof(hdr.sorts));
  hdr.needsCompact = toc->needs_compact;

  if (fwrite(&hdr, sizeof(hdr), 1, f) != 1) goto fail;

  if (toc->count > 0) {
    MacmbxDiskSum diskSum;
    for (int i = 0; i < toc->count; i++) {
      macmbx_sum_to_disk(&toc->msgs[i], &diskSum);
      if (fwrite(&diskSum, sizeof(diskSum), 1, f) != 1) goto fail;
    }
  }
  fclose(f);

  if (rename(tmp, toc->toc_path) != 0) { remove(tmp); toc->being_written = false; return -1; }

  toc->dirty = false;
  toc->being_written = false;
  macmbx_emit_simple(MACMBX_EVENT_TOC_SAVED, toc, -1);
  return 0;

fail:
  fclose(f);
  remove(tmp);
  toc->being_written = false;
  return -1;
}

/* ================================================================
 * TOC lifecycle
 * ================================================================ */

MacmbxTOC *macmbx_toc_open(const char *mbox_path) {
  if (!mbox_path) return NULL;

  /* Check registry first */
  MacmbxTOC *existing = macmbx_registry_find(mbox_path);
  if (existing) return existing;

  char toc_path[PATH_MAX];
  toc_path_from_mbox(mbox_path, toc_path, sizeof(toc_path));

  MacmbxTOC *toc = toc_load_eudora(toc_path);
  if (toc) {
    snprintf(toc->mbox_path, sizeof(toc->mbox_path), "%s", mbox_path);
    snprintf(toc->toc_path, sizeof(toc->toc_path), "%s", toc_path);
    if (toc->which == 0) toc->which = (int16_t)macmbx_detect_type(mbox_path);

    int err = macmbx_toc_validate(toc);
    if (err == 0) {
      macmbx_toc_repair(toc);
      registry_add(toc);
      return toc;
    }
    /* Corrupt — rebuild with salvage */
    MacmbxTOC *rebuilt = macmbx_toc_rebuild(mbox_path, toc);
    macmbx_toc_close(toc);
    if (rebuilt) { registry_add(rebuilt); return rebuilt; }
    return NULL;
  }

  /* No TOC — build from scratch */
  toc = macmbx_toc_build(mbox_path);
  if (toc) {
    toc->which = (int16_t)macmbx_detect_type(mbox_path);
    registry_add(toc);
  }
  return toc;
}

void macmbx_toc_close(MacmbxTOC *toc) {
  if (!toc) return;
  registry_remove(toc);
  macmbx_unlock(toc);
  free(toc->msgs);
  free(toc);
}

bool macmbx_toc_valid(MacmbxTOC *toc) {
  return toc && macmbx_toc_validate(toc) == 0;
}

/* ================================================================
 * Peek: read header only
 * ================================================================ */

int macmbx_toc_peek(const char *mbox_path, int *count, int *which,
                     long *box_size) {
  char toc_path[PATH_MAX];
  toc_path_from_mbox(mbox_path, toc_path, sizeof(toc_path));
  FILE *f = fopen(toc_path, "rb");
  if (!f) return -1;
  MacmbxDiskHeader hdr;
  if (fread(&hdr, sizeof(hdr), 1, f) != 1) { fclose(f); return -1; }
  fclose(f);
  if (count) *count = hdr.count;
  if (which) *which = hdr.which;
  if (box_size) *box_size = hdr.boxSize;
  return 0;
}

/* ================================================================
 * Validation and repair
 * ================================================================ */

int macmbx_toc_validate(MacmbxTOC *toc) {
  if (!toc) return MACMBX_ERR_CORRUPT;
  if (toc->count < 0) return MACMBX_ERR_CORRUPT;

  long mbox_size = 0;
  file_info(toc->mbox_path, &mbox_size, NULL);

  /* Size mismatch check */
  if (toc->box_size > 0 && mbox_size > 0 &&
      labs((long)toc->box_size - mbox_size) > 1)
    return MACMBX_ERR_MISMATCH;

  /* Check each summary */
  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum *m = &toc->msgs[i];
    if (m->offset < 0 || m->length < 0 || m->body_offset < 0 ||
        m->body_offset > m->length ||
        (m->offset + m->length > mbox_size && mbox_size > 0))
      return MACMBX_ERR_CORRUPT;
  }

  /* Version check */
  if (toc->major_version > MACMBX_TOC_MAJOR)
    return MACMBX_ERR_BAD_VERSION;

  return 0;
}

static void check_string(char *s, int max_len, int buf_size) {
  int len = (int)strlen(s);
  if (len > buf_size) {
    memset(s, 0, buf_size); /* total garbage */
  } else if (len > max_len) {
    s[max_len] = '\0';
  }
}

void macmbx_toc_repair(MacmbxTOC *toc) {
  if (!toc) return;
  bool need_serials = toc->major_version < 1 ||
                      (toc->major_version == 1 && toc->minor_version < 2);
  if (need_serials) toc->next_serial = 1;

  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum *m = &toc->msgs[i];
    check_string(m->from, (int)sizeof(m->from) - 1, 48);
    check_string(m->subject, (int)sizeof(m->subject) - 1, 60);
    if (!m->arrival) m->arrival = m->seconds;
    if (need_serials) m->serial_num = toc->next_serial++;
    if (m->spam_score == 0 && m->spam_because == 0 &&
        toc->minor_version < 5) {
      m->spam_score = -1;
    }
  }

  toc->major_version = MACMBX_TOC_MAJOR;
  toc->minor_version = MACMBX_TOC_MINOR;
}

/* ================================================================
 * File locking
 * ================================================================ */

int macmbx_lock(MacmbxTOC *toc) {
  if (!toc || toc->lock_fd >= 0) return 0; /* already locked */
#ifdef _WIN32
  return 0; /* TODO: Windows locking */
#else
  char lock_path[PATH_MAX];
  snprintf(lock_path, sizeof(lock_path), "%s.lock", toc->mbox_path);
  int fd = open(lock_path, O_CREAT | O_RDWR, 0644);
  if (fd < 0) return -1;
  if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
    close(fd);
    return -1; /* already locked by another process */
  }
  toc->lock_fd = fd;
  return 0;
#endif
}

void macmbx_unlock(MacmbxTOC *toc) {
  if (!toc || toc->lock_fd < 0) return;
#ifndef _WIN32
  flock(toc->lock_fd, LOCK_UN);
  close(toc->lock_fd);
  char lock_path[PATH_MAX];
  snprintf(lock_path, sizeof(lock_path), "%s.lock", toc->mbox_path);
  remove(lock_path);
#endif
  toc->lock_fd = -1;
}

/* ================================================================
 * Special mailbox type detection
 * ================================================================ */

MacmbxType macmbx_detect_type(const char *mbox_path) {
  if (!mbox_path) return MACMBX_TYPE_NORMAL;
  const char *name = path_basename(mbox_path);
  if (strcasecmp(name, "In") == 0)       return MACMBX_TYPE_IN;
  if (strcasecmp(name, "Out") == 0)      return MACMBX_TYPE_OUT;
  if (strcasecmp(name, "Trash") == 0)    return MACMBX_TYPE_TRASH;
  if (strcasecmp(name, "Junk") == 0)     return MACMBX_TYPE_JUNK;
  if (strcasecmp(name, "In.temp") == 0)  return MACMBX_TYPE_IN_TEMP;
  if (strcasecmp(name, "Out.temp") == 0) return MACMBX_TYPE_OUT_TEMP;
  return MACMBX_TYPE_NORMAL;
}

/* ================================================================
 * Attachment detection from headers
 * ================================================================ */

bool macmbx_detect_attachment(const char *headers) {
  if (!headers) return false;
  /* Check for Content-Type: multipart/mixed (main indicator) */
  if (strcasestr(headers, "multipart/mixed")) return true;
  /* Check for Content-Disposition: attachment */
  if (strcasestr(headers, "Content-Disposition: attachment")) return true;
  /* Check for Eudora's Attachment: header */
  if (strncasecmp(headers, "Attachment:", 11) == 0) return true;
  const char *att = headers;
  while ((att = strcasestr(att, "\nAttachment:")) != NULL) {
    return true;
  }
  return false;
}

/* ================================================================
 * Message access
 * ================================================================ */

char *macmbx_read_message(MacmbxTOC *toc, int index, long *outLen) {
  if (!toc || index < 0 || index >= toc->count) return NULL;
  MacmbxMsgSum *msg = &toc->msgs[index];
  FILE *f = fopen(toc->mbox_path, "rb");
  if (!f) return NULL;
  if (fseek(f, msg->offset, SEEK_SET) != 0) { fclose(f); return NULL; }
  char *buf = (char *)malloc(msg->length + 1);
  if (!buf) { fclose(f); return NULL; }
  long got = (long)fread(buf, 1, msg->length, f);
  fclose(f);
  buf[got] = '\0';
  if (outLen) *outLen = got;
  return buf;
}

char *macmbx_read_headers(MacmbxTOC *toc, int index) {
  if (!toc || index < 0 || index >= toc->count) return NULL;
  MacmbxMsgSum *msg = &toc->msgs[index];
  int hdr_len = msg->body_offset > 0 ? msg->body_offset : msg->length;
  FILE *f = fopen(toc->mbox_path, "rb");
  if (!f) return NULL;
  if (fseek(f, msg->offset, SEEK_SET) != 0) { fclose(f); return NULL; }
  char *buf = (char *)malloc(hdr_len + 1);
  if (!buf) { fclose(f); return NULL; }
  long got = (long)fread(buf, 1, hdr_len, f);
  fclose(f);
  buf[got] = '\0';
  return buf;
}

char *macmbx_read_body(MacmbxTOC *toc, int index, long *outLen) {
  if (!toc || index < 0 || index >= toc->count) return NULL;
  MacmbxMsgSum *msg = &toc->msgs[index];
  if (msg->body_offset <= 0) return NULL;
  long body_len = msg->length - msg->body_offset;
  if (body_len <= 0) return NULL;
  FILE *f = fopen(toc->mbox_path, "rb");
  if (!f) return NULL;
  if (fseek(f, msg->offset + msg->body_offset, SEEK_SET) != 0) {
    fclose(f); return NULL;
  }
  char *buf = (char *)malloc(body_len + 1);
  if (!buf) { fclose(f); return NULL; }
  long got = (long)fread(buf, 1, body_len, f);
  fclose(f);
  buf[got] = '\0';
  if (outLen) *outLen = got;
  return buf;
}

char *macmbx_read_header_field(MacmbxTOC *toc, int index, const char *field) {
  char *headers = macmbx_read_headers(toc, index);
  if (!headers) return NULL;
  size_t flen = strlen(field);
  char *result = NULL;
  char *p = headers;
  while (*p) {
    if (strncasecmp(p, field, flen) == 0 && p[flen] == ':') {
      char *val = p + flen + 1;
      while (*val == ' ' || *val == '\t') val++;
      char *end = val;
      while (*end) {
        if (*end == '\r' || *end == '\n') {
          end++;
          if (*end == '\r' || *end == '\n') end++;
          if (*end == ' ' || *end == '\t') continue;
          break;
        }
        end++;
      }
      while (end > val && (end[-1] == '\r' || end[-1] == '\n')) end--;
      size_t vlen = end - val;
      result = (char *)malloc(vlen + 1);
      if (result) { memcpy(result, val, vlen); result[vlen] = '\0'; }
      break;
    }
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;
  }
  free(headers);
  return result;
}

/* ================================================================
 * Message modification
 * ================================================================ */

int macmbx_append_message(MacmbxTOC *toc, const char *message, long len,
                           const char *sender, uint8_t state, uint8_t priority) {
  if (!toc || !message) return -1;
  if (len < 0) len = (long)strlen(message);

  FILE *f = fopen(toc->mbox_path, "ab");
  if (!f) return -1;

  /* Write "From " separator */
  char from_line[512];
  macmbx_write_from_line(from_line, sizeof(from_line), sender);
  long from_len = (long)strlen(from_line);
  fwrite(from_line, 1, from_len, f);

  /* Write message */
  fwrite(message, 1, len, f);
  if (len > 0 && message[len-1] != '\n') fwrite("\n", 1, 1, f);
  fwrite("\n", 1, 1, f);

  fseek(f, 0, SEEK_END);
  long file_end = ftell(f);
  fclose(f);

  /* Build summary */
  toc_ensure_capacity(toc, toc->count + 1);
  MacmbxMsgSum *sum = &toc->msgs[toc->count];
  memset(sum, 0, sizeof(*sum));

  long total_written = from_len + len;
  if (len > 0 && message[len-1] != '\n') total_written++;
  total_written++; /* trailing blank */

  sum->offset = file_end - total_written;
  sum->length = total_written;
  sum->state = state;
  sum->priority = priority ? priority : 3;
  sum->serial_num = toc->next_serial++;
  sum->seconds = (uint32_t)time(NULL);
  sum->arrival = sum->seconds;
  sum->spam_score = -1;
  sum->flags = MACMBX_FLAG_UTF8;

  /* Extract from/subject from headers */
  const char *p = message;
  while (p < message + len) {
    if (*p == '\r' || *p == '\n') {
      if (*p == '\r') p++;
      if (*p == '\n') p++;
      sum->body_offset = (int)(p - message) + (int)from_len;
      break;
    }
    if (strncasecmp(p, "From:", 5) == 0) {
      const char *val = p + 5;
      while (*val == ' ' || *val == '\t') val++;
      const char *end = val;
      while (*end && *end != '\r' && *end != '\n') end++;
      size_t vlen = (size_t)(end - val);
      if (vlen >= sizeof(sum->from)) vlen = sizeof(sum->from) - 1;
      memcpy(sum->from, val, vlen);
      sum->from[vlen] = '\0';
    } else if (strncasecmp(p, "Subject:", 8) == 0) {
      const char *val = p + 8;
      while (*val == ' ' || *val == '\t') val++;
      const char *end = val;
      while (*end && *end != '\r' && *end != '\n') end++;
      size_t vlen = (size_t)(end - val);
      if (vlen >= sizeof(sum->subject)) vlen = sizeof(sum->subject) - 1;
      memcpy(sum->subject, val, vlen);
      sum->subject[vlen] = '\0';
    }
    while (p < message + len && *p != '\n') p++;
    if (*p == '\n') p++;
  }
  if (sender && !sum->from[0])
    snprintf(sum->from, sizeof(sum->from), "%s", sender);

  /* Detect attachment */
  sum->has_attachment = macmbx_detect_attachment(message);
  if (sum->has_attachment) sum->flags |= MACMBX_FLAG_ATTACHMENT;

  int idx = toc->count++;
  toc->dirty = true;
  { MacmbxEvent ev = {0}; ev.type = MACMBX_EVENT_NEW_MAIL; ev.toc = toc;
    ev.index = idx; ev.count = 1; macmbx_emit(&ev); }
  return idx;
}

int macmbx_delete_message(MacmbxTOC *toc, int index) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].flags |= MACMBX_FLAG_DELETED;
  toc->msgs[index].opts |= MACMBX_OPT_DELETED;
  toc->dirty = true;
  macmbx_emit_simple(MACMBX_EVENT_DELETED, toc, index);
  return 0;
}

int macmbx_undelete_message(MacmbxTOC *toc, int index) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].flags &= ~MACMBX_FLAG_DELETED;
  toc->msgs[index].opts &= ~MACMBX_OPT_DELETED;
  toc->dirty = true;
  macmbx_emit_simple(MACMBX_EVENT_UNDELETED, toc, index);
  return 0;
}

int macmbx_set_state(MacmbxTOC *toc, int index, uint8_t state) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].state = state;
  toc->dirty = true;
  macmbx_emit_simple(MACMBX_EVENT_STATE_CHANGED, toc, index);
  return 0;
}

int macmbx_set_flags(MacmbxTOC *toc, int index, uint32_t flags) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].flags |= flags;
  toc->dirty = true;
  macmbx_emit_simple(MACMBX_EVENT_FLAGS_CHANGED, toc, index);
  return 0;
}

int macmbx_clear_flags(MacmbxTOC *toc, int index, uint32_t flags) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].flags &= ~flags;
  toc->dirty = true;
  macmbx_emit_simple(MACMBX_EVENT_FLAGS_CHANGED, toc, index);
  return 0;
}

int macmbx_set_priority(MacmbxTOC *toc, int index, uint8_t priority) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].priority = priority;
  toc->dirty = true;
  return 0;
}

int macmbx_set_label(MacmbxTOC *toc, int index, uint8_t label) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].flags = (toc->msgs[index].flags & ~MACMBX_FLAG_LABEL_MASK) |
    ((uint32_t)(label & 7) << MACMBX_FLAG_LABEL_SHIFT);
  toc->dirty = true;
  return 0;
}

uint8_t macmbx_get_label(MacmbxTOC *toc, int index) {
  if (!toc || index < 0 || index >= toc->count) return 0;
  return (toc->msgs[index].flags & MACMBX_FLAG_LABEL_MASK) >> MACMBX_FLAG_LABEL_SHIFT;
}

/* ================================================================
 * Mbox utilities
 * ================================================================ */

void macmbx_write_from_line(char *buf, size_t bufsz, const char *sender) {
  time_t now = time(NULL);
  struct tm *tm = gmtime(&now);
  char timebuf[64];
  strftime(timebuf, sizeof(timebuf), "%a %b %d %H:%M:%S %Y", tm);
  snprintf(buf, bufsz, "From %s %s\n",
           (sender && sender[0]) ? sender : "unknown", timebuf);
}

/* ================================================================
 * Statistics
 * ================================================================ */

int macmbx_count_unread(MacmbxTOC *toc) {
  if (!toc) return 0;
  int n = 0;
  for (int i = 0; i < toc->count; i++)
    if (toc->msgs[i].state == MACMBX_UNREAD &&
        !(toc->msgs[i].flags & MACMBX_FLAG_DELETED)) n++;
  return n;
}

int macmbx_count_flagged(MacmbxTOC *toc) {
  if (!toc) return 0;
  int n = 0;
  for (int i = 0; i < toc->count; i++)
    if ((toc->msgs[i].flags & MACMBX_FLAG_FLAGGED) &&
        !(toc->msgs[i].flags & MACMBX_FLAG_DELETED)) n++;
  return n;
}

long macmbx_total_size(MacmbxTOC *toc) {
  if (!toc) return 0;
  long total = 0;
  for (int i = 0; i < toc->count; i++) total += toc->msgs[i].length;
  return total;
}
