#include "gedit-state.h"
#include "geditctrl.h"
#include "gedit-clipboard.h"
#include "gedit-print.h"
#include <gtk/gtk.h>
#include <string.h>

GtkWidget *geditctrl_new(void) {
  GtkWidget *scrolled = gtk_scrolled_window_new();
  GtkWidget *area = gtk_drawing_area_new();
  gtk_widget_set_hexpand(area, TRUE);
  gtk_widget_set_vexpand(area, TRUE);
  gtk_widget_set_can_focus(area, TRUE);
  gtk_widget_set_can_focus(scrolled, TRUE);
  
  /* Set minimum size for the drawing area */
  gtk_widget_set_size_request(area, 200, 200);
  
  /* Configure scrolled window to use content-based sizing */
  gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled), TRUE);
  gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(scrolled), TRUE);

  geditDocument *doc = gedit_document_new();
  GEditCtrlState *s = g_new0(GEditCtrlState, 1);
  s->doc = doc;
  s->caret = 0;
  s->sel_anchor = -1;
  s->sel_start = 0;
  s->sel_end = 0;
  s->dragging = FALSE;
  s->caret_visible = TRUE;
  s->preferred_x = -1;

  g_object_set_data_full(G_OBJECT(area), "gedit-document", doc, g_object_unref);
  g_object_ref(doc);
  g_object_set_data_full(G_OBJECT(scrolled), "gedit-document", doc,
                         g_object_unref);
  g_object_set_data_full(G_OBJECT(scrolled), "gedit-state", s,
                         (GDestroyNotify)g_free);
  g_object_set_data(G_OBJECT(area), "gedit-state", s);

  GtkGesture *click = GTK_GESTURE(gtk_gesture_click_new());
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
  gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(click));
  g_signal_connect(click, "pressed", G_CALLBACK(gedit_pressed_cb), area);
  g_signal_connect(click, "released", G_CALLBACK(gedit_released_cb), area);

  GtkEventController *motion =
      GTK_EVENT_CONTROLLER(gtk_event_controller_motion_new());
  gtk_widget_add_controller(area, motion);
  g_signal_connect(motion, "motion", G_CALLBACK(gedit_motion_cb), area);

  GtkEventController *k1 = GTK_EVENT_CONTROLLER(gtk_event_controller_key_new());
  gtk_widget_add_controller(scrolled, k1);
  g_signal_connect(k1, "key-pressed", G_CALLBACK(gedit_key_pressed_cb), area);
  GtkEventController *k2 = GTK_EVENT_CONTROLLER(gtk_event_controller_key_new());
  gtk_widget_add_controller(area, k2);
  g_signal_connect(k2, "key-pressed", G_CALLBACK(gedit_key_pressed_cb), area);

  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), gedit_draw_cb, doc,
                                 NULL);
  g_signal_connect(doc, "document-changed", G_CALLBACK(gedit_doc_changed_cb),
                   area);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), area);
  return scrolled;
}

geditDocument *geditctrl_get_document(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return NULL;
  gpointer p = g_object_get_data(G_OBJECT(ctrl), "gedit-document");
  return gedit_DOCUMENT(p);
}

gint geditctrl_get_caret_offset(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return 0;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  return s ? s->caret : 0;
}

void geditctrl_toggle_style(GtkWidget *ctrl, gboolean bold, gboolean italic,
                            gboolean underline) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (a == b) {
    gchar *t = gedit_document_get_text(doc);
    if (!t)
      return;
    gint len = g_utf8_strlen(t, -1);
    gint pos = s->caret;
    if (pos < 0)
      pos = 0;
    if (pos > len)
      pos = len;
    gint st = pos;
    while (st > 0) {
      const gchar *p = g_utf8_offset_to_pointer(t, st - 1);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc))
        break;
      st--;
    }
    gint ed = pos;
    while (ed < len) {
      const gchar *p = g_utf8_offset_to_pointer(t, ed);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc))
        break;
      ed++;
    }
    if (ed > st)
      gedit_document_toggle_style(doc, st, ed - st, bold, italic, underline);
    g_free(t);
  } else
    gedit_document_toggle_style(doc, a, b - a, bold, italic, underline);
}

void geditctrl_set_color(GtkWidget *ctrl, const GdkRGBA *color) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (a == b) {
    gchar *t = gedit_document_get_text(doc);
    if (!t)
      return;
    gint len = g_utf8_strlen(t, -1);
    gint pos = s->caret;
    if (pos < 0)
      pos = 0;
    if (pos > len)
      pos = len;
    gint st = pos;
    while (st > 0) {
      const gchar *p = g_utf8_offset_to_pointer(t, st - 1);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc) || uc == '\n')
        break;
      st--;
    }
    gint ed = pos;
    while (ed < len) {
      const gchar *p = g_utf8_offset_to_pointer(t, ed);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc) || uc == '\n')
        break;
      ed++;
    }
    if (ed > st)
      gedit_document_add_style_run(doc, st, ed - st, FALSE, FALSE, FALSE, color,
                                   0);
    g_free(t);
  } else
    gedit_document_add_style_run(doc, a, b - a, FALSE, FALSE, FALSE, color, 0);
}

void geditctrl_change_font_size(GtkWidget *ctrl, gint delta_points) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (a == b) {
    gchar *t = gedit_document_get_text(doc);
    if (!t)
      return;
    gint len = g_utf8_strlen(t, -1);
    gint pos = s->caret;
    if (pos < 0)
      pos = 0;
    if (pos > len)
      pos = len;
    gint st = pos;
    while (st > 0) {
      const gchar *p = g_utf8_offset_to_pointer(t, st - 1);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc) || uc == '\n')
        break;
      st--;
    }
    gint ed = pos;
    while (ed < len) {
      const gchar *p = g_utf8_offset_to_pointer(t, ed);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc) || uc == '\n')
        break;
      ed++;
    }
    if (ed > st)
      gedit_document_add_style_run(doc, st, ed - st, FALSE, FALSE, FALSE, NULL,
                                   delta_points);
    g_free(t);
  } else
    gedit_document_add_style_run(doc, a, b - a, FALSE, FALSE, FALSE, NULL,
                                 delta_points);
}

void geditctrl_set_font_size(GtkWidget *ctrl, gint points) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (a == b) {
    gchar *t = gedit_document_get_text(doc);
    if (!t)
      return;
    gint len = g_utf8_strlen(t, -1);
    gint pos = s->caret;
    if (pos < 0)
      pos = 0;
    if (pos > len)
      pos = len;
    gint st = pos;
    while (st > 0) {
      const gchar *p = g_utf8_offset_to_pointer(t, st - 1);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc) || uc == '\n')
        break;
      st--;
    }
    gint ed = pos;
    while (ed < len) {
      const gchar *p = g_utf8_offset_to_pointer(t, ed);
      gunichar uc = g_utf8_get_char(p);
      if (g_unichar_isspace(uc) || uc == '\n')
        break;
      ed++;
    }
    if (ed > st)
      gedit_document_add_style_run(doc, st, ed - st, FALSE, FALSE, FALSE, NULL,
                                   points);
    g_free(t);
  } else
    gedit_document_add_style_run(doc, a, b - a, FALSE, FALSE, FALSE, NULL,
                                 points);
}

gboolean geditctrl_handle_key(GtkWidget *ctrl, guint keyval, guint keycode,
                              GdkModifierType state_mask) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return GDK_EVENT_PROPAGATE;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return GDK_EVENT_PROPAGATE;
  return gedit_key_pressed_cb(NULL, keyval, keycode, state_mask, area) ==
                 GDK_EVENT_STOP
             ? GDK_EVENT_STOP
             : GDK_EVENT_PROPAGATE;
}

void geditctrl_set_alignment(GtkWidget *ctrl, geditAlignment align) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint st, ln;
  gedit_get_active_para_range(s, doc, &st, &ln);
  if (ln > 0)
    gedit_document_set_alignment(doc, st, ln, align);
  gtk_widget_queue_draw(area);
}
void geditctrl_toggle_bullet(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint st, ln;
  gedit_get_active_para_range(s, doc, &st, &ln);
  if (ln > 0)
    gedit_document_toggle_bullet(doc, st, ln);
  gtk_widget_queue_draw(area);
}

void geditctrl_indent(GtkWidget *ctrl, gint delta_pixels) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint st, ln;
  gedit_get_active_para_range(s, doc, &st, &ln);
  if (ln > 0)
    gedit_document_indent(doc, st, ln, delta_pixels);
  gtk_widget_queue_draw(area);
}

void geditctrl_set_quote_level(GtkWidget *ctrl, gint level) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint st, ln;
  gedit_get_active_para_range(s, doc, &st, &ln);
  if (ln > 0)
    gedit_document_set_quote_level(doc, st, ln, level);
  gtk_widget_queue_draw(area);
}

void geditctrl_change_quote_level(GtkWidget *ctrl, gint delta) {
  g_print("gedit: geditctrl_change_quote_level delta=%d\n", delta);
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint st, ln;
  gedit_get_active_para_range(s, doc, &st, &ln);
  g_print("gedit: range st=%d ln=%d\n", st, ln);
  if (ln > 0) {
    geditParaAttr pattr;
    gedit_document_get_para_attr(doc, st, ln, &pattr);
    int new_level = pattr.quote_level + delta;
    if (new_level < 0)
      new_level = 0;
    g_print("gedit: setting quote level to %d\n", new_level);
    gedit_document_set_quote_level(doc, st, ln, new_level);
  }
  g_print("gedit: change_quote_level calling draw\n");
  gtk_widget_queue_draw(area);
}

void geditctrl_insert_image(GtkWidget *ctrl, GdkPixbuf *texture, gint width,
                            gint height) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gedit_document_insert_graphic(doc, s->caret, texture, width, height);
  s->caret++;
  s->sel_start = s->caret;
  s->sel_end = s->caret;
  gtk_widget_queue_draw(area);
}


void geditctrl_insert_hr(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gedit_document_insert_hr(doc, s->caret);
  s->caret++;
  s->sel_start = s->caret;
  s->sel_end = s->caret;
  gtk_widget_queue_draw(area);
}

void geditctrl_set_direction(GtkWidget *ctrl, geditDirection direction) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gint st, ln;
  gedit_get_active_para_range(s, doc, &st, &ln);
  if (ln > 0)
    gedit_document_set_direction(doc, st, ln, direction);
  gtk_widget_queue_draw(area);
}

void geditctrl_insert_page_break(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gedit_document_insert_page_break(doc, s->caret);
  s->caret++;
  s->sel_start = s->caret;
  s->sel_end = s->caret;
  gtk_widget_queue_draw(area);
}

void geditctrl_paste_plain(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;
  gedit_clipboard_paste_plain(area, s->caret, doc);
}

gint geditctrl_find_text(GtkWidget *ctrl, const gchar *search_text,
                         gboolean case_sensitive) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return -1;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return -1;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return -1;
  
  gint doc_len = gedit_document_get_length(doc);
  gint found = gedit_document_find_text(doc, search_text, s->caret, doc_len,
                                        case_sensitive);
  
  /* If not found from current position, wrap to beginning */
  if (found < 0 && s->caret > 0) {
    g_print("gedit: wrapping search to beginning\n");
    found = gedit_document_find_text(doc, search_text, 0, s->caret,
                                     case_sensitive);
  }
  
  if (found >= 0) {
    /* Select the found text */
    gint search_len = g_utf8_strlen(search_text, -1);
    s->sel_start = found;
    s->sel_end = found + search_len;
    s->caret = found + search_len;
    s->sel_anchor = -1;
    g_signal_emit_by_name(doc, "selection-changed");
    gedit_scroll_to_caret(area);
    gtk_widget_queue_draw(area);
    g_print("gedit: found at offset %d, selected %d-%d\n", found, s->sel_start, s->sel_end);
  } else {
    g_print("gedit: text not found\n");
  }
  return found;
}

gint geditctrl_replace_text(GtkWidget *ctrl, const gchar *search_text,
                            const gchar *replace_text, gboolean replace_all,
                            gboolean case_sensitive) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return 0;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return 0;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return 0;
  
  gint doc_len = gedit_document_get_length(doc);
  gint count = gedit_document_replace_text(doc, search_text, replace_text,
                                           s->caret, doc_len, replace_all,
                                           case_sensitive);
  if (count > 0) {
    s->sel_start = s->caret;
    s->sel_end = s->caret;
    s->sel_anchor = -1;
    g_signal_emit_by_name(doc, "selection-changed");
    gtk_widget_queue_draw(area);
    g_print("gedit: replaced %d occurrence(s)\n", count);
  }
  return count;
}


void geditctrl_print(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc)
    return;
  
  /* Get document title from window if available */
  GtkRoot *root = gtk_widget_get_root(ctrl);
  const gchar *title = "Document";
  if (GTK_IS_WINDOW(root)) {
    title = gtk_window_get_title(GTK_WINDOW(root));
  }
  
  gedit_print_document(ctrl, doc, title);
}

void geditctrl_set_label(GtkWidget *ctrl, gint start, gint end, gint label) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  if (!buf) return;

  /* Create or look up a tag for this label id */
  char tag_name[32];
  snprintf(tag_name, sizeof(tag_name), "label-%d", label);
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, tag_name);
  if (!tag)
    tag = gtk_text_buffer_create_tag(buf, tag_name, NULL);

  GtkTextIter si, ei;
  gtk_text_buffer_get_iter_at_offset(buf, &si, start);
  gtk_text_buffer_get_iter_at_offset(buf, &ei, end);
  gtk_text_buffer_apply_tag(buf, tag, &si, &ei);
}

void geditctrl_lock_range(GtkWidget *ctrl, gint start, gint end, guint lock_flags) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  if (!buf) return;

  /* Use a "locked" tag to make the range non-editable */
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, "locked");
  if (!tag)
    tag = gtk_text_buffer_create_tag(buf, "locked", "editable", FALSE, NULL);

  GtkTextIter si, ei;
  gtk_text_buffer_get_iter_at_offset(buf, &si, start);
  gtk_text_buffer_get_iter_at_offset(buf, &ei, end);
  gtk_text_buffer_apply_tag(buf, tag, &si, &ei);
}

void geditctrl_select_range(GtkWidget *ctrl, gint start, gint end) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  if (!buf) return;

  GtkTextIter si, ei;
  gtk_text_buffer_get_iter_at_offset(buf, &si, start);
  gtk_text_buffer_get_iter_at_offset(buf, &ei, end);
  gtk_text_buffer_select_range(buf, &si, &ei);
}

void geditctrl_plain_para_at(GtkWidget *ctrl, gint start, gint end) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  /* Reset quote level and formatting for the paragraph range */
  gedit_document_set_quote_level(doc, start, end - start, 0);
}

void geditctrl_set_editable(GtkWidget *ctrl, gboolean editable) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  if (!buf) return;
  /* If the entire buffer should be read-only, apply a non-editable tag
     over the whole range; otherwise remove it. */
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(buf, &start, &end);
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
  GtkTextTag *tag = gtk_text_tag_table_lookup(table, "readonly");
  if (!editable) {
    if (!tag)
      tag = gtk_text_buffer_create_tag(buf, "readonly", "editable", FALSE, NULL);
    gtk_text_buffer_apply_tag(buf, tag, &start, &end);
  } else if (tag) {
    gtk_text_buffer_remove_tag(buf, tag, &start, &end);
  }
}

void geditctrl_set_rich_text(GtkWidget *ctrl, gint offset, gboolean is_rich) {
  /* Mark whether the content from `offset` onward is rich (styled) text.
     For now this is informational — the gEditCtrl always supports styled
     text internally. Store the flag as object data for callers to query. */
  if (!ctrl) return;
  g_object_set_data(G_OBJECT(ctrl), "gedit-is-rich",
                    GINT_TO_POINTER(is_rich ? 1 : 0));
}

void gedit_document_insert_markup(geditDocument *self, gint offset,
                                   const gchar *markup) {
  if (!self || !markup) return;
  /* Simple markup parser: strips <b>, <i>, <u>, <br>, <hr/> tags and
     inserts the plain text. A full implementation would create style runs
     for bold/italic/underline ranges. */
  GString *plain = g_string_new(NULL);
  const gchar *p = markup;
  while (*p) {
    if (*p == '<') {
      /* Skip tag */
      const gchar *end = strchr(p, '>');
      if (end) {
        /* Check for <br> or <br/> — insert newline */
        if (g_ascii_strncasecmp(p, "<br", 3) == 0)
          g_string_append_c(plain, '\n');
        else if (g_ascii_strncasecmp(p, "<hr", 3) == 0)
          g_string_append(plain, "\n---\n");
        p = end + 1;
      } else {
        g_string_append_c(plain, *p);
        p++;
      }
    } else if (*p == '&') {
      /* Basic HTML entities */
      if (g_str_has_prefix(p, "&amp;")) { g_string_append_c(plain, '&'); p += 5; }
      else if (g_str_has_prefix(p, "&lt;")) { g_string_append_c(plain, '<'); p += 4; }
      else if (g_str_has_prefix(p, "&gt;")) { g_string_append_c(plain, '>'); p += 4; }
      else if (g_str_has_prefix(p, "&quot;")) { g_string_append_c(plain, '"'); p += 6; }
      else if (g_str_has_prefix(p, "&nbsp;")) { g_string_append_c(plain, ' '); p += 6; }
      else { g_string_append_c(plain, *p); p++; }
    } else {
      g_string_append_c(plain, *p);
      p++;
    }
  }
  if (plain->len > 0)
    gedit_document_insert_text(self, offset, plain->str);
  g_string_free(plain, TRUE);
}
