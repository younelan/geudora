/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/*
 * mbwin.c - GTK port of Eudora's mailbox browser window
 *
 * Original Carbon version used ListViews, ControlHandles, and WIND resources.
 * This GTK port uses GtkTreeView for the mailbox hierarchy and creates the
 * window through GetNewMyWindow() so it integrates with the wazoo system.
 */

#include "mailbox.h"
#include "message.h"
#include "mydefs.h"
#include "wazoo.h"

extern const char *prefs_get_mailboxes_path(void);
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>

/* First items in list (matching original) */
enum {
  kItemEudoraFolder = 1,
  kItemInBox,
  kItemOutBox,
  kItemJunkBox,
  kItemTrash,
  kItemMailBoxes
};

/* Tree store columns */
enum {
  COL_ICON_NAME,  /* icon name for GtkCellRendererPixbuf */
  COL_NAME,       /* display name */
  COL_PATH,       /* full path to mailbox file */
  COL_IS_FOLDER,  /* true if this is a folder, not a mailbox */
  COL_UNREAD,     /* unread message count */
  NUM_COLS
};

/* The MB window structure */
typedef struct {
  MyWindowPtr win;
  GtkWidget *tree_view;
  GtkTreeStore *store;
  GtkWidget *btn_new_mb;
  GtkWidget *btn_new_folder;
  GtkWidget *btn_remove;
  bool inited;
} MBType;

static MBType MB = {0};

/* Forward declarations */
static bool MBClose(MyWindowPtr win);
static void MBDidResize(MyWindowPtr win, Rect *oldContR);
static bool MBMenu(MyWindowPtr win, int menu, int item, short modifiers);
static void MBActivate(MyWindowPtr win);
static void on_new_mailbox_clicked(GtkButton *button, gpointer user_data);
static void on_new_folder_clicked(GtkButton *button, gpointer user_data);
static void on_remove_clicked(GtkButton *button, gpointer user_data);
void MBRefill(void);

/* Eudora mail directory */
static void ensure_mailbox_file(const char *path) {
  if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
    FILE *f = fopen(path, "w");
    if (f) fclose(f);
  }
}

static const char *get_eudora_mail_dir(void) {
  static char mail_dir[1024] = {0};
  if (!mail_dir[0]) {
    /* Use same directory as gtk_mailbox.c so both views show the same mailboxes */
    const char *prefs_path = prefs_get_mailboxes_path();
    if (prefs_path && prefs_path[0]) {
      snprintf(mail_dir, sizeof(mail_dir), "%s", prefs_path);
    } else {
      const char *home = g_get_home_dir();
      snprintf(mail_dir, sizeof(mail_dir), "%s/.local/share/geudora/mailboxes", home);
    }
    g_mkdir_with_parents(mail_dir, 0755);

    /* Create standard mailbox files if they don't exist (like original Eudora) */
    char path[1024];
    snprintf(path, sizeof(path), "%s/In", mail_dir);
    ensure_mailbox_file(path);
    snprintf(path, sizeof(path), "%s/Out", mail_dir);
    ensure_mailbox_file(path);
    snprintf(path, sizeof(path), "%s/Trash", mail_dir);
    ensure_mailbox_file(path);
    snprintf(path, sizeof(path), "%s/Junk", mail_dir);
    ensure_mailbox_file(path);
    snprintf(path, sizeof(path), "%s/Drafts", mail_dir);
    ensure_mailbox_file(path);
  }
  return mail_dir;
}

/**********************************************************************
 * scan_mailbox_dir - recursively scan a directory for mailbox files
 **********************************************************************/
static void scan_mailbox_dir(GtkTreeStore *store, GtkTreeIter *parent,
                             const char *dir_path) {
  DIR *dir = opendir(dir_path);
  if (!dir)
    return;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue; /* skip hidden files */

    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) != 0)
      continue;

    /* Skip .toc files */
    const char *ext = strrchr(entry->d_name, '.');
    if (ext && strcmp(ext, ".toc") == 0)
      continue;

    GtkTreeIter iter;
    if (S_ISDIR(st.st_mode)) {
      /* It's a folder — add it and recurse */
      gtk_tree_store_append(store, &iter, parent);
      gtk_tree_store_set(store, &iter,
                         COL_ICON_NAME, "folder",
                         COL_NAME, entry->d_name,
                         COL_PATH, full_path,
                         COL_IS_FOLDER, TRUE,
                         COL_UNREAD, 0,
                         -1);
      scan_mailbox_dir(store, &iter, full_path);
    } else if (S_ISREG(st.st_mode)) {
      /* It's a mailbox file */
      gtk_tree_store_append(store, &iter, parent);
      gtk_tree_store_set(store, &iter,
                         COL_ICON_NAME, "mail-unread",
                         COL_NAME, entry->d_name,
                         COL_PATH, full_path,
                         COL_IS_FOLDER, FALSE,
                         COL_UNREAD, 0,
                         -1);
    }
  }
  closedir(dir);
}

/**********************************************************************
 * mb_fill - populate the mailbox tree with standard Eudora mailboxes
 **********************************************************************/
static void mb_fill(GtkTreeStore *store) {
  const char *mail_dir = get_eudora_mail_dir();
  GtkTreeIter iter, child;

  gtk_tree_store_clear(store);

  /* Eudora Folder (root) */
  gtk_tree_store_append(store, &iter, NULL);
  gtk_tree_store_set(store, &iter,
                     COL_ICON_NAME, "folder",
                     COL_NAME, "Eudora",
                     COL_PATH, mail_dir,
                     COL_IS_FOLDER, TRUE,
                     COL_UNREAD, 0,
                     -1);

  /* Standard mailboxes */
  char path[1024];

  snprintf(path, sizeof(path), "%s/In", mail_dir);
  gtk_tree_store_append(store, &child, &iter);
  gtk_tree_store_set(store, &child,
                     COL_ICON_NAME, "mail-inbox",
                     COL_NAME, "In",
                     COL_PATH, path,
                     COL_IS_FOLDER, FALSE,
                     COL_UNREAD, 0,
                     -1);

  snprintf(path, sizeof(path), "%s/Out", mail_dir);
  gtk_tree_store_append(store, &child, &iter);
  gtk_tree_store_set(store, &child,
                     COL_ICON_NAME, "mail-outbox",
                     COL_NAME, "Out",
                     COL_PATH, path,
                     COL_IS_FOLDER, FALSE,
                     COL_UNREAD, 0,
                     -1);

  snprintf(path, sizeof(path), "%s/Junk", mail_dir);
  gtk_tree_store_append(store, &child, &iter);
  gtk_tree_store_set(store, &child,
                     COL_ICON_NAME, "mail-mark-junk",
                     COL_NAME, "Junk",
                     COL_PATH, path,
                     COL_IS_FOLDER, FALSE,
                     COL_UNREAD, 0,
                     -1);

  snprintf(path, sizeof(path), "%s/Trash", mail_dir);
  gtk_tree_store_append(store, &child, &iter);
  gtk_tree_store_set(store, &child,
                     COL_ICON_NAME, "user-trash",
                     COL_NAME, "Trash",
                     COL_PATH, path,
                     COL_IS_FOLDER, FALSE,
                     COL_UNREAD, 0,
                     -1);

  /* Scan for additional mailboxes/folders in the mail directory */
  DIR *dir = opendir(mail_dir);
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] == '.')
        continue;
      /* Skip the standard mailboxes we already added */
      if (strcmp(entry->d_name, "In") == 0 ||
          strcmp(entry->d_name, "Out") == 0 ||
          strcmp(entry->d_name, "Junk") == 0 ||
          strcmp(entry->d_name, "Trash") == 0)
        continue;
      /* Skip .toc files */
      const char *ext = strrchr(entry->d_name, '.');
      if (ext && strcmp(ext, ".toc") == 0)
        continue;

      snprintf(path, sizeof(path), "%s/%s", mail_dir, entry->d_name);
      struct stat st;
      if (stat(path, &st) != 0)
        continue;

      GtkTreeIter extra;
      if (S_ISDIR(st.st_mode)) {
        gtk_tree_store_append(store, &extra, &iter);
        gtk_tree_store_set(store, &extra,
                           COL_ICON_NAME, "folder",
                           COL_NAME, entry->d_name,
                           COL_PATH, path,
                           COL_IS_FOLDER, TRUE,
                           COL_UNREAD, 0,
                           -1);
        scan_mailbox_dir(store, &extra, path);
      } else if (S_ISREG(st.st_mode)) {
        gtk_tree_store_append(store, &extra, &iter);
        gtk_tree_store_set(store, &extra,
                           COL_ICON_NAME, "mail-unread",
                           COL_NAME, entry->d_name,
                           COL_PATH, path,
                           COL_IS_FOLDER, FALSE,
                           COL_UNREAD, 0,
                           -1);
      }
    }
    closedir(dir);
  }
}

/**********************************************************************
 * get_selected_parent_path - get the directory path for inserting new items.
 * If a folder is selected, returns its path. If a mailbox is selected,
 * returns its parent directory. Otherwise returns the mail root.
 **********************************************************************/
static const char *get_selected_parent_path(GtkTreeIter *out_parent,
                                             gboolean *have_parent) {
  static char parent_path[1024];
  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(MB.tree_view));
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(MB.store);

  *have_parent = FALSE;

  if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
    gboolean is_folder;
    gchar *path;
    gtk_tree_model_get(model, &iter, COL_PATH, &path, COL_IS_FOLDER,
                       &is_folder, -1);

    if (is_folder) {
      snprintf(parent_path, sizeof(parent_path), "%s", path);
      *out_parent = iter;
      *have_parent = TRUE;
    } else {
      GtkTreeIter parent_iter;
      if (gtk_tree_model_iter_parent(model, &parent_iter, &iter)) {
        gchar *pp;
        gtk_tree_model_get(model, &parent_iter, COL_PATH, &pp, -1);
        snprintf(parent_path, sizeof(parent_path), "%s", pp);
        g_free(pp);
        *out_parent = parent_iter;
        *have_parent = TRUE;
      } else {
        snprintf(parent_path, sizeof(parent_path), "%s",
                 get_eudora_mail_dir());
      }
    }
    g_free(path);
  } else {
    snprintf(parent_path, sizeof(parent_path), "%s", get_eudora_mail_dir());
  }

  return parent_path;
}

/**********************************************************************
 * make_unique_untitled - generate a unique "Untitled Mailbox" or
 * "Untitled Folder" name, matching the original Eudora behavior.
 **********************************************************************/
static void make_unique_untitled(const char *dir, bool folder,
                                  char *out_name, size_t out_len,
                                  char *out_path, size_t path_len) {
  const char *base = folder ? "Untitled Folder" : "Untitled Mailbox";
  snprintf(out_name, out_len, "%s", base);
  snprintf(out_path, path_len, "%s/%s", dir, base);

  int n = 1;
  while (g_file_test(out_path, G_FILE_TEST_EXISTS)) {
    n++;
    snprintf(out_name, out_len, "%s %d", base, n);
    snprintf(out_path, path_len, "%s/%s %d", dir, base, n);
  }
}

/**********************************************************************
 * on_name_edited - inline rename callback for the tree view
 **********************************************************************/
static void on_name_edited(GtkCellRendererText *cell, const char *path_str,
                           const char *new_name, gpointer user_data) {
  (void)cell;
  (void)user_data;

  if (!new_name || !new_name[0])
    return;

  GtkTreePath *tree_path = gtk_tree_path_new_from_string(path_str);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(MB.store), &iter, tree_path)) {
    gtk_tree_path_free(tree_path);
    return;
  }
  gtk_tree_path_free(tree_path);

  gchar *old_path;
  gtk_tree_model_get(GTK_TREE_MODEL(MB.store), &iter, COL_PATH, &old_path, -1);

  /* Build new path by replacing the filename component */
  char *slash = strrchr(old_path, '/');
  char new_path[1024];
  if (slash) {
    size_t dir_len = slash - old_path;
    snprintf(new_path, sizeof(new_path), "%.*s/%s", (int)dir_len, old_path,
             new_name);
  } else {
    snprintf(new_path, sizeof(new_path), "%s", new_name);
  }

  /* Rename on disk */
  if (rename(old_path, new_path) == 0) {
    /* Also rename .toc file if it exists */
    char old_toc[1024], new_toc[1024];
    snprintf(old_toc, sizeof(old_toc), "%s.toc", old_path);
    snprintf(new_toc, sizeof(new_toc), "%s.toc", new_path);
    rename(old_toc, new_toc); /* ignore error — toc may not exist */

    gtk_tree_store_set(MB.store, &iter, COL_NAME, new_name, COL_PATH, new_path,
                       -1);
  }

  g_free(old_path);
}

/**********************************************************************
 * DoNewMailbox - create a new mailbox or folder, matching original
 * Eudora behavior: creates an "Untitled" item under the selected
 * folder and starts inline renaming.
 **********************************************************************/
static void DoNewMailbox(bool folder) {
  GtkTreeIter parent_iter;
  gboolean have_parent;
  const char *dir = get_selected_parent_path(&parent_iter, &have_parent);

  char name[256], full_path[1024];
  make_unique_untitled(dir, folder, name, sizeof(name), full_path,
                       sizeof(full_path));

  if (folder) {
    if (g_mkdir_with_parents(full_path, 0755) != 0)
      return;
  } else {
    FILE *f = fopen(full_path, "w");
    if (!f)
      return;
    fclose(f);
  }

  /* Add to tree store */
  GtkTreeIter new_iter;
  gtk_tree_store_append(MB.store, &new_iter,
                        have_parent ? &parent_iter : NULL);
  gtk_tree_store_set(MB.store, &new_iter, COL_ICON_NAME,
                     folder ? "folder" : "mail-unread", COL_NAME, name,
                     COL_PATH, full_path, COL_IS_FOLDER, folder ? TRUE : FALSE,
                     COL_UNREAD, 0, -1);

  /* Expand parent so the new item is visible */
  if (have_parent) {
    GtkTreePath *pp =
        gtk_tree_model_get_path(GTK_TREE_MODEL(MB.store), &parent_iter);
    gtk_tree_view_expand_row(GTK_TREE_VIEW(MB.tree_view), pp, FALSE);
    gtk_tree_path_free(pp);
  }

  /* Select and start inline rename — like original Eudora's LVRename() */
  GtkTreePath *new_path =
      gtk_tree_model_get_path(GTK_TREE_MODEL(MB.store), &new_iter);
  GtkTreeViewColumn *col =
      gtk_tree_view_get_column(GTK_TREE_VIEW(MB.tree_view), 0);
  gtk_tree_view_set_cursor(GTK_TREE_VIEW(MB.tree_view), new_path, col, TRUE);
  gtk_tree_path_free(new_path);
}

/**********************************************************************
 * on_new_mailbox_clicked - create a new mailbox
 **********************************************************************/
static void on_new_mailbox_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  DoNewMailbox(false);
}

/**********************************************************************
 * on_new_folder_clicked - create a new folder
 **********************************************************************/
static void on_new_folder_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  DoNewMailbox(true);
}

/**********************************************************************
 * on_remove_response - handle confirmation dialog response
 **********************************************************************/
static void on_remove_response(GtkDialog *dlg, int response,
                                gpointer user_data) {
  (void)user_data;
  if (response == GTK_RESPONSE_OK) {
    const char *rm_path =
        (const char *)g_object_get_data(G_OBJECT(dlg), "rm-path");
    gboolean rm_folder =
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dlg), "rm-is-folder"));

    if (rm_path) {
      if (rm_folder) {
        /* Remove directory (only if empty, like original) */
        g_rmdir(rm_path);
      } else {
        /* Remove mailbox file and its .toc */
        g_unlink(rm_path);
        char toc_path[1024];
        snprintf(toc_path, sizeof(toc_path), "%s.toc", rm_path);
        g_unlink(toc_path);
      }
    }
    MBRefill();
  }
  gtk_window_destroy(GTK_WINDOW(dlg));
}

/**********************************************************************
 * on_remove_clicked - remove selected mailbox or folder
 * Shows confirmation dialog matching original Eudora's DoRemoveBox.
 **********************************************************************/
static void on_remove_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;

  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(MB.tree_view));
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(MB.store);

  if (!gtk_tree_selection_get_selected(sel, &model, &iter))
    return;

  gchar *name, *path;
  gboolean is_folder;
  gtk_tree_model_get(model, &iter, COL_NAME, &name, COL_PATH, &path,
                     COL_IS_FOLDER, &is_folder, -1);

  /* Don't allow removing standard mailboxes (like original IsSpecialBox) */
  if (name && (strcmp(name, "In") == 0 || strcmp(name, "Out") == 0 ||
               strcmp(name, "Trash") == 0 || strcmp(name, "Junk") == 0 ||
               strcmp(name, "Eudora") == 0)) {
    g_free(name);
    g_free(path);
    return;
  }

  /* Check if empty (like original DoRemoveBox) */
  bool is_empty = true;
  if (is_folder) {
    DIR *d = opendir(path);
    if (d) {
      struct dirent *ent;
      while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] != '.') {
          is_empty = false;
          break;
        }
      }
      closedir(d);
    }
  } else {
    struct stat st;
    if (stat(path, &st) == 0 && st.st_size > 0)
      is_empty = false;
  }

  /* Show confirmation — different message for empty vs non-empty
   * like original Eudora's DELETE_EMPTY_SINGLE_ASTR / DELETE_NON_EMPTY_SINGLE_ASTR */
  char msg[512];
  if (is_empty)
    snprintf(msg, sizeof(msg), "Remove the empty %s \"%s\"?",
             is_folder ? "folder" : "mailbox", name ? name : "");
  else
    snprintf(msg, sizeof(msg),
             "The %s \"%s\" is not empty. Remove it anyway?",
             is_folder ? "folder" : "mailbox", name ? name : "");

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      "Remove", GTK_WINDOW(gtk_widget_get_root(MB.tree_view)),
      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, "_Remove",
      GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *label = gtk_label_new(msg);
  gtk_widget_set_margin_start(label, 12);
  gtk_widget_set_margin_end(label, 12);
  gtk_widget_set_margin_top(label, 12);
  gtk_widget_set_margin_bottom(label, 12);
  gtk_box_append(GTK_BOX(content), label);

  g_object_set_data_full(G_OBJECT(dialog), "rm-path", g_strdup(path), g_free);
  g_object_set_data(G_OBJECT(dialog), "rm-is-folder",
                    GINT_TO_POINTER(is_folder));

  g_signal_connect(dialog, "response", G_CALLBACK(on_remove_response), NULL);

  g_free(name);
  g_free(path);
  gtk_window_present(GTK_WINDOW(dialog));
}

/**********************************************************************
 * on_mb_row_activated - double-click or Enter on a mailbox row
 **********************************************************************/
static void on_mb_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                GtkTreeViewColumn *column, gpointer user_data) {
  (void)column;
  (void)user_data;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;

  if (!gtk_tree_model_get_iter(model, &iter, path))
    return;

  gboolean is_folder;
  gchar *mb_path;
  gchar *name;
  gtk_tree_model_get(model, &iter,
                     COL_PATH, &mb_path,
                     COL_NAME, &name,
                     COL_IS_FOLDER, &is_folder,
                     -1);

  if (is_folder) {
    /* Toggle expand/collapse */
    if (gtk_tree_view_row_expanded(tree_view, path))
      gtk_tree_view_collapse_row(tree_view, path);
    else
      gtk_tree_view_expand_row(tree_view, path, FALSE);
  } else if (mb_path) {
    /* Open this mailbox */
    FSSpec spec;
    memset(&spec, 0, sizeof(spec));
    strncpy(spec.path, mb_path, sizeof(spec.path) - 1);
    strncpy(spec.name, name ? name : "", sizeof(spec.name) - 1);
    OpenMailbox(&spec, true, NULL);
  }

  g_free(mb_path);
  g_free(name);
}

/**********************************************************************
 * OpenMBWin - open the mailbox browser window
 *
 * Original Carbon version used ListViews, ControlHandles, WIND resources.
 * This GTK port uses GtkTreeView with GtkTreeStore.
 **********************************************************************/
void OpenMBWin(void) {
  if (SelectOpenWazoo(MB_WIN))
    return; /* Already opened in a wazoo */

  if (!MB.inited) {
    MyWindowPtr win = GetNewMyWindow(0, NULL, NULL, NULL, false, false, MB_WIN);
    if (!win)
      return;

    WindowPtr winWP = GetMyWindowWindowPtr(win);
    MB.win = win;

    gtk_window_set_title(GTK_WINDOW(winWP), "Mailboxes");
    gtk_window_set_default_size(GTK_WINDOW(winWP), 250, 500);

    /* Create the tree store and view */
    MB.store = gtk_tree_store_new(NUM_COLS,
                                  G_TYPE_STRING,   /* icon name */
                                  G_TYPE_STRING,   /* name */
                                  G_TYPE_STRING,   /* path */
                                  G_TYPE_BOOLEAN,  /* is_folder */
                                  G_TYPE_INT);     /* unread */

    MB.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(MB.store));
    g_object_unref(MB.store); /* tree view holds ref */

    /* Icon + Name column */
    GtkTreeViewColumn *col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Mailboxes");

    GtkCellRenderer *icon_renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(col, icon_renderer, FALSE);
    gtk_tree_view_column_add_attribute(col, icon_renderer, "icon-name",
                                       COL_ICON_NAME);

    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
    g_object_set(text_renderer, "editable", TRUE, NULL);
    g_signal_connect(text_renderer, "edited", G_CALLBACK(on_name_edited), NULL);
    gtk_tree_view_column_pack_start(col, text_renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, text_renderer, "text", COL_NAME);

    gtk_tree_view_append_column(GTK_TREE_VIEW(MB.tree_view), col);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(MB.tree_view), FALSE);

    /* Double-click to open mailbox */
    g_signal_connect(MB.tree_view, "row-activated",
                     G_CALLBACK(on_mb_row_activated), NULL);

    /* Populate the tree */
    mb_fill(MB.store);

    /* Expand the root "Eudora" folder by default */
    GtkTreePath *root_path = gtk_tree_path_new_first();
    gtk_tree_view_expand_row(GTK_TREE_VIEW(MB.tree_view), root_path, FALSE);
    gtk_tree_path_free(root_path);

    /* Layout: scrolled window containing tree view, with toolbar at bottom */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), MB.tree_view);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);
    gtk_box_append(GTK_BOX(vbox), scrolled);

    /* Toolbar buttons: New Mailbox, New Folder, Remove */
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(toolbar, 4);
    gtk_widget_set_margin_end(toolbar, 4);
    gtk_widget_set_margin_top(toolbar, 4);
    gtk_widget_set_margin_bottom(toolbar, 4);

    MB.btn_new_mb = gtk_button_new_with_label("New Mailbox");
    MB.btn_new_folder = gtk_button_new_with_label("New Folder");
    MB.btn_remove = gtk_button_new_with_label("Remove");

    g_signal_connect(MB.btn_new_mb, "clicked",
                     G_CALLBACK(on_new_mailbox_clicked), NULL);
    g_signal_connect(MB.btn_new_folder, "clicked",
                     G_CALLBACK(on_new_folder_clicked), NULL);
    g_signal_connect(MB.btn_remove, "clicked",
                     G_CALLBACK(on_remove_clicked), NULL);

    gtk_box_append(GTK_BOX(toolbar), MB.btn_new_mb);
    gtk_box_append(GTK_BOX(toolbar), MB.btn_new_folder);
    gtk_box_append(GTK_BOX(toolbar), MB.btn_remove);
    gtk_box_append(GTK_BOX(vbox), toolbar);

    /* Set the vbox as the window content */
    gtk_window_set_child(GTK_WINDOW(winWP), vbox);

    /* Set window callbacks */
    win->close = MBClose;
    win->didResize = MBDidResize;
    win->menu = MBMenu;
    win->activate = MBActivate;

    /* Promote to wazoo if applicable */
    PromoteToWazoo(win);

    ShowMyWindow(winWP);
    MB.inited = true;
  } else {
    WindowPtr winWP = GetMyWindowWindowPtr(MB.win);
    UserSelectWindow(winWP);
  }
}

/**********************************************************************
 * MBClose - close the mailbox browser window
 **********************************************************************/
static bool MBClose(MyWindowPtr win) {
  (void)win;
  MB.inited = false;
  MB.win = NULL;
  MB.tree_view = NULL;
  MB.store = NULL;
  return true;
}

/**********************************************************************
 * MBDidResize - handle resize (GTK handles this automatically)
 **********************************************************************/
static void MBDidResize(MyWindowPtr win, Rect *oldContR) {
  (void)win;
  (void)oldContR;
}

/**********************************************************************
 * MBMenu - handle menu commands
 **********************************************************************/
static bool MBMenu(MyWindowPtr win, int menu, int item, short modifiers) {
  (void)win;
  (void)menu;
  (void)item;
  (void)modifiers;
  return false;
}

/**********************************************************************
 * MBActivate - handle window activation
 **********************************************************************/
static void MBActivate(MyWindowPtr win) {
  (void)win;
}

/**********************************************************************
 * MBRefill - refresh the mailbox list
 **********************************************************************/
void MBRefill(void) {
  if (MB.inited && MB.store) {
    mb_fill(MB.store);
    /* Re-expand root */
    GtkTreePath *root_path = gtk_tree_path_new_first();
    gtk_tree_view_expand_row(GTK_TREE_VIEW(MB.tree_view), root_path, FALSE);
    gtk_tree_path_free(root_path);
  }
}

/**********************************************************************
 * MBOpenFolder - open and select folder (called from menu system)
 **********************************************************************/
void MBOpenFolder(void *hStringList, bool isIMAP) {
  (void)hStringList;
  (void)isIMAP;
  OpenMBWin();
}
