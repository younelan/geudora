# Transfer & Compact

Move/copy messages between mailboxes and compact to reclaim space.

## Transfer

| Function | Description |
|----------|-------------|
| `macmbx_transfer(src, index, dst, copy)` | Transfer one message. copy=false marks source deleted. Returns new index in dst. |
| `macmbx_transfer_multi(src, indices, count, dst, copy)` | Transfer multiple messages. indices must be sorted ascending. Returns count transferred. |

Transfer preserves all metadata: state, priority, flags, dates, personality IDs, from, subject, spam score, hashes.

When `copy=false` (move):
1. Message is appended to destination
2. Source is marked deleted (not removed)
3. Call `macmbx_compact(src)` later to reclaim space

When `copy=true`:
1. Message is appended to destination
2. Source is unchanged

Events fired:
- `NEW_MAIL` on destination (from append)
- `DELETED` on source (if move)

## Compact

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_compact(toc)` | Rewrite mbox without deleted messages. Updates offsets. Saves TOC. | `COMPACTED` |
| `macmbx_count_deleted(toc)` | Count messages with DELETED flag. | |
| `macmbx_reclaimable(toc)` | Bytes that would be freed by compaction. | |

### Compact Process

1. Open source mbox for reading
2. Create temp file `mbox_path.compact.tmp`
3. Copy each non-deleted message to temp, tracking new offsets
4. Atomic rename temp over original (POSIX `rename()`)
5. If rename fails, fall back to copy approach
6. Update TOC: new offsets, remove deleted entries
7. Save TOC to disk
8. Fire `COMPACTED` event

The compact is atomic -- if anything fails, the original mbox is untouched.

### When to Compact

- After bulk deletes (e.g. emptying Trash)
- When `macmbx_reclaimable()` exceeds a threshold
- On `macmbx_store_compact_all()` for batch cleanup

## Example

```c
MacmbxTOC *inbox = macmbx_toc_open("/path/to/In");
MacmbxTOC *archive = macmbx_toc_open("/path/to/Archive");

/* Move message 5 to archive */
int new_idx = macmbx_transfer(inbox, 5, archive, false);
printf("Archived as index %d\n", new_idx);

/* Move multiple */
int indices[] = {0, 2, 7};
int moved = macmbx_transfer_multi(inbox, indices, 3, archive, false);
printf("Moved %d messages\n", moved);

/* Check reclaimable space */
long bytes = macmbx_reclaimable(inbox);
printf("%ld bytes reclaimable (%d deleted)\n", bytes, macmbx_count_deleted(inbox));

/* Compact */
macmbx_compact(inbox);

/* Save */
macmbx_toc_save(inbox);
macmbx_toc_save(archive);
```
