# Message Threading

Build conversation thread trees from In-Reply-To/References headers.

## How It Works

1. Each message summary stores `msg_id_hash` (hash of its Message-ID) and `in_reply_to_hash` (hash of its In-Reply-To header)
2. `macmbx_build_threads()` creates a hash table mapping msg_id_hash to thread nodes
3. For each message with an in_reply_to_hash, links it as a child of the matching parent
4. Collects all root nodes (messages with no parent) into a linked list

## Types

```c
typedef struct MacmbxThread {
  int index;              /* message index in TOC (-1 for dummy root) */
  MacmbxThread *parent;
  MacmbxThread *child;    /* first child */
  MacmbxThread *next;     /* next sibling */
  int depth;              /* nesting depth (0 = root) */
} MacmbxThread;
```

## Functions

| Function | Description |
|----------|-------------|
| `macmbx_build_threads(toc)` | Build thread tree. Returns linked list of root threads. Caller frees with `macmbx_threads_free()`. |
| `macmbx_thread_flatten(threads, &indices, &depths)` | Depth-first walk into flat arrays for display. Returns count. Caller frees both arrays. |
| `macmbx_thread_find(threads, index)` | Find the root thread containing a message. |
| `macmbx_thread_count(threads)` | Count top-level conversations. |
| `macmbx_threads_free(threads)` | Free entire thread tree. |

## Example: Threaded Message List

```c
MacmbxTOC *toc = macmbx_toc_open("/path/to/In");
MacmbxThread *threads = macmbx_build_threads(toc);

printf("%d conversations\n", macmbx_thread_count(threads));

/* Flat list for display */
int *indices, *depths;
int count = macmbx_thread_flatten(threads, &indices, &depths);

for (int i = 0; i < count; i++) {
  int idx = indices[i];
  int depth = depths[i];
  /* Indent by depth */
  for (int d = 0; d < depth; d++) printf("  ");
  printf("%s: %s\n", toc->msgs[idx].from, toc->msgs[idx].subject);
}

free(indices);
free(depths);
macmbx_threads_free(threads);
```

Output:
```
alice@example.com: Meeting tomorrow
  bob@example.com: Re: Meeting tomorrow
    alice@example.com: Re: Re: Meeting tomorrow
  carol@example.com: Re: Meeting tomorrow
dave@example.com: Budget report
  eve@example.com: Re: Budget report
```

## Example: Find a Message's Thread

```c
MacmbxThread *root = macmbx_thread_find(threads, 5);
if (root) {
  printf("Message 5 is in thread starting with: %s\n",
         toc->msgs[root->index].subject);
}
```

## Thread Tree Structure

```
MacmbxThread (root: "Meeting tomorrow", index=0)
  |-- child -> MacmbxThread ("Re: Meeting tomorrow", index=1)
  |              |-- child -> MacmbxThread ("Re: Re: Meeting tomorrow", index=3)
  |-- next sibling in child list -> MacmbxThread ("Re: Meeting tomorrow", index=2)
  |
next -> MacmbxThread (root: "Budget report", index=4)
  |-- child -> MacmbxThread ("Re: Budget report", index=5)
```

Walk roots with `->next`. Walk children with `->child` then `->next` for siblings.

## Notes

- Deleted messages are excluded from thread building
- Messages without In-Reply-To become root threads
- The `in_reply_to_hash` field is computed during TOC build from the In-Reply-To header
- This is in-memory only -- not stored in the .toc disk format (computed on load)
