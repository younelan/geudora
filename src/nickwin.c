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

/* ─── Tagged field helpers (original <tag:value> format) ──────────────── */

/* Get a tagged field value from a notes string. Returns malloc'd string or NULL. */
static char *get_tagged_field(const char *notes, const char *tag) {
  if (!notes || !tag) return NULL;
  size_t tlen = strlen(tag);
  const char *p = notes;
  while ((p = strchr(p, '<')) != NULL) {
    p++;
    if (strncmp(p, tag, tlen) == 0 && p[tlen] == ':') {
      const char *val = p + tlen + 1;
      const char *end = strchr(val, '>');
      if (end) return g_strndup(val, end - val);
    }
  }
  return NULL;
}

/* Set a tagged field value in a notes string. Returns new malloc'd string. */
static char *set_tagged_field(const char *notes, const char *tag, const char *value) {
  GString *out = g_string_new("");
  size_t tlen = strlen(tag);
  bool replaced = false;

  if (notes) {
    const char *p = notes;
    while (*p) {
      if (*p == '<') {
        const char *s = p + 1;
        if (strncmp(s, tag, tlen) == 0 && s[tlen] == ':') {
          /* Skip existing tag */
          const char *end = strchr(s, '>');
          if (end) { p = end + 1; continue; }
        }
      }
      g_string_append_c(out, *p);
      p++;
    }
  }

  /* Append new value if non-empty */
  if (value && value[0]) {
    g_string_append_printf(out, "<%s:%s>", tag, value);
    replaced = true;
  }
  (void)replaced;
  return g_string_free(out, FALSE);
}

/* Bulk set: apply multiple tag/value pairs to notes. tags is NULL-terminated. */
static char *set_tagged_fields(const char *notes, const char **tags, const char **values) {
  char *cur = g_strdup(notes ? notes : "");
  for (int i = 0; tags[i]; i++) {
    char *next = set_tagged_field(cur, tags[i], values[i]);
    g_free(cur);
    cur = next;
  }
  return cur;
}

/* ─── Address Book window state ──────────────────────────────────────── */

typedef struct {
  GtkWidget *window;
  GtkWidget *list_view;     /* GtkListBox for nicknames */
  GtkWidget *nick_entry;    /* Nickname name field */
  GtkWidget *addr_view;     /* Address text view */
  GtkWidget *notes_view;    /* Notes text view */
  GtkWidget *notebook;      /* Tab notebook */
  NickFile *files;
  bool inited;

  /* Personal tab */
  GtkWidget *full_name;
  GtkWidget *first_name;
  GtkWidget *last_name;

  /* Home tab */
  GtkWidget *h_address;
  GtkWidget *h_city;
  GtkWidget *h_state;
  GtkWidget *h_zip;
  GtkWidget *h_country;
  GtkWidget *h_phone;
  GtkWidget *h_fax;
  GtkWidget *h_mobile;
  GtkWidget *h_web;

  /* Work tab */
  GtkWidget *w_title;
  GtkWidget *w_company;
  GtkWidget *w_address;
  GtkWidget *w_city;
  GtkWidget *w_state;
  GtkWidget *w_zip;
  GtkWidget *w_country;
  GtkWidget *w_phone;
  GtkWidget *w_fax;
  GtkWidget *w_mobile;
  GtkWidget *w_web;

  /* Other tab */
  GtkWidget *other_email;
  GtkWidget *other_phone;
  GtkWidget *other_web;

  /* Photo tab */
  GtkWidget *photo_image;    /* GtkPicture for display */
  GtkWidget *photo_btn;      /* "Select Photo" button */
  GtkWidget *photo_remove;   /* "Remove Photo" button */
  char *photo_path;          /* Current photo file path (owned) */
} ABState;

static ABState AB = {0};

/* GObject data keys for list rows */
#define AB_KEY_IS_FILE  "ab-is-file"
#define AB_KEY_FILE_IDX "ab-file-idx"
#define AB_KEY_NICK_IDX "ab-nick-idx"

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
    if (strcmp(spec_name(e), name) == 0)
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
  nf = g_strdup(path);
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
  if (!nf || !nf)
    return;

  FILE *fp = fopen(nf, "w");
  if (!fp)
    return;

  /* Write all alias lines first, then all note lines (like original) */
  NickEntry *we;
  for (we = nf->entries; we; we = we->next) {
    const char *addr = (we->addresses && we->addresses[0]) ? we->addresses : "";
    if (strchr(spec_name(we), ' '))
      fprintf(fp, "alias \"%s\" %s\n", spec_name(we), addr);
    else
      fprintf(fp, "alias %s %s\n", spec_name(we), addr);
  }
  for (we = nf->entries; we; we = we->next) {
    if (we->notes && we->notes[0]) {
      if (strchr(spec_name(we), ' '))
        fprintf(fp, "note \"%s\" %s\n", spec_name(we), we->notes);
      else
        fprintf(fp, "note %s %s\n", spec_name(we), we->notes);
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
    g_free(spec_name(e));
    g_free(e->addresses);
    g_free(e->notes);
    g_free(e);
    e = next;
  }
  g_free(nf);
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

/* ─── Populate list box ──────────────────────────────────────────────── */

static void ab_fill_list(void) {
  /* Clear existing rows */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(AB.list_view)) != NULL)
    gtk_list_box_remove(GTK_LIST_BOX(AB.list_view), child);

  int file_idx = 0;
  for (NickFile *nf = AB.files; nf; nf = nf->next, file_idx++) {
    /* Section header row (non-selectable) */
    GtkWidget *hdr_label = gtk_label_new(nf->label);
    gtk_label_set_xalign(GTK_LABEL(hdr_label), 0);
    gtk_widget_add_css_class(hdr_label, "ab-section-title");
    GtkWidget *hdr_row = gtk_list_box_row_new();
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(hdr_row), FALSE);
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(hdr_row), FALSE);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(hdr_row), hdr_label);
    g_object_set_data(G_OBJECT(hdr_row), AB_KEY_IS_FILE, GINT_TO_POINTER(1));
    g_object_set_data(G_OBJECT(hdr_row), AB_KEY_FILE_IDX, GINT_TO_POINTER(file_idx));
    gtk_list_box_append(GTK_LIST_BOX(AB.list_view), hdr_row);

    int nick_idx = 0;
    for (NickEntry *e = nf->entries; e; e = e->next, nick_idx++) {
      GtkWidget *nick_label = gtk_label_new(spec_name(e));
      gtk_label_set_xalign(GTK_LABEL(nick_label), 0);
      gtk_widget_set_margin_start(nick_label, 12);
      GtkWidget *nick_row = gtk_list_box_row_new();
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(nick_row), nick_label);
      g_object_set_data(G_OBJECT(nick_row), AB_KEY_IS_FILE, GINT_TO_POINTER(0));
      g_object_set_data(G_OBJECT(nick_row), AB_KEY_FILE_IDX, GINT_TO_POINTER(file_idx));
      g_object_set_data(G_OBJECT(nick_row), AB_KEY_NICK_IDX, GINT_TO_POINTER(nick_idx));
      gtk_list_box_append(GTK_LIST_BOX(AB.list_view), nick_row);
    }
  }
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

/* Get file_idx and nick_idx from the currently selected list box row.
 * Returns false if nothing is selected or a file header is selected. */
static bool ab_get_selected(int *file_idx_out, int *nick_idx_out) {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(AB.list_view));
  if (!row) return false;
  int is_file = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_IS_FILE));
  if (is_file) return false;
  *file_idx_out = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_FILE_IDX));
  *nick_idx_out = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_NICK_IDX));
  return true;
}

/* Update the label text in the currently selected list row */
static void ab_update_selected_label(const char *text) {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(AB.list_view));
  if (!row) return;
  GtkWidget *label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
  if (GTK_IS_LABEL(label))
    gtk_label_set_text(GTK_LABEL(label), text);
}

/* Forward declarations */
static void display_photo(const char *path);

/* ─── Helper: set entry text or clear ────────────────────────────────── */

static void set_entry_from_tag(GtkWidget *entry, const char *notes, const char *tag) {
  char *val = get_tagged_field(notes, tag);
  gtk_editable_set_text(GTK_EDITABLE(entry), val ? val : "");
  g_free(val);
}

static void clear_all_detail_fields(void) {
  gtk_editable_set_text(GTK_EDITABLE(AB.nick_entry), "");
  GtkTextBuffer *addr_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  gtk_text_buffer_set_text(addr_buf, "", -1);
  GtkTextBuffer *notes_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  gtk_text_buffer_set_text(notes_buf, "", -1);

  /* Personal */
  gtk_editable_set_text(GTK_EDITABLE(AB.full_name), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.first_name), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.last_name), "");

  /* Home */
  gtk_editable_set_text(GTK_EDITABLE(AB.h_address), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_city), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_state), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_zip), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_country), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_phone), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_fax), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_mobile), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.h_web), "");

  /* Work */
  gtk_editable_set_text(GTK_EDITABLE(AB.w_title), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_company), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_address), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_city), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_state), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_zip), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_country), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_phone), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_fax), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_mobile), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.w_web), "");

  /* Other */
  gtk_editable_set_text(GTK_EDITABLE(AB.other_email), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.other_phone), "");
  gtk_editable_set_text(GTK_EDITABLE(AB.other_web), "");

  /* Photo */
  g_free(AB.photo_path);
  AB.photo_path = NULL;
  if (AB.photo_image)
    gtk_picture_set_paintable(GTK_PICTURE(AB.photo_image), NULL);
}

/* ─── Selection changed — update detail fields ──────────────────────── */

static void on_ab_selection_changed(GtkListBox *box, GtkListBoxRow *row,
                                     gpointer user_data) {
  (void)box; (void)user_data;
  if (!row) {
    clear_all_detail_fields();
    return;
  }

  int is_file = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_IS_FILE));
  if (is_file) {
    clear_all_detail_fields();
    return;
  }

  int file_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_FILE_IDX));
  int nick_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_NICK_IDX));

  NickEntry *e = get_nick_by_indices(file_idx, nick_idx);
  if (!e)
    return;

  gtk_editable_set_text(GTK_EDITABLE(AB.nick_entry), spec_name(e) ? spec_name(e) : "");

  GtkTextBuffer *addr_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  gtk_text_buffer_set_text(addr_buf, e->addresses ? e->addresses : "", -1);

  /* Populate tagged fields from notes */
  const char *notes = e->notes ? e->notes : "";

  /* Personal */
  set_entry_from_tag(AB.full_name, notes, "name");
  set_entry_from_tag(AB.first_name, notes, "first");
  set_entry_from_tag(AB.last_name, notes, "last");

  /* Home */
  set_entry_from_tag(AB.h_address, notes, "address");
  set_entry_from_tag(AB.h_city, notes, "city");
  set_entry_from_tag(AB.h_state, notes, "state");
  set_entry_from_tag(AB.h_zip, notes, "zip");
  set_entry_from_tag(AB.h_country, notes, "country");
  set_entry_from_tag(AB.h_phone, notes, "phone");
  set_entry_from_tag(AB.h_fax, notes, "fax");
  set_entry_from_tag(AB.h_mobile, notes, "mobile");
  set_entry_from_tag(AB.h_web, notes, "web");

  /* Work */
  set_entry_from_tag(AB.w_title, notes, "title");
  set_entry_from_tag(AB.w_company, notes, "company");
  set_entry_from_tag(AB.w_address, notes, "address2");
  set_entry_from_tag(AB.w_city, notes, "city2");
  set_entry_from_tag(AB.w_state, notes, "state2");
  set_entry_from_tag(AB.w_zip, notes, "zip2");
  set_entry_from_tag(AB.w_country, notes, "country2");
  set_entry_from_tag(AB.w_phone, notes, "phone2");
  set_entry_from_tag(AB.w_fax, notes, "fax2");
  set_entry_from_tag(AB.w_mobile, notes, "mobile2");
  set_entry_from_tag(AB.w_web, notes, "web2");

  /* Other */
  set_entry_from_tag(AB.other_email, notes, "otheremail");
  set_entry_from_tag(AB.other_phone, notes, "otherphone");
  set_entry_from_tag(AB.other_web, notes, "otherweb");

  /* Notes — strip tagged fields, show remaining plain text */
  GtkTextBuffer *notes_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  char *note_val = get_tagged_field(notes, "note");
  gtk_text_buffer_set_text(notes_buf, note_val ? note_val : "", -1);
  g_free(note_val);

  /* Photo */
  g_free(AB.photo_path);
  AB.photo_path = get_tagged_field(notes, "picture");
  display_photo(AB.photo_path);
}

/* ─── Save current entry back to data ────────────────────────────────── */

static void save_current_entry(void) {
  int file_idx, nick_idx;
  if (!ab_get_selected(&file_idx, &nick_idx))
    return;

  NickEntry *e = get_nick_by_indices(file_idx, nick_idx);
  NickFile *nf = get_nick_file_by_index(file_idx);
  if (!e || !nf)
    return;

  /* Update nickname name */
  const char *new_name = gtk_editable_get_text(GTK_EDITABLE(AB.nick_entry));
  if (new_name && new_name[0] && strcmp(spec_name(e), new_name) != 0) {
    g_free(spec_name(e));
    e->name = g_strdup(new_name);
    ab_update_selected_label(new_name);
  }

  /* Update addresses */
  GtkTextBuffer *addr_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(addr_buf, &start, &end);
  char *addr_text = gtk_text_buffer_get_text(addr_buf, &start, &end, FALSE);
  g_free(e->addresses);
  e->addresses = addr_text;

  /* Update notes: get plain note text, then set all tagged fields */
  GtkTextBuffer *notes_buf =
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  gtk_text_buffer_get_bounds(notes_buf, &start, &end);
  char *note_text = gtk_text_buffer_get_text(notes_buf, &start, &end, FALSE);

  /* All tags to save */
  const char *tags[] = {
    "name", "first", "last",
    "address", "city", "state", "zip", "country",
    "phone", "fax", "mobile", "web",
    "title", "company",
    "address2", "city2", "state2", "zip2", "country2",
    "phone2", "fax2", "mobile2", "web2",
    "otheremail", "otherphone", "otherweb",
    "note", "picture", NULL
  };
  const char *values[] = {
    gtk_editable_get_text(GTK_EDITABLE(AB.full_name)),
    gtk_editable_get_text(GTK_EDITABLE(AB.first_name)),
    gtk_editable_get_text(GTK_EDITABLE(AB.last_name)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_address)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_city)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_state)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_zip)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_country)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_phone)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_fax)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_mobile)),
    gtk_editable_get_text(GTK_EDITABLE(AB.h_web)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_title)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_company)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_address)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_city)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_state)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_zip)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_country)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_phone)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_fax)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_mobile)),
    gtk_editable_get_text(GTK_EDITABLE(AB.w_web)),
    gtk_editable_get_text(GTK_EDITABLE(AB.other_email)),
    gtk_editable_get_text(GTK_EDITABLE(AB.other_phone)),
    gtk_editable_get_text(GTK_EDITABLE(AB.other_web)),
    note_text,
    AB.photo_path ? AB.photo_path : "",
    NULL
  };

  /* Build notes string from tags — start fresh (no old notes to preserve) */
  g_free(e->notes);
  e->notes = set_tagged_fields("", tags, values);
  g_free(note_text);

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
    nf = g_strdup(path);
    nf->label = g_strdup("Eudora Nicknames");
    nf->next = AB.files;
    AB.files = nf;
  }

  /* Generate unique "New Nickname" name */
  char name[64];
  int n = 0;
  for (NickEntry *check = nf->entries; check; check = check->next) {
    if (strncmp(spec_name(check), "New Nickname", 12) == 0)
      n++;
  }
  if (n == 0)
    snprintf(name, sizeof(name), "New Nickname");
  else
    snprintf(name, sizeof(name), "New Nickname %d", n + 1);

  NickEntry *e = g_new0(NickEntry, 1);

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

  int file_idx, nick_idx;
  if (!ab_get_selected(&file_idx, &nick_idx))
    return;

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
      g_free(spec_name(doomed));
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

  int file_idx, nick_idx;
  if (!ab_get_selected(&file_idx, &nick_idx))
    return;

  NickEntry *e = get_nick_by_indices(file_idx, nick_idx);
  if (!e || !e->addresses || !e->addresses[0])
    return;

  /* Copy address to clipboard with field indicator */
  GdkClipboard *clipboard =
      gdk_display_get_clipboard(gdk_display_get_default());
  gdk_clipboard_set_text(clipboard, e->addresses);

  g_print("Address Book: copied %s address for %s field\n", spec_name(e), field);
}

/* ─── Build form row: label + entry ──────────────────────────────────── */

static GtkWidget *form_row(const char *label_text, GtkWidget **out_entry) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *lbl = gtk_label_new(label_text);
  gtk_widget_set_size_request(lbl, 90, -1);
  gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
  *out_entry = gtk_entry_new();
  gtk_widget_set_hexpand(*out_entry, TRUE);
  gtk_box_append(GTK_BOX(row), lbl);
  gtk_box_append(GTK_BOX(row), *out_entry);
  return row;
}

/* ─── Photo support ─────────────────────────────────────────────────── */

static const char *get_photos_dir(void) {
  static char dir[1024] = {0};
  if (!dir[0]) {
    const char *nick_dir = get_nicknames_dir();
    /* Photos dir alongside Nicknames */
    char *slash = strrchr(nick_dir, '/');
    if (slash) {
      size_t len = slash - nick_dir;
      snprintf(dir, sizeof(dir), "%.*s/Photos", (int)len, nick_dir);
    } else {
      snprintf(dir, sizeof(dir), "%s/Photos", nick_dir);
    }
    g_mkdir_with_parents(dir, 0755);
  }
  return dir;
}

/* Copy image file to photos dir, converting to PNG for consistency */
static char *copy_photo_to_store(const char *src_path, const char *nick_name) {
  GdkTexture *tex = gdk_texture_new_from_filename(src_path, NULL);
  if (!tex) return NULL;

  /* Build destination path */
  char *safe_name = g_strdup(nick_name);
  for (char *c = safe_name; *c; c++)
    if (*c == '/' || *c == ' ') *c = '_';

  char dest[1024];
  snprintf(dest, sizeof(dest), "%s/%s.png", get_photos_dir(), safe_name);
  g_free(safe_name);

  /* Save as PNG */
  GBytes *png_bytes = gdk_texture_save_to_png_bytes(tex);
  g_object_unref(tex);
  if (!png_bytes) return NULL;

  gsize size;
  const void *data = g_bytes_get_data(png_bytes, &size);
  FILE *fp = fopen(dest, "wb");
  if (fp) {
    fwrite(data, 1, size, fp);
    fclose(fp);
  }
  g_bytes_unref(png_bytes);

  return g_strdup(dest);
}

static void display_photo(const char *path) {
  if (path && path[0] && g_file_test(path, G_FILE_TEST_EXISTS)) {
    GdkTexture *tex = gdk_texture_new_from_filename(path, NULL);
    if (tex) {
      GdkPaintable *paintable = GDK_PAINTABLE(tex);
      gtk_picture_set_paintable(GTK_PICTURE(AB.photo_image), paintable);
      g_object_unref(tex);
    } else {
      gtk_picture_set_paintable(GTK_PICTURE(AB.photo_image), NULL);
    }
  } else {
    gtk_picture_set_paintable(GTK_PICTURE(AB.photo_image), NULL);
  }
}

static void on_photo_file_response(GObject *source, GAsyncResult *res,
                                    gpointer user_data) {
  (void)user_data;
  GtkFileDialog *dlg = GTK_FILE_DIALOG(source);
  GFile *file = gtk_file_dialog_open_finish(dlg, res, NULL);
  if (!file) return;

  char *path = g_file_get_path(file);
  g_object_unref(file);
  if (!path) return;

  /* Get current nickname name for the photo filename */
  const char *nick = gtk_editable_get_text(GTK_EDITABLE(AB.nick_entry));
  if (!nick || !nick[0]) nick = "unnamed";

  char *stored = copy_photo_to_store(path, nick);
  g_free(path);
  if (!stored) return;

  g_free(AB.photo_path);
  AB.photo_path = stored;
  display_photo(stored);
}

static void on_photo_select(GtkButton *btn, gpointer user_data) {
  (void)btn; (void)user_data;
  GtkFileDialog *dlg = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dlg, "Select Contact Photo");

  /* Filter for images */
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Images");
  gtk_file_filter_add_mime_type(filter, "image/*");
  GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
  g_list_store_append(filters, filter);
  gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));
  g_object_unref(filter);
  g_object_unref(filters);

  GtkWidget *toplevel = gtk_widget_get_root(GTK_WIDGET(btn));
  gtk_file_dialog_open(dlg, GTK_WINDOW(toplevel), NULL,
                       on_photo_file_response, NULL);
  g_object_unref(dlg);
}

static void on_photo_remove(GtkButton *btn, gpointer user_data) {
  (void)btn; (void)user_data;
  g_free(AB.photo_path);
  AB.photo_path = NULL;
  gtk_picture_set_paintable(GTK_PICTURE(AB.photo_image), NULL);
}

/* Create a tab label with icon + text */
static GtkWidget *ab_tab_label(const char *icon_name, const char *text) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 14);
  gtk_box_append(GTK_BOX(box), icon);
  gtk_box_append(GTK_BOX(box), gtk_label_new(text));
  return box;
}

/* Build all 6 tabs into AB.notebook. Assumes AB.notebook is already created. */
static void build_ab_tabs(void) {
  int margin = 8;

  /* ── Tab 1: Personal ── */
  GtkWidget *p_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_add_css_class(p_box, "ab-tab-content");
  gtk_widget_set_margin_start(p_box, margin);
  gtk_widget_set_margin_end(p_box, margin);
  gtk_widget_set_margin_top(p_box, margin);
  gtk_widget_set_margin_bottom(p_box, margin);

  gtk_box_append(GTK_BOX(p_box), form_row("Full Name:", &AB.full_name));
  gtk_box_append(GTK_BOX(p_box), form_row("First Name:", &AB.first_name));
  gtk_box_append(GTK_BOX(p_box), form_row("Last Name:", &AB.last_name));

  /* Address (expansion) in personal tab */
  GtkWidget *addr_lbl = gtk_label_new("Address(es):");
  gtk_label_set_xalign(GTK_LABEL(addr_lbl), 0);
  gtk_box_append(GTK_BOX(p_box), addr_lbl);
  GtkWidget *addr_scroll = gtk_scrolled_window_new();
  AB.addr_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(AB.addr_view), GTK_WRAP_WORD_CHAR);
  gtk_widget_set_vexpand(addr_scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(addr_scroll), AB.addr_view);
  gtk_box_append(GTK_BOX(p_box), addr_scroll);

  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), p_box,
                           ab_tab_label("avatar-default-symbolic", "Personal"));

  /* ── Tab 2: Home ── */
  GtkWidget *h_scroll = gtk_scrolled_window_new();
  gtk_widget_add_css_class(h_scroll, "ab-tab-content");
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(h_scroll),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  GtkWidget *h_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(h_box, margin);
  gtk_widget_set_margin_end(h_box, margin);
  gtk_widget_set_margin_top(h_box, margin);
  gtk_widget_set_margin_bottom(h_box, margin);

  gtk_box_append(GTK_BOX(h_box), form_row("Address:", &AB.h_address));
  gtk_box_append(GTK_BOX(h_box), form_row("City:", &AB.h_city));
  gtk_box_append(GTK_BOX(h_box), form_row("State:", &AB.h_state));
  gtk_box_append(GTK_BOX(h_box), form_row("Zip:", &AB.h_zip));
  gtk_box_append(GTK_BOX(h_box), form_row("Country:", &AB.h_country));
  gtk_box_append(GTK_BOX(h_box), form_row("Phone:", &AB.h_phone));
  gtk_box_append(GTK_BOX(h_box), form_row("Fax:", &AB.h_fax));
  gtk_box_append(GTK_BOX(h_box), form_row("Mobile:", &AB.h_mobile));
  gtk_box_append(GTK_BOX(h_box), form_row("Web:", &AB.h_web));

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(h_scroll), h_box);
  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), h_scroll,
                           ab_tab_label("go-home-symbolic", "Home"));

  /* ── Tab 3: Work ── */
  GtkWidget *w_scroll = gtk_scrolled_window_new();
  gtk_widget_add_css_class(w_scroll, "ab-tab-content");
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w_scroll),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  GtkWidget *w_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(w_box, margin);
  gtk_widget_set_margin_end(w_box, margin);
  gtk_widget_set_margin_top(w_box, margin);
  gtk_widget_set_margin_bottom(w_box, margin);

  gtk_box_append(GTK_BOX(w_box), form_row("Title:", &AB.w_title));
  gtk_box_append(GTK_BOX(w_box), form_row("Company:", &AB.w_company));
  gtk_box_append(GTK_BOX(w_box), form_row("Address:", &AB.w_address));
  gtk_box_append(GTK_BOX(w_box), form_row("City:", &AB.w_city));
  gtk_box_append(GTK_BOX(w_box), form_row("State:", &AB.w_state));
  gtk_box_append(GTK_BOX(w_box), form_row("Zip:", &AB.w_zip));
  gtk_box_append(GTK_BOX(w_box), form_row("Country:", &AB.w_country));
  gtk_box_append(GTK_BOX(w_box), form_row("Phone:", &AB.w_phone));
  gtk_box_append(GTK_BOX(w_box), form_row("Fax:", &AB.w_fax));
  gtk_box_append(GTK_BOX(w_box), form_row("Mobile:", &AB.w_mobile));
  gtk_box_append(GTK_BOX(w_box), form_row("Web:", &AB.w_web));

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w_scroll), w_box);
  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), w_scroll,
                           ab_tab_label("x-office-document-symbolic", "Work"));

  /* ── Tab 4: Other ── */
  GtkWidget *o_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_add_css_class(o_box, "ab-tab-content");
  gtk_widget_set_margin_start(o_box, margin);
  gtk_widget_set_margin_end(o_box, margin);
  gtk_widget_set_margin_top(o_box, margin);
  gtk_widget_set_margin_bottom(o_box, margin);

  gtk_box_append(GTK_BOX(o_box), form_row("Other Email:", &AB.other_email));
  gtk_box_append(GTK_BOX(o_box), form_row("Other Phone:", &AB.other_phone));
  gtk_box_append(GTK_BOX(o_box), form_row("Other Web:", &AB.other_web));

  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), o_box,
                           ab_tab_label("view-more-symbolic", "Other"));

  /* ── Tab 5: Notes ── */
  GtkWidget *n_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_add_css_class(n_box, "ab-tab-content");
  gtk_widget_set_margin_start(n_box, margin);
  gtk_widget_set_margin_end(n_box, margin);
  gtk_widget_set_margin_top(n_box, margin);
  gtk_widget_set_margin_bottom(n_box, margin);

  GtkWidget *notes_scroll = gtk_scrolled_window_new();
  AB.notes_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(AB.notes_view), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(notes_scroll, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(notes_scroll), AB.notes_view);
  gtk_box_append(GTK_BOX(n_box), notes_scroll);

  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), n_box,
                           ab_tab_label("accessories-text-editor-symbolic", "Notes"));

  /* ── Tab 6: Photo ── */
  GtkWidget *ph_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_add_css_class(ph_box, "ab-tab-content");
  gtk_widget_set_margin_start(ph_box, margin);
  gtk_widget_set_margin_end(ph_box, margin);
  gtk_widget_set_margin_top(ph_box, margin);
  gtk_widget_set_margin_bottom(ph_box, margin);

  /* Photo display */
  AB.photo_image = gtk_picture_new();
  gtk_picture_set_content_fit(GTK_PICTURE(AB.photo_image), GTK_CONTENT_FIT_CONTAIN);
  gtk_widget_set_size_request(AB.photo_image, 200, 200);
  gtk_widget_set_vexpand(AB.photo_image, TRUE);
  gtk_widget_set_halign(AB.photo_image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(AB.photo_image, GTK_ALIGN_CENTER);

  GtkWidget *ph_frame = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(ph_frame), AB.photo_image);
  gtk_widget_set_vexpand(ph_frame, TRUE);
  gtk_box_append(GTK_BOX(ph_box), ph_frame);

  /* Buttons */
  GtkWidget *ph_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(ph_btns, GTK_ALIGN_CENTER);
  AB.photo_btn = gtk_button_new_with_label("Select Photo...");
  AB.photo_remove = gtk_button_new_with_label("Remove");
  g_signal_connect(AB.photo_btn, "clicked", G_CALLBACK(on_photo_select), NULL);
  g_signal_connect(AB.photo_remove, "clicked", G_CALLBACK(on_photo_remove), NULL);
  gtk_box_append(GTK_BOX(ph_btns), AB.photo_btn);
  gtk_box_append(GTK_BOX(ph_btns), AB.photo_remove);
  gtk_box_append(GTK_BOX(ph_box), ph_btns);

  GtkWidget *ph_hint = gtk_label_new("Supports JPEG, PNG, GIF, WebP, TIFF, SVG, HEIF, AVIF");
  gtk_widget_add_css_class(ph_hint, "dim-label");
  gtk_box_append(GTK_BOX(ph_box), ph_hint);

  gtk_notebook_append_page(GTK_NOTEBOOK(AB.notebook), ph_box,
                           ab_tab_label("camera-photo-symbolic", "Photo"));
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

  /* Left: list box showing address books and nicknames */
  AB.list_view = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(AB.list_view), GTK_SELECTION_SINGLE);
  g_signal_connect(AB.list_view, "row-selected",
                   G_CALLBACK(on_ab_selection_changed), NULL);

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

  /* Tabbed notebook — 6 tabs matching original Mac Eudora */
  AB.notebook = gtk_notebook_new();
  gtk_widget_set_vexpand(AB.notebook, TRUE);
  build_ab_tabs();
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

  /* void *window close */
  g_signal_connect(AB.window, "close-request",
                   G_CALLBACK(on_ab_window_close), NULL);

  AB.inited = true;
  gtk_window_present(GTK_WINDOW(AB.window));
}

/************************************************************************
 * CreateAddressBookPanel — embeddable panel for the main notebook
 ************************************************************************/
GtkWidget *CreateAddressBookPanel(void) {
  load_nick_files();

  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(outer, "ab-panel");

  /* ── Hero header ── */
  GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_add_css_class(hero, "ab-hero");

  GtkWidget *hero_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_hexpand(hero_vbox, TRUE);
  GtkWidget *hero_title = gtk_label_new("Address Book");
  gtk_widget_add_css_class(hero_title, "ab-hero-title");
  gtk_label_set_xalign(GTK_LABEL(hero_title), 0);
  gtk_box_append(GTK_BOX(hero_vbox), hero_title);

  GtkWidget *hero_sub = gtk_label_new("Manage contacts and mailing lists");
  gtk_widget_add_css_class(hero_sub, "ab-hero-sub");
  gtk_label_set_xalign(GTK_LABEL(hero_sub), 0);
  gtk_box_append(GTK_BOX(hero_vbox), hero_sub);
  gtk_box_append(GTK_BOX(hero), hero_vbox);

  /* Contact count pill */
  int total = 0;
  for (NickFile *nf = AB.files; nf; nf = nf->next)
    for (NickEntry *e = nf->entries; e; e = e->next) total++;
  char count_str[32];
  snprintf(count_str, sizeof(count_str), "%d contacts", total);
  GtkWidget *count_pill = gtk_label_new(count_str);
  gtk_widget_add_css_class(count_pill, "ab-count-pill");
  gtk_widget_set_valign(count_pill, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(hero), count_pill);

  gtk_box_append(GTK_BOX(outer), hero);

  /* ── Toolbar ── */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(toolbar, 8);
  gtk_widget_set_margin_end(toolbar, 8);
  gtk_widget_set_margin_top(toolbar, 6);
  gtk_widget_set_margin_bottom(toolbar, 4);

  GtkWidget *btn_new = gtk_button_new_with_label("New");
  GtkWidget *btn_del = gtk_button_new_with_label("Delete");
  GtkWidget *btn_save = gtk_button_new_with_label("Save");
  g_signal_connect(btn_new, "clicked", G_CALLBACK(on_ab_new_clicked), NULL);
  g_signal_connect(btn_del, "clicked", G_CALLBACK(on_ab_delete_clicked), NULL);
  g_signal_connect_swapped(btn_save, "clicked",
                           G_CALLBACK(save_current_entry), NULL);
  gtk_box_append(GTK_BOX(toolbar), btn_new);
  gtk_box_append(GTK_BOX(toolbar), btn_del);

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(toolbar), spacer);

  GtkWidget *btn_to = gtk_button_new_with_label("To:");
  GtkWidget *btn_cc = gtk_button_new_with_label("Cc:");
  GtkWidget *btn_bcc = gtk_button_new_with_label("Bcc:");
  g_signal_connect(btn_to, "clicked", G_CALLBACK(on_ab_recipient_clicked),
                   (gpointer)"to");
  g_signal_connect(btn_cc, "clicked", G_CALLBACK(on_ab_recipient_clicked),
                   (gpointer)"cc");
  g_signal_connect(btn_bcc, "clicked", G_CALLBACK(on_ab_recipient_clicked),
                   (gpointer)"bcc");
  gtk_box_append(GTK_BOX(toolbar), btn_to);
  gtk_box_append(GTK_BOX(toolbar), btn_cc);
  gtk_box_append(GTK_BOX(toolbar), btn_bcc);
  gtk_box_append(GTK_BOX(toolbar), btn_save);

  gtk_box_append(GTK_BOX(outer), toolbar);

  /* ── Paned: contact list left, detail right ── */
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(hpaned), 240);
  gtk_widget_set_vexpand(hpaned, TRUE);

  /* Left: list box */
  AB.list_view = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(AB.list_view), GTK_SELECTION_SINGLE);
  g_signal_connect(AB.list_view, "row-selected",
                   G_CALLBACK(on_ab_selection_changed), NULL);

  GtkWidget *list_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), AB.list_view);
  gtk_paned_set_start_child(GTK_PANED(hpaned), list_scroll);
  gtk_paned_set_resize_start_child(GTK_PANED(hpaned), FALSE);
  gtk_paned_set_shrink_start_child(GTK_PANED(hpaned), FALSE);

  /* Right: detail */
  GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(right_box, 10);
  gtk_widget_set_margin_end(right_box, 10);
  gtk_widget_set_margin_top(right_box, 6);
  gtk_widget_set_margin_bottom(right_box, 6);

  /* Nickname name */
  GtkWidget *name_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *name_label = gtk_label_new("Nickname:");
  gtk_widget_set_size_request(name_label, 80, -1);
  AB.nick_entry = gtk_entry_new();
  gtk_widget_set_hexpand(AB.nick_entry, TRUE);
  gtk_box_append(GTK_BOX(name_row), name_label);
  gtk_box_append(GTK_BOX(name_row), AB.nick_entry);
  gtk_box_append(GTK_BOX(right_box), name_row);

  /* Tabbed notebook — 6 tabs matching original Mac Eudora */
  AB.notebook = gtk_notebook_new();
  gtk_widget_set_vexpand(AB.notebook, TRUE);
  build_ab_tabs();
  gtk_box_append(GTK_BOX(right_box), AB.notebook);

  gtk_paned_set_end_child(GTK_PANED(hpaned), right_box);
  gtk_paned_set_resize_end_child(GTK_PANED(hpaned), TRUE);
  gtk_paned_set_shrink_end_child(GTK_PANED(hpaned), FALSE);

  gtk_box_append(GTK_BOX(outer), hpaned);

  ab_fill_list();
  AB.inited = true;

  return outer;
}
