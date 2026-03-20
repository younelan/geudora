/* macmbx_junk.c — Junk mail management
 * Part of macmbx: standalone Eudora mbox storage library.
 *
 * Ported from Eudora junk.c. Scoring is pluggable via callback.
 */

#include "macmbx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/* ================================================================
 * Configuration
 * ================================================================ */

void macmbx_junk_config_init(MacmbxJunkConfig *cfg) {
  if (!cfg) return;
  memset(cfg, 0, sizeof(*cfg));
  cfg->threshold = 50;
  cfg->archive_days = 14;
  cfg->archive_threshold = 50;
  cfg->believe_date = false;
  cfg->server_delete = false;
}

void macmbx_junk_set_scorer(MacmbxJunkConfig *cfg,
                             MacmbxJunkScoreFn fn, void *ctx) {
  if (!cfg) return;
  cfg->score_fn = fn;
  cfg->score_ctx = ctx;
}

void macmbx_junk_set_whitelist(MacmbxJunkConfig *cfg,
                                MacmbxWhitelistFn fn, void *ctx) {
  if (!cfg) return;
  cfg->whitelist_fn = fn;
  cfg->whitelist_ctx = ctx;
}

/* ================================================================
 * Scoring
 * ================================================================ */

int macmbx_junk_score(MacmbxJunkConfig *cfg, MacmbxTOC *toc, int index) {
  if (!cfg || !toc || index < 0 || index >= toc->count) return -1;
  MacmbxMsgSum *msg = &toc->msgs[index];

  /* Whitelist check — if sender is whitelisted, score 0 */
  if (cfg->whitelist_fn) {
    if (cfg->whitelist_fn(msg->from, cfg->whitelist_ctx)) {
      msg->spam_score = 0;
      msg->spam_because = MACMBX_JUNK_BECAUSE_WHITELIST;
      toc->dirty = true;
      return 0;
    }
  }

  /* Call scorer callback */
  if (!cfg->score_fn) return -1;

  char *headers = macmbx_read_headers(toc, index);
  long bodyLen = 0;
  char *body = macmbx_read_body(toc, index, &bodyLen);

  int score = cfg->score_fn(toc, index, headers, body,
                             MACMBX_SCORE_AUTO, cfg->score_ctx);
  free(headers);
  free(body);

  if (score >= 0) {
    if (score > 100) score = 100;
    msg->spam_score = (int8_t)score;
    msg->spam_because = MACMBX_JUNK_BECAUSE_PLUG;
    toc->dirty = true;
  }
  return score;
}

int macmbx_junk_score_box(MacmbxJunkConfig *cfg, MacmbxTOC *toc) {
  if (!cfg || !toc) return 0;
  int scored = 0;
  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    /* Only score if never scored */
    if (toc->msgs[i].spam_score != -1) continue;
    if (macmbx_junk_score(cfg, toc, i) >= 0) scored++;
  }
  return scored;
}

int macmbx_junk_rescore_box(MacmbxJunkConfig *cfg, MacmbxTOC *toc) {
  if (!cfg || !toc) return 0;
  int scored = 0;
  for (int i = 0; i < toc->count; i++) {
    if (toc->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    /* Rescore if never scored or scored by plugin (not by user) */
    if (toc->msgs[i].spam_score != -1 &&
        toc->msgs[i].spam_because == MACMBX_JUNK_BECAUSE_USER) continue;
    if (macmbx_junk_score(cfg, toc, i) >= 0) scored++;
  }
  return scored;
}

/* ================================================================
 * Mark as junk / not-junk
 * ================================================================ */

int macmbx_junk_set_score(MacmbxTOC *toc, int index,
                           int8_t score, uint8_t because) {
  if (!toc || index < 0 || index >= toc->count) return -1;
  toc->msgs[index].spam_score = score;
  toc->msgs[index].spam_because = because;
  toc->dirty = true;
  return 0;
}

int macmbx_junk_mark(MacmbxJunkConfig *cfg, MacmbxTOC *toc, int index,
                      bool is_junk, MacmbxStore *store) {
  if (!cfg || !toc || index < 0 || index >= toc->count) return -1;
  MacmbxMsgSum *msg = &toc->msgs[index];

  if (is_junk) {
    /* Train scorer if available */
    if (cfg->score_fn) {
      char *headers = macmbx_read_headers(toc, index);
      long bodyLen = 0;
      char *body = macmbx_read_body(toc, index, &bodyLen);
      cfg->score_fn(toc, index, headers, body,
                     MACMBX_SCORE_USER_JUNK, cfg->score_ctx);
      free(headers);
      free(body);
    }

    /* Set score high */
    int score = (msg->spam_score > 0) ? msg->spam_score : 100;
    msg->spam_score = (int8_t)score;
    msg->spam_because = MACMBX_JUNK_BECAUSE_USER;

    /* Don't trust date on junk */
    if (!cfg->believe_date && msg->arrival)
      msg->seconds = msg->arrival;

    /* Move to Junk mailbox if store available */
    if (store) {
      MacmbxNode *junk_node = macmbx_store_find_special(store, MACMBX_TYPE_JUNK);
      if (junk_node) {
        MacmbxTOC *junk_toc = macmbx_toc_open(junk_node->path);
        if (junk_toc && junk_toc != toc) {
          macmbx_transfer(toc, index, junk_toc, false);
        }
      }
    }
  } else {
    /* Train as not-junk */
    if (cfg->score_fn) {
      char *headers = macmbx_read_headers(toc, index);
      long bodyLen = 0;
      char *body = macmbx_read_body(toc, index, &bodyLen);
      cfg->score_fn(toc, index, headers, body,
                     MACMBX_SCORE_USER_NOT_JUNK, cfg->score_ctx);
      free(headers);
      free(body);
    }

    msg->spam_score = 0;
    msg->spam_because = MACMBX_JUNK_BECAUSE_USER;
  }

  toc->dirty = true;
  return 0;
}

/* ================================================================
 * Move spam to Junk mailbox
 * ================================================================ */

int macmbx_junk_move_spam(MacmbxJunkConfig *cfg, MacmbxTOC *toc,
                            MacmbxStore *store) {
  if (!cfg || !toc || !store) return 0;

  MacmbxNode *junk_node = macmbx_store_find_special(store, MACMBX_TYPE_JUNK);
  if (!junk_node) return 0;

  MacmbxTOC *junk_toc = macmbx_toc_open(junk_node->path);
  if (!junk_toc || junk_toc == toc) return 0;

  /* Collect indices to move (backwards to avoid index shift) */
  int moved = 0;
  for (int i = toc->count - 1; i >= 0; i--) {
    MacmbxMsgSum *msg = &toc->msgs[i];
    if (msg->flags & MACMBX_FLAG_DELETED) continue;
    if (msg->spam_score < cfg->threshold) continue;

    /* Don't trust date on junk */
    if (!cfg->believe_date && msg->arrival)
      msg->seconds = msg->arrival;

    macmbx_transfer(toc, i, junk_toc, false);
    moved++;
  }

  return moved;
}

/* ================================================================
 * Archive old junk
 * ================================================================ */

int macmbx_junk_archive(MacmbxJunkConfig *cfg, MacmbxTOC *toc) {
  if (!cfg || !toc || cfg->archive_days <= 0) return 0;

  uint32_t now = (uint32_t)time(NULL);
  uint32_t thresh_time = now - (uint32_t)(cfg->archive_days * 86400);
  int deleted = 0;

  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum *msg = &toc->msgs[i];
    if (msg->flags & MACMBX_FLAG_DELETED) continue;

    /* Check age and score */
    uint32_t msg_time = msg->arrival ? msg->arrival : msg->seconds;
    if (msg_time < thresh_time &&
        msg->spam_score >= cfg->archive_threshold) {
      macmbx_delete_message(toc, i);
      deleted++;
    }
  }

  return deleted;
}

/* ================================================================
 * Utilities
 * ================================================================ */

bool macmbx_is_junk_mailbox(MacmbxTOC *toc) {
  if (!toc) return false;
  return toc->which == MACMBX_TYPE_JUNK;
}

void macmbx_junk_toc_cleanse(MacmbxJunkConfig *cfg, MacmbxTOC *toc) {
  if (!toc) return;
  int default_score = cfg ? cfg->threshold : 50;

  for (int i = 0; i < toc->count; i++) {
    MacmbxMsgSum *msg = &toc->msgs[i];
    /* If in Junk mailbox and never scored, set transfer score */
    if (msg->spam_because == MACMBX_JUNK_BECAUSE_NOT_JUNK &&
        macmbx_is_junk_mailbox(toc)) {
      msg->spam_score = (int8_t)default_score;
      msg->spam_because = MACMBX_JUNK_BECAUSE_XFER;
      toc->dirty = true;
    }
  }
}
