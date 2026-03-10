#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_miscellaneous_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Miscellaneous",
        "Various options that don't fit elsewhere."));

    GtkWidget *grp = group_box(NULL);
    GtkWidget *close_mb = group_check("Close messages with mailbox", s->close_msg_with_mailbox);
    GtkWidget *empty = group_check("Empty Trash on Quit", s->empty_trash_on_quit);
    GtkWidget *turbo = group_check("Turbo redirect by default", s->turbo_redirect);
    GtkWidget *resort = group_check("Re-sort mailboxes less often", s->resort_less_often);
    GtkWidget *old_toc = group_check("Use old-style \".toc\" files", s->use_old_toc);
    GtkWidget *filter = group_check("Generate filter reports", s->generate_filter_reports);
    GtkWidget *keychain = group_check("Use system keychain to store passwords", s->use_keychain);

    group_add(grp, close_mb); group_add(grp, empty); group_add(grp, turbo);
    group_add(grp, resort); group_add(grp, old_toc); group_add(grp, filter);
    group_add(grp, keychain);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "misc-close", close_mb);
    g_object_set_data(G_OBJECT(page), "misc-empty", empty);
    g_object_set_data(G_OBJECT(page), "misc-turbo", turbo);
    g_object_set_data(G_OBJECT(page), "misc-resort", resort);
    g_object_set_data(G_OBJECT(page), "misc-toc", old_toc);
    g_object_set_data(G_OBJECT(page), "misc-filter", filter);
    g_object_set_data(G_OBJECT(page), "misc-keychain", keychain);

    return page;
}
