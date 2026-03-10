#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_hosts_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Hosts",
        "Server hosts and directory services."));

    /* Mail Servers (read-only view — actual editing in Checking/Sending) */
    GtkWidget *grp = group_box("Checking Mail");
    GtkWidget *mail_sv = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(mail_sv), s->pop_server);
    group_add(grp, form_row("Mail Server", mail_sv));
    gtk_box_append(GTK_BOX(page), grp);

    grp = group_box("Sending Mail");
    GtkWidget *smtp_sv = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(smtp_sv), s->smtp_server);
    group_add(grp, form_row("SMTP Server", smtp_sv));
    gtk_box_append(GTK_BOX(page), grp);

    /* Directory Services */
    grp = group_box("Directory Services");
    GtkWidget *ph = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(ph), s->ph_server);
    gtk_entry_set_placeholder_text(GTK_ENTRY(ph), "ldap.example.com");
    group_add(grp, form_row("Ph/LDAP Server", ph));

    GtkWidget *finger = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(finger), s->finger_server);
    group_add(grp, form_row("Finger Server", finger));
    gtk_box_append(GTK_BOX(page), grp);

    /* All Connections */
    grp = group_box("All Connections");
    GtkWidget *dns = group_check("DNS load balancing", s->dns_load_balance);
    GtkWidget *offline = group_check("Offline (no connections)", s->offline_mode);
    group_add(grp, dns); group_add(grp, offline);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "ho-ph", ph);
    g_object_set_data(G_OBJECT(page), "ho-finger", finger);
    g_object_set_data(G_OBJECT(page), "ho-dns", dns);
    g_object_set_data(G_OBJECT(page), "ho-offline", offline);

    return page;
}
