# Events

Optional event notification system. Register callbacks to get notified when mailbox state changes.

## Design

- **Optional** -- zero callbacks registered by default. All operations work without events.
- **Independent** -- no Eudora types. Pure macmbx types only.
- **Multiple handlers** -- up to 16 registered simultaneously (UI + logging + sync, etc.).
- **Handle-based** -- register returns a handle, use it to unregister later.

## Registration

| Function | Description |
|----------|-------------|
| `macmbx_on(fn, ctx)` | Register an event handler. Returns handle (>= 0) or -1 on error. |
| `macmbx_off(handle)` | Unregister by handle. |
| `macmbx_off_all()` | Unregister all handlers. |

```c
typedef void (*MacmbxEventFn)(const MacmbxEvent *event, void *ctx);
```

The `ctx` pointer is passed through to every callback invocation -- use it for your application state.

## Event Types

### Message Events

| Event | Fired by | Payload |
|-------|----------|---------|
| `MACMBX_EVENT_NEW_MAIL` | `macmbx_append_message()` | toc, index, count |
| `MACMBX_EVENT_DELETED` | `macmbx_delete_message()` | toc, index |
| `MACMBX_EVENT_UNDELETED` | `macmbx_undelete_message()` | toc, index |
| `MACMBX_EVENT_STATE_CHANGED` | `macmbx_set_state()` | toc, index |
| `MACMBX_EVENT_FLAGS_CHANGED` | `macmbx_set_flags()`, `macmbx_clear_flags()` | toc, index |

### TOC Events

| Event | Fired by | Payload |
|-------|----------|---------|
| `MACMBX_EVENT_COMPACTED` | `macmbx_compact()` | toc |
| `MACMBX_EVENT_TOC_SAVED` | `macmbx_toc_save()` | toc |
| `MACMBX_EVENT_TOC_REBUILT` | `macmbx_toc_rebuild()` | toc |

### Store Events

| Event | Fired by | Payload |
|-------|----------|---------|
| `MACMBX_EVENT_MAILBOX_CREATED` | `macmbx_store_create_mailbox()` | path |
| `MACMBX_EVENT_MAILBOX_DELETED` | `macmbx_store_delete()` | path |
| `MACMBX_EVENT_MAILBOX_RENAMED` | `macmbx_store_rename()` | old_path, path |
| `MACMBX_EVENT_MAILBOX_MOVED` | `macmbx_store_move()` | old_path, path |
| `MACMBX_EVENT_FOLDER_CREATED` | `macmbx_store_create_folder()` | path |
| `MACMBX_EVENT_FOLDER_DELETED` | `macmbx_store_delete()` (folder) | path |

### Status Events

| Event | Fired by | Payload |
|-------|----------|---------|
| `MACMBX_EVENT_ERROR` | Any error condition | message |
| `MACMBX_EVENT_STATUS` | Informational | message |

## Event Data

```c
typedef struct {
  MacmbxEventType type;     /* which event */
  MacmbxTOC *toc;           /* which mailbox (NULL for store events) */
  int index;                /* message index (-1 if N/A) */
  int count;                /* count (e.g. messages appended) */
  const char *path;         /* path (store events, new path for rename) */
  const char *old_path;     /* old path (rename/move only) */
  const char *message;      /* text (error/status only) */
} MacmbxEvent;
```

## Example: UI Refresh

```c
void on_mailbox_event(const MacmbxEvent *ev, void *ctx) {
  GtkWidget *window = (GtkWidget *)ctx;

  switch (ev->type) {
  case MACMBX_EVENT_NEW_MAIL:
    /* Refresh message list */
    refresh_message_list(window, ev->toc);
    /* Update unread badge */
    update_unread_count(window, ev->toc);
    break;

  case MACMBX_EVENT_DELETED:
  case MACMBX_EVENT_UNDELETED:
  case MACMBX_EVENT_STATE_CHANGED:
    /* Refresh the changed row */
    refresh_message_row(window, ev->toc, ev->index);
    break;

  case MACMBX_EVENT_COMPACTED:
    /* Full refresh -- indices shifted */
    refresh_message_list(window, ev->toc);
    break;

  case MACMBX_EVENT_MAILBOX_CREATED:
  case MACMBX_EVENT_MAILBOX_DELETED:
  case MACMBX_EVENT_FOLDER_CREATED:
  case MACMBX_EVENT_FOLDER_DELETED:
    /* Refresh mailbox tree sidebar */
    refresh_mailbox_tree(window);
    break;

  default:
    break;
  }
}

/* Register */
int handle = macmbx_on(on_mailbox_event, main_window);

/* ... use macmbx normally ... */

/* Unregister on shutdown */
macmbx_off(handle);
```

## Example: Logging

```c
void log_event(const MacmbxEvent *ev, void *ctx) {
  FILE *log = (FILE *)ctx;
  switch (ev->type) {
  case MACMBX_EVENT_NEW_MAIL:
    fprintf(log, "NEW: %s #%d\n", ev->toc->mbox_path, ev->index);
    break;
  case MACMBX_EVENT_DELETED:
    fprintf(log, "DEL: %s #%d\n", ev->toc->mbox_path, ev->index);
    break;
  case MACMBX_EVENT_MAILBOX_CREATED:
    fprintf(log, "CREATED: %s\n", ev->path);
    break;
  case MACMBX_EVENT_ERROR:
    fprintf(log, "ERROR: %s\n", ev->message);
    break;
  default:
    break;
  }
}

FILE *logfile = fopen("macmbx.log", "a");
macmbx_on(log_event, logfile);
```

## Thread Safety

Event handlers are called synchronously on the thread that triggers the event. If your handler needs to touch UI (GTK, etc.), use `g_idle_add()` or equivalent to dispatch to the main thread.
