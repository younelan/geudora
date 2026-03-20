# Search & Sort

Search within mailboxes and sort message lists.

## Search

```c
int macmbx_search(toc, field, pattern, &results);
```

Returns count of matching message indices. Allocates `*results` array (caller frees with `free()`).

### Search Fields

| Field | What it searches | Speed |
|-------|-----------------|-------|
| `"from"` | Summary `from` field | Fast (in-memory) |
| `"subject"` | Summary `subject` field | Fast (in-memory) |
| `"all"` | From + subject, then full message text | Slow for body |
| `"body"` | Message body only | Slow (reads mbox) |
| `"header:X-Mailer"` | Specific header value | Medium (reads headers) |

All searches are case-insensitive substring matches. Deleted messages are skipped.

### Example

```c
int *results;
int count = macmbx_search(toc, "from", "john", &results);
printf("Found %d messages from john:\n", count);
for (int i = 0; i < count; i++)
  printf("  #%d: %s\n", results[i], toc->msgs[results[i]].subject);
free(results);

/* Search body for a phrase */
count = macmbx_search(toc, "body", "meeting tomorrow", &results);
free(results);

/* Search a custom header */
count = macmbx_search(toc, "header:List-Id", "dev-team", &results);
free(results);
```

## Sort

```c
int macmbx_sort(toc, field, ascending);
```

Sorts the TOC summaries in place. Marks TOC dirty. Returns 0 on success.

### Sort Fields

| Field | What it sorts by | Notes |
|-------|-----------------|-------|
| `"date"` | `seconds` (UTC timestamp) | Most common |
| `"from"` | `from` string | Case-insensitive |
| `"subject"` | `subject` string | Strips Re:/Fwd: prefixes |
| `"size"` | `length` (bytes) | |
| `"state"` | `state` enum value | Unread first |
| `"priority"` | `priority` (1-5) | 1=highest |
| `"label"` | Label color (0-7) | From flags bits 12-14 |

### Example

```c
/* Sort by date, newest first */
macmbx_sort(toc, "date", false);

/* Sort by sender, A-Z */
macmbx_sort(toc, "from", true);

/* Sort by subject (strips Re:) */
macmbx_sort(toc, "subject", true);

/* Sort by size, largest first */
macmbx_sort(toc, "size", false);
```

### Subject Sort

When sorting by subject, `Re:` and `Fwd:` prefixes are stripped so replies sort with their original thread:

- "Re: Meeting notes" sorts as "Meeting notes"
- "Re: Re: Re: Hello" sorts as "Hello"
- "Fwd: Budget report" sorts as "Budget report"
