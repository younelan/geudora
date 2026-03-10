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
        TOCHandle toc = BuildTOC(&spec);
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
      TOCHandle toc = BuildTOC(&spec);
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
  TOCHandle toc = toc_load(mailbox_path);
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
  TOCHandle toc = toc_load(mailbox_path);
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
  TOCHandle toc = BuildTOC(&spec);
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
  TOCHandle toc = toc_load(mailbox_path);
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
  TOCHandle from_toc = toc_load(from_path);
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

/* Create mailbox tree view */
GtkWidget *gtk_mailbox_tree_new(void) {
  /* Create tree store: name, path, message_count, unread_count */
  GtkTreeStore *store = gtk_tree_store_new(4, G_TYPE_STRING, /* name */
                                           G_TYPE_STRING,    /* path */
                                           G_TYPE_INT,       /* message_count */
                                           G_TYPE_INT);      /* unread_count */

  /* Create tree view */
  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store);

  /* Add columns */
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
      "Mailbox", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  /* Add message count column */
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Messages", renderer,
                                                    "text", 2, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  return tree;
}

/* Load mailboxes into tree view - exact Mac Eudora logic */
void gtk_mailbox_tree_load(GtkWidget *tree) {
  if (!GTK_IS_TREE_VIEW(tree))
    return;

  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  GtkTreeStore *store = GTK_TREE_STORE(model);

  /* Clear existing items */
  gtk_tree_store_clear(store);

  gchar *mailboxes_dir = get_mailboxes_dir();
  GDir *dir = g_dir_open(mailboxes_dir, 0, NULL);

  if (!dir) {
    g_warning("Failed to open mailboxes directory: %s", mailboxes_dir);
    g_free(mailboxes_dir);
    return;
  }

  const gchar *filename;
  GtkTreeIter iter;
  GList *mailboxes = NULL;
  GHashTable *seen =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  /* Collect mailbox files first */
  while ((filename = g_dir_read_name(dir)) != NULL) {
    /* Skip hidden files and .toc files */
    if (g_str_has_prefix(filename, ".") || g_str_has_suffix(filename, ".toc")) {
      continue;
    }

    gchar *mailbox_path = g_build_filename(mailboxes_dir, filename, NULL);

    /* Only add regular files (mailboxes), not directories */
    if (g_file_test(mailbox_path, G_FILE_TEST_IS_REGULAR)) {
      /* Check if this is a split mailbox segment (.001, .002, etc.) */
      gchar *base_name = g_strdup(filename);
      if (g_str_has_suffix(base_name, ".001")) {
        /* Remove .001 to get base name */
        base_name[strlen(base_name) - 4] = '\0';
      }

      /* Only process first segment or non-split mailboxes */
      if (!g_hash_table_contains(seen, base_name)) {
        g_hash_table_insert(seen, g_strdup(base_name), GINT_TO_POINTER(1));

        /* Check if TOC exists, if not build it */
        gchar *toc_path = g_strdup_printf("%s.toc", mailbox_path);
        if (!g_file_test(toc_path, G_FILE_TEST_EXISTS)) {
          g_print("Building TOC for mailbox: %s\n", mailbox_path);
          FSSpec spec;
          memset(&spec, 0, sizeof(FSSpec));
          strncpy(spec.path, mailbox_path, sizeof(spec.path) - 1);
          TOCHandle toc = BuildTOC(&spec);
          if (toc) {
            toc_save(toc);
            toc_free(toc);
          }
        }
        g_free(toc_path);

        mailboxes = g_list_append(mailboxes, mailbox_path);
      } else {
        g_free(mailbox_path);
      }

      g_free(base_name);
    } else {
      g_free(mailbox_path);
    }
  }

  g_dir_close(dir);
  g_hash_table_destroy(seen);
  g_free(mailboxes_dir);

  /* Now add mailboxes to tree (after freeing mailboxes_dir) */
  for (GList *item = mailboxes; item; item = item->next) {
    gchar *mailbox_path = (gchar *)item->data;
    gchar *basename = g_path_get_basename(mailbox_path);

    int message_count = gtk_mailbox_get_message_count(mailbox_path);
    int unread_count = gtk_mailbox_get_unread_count(mailbox_path);

    gtk_tree_store_append(store, &iter, NULL);
    gtk_tree_store_set(store, &iter, 0, basename, 1, mailbox_path, 2,
                       message_count, 3, unread_count, -1);

    g_free(basename);
    g_free(mailbox_path);
  }

  g_list_free(mailboxes);
}

/* Refresh mailbox tree */
void gtk_mailbox_tree_refresh(GtkWidget *tree) { gtk_mailbox_tree_load(tree); }
