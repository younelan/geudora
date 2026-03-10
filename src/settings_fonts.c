#include "settings_pages.h"
#include "settings_common.h"

static GtkWidget *font_row(const char *label, const char *current_font,
                           GtkWidget **font_out, GtkWidget **size_out, int cur_size) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkFontDialog *fd = gtk_font_dialog_new();
    GtkWidget *btn = gtk_font_dialog_button_new(fd);
    if (current_font && current_font[0]) {
        PangoFontDescription *d = pango_font_description_from_string(current_font);
        gtk_font_dialog_button_set_font_desc(GTK_FONT_DIALOG_BUTTON(btn), d);
        pango_font_description_free(d);
    }
    gtk_box_append(GTK_BOX(hbox), btn);
    GtkWidget *spin = gtk_spin_button_new_with_range(6, 72, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), cur_size > 0 ? cur_size : 12);
    gtk_box_append(GTK_BOX(hbox), spin);
    gtk_box_append(GTK_BOX(hbox), gtk_label_new("pt"));
    *font_out = btn;
    *size_out = spin;
    (void)label;
    return hbox;
}

GtkWidget *create_fonts_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Fonts & Display",
        "Choose fonts for reading, composing, and printing."));

    GtkWidget *grp, *fb, *sz;

    grp = group_box("Screen Font");
    GtkWidget *row1 = font_row("Screen", s->screen_font, &fb, &sz, s->screen_font_size);
    group_add(grp, form_row("Font / Size", row1));
    gtk_box_append(GTK_BOX(page), grp);
    g_object_set_data(G_OBJECT(page), "f-screen", fb);
    g_object_set_data(G_OBJECT(page), "f-screen-sz", sz);

    grp = group_box("Fixed-Width Font");
    row1 = font_row("Fixed", s->fixed_font, &fb, &sz, s->fixed_font_size);
    group_add(grp, form_row("Font / Size", row1));
    gtk_box_append(GTK_BOX(page), grp);
    g_object_set_data(G_OBJECT(page), "f-fixed", fb);
    g_object_set_data(G_OBJECT(page), "f-fixed-sz", sz);

    grp = group_box("Print Font");
    row1 = font_row("Print", s->print_font, &fb, &sz, s->print_font_size);
    group_add(grp, form_row("Font / Size", row1));
    gtk_box_append(GTK_BOX(page), grp);
    g_object_set_data(G_OBJECT(page), "f-print", fb);
    g_object_set_data(G_OBJECT(page), "f-print-sz", sz);

    /* Display Options */
    grp = group_box("Display Options");
    GtkWidget *gfx = group_check("Display graphics in messages", s->display_graphics);
    GtkWidget *emo = group_check("Display emoticons as pictures", s->display_emoticons);
    GtkWidget *zoom = group_check("Zoom windows when opening", s->zoom_on_open);
    group_add(grp, gfx); group_add(grp, emo); group_add(grp, zoom);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "f-gfx", gfx);
    g_object_set_data(G_OBJECT(page), "f-emo", emo);
    g_object_set_data(G_OBJECT(page), "f-zoom", zoom);

    return page;
}
