# macmbx Library Overview

Standalone, portable Eudora mbox + TOC mailbox storage library.

## Architecture

```
MacmbxStore (mailbox directory manager)
  |
  +-- MacmbxNode tree (folders + mailboxes)
  |     |
  |     +-- MacmbxNode (folder: "Emails")
  |     |     +-- MacmbxNode (mailbox: "Work")
  |     |     +-- MacmbxNode (mailbox: "Personal")
  |     |
  |     +-- MacmbxNode (mailbox: "In")
  |     +-- MacmbxNode (mailbox: "Out")
  |     +-- MacmbxNode (mailbox: "Trash")
  |
  +-- MacmbxTOC per open mailbox
        |
        +-- MacmbxMsgSum[] (message summaries)
```

## Files

| File | Purpose |
|------|---------|
| `macmbx.h` | Public API header |
| `macmbx.c` | TOC I/O, message access, events, registry, locking |
| `macmbx_build.c` | Build TOC from mbox scan |
| `macmbx_compact.c` | Compact mailbox (remove deleted) |
| `macmbx_ops.c` | Transfer, create, remove, search, sort |
| `macmbx_store.c` | Directory hierarchy manager |
| `macmbx_thread.c` | Threading, dedup, import/export |
| `macmbx_filter.c` | Filter rule engine |
| `macmbx_junk.c` | Junk mail management |
| `macmbx_nicknames.c` | Nickname/address book management |
| `macmbx_sigstat.c` | Signatures and stationery |

## Binary Compatibility

On-disk TOC format is binary-compatible with Eudora:
- `MacmbxDiskHeader`: 76 bytes (matches `TOCDiskHeader`)
- `MacmbxDiskSum`: 224 bytes (matches `MSumDisk`)

Existing Eudora .toc files can be read and written without conversion.

## Dependencies

None. Pure C99 + POSIX. No GLib, no GTK, no Eudora headers.

## Portability

- POSIX: Linux, macOS, FreeBSD
- Windows: `_WIN32` guards for `FindFirstFile`, `_mkdir`, `WSAPoll`

## Documentation

- [Store Operations](store.md) - Directory and hierarchy management
- [Mailbox Operations](mailbox.md) - Create, open, close, delete, rename
- [TOC Operations](toc.md) - TOC lifecycle, validation, repair, peek
- [Message Operations](message.md) - Read, append, delete, state, flags
- [Transfer & Compact](transfer.md) - Move/copy messages, compact
- [Search & Sort](search_sort.md) - Search by field, sort by column
- [Locking](locking.md) - File locking for concurrent access
- [Events](events.md) - Optional event notification system
- [Threading](threading.md) - Build conversation thread trees
- [Dedup](dedup.md) - Message deduplication by Message-ID
- [Import/Export](import_export.md) - EML, maildir, mbox import/export
- [Filters](filters.md) - Rule-based message routing
- [Junk](junk.md) - Spam scoring, marking, archiving with pluggable classifier
- [Nicknames](nicknames.md) - Address book management, whitelist integration
- [Signatures](signatures.md) - Signature block management
- [Stationery](stationery.md) - Message template management
- [Disk Format](disk_format.md) - Binary TOC format specification
