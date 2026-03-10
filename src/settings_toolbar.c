#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_toolbar_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Toolbar",
        "Configure the toolbar appearance."));

    /* Show */
    GtkWidget *grp = group_box("Show");
    GtkWidget *show_tb = group_check("Show toolbar", s->show_toolbar);
    GtkWidget *show_search = group_check("Show search field", s->show_search_field);
    group_add(grp, show_tb); group_add(grp, show_search);
    gtk_box_append(GTK_BOX(page), grp);

    /* Button Style */
    grp = group_box("Button Style");
    const char *styles[] = {
        "Large icons with names", "Large icons only",
        "Small icons & names", "Small icons only", "Names only", NULL
    };
    GtkWidget *combo = gtk_drop_down_new_from_strings(styles);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(combo), s->toolbar_button_style);
    group_add(grp, form_row("Style", combo));
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "tb-show", show_tb);
    g_object_set_data(G_OBJECT(page), "tb-search", show_search);
    g_object_set_data(G_OBJECT(page), "tb-style", combo);

    return page;
}
