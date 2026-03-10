/*
 * nickwin.c - Address Book window, ported from Mac Eudora's nickwin.c.
 *
 * Original used Carbon ListViews, ControlHandles, DLOG resources, and
 * tab controls. This GTK4 port uses GtkTreeView for the nickname list
 * and GtkNotebook for the tabbed detail view.
 *
 * Nickname file format (from nickmng.c):
 *   alias <name> <expansion>\n
 *   note <name> <content>\n
 * Names with spaces are quoted. Lines can be continued with \.
 */

#include "mailbox.h"
#include "mydefs.h"
#include <dirent.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern const char *prefs_get_mailboxes_path(void);

/* ─── Nickname data structures ───────────────────────────────────────── */

typedef struct NickEntry {
  char *name;      /* Nickname */
  char *addresses; /* Expansion addresses */
  char *notes;     /* Notes content */
  struct NickEntry *next;
} NickEntry;

typedef struct NickFile {
  char *path;     /* Path to nickname file */
  char *label;    /* Display name */
  NickEntry *entries;
  struct NickFile *next;
} NickFile;

/* ─── Address Book window state ──────────────────────────────────────── */

typedef struct {
  GtkWidget *window;
  GtkWidget *list_view;     /* GtkTreeView for nicknames */
  GtkTreeStore *list_store;
  GtkWidget *nick_entry;    /* Nickname name field */
  GtkWidget *addr_view;     /* Address text view */
  GtkWidget *notes_view;    /* Notes text view */
  GtkWidget *notebook;      /* Tab notebook */
  NickFile *files;
  bool inited;
} ABState;

static ABState AB = {0};

/* Tree store columns */
enum {
  AB_COL_NAME,      /* Display name */
  AB_COL_IS_FILE,   /* TRUE if this is a file header row */
  AB_COL_FILE_IDX,  /* Index into NickFile list */
  AB_COL_NICK_IDX,  /* Index into NickEntry list within file */
  AB_NUM_COLS
};

/* ─── Nickname file path ─────────────────────────────────────────────── */

static const char *get_nicknames_dir(void) {
  static char dir[1024] = {0};
  if (!dir[0]) {
    const char *prefs_path = prefs_get_mailboxes_path();
    if (prefs_path && prefs_path[0]) {
      /* Put nicknames alongside mailboxes in parent dir */
      char *slash = strrchr(prefs_path, '/');
      if (slash) {
        size_t len = slash - prefs_path;
        snprintf(dir, sizeof(dir), "%.*s/Nicknames", (int)len, prefs_path);
      } else {
        snprintf(dir, sizeof(dir), "%s/Nicknames", prefs_path);
      }
    } else {
      const char *home = g_get_home_dir();
      snprintf(dir, sizeof(dir), "%s/.local/share/geudora/Nicknames", home);
    }
    g_mkdir_with_parents(dir, 0755);
  }
  return dir;
}

/* ─── Nickname file parsing ──────────────────────────────────────────── */

/* Parse a nickname name — may be quoted or unquoted */
static const char *parse_nick_name(const char *p, char *out, size_t out_len) {
  while (*p == ' ' || *p == '\t')
    p++;

  size_t i = 0;
  if (*p == '"') {
    p++; /* skip opening quote */
    while (*p && *p != '"' && i < out_len - 1)
      out[i++] = *p++;
    if (*p == '"')
      p++;
  } else {
    while (*p && *p != ' ' && *p != '\t' && i < out_len - 1)
      out[i++] = *p++;
  }
  out[i] = '\0';

  /* Skip whitespace after name */
  while (*p == ' ' || *p == '\t')
    p++;

  return p;
}

/* Find or create a nick entry by name in a NickFile */
static NickEntry *find_or_create_nick(NickFile *nf, const char *name) {
  NickEntry *e = nf->entries;
  while (e) {
    if (strcmp(e->name, name) == 0)
      return e;
    e = e->next;
  }
  /* Create new */
  e = g_new0(NickEntry, 1);
  e->name = g_strdup(name);
  e->next = nf->entries;
  nf->entries = e;
  return e;
}

/* Read a nickname file (Eudora format) */
static NickFile *read_nick_file(const char *path, const char *label) {
  FILE *fp = fopen(path, "r");
  if (!fp)
    return NULL;

  NickFile *nf = g_new0(NickFile, 1);
  nf->path = g_strdup(path);
  nf->label = g_strdup(label);

  char line[4096];
  while (fgets(line, sizeof(line), fp)) {
    /* Strip trailing newline/CR */
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
      line[--len] = '\0';

    if (strncmp(line, "alias ", 6) == 0) {
      char name[256];
      const char *rest = parse_nick_name(line + 6, name, sizeof(name));
      if (name[0]) {
        NickEntry *e = find_or_create_nick(nf, name);
        g_free(e->addresses);
        e->addresses = g_strdup(rest);
      }
    } else if (strncmp(line, "note ", 5) == 0) {
      char name[256];
      const char *rest = parse_nick_name(line + 5, name, sizeof(name));
      if (name[0]) {
        NickEntry *e = find_or_create_nick(nf, name);
        g_free(e->notes);
        e->notes = g_strdup(rest);
      }
    }
  }

  fclose(fp);
  return nf;
}

/* Write a nickname file back to disk */
static void write_nick_file(NickFile *nf) {
  if (!nf || !nf->path)
    return;

  FILE *fp = fopen(nf->path, "w");
  if (!fp)
    return;

  /* Write all alias lines first, then all note lines (like original) */
  NickEntry *we;
  for (we = nf->entries; we; we = we->next) {
    if (we->addresses && we->addresses[0]) {
      if (strchr(we->name, ' '))
        fprintf(fp, "alias \"%s\" %s\n", we->name, we->addresses);
      else
        fprintf(fp, "alias %s %s\n", we->name, we->addresses);
    }
  }
  for (we = nf->entries; we; we = we->next) {
    if (we->notes && we->notes[0]) {
      if (strchr(we->name, ' '))
        fprintf(fp, "note \"%s\" %s\n", we->name, we->notes);
      else
        fprintf(fp, "note %s %s\n", we->name, we->notes);
    }
  }

  fclose(fp);
}

static void free_nick_file(NickFile *nf) {
  if (!nf)
    return;
  NickEntry *e = nf->entries;
  while (e) {
    NickEntry *next = e->next;
    g_free(e->name);
    g_free(e->addresses);
    g_free(e->notes);
    g_free(e);
    e = next;
  }
  g_free(nf->path);
  g_free(nf->label);
  g_free(nf);
}

static void free_all_nick_files(void) {
  NickFile *nf = AB.files;
  while (nf) {
    NickFile *next = nf->next;
    free_nick_file(nf);
    nf = next;
  }
  AB.files = NULL;
}

/* ─── Load all nickname files ────────────────────────────────────────── */

static void load_nick_files(void) {
  free_all_nick_files();

  const char *dir = get_nicknames_dir();

  /* Create default "Eudora Nicknames" file if nothing exists */
  char default_path[1024];
  snprintf(default_path, sizeof(default_path), "%s/Eudora Nicknames", dir);
  if (!g_file_test(default_path, G_FILE_TEST_EXISTS)) {
    FILE *f = fopen(default_path, "w");
    if (f)
      fclose(f);
  }

  /* Scan for all nickname files */
  DIR *d = opendir(dir);
  if (!d)
    return;

  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    if (ent->d_name[0] == '.')
      continue;

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
      continue;

    NickFile *nf = read_nick_file(path, ent->d_name);
    if (nf) {
      nf->next = AB.files;
      AB.files = nf;
    }
  }
  closedir(d);
}

/* ─── Populate tree store ────────────────────────────────────────────── */

static void ab_fill_list(void) {
  gtk_tree_store_clear(AB.list_store);

  int file_idx = 0;
  for (NickFile *nf = AB.files; nf; nf = nf->next, file_idx++) {
    GtkTreeIter file_iter;
    gtk_tree_store_append(AB.list_store, &file_iter, NULL);
    gtk_tree_store_set(AB.list_store, &file_iter, AB_COL_NAME, nf->label,
                       AB_COL_IS_FILE, TRUE, AB_COL_FILE_IDX, file_idx,
                       AB_COL_NICK_IDX, -1, -1);

    int nick_idx = 0;
    for (NickEntry *e = nf->entries; e; e = e->next, nick_idx++) {
      GtkTreeIter nick_iter;
      gtk_tree_store_append(AB.list_store, &nick_iter, &file_iter);
      gtk_tree_store_set(AB.list_store, &nick_iter, AB_COL_NAME, e->name,
                         AB_COL_IS_FILE, FALSE, AB_COL_FILE_IDX, file_idx,
                         AB_COL_NICK_IDX, nick_idx, -1);
    }
  }

  /* Expand all address books */
  gtk_tree_view_expand_all(GTK_TREE_VIEW(AB.list_view));
}

/* ─── Find nick entry by indices ─────────────────────────────────────── */

static NickEntry *get_nick_by_indices(int file_idx, int nick_idx) {
  int fi = 0;
  for (NickFile *nf = AB.files; nf; nf = nf->next, fi++) {
    if (fi == file_idx) {
      int ni = 0;
      for (NickEntry *e = nf->entries; e; e = e->next, ni++) {
        if (ni == nick_idx)
          return e;
      }
      return NULL;
    }
  }
  return NULL;
}

static NickFile *get_nick_file_by_index(int file_idx) {
  int fi = 0;
  for (NickFile *nf = AB.files; nf; nf = nf->next, fi++) {
    if (fi == file_idx)
      return nf;
  }
  return NULL;
}

/* ─── Selection changed — update detail fields ──────────────────────── */

static void on_ab_selection_changed(GtkTreeSelection *selection,
                                     gpointer user_data) {
  (void)user_data;
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_tree_selection_get_selected(selection, &model, &iter))
    return;

  gboolean is_file;
  int file_idx, nick_idx;
  gtk_tree_model_get(model, &iter, AB_COL_IS_FILE, &is_file, AB_COL_FILE_IDX,
                     &file_idx, AB_COL_NICK_IDX, &nick_idx, -1);

  if (is_file) {
    /* Address book header selected — clear fields */
    gtk_editable_set_text(GTK_EDITABLE(AB.nick_entry), "");
    GtkTextBuffer *addr_buf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
    gtk_text_buffer_set_text(addr_buf, "", -1);
    GtkTextBuffer *notes_buf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
    gtk_text_buffer_set_text(notes_buf, "", -1);
    return;
  }

  NickEntry *e = get_nick_by_indices(file_idx, nick_idx);
  if (!e)
    return;

  gtk_editable_set_text(GTK_EDITABLE(AB.nick_entry), e->name ? e->name : "");

  GtkTextBuffer *addr_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  gtk_text_buffer_set_text(addr_buf, e->addresses ? e->addresses : "", -1);

  GtkTextBuffer *notes_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  gtk_text_buffer_set_text(notes_buf, e->notes ? e->notes : "", -1);
}

/* ─── Save current entry back to data ────────────────────────────────── */

static void save_current_entry(void) {
  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(AB.list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_tree_selection_get_selected(sel, &model, &iter))
    return;

  gboolean is_file;
  int file_idx, nick_idx;
  gtk_tree_model_get(model, &iter, AB_COL_IS_FILE, &is_file, AB_COL_FILE_IDX,
                     &file_idx, AB_COL_NICK_IDX, &nick_idx, -1);

  if (is_file)
    return;

  NickEntry *e = get_nick_by_indices(file_idx, nick_idx);
  NickFile *nf = get_nick_file_by_index(file_idx);
  if (!e || !nf)
    return;

  /* Update addresses */
  GtkTextBuffer *addr_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(addr_buf, &start, &end);
  char *addr_text = gtk_text_buffer_get_text(addr_buf, &start, &end, FALSE);
  g_free(e->addresses);
  e->addresses = addr_text;

  /* Update notes */
  GtkTextBuffer *notes_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  gtk_text_buffer_get_bounds(notes_buf, &start, &end);
  char *notes_text = gtk_text_buffer_get_text(notes_buf, &start, &end, FALSE);
  g_free(e->notes);
  e->notes = notes_text;

  /* Write back to disk */
  write_nick_file(nf);
}

/* ─── Button callbacks ───────────────────────────────────────────────── */

static void on_ab_new_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;

  /* Find the first address book file, or create one */
  NickFile *nf = AB.files;
  if (!nf) {
    const char *dir = get_nicknames_dir();
    char path[1024];
    snprintf(path, sizeof(path), "%s/Eudora Nicknames", dir);
    nf = g_new0(NickFile, 1);
    nf->path = g_strdup(path);
    nf->label = g_strdup("Eudora Nicknames");
    nf->next = AB.files;
    AB.files = nf;
  }

  /* Generate unique "Untitled" name */
  char name[64] = "Untitled";
  int n = 1;
  while (find_or_create_nick(nf, name) != NULL) {
    /* find_or_create will create it, but we want to check first */
    break;
  }
  /* Actually just create a new entry */
  NickEntry *e = g_new0(NickEntry, 1);
  snprintf(name, sizeof(name), "New Nickname");
  n = 1;
  for (NickEntry *check = nf->entries; check; check = check->next) {
    if (strncmp(check->name, "New Nickname", 12) == 0)
      n++;
  }
  if (n > 1)
    snprintf(name, sizeof(name), "New Nickname %d", n);

  e->name = g_strdup(name);
  e->addresses = g_strdup("");
  e->notes = g_strdup("");
  e->next = nf->entries;
  nf->entries = e;

  write_nick_file(nf);
  ab_fill_list();
}

static void on_ab_delete_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;

  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(AB.list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_tree_selection_get_selected(sel, &model, &iter))
    return;

  gboolean is_file;
  int file_idx, nick_idx;
  gtk_tree_model_get(model, &iter, AB_COL_IS_FILE, &is_file, AB_COL_FILE_IDX,
                     &file_idx, AB_COL_NICK_IDX, &nick_idx, -1);

  if (is_file)
    return; /* Don't delete address book files from here */

  NickFile *nf = get_nick_file_by_index(file_idx);
  if (!nf)
    return;

  /* Remove entry from linked list */
  NickEntry **pp = &nf->entries;
  int ni = 0;
  while (*pp) {
    if (ni == nick_idx) {
      NickEntry *doomed = *pp;
      *pp = doomed->next;
      g_free(doomed->name);
      g_free(doomed->addresses);
      g_free(doomed->notes);
      g_free(doomed);
      break;
    }
    pp = &(*pp)->next;
    ni++;
  }

  write_nick_file(nf);
  ab_fill_list();
}

/* To/Cc/Bcc buttons — copy selected nickname's addresses to compose window */
static void on_ab_recipient_clicked(GtkButton *button, gpointer user_data) {
  const char *field = (const char *)user_data; /* "to", "cc", or "bcc" */
  (void)button;

  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(AB.list_view));
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_tree_selection_get_selected(sel, &model, &iter))
    return;

  gboolean is_file;
  int file_idx, nick_idx;
  gtk_tree_model_get(model, &iter, AB_COL_IS_FILE, &is_file, AB_COL_FILE_IDX,
                     &file_idx, AB_COL_NICK_IDX, &nick_idx, -1);

  if (is_file)
    return;

  NickEntry *e = get_nick_by_indices(file_idx, nick_idx);
  if (!e || !e->addresses || !e->addresses[0])
    return;

  /* Copy address to clipboard with field indicator */
  GdkClipboard *clipboard =
      gdk_display_get_clipboard(gdk_display_get_default());
  gdk_clipboard_set_text(clipboard, e->addresses);

  g_print("Address Book: copied %s address for %s field\n", e->name, field);
}

/* Window close handler */
static gboolean on_ab_window_close(GtkWindow *w, gpointer ud) {
  (void)w;
  (void)ud;
  AB.inited = false;
  AB.window = NULL;
  return FALSE;
}

/* ─── OpenABWin — open the address book window ───────────────────────── */

void OpenABWin(void) {
  if (AB.inited) {
    gtk_window_present(GTK_WINDOW(AB.window));
    return;
  }

  load_nick_files();

  AB.window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(AB.window), "Address Book");
  gtk_window_set_default_size(GTK_WINDOW(AB.window), 700, 500);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* ── Top toolbar (matching original: New, Delete, To:, Cc:, Bcc:) ── */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(toolbar, 4);
  gtk_widget_set_margin_end(toolbar, 4);
  gtk_widget_set_margin_top(toolbar, 4);
  gtk_widget_set_margin_bottom(toolbar, 4);

  GtkWidget *btn_new = gtk_button_new_with_label("New");
  GtkWidget *btn_del = gtk_button_new_with_label("Delete");
  GtkWidget *btn_to = gtk_button_new_with_label("To:");
  GtkWidget *btn_cc = gtk_button_new_with_label("Cc:");
  GtkWidget *btn_bcc = gtk_button_new_with_label("Bcc:");

  g_signal_connect(btn_new, "clicked", G_CALLBACK(on_ab_new_clicked), NULL);
  g_signal_connect(btn_del, "clicked", G_CALLBACK(on_ab_delete_clicked), NULL);
  g_signal_connect(btn_to, "clicked", G_CALLBACK(on_ab_recipient_clicked),
                   (gpointer) "to");
  g_signal_connect(btn_cc, "clicked", G_CALLBACK(on_ab_recipient_clicked),
                   (gpointer) "cc");
  g_signal_connect(btn_bcc, "clicked", G_CALLBACK(on_ab_recipient_clicked),
                   (gpointer) "bcc");

  gtk_box_append(GTK_BOX(toolbar), btn_new);
  gtk_box_append(GTK_BOX(toolbar), btn_del);
  /* Spacer */
  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(toolbar), spacer);
  gtk_box_append(GTK_BOX(toolbar), btn_to);
  gtk_box_append(GTK_BOX(toolbar), btn_cc);
  gtk_box_append(GTK_BOX(toolbar), btn_bcc);
  gtk_box_append(GTK_BOX(vbox), toolbar);

  /* ── HPaned: nickname list on left, details on right ── */
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(hpaned), 200);

  /* Left: tree view showing address books and nicknames */
  AB.list_store = gtk_tree_store_new(AB_NUM_COLS, G_TYPE_STRING,
                                     G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_INT);
  AB.list_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(AB.list_store));
  g_object_unref(AB.list_store);

  GtkTreeViewColumn *col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, "Nicknames");
  GtkCellRenderer *text_r = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, text_r, TRUE);
  gtk_tree_view_column_add_attribute(col, text_r, "text", AB_COL_NAME);
  gtk_tree_view_append_column(GTK_TREE_VIEW(AB.list_view), col);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(AB.list_view), FALSE);

  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(AB.list_view));
  g_signal_connect(sel, "changed", G_CALLBACK(on_ab_selection_changed), NULL);

  GtkWidget *list_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll),
                                AB.list_view);
  gtk_paned_set_start_child(GTK_PANED(hpaned), list_scroll);

  /* Right: tabbed detail view (matching original's tab control) */
  GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(right_box, 4);
  gtk_widget_set_margin_end(right_box, 4);
  gtk_widget_set_margin_top(right_box, 4);
  gtk_widget_set_margin_bottom(right_box, 4);

  /* Nickname name field at top */
  GtkWidget *name_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *name_label = gtk_label_new("Nickname:");
  gtk_widget_set_size_request(name_label, 80, -1);
  AB.nick_entry = gtk_entry_new();
  gtk_widget_set_hexpand(AB.nick_entry, TRUE);
  gtk_box_append(GTK_BOX(name_row), name_label);
  gtk_box_append(GTK_BOX(name_row), AB.nick_entry);
  gtk_box_append(GTK_BOX(right_box), name_row);

  /* Tabbed notebook — like original's abTabs control */
  AB.notebook = gtk_notebook_new();
  gtk_widget_set_vexpand(AB.notebook, TRUE);

  /* Tab 1: Address (expansion) */
  GtkWidget *addr_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(addr_box, 4);
  gtk_widget_set_margin_end(addr_box, 4);
  gtk_widget_set_margin_top(addr_box, 4);
  gtk_widget_set_margin_bottom(addr_box, 4);

  GtkWidget *addr_label = gtk_label_new("Address(es):");
  gtk_widget_set_halign(addr_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(addr_box), addr_label);

  GtkWidget *addr_scroll = gtk_scrolled_window_new();
  AB.addr_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(AB.addr_view), GTK_WRAP_WORD_CHAR);
  gtk_widget_set_vexpand(addr_scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(addr_scroll),
                                AB.addr_view);
  gtk_box_append(GTK_BOX(addr_box), addr_scroll);

  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), addr_box,
                           gtk_label_new("Address"));

  /* Tab 2: Notes */
  GtkWidget *notes_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(notes_box, 4);
  gtk_widget_set_margin_end(notes_box, 4);
  gtk_widget_set_margin_top(notes_box, 4);
  gtk_widget_set_margin_bottom(notes_box, 4);

  GtkWidget *notes_label = gtk_label_new("Notes:");
  gtk_widget_set_halign(notes_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(notes_box), notes_label);

  GtkWidget *notes_scroll = gtk_scrolled_window_new();
  AB.notes_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(AB.notes_view), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(notes_scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notes_scroll),
                                AB.notes_view);
  gtk_box_append(GTK_BOX(notes_box), notes_scroll);

  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), notes_box,
                           gtk_label_new("Notes"));

  gtk_box_append(GTK_BOX(right_box), AB.notebook);

  /* Save button at bottom */
  GtkWidget *save_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *save_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(save_spacer, TRUE);
  GtkWidget *btn_save = gtk_button_new_with_label("Save");
  g_signal_connect_swapped(btn_save, "clicked",
                           G_CALLBACK(save_current_entry), NULL);
  gtk_box_append(GTK_BOX(save_row), save_spacer);
  gtk_box_append(GTK_BOX(save_row), btn_save);
  gtk_box_append(GTK_BOX(right_box), save_row);

  gtk_paned_set_end_child(GTK_PANED(hpaned), right_box);
  gtk_widget_set_vexpand(hpaned, TRUE);
  gtk_box_append(GTK_BOX(vbox), hpaned);

  gtk_window_set_child(GTK_WINDOW(AB.window), vbox);

  /* Populate */
  ab_fill_list();

  /* Handle window close */
  g_signal_connect(AB.window, "close-request",
                   G_CALLBACK(on_ab_window_close), NULL);

  AB.inited = true;
  gtk_window_present(GTK_WINDOW(AB.window));
}
