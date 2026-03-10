#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_moving_around_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Moving Around",
        "Keyboard navigation and message switching behavior."));

    /* After finishing a message */
    GtkWidget *grp = group_box("After Finishing a Message");
    const char *after[] = {
        "Don't open anything automatically",
        "Open the next message",
        "Open next unread message",
        "Open next message if unread",
        "Open next if unread & same subject",
        NULL
    };
    GtkWidget *combo = gtk_drop_down_new_from_strings(after);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(combo), s->after_message);
    group_add(grp, form_row("Action", combo));
    gtk_box_append(GTK_BOX(page), grp);

    /* Keystroke options */
    grp = group_box("Keystroke Options");
    GtkWidget *tab = group_check("Tab to switch fields, Option-Tab to insert tab", s->tab_switches_fields);
    GtkWidget *ret = group_check("Return switches among header fields", s->return_switches_fields);
    group_add(grp, tab); group_add(grp, ret);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "mv-after", combo);
    g_object_set_data(G_OBJECT(page), "mv-tab", tab);
    g_object_set_data(G_OBJECT(page), "mv-return", ret);

    return page;
}
