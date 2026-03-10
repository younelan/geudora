#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_ssl_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("SSL",
        "Secure Sockets Layer settings for encrypted connections."));

    const char *ssl_modes[] = {"Off", "Required", "Required, Alternate Port", NULL};

    GtkWidget *grp = group_box("Secure Sockets Layer");
    GtkWidget *pop_combo = gtk_drop_down_new_from_strings(ssl_modes);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(pop_combo), s->ssl_pop_mode);
    group_add(grp, form_row("SSL for POP", pop_combo));

    GtkWidget *smtp_combo = gtk_drop_down_new_from_strings(ssl_modes);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(smtp_combo), s->ssl_smtp_mode);
    group_add(grp, form_row("SSL for SMTP", smtp_combo));

    GtkWidget *imap_combo = gtk_drop_down_new_from_strings(ssl_modes);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(imap_combo), s->ssl_imap_mode);
    group_add(grp, form_row("SSL for IMAP", imap_combo));
    gtk_box_append(GTK_BOX(page), grp);

    /* Credentials */
    grp = group_box("Credentials");
    GtkWidget *save_pw = group_check("Remember password (use system keychain)", s->save_password);
    group_add(grp, save_pw);

    GtkWidget *chpw = gtk_button_new_with_label("Change Password...");
    gtk_widget_set_halign(chpw, GTK_ALIGN_START);
    gtk_widget_set_margin_start(chpw, 8);
    gtk_widget_set_margin_bottom(chpw, 6);
    group_add(grp, chpw);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "ssl-pop", pop_combo);
    g_object_set_data(G_OBJECT(page), "ssl-smtp", smtp_combo);
    g_object_set_data(G_OBJECT(page), "ssl-imap", imap_combo);
    g_object_set_data(G_OBJECT(page), "ssl-savepw", save_pw);

    return page;
}
