/* gtk_autocomplete.c — Nickname autocomplete popover for address entries
 *
 * Attaches to GtkEntry fields (To, Cc, Bcc). As the user types,
 * queries macmbx_nick_complete() and shows a popover with matches.
 *
 * Behavior (ported from original nickexp.c):
 * - Type-ahead: inline completion appended as selected text
 * - Popup list: shows when multiple matches exist
 * - Tab: accepts current inline completion, inserts comma
 * - Enter/click: picks from popup list
 * - Escape: dismisses popup
 * - Comma: finalizes current recipient, starts new one
 * - Handles multiple recipients separated by commas
 */

#include "gtk_autocomplete.h"
#include "macmbx.h"

#include <string.h>
#include <strings.h>
#include <ctype.h>

#define MAX_POPUP_ROWS 10
#define MIN_PREFIX_LEN 1

typedef struct {
  GtkWidget *entry;
  MacmbxAddressBooks *abs;
  GtkWidget *popover;
  GtkWidget *listbox;
  MacmbxCompleteResult *results;
  int result_count;
  bool suppress;          /* suppress next changed signal (we're inserting) */
  int recipient_start;    /* char offset where current recipient begins */
} AutocompleteCtx;

static void ctx_free(AutocompleteCtx *ctx) {
  free(ctx->results);
  g_free(ctx);
}

/* Find the start of the current recipient being typed.
 * Recipients are comma-separated. Returns offset into entry text. */
static int find_recipient_start(const char *text, int cursor) {
  int start = cursor;
  while (start > 0 && text[start - 1] != ',') start--;
  /* Skip leading whitespace */
  while (start < cursor && text[start] == ' ') start++;
  return start;
}

/* Extract the current prefix being typed (from last comma to cursor). */
static char *get_current_prefix(GtkEditable *editable, int *out_start) {
  const char *text = gtk_editable_get_text(editable);
  int cursor = gtk_editable_get_position(editable);
  int start = find_recipient_start(text, cursor);
  if (out_start) *out_start = start;

  int len = cursor - start;
  if (len <= 0) return g_strdup("");

  return g_strndup(text + start, len);
}

static void dismiss_popup(AutocompleteCtx *ctx) {
  if (ctx->popover && gtk_widget_get_visible(ctx->popover))
    gtk_popover_popdown(GTK_POPOVER(ctx->popover));
}

/* Insert a completed address, replacing the current prefix. */
static void insert_completion(AutocompleteCtx *ctx, const char *completion) {
  GtkEditable *editable = GTK_EDITABLE(ctx->entry);
  const char *text = gtk_editable_get_text(editable);
  int cursor = gtk_editable_get_position(editable);
  int start = find_recipient_start(text, cursor);

  ctx->suppress = true;

  /* Build new text: everything before start + completion + ", " + everything after cursor */
  int text_len = (int)strlen(text);
  /* Check if there's already content after the cursor (more recipients) */
  const char *after = text + cursor;
  /* Skip any selected text that might follow the cursor */

  GString *buf = g_string_new(NULL);
  g_string_append_len(buf, text, start);
  g_string_append(buf, completion);
  g_string_append(buf, ", ");
  int new_cursor = (int)buf->len;
  if (*after) g_string_append(buf, after);

  gtk_editable_set_text(editable, buf->str);
  gtk_editable_set_position(editable, new_cursor);

  g_string_free(buf, TRUE);
  ctx->suppress = false;

  dismiss_popup(ctx);
}

/* Show inline type-ahead: append the rest of top match as selected text */
static void show_typeahead(AutocompleteCtx *ctx, const char *prefix, int prefix_start) {
  if (ctx->result_count == 0) return;

  const char *display = ctx->results[0].display;
  if (!display) display = ctx->results[0].name;
  if (!display) return;

  int prefix_len = (int)strlen(prefix);
  int display_len = (int)strlen(display);
  if (display_len <= prefix_len) return;

  /* Check that display starts with prefix (case insensitive) */
  if (strncasecmp(display, prefix, prefix_len) != 0) {
    /* Try matching against just the name */
    display = ctx->results[0].name;
    if (!display || strncasecmp(display, prefix, prefix_len) != 0)
      return;
    display_len = (int)strlen(display);
  }

  ctx->suppress = true;

  GtkEditable *editable = GTK_EDITABLE(ctx->entry);
  const char *text = gtk_editable_get_text(editable);
  int cursor = gtk_editable_get_position(editable);

  /* Insert the remaining characters after cursor */
  const char *tail = display + prefix_len;
  int tail_len = display_len - prefix_len;

  GString *buf = g_string_new(NULL);
  g_string_append_len(buf, text, cursor);
  g_string_append_len(buf, tail, tail_len);
  /* Preserve anything after current cursor (other recipients) */
  int old_len = (int)strlen(text);
  if (cursor < old_len)
    g_string_append(buf, text + cursor);

  gtk_editable_set_text(editable, buf->str);
  /* Select the auto-completed part so typing replaces it */
  gtk_editable_select_region(editable, cursor, cursor + tail_len);

  g_string_free(buf, TRUE);
  ctx->suppress = false;
}

/* Build and show the popup listbox */
static void show_popup(AutocompleteCtx *ctx) {
  if (ctx->result_count <= 0) {
    dismiss_popup(ctx);
    return;
  }

  /* Create popover on first use */
  if (!ctx->popover) {
    ctx->popover = gtk_popover_new();
    gtk_popover_set_autohide(GTK_POPOVER(ctx->popover), FALSE);
    gtk_popover_set_has_arrow(GTK_POPOVER(ctx->popover), FALSE);
    gtk_widget_set_parent(ctx->popover, ctx->entry);

    GtkWidget *sw = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(sw), 250);
    gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(sw), TRUE);
    gtk_widget_set_size_request(sw, 350, -1);

    ctx->listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(ctx->listbox),
                                     GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sw), ctx->listbox);
    gtk_popover_set_child(GTK_POPOVER(ctx->popover), sw);
  }

  /* Clear old rows */
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(ctx->listbox)))
    gtk_list_box_remove(GTK_LIST_BOX(ctx->listbox), child);

  /* Add result rows */
  int show = ctx->result_count < MAX_POPUP_ROWS ? ctx->result_count : MAX_POPUP_ROWS;
  for (int i = 0; i < show; i++) {
    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(row_box, 6);
    gtk_widget_set_margin_end(row_box, 6);
    gtk_widget_set_margin_top(row_box, 3);
    gtk_widget_set_margin_bottom(row_box, 3);

    /* Display name / nickname */
    const char *display = ctx->results[i].display;
    if (!display) display = ctx->results[i].name;
    GtkWidget *name_label = gtk_label_new(display);
    gtk_label_set_xalign(GTK_LABEL(name_label), 0);
    gtk_label_set_ellipsize(GTK_LABEL(name_label), PANGO_ELLIPSIZE_END);
    gtk_widget_add_css_class(name_label, "autocomplete-name");
    gtk_box_append(GTK_BOX(row_box), name_label);

    /* Address line (if different from display) */
    const char *addr = ctx->results[i].addresses;
    if (addr && addr[0]) {
      GtkWidget *addr_label = gtk_label_new(addr);
      gtk_label_set_xalign(GTK_LABEL(addr_label), 0);
      gtk_label_set_ellipsize(GTK_LABEL(addr_label), PANGO_ELLIPSIZE_END);
      gtk_widget_add_css_class(addr_label, "autocomplete-addr");
      gtk_widget_set_opacity(addr_label, 0.6);
      gtk_box_append(GTK_BOX(row_box), addr_label);
    }

    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    /* Store index for selection */
    g_object_set_data(G_OBJECT(row), "ac-index", GINT_TO_POINTER(i));
    gtk_list_box_append(GTK_LIST_BOX(ctx->listbox), row);
  }

  /* Position below the entry */
  GdkRectangle rect = { 0, 0, 0, 0 };
  gtk_widget_get_allocation(ctx->entry, (GtkAllocation *)&rect);
  rect.y = rect.height;
  rect.height = 0;
  gtk_popover_set_pointing_to(GTK_POPOVER(ctx->popover), &rect);

  gtk_popover_popup(GTK_POPOVER(ctx->popover));
}

/* Row activated — pick the selection */
static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer ud) {
  (void)box;
  AutocompleteCtx *ctx = (AutocompleteCtx *)ud;
  int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "ac-index"));
  if (idx >= 0 && idx < ctx->result_count) {
    /* Use addresses if available, else display name */
    const char *insert = ctx->results[idx].addresses;
    if (!insert || !insert[0]) insert = ctx->results[idx].display;
    if (!insert) insert = ctx->results[idx].name;
    if (insert) insert_completion(ctx, insert);
  }
}

/* Deferred typeahead + popup — runs after the changed signal completes,
 * avoiding GTK's "Cannot begin irreversible action while in user action" */
static gboolean deferred_complete_cb(gpointer ud) {
  AutocompleteCtx *ctx = (AutocompleteCtx *)ud;
  if (!ctx->entry || !gtk_widget_get_realized(ctx->entry))
    return G_SOURCE_REMOVE;

  int prefix_start = 0;
  char *prefix = get_current_prefix(GTK_EDITABLE(ctx->entry), &prefix_start);

  if (ctx->result_count > 0 && (int)strlen(prefix) >= MIN_PREFIX_LEN) {
    show_typeahead(ctx, prefix, prefix_start);

    if (ctx->result_count > 1)
      show_popup(ctx);
    else
      dismiss_popup(ctx);
  }

  g_free(prefix);
  return G_SOURCE_REMOVE;
}

/* Main handler: text changed in entry */
static void on_entry_changed(GtkEditable *editable, gpointer ud) {
  AutocompleteCtx *ctx = (AutocompleteCtx *)ud;
  if (ctx->suppress) return;
  if (!ctx->abs) return;

  int prefix_start = 0;
  char *prefix = get_current_prefix(editable, &prefix_start);
  ctx->recipient_start = prefix_start;

  /* Free previous results */
  free(ctx->results);
  ctx->results = NULL;
  ctx->result_count = 0;

  if ((int)strlen(prefix) < MIN_PREFIX_LEN) {
    dismiss_popup(ctx);
    g_free(prefix);
    return;
  }

  /* Query macmbx — data lookup is fine inside the signal */
  ctx->result_count = macmbx_nick_complete(ctx->abs, prefix,
                                             &ctx->results, MAX_POPUP_ROWS);
  g_free(prefix);

  if (ctx->result_count <= 0) {
    dismiss_popup(ctx);
    return;
  }

  /* Defer the text modification to avoid GTK user-action nesting */
  g_idle_add(deferred_complete_cb, ctx);
}

/* Key handler — Tab accepts completion, Escape dismisses, arrow keys navigate */
static gboolean on_key_pressed(GtkEventControllerKey *controller,
                                guint keyval, guint keycode,
                                GdkModifierType state, gpointer ud) {
  (void)controller; (void)keycode; (void)state;
  AutocompleteCtx *ctx = (AutocompleteCtx *)ud;

  if (keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab) {
    if (ctx->result_count > 0) {
      /* Accept top match */
      const char *insert = ctx->results[0].addresses;
      if (!insert || !insert[0]) insert = ctx->results[0].display;
      if (!insert) insert = ctx->results[0].name;
      if (insert) {
        insert_completion(ctx, insert);
        return TRUE;
      }
    }
  }

  if (keyval == GDK_KEY_Escape) {
    dismiss_popup(ctx);
    /* Clear any type-ahead selection */
    GtkEditable *editable = GTK_EDITABLE(ctx->entry);
    int pos = gtk_editable_get_position(editable);
    gtk_editable_select_region(editable, pos, pos);
    return TRUE;
  }

  if (keyval == GDK_KEY_Down && ctx->popover &&
      gtk_widget_get_visible(ctx->popover)) {
    /* Move focus into the popup list */
    gtk_widget_grab_focus(ctx->listbox);
    GtkListBoxRow *first = gtk_list_box_get_row_at_index(
        GTK_LIST_BOX(ctx->listbox), 0);
    if (first)
      gtk_list_box_select_row(GTK_LIST_BOX(ctx->listbox), first);
    return TRUE;
  }

  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
    if (ctx->popover && gtk_widget_get_visible(ctx->popover)) {
      GtkListBoxRow *sel = gtk_list_box_get_selected_row(
          GTK_LIST_BOX(ctx->listbox));
      if (sel) {
        on_row_activated(GTK_LIST_BOX(ctx->listbox), sel, ctx);
        return TRUE;
      }
    }
    /* Accept inline type-ahead if present */
    if (ctx->result_count > 0) {
      int start, end;
      if (gtk_editable_get_selection_bounds(GTK_EDITABLE(ctx->entry),
                                              &start, &end) && end > start) {
        const char *insert = ctx->results[0].addresses;
        if (!insert || !insert[0]) insert = ctx->results[0].display;
        if (!insert) insert = ctx->results[0].name;
        if (insert) {
          insert_completion(ctx, insert);
          return TRUE;
        }
      }
    }
  }

  return FALSE;
}

/* Cleanup on widget destroy */
static void on_entry_destroy(GtkWidget *widget, gpointer ud) {
  (void)widget;
  AutocompleteCtx *ctx = (AutocompleteCtx *)ud;
  if (ctx->popover) {
    gtk_widget_unparent(ctx->popover);
    ctx->popover = NULL;
  }
  ctx_free(ctx);
}

/* ================================================================
 * Public API
 * ================================================================ */

void gtk_autocomplete_attach(GtkWidget *entry, MacmbxAddressBooks *abs) {
  if (!entry || !abs) return;

  AutocompleteCtx *ctx = g_new0(AutocompleteCtx, 1);
  ctx->entry = entry;
  ctx->abs = abs;

  g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), ctx);
  g_signal_connect(entry, "destroy", G_CALLBACK(on_entry_destroy), ctx);

  GtkEventController *key_ctrl = gtk_event_controller_key_new();
  g_signal_connect(key_ctrl, "key-pressed", G_CALLBACK(on_key_pressed), ctx);
  gtk_widget_add_controller(entry, key_ctrl);

  /* Store ctx on the widget for detach */
  g_object_set_data(G_OBJECT(entry), "autocomplete-ctx", ctx);
}

void gtk_autocomplete_detach(GtkWidget *entry) {
  if (!entry) return;
  AutocompleteCtx *ctx = g_object_get_data(G_OBJECT(entry), "autocomplete-ctx");
  if (!ctx) return;

  g_signal_handlers_disconnect_by_data(entry, ctx);
  if (ctx->popover) {
    gtk_widget_unparent(ctx->popover);
    ctx->popover = NULL;
  }
  g_object_set_data(G_OBJECT(entry), "autocomplete-ctx", NULL);
  ctx_free(ctx);
}
