# Message Deduplication

Prevent duplicate messages using Message-ID hashing.

## How It Works

Each message summary stores `msg_id_hash` -- a djb2 hash of the Message-ID header value (without angle brackets). When appending a new message, the hash is checked against all existing messages in the TOC.

## Functions

| Function | Description |
|----------|-------------|
| `macmbx_find_duplicate(toc, msg_id_hash)` | Check if a hash exists in the TOC. Returns existing index or -1. |
| `macmbx_is_duplicate(toc, message, len)` | Extract Message-ID from raw RFC822 message, hash it, check TOC. Returns existing index or -1. |
| `macmbx_append_unique(toc, msg, len, sender, state, priority)` | Append only if not duplicate. Returns new index (>= 0) or negative of existing index if duplicate. |

## Return Values for append_unique

| Return | Meaning |
|--------|---------|
| `>= 0` | New message appended at this index |
| `< 0` | Duplicate found. Existing index = `-(return + 1)` |
| `-1` | Error (NULL toc, NULL message, etc.) |

Example: if return is `-4`, the duplicate is at index 3.

## Example: POP3 Download with Dedup

```c
MacmbxTOC *inbox = macmbx_toc_open("/path/to/In");

/* Download messages from POP3 */
for (int i = 0; i < pop_count; i++) {
  char *msg = pop3_download(session, i, &len);

  int result = macmbx_append_unique(inbox, msg, len, NULL, MACMBX_UNREAD, 3);
  if (result >= 0) {
    printf("New message #%d\n", result);
  } else if (result < -1) {
    printf("Duplicate of existing #%d, skipped\n", -(result + 1));
  }
  free(msg);
}

macmbx_toc_save(inbox);
```

## Example: Check Before Append

```c
/* Manual check */
int existing = macmbx_is_duplicate(inbox, raw_message, msg_len);
if (existing >= 0) {
  printf("Already have this message at index %d\n", existing);
} else {
  macmbx_append_message(inbox, raw_message, msg_len, sender, MACMBX_UNREAD, 3);
}
```

## Example: Import with Dedup

```c
/* Import from another mbox, skipping duplicates */
int imported = macmbx_import_mbox(inbox, "/path/to/old.mbox", true);
printf("Imported %d new messages (duplicates skipped)\n", imported);
```

## Hash Collisions

The djb2 hash is 32 bits, so collisions are possible (roughly 1 in 4 billion). A collision means a unique message is incorrectly identified as a duplicate. For practical mailbox sizes (< 100K messages), this is negligible.

If zero-collision dedup is needed, do a full Message-ID string comparison after the hash match:

```c
int existing = macmbx_find_duplicate(toc, hash);
if (existing >= 0) {
  /* Verify with full string comparison */
  char *existing_id = macmbx_read_header_field(toc, existing, "Message-ID");
  if (existing_id && strcmp(existing_id, new_msg_id) == 0) {
    /* Confirmed duplicate */
  } else {
    /* Hash collision -- not actually a duplicate */
  }
  free(existing_id);
}
```

## Notes

- Deleted messages are excluded from duplicate checks
- Messages without a Message-ID header have hash = 0 and are never considered duplicates
- The hash is computed during TOC build and stored in `MacmbxMsgSum.msg_id_hash`
- Also stored on disk in `MacmbxDiskSum.msgIdHash` for persistence
