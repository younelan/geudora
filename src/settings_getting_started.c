#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_getting_started_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Getting Started",
        "Configure your basic mail settings to get up and running."));

    /* Incoming */
    GtkWidget *grp = group_box("Incoming Mail");
    GtkWidget *user = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(user), "you@example.com");
    gtk_editable_set_text(GTK_EDITABLE(user), s->pop_username);
    group_add(grp, form_row("User Name", user));

    GtkWidget *server = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(server), "mail.example.com");
    gtk_editable_set_text(GTK_EDITABLE(server), s->pop_server);
    group_add(grp, form_row("Mail Server", server));
    gtk_box_append(GTK_BOX(page), grp);

    /* Outgoing */
    grp = group_box("Outgoing Mail");
    GtkWidget *realname = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(realname), "John Doe");
    gtk_editable_set_text(GTK_EDITABLE(realname), s->real_name);
    group_add(grp, form_row("Real Name", realname));

    GtkWidget *smtp = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(smtp), "smtp.example.com");
    gtk_editable_set_text(GTK_EDITABLE(smtp), s->smtp_server);
    group_add(grp, form_row("SMTP Server", smtp));

    GtkWidget *email = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(email), "you@example.com");
    gtk_editable_set_text(GTK_EDITABLE(email), s->email_address);
    group_add(grp, form_row("Email Address", email));
    gtk_box_append(GTK_BOX(page), grp);

    /* Options */
    grp = group_box(NULL);
    GtkWidget *defmail = group_check("Make gEudora the default mail application", s->default_mailer);
    group_add(grp, defmail);
    gtk_box_append(GTK_BOX(page), grp);

    /* Store widget refs */
    g_object_set_data(G_OBJECT(page), "gs-user", user);
    g_object_set_data(G_OBJECT(page), "gs-server", server);
    g_object_set_data(G_OBJECT(page), "gs-realname", realname);
    g_object_set_data(G_OBJECT(page), "gs-smtp", smtp);
    g_object_set_data(G_OBJECT(page), "gs-email", email);
    g_object_set_data(G_OBJECT(page), "gs-defmail", defmail);

    return page;
}
