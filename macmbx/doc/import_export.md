# Import / Export

Import messages from EML files, maildir, and mbox. Export messages as EML files.

## Export

### Single Message

```c
int macmbx_export_eml(toc, index, eml_path);
```

Exports one message as an .eml file. Strips the mbox "From " separator line. Returns 0 on success.

### Batch Export

```c
int macmbx_export_eml_multi(toc, indices, count, dir_path);
```

Exports multiple messages to a directory. Files named by serial number: `00001.eml`, `00042.eml`, etc. Returns count exported.

### Full Mailbox Export

```c
int macmbx_export_all_eml(toc, dir_path);
```

Exports all non-deleted messages. Creates directory if needed. Returns count exported.

### Example

```c
MacmbxTOC *toc = macmbx_toc_open("/path/to/In");

/* Export single message */
macmbx_export_eml(toc, 0, "/tmp/first_message.eml");

/* Export selected messages */
int selected[] = {0, 3, 7, 12};
macmbx_export_eml_multi(toc, selected, 4, "/tmp/exported");

/* Export entire mailbox */
int count = macmbx_export_all_eml(toc, "/tmp/inbox_backup");
printf("Exported %d messages\n", count);
```

## Import

### From EML File

```c
int macmbx_import_eml(toc, eml_path);
```

Imports a single .eml file (standard RFC822 message). Extracts sender from From: header. Returns new message index.

### From EML Directory

```c
int macmbx_import_eml_dir(toc, dir_path);
```

Imports all `*.eml` files from a directory. Returns count imported.

### From Maildir

```c
int macmbx_import_maildir(toc, maildir_path);
```

Imports from Maildir format:
- Reads `new/` (unread) and `cur/` (read) subdirectories
- Parses Maildir flags from filename (`:2,S` = Seen)
- Sets message state accordingly (UNREAD or READ)
- Returns count imported

Maildir structure:
```
maildir/
  new/          <- unread messages
    1234567.abc.host
    1234568.def.host
  cur/          <- read messages
    1234560.xyz.host:2,S    <- Seen flag
    1234561.uvw.host:2,RS   <- Replied + Seen
  tmp/          <- ignored (temporary)
```

### From Another Mbox

```c
int macmbx_import_mbox(toc, mbox_path, dedup);
```

Imports all messages from another Unix mbox file:
- Builds a temporary TOC from the source mbox
- Reads each message and appends to destination
- If `dedup=true`, skips messages with duplicate Message-IDs
- Preserves state, priority, and sender from source
- Returns count imported

### Examples

```c
MacmbxTOC *inbox = macmbx_toc_open("/path/to/In");

/* Import single EML */
int idx = macmbx_import_eml(inbox, "/tmp/important.eml");
printf("Imported as message #%d\n", idx);

/* Import all EMLs from a directory */
int count = macmbx_import_eml_dir(inbox, "/tmp/exported_emails");
printf("Imported %d messages\n", count);

/* Import from Thunderbird maildir */
count = macmbx_import_maildir(inbox, "/home/user/.thunderbird/xxx/ImapMail/server/INBOX");
printf("Imported %d from maildir\n", count);

/* Import from old mbox, skip duplicates */
count = macmbx_import_mbox(inbox, "/var/mail/user", true);
printf("Imported %d new messages from mbox\n", count);

macmbx_toc_save(inbox);
```

## Format Summary

| Source | Function | Dedup | State Handling |
|--------|----------|-------|----------------|
| Single .eml | `macmbx_import_eml()` | No | Always UNREAD |
| Directory of .eml | `macmbx_import_eml_dir()` | No | Always UNREAD |
| Maildir | `macmbx_import_maildir()` | No | From filename flags (`:2,S`) |
| Mbox file | `macmbx_import_mbox()` | Optional | From source TOC |

## Round-Trip

Export and re-import produces identical messages:

```c
/* Export */
macmbx_export_all_eml(inbox, "/tmp/backup");

/* Create new mailbox and re-import */
macmbx_create("/path/to/Restored");
MacmbxTOC *restored = macmbx_toc_open("/path/to/Restored");
macmbx_import_eml_dir(restored, "/tmp/backup");
macmbx_toc_save(restored);
```

Note: metadata (state, priority, flags, labels) is NOT preserved in .eml export/import since .eml is standard RFC822 with no Eudora extensions. Use `macmbx_transfer()` for metadata-preserving moves between macmbx mailboxes.
