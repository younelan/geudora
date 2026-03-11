#include "gedit-state.h"
#include "gedit-clipboard.h"
#include <pango/pangocairo.h>

G_GNUC_INTERNAL void gedit_pressed_cb(GtkGestureClick *gesture, gint n_press,
                                      gdouble x, gdouble y,
                                      gpointer user_data) {
  (void)gesture;
  (void)n_press;
  GtkWidget *area = GTK_WIDGET(user_data);
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;

  gtk_widget_grab_focus(area);

  gchar *full = gedit_document_get_text(doc);
  if (!full)
    return;

  int ix = (int)x - 6;
  if (ix < 0)
    ix = 0;
  int iy = (int)y - 6;
  if (iy < 0)
    iy = 0;

  gint cur = 0;
  double y_off = 6.0;
  int width = MAX(10, gtk_widget_get_width(area) - 12);
  gboolean found = FALSE;
  
  while (cur <= g_utf8_strlen(full, -1)) {
    const gchar *start_ptr = g_utf8_offset_to_pointer(full, cur);
    if (!start_ptr || *start_ptr == '\0')
      break;
    const gchar *p = start_ptr;
    gint para_chars = 0;
    while (*p && *p != '\n') {
      p = g_utf8_next_char(p);
      para_chars++;
    }
    gint para_bytes = (gint)(p - start_ptr);
    gchar *para_text = g_strndup(start_ptr, para_bytes);
    PangoLayout *pl = gedit_layout_for_paragraph(area, para_text, width);
    int pxw, pxh;
    pango_layout_get_pixel_size(pl, &pxw, &pxh);
    
    if ((int)y >= (int)y_off && (int)y < (int)(y_off + pxh)) {
      /* Click is within this paragraph's vertical bounds */
      int byte_index = 0, trailing = 0;
      pango_layout_xy_to_index(pl, ix * PANGO_SCALE,
                               (iy - (int)y_off) * PANGO_SCALE, &byte_index,
                               &trailing);
      gint char_index = gedit_byte_to_char(para_text, byte_index) + cur;
      
      /* If click is beyond the text width, move to end of line */
      if (ix > pxw) {
        char_index = cur + para_chars;
      }
      
      s->caret = char_index;
      s->sel_anchor = char_index;
      s->sel_start = char_index;
      s->sel_end = char_index;
      s->dragging = TRUE;
      found = TRUE;
      g_object_unref(pl);
      g_free(para_text);
      break;
    }
    g_object_unref(pl);
    g_free(para_text);
    cur += para_chars;
    if (*p == '\n')
      cur += 1;
    y_off += pxh > 0 ? pxh : 14;
  }

  /* If click was below all text, place cursor at end of document */
  if (!found) {
    gint doc_len = gedit_document_get_length(doc);
    s->caret = doc_len;
    s->sel_anchor = doc_len;
    s->sel_start = doc_len;
    s->sel_end = doc_len;
    s->dragging = TRUE;
  }

  g_signal_emit_by_name(doc, "selection-changed");
  gedit_scroll_to_caret(area);
  g_free(full);
  gtk_widget_queue_draw(area);
}

G_GNUC_INTERNAL void gedit_motion_cb(GtkEventControllerMotion *controller,
                                     gdouble x, gdouble y, gpointer user_data) {
  (void)controller;
  GtkWidget *area = GTK_WIDGET(user_data);
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc || !s->dragging)
    return;

  gchar *full = gedit_document_get_text(doc);
  if (!full)
    return;

  int ix = (int)x - 6;
  if (ix < 0)
    ix = 0;
  int iy = (int)y - 6;
  if (iy < 0)
    iy = 0;

  gint cur = 0;
  double y_off = 6.0;
  int width = MAX(10, gtk_widget_get_width(area) - 12);
  gboolean found = FALSE;
  
  while (cur <= g_utf8_strlen(full, -1)) {
    const gchar *start_ptr = g_utf8_offset_to_pointer(full, cur);
    if (!start_ptr || *start_ptr == '\0')
      break;
    const gchar *p = start_ptr;
    gint para_chars = 0;
    while (*p && *p != '\n') {
      p = g_utf8_next_char(p);
      para_chars++;
    }
    gint para_bytes = (gint)(p - start_ptr);
    gchar *para_text = g_strndup(start_ptr, para_bytes);
    PangoLayout *pl = gedit_layout_for_paragraph(area, para_text, width);
    int pxw, pxh;
    pango_layout_get_pixel_size(pl, &pxw, &pxh);
    if ((int)y >= (int)y_off && (int)y < (int)(y_off + pxh)) {
      int byte_index = 0, trailing = 0;
      pango_layout_xy_to_index(pl, ix * PANGO_SCALE,
                               (iy - (int)y_off) * PANGO_SCALE, &byte_index,
                               &trailing);
      gint char_index = gedit_byte_to_char(para_text, byte_index) + cur;
      
      /* If drag is beyond the text width, move to end of line */
      if (ix > pxw) {
        char_index = cur + para_chars;
      }
      
      s->sel_end = char_index;
      s->caret = char_index;
      found = TRUE;
      g_signal_emit_by_name(doc, "selection-changed");
      g_object_unref(pl);
      g_free(para_text);
      break;
    }
    g_object_unref(pl);
    g_free(para_text);
    cur += para_chars;
    if (*p == '\n')
      cur += 1;
    y_off += pxh > 0 ? pxh : 14;
  }

  /* If drag is below all text, extend selection to end of document */
  if (!found) {
    gint doc_len = gedit_document_get_length(doc);
    s->sel_end = doc_len;
    s->caret = doc_len;
    g_signal_emit_by_name(doc, "selection-changed");
  }

  g_free(full);
  gtk_widget_queue_draw(area);
}

G_GNUC_INTERNAL void gedit_released_cb(GtkGestureClick *gesture, gint n_press,
                                       gdouble x, gdouble y,
                                       gpointer user_data) {
  (void)gesture;
  (void)n_press;
  (void)x;
  (void)y;
  GtkWidget *area = GTK_WIDGET(user_data);
  GEditCtrlState *s = gedit_state_for_area(area);
  if (!s)
    return;
  s->dragging = FALSE;
  gtk_widget_queue_draw(area);
}

G_GNUC_INTERNAL gboolean gedit_key_pressed_cb(GtkEventControllerKey *controller,
                                              guint keyval, guint keycode,
                                              GdkModifierType mods,
                                              gpointer user_data) {
  (void)controller;
  (void)keycode;
  GtkWidget *area = GTK_WIDGET(user_data);
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return GDK_EVENT_PROPAGATE;

  /* Undo / Redo / Clipboard shortcuts */
  if ((mods & (GDK_CONTROL_MASK | GDK_META_MASK | GDK_SUPER_MASK)) &&
      !(mods & GDK_ALT_MASK)) {
    guint lower_key = gdk_keyval_to_lower(keyval);
    if (lower_key == GDK_KEY_z) {
      if (mods & GDK_SHIFT_MASK) {
        g_print("gedit: Redo shortcut triggered\n");
        gedit_document_redo(doc);
      } else {
        g_print("gedit: Undo shortcut triggered\n");
        gedit_document_undo(doc);
      }
      gtk_widget_queue_draw(area);
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_y) {
      g_print("gedit: Redo shortcut triggered\n");
      gedit_document_redo(doc);
      gtk_widget_queue_draw(area);
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_c) {
      if (s->sel_start != s->sel_end) {
        g_print("gedit: Copy shortcut triggered\n");
        gedit_clipboard_copy(area, s->sel_start, s->sel_end, doc);
      }
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_x) {
      if (s->sel_start != s->sel_end) {
        g_print("gedit: Cut shortcut triggered\n");
        gedit_clipboard_cut(area, s->sel_start, s->sel_end, doc);
        s->caret = MIN(s->sel_start, s->sel_end);
        s->sel_start = s->sel_end = s->caret;
        s->sel_anchor = -1;
      }
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_v) {
      g_print("gedit: Paste shortcut triggered\n");
      if (s->sel_start != s->sel_end) {
        gint a = MIN(s->sel_start, s->sel_end);
        gint b = MAX(s->sel_start, s->sel_end);
        gedit_document_delete_range(doc, a, b - a);
        s->caret = a;
      }
      gedit_clipboard_paste(area, s->caret, doc);
      s->sel_start = s->sel_end = s->caret;
      s->sel_anchor = -1;
      return GDK_EVENT_STOP;
    }
  }

  if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) {
    if (s->sel_start != s->sel_end) {
      gint a = MIN(s->sel_start, s->sel_end);
      gint b = MAX(s->sel_start, s->sel_end);
      gedit_document_delete_range(doc, a, b - a);
      s->caret = a;
      s->sel_start = s->sel_end = a;
      s->sel_anchor = -1;
    }
    gedit_document_insert_text(doc, s->caret, "\n");
    s->caret += 1;
    s->sel_start = s->sel_end = s->caret;
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  if (keyval == GDK_KEY_Tab) {
    if (s->sel_start != s->sel_end) {
      gint a = MIN(s->sel_start, s->sel_end);
      gint b = MAX(s->sel_start, s->sel_end);
      gedit_document_delete_range(doc, a, b - a);
      s->caret = a;
      s->sel_start = s->sel_end = a;
      s->sel_anchor = -1;
    }
    gedit_document_insert_text(doc, s->caret, "\t");
    s->caret += 1;
    s->sel_start = s->sel_end = s->caret;
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  if (keyval == GDK_KEY_BackSpace) {
    if (s->sel_start != s->sel_end) {
      gint a = MIN(s->sel_start, s->sel_end);
      gint b = MAX(s->sel_start, s->sel_end);
      gedit_document_delete_range(doc, a, b - a);
      s->caret = a;
      s->sel_start = s->sel_end = a;
      s->sel_anchor = -1;
    } else if (s->caret > 0) {
      gedit_document_delete_range(doc, s->caret - 1, 1);
      s->caret -= 1;
      s->sel_start = s->sel_end = s->caret;
      s->sel_anchor = -1;
    }
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  if (keyval == GDK_KEY_Delete) {
    if (s->sel_start != s->sel_end) {
      gint a = MIN(s->sel_start, s->sel_end);
      gint b = MAX(s->sel_start, s->sel_end);
      gedit_document_delete_range(doc, a, b - a);
      s->caret = a;
      s->sel_start = s->sel_end = a;
      s->sel_anchor = -1;
    } else
      gedit_document_delete_range(doc, s->caret, 1);
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch != 0) {
    gchar buf[7] = {0};
    g_unichar_to_utf8(ch, buf);
    if (s->sel_start != s->sel_end) {
      gint a = MIN(s->sel_start, s->sel_end);
      gint b = MAX(s->sel_start, s->sel_end);
      gedit_document_delete_range(doc, a, b - a);
      s->caret = a;
      s->sel_start = s->sel_end = a;
      s->sel_anchor = -1;
    }
    gedit_document_insert_text(doc, s->caret, buf);
    s->caret += 1;
    s->sel_start = s->sel_end = s->caret;
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  /* Enhanced arrow key handling with Ctrl/Cmd modifiers */
  if (keyval == GDK_KEY_Home || keyval == GDK_KEY_End ||
      keyval == GDK_KEY_Left || keyval == GDK_KEY_Right ||
      keyval == GDK_KEY_Up || keyval == GDK_KEY_Down) {
    gint newpos = s->caret;
    gint doc_len = gedit_document_get_length(doc);
    
    /* Ctrl+Home - go to document start */
    if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_Home) {
      newpos = 0;
      g_print("gedit: Ctrl+Home - go to document start\n");
    }
    /* Ctrl+End - go to document end */
    else if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_End) {
      newpos = doc_len;
      g_print("gedit: Ctrl+End - go to document end\n");
    }
    /* Home - go to line start */
    else if (keyval == GDK_KEY_Home) {
      newpos = gedit_document_find_line_start(doc, s->caret);
      g_print("gedit: Home - go to line start (pos %d)\n", newpos);
    }
    /* End - go to line end */
    else if (keyval == GDK_KEY_End) {
      newpos = gedit_document_find_line_end(doc, s->caret);
      g_print("gedit: End - go to line end (pos %d)\n", newpos);
    }
    /* Ctrl+Left - move by word backward */
    else if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_Left) {
      newpos = gedit_document_find_word_boundary_left(doc, s->caret);
      g_print("gedit: Ctrl+Left - word boundary left (pos %d)\n", newpos);
    }
    /* Ctrl+Right - move by word forward */
    else if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_Right) {
      newpos = gedit_document_find_word_boundary_right(doc, s->caret);
      g_print("gedit: Ctrl+Right - word boundary right (pos %d)\n", newpos);
    }
    /* Regular arrow keys - use existing logic */
    else {
      gchar *full = gedit_document_get_text(doc);
      if (!full)
        return GDK_EVENT_STOP;
      
      if (keyval == GDK_KEY_Left) {
        if (!(mods & GDK_SHIFT_MASK) && s->sel_start != s->sel_end)
          newpos = MIN(s->sel_start, s->sel_end);
        else if (newpos > 0)
          newpos = newpos - 1;
      } else if (keyval == GDK_KEY_Right) {
        if (!(mods & GDK_SHIFT_MASK) && s->sel_start != s->sel_end)
          newpos = MAX(s->sel_start, s->sel_end);
        else {
          gint len = gedit_document_get_length(doc);
          if (newpos < len)
            newpos = newpos + 1;
        }
      } else { /* Up/Down simple paragraph fallback */
        if (keyval == GDK_KEY_Up) {
          if (newpos > 0) {
            gint prev = newpos - 1;
            while (prev > 0) {
              const gchar *p = g_utf8_offset_to_pointer(full, prev - 1);
              if (*p == '\n')
                break;
              prev--;
            }
            newpos = prev > 0 ? prev : 0;
          }
        } else {
          gint len = g_utf8_strlen(full, -1);
          gint next = newpos;
          while (next < len) {
            const gchar *p = g_utf8_offset_to_pointer(full, next);
            if (*p == '\n') {
              next++;
              break;
            }
            next++;
          }
          newpos = next;
        }
      }
      g_free(full);
    }
    
    if (mods & GDK_SHIFT_MASK) {
      if (s->sel_anchor == -1)
        s->sel_anchor = s->caret;
      s->sel_start = MIN(s->sel_anchor, newpos);
      s->sel_end = MAX(s->sel_anchor, newpos);
      s->caret = newpos;
    } else {
      s->sel_anchor = -1;
      s->caret = newpos;
      s->sel_start = s->sel_end = newpos;
      s->preferred_x = -1;
    }
    g_signal_emit_by_name(doc, "selection-changed");
    gedit_scroll_to_caret(area);
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  return GDK_EVENT_PROPAGATE;
}
