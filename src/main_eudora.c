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
#include "wazoo.h"
#include <gtk/gtk.h>
#include <glib/gstdio.h>

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
  TOCType *current_toc;  /* Currently displayed TOC */
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
static void open_mailbox_tab(const char *name, const char *path);

/*
 * read_message_body - Read message body from mailbox file using TOC summary info.
 * Returns allocated string (caller must g_free), or NULL on error.
 */
static gchar *read_message_body(TOCType *toc, int msg_index) {
  if (!toc || msg_index < 0 || msg_index >= toc->count)
    return NULL;

  MessageSummary *sum = &toc->sums[msg_index];
  FILE *fp = fopen(toc->mailbox.spec.path, "rb");
  if (!fp)
    fp = fopen(toc->path, "rb");
  if (!fp)
    return NULL;

  if (fseek(fp, sum->offset, SEEK_SET) != 0) {
    fclose(fp);
    return NULL;
  }

  long len = sum->length;
  if (len <= 0 || len > 10 * 1024 * 1024) { /* cap at 10 MB */
    fclose(fp);
    return NULL;
  }

  gchar *buf = g_malloc(len + 1);
  size_t nread = fread(buf, 1, len, fp);
  fclose(fp);
  buf[nread] = '\0';

  /* Find body after blank line (headers end with \n\n or \r\n\r\n) */
  const char *body = strstr(buf, "\n\n");
  if (body) {
    body += 2;
    gchar *result = g_strdup(body);
    g_free(buf);
    return result;
  }
  /* No blank line separator found — return the whole thing */
  return buf;
}

/*
 * get_selected_message - Get the currently selected message summary info.
 * Returns the message index, or -1 if nothing selected.
 */
static int get_selected_message_index(void) {
  if (!app_state.selection_model)
    return -1;
  GtkMessageListItem *msg = GTK_MESSAGELIST_ITEM(
      gtk_single_selection_get_selected_item(app_state.selection_model));
  if (!msg)
    return -1;
  return gtk_messagelist_item_get_index(msg);
}

/*
 * extract_reply_address - Extract reply-to address from message headers.
 * Falls back to From header if Reply-To not found.
 */
static gchar *extract_reply_address(TOCType *toc, int msg_index) {
  if (!toc || msg_index < 0 || msg_index >= toc->count)
    return NULL;

  MessageSummary *sum = &toc->sums[msg_index];
  FILE *fp = fopen(toc->mailbox.spec.path, "rb");
  if (!fp)
    fp = fopen(toc->path, "rb");
  if (!fp)
    return g_strdup(sum->from);

  if (fseek(fp, sum->offset, SEEK_SET) != 0) {
    fclose(fp);
    return g_strdup(sum->from);
  }

  /* Read just the headers (up to first blank line, max 8KB) */
  char hdr[8192];
  size_t nread = fread(hdr, 1, sizeof(hdr) - 1, fp);
  fclose(fp);
  hdr[nread] = '\0';

  /* Look for Reply-To header first */
  const char *p = strcasestr(hdr, "\nReply-To:");
  if (p) {
    p += 10; /* skip "\nReply-To:" */
    while (*p == ' ' || *p == '\t') p++;
    const char *end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    /* Trim \r */
    while (end > p && (end[-1] == '\r' || end[-1] == '\n')) end--;
    return g_strndup(p, end - p);
  }

  /* Fall back to From header */
  p = strcasestr(hdr, "\nFrom:");
  if (p) {
    p += 6;
    while (*p == ' ' || *p == '\t') p++;
    const char *end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    while (end > p && (end[-1] == '\r' || end[-1] == '\n')) end--;
    return g_strndup(p, end - p);
  }

  return g_strdup(sum->from);
}

/*
 * extract_all_recipients - Extract To and Cc addresses from message headers.
 * For Reply All - returns To and Cc combined.
 */
static void extract_all_recipients(TOCType *toc, int msg_index,
                                   gchar **out_to, gchar **out_cc) {
  *out_to = NULL;
  *out_cc = NULL;
  if (!toc || msg_index < 0 || msg_index >= toc->count)
    return;

  MessageSummary *sum = &toc->sums[msg_index];
  FILE *fp = fopen(toc->mailbox.spec.path, "rb");
  if (!fp)
    fp = fopen(toc->path, "rb");
  if (!fp)
    return;

  if (fseek(fp, sum->offset, SEEK_SET) != 0) {
    fclose(fp);
    return;
  }

  char hdr[8192];
  size_t nread = fread(hdr, 1, sizeof(hdr) - 1, fp);
  fclose(fp);
  hdr[nread] = '\0';

  /* Extract To: */
  const char *p = strcasestr(hdr, "\nTo:");
  if (p) {
    p += 4;
    while (*p == ' ' || *p == '\t') p++;
    const char *end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    while (end > p && (end[-1] == '\r' || end[-1] == '\n')) end--;
    *out_to = g_strndup(p, end - p);
  }

  /* Extract Cc: */
  p = strcasestr(hdr, "\nCc:");
  if (p) {
    p += 4;
    while (*p == ' ' || *p == '\t') p++;
    const char *end = strchr(p, '\n');
    if (!end) end = p + strlen(p);
    while (end > p && (end[-1] == '\r' || end[-1] == '\n')) end--;
    *out_cc = g_strndup(p, end - p);
  }
}

/*
 * quote_text - Add "> " prefix to each line for email quoting.
 */
static gchar *quote_text(const char *text) {
  if (!text || !*text)
    return g_strdup("");

  GString *quoted = g_string_new(NULL);
  const char *p = text;
  while (*p) {
    g_string_append(quoted, "> ");
    const char *eol = strchr(p, '\n');
    if (eol) {
      g_string_append_len(quoted, p, eol - p + 1);
      p = eol + 1;
    } else {
      g_string_append(quoted, p);
      g_string_append_c(quoted, '\n');
      break;
    }
  }
  return g_string_free(quoted, FALSE);
}

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

/* Mailbox selection callback — just tracks which row is highlighted */
static void on_mailbox_selected(GtkTreeView *tree_view, gpointer user_data) {
  (void)user_data;
  (void)tree_view;
  /* Selection tracked for context (e.g. which mailbox to act on).
   * Actual opening happens on double-click (row-activated). */
}

/* Double-click / Enter on mailbox tree → open a tab on the right */
static void on_mailbox_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                  GtkTreeViewColumn *column, gpointer ud) {
  (void)column; (void)ud;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(model, &iter, path))
    return;

  gchar *name = NULL;
  gchar *mb_path = NULL;
  gtk_tree_model_get(model, &iter, 0, &name, 1, &mb_path, -1);

  if (mb_path && *mb_path)
    open_mailbox_tab(name, mb_path);

  g_free(name);
  g_free(mb_path);
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

/* Double-click / Enter on message list → open message in its own window */
static void on_message_activated(GtkColumnView *col_view, guint position,
                                 gpointer user_data) {
  (void)user_data;

  /* The TOC is stored on the parent vpaned */
  GtkWidget *vpaned = gtk_widget_get_ancestor(GTK_WIDGET(col_view),
                                               GTK_TYPE_PANED);
  if (!vpaned)
    return;

  TOCType *toc = g_object_get_data(G_OBJECT(vpaned), "toc");
  if (!toc || (int)position >= toc->count)
    return;

  GetAMessage(toc, (short)position, NULL, NULL, true);
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

  int idx = get_selected_message_index();
  TOCType *toc = app_state.current_toc;
  if (idx < 0 || !toc) {
    g_print("Reply: no message selected\n");
    return;
  }

  MessageSummary *sum = &toc->sums[idx];
  gchar *reply_addr = extract_reply_address(toc, idx);
  gchar *body = read_message_body(toc, idx);
  gchar *quoted = quote_text(body);

  GtkWindow *main_window = GTK_WINDOW(app_state.window);
  GtkWidget *compose = create_compose_window(main_window);

  if (reply_addr)
    compose_window_set_to(compose, reply_addr);

  /* Build Re: subject */
  const char *subj = sum->subj;
  gchar *re_subj;
  if (subj && (g_ascii_strncasecmp(subj, "Re:", 3) == 0 ||
               g_ascii_strncasecmp(subj, "Re: ", 4) == 0))
    re_subj = g_strdup(subj);
  else
    re_subj = g_strdup_printf("Re: %s", subj ? subj : "");
  compose_window_set_subject(compose, re_subj);

  /* Set quoted body */
  if (quoted) {
    gchar *reply_body = g_strdup_printf("On %s wrote:\n%s",
                                         sum->from, quoted);
    compose_window_set_text(compose, reply_body);
    g_free(reply_body);
  }

  gtk_window_present(GTK_WINDOW(compose));

  g_free(reply_addr);
  g_free(body);
  g_free(quoted);
  g_free(re_subj);
}

static void action_reply_all(GSimpleAction *action, GVariant *parameter,
                             gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  int idx = get_selected_message_index();
  TOCType *toc = app_state.current_toc;
  if (idx < 0 || !toc) {
    g_print("Reply All: no message selected\n");
    return;
  }

  MessageSummary *sum = &toc->sums[idx];
  gchar *reply_addr = extract_reply_address(toc, idx);
  gchar *orig_to = NULL, *orig_cc = NULL;
  extract_all_recipients(toc, idx, &orig_to, &orig_cc);
  gchar *body = read_message_body(toc, idx);
  gchar *quoted = quote_text(body);

  GtkWindow *main_window = GTK_WINDOW(app_state.window);
  GtkWidget *compose = create_compose_window(main_window);

  if (reply_addr)
    compose_window_set_to(compose, reply_addr);

  /* Combine original To + Cc into Cc field */
  if (orig_to && orig_cc) {
    gchar *combined = g_strdup_printf("%s, %s", orig_to, orig_cc);
    compose_window_set_cc(compose, combined);
    g_free(combined);
  } else if (orig_to) {
    compose_window_set_cc(compose, orig_to);
  } else if (orig_cc) {
    compose_window_set_cc(compose, orig_cc);
  }

  const char *subj = sum->subj;
  gchar *re_subj;
  if (subj && (g_ascii_strncasecmp(subj, "Re:", 3) == 0))
    re_subj = g_strdup(subj);
  else
    re_subj = g_strdup_printf("Re: %s", subj ? subj : "");
  compose_window_set_subject(compose, re_subj);

  if (quoted) {
    gchar *reply_body = g_strdup_printf("On %s wrote:\n%s",
                                         sum->from, quoted);
    compose_window_set_text(compose, reply_body);
    g_free(reply_body);
  }

  gtk_window_present(GTK_WINDOW(compose));

  g_free(reply_addr);
  g_free(orig_to);
  g_free(orig_cc);
  g_free(body);
  g_free(quoted);
  g_free(re_subj);
}

static void action_forward(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  int idx = get_selected_message_index();
  TOCType *toc = app_state.current_toc;
  if (idx < 0 || !toc) {
    g_print("Forward: no message selected\n");
    return;
  }

  MessageSummary *sum = &toc->sums[idx];
  gchar *body = read_message_body(toc, idx);

  GtkWindow *main_window = GTK_WINDOW(app_state.window);
  GtkWidget *compose = create_compose_window(main_window);

  /* Fwd: subject */
  const char *subj = sum->subj;
  gchar *fwd_subj = g_strdup_printf("Fwd: %s", subj ? subj : "");
  compose_window_set_subject(compose, fwd_subj);

  /* Forwarded body */
  if (body) {
    gchar *fwd_body = g_strdup_printf(
        "---------- Forwarded message ----------\n"
        "From: %s\nSubject: %s\n\n%s",
        sum->from, subj ? subj : "", body);
    compose_window_set_text(compose, fwd_body);
    g_free(fwd_body);
  }

  gtk_window_present(GTK_WINDOW(compose));

  g_free(body);
  g_free(fwd_subj);
}

static void action_delete(GSimpleAction *action, GVariant *parameter,
                          gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  int idx = get_selected_message_index();
  TOCType *toc = app_state.current_toc;
  if (idx < 0 || !toc) {
    g_print("Delete: no message selected\n");
    return;
  }

  /* Mark the message as deleted in the TOC */
  toc->sums[idx].opts |= OPT_DELETED;
  toc->sums[idx].state = MESG_ERR; /* visual indicator */
  TOCSetDirty(toc, true);

  /* Remove from the visible list */
  if (app_state.message_store) {
    guint n = g_list_model_get_n_items(G_LIST_MODEL(app_state.message_store));
    for (guint i = 0; i < n; i++) {
      GtkMessageListItem *item = g_list_model_get_item(
          G_LIST_MODEL(app_state.message_store), i);
      if (item && gtk_messagelist_item_get_index(item) == idx) {
        g_list_store_remove(app_state.message_store, i);
        g_object_unref(item);
        break;
      }
      if (item) g_object_unref(item);
    }
  }

  g_print("Deleted message %d: %s\n", idx, toc->sums[idx].subj);
}

static void action_check_mail(GSimpleAction *action, GVariant *parameter,
                              gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  g_print("Checking mail...\n");
  /* TODO: Trigger POP3/IMAP mail check via GetMail/CheckMail */
  /* For now, show status */
  if (app_state.preview_buffer) {
    gtk_text_buffer_set_text(app_state.preview_buffer,
                             "Checking for new mail...", -1);
  }
}

static void action_send_queued(GSimpleAction *action, GVariant *parameter,
                               gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  g_print("Sending queued messages...\n");
  /* TODO: Call SendQueuedMessages from sendmail.c */
  if (app_state.preview_buffer) {
    gtk_text_buffer_set_text(app_state.preview_buffer,
                             "Sending queued messages...", -1);
  }
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

/* ─── Wazoo window action handlers ────────────────────────────────────── */

/* OpenABWin declared in nickwin.c */
extern void OpenABWin(void);

static void action_address_book(GSimpleAction *action, GVariant *parameter,
                                gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;
  OpenABWin();
}

static void action_filters(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;

  GtkWidget *win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(win), "Filters");
  gtk_window_set_default_size(GTK_WINDOW(win), 700, 500);
  gtk_window_set_transient_for(GTK_WINDOW(win),
                               GTK_WINDOW(app_state.window));

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(vbox, 8);
  gtk_widget_set_margin_end(vbox, 8);
  gtk_widget_set_margin_top(vbox, 8);
  gtk_widget_set_margin_bottom(vbox, 8);

  /* Toolbar */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *btn_new = gtk_button_new_with_label("New");
  GtkWidget *btn_del = gtk_button_new_with_label("Delete");
  GtkWidget *btn_up = gtk_button_new_with_label("Up");
  GtkWidget *btn_down = gtk_button_new_with_label("Down");
  gtk_box_append(GTK_BOX(toolbar), btn_new);
  gtk_box_append(GTK_BOX(toolbar), btn_del);
  gtk_box_append(GTK_BOX(toolbar), btn_up);
  gtk_box_append(GTK_BOX(toolbar), btn_down);
  gtk_box_append(GTK_BOX(vbox), toolbar);

  /* HPaned: filter list on left, rule editor on right */
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(hpaned), 250);

  /* Left: filter list */
  GtkWidget *list_scroll = gtk_scrolled_window_new();
  GtkWidget *list_box = gtk_list_box_new();
  gtk_list_box_append(GTK_LIST_BOX(list_box),
                      gtk_label_new("(filters appear here)"));
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), list_box);
  gtk_paned_set_start_child(GTK_PANED(hpaned), list_scroll);

  /* Right: filter rule editor */
  GtkWidget *rule_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

  /* Match section */
  GtkWidget *match_frame = gtk_frame_new("Match");
  GtkWidget *match_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(match_box, 8);
  gtk_widget_set_margin_end(match_box, 8);
  gtk_widget_set_margin_top(match_box, 4);
  gtk_widget_set_margin_bottom(match_box, 4);

  /* Header dropdown + contains + value */
  GtkWidget *cond_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *header_combo = gtk_drop_down_new_from_strings(
      (const char *[]){"From", "To", "Subject", "Any Header", NULL});
  GtkWidget *verb_combo = gtk_drop_down_new_from_strings(
      (const char *[]){"contains", "doesn't contain", "is", "is not",
                        "starts with", "ends with", NULL});
  GtkWidget *value_entry = gtk_entry_new();
  gtk_widget_set_hexpand(value_entry, TRUE);
  gtk_box_append(GTK_BOX(cond_row), header_combo);
  gtk_box_append(GTK_BOX(cond_row), verb_combo);
  gtk_box_append(GTK_BOX(cond_row), value_entry);
  gtk_box_append(GTK_BOX(match_box), cond_row);
  gtk_frame_set_child(GTK_FRAME(match_frame), match_box);
  gtk_box_append(GTK_BOX(rule_box), match_frame);

  /* Action section */
  GtkWidget *action_frame = gtk_frame_new("Action");
  GtkWidget *action_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(action_box, 8);
  gtk_widget_set_margin_end(action_box, 8);
  gtk_widget_set_margin_top(action_box, 4);
  gtk_widget_set_margin_bottom(action_box, 4);

  GtkWidget *action_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *action_combo = gtk_drop_down_new_from_strings(
      (const char *[]){"Transfer To", "Skip Rest", "Delete",
                        "Mark Read", "Label", "Play Sound", NULL});
  GtkWidget *action_value = gtk_entry_new();
  gtk_widget_set_hexpand(action_value, TRUE);
  gtk_box_append(GTK_BOX(action_row), action_combo);
  gtk_box_append(GTK_BOX(action_row), action_value);
  gtk_box_append(GTK_BOX(action_box), action_row);
  gtk_frame_set_child(GTK_FRAME(action_frame), action_box);
  gtk_box_append(GTK_BOX(rule_box), action_frame);

  /* Incoming/Outgoing/Manual checkboxes */
  GtkWidget *when_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(when_box),
                 gtk_check_button_new_with_label("Incoming"));
  gtk_box_append(GTK_BOX(when_box),
                 gtk_check_button_new_with_label("Outgoing"));
  gtk_box_append(GTK_BOX(when_box),
                 gtk_check_button_new_with_label("Manual"));
  gtk_box_append(GTK_BOX(rule_box), when_box);

  gtk_paned_set_end_child(GTK_PANED(hpaned), rule_box);
  gtk_widget_set_vexpand(hpaned, TRUE);
  gtk_box_append(GTK_BOX(vbox), hpaned);

  gtk_window_set_child(GTK_WINDOW(win), vbox);
  gtk_window_present(GTK_WINDOW(win));
}

static void action_personalities(GSimpleAction *action, GVariant *parameter,
                                 gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;

  GtkWidget *win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(win), "Personalities");
  gtk_window_set_default_size(GTK_WINDOW(win), 600, 400);
  gtk_window_set_transient_for(GTK_WINDOW(win),
                               GTK_WINDOW(app_state.window));

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(vbox, 8);
  gtk_widget_set_margin_end(vbox, 8);
  gtk_widget_set_margin_top(vbox, 8);
  gtk_widget_set_margin_bottom(vbox, 8);

  /* Toolbar */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(toolbar), gtk_button_new_with_label("New"));
  gtk_box_append(GTK_BOX(toolbar), gtk_button_new_with_label("Delete"));
  gtk_box_append(GTK_BOX(vbox), toolbar);

  /* HPaned: personality list on left, settings on right */
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(hpaned), 180);

  GtkWidget *list_scroll = gtk_scrolled_window_new();
  GtkWidget *list_box = gtk_list_box_new();
  gtk_list_box_append(GTK_LIST_BOX(list_box),
                      gtk_label_new("Dominant"));
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), list_box);
  gtk_paned_set_start_child(GTK_PANED(hpaned), list_scroll);

  /* Right: personality settings */
  GtkWidget *detail_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  const char *labels[] = {"Real Name:", "Email:", "POP Server:",
                           "SMTP Server:", "Login:", NULL};
  for (int i = 0; labels[i]; i++) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget *lbl = gtk_label_new(labels[i]);
    gtk_widget_set_size_request(lbl, 100, -1);
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(row), lbl);
    gtk_box_append(GTK_BOX(row), entry);
    gtk_box_append(GTK_BOX(detail_box), row);
  }

  /* Check interval */
  GtkWidget *check_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *check_lbl = gtk_label_new("Check every:");
  gtk_widget_set_size_request(check_lbl, 100, -1);
  gtk_label_set_xalign(GTK_LABEL(check_lbl), 1.0);
  GtkWidget *spin = gtk_spin_button_new_with_range(0, 999, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), 5);
  gtk_box_append(GTK_BOX(check_row), check_lbl);
  gtk_box_append(GTK_BOX(check_row), spin);
  gtk_box_append(GTK_BOX(check_row), gtk_label_new("minutes"));
  gtk_box_append(GTK_BOX(detail_box), check_row);

  /* Leave on server checkbox */
  gtk_box_append(GTK_BOX(detail_box),
                 gtk_check_button_new_with_label("Leave mail on server"));
  gtk_box_append(GTK_BOX(detail_box),
                 gtk_check_button_new_with_label("Use SSL"));

  gtk_paned_set_end_child(GTK_PANED(hpaned), detail_box);
  gtk_widget_set_vexpand(hpaned, TRUE);
  gtk_box_append(GTK_BOX(vbox), hpaned);

  gtk_window_set_child(GTK_WINDOW(win), vbox);
  gtk_window_present(GTK_WINDOW(win));
}

static void action_signatures(GSimpleAction *action, GVariant *parameter,
                              gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;

  GtkWidget *win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(win), "Signatures");
  gtk_window_set_default_size(GTK_WINDOW(win), 500, 400);
  gtk_window_set_transient_for(GTK_WINDOW(win),
                               GTK_WINDOW(app_state.window));

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(vbox, 8);
  gtk_widget_set_margin_end(vbox, 8);
  gtk_widget_set_margin_top(vbox, 8);
  gtk_widget_set_margin_bottom(vbox, 8);

  /* Toolbar */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(toolbar), gtk_button_new_with_label("New"));
  gtk_box_append(GTK_BOX(toolbar), gtk_button_new_with_label("Delete"));
  gtk_box_append(GTK_BOX(toolbar), gtk_button_new_with_label("Rename"));
  gtk_box_append(GTK_BOX(vbox), toolbar);

  /* HPaned: signature list on left, editor on right */
  GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(hpaned), 150);

  GtkWidget *list_scroll = gtk_scrolled_window_new();
  GtkWidget *list_box = gtk_list_box_new();
  gtk_list_box_append(GTK_LIST_BOX(list_box),
                      gtk_label_new("Standard"));
  gtk_list_box_append(GTK_LIST_BOX(list_box),
                      gtk_label_new("Alternate"));
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), list_box);
  gtk_paned_set_start_child(GTK_PANED(hpaned), list_scroll);

  /* Right: signature editor */
  GtkWidget *edit_scroll = gtk_scrolled_window_new();
  GtkWidget *editor = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(editor), GTK_WRAP_WORD);
  GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor));
  gtk_text_buffer_set_text(buf,
      "-- \nYour Name\nyour@email.com", -1);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(edit_scroll), editor);
  gtk_paned_set_end_child(GTK_PANED(hpaned), edit_scroll);
  gtk_widget_set_vexpand(hpaned, TRUE);
  gtk_box_append(GTK_BOX(vbox), hpaned);

  gtk_window_set_child(GTK_WINDOW(win), vbox);
  gtk_window_present(GTK_WINDOW(win));
}

static void action_statistics(GSimpleAction *action, GVariant *parameter,
                              gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;
  /* Delegate to existing OpenStatWin if available */
  extern void OpenStatWin(void);
  OpenStatWin();
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

/* ── Mailbox management buttons (New Mailbox, New Folder, Remove) ── */

/* Get the parent directory for new items: if a folder is selected, create
 * inside it; otherwise create at root mailboxes dir. */
static const char *mb_get_target_dir(void) {
  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(app_state.mailbox_tree));
  GtkTreeIter iter;
  GtkTreeModel *model =
      gtk_tree_view_get_model(GTK_TREE_VIEW(app_state.mailbox_tree));
  if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
    gchar *sel_path = NULL;
    gtk_tree_model_get(model, &iter, 1, &sel_path, -1);
    if (sel_path && g_file_test(sel_path, G_FILE_TEST_IS_DIR)) {
      /* Selected item is a folder — create inside it */
      return sel_path;  /* caller must g_free */
    }
    if (sel_path) {
      /* Selected item is a mailbox file — create in its parent dir */
      gchar *parent = g_path_get_dirname(sel_path);
      g_free(sel_path);
      return parent;  /* caller must g_free */
    }
  }
  return g_strdup(prefs_get_mailboxes_path());
}

static void on_mb_new_mailbox(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  gchar *dir = (gchar *)mb_get_target_dir();
  char name[256], path[1024];
  snprintf(name, sizeof(name), "Untitled Mailbox");
  snprintf(path, sizeof(path), "%s/%s", dir, name);
  int n = 1;
  while (g_file_test(path, G_FILE_TEST_EXISTS)) {
    n++;
    snprintf(name, sizeof(name), "Untitled Mailbox %d", n);
    snprintf(path, sizeof(path), "%s/%s", dir, name);
  }
  FILE *f = fopen(path, "w");
  if (f) fclose(f);
  g_free(dir);
  gtk_mailbox_tree_refresh(app_state.mailbox_tree);
}

static void on_mb_new_folder(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  gchar *dir = (gchar *)mb_get_target_dir();
  char name[256], path[1024];
  snprintf(name, sizeof(name), "Untitled Folder");
  snprintf(path, sizeof(path), "%s/%s", dir, name);
  int n = 1;
  while (g_file_test(path, G_FILE_TEST_EXISTS)) {
    n++;
    snprintf(name, sizeof(name), "Untitled Folder %d", n);
    snprintf(path, sizeof(path), "%s/%s", dir, name);
  }
  g_mkdir_with_parents(path, 0755);
  g_free(dir);
  gtk_mailbox_tree_refresh(app_state.mailbox_tree);
}

/* Recursively remove a directory and its contents */
static void remove_directory_recursive(const char *dir_path) {
  GDir *dir = g_dir_open(dir_path, 0, NULL);
  if (dir) {
    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
      gchar *child = g_build_filename(dir_path, name, NULL);
      if (g_file_test(child, G_FILE_TEST_IS_DIR))
        remove_directory_recursive(child);
      else
        g_unlink(child);
      g_free(child);
    }
    g_dir_close(dir);
  }
  g_rmdir(dir_path);
}

static void on_mb_remove_response(GObject *source, GAsyncResult *res, gpointer ud) {
  gchar *path = (gchar *)ud;
  GError *err = NULL;
  int choice = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source), res, &err);
  if (err) { g_error_free(err); g_free(path); return; }
  if (choice == 0) {  /* Remove */
    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
      remove_directory_recursive(path);
    } else {
      g_unlink(path);
      char toc[1024];
      snprintf(toc, sizeof(toc), "%s.toc", path);
      g_unlink(toc);
    }
    gtk_mailbox_tree_refresh(app_state.mailbox_tree);
  }
  g_free(path);
}

static void on_mb_remove(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  GtkTreeSelection *sel =
      gtk_tree_view_get_selection(GTK_TREE_VIEW(app_state.mailbox_tree));
  GtkTreeIter iter;
  GtkTreeModel *model =
      gtk_tree_view_get_model(GTK_TREE_VIEW(app_state.mailbox_tree));
  if (!gtk_tree_selection_get_selected(sel, &model, &iter))
    return;

  gchar *name = NULL, *path = NULL;
  gtk_tree_model_get(model, &iter, 0, &name, 1, &path, -1);

  /* Don't allow removing standard mailboxes */
  if (name && (g_strcmp0(name, "In") == 0 || g_strcmp0(name, "Out") == 0 ||
               g_strcmp0(name, "Trash") == 0 || g_strcmp0(name, "Junk") == 0 ||
               g_strcmp0(name, "Drafts") == 0)) {
    g_free(name); g_free(path);
    return;
  }

  gchar *msg = g_strdup_printf("Remove mailbox \"%s\"?", name ? name : "");
  GtkAlertDialog *dlg = gtk_alert_dialog_new("%s", msg);
  g_free(msg);

  const char *buttons[] = {"Remove", "Cancel", NULL};
  gtk_alert_dialog_set_buttons(dlg, buttons);
  gtk_alert_dialog_set_cancel_button(dlg, 1);
  gtk_alert_dialog_set_default_button(dlg, 1);

  gtk_alert_dialog_choose(dlg, GTK_WINDOW(app_state.window), NULL,
                          on_mb_remove_response, g_strdup(path));
  g_object_unref(dlg);

  g_free(name);
  g_free(path);
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

/* Toolbar button callbacks */
#define TB_CB(name, action_fn)                                                 \
  static void name(GtkButton *b, gpointer u) {                                \
    (void)b; (void)u; action_fn(NULL, NULL, NULL);                             \
  }

TB_CB(tb_new_message,  action_new_message)
TB_CB(tb_reply,        action_reply)
TB_CB(tb_reply_all,    action_reply_all)
TB_CB(tb_forward,      action_forward)
TB_CB(tb_delete,       action_delete)
TB_CB(tb_check_mail,   action_check_mail)
TB_CB(tb_send_queued,  action_send_queued)
TB_CB(tb_address_book, action_address_book)
TB_CB(tb_filters,      action_filters)
TB_CB(tb_print,        action_print)

/* Mailbox shortcut callbacks — open the named mailbox tab */
static void tb_open_mailbox(const char *name) {
  gchar *path = gtk_mailbox_get_path(name);
  if (path) { open_mailbox_tab(name, path); g_free(path); }
}
static void tb_in(GtkButton *b, gpointer u)    { (void)b;(void)u; tb_open_mailbox("In"); }
static void tb_out(GtkButton *b, gpointer u)   { (void)b;(void)u; tb_open_mailbox("Out"); }
static void tb_trash(GtkButton *b, gpointer u) { (void)b;(void)u; tb_open_mailbox("Trash"); }
static void tb_junk(GtkButton *b, gpointer u)  { (void)b;(void)u; tb_open_mailbox("Junk"); }

/* Redirect — reuse forward for now (TODO: proper redirect) */
static void tb_redirect(GtkButton *b, gpointer u) {
  (void)b;(void)u; action_forward(NULL, NULL, NULL);
}

/* Search — placeholder */
static void tb_search(GtkButton *b, gpointer u) {
  (void)b;(void)u;
  g_print("Search not yet implemented\n");
}

/*
 * Create main toolbar matching original Eudora 6.x layout.
 * Original used small icon-only buttons (no labels) with tooltips.
 *
 *   In | Out | Trash | Junk | [sep] | Delete | [sep] |
 *   New | Reply | Reply All | Forward | Redirect | [sep] |
 *   Check Mail | Send Queued | [sep] |
 *   Nicknames | Filters | Search | [sep] | Print
 */

/* Helper: icon-only button with tooltip */
static GtkWidget *tb_btn(ToolbarIcon icon, const char *tip, GCallback cb) {
  GtkWidget *btn = create_toolbar_button_no_label(icon, ICON_SIZE_MEDIUM);
  gtk_widget_set_tooltip_text(btn, tip);
  g_signal_connect(btn, "clicked", cb, NULL);
  return btn;
}

static void create_toolbars(GtkBox *toolbar_container) {
  app_state.main_toolbar = create_dockable_toolbar("Main Toolbar");

  /* Mailbox shortcuts */
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_TRANSFER_IN,  "In",    G_CALLBACK(tb_in)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_TRANSFER_OUT, "Out",   G_CALLBACK(tb_out)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_EMPTY_TRASH,  "Trash", G_CALLBACK(tb_trash)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_JUNK,         "Junk",  G_CALLBACK(tb_junk)));
  toolbar_add_separator(app_state.main_toolbar);

  /* Delete */
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_DELETE, "Delete", G_CALLBACK(tb_delete)));
  toolbar_add_separator(app_state.main_toolbar);

  /* Message composition */
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_NEW_MESSAGE, "New Message",  G_CALLBACK(tb_new_message)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_REPLY,       "Reply",        G_CALLBACK(tb_reply)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_REPLY_ALL,   "Reply All",    G_CALLBACK(tb_reply_all)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_FORWARD,     "Forward",      G_CALLBACK(tb_forward)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_REDIRECT,    "Redirect",     G_CALLBACK(tb_redirect)));
  toolbar_add_separator(app_state.main_toolbar);

  /* Mail operations */
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_CHECK_MAIL,  "Check Mail",   G_CALLBACK(tb_check_mail)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_SEND_QUEUED, "Send Queued",  G_CALLBACK(tb_send_queued)));
  toolbar_add_separator(app_state.main_toolbar);

  /* Tools */
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_ADDRESS_BOOK, "Nicknames",   G_CALLBACK(tb_address_book)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_FILTERS,      "Filters",     G_CALLBACK(tb_filters)));
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_SEARCH,       "Search",      G_CALLBACK(tb_search)));
  toolbar_add_separator(app_state.main_toolbar);
  toolbar_add_button(app_state.main_toolbar, tb_btn(ICON_PRINT,        "Print",       G_CALLBACK(tb_print)));

  set_toolbar_position(app_state.main_toolbar, TOOLBAR_DOCK_TOP);
  gtk_box_append(GTK_BOX(toolbar_container),
                 get_toolbar_widget(app_state.main_toolbar));
}

/* ─── Wazoo dock system ─────────────────────────────────────────────── */
/*
 * Original Eudora layout:
 *   Left:  Wazoo dock — GtkNotebook with Mailboxes (+ other wazoo tabs).
 *          The entire left notebook can be dragged out to float in its own
 *          window (like kFloatingWindowClass) and re-docked by closing it.
 *   Right: Mailbox notebook — each opened mailbox gets its own tab.
 *          Each tab contains a VPaned with message list on top and
 *          preview on bottom (like original Eudora mailbox windows).
 */

#define WAZOO_GROUP "eudora-wazoo"

static GtkWidget *left_wazoo = NULL;       /* GtkNotebook for wazoo tabs */
static GtkWidget *left_wazoo_box = NULL;   /* VBox: titlebar + notebook */
static GtkWidget *mailbox_notebook = NULL;  /* GtkNotebook for opened mailboxes */
static GtkWidget *main_hpaned = NULL;
static GtkWidget *left_wazoo_float = NULL;  /* Floating window when undocked */
static GtkWidget *wazoo_titlebar = NULL;    /* Title bar widget for drag */

/* ── Left wazoo dock/undock ── */

static gboolean on_wazoo_float_close(GtkWindow *win, gpointer ud) {
  (void)ud;
  if (!left_wazoo_box || !main_hpaned)
    return FALSE;
  g_object_ref(left_wazoo_box);
  gtk_window_set_child(GTK_WINDOW(win), NULL);
  gtk_paned_set_start_child(GTK_PANED(main_hpaned), left_wazoo_box);
  g_object_unref(left_wazoo_box);
  /* Restore paned position so wazoo is visible */
  gtk_paned_set_position(GTK_PANED(main_hpaned), 220);
  left_wazoo_float = NULL;
  return TRUE;
}

static void wazoo_undock(void) {
  if (!left_wazoo_box || !main_hpaned || left_wazoo_float)
    return;
  g_object_ref(left_wazoo_box);
  /* Remove from paned — collapse paned to 0 so right side fills */
  gtk_paned_set_start_child(GTK_PANED(main_hpaned),
                            gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
  gtk_paned_set_position(GTK_PANED(main_hpaned), 0);

  left_wazoo_float = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(left_wazoo_float), "Wazoo");
  gtk_window_set_default_size(GTK_WINDOW(left_wazoo_float), 250, 500);
  gtk_window_set_child(GTK_WINDOW(left_wazoo_float), left_wazoo_box);
  g_object_unref(left_wazoo_box);
  g_signal_connect(left_wazoo_float, "close-request",
                   G_CALLBACK(on_wazoo_float_close), NULL);
  gtk_window_present(GTK_WINDOW(left_wazoo_float));
}

static void wazoo_redock(void) {
  if (!left_wazoo_float)
    return;
  on_wazoo_float_close(GTK_WINDOW(left_wazoo_float), NULL);
  gtk_window_destroy(GTK_WINDOW(left_wazoo_float));
  left_wazoo_float = NULL;
}

static void wazoo_dock_toggle(void) {
  if (left_wazoo_float)
    wazoo_redock();
  else
    wazoo_undock();
}

/* Undock button clicked */
static void on_wazoo_undock_clicked(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  wazoo_dock_toggle();
}

/* Click on wazoo title label to toggle dock */
static void on_wazoo_title_clicked(GtkGestureClick *gesture, int n_press,
                                    double x, double y, gpointer ud) {
  (void)n_press; (void)x; (void)y; (void)ud;
  wazoo_dock_toggle();
  gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

/* When a tab is dragged out of a notebook, GTK calls this to get the
 * target notebook for the new floating window. */
static GtkNotebook *on_create_window(GtkNotebook *notebook, GtkWidget *page,
                                      gpointer user_data) {
  (void)user_data;
  const char *title = "Eudora";
  GtkWidget *tab_label = gtk_notebook_get_tab_label(notebook, page);
  if (tab_label) {
    /* Tab label might be a box with a label child */
    if (GTK_IS_LABEL(tab_label))
      title = gtk_label_get_text(GTK_LABEL(tab_label));
    else if (GTK_IS_BOX(tab_label)) {
      GtkWidget *child = gtk_widget_get_first_child(tab_label);
      if (child && GTK_IS_LABEL(child))
        title = gtk_label_get_text(GTK_LABEL(child));
    }
  }

  GtkWidget *win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(win), title);
  gtk_window_set_default_size(GTK_WINDOW(win), 400, 500);

  GtkWidget *nb = gtk_notebook_new();
  gtk_notebook_set_group_name(GTK_NOTEBOOK(nb), WAZOO_GROUP);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_BOTTOM);
  g_signal_connect(nb, "create-window",
                   G_CALLBACK(on_create_window), NULL);
  gtk_window_set_child(GTK_WINDOW(win), nb);
  gtk_window_present(GTK_WINDOW(win));

  return GTK_NOTEBOOK(nb);
}

/* ── Mailbox tab management ── */

/* Create a mailbox tab: VPaned with message list on top, preview on bottom.
 * Like original Eudora mailbox window. */
static GtkWidget *create_mailbox_tab_content(TOCType *toc) {
  GtkWidget *vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_position(GTK_PANED(vpaned), 250);

  /* Message list (top) — same columns as create_message_list */
  GListStore *store = g_list_store_new(GTK_TYPE_MESSAGELIST_ITEM);

  if (toc) {
    for (int i = 0; i < toc->count; i++) {
      GtkMessageListItem *item = gtk_messagelist_item_new(&toc->sums[i], i);
      g_list_store_append(store, item);
      g_object_unref(item);
    }
  }

  GtkSingleSelection *sel = gtk_single_selection_new(G_LIST_MODEL(store));
  GtkWidget *col_view = gtk_column_view_new(GTK_SELECTION_MODEL(sel));
  gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(col_view), TRUE);
  gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(col_view), TRUE);

  /* From column */
  GtkListItemFactory *f = gtk_signal_list_item_factory_new();
  g_signal_connect(f, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(f, "bind", G_CALLBACK(bind_from_cb), NULL);
  GtkColumnViewColumn *c = gtk_column_view_column_new("From", f);
  gtk_column_view_column_set_resizable(c, TRUE);
  gtk_column_view_column_set_fixed_width(c, 150);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(col_view), c);
  g_object_unref(c);

  /* Subject column */
  f = gtk_signal_list_item_factory_new();
  g_signal_connect(f, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(f, "bind", G_CALLBACK(bind_subject_cb), NULL);
  c = gtk_column_view_column_new("Subject", f);
  gtk_column_view_column_set_resizable(c, TRUE);
  gtk_column_view_column_set_expand(c, TRUE);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(col_view), c);
  g_object_unref(c);

  /* Date column */
  f = gtk_signal_list_item_factory_new();
  g_signal_connect(f, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(f, "bind", G_CALLBACK(bind_date_cb), NULL);
  c = gtk_column_view_column_new("Date", f);
  gtk_column_view_column_set_resizable(c, TRUE);
  gtk_column_view_column_set_fixed_width(c, 120);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(col_view), c);
  g_object_unref(c);

  /* Size column */
  f = gtk_signal_list_item_factory_new();
  g_signal_connect(f, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(f, "bind", G_CALLBACK(bind_size_cb), NULL);
  c = gtk_column_view_column_new("Size", f);
  gtk_column_view_column_set_resizable(c, TRUE);
  gtk_column_view_column_set_fixed_width(c, 80);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(col_view), c);
  g_object_unref(c);

  /* Double-click / Enter opens message in its own window */
  g_signal_connect(col_view, "activate", G_CALLBACK(on_message_activated), NULL);

  GtkWidget *list_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), col_view);
  gtk_paned_set_start_child(GTK_PANED(vpaned), list_scroll);
  gtk_paned_set_resize_start_child(GTK_PANED(vpaned), TRUE);

  /* Preview (bottom) */
  GtkTextBuffer *buf = gtk_text_buffer_new(NULL);
  gtk_text_buffer_set_text(buf, "Select a message to preview.", -1);
  GtkWidget *preview = gtk_text_view_new_with_buffer(buf);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(preview), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(preview), GTK_WRAP_WORD_CHAR);

  GtkWidget *preview_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(preview_scroll), preview);
  gtk_paned_set_end_child(GTK_PANED(vpaned), preview_scroll);
  gtk_paned_set_resize_end_child(GTK_PANED(vpaned), TRUE);

  /* Store refs on the vpaned for later use */
  g_object_set_data(G_OBJECT(vpaned), "toc", toc);
  g_object_set_data(G_OBJECT(vpaned), "store", store);
  g_object_set_data(G_OBJECT(vpaned), "selection", sel);
  g_object_set_data(G_OBJECT(vpaned), "preview-buffer", buf);

  return vpaned;
}

/* Close button callback for mailbox tabs */
static void on_mailbox_tab_close(GtkButton *btn, gpointer ud) {
  (void)ud;
  GtkWidget *page = g_object_get_data(G_OBJECT(btn), "page-widget");
  if (page && mailbox_notebook) {
    int idx = gtk_notebook_page_num(GTK_NOTEBOOK(mailbox_notebook), page);
    if (idx >= 0)
      gtk_notebook_remove_page(GTK_NOTEBOOK(mailbox_notebook), idx);
  }
}

/* Open a mailbox as a new tab in the mailbox notebook, or switch to
 * existing tab if already open.  Like original Eudora mailbox windows. */
static void open_mailbox_tab(const char *name, const char *path) {
  if (!mailbox_notebook)
    return;

  /* Check if already open */
  int n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(mailbox_notebook));
  for (int i = 0; i < n; i++) {
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(mailbox_notebook), i);
    const char *tab_path = g_object_get_data(G_OBJECT(page), "mailbox-path");
    if (tab_path && strcmp(tab_path, path) == 0) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(mailbox_notebook), i);
      return;
    }
  }

  /* Load TOC data only — don't create a separate mailbox window */
  TOCType *toc = toc_load(path);

  /* Create tab content */
  GtkWidget *content = create_mailbox_tab_content(toc);
  g_object_set_data_full(G_OBJECT(content), "mailbox-path",
                         g_strdup(path), g_free);

  /* Tab label with close button */
  GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *label = gtk_label_new(name);
  GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close-symbolic");
  gtk_button_set_has_frame(GTK_BUTTON(close_btn), FALSE);
  gtk_box_append(GTK_BOX(tab_box), label);
  gtk_box_append(GTK_BOX(tab_box), close_btn);

  int page_idx = gtk_notebook_append_page(GTK_NOTEBOOK(mailbox_notebook),
                                           content, tab_box);
  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(mailbox_notebook), content, TRUE);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(mailbox_notebook), content, TRUE);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(mailbox_notebook), page_idx);

  /* Close button removes the tab */
  g_object_set_data(G_OBJECT(close_btn), "page-widget", content);
  g_signal_connect(close_btn, "clicked",
                   G_CALLBACK(on_mailbox_tab_close), NULL);

  /* Update app_state for backward compat */
  app_state.current_toc = toc;
  g_free(app_state.current_mailbox_path);
  app_state.current_mailbox_path = g_strdup(path);
}

/* Create main layout */
static GtkWidget *create_main_layout(void) {
  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* Toolbar container */
  GtkWidget *toolbar_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(toolbar_container, "toolbar-area");
  create_toolbars(GTK_BOX(toolbar_container));
  gtk_box_append(GTK_BOX(main_box), toolbar_container);

  /* Main content: HPaned */
  main_hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_position(GTK_PANED(main_hpaned), 220);

  /* ── Left: Wazoo dock ── */
  /* Original Eudora: tabs at top, drag a tab to tear it off into a
   * floating window.  We wrap the notebook in a VBox with a title bar
   * that has a drag handle / undock button. */

  left_wazoo_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* Title bar: draggable handle + label + undock button.
   * Drag the title bar to undock, like original Eudora wazoo. */
  wazoo_titlebar = gtk_center_box_new();
  gtk_widget_add_css_class(wazoo_titlebar, "wazoo-titlebar");
  gtk_widget_set_margin_start(wazoo_titlebar, 2);
  gtk_widget_set_margin_end(wazoo_titlebar, 2);
  gtk_widget_set_margin_top(wazoo_titlebar, 1);
  gtk_widget_set_margin_bottom(wazoo_titlebar, 1);

  /* Drag handle icon on the left */
  GtkWidget *drag_handle = gtk_image_new_from_icon_name("open-menu-symbolic");
  gtk_widget_set_opacity(drag_handle, 0.5);
  gtk_widget_set_tooltip_text(drag_handle, "Drag to undock");
  gtk_center_box_set_start_widget(GTK_CENTER_BOX(wazoo_titlebar), drag_handle);

  GtkWidget *wazoo_title_label = gtk_label_new("Wazoo");
  PangoAttrList *attrs = pango_attr_list_new();
  pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
  pango_attr_list_insert(attrs, pango_attr_scale_new(0.85));
  gtk_label_set_attributes(GTK_LABEL(wazoo_title_label), attrs);
  pango_attr_list_unref(attrs);
  gtk_center_box_set_center_widget(GTK_CENTER_BOX(wazoo_titlebar), wazoo_title_label);

  GtkWidget *undock_btn = gtk_button_new_from_icon_name("view-restore-symbolic");
  gtk_button_set_has_frame(GTK_BUTTON(undock_btn), FALSE);
  gtk_widget_set_tooltip_text(undock_btn, "Float / Dock");
  g_signal_connect(undock_btn, "clicked",
                   G_CALLBACK(on_wazoo_undock_clicked), NULL);
  gtk_center_box_set_end_widget(GTK_CENTER_BOX(wazoo_titlebar), undock_btn);

  /* Click the title label to toggle dock */
  GtkGesture *title_click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(title_click), 1);
  g_signal_connect(title_click, "pressed",
                   G_CALLBACK(on_wazoo_title_clicked), NULL);
  gtk_widget_add_controller(wazoo_title_label, GTK_EVENT_CONTROLLER(title_click));
  gtk_widget_set_cursor_from_name(wazoo_titlebar, "pointer");

  gtk_box_append(GTK_BOX(left_wazoo_box), wazoo_titlebar);
  gtk_box_append(GTK_BOX(left_wazoo_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  /* Notebook with tabs at top — like original Eudora wazoo tabs */
  left_wazoo = gtk_notebook_new();
  gtk_notebook_set_group_name(GTK_NOTEBOOK(left_wazoo), WAZOO_GROUP);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(left_wazoo), GTK_POS_BOTTOM);
  g_signal_connect(left_wazoo, "create-window",
                   G_CALLBACK(on_create_window), NULL);

  /* Mailbox tree tab: tree + button bar in a VBox */
  GtkWidget *mb_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  app_state.mailbox_tree = create_mailbox_tree();
  GtkWidget *mb_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mb_scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(mb_scroll), FALSE);
  gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(mb_scroll), FALSE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(mb_scroll),
                                app_state.mailbox_tree);
  gtk_widget_set_vexpand(mb_scroll, TRUE);
  gtk_widget_set_hexpand(mb_scroll, TRUE);
  gtk_box_append(GTK_BOX(mb_vbox), mb_scroll);

  /* Button bar: New Mailbox | New Folder | Remove */
  GtkWidget *mb_btn_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_margin_start(mb_btn_bar, 2);
  gtk_widget_set_margin_end(mb_btn_bar, 2);
  gtk_widget_set_margin_top(mb_btn_bar, 2);
  gtk_widget_set_margin_bottom(mb_btn_bar, 2);
  gtk_widget_set_vexpand(mb_btn_bar, FALSE);

  GtkWidget *btn_new_mb = gtk_button_new_with_label("New");
  gtk_widget_set_tooltip_text(btn_new_mb, "Create a new mailbox");
  g_signal_connect(btn_new_mb, "clicked", G_CALLBACK(on_mb_new_mailbox), NULL);
  gtk_box_append(GTK_BOX(mb_btn_bar), btn_new_mb);

  GtkWidget *btn_new_fld = gtk_button_new_with_label("Folder");
  gtk_widget_set_tooltip_text(btn_new_fld, "Create a new folder");
  g_signal_connect(btn_new_fld, "clicked", G_CALLBACK(on_mb_new_folder), NULL);
  gtk_box_append(GTK_BOX(mb_btn_bar), btn_new_fld);

  GtkWidget *btn_remove = gtk_button_new_with_label("Del");
  gtk_widget_set_tooltip_text(btn_remove, "Remove selected mailbox");
  g_signal_connect(btn_remove, "clicked", G_CALLBACK(on_mb_remove), NULL);
  gtk_box_append(GTK_BOX(mb_btn_bar), btn_remove);

  gtk_box_append(GTK_BOX(mb_vbox), mb_btn_bar);

  gtk_notebook_append_page(GTK_NOTEBOOK(left_wazoo), mb_vbox,
                           gtk_label_new("Mailboxes"));
  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(left_wazoo), mb_vbox, TRUE);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(left_wazoo), mb_vbox, TRUE);

  gtk_widget_set_vexpand(left_wazoo, TRUE);
  gtk_box_append(GTK_BOX(left_wazoo_box), left_wazoo);

  /* Double-click / Enter opens a mailbox tab on the right */
  g_signal_connect(app_state.mailbox_tree, "row-activated",
                   G_CALLBACK(on_mailbox_activated), NULL);

  gtk_paned_set_start_child(GTK_PANED(main_hpaned), left_wazoo_box);
  gtk_paned_set_resize_start_child(GTK_PANED(main_hpaned), FALSE);

  /* ── Right: Mailbox notebook (one tab per opened mailbox) ── */
  mailbox_notebook = gtk_notebook_new();
  gtk_notebook_set_group_name(GTK_NOTEBOOK(mailbox_notebook), WAZOO_GROUP);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(mailbox_notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(mailbox_notebook), TRUE);
  g_signal_connect(mailbox_notebook, "create-window",
                   G_CALLBACK(on_create_window), NULL);

  /* Start with a welcome page */
  GtkWidget *welcome = gtk_label_new("Double-click a mailbox to open it.");
  gtk_notebook_append_page(GTK_NOTEBOOK(mailbox_notebook), welcome,
                           gtk_label_new("Welcome"));

  gtk_paned_set_end_child(GTK_PANED(main_hpaned), mailbox_notebook);
  gtk_paned_set_resize_end_child(GTK_PANED(main_hpaned), TRUE);

  gtk_box_append(GTK_BOX(main_box), main_hpaned);
  gtk_widget_set_vexpand(main_hpaned, TRUE);
  gtk_widget_set_hexpand(main_hpaned, TRUE);

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

  /* First-run detection: if no mail server configured, open Settings
     to "Getting Started" page so user can set up their account.
     This matches original Mac Eudora behavior (NAG_INTRO_DLOG). */
  gboolean first_run = (app_state.settings->pop_server[0] == '\0' &&
                         app_state.settings->smtp_server[0] == '\0');

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

  action = g_simple_action_new("check-mail", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_check_mail), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("send-queued", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_send_queued), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("address-book", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_address_book), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("filters", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_filters), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("personalities", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_personalities), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("signatures", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_signatures), NULL);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(action));
  g_object_unref(action);

  action = g_simple_action_new("statistics", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(action_statistics), NULL);
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

  /* Initialize Eudora wazoo (tabbed window) system */
  SetupDefaultWazoos();

  /* Create main layout */
  app_state.main_box = create_main_layout();
  gtk_window_set_child(GTK_WINDOW(app_state.window), app_state.main_box);

  /* Create system menu bar */
  create_menu_bar(app_state.window);

  gtk_window_present(GTK_WINDOW(app_state.window));

  /* First run: open Settings to "Getting Started" so user configures account */
  if (first_run) {
    SettingsDialog *sd =
        create_settings_dialog(GTK_WINDOW(app_state.window), app_state.settings);
    show_settings_section(sd, SETTINGS_GETTING_STARTED);
    GtkWidget *dialog = get_settings_dialog_widget(sd);
    gtk_window_present(GTK_WINDOW(dialog));
  }

  /* Mailboxes are now integrated in the sidebar — no separate mailbox
     window needed on startup. */
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
