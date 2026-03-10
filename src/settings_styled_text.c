#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_styled_text_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Styled Text",
        "Control how styled (HTML) mail is sent and displayed."));

    /* Sending */
    GtkWidget *grp = group_box("Sending Mail with Styles");
    GtkWidget *both = gtk_check_button_new_with_label("Send plain & styled both");
    GtkWidget *styled = gtk_check_button_new_with_label("Send styled mail only");
    GtkWidget *plain = gtk_check_button_new_with_label("Send plain text mail only");
    GtkWidget *ask = gtk_check_button_new_with_label("Ask each time");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(styled), GTK_CHECK_BUTTON(both));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(plain), GTK_CHECK_BUTTON(both));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(ask), GTK_CHECK_BUTTON(both));
    switch (s->styled_send_mode) {
        case 1: gtk_check_button_set_active(GTK_CHECK_BUTTON(styled), TRUE); break;
        case 2: gtk_check_button_set_active(GTK_CHECK_BUTTON(plain), TRUE); break;
        case 3: gtk_check_button_set_active(GTK_CHECK_BUTTON(ask), TRUE); break;
        default: gtk_check_button_set_active(GTK_CHECK_BUTTON(both), TRUE); break;
    }
    gtk_widget_set_margin_start(both, 8); gtk_widget_set_margin_top(both, 6); gtk_widget_set_margin_bottom(both, 2);
    gtk_widget_set_margin_start(styled, 8); gtk_widget_set_margin_bottom(styled, 2);
    gtk_widget_set_margin_start(plain, 8); gtk_widget_set_margin_bottom(plain, 2);
    gtk_widget_set_margin_start(ask, 8); gtk_widget_set_margin_bottom(ask, 6);
    group_add(grp, both); group_add(grp, styled); group_add(grp, plain); group_add(grp, ask);
    gtk_box_append(GTK_BOX(page), grp);

    /* Receiving */
    grp = group_box("When Receiving Styled Mail, Pay Attention To");
    GtkWidget *cb_bold = group_check("Bold", s->styled_bold);
    GtkWidget *cb_italic = group_check("Italic", s->styled_italic);
    GtkWidget *cb_underline = group_check("Underline", s->styled_underline);
    GtkWidget *cb_color = group_check("Color", s->styled_color);
    GtkWidget *cb_size = group_check("Size", s->styled_size);
    GtkWidget *cb_font = group_check("Font", s->styled_font);
    GtkWidget *cb_margins = group_check("Margins", s->styled_margins);
    group_add(grp, cb_bold); group_add(grp, cb_italic); group_add(grp, cb_underline);
    group_add(grp, cb_color); group_add(grp, cb_size); group_add(grp, cb_font);
    group_add(grp, cb_margins);
    gtk_box_append(GTK_BOX(page), grp);

    /* Formatting Toolbar */
    grp = group_box(NULL);
    GtkWidget *fmttb = group_check("Show formatting toolbar", s->show_format_toolbar);
    group_add(grp, fmttb);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "st-both", both);
    g_object_set_data(G_OBJECT(page), "st-styled", styled);
    g_object_set_data(G_OBJECT(page), "st-plain", plain);
    g_object_set_data(G_OBJECT(page), "st-ask", ask);
    g_object_set_data(G_OBJECT(page), "st-bold", cb_bold);
    g_object_set_data(G_OBJECT(page), "st-italic", cb_italic);
    g_object_set_data(G_OBJECT(page), "st-underline", cb_underline);
    g_object_set_data(G_OBJECT(page), "st-color", cb_color);
    g_object_set_data(G_OBJECT(page), "st-size", cb_size);
    g_object_set_data(G_OBJECT(page), "st-font", cb_font);
    g_object_set_data(G_OBJECT(page), "st-margins", cb_margins);
    g_object_set_data(G_OBJECT(page), "st-fmttb", fmttb);

    return page;
}
