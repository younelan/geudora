# Nicknames / Address Book

Eudora-compatible nickname file management with lookup for filters and junk whitelist.

## File Format

Eudora nickname files contain `alias` and `note` lines:

```
alias cjones cjones@hermes.example.net
alias youness youness@hermes.example.net
alias "John Doe" john@example.com,john.doe@work.com
note cjones <first:Chris><last:Jones><note:My friend>
note youness <first:Youness><last:El Andaloussi><picture:/path/to/photo.png>
```

Rules:
- `alias name addresses` — nickname to address mapping (comma-separated)
- `note name <key:value><key:value>...` — metadata fields
- Names with spaces are quoted: `"John Doe"`
- All alias lines come before note lines (Eudora convention)
- Multiple address book files in a `Nicknames/` directory

## Types

```c
typedef struct {
  char name[256];           /* nickname */
  char *addresses;          /* comma-separated, malloc'd */
  MacmbxNoteField *notes;   /* linked list of <key:value> fields */
  uint32_t name_hash;       /* for fast lookup */
  uint32_t addr_hash;       /* hash of first address */
  bool deleted, dirty;
} MacmbxNickname;

typedef struct {
  char name[256];           /* book display name (filename) */
  char path[PATH_MAX];
  MacmbxNickname *entries;
  int count, capacity;
  bool dirty;
} MacmbxAddressBook;

typedef struct {
  char dir_path[PATH_MAX];  /* Nicknames/ directory */
  MacmbxAddressBook *books;
  int count, capacity;
} MacmbxAddressBooks;
```

## Lifecycle

| Function | Description |
|----------|-------------|
| `macmbx_nick_open(dir)` | Open all nickname files from directory. Creates dir if needed. |
| `macmbx_nick_close(abs)` | Free all. Does NOT save. |
| `macmbx_nick_save(abs)` | Save all dirty books. |
| `macmbx_nick_save_book(book)` | Save one book (atomic via temp+rename). |

## Book Management

| Function | Description |
|----------|-------------|
| `macmbx_nick_create_book(abs, name)` | Create new empty book. |
| `macmbx_nick_remove_book(abs, index)` | Delete book + file. |
| `macmbx_nick_get_book(abs, index)` | Get by index. |
| `macmbx_nick_find_book(abs, name)` | Find by name. |

## Nickname CRUD

| Function | Description |
|----------|-------------|
| `macmbx_nick_add(book, name, addresses)` | Add nickname. Returns index. |
| `macmbx_nick_remove(book, index)` | Mark deleted. |
| `macmbx_nick_find(book, name)` | Find by name in one book. Returns index or -1. |
| `macmbx_nick_find_all(abs, name, &book_idx)` | Find across all books. |
| `macmbx_nick_get_addresses(book, index)` | Get address string. |
| `macmbx_nick_set_addresses(book, index, addrs)` | Set addresses. |
| `macmbx_nick_rename(book, index, new_name)` | Rename nickname. |

## Note Fields

| Function | Description |
|----------|-------------|
| `macmbx_nick_get_field(book, index, "first")` | Get a note field. Returns NULL if not set. |
| `macmbx_nick_set_field(book, index, "first", "John")` | Set/create a field. |
| `macmbx_nick_remove_field(book, index, "picture")` | Remove a field. |

Common fields: `first`, `last`, `note`, `picture`, `phone`, `company`, `title`, `address`

## Lookup and Search

| Function | Description |
|----------|-------------|
| `macmbx_nick_contains_address(abs, "john@example.com")` | Check if email exists in any book. Case-insensitive substring. |
| `macmbx_nick_contains_hash(abs, hash)` | Fast hash-based lookup. |
| `macmbx_nick_expand(abs, "cjones")` | Expand nickname to addresses. Returns malloc'd string. |
| `macmbx_nick_search(abs, "john", &results)` | Search all books by name, address, or note fields. Returns pairs of (book_idx, nick_idx). |
| `macmbx_nick_hash(str)` | Hash a string (djb2, case-insensitive). |

## Integration with Junk Whitelist

Use the address book as a whitelist — senders in the address book are never junk:

```c
MacmbxAddressBooks *abs = macmbx_nick_open("/path/to/Nicknames");
MacmbxJunkConfig cfg;
macmbx_junk_config_init(&cfg);

/* Whitelist callback using address book */
bool nick_whitelist(const char *from, void *ctx) {
  return macmbx_nick_contains_address((MacmbxAddressBooks *)ctx, from);
}
macmbx_junk_set_whitelist(&cfg, nick_whitelist, abs);

/* Now scoring will auto-whitelist known senders */
macmbx_junk_score_box(&cfg, inbox);
```

## Integration with Filters

Use nickname lookup in filter conditions. The filter engine's `Any:` and generic header matching can check addresses, but for "sender is in address book" you'd use a custom condition check:

```c
/* Before applying filters, check if sender is known */
for (int i = 0; i < inbox->count; i++) {
  if (macmbx_nick_contains_address(abs, inbox->msgs[i].from)) {
    /* Known sender — maybe skip junk scoring */
    inbox->msgs[i].spam_score = 0;
    inbox->msgs[i].spam_because = MACMBX_JUNK_BECAUSE_WHITELIST;
  }
}
```

## Example: Full Address Book Flow

```c
MacmbxAddressBooks *abs = macmbx_nick_open("/path/to/Nicknames");

/* List all books */
for (int b = 0; b < abs->count; b++)
  printf("Book: %s (%d entries)\n", abs->books[b].name, abs->books[b].count);

/* Add a contact */
MacmbxAddressBook *main = macmbx_nick_find_book(abs, "Eudora Nicknames");
int idx = macmbx_nick_add(main, "alice", "alice@example.com");
macmbx_nick_set_field(main, idx, "first", "Alice");
macmbx_nick_set_field(main, idx, "last", "Smith");
macmbx_nick_set_field(main, idx, "note", "Met at conference");

/* Look up */
char *addrs = macmbx_nick_expand(abs, "alice");
printf("alice -> %s\n", addrs);  /* "alice@example.com" */
free(addrs);

/* Check whitelist */
bool known = macmbx_nick_contains_address(abs, "alice@example.com");
printf("Known sender: %s\n", known ? "yes" : "no");

/* Search */
int *results;
int found = macmbx_nick_search(abs, "smith", &results);
for (int i = 0; i < found; i++) {
  int bi = results[i*2], ni = results[i*2+1];
  printf("Found: %s in %s\n", abs->books[bi].entries[ni].name,
         abs->books[bi].name);
}
free(results);

/* Save */
macmbx_nick_save(abs);
macmbx_nick_close(abs);
```
