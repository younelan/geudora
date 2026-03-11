#include "gedit-state.h"
#include <string.h>

G_GNUC_INTERNAL gint
gedit_char_to_byte(const gchar *text, gint char_index)
{
    if (!text) return 0;
    const gchar *p = g_utf8_offset_to_pointer(text, char_index);
    return (gint)(p - text);
}

G_GNUC_INTERNAL gint
gedit_byte_to_char(const gchar *text, gint byte_index)
{
    if (!text) return 0;
    return g_utf8_strlen(text, byte_index);
}

G_GNUC_INTERNAL GEditCtrlState *
gedit_state_for_area(GtkWidget *area)
{
    if (!GTK_IS_WIDGET(area)) return NULL;
    GEditCtrlState *s = g_object_get_data(G_OBJECT(area), "gedit-state");
    if (s) return s;
    GtkWidget *parent = gtk_widget_get_parent(area);
    if (parent) return g_object_get_data(G_OBJECT(parent), "gedit-state");
    return NULL;
}

G_GNUC_INTERNAL void
gedit_get_active_para_range(GEditCtrlState *s, geditDocument *doc, gint *out_start, gint *out_len)
{
    gchar *t = gedit_document_get_text(doc);
    if (!t) { *out_start = 0; *out_len = 0; return; }
    gint a = MIN(s->sel_start, s->sel_end);
    gint b = MAX(s->sel_start, s->sel_end);
    if (a == b) {
        gint len = g_utf8_strlen(t, -1);
        gint pos = s->caret; if (pos < 0) pos = 0; if (pos > len) pos = len;
        gint st = pos; while (st > 0) { const gchar *ptr = g_utf8_offset_to_pointer(t, st - 1); if (*ptr == '\n') break; st--; }
        gint ed = pos; while (ed < len) { const gchar *ptr = g_utf8_offset_to_pointer(t, ed); if (*ptr == '\n') { ed++; break; } ed++; }
        if (ed < st) ed = st;
        *out_start = st; *out_len = ed - st;
    } else {
        *out_start = a; *out_len = b - a;
    }
    g_free(t);
}
