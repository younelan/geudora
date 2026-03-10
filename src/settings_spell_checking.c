#include "settings_pages.h"
#include "settings_common.h"

GtkWidget *create_spell_checking_page(AppSettings *s) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Spell Checking",
        "Configure automatic spell checking options."));

    /* Check Spelling */
    GtkWidget *grp = group_box("Check Spelling");
    GtkWidget *auto_chk = group_check("Automatically as you type", s->spell_auto_check);
    GtkWidget *warn_send = group_check("Warn when sending/queueing message with misspellings", s->spell_warn_on_send);
    group_add(grp, auto_chk); group_add(grp, warn_send);
    gtk_box_append(GTK_BOX(page), grp);

    /* Ignore words with */
    grp = group_box("Ignore Words With");
    GtkWidget *ig_caps = group_check("Initial capitals (Nostromo)", s->spell_ignore_caps);
    GtkWidget *ig_mixed = group_check("Mixed capitals (noSTRomo)", s->spell_ignore_mixed_case);
    GtkWidget *ig_nums = group_check("Numbers (n8str9m0)", s->spell_ignore_numbers);
    GtkWidget *ig_allcaps = group_check("All capitals (NOSTROMO)", s->spell_ignore_all_caps);
    group_add(grp, ig_caps); group_add(grp, ig_mixed);
    group_add(grp, ig_nums); group_add(grp, ig_allcaps);
    gtk_box_append(GTK_BOX(page), grp);

    /* Suggest words */
    grp = group_box("Suggest Words That");
    const char *suggest[] = {
        "Look like the word you typed",
        "Sound like the word you typed",
        "Look or sound like the word",
        "Never make suggestions",
        NULL
    };
    GtkWidget *combo = gtk_drop_down_new_from_strings(suggest);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(combo), s->spell_suggest_mode);
    group_add(grp, form_row("Mode", combo));
    gtk_box_append(GTK_BOX(page), grp);

    g_object_set_data(G_OBJECT(page), "sp-auto", auto_chk);
    g_object_set_data(G_OBJECT(page), "sp-warn", warn_send);
    g_object_set_data(G_OBJECT(page), "sp-caps", ig_caps);
    g_object_set_data(G_OBJECT(page), "sp-mixed", ig_mixed);
    g_object_set_data(G_OBJECT(page), "sp-nums", ig_nums);
    g_object_set_data(G_OBJECT(page), "sp-allcaps", ig_allcaps);
    g_object_set_data(G_OBJECT(page), "sp-suggest", combo);

    return page;
}
