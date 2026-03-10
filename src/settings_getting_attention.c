#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_getting_attention_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Getting Attention",
        "How gEudora notifies you of new mail and activity."));

    /* New Mail */
    GtkWidget *grp = group_box("When Something Exciting Happens");
    GtkWidget *alert = group_check("Put up an alert", s->alert_on_new);
    GtkWidget *bounce = group_check("Bounce the icon in dock", s->bounce_dock);
    GtkWidget *open_mb = group_check("Open mailbox (new mail only)", s->open_mailbox_on_new);
    GtkWidget *sound = group_check("Play a sound", s->play_sound_on_new);
    group_add(grp, alert); group_add(grp, bounce);
    group_add(grp, open_mb); group_add(grp, sound);
    gtk_box_append(GTK_BOX(page), grp);

    /* Sounds */
    grp = group_box("Sounds");
    GtkWidget *snd_entry = gtk_entry_new();
    gtk_editable_set_text(GTK_EDITABLE(snd_entry), s->new_mail_sound);
    gtk_entry_set_placeholder_text(GTK_ENTRY(snd_entry), "New Mail");
    group_add(grp, form_row("New Mail Sound", snd_entry));
    gtk_box_append(GTK_BOX(page), grp);

    /* More */
    grp = group_box("More Attention-Getting Behavior");
    GtkWidget *progress = group_check("Show Task Progress during background activity", s->show_task_progress);
    group_add(grp, progress);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "ga-alert", alert);
    g_object_set_data(G_OBJECT(page), "ga-bounce", bounce);
    g_object_set_data(G_OBJECT(page), "ga-open", open_mb);
    g_object_set_data(G_OBJECT(page), "ga-sound", sound);
    g_object_set_data(G_OBJECT(page), "ga-snd-entry", snd_entry);
    g_object_set_data(G_OBJECT(page), "ga-progress", progress);

    return page;
}
