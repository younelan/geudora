/*
 * gEudora - GTK4 Mail Client
 * Main application entry point and UI layout
 */

#include "../gEditCtrl/editor_control.h"
#include "../gEditCtrl/gedit-document.h"
#include "../gEditCtrl/geditctrl.h"
#include "comp.h"
#include "gtk_icons.h"
#include "gtk_mailbox.h"
#include "gtk_menus.h"
#include "gtk_messagelist.h"
#include "gtk_prefs.h"
#include "gtk_settings.h"
#include "gtk_toolbar_dock.h"
#include "mailbox.h" /* For MessageSummary */
#include "mailxfer.h"
#include "message.h"
#include "taskProgress.h"
#include "theme.h"
#include "threading.h"
#include "schizo.h"
#include "toc.h"
#include "wazoo.h"
#include "StringUtil.h"
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
static GtkWidget *open_panel_tab_icon(const char *panel_id, const char *title,
                                       const char *icon_name,
                                       GtkWidget *(*builder)(void));

/*
 * read_message_raw - Read entire message from mailbox file (headers + body).
 * Returns allocated string (caller must g_free), or NULL on error.
 */
static gchar *read_message_raw(TOCType *toc, int msg_index) {
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
  if (len <= 0 || len > 10 * 1024 * 1024) {
    fclose(fp);
    return NULL;
  }

  gchar *buf = g_malloc(len + 1);
  size_t nread = fread(buf, 1, len, fp);
  fclose(fp);
  buf[nread] = '\0';

  /* Ensure valid UTF-8 — auto-convert from Windows-1252 if needed */
  if (!g_utf8_validate(buf, nread, NULL)) {
    gchar *utf8 = ensure_utf8(buf);
    g_free(buf);
    return utf8;
  }
  return buf;
}

/* Headers we display (case-insensitive match). Order matters for display. */
static const char *const kDisplayHeaders[] = {
  "From", "To", "Subject", "Date", "Cc", "Reply-To", NULL
};

/*
 * Extract a single header value from raw message text.
 * Handles continuation lines (lines starting with space/tab).
 * Returns allocated string or NULL. Caller must g_free.
 */
static gchar *extract_header(const char *raw, const char *name) {
  size_t nlen = strlen(name);
  const char *p = raw;

  while (p && *p) {
    /* Check for end of headers (blank line)
       Tolerant of \n\n, \r\n\r\n, \r\r, and \n\r\n */
    if (*p == '\n') break;
    if (*p == '\r') {
        if (p[1] == '\n') break; /* \r\n */
        if (p[1] == '\r' || (p[1] != ' ' && p[1] != '\t')) {
            /* \r\r or \r followed by next header = end of this header line.
               But a lone \r at start of "line" usually means blank line in legacy Mac */
            break;
        }
    }

    /* Match "Name:" at start of line (case-insensitive) */
    if (g_ascii_strncasecmp(p, name, nlen) == 0 && p[nlen] == ':') {
      const char *val = p + nlen + 1;
      while (*val == ' ' || *val == '\t') val++;

      /* Collect value including continuation lines */
      GString *s = g_string_new(NULL);
      while (*val && *val != '\r' && *val != '\n') {
        g_string_append_c(s, *val);
        val++;
      }

      /* Handle continuation lines (next line starts with space or tab) */
      while (TRUE) {
        const char *next = val;
        /* Skip over any flavor of newline */
        if (*next == '\r' && next[1] == '\n') next += 2;
        else if (*next == '\r' || *next == '\n') next++;
        else break;

        /* If next line starts with space/tab, it's a continuation */
        if (*next == ' ' || *next == '\t') {
          val = next;
          g_string_append_c(s, ' ');
          while (*val == ' ' || *val == '\t') val++;
          while (*val && *val != '\r' && *val != '\n') {
            g_string_append_c(s, *val);
            val++;
          }
        } else {
          break;
        }
      }

      gchar *val_raw = g_string_free(s, FALSE);
      gchar *val_utf8 = ensure_utf8(val_raw);
      g_free(val_raw);
      return val_utf8;
    }

    /* Skip to next line - handle \r\n, \n, or \r */
    const char *next_n = strchr(p, '\n');
    const char *next_r = strchr(p, '\r');
    if (next_n && (!next_r || next_n < next_r)) p = next_n + 1;
    else if (next_r) p = next_r + (next_r[1] == '\n' ? 2 : 1);
    else break;
  }
  return NULL;
}

/*
 * Find the body start (after blank line separator).
 * Returns pointer into the raw string, or raw itself if no separator found.
 */
static const char *find_body(const char *raw) {
  const char *p;
  /* Tolerant search for double newline in any combination */
  p = strstr(raw, "\r\n\r\n"); if (p) return p + 4;
  p = strstr(raw, "\n\n");     if (p) return p + 2;
  p = strstr(raw, "\r\r");     if (p) return p + 2;
  p = strstr(raw, "\n\r\n");   if (p) return p + 3;
  p = strstr(raw, "\r\n\n");   if (p) return p + 3;
  return raw;
}

/* Message CSS is now provided by theme.c */
static void ensure_message_css(void) { /* handled by theme engine */ }

/*
 * Create styled header row: "Name:  value"
 */
static GtkWidget *make_header_row(const char *name, const char *value,
                                   const char *extra_class) {
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  if (extra_class)
    gtk_widget_add_css_class(row, extra_class);

  GtkWidget *lbl_name = gtk_label_new(name);
  gtk_widget_add_css_class(lbl_name, "msg-header-name");
  gtk_label_set_xalign(GTK_LABEL(lbl_name), 1.0);
  gtk_widget_set_valign(lbl_name, GTK_ALIGN_START);

  GtkWidget *lbl_colon = gtk_label_new(":");
  gtk_widget_add_css_class(lbl_colon, "msg-header-name");

  GtkWidget *lbl_val = gtk_label_new(value ? value : "");
  gtk_widget_add_css_class(lbl_val, "msg-header-value");
  gtk_label_set_xalign(GTK_LABEL(lbl_val), 0.0);
  gtk_label_set_wrap(GTK_LABEL(lbl_val), TRUE);
  gtk_label_set_selectable(GTK_LABEL(lbl_val), TRUE);
  gtk_widget_set_hexpand(lbl_val, TRUE);

  gtk_box_append(GTK_BOX(row), lbl_name);
  gtk_box_append(GTK_BOX(row), lbl_colon);
  gtk_box_append(GTK_BOX(row), lbl_val);
  return row;
}

/*
 * Build the header widget box from raw message text.
 * Shows From, To, Subject, Date, Cc, Reply-To (if present).
 */
static GtkWidget *build_header_box(const char *raw) {
  ensure_message_css();

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_add_css_class(box, "msg-header-box");

  gboolean found_any = FALSE;
  for (int i = 0; kDisplayHeaders[i]; i++) {
    gchar *val = extract_header(raw, kDisplayHeaders[i]);
    if (!val) continue;

    found_any = TRUE;
    const char *cls = NULL;
    if (g_ascii_strcasecmp(kDisplayHeaders[i], "Subject") == 0)
      cls = "msg-header-subject";

    gtk_box_append(GTK_BOX(box), make_header_row(kDisplayHeaders[i], val, cls));
    g_free(val);
  }

  if (!found_any) {
    g_object_ref_sink(box);
    g_object_unref(box);
    return NULL;
  }

  /* Separator line */
  GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_add_css_class(sep, "msg-separator");
  gtk_box_append(GTK_BOX(box), sep);

  return box;
}

/*
 * Apply quote-level styling to a GtkTextView displaying a message body.
 * Lines starting with > get colored by depth.
 */
static void style_quotes(GtkTextView *view) {
  GtkTextBuffer *buf = gtk_text_view_get_buffer(view);
  GtkTextIter start, end, line_start, line_end;
  gtk_text_buffer_get_bounds(buf, &start, &end);

  /* Create tags for quote levels */
  static const char *tag_names[] = {"q1", "q2", "q3"};
  static const char *tag_colors[] = {"#2962FF", "#00796B", "#6A1B9A"};
  for (int i = 0; i < 3; i++) {
    if (!gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buf), tag_names[i]))
      gtk_text_buffer_create_tag(buf, tag_names[i], "foreground", tag_colors[i], NULL);
  }

  /* Scan each line */
  line_start = start;
  while (gtk_text_iter_compare(&line_start, &end) < 0) {
    line_end = line_start;
    gtk_text_iter_forward_to_line_end(&line_end);

    gchar *line = gtk_text_buffer_get_text(buf, &line_start, &line_end, FALSE);
    if (line) {
      int depth = 0;
      const char *p = line;
      while (*p == '>' || *p == ' ') {
        if (*p == '>') depth++;
        p++;
      }
      if (depth > 0) {
        int tag_idx = (depth > 3) ? 2 : depth - 1;
        gtk_text_buffer_apply_tag_by_name(buf, tag_names[tag_idx],
                                           &line_start, &line_end);
      }
      g_free(line);
    }

    if (!gtk_text_iter_forward_line(&line_start))
      break;
  }
}

/*
 * Create a complete message view widget (header box + body text view).
 * Used by both the preview pane and the message window.
 * If max_body_len > 0, truncate body to that many bytes (for preview).
 */
static GtkWidget *create_message_view(const char *raw, long max_body_len) {
  ensure_message_css();

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* Header area */
  GtkWidget *hdr = build_header_box(raw);
  if (hdr)
      gtk_box_append(GTK_BOX(vbox), hdr);

  /* Body text view */
  const char *body = find_body(raw);
  GtkTextBuffer *buf = gtk_text_buffer_new(NULL);
  if (max_body_len > 0 && (long)strlen(body) > max_body_len) {
    gchar *trunc = g_strndup(body, max_body_len);
    gchar *utf8 = ensure_utf8(trunc);
    gtk_text_buffer_set_text(buf, utf8, -1);
    g_free(trunc);
    g_free(utf8);
  } else {
    gchar *utf8 = ensure_utf8(body);
    gtk_text_buffer_set_text(buf, utf8, -1);
    g_free(utf8);
  }


  GtkWidget *view = gtk_text_view_new_with_buffer(buf);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 12);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 12);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 4);
  gtk_widget_add_css_class(view, "msg-body-view");
  g_object_unref(buf);

  /* Style quoted lines */
  style_quotes(GTK_TEXT_VIEW(view));

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), view);
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_box_append(GTK_BOX(vbox), scroll);

  return vbox;
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

/* Bind callbacks for each column — one per column type */
#define BIND_CB(name, getter)                                                  \
  static void name(GtkSignalListItemFactory *self, GtkListItem *list_item,     \
                   gpointer user_data) {                                       \
    (void)self; (void)user_data;                                               \
    GtkMessageListItem *msg =                                                  \
        GTK_MESSAGELIST_ITEM(gtk_list_item_get_item(list_item));               \
    GtkWidget *label = gtk_list_item_get_child(list_item);                     \
    gtk_label_set_text(GTK_LABEL(label), getter(msg));                         \
  }

BIND_CB(bind_status_cb,   gtk_messagelist_item_get_status)
BIND_CB(bind_priority_cb, gtk_messagelist_item_get_priority)
BIND_CB(bind_attach_cb,   gtk_messagelist_item_get_attach)
BIND_CB(bind_label_cb,    gtk_messagelist_item_get_label)
BIND_CB(bind_from_cb,     gtk_messagelist_item_get_from)
BIND_CB(bind_subject_cb,  gtk_messagelist_item_get_subject)
BIND_CB(bind_date_cb,     gtk_messagelist_item_get_date)
BIND_CB(bind_size_cb,     gtk_messagelist_item_get_size)

/* Double-click / Enter on mailbox list → open a tab on the right */
static void on_mailbox_activated(GtkListBox *box, GtkListBoxRow *row,
                                  gpointer ud) {
  (void)box; (void)ud;
  if (!row) return;

  const char *name = g_object_get_data(G_OBJECT(row), "mb-name");
  const char *mb_path = g_object_get_data(G_OBJECT(row), "mb-path");
  gboolean is_dir = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "mb-is-dir"));

  if (is_dir) return;  /* folders are not openable */

  if (mb_path && *mb_path)
    open_mailbox_tab(name, mb_path);
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

/*
 * Open a comp.c compose window for an existing outgoing/draft message.
 * Uses OpenComp which parses headers and body from the stored message.
 */
static void open_comp_window_for_message(TOCType *toc, int sumNum) {
  bool isSent = (toc->sums[sumNum].state == SENT ||
                 toc->sums[sumNum].state == BUSY_SENDING);
  MyWindowPtr win = OpenComp(toc, sumNum, NULL, NULL, true, false);
  if (win && win->window)
    gtk_window_present(GTK_WINDOW(win->window));
  (void)isSent;
}

/* Double-click / Enter on message list → open message in its own window.
 * For outgoing/draft messages, opens the compose window instead. */
static void on_message_activated(GtkColumnView *col_view, guint position,
                                 gpointer user_data) {
  (void)user_data;

  GtkWidget *vpaned = gtk_widget_get_ancestor(GTK_WIDGET(col_view),
                                               GTK_TYPE_PANED);
  if (!vpaned)
    return;

  TOCType *toc = g_object_get_data(G_OBJECT(vpaned), "toc");
  if (!toc || (int)position >= toc->count)
    return;

  MessageSummary *sum = &toc->sums[position];

  /* Outgoing/draft messages open in the comp.c compose window */
  if (toc->which == MBX_OUT || toc->which == MBX_OUT_TEMP ||
      sum->state == UNSENDABLE || sum->state == SENDABLE ||
      sum->state == QUEUED || sum->state == UNSENT ||
      sum->state == TIMED) {
    open_comp_window_for_message(toc, (int)position);
    return;
  }

  /* Incoming messages open read-only in a message viewer */
  gchar *raw = read_message_raw(toc, (int)position);
  if (!raw) return;

  GtkWidget *win = gtk_window_new();
  gchar *title = g_strdup_printf("%s — %s", sum->from, sum->subj);
  gtk_window_set_title(GTK_WINDOW(win), title);
  g_free(title);
  gtk_window_set_default_size(GTK_WINDOW(win), 700, 550);

  GtkWidget *msg_view = create_message_view(raw, 0);
  gtk_window_set_child(GTK_WINDOW(win), msg_view);
  g_free(raw);

  GtkWidget *toplevel = gtk_widget_get_ancestor(GTK_WIDGET(col_view),
                                                 GTK_TYPE_WINDOW);
  if (toplevel)
    gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(toplevel));

  gtk_window_present(GTK_WINDOW(win));
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
  (void)action;
  (void)parameter;
  (void)user_data;

  extern MyWindowPtr DoComposeNew(int type);
  MyWindowPtr win = DoComposeNew(0);
  if (win && win->window)
    gtk_window_present(GTK_WINDOW(win->window));
}

/* action_old_compose removed — action_new_message now uses comp.c directly */

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

/* Helper: set a GtkEntry field on a comp window by g_object_set_data key */
void comp_set_field(MyWindowPtr win, const char *key, const char *value) {
  if (!win || !win->window || !value) return;
  GtkWidget *entry = g_object_get_data(G_OBJECT(win->window), key);
  if (entry)
    gtk_editable_set_text(GTK_EDITABLE(entry), value);
}

/* Helper: set the body text on a comp window's gEditCtrl */
void comp_set_body(MyWindowPtr win, const char *text) {
  if (!win || !win->pte || !text) return;
  geditDocument *doc = geditctrl_get_document(win->pte);
  if (doc)
    gedit_document_insert_text(doc, 0, text);
}

/* Insert text as quoted (with quote bars, not "> " prefix) */
void comp_set_body_quoted(MyWindowPtr win, const char *attribution,
                          const char *body) {
  if (!win || !win->pte) return;
  geditDocument *doc = geditctrl_get_document(win->pte);
  if (!doc) return;

  /* Insert attribution line first (unquoted) */
  int attr_len = 0;
  if (attribution && *attribution) {
    gedit_document_insert_text(doc, 0, attribution);
    attr_len = (int)strlen(attribution);
  }

  /* Insert body text after attribution */
  if (body && *body) {
    int body_len = (int)strlen(body);
    gedit_document_insert_text(doc, attr_len, body);
    /* Set quote level 1 on the body portion */
    gedit_document_set_quote_level(doc, attr_len, body_len, 1);
  }
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
  gchar *body = ({ gchar *_raw = read_message_raw(toc, idx); gchar *_b = _raw ? g_strdup(find_body(_raw)) : NULL; g_free(_raw); _b; });

  extern MyWindowPtr DoComposeNew(int type);
  MyWindowPtr win = DoComposeNew(0);
  if (!win || !win->window) goto cleanup;

  if (reply_addr)
    comp_set_field(win, "comp-to", reply_addr);

  /* Build Re: subject */
  const char *subj = sum->subj;
  gchar *re_subj;
  if (subj && (g_ascii_strncasecmp(subj, "Re:", 3) == 0 ||
               g_ascii_strncasecmp(subj, "Re: ", 4) == 0))
    re_subj = g_strdup(subj);
  else
    re_subj = g_strdup_printf("Re: %s", subj ? subj : "");
  comp_set_field(win, "comp-subject", re_subj);

  /* Insert body with quote bars */
  {
    gchar *attribution = g_strdup_printf("On %s wrote:\n", sum->from);
    comp_set_body_quoted(win, attribution, body);
    g_free(attribution);
  }

  gtk_window_present(GTK_WINDOW(win->window));

cleanup:
  g_free(reply_addr);
  g_free(body);
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
  gchar *body = ({ gchar *_raw = read_message_raw(toc, idx); gchar *_b = _raw ? g_strdup(find_body(_raw)) : NULL; g_free(_raw); _b; });

  extern MyWindowPtr DoComposeNew(int type);
  MyWindowPtr win = DoComposeNew(0);
  if (!win || !win->window) goto cleanup;

  if (reply_addr)
    comp_set_field(win, "comp-to", reply_addr);

  /* Combine original To + Cc into Cc field */
  if (orig_to && orig_cc) {
    gchar *combined = g_strdup_printf("%s, %s", orig_to, orig_cc);
    comp_set_field(win, "comp-cc", combined);
    g_free(combined);
  } else if (orig_to) {
    comp_set_field(win, "comp-cc", orig_to);
  } else if (orig_cc) {
    comp_set_field(win, "comp-cc", orig_cc);
  }

  const char *subj = sum->subj;
  gchar *re_subj;
  if (subj && (g_ascii_strncasecmp(subj, "Re:", 3) == 0))
    re_subj = g_strdup(subj);
  else
    re_subj = g_strdup_printf("Re: %s", subj ? subj : "");
  comp_set_field(win, "comp-subject", re_subj);

  /* Insert body with quote bars */
  {
    gchar *attribution = g_strdup_printf("On %s wrote:\n", sum->from);
    comp_set_body_quoted(win, attribution, body);
    g_free(attribution);
  }

  gtk_window_present(GTK_WINDOW(win->window));

cleanup:
  g_free(reply_addr);
  g_free(orig_to);
  g_free(orig_cc);
  g_free(body);
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
  gchar *body = ({ gchar *_raw = read_message_raw(toc, idx); gchar *_b = _raw ? g_strdup(find_body(_raw)) : NULL; g_free(_raw); _b; });

  extern MyWindowPtr DoComposeNew(int type);
  MyWindowPtr win = DoComposeNew(0);
  if (!win || !win->window) { g_free(body); return; }

  /* Fwd: subject */
  const char *subj = sum->subj;
  gchar *fwd_subj = g_strdup_printf("Fwd: %s", subj ? subj : "");
  comp_set_field(win, "comp-subject", fwd_subj);

  /* Forwarded body */
  if (body) {
    gchar *fwd_body = g_strdup_printf(
        "---------- Forwarded message ----------\n"
        "From: %s\nSubject: %s\n\n%s",
        sum->from, subj ? subj : "", body);
    comp_set_body(win, fwd_body);
    g_free(fwd_body);
  }

  gtk_window_present(GTK_WINDOW(win->window));

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
  XferMail(true, false, true, false, true, 0);
}

static void action_send_queued(GSimpleAction *action, GVariant *parameter,
                               gpointer user_data) {
  (void)action;
  (void)parameter;
  (void)user_data;

  g_print("Sending queued messages...\n");
  XferMail(false, true, true, false, true, 0);
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

/* Address Book panel */
extern void OpenABWin(void);
extern GtkWidget *CreateAddressBookPanel(void);

static void action_address_book(GSimpleAction *action, GVariant *parameter,
                                gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;
  open_panel_tab_icon("addressbook", "Address Book", "x-office-address-book-symbolic", CreateAddressBookPanel);
}

static void action_filters(GSimpleAction *action, GVariant *parameter,
                           gpointer user_data) {
  (void)action; (void)parameter; (void)user_data;
  extern GtkWidget *CreateFiltersPanel(void);
  open_panel_tab_icon("filters", "Filters", "edit-find-symbolic", CreateFiltersPanel);
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
  extern GtkWidget *CreateStatisticsPanel(void);
  open_panel_tab_icon("statistics", "Statistics", "utilities-system-monitor-symbolic", CreateStatisticsPanel);
}

/* Theme cycling action */
static GtkWidget *theme_toggle_btn = NULL;

static void update_theme_tooltip(void) {
  if (!theme_toggle_btn) return;
  char tip[64];
  snprintf(tip, sizeof(tip), "Theme: %s (click to change)",
           theme_get_name(theme_get_current()));
  gtk_widget_set_tooltip_text(theme_toggle_btn, tip);
}

static void on_theme_toggle(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  theme_cycle();
  update_theme_tooltip();
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
  GtkListBoxRow *row =
      gtk_list_box_get_selected_row(GTK_LIST_BOX(app_state.mailbox_tree));
  if (row) {
    const char *sel_path = g_object_get_data(G_OBJECT(row), "mb-path");
    if (sel_path && g_file_test(sel_path, G_FILE_TEST_IS_DIR)) {
      return g_strdup(sel_path);  /* caller must g_free */
    }
    if (sel_path) {
      /* Selected item is a mailbox file — create in its parent dir */
      return g_path_get_dirname(sel_path);  /* caller must g_free */
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

/* ── Rename mailbox via inline GtkEntry overlay ── */

static void on_rename_entry_activate(GtkEntry *entry, gpointer ud) {
  GtkListBoxRow *row = GTK_LIST_BOX_ROW(ud);
  const char *old_path = g_object_get_data(G_OBJECT(row), "mb-path");
  const char *new_name = gtk_editable_get_text(GTK_EDITABLE(entry));

  if (new_name && *new_name && old_path) {
    gchar *parent = g_path_get_dirname(old_path);
    gchar *new_path = g_build_filename(parent, new_name, NULL);

    if (g_rename(old_path, new_path) == 0) {
      /* Also rename .toc file if it exists */
      gchar *old_toc = g_strdup_printf("%s.toc", old_path);
      gchar *new_toc = g_strdup_printf("%s.toc", new_path);
      if (g_file_test(old_toc, G_FILE_TEST_EXISTS))
        g_rename(old_toc, new_toc);
      g_free(old_toc);
      g_free(new_toc);
    }
    g_free(new_path);
    g_free(parent);
  }

  /* Destroy the entry popover and refresh */
  GtkWidget *popover = gtk_widget_get_ancestor(GTK_WIDGET(entry), GTK_TYPE_POPOVER);
  if (popover) gtk_popover_popdown(GTK_POPOVER(popover));
  gtk_mailbox_tree_refresh(app_state.mailbox_tree);
}

static void on_mb_rename(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  GtkListBoxRow *row =
      gtk_list_box_get_selected_row(GTK_LIST_BOX(app_state.mailbox_tree));
  if (!row) return;

  const char *name = g_object_get_data(G_OBJECT(row), "mb-name");

  /* Don't allow renaming standard mailboxes */
  if (name && (g_strcmp0(name, "In") == 0 || g_strcmp0(name, "Out") == 0 ||
               g_strcmp0(name, "Trash") == 0 || g_strcmp0(name, "Junk") == 0 ||
               g_strcmp0(name, "Drafts") == 0))
    return;

  /* Show a popover with an entry for inline rename */
  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, GTK_WIDGET(row));

  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), name ? name : "");
  gtk_entry_set_activates_default(GTK_ENTRY(entry), FALSE);
  g_signal_connect(entry, "activate", G_CALLBACK(on_rename_entry_activate), row);
  gtk_popover_set_child(GTK_POPOVER(popover), entry);
  gtk_popover_popup(GTK_POPOVER(popover));
  gtk_widget_grab_focus(entry);
}

static void on_mb_remove(GtkButton *btn, gpointer ud) {
  (void)btn; (void)ud;
  GtkListBoxRow *row =
      gtk_list_box_get_selected_row(GTK_LIST_BOX(app_state.mailbox_tree));
  if (!row) return;

  const char *name = g_object_get_data(G_OBJECT(row), "mb-name");
  const char *path = g_object_get_data(G_OBJECT(row), "mb-path");

  /* Don't allow removing standard mailboxes */
  if (name && (g_strcmp0(name, "In") == 0 || g_strcmp0(name, "Out") == 0 ||
               g_strcmp0(name, "Trash") == 0 || g_strcmp0(name, "Junk") == 0 ||
               g_strcmp0(name, "Drafts") == 0)) {
    return;
  }

  gboolean is_dir = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "mb-is-dir"));
  gchar *msg = is_dir
      ? g_strdup_printf("Remove folder \"%s\" and all its contents?", name ? name : "")
      : g_strdup_printf("Remove mailbox \"%s\"?", name ? name : "");
  GtkAlertDialog *dlg = gtk_alert_dialog_new("%s", msg);
  g_free(msg);

  const char *buttons[] = {"Remove", "Cancel", NULL};
  gtk_alert_dialog_set_buttons(dlg, buttons);
  gtk_alert_dialog_set_cancel_button(dlg, 1);
  gtk_alert_dialog_set_default_button(dlg, 1);

  gtk_alert_dialog_choose(dlg, GTK_WINDOW(app_state.window), NULL,
                          on_mb_remove_response, g_strdup(path));
  g_object_unref(dlg);
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

  /* Column definitions matching original Eudora mailbox list */
  struct {
    const char *title;
    GCallback bind_cb;
    int width;       /* fixed width, or -1 for expand */
  } col_defs[] = {
    {"",        G_CALLBACK(bind_status_cb),   28},
    {"!",       G_CALLBACK(bind_priority_cb), 28},
    {"\xf0\x9f\x93\x8e", G_CALLBACK(bind_attach_cb), 28}, /* 📎 */
    {"Label",   G_CALLBACK(bind_label_cb),    40},
    {"From",    G_CALLBACK(bind_from_cb),    150},
    {"Date",    G_CALLBACK(bind_date_cb),    130},
    {"Size",    G_CALLBACK(bind_size_cb),     70},
    {"Subject", G_CALLBACK(bind_subject_cb),  -1},
  };
  int ncols = sizeof(col_defs) / sizeof(col_defs[0]);
  for (int c = 0; c < ncols; c++) {
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
    g_signal_connect(factory, "bind", col_defs[c].bind_cb, NULL);
    GtkColumnViewColumn *column =
        gtk_column_view_column_new(col_defs[c].title, factory);
    gtk_column_view_column_set_resizable(column, TRUE);
    if (col_defs[c].width > 0)
      gtk_column_view_column_set_fixed_width(column, col_defs[c].width);
    else
      gtk_column_view_column_set_expand(column, TRUE);
    gtk_column_view_append_column(GTK_COLUMN_VIEW(view), column);
    g_object_unref(column);
  }

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
  toolbar_add_separator(app_state.main_toolbar);

  /* Theme toggle button */
  theme_toggle_btn = gtk_button_new_from_icon_name("preferences-desktop-appearance-symbolic");
  gtk_button_set_has_frame(GTK_BUTTON(theme_toggle_btn), FALSE);
  gtk_widget_add_css_class(theme_toggle_btn, "theme-toggle");
  g_signal_connect(theme_toggle_btn, "clicked", G_CALLBACK(on_theme_toggle), NULL);
  toolbar_add_button(app_state.main_toolbar, theme_toggle_btn);

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
static GtkWidget *wazoo_title_label_g = NULL; /* Title label in sidebar header */

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
  /* Don't clear left_wazoo_float here — let the caller (wazoo_redock or
   * GTK close-request) handle destruction and pointer cleanup. */
  return FALSE;  /* Allow GTK to destroy the window */
}

/* Get the tab title for a given page widget */
static const char *wazoo_tab_title_for_page(GtkWidget *page) {
  if (!left_wazoo || !GTK_IS_NOTEBOOK(left_wazoo) || !page)
    return "Mailboxes";
  GtkWidget *tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(left_wazoo), page);
  if (!tab_label) return "Mailboxes";
  if (GTK_IS_LABEL(tab_label))
    return gtk_label_get_text(GTK_LABEL(tab_label));
  if (GTK_IS_BOX(tab_label)) {
    for (GtkWidget *child = gtk_widget_get_first_child(tab_label);
         child; child = gtk_widget_get_next_sibling(child)) {
      if (GTK_IS_LABEL(child))
        return gtk_label_get_text(GTK_LABEL(child));
    }
  }
  return "Mailboxes";
}

/* Update titles when wazoo tab is switched — use the page arg, not
   get_current_page which still returns the old page during switch-page */
static void on_wazoo_tab_switched(GtkNotebook *nb, GtkWidget *page,
                                   guint page_num, gpointer ud) {
  (void)nb; (void)page_num; (void)ud;
  const char *title = wazoo_tab_title_for_page(page);
  if (left_wazoo_float)
    gtk_window_set_title(GTK_WINDOW(left_wazoo_float), title);
  if (wazoo_title_label_g)
    gtk_label_set_text(GTK_LABEL(wazoo_title_label_g), title);
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
  {
    int pn = gtk_notebook_get_current_page(GTK_NOTEBOOK(left_wazoo));
    GtkWidget *pg = pn >= 0 ? gtk_notebook_get_nth_page(GTK_NOTEBOOK(left_wazoo), pn) : NULL;
    gtk_window_set_title(GTK_WINDOW(left_wazoo_float), wazoo_tab_title_for_page(pg));
  }
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
  GtkWidget *win = left_wazoo_float;
  left_wazoo_float = NULL;
  /* Reparent content back to main paned, then destroy the float window */
  on_wazoo_float_close(GTK_WINDOW(win), NULL);
  gtk_window_destroy(GTK_WINDOW(win));
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

/* Create a tab label widget with icon + text */
static GtkWidget *make_tab_label(const char *icon_name, const char *text) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  if (icon_name) {
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 14);
    gtk_box_append(GTK_BOX(box), icon);
  }
  GtkWidget *lbl = gtk_label_new(text);
  gtk_box_append(GTK_BOX(box), lbl);
  return box;
}

/* When a tab is dragged out of a notebook, GTK calls this to get the
 * target notebook for the new floating window. */
static GtkNotebook *on_create_window(GtkNotebook *notebook, GtkWidget *page,
                                      gpointer user_data) {
  (void)user_data;
  const char *title = "Eudora";
  GtkWidget *tab_label = gtk_notebook_get_tab_label(notebook, page);
  if (tab_label) {
    /* Tab label might be a box with icon + label children */
    if (GTK_IS_LABEL(tab_label))
      title = gtk_label_get_text(GTK_LABEL(tab_label));
    else if (GTK_IS_BOX(tab_label)) {
      for (GtkWidget *child = gtk_widget_get_first_child(tab_label);
           child; child = gtk_widget_get_next_sibling(child)) {
        if (GTK_IS_LABEL(child)) {
          title = gtk_label_get_text(GTK_LABEL(child));
          break;
        }
      }
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

/* Selection changed in a mailbox tab — update tab's preview pane */
static void on_tab_selection_changed(GtkSelectionModel *model,
                                     guint position, guint n_items,
                                     gpointer user_data) {
  (void)position; (void)n_items;
  GtkWidget *vpaned = GTK_WIDGET(user_data);
  GtkWidget *preview_box = g_object_get_data(G_OBJECT(vpaned), "preview-box");
  TOCType *toc = g_object_get_data(G_OBJECT(vpaned), "toc");
  GtkSingleSelection *sel = GTK_SINGLE_SELECTION(model);
  if (!preview_box || !toc) return;

  GtkMessageListItem *msg = GTK_MESSAGELIST_ITEM(
      gtk_single_selection_get_selected_item(sel));
  if (!msg) return;

  int idx = gtk_messagelist_item_get_index(msg);
  gchar *raw = read_message_raw(toc, idx);
  if (!raw) return;

  /* Remove old child from preview box */
  GtkWidget *old_child = gtk_widget_get_first_child(preview_box);
  if (old_child)
    gtk_box_remove(GTK_BOX(preview_box), old_child);

  /* Add new message view */
  GtkWidget *msg_view = create_message_view(raw, 4096);
  gtk_widget_set_vexpand(msg_view, TRUE);
  gtk_box_append(GTK_BOX(preview_box), msg_view);
  g_free(raw);
}

/* Create a mailbox tab: VPaned with message list on top, preview on bottom.
 * Like original Eudora mailbox window. */
static GtkWidget *create_mailbox_tab_content(TOCType *toc) {
  /* Use the real mailbox panel from mailbox.c with all original
     Eudora columns (Status, Priority, Attach, Label, Who, Date,
     Size, Junk, Subject) and preview pane */
  return CreateMailboxPanel(toc);
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

/* Open a named panel as a tab in the right notebook.
 * If already open, switches to it. Returns the page widget. */
static GtkWidget *open_panel_tab_icon(const char *panel_id, const char *title,
                                       const char *icon_name,
                                       GtkWidget *(*builder)(void)) {
  if (!mailbox_notebook) return NULL;

  /* Check if already open */
  int n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(mailbox_notebook));
  for (int i = 0; i < n; i++) {
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(mailbox_notebook), i);
    const char *pid = g_object_get_data(G_OBJECT(page), "panel-id");
    if (pid && strcmp(pid, panel_id) == 0) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(mailbox_notebook), i);
      return page;
    }
  }

  /* Build new content */
  GtkWidget *content = builder();
  g_object_set_data_full(G_OBJECT(content), "panel-id",
                         g_strdup(panel_id), g_free);

  /* Tab label with icon and close button */
  GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  if (icon_name) {
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 14);
    gtk_box_append(GTK_BOX(tab_box), icon);
  }
  GtkWidget *label = gtk_label_new(title);
  GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close-symbolic");
  gtk_button_set_has_frame(GTK_BUTTON(close_btn), FALSE);
  gtk_box_append(GTK_BOX(tab_box), label);
  gtk_box_append(GTK_BOX(tab_box), close_btn);

  int idx = gtk_notebook_append_page(GTK_NOTEBOOK(mailbox_notebook),
                                      content, tab_box);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(mailbox_notebook), content, TRUE);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(mailbox_notebook), idx);

  g_object_set_data(G_OBJECT(close_btn), "page-widget", content);
  g_signal_connect(close_btn, "clicked",
                   G_CALLBACK(on_mailbox_tab_close), NULL);

  return content;
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

  /* Tab label with icon and close button */
  GtkWidget *tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *mail_icon = gtk_image_new_from_icon_name("mail-unread-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(mail_icon), 14);
  gtk_box_append(GTK_BOX(tab_box), mail_icon);
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

/* ── Welcome dashboard ── */

/* Welcome CSS is now provided by theme.c */
static void ensure_welcome_css(void) { /* handled by theme engine */ }

/* Count messages in a .toc file (each summary is fixed-size after header) */
static int count_toc_messages(const char *mailbox_name) {
  const char *mdir = prefs_get_mailboxes_path();
  if (!mdir) return 0;
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s.toc", mdir, mailbox_name);
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  long data = st.st_size - (long)sizeof(short); /* TOC version header */
  if (data <= 0) return 0;
  return (int)(data / (long)sizeof(MSumType));
}

/* ── Stat pill widget ── */
/* Callback for clickable stat pills — opens the named mailbox */
static void on_stat_pill_clicked(GtkButton *btn, gpointer ud) {
  (void)btn;
  const char *name = (const char *)ud;
  gchar *path = gtk_mailbox_get_path(name);
  if (path) { open_mailbox_tab(name, path); g_free(path); }
}

static GtkWidget *stat_pill(const char *number, const char *label,
                             const char *accent_class,
                             const char *mailbox_name) {
  GtkWidget *btn = gtk_button_new();
  gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
  gtk_widget_add_css_class(btn, "stat-card");
  if (accent_class) gtk_widget_add_css_class(btn, accent_class);
  gtk_widget_set_hexpand(btn, TRUE);

  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  GtkWidget *num = gtk_label_new(number);
  gtk_widget_add_css_class(num, "stat-number");
  gtk_label_set_xalign(GTK_LABEL(num), 0);
  gtk_box_append(GTK_BOX(card), num);

  GtkWidget *lbl = gtk_label_new(label);
  gtk_widget_add_css_class(lbl, "stat-label");
  gtk_label_set_xalign(GTK_LABEL(lbl), 0);
  gtk_box_append(GTK_BOX(card), lbl);

  gtk_button_set_child(GTK_BUTTON(btn), card);

  if (mailbox_name)
    g_signal_connect(btn, "clicked",
                     G_CALLBACK(on_stat_pill_clicked), (gpointer)mailbox_name);

  return btn;
}

/* ── Action card widget ── */
static GtkWidget *action_card(const char *icon_name, const char *icon_color_class,
                               const char *title, const char *desc,
                               const char *action_name) {
  GtkWidget *btn = gtk_button_new();
  gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
  gtk_widget_add_css_class(btn, "action-card");
  gtk_widget_set_hexpand(btn, TRUE);

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);

  /* Icon in colored circle */
  GtkWidget *icon_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(icon_frame, "action-icon");
  if (icon_color_class) gtk_widget_add_css_class(icon_frame, icon_color_class);
  gtk_widget_set_valign(icon_frame, GTK_ALIGN_CENTER);
  GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 22);
  gtk_box_append(GTK_BOX(icon_frame), icon);
  gtk_box_append(GTK_BOX(hbox), icon_frame);

  /* Text */
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_hexpand(vbox, TRUE);

  GtkWidget *t = gtk_label_new(title);
  gtk_widget_add_css_class(t, "action-title");
  gtk_label_set_xalign(GTK_LABEL(t), 0);
  gtk_box_append(GTK_BOX(vbox), t);

  GtkWidget *d = gtk_label_new(desc);
  gtk_widget_add_css_class(d, "action-desc");
  gtk_label_set_xalign(GTK_LABEL(d), 0);
  gtk_label_set_wrap(GTK_LABEL(d), TRUE);
  gtk_box_append(GTK_BOX(vbox), d);

  gtk_box_append(GTK_BOX(hbox), vbox);

  /* Arrow indicator */
  GtkWidget *arrow = gtk_image_new_from_icon_name("go-next-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(arrow), 16);
  gtk_widget_set_opacity(arrow, 0.3);
  gtk_widget_set_valign(arrow, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(hbox), arrow);

  gtk_button_set_child(GTK_BUTTON(btn), hbox);
  gtk_actionable_set_action_name(GTK_ACTIONABLE(btn), action_name);
  return btn;
}

/* ── Shortcut pill ── */
static GtkWidget *shortcut_pill(const char *key, const char *label,
                                 const char *action_name) {
  GtkWidget *btn = gtk_button_new();
  gtk_button_set_has_frame(GTK_BUTTON(btn), FALSE);
  gtk_widget_add_css_class(btn, "shortcut-pill");
  gtk_widget_set_hexpand(btn, TRUE);

  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(hbox, GTK_ALIGN_CENTER);

  GtkWidget *k = gtk_label_new(key);
  gtk_widget_add_css_class(k, "shortcut-key");
  gtk_box_append(GTK_BOX(hbox), k);

  GtkWidget *l = gtk_label_new(label);
  gtk_widget_add_css_class(l, "shortcut-label");
  gtk_box_append(GTK_BOX(hbox), l);

  gtk_button_set_child(GTK_BUTTON(btn), hbox);
  if (action_name)
    gtk_actionable_set_action_name(GTK_ACTIONABLE(btn), action_name);
  return btn;
}

/* ── Section header ── */
static GtkWidget *wc_section(const char *text) {
  GtkWidget *lbl = gtk_label_new(text);
  gtk_widget_add_css_class(lbl, "wc-section-title");
  gtk_label_set_xalign(GTK_LABEL(lbl), 0);
  gtk_widget_set_margin_top(lbl, 20);
  gtk_widget_set_margin_bottom(lbl, 8);
  return lbl;
}

static GtkWidget *create_welcome_page(void) {
  ensure_welcome_css();

  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_vexpand(scroll, TRUE);

  GtkWidget *bg = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(bg, "welcome-bg");
  gtk_widget_set_hexpand(bg, TRUE);

  /* ════════════════════════════════════════════
   * Hero banner with greeting + account info
   * ════════════════════════════════════════════ */
  GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_add_css_class(hero, "welcome-hero");

  /* Greeting */
  gchar *real_name = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "real_name", "");
  const char *first = (real_name && real_name[0]) ? real_name : "there";
  /* Extract first name only */
  char first_name[64];
  g_strlcpy(first_name, first, sizeof(first_name));
  char *sp = strchr(first_name, ' ');
  if (sp) *sp = '\0';

  GDateTime *now = g_date_time_new_now_local();
  int hour = g_date_time_get_hour(now);
  const char *tod = (hour < 12) ? "Good morning" :
                    (hour < 17) ? "Good afternoon" : "Good evening";
  char greeting[128];
  snprintf(greeting, sizeof(greeting), "%s, %s", tod, first_name);
  g_date_time_unref(now);
  g_free(real_name);

  GtkWidget *greet_lbl = gtk_label_new(greeting);
  gtk_widget_add_css_class(greet_lbl, "welcome-greeting");
  gtk_label_set_xalign(GTK_LABEL(greet_lbl), 0);
  gtk_box_append(GTK_BOX(hero), greet_lbl);

  /* Account line */
  gchar *email = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "email_address", "");
  gchar *server = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_server", "");
  gboolean use_imap = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_imap", FALSE);
  gboolean use_ssl = prefs_get_bool(PREFS_GROUP_SSL, "use_ssl", FALSE);
  char acct_line[256];
  snprintf(acct_line, sizeof(acct_line), "%s  %s  %s  %s",
           (email && email[0]) ? email : "No email configured",
           use_imap ? "IMAP" : "POP3",
           (server && server[0]) ? server : "",
           use_ssl ? "SSL" : "");
  g_free(email);
  g_free(server);

  GtkWidget *acct_lbl = gtk_label_new(acct_line);
  gtk_widget_add_css_class(acct_lbl, "welcome-account-line");
  gtk_label_set_xalign(GTK_LABEL(acct_lbl), 0);
  gtk_box_append(GTK_BOX(hero), acct_lbl);

  gtk_box_append(GTK_BOX(bg), hero);

  /* ════════════════════════════════════════════
   * Dashboard body — padded content area
   * ════════════════════════════════════════════ */
  GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_start(body, 32);
  gtk_widget_set_margin_end(body, 32);
  gtk_widget_set_margin_top(body, 20);
  gtk_widget_set_margin_bottom(body, 32);

  /* ── Stats row: 4 metric cards ── */
  int inbox_n = count_toc_messages("In");
  int out_n   = count_toc_messages("Out");
  int trash_n = count_toc_messages("Trash");
  int junk_n  = count_toc_messages("Junk");

  char s1[16], s2[16], s3[16], s4[16];
  snprintf(s1, sizeof(s1), "%d", inbox_n);
  snprintf(s2, sizeof(s2), "%d", out_n);
  snprintf(s3, sizeof(s3), "%d", junk_n);
  snprintf(s4, sizeof(s4), "%d", trash_n);

  GtkWidget *stats = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_append(GTK_BOX(stats), stat_pill(s1, "Inbox",   "stat-accent-blue",  "In"));
  gtk_box_append(GTK_BOX(stats), stat_pill(s2, "Outbox",  "stat-accent-green", "Out"));
  gtk_box_append(GTK_BOX(stats), stat_pill(s3, "Junk",    "stat-accent-amber", "Junk"));
  gtk_box_append(GTK_BOX(stats), stat_pill(s4, "Trash",   "stat-accent-red",   "Trash"));
  gtk_box_append(GTK_BOX(body), stats);

  /* ── Two-column grid: actions left, tools right ── */
  GtkWidget *columns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_widget_set_margin_top(columns, 4);

  /* Left column — primary actions */
  GtkWidget *col_left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(col_left, TRUE);

  gtk_box_append(GTK_BOX(col_left), wc_section("ACTIONS"));

  GtkWidget *al = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(al),
      action_card("mail-unread-symbolic", NULL,
                  "Check Mail", "Fetch new messages  \xe2\x8c\x98M",
                  "app.check-mail"));
  gtk_box_append(GTK_BOX(al),
      action_card("document-new-symbolic", "action-icon-green",
                  "New Message", "Compose a new email  \xe2\x8c\x98N",
                  "app.new-message"));
  gtk_box_append(GTK_BOX(al),
      action_card("mail-send-symbolic", "action-icon-amber",
                  "Send Queued", "Deliver queued messages  \xe2\x8c\x98T",
                  "app.send-queued"));
  gtk_box_append(GTK_BOX(al),
      action_card("mail-reply-sender-symbolic", "action-icon-purple",
                  "Reply", "Reply to the selected message  \xe2\x8c\x98R",
                  "app.reply"));
  gtk_box_append(GTK_BOX(al),
      action_card("mail-forward-symbolic", "action-icon-cyan",
                  "Forward", "Forward the selected message",
                  "app.forward"));
  gtk_box_append(GTK_BOX(col_left), al);
  gtk_box_append(GTK_BOX(columns), col_left);

  /* Right column — tools & manage */
  GtkWidget *col_right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_hexpand(col_right, TRUE);

  gtk_box_append(GTK_BOX(col_right), wc_section("MANAGE"));

  GtkWidget *ar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_append(GTK_BOX(ar),
      action_card("x-office-address-book-symbolic", NULL,
                  "Address Book", "Manage contacts and nicknames",
                  "app.address-book"));
  gtk_box_append(GTK_BOX(ar),
      action_card("edit-find-symbolic", "action-icon-amber",
                  "Filters", "Automatic mail sorting rules",
                  "app.filters"));
  gtk_box_append(GTK_BOX(ar),
      action_card("preferences-system-symbolic", "action-icon-purple",
                  "Settings", "Accounts, display, and behavior  \xe2\x8c\x98,",
                  "app.preferences"));
  gtk_box_append(GTK_BOX(ar),
      action_card("avatar-default-symbolic", "action-icon-rose",
                  "Personalities", "Manage email identities",
                  "app.personalities"));
  gtk_box_append(GTK_BOX(ar),
      action_card("utilities-system-monitor-symbolic", "action-icon-cyan",
                  "Statistics", "View mail usage statistics",
                  "app.statistics"));
  gtk_box_append(GTK_BOX(col_right), ar);
  gtk_box_append(GTK_BOX(columns), col_right);

  gtk_box_append(GTK_BOX(body), columns);

  /* ── Tip bar ── */
  GtkWidget *tip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_add_css_class(tip, "tip-bar");
  gtk_widget_set_margin_top(tip, 16);

  GtkWidget *tip_icon = gtk_image_new_from_icon_name("dialog-information-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(tip_icon), 18);
  gtk_widget_set_valign(tip_icon, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(tip), tip_icon);

  GtkWidget *tip_text = gtk_label_new(
      "Tip: Double-click any mailbox in the sidebar to open it in a new tab. "
      "Drag tabs to reorder or tear them off into separate windows.");
  gtk_widget_add_css_class(tip_text, "tip-text");
  gtk_label_set_wrap(GTK_LABEL(tip_text), TRUE);
  gtk_widget_set_hexpand(tip_text, TRUE);
  gtk_box_append(GTK_BOX(tip), tip_text);

  gtk_box_append(GTK_BOX(body), tip);

  gtk_box_append(GTK_BOX(bg), body);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), bg);

  return scroll;
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

  GtkWidget *wazoo_title_label = gtk_label_new("Mailboxes");
  wazoo_title_label_g = wazoo_title_label;
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
  g_signal_connect(left_wazoo, "switch-page",
                   G_CALLBACK(on_wazoo_tab_switched), NULL);

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

  GtkWidget *btn_rename = gtk_button_new_with_label("Rename");
  gtk_widget_set_tooltip_text(btn_rename, "Rename selected mailbox");
  g_signal_connect(btn_rename, "clicked", G_CALLBACK(on_mb_rename), NULL);
  gtk_box_append(GTK_BOX(mb_btn_bar), btn_rename);

  GtkWidget *btn_remove = gtk_button_new_with_label("Del");
  gtk_widget_set_tooltip_text(btn_remove, "Remove selected mailbox");
  g_signal_connect(btn_remove, "clicked", G_CALLBACK(on_mb_remove), NULL);
  gtk_box_append(GTK_BOX(mb_btn_bar), btn_remove);

  gtk_box_append(GTK_BOX(mb_vbox), mb_btn_bar);

  gtk_notebook_append_page(GTK_NOTEBOOK(left_wazoo), mb_vbox,
                           make_tab_label("folder-symbolic", "Mailboxes"));
  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(left_wazoo), mb_vbox, TRUE);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(left_wazoo), mb_vbox, TRUE);

  /* Task Progress tab — shows active tasks, errors, check times */
  GtkWidget *tp_panel = create_task_progress_widget();
  gtk_notebook_append_page(GTK_NOTEBOOK(left_wazoo), tp_panel,
                           make_tab_label("view-list-symbolic", "Tasks"));
  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(left_wazoo), tp_panel, TRUE);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(left_wazoo), tp_panel, TRUE);

  gtk_widget_set_vexpand(left_wazoo, TRUE);
  gtk_box_append(GTK_BOX(left_wazoo_box), left_wazoo);

  /* Double-click / Enter opens a mailbox tab on the right */
  g_signal_connect(app_state.mailbox_tree, "row-activated",
                   G_CALLBACK(on_mailbox_activated), NULL);

  gtk_widget_set_size_request(left_wazoo_box, 200, -1);
  gtk_paned_set_start_child(GTK_PANED(main_hpaned), left_wazoo_box);
  gtk_paned_set_resize_start_child(GTK_PANED(main_hpaned), FALSE);
  gtk_paned_set_shrink_start_child(GTK_PANED(main_hpaned), FALSE);
  gtk_paned_set_position(GTK_PANED(main_hpaned), 220);

  /* ── Right: Mailbox notebook (one tab per opened mailbox) ── */
  mailbox_notebook = gtk_notebook_new();
  gtk_notebook_set_group_name(GTK_NOTEBOOK(mailbox_notebook), WAZOO_GROUP);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(mailbox_notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(mailbox_notebook), TRUE);
  g_signal_connect(mailbox_notebook, "create-window",
                   G_CALLBACK(on_create_window), NULL);

  /* Start with a welcome page */
  GtkWidget *welcome = create_welcome_page();
  gtk_notebook_append_page(GTK_NOTEBOOK(mailbox_notebook), welcome,
                           make_tab_label("go-home-symbolic", "Welcome"));

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

  /* Initialize personalities (mail accounts) from prefs */
  InitPersonalities();

  /* Load settings from disk */
  app_state.settings = prefs_load();
  if (!app_state.settings) {
    /* Create default settings if load failed */
    app_state.settings = g_new0(AppSettings, 1);
  }

  /* Initialize theme system (loads saved theme from prefs) */
  theme_init(app_state.window);
  update_theme_tooltip();

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

  /* Initialize thread-local globals pointer for the main thread.
     Must happen before ANY code accesses CurThreadGlobals macros
     (PersList, CurPers, etc). Cannot use a TLS initializer on
     macOS ARM64 because address-valued TLS initializers don't
     get proper fixups. */
  CurThreadGlobals = &ThreadGlobals;

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
