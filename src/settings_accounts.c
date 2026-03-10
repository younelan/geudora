#include "settings_pages.h"
#include "settings_common.h"
#include "gtk_prefs.h"
#include <string.h>

/* Account dialog callbacks */
static void on_add_ok(GtkWidget *widget, gpointer user_data);
static void on_edit_save(GtkWidget *widget, gpointer user_data);

static void on_account_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)user_data;
    GtkWidget *edit = g_object_get_data(G_OBJECT(box), "edit-button");
    GtkWidget *rm = g_object_get_data(G_OBJECT(box), "remove-button");
    gboolean sel = (row != NULL);
    gtk_widget_set_sensitive(edit, sel);
    gtk_widget_set_sensitive(rm, sel);
}

static GtkWidget *make_account_row(const char *name, const char *type, const char *server) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *rbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(rbox, 8); gtk_widget_set_margin_end(rbox, 8);
    gtk_widget_set_margin_top(rbox, 6); gtk_widget_set_margin_bottom(rbox, 6);

    gtk_box_append(GTK_BOX(rbox), gtk_image_new_from_icon_name("mail-send-receive-symbolic"));

    GtkWidget *info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *nm = gtk_label_new(name);
    gtk_label_set_xalign(GTK_LABEL(nm), 0);
    PangoAttrList *ba = pango_attr_list_new();
    pango_attr_list_insert(ba, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(nm), ba);
    pango_attr_list_unref(ba);
    gtk_box_append(GTK_BOX(info), nm);

    char detail[512];
    snprintf(detail, sizeof(detail), "%s — %s", type, server);
    GtkWidget *dt = gtk_label_new(detail);
    gtk_label_set_xalign(GTK_LABEL(dt), 0);
    gtk_widget_add_css_class(dt, "dim-label");
    gtk_box_append(GTK_BOX(info), dt);

    gtk_widget_set_hexpand(info, TRUE);
    gtk_box_append(GTK_BOX(rbox), info);
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rbox);
    return row;
}

static void open_account_dialog(GtkWindow *parent, GtkWidget *page, int idx) {
    PrefsAccount *accounts = g_object_get_data(G_OBJECT(page), "acct-array");
    PrefsAccount *acct = (idx >= 0) ? &accounts[idx] : NULL;

    GtkWidget *dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), idx >= 0 ? "Edit Account" : "Add Email Account");
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 420, -1);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(box, 20); gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 16); gtk_widget_set_margin_bottom(box, 16);
    gtk_window_set_child(GTK_WINDOW(dlg), box);

    GtkWidget *grp = group_box("Account Details");
    GtkWidget *n = gtk_entry_new(); if (acct) gtk_editable_set_text(GTK_EDITABLE(n), acct->name);
    group_add(grp, form_row("Name", n));
    GtkWidget *e = gtk_entry_new(); if (acct) gtk_editable_set_text(GTK_EDITABLE(e), acct->email);
    group_add(grp, form_row("Email", e));
    const char *types[] = {"IMAP", "POP", NULL};
    GtkWidget *t = gtk_drop_down_new_from_strings(types);
    if (acct && g_strcmp0(acct->type, "POP") == 0) gtk_drop_down_set_selected(GTK_DROP_DOWN(t), 1);
    group_add(grp, form_row("Type", t));
    GtkWidget *sv = gtk_entry_new(); if (acct) gtk_editable_set_text(GTK_EDITABLE(sv), acct->server);
    group_add(grp, form_row("Mail Server", sv));
    GtkWidget *sm = gtk_entry_new(); if (acct) gtk_editable_set_text(GTK_EDITABLE(sm), acct->smtp_server);
    group_add(grp, form_row("SMTP Server", sm));
    GtkWidget *u = gtk_entry_new(); if (acct) gtk_editable_set_text(GTK_EDITABLE(u), acct->username);
    group_add(grp, form_row("Username", u));
    gtk_box_append(GTK_BOX(box), grp);

    GtkWidget *bbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(bbar, GTK_ALIGN_END);
    gtk_widget_set_margin_top(bbar, 12);
    GtkWidget *cancel = gtk_button_new_with_label("Cancel");
    GtkWidget *ok = gtk_button_new_with_label(idx >= 0 ? "Save" : "Add");
    gtk_widget_add_css_class(ok, "suggested-action");
    gtk_box_append(GTK_BOX(bbar), cancel);
    gtk_box_append(GTK_BOX(bbar), ok);
    gtk_box_append(GTK_BOX(box), bbar);

    g_object_set_data(G_OBJECT(dlg), "name-entry", n);
    g_object_set_data(G_OBJECT(dlg), "email-entry", e);
    g_object_set_data(G_OBJECT(dlg), "type-combo", t);
    g_object_set_data(G_OBJECT(dlg), "server-entry", sv);
    g_object_set_data(G_OBJECT(dlg), "smtp-entry", sm);
    g_object_set_data(G_OBJECT(dlg), "user-entry", u);
    g_object_set_data(G_OBJECT(dlg), "page", page);
    g_object_set_data(G_OBJECT(dlg), "account-index", GINT_TO_POINTER(idx));

    g_signal_connect_swapped(cancel, "clicked", G_CALLBACK(gtk_window_close), dlg);
    g_signal_connect(ok, "clicked", G_CALLBACK(idx >= 0 ? on_edit_save : on_add_ok), dlg);
    gtk_window_present(GTK_WINDOW(dlg));
}

static void on_add_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *page = (GtkWidget *)user_data;
    GtkWidget *dialog = gtk_widget_get_ancestor(page, GTK_TYPE_WINDOW);
    open_account_dialog(dialog ? GTK_WINDOW(dialog) : NULL, page, -1);
}

static void on_edit_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *page = (GtkWidget *)user_data;
    GtkWidget *list = g_object_get_data(G_OBJECT(page), "acct-list");
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list));
    if (!row) return;
    GtkWidget *dialog = gtk_widget_get_ancestor(page, GTK_TYPE_WINDOW);
    open_account_dialog(dialog ? GTK_WINDOW(dialog) : NULL, page,
                        gtk_list_box_row_get_index(row));
}

static void on_remove_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *page = (GtkWidget *)user_data;
    GtkWidget *list = g_object_get_data(G_OBJECT(page), "acct-list");
    int *count = g_object_get_data(G_OBJECT(page), "acct-count");
    PrefsAccount *accounts = g_object_get_data(G_OBJECT(page), "acct-array");
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(list));
    if (!row) return;
    int idx = gtk_list_box_row_get_index(row);
    gtk_list_box_remove(GTK_LIST_BOX(list), GTK_WIDGET(row));
    if (idx >= 0 && idx < *count) {
        for (int i = idx; i < *count - 1; i++) accounts[i] = accounts[i + 1];
        (*count)--;
    }
}

static void on_add_ok(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *dlg = (GtkWidget *)user_data;
    GtkWidget *page = g_object_get_data(G_OBJECT(dlg), "page");
    GtkWidget *list = g_object_get_data(G_OBJECT(page), "acct-list");
    int *count = g_object_get_data(G_OBJECT(page), "acct-count");
    PrefsAccount *accounts = g_object_get_data(G_OBJECT(page), "acct-array");

    const char *name = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "name-entry")));
    if (!name || !*name) return;
    if (*count >= 10) return;

    PrefsAccount *a = &accounts[*count];
    strncpy(a->name, name, sizeof(a->name) - 1);
    strncpy(a->email, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "email-entry"))), sizeof(a->email) - 1);
    GtkStringObject *to = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "type-combo"))));
    strncpy(a->type, to ? gtk_string_object_get_string(to) : "IMAP", sizeof(a->type) - 1);
    strncpy(a->server, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "server-entry"))), sizeof(a->server) - 1);
    strncpy(a->smtp_server, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "smtp-entry"))), sizeof(a->smtp_server) - 1);
    strncpy(a->username, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "user-entry"))), sizeof(a->username) - 1);
    a->enabled = TRUE;

    gtk_list_box_append(GTK_LIST_BOX(list), make_account_row(a->name, a->type, a->server));
    (*count)++;
    gtk_window_close(GTK_WINDOW(dlg));
}

static void on_edit_save(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *dlg = (GtkWidget *)user_data;
    GtkWidget *page = g_object_get_data(G_OBJECT(dlg), "page");
    GtkWidget *list = g_object_get_data(G_OBJECT(page), "acct-list");
    PrefsAccount *accounts = g_object_get_data(G_OBJECT(page), "acct-array");
    int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dlg), "account-index"));

    const char *name = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "name-entry")));
    if (!name || !*name) return;

    PrefsAccount *a = &accounts[idx];
    strncpy(a->name, name, sizeof(a->name) - 1);
    strncpy(a->email, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "email-entry"))), sizeof(a->email) - 1);
    GtkStringObject *to = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "type-combo"))));
    strncpy(a->type, to ? gtk_string_object_get_string(to) : "IMAP", sizeof(a->type) - 1);
    strncpy(a->server, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "server-entry"))), sizeof(a->server) - 1);
    strncpy(a->smtp_server, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "smtp-entry"))), sizeof(a->smtp_server) - 1);
    strncpy(a->username, gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "user-entry"))), sizeof(a->username) - 1);

    /* Refresh the row in the list */
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(list), idx);
    if (row) {
        gtk_list_box_remove(GTK_LIST_BOX(list), GTK_WIDGET(row));
        GtkWidget *new_row = make_account_row(a->name, a->type, a->server);
        gtk_list_box_insert(GTK_LIST_BOX(list), new_row, idx);
    }
    gtk_window_close(GTK_WINDOW(dlg));
}

/* Persistent storage for account count — freed when page is destroyed */
static void free_count(gpointer data) { g_free(data); }
static void free_accounts(gpointer data) { g_free(data); }

GtkWidget *create_accounts_page(AppSettings *s, GtkWindow *parent) {
    (void)parent;
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_start(page, 24); gtk_widget_set_margin_end(page, 24);
    gtk_widget_set_margin_top(page, 20); gtk_widget_set_margin_bottom(page, 20);

    gtk_box_append(GTK_BOX(page), page_title("Accounts",
        "Manage your email accounts (personalities)."));

    /* Load accounts */
    PrefsAccount *accounts = g_new0(PrefsAccount, 10);
    int loaded = prefs_load_accounts(accounts, 10);
    int *count = g_new0(int, 1);
    *count = loaded;

    g_object_set_data_full(G_OBJECT(page), "acct-array", accounts, free_accounts);
    g_object_set_data_full(G_OBJECT(page), "acct-count", count, free_count);

    /* List */
    GtkWidget *acct_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(acct_scroll), 150);
    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(list, "boxed-list");
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(acct_scroll), list);
    gtk_widget_set_vexpand(acct_scroll, TRUE);
    gtk_box_append(GTK_BOX(page), acct_scroll);

    g_object_set_data(G_OBJECT(page), "acct-list", list);

    for (int i = 0; i < loaded; i++)
        gtk_list_box_append(GTK_LIST_BOX(list),
            make_account_row(accounts[i].name, accounts[i].type, accounts[i].server));

    /* Buttons */
    GtkWidget *btn_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(btn_bar, 8);
    GtkWidget *add_btn = gtk_button_new_with_label("Add");
    GtkWidget *edit_btn = gtk_button_new_with_label("Edit");
    GtkWidget *rm_btn = gtk_button_new_with_label("Remove");
    gtk_widget_set_sensitive(edit_btn, FALSE);
    gtk_widget_set_sensitive(rm_btn, FALSE);
    gtk_box_append(GTK_BOX(btn_bar), add_btn);
    gtk_box_append(GTK_BOX(btn_bar), edit_btn);
    gtk_box_append(GTK_BOX(btn_bar), rm_btn);
    gtk_box_append(GTK_BOX(page), btn_bar);

    g_object_set_data(G_OBJECT(list), "edit-button", edit_btn);
    g_object_set_data(G_OBJECT(list), "remove-button", rm_btn);

    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), page);
    g_signal_connect(edit_btn, "clicked", G_CALLBACK(on_edit_clicked), page);
    g_signal_connect(rm_btn, "clicked", G_CALLBACK(on_remove_clicked), page);
    g_signal_connect(list, "row-selected", G_CALLBACK(on_account_selected), NULL);

    (void)s;
    return page;
}
