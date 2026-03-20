# Mailbox Operations

Direct mailbox file operations (create, remove, rename) without going through the store.

## File Operations

| Function | Description |
|----------|-------------|
| `macmbx_create(path)` | Create empty mbox file + empty .toc. Returns 0 on success. |
| `macmbx_remove(path)` | Delete mbox file + .toc file. Returns 0 on success. |
| `macmbx_rename(old, new)` | Rename both mbox and .toc files atomically. |
| `macmbx_is_mbox(path)` | Check if path is a valid mbox (exists, starts with "From "). |
| `macmbx_list_mailboxes(dir, &names)` | List mbox files in a directory. Returns count. |

## Special Mailbox Detection

| Function | Description |
|----------|-------------|
| `macmbx_detect_type(path)` | Detect type from filename: In/Out/Trash/Junk/In.temp/Out.temp/Normal. |

```c
typedef enum {
  MACMBX_TYPE_NORMAL   = 0,
  MACMBX_TYPE_IN       = 1,
  MACMBX_TYPE_OUT      = 2,
  MACMBX_TYPE_TRASH    = 3,
  MACMBX_TYPE_JUNK     = 4,
  MACMBX_TYPE_IN_TEMP  = 11,
  MACMBX_TYPE_OUT_TEMP = 12,
} MacmbxType;
```

## Mbox "From " Line

| Function | Description |
|----------|-------------|
| `macmbx_is_from_line(line)` | Validate a "From " separator line (checks date format). |
| `macmbx_write_from_line(buf, sz, sender)` | Write a "From " separator with current timestamp. |

The "From " line format: `From sender@host Wed Jun 14 12:36:18 2023\n`

## Attachment Detection

| Function | Description |
|----------|-------------|
| `macmbx_detect_attachment(headers)` | Returns true if headers indicate attachments. |

Checks for:
- `Content-Type: multipart/mixed`
- `Content-Disposition: attachment`
- Eudora's `Attachment:` header

## Example

```c
/* Create and populate a mailbox */
macmbx_create("/path/to/MyBox");
MacmbxTOC *toc = macmbx_toc_open("/path/to/MyBox");
macmbx_append_message(toc, msg, len, "sender@host", MACMBX_UNREAD, 3);
macmbx_toc_save(toc);
macmbx_toc_close(toc);

/* Rename it */
macmbx_rename("/path/to/MyBox", "/path/to/BetterName");
```
