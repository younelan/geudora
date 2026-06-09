#pragma once

#include <gtk/gtk.h>
#include "gedit-document.h"

typedef struct {
    geditDocument *doc;
    gint caret;           /* UTF-8 char offset */
    gint sel_anchor;      /* -1 when none */
    gint sel_start;       /* char offset */
    gint sel_end;         /* char offset */
    gboolean dragging;
    gboolean caret_visible;
    gint preferred_x;     /* pixels, -1 = unset */

    /* Image resize state */
    gboolean resizing;          /* TRUE while dragging a resize handle */
    gint resize_graphic_offset; /* char offset of the graphic being resized */
    gdouble resize_start_x;    /* mouse x at drag start */
    gdouble resize_start_y;    /* mouse y at drag start */
    gint resize_orig_w;        /* original width at drag start */
    gint resize_orig_h;        /* original height at drag start */
    gint selected_graphic;     /* char offset of selected graphic, -1 if none */

    /* Editable state — FALSE means read-only (selection OK, no text changes) */
    gboolean editable;

    /* Theming colors — all default to 0 (unset), meaning white bg / black text */
    gboolean has_theme;
    GdkRGBA bg_color;          /* background */
    GdkRGBA text_color;        /* default text */
    GdkRGBA caret_color;       /* caret / cursor */
    GdkRGBA sel_bg_color;      /* selection background */
} GEditCtrlState;

/* UTF-8 helpers */
G_GNUC_INTERNAL gint gedit_char_to_byte(const gchar *text, gint char_index);
G_GNUC_INTERNAL gint gedit_byte_to_char(const gchar *text, gint byte_index);

G_GNUC_INTERNAL GEditCtrlState *gedit_state_for_area(GtkWidget *area);
G_GNUC_INTERNAL void gedit_get_active_para_range(GEditCtrlState *s, geditDocument *doc, gint *out_start, gint *out_len);

/* Prototypes implemented in other modules */
G_GNUC_INTERNAL PangoLayout *gedit_layout_for_paragraph(GtkWidget *area, const gchar *text, int width);
G_GNUC_INTERNAL void gedit_draw_cb(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);
G_GNUC_INTERNAL void gedit_doc_changed_cb(geditDocument *doc, gpointer user_data);
G_GNUC_INTERNAL void gedit_scroll_to_caret(GtkWidget *area);
G_GNUC_INTERNAL void gedit_pressed_cb(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data);
G_GNUC_INTERNAL void gedit_motion_cb(GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer user_data);
G_GNUC_INTERNAL void gedit_released_cb(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data);
G_GNUC_INTERNAL gboolean gedit_key_pressed_cb(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType mods, gpointer user_data);
