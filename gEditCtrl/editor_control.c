/* editor_control now produces the new gEditCtrl widget */
#include "editor_control.h"
#include "geditctrl.h"
#include "gedit-document.h"

/* helper: find substring and apply a style run */
static void
apply_style_by_substr(geditDocument *d, const char *substr, gboolean bold, gboolean italic, gboolean underline, const char *color_hex)
{
    gchar *all = gedit_document_get_text(d);
    if (!all) return;
    char *p = strstr(all, substr);
    if (!p) { g_free(all); return; }
    gint byte_index = (gint)(p - all);
    gint offset = g_utf8_strlen(all, byte_index);
    gint length = g_utf8_strlen(substr, -1);
    GdkRGBA col;
    if (color_hex && *color_hex)
        gdk_rgba_parse(&col, color_hex);
    else
        gdk_rgba_parse(&col, "#000000");
    gedit_document_add_style_run(d, offset, length, bold, italic, underline, &col, 0);
    g_free(all);
}

GtkWidget *
editor_control_new(void)
{
    GtkWidget *ctrl = geditctrl_new();
    /* populate document with demo text */
    geditDocument *doc = geditctrl_get_document(ctrl);
    GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
    g_print("editor_control_new: ctrl=%p area=%p doc=%p\n", ctrl, area, doc);
    if (doc) {
        g_print("editor_control_new: got doc=%p\n", doc);
        g_print("before insert, length=%d\n", gedit_document_get_length(doc));
        const char *demo =
            "gEdit editor — placeholder control\n\n"
            "This is <b>demo text</b> inside the editor control.\n"
            "Select some text and press the toolbar buttons to toggle styles.\n";
        
        g_print("inserting demo text of length %d\n", (int)strlen(demo));
        gedit_document_insert_text(doc, 0, demo);
        g_print("after insert, length=%d\n", gedit_document_get_length(doc));
        apply_style_by_substr(doc, "demo text", TRUE, FALSE, FALSE, NULL);
        apply_style_by_substr(doc, "placeholder control", FALSE, TRUE, FALSE, NULL);
        apply_style_by_substr(doc, "Select some text", FALSE, FALSE, TRUE, NULL);
        apply_style_by_substr(doc, "toolbar buttons", FALSE, FALSE, FALSE, "#1e90ff");
        /* ensure the drawing area redraws */
        gtk_widget_queue_draw(area);
        /* try to grab keyboard focus so typing works immediately */
        if (GTK_IS_WIDGET(area))
            gtk_widget_grab_focus(area);
    } else {
        g_print("editor_control_new: no doc found!\n");
    }
    return ctrl;
}
