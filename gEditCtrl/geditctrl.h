#ifndef geditCTRL_H
#define geditCTRL_H

#include "gedit-document.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Create a new gEdit control widget. The returned widget is a GtkWidget
 * (a scrolled window containing the custom drawing area). The control
 * owns and displays a `geditDocument` internally.
 */
GtkWidget *geditctrl_new(void);

/* Return the internal `geditDocument` owned by `ctrl` (a scrolled window).
 * The returned pointer is owned by the control; do not unref it.
 */
geditDocument *geditctrl_get_document(GtkWidget *ctrl);
gint geditctrl_get_caret_offset(GtkWidget *ctrl);

/* Toggle style on the current selection inside the given gEditCtrl scrolled
 * widget. `ctrl` is the scrolled window returned by `geditctrl_new()` or
 * `editor_control_new()`.
 */
void geditctrl_toggle_style(GtkWidget *ctrl, gboolean bold, gboolean italic,
                            gboolean underline);

/* Color and font-size helpers */
void geditctrl_set_color(GtkWidget *ctrl, const GdkRGBA *color);
void geditctrl_change_font_size(GtkWidget *ctrl, gint delta_points);
void geditctrl_set_font_size(GtkWidget *ctrl, gint points);
void geditctrl_insert_image(GtkWidget *ctrl, GdkPixbuf *texture, gint width,
                            gint height);

/* Forward a key event (from a toplevel/window controller) to the control.
 * Returns GDK_EVENT_STOP if the event was handled. */
gboolean geditctrl_handle_key(GtkWidget *ctrl, guint keyval, guint keycode,
                              GdkModifierType state_mask);

/* Paragraph-level helpers invoked by UI (toolbar/menu). These operate on the
 * current selection or the paragraph containing the caret when no selection
 * is present. */
void geditctrl_set_alignment(GtkWidget *ctrl, geditAlignment align);
void geditctrl_toggle_bullet(GtkWidget *ctrl);
void geditctrl_indent(GtkWidget *ctrl, gint delta_pixels);
void geditctrl_set_quote_level(GtkWidget *ctrl, gint level);
void geditctrl_change_quote_level(GtkWidget *ctrl, gint delta);

/* Insert a horizontal rule at the current caret position */
void geditctrl_insert_hr(GtkWidget *ctrl);

/* Set text direction (LTR or RTL) for the current paragraph */
void geditctrl_set_direction(GtkWidget *ctrl, geditDirection direction);

/* Insert a page break at the current caret position */
void geditctrl_insert_page_break(GtkWidget *ctrl);

/* Paste as plain text (strips formatting) */
void geditctrl_paste_plain(GtkWidget *ctrl);

/* Find text in the document starting from the current caret position.
 * Returns the offset of the found text, or -1 if not found.
 * If found, selects the text and scrolls it into view. */
gint geditctrl_find_text(GtkWidget *ctrl, const gchar *search_text,
                         gboolean case_sensitive);

/* Replace text in the document starting from the current caret position.
 * Returns the number of replacements made.
 * If replace_all is TRUE, replaces all occurrences; otherwise replaces only the
 * next one. */
gint geditctrl_replace_text(GtkWidget *ctrl, const gchar *search_text,
                            const gchar *replace_text, gboolean replace_all,
                            gboolean case_sensitive);

/* Set font family on the current selection */
void geditctrl_set_font_family(GtkWidget *ctrl, const gchar *family);

/* Clear all formatting on the current selection */
void geditctrl_clear_style(GtkWidget *ctrl);

/* Hyperlink helpers */
void geditctrl_set_link(GtkWidget *ctrl, const gchar *url);
void geditctrl_insert_link(GtkWidget *ctrl, const gchar *url, const gchar *text);
gchar *geditctrl_get_link_at(GtkWidget *ctrl, gint offset);

/* Emoji insertion — inserts emoji text at caret with "emoticon" tag */
void geditctrl_insert_emoji(GtkWidget *ctrl, const gchar *emoji);

/* Print the document */
void geditctrl_print(GtkWidget *ctrl);

G_END_DECLS

/* Rich text and editable helpers */
void geditctrl_set_rich_text(GtkWidget *ctrl, gint offset, gboolean is_rich);
void geditctrl_set_editable(GtkWidget *ctrl, gboolean editable);

/* Label a range of text (applies a named tag for identification/styling) */
void geditctrl_set_label(GtkWidget *ctrl, gint start, gint end, gint label);

/* Lock a range of text (make it non-editable) */
void geditctrl_lock_range(GtkWidget *ctrl, gint start, gint end, guint lock_flags);

/* Select a range of text */
void geditctrl_select_range(GtkWidget *ctrl, gint start, gint end);

/* Reset paragraph formatting at a range */
void geditctrl_plain_para_at(GtkWidget *ctrl, gint start, gint end);

#endif /* geditCTRL_H */
