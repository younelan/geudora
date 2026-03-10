/*
 * gEudora - GTK4 Mail Client
 * Main application entry point and UI layout
 */

#include "../gEditCtrl/editor_control.h"
#include "../gEditCtrl/geditctrl.h"
#include "compose_window.h"
#include "gtk_icons.h"
#include "gtk_mailbox.h"
#include "gtk_menus.h"
#include "gtk_messagelist.h"
#include "gtk_prefs.h"
#include "gtk_settings.h"
#include "gtk_toolbar_dock.h"
#include "mailbox.h" /* For MessageSummary */
#include "message.h"
#include "toc.h"
#include <gtk/gtk.h>

/* Application state */
typedef struct {
  GtkWidget *window;
  GtkWidget *main_box;
  GtkWidget *mailbox_tree;
  GtkWidget *message_list;
  GtkWidget *message_preview;
  GtkWidget *status_bar;
  DockableToolbar *main_toolbar;
  GListStore *message_store;
  GtkSingleSelection *selection_model;
  GtkTextBuffer *preview_buffer;
  AppSettings *settings;
  gchar *current_mailbox_path;
} AppState;

/* Global variables for legacy compatibility */
RootSpec Root;
bool CommandPeriod;
long YieldTicks;
OSErr inProgress = 1;      /* From MachOPreComp.pch */
OSErr cacheFault = -23042; /* From MachOPreComp.pch */
OSErr userCancelled = -29999;

static AppState app_state = {0};

/* Forward declarations */
static GtkWidget *create_message_list(void);

/* Message list factory callbacks */
static void setup_cb(GtkSignalListItemFactory *self, GtkListItem *list_item,
                     gpointer user_data) {
  GtkWidget *label = gtk_label_new(NULL);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_list_item_set_child(list_item, label);
}

static void bind_from_cb(GtkSignalListItemFactory *self, GtkListItem *list_item,
                         gpointer user_data) {
  GtkMessageListItem *msg =
      GTK_MESSAGELIST_ITEM(gtk_list_item_get_item(list_item));
  GtkWidget *label = gtk_list_item_get_child(list_item);
  gtk_label_set_text(GTK_LABEL(label), gtk_messagelist_item_get_from(msg));
}

static void bind_subject_cb(GtkSignalListItemFactory *self,
                            GtkListItem *list_item, gpointer user_data) {
  GtkMessageListItem *msg =
      GTK_MESSAGELIST_ITEM(gtk_list_item_get_item(list_item));
  GtkWidget *label = gtk_list_item_get_child(list_item);
  gtk_label_set_text(GTK_LABEL(label), gtk_messagelist_item_get_subject(msg));
}

static void bind_date_cb(GtkSignalListItemFactory *self, GtkListItem *list_item,
                         gpointer user_data) {
  GtkMessageListItem *msg =
      GTK_MESSAGELIST_ITEM(gtk_list_item_get_item(list_item));
  GtkWidget *label = gtk_list_item_get_child(list_item);
  gtk_label_set_text(GTK_LABEL(label), gtk_messagelist_item_get_date(msg));
}

static void bind_size_cb(GtkSignalListItemFactory *self, GtkListItem *list_item,
                         gpointer user_data) {
  GtkMessageListItem *msg =
      GTK_MESSAGELIST_ITEM(gtk_list_item_get_item(list_item));
  GtkWidget *label = gtk_list_item_get_child(list_item);
  gtk_label_set_text(GTK_LABEL(label), gtk_messagelist_item_get_size(msg));
}

/* Mailbox selection callback */
static void on_mailbox_selected(GtkTreeView *tree_view, gpointer user_data) {
  GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view);
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gchar *path;
    gchar *name;

    gtk_tree_model_get(model, &iter, 0, &name, 1, &path, -1);
    g_print("Selected mailbox: %s (%s)\n", name, path);

    /* Update current mailbox path */
    if (app_state.current_mailbox_path) {
      g_free(app_state.current_mailbox_path);
    }
    app_state.current_mailbox_path = g_strdup(path);

    /* Load messages for this mailbox */
    /* Clear existing */
    g_list_store_remove_all(app_state.message_store);

    /* Construct TOC path */
    gchar *toc_path = g_strconcat(path, ".toc", NULL);
    TOCHandle toc = toc_load(toc_path);
    g_free(toc_path);

    if (toc) {
      int count = 0;
      MessageSummary *summaries = toc_get_summaries(toc, &count);

      for (int i = 0; i < count; i++) {
        GtkMessageListItem *msg = gtk_messagelist_item_new(&summaries[i], i);
        g_list_store_append(app_state.message_store, msg);
        g_object_unref(msg);
      }
      /* toc_free(toc); don't free yet as summaries rely on it? No, summaries
       * are copied in eudora_message_new */
      /* Actually eudora_message_new copies strings, so we can free toc if we
       * wanted to, but let's be safe */
    }

    g_free(name);
    g_free(path);
  }
}

/* Message selection callback */
static void on_message_selection_changed(GtkSelectionModel *model,
                                         guint position, guint n_items,
                                         gpointer user_data) {
  /* Get selected item */
  GtkMessageListItem *msg = GTK_MESSAGELIST_ITEM(
      gtk_single_selection_get_selected_item(app_state.selection_model));

  if (msg) {
    /* Update preview */
    const char *subject = gtk_messagelist_item_get_subject(msg);
    const char *from = gtk_messagelist_item_get_from(msg);
    char *preview_text = g_strdup_printf(
        "From: %s\nSubject: %s\n\n[Body preview would go here]", from, subject);
    gtk_text_buffer_set_text(app_state.preview_buffer, preview_text, -1);
    g_free(preview_text);
  }
}

/* Simple action handlers */
static void action_quit(GSimpleAction *action, GVariant *parameter,
                        gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_application_quit(g_application_get_default());
}

static void action_new_message(GSimpleAction *action, GVariant *parameter,
                               gpointer user_data) {
  /* Handle both action system calls and direct button clicks */
  GtkApplication *app = NULL;

  if (action != NULL) {
    /* Called from action system */
    app = GTK_APPLICATION(user_data);
  } else {
    /* Called from toolbar button - user_data is NULL, use app from app_state */
    app = NULL;
  }

  GtkWindow *main_window = app ? gtk_application_get_active_window(app)
                               : GTK_WINDOW(app_state.window);
  if (!main_window) {
    g_warning("No main window available");
    return;
  }

  /* Create a new compose window */
  GtkWidget *compose_window = create_compose_window(main_window);
  gtk_window_present(GTK_WINDOW(compose_window));
}

static void action_open(GSimpleAction *action, GVariant *parameter,
                        gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Open\n");
}

static void action_save(GSimpleAction *action, GVariant *parameter,
                        gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Save\n");
}

static void action_print(GSimpleAction *action, GVariant *parameter,
                         gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Print\n");
}

static void action_undo(GSimpleAction *action, GVariant *parameter,
                        gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Undo\n");
}

static void action_cut(GSimpleAction *action, GVariant *parameter,
                       gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Cut\n");
}

static void action_copy(GSimpleAction *action, GVariant *parameter,
                        gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Copy\n");
}

static void action_paste(GSimpleAction *action, GVariant *parameter,
                         gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Paste\n");
}

static void action_select_all(GSimpleAction *action, GVariant *parameter,
                              gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Select All\n");
}

static void action_reply(GSimpleAction *action, GVariant *parameter,
                         gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Reply\n");
}

static void action_reply_all(GSimpleAction *action, GVariant *parameter,
                             gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Reply All\n");
}

static void action_forward(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Forward\n");
}

static void action_delete(GSimpleAction *action, GVariant *parameter,
                          gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;
  g_print("Delete\n");
}

static void action_preferences(GSimpleAction *action, GVariant *parameter,
                               gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  /* Show preferences dialog */
  SettingsDialog *sd =
      create_settings_dialog(GTK_WINDOW(app_state.window), app_state.settings);
  GtkWidget *dialog = get_settings_dialog_widget(sd);
  gtk_window_present(GTK_WINDOW(dialog));
}

/* Create mailbox tree view */
static GtkWidget *create_mailbox_tree(void) {
  /* Create default mailboxes if they don't exist */
  gtk_mailbox_create_default();

  /* Create and populate the mailbox tree */
  GtkWidget *tree = gtk_mailbox_tree_new();
  gtk_mailbox_tree_load(tree);

  return tree;
}

/* Create message list view using GTK4 GtkColumnView */
static GtkWidget *create_message_list(void) {
  /* Create list store for messages */
  app_state.message_store = g_list_store_new(GTK_TYPE_MESSAGELIST_ITEM);
  app_state.selection_model =
      gtk_single_selection_new(G_LIST_MODEL(app_state.message_store));

  /* Connect selection change signal */
  g_signal_connect(app_state.selection_model, "selection-changed",
                   G_CALLBACK(on_message_selection_changed), NULL);

  /* Create column view */
  GtkWidget *view =
      gtk_column_view_new(GTK_SELECTION_MODEL(app_state.selection_model));
  gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(view), TRUE);
  gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(view), TRUE);

  /* From Column */
  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_from_cb), NULL);
  GtkColumnViewColumn *column = gtk_column_view_column_new("From", factory);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_fixed_width(column, 150);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(view), column);
  g_object_unref(column);

  /* Subject Column */
  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_subject_cb), NULL);
  column = gtk_column_view_column_new("Subject", factory);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(view), column);
  g_object_unref(column);

  /* Date Column */
  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_date_cb), NULL);
  column = gtk_column_view_column_new("Date", factory);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_fixed_width(column, 120);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(view), column);
  g_object_unref(column);

  /* Size Column */
  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_size_cb), NULL);
  column = gtk_column_view_column_new("Size", factory);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_fixed_width(column, 80);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(view), column);
  g_object_unref(column);

  return view;
}

/* Create message preview */
static GtkWidget *create_message_preview(void) {
  app_state.preview_buffer = gtk_text_buffer_new(NULL);
  gtk_text_buffer_set_text(app_state.preview_buffer,
                           "Select a message to preview", -1);

  GtkWidget *view = gtk_text_view_new_with_buffer(app_state.preview_buffer);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);

  return view;
}

/* Toolbar button callbacks - simple wrappers */
static void toolbar_new_message_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  action_new_message(NULL, NULL, NULL);
}

static void toolbar_reply_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  action_reply(NULL, NULL, NULL);
}

static void toolbar_reply_all_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  action_reply_all(NULL, NULL, NULL);
}

static void toolbar_forward_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  action_forward(NULL, NULL, NULL);
}

static void toolbar_delete_cb(GtkButton *button, gpointer user_data) {
  (void)button;
  (void)user_data;
  action_delete(NULL, NULL, NULL);
}

/* Create dockable toolbars */
static void create_toolbars(GtkBox *toolbar_container) {
  /* Main toolbar */
  app_state.main_toolbar = create_dockable_toolbar("Main Toolbar");

  /* Add buttons to main toolbar */
  GtkWidget *btn =
      create_toolbar_button(ICON_NEW_MESSAGE, "New", ICON_SIZE_LARGE);
  g_signal_connect(btn, "clicked", G_CALLBACK(toolbar_new_message_cb), NULL);
  toolbar_add_button(app_state.main_toolbar, btn);

  btn = create_toolbar_button(ICON_REPLY, "Reply", ICON_SIZE_LARGE);
  g_signal_connect(btn, "clicked", G_CALLBACK(toolbar_reply_cb), NULL);
  toolbar_add_button(app_state.main_toolbar, btn);

  btn = create_toolbar_button(ICON_REPLY_ALL, "Reply All", ICON_SIZE_LARGE);
  g_signal_connect(btn, "clicked", G_CALLBACK(toolbar_reply_all_cb), NULL);
  toolbar_add_button(app_state.main_toolbar, btn);

  btn = create_toolbar_button(ICON_FORWARD, "Forward", ICON_SIZE_LARGE);
  g_signal_connect(btn, "clicked", G_CALLBACK(toolbar_forward_cb), NULL);
  toolbar_add_button(app_state.main_toolbar, btn);

  btn = create_toolbar_button(ICON_DELETE, "Delete", ICON_SIZE_LARGE);
  g_signal_connect(btn, "clicked", G_CALLBACK(toolbar_delete_cb), NULL);
  toolbar_add_button(app_state.main_toolbar, btn);

  toolbar_add_separator(app_state.main_toolbar);

  btn = create_toolbar_button(ICON_CHECK_MAIL, "Check Mail", ICON_SIZE_LARGE);
  toolbar_add_button(app_state.main_toolbar, btn);

  btn = create_toolbar_button(ICON_SEND_QUEUED, "Send Queued", ICON_SIZE_LARGE);
  toolbar_add_button(app_state.main_toolbar, btn);

  /* Set position and add to container */
  set_toolbar_position(app_state.main_toolbar, TOOLBAR_DOCK_TOP);
  gtk_box_append(GTK_BOX(toolbar_container),
                 get_toolbar_widget(app_state.main_toolbar));
}

/* Create main layout */
static GtkWidget *create_main_layout(void) {
  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* Toolbar container */
  GtkWidget *toolbar_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(toolbar_container, "toolbar-area");
  create_toolbars(GTK_BOX(toolbar_container));
  gtk_box_append(GTK_BOX(main_box), toolbar_container);

  /* Main content area with paned layout */
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(hpaned), 200);

  /* Left pane: Mailbox tree */
  GtkWidget *left_frame = gtk_frame_new("Mailboxes");
  app_state.mailbox_tree = create_mailbox_tree();
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),
                                app_state.mailbox_tree);
  gtk_frame_set_child(GTK_FRAME(left_frame), scroll);
  gtk_paned_set_start_child(GTK_PANED(hpaned), left_frame);
  gtk_paned_set_resize_start_child(GTK_PANED(hpaned), FALSE);

  /* Connect mailbox selection signal */
  g_signal_connect(app_state.mailbox_tree, "cursor-changed",
                   G_CALLBACK(on_mailbox_selected), NULL);

  /* Right pane: Message list and preview */
  GtkWidget *right_vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(right_vpaned), 250);

  /* Message list */
  GtkWidget *list_frame = gtk_frame_new("Messages");
  app_state.message_list = create_message_list();
  scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),
                                app_state.message_list);
  gtk_frame_set_child(GTK_FRAME(list_frame), scroll);
  gtk_paned_set_start_child(GTK_PANED(right_vpaned), list_frame);
  gtk_paned_set_resize_start_child(GTK_PANED(right_vpaned), TRUE);

  /* Message preview */
  GtkWidget *preview_frame = gtk_frame_new("Preview");
  app_state.message_preview = create_message_preview();
  scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),
                                app_state.message_preview);
  gtk_frame_set_child(GTK_FRAME(preview_frame), scroll);
  gtk_paned_set_end_child(GTK_PANED(right_vpaned), preview_frame);
  gtk_paned_set_resize_end_child(GTK_PANED(right_vpaned), TRUE);

  gtk_paned_set_end_child(GTK_PANED(hpaned), right_vpaned);
  gtk_paned_set_resize_end_child(GTK_PANED(hpaned), TRUE);

  gtk_box_append(GTK_BOX(main_box), hpaned);
  gtk_widget_set_vexpand(hpaned, TRUE);
  gtk_widget_set_hexpand(hpaned, TRUE);

  return main_box;
}

/* Application activation */
static void activate(GtkApplication *app, gpointer user_data) {
  (void)user_data;

  app_state.window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(app_state.window), "gEudora - Email Client");
  gtk_window_set_default_size(GTK_WINDOW(app_state.window), 1200, 800);

  /* Initialize icon system */
  init_icon_system("./resources");

  /* Initialize preferences system */
  prefs_init(NULL); /* Uses default ~/.config directory */

  /* Load settings from disk */
  app_state.settings = prefs_load();
  if (!app_state.settings) {
    /* Create default settings if load failed */
    app_state.settings = g_new0(AppSettings, 1);
  }

  /* Set some defaults if not already set */
  if (app_state.settings->check_interval == 0) {
    app_state.settings->check_interval = 5;
  }
  app_state.settings->keep_sent_copy = TRUE;
  app_state.settings->wrap_outgoing = TRUE;
  app_state.settings->include_signature = TRUE;
  app_state.settings->use_ssl = TRUE;

  /* Register application actions */
  GSimpleAction *action;

  action = g_simple_action_new("quit", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_quit), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("new-message", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_new_message), app);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("open", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_open), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("save", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_save), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("print", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_print), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("undo", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_undo), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("cut", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_cut), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("copy", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_copy), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("paste", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_paste), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("select-all", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_select_all), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("reply", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_reply), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("reply-all", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_reply_all), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("forward", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_forward), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("delete", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_delete), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("preferences", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_preferences), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  /* Add stub actions for remaining menu items */
  const char *stub_actions[] = {"open-selection",
                                "open-browser",
                                "close",
                                "save-as",
                                "revert",
                                "page-setup",
                                "paste-quotation",
                                "clear",
                                "wrap-selection",
                                "finish-nickname",
                                "insert-recipient",
                                "insert-emoticon",
                                "speak",
                                "spelling",
                                "redirect",
                                "send-again",
                                "queue",
                                "mark-read",
                                "mark-unread",
                                "mark-junk",
                                "mark-not-junk",
                                "change-status",
                                "change-priority",
                                "change-label",
                                "change-personality",
                                "transfer-in",
                                "transfer-out",
                                "minimize",
                                "bring-to-front",
                                "send-to-back",
                                "tabs",
                                "drawer",
                                "open-scripts",
                                NULL};

  for (int i = 0; stub_actions[i] != NULL; i++) {
    action = g_simple_action_new(stub_actions[i], NULL);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
    g_object_unref(action);
  }

  /* Create main layout */
  app_state.main_box = create_main_layout();
  gtk_window_set_child(GTK_WINDOW(app_state.window), app_state.main_box);

  /* Create system menu bar */
  create_menu_bar(app_state.window);

  gtk_window_present(GTK_WINDOW(app_state.window));
}

int main(int argc, char **argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.geudora.mail", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);

  /* Cleanup */
  cleanup_icon_system();
  prefs_cleanup();

  if (app_state.settings) {
    g_free(app_state.settings);
  }

  g_object_unref(app);

  /* Cleanup app state */
  if (app_state.current_mailbox_path) {
    g_free(app_state.current_mailbox_path);
  }

  return status;
}
