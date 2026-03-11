/*
 * gTextviewCtrl.c
 * Implementation of gEditCtrl interface using GtkTextView backend.
 * This allows swapping for the real gEditCtrl later without changing client
 * code.
 */

#include "geditctrl.h"
#include <gtk/gtk.h>

/* Internal definition of geditDocument for this backend */
struct _geditDocument {
  GtkTextBuffer *buffer;
  GtkTextView
      *view; /* Weak reference/backlink if needed, or just handle buffer */
};

/* Internal helper to get TextView from ScrolledWindow */
static GtkTextView *get_text_view(GtkWidget *ctrl) {
  GtkWidget *child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
  if (GTK_IS_TEXT_VIEW(child)) {
    return GTK_TEXT_VIEW(child);
  }
  return NULL;
}

GtkWidget *geditctrl_new(void) {
  GtkWidget *scrolled = gtk_scrolled_window_new();
  GtkWidget *view = gtk_text_view_new();

  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 10);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 10);
  gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 10);
  gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(view), 10);

  /* Define basic tags */
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
  gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC,
                             NULL);
  gtk_text_buffer_create_tag(buffer, "underline", "underline",
                             PANGO_UNDERLINE_SINGLE, NULL);

  /* Store geditDocument as object data for retrieval */
  geditDocument *doc = g_new0(geditDocument, 1);
  doc->buffer = buffer;
  doc->view = GTK_TEXT_VIEW(view);

  g_object_set_data_full(G_OBJECT(scrolled), "gedit-document", doc, g_free);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), view);

  return scrolled;
}

geditDocument *geditctrl_get_document(GtkWidget *ctrl) {
  return (geditDocument *)g_object_get_data(G_OBJECT(ctrl), "gedit-document");
}

/* Document API stubs/wrappers */
GtkTextBuffer *gedit_document_get_buffer(geditDocument *self) {
  if (!self)
    return NULL;
  return self->buffer;
}

void geditctrl_toggle_style(GtkWidget *ctrl, gboolean bold, gboolean italic,
                            gboolean underline) {
  GtkTextView *view = get_text_view(ctrl);
  if (!view)
    return;

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
  GtkTextIter start, end;

  if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
    if (bold)
      gtk_text_buffer_apply_tag_by_name(buffer, "bold", &start, &end);
    if (italic)
      gtk_text_buffer_apply_tag_by_name(buffer, "italic", &start, &end);
    if (underline)
      gtk_text_buffer_apply_tag_by_name(buffer, "underline", &start, &end);
  }
}

/* Stubs for other functions */
void geditctrl_set_color(GtkWidget *ctrl, const GdkRGBA *color) {}
void geditctrl_change_font_size(GtkWidget *ctrl, gint delta_points) {}
void geditctrl_set_font_size(GtkWidget *ctrl, gint points) {}
void geditctrl_insert_image(GtkWidget *ctrl, GdkTexture *texture, gint width,
                            gint height) {}
gboolean geditctrl_handle_key(GtkWidget *ctrl, guint keyval, guint keycode,
                              GdkModifierType state_mask) {
  return FALSE;
}
void geditctrl_set_alignment(GtkWidget *ctrl, geditAlignment align) {}
void geditctrl_toggle_bullet(GtkWidget *ctrl) {}
void geditctrl_indent(GtkWidget *ctrl, gint delta_pixels) {}
void geditctrl_set_quote_level(GtkWidget *ctrl, gint level) {}
void geditctrl_change_quote_level(GtkWidget *ctrl, gint delta) {}
void geditctrl_insert_hr(GtkWidget *ctrl) {}
void geditctrl_set_direction(GtkWidget *ctrl, geditDirection direction) {}
void geditctrl_insert_page_break(GtkWidget *ctrl) {}
void geditctrl_paste_plain(GtkWidget *ctrl) {}
gint geditctrl_find_text(GtkWidget *ctrl, const gchar *search_text,
                         gboolean case_sensitive) {
  return -1;
}
gint geditctrl_replace_text(GtkWidget *ctrl, const gchar *search_text,
                            const gchar *replace_text, gboolean replace_all,
                            gboolean case_sensitive) {
  return 0;
}
void geditctrl_print(GtkWidget *ctrl) {}

/* Document method stubs */
GType gedit_document_get_type(void) { return G_TYPE_OBJECT; } /* Dummy */
void gedit_document_insert_text(geditDocument *self, gint offset,
                                const gchar *text) {}
void gedit_document_delete_range(geditDocument *self, gint offset,
                                 gint length) {}
gchar *gedit_document_get_text(geditDocument *self) { return NULL; }
gint gedit_document_get_length(geditDocument *self) { return 0; }
