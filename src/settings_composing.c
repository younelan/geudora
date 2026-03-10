#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_composing_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Composing Mail",
        "Options for composing new messages."));

    /* Nickname Expansion */
    GtkWidget *grp = group_box("Nickname Expansion");
    GtkWidget *autocomp = group_check("Auto-complete nicknames", s->auto_complete_nicknames);
    group_add(grp, autocomp);
    GtkWidget *expand = group_check("Expand nicknames immediately", s->expand_nicknames_immediately);
    group_add(grp, expand);
    gtk_box_append(GTK_BOX(page), grp);

    /* Message Settings */
    grp = group_box("Message Settings");
    GtkWidget *wrap_spin;
    GtkWidget *wrap_row = spin_row("Word wrap at column", &wrap_spin,
                                    40, 132, s->word_wrap_column > 0 ? s->word_wrap_column : 76, NULL);
    group_add(grp, wrap_row);
    GtkWidget *qp = group_check("May use Quoted-Printable encoding", s->may_use_qp);
    group_add(grp, qp);
    GtkWidget *keep = group_check("Keep copies of outgoing mail", s->keep_sent_copy);
    group_add(grp, keep);
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "comp-autocomp", autocomp);
    g_object_set_data(G_OBJECT(page), "comp-expand", expand);
    g_object_set_data(G_OBJECT(page), "comp-wrap", wrap_spin);
    g_object_set_data(G_OBJECT(page), "comp-qp", qp);
    g_object_set_data(G_OBJECT(page), "comp-keep", keep);

    return page;
}
