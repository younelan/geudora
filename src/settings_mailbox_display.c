#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_mailbox_display_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Mailbox Display",
        "Choose which columns appear and how messages are displayed."));

    /* Columns */
    GtkWidget *grp = group_box("Columns");
    GtkWidget *c_status = group_check("Status", s->show_col_status);
    GtkWidget *c_priority = group_check("Priority", s->show_col_priority);
    GtkWidget *c_attach = group_check("Attachments", s->show_col_attachments);
    GtkWidget *c_label = group_check("Label", s->show_col_label);
    GtkWidget *c_who = group_check("Who", s->show_col_who);
    GtkWidget *c_date = group_check("Date", s->show_col_date);
    GtkWidget *c_size = group_check("Size", s->show_col_size);
    GtkWidget *c_server = group_check("Server", s->show_col_server);
    GtkWidget *c_mood = group_check("Mood", s->show_col_mood);
    GtkWidget *c_junk = group_check("Junk", s->show_col_junk);
    group_add(grp, c_status); group_add(grp, c_priority);
    group_add(grp, c_attach); group_add(grp, c_label);
    group_add(grp, c_who); group_add(grp, c_date);
    group_add(grp, c_size); group_add(grp, c_server);
    group_add(grp, c_mood); group_add(grp, c_junk);
    gtk_box_append(GTK_BOX(page), grp);

    /* Drawing */
    grp = group_box("Drawing");
    GtkWidget *hlines = group_check("Draw horizontal separator lines", s->draw_horiz_lines);
    GtkWidget *vlines = group_check("Draw vertical separator lines", s->draw_vert_lines);
    GtkWidget *selcount = group_check("Show count of selected messages", s->show_selected_count);
    group_add(grp, hlines); group_add(grp, vlines); group_add(grp, selcount);
    gtk_box_append(GTK_BOX(page), grp);

    /* Message Preview */
    grp = group_box("Message Preview");
    GtkWidget *preview = group_check("Show message previews by default", s->show_preview);
    group_add(grp, preview);

    GtkWidget *read_spin;
    GtkWidget *read_row = spin_row("Mark read after", &read_spin,
                                    0, 999, s->mark_read_delay, "seconds");
    group_add(grp, read_row);
    GtkWidget *read_scroll = group_check("Mark read if scrolled or clicked in", s->mark_read_on_scroll);
    group_add(grp, read_scroll);
    GtkWidget *read_del = group_check("Mark read if deleted", s->mark_read_on_delete);
    group_add(grp, read_del);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "mbd-status", c_status);
    g_object_set_data(G_OBJECT(page), "mbd-priority", c_priority);
    g_object_set_data(G_OBJECT(page), "mbd-attach", c_attach);
    g_object_set_data(G_OBJECT(page), "mbd-label", c_label);
    g_object_set_data(G_OBJECT(page), "mbd-who", c_who);
    g_object_set_data(G_OBJECT(page), "mbd-date", c_date);
    g_object_set_data(G_OBJECT(page), "mbd-size", c_size);
    g_object_set_data(G_OBJECT(page), "mbd-server", c_server);
    g_object_set_data(G_OBJECT(page), "mbd-mood", c_mood);
    g_object_set_data(G_OBJECT(page), "mbd-junk", c_junk);
    g_object_set_data(G_OBJECT(page), "mbd-hlines", hlines);
    g_object_set_data(G_OBJECT(page), "mbd-vlines", vlines);
    g_object_set_data(G_OBJECT(page), "mbd-selcount", selcount);
    g_object_set_data(G_OBJECT(page), "mbd-preview", preview);
    g_object_set_data(G_OBJECT(page), "mbd-read-spin", read_spin);
    g_object_set_data(G_OBJECT(page), "mbd-read-scroll", read_scroll);
    g_object_set_data(G_OBJECT(page), "mbd-read-del", read_del);

    return page;
}
