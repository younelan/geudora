/*
 * GTK4 Mailbox Sidebar for gEudora — GtkTreeView + GtkTreeStore
 * Backed by macmbx library — walks MacmbxNode tree for the sidebar,
 * uses MacmbxStore for create/delete/rename/move operations.
 */

#include "gtk_mailbox.h"
#include "gtk_prefs.h"
#include <pango/pango.h>
#include <glib.h>
#include <string.h>

/* ── GtkTreeStore column indices ─────────────────────────────────── */

enum {
  COL_NAME = 0,   /* display name (may include unread count) */
  COL_ICON,       /* icon name string */
  COL_PATH,       /* full filesystem path */
  COL_UNREAD,     /* unread message count */
  COL_IS_DIR,     /* TRUE for folders */
  COL_WEIGHT,     /* PANGO_WEIGHT_BOLD or PANGO_WEIGHT_NORMAL */
  N_COLUMNS
};

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

/* ── Folder expand/collapse state persistence ──────────────────── */

#define PREFS_GROUP_SIDEBAR "sidebar"

static GHashTable *g_expanded = NULL; /* path → TRUE for expanded folders */

static void ensure_expanded_table(void) {
  if (g_expanded) return;
  g_expanded = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

static bool is_folder_expanded(const char *path) {
  ensure_expanded_table();
  return g_hash_table_contains(g_expanded, path);
}

static void set_folder_expanded(const char *path, bool expanded) {
  ensure_expanded_table();
  if (expanded)
    g_hash_table_insert(g_expanded, g_strdup(path), GINT_TO_POINTER(1));
  else
    g_hash_table_remove(g_expanded, path);
}

/* Load expanded state from prefs (pipe-separated paths) */
static void load_expanded_state(void) {
  ensure_expanded_table();
  const char *val = prefs_get_string(PREFS_GROUP_SIDEBAR, "expanded", "");
  if (!val || !val[0]) return;
  char *copy = g_strdup(val);
  char *tok = strtok(copy, "|");
  while (tok) {
    g_hash_table_insert(g_expanded, g_strdup(tok), GINT_TO_POINTER(1));
    tok = strtok(NULL, "|");
  }
  g_free(copy);
}

/* Save expanded state to prefs */
static void save_expanded_state(void) {
  if (!g_expanded) return;
  GString *buf = g_string_new(NULL);
  GHashTableIter iter;
  gpointer key;
  g_hash_table_iter_init(&iter, g_expanded);
  while (g_hash_table_iter_next(&iter, &key, NULL)) {
    if (buf->len > 0) g_string_append_c(buf, '|');
    g_string_append(buf, (const char *)key);
  }
  prefs_set_string(PREFS_GROUP_SIDEBAR, "expanded", buf->str);
  g_string_free(buf, TRUE);
}

static bool g_expanded_loaded = false;

/* ── GtkTreeView signal callbacks ─────────────────────────────── */

/* row-expanded: save expanded state */
static void on_row_expanded(GtkTreeView *tree_view, GtkTreeIter *iter,
                             GtkTreePath *path, gpointer user_data) {
  (void)tree_view; (void)path; (void)user_data;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  gchar *mbox_path = NULL;
  gtk_tree_model_get(model, iter, COL_PATH, &mbox_path, -1);
  if (mbox_path) {
    set_folder_expanded(mbox_path, true);
    save_expanded_state();
    g_free(mbox_path);
  }
}

/* row-collapsed: remove expanded state */
static void on_row_collapsed(GtkTreeView *tree_view, GtkTreeIter *iter,
                              GtkTreePath *path, gpointer user_data) {
  (void)tree_view; (void)path; (void)user_data;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  gchar *mbox_path = NULL;
  gtk_tree_model_get(model, iter, COL_PATH, &mbox_path, -1);
  if (mbox_path) {
    set_folder_expanded(mbox_path, false);
    save_expanded_state();
    g_free(mbox_path);
  }
}

/* row-activated: open the mailbox */
static void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                              GtkTreeViewColumn *column, gpointer user_data) {
  (void)column; (void)user_data;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path)) return;

  gboolean is_dir = FALSE;
  gchar *mbox_path = NULL;
  gchar *name = NULL;
  gtk_tree_model_get(model, &iter,
                     COL_IS_DIR, &is_dir,
                     COL_PATH, &mbox_path,
                     COL_NAME, &name,
                     -1);

  if (is_dir) {
    /* Toggle expand/collapse for folders */
    if (gtk_tree_view_row_expanded(tree_view, path))
      gtk_tree_view_collapse_row(tree_view, path);
    else
      gtk_tree_view_expand_row(tree_view, path, FALSE);
  } else if (mbox_path && *mbox_path) {
    extern void eudora_open_mailbox(const char *path, const char *name);
    eudora_open_mailbox(mbox_path, name);
  }

  g_free(mbox_path);
  g_free(name);
}

/* ── Drag and drop ────────────────────────────────────────────── */

/* Drag source: drag mailboxes/folders from the sidebar for reorder */
static GdkContentProvider *on_sidebar_drag_prepare(GtkDragSource *source,
                                                     double x, double y,
                                                     gpointer ud) {
  (void)source;
  GtkTreeView *tree_view = GTK_TREE_VIEW(ud);
  GtkTreePath *path;
  if (!gtk_tree_view_get_path_at_pos(tree_view, (int)x, (int)y, &path, NULL, NULL, NULL))
    return NULL;

  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path)) {
    gtk_tree_path_free(path);
    return NULL;
  }
  char *mbox_path = NULL;
  gtk_tree_model_get(model, &iter, COL_PATH, &mbox_path, -1);
  gtk_tree_path_free(path);
  if (!mbox_path) return NULL;

  GdkContentProvider *provider = gdk_content_provider_new_typed(G_TYPE_STRING, mbox_path);
  g_free(mbox_path);
  return provider;
}

/* Drop handler */
static gboolean on_tree_drop(GtkDropTarget *target, const GValue *value,
                              double x, double y, gpointer ud) {
  (void)target;
  GtkTreeView *tree_view = GTK_TREE_VIEW(ud);
  if (!G_VALUE_HOLDS_STRING(value)) return FALSE;

  const char *drop_str = g_value_get_string(value);
  if (!drop_str || !*drop_str) return FALSE;

  /* Find which row is under the cursor */
  GtkTreePath *path = NULL;
  gchar *folder_path = NULL;
  int ix = (int)x, iy = (int)y;

  if (gtk_tree_view_get_path_at_pos(tree_view, ix, iy,
                                     &path, NULL, NULL, NULL)) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter(model, &iter, path))
      gtk_tree_model_get(model, &iter, COL_PATH, &folder_path, -1);
    gtk_tree_path_free(path);
  }

  if (!folder_path) {
    /* Dropped on empty space — treat as root */
    if (g_store)
      folder_path = g_strdup(g_store->base_path);
    else
      return FALSE;
  }

  gboolean result = FALSE;

  /* Message transfer: "msg:<toc_path>\t<idx1>,<idx2>,..." */
  if (strncmp(drop_str, "msg:", 4) == 0) {
    const char *tab = strchr(drop_str + 4, '\t');
    if (!tab) { g_free(folder_path); return FALSE; }

    /* Extract source TOC path */
    int path_len = (int)(tab - (drop_str + 4));
    char *src_path = g_strndup(drop_str + 4, path_len);

    /* Parse message indices */
    const char *idx_str = tab + 1;
    int indices[256];
    int count = 0;
    while (*idx_str && count < 256) {
      indices[count++] = (int)strtol(idx_str, (char **)&idx_str, 10);
      if (*idx_str == ',') idx_str++;
    }

    if (count > 0) {
      /* Open source and destination TOCs */
      MacmbxTOC *src_toc = macmbx_toc_open(src_path);
      MacmbxTOC *dst_toc = macmbx_toc_open(folder_path);
      if (src_toc && dst_toc) {
        /* Transfer messages (move, not copy) — process in reverse order
         * so indices stay valid as messages are removed */
        for (int i = count - 1; i >= 0; i--)
          macmbx_transfer(src_toc, indices[i], dst_toc, false);
        macmbx_toc_save(src_toc);
        macmbx_toc_save(dst_toc);

        /* Refresh open mailbox views */
        extern void eudora_refresh_open_mailboxes(void);
        eudora_refresh_open_mailboxes();

        /* Update sidebar unread counts */
        if (g_store) macmbx_store_update_counts(g_store);
        gtk_mailbox_tree_refresh(GTK_WIDGET(tree_view));
      }
    }
    g_free(src_path);
    result = TRUE;
  }
  /* Mailbox reorder: plain path string */
  else if (g_store) {
    const char *base = g_store->base_path;
    size_t blen = strlen(base);
    const char *src_rel = drop_str;
    const char *dst_rel = folder_path;
    if (strncmp(drop_str, base, blen) == 0 && drop_str[blen] == '/')
      src_rel = drop_str + blen + 1;
    if (strncmp(folder_path, base, blen) == 0 && folder_path[blen] == '/')
      dst_rel = folder_path + blen + 1;

    if (macmbx_store_move(g_store, src_rel, dst_rel) == 0) {
      gtk_mailbox_tree_refresh(GTK_WIDGET(tree_view));
      result = TRUE;
    }
  }

  g_free(folder_path);
  return result;
}

/* Drop target highlight: use CSS class on the whole tree view.
 * We track the hovered path for visual feedback. */
static GtkTreePath *g_hover_path = NULL;

static GdkDragAction on_tree_drop_enter(GtkDropTarget *target, double x, double y,
                                         gpointer ud) {
  (void)target;
  GtkTreeView *tree_view = GTK_TREE_VIEW(ud);
  gtk_widget_add_css_class(GTK_WIDGET(tree_view), "mb-drop-active");

  /* Highlight the row under cursor */
  GtkTreePath *path = NULL;
  int ix = (int)x, iy = (int)y;
  if (gtk_tree_view_get_path_at_pos(tree_view, ix, iy,
                                     &path, NULL, NULL, NULL)) {
    gtk_tree_view_set_cursor(tree_view, path, NULL, FALSE);
    if (g_hover_path) gtk_tree_path_free(g_hover_path);
    g_hover_path = path;
  }
  return GDK_ACTION_MOVE;
}

static GdkDragAction on_tree_drop_motion(GtkDropTarget *target, double x, double y,
                                          gpointer ud) {
  (void)target;
  GtkTreeView *tree_view = GTK_TREE_VIEW(ud);
  GtkTreePath *path = NULL;
  int ix = (int)x, iy = (int)y;
  if (gtk_tree_view_get_path_at_pos(tree_view, ix, iy,
                                     &path, NULL, NULL, NULL)) {
    gtk_tree_view_set_cursor(tree_view, path, NULL, FALSE);
    if (g_hover_path) gtk_tree_path_free(g_hover_path);
    g_hover_path = path;
  }
  return GDK_ACTION_MOVE;
}

static void on_tree_drop_leave(GtkDropTarget *target, gpointer ud) {
  (void)target;
  gtk_widget_remove_css_class(GTK_WIDGET(ud), "mb-drop-active");
  if (g_hover_path) {
    gtk_tree_path_free(g_hover_path);
    g_hover_path = NULL;
  }
}

/* ── Recursive node tree loading into GtkTreeStore ───────────── */

/* Fixed ordering for special mailboxes — always shown first at root */
static const MacmbxType SPECIAL_ORDER[] = {
  MACMBX_TYPE_IN, MACMBX_TYPE_OUT, MACMBX_TYPE_NORMAL /* Drafts */,
  MACMBX_TYPE_TRASH, MACMBX_TYPE_JUNK
};
static const char *SPECIAL_NAMES[] = {"In", "Out", "Drafts", "Trash", "Junk"};
#define N_SPECIALS 5

/* Build display name: append unread count for mailboxes */
static gchar *make_display_name(MacmbxNode *node) {
  if (node->type == MACMBX_NODE_FOLDER || node->unread <= 0)
    return g_strdup(node->name);
  return g_strdup_printf("%s (%d)", node->name, node->unread);
}

/* Add a single node to the tree store under parent_iter (NULL = root) */
static void add_node_to_store(GtkTreeStore *store, GtkTreeIter *parent_iter,
                               MacmbxNode *node) {
  GtkTreeIter iter;
  gboolean is_dir = (node->type == MACMBX_NODE_FOLDER);
  gchar *display = make_display_name(node);
  int weight = (!is_dir && node->unread > 0) ? PANGO_WEIGHT_BOLD
                                               : PANGO_WEIGHT_NORMAL;

  gtk_tree_store_append(store, &iter, parent_iter);
  gtk_tree_store_set(store, &iter,
                     COL_NAME, display,
                     COL_ICON, node_icon_name(node),
                     COL_PATH, node->path,
                     COL_UNREAD, node->unread,
                     COL_IS_DIR, is_dir,
                     COL_WEIGHT, weight,
                     -1);
  g_free(display);

  /* Recurse into folder children */
  if (is_dir && node->children) {
    for (MacmbxNode *child = node->children; child; child = child->next)
      add_node_to_store(store, &iter, child);
  }
}

static gint node_name_cmp(gconstpointer a, gconstpointer b) {
  MacmbxNode *na = *(MacmbxNode **)a, *nb = *(MacmbxNode **)b;
  return g_ascii_strcasecmp(na->name, nb->name);
}

static void load_store_into_treestore(GtkTreeStore *ts, MacmbxStore *store) {
  MacmbxNode *root = macmbx_store_root(store);
  if (!root) return;

  /* Update unread/total counts before displaying */
  macmbx_store_update_counts(store);

  /* Phase 1: add special mailboxes in fixed order */
  GHashTable *shown = g_hash_table_new(g_str_hash, g_str_equal);
  for (int i = 0; i < N_SPECIALS; i++) {
    MacmbxNode *node = NULL;
    if (SPECIAL_ORDER[i] != MACMBX_TYPE_NORMAL)
      node = macmbx_store_find_special(store, SPECIAL_ORDER[i]);
    else
      node = macmbx_store_find_by_name(store, SPECIAL_NAMES[i]);
    if (!node) continue;
    add_node_to_store(ts, NULL, node);
    g_hash_table_insert(shown, node->name, node);
  }

  /* Phase 2: add remaining nodes (folders first, then mailboxes, sorted) */
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
  for (guint i = 0; i < folders->len; i++)
    add_node_to_store(ts, NULL, g_ptr_array_index(folders, i));

  /* Add regular mailboxes */
  for (guint i = 0; i < mailboxes->len; i++)
    add_node_to_store(ts, NULL, g_ptr_array_index(mailboxes, i));

  g_ptr_array_free(folders, TRUE);
  g_ptr_array_free(mailboxes, TRUE);
}

/* ── Restore expand/collapse state after populating the tree ──── */

static gboolean restore_expand_foreach(GtkTreeModel *model, GtkTreePath *path,
                                        GtkTreeIter *iter, gpointer data) {
  GtkTreeView *tree_view = GTK_TREE_VIEW(data);
  gboolean is_dir = FALSE;
  gchar *mbox_path = NULL;
  gtk_tree_model_get(model, iter, COL_IS_DIR, &is_dir, COL_PATH, &mbox_path, -1);
  if (is_dir && mbox_path && is_folder_expanded(mbox_path))
    gtk_tree_view_expand_row(tree_view, path, FALSE);
  g_free(mbox_path);
  return FALSE; /* continue iteration */
}

static void restore_expanded_state(GtkTreeView *tree_view) {
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  if (!model) return;
  gtk_tree_model_foreach(model, restore_expand_foreach, tree_view);
}

/* ── Public API ────────────────────────────────────────────────── */

GtkWidget *gtk_mailbox_tree_new(void) {
  /* Load persisted expand state on first call */
  if (!g_expanded_loaded) {
    load_expanded_state();
    g_expanded_loaded = true;
  }

  /* Create the GtkTreeStore */
  GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS,
      G_TYPE_STRING,   /* COL_NAME */
      G_TYPE_STRING,   /* COL_ICON */
      G_TYPE_STRING,   /* COL_PATH */
      G_TYPE_INT,      /* COL_UNREAD */
      G_TYPE_BOOLEAN,  /* COL_IS_DIR */
      G_TYPE_INT       /* COL_WEIGHT */
  );

  /* Create the GtkTreeView */
  GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  g_object_unref(store); /* tree view holds ref */

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), FALSE);
  gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(tree_view), TRUE);
  gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(tree_view), FALSE);
  gtk_tree_view_set_level_indentation(GTK_TREE_VIEW(tree_view), 4);
  gtk_widget_add_css_class(tree_view, "mb-sidebar");

  /* Single column: icon + name */
  GtkTreeViewColumn *col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_expand(col, TRUE);

  /* Icon cell renderer */
  GtkCellRenderer *icon_renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(col, icon_renderer, FALSE);
  gtk_tree_view_column_add_attribute(col, icon_renderer, "icon-name", COL_ICON);

  /* Text cell renderer */
  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
  g_object_set(text_renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_pack_start(col, text_renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, text_renderer, "text", COL_NAME);
  gtk_tree_view_column_add_attribute(col, text_renderer, "weight", COL_WEIGHT);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

  /* Selection mode */
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
  gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);

  /* Connect signals */
  g_signal_connect(tree_view, "row-activated",
                   G_CALLBACK(on_row_activated), NULL);
  g_signal_connect(tree_view, "row-expanded",
                   G_CALLBACK(on_row_expanded), NULL);
  g_signal_connect(tree_view, "row-collapsed",
                   G_CALLBACK(on_row_collapsed), NULL);

  /* Drop target: messages and mailbox reorder */
  GtkDropTarget *drop = gtk_drop_target_new(G_TYPE_STRING,
                          GDK_ACTION_MOVE | GDK_ACTION_COPY);
  g_signal_connect(drop, "drop", G_CALLBACK(on_tree_drop), tree_view);
  g_signal_connect(drop, "enter", G_CALLBACK(on_tree_drop_enter), tree_view);
  g_signal_connect(drop, "motion", G_CALLBACK(on_tree_drop_motion), tree_view);
  g_signal_connect(drop, "leave", G_CALLBACK(on_tree_drop_leave), tree_view);
  gtk_widget_add_controller(tree_view, GTK_EVENT_CONTROLLER(drop));

  /* Drag source: drag mailboxes/folders for reorder */
  GtkDragSource *drag = gtk_drag_source_new();
  gtk_drag_source_set_actions(drag, GDK_ACTION_MOVE);
  g_signal_connect(drag, "prepare", G_CALLBACK(on_sidebar_drag_prepare), tree_view);
  gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(drag),
                                              GTK_PHASE_BUBBLE);
  gtk_widget_add_controller(tree_view, GTK_EVENT_CONTROLLER(drag));

  return tree_view;
}

void gtk_mailbox_tree_load(GtkWidget *tree, MacmbxStore *store) {
  if (!GTK_IS_TREE_VIEW(tree) || !store) return;

  GtkTreeStore *ts = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
  if (!ts) return;

  gtk_tree_store_clear(ts);
  load_store_into_treestore(ts, store);

  /* Restore expand/collapse state */
  restore_expanded_state(GTK_TREE_VIEW(tree));
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
