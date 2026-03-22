#include "gedit-state.h"
#include "geditctrl.h"
#include "gedit-clipboard.h"
#include "gedit-print.h"
#include "gedit-table.h"
#include <gtk/gtk.h>
#include <string.h>

/* ================================================================
 * Batch update — suppress layout recalc during multiple edits
 * ================================================================ */

void geditctrl_begin_update(GtkWidget *ctrl) {
  if (!ctrl) return;
  gint depth = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ctrl), "gedit-batch-depth"));
  g_object_set_data(G_OBJECT(ctrl), "gedit-batch-depth", GINT_TO_POINTER(depth + 1));
  /* Freeze the drawing area to suppress redraws */
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (area && depth == 0)
    gtk_widget_set_visible(area, FALSE);  /* suppress draws during batch */
}

void geditctrl_end_update(GtkWidget *ctrl) {
  if (!ctrl) return;
  gint depth = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ctrl), "gedit-batch-depth"));
  if (depth <= 0) return;
  depth--;
  g_object_set_data(G_OBJECT(ctrl), "gedit-batch-depth", GINT_TO_POINTER(depth));
  if (depth == 0) {
    /* Unfreeze and trigger full redraw */
    GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
    if (area) {
      gtk_widget_set_visible(area, TRUE);
      gtk_widget_queue_draw(area);
    }
  }
}

/* ================================================================
 * Dirty state, text access, focus — convenience wrappers
 * ================================================================ */

gboolean geditctrl_is_dirty(GtkWidget *ctrl) {
  if (!ctrl) return FALSE;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return FALSE;
  /* Use a stored flag on the document via g_object_get_data */
  return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(ctrl), "gedit-dirty"));
}

void geditctrl_set_dirty(GtkWidget *ctrl, gboolean dirty) {
  if (!ctrl) return;
  g_object_set_data(G_OBJECT(ctrl), "gedit-dirty", GINT_TO_POINTER(dirty));
}

void geditctrl_clean(GtkWidget *ctrl) {
  geditctrl_set_dirty(ctrl, FALSE);
}

gchar *geditctrl_get_text(GtkWidget *ctrl) {
  if (!ctrl) return NULL;
  geditDocument *doc = geditctrl_get_document(ctrl);
  return doc ? gedit_document_get_text(doc) : NULL;
}

gboolean geditctrl_has_selection(GtkWidget *ctrl) {
  if (!ctrl || !GTK_IS_SCROLLED_WINDOW(ctrl)) return FALSE;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  return s && s->sel_start != s->sel_end;
}

gchar *geditctrl_get_selected_text(GtkWidget *ctrl) {
  if (!ctrl || !GTK_IS_SCROLLED_WINDOW(ctrl)) return NULL;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  if (!s || s->sel_start == s->sel_end) return NULL;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return NULL;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  return gedit_document_get_text_range(doc, a, b - a);
}

void geditctrl_get_selection_bounds(GtkWidget *ctrl, gint *start, gint *end) {
  if (start) *start = 0;
  if (end) *end = 0;
  if (!ctrl || !GTK_IS_SCROLLED_WINDOW(ctrl)) return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  if (!s) return;
  if (start) *start = MIN(s->sel_start, s->sel_end);
  if (end) *end = MAX(s->sel_start, s->sel_end);
}

gchar *geditctrl_get_html(GtkWidget *ctrl) {
  if (!ctrl) return NULL;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return NULL;
  gint len = gedit_document_get_length(doc);
  return gedit_document_get_markup(doc, 0, len);
}

gchar *geditctrl_get_html_range(GtkWidget *ctrl, gint start, gint end) {
  if (!ctrl) return NULL;
  geditDocument *doc = geditctrl_get_document(ctrl);
  return doc ? gedit_document_get_markup(doc, start, end) : NULL;
}

gint geditctrl_get_length(GtkWidget *ctrl) {
  if (!ctrl) return 0;
  geditDocument *doc = geditctrl_get_document(ctrl);
  return doc ? gedit_document_get_length(doc) : 0;
}

void geditctrl_set_text(GtkWidget *ctrl, const gchar *text, gint len) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  /* Clear then insert */
  gint cur_len = gedit_document_get_length(doc);
  if (cur_len > 0) gedit_document_delete_range(doc, 0, cur_len);
  if (text) {
    /* If len specified, make null-terminated copy */
    if (len >= 0) {
      gchar *tmp = g_strndup(text, len);
      gedit_document_insert_text(doc, 0, tmp);
      g_free(tmp);
    } else {
      gedit_document_insert_text(doc, 0, text);
    }
  }
}

void geditctrl_insert_text(GtkWidget *ctrl, gint offset, const gchar *text, gint len) {
  if (!ctrl || !text) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return;
  if (len >= 0) {
    gchar *tmp = g_strndup(text, len);
    gedit_document_insert_text(doc, offset, tmp);
    g_free(tmp);
  } else {
    gedit_document_insert_text(doc, offset, text);
  }
}

void geditctrl_delete_range(GtkWidget *ctrl, gint offset, gint length) {
  if (!ctrl) return;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (doc) gedit_document_delete_range(doc, offset, length);
}

void geditctrl_focus(GtkWidget *ctrl) {
  if (ctrl && GTK_IS_WIDGET(ctrl))
    gtk_widget_grab_focus(ctrl);
}

/* Scroll event handler for mouse wheel */
static gboolean gedit_scroll_cb(GtkEventControllerScroll *controller,
                                gdouble dx, gdouble dy, gpointer user_data) {
  (void)controller; (void)dx;
  GtkWidget *scrolled = GTK_WIDGET(user_data);
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
      GTK_SCROLLED_WINDOW(scrolled));
  if (!adj) return GDK_EVENT_PROPAGATE;
  double step = 40.0; /* pixels per scroll step */
  double val = gtk_adjustment_get_value(adj);
  double upper = gtk_adjustment_get_upper(adj);
  double page = gtk_adjustment_get_page_size(adj);
  double new_val = CLAMP(val + dy * step, 0, MAX(0, upper - page));
  gtk_adjustment_set_value(adj, new_val);
  return GDK_EVENT_STOP;
}

GtkWidget *geditctrl_new(void) {
  GtkWidget *scrolled = gtk_scrolled_window_new();
  GtkWidget *area = gtk_drawing_area_new();
  gtk_widget_add_css_class(area, "gedit-view");
  gtk_widget_set_hexpand(area, TRUE);
  gtk_widget_set_vexpand(area, TRUE);
  gtk_widget_set_can_focus(area, TRUE);
  gtk_widget_set_focusable(area, TRUE);
  gtk_widget_set_can_focus(scrolled, TRUE);
  gtk_widget_set_focusable(scrolled, TRUE);

  /* Set minimum size for the drawing area */
  gtk_widget_set_size_request(area, 200, 200);

  /* Scrollbar policy: always show vertical scrollbar, auto horizontal */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  /* Do NOT propagate natural height — let the scrolled window clip and scroll */
  gtk_scrolled_window_set_propagate_natural_height(
      GTK_SCROLLED_WINDOW(scrolled), FALSE);
  gtk_scrolled_window_set_propagate_natural_width(
      GTK_SCROLLED_WINDOW(scrolled), FALSE);

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
  s->resizing = FALSE;
  s->resize_graphic_offset = -1;
  s->selected_graphic = -1;

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

  /* Mouse wheel and trackpad scroll on the drawing area */
  GtkEventController *scroll_ctrl = GTK_EVENT_CONTROLLER(
      gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL));
  gtk_event_controller_set_propagation_phase(
      scroll_ctrl, GTK_PHASE_CAPTURE);
  gtk_widget_add_controller(scrolled, scroll_ctrl);
  g_signal_connect(scroll_ctrl, "scroll", G_CALLBACK(gedit_scroll_cb), scrolled);

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
      gedit_document_add_style_run(doc, st, ed - st, -1, -1, -1, color, -1);
    g_free(t);
  } else
    gedit_document_add_style_run(doc, a, b - a, -1, -1, -1, color, -1);
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
      gedit_document_add_style_run(doc, st, ed - st, -1, -1, -1, NULL,
                                   delta_points);
    g_free(t);
  } else
    gedit_document_add_style_run(doc, a, b - a, -1, -1, -1, NULL,
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
      gedit_document_add_style_run(doc, st, ed - st, -1, -1, -1, NULL, points);
    g_free(t);
  } else
    gedit_document_add_style_run(doc, a, b - a, -1, -1, -1, NULL, points);
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
  if (ln > 0) {
    geditParaAttr pattr;
    gedit_document_get_para_attr(doc, st, ln, &pattr);
    int new_level = pattr.quote_level + delta;
    if (new_level < 0)
      new_level = 0;
    gedit_document_set_quote_level(doc, st, ln, new_level);
  }
  gtk_widget_queue_draw(area);
}

void geditctrl_insert_image(GtkWidget *ctrl, GdkPixbuf *pixbuf, gint width,
                            gint height) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl))
    return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (!GTK_IS_WIDGET(area))
    return;
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc || !pixbuf)
    return;
  /* Convert GdkPixbuf → GdkTexture for GTK4 rendering */
  GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
  if (!texture) return;
  gedit_document_insert_graphic(doc, s->caret, texture, width, height);
  g_object_unref(texture); /* insert_graphic refs it internally */
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
  } else {
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

/* --- HTML markup parser with style application --- */

/* Style span recorded during HTML parse pass */
typedef struct {
  gint start;    /* char offset in plain text */
  gint end;      /* char offset in plain text */
  gint bold;     /* 1=set, 0=unset, -1=preserve */
  gint italic;
  gint underline;
  gint font_size;    /* 0=default, >0 absolute pt size */
  GdkRGBA color;
  gboolean has_color;
  gchar *link_url;   /* non-NULL for <a href> */
  geditAlignment alignment;
  gboolean has_alignment;
  gint quote_level;
  gboolean has_quote_level;
  gboolean is_hr;
} MarkupStyleSpan;

/* Stack entry for nested formatting state */
typedef struct {
  gint bold_count;
  gint italic_count;
  gint underline_count;
  gint font_size;
  GdkRGBA color;
  gboolean has_color;
  gchar *link_url;
  geditAlignment alignment;
  gint quote_level;
} MarkupState;

/* Extract an attribute value from a tag string. Returns newly allocated string or NULL. */
static gchar *markup_get_attr(const gchar *tag, gint tag_len, const gchar *attr_name) {
  gchar *tag_copy = g_strndup(tag, tag_len);
  gchar *found = NULL;
  gchar *pos = tag_copy;
  gint attr_name_len = strlen(attr_name);

  while (*pos) {
    /* Skip whitespace */
    while (*pos && g_ascii_isspace(*pos)) pos++;
    if (!*pos) break;

    /* Check if this attribute matches */
    if (g_ascii_strncasecmp(pos, attr_name, attr_name_len) == 0 &&
        pos[attr_name_len] == '=') {
      pos += attr_name_len + 1;
      /* Skip optional quotes */
      char quote = 0;
      if (*pos == '"' || *pos == '\'') { quote = *pos; pos++; }
      const gchar *start = pos;
      if (quote) {
        while (*pos && *pos != quote) pos++;
      } else {
        while (*pos && !g_ascii_isspace(*pos) && *pos != '>') pos++;
      }
      found = g_strndup(start, pos - start);
      break;
    }
    /* Skip to next attribute */
    while (*pos && !g_ascii_isspace(*pos)) pos++;
  }
  g_free(tag_copy);
  return found;
}

/* Parse a CSS color like #rrggbb or named colors */
static gboolean markup_parse_color(const gchar *str, GdkRGBA *out) {
  if (!str || !*str) return FALSE;
  return gdk_rgba_parse(out, str);
}

/* Parse font size attribute (1-7 scale or +N/-N) */
static gint markup_parse_font_size(const gchar *str) {
  if (!str || !*str) return 0;
  gint val;
  if (*str == '+' || *str == '-')
    val = 3 + atoi(str); /* relative to base size 3 */
  else
    val = atoi(str);
  /* Map HTML 1-7 scale to point sizes */
  static const gint sizes[] = { 0, 8, 10, 12, 14, 18, 24, 36 };
  if (val < 1) val = 1;
  if (val > 7) val = 7;
  return sizes[val];
}

void gedit_document_insert_markup(geditDocument *self, gint offset,
                                   const gchar *markup) {
  if (!self || !markup) return;

  /* Pass 1: Parse HTML, extract plain text, record style spans */
  GString *plain = g_string_new(NULL);
  GArray *spans = g_array_new(FALSE, TRUE, sizeof(MarkupStyleSpan));

  /* Formatting state with nesting counts */
  gint bold_count = 0, italic_count = 0, underline_count = 0;
  gint cur_font_size = 0;
  GdkRGBA cur_color = {0, 0, 0, 1};
  gboolean has_color = FALSE;
  gchar *cur_link = NULL;
  geditAlignment cur_align = gedit_ALIGN_LEFT;
  gint cur_quote_level = 0;
  gboolean in_pre = FALSE;

  /* Track open style span starts for deferred span creation */
  gint link_start = -1;

  const gchar *p = markup;
  while (*p) {
    if (*p == '<') {
      const gchar *end = strchr(p, '>');
      if (!end) {
        g_string_append_c(plain, *p);
        p++;
        continue;
      }
      gint tag_len = end - p - 1; /* length of tag content (between < and >) */
      const gchar *tag = p + 1;   /* points past '<' */
      gboolean is_closing = (tag[0] == '/');
      const gchar *tag_name = is_closing ? tag + 1 : tag;
      gint name_len = 0;
      while (name_len < tag_len && tag_name[name_len] != ' ' &&
             tag_name[name_len] != '/' && tag_name[name_len] != '>')
        name_len++;

      /* --- Block elements that insert line breaks --- */
      if (!is_closing &&
          (g_ascii_strncasecmp(tag_name, "br", name_len) == 0 && name_len == 2)) {
        g_string_append_c(plain, '\n');
      }
      else if (!is_closing &&
               (g_ascii_strncasecmp(tag_name, "hr", name_len) == 0 && name_len == 2)) {
        /* Insert newline then record HR span */
        if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
          g_string_append_c(plain, '\n');
        gint hr_pos = plain->len;
        g_string_append_c(plain, '\n');
        MarkupStyleSpan hr_span = {0};
        hr_span.start = hr_pos;
        hr_span.end = hr_pos + 1;
        hr_span.bold = -1; hr_span.italic = -1; hr_span.underline = -1;
        hr_span.is_hr = TRUE;
        g_array_append_val(spans, hr_span);
      }
      /* <p>, <div> — paragraph break */
      else if ((g_ascii_strncasecmp(tag_name, "p", name_len) == 0 && name_len == 1) ||
               (g_ascii_strncasecmp(tag_name, "div", name_len) == 0 && name_len == 3)) {
        if (!is_closing) {
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
          /* Check for align attribute */
          gchar *align_val = markup_get_attr(tag, end - tag, "align");
          if (align_val) {
            if (g_ascii_strcasecmp(align_val, "center") == 0)
              cur_align = gedit_ALIGN_CENTER;
            else if (g_ascii_strcasecmp(align_val, "right") == 0)
              cur_align = gedit_ALIGN_RIGHT;
            else
              cur_align = gedit_ALIGN_LEFT;
            g_free(align_val);
          }
        } else {
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
          cur_align = gedit_ALIGN_LEFT;
        }
      }
      /* --- Inline style tags --- */
      else if ((g_ascii_strncasecmp(tag_name, "b", name_len) == 0 && name_len == 1) ||
               (g_ascii_strncasecmp(tag_name, "strong", name_len) == 0 && name_len == 6)) {
        if (is_closing) { if (bold_count > 0) bold_count--; }
        else bold_count++;
      }
      else if ((g_ascii_strncasecmp(tag_name, "i", name_len) == 0 && name_len == 1) ||
               (g_ascii_strncasecmp(tag_name, "em", name_len) == 0 && name_len == 2) ||
               (g_ascii_strncasecmp(tag_name, "cite", name_len) == 0 && name_len == 4) ||
               (g_ascii_strncasecmp(tag_name, "var", name_len) == 0 && name_len == 3) ||
               (g_ascii_strncasecmp(tag_name, "address", name_len) == 0 && name_len == 7)) {
        if (is_closing) { if (italic_count > 0) italic_count--; }
        else italic_count++;
      }
      else if (g_ascii_strncasecmp(tag_name, "u", name_len) == 0 && name_len == 1) {
        if (is_closing) { if (underline_count > 0) underline_count--; }
        else underline_count++;
      }
      /* Headings: bold + size */
      else if (name_len == 2 && g_ascii_tolower(tag_name[0]) == 'h' &&
               tag_name[1] >= '1' && tag_name[1] <= '6') {
        if (is_closing) {
          if (bold_count > 0) bold_count--;
          cur_font_size = 0;
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
        } else {
          bold_count++;
          gint level = tag_name[1] - '0';
          /* h1=24pt, h2=18pt, h3=14pt, h4=12pt, h5=10pt, h6=8pt */
          static const gint heading_sizes[] = { 0, 24, 18, 14, 12, 10, 8 };
          cur_font_size = heading_sizes[level];
          /* h5, h6 also italic */
          if (level >= 5) italic_count++;
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
        }
      }
      /* <font> with color/size attributes */
      else if (g_ascii_strncasecmp(tag_name, "font", name_len) == 0 && name_len == 4) {
        if (is_closing) {
          has_color = FALSE;
          cur_font_size = 0;
        } else {
          gchar *color_val = markup_get_attr(tag, end - tag, "color");
          if (color_val) {
            if (markup_parse_color(color_val, &cur_color))
              has_color = TRUE;
            g_free(color_val);
          }
          gchar *size_val = markup_get_attr(tag, end - tag, "size");
          if (size_val) {
            cur_font_size = markup_parse_font_size(size_val);
            g_free(size_val);
          }
        }
      }
      /* <a href> links */
      else if (g_ascii_strncasecmp(tag_name, "a", name_len) == 0 && name_len == 1) {
        if (is_closing) {
          /* Close link span */
          if (cur_link && link_start >= 0 && (gint)plain->len > link_start) {
            MarkupStyleSpan lspan = {0};
            lspan.start = link_start;
            lspan.end = plain->len;
            lspan.bold = -1; lspan.italic = -1;
            lspan.underline = 1;
            lspan.has_color = TRUE;
            lspan.color = (GdkRGBA){0.0, 0.0, 0.8, 1.0}; /* blue */
            lspan.link_url = g_strdup(cur_link);
            g_array_append_val(spans, lspan);
          }
          g_free(cur_link);
          cur_link = NULL;
          link_start = -1;
          if (underline_count > 0) underline_count--;
        } else {
          gchar *href = markup_get_attr(tag, end - tag, "href");
          if (href) {
            cur_link = href;
            link_start = plain->len;
            underline_count++;
          }
        }
      }
      /* <blockquote> */
      else if (g_ascii_strncasecmp(tag_name, "blockquote", name_len) == 0 && name_len == 10) {
        if (is_closing) {
          if (cur_quote_level > 0) cur_quote_level--;
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
        } else {
          cur_quote_level++;
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
        }
      }
      /* <center> */
      else if (g_ascii_strncasecmp(tag_name, "center", name_len) == 0 && name_len == 6) {
        if (is_closing) cur_align = gedit_ALIGN_LEFT;
        else cur_align = gedit_ALIGN_CENTER;
      }
      /* <pre>, <code>, <tt>, <kbd>, <samp> — we don't change font but preserve whitespace for <pre> */
      else if (g_ascii_strncasecmp(tag_name, "pre", name_len) == 0 && name_len == 3) {
        in_pre = is_closing ? FALSE : TRUE;
        if (!is_closing && plain->len > 0 && plain->str[plain->len - 1] != '\n')
          g_string_append_c(plain, '\n');
      }
      /* <ul>, <ol>, <li> — list handling */
      else if ((g_ascii_strncasecmp(tag_name, "ul", name_len) == 0 && name_len == 2) ||
               (g_ascii_strncasecmp(tag_name, "ol", name_len) == 0 && name_len == 2)) {
        if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
          g_string_append_c(plain, '\n');
      }
      else if (g_ascii_strncasecmp(tag_name, "li", name_len) == 0 && name_len == 2) {
        if (!is_closing) {
          if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
            g_string_append_c(plain, '\n');
        }
      }
      /* Skip head, title, style, script content (ignore everything until closing) */
      else if (!is_closing &&
               ((g_ascii_strncasecmp(tag_name, "style", name_len) == 0 && name_len == 5) ||
                (g_ascii_strncasecmp(tag_name, "script", name_len) == 0 && name_len == 6))) {
        /* Skip to closing tag */
        gchar *close_tag = g_strdup_printf("</%.*s>", name_len, tag_name);
        const gchar *close_pos = g_strstr_len(end + 1, -1, close_tag);
        if (!close_pos) {
          /* Try case-insensitive search */
          const gchar *scan = end + 1;
          while (*scan) {
            if (*scan == '<' && *(scan+1) == '/') {
              if (g_ascii_strncasecmp(scan + 2, tag_name, name_len) == 0) {
                close_pos = strchr(scan, '>');
                break;
              }
            }
            scan++;
          }
        }
        g_free(close_tag);
        if (close_pos) { p = close_pos + 1; continue; }
      }
      /* Handle <table>: parse the entire table block and insert text content */
      else if (!is_closing &&
               g_ascii_strncasecmp(tag_name, "table", name_len) == 0 && name_len == 5) {
        /* Find the closing </table> tag */
        const gchar *table_start = p;
        const gchar *scan_t = end + 1;
        const gchar *table_end_tag = NULL;
        int nesting = 1;
        while (*scan_t && nesting > 0) {
          if (*scan_t == '<') {
            if (g_ascii_strncasecmp(scan_t + 1, "table", 5) == 0 &&
                (scan_t[6] == '>' || scan_t[6] == ' ' || scan_t[6] == '/'))
              nesting++;
            else if (g_ascii_strncasecmp(scan_t + 1, "/table", 6) == 0) {
              nesting--;
              if (nesting == 0) { table_end_tag = scan_t; break; }
            }
          }
          scan_t++;
        }
        if (table_end_tag) {
          const gchar *after_close = strchr(table_end_tag, '>');
          if (after_close) after_close++; else after_close = table_end_tag + 8;
          int table_html_len = (int)(after_close - table_start);
          /* Parse table into GEditTable to extract text content */
          {
            /* Reuse the HTML parser from gedit-table to build the data model */
            /* We insert a text-based representation into the document */
            const gchar *tEnd = table_start + table_html_len;
            /* Minimal inline parse: find <tr>/<td>/<th> and extract text */
            if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
              g_string_append_c(plain, '\n');
            const gchar *tp = end + 1; /* past <table ...> */
            while (tp < tEnd) {
              if (*tp == '<') {
                const gchar *tge = strchr(tp, '>');
                if (!tge) break;
                int tl = (int)(tge - tp - 1);
                const gchar *tn = tp + 1;
                gboolean tclosing = (tn[0] == '/');
                const gchar *tnn = tclosing ? tn + 1 : tn;
                int tnl = 0;
                while (tnl < tl && tnn[tnl] != ' ' && tnn[tnl] != '/' && tnn[tnl] != '>') tnl++;
                if (tclosing && g_ascii_strncasecmp(tnn, "tr", tnl) == 0 && tnl == 2) {
                  if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
                    g_string_append_c(plain, '\n');
                }
                else if (!tclosing &&
                         ((g_ascii_strncasecmp(tnn, "td", tnl) == 0 && tnl == 2) ||
                          (g_ascii_strncasecmp(tnn, "th", tnl) == 0 && tnl == 2))) {
                  /* If not the first cell in the row, add a tab separator */
                  if (plain->len > 0 && plain->str[plain->len - 1] != '\n' &&
                      plain->str[plain->len - 1] != '\t')
                    g_string_append_c(plain, '\t');
                }
                tp = tge + 1;
                continue;
              }
              else if (*tp == '&') {
                if (g_str_has_prefix(tp, "&amp;"))  { g_string_append_c(plain, '&'); tp += 5; }
                else if (g_str_has_prefix(tp, "&lt;"))  { g_string_append_c(plain, '<'); tp += 4; }
                else if (g_str_has_prefix(tp, "&gt;"))  { g_string_append_c(plain, '>'); tp += 4; }
                else if (g_str_has_prefix(tp, "&nbsp;")) { g_string_append_c(plain, ' '); tp += 6; }
                else if (g_str_has_prefix(tp, "&quot;")) { g_string_append_c(plain, '"'); tp += 6; }
                else { g_string_append_c(plain, *tp); tp++; }
                continue;
              }
              else {
                g_string_append_c(plain, *tp);
                tp++;
              }
            }
            if (plain->len > 0 && plain->str[plain->len - 1] != '\n')
              g_string_append_c(plain, '\n');
          }
          p = after_close;
          continue;
        }
        /* If no closing tag found, fall through to ignore */
      }
      /* <img src="..." width="W" height="H"> — record for Pass 2 */
      else if (g_ascii_strncasecmp(tag_name, "img", name_len) == 0 && name_len == 3 && !is_closing) {
        gchar *tag_str = g_strndup(p, (end - p) + 1);
        gchar *src_val = NULL;
        int img_w = 0, img_h = 0;

        /* Parse src= */
        const gchar *src_attr = g_strstr_len(tag_str, -1, "src=");
        if (!src_attr) src_attr = g_strstr_len(tag_str, -1, "SRC=");
        if (src_attr) {
          src_attr += 4;
          char q = *src_attr;
          if (q == '"' || q == '\'') {
            src_attr++;
            const gchar *src_end = strchr(src_attr, q);
            if (src_end) src_val = g_strndup(src_attr, src_end - src_attr);
          }
        }
        const gchar *w_attr = g_strstr_len(tag_str, -1, "width=");
        if (!w_attr) w_attr = g_strstr_len(tag_str, -1, "WIDTH=");
        if (w_attr) { w_attr += 6; if (*w_attr == '"') w_attr++; img_w = atoi(w_attr); }
        const gchar *h_attr = g_strstr_len(tag_str, -1, "height=");
        if (!h_attr) h_attr = g_strstr_len(tag_str, -1, "HEIGHT=");
        if (h_attr) { h_attr += 7; if (*h_attr == '"') h_attr++; img_h = atoi(h_attr); }

        /* Insert a U+FFFC placeholder and record image info via a span */
        g_string_append(plain, "\xef\xbf\xbc");
        MarkupStyleSpan span = {0};
        span.start = plain->len - 3; /* byte offset of the placeholder */
        span.end = plain->len;
        span.bold = -1; span.italic = -1; span.underline = -1;
        span.font_size = 0;
        /* Encode image info: stash src/w/h in link_url with "img:" prefix */
        span.link_url = g_strdup_printf("img:%d:%d:%s",
                                         img_w, img_h, src_val ? src_val : "");
        g_array_append_val(spans, span);

        g_free(src_val);
        g_free(tag_str);
        p = end + 1;
        continue;
      }

      /* Skip <tr>, <td>, <th> and closing tags that appear outside a <table> block */
      /* Ignore: html, head, body, title, meta, link, tr, td, th, etc. */

      p = end + 1;
      continue;
    }
    /* HTML entities */
    else if (*p == '&') {
      if (g_str_has_prefix(p, "&amp;")) { g_string_append_c(plain, '&'); p += 5; }
      else if (g_str_has_prefix(p, "&lt;")) { g_string_append_c(plain, '<'); p += 4; }
      else if (g_str_has_prefix(p, "&gt;")) { g_string_append_c(plain, '>'); p += 4; }
      else if (g_str_has_prefix(p, "&quot;")) { g_string_append_c(plain, '"'); p += 6; }
      else if (g_str_has_prefix(p, "&nbsp;")) { g_string_append_c(plain, ' '); p += 6; }
      else if (g_str_has_prefix(p, "&apos;")) { g_string_append_c(plain, '\''); p += 6; }
      else if (g_str_has_prefix(p, "&#")) {
        /* Numeric character reference */
        const gchar *semi = strchr(p, ';');
        if (semi && semi - p < 10) {
          gunichar ch;
          if (p[2] == 'x' || p[2] == 'X')
            ch = (gunichar)strtoul(p + 3, NULL, 16);
          else
            ch = (gunichar)strtoul(p + 2, NULL, 10);
          if (ch > 0 && ch < 0x110000) {
            gchar utf8[6];
            gint len = g_unichar_to_utf8(ch, utf8);
            g_string_append_len(plain, utf8, len);
          }
          p = semi + 1;
        } else {
          g_string_append_c(plain, *p); p++;
        }
      }
      else { g_string_append_c(plain, *p); p++; }
      continue;
    }
    /* Regular text character */
    else {
      /* Collapse whitespace outside <pre> */
      if (!in_pre && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        /* Collapse runs of whitespace to single space */
        if (plain->len == 0 || plain->str[plain->len - 1] != ' ')
          g_string_append_c(plain, ' ');
        p++;
        while (*p && !in_pre && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
          p++;
        continue;
      }
      /* Record style state for this character */
      gint char_start = plain->len;
      g_string_append_c(plain, *p);
      p++;

      /* Create style span for styled characters */
      if (bold_count > 0 || italic_count > 0 || underline_count > 0 ||
          has_color || cur_font_size > 0) {
        /* Try to extend previous span if same style */
        gboolean extended = FALSE;
        if (spans->len > 0) {
          MarkupStyleSpan *prev = &g_array_index(spans, MarkupStyleSpan, spans->len - 1);
          if (prev->end == char_start && !prev->is_hr && !prev->link_url &&
              !prev->has_alignment && !prev->has_quote_level &&
              prev->bold == (bold_count > 0 ? 1 : -1) &&
              prev->italic == (italic_count > 0 ? 1 : -1) &&
              prev->underline == (underline_count > 0 ? 1 : -1) &&
              prev->font_size == cur_font_size &&
              prev->has_color == has_color) {
            prev->end = char_start + 1;
            extended = TRUE;
          }
        }
        if (!extended) {
          MarkupStyleSpan span = {0};
          span.start = char_start;
          span.end = char_start + 1;
          span.bold = bold_count > 0 ? 1 : -1;
          span.italic = italic_count > 0 ? 1 : -1;
          span.underline = underline_count > 0 ? 1 : -1;
          span.font_size = cur_font_size;
          span.has_color = has_color;
          if (has_color) span.color = cur_color;
          g_array_append_val(spans, span);
        }
      }

      /* Record alignment/quote for paragraph-level styling */
      if (cur_align != gedit_ALIGN_LEFT || cur_quote_level > 0) {
        /* These are applied per-paragraph in pass 2 */
        if (spans->len > 0) {
          MarkupStyleSpan *last = &g_array_index(spans, MarkupStyleSpan, spans->len - 1);
          if (last->end == char_start + 1 && !last->has_alignment && !last->has_quote_level) {
            if (cur_align != gedit_ALIGN_LEFT) {
              last->has_alignment = TRUE;
              last->alignment = cur_align;
            }
            if (cur_quote_level > 0) {
              last->has_quote_level = TRUE;
              last->quote_level = cur_quote_level;
            }
          }
        }
      }
      continue;
    }
  }

  g_free(cur_link);

  /* Pass 2: Insert plain text, then apply style spans */
  if (plain->len > 0) {
    gedit_document_insert_text(self, offset, plain->str);

    /* Apply recorded style spans */
    for (guint i = 0; i < spans->len; i++) {
      MarkupStyleSpan *sp = &g_array_index(spans, MarkupStyleSpan, i);
      gint sp_off = offset + sp->start;
      gint sp_len = sp->end - sp->start;

      if (sp->is_hr) {
        gedit_document_insert_hr(self, sp_off);
        continue;
      }

      /* Inline styles */
      if (sp->bold != -1 || sp->italic != -1 || sp->underline != -1 ||
          sp->has_color || sp->font_size > 0) {
        gedit_document_add_style_run(self, sp_off, sp_len,
                                      sp->bold, sp->italic, sp->underline,
                                      sp->has_color ? &sp->color : NULL,
                                      sp->font_size > 0 ? sp->font_size : -1);
      }

      /* Links or images (images encoded as "img:W:H:src") */
      if (sp->link_url) {
        if (g_str_has_prefix(sp->link_url, "img:")) {
          /* Parse "img:W:H:src" */
          int iw = 0, ih = 0;
          const char *ip = sp->link_url + 4;
          iw = (int)strtol(ip, (char **)&ip, 10);
          if (*ip == ':') ip++;
          ih = (int)strtol(ip, (char **)&ip, 10);
          if (*ip == ':') ip++;
          const char *isrc = ip;

          /* Delete the U+FFFC placeholder char at sp_off */
          gedit_document_delete_range(self, sp_off, 1);
          /* Adjust subsequent span offsets */
          for (guint j = i + 1; j < spans->len; j++) {
            MarkupStyleSpan *nsp = &g_array_index(spans, MarkupStyleSpan, j);
            if (nsp->start > sp->start) { nsp->start--; nsp->end--; }
          }

          /* Load texture from path */
          GdkTexture *tex = NULL;
          if (isrc[0]) {
            GFile *file = g_file_new_for_path(isrc);
            if (!g_file_query_exists(file, NULL)) {
              g_object_unref(file);
              file = g_file_new_for_uri(isrc);
            }
            if (g_file_query_exists(file, NULL)) {
              GError *err = NULL;
              tex = gdk_texture_new_from_file(file, &err);
              if (err) g_error_free(err);
            }
            g_object_unref(file);
          }

          if (tex) {
            if (iw <= 0) iw = gdk_texture_get_width(tex);
            if (ih <= 0) ih = gdk_texture_get_height(tex);
            gedit_document_insert_graphic(self, sp_off, tex, iw, ih);
            /* Store src path */
            GList *all_runs = gedit_document_get_style_runs(self);
            for (GList *rl = all_runs; rl; rl = rl->next) {
              geditStyleRun *r = (geditStyleRun *)rl->data;
              if (r->is_graphic && r->graphic && r->offset == sp_off) {
                r->graphic->src = g_strdup(isrc);
                break;
              }
            }
            g_list_free(all_runs);
            g_object_unref(tex);
          }
        } else {
          gedit_document_set_link(self, sp_off, sp_len, sp->link_url);
        }
        g_free(sp->link_url);
      }

      /* Paragraph-level styles */
      if (sp->has_alignment)
        gedit_document_set_alignment(self, sp_off, sp_len, sp->alignment);
      if (sp->has_quote_level)
        gedit_document_set_quote_level(self, sp_off, sp_len, sp->quote_level);
    }
  }

  g_string_free(plain, TRUE);
  g_array_free(spans, TRUE);
}

/* Set font family on the current selection */
void geditctrl_set_font_family(GtkWidget *ctrl, const gchar *family) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl)) return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc) return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (b > a)
    gedit_document_set_font_family(doc, a, b - a, family);
  gtk_widget_queue_draw(area);
}

/* Clear all formatting on the current selection */
void geditctrl_clear_style(GtkWidget *ctrl) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl)) return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc) return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (b <= a) return;
  GdkRGBA black = {0, 0, 0, 1};
  gedit_document_add_style_run(doc, a, b - a, 0, 0, 0, &black, 0);
  gedit_document_set_link(doc, a, b - a, NULL);
  gedit_document_set_font_family(doc, a, b - a, NULL);
  gtk_widget_queue_draw(area);
}

/* Set a hyperlink on the current selection */
void geditctrl_set_link(GtkWidget *ctrl, const gchar *url) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl)) return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc) return;
  gint a = MIN(s->sel_start, s->sel_end);
  gint b = MAX(s->sel_start, s->sel_end);
  if (b > a)
    gedit_document_set_link(doc, a, b - a, url);
  gtk_widget_queue_draw(area);
}

/* Insert link text at caret (or replace selection) and apply link style */
void geditctrl_insert_link(GtkWidget *ctrl, const gchar *url, const gchar *text) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl)) return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc || !url || !url[0]) return;

  /* If there's a selection, delete it first */
  gint insert_at = s->caret;
  if (s->sel_start != s->sel_end) {
    gint a = MIN(s->sel_start, s->sel_end);
    gint b = MAX(s->sel_start, s->sel_end);
    gedit_document_delete_range(doc, a, b - a);
    insert_at = a;
  }

  /* Insert the display text */
  const gchar *display = (text && text[0]) ? text : url;
  gedit_document_insert_text(doc, insert_at, display);
  gint len = (gint)g_utf8_strlen(display, -1);

  /* Apply link style (blue underline + URL) */
  gedit_document_set_link(doc, insert_at, len, url);

  /* Move caret past the inserted link */
  s->caret = insert_at + len;
  s->sel_start = s->sel_end = s->caret;
  s->sel_anchor = -1;

  gedit_scroll_to_caret(area);
  gtk_widget_queue_draw(area);
}

/* Get link URL at a character offset */
gchar *geditctrl_get_link_at(GtkWidget *ctrl, gint offset) {
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return NULL;
  return gedit_document_get_link_at(doc, offset);
}

/* Insert emoji text at the current caret position.
 * Uses gedit_document_insert_text for proper undo/state tracking,
 * then applies an "emoticon" GtkTextTag so we can identify/revert.
 *
 * Builds a single insert string (with optional boundary spaces) to
 * keep undo/style-run state consistent — one snapshot, one adjustment. */
void geditctrl_insert_emoji(GtkWidget *ctrl, const gchar *emoji) {
  if (!GTK_IS_SCROLLED_WINDOW(ctrl) || !emoji || !emoji[0]) return;
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc) return;

  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  if (!buf) return;

  /* Delete selection if any */
  gint insert_at = s->caret;
  if (s->sel_start != s->sel_end) {
    gint a = MIN(s->sel_start, s->sel_end);
    gint b = MAX(s->sel_start, s->sel_end);
    gedit_document_delete_range(doc, a, b - a);
    insert_at = a;
  }

  /* Clamp insert_at to valid range */
  gint doc_len = gedit_document_get_length(doc);
  if (insert_at < 0) insert_at = 0;
  if (insert_at > doc_len) insert_at = doc_len;

  /* Determine whether we need boundary spaces */
  gboolean space_before = FALSE, space_after = FALSE;
  gchar *full_text = gedit_document_get_text(doc);
  if (full_text) {
    gint full_char_len = (gint)g_utf8_strlen(full_text, -1);
    if (insert_at > 0 && insert_at <= full_char_len) {
      const gchar *p = g_utf8_offset_to_pointer(full_text, insert_at - 1);
      gunichar pc = g_utf8_get_char(p);
      if (g_unichar_isalnum(pc) || pc == '_')
        space_before = TRUE;
    }
    if (insert_at < full_char_len) {
      const gchar *p = g_utf8_offset_to_pointer(full_text, insert_at);
      gunichar nc = g_utf8_get_char(p);
      if (g_unichar_isalnum(nc) || nc == '_')
        space_after = TRUE;
    }
    g_free(full_text);
  }

  /* Build combined string: [space] + emoji + [space] */
  GString *combined = g_string_new(NULL);
  if (space_before) g_string_append_c(combined, ' ');
  gint emoji_start_chars = space_before ? 1 : 0;
  g_string_append(combined, emoji);
  gint emoji_char_len = (gint)g_utf8_strlen(emoji, -1);
  if (space_after) g_string_append_c(combined, ' ');

  /* Single insert — one undo snapshot, one style-run adjustment */
  gedit_document_insert_text(doc, insert_at, combined->str);
  gint total_chars = (gint)g_utf8_strlen(combined->str, -1);
  g_string_free(combined, TRUE);

  /* Apply "emoticon" tag only on the emoji portion (not spaces) */
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
  GtkTextTag *emo_tag = gtk_text_tag_table_lookup(table, "emoticon");
  if (!emo_tag)
    emo_tag = gtk_text_buffer_create_tag(buf, "emoticon", NULL);
  GtkTextIter tag_start, tag_end;
  gtk_text_buffer_get_iter_at_offset(buf, &tag_start,
                                      insert_at + emoji_start_chars);
  gtk_text_buffer_get_iter_at_offset(buf, &tag_end,
                                      insert_at + emoji_start_chars + emoji_char_len);
  gtk_text_buffer_apply_tag(buf, emo_tag, &tag_start, &tag_end);

  /* Update caret and selection — place cursor after everything */
  s->caret = insert_at + total_chars;
  s->sel_start = s->sel_end = s->caret;
  s->sel_anchor = -1;

  gedit_scroll_to_caret(area);
  gtk_widget_queue_draw(area);
}

void geditctrl_set_theme_colors(GtkWidget *ctrl,
                                const GdkRGBA *bg, const GdkRGBA *text,
                                const GdkRGBA *caret, const GdkRGBA *sel_bg) {
  if (!ctrl) return;
  /* State is stored on both the scrolled window and the drawing area */
  GEditCtrlState *s = g_object_get_data(G_OBJECT(ctrl), "gedit-state");
  if (!s) return;
  s->has_theme = TRUE;
  if (bg) s->bg_color = *bg;
  if (text) s->text_color = *text;
  if (caret) s->caret_color = *caret;
  if (sel_bg) s->sel_bg_color = *sel_bg;
  /* Redraw if already realized */
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (area && GTK_IS_DRAWING_AREA(area))
    gtk_widget_queue_draw(area);
}

/* ================================================================
 * get_markup — serialize document to HTML
 * Reverse of insert_markup: walks text + style runs, emits HTML tags.
 * ================================================================ */

gchar *gedit_document_get_markup(geditDocument *self, gint start, gint end) {
  if (!self) return g_strdup("");

  gchar *full_text = gedit_document_get_text(self);
  if (!full_text) return g_strdup("");

  gint doc_len = gedit_document_get_length(self);
  if (start < 0) start = 0;
  if (end < 0 || end > doc_len) end = doc_len;
  if (start >= end) { g_free(full_text); return g_strdup(""); }

  GList *runs = gedit_document_get_style_runs(self);
  GString *html = g_string_new(NULL);

  /* Build a sorted array of run boundaries within [start, end) for O(1) lookup.
   * Between runs, text has default style (no bold/italic/etc). */

  /* Pre-compute UTF-8 byte offset for start */
  const gchar *text_at_start = g_utf8_offset_to_pointer(full_text, start);

  /* State tracking for open tags */
  gboolean cur_bold = FALSE, cur_italic = FALSE, cur_underline = FALSE;
  gboolean cur_has_color = FALSE;
  gint cur_font_size = 0;
  gchar *cur_font_family = NULL;
  gchar *cur_link = NULL;

  /* Walk by runs: for each position, find the covering run (if any).
   * Use run list iterator to avoid O(n) scan per position. */
  GList *run_iter = runs;
  const gchar *cp = text_at_start;
  gint pos = start;

  while (pos < end) {
    /* Advance run_iter to the first run that covers or is past pos */
    while (run_iter) {
      geditStyleRun *r = (geditStyleRun *)run_iter->data;
      if (r->offset + r->length > pos) break;
      run_iter = run_iter->next;
    }

    /* Check if current run covers pos */
    geditStyleRun *cur_run = NULL;
    gint run_end = end;
    if (run_iter) {
      geditStyleRun *r = (geditStyleRun *)run_iter->data;
      if (r->offset <= pos) {
        cur_run = r;
        run_end = r->offset + r->length;
        if (run_end > end) run_end = end;
      } else {
        /* Gap before next run — unstyled text until run starts */
        run_end = r->offset;
        if (run_end > end) run_end = end;
      }
    }

    /* Close tags for style changes */
    gboolean rb = cur_run ? cur_run->bold : FALSE;
    gboolean ri = cur_run ? cur_run->italic : FALSE;
    gboolean ru = cur_run ? cur_run->underline : FALSE;
    gint rfs = cur_run ? cur_run->font_size : 0;
    const gchar *rff = cur_run ? cur_run->font_family : NULL;
    const gchar *rlu = cur_run ? cur_run->link_url : NULL;
    gboolean r_has_color = FALSE;
    if (cur_run && (cur_run->color.red > 0.01 || cur_run->color.green > 0.01 ||
                    cur_run->color.blue > 0.01))
      r_has_color = TRUE;

    if (cur_link && (!rlu || strcmp(cur_link, rlu) != 0)) {
      g_string_append(html, "</a>"); g_free(cur_link); cur_link = NULL;
    }
    if (cur_underline && !ru) { g_string_append(html, "</u>"); cur_underline = FALSE; }
    if (cur_italic && !ri) { g_string_append(html, "</i>"); cur_italic = FALSE; }
    if (cur_bold && !rb) { g_string_append(html, "</b>"); cur_bold = FALSE; }
    if (cur_font_size && cur_font_size != rfs) {
      g_string_append(html, "</span>"); cur_font_size = 0;
    }
    if (cur_font_family && (!rff || strcmp(cur_font_family, rff) != 0)) {
      g_string_append(html, "</span>"); g_free(cur_font_family); cur_font_family = NULL;
    }
    if (cur_has_color && !r_has_color) {
      g_string_append(html, "</span>"); cur_has_color = FALSE;
    }

    /* Open new tags */
    if (rff && rff[0] && !cur_font_family) {
      g_string_append_printf(html, "<span style=\"font-family:%s\">", rff);
      cur_font_family = g_strdup(rff);
    }
    if (rfs > 0 && rfs != cur_font_size) {
      if (cur_font_size) g_string_append(html, "</span>");
      g_string_append_printf(html, "<span style=\"font-size:%dpt\">", rfs);
      cur_font_size = rfs;
    }
    if (r_has_color && !cur_has_color) {
      g_string_append_printf(html, "<span style=\"color:#%02x%02x%02x\">",
        (int)(cur_run->color.red * 255), (int)(cur_run->color.green * 255),
        (int)(cur_run->color.blue * 255));
      cur_has_color = TRUE;
    }
    if (rb && !cur_bold) { g_string_append(html, "<b>"); cur_bold = TRUE; }
    if (ri && !cur_italic) { g_string_append(html, "<i>"); cur_italic = TRUE; }
    if (ru && !cur_underline) { g_string_append(html, "<u>"); cur_underline = TRUE; }
    if (rlu && !cur_link) {
      g_string_append_printf(html, "<a href=\"%s\">", rlu);
      cur_link = g_strdup(rlu);
    }

    /* Emit content for this span */
    if (cur_run && cur_run->is_graphic && cur_run->graphic) {
      const char *src = cur_run->graphic->src ? cur_run->graphic->src : "";
      g_string_append_printf(html, "<img src=\"%s\" width=\"%d\" height=\"%d\">",
                             src, cur_run->graphic->width, cur_run->graphic->height);
      /* Skip the U+FFFC char */
      cp = g_utf8_next_char(cp);
    } else {
      /* Output text with HTML escaping — walk UTF-8 chars */
      for (gint ci = pos; ci < run_end; ci++) {
        gunichar uc = g_utf8_get_char(cp);
        if (uc == '\n') g_string_append(html, "<br>\n");
        else if (uc == '<') g_string_append(html, "&lt;");
        else if (uc == '>') g_string_append(html, "&gt;");
        else if (uc == '&') g_string_append(html, "&amp;");
        else if (uc == '"') g_string_append(html, "&quot;");
        else if (uc == 0xFFFC) { /* skip */ }
        else {
          gchar buf[6];
          gint blen = g_unichar_to_utf8(uc, buf);
          g_string_append_len(html, buf, blen);
        }
        cp = g_utf8_next_char(cp);
      }
    }

    pos = run_end;
  }

  /* Close remaining open tags */
  if (cur_link) { g_string_append(html, "</a>"); g_free(cur_link); }
  if (cur_underline) g_string_append(html, "</u>");
  if (cur_italic) g_string_append(html, "</i>");
  if (cur_bold) g_string_append(html, "</b>");
  if (cur_font_size) g_string_append(html, "</span>");
  if (cur_font_family) { g_string_append(html, "</span>"); g_free(cur_font_family); }
  if (cur_has_color) g_string_append(html, "</span>");

  g_list_free(runs);
  g_free(full_text);
  return g_string_free(html, FALSE);
}
