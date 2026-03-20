# Stationery

Message templates stored as individual files in a Stationery/ directory.

## Storage

```
Stationery/
  Meeting Invite       ← template file
  Bug Report           ← template file
  Weekly Update        ← template file
```

Each file contains a complete RFC822 message (headers + body) used as a starting point for new messages. When composing from stationery, the template's headers (To, Cc, Subject) and body are pre-filled.

## Types

```c
typedef struct {
  char name[256];
  char path[PATH_MAX];
  char *content;          /* malloc'd full message, NULL if not loaded */
  long content_len;
  bool dirty;
} MacmbxStationery;

typedef struct {
  char dir_path[PATH_MAX];
  MacmbxStationery *items;
  int count, capacity;
} MacmbxStationerySet;
```

## Lifecycle

| Function | Description |
|----------|-------------|
| `macmbx_stat_open(dir)` | Open stationery directory. Creates dir if needed. |
| `macmbx_stat_close(ss)` | Free all. Does NOT save. |
| `macmbx_stat_save(ss)` | Save all dirty templates (atomic via temp+rename). |

## CRUD

| Function | Description |
|----------|-------------|
| `macmbx_stat_add(ss, "Bug Report", msg, len)` | Add new template. Returns index. |
| `macmbx_stat_remove(ss, index)` | Delete template file. |
| `macmbx_stat_rename(ss, index, "Defect Report")` | Rename template file. |
| `macmbx_stat_find(ss, "Bug Report")` | Find by name. Returns index or -1. |
| `macmbx_stat_count(ss)` | Number of templates. |

## Access

| Function | Description |
|----------|-------------|
| `macmbx_stat_get(ss, index, &len)` | Get template content (lazy-loads). Do not free. |
| `macmbx_stat_set(ss, index, msg, len)` | Replace template content. |
| `macmbx_stat_new_message(ss, index, &len)` | Get a copy of template for composing. Caller frees. |
| `macmbx_stat_save_from_message(ss, name, msg, len)` | Save current compose as stationery (creates or updates). |

## Template Format

Templates are standard RFC822 messages:

```
To: team@example.com
Subject: Weekly Status Update
Content-Type: text/plain

Hi team,

Here is my weekly update:

1. Completed:
   -

2. In Progress:
   -

3. Blockers:
   -

Best,
```

The compose window reads this and pre-fills To, Subject, and body. The user edits and sends.

## Example: Create and Use Stationery

```c
MacmbxStationerySet *ss = macmbx_stat_open("/path/to/Stationery");

/* Create a template */
const char *tmpl =
  "To: bugs@example.com\r\n"
  "Subject: [BUG] \r\n"
  "Content-Type: text/plain\r\n"
  "\r\n"
  "Steps to reproduce:\n"
  "1. \n"
  "\n"
  "Expected behavior:\n"
  "\n"
  "Actual behavior:\n";

macmbx_stat_add(ss, "Bug Report", tmpl, -1);
macmbx_stat_save(ss);

/* Later: compose from template */
long len;
char *msg = macmbx_stat_new_message(ss, 0, &len);
/* msg is now a copy of the template — edit and send */
free(msg);
```

## Example: Save Compose as Stationery

```c
/* User composes a message and clicks "Save as Stationery" */
const char *composed_msg = get_compose_content();
long msg_len = strlen(composed_msg);

macmbx_stat_save_from_message(ss, "My Template", composed_msg, msg_len);
macmbx_stat_save(ss);
```

## Example: List Available Templates

```c
MacmbxStationerySet *ss = macmbx_stat_open("/path/to/Stationery");

printf("Available stationery:\n");
for (int i = 0; i < macmbx_stat_count(ss); i++)
  printf("  %d. %s\n", i + 1, ss->items[i].name);

macmbx_stat_close(ss);
```
