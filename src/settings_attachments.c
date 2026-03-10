#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_attachments_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Attachments",
        "How attachments are sent and received."));

    /* Sending */
    GtkWidget *grp = group_box("Sending Attachments");
    GtkWidget *mime_r = gtk_check_button_new_with_label("MIME (AppleDouble)");
    GtkWidget *bh_r = gtk_check_button_new_with_label("BinHex");
    GtkWidget *uu_r = gtk_check_button_new_with_label("UUencode");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(bh_r), GTK_CHECK_BUTTON(mime_r));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(uu_r), GTK_CHECK_BUTTON(mime_r));
    switch (s->attach_encoding) {
        case 1: gtk_check_button_set_active(GTK_CHECK_BUTTON(bh_r), TRUE); break;
        case 2: gtk_check_button_set_active(GTK_CHECK_BUTTON(uu_r), TRUE); break;
        default: gtk_check_button_set_active(GTK_CHECK_BUTTON(mime_r), TRUE); break;
    }
    gtk_widget_set_margin_start(mime_r, 8); gtk_widget_set_margin_top(mime_r, 6); gtk_widget_set_margin_bottom(mime_r, 2);
    gtk_widget_set_margin_start(bh_r, 8); gtk_widget_set_margin_bottom(bh_r, 2);
    gtk_widget_set_margin_start(uu_r, 8); gtk_widget_set_margin_bottom(uu_r, 6);
    group_add(grp, mime_r); group_add(grp, bh_r); group_add(grp, uu_r);
    gtk_box_append(GTK_BOX(page), grp);

    /* Receiving */
    grp = group_box("Receiving Attachments");
    GtkWidget *folder = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(folder), s->attach_folder);
    gtk_entry_set_placeholder_text(GTK_ENTRY(folder), "~/Downloads");
    group_add(grp, form_row("Attachment Folder", folder));

    GtkWidget *trash = group_check("Trash attachments with messages", s->trash_attachments_with_msg);
    group_add(grp, trash);
    GtkWidget *digest = group_check("Receive MIME digests as attachments", s->receive_digests_as_attach);
    group_add(grp, digest);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "att-mime", mime_r);
    g_object_set_data(G_OBJECT(page), "att-binhex", bh_r);
    g_object_set_data(G_OBJECT(page), "att-uu", uu_r);
    g_object_set_data(G_OBJECT(page), "att-folder", folder);
    g_object_set_data(G_OBJECT(page), "att-trash", trash);
    g_object_set_data(G_OBJECT(page), "att-digest", digest);

    return page;
}
