#include "gedit-state.h"
#include "gedit-clipboard.h"
#include <pango/pangocairo.h>
#include <math.h>

/* ---- Link popover helpers ---- */

typedef struct {
  GtkWidget *area;
  gchar *url;
  gint link_offset;
  gint link_length;
} LinkPopoverData;

static void link_popover_data_free(LinkPopoverData *d) {
  g_free(d->url);
  g_free(d);
}

static void on_link_visit(GtkWidget *btn, gpointer user_data) {
  (void)btn;
  LinkPopoverData *d = user_data;
  if (d->url && d->url[0]) {
    GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(d->area));
    gtk_show_uri(GTK_WINDOW(toplevel), d->url, GDK_CURRENT_TIME);
  }
  /* Close popover (parent of button's parent box) */
  GtkWidget *pop = gtk_widget_get_ancestor(btn, GTK_TYPE_POPOVER);
  if (pop) gtk_popover_popdown(GTK_POPOVER(pop));
}

/* Edit dialog button callbacks */
static void on_link_edit_ok(GtkWidget *btn, gpointer ud) {
  (void)btn;
  GtkWidget *dlg = GTK_WIDGET(ud);
  GtkWidget *entry = g_object_get_data(G_OBJECT(dlg), "link-entry");
  LinkPopoverData *ld = g_object_get_data(G_OBJECT(dlg), "link-data");
  const gchar *new_url = gtk_editable_get_text(GTK_EDITABLE(entry));
  GEditCtrlState *s = gedit_state_for_area(ld->area);
  geditDocument *doc = s ? s->doc : NULL;
  if (doc && new_url && new_url[0])
    gedit_document_set_link(doc, ld->link_offset, ld->link_length, new_url);
  gtk_widget_queue_draw(ld->area);
  gtk_window_destroy(GTK_WINDOW(dlg));
}

static void on_link_edit_remove(GtkWidget *btn, gpointer ud) {
  (void)btn;
  GtkWidget *dlg = GTK_WIDGET(ud);
  LinkPopoverData *ld = g_object_get_data(G_OBJECT(dlg), "link-data");
  GEditCtrlState *s = gedit_state_for_area(ld->area);
  geditDocument *doc = s ? s->doc : NULL;
  if (doc)
    gedit_document_set_link(doc, ld->link_offset, ld->link_length, NULL);
  gtk_widget_queue_draw(ld->area);
  gtk_window_destroy(GTK_WINDOW(dlg));
}

static void on_link_edit(GtkWidget *btn, gpointer user_data) {
  (void)btn;
  LinkPopoverData *d = user_data;

  /* Close the popover first */
  GtkWidget *pop = gtk_widget_get_ancestor(btn, GTK_TYPE_POPOVER);
  if (pop) gtk_popover_popdown(GTK_POPOVER(pop));

  /* Create an edit dialog */
  GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(d->area));
  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Edit Link");
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(toplevel));
  gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  gtk_window_set_child(GTK_WINDOW(dialog), box);

  GtkWidget *label = gtk_label_new("URL:");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), label);

  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), d->url ? d->url : "");
  gtk_box_append(GTK_BOX(box), entry);

  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(box), btn_box);

  GtkWidget *remove_btn = gtk_button_new_with_label("Remove Link");
  GtkWidget *ok_btn = gtk_button_new_with_label("OK");
  GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
  gtk_box_append(GTK_BOX(btn_box), remove_btn);
  gtk_box_append(GTK_BOX(btn_box), cancel_btn);
  gtk_box_append(GTK_BOX(btn_box), ok_btn);

  /* Store references for callbacks */
  g_object_set_data(G_OBJECT(dialog), "link-entry", entry);
  g_object_set_data(G_OBJECT(dialog), "link-data", d);

  g_signal_connect_swapped(cancel_btn, "clicked",
                           G_CALLBACK(gtk_window_destroy), dialog);

  g_signal_connect(ok_btn, "clicked", G_CALLBACK(on_link_edit_ok), dialog);
  g_signal_connect(remove_btn, "clicked", G_CALLBACK(on_link_edit_remove), dialog);

  gtk_window_present(GTK_WINDOW(dialog));
}

/* Find the full extent of a link at a given offset (all contiguous runs with same URL) */
static void gedit_find_link_extent(geditDocument *doc, gint offset,
                                   gint *out_start, gint *out_end, gchar **out_url) {
  GList *runs = gedit_document_get_style_runs(doc);
  gchar *url = NULL;
  gint link_start = offset, link_end = offset + 1;

  /* Find the run containing offset */
  for (GList *l = runs; l; l = l->next) {
    geditStyleRun *r = l->data;
    if (offset >= r->offset && offset < r->offset + r->length &&
        r->link_url && r->link_url[0]) {
      url = g_strdup(r->link_url);
      link_start = r->offset;
      link_end = r->offset + r->length;
      break;
    }
  }

  if (url) {
    /* Extend backward: find contiguous runs with same URL */
    gboolean extended = TRUE;
    while (extended) {
      extended = FALSE;
      for (GList *l = runs; l; l = l->next) {
        geditStyleRun *r = l->data;
        if (r->link_url && g_strcmp0(r->link_url, url) == 0 &&
            r->offset + r->length == link_start) {
          link_start = r->offset;
          extended = TRUE;
        }
      }
    }
    /* Extend forward */
    extended = TRUE;
    while (extended) {
      extended = FALSE;
      for (GList *l = runs; l; l = l->next) {
        geditStyleRun *r = l->data;
        if (r->link_url && g_strcmp0(r->link_url, url) == 0 &&
            r->offset == link_end) {
          link_end = r->offset + r->length;
          extended = TRUE;
        }
      }
    }
  }

  g_list_free(runs);
  *out_start = link_start;
  *out_end = link_end;
  *out_url = url;
}

static void gedit_show_link_popover(GtkWidget *area, gint char_offset,
                                    gdouble click_x, gdouble click_y) {
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!doc) return;

  gint link_start, link_end;
  gchar *url = NULL;
  gedit_find_link_extent(doc, char_offset, &link_start, &link_end, &url);
  if (!url) return;

  LinkPopoverData *d = g_new0(LinkPopoverData, 1);
  d->area = area;
  d->url = url; /* takes ownership */
  d->link_offset = link_start;
  d->link_length = link_end - link_start;

  GtkWidget *popover = gtk_popover_new();
  gtk_widget_set_parent(popover, area);
  GdkRectangle rect = { (int)click_x, (int)click_y, 1, 1 };
  gtk_popover_set_pointing_to(GTK_POPOVER(popover), &rect);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(box, 8);
  gtk_widget_set_margin_end(box, 8);
  gtk_widget_set_margin_top(box, 8);
  gtk_widget_set_margin_bottom(box, 8);
  gtk_popover_set_child(GTK_POPOVER(popover), box);

  /* Show truncated URL */
  gchar *display_url = g_utf8_strlen(url, -1) > 50
      ? g_strdup_printf("%.47s...", url) : g_strdup(url);
  GtkWidget *url_label = gtk_label_new(display_url);
  gtk_label_set_ellipsize(GTK_LABEL(url_label), PANGO_ELLIPSIZE_MIDDLE);
  gtk_widget_set_opacity(url_label, 0.7);
  gtk_box_append(GTK_BOX(box), url_label);
  g_free(display_url);

  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(box), btn_box);

  GtkWidget *visit_btn = gtk_button_new_with_label("Visit");
  GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
  gtk_box_append(GTK_BOX(btn_box), visit_btn);
  gtk_box_append(GTK_BOX(btn_box), edit_btn);

  g_signal_connect(visit_btn, "clicked", G_CALLBACK(on_link_visit), d);
  g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_link_edit), d);

  /* Free data when popover is destroyed */
  g_object_set_data_full(G_OBJECT(popover), "link-data", d,
                         (GDestroyNotify)link_popover_data_free);

  gtk_popover_popup(GTK_POPOVER(popover));
}

/* Helper: find graphic screen rect at a given char offset.
 * Returns TRUE if found, and fills gx/gy/gw/gh in widget coords. */
static gboolean gedit_find_graphic_rect(GtkWidget *area, geditDocument *doc,
                                        gint graphic_offset,
                                        double *out_gx, double *out_gy,
                                        int *out_gw, int *out_gh) {
  gchar *full = gedit_document_get_text(doc);
  if (!full) return FALSE;

  int awidth = MAX(10, gtk_widget_get_width(area) - 12);
  double x_base = 6.0, yy = 6.0;
  gint cur = 0;
  gboolean found = FALSE;

  while (cur <= g_utf8_strlen(full, -1)) {
    const gchar *sp = g_utf8_offset_to_pointer(full, cur);
    if (!sp || !*sp) break;
    const gchar *p = sp;
    gint pc = 0;
    while (*p && *p != '\n') { p = g_utf8_next_char(p); pc++; }
    gchar *pt = g_strndup(sp, (gsize)(p - sp));
    PangoLayout *pl = gedit_layout_for_paragraph(area, pt, awidth);
    /* Apply shape attrs so image placeholders have correct dimensions */
    PangoAttrList *alist =
        gedit_document_get_attr_list_for_range(doc, cur, pc);
    if (alist) {
      pango_layout_set_attributes(pl, alist);
      pango_attr_list_unref(alist);
    }
    int pxw, pxh;
    pango_layout_get_pixel_size(pl, &pxw, &pxh);

    if (graphic_offset >= cur && graphic_offset < cur + pc) {
      const gchar *gr_ptr = g_utf8_offset_to_pointer(full, graphic_offset);
      gint rb = (gint)(gr_ptr - sp);
      PangoRectangle crect, wrect;
      pango_layout_get_cursor_pos(pl, rb, &crect, &wrect);
      *out_gx = x_base + crect.x / (double)PANGO_SCALE;
      *out_gy = yy + crect.y / (double)PANGO_SCALE;

      /* Look up the graphic's width/height */
      GList *runs = gedit_document_get_style_runs(doc);
      for (GList *l = runs; l; l = l->next) {
        geditStyleRun *r = l->data;
        if (r->is_graphic && r->graphic && r->offset == graphic_offset) {
          *out_gw = r->graphic->width;
          *out_gh = r->graphic->height;
          found = TRUE;
          break;
        }
      }
      g_list_free(runs);
    }
    g_object_unref(pl);
    g_free(pt);
    if (found) break;
    cur += pc;
    if (*p == '\n') cur++;
    yy += pxh > 0 ? pxh : 14;
  }
  g_free(full);
  return found;
}

/* Helper: check if point (x,y) hits a graphic.
 * Returns the graphic's char offset or -1.
 * If near a resize handle, sets *on_handle = TRUE. */
static gint gedit_hit_test_graphic(GtkWidget *area, geditDocument *doc,
                                   double mx, double my,
                                   gboolean *on_handle) {
  *on_handle = FALSE;
  GList *runs = gedit_document_get_style_runs(doc);
  gint hit = -1;
  for (GList *l = runs; l; l = l->next) {
    geditStyleRun *r = l->data;
    if (!r->is_graphic || !r->graphic) continue;
    double gx, gy; int gw, gh;
    if (!gedit_find_graphic_rect(area, doc, r->offset, &gx, &gy, &gw, &gh))
      continue;

    /* Check resize handles at all 4 corners (12x12 hit zones) */
    double corners[][2] = {
      {gx - 4, gy - 4},           /* top-left */
      {gx + gw - 8, gy - 4},     /* top-right */
      {gx - 4, gy + gh - 8},     /* bottom-left */
      {gx + gw - 8, gy + gh - 8} /* bottom-right */
    };
    for (int ci = 0; ci < 4; ci++) {
      if (mx >= corners[ci][0] && mx <= corners[ci][0] + 12 &&
          my >= corners[ci][1] && my <= corners[ci][1] + 12) {
        *on_handle = TRUE;
        hit = r->offset;
        break;
      }
    }
    if (hit >= 0) break;

    /* Check if inside the image bounds */
    if (mx >= gx && mx <= gx + gw && my >= gy && my <= gy + gh) {
      hit = r->offset;
      break;
    }
  }
  g_list_free(runs);
  return hit;
}

G_GNUC_INTERNAL void gedit_pressed_cb(GtkGestureClick *gesture, gint n_press,
                                      gdouble x, gdouble y,
                                      gpointer user_data) {
  (void)gesture;
  GtkWidget *area = GTK_WIDGET(user_data);
  GEditCtrlState *s = gedit_state_for_area(area);
  geditDocument *doc = s ? s->doc : NULL;
  if (!s || !doc)
    return;

  gtk_widget_grab_focus(area);

  /* Check if clicking on a graphic or its resize handle */
  gboolean on_handle = FALSE;
  gint graphic_hit = gedit_hit_test_graphic(area, doc, x, y, &on_handle);

  if (graphic_hit >= 0 && on_handle) {
    /* Start resize drag */
    s->selected_graphic = graphic_hit;
    s->resizing = TRUE;
    s->resize_graphic_offset = graphic_hit;
    s->resize_start_x = x;
    s->resize_start_y = y;
    /* Find current size */
    GList *runs = gedit_document_get_style_runs(doc);
    for (GList *l = runs; l; l = l->next) {
      geditStyleRun *r = l->data;
      if (r->is_graphic && r->graphic && r->offset == graphic_hit) {
        s->resize_orig_w = r->graphic->width;
        s->resize_orig_h = r->graphic->height;
        break;
      }
    }
    g_list_free(runs);
    s->dragging = FALSE;
    gtk_widget_queue_draw(area);
    return;
  }

  if (graphic_hit >= 0) {
    /* Clicked on graphic body — select it */
    s->selected_graphic = graphic_hit;
    s->caret = graphic_hit;
    s->sel_anchor = graphic_hit;
    s->sel_start = graphic_hit;
    s->sel_end = graphic_hit + 1;
    s->dragging = FALSE;
    g_signal_emit_by_name(doc, "selection-changed");
    gtk_widget_queue_draw(area);
    return;
  }

  /* Clear graphic selection */
  s->selected_graphic = -1;
  s->resizing = FALSE;

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
  gint total = g_utf8_strlen(full, -1);
  const gchar *cur_ptr = full;

  while (cur <= total) {
    const gchar *start_ptr = cur_ptr;
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
    PangoAttrList *al = gedit_document_get_attr_list_for_range(doc, cur, para_chars);
    if (al) { pango_layout_set_attributes(pl, al); pango_attr_list_unref(al); }
    int pxw, pxh;
    pango_layout_get_pixel_size(pl, &pxw, &pxh);
    /* Ensure empty lines have a clickable height */
    if (pxh < 14) pxh = 14;

    if ((int)y >= (int)y_off && (int)y < (int)(y_off + pxh)) {
      /* Click is within this paragraph's vertical bounds */
      gint char_index;
      if (para_chars == 0) {
        /* Empty line — caret goes to start of this line */
        char_index = cur;
      } else {
        int byte_index = 0, trailing = 0;
        pango_layout_xy_to_index(pl, ix * PANGO_SCALE,
                                 (iy - (int)y_off) * PANGO_SCALE, &byte_index,
                                 &trailing);
        char_index = gedit_byte_to_char(para_text, byte_index) + cur;
        if (trailing > 0) char_index++;

        /* If click is past the end of text on this visual line,
         * place caret at end of that line (not at nearest char).
         * Find which visual line was clicked and get its end. */
        int click_line_y = iy - (int)y_off;
        PangoLayoutIter *li = pango_layout_get_iter(pl);
        do {
          PangoRectangle line_ext;
          pango_layout_iter_get_line_extents(li, NULL, &line_ext);
          int ly = line_ext.y / PANGO_SCALE;
          int lh = line_ext.height / PANGO_SCALE;
          if (click_line_y >= ly && click_line_y < ly + lh) {
            /* This is the clicked visual line */
            int line_x_end = (line_ext.x + line_ext.width) / PANGO_SCALE;
            if (ix > line_x_end) {
              /* Past end of line text — go to end of this visual line */
              PangoLayoutLine *pll = pango_layout_iter_get_line_readonly(li);
              if (pll) {
                gint line_end_byte = pll->start_index + pll->length;
                char_index = gedit_byte_to_char(para_text, line_end_byte) + cur;
              }
            }
            break;
          }
        } while (pango_layout_iter_next_line(li));
        pango_layout_iter_free(li);

        if (char_index > cur + para_chars) char_index = cur + para_chars;
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
    cur_ptr = p;
    if (*p == '\n') {
      cur += 1;
      cur_ptr = g_utf8_next_char(p);
    }
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

  /* Double-click: select word. Triple-click: select line. */
  if (n_press >= 2 && full) {
    gint pos = s->caret;
    gint doc_len = g_utf8_strlen(full, -1);

    if (n_press == 2) {
      /* Select word: expand left/right to word boundaries.
       * Word chars: alphanumeric + underscore. */
      gint wstart = pos, wend = pos;

      /* Walk left to find word start */
      while (wstart > 0) {
        const gchar *p = g_utf8_offset_to_pointer(full, wstart - 1);
        gunichar uc = g_utf8_get_char(p);
        if (!g_unichar_isalnum(uc) && uc != '_') break;
        wstart--;
      }
      /* Walk right to find word end */
      while (wend < doc_len) {
        const gchar *p = g_utf8_offset_to_pointer(full, wend);
        gunichar uc = g_utf8_get_char(p);
        if (!g_unichar_isalnum(uc) && uc != '_') break;
        wend++;
      }
      /* If we didn't expand at all (clicked on whitespace/punctuation),
       * select the single non-word character */
      if (wstart == wend && pos < doc_len) {
        wstart = pos;
        wend = pos + 1;
      }

      s->sel_start = wstart;
      s->sel_end = wend;
      s->sel_anchor = wstart;
      s->caret = wend;
    } else {
      /* Triple-click: select entire line (paragraph).
       * Find \n before and after caret position. */
      gint lstart = pos, lend = pos;

      while (lstart > 0) {
        const gchar *p = g_utf8_offset_to_pointer(full, lstart - 1);
        if (*p == '\n') break;
        lstart--;
      }
      while (lend < doc_len) {
        const gchar *p = g_utf8_offset_to_pointer(full, lend);
        if (*p == '\n') { lend++; break; }
        lend++;
      }

      s->sel_start = lstart;
      s->sel_end = lend;
      s->sel_anchor = lstart;
      s->caret = lend;
    }
    s->dragging = FALSE;
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
  if (!s || !doc)
    return;

  /* Handle image resize drag */
  if (s->resizing && s->resize_graphic_offset >= 0) {
    double dx = x - s->resize_start_x;
    double dy = y - s->resize_start_y;
    /* Maintain aspect ratio using the larger delta */
    double aspect = (s->resize_orig_h > 0) ?
        (double)s->resize_orig_w / s->resize_orig_h : 1.0;
    int new_w, new_h;
    if (fabs(dx) > fabs(dy)) {
      new_w = MAX(32, s->resize_orig_w + (int)dx);
      new_h = MAX(32, (int)(new_w / aspect));
    } else {
      new_h = MAX(32, s->resize_orig_h + (int)dy);
      new_w = MAX(32, (int)(new_h * aspect));
    }
    /* Update the graphic dimensions */
    GList *runs = gedit_document_get_style_runs(doc);
    for (GList *l = runs; l; l = l->next) {
      geditStyleRun *r = l->data;
      if (r->is_graphic && r->graphic && r->offset == s->resize_graphic_offset) {
        r->graphic->width = new_w;
        r->graphic->height = new_h;
        break;
      }
    }
    g_list_free(runs);
    /* Update the Pango shape attribute for the resized graphic */
    g_signal_emit_by_name(doc, "document-changed");
    gtk_widget_queue_draw(area);
    return;
  }

  if (!s->dragging)
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
  gint dtotal = g_utf8_strlen(full, -1);
  const gchar *dcur_ptr = full;

  while (cur <= dtotal) {
    const gchar *start_ptr = dcur_ptr;
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
    PangoAttrList *al2 = gedit_document_get_attr_list_for_range(doc, cur, para_chars);
    if (al2) { pango_layout_set_attributes(pl, al2); pango_attr_list_unref(al2); }
    int pxw, pxh;
    pango_layout_get_pixel_size(pl, &pxw, &pxh);
    if (pxh < 14) pxh = 14;
    if ((int)y >= (int)y_off && (int)y < (int)(y_off + pxh)) {
      gint char_index;
      if (para_chars == 0) {
        char_index = cur;
      } else {
        int byte_index = 0, trailing = 0;
        pango_layout_xy_to_index(pl, ix * PANGO_SCALE,
                                 (iy - (int)y_off) * PANGO_SCALE, &byte_index,
                                 &trailing);
        char_index = gedit_byte_to_char(para_text, byte_index) + cur;
        if (trailing > 0) char_index++;

        /* Past end of visual line → end of that line */
        int click_line_y = iy - (int)y_off;
        PangoLayoutIter *dli = pango_layout_get_iter(pl);
        do {
          PangoRectangle dle;
          pango_layout_iter_get_line_extents(dli, NULL, &dle);
          int dly = dle.y / PANGO_SCALE;
          int dlh = dle.height / PANGO_SCALE;
          if (click_line_y >= dly && click_line_y < dly + dlh) {
            int lxe = (dle.x + dle.width) / PANGO_SCALE;
            if (ix > lxe) {
              PangoLayoutLine *pll = pango_layout_iter_get_line_readonly(dli);
              if (pll)
                char_index = gedit_byte_to_char(para_text,
                    pll->start_index + pll->length) + cur;
            }
            break;
          }
        } while (pango_layout_iter_next_line(dli));
        pango_layout_iter_free(dli);

        if (char_index > cur + para_chars) char_index = cur + para_chars;
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
    dcur_ptr = p;
    if (*p == '\n') {
      cur += 1;
      dcur_ptr = g_utf8_next_char(p);
    }
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
  GtkWidget *area = GTK_WIDGET(user_data);
  GEditCtrlState *s = gedit_state_for_area(area);
  if (!s)
    return;

  gboolean was_dragging = s->dragging;
  s->dragging = FALSE;
  s->resizing = FALSE;
  s->resize_graphic_offset = -1;

  /* If this was a simple click (no selection drag), check for link */
  if (was_dragging && s->sel_start == s->sel_end && s->doc) {
    gchar *url = gedit_document_get_link_at(s->doc, s->caret);
    if (url) {
      g_free(url);
      gedit_show_link_popover(area, s->caret, x, y);
    }
  }

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

  /* Clipboard and undo shortcuts */
  if ((mods & (GDK_CONTROL_MASK | GDK_META_MASK | GDK_SUPER_MASK)) &&
      !(mods & GDK_ALT_MASK)) {
    guint lower_key = gdk_keyval_to_lower(keyval);

    /* Copy always works (even read-only) */
    if (lower_key == GDK_KEY_c) {
      if (s->sel_start != s->sel_end)
        gedit_clipboard_copy(area, s->sel_start, s->sel_end, doc);
      return GDK_EVENT_STOP;
    }
    /* Select All always works */
    if (lower_key == GDK_KEY_a) {
      gint doc_len = gedit_document_get_length(doc);
      s->sel_start = 0;
      s->sel_end = doc_len;
      s->sel_anchor = 0;
      s->caret = doc_len;
      g_signal_emit_by_name(doc, "selection-changed");
      gtk_widget_queue_draw(area);
      return GDK_EVENT_STOP;
    }

    /* Everything below modifies content — block in read-only mode */
    if (!s->editable) return GDK_EVENT_PROPAGATE;

    if (lower_key == GDK_KEY_z) {
      if (mods & GDK_SHIFT_MASK)
        gedit_document_redo(doc);
      else
        gedit_document_undo(doc);
      gtk_widget_queue_draw(area);
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_y) {
      gedit_document_redo(doc);
      gtk_widget_queue_draw(area);
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_x) {
      if (s->sel_start != s->sel_end) {
        gedit_clipboard_cut(area, s->sel_start, s->sel_end, doc);
        s->caret = MIN(s->sel_start, s->sel_end);
        s->sel_start = s->sel_end = s->caret;
        s->sel_anchor = -1;
      }
      return GDK_EVENT_STOP;
    }
    if (lower_key == GDK_KEY_v) {
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

  /* All keys below modify the document — block in read-only mode.
   * Arrow keys and navigation are handled further down and allowed. */
  if (!s->editable) {
    /* Allow arrow keys, Home, End, Page Up/Down in read-only */
    if (keyval == GDK_KEY_Left || keyval == GDK_KEY_Right ||
        keyval == GDK_KEY_Up || keyval == GDK_KEY_Down ||
        keyval == GDK_KEY_Home || keyval == GDK_KEY_End ||
        keyval == GDK_KEY_Page_Up || keyval == GDK_KEY_Page_Down)
      goto handle_navigation;
    return GDK_EVENT_PROPAGATE;
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
    gedit_scroll_to_caret(area);
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
    gedit_scroll_to_caret(area);
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
    gedit_scroll_to_caret(area);
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
    gedit_scroll_to_caret(area);
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
    gedit_scroll_to_caret(area);
    gtk_widget_queue_draw(area);
    return GDK_EVENT_STOP;
  }

  /* Navigation keys — always allowed, even in read-only mode */
handle_navigation:
  if (keyval == GDK_KEY_Home || keyval == GDK_KEY_End ||
      keyval == GDK_KEY_Left || keyval == GDK_KEY_Right ||
      keyval == GDK_KEY_Up || keyval == GDK_KEY_Down) {
    gint newpos = s->caret;
    gint doc_len = gedit_document_get_length(doc);
    
    /* Ctrl+Home - go to document start */
    if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_Home) {
      newpos = 0;
    }
    /* Ctrl+End - go to document end */
    else if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_End) {
      newpos = doc_len;
    }
    /* Home - go to line start */
    else if (keyval == GDK_KEY_Home) {
      newpos = gedit_document_find_line_start(doc, s->caret);
    }
    /* End - go to line end */
    else if (keyval == GDK_KEY_End) {
      newpos = gedit_document_find_line_end(doc, s->caret);
    }
    /* Ctrl+Left - move by word backward */
    else if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_Left) {
      newpos = gedit_document_find_word_boundary_left(doc, s->caret);
    }
    /* Ctrl+Right - move by word forward */
    else if ((mods & GDK_CONTROL_MASK) && keyval == GDK_KEY_Right) {
      newpos = gedit_document_find_word_boundary_right(doc, s->caret);
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
