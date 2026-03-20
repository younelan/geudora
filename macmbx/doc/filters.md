# Filters

Rule-based message routing engine. Eudora Filters file compatible.

## Design

- **Pure logic** — match conditions produce actions. Built-in actions (move, label, priority, state) execute directly. App-specific actions (sound, open, forward) delegate to a callback.
- **Eudora compatible** — reads and writes the same Filters file format Eudora uses.
- **Optional callback** — for actions macmbx can't handle (play sound, open window, send reply). Pass NULL if not needed.

## Rule Structure

Each rule has:
- **Name** — display name
- **When** — INCOMING, OUTGOING, MANUAL (flags, can combine)
- **1-2 conditions** — header field + verb + value, optionally joined by AND/OR/UNLESS
- **1-5 actions** — what to do when conditions match

```c
typedef struct {
  char name[128];
  int id;
  uint8_t when;                          /* MACMBX_WHEN_* flags */
  MacmbxConjunction conjunction;          /* AND, OR, UNLESS */
  MacmbxCondition conditions[2];
  int condition_count;
  MacmbxAction actions[5];
  int action_count;
} MacmbxRule;
```

## Conditions

```c
typedef struct {
  char header[64];     /* "From:", "Subject:", "To:", "Any:", "Junk:" */
  MacmbxVerb verb;     /* how to compare */
  char value[256];     /* what to compare against */
} MacmbxCondition;
```

### Match Verbs

| Verb | File keyword | Meaning |
|------|-------------|---------|
| `MACMBX_VERB_CONTAINS` | `contains` | Case-insensitive substring |
| `MACMBX_VERB_NOT_CONTAINS` | `!contains` | Does not contain |
| `MACMBX_VERB_IS` | `is` | Exact match (case-insensitive) |
| `MACMBX_VERB_IS_NOT` | `!is` | Not exact match |
| `MACMBX_VERB_STARTS_WITH` | `starts` | Starts with prefix |
| `MACMBX_VERB_ENDS_WITH` | `ends` | Ends with suffix |
| `MACMBX_VERB_APPEARS` | `appears` | Header exists and non-empty |
| `MACMBX_VERB_NOT_APPEARS` | `!appears` | Header missing or empty |
| `MACMBX_VERB_REGEX` | `regex` | POSIX extended regex (if compiled with `MACMBX_HAVE_REGEX`) |
| `MACMBX_VERB_JUNK_LESS` | `less` | Spam score below value |
| `MACMBX_VERB_JUNK_MORE` | `greater` | Spam score above value |
| `MACMBX_VERB_DATE_BEFORE` | `before` | Date is before value (mm/dd/yyyy or days-ago) |
| `MACMBX_VERB_DATE_AFTER` | `after` | Date is after value |
| `MACMBX_VERB_DATE_IS` | `dateIs` | Date matches value (same day) |
| `MACMBX_VERB_PRIORITY_IS` | `priorIs` | Priority equals value (1-5) |
| `MACMBX_VERB_PRIORITY_ABOVE` | `priorAbove` | Higher priority (lower number) |
| `MACMBX_VERB_PRIORITY_BELOW` | `priorBelow` | Lower priority (higher number) |

### Special Headers

| Header | What it matches |
|--------|----------------|
| `From:` | Summary `from` field (fast, no disk I/O) |
| `Subject:` | Summary `subject` field (fast) |
| `Any:` | From + Subject + To + Cc (reads headers from mbox) |
| `Junk:` | Spam score (use with `less`/`greater` verbs) |
| `Date:` | Message date (use with `before`/`after`/`dateIs` verbs; value is mm/dd/yyyy, yyyy-mm-dd, or days-ago number) |
| `Priority:` | Message priority 1-5 (use with `priorIs`/`priorAbove`/`priorBelow` verbs) |
| `Body:` or `<<Body>>` | Full message body text (reads from mbox, slower) |
| Other | Reads actual header from mbox file |

### Conjunctions

| Conjunction | Meaning |
|-------------|---------|
| `MACMBX_CONJ_AND` | Both conditions must match |
| `MACMBX_CONJ_OR` | Either condition matches |
| `MACMBX_CONJ_UNLESS` | First matches AND second does NOT |

## Actions

| Action | File keyword | Built-in? | Description |
|--------|-------------|-----------|-------------|
| `MACMBX_ACT_STATUS` | `status N` | Yes | Set message state (0=unread, 1=read, etc.) |
| `MACMBX_ACT_PRIORITY` | `priority N` | Yes | Set priority (1-5) |
| `MACMBX_ACT_LABEL` | `label N` | Yes | Set label color (0-7) |
| `MACMBX_ACT_TRANSFER` | `transfer Mailbox` | Yes | Move to mailbox (needs store) |
| `MACMBX_ACT_COPY` | `copy Mailbox` | Yes | Copy to mailbox (needs store) |
| `MACMBX_ACT_DELETE` | | Yes | Mark deleted |
| `MACMBX_ACT_JUNK` | `junk N` | Yes | Set spam score |
| `MACMBX_ACT_STOP` | `stop` | Yes | Stop processing further rules |
| `MACMBX_ACT_SOUND` | `sound Name` | Callback | Play sound |
| `MACMBX_ACT_OPEN` | `open` | Callback | Open message window |
| `MACMBX_ACT_PRINT` | `print` | Callback | Print message |
| `MACMBX_ACT_FORWARD` | `forward addr` | Callback | Forward to address |
| `MACMBX_ACT_REDIRECT` | `redirect addr` | Callback | Redirect to address |
| `MACMBX_ACT_REPLY` | `reply` | Callback | Auto-reply |
| `MACMBX_ACT_NOTIFY` | `notifyUser` | Callback | Notify user |

## Filter Set Management

| Function | Description |
|----------|-------------|
| `macmbx_filter_new()` | Create empty filter set. |
| `macmbx_filter_free(fs)` | Free filter set. |
| `macmbx_filter_load(path)` | Load from Eudora Filters file. |
| `macmbx_filter_save(fs)` | Save to Eudora Filters file. |
| `macmbx_filter_add_rule(fs, rule)` | Add a rule. Returns index. |
| `macmbx_filter_remove_rule(fs, index)` | Remove a rule. |
| `macmbx_filter_move_rule(fs, from, to)` | Reorder a rule. |
| `macmbx_filter_get_rule(fs, index)` | Get rule by index. |

## Matching and Execution

| Function | Description |
|----------|-------------|
| `macmbx_filter_match(rule, toc, index)` | Test if one rule matches a message. Pure logic, no side effects. |
| `macmbx_filter_apply(fs, toc, index, store, fn, ctx)` | Apply all rules to one message. Returns result. |
| `macmbx_filter_apply_ex(fs, toc, index, no_xfer, store, fn, ctx)` | Same but with noXfer flag — skip transfer/copy/delete actions. For preview/test. |
| `macmbx_filter_apply_all(fs, toc, when, store, fn, ctx)` | Apply to all messages. `when` selects INCOMING/OUTGOING/MANUAL rules. Returns count matched. |
| `macmbx_filter_apply_selected(fs, toc, indices, count, when, no_xfer, store, fn, ctx)` | Apply to specific messages only. Indices sorted ascending. Returns count matched. |

### Filter Result

```c
typedef struct {
  bool matched;              /* at least one rule matched */
  bool stopped;              /* hit a STOP action */
  bool transferred;          /* message was moved */
  bool copied;               /* message was copied */
  bool deleted;              /* message was marked deleted */
  char transfer_dest[PATH_MAX]; /* destination if transferred */
  int new_state;             /* -1 = unchanged */
  int new_priority;          /* -1 = unchanged */
  int new_label;             /* -1 = unchanged */
} MacmbxFilterResult;
```

## File Format

Eudora-compatible plain text, one rule per block:

```
rule Subject:: Jean
id 4
incoming
manual
header Subject:
verb contains
value Jean
priority 1
rule From:: bob
id 2
incoming
outgoing
header From:
verb contains
value bob
label 3
rule From:: hello
id 1
header From:
verb contains
value hello
conjunction and
header Subject:
verb !contains
value unsubscribe
priority 1
label 1
sound Default
stop
```

## Example: Apply Incoming Filters

```c
MacmbxStore *store = macmbx_store_open("/path/to/mailboxes");
MacmbxTOC *inbox = macmbx_store_open_mailbox(store, "In");
MacmbxFilterSet *filters = macmbx_filter_load("/path/to/Filters");

/* After downloading new mail, apply incoming filters */
int matched = macmbx_filter_apply_all(filters, inbox,
                                       MACMBX_WHEN_INCOMING,
                                       store, my_action_callback, app);
printf("%d messages filtered\n", matched);

macmbx_store_flush(store);
macmbx_filter_free(filters);
```

## Example: Custom Action Callback

```c
void my_action_callback(MacmbxTOC *toc, int index,
                         const MacmbxAction *action, void *ctx) {
  AppState *app = (AppState *)ctx;
  switch (action->type) {
  case MACMBX_ACT_SOUND:
    play_sound(action->str_value);
    break;
  case MACMBX_ACT_FORWARD:
    send_forward(app->smtp, toc, index, action->str_value);
    break;
  case MACMBX_ACT_NOTIFY:
    show_notification("New mail", toc->msgs[index].subject);
    break;
  default:
    break;
  }
}
```

## Example: Create a Filter Programmatically

```c
MacmbxFilterSet *fs = macmbx_filter_new();

MacmbxRule rule = {0};
snprintf(rule.name, sizeof(rule.name), "Move newsletters");
rule.when = MACMBX_WHEN_INCOMING;
rule.condition_count = 1;
snprintf(rule.conditions[0].header, 64, "From:");
rule.conditions[0].verb = MACMBX_VERB_CONTAINS;
snprintf(rule.conditions[0].value, 256, "newsletter@");
rule.action_count = 2;
rule.actions[0].type = MACMBX_ACT_TRANSFER;
snprintf(rule.actions[0].str_value, PATH_MAX, "Newsletters");
rule.actions[1].type = MACMBX_ACT_LABEL;
rule.actions[1].int_value = 3;

macmbx_filter_add_rule(fs, &rule);
snprintf(fs->path, PATH_MAX, "/path/to/Filters");
macmbx_filter_save(fs);
macmbx_filter_free(fs);
```

## Multi-Pass Execution

Actions within a matched rule execute in two passes (matching Eudora's behavior):
- **Pass 0**: Metadata changes — status, priority, label, junk score, subject
- **Pass 1**: Transfers, copies, deletes, stop, callbacks

This ensures metadata is set before the message gets moved.

## noXfer Flag

When `no_xfer=true`:
- TRANSFER, COPY, and DELETE actions are skipped
- All other actions (status, priority, label, sound, etc.) still execute
- Useful for: filter preview ("what would happen?"), manual mode, testing

## Processing Order

Rules are applied in order (first rule first). Processing stops when:
1. A `STOP` action is hit
2. A `TRANSFER` action moves the message (it's gone from the source)
3. All rules have been checked

For `macmbx_filter_apply_all()` and `macmbx_filter_apply_selected()`, messages are processed in reverse order (highest index first) so that transfers don't invalidate indices of unprocessed messages.
