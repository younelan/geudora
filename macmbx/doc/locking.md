# Locking

File-based locking for concurrent access.

## Mailbox-Level Locking

| Function | Description |
|----------|-------------|
| `macmbx_lock(toc)` | Lock a single mailbox for exclusive access. |
| `macmbx_unlock(toc)` | Release the mailbox lock. |

Creates a `.lock` file next to the mbox (e.g. `In.lock`). Uses `flock(LOCK_EX|LOCK_NB)` on POSIX for non-blocking exclusive lock.

If the lock is already held by another process, `macmbx_lock()` returns -1 immediately (non-blocking).

Lock is automatically released on `macmbx_toc_close()`.

## Store-Level Locking

| Function | Description |
|----------|-------------|
| `macmbx_store_lock(store)` | Lock the entire mailbox directory. |
| `macmbx_store_unlock(store)` | Release the store lock. |

Creates `.store.lock` in the base directory. Used for operations that affect the directory structure (create, delete, rename, move).

Lock is automatically released on `macmbx_store_close()`.

## Reentrant Write Protection

`MacmbxTOC.being_written` prevents reentrant calls to `macmbx_toc_save()` (e.g. if a signal handler or callback triggers a save during a save).

## Implementation

### POSIX (Linux, macOS)
```
flock(fd, LOCK_EX | LOCK_NB)  /* acquire */
flock(fd, LOCK_UN)             /* release */
```

### Windows
Not yet implemented -- placeholder returns 0 (always succeeds).

## When to Lock

- **Always lock** before `macmbx_compact()` -- it rewrites the mbox file
- **Lock recommended** before batch `macmbx_append_message()` calls
- **Store lock** before `macmbx_store_create_*()`, `macmbx_store_delete()`, `macmbx_store_rename()`, `macmbx_store_move()`
- **Not needed** for `macmbx_read_*()` calls (read-only, no file modification)

## Example

```c
MacmbxTOC *toc = macmbx_toc_open("/path/to/In");

if (macmbx_lock(toc) == 0) {
  /* Exclusive access */
  macmbx_append_message(toc, msg, len, sender, MACMBX_UNREAD, 3);
  macmbx_toc_save(toc);
  macmbx_unlock(toc);
} else {
  /* Another process has the lock */
  fprintf(stderr, "Mailbox is locked by another process\n");
}
```
