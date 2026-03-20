# TOC Operations

Table of Contents lifecycle: open, save, close, build, rebuild, validate, repair, peek.

## Lifecycle

| Function | Description |
|----------|-------------|
| `macmbx_toc_open(mbox_path)` | Load .toc if valid, else build from mbox scan. Checks registry first. Returns `MacmbxTOC *`. |
| `macmbx_toc_save(toc)` | Write TOC to disk (atomic via temp+rename). Returns 0 on success. Fires `TOC_SAVED`. |
| `macmbx_toc_close(toc)` | Free TOC. Does NOT save -- call `macmbx_toc_save()` first if dirty. Removes from registry. |
| `macmbx_toc_build(mbox_path)` | Full mbox scan: parse "From " lines, extract headers, build summaries. |
| `macmbx_toc_rebuild(mbox_path, old)` | Rebuild from mbox, salvage state/flags from old TOC by offset or msg_id_hash match. Fires `TOC_REBUILT`. |
| `macmbx_toc_valid(toc)` | Check if TOC still matches mbox (offsets within file bounds). |

## Peek

| Function | Description |
|----------|-------------|
| `macmbx_toc_peek(mbox_path, &count, &which, &box_size)` | Read just the 76-byte header without loading summaries. Fast for unread counts. |

## Validation

| Function | Returns | Description |
|----------|---------|-------------|
| `macmbx_toc_validate(toc)` | 0, `MACMBX_ERR_CORRUPT`, `MACMBX_ERR_MISMATCH`, `MACMBX_ERR_BAD_VERSION` | Check integrity: count, offsets, lengths, mbox size match. |

Error codes:
- `MACMBX_ERR_CORRUPT` (-1): Bad offsets, negative counts, out-of-bounds messages
- `MACMBX_ERR_MISMATCH` (-2): Mbox file size doesn't match what TOC expects
- `MACMBX_ERR_BAD_VERSION` (-3): TOC version newer than this library supports

## Repair

| Function | Description |
|----------|-------------|
| `macmbx_toc_repair(toc)` | Fix in place: truncate overlong strings, assign missing serial numbers, set missing arrival times, update version stamp. |

What repair fixes:
- From/subject strings exceeding buffer size
- Missing serial numbers (old TOC versions)
- Missing arrival timestamps (copies from date)
- Spam score initialized to -1 for old formats
- Version stamp updated to current

## Registry

| Function | Description |
|----------|-------------|
| `macmbx_registry_find(mbox_path)` | Find already-open TOC. Returns NULL if not open. |
| `macmbx_registry_flush()` | Save all dirty TOCs. Returns count saved. |
| `macmbx_registry_close_all()` | Save + close all open TOCs. |

The registry prevents double-opening the same mailbox. `macmbx_toc_open()` checks the registry before loading from disk.

## TOC Fields

```c
typedef struct MacmbxTOC {
  char mbox_path[PATH_MAX];
  char toc_path[PATH_MAX];
  int count;                  /* number of messages */
  int capacity;               /* allocated slots */
  long next_serial;           /* next serial number */
  int16_t which;              /* MacmbxType */
  bool dirty;                 /* needs saving */
  bool being_written;         /* reentrant protection */
  int lock_fd;                /* file lock, -1 if unlocked */
  /* Disk header fields preserved for round-trip */
  int32_t major_version, minor_version;
  int32_t box_size, write_date;
  int32_t sort_order, last_sort;
  int32_t plugin_key, plugin_value;
  int32_t preview_hi, unread_base;
  int32_t sorts[6], needs_compact;
  MacmbxMsgSum *msgs;         /* array of summaries */
  MacmbxTOC *next;            /* registry linked list */
};
```

## Build Process

`macmbx_toc_build()` scans the mbox file line by line:

1. Find "From " separator lines (validated with date parsing)
2. Parse headers: Date, From, Sender, Reply-To, Subject, X-Priority, Importance, Message-ID, Status, Content-Type, Precedence
3. Beautify From field (extract display name or bare address)
4. Parse date to UTC seconds
5. Detect attachments from Content-Type
6. Hash Message-ID for dedup
7. Track body offset (blank line after headers)

## Salvage (Rebuild)

`macmbx_toc_rebuild()` matches old and new summaries:

1. Build fresh TOC from mbox scan
2. For each new message, binary-search old TOC by offset
3. If offset+length match: copy state, priority, flags, personality from old
4. If no offset match but msg_id_hash matches: copy state, priority, flags
5. Mark TOC dirty

## Example

```c
MacmbxTOC *toc = macmbx_toc_open("/path/to/In");
printf("%d messages, %d unread\n", toc->count, macmbx_count_unread(toc));

/* Validate */
int err = macmbx_toc_validate(toc);
if (err == MACMBX_ERR_MISMATCH) {
  MacmbxTOC *rebuilt = macmbx_toc_rebuild("/path/to/In", toc);
  macmbx_toc_close(toc);
  toc = rebuilt;
}

/* Peek without loading */
int count;
macmbx_toc_peek("/path/to/Out", &count, NULL, NULL);
printf("Out has %d messages\n", count);
```
