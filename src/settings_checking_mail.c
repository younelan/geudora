#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_checking_mail_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Checking Mail",
        "Configure how gEudora retrieves your email."));

    /* Server */
    GtkWidget *grp = group_box("Account / Server Information");
    GtkWidget *user = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(user), s->pop_username);
    group_add(grp, form_row("User Name", user));

    GtkWidget *server = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(server), s->pop_server);
    group_add(grp, form_row("Mail Server", server));

    /* Protocol */
    GtkWidget *proto_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *pop_r = gtk_check_button_new_with_label("POP");
    GtkWidget *imap_r = gtk_check_button_new_with_label("IMAP");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(imap_r), GTK_CHECK_BUTTON(pop_r));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(s->use_pop ? pop_r : imap_r), TRUE);
    gtk_box_append(GTK_BOX(proto_box), pop_r);
    gtk_box_append(GTK_BOX(proto_box), imap_r);
    group_add(grp, form_row("Mail Protocol", proto_box));

    /* Authentication */
    GtkWidget *auth_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *pwd_r = gtk_check_button_new_with_label("Passwords");
    GtkWidget *kerb_r = gtk_check_button_new_with_label("Kerberos");
    GtkWidget *apop_r = gtk_check_button_new_with_label("APOP");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(kerb_r), GTK_CHECK_BUTTON(pwd_r));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(apop_r), GTK_CHECK_BUTTON(pwd_r));
    if (s->use_kerberos) gtk_check_button_set_active(GTK_CHECK_BUTTON(kerb_r), TRUE);
    else if (s->use_apop) gtk_check_button_set_active(GTK_CHECK_BUTTON(apop_r), TRUE);
    else gtk_check_button_set_active(GTK_CHECK_BUTTON(pwd_r), TRUE);
    gtk_box_append(GTK_BOX(auth_box), pwd_r);
    gtk_box_append(GTK_BOX(auth_box), kerb_r);
    gtk_box_append(GTK_BOX(auth_box), apop_r);
    group_add(grp, form_row("Authentication", auth_box));

    GtkWidget *overlap = group_check("Overlap commands (pipelining)", s->overlap_commands);
    group_add(grp, overlap);
    gtk_box_append(GTK_BOX(page), grp);

    /* Connection / Schedule */
    grp = group_box("Connection");
    GtkWidget *interval_spin;
    GtkWidget *interval_row = spin_row("Check for mail every", &interval_spin,
                                        1, 999, s->check_interval, "minutes");
    group_add(grp, interval_row);

    GtkWidget *battery = group_check("Don't auto-check when using battery", s->check_battery);
    group_add(grp, battery);
    GtkWidget *send_check = group_check("Send on check", s->send_on_check);
    group_add(grp, send_check);
    gtk_box_append(GTK_BOX(page), grp);

    /* Mail Management */
    grp = group_box("Mail Management");
    GtkWidget *leave_spin;
    GtkWidget *leave_row = spin_row("Leave on server for", &leave_spin,
                                     0, 999, s->leave_days, "days");
    group_add(grp, leave_row);

    GtkWidget *del_trash = group_check("Delete from server when emptied from Trash", s->delete_from_trash);
    group_add(grp, del_trash);

    GtkWidget *skip_spin;
    GtkWidget *skip_row = spin_row("Skip messages over", &skip_spin,
                                    0, 10000, s->skip_size_kb, "K");
    group_add(grp, skip_row);
    gtk_box_append(GTK_BOX(page), grp);

    /* Store refs */
    g_object_set_data(G_OBJECT(page), "cm-user", user);
    g_object_set_data(G_OBJECT(page), "cm-server", server);
    g_object_set_data(G_OBJECT(page), "cm-pop", pop_r);
    g_object_set_data(G_OBJECT(page), "cm-imap", imap_r);
    g_object_set_data(G_OBJECT(page), "cm-pwd", pwd_r);
    g_object_set_data(G_OBJECT(page), "cm-kerb", kerb_r);
    g_object_set_data(G_OBJECT(page), "cm-apop", apop_r);
    g_object_set_data(G_OBJECT(page), "cm-overlap", overlap);
    g_object_set_data(G_OBJECT(page), "cm-interval", interval_spin);
    g_object_set_data(G_OBJECT(page), "cm-battery", battery);
    g_object_set_data(G_OBJECT(page), "cm-send-check", send_check);
    g_object_set_data(G_OBJECT(page), "cm-leave", leave_spin);
    g_object_set_data(G_OBJECT(page), "cm-del-trash", del_trash);
    g_object_set_data(G_OBJECT(page), "cm-skip", skip_spin);

    return page;
}
