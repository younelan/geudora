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
        FSSpec spec;
        memset(&spec, 0, sizeof(FSSpec));
        strncpy(spec.path, mbx_path, sizeof(spec.path) - 1);
        TOCType * toc = BuildTOC(&spec);
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
      FSSpec spec;
      memset(&spec, 0, sizeof(FSSpec));
      strncpy(spec.path, mailbox_path, sizeof(spec.path) - 1);
      TOCType * toc = BuildTOC(&spec);
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
  FSSpec spec;
  memset(&spec, 0, sizeof(FSSpec));
  strncpy(spec.path, mailbox_path, sizeof(spec.path) - 1);
  TOCType * toc = BuildTOC(&spec);
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
static const char *mailbox_icon_name(const char *name, const char *path) {
  if (!name) return "folder";
  if (g_strcmp0(name, "In") == 0)     return "mail-inbox";
  if (g_strcmp0(name, "Out") == 0)    return "mail-outbox";
  if (g_strcmp0(name, "Trash") == 0)  return "user-trash";
  if (g_strcmp0(name, "Junk") == 0)   return "edit-delete";
  if (g_strcmp0(name, "Drafts") == 0) return "document-edit";
  /* Check if path is a directory (folder) */
  if (path && g_file_test(path, G_FILE_TEST_IS_DIR))
    return "folder";
  return "mail-unread";  /* generic mailbox icon */
}

/* Cell data function: sets icon based on mailbox name */
static void mailbox_icon_cell_data(GtkTreeViewColumn *col,
                                   GtkCellRenderer *cell,
                                   GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer data) {
  (void)col; (void)data;
  gchar *name = NULL, *path = NULL;
  gtk_tree_model_get(model, iter, 0, &name, 1, &path, -1);
  g_object_set(cell, "icon-name", mailbox_icon_name(name, path), NULL);
  g_free(name);
  g_free(path);
}

/* Cell data function: renders name with unread pill like original Eudora.
 * Unread mailboxes show bold name + " (N)" count suffix. */
static void mailbox_name_cell_data(GtkTreeViewColumn *col,
                                   GtkCellRenderer *cell,
                                   GtkTreeModel *model,
                                   GtkTreeIter *iter,
                                   gpointer data) {
  (void)col; (void)data;
  gchar *name = NULL;
  int unread = 0;
  gtk_tree_model_get(model, iter, 0, &name, 3, &unread, -1);

  if (unread > 0) {
    gchar *markup = g_markup_printf_escaped(
        "<b>%s</b>  <span size=\"small\" background=\"#4a90d9\""
        " foreground=\"white\"> %d </span>", name, unread);
    g_object_set(cell, "markup", markup, NULL);
    g_free(markup);
  } else {
    g_object_set(cell, "markup", name, "weight", PANGO_WEIGHT_NORMAL, NULL);
  }
  g_free(name);
}

/* Rename callback: when user edits a mailbox name in the tree */
static void on_mailbox_name_edited(GtkCellRendererText *cell,
                                    const gchar *path_str,
                                    const gchar *new_name,
                                    gpointer user_data) {
  (void)cell;
  GtkTreeView *tree = GTK_TREE_VIEW(user_data);
  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter_from_string(model, &iter, path_str))
    return;

  gchar *old_path = NULL, *old_name = NULL;
  gtk_tree_model_get(model, &iter, 0, &old_name, 1, &old_path, -1);

  if (!old_path || !new_name || *new_name == '\0' ||
      g_strcmp0(old_name, new_name) == 0) {
    g_free(old_path); g_free(old_name);
    return;
  }

  /* Don't rename standard mailboxes */
  if (old_name && (g_strcmp0(old_name, "In") == 0 ||
                   g_strcmp0(old_name, "Out") == 0 ||
                   g_strcmp0(old_name, "Trash") == 0 ||
                   g_strcmp0(old_name, "Junk") == 0 ||
                   g_strcmp0(old_name, "Drafts") == 0)) {
    g_free(old_path); g_free(old_name);
    return;
  }

  /* Build new path: same parent directory, new name */
  gchar *parent_dir = g_path_get_dirname(old_path);
  gchar *new_path = g_build_filename(parent_dir, new_name, NULL);

  if (g_file_test(new_path, G_FILE_TEST_EXISTS)) {
    g_warning("Cannot rename: \"%s\" already exists", new_name);
  } else {
    g_rename(old_path, new_path);
    /* Also rename .toc file if it exists */
    gchar *old_toc = g_strdup_printf("%s.toc", old_path);
    gchar *new_toc = g_strdup_printf("%s.toc", new_path);
    if (g_file_test(old_toc, G_FILE_TEST_EXISTS))
      g_rename(old_toc, new_toc);
    g_free(old_toc);
    g_free(new_toc);

    /* Reload entire tree so children get updated paths */
    gtk_mailbox_tree_load(GTK_WIDGET(tree));
  }

  g_free(parent_dir);
  g_free(new_path);
  g_free(old_path);
  g_free(old_name);
}

/* ── Internal drag-and-drop: move mailboxes/folders into other folders ── */

/* Store dragged row's filesystem path */
static gchar *dnd_src_path = NULL;
static gchar *dnd_src_name = NULL;

static GdkContentProvider *on_drag_prepare(GtkDragSource *source,
                                            double x, double y,
                                            gpointer user_data) {
  (void)source;
  GtkTreeView *tree = GTK_TREE_VIEW(user_data);
  GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_tree_selection_get_selected(sel, &model, &iter))
    return NULL;

  g_free(dnd_src_path); dnd_src_path = NULL;
  g_free(dnd_src_name); dnd_src_name = NULL;
  gtk_tree_model_get(model, &iter, 0, &dnd_src_name, 1, &dnd_src_path, -1);

  /* Don't drag standard mailboxes */
  if (dnd_src_name && (g_strcmp0(dnd_src_name, "In") == 0 ||
                       g_strcmp0(dnd_src_name, "Out") == 0 ||
                       g_strcmp0(dnd_src_name, "Trash") == 0 ||
                       g_strcmp0(dnd_src_name, "Junk") == 0 ||
                       g_strcmp0(dnd_src_name, "Drafts") == 0)) {
    g_free(dnd_src_path); dnd_src_path = NULL;
    g_free(dnd_src_name); dnd_src_name = NULL;
    return NULL;
  }

  GValue val = G_VALUE_INIT;
  g_value_init(&val, G_TYPE_STRING);
  g_value_set_string(&val, dnd_src_path);
  GdkContentProvider *cp = gdk_content_provider_new_for_value(&val);
  g_value_unset(&val);
  return cp;
}

static gboolean on_drop(GtkDropTarget *target, const GValue *value,
                         double x, double y, gpointer user_data) {
  (void)target; (void)value;
  GtkTreeView *tree = GTK_TREE_VIEW(user_data);

  if (!dnd_src_path || !dnd_src_name)
    return FALSE;

  /* Find which row we dropped onto */
  GtkTreePath *path = NULL;
  GtkTreeViewDropPosition pos;
  if (!gtk_tree_view_get_dest_row_at_pos(tree, (int)x, (int)y, &path, &pos))
    return FALSE;

  GtkTreeModel *model = gtk_tree_view_get_model(tree);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path)) {
    gtk_tree_path_free(path);
    return FALSE;
  }

  gchar *dest_fs_path = NULL;
  gtk_tree_model_get(model, &iter, 1, &dest_fs_path, -1);
  gtk_tree_path_free(path);

  /* Determine target directory */
  gchar *target_dir = NULL;
  if (dest_fs_path && g_file_test(dest_fs_path, G_FILE_TEST_IS_DIR))
    target_dir = g_strdup(dest_fs_path);
  else if (dest_fs_path)
    target_dir = g_path_get_dirname(dest_fs_path);
  g_free(dest_fs_path);

  if (!target_dir)
    return FALSE;

  gchar *new_path = g_build_filename(target_dir, dnd_src_name, NULL);

  if (g_strcmp0(dnd_src_path, new_path) != 0 &&
      !g_file_test(new_path, G_FILE_TEST_EXISTS)) {
    g_rename(dnd_src_path, new_path);
    gchar *old_toc = g_strdup_printf("%s.toc", dnd_src_path);
    gchar *new_toc = g_strdup_printf("%s.toc", new_path);
    if (g_file_test(old_toc, G_FILE_TEST_EXISTS))
      g_rename(old_toc, new_toc);
    g_free(old_toc);
    g_free(new_toc);
    gtk_mailbox_tree_load(GTK_WIDGET(tree));
  }

  g_free(new_path);
  g_free(target_dir);
  g_free(dnd_src_path); dnd_src_path = NULL;
  g_free(dnd_src_name); dnd_src_name = NULL;
  return TRUE;
}

/* Create mailbox tree view — single column with icon + name + unread pill,
 * matching the original Mac Eudora mailbox browser layout. */
GtkWidget *gtk_mailbox_tree_new(void) {
  /* Tree store: [0]=name, [1]=path, [2]=message_count, [3]=unread_count */
  GtkTreeStore *store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING,
                                           G_TYPE_INT, G_TYPE_INT);

  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  /* Use fixed height mode so tree doesn't request excessive width */
  gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(tree), TRUE);

  /* Single combined column: icon + name (like original Eudora) */
  GtkTreeViewColumn *col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);

  /* Icon renderer — icon chosen by cell data func based on mailbox name */
  GtkCellRenderer *icon_r = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(col, icon_r, FALSE);
  gtk_tree_view_column_set_cell_data_func(col, icon_r,
      mailbox_icon_cell_data, NULL, NULL);

  /* Name renderer — editable for rename, with unread pill */
  GtkCellRenderer *text_r = gtk_cell_renderer_text_new();
  g_object_set(text_r, "ellipsize", PANGO_ELLIPSIZE_END,
                        "editable", TRUE, NULL);
  g_signal_connect(text_r, "edited",
                   G_CALLBACK(on_mailbox_name_edited), tree);
  gtk_tree_view_column_pack_start(col, text_r, TRUE);
  gtk_tree_view_column_set_cell_data_func(col, text_r,
                                          mailbox_name_cell_data, NULL, NULL);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

  /* Enable drag-and-drop: drag mailboxes/folders into folders */
  GtkDragSource *drag_src = gtk_drag_source_new();
  gtk_drag_source_set_actions(drag_src, GDK_ACTION_MOVE);
  g_signal_connect(drag_src, "prepare", G_CALLBACK(on_drag_prepare), tree);
  gtk_widget_add_controller(tree, GTK_EVENT_CONTROLLER(drag_src));

  GtkDropTarget *drop_tgt = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
  g_signal_connect(drop_tgt, "drop", G_CALLBACK(on_drop), tree);
  gtk_widget_add_controller(tree, GTK_EVENT_CONTROLLER(drop_tgt));

  return tree;
}

/* Recursively load a directory into the tree store under parent_iter */
static void load_directory(GtkTreeStore *store, GtkTreeIter *parent_iter,
                           const gchar *dir_path) {
  GDir *dir = g_dir_open(dir_path, 0, NULL);
  if (!dir) return;

  const gchar *filename;
  GHashTable *seen =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  while ((filename = g_dir_read_name(dir)) != NULL) {
    if (g_str_has_prefix(filename, ".") || g_str_has_suffix(filename, ".toc"))
      continue;

    gchar *full_path = g_build_filename(dir_path, filename, NULL);
    gboolean is_d = g_file_test(full_path, G_FILE_TEST_IS_DIR);
    gboolean is_f = g_file_test(full_path, G_FILE_TEST_IS_REGULAR);

    if (!is_f && !is_d) { g_free(full_path); continue; }

    /* Handle split segments */
    gchar *base_name = g_strdup(filename);
    if (g_str_has_suffix(base_name, ".001"))
      base_name[strlen(base_name) - 4] = '\0';

    if (g_hash_table_contains(seen, base_name)) {
      g_free(base_name); g_free(full_path); continue;
    }
    g_hash_table_insert(seen, g_strdup(base_name), GINT_TO_POINTER(1));
    g_free(base_name);

    /* Build TOC for mailbox files if needed */
    if (is_f) {
      gchar *toc_path = g_strdup_printf("%s.toc", full_path);
      if (!g_file_test(toc_path, G_FILE_TEST_EXISTS)) {
        g_print("Building TOC for mailbox: %s\n", full_path);
        TOCType *toc = BuildTOC(full_path);
        if (toc) { toc_save(toc); toc_free(toc); }
      }
      g_free(toc_path);
    }

    int msg_count = is_f ? gtk_mailbox_get_message_count(full_path) : 0;
    int unread = is_f ? gtk_mailbox_get_unread_count(full_path) : 0;

    GtkTreeIter iter;
    gtk_tree_store_append(store, &iter, parent_iter);
    gtk_tree_store_set(store, &iter,
                       0, filename, 1, full_path,
                       2, msg_count, 3, unread, -1);

    /* Recurse into directories */
    if (is_d)
      load_directory(store, &iter, full_path);

    g_free(full_path);
  }

  g_hash_table_destroy(seen);
  g_dir_close(dir);
}

/* Load mailboxes into tree view — recursive, like Mac Eudora */
void gtk_mailbox_tree_load(GtkWidget *tree) {
  if (!GTK_IS_TREE_VIEW(tree))
    return;

  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  GtkTreeStore *store = GTK_TREE_STORE(model);

  gtk_tree_store_clear(store);

  gchar *mailboxes_dir = get_mailboxes_dir();
  load_directory(store, NULL, mailboxes_dir);
  g_free(mailboxes_dir);

  /* Expand all rows so folder contents are visible */
  gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));
}

/* Refresh mailbox tree */
void gtk_mailbox_tree_refresh(GtkWidget *tree) { gtk_mailbox_tree_load(tree); }
