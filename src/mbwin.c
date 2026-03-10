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
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <dirent.h>

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
    const char *home = g_get_home_dir();
    snprintf(mail_dir, sizeof(mail_dir), "%s/.eudora", home);
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
