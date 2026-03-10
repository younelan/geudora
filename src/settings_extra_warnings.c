#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_extra_warnings_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Extra Warnings",
        "Confirmation dialogs for potentially destructive actions."));

    /* Deleting */
    GtkWidget *grp = group_box("Deleting");
    GtkWidget *del_unread = group_check("Try to delete unread mail", s->warn_delete_unread);
    GtkWidget *del_queued = group_check("Try to delete queued mail", s->warn_delete_queued);
    GtkWidget *del_unsent = group_check("Try to delete any unsent messages", s->warn_delete_unsent);
    group_add(grp, del_unread); group_add(grp, del_queued); group_add(grp, del_unsent);
    gtk_box_append(GTK_BOX(page), grp);

    /* Queueing & Sending */
    grp = group_box("Queueing & Sending");
    GtkWidget *q_subj = group_check("Try to queue a message with no subject", s->warn_queue_no_subject);
    GtkWidget *q_styled = group_check("Try to queue a message with styled text", s->warn_queue_styled);
    GtkWidget *q_quit = group_check("Try to quit with messages queued to be sent", s->warn_quit_queued);
    group_add(grp, q_subj); group_add(grp, q_styled); group_add(grp, q_quit);

    GtkWidget *sz_spin;
    GtkWidget *sz_row = spin_row("Warn if message size exceeds", &sz_spin,
                                  0, 99999, s->warn_send_size_kb, "K");
    group_add(grp, sz_row);
    gtk_box_append(GTK_BOX(page), grp);

    /* Other */
    grp = group_box("Other");
    GtkWidget *trash = group_check("Empty the Trash mailbox", s->warn_empty_trash);
    group_add(grp, trash);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "ew-unread", del_unread);
    g_object_set_data(G_OBJECT(page), "ew-queued", del_queued);
    g_object_set_data(G_OBJECT(page), "ew-unsent", del_unsent);
    g_object_set_data(G_OBJECT(page), "ew-subj", q_subj);
    g_object_set_data(G_OBJECT(page), "ew-styled", q_styled);
    g_object_set_data(G_OBJECT(page), "ew-quit", q_quit);
    g_object_set_data(G_OBJECT(page), "ew-size", sz_spin);
    g_object_set_data(G_OBJECT(page), "ew-trash", trash);

    return page;
}
