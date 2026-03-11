#include "gedit-state.h"
#include <gdk/gdkcairo.h>
#include <pango/pangocairo.h>

G_GNUC_INTERNAL PangoLayout *
gedit_layout_for_paragraph(GtkWidget *area, const gchar *text, int width) {
  PangoLayout *pl = gtk_widget_create_pango_layout(area, text ? text : "");
  pango_layout_set_width(pl, MAX(10, width) * PANGO_SCALE);
  pango_layout_set_wrap(pl, PANGO_WRAP_WORD_CHAR);
  
  /* Set up default tab stops (every 40 pixels) */
  PangoTabArray *tabs = pango_tab_array_new(10, TRUE);
  for (int i = 0; i < 10; i++) {
    pango_tab_array_set_tab(tabs, i, PANGO_TAB_LEFT, (i + 1) * 40 * PANGO_SCALE);
  }
  pango_layout_set_tabs(pl, tabs);
  pango_tab_array_free(tabs);
  
  return pl;
}

static void gedit_draw_quote_marks(cairo_t *cr, double x, double y,
                                   double height, int level) {
  if (level <= 0)
    return;

  double bar_width = 2.0;
  double bar_spacing = 4.0;
  double total_level_width = 12.0;

  cairo_save(cr);
  for (int i = 0; i < level; i++) {
    /* Standard email quote colors: Blue, Green, Maroon, Red, Purple */
    switch (i % 5) {
    case 0:
      cairo_set_source_rgb(cr, 0.0, 0.0, 0.8);
      break; /* Blue */
    case 1:
      cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
      break; /* Green */
    case 2:
      cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
      break; /* Maroon */
    case 3:
      cairo_set_source_rgb(cr, 0.8, 0.0, 0.0);
      break; /* Red */
    case 4:
      cairo_set_source_rgb(cr, 0.5, 0.0, 0.5);
      break; /* Purple */
    }

    double bx = x + (i * total_level_width) + bar_spacing;
    cairo_rectangle(cr, bx, y, bar_width, height);
    cairo_fill(cr);
  }
  cairo_restore(cr);
}

G_GNUC_INTERNAL void gedit_draw_cb(GtkDrawingArea *area, cairo_t *cr, int width,
                                   int height, gpointer user_data) {
  (void)height;
  geditDocument *doc = gedit_DOCUMENT(user_data);
  if (!doc)
    return;

  GEditCtrlState *s = gedit_state_for_area(GTK_WIDGET(area));

  cairo_save(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);

  gchar *full = gedit_document_get_text(doc);
  if (!full) {
    cairo_restore(cr);
    return;
  }

  double x_base = 6.0;
  double y = 6.0;
  int avail = MAX(10, width - 12);

  gint total = g_utf8_strlen(full, -1);
  gint cur = 0;
  while (cur <= total) {
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

    /* Check for HR at this position (including empty paragraphs) */
    geditParaAttr pattr;
    gedit_document_get_para_attr(doc, cur, MAX(1, para_chars), &pattr);

    /* Draw horizontal rule if this paragraph is marked as HR */
    if (pattr.is_hr) {
      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
      cairo_set_line_width(cr, 1.0);
      cairo_move_to(cr, x_base + 6, y + 7);
      cairo_line_to(cr, x_base + width - 6, y + 7);
      cairo_stroke(cr);
      y += 14.0;
      g_free(para_text);
      cur += para_chars;
      if (*p == '\n')
        cur += 1;
      continue;
    }

    /* Draw page break if this paragraph is marked as page break */
    if (pattr.page_break) {
      /* Draw a dashed line to indicate page break */
      cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
      cairo_set_line_width(cr, 2.0);
      double dashes[] = {5.0, 5.0};
      cairo_set_dash(cr, dashes, 2, 0);
      cairo_move_to(cr, x_base + 6, y + 10);
      cairo_line_to(cr, x_base + width - 6, y + 10);
      cairo_stroke(cr);
      cairo_set_dash(cr, NULL, 0, 0); /* Reset dash */
      
      /* Draw "Page Break" text */
      cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
      cairo_move_to(cr, x_base + 10, y + 5);
      PangoLayout *pb_layout = gtk_widget_create_pango_layout(GTK_WIDGET(area), "Page Break");
      pango_cairo_show_layout(cr, pb_layout);
      g_object_unref(pb_layout);
      
      y += 24.0;
      g_free(para_text);
      cur += para_chars;
      if (*p == '\n')
        cur += 1;
      continue;
    }

    int width_for_layout = avail;
    if (pattr.quote_level > 0)
      width_for_layout -= (pattr.quote_level * 12);
    if (pattr.bullet)
      width_for_layout = MAX(10, width_for_layout - 12);

    PangoLayout *pl = gedit_layout_for_paragraph(GTK_WIDGET(area), para_text,
                                                 width_for_layout);

    if (pattr.alignment == gedit_ALIGN_CENTER)
      pango_layout_set_alignment(pl, PANGO_ALIGN_CENTER);
    else if (pattr.alignment == gedit_ALIGN_RIGHT)
      pango_layout_set_alignment(pl, PANGO_ALIGN_RIGHT);
    else
      pango_layout_set_alignment(pl, PANGO_ALIGN_LEFT);

    /* Set text direction for RTL support */
    if (pattr.direction == gedit_DIR_RTL) {
      PangoContext *ctx = pango_layout_get_context(pl);
      pango_context_set_base_dir(ctx, PANGO_DIRECTION_RTL);
    }

    PangoAttrList *alist =
        gedit_document_get_attr_list_for_range(doc, cur, para_chars);
    PangoAttrList *draw_attrs =
        alist ? pango_attr_list_copy(alist) : pango_attr_list_new();

    if (s && s->sel_start != s->sel_end) {
      gint sa = MIN(s->sel_start, s->sel_end);
      gint sb = MAX(s->sel_start, s->sel_end);
      gint ia = MAX(sa, cur);
      gint ib = MIN(sb, cur + para_chars);
      if (ib > ia) {
        const gchar *ls = g_utf8_offset_to_pointer(full, ia);
        const gchar *le = g_utf8_offset_to_pointer(full, ib);
        gint rel_start = (gint)(ls - start_ptr);
        gint rel_end = (gint)(le - start_ptr);
        PangoAttribute *bg = pango_attr_background_new(45000, 52000, 65000);
        bg->start_index = rel_start;
        bg->end_index = rel_end;
        pango_attr_list_insert(draw_attrs, bg);
        PangoAttribute *fg = pango_attr_foreground_new(0, 0, 0);
        fg->start_index = rel_start;
        fg->end_index = rel_end;
        pango_attr_list_insert(draw_attrs, fg);
      }
    }

    pango_layout_set_attributes(pl, draw_attrs);
    if (alist)
      pango_attr_list_unref(alist);
    pango_attr_list_unref(draw_attrs);

    int pxw, pxh;
    pango_layout_get_pixel_size(pl, &pxw, &pxh);

    /* Get the first line's alignment offset so the bullet follows the text */
    int align_offset = 0;
    int line_height = pxh;
    PangoLayoutLine *first_line = pango_layout_get_line_readonly(pl, 0);
    if (first_line) {
      PangoRectangle l_ext;
      pango_layout_line_get_pixel_extents(first_line, NULL, &l_ext);
      align_offset = l_ext.x;
      line_height = l_ext.height;
    }

    g_print("gedit: align_offset=%d para_chars=%d bullet=%d width=%d\n",
            align_offset, para_chars, pattr.bullet, width_for_layout);

    double x_para = x_base + pattr.indent;
    if (pattr.quote_level > 0) {
      gedit_draw_quote_marks(cr, x_para, y, pxh, pattr.quote_level);
      x_para += (pattr.quote_level * 12);
    }
    double x_draw = x_para;

    if (pattr.bullet != gedit_BULLET_NONE) {
      double bx = x_para + align_offset + 4.0; /* 4px radius room roughly */
      double by = y + (line_height / 2.0);
      cairo_set_source_rgb(cr, 0, 0, 0);
      
      switch (pattr.bullet) {
        case gedit_BULLET_CIRCLE:
          cairo_arc(cr, bx, by, 3.0, 0, 2 * G_PI);
          cairo_fill(cr);
          break;
        case gedit_BULLET_SQUARE:
          cairo_rectangle(cr, bx - 3.0, by - 3.0, 6.0, 6.0);
          cairo_fill(cr);
          break;
        case gedit_BULLET_DISK:
          cairo_arc(cr, bx, by, 4.0, 0, 2 * G_PI);
          cairo_fill(cr);
          break;
        default:
          break;
      }
      x_draw += 12.0; /* 12px padding for bullet */
    }

    cairo_save(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_translate(cr, x_draw, y);
    pango_cairo_show_layout(cr, pl);
    cairo_restore(cr);

    /* Draw graphics in this paragraph */
    GList *runs = gedit_document_get_style_runs(doc);
    for (GList *l = runs; l; l = l->next) {
      geditStyleRun *run = (geditStyleRun *)l->data;
      if (run->is_graphic && run->graphic && run->offset >= cur &&
          run->offset < cur + para_chars) {
        const gchar *gr_ptr = g_utf8_offset_to_pointer(full, run->offset);
        gint rel_bytes = (gint)(gr_ptr - start_ptr);
        PangoRectangle crect, wrect;
        pango_layout_get_cursor_pos(pl, rel_bytes, &crect, &wrect);
        double gx = x_draw + crect.x / (double)PANGO_SCALE;
        double gy = y + crect.y / (double)PANGO_SCALE;

        if (run->graphic->texture) {
          cairo_save(cr);
          cairo_translate(cr, gx, gy);
          double sw = (double)run->graphic->width /
                      gdk_texture_get_width(run->graphic->texture);
          double sh = (double)run->graphic->height /
                      gdk_texture_get_height(run->graphic->texture);
          cairo_scale(cr, sw, sh);

          /* Draw a light blue placeholder for the texture */
          cairo_set_source_rgb(cr, 0.8, 0.9, 1.0);
          cairo_rectangle(cr, 0, 0, gdk_texture_get_width(run->graphic->texture),
                          gdk_texture_get_height(run->graphic->texture));
          cairo_fill(cr);
          cairo_set_source_rgb(cr, 0.2, 0.4, 0.8);
          cairo_set_line_width(cr, 1.0);
          cairo_stroke(cr);
          cairo_restore(cr);
        } else {
          /* Placeholder: draw a gray box */
          cairo_save(cr);
          cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
          cairo_rectangle(cr, gx, gy, run->graphic->width,
                          run->graphic->height);
          cairo_fill(cr);
          cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
          cairo_set_line_width(cr, 1.0);
          cairo_stroke(cr);
          cairo_restore(cr);
        }
      }
    }
    g_list_free(runs);

    if (s && s->caret_visible) {
      gint c = s->caret;
      if (c >= cur && c <= cur + para_chars) {
        const gchar *caret_ptr = g_utf8_offset_to_pointer(full, c);
        gint rel = (gint)(caret_ptr - start_ptr);
        PangoRectangle crect, wrect;
        pango_layout_get_cursor_pos(pl, rel, &crect, &wrect);
        double cx = x_draw + crect.x / (double)PANGO_SCALE;
        double cy = y + crect.y / (double)PANGO_SCALE;
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, cx, cy);
        cairo_line_to(cr, cx, cy + crect.height / (double)PANGO_SCALE);
        cairo_stroke(cr);
      }
    }

    g_object_unref(pl);
    g_free(para_text);
    cur += para_chars;
    if (*p == '\n')
      cur += 1;
    y += pxh > 0 ? pxh : 14;
    if (cur > total)
      break;
  }

  g_free(full);
  cairo_restore(cr);
}

G_GNUC_INTERNAL void gedit_scroll_to_caret(GtkWidget *area) {
  if (!GTK_IS_DRAWING_AREA(area))
    return;

  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;

  gchar *full = gedit_document_get_text(doc);
  if (!full)
    return;

  /* Calculate caret position by rendering paragraphs */
  gint cur = 0;
  double y = 6.0;
  int width = MAX(10, gtk_widget_get_width(area) - 12);
  gint caret_y = -1;
  gint caret_height = 14;
  double max_y = 6.0;

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

    /* Check if caret is in this paragraph */
    if (s->caret >= cur && s->caret <= cur + para_chars) {
      caret_y = (gint)y;
      caret_height = pxh > 0 ? pxh : 14;
    }

    max_y = y + (pxh > 0 ? pxh : 14);

    g_object_unref(pl);
    g_free(para_text);
    cur += para_chars;
    if (*p == '\n')
      cur += 1;
    y += pxh > 0 ? pxh : 14;
  }

  g_free(full);

  /* Update content size for scrolling */
  int content_height = (int)max_y + 6;
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area), content_height);

  /* Scroll to make caret visible */
  if (caret_y >= 0) {
    /* Find the scrolled window by walking up the widget hierarchy */
    GtkWidget *parent = gtk_widget_get_parent(area);
    while (parent && !GTK_IS_SCROLLED_WINDOW(parent)) {
      parent = gtk_widget_get_parent(parent);
    }
    
    if (parent && GTK_IS_SCROLLED_WINDOW(parent)) {
      GtkAdjustment *adj =
          gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(parent));
      if (adj) {
        /* Ensure the adjustment's upper bound is set correctly */
        gtk_adjustment_set_upper(adj, content_height);
        
        double page_size = gtk_adjustment_get_page_size(adj);
        double value = gtk_adjustment_get_value(adj);
        double upper = gtk_adjustment_get_upper(adj);

        g_print("gedit: scroll_to_caret caret_y=%d caret_h=%d page_size=%.0f "
                "value=%.0f upper=%.0f content_height=%d\n",
                caret_y, caret_height, page_size, value, upper, content_height);

        /* If page_size is 0, the adjustment hasn't been realized yet.
           Try to force an update by querying the scrolled window's allocation. */
        if (page_size <= 0) {
          int sw_height = gtk_widget_get_height(parent);
          g_print("gedit: page_size is 0, scrolled window height=%d\n", sw_height);
          if (sw_height > 0) {
            page_size = sw_height;
            gtk_adjustment_set_page_size(adj, page_size);
          }
        }

        /* Check if caret is visible */
        gboolean visible = (caret_y >= value) && (caret_y + caret_height <= value + page_size);
        
        if (!visible && page_size > 0) {
          /* Caret is above visible area */
          if (caret_y < value) {
            double new_val = MAX(0, caret_y - 10);
            g_print("gedit: caret above, scrolling to %.0f\n", new_val);
            gtk_adjustment_set_value(adj, new_val);
          }
          /* Caret is below visible area */
          else if (caret_y + caret_height > value + page_size) {
            double new_val = caret_y + caret_height - page_size + 10;
            new_val = MIN(new_val, upper - page_size);
            g_print("gedit: caret below, scrolling to %.0f\n", new_val);
            gtk_adjustment_set_value(adj, new_val);
          }
        } else {
          g_print("gedit: caret already visible or page_size invalid\n");
        }
      }
    }
  }
}

G_GNUC_INTERNAL void gedit_doc_changed_cb(geditDocument *doc,
                                          gpointer user_data) {
  GtkWidget *area = GTK_WIDGET(user_data);
  if (GTK_IS_WIDGET(area)) {
    /* Calculate total content height */
    GEditCtrlState *s = gedit_state_for_area(area);
    if (s) {
      gchar *full = gedit_document_get_text(doc);
      if (full) {
        double y = 6.0;
        int width = MAX(10, gtk_widget_get_width(area) - 12);
        gint cur = 0;

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
          PangoLayout *pl =
              gedit_layout_for_paragraph(area, para_text, width);
          int pxw, pxh;
          pango_layout_get_pixel_size(pl, &pxw, &pxh);

          y += pxh > 0 ? pxh : 14;

          g_object_unref(pl);
          g_free(para_text);
          cur += para_chars;
          if (*p == '\n')
            cur += 1;
        }

        int content_height = (int)y + 6;
        gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(area),
                                            content_height);
        g_free(full);
      }
    }
    gtk_widget_queue_draw(area);
  }
}
