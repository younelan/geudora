/* macmbx_compact.c — Compact mbox (remove deleted messages)
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Rewrites the mbox file without deleted messages, updates TOC offsets.
 * Uses a temp file for atomic operation — if anything fails, the
 * original mbox is untouched.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
  #include <io.h>
#else
  #include <unistd.h>
#endif

int macmbx_count_deleted(MacmbxTOC *toc) {
  if (!toc) return 0;
  int count = 0;
  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) count++;
  }
  return count;
}

long macmbx_reclaimable(MacmbxTOC *toc) {
  if (!toc) return 0;
  long bytes = 0;
  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED)
      bytes += toc->msgs[i].length;
  }
  return bytes;
}

int macmbx_compact(MacmbxTOC *toc) {
  if (!toc) return -1;

  int deleted = macmbx_count_deleted(toc);
  if (deleted == 0) return 0; /* nothing to do */

  /* Open source mbox */
  FILE *src = fopen(toc->mbox_path, "rb");
  if (!src) return -1;

  /* Create temp file */
  char tmp_path[PATH_MAX];
  snprintf(tmp_path, sizeof(tmp_path), "%s.compact.tmp", toc->mbox_path);
  FILE *dst = fopen(tmp_path, "wb");
  if (!dst) { fclose(src); return -1; }

  /* Copy non-deleted messages, tracking new offsets */
  long new_offset = 0;
  int new_count = 0;
  char buf[8192];

  /* Build new summary array */
  int live = toc->count - deleted;
  MacmbxMsgSum *new_msgs = (MacmbxMsgSum *)calloc(live > 0 ? live : 1,
                                                    sizeof(MacmbxMsgSum));
  if (!new_msgs) { fclose(src); fclose(dst); remove(tmp_path); return -1; }

  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum *msg = &toc->msgs[i];

    if (msg->flags & MACMBX_FLAG_DELETED) continue;

    /* Seek to message in source */
    if (fseek(src, msg->offset, SEEK_SET) != 0) goto fail;

    /* Copy message bytes */
    long remaining = msg->length;
    while (remaining > 0) {
      size_t chunk = remaining > (long)sizeof(buf) ? sizeof(buf) : (size_t)remaining;
      size_t got = fread(buf, 1, chunk, src);
      if (got == 0) goto fail;
      if (fwrite(buf, 1, got, dst) != got) goto fail;
      remaining -= (long)got;
    }

    /* Update summary with new offset */
    new_msgs[new_count] = *msg;
    new_msgs[new_count].offset = new_offset;
    new_offset += msg->length;
    new_count++;
  }

  fclose(src); src = NULL;
  fclose(dst); dst = NULL;

  /* Atomic replace: rename temp over original */
  /* On POSIX, rename is atomic on same filesystem */
  if (rename(tmp_path, toc->mbox_path) != 0) {
    /* Rename failed — try copy approach */
    FILE *s = fopen(tmp_path, "rb");
    FILE *d = fopen(toc->mbox_path, "wb");
    if (!s || !d) {
      if (s) fclose(s);
      if (d) fclose(d);
      remove(tmp_path);
      free(new_msgs);
      return -1;
    }
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), s)) > 0)
      fwrite(buf, 1, n, d);
    fclose(s);
    fclose(d);
    remove(tmp_path);
  }

  /* Update TOC */
  free(toc->msgs);
  toc->msgs = new_msgs;
  toc->count = new_count;
  toc->capacity = live > 0 ? live : 1;
  toc->dirty = true;

  /* Save updated TOC */
  macmbx_toc_save(toc);

  /* Notify: compact done */
  extern void macmbx_emit_simple(MacmbxEventType, MacmbxTOC *, int);
  macmbx_emit_simple(MACMBX_EVENT_COMPACTED, toc, -1);

  return 0;

fail:
  if (src) fclose(src);
  if (dst) fclose(dst);
  remove(tmp_path);
  free(new_msgs);
  return -1;
}
