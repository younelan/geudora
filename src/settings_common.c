/*
 * settings_common.c — shared UI helpers for settings pages
 */
#include "settings_common.h"

GtkWidget *form_row(const char *label_text, GtkWidget *widget) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(row, 4);
    gtk_widget_set_margin_end(row, 4);
    gtk_widget_set_margin_top(row, 6);
    gtk_widget_set_margin_bottom(row, 6);

    GtkWidget *lbl = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
    gtk_widget_set_size_request(lbl, 140, -1);
    gtk_widget_add_css_class(lbl, "dim-label");
    gtk_box_append(GTK_BOX(row), lbl);

    gtk_widget_set_hexpand(widget, TRUE);
    gtk_box_append(GTK_BOX(row), widget);
    return row;
}

GtkWidget *group_box(const char *title) {
    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_widget_add_css_class(frame, "boxed-list-separate");
    gtk_widget_set_margin_top(frame, 6);
    gtk_widget_set_margin_bottom(frame, 6);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_frame_set_child(GTK_FRAME(frame), vbox);

    if (title) {
        GtkWidget *header = gtk_label_new(title);
        gtk_label_set_xalign(GTK_LABEL(header), 0);
        gtk_widget_set_margin_start(header, 8);
        gtk_widget_set_margin_top(header, 8);
        gtk_widget_set_margin_bottom(header, 4);
        PangoAttrList *attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        pango_attr_list_insert(attrs, pango_attr_scale_new(0.9));
        gtk_label_set_attributes(GTK_LABEL(header), attrs);
        pango_attr_list_unref(attrs);
        gtk_widget_add_css_class(header, "dim-label");
        gtk_box_append(GTK_BOX(vbox), header);
        gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    }
    return frame;
}

void group_add(GtkWidget *frame, GtkWidget *child) {
    GtkWidget *vbox = gtk_frame_get_child(GTK_FRAME(frame));
    gtk_box_append(GTK_BOX(vbox), child);
}

GtkWidget *page_title(const char *text, const char *subtitle) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_bottom(box, 8);

    GtkWidget *lbl = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_append(GTK_BOX(box), lbl);

    if (subtitle) {
        GtkWidget *sub = gtk_label_new(subtitle);
        gtk_label_set_xalign(GTK_LABEL(sub), 0);
        gtk_widget_add_css_class(sub, "dim-label");
        gtk_box_append(GTK_BOX(box), sub);
    }
    return box;
}

GtkWidget *group_check(const char *label, gboolean active) {
    GtkWidget *cb = gtk_check_button_new_with_label(label);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(cb), active);
    gtk_widget_set_margin_start(cb, 8);
    gtk_widget_set_margin_top(cb, 4);
    gtk_widget_set_margin_bottom(cb, 4);
    return cb;
}

GtkWidget *spin_row(const char *prefix, GtkWidget **spin_out,
                    double min, double max, double val, const char *suffix) {
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(hbox, 8);
    gtk_widget_set_margin_top(hbox, 4);
    gtk_widget_set_margin_bottom(hbox, 4);
    if (prefix) gtk_box_append(GTK_BOX(hbox), gtk_label_new(prefix));
    GtkWidget *spin = gtk_spin_button_new_with_range(min, max, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), val);
    gtk_box_append(GTK_BOX(hbox), spin);
    if (suffix) gtk_box_append(GTK_BOX(hbox), gtk_label_new(suffix));
    if (spin_out) *spin_out = spin;
    return hbox;
}
