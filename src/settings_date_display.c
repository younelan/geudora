#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_date_display_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Date Display",
        "How dates are shown in mailbox listings."));

    /* Date Format */
    GtkWidget *grp = group_box("Date Format");
    GtkWidget *age = gtk_check_button_new_with_label("Age-sensitive (Today, Yesterday, etc.)");
    GtkWidget *fixed = gtk_check_button_new_with_label("Fixed format");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(fixed), GTK_CHECK_BUTTON(age));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(s->date_age_sensitive ? age : fixed), TRUE);
    gtk_widget_set_margin_start(age, 8); gtk_widget_set_margin_top(age, 6); gtk_widget_set_margin_bottom(age, 4);
    gtk_widget_set_margin_start(fixed, 8); gtk_widget_set_margin_bottom(fixed, 6);
    group_add(grp, age);
    group_add(grp, fixed);
    gtk_box_append(GTK_BOX(page), grp);

    /* Timezone */
    grp = group_box("Display Timezone");
    GtkWidget *local_tz = gtk_check_button_new_with_label("Local timezone");
    GtkWidget *sender_tz = gtk_check_button_new_with_label("Sender's timezone");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(sender_tz), GTK_CHECK_BUTTON(local_tz));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(s->date_local_timezone ? local_tz : sender_tz), TRUE);
    gtk_widget_set_margin_start(local_tz, 8); gtk_widget_set_margin_top(local_tz, 6); gtk_widget_set_margin_bottom(local_tz, 4);
    gtk_widget_set_margin_start(sender_tz, 8); gtk_widget_set_margin_bottom(sender_tz, 6);
    group_add(grp, local_tz);
    group_add(grp, sender_tz);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "dd-age", age);
    g_object_set_data(G_OBJECT(page), "dd-fixed", fixed);
    g_object_set_data(G_OBJECT(page), "dd-local", local_tz);
    g_object_set_data(G_OBJECT(page), "dd-sender", sender_tz);

    return page;
}
