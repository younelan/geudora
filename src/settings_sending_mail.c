#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_sending_mail_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Sending Mail",
        "Configure how gEudora sends your email."));

    /* Server */
    GtkWidget *grp = group_box("Server");
    GtkWidget *email = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(email), "you@example.com");
    gtk_editable_set_text(GTK_EDITABLE(email), s->email_address);
    group_add(grp, form_row("Email Address", email));

    GtkWidget *domain = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(domain), "example.com");
    gtk_editable_set_text(GTK_EDITABLE(domain), s->default_domain);
    group_add(grp, form_row("Default Domain", domain));

    GtkWidget *smtp = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(smtp), "smtp.example.com");
    gtk_editable_set_text(GTK_EDITABLE(smtp), s->smtp_server);
    group_add(grp, form_row("SMTP Server", smtp));

    GtkWidget *subport = group_check("Use submission port (587)", s->use_submission_port);
    group_add(grp, subport);
    GtkWidget *smtpauth = group_check("Allow authorization", s->allow_smtp_auth);
    group_add(grp, smtpauth);
    gtk_box_append(GTK_BOX(page), grp);

    /* Connection */
    grp = group_box("Connection");
    GtkWidget *immediate = group_check("Immediate send", s->send_immediately);
    group_add(grp, immediate);
    GtkWidget *sendcheck = group_check("Send on check", s->send_on_check);
    group_add(grp, sendcheck);
    gtk_box_append(GTK_BOX(page), grp);

    /* Message */
    grp = group_box("Message");
    GtkWidget *curly = group_check("Fix curly quotes", s->fix_curly_quotes);
    group_add(grp, curly);
    GtkWidget *keep = group_check("Keep copies of outgoing mail", s->keep_sent_copy);
    group_add(grp, keep);
    GtkWidget *autofcc = group_check("Automatically Fcc to original mailbox", s->auto_fcc_original);
    group_add(grp, autofcc);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "sm-email", email);
    g_object_set_data(G_OBJECT(page), "sm-domain", domain);
    g_object_set_data(G_OBJECT(page), "sm-smtp", smtp);
    g_object_set_data(G_OBJECT(page), "sm-subport", subport);
    g_object_set_data(G_OBJECT(page), "sm-smtpauth", smtpauth);
    g_object_set_data(G_OBJECT(page), "sm-immediate", immediate);
    g_object_set_data(G_OBJECT(page), "sm-sendcheck", sendcheck);
    g_object_set_data(G_OBJECT(page), "sm-curly", curly);
    g_object_set_data(G_OBJECT(page), "sm-keep", keep);
    g_object_set_data(G_OBJECT(page), "sm-autofcc", autofcc);

    return page;
}
