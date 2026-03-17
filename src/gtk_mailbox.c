/*
 * GTK4 Mailbox Management for gEudora
 * Ported from Mac Eudora mailbox.c with GTK4 adaptations
 *
 * Mailbox format (exact Mac Eudora logic):
 * - Mailboxes are files (not directories)
 * - Each mailbox has a corresponding .toc file (Table of Contents)
 * - Messages in mailbox are separated by "From " lines (sendmail format)
 * - TOC file contains binary message summaries
 */

#include "gtk_mailbox.h"
#include "buildtoc.h"
#include "gtk_prefs.h"
#include "toc.h"
#include <pango/pango.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

/* Default mailbox names - exact Mac Eudora names */
static const gchar *DEFAULT_MAILBOXES[] = {"In",   "Out",    "Trash",
                                           "Junk", "Drafts", NULL};

/* Get the mailboxes directory path */
static gchar *get_mailboxes_dir(void) {
  /* Don't free this - it's managed by prefs system */
  return g_strdup(prefs_get_mailboxes_path());
}

/* Create default mailboxes if they don't exist */
void gtk_mailbox_create_default(void) {
  gchar *mailboxes_dir = get_mailboxes_dir();

  /* Create mailboxes directory if it doesn't exist */
  if (g_mkdir_with_parents(mailboxes_dir, 0755) != 0) {
    g_warning("Failed to create mailboxes directory: %s", mailboxes_dir);
    g_free(mailboxes_dir);
    return;
  }

  /* Create default mailboxes - exact Mac Eudora logic */
  for (int i = 0; DEFAULT_MAILBOXES[i] != NULL; i++) {
    gchar *mailbox_name = (gchar *)DEFAULT_MAILBOXES[i];
    gchar *mbx_path = g_build_filename(mailboxes_dir, mailbox_name, NULL);

    /* Create mailbox file if it doesn't exist */
    if (!g_file_test(mbx_path, G_FILE_TEST_EXISTS)) {
      /* Create empty mailbox file (no extension) */
      FILE *f = fopen(mbx_path, "wb");
      if (f) {
        fclose(f);
        g_print("Created mailbox file: %s\n", mbx_path);

        /* Build TOC for this mailbox - creates .toc file */
        TOCType *toc = BuildTOC(mbx_path);
        if (toc) {
          toc_save(toc);
          toc_free(toc);
          g_print("Created TOC for mailbox: %s\n", mailbox_name);
        }
      } else {
        g_warning("Failed to create mailbox file: %s", mbx_path);
      }
    }

    g_free(mbx_path);
  }

  g_free(mailboxes_dir);
}

/* Create a new mailbox - exact Mac Eudora logic */
gboolean gtk_mailbox_create(const gchar *name, gboolean is_folder) {
  if (!name || strlen(name) == 0) {
    g_warning("Invalid mailbox name");
    return FALSE;
  }

  gchar *mailboxes_dir = get_mailboxes_dir();
  gchar *mailbox_path = g_build_filename(mailboxes_dir, name, NULL);

  gboolean success = FALSE;

  if (!g_file_test(mailbox_path, G_FILE_TEST_EXISTS)) {
    /* Create empty mailbox file (not directory) */
    FILE *f = fopen(mailbox_path, "wb");
    if (f) {
      fclose(f);
      g_print("Created mailbox file: %s\n", mailbox_path);

      /* Build TOC for this mailbox */
      TOCType *toc = BuildTOC(mailbox_path);
      if (toc) {
        toc_save(toc);
        toc_free(toc);
        g_print("Created TOC for mailbox: %s\n", name);
        success = TRUE;
      }
    } else {
      g_warning("Failed to create mailbox file: %s", mailbox_path);
    }
  } else {
    g_warning("Mailbox already exists: %s", name);
  }

  g_free(mailbox_path);
  g_free(mailboxes_dir);

  return success;
}

/* Delete a mailbox - exact Mac Eudora logic */
gboolean gtk_mailbox_delete(const gchar *path) {
  if (!path)
    return FALSE;

  /* Don't allow deleting special mailboxes */
  gchar *basename = g_path_get_basename(path);
  for (int i = 0; DEFAULT_MAILBOXES[i] != NULL; i++) {
    if (g_strcmp0(basename, DEFAULT_MAILBOXES[i]) == 0) {
      g_warning("Cannot delete special mailbox: %s", basename);
      g_free(basename);
      return FALSE;
    }
  }
  g_free(basename);

  /* Delete mailbox file */
  if (g_unlink(path) == 0) {
    g_print("Deleted mailbox: %s\n", path);

    /* Also delete corresponding TOC file */
    gchar *toc_path = g_strdup_printf("%s.toc", path);
    if (g_file_test(toc_path, G_FILE_TEST_EXISTS)) {
      g_unlink(toc_path);
      g_print("Deleted TOC: %s\n", toc_path);
    }
    g_free(toc_path);

    return TRUE;
  } else {
    g_warning("Failed to delete mailbox: %s", path);
    return FALSE;
  }
}

/* Rename a mailbox */
gboolean gtk_mailbox_rename(const gchar *old_path, const gchar *new_name) {
  if (!old_path || !new_name)
    return FALSE;

  gchar *mailboxes_dir = get_mailboxes_dir();
  gchar *new_path = g_build_filename(mailboxes_dir, new_name, NULL);

  gboolean success = FALSE;

  if (g_rename(old_path, new_path) == 0) {
    g_print("Renamed mailbox to: %s\n", new_name);
    success = TRUE;
  } else {
    g_warning("Failed to rename mailbox");
  }

  g_free(new_path);
  g_free(mailboxes_dir);

  return success;
}

/* Get mailbox path by name */
gchar *gtk_mailbox_get_path(const gchar *name) {
  if (!name)
    return NULL;

  gchar *mailboxes_dir = get_mailboxes_dir();
  gchar *path = g_build_filename(mailboxes_dir, name, NULL);

  g_free(mailboxes_dir);

  return path;
}

/* Get TOC (table of contents) path for a mailbox */
gchar *gtk_mailbox_get_toc_path(const gchar *name) {
  if (!name)
    return NULL;

  gchar *mailboxes_dir = get_mailboxes_dir();
  gchar *mbx_path = g_build_filename(mailboxes_dir, name, NULL);
  /* TOC file is mailbox_name.toc */
  gchar *toc_path = g_strdup_printf("%s.toc", mbx_path);

  g_free(mbx_path);
  g_free(mailboxes_dir);

  return toc_path;
}

/* Get message count in a mailbox - read from TOC file */
int gtk_mailbox_get_message_count(const gchar *mailbox_path) {
  if (!mailbox_path)
    return 0;

  /* Load TOC and get message count */
  TOCType * toc = toc_load(mailbox_path);
  if (!toc)
    return 0;

  int count = toc_get_message_count(toc);
  toc_free(toc);

  return count;
}

/* Get unread message count - read from TOC file */
int gtk_mailbox_get_unread_count(const gchar *mailbox_path) {
  if (!mailbox_path)
    return 0;

  /* Load TOC and get unread count */
  TOCType * toc = toc_load(mailbox_path);
  if (!toc)
    return 0;

  int count = toc_get_unread_count(toc);
  toc_free(toc);

  return count;
}

/* Add a message to a mailbox - append to mailbox file with From separator */
void gtk_mailbox_add_message(const gchar *mailbox_path,
                             const gchar *message_data) {
  if (!mailbox_path || !message_data)
    return;

  /* Append message to mailbox file with From separator */
  FILE *f = fopen(mailbox_path, "ab");
  if (!f) {
    g_warning("Failed to open mailbox for appending: %s", mailbox_path);
    return;
  }

  /* Write From separator line (sendmail format) */
  fprintf(f, "From sender@example.com %ld\n", time(NULL));

  /* Write message data */
  fputs(message_data, f);

  /* Ensure message ends with newline */
  if (message_data[strlen(message_data) - 1] != '\n') {
    fputc('\n', f);
  }

  fclose(f);

  /* Rebuild TOC to include new message */
  TOCType *toc = BuildTOC(mailbox_path);
  if (toc) {
    toc_save(toc);
    toc_free(toc);
  }

  g_print("Message added to mailbox: %s\n", mailbox_path);
}

/* Delete a message from a mailbox - mark as deleted in TOC */
void gtk_mailbox_delete_message(const gchar *mailbox_path, int message_id) {
  if (!mailbox_path)
    return;

  /* Load TOC, mark message as deleted, and save */
  TOCType * toc = toc_load(mailbox_path);
  if (!toc)
    return;

  MessageSummary *msg = toc_get_message(toc, message_id);
  if (msg) {
    msg->flags |= MSG_FLAG_DELETED;
    toc_save(toc);
    g_print("Message marked as deleted: %s[%d]\n", mailbox_path, message_id);
  }

  toc_free(toc);
}

/* Transfer a message between mailboxes - copy to destination, mark deleted in
 * source */
void gtk_mailbox_transfer_message(const gchar *from_path, const gchar *to_path,
                                  int message_id) {
  if (!from_path || !to_path)
    return;

  /* Load source TOC to get message data */
  TOCType * from_toc = toc_load(from_path);
  if (!from_toc)
    return;

  MessageSummary *msg = toc_get_message(from_toc, message_id);
  if (!msg) {
    toc_free(from_toc);
    return;
  }

  /* Read message from source mailbox file */
  FILE *f = fopen(from_path, "rb");
  if (!f) {
    g_warning("Failed to open source mailbox: %s", from_path);
    toc_free(from_toc);
    return;
  }

  /* Seek to message offset and read message data */
  fseek(f, msg->offset, SEEK_SET);
  gchar *message_data = g_malloc(msg->length + 1);
  size_t read_size = fread(message_data, 1, msg->length, f);
  fclose(f);

  if (read_size != msg->length) {
    g_warning("Failed to read complete message from source");
    g_free(message_data);
    toc_free(from_toc);
    return;
  }
  message_data[msg->length] = '\0';

  /* Append message to destination mailbox */
  gtk_mailbox_add_message(to_path, message_data);

  /* Mark message as deleted in source */
  msg->flags |= MSG_FLAG_DELETED;
  toc_save(from_toc);

  g_print("Message transferred from %s to %s\n", from_path, to_path);

  g_free(message_data);
  toc_free(from_toc);
}

/* Get mailbox by name - exact Mac Eudora logic */
GtkMailbox *gtk_mailbox_get_by_name(const gchar *name) {
  if (!name)
    return NULL;

  gchar *path = gtk_mailbox_get_path(name);

  /* Check if mailbox file exists */
  if (!g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
    g_free(path);
    return NULL;
  }

  GtkMailbox *mailbox = g_new0(GtkMailbox, 1);
  mailbox->name = g_strdup(name);
  mailbox->path = path;
  mailbox->toc_path = gtk_mailbox_get_toc_path(name);
  mailbox->message_count = gtk_mailbox_get_message_count(path);
  mailbox->unread_count = gtk_mailbox_get_unread_count(path);

  /* Check if it's a special mailbox */
  for (int i = 0; DEFAULT_MAILBOXES[i] != NULL; i++) {
    if (g_strcmp0(name, DEFAULT_MAILBOXES[i]) == 0) {
      mailbox->is_special = TRUE;
      break;
    }
  }

  return mailbox;
}

/* Get mailbox by path */
GtkMailbox *gtk_mailbox_get_by_path(const gchar *path) {
  if (!path)
    return NULL;

  gchar *basename = g_path_get_basename(path);
  GtkMailbox *mailbox = gtk_mailbox_get_by_name(basename);

  g_free(basename);

  return mailbox;
}

/* Free mailbox structure */
void gtk_mailbox_free(GtkMailbox *mailbox) {
  if (!mailbox)
    return;

  g_free(mailbox->name);
  g_free(mailbox->path);
  g_free(mailbox->toc_path);
  g_free(mailbox);
}

/* Icon name for a mailbox based on its name (matches original Eudora icons) */
static const char *mailbox_icon_name(const char *name, gboolean is_dir) {
  if (!name) return "folder-symbolic";
  if (g_strcmp0(name, "In") == 0)     return "mail-inbox-symbolic";
  if (g_strcmp0(name, "Out") == 0)    return "mail-outbox-symbolic";
  if (g_strcmp0(name, "Trash") == 0)  return "user-trash-symbolic";
  if (g_strcmp0(name, "Junk") == 0)   return "edit-delete-symbolic";
  if (g_strcmp0(name, "Drafts") == 0) return "document-edit-symbolic";
  if (is_dir) return "folder-symbolic";
  return "mail-unread-symbolic";
}

/* GObject data keys for mailbox rows */
#define MB_KEY_PATH  "mb-path"
#define MB_KEY_NAME  "mb-name"
#define MB_KEY_IS_DIR "mb-is-dir"

/* ── Drag and drop callbacks ──────────────────────────────────────── */

/* Prepare drag data: provide the mailbox path as a string */
static GdkContentProvider *on_mb_drag_prepare(GtkDragSource *source,
                                               double x, double y,
                                               gpointer ud) {
  (void)source; (void)x; (void)y;
  GtkWidget *row = GTK_WIDGET(ud);
  const char *path = g_object_get_data(G_OBJECT(row), MB_KEY_PATH);
  if (!path) return NULL;
  GValue val = G_VALUE_INIT;
  g_value_init(&val, G_TYPE_STRING);
  g_value_set_string(&val, path);
  return gdk_content_provider_new_for_value(&val);
}

/* Visual feedback when drag starts */
static void on_mb_drag_begin(GtkDragSource *source, GdkDrag *drag,
                              gpointer ud) {
  (void)source; (void)drag;
  GtkWidget *row = GTK_WIDGET(ud);
  gtk_widget_set_opacity(row, 0.4);
}

/* Drop onto a folder row: move the mailbox file into that folder */
static gboolean on_mb_drop(GtkDropTarget *target, const GValue *value,
                            double x, double y, gpointer ud) {
  (void)target; (void)x; (void)y;
  GtkWidget *folder_row = GTK_WIDGET(ud);
  const char *folder_path = g_object_get_data(G_OBJECT(folder_row), MB_KEY_PATH);
  if (!folder_path || !G_VALUE_HOLDS_STRING(value)) return FALSE;

  const char *src_path = g_value_get_string(value);
  if (!src_path || !*src_path) return FALSE;

  gchar *basename = g_path_get_basename(src_path);
  gchar *dest_path = g_build_filename(folder_path, basename, NULL);

  gboolean ok = FALSE;
  if (g_strcmp0(src_path, dest_path) != 0 &&
      !g_file_test(dest_path, G_FILE_TEST_EXISTS)) {
    if (g_rename(src_path, dest_path) == 0) {
      /* Move .toc too */
      gchar *src_toc = g_strdup_printf("%s.toc", src_path);
      gchar *dst_toc = g_strdup_printf("%s.toc", dest_path);
      if (g_file_test(src_toc, G_FILE_TEST_EXISTS))
        g_rename(src_toc, dst_toc);
      g_free(src_toc);
      g_free(dst_toc);
      ok = TRUE;
    }
  }

  g_free(basename);
  g_free(dest_path);

  if (ok) {
    /* Find the listbox and refresh */
    GtkWidget *listbox = gtk_widget_get_ancestor(folder_row, GTK_TYPE_LIST_BOX);
    if (listbox) gtk_mailbox_tree_refresh(listbox);
  }
  return ok;
}

/* Highlight folder on drag enter */
static GdkDragAction on_mb_drop_enter(GtkDropTarget *target, double x, double y,
                                       gpointer ud) {
  (void)target; (void)x; (void)y;
  GtkWidget *row = GTK_WIDGET(ud);
  gtk_widget_add_css_class(row, "mb-drop-target");
  return GDK_ACTION_MOVE;
}

/* Remove highlight on drag leave */
static void on_mb_drop_leave(GtkDropTarget *target, gpointer ud) {
  (void)target;
  GtkWidget *row = GTK_WIDGET(ud);
  gtk_widget_remove_css_class(row, "mb-drop-target");
}

/* ── Drop target on the listbox itself: drop to root ─────────────── */

static gboolean on_mb_drop_root(GtkDropTarget *target, const GValue *value,
                                 double x, double y, gpointer ud) {
  (void)target; (void)x; (void)y;
  GtkWidget *listbox = GTK_WIDGET(ud);
  if (!G_VALUE_HOLDS_STRING(value)) return FALSE;

  const char *src_path = g_value_get_string(value);
  if (!src_path || !*src_path) return FALSE;

  gchar *mailboxes_dir = get_mailboxes_dir();
  gchar *basename = g_path_get_basename(src_path);
  gchar *dest_path = g_build_filename(mailboxes_dir, basename, NULL);

  gboolean ok = FALSE;
  if (g_strcmp0(src_path, dest_path) != 0 &&
      !g_file_test(dest_path, G_FILE_TEST_EXISTS)) {
    if (g_rename(src_path, dest_path) == 0) {
      gchar *src_toc = g_strdup_printf("%s.toc", src_path);
      gchar *dst_toc = g_strdup_printf("%s.toc", dest_path);
      if (g_file_test(src_toc, G_FILE_TEST_EXISTS))
        g_rename(src_toc, dst_toc);
      g_free(src_toc);
      g_free(dst_toc);
      ok = TRUE;
    }
  }

  g_free(basename);
  g_free(dest_path);
  g_free(mailboxes_dir);

  if (ok) gtk_mailbox_tree_refresh(listbox);
  return ok;
}

/* ── Create a single mailbox row widget ────────────────────────────── */

static GtkWidget *make_mb_row(const char *name, const char *path,
                               gboolean is_dir, int unread, int depth) {
  GtkWidget *row = gtk_list_box_row_new();
  g_object_set_data_full(G_OBJECT(row), MB_KEY_PATH, g_strdup(path), g_free);
  g_object_set_data_full(G_OBJECT(row), MB_KEY_NAME, g_strdup(name), g_free);
  g_object_set_data(G_OBJECT(row), MB_KEY_IS_DIR, GINT_TO_POINTER(is_dir));

  /* Folders are selectable (for delete/rename) but not activatable
   * (double-click doesn't open them as a mailbox) */
  if (is_dir) {
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
  }

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(hbox, 6 + depth * 16);
  gtk_widget_set_margin_end(hbox, 6);
  gtk_widget_set_margin_top(hbox, 3);
  gtk_widget_set_margin_bottom(hbox, 3);

  /* Icon */
  GtkWidget *icon = gtk_image_new_from_icon_name(mailbox_icon_name(name, is_dir));
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
  gtk_widget_add_css_class(icon, "mb-icon");
  gtk_box_append(GTK_BOX(hbox), icon);

  /* Name label */
  GtkWidget *lbl = gtk_label_new(name);
  gtk_label_set_xalign(GTK_LABEL(lbl), 0);
  gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(lbl, TRUE);
  if (is_dir)
    gtk_widget_add_css_class(lbl, "mb-folder");
  else if (unread > 0)
    gtk_widget_add_css_class(lbl, "mb-unread");
  else
    gtk_widget_add_css_class(lbl, "mb-name");
  gtk_box_append(GTK_BOX(hbox), lbl);

  /* Unread pill — right-aligned, only if > 0 */
  if (unread > 0) {
    char pill_text[16];
    snprintf(pill_text, sizeof(pill_text), "%d", unread);
    GtkWidget *pill = gtk_label_new(pill_text);
    gtk_widget_add_css_class(pill, "mb-pill");
    gtk_widget_set_halign(pill, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(hbox), pill);
  }

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), hbox);

  /* ── Drag source: mailbox files (not folders, not special) can be dragged ── */
  if (!is_dir) {
    GtkDragSource *drag = gtk_drag_source_new();
    gtk_drag_source_set_actions(drag, GDK_ACTION_MOVE);
    g_signal_connect(drag, "prepare", G_CALLBACK(on_mb_drag_prepare), row);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(on_mb_drag_begin), row);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(drag));
  }

  /* ── Drop target: folders accept mailbox drops ── */
  if (is_dir) {
    GtkDropTarget *drop = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
    g_signal_connect(drop, "drop", G_CALLBACK(on_mb_drop), row);
    g_signal_connect(drop, "enter", G_CALLBACK(on_mb_drop_enter), row);
    g_signal_connect(drop, "leave", G_CALLBACK(on_mb_drop_leave), row);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(drop));
  }

  return row;
}

/* ── Recursive directory loading into list box ───────────────────── */

/* Fixed ordering for special mailboxes — always shown first at root */
static const char *SPECIAL_ORDER[] = {"In", "Out", "Drafts", "Trash", "Junk", NULL};

static gboolean is_special_mailbox(const char *name) {
  for (int i = 0; SPECIAL_ORDER[i]; i++)
    if (g_strcmp0(name, SPECIAL_ORDER[i]) == 0) return TRUE;
  return FALSE;
}

static void load_directory_lb(GtkWidget *listbox, const gchar *dir_path,
                               int depth) {
  GDir *dir = g_dir_open(dir_path, 0, NULL);
  if (!dir) return;

  const gchar *filename;
  GHashTable *seen =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  /* At root level, add special mailboxes first in fixed order */
  if (depth == 0) {
    for (int i = 0; SPECIAL_ORDER[i]; i++) {
      gchar *full_path = g_build_filename(dir_path, SPECIAL_ORDER[i], NULL);
      if (g_file_test(full_path, G_FILE_TEST_IS_REGULAR)) {
        int unread = gtk_mailbox_get_unread_count(full_path);
        GtkWidget *row = make_mb_row(SPECIAL_ORDER[i], full_path, FALSE, unread, 0);
        gtk_list_box_append(GTK_LIST_BOX(listbox), row);
        g_hash_table_insert(seen, g_strdup(SPECIAL_ORDER[i]), GINT_TO_POINTER(1));
      }
      g_free(full_path);
    }
  }

  /* Collect non-special entries, then sort alphabetically (folders first) */
  GPtrArray *dirs  = g_ptr_array_new_with_free_func(g_free);
  GPtrArray *files = g_ptr_array_new_with_free_func(g_free);

  while ((filename = g_dir_read_name(dir)) != NULL) {
    if (g_str_has_prefix(filename, ".") || g_str_has_suffix(filename, ".toc"))
      continue;

    gchar *full_path = g_build_filename(dir_path, filename, NULL);
    gboolean is_d = g_file_test(full_path, G_FILE_TEST_IS_DIR);
    gboolean is_f = g_file_test(full_path, G_FILE_TEST_IS_REGULAR);

    if (!is_f && !is_d) { g_free(full_path); continue; }

    /* void *split segments */
    gchar *base_name = g_strdup(filename);
    if (g_str_has_suffix(base_name, ".001"))
      base_name[strlen(base_name) - 4] = '\0';

    if (g_hash_table_contains(seen, base_name)) {
      g_free(base_name); g_free(full_path); continue;
    }
    g_hash_table_insert(seen, g_strdup(base_name), GINT_TO_POINTER(1));
    g_free(base_name);

    if (is_d)
      g_ptr_array_add(dirs, g_strdup(filename));
    else
      g_ptr_array_add(files, g_strdup(filename));

    g_free(full_path);
  }

  g_hash_table_destroy(seen);
  g_dir_close(dir);

  /* Sort both arrays alphabetically (case-insensitive) */
  g_ptr_array_sort(dirs,  (GCompareFunc)g_ascii_strcasecmp);
  g_ptr_array_sort(files, (GCompareFunc)g_ascii_strcasecmp);

  /* Add folders first, then files — both sorted */
  for (guint i = 0; i < dirs->len; i++) {
    const char *name = g_ptr_array_index(dirs, i);
    gchar *full_path = g_build_filename(dir_path, name, NULL);
    GtkWidget *row = make_mb_row(name, full_path, TRUE, 0, depth);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    load_directory_lb(listbox, full_path, depth + 1);
    g_free(full_path);
  }

  for (guint i = 0; i < files->len; i++) {
    const char *name = g_ptr_array_index(files, i);
    gchar *full_path = g_build_filename(dir_path, name, NULL);

    /* Build TOC if needed */
    gchar *toc_path = g_strdup_printf("%s.toc", full_path);
    if (!g_file_test(toc_path, G_FILE_TEST_EXISTS)) {
      TOCType *toc = BuildTOC(full_path);
      if (toc) { toc_save(toc); toc_free(toc); }
    }
    g_free(toc_path);

    int unread = gtk_mailbox_get_unread_count(full_path);
    GtkWidget *row = make_mb_row(name, full_path, FALSE, unread, depth);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    g_free(full_path);
  }

  g_ptr_array_free(dirs, TRUE);
  g_ptr_array_free(files, TRUE);
}

/* Create mailbox list — modern GtkListBox with custom row widgets,
 * matching a professional mail app sidebar layout. */
GtkWidget *gtk_mailbox_tree_new(void) {
  GtkWidget *listbox = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
  gtk_widget_add_css_class(listbox, "mb-sidebar");

  /* Drop target on the listbox background — moves mailbox to root */
  GtkDropTarget *root_drop = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
  g_signal_connect(root_drop, "drop", G_CALLBACK(on_mb_drop_root), listbox);
  gtk_widget_add_controller(listbox, GTK_EVENT_CONTROLLER(root_drop));

  return listbox;
}

/* Load mailboxes into list box */
void gtk_mailbox_tree_load(GtkWidget *listbox) {
  if (!GTK_IS_LIST_BOX(listbox))
    return;

  /* Clear existing rows */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(listbox)) != NULL)
    gtk_list_box_remove(GTK_LIST_BOX(listbox), child);

  gchar *mailboxes_dir = get_mailboxes_dir();
  load_directory_lb(listbox, mailboxes_dir, 0);
  g_free(mailboxes_dir);
}

/* Refresh mailbox tree */
void gtk_mailbox_tree_refresh(GtkWidget *tree) { gtk_mailbox_tree_load(tree); }
