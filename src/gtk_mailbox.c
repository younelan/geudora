/*
 * GTK4 Mailbox Sidebar for gEudora
 * Backed by macmbx library — walks MacmbxNode tree for the sidebar,
 * uses MacmbxStore for create/delete/rename/move operations.
 */

#include "gtk_mailbox.h"
#include "gtk_prefs.h"
#include <pango/pango.h>
#include <glib.h>
#include <string.h>

/* ── Global store reference ─────────────────────────────────────── */

static MacmbxStore *g_store = NULL;

void gtk_mailbox_set_store(MacmbxStore *store) { g_store = store; }
MacmbxStore *gtk_mailbox_get_store(void) { return g_store; }

/* ── Helpers ────────────────────────────────────────────────────── */

/* Get the mailboxes directory path */
gchar *gtk_mailbox_get_path(const gchar *name) {
  if (!name) return NULL;
  /* Use store's root if available, else fall back to prefs */
  const char *dir = g_store ? macmbx_store_root_dir(g_store)
                             : prefs_get_mailboxes_path();
  if (!dir) return NULL;
  return g_build_filename(dir, name, NULL);
}

/* Icon name for a mailbox based on its type or name */
static const char *node_icon_name(MacmbxNode *node) {
  if (!node) return "folder-symbolic";
  if (node->type == MACMBX_NODE_FOLDER) return "folder-symbolic";
  switch (node->mbox_type) {
    case MACMBX_TYPE_IN:      return "mail-inbox-symbolic";
    case MACMBX_TYPE_OUT:     return "mail-outbox-symbolic";
    case MACMBX_TYPE_TRASH:   return "user-trash-symbolic";
    case MACMBX_TYPE_JUNK:    return "edit-delete-symbolic";
    default: break;
  }
  if (strcmp(node->name, "Drafts") == 0) return "document-edit-symbolic";
  return "mail-unread-symbolic";
}

/* ── GObject data keys for mailbox rows ────────────────────────── */
#define MB_KEY_PATH   "mb-path"
#define MB_KEY_NAME   "mb-name"
#define MB_KEY_IS_DIR "mb-is-dir"

/* ── Drag and drop callbacks ──────────────────────────────────── */

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

static void on_mb_drag_begin(GtkDragSource *source, GdkDrag *drag,
                              gpointer ud) {
  (void)source; (void)drag;
  gtk_widget_set_opacity(GTK_WIDGET(ud), 0.4);
}

static gboolean on_mb_drop(GtkDropTarget *target, const GValue *value,
                            double x, double y, gpointer ud) {
  (void)target; (void)x; (void)y;
  GtkWidget *folder_row = GTK_WIDGET(ud);
  const char *folder_path = g_object_get_data(G_OBJECT(folder_row), MB_KEY_PATH);
  if (!folder_path || !G_VALUE_HOLDS_STRING(value)) return FALSE;

  const char *src_path = g_value_get_string(value);
  if (!src_path || !*src_path) return FALSE;

  /* Use macmbx_store_move if store available */
  if (g_store) {
    /* Compute relative paths from store base */
    const char *base = g_store->base_path;
    size_t blen = strlen(base);
    const char *src_rel = src_path;
    const char *dst_rel = folder_path;
    if (strncmp(src_path, base, blen) == 0 && src_path[blen] == '/')
      src_rel = src_path + blen + 1;
    if (strncmp(folder_path, base, blen) == 0 && folder_path[blen] == '/')
      dst_rel = folder_path + blen + 1;

    if (macmbx_store_move(g_store, src_rel, dst_rel) == 0) {
      GtkWidget *listbox = gtk_widget_get_ancestor(folder_row, GTK_TYPE_LIST_BOX);
      if (listbox) gtk_mailbox_tree_refresh(listbox);
      return TRUE;
    }
  }
  return FALSE;
}

static GdkDragAction on_mb_drop_enter(GtkDropTarget *target, double x, double y,
                                       gpointer ud) {
  (void)target; (void)x; (void)y;
  gtk_widget_add_css_class(GTK_WIDGET(ud), "mb-drop-target");
  return GDK_ACTION_MOVE;
}

static void on_mb_drop_leave(GtkDropTarget *target, gpointer ud) {
  (void)target;
  gtk_widget_remove_css_class(GTK_WIDGET(ud), "mb-drop-target");
}

/* Drop onto listbox background — move to root */
static gboolean on_mb_drop_root(GtkDropTarget *target, const GValue *value,
                                 double x, double y, gpointer ud) {
  (void)target; (void)x; (void)y;
  GtkWidget *listbox = GTK_WIDGET(ud);
  if (!G_VALUE_HOLDS_STRING(value) || !g_store) return FALSE;

  const char *src_path = g_value_get_string(value);
  if (!src_path || !*src_path) return FALSE;

  const char *base = g_store->base_path;
  size_t blen = strlen(base);
  const char *src_rel = src_path;
  if (strncmp(src_path, base, blen) == 0 && src_path[blen] == '/')
    src_rel = src_path + blen + 1;

  if (macmbx_store_move(g_store, src_rel, NULL) == 0) {
    gtk_mailbox_tree_refresh(listbox);
    return TRUE;
  }
  return FALSE;
}

/* ── Create a single mailbox row widget ────────────────────────── */

static GtkWidget *make_mb_row(MacmbxNode *node, int depth) {
  gboolean is_dir = (node->type == MACMBX_NODE_FOLDER);

  GtkWidget *row = gtk_list_box_row_new();
  g_object_set_data_full(G_OBJECT(row), MB_KEY_PATH, g_strdup(node->path), g_free);
  g_object_set_data_full(G_OBJECT(row), MB_KEY_NAME, g_strdup(node->name), g_free);
  g_object_set_data(G_OBJECT(row), MB_KEY_IS_DIR, GINT_TO_POINTER(is_dir));

  if (is_dir)
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_start(hbox, 6 + depth * 16);
  gtk_widget_set_margin_end(hbox, 6);
  gtk_widget_set_margin_top(hbox, 3);
  gtk_widget_set_margin_bottom(hbox, 3);

  /* Icon */
  GtkWidget *icon = gtk_image_new_from_icon_name(node_icon_name(node));
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
  gtk_widget_add_css_class(icon, "mb-icon");
  gtk_box_append(GTK_BOX(hbox), icon);

  /* Name label */
  GtkWidget *lbl = gtk_label_new(node->name);
  gtk_label_set_xalign(GTK_LABEL(lbl), 0);
  gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(lbl, TRUE);
  if (is_dir)
    gtk_widget_add_css_class(lbl, "mb-folder");
  else if (node->unread > 0)
    gtk_widget_add_css_class(lbl, "mb-unread");
  else
    gtk_widget_add_css_class(lbl, "mb-name");
  gtk_box_append(GTK_BOX(hbox), lbl);

  /* Unread pill */
  if (!is_dir && node->unread > 0) {
    char pill_text[16];
    snprintf(pill_text, sizeof(pill_text), "%d", node->unread);
    GtkWidget *pill = gtk_label_new(pill_text);
    gtk_widget_add_css_class(pill, "mb-pill");
    gtk_widget_set_halign(pill, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(hbox), pill);
  }

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), hbox);

  /* Drag source: mailbox files can be dragged */
  if (!is_dir) {
    GtkDragSource *drag = gtk_drag_source_new();
    gtk_drag_source_set_actions(drag, GDK_ACTION_MOVE);
    g_signal_connect(drag, "prepare", G_CALLBACK(on_mb_drag_prepare), row);
    g_signal_connect(drag, "drag-begin", G_CALLBACK(on_mb_drag_begin), row);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(drag));
  }

  /* Drop target: folders accept mailbox drops */
  if (is_dir) {
    GtkDropTarget *drop = gtk_drop_target_new(G_TYPE_STRING, GDK_ACTION_MOVE);
    g_signal_connect(drop, "drop", G_CALLBACK(on_mb_drop), row);
    g_signal_connect(drop, "enter", G_CALLBACK(on_mb_drop_enter), row);
    g_signal_connect(drop, "leave", G_CALLBACK(on_mb_drop_leave), row);
    gtk_widget_add_controller(row, GTK_EVENT_CONTROLLER(drop));
  }

  return row;
}

/* ── Recursive node tree loading into list box ───────────────── */

/* Fixed ordering for special mailboxes — always shown first at root */
static const MacmbxType SPECIAL_ORDER[] = {
  MACMBX_TYPE_IN, MACMBX_TYPE_OUT, MACMBX_TYPE_NORMAL /* Drafts */,
  MACMBX_TYPE_TRASH, MACMBX_TYPE_JUNK
};
static const char *SPECIAL_NAMES[] = {"In", "Out", "Drafts", "Trash", "Junk"};
#define N_SPECIALS 5

static void load_node_tree(GtkWidget *listbox, MacmbxNode *node, int depth) {
  for (MacmbxNode *n = node; n; n = n->next) {
    GtkWidget *row = make_mb_row(n, depth);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    /* Recurse into folder children */
    if (n->type == MACMBX_NODE_FOLDER && n->children)
      load_node_tree(listbox, n->children, depth + 1);
  }
}

static gint node_name_cmp(gconstpointer a, gconstpointer b) {
  MacmbxNode *na = *(MacmbxNode **)a, *nb = *(MacmbxNode **)b;
  return g_ascii_strcasecmp(na->name, nb->name);
}

static void load_store_into_listbox(GtkWidget *listbox, MacmbxStore *store) {
  MacmbxNode *root = macmbx_store_root(store);
  if (!root) return;

  /* Update unread/total counts before displaying */
  macmbx_store_update_counts(store);

  /* Phase 1: add special mailboxes in fixed order */
  GHashTable *shown = g_hash_table_new(g_str_hash, g_str_equal);
  for (int i = 0; i < N_SPECIALS; i++) {
    /* Find by name for Drafts (type NORMAL), by type for others */
    MacmbxNode *node = NULL;
    if (SPECIAL_ORDER[i] != MACMBX_TYPE_NORMAL)
      node = macmbx_store_find_special(store, SPECIAL_ORDER[i]);
    else
      node = macmbx_store_find_by_name(store, SPECIAL_NAMES[i]);
    if (!node) continue;
    GtkWidget *row = make_mb_row(node, 0);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    g_hash_table_insert(shown, node->name, node);
  }

  /* Phase 2: add remaining nodes (folders first, then mailboxes, sorted) */
  /* Collect non-special top-level nodes */
  GPtrArray *folders = g_ptr_array_new();
  GPtrArray *mailboxes = g_ptr_array_new();
  for (MacmbxNode *n = root; n; n = n->next) {
    if (g_hash_table_contains(shown, n->name)) continue;
    if (n->type == MACMBX_NODE_FOLDER)
      g_ptr_array_add(folders, n);
    else
      g_ptr_array_add(mailboxes, n);
  }
  g_hash_table_destroy(shown);

  /* Sort by name (case-insensitive) */
  g_ptr_array_sort(folders, node_name_cmp);
  g_ptr_array_sort(mailboxes, node_name_cmp);

  /* Add folders (with recursive children) */
  for (guint i = 0; i < folders->len; i++) {
    MacmbxNode *n = g_ptr_array_index(folders, i);
    GtkWidget *row = make_mb_row(n, 0);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    if (n->children)
      load_node_tree(listbox, n->children, 1);
  }

  /* Add regular mailboxes */
  for (guint i = 0; i < mailboxes->len; i++) {
    MacmbxNode *n = g_ptr_array_index(mailboxes, i);
    GtkWidget *row = make_mb_row(n, 0);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
  }

  g_ptr_array_free(folders, TRUE);
  g_ptr_array_free(mailboxes, TRUE);
}

/* ── Public API ────────────────────────────────────────────────── */

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

void gtk_mailbox_tree_load(GtkWidget *listbox, MacmbxStore *store) {
  if (!GTK_IS_LIST_BOX(listbox) || !store) return;

  /* Clear existing rows */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(listbox)) != NULL)
    gtk_list_box_remove(GTK_LIST_BOX(listbox), child);

  load_store_into_listbox(listbox, store);
}

void gtk_mailbox_tree_refresh(GtkWidget *tree) {
  if (!g_store) return;
  macmbx_store_refresh(g_store);
  gtk_mailbox_tree_load(tree, g_store);
}

/* Ensure default mailboxes exist in the store */
void gtk_mailbox_ensure_defaults(MacmbxStore *store) {
  if (!store) return;
  static const char *defaults[] = {"In", "Out", "Drafts", "Trash", "Junk", NULL};
  for (int i = 0; defaults[i]; i++) {
    if (!macmbx_store_find_by_name(store, defaults[i]))
      macmbx_store_create_mailbox(store, NULL, defaults[i]);
  }
}
