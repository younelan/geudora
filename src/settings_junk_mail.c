#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_junk_mail_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Junk Mail",
        "Control junk mail filtering and handling."));

    /* Threshold */
    GtkWidget *grp = group_box("Junk Threshold");
    GtkWidget *thresh_spin;
    GtkWidget *thresh_row = spin_row("Consider mail junk if score is at least", &thresh_spin,
                                      0, 100, s->junk_threshold > 0 ? s->junk_threshold : 50, NULL);
    group_add(grp, thresh_row);
    gtk_box_append(GTK_BOX(page), grp);

    /* Address Books */
    grp = group_box("Junk & Address Books");
    GtkWidget *check_ab = group_check("Mail isn't junk if sender is in an address book", s->junk_check_addressbook);
    group_add(grp, check_ab);
    gtk_box_append(GTK_BOX(page), grp);

    /* Junk Mailbox */
    grp = group_box("Junk Mailbox");
    GtkWidget *hold = group_check("Hold junk in Junk mailbox", s->junk_hold_mailbox);
    GtkWidget *unread = group_check("Junk mailbox is never marked unread", s->junk_never_unread);
    group_add(grp, hold); group_add(grp, unread);

    GtkWidget *remove_spin;
    GtkWidget *remove_row = spin_row("Remove mail that is at least", &remove_spin,
                                      0, 999, s->junk_remove_days, "days old");
    group_add(grp, remove_row);

    GtkWidget *warn = group_check("Warn before removing", s->junk_warn_remove);
    group_add(grp, warn);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "junk-thresh", thresh_spin);
    g_object_set_data(G_OBJECT(page), "junk-ab", check_ab);
    g_object_set_data(G_OBJECT(page), "junk-hold", hold);
    g_object_set_data(G_OBJECT(page), "junk-unread", unread);
    g_object_set_data(G_OBJECT(page), "junk-remove", remove_spin);
    g_object_set_data(G_OBJECT(page), "junk-warn", warn);

    return page;
}
