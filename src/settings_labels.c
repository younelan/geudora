#include "settings_pages.h"
#include "settings_common.h"

static const char *default_label_names[8] = {
    "Label 1", "Label 2", "Label 3", "Label 4",
    "Label 5", "Label 6", "Label 7", "Label 8"
};

GtkWidget *create_labels_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Labels",
        "Customize the eight message labels and their colors."));

    GtkWidget *grp = group_box("Label Names and Colors");
    GtkWidget *entries[8];
    GtkWidget *colors[8];

    for (int i = 0; i < 8; i++) {
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_margin_start(hbox, 8);
        gtk_widget_set_margin_top(hbox, 4);
        gtk_widget_set_margin_bottom(hbox, 4);

        char num[16];
        snprintf(num, sizeof(num), "Label %d", i + 1);
        GtkWidget *lbl = gtk_label_new(num);
        gtk_widget_set_size_request(lbl, 60, -1);
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
        gtk_widget_add_css_class(lbl, "dim-label");
        gtk_box_append(GTK_BOX(hbox), lbl);

        entries[i] = gtk_entry_new();
        gtk_widget_set_hexpand(entries[i], TRUE);
        const char *name = (s->label_names[i][0]) ? s->label_names[i] : default_label_names[i];
        gtk_editable_set_text(GTK_EDITABLE(entries[i]), name);
        gtk_box_append(GTK_BOX(hbox), entries[i]);

        GdkRGBA rgba = { s->label_colors[i][0], s->label_colors[i][1], s->label_colors[i][2], 1.0 };
        if (rgba.red == 0 && rgba.green == 0 && rgba.blue == 0) {
            /* Default colors matching original Eudora */
            static const double defaults[8][3] = {
                {1,0,0}, {0,0,1}, {0,0.6,0}, {0.6,0,0.6},
                {1,0.5,0}, {0,0.6,0.6}, {0.4,0.4,0.4}, {0,0,0}
            };
            rgba.red = defaults[i][0]; rgba.green = defaults[i][1]; rgba.blue = defaults[i][2];
        }
        GtkColorDialog *cd = gtk_color_dialog_new();
        colors[i] = gtk_color_dialog_button_new(cd);
        gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(colors[i]), &rgba);
        gtk_box_append(GTK_BOX(hbox), colors[i]);

        group_add(grp, hbox);

        char key_e[32], key_c[32];
        snprintf(key_e, sizeof(key_e), "lbl-entry-%d", i);
        snprintf(key_c, sizeof(key_c), "lbl-color-%d", i);
        g_object_set_data(G_OBJECT(page), key_e, entries[i]);
        g_object_set_data(G_OBJECT(page), key_c, colors[i]);
    }
    gtk_box_append(GTK_BOX(page), grp);

    return page;
}
