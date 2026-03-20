# Message Operations

Read, append, delete, and modify messages within a mailbox.

## Reading Messages

| Function | Description |
|----------|-------------|
| `macmbx_read_message(toc, index, &len)` | Read full message (including From line). Returns malloc'd buffer. |
| `macmbx_read_headers(toc, index)` | Read headers only (up to body_offset). Returns malloc'd string. |
| `macmbx_read_body(toc, index, &len)` | Read body only (after blank line). Returns malloc'd buffer. |
| `macmbx_read_header_field(toc, index, "Subject")` | Read a specific header value. Handles continuation lines. Returns malloc'd string or NULL. |

All return values are caller-freed with `free()`.

## Appending Messages

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_append_message(toc, msg, len, sender, state, priority)` | Append to mbox file with "From " separator. Returns new index. | `NEW_MAIL` |

Parameters:
- `msg`: raw RFC822 message (headers + body)
- `len`: message length (-1 for strlen)
- `sender`: envelope sender for "From " line (NULL = "unknown")
- `state`: initial state (MACMBX_UNREAD, MACMBX_SENT, etc.)
- `priority`: 1-5 (0 = default to 3/normal)

The function automatically:
- Writes a "From " separator with current timestamp
- Extracts From/Subject from headers for the summary
- Detects attachments from Content-Type
- Sets the `MACMBX_FLAG_UTF8` flag
- Assigns a unique serial number

## Deleting Messages

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_delete_message(toc, index)` | Mark deleted (flag only). Message stays in mbox until compact. | `DELETED` |
| `macmbx_undelete_message(toc, index)` | Clear deleted flag. | `UNDELETED` |

Deletion is a two-phase process:
1. `macmbx_delete_message()` sets `MACMBX_FLAG_DELETED` and `MACMBX_OPT_DELETED`
2. `macmbx_compact()` rewrites the mbox without deleted messages

## State Changes

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_set_state(toc, index, state)` | Set message state. | `STATE_CHANGED` |

States:
| Value | Constant | Meaning |
|-------|----------|---------|
| 0 | `MACMBX_UNREAD` | New, not yet read |
| 1 | `MACMBX_READ` | Read |
| 2 | `MACMBX_REPLIED` | Replied to |
| 3 | `MACMBX_FORWARDED` | Forwarded |
| 4 | `MACMBX_REDIRECTED` | Redirected |
| 5 | `MACMBX_QUEUED` | Queued for sending |
| 6 | `MACMBX_SENT` | Sent |
| 7 | `MACMBX_UNSENT` | Draft, not sent |
| 8 | `MACMBX_TIMED` | Timed send |
| 9 | `MACMBX_SENDABLE` | Ready to send |
| 10 | `MACMBX_REBUILT` | Rebuilt from scan |

## Flag Operations

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_set_flags(toc, index, flags)` | Set (OR) flag bits. | `FLAGS_CHANGED` |
| `macmbx_clear_flags(toc, index, flags)` | Clear (AND NOT) flag bits. | `FLAGS_CHANGED` |
| `macmbx_set_priority(toc, index, pri)` | Set priority 1-5. | `FLAGS_CHANGED` |
| `macmbx_set_label(toc, index, label)` | Set label 0-7 (color). | `FLAGS_CHANGED` |
| `macmbx_get_label(toc, index)` | Get current label. | |

Flags:
| Flag | Bit | Meaning |
|------|-----|---------|
| `MACMBX_FLAG_DELETED` | 0 | Marked for deletion |
| `MACMBX_FLAG_READ` | 1 | Has been read |
| `MACMBX_FLAG_ANSWERED` | 2 | Has been replied to |
| `MACMBX_FLAG_FLAGGED` | 3 | User-flagged |
| `MACMBX_FLAG_DRAFT` | 4 | Is a draft |
| `MACMBX_FLAG_ATTACHMENT` | 5 | Has attachments |
| `MACMBX_FLAG_HTML` | 6 | Contains HTML |
| `MACMBX_FLAG_BULK` | 7 | Bulk/list mail |
| `MACMBX_FLAG_UTF8` | 8 | Content is UTF-8 |
| `MACMBX_FLAG_KEEP_COPY` | 9 | Keep copy on send |
| `MACMBX_FLAG_RR` | 10 | Return receipt requested |
| `MACMBX_FLAG_LABEL_MASK` | 12-14 | Label color (3 bits, 0-7) |

## Message Summary Fields

```c
typedef struct {
  long offset;              /* byte offset in mbox */
  long length;              /* total bytes */
  int body_offset;          /* offset to body from msg start */
  uint8_t state;            /* MacmbxState */
  uint8_t priority;         /* 1=highest, 3=normal, 5=lowest */
  uint8_t origPriority;     /* original priority */
  int8_t spam_score;        /* -1=unscored, 0-100 */
  uint32_t flags;           /* MACMBX_FLAG_* */
  uint32_t opts;            /* MACMBX_OPT_* */
  uint32_t seconds;         /* Date: as UTC seconds */
  uint32_t arrival;         /* arrival time */
  uint32_t from_hash;       /* hash of From address */
  int32_t msg_id_hash;      /* hash of Message-ID */
  uint32_t uid_hash;        /* hash for UIDL tracking */
  long serial_num;          /* unique within mailbox */
  int16_t orig_zone;        /* timezone in minutes */
  uint32_t pop_pers_id;     /* personality that received */
  uint32_t pers_id;         /* personality that sent */
  char from[48];            /* display sender */
  char subject[60];         /* subject line */
  bool has_attachment;       /* detected from headers */
} MacmbxMsgSum;
```

## Statistics

| Function | Description |
|----------|-------------|
| `macmbx_count_unread(toc)` | Count messages with state=UNREAD and not deleted. |
| `macmbx_count_flagged(toc)` | Count messages with FLAGGED flag and not deleted. |
| `macmbx_total_size(toc)` | Total bytes of all messages. |
| `macmbx_count_deleted(toc)` | Count messages with DELETED flag. |

## Example

```c
MacmbxTOC *toc = macmbx_toc_open("/path/to/In");

/* Read a message */
long len;
char *msg = macmbx_read_message(toc, 0, &len);
char *subj = macmbx_read_header_field(toc, 0, "Subject");
printf("Subject: %s\n", subj);
free(subj);
free(msg);

/* Mark as read */
macmbx_set_state(toc, 0, MACMBX_READ);

/* Flag it */
macmbx_set_flags(toc, 0, MACMBX_FLAG_FLAGGED);

/* Set red label */
macmbx_set_label(toc, 0, 1);

/* Delete */
macmbx_delete_message(toc, 0);

/* Save changes */
macmbx_toc_save(toc);
```
