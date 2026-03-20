# Store Operations

The `MacmbxStore` manages a hierarchy of mailboxes and folders rooted at a base directory.

## Types

```c
typedef enum {
  MACMBX_NODE_MAILBOX = 0,  /* mbox file */
  MACMBX_NODE_FOLDER  = 1,  /* directory containing mailboxes/folders */
} MacmbxNodeType;

typedef struct MacmbxNode {
  char name[256];           /* display name */
  char path[PATH_MAX];      /* full filesystem path */
  MacmbxNodeType type;
  MacmbxType mbox_type;     /* In/Out/Trash/Junk/Normal */
  int unread;               /* cached count, -1 = unknown */
  int total;                /* cached count, -1 = unknown */
  MacmbxNode *children;     /* first child (folders) */
  MacmbxNode *next;         /* next sibling */
  MacmbxNode *parent;       /* parent folder */
} MacmbxNode;

typedef struct {
  char base_path[PATH_MAX];
  MacmbxNode *root;         /* tree of nodes */
  int lock_fd;
} MacmbxStore;
```

## Lifecycle

| Function | Description |
|----------|-------------|
| `macmbx_store_open(path)` | Open store, scan directory tree, create dir if needed. Returns `MacmbxStore *`. |
| `macmbx_store_close(store)` | Flush all dirty TOCs, release locks, free tree. |
| `macmbx_store_refresh(store)` | Rescan directory tree after external changes. Returns 0 on success. |

## Navigation

| Function | Description |
|----------|-------------|
| `macmbx_store_root(store)` | Get first top-level node (In, Out, folders...). Walk with `->next`. |
| `macmbx_store_find(store, "Emails/Work")` | Find node by relative path. Returns NULL if not found. |
| `macmbx_store_find_by_name(store, "Work")` | Recursive name search. Returns first match. |
| `macmbx_store_find_special(store, MACMBX_TYPE_IN)` | Find In/Out/Trash/Junk mailbox. |
| `macmbx_store_count_mailboxes(store)` | Count all mailboxes (recursive). |
| `macmbx_store_count_folders(store)` | Count all folders (recursive). |

## Folder Operations

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_store_create_folder(store, parent, name)` | Create directory. parent=NULL for top level. Returns new node. | `FOLDER_CREATED` |
| `macmbx_store_delete(store, rel_path)` | Delete empty folder. Fails if has children. | `FOLDER_DELETED` |
| `macmbx_store_rename(store, rel_path, new_name)` | Rename folder. | `MAILBOX_RENAMED` |
| `macmbx_store_move(store, rel_path, new_parent)` | Move folder to new parent. | `MAILBOX_MOVED` |

## Mailbox Operations (through store)

| Function | Description | Event |
|----------|-------------|-------|
| `macmbx_store_create_mailbox(store, parent, name)` | Create mbox + toc. Returns new node. | `MAILBOX_CREATED` |
| `macmbx_store_delete(store, rel_path)` | Delete mailbox + toc. | `MAILBOX_DELETED` |
| `macmbx_store_rename(store, rel_path, new_name)` | Rename mailbox + toc. | `MAILBOX_RENAMED` |
| `macmbx_store_move(store, rel_path, new_parent)` | Move mailbox to new parent. | `MAILBOX_MOVED` |
| `macmbx_store_open_mailbox(store, rel_path)` | Open TOC (registered). Returns `MacmbxTOC *`. | |

## Batch Operations

| Function | Description |
|----------|-------------|
| `macmbx_store_flush(store)` | Save all dirty TOCs. Returns count saved. |
| `macmbx_store_compact_all(store)` | Compact all mailboxes with deleted messages. Returns count. |
| `macmbx_store_update_counts(store)` | Refresh unread/total for all nodes. Uses peek for unopened. |

## Enumeration

| Function | Description |
|----------|-------------|
| `macmbx_store_list_mailboxes(store, &paths)` | Flat list of all mailbox relative paths. Caller frees. |
| `macmbx_store_list_folders(store, &paths)` | Flat list of all folder relative paths. Caller frees. |

## Locking

| Function | Description |
|----------|-------------|
| `macmbx_store_lock(store)` | Exclusive `.store.lock` for the entire directory. |
| `macmbx_store_unlock(store)` | Release store lock. |

## Node Sort Order

Nodes are sorted within each parent:
1. Special mailboxes first (In, Out, Trash, Junk)
2. Folders next (alphabetical)
3. Regular mailboxes last (alphabetical)

## Example

```c
MacmbxStore *store = macmbx_store_open("/home/user/.eudora/mailboxes");

/* List all mailboxes */
for (MacmbxNode *n = macmbx_store_root(store); n; n = n->next) {
  printf("%s %s\n", n->type == MACMBX_NODE_FOLDER ? "[folder]" : "[mbox]", n->name);
}

/* Create folder + mailbox */
macmbx_store_create_folder(store, NULL, "Projects");
macmbx_store_create_mailbox(store, "Projects", "Alpha");

/* Open and use a mailbox */
MacmbxTOC *toc = macmbx_store_open_mailbox(store, "Projects/Alpha");
macmbx_append_message(toc, msg, msgLen, "user@host", MACMBX_UNREAD, 3);

/* Flush and close */
macmbx_store_close(store);
```
