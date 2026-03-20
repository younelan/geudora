# Junk Mail Management

Spam scoring, marking, moving, and archiving with pluggable classifier.

## Design

- **Pluggable scoring** — register any classifier via callback. The library handles scoring flow, threshold comparison, moving, and archiving.
- **Pluggable whitelist** — register an address book check. Whitelisted senders are never junk.
- **Ported from Eudora** — same logic as junk.c: score, threshold, move to Junk, archive old, rescan, user mark junk/not-junk.

## Configuration

```c
MacmbxJunkConfig cfg;
macmbx_junk_config_init(&cfg);  /* sensible defaults */

cfg.threshold = 50;        /* score >= 50 is junk */
cfg.archive_days = 14;     /* delete junk older than 14 days */
cfg.archive_threshold = 50;/* only archive high-scoring junk */
cfg.believe_date = false;  /* use arrival time, not Date: header */
cfg.server_delete = false; /* don't delete from server */
```

| Field | Default | Description |
|-------|---------|-------------|
| `threshold` | 50 | Score >= this is junk (0-100) |
| `archive_days` | 14 | Auto-delete junk older than N days (0=never) |
| `archive_threshold` | 50 | Only archive if score >= this |
| `believe_date` | false | If false, uses arrival time for junk (prevents date spoofing) |
| `server_delete` | false | Delete from POP server when junked |

## Registering a Scorer

```c
int my_scorer(MacmbxTOC *toc, int index,
              const char *headers, const char *body,
              MacmbxScoreAction action, void *ctx) {
  switch (action) {
  case MACMBX_SCORE_AUTO:
  case MACMBX_SCORE_RESCAN:
    /* Return 0-100 spam score */
    return bayesian_classify(headers, body);

  case MACMBX_SCORE_USER_JUNK:
    /* User marked as junk — train classifier */
    bayesian_train_spam(headers, body);
    return 100;

  case MACMBX_SCORE_USER_NOT_JUNK:
    /* User marked as ham — train classifier */
    bayesian_train_ham(headers, body);
    return 0;
  }
  return -1;  /* skip */
}

macmbx_junk_set_scorer(&cfg, my_scorer, my_classifier);
```

### Score Actions

| Action | When | Expected behavior |
|--------|------|-------------------|
| `MACMBX_SCORE_AUTO` | New mail arrives | Score the message, return 0-100 |
| `MACMBX_SCORE_RESCAN` | User rescans mailbox | Re-score, return 0-100 |
| `MACMBX_SCORE_USER_JUNK` | User marks as junk | Train as spam, return score |
| `MACMBX_SCORE_USER_NOT_JUNK` | User marks as not-junk | Train as ham, return 0 |

## Registering a Whitelist

```c
bool my_whitelist(const char *from, void *ctx) {
  AddressBook *ab = (AddressBook *)ctx;
  return address_book_contains(ab, from);
}

macmbx_junk_set_whitelist(&cfg, my_whitelist, address_book);
```

Whitelisted senders always get score 0 and `MACMBX_JUNK_BECAUSE_WHITELIST`.

## Functions

### Scoring

| Function | Description |
|----------|-------------|
| `macmbx_junk_score(cfg, toc, index)` | Score one message. Checks whitelist first, then calls scorer. Returns score or -1. |
| `macmbx_junk_score_box(cfg, toc)` | Score all unscored messages (spam_score == -1). Returns count scored. |
| `macmbx_junk_rescore_box(cfg, toc)` | Rescore all except user-marked. Returns count scored. |

### Marking

| Function | Description |
|----------|-------------|
| `macmbx_junk_mark(cfg, toc, index, is_junk, store)` | Mark as junk or not-junk. Trains scorer, sets score, moves to Junk if store provided. |
| `macmbx_junk_set_score(toc, index, score, because)` | Set score directly (no callback). |

### Score Sources (because field)

| Value | Constant | Meaning |
|-------|----------|---------|
| 0 | `MACMBX_JUNK_BECAUSE_NOT_JUNK` | Not scored / not junk |
| 1 | `MACMBX_JUNK_BECAUSE_PLUG` | Scored by classifier callback |
| 2 | `MACMBX_JUNK_BECAUSE_USER` | Manually marked by user |
| 3 | `MACMBX_JUNK_BECAUSE_XFER` | Moved to Junk mailbox |
| 4 | `MACMBX_JUNK_BECAUSE_WHITELIST` | Whitelisted sender |

### Moving

| Function | Description |
|----------|-------------|
| `macmbx_junk_move_spam(cfg, toc, store)` | Move all messages above threshold to Junk mailbox. Returns count moved. |

### Archiving

| Function | Description |
|----------|-------------|
| `macmbx_junk_archive(cfg, toc)` | Delete junk messages older than archive_days with score >= archive_threshold. Call on the Junk mailbox. Returns count deleted. |

### Utilities

| Function | Description |
|----------|-------------|
| `macmbx_is_junk_mailbox(toc)` | Check if TOC is the Junk mailbox (by which field). |
| `macmbx_junk_toc_cleanse(cfg, toc)` | Set transfer score on unscored messages in Junk mailbox. Called on TOC load. |

## Example: Full Junk Flow

```c
/* Setup */
MacmbxJunkConfig cfg;
macmbx_junk_config_init(&cfg);
macmbx_junk_set_scorer(&cfg, my_bayesian_scorer, my_db);
macmbx_junk_set_whitelist(&cfg, my_address_book_check, my_ab);

MacmbxStore *store = macmbx_store_open("/path/to/mailboxes");
MacmbxTOC *inbox = macmbx_store_open_mailbox(store, "In");

/* After downloading new mail: score + move spam */
macmbx_junk_score_box(&cfg, inbox);
int moved = macmbx_junk_move_spam(&cfg, inbox, store);
printf("Moved %d spam messages to Junk\n", moved);

/* User marks message 3 as not-junk (trains classifier) */
macmbx_junk_mark(&cfg, inbox, 3, false, store);

/* Periodic: archive old junk */
MacmbxTOC *junk = macmbx_store_open_mailbox(store, "Junk");
if (junk) {
  int archived = macmbx_junk_archive(&cfg, junk);
  if (archived > 0) {
    macmbx_compact(junk);
    printf("Archived %d old junk messages\n", archived);
  }
}

/* Rescan after training */
macmbx_junk_rescore_box(&cfg, inbox);

macmbx_store_flush(store);
```

## Integration with Filters

Junk scoring works alongside filters. Typical flow:

1. Download new mail to inbox
2. `macmbx_junk_score_box()` — score all new messages
3. `macmbx_filter_apply_all()` — apply user filters (may move messages)
4. `macmbx_junk_move_spam()` — move remaining spam to Junk

Or use a filter rule with `Junk:` header and `greater`/`less` verbs for threshold-based actions within the filter engine.
