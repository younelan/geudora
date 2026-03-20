# Signatures

Plain text signature blocks, stored as individual files in a Signatures/ directory.

## Storage

```
Signatures/
  Standard          ← default signature (always exists)
  Alternate         ← alternate signature (always exists)
  Work              ← custom signature
  Personal          ← custom signature
```

Each file is plain text — the signature content exactly as it will be appended to outgoing messages.

## Types

```c
typedef struct {
  char name[256];
  char path[PATH_MAX];
  char *content;        /* malloc'd, NULL if not loaded yet */
  bool dirty;
} MacmbxSignature;

typedef struct {
  char dir_path[PATH_MAX];
  MacmbxSignature *sigs;
  int count, capacity;
} MacmbxSignatures;
```

## Lifecycle

| Function | Description |
|----------|-------------|
| `macmbx_sig_open(dir)` | Open signatures directory. Creates Standard + Alternate if missing. |
| `macmbx_sig_close(sigs)` | Free all. Does NOT save. |
| `macmbx_sig_save(sigs)` | Save all dirty signatures (atomic via temp+rename). |

## CRUD

| Function | Description |
|----------|-------------|
| `macmbx_sig_add(sigs, "Work", "-- \nJohn Doe\nACME Corp")` | Add new signature. Returns index. |
| `macmbx_sig_remove(sigs, index)` | Delete signature file. |
| `macmbx_sig_rename(sigs, index, "Office")` | Rename signature file. |
| `macmbx_sig_find(sigs, "Work")` | Find by name. Returns index or -1. |
| `macmbx_sig_count(sigs)` | Number of signatures. |

## Access

| Function | Description |
|----------|-------------|
| `macmbx_sig_get(sigs, index)` | Get content (lazy-loads from disk). Do not free. |
| `macmbx_sig_set(sigs, index, text)` | Set content. Marks dirty. |
| `macmbx_sig_standard(sigs)` | Get Standard signature (index 0). |
| `macmbx_sig_alternate(sigs)` | Get Alternate signature (index 1). |

Content is lazy-loaded — not read from disk until first `macmbx_sig_get()` call.

## Sort Order

- Index 0: Standard (always first)
- Index 1: Alternate (always second)
- Index 2+: Custom signatures (order from directory scan)

## Example

```c
MacmbxSignatures *sigs = macmbx_sig_open("/path/to/Signatures");

/* Set the standard signature */
macmbx_sig_set(sigs, 0, "-- \nJohn Doe\njohn@example.com\n");

/* Add a work signature */
int idx = macmbx_sig_add(sigs, "Work",
  "-- \nJohn Doe\nSenior Engineer\nACME Corp\n");

/* Use in compose */
const char *sig = macmbx_sig_get(sigs, idx);
printf("Signature:\n%s", sig);

/* List all */
for (int i = 0; i < macmbx_sig_count(sigs); i++)
  printf("  %s\n", sigs->sigs[i].name);

macmbx_sig_save(sigs);
macmbx_sig_close(sigs);
```

## Integration with Compose

When composing a message, append the signature:

```c
const char *sig = macmbx_sig_standard(sigs);
if (sig && sig[0]) {
  /* Append to message body */
  strcat(body, "\n");
  strcat(body, sig);
}
```

Per-personality signatures: store the signature name in the personality settings, then look up with `macmbx_sig_find()`.
