#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_replying_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Replying",
        "Control reply behavior and address handling."));

    /* Reply to All */
    GtkWidget *grp = group_box("Reply to All");
    GtkWidget *all_def = group_check("Reply to All by default", s->reply_to_all_default);
    group_add(grp, all_def);
    gtk_box_append(GTK_BOX(page), grp);

    /* Address Handling */
    grp = group_box("Address Handling for Reply to All");
    GtkWidget *inc_self = group_check("Include yourself", s->reply_include_self);
    GtkWidget *to_cc = group_check("Put original To: recipients in Cc: field", s->reply_to_in_cc);
    group_add(grp, inc_self); group_add(grp, to_cc);
    gtk_box_append(GTK_BOX(page), grp);

    /* Miscellaneous */
    grp = group_box("Miscellaneous");
    GtkWidget *copy_pri = group_check("Copy original's priority to reply", s->copy_original_priority);
    group_add(grp, copy_pri);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "rep-all", all_def);
    g_object_set_data(G_OBJECT(page), "rep-self", inc_self);
    g_object_set_data(G_OBJECT(page), "rep-cc", to_cc);
    g_object_set_data(G_OBJECT(page), "rep-pri", copy_pri);

    return page;
}
