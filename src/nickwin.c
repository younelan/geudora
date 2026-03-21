/*
 * nickwin.c - Address Book window, ported from Mac Eudora's nickwin.c.
 *
 * Data layer uses macmbx address book API exclusively.
 * UI is GTK4 with tabs, forms, photo support, CSS classes, hero header.
 */

#include "mailbox.h"
#include "mydefs.h"
#include "macmbx.h"
#include "gtk_autocomplete.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern const char *prefs_get_mailboxes_path(void);
extern MacmbxAddressBooks *get_address_books(void);

/* ─── macmbx-backed field helpers ──────────────────────────────────── */

/* Get a note field from macmbx nickname. Returns value (do not free) or "". */
static const char *mb_get_field(MacmbxAddressBook *book, int nick_idx, const char *field) {
  const char *v = macmbx_nick_get_field(book, nick_idx, field);
  return v ? v : "";
}

/* ─── Address Book window state ──────────────────────────────────────── */

typedef struct {
  GtkWidget *window;
  GtkWidget *list_view;     /* GtkListBox for nicknames */
  GtkWidget *nick_entry;    /* Nickname name field */
  GtkWidget *addr_view;     /* Address text view */
  GtkWidget *notes_view;    /* Notes text view */
  GtkWidget *notebook;      /* Tab notebook */
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
#define AB_KEY_BOOK_IDX "ab-book-idx"
#define AB_KEY_NICK_IDX "ab-nick-idx"

/* ─── Populate list box ──────────────────────────────────────────────── */

static void ab_fill_list(void) {
  /* Clear existing rows */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(AB.list_view)) != NULL)
    gtk_list_box_remove(GTK_LIST_BOX(AB.list_view), child);

  MacmbxAddressBooks *abs = get_address_books();
  if (!abs) return;

  for (int bi = 0; bi < abs->count; bi++) {
    MacmbxAddressBook *book = macmbx_nick_get_book(abs, bi);
    if (!book) continue;

    /* Section header row (non-selectable) */
    GtkWidget *hdr_label = gtk_label_new(book->name);
    gtk_label_set_xalign(GTK_LABEL(hdr_label), 0);
    gtk_widget_add_css_class(hdr_label, "ab-section-title");
    GtkWidget *hdr_row = gtk_list_box_row_new();
    gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(hdr_row), FALSE);
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(hdr_row), FALSE);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(hdr_row), hdr_label);
    g_object_set_data(G_OBJECT(hdr_row), AB_KEY_IS_FILE, GINT_TO_POINTER(1));
    g_object_set_data(G_OBJECT(hdr_row), AB_KEY_BOOK_IDX, GINT_TO_POINTER(bi));
    gtk_list_box_append(GTK_LIST_BOX(AB.list_view), hdr_row);

    for (int ni = 0; ni < book->count; ni++) {
      if (book->entries[ni].deleted) continue;

      GtkWidget *nick_label = gtk_label_new(book->entries[ni].name);
      gtk_label_set_xalign(GTK_LABEL(nick_label), 0);
      gtk_widget_set_margin_start(nick_label, 12);
      GtkWidget *nick_row = gtk_list_box_row_new();
      gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(nick_row), nick_label);
      g_object_set_data(G_OBJECT(nick_row), AB_KEY_IS_FILE, GINT_TO_POINTER(0));
      g_object_set_data(G_OBJECT(nick_row), AB_KEY_BOOK_IDX, GINT_TO_POINTER(bi));
      g_object_set_data(G_OBJECT(nick_row), AB_KEY_NICK_IDX, GINT_TO_POINTER(ni));
      gtk_list_box_append(GTK_LIST_BOX(AB.list_view), nick_row);
    }
  }
}

/* Get book_idx and nick_idx from the currently selected list box row.
 * Returns false if nothing is selected or a file header is selected. */
static bool ab_get_selected(int *book_idx_out, int *nick_idx_out) {
  GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(AB.list_view));
  if (!row) return false;
  int is_file = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_IS_FILE));
  if (is_file) return false;
  *book_idx_out = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_BOOK_IDX));
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

/* ─── Helper: set entry text from macmbx field ───────────────────────── */

static void set_entry_from_macmbx(GtkWidget *entry, MacmbxAddressBook *book,
                                   int nick_idx, const char *field) {
  gtk_editable_set_text(GTK_EDITABLE(entry), mb_get_field(book, nick_idx, field));
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

  int book_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_BOOK_IDX));
  int nick_idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), AB_KEY_NICK_IDX));

  MacmbxAddressBooks *abs = get_address_books();
  if (!abs) return;
  MacmbxAddressBook *book = macmbx_nick_get_book(abs, book_idx);
  if (!book || nick_idx < 0 || nick_idx >= book->count) return;

  /* Nickname name */
  gtk_editable_set_text(GTK_EDITABLE(AB.nick_entry), book->entries[nick_idx].name);

  /* Addresses */
  const char *addrs = macmbx_nick_get_addresses(book, nick_idx);
  GtkTextBuffer *addr_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  gtk_text_buffer_set_text(addr_buf, addrs ? addrs : "", -1);

  /* Personal */
  set_entry_from_macmbx(AB.full_name,  book, nick_idx, "name");
  set_entry_from_macmbx(AB.first_name, book, nick_idx, "first");
  set_entry_from_macmbx(AB.last_name,  book, nick_idx, "last");

  /* Home */
  set_entry_from_macmbx(AB.h_address, book, nick_idx, "address");
  set_entry_from_macmbx(AB.h_city,    book, nick_idx, "city");
  set_entry_from_macmbx(AB.h_state,   book, nick_idx, "state");
  set_entry_from_macmbx(AB.h_zip,     book, nick_idx, "zip");
  set_entry_from_macmbx(AB.h_country, book, nick_idx, "country");
  set_entry_from_macmbx(AB.h_phone,   book, nick_idx, "phone");
  set_entry_from_macmbx(AB.h_fax,     book, nick_idx, "fax");
  set_entry_from_macmbx(AB.h_mobile,  book, nick_idx, "mobile");
  set_entry_from_macmbx(AB.h_web,     book, nick_idx, "web");

  /* Work */
  set_entry_from_macmbx(AB.w_title,   book, nick_idx, "title");
  set_entry_from_macmbx(AB.w_company, book, nick_idx, "company");
  set_entry_from_macmbx(AB.w_address, book, nick_idx, "address2");
  set_entry_from_macmbx(AB.w_city,    book, nick_idx, "city2");
  set_entry_from_macmbx(AB.w_state,   book, nick_idx, "state2");
  set_entry_from_macmbx(AB.w_zip,     book, nick_idx, "zip2");
  set_entry_from_macmbx(AB.w_country, book, nick_idx, "country2");
  set_entry_from_macmbx(AB.w_phone,   book, nick_idx, "phone2");
  set_entry_from_macmbx(AB.w_fax,     book, nick_idx, "fax2");
  set_entry_from_macmbx(AB.w_mobile,  book, nick_idx, "mobile2");
  set_entry_from_macmbx(AB.w_web,     book, nick_idx, "web2");

  /* Other */
  set_entry_from_macmbx(AB.other_email, book, nick_idx, "otheremail");
  set_entry_from_macmbx(AB.other_phone, book, nick_idx, "otherphone");
  set_entry_from_macmbx(AB.other_web,   book, nick_idx, "otherweb");

  /* Notes */
  GtkTextBuffer *notes_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  gtk_text_buffer_set_text(notes_buf, mb_get_field(book, nick_idx, "note"), -1);

  /* Photo */
  g_free(AB.photo_path);
  const char *pic = macmbx_nick_get_field(book, nick_idx, "picture");
  AB.photo_path = (pic && pic[0]) ? g_strdup(pic) : NULL;
  display_photo(AB.photo_path);
}

/* ─── Save current entry back to macmbx ──────────────────────────────── */

static void save_field_from_entry(MacmbxAddressBook *book, int nick_idx,
                                   const char *field, GtkWidget *entry) {
  const char *val = gtk_editable_get_text(GTK_EDITABLE(entry));
  macmbx_nick_set_field(book, nick_idx, field, val ? val : "");
}

static void save_current_entry(void) {
  int book_idx, nick_idx;
  if (!ab_get_selected(&book_idx, &nick_idx))
    return;

  MacmbxAddressBooks *abs = get_address_books();
  if (!abs) return;
  MacmbxAddressBook *book = macmbx_nick_get_book(abs, book_idx);
  if (!book || nick_idx < 0 || nick_idx >= book->count) return;

  /* Update nickname name (rename) */
  const char *new_name = gtk_editable_get_text(GTK_EDITABLE(AB.nick_entry));
  if (new_name && new_name[0] && strcmp(book->entries[nick_idx].name, new_name) != 0) {
    macmbx_nick_rename(book, nick_idx, new_name);
    ab_update_selected_label(new_name);
  }

  /* Update addresses */
  GtkTextBuffer *addr_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.addr_view));
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(addr_buf, &start, &end);
  char *addr_text = gtk_text_buffer_get_text(addr_buf, &start, &end, FALSE);
  macmbx_nick_set_addresses(book, nick_idx, addr_text ? addr_text : "");
  g_free(addr_text);

  /* Personal */
  save_field_from_entry(book, nick_idx, "name",  AB.full_name);
  save_field_from_entry(book, nick_idx, "first", AB.first_name);
  save_field_from_entry(book, nick_idx, "last",  AB.last_name);

  /* Home */
  save_field_from_entry(book, nick_idx, "address", AB.h_address);
  save_field_from_entry(book, nick_idx, "city",    AB.h_city);
  save_field_from_entry(book, nick_idx, "state",   AB.h_state);
  save_field_from_entry(book, nick_idx, "zip",     AB.h_zip);
  save_field_from_entry(book, nick_idx, "country", AB.h_country);
  save_field_from_entry(book, nick_idx, "phone",   AB.h_phone);
  save_field_from_entry(book, nick_idx, "fax",     AB.h_fax);
  save_field_from_entry(book, nick_idx, "mobile",  AB.h_mobile);
  save_field_from_entry(book, nick_idx, "web",     AB.h_web);

  /* Work */
  save_field_from_entry(book, nick_idx, "title",    AB.w_title);
  save_field_from_entry(book, nick_idx, "company",  AB.w_company);
  save_field_from_entry(book, nick_idx, "address2", AB.w_address);
  save_field_from_entry(book, nick_idx, "city2",    AB.w_city);
  save_field_from_entry(book, nick_idx, "state2",   AB.w_state);
  save_field_from_entry(book, nick_idx, "zip2",     AB.w_zip);
  save_field_from_entry(book, nick_idx, "country2", AB.w_country);
  save_field_from_entry(book, nick_idx, "phone2",   AB.w_phone);
  save_field_from_entry(book, nick_idx, "fax2",     AB.w_fax);
  save_field_from_entry(book, nick_idx, "mobile2",  AB.w_mobile);
  save_field_from_entry(book, nick_idx, "web2",     AB.w_web);

  /* Other */
  save_field_from_entry(book, nick_idx, "otheremail", AB.other_email);
  save_field_from_entry(book, nick_idx, "otherphone", AB.other_phone);
  save_field_from_entry(book, nick_idx, "otherweb",   AB.other_web);

  /* Notes */
  GtkTextBuffer *notes_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(AB.notes_view));
  gtk_text_buffer_get_bounds(notes_buf, &start, &end);
  char *note_text = gtk_text_buffer_get_text(notes_buf, &start, &end, FALSE);
  macmbx_nick_set_field(book, nick_idx, "note", note_text ? note_text : "");
  g_free(note_text);

  /* Photo */
  macmbx_nick_set_field(book, nick_idx, "picture",
                         AB.photo_path ? AB.photo_path : "");

  /* Write back to disk */
  macmbx_nick_save_book(book);
}

/* ─── Button callbacks ───────────────────────────────────────────────── */

static void on_ab_new_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;

  MacmbxAddressBooks *abs = get_address_books();
  if (!abs || abs->count == 0) return;

  /* Add to the first book */
  MacmbxAddressBook *book = macmbx_nick_get_book(abs, 0);
  if (!book) return;

  /* Generate unique "New Nickname" name */
  char name[64];
  int n = 0;
  for (int i = 0; i < book->count; i++) {
    if (!book->entries[i].deleted &&
        strncmp(book->entries[i].name, "New Nickname", 12) == 0)
      n++;
  }
  if (n == 0)
    snprintf(name, sizeof(name), "New Nickname");
  else
    snprintf(name, sizeof(name), "New Nickname %d", n + 1);

  macmbx_nick_add(book, name, "");
  macmbx_nick_save_book(book);
  ab_fill_list();
}

static void on_ab_delete_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;

  int book_idx, nick_idx;
  if (!ab_get_selected(&book_idx, &nick_idx))
    return;

  MacmbxAddressBooks *abs = get_address_books();
  if (!abs) return;
  MacmbxAddressBook *book = macmbx_nick_get_book(abs, book_idx);
  if (!book) return;

  macmbx_nick_remove(book, nick_idx);
  macmbx_nick_save_book(book);
  ab_fill_list();
}

/* To/Cc/Bcc buttons — copy selected nickname's addresses to compose window */
static void on_ab_recipient_clicked(GtkButton *button, gpointer user_data) {
  const char *field = (const char *)user_data; /* "to", "cc", or "bcc" */
  (void)button;

  int book_idx, nick_idx;
  if (!ab_get_selected(&book_idx, &nick_idx))
    return;

  MacmbxAddressBooks *abs = get_address_books();
  if (!abs) return;
  MacmbxAddressBook *book = macmbx_nick_get_book(abs, book_idx);
  if (!book) return;

  const char *addrs = macmbx_nick_get_addresses(book, nick_idx);
  if (!addrs || !addrs[0])
    return;

  /* Copy address to clipboard with field indicator */
  GdkClipboard *clipboard =
      gdk_display_get_clipboard(gdk_display_get_default());
  gdk_clipboard_set_text(clipboard, addrs);

  g_print("Address Book: copied %s address for %s field\n",
          book->entries[nick_idx].name, field);
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
    MacmbxAddressBooks *abs = get_address_books();
    if (abs && abs->dir_path[0]) {
      /* Photos dir alongside Nicknames */
      char *slash = strrchr(abs->dir_path, '/');
      if (slash) {
        size_t len = (size_t)(slash - abs->dir_path);
        snprintf(dir, sizeof(dir), "%.*s/Photos", (int)len, abs->dir_path);
      } else {
        snprintf(dir, sizeof(dir), "%s/Photos", abs->dir_path);
      }
    } else {
      const char *home = g_get_home_dir();
      snprintf(dir, sizeof(dir), "%s/.local/share/geudora/Photos", home);
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

  /* Attach autocomplete to address fields */
  MacmbxAddressBooks *abs = get_address_books();
  if (abs) {
    gtk_autocomplete_attach(AB.nick_entry, abs);
    gtk_autocomplete_attach(AB.other_email, abs);
  }

  /* Window close */
  g_signal_connect(AB.window, "close-request",
                   G_CALLBACK(on_ab_window_close), NULL);

  AB.inited = true;
  gtk_window_present(GTK_WINDOW(AB.window));
}

/************************************************************************
 * CreateAddressBookPanel — embeddable panel for the main notebook
 ************************************************************************/
GtkWidget *CreateAddressBookPanel(void) {
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
  MacmbxAddressBooks *abs = get_address_books();
  if (abs) {
    for (int bi = 0; bi < abs->count; bi++) {
      MacmbxAddressBook *book = macmbx_nick_get_book(abs, bi);
      if (book) {
        for (int ni = 0; ni < book->count; ni++) {
          if (!book->entries[ni].deleted) total++;
        }
      }
    }
  }
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

  /* Attach autocomplete to address fields */
  if (abs) {
    gtk_autocomplete_attach(AB.nick_entry, abs);
    gtk_autocomplete_attach(AB.other_email, abs);
  }

  AB.inited = true;

  return outer;
}
