/*
 * GTK4 Settings Dialog for gEudora — coordinator
 * Builds the sidebar + page-switching shell; each section page is in its own file.
 */

#include "gtk_settings.h"
#include "gtk_prefs.h"
#include "settings_pages.h"
#include "settings_common.h"
#include "theme.h"
#include <stdlib.h>
#include <string.h>

/* ── Section selection ── */
static void on_section_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    (void)box;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    if (!row || !dialog) return;
    int index = gtk_list_box_row_get_index(row);
    for (int i = 0; i < SETTINGS_COUNT; i++)
        gtk_widget_set_visible(dialog->section_widgets[i], (i == index));
}

/* ── Sync all pages back to settings before save ── */
static void sync_all_pages(SettingsDialog *dialog)
{
    AppSettings *s = dialog->settings;

    for (int i = 0; i < SETTINGS_COUNT; i++) {
        GtkWidget *scroll = dialog->section_widgets[i];
        GtkWidget *raw = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scroll));
        if (!raw) { g_print("DEBUG sync: page %d raw=NULL\n", i); continue; }
        GtkWidget *page = raw;
        g_print("DEBUG sync: page %d type=%s\n", i, G_OBJECT_TYPE_NAME(raw));
        /* GTK4 wraps non-scrollable children in a GtkViewport — unwrap it */
        if (GTK_IS_VIEWPORT(page))
            page = gtk_viewport_get_child(GTK_VIEWPORT(page));
        if (!page) { g_print("DEBUG sync: page %d viewport child=NULL\n", i); continue; }
        if (page != raw)
            g_print("DEBUG sync: page %d unwrapped to %s\n", i, G_OBJECT_TYPE_NAME(page));

        /* Helper macros to reduce boilerplate */
        #define GET_CHECK(key, field) do { \
            GtkWidget *w = g_object_get_data(G_OBJECT(page), key); \
            if (w) s->field = gtk_check_button_get_active(GTK_CHECK_BUTTON(w)); \
            else g_print("DEBUG sync: key '%s' not found on page %d\n", key, i); \
        } while(0)
        #define GET_TEXT(key, field) do { \
            GtkWidget *w = g_object_get_data(G_OBJECT(page), key); \
            if (w) { \
                const char *_t = gtk_editable_get_text(GTK_EDITABLE(w)); \
                g_print("DEBUG sync: %s = '%s'\n", key, _t ? _t : "(null)"); \
                strncpy(s->field, _t ? _t : "", sizeof(s->field) - 1); \
            } else g_print("DEBUG sync: key '%s' not found on page %d\n", key, i); \
        } while(0)
        #define GET_SPIN(key, field) do { \
            GtkWidget *w = g_object_get_data(G_OBJECT(page), key); \
            if (w) s->field = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)); \
        } while(0)
        #define GET_DROP(key, field) do { \
            GtkWidget *w = g_object_get_data(G_OBJECT(page), key); \
            if (w) s->field = (int)gtk_drop_down_get_selected(GTK_DROP_DOWN(w)); \
        } while(0)

        switch ((SettingsSection)i) {
        case SETTINGS_GETTING_STARTED:
            GET_TEXT("gs-user", pop_username);
            GET_TEXT("gs-server", pop_server);
            GET_TEXT("gs-realname", real_name);
            GET_TEXT("gs-smtp", smtp_server);
            GET_TEXT("gs-email", email_address);
            GET_CHECK("gs-defmail", default_mailer);
            break;

        case SETTINGS_CHECKING_MAIL:
            /* pop_username and pop_server are synced from Getting Started page */
            GET_CHECK("cm-pop", use_pop);
            GET_CHECK("cm-imap", use_imap);
            GET_CHECK("cm-pwd", use_passwords);
            GET_CHECK("cm-kerb", use_kerberos);
            GET_CHECK("cm-apop", use_apop);
            GET_CHECK("cm-overlap", overlap_commands);
            GET_SPIN("cm-interval", check_interval);
            GET_CHECK("cm-battery", check_battery);
            GET_CHECK("cm-send-check", send_on_check);
            GET_SPIN("cm-leave", leave_days);
            GET_CHECK("cm-del-trash", delete_from_trash);
            GET_SPIN("cm-skip", skip_size_kb);
            break;

        case SETTINGS_SENDING_MAIL:
            /* email_address and smtp_server are synced from Getting Started page */
            GET_TEXT("sm-domain", default_domain);
            GET_CHECK("sm-subport", use_submission_port);
            GET_CHECK("sm-smtpauth", allow_smtp_auth);
            GET_CHECK("sm-immediate", send_immediately);
            GET_CHECK("sm-sendcheck", send_on_check);
            GET_CHECK("sm-curly", fix_curly_quotes);
            GET_CHECK("sm-keep", keep_sent_copy);
            GET_CHECK("sm-autofcc", auto_fcc_original);
            break;

        case SETTINGS_COMPOSING:
            GET_CHECK("comp-autocomp", auto_complete_nicknames);
            GET_CHECK("comp-expand", expand_nicknames_immediately);
            GET_SPIN("comp-wrap", word_wrap_column);
            GET_CHECK("comp-qp", may_use_qp);
            GET_CHECK("comp-keep", keep_sent_copy);
            break;

        case SETTINGS_MAILBOX_DISPLAY:
            GET_CHECK("mbd-status", show_col_status);
            GET_CHECK("mbd-priority", show_col_priority);
            GET_CHECK("mbd-attach", show_col_attachments);
            GET_CHECK("mbd-label", show_col_label);
            GET_CHECK("mbd-who", show_col_who);
            GET_CHECK("mbd-date", show_col_date);
            GET_CHECK("mbd-size", show_col_size);
            GET_CHECK("mbd-server", show_col_server);
            GET_CHECK("mbd-mood", show_col_mood);
            GET_CHECK("mbd-junk", show_col_junk);
            GET_CHECK("mbd-hlines", draw_horiz_lines);
            GET_CHECK("mbd-vlines", draw_vert_lines);
            GET_CHECK("mbd-selcount", show_selected_count);
            GET_CHECK("mbd-preview", show_preview);
            GET_SPIN("mbd-read-spin", mark_read_delay);
            GET_CHECK("mbd-read-scroll", mark_read_on_scroll);
            GET_CHECK("mbd-read-del", mark_read_on_delete);
            break;

        case SETTINGS_DATE_DISPLAY:
            GET_CHECK("dd-age", date_age_sensitive);
            GET_CHECK("dd-local", date_local_timezone);
            break;

        case SETTINGS_STYLED_TEXT: {
            GtkWidget *w;
            w = g_object_get_data(G_OBJECT(page), "st-both");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->styled_send_mode = 0;
            w = g_object_get_data(G_OBJECT(page), "st-styled");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->styled_send_mode = 1;
            w = g_object_get_data(G_OBJECT(page), "st-plain");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->styled_send_mode = 2;
            w = g_object_get_data(G_OBJECT(page), "st-ask");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->styled_send_mode = 3;
            GET_CHECK("st-bold", styled_bold);
            GET_CHECK("st-italic", styled_italic);
            GET_CHECK("st-underline", styled_underline);
            GET_CHECK("st-color", styled_color);
            GET_CHECK("st-size", styled_size);
            GET_CHECK("st-font", styled_font);
            GET_CHECK("st-margins", styled_margins);
            GET_CHECK("st-fmttb", show_format_toolbar);
            break;
        }

        case SETTINGS_FONTS: {
            const char *font_keys[] = {"f-screen", "f-fixed", "f-print"};
            const char *size_keys[] = {"f-screen-sz", "f-fixed-sz", "f-print-sz"};
            char *font_fields[] = {s->screen_font, s->fixed_font, s->print_font};
            int *size_fields[] = {&s->screen_font_size, &s->fixed_font_size, &s->print_font_size};
            for (int j = 0; j < 3; j++) {
                GtkWidget *fb = g_object_get_data(G_OBJECT(page), font_keys[j]);
                if (fb) {
                    PangoFontDescription *fd = gtk_font_dialog_button_get_font_desc(GTK_FONT_DIALOG_BUTTON(fb));
                    if (fd) { gchar *fs = pango_font_description_to_string(fd); if (fs) { strncpy(font_fields[j], fs, 255); g_free(fs); } }
                }
                GtkWidget *sz = g_object_get_data(G_OBJECT(page), size_keys[j]);
                if (sz) *size_fields[j] = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(sz));
            }
            GET_CHECK("f-gfx", display_graphics);
            GET_CHECK("f-emo", display_emoticons);
            GET_CHECK("f-zoom", zoom_on_open);
            break;
        }

        case SETTINGS_LABELS:
            for (int j = 0; j < 8; j++) {
                char key_e[32], key_c[32];
                snprintf(key_e, sizeof(key_e), "lbl-entry-%d", j);
                snprintf(key_c, sizeof(key_c), "lbl-color-%d", j);
                GtkWidget *entry = g_object_get_data(G_OBJECT(page), key_e);
                if (entry) strncpy(s->label_names[j], gtk_editable_get_text(GTK_EDITABLE(entry)), 63);
                GtkWidget *color = g_object_get_data(G_OBJECT(page), key_c);
                if (color) {
                    const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(color));
                    if (rgba) {
                        s->label_colors[j][0] = rgba->red;
                        s->label_colors[j][1] = rgba->green;
                        s->label_colors[j][2] = rgba->blue;
                    }
                }
            }
            break;

        case SETTINGS_ATTACHMENTS: {
            GtkWidget *w;
            w = g_object_get_data(G_OBJECT(page), "att-mime");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->attach_encoding = 0;
            w = g_object_get_data(G_OBJECT(page), "att-binhex");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->attach_encoding = 1;
            w = g_object_get_data(G_OBJECT(page), "att-uu");
            if (w && gtk_check_button_get_active(GTK_CHECK_BUTTON(w))) s->attach_encoding = 2;
            GET_TEXT("att-folder", attach_folder);
            GET_CHECK("att-trash", trash_attachments_with_msg);
            GET_CHECK("att-digest", receive_digests_as_attach);
            break;
        }

        case SETTINGS_REPLYING:
            GET_CHECK("rep-all", reply_to_all_default);
            GET_CHECK("rep-self", reply_include_self);
            GET_CHECK("rep-cc", reply_to_in_cc);
            GET_CHECK("rep-pri", copy_original_priority);
            break;

        case SETTINGS_JUNK_MAIL:
            GET_SPIN("junk-thresh", junk_threshold);
            GET_CHECK("junk-ab", junk_check_addressbook);
            GET_CHECK("junk-hold", junk_hold_mailbox);
            GET_CHECK("junk-unread", junk_never_unread);
            GET_SPIN("junk-remove", junk_remove_days);
            GET_CHECK("junk-warn", junk_warn_remove);
            break;

        case SETTINGS_TOOLBAR:
            GET_CHECK("tb-show", show_toolbar);
            GET_CHECK("tb-search", show_search_field);
            GET_DROP("tb-style", toolbar_button_style);
            break;

        case SETTINGS_GETTING_ATTENTION:
            GET_CHECK("ga-alert", alert_on_new);
            GET_CHECK("ga-bounce", bounce_dock);
            GET_CHECK("ga-open", open_mailbox_on_new);
            GET_CHECK("ga-sound", play_sound_on_new);
            GET_TEXT("ga-snd-entry", new_mail_sound);
            GET_CHECK("ga-progress", show_task_progress);
            break;

        case SETTINGS_HOSTS:
            GET_TEXT("ho-ph", ph_server);
            GET_TEXT("ho-finger", finger_server);
            GET_CHECK("ho-dns", dns_load_balance);
            GET_CHECK("ho-offline", offline_mode);
            break;

        case SETTINGS_MOVING_AROUND:
            GET_DROP("mv-after", after_message);
            GET_CHECK("mv-tab", tab_switches_fields);
            GET_CHECK("mv-return", return_switches_fields);
            break;

        case SETTINGS_EXTRA_WARNINGS:
            GET_CHECK("ew-unread", warn_delete_unread);
            GET_CHECK("ew-queued", warn_delete_queued);
            GET_CHECK("ew-unsent", warn_delete_unsent);
            GET_CHECK("ew-subj", warn_queue_no_subject);
            GET_CHECK("ew-styled", warn_queue_styled);
            GET_CHECK("ew-quit", warn_quit_queued);
            GET_CHECK("ew-trash", warn_empty_trash);
            GET_SPIN("ew-size", warn_send_size_kb);
            break;

        case SETTINGS_MISCELLANEOUS:
            GET_CHECK("misc-close", close_msg_with_mailbox);
            GET_CHECK("misc-empty", empty_trash_on_quit);
            GET_CHECK("misc-turbo", turbo_redirect);
            GET_CHECK("misc-resort", resort_less_often);
            GET_CHECK("misc-toc", use_old_toc);
            GET_CHECK("misc-filter", generate_filter_reports);
            GET_CHECK("misc-keychain", use_keychain);
            break;

        case SETTINGS_ACCOUNTS:
            break;

        case SETTINGS_SSL:
            GET_DROP("ssl-pop", ssl_pop_mode);
            GET_DROP("ssl-smtp", ssl_smtp_mode);
            GET_DROP("ssl-imap", ssl_imap_mode);
            GET_CHECK("ssl-savepw", save_password);
            break;

        case SETTINGS_SPELL_CHECKING:
            GET_CHECK("sp-auto", spell_auto_check);
            GET_CHECK("sp-warn", spell_warn_on_send);
            GET_CHECK("sp-caps", spell_ignore_caps);
            GET_CHECK("sp-mixed", spell_ignore_mixed_case);
            GET_CHECK("sp-nums", spell_ignore_numbers);
            GET_CHECK("sp-allcaps", spell_ignore_all_caps);
            GET_DROP("sp-suggest", spell_suggest_mode);
            break;

        case SETTINGS_COUNT:
            break;
        }

        #undef GET_CHECK
        #undef GET_TEXT
        #undef GET_SPIN
        #undef GET_DROP
    }
}

/* ── OK / Cancel ── */

static void on_ok_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    sync_all_pages(dialog);

    /* Save accounts */
    GtkWidget *acct_scroll = dialog->section_widgets[SETTINGS_ACCOUNTS];
    GtkWidget *acct_page = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(acct_scroll));
    if (acct_page && GTK_IS_VIEWPORT(acct_page))
        acct_page = gtk_viewport_get_child(GTK_VIEWPORT(acct_page));
    if (acct_page) {
        int *count = g_object_get_data(G_OBJECT(acct_page), "acct-count");
        PrefsAccount *accounts = g_object_get_data(G_OBJECT(acct_page), "acct-array");
        if (count && accounts && *count > 0)
            prefs_save_accounts(accounts, *count);
    }

    prefs_save(dialog->settings);
    g_object_set_data(G_OBJECT(dialog->dialog), "response-id", GINT_TO_POINTER(GTK_RESPONSE_OK));
    gtk_window_close(GTK_WINDOW(dialog->dialog));
}

static void on_cancel_clicked(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    SettingsDialog *dialog = (SettingsDialog *)user_data;
    g_object_set_data(G_OBJECT(dialog->dialog), "response-id", GINT_TO_POINTER(GTK_RESPONSE_CANCEL));
    gtk_window_close(GTK_WINDOW(dialog->dialog));
}

/* ── Create the settings dialog ── */

SettingsDialog* create_settings_dialog(GtkWindow *parent, AppSettings *settings)
{
    SettingsDialog *dialog = g_new0(SettingsDialog, 1);
    /* Use live settings directly — changes take effect immediately */
    dialog->settings = settings ? settings : g_new0(AppSettings, 1);

    dialog->dialog = gtk_window_new();
    theme_setup_headerbar(dialog->dialog, "gEudora Settings");
    gtk_window_set_modal(GTK_WINDOW(dialog->dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog->dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog->dialog), 780, 600);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(dialog->dialog), main_vbox);

    GtkWidget *hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(hpaned), 190);
    gtk_widget_set_vexpand(hpaned, TRUE);
    gtk_widget_set_hexpand(hpaned, TRUE);

    /* Sidebar */
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    dialog->section_list = GTK_LIST_BOX(gtk_list_box_new());
    gtk_list_box_set_selection_mode(dialog->section_list, GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(GTK_WIDGET(dialog->section_list), "navigation-sidebar");

    const char *section_names[] = {
        "Getting Started", "Checking Mail", "Sending Mail",
        "Composing", "Mailbox Display", "Date Display",
        "Styled Text", "Fonts", "Labels",
        "Attachments", "Replying", "Junk Mail",
        "Toolbar", "Getting Attention", "Hosts",
        "Moving Around", "Extra Warnings", "Miscellaneous",
        "Accounts", "SSL", "Spell Checking"
    };
    const char *section_icons[] = {
        "go-home-symbolic", "mail-read-symbolic", "mail-send-symbolic",
        "accessories-text-editor-symbolic", "view-list-symbolic", "x-office-calendar-symbolic",
        "format-text-rich-symbolic", "font-x-generic-symbolic", "starred-symbolic",
        "mail-attachment-symbolic", "mail-reply-all-symbolic", "edit-delete-symbolic",
        "applications-system-symbolic", "dialog-information-symbolic", "network-server-symbolic",
        "go-next-symbolic", "dialog-warning-symbolic", "preferences-system-symbolic",
        "system-users-symbolic", "channel-secure-symbolic", "tools-check-spelling-symbolic"
    };

    for (int i = 0; i < SETTINGS_COUNT; i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *rbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_margin_start(rbox, 8); gtk_widget_set_margin_end(rbox, 8);
        gtk_widget_set_margin_top(rbox, 6); gtk_widget_set_margin_bottom(rbox, 6);
        gtk_box_append(GTK_BOX(rbox), gtk_image_new_from_icon_name(section_icons[i]));
        GtkWidget *label = gtk_label_new(section_names[i]);
        gtk_label_set_xalign(GTK_LABEL(label), 0);
        gtk_box_append(GTK_BOX(rbox), label);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), rbox);
        gtk_list_box_append(dialog->section_list, row);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroll), GTK_WIDGET(dialog->section_list));
    gtk_paned_set_start_child(GTK_PANED(hpaned), sidebar_scroll);
    gtk_paned_set_resize_start_child(GTK_PANED(hpaned), FALSE);

    /* Content area */
    dialog->content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(dialog->content_area, TRUE);
    gtk_widget_set_vexpand(dialog->content_area, TRUE);

    AppSettings *s = dialog->settings;
    typedef GtkWidget* (*PageBuilder)(AppSettings *);
    PageBuilder builders[] = {
        create_getting_started_page,
        create_checking_mail_page,
        create_sending_mail_page,
        create_composing_page,
        create_mailbox_display_page,
        create_date_display_page,
        create_styled_text_page,
        create_fonts_page,
        create_labels_page,
        create_attachments_page,
        create_replying_page,
        create_junk_mail_page,
        create_toolbar_page,
        create_getting_attention_page,
        create_hosts_page,
        create_moving_around_page,
        create_extra_warnings_page,
        create_miscellaneous_page,
        NULL, /* SETTINGS_ACCOUNTS — handled specially */
        create_ssl_page,
        create_spell_checking_page,
    };

    for (int i = 0; i < SETTINGS_COUNT; i++) {
        GtkWidget *scroll = gtk_scrolled_window_new();
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        GtkWidget *page_widget;
        if (i == SETTINGS_ACCOUNTS)
            page_widget = create_accounts_page(s, GTK_WINDOW(dialog->dialog));
        else
            page_widget = builders[i](s);

        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), page_widget);
        gtk_widget_set_visible(scroll, (i == 0));
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_box_append(GTK_BOX(dialog->content_area), scroll);
        dialog->section_widgets[i] = scroll;
    }

    /* Share GtkEntryBuffers between Getting Started and Checking/Sending pages
       so editing in either place keeps both in sync automatically. */
    {
        /* Helper to unwrap scrolled-window → viewport → page */
        #define UNWRAP_PAGE(idx) ({ \
            GtkWidget *_s = dialog->section_widgets[idx]; \
            GtkWidget *_p = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(_s)); \
            if (_p && GTK_IS_VIEWPORT(_p)) _p = gtk_viewport_get_child(GTK_VIEWPORT(_p)); \
            _p; \
        })
        GtkWidget *gs_page = UNWRAP_PAGE(SETTINGS_GETTING_STARTED);
        GtkWidget *cm_page = UNWRAP_PAGE(SETTINGS_CHECKING_MAIL);
        GtkWidget *sm_page = UNWRAP_PAGE(SETTINGS_SENDING_MAIL);
        #undef UNWRAP_PAGE

        /* Share pop_username buffer: gs-user ↔ cm-user */
        GtkWidget *gs_user = g_object_get_data(G_OBJECT(gs_page), "gs-user");
        GtkWidget *cm_user = g_object_get_data(G_OBJECT(cm_page), "cm-user");
        if (gs_user && cm_user)
            gtk_entry_set_buffer(GTK_ENTRY(cm_user), gtk_entry_get_buffer(GTK_ENTRY(gs_user)));

        /* Share pop_server buffer: gs-server ↔ cm-server */
        GtkWidget *gs_server = g_object_get_data(G_OBJECT(gs_page), "gs-server");
        GtkWidget *cm_server = g_object_get_data(G_OBJECT(cm_page), "cm-server");
        if (gs_server && cm_server)
            gtk_entry_set_buffer(GTK_ENTRY(cm_server), gtk_entry_get_buffer(GTK_ENTRY(gs_server)));

        /* Share email_address buffer: gs-email ↔ sm-email */
        GtkWidget *gs_email = g_object_get_data(G_OBJECT(gs_page), "gs-email");
        GtkWidget *sm_email = g_object_get_data(G_OBJECT(sm_page), "sm-email");
        if (gs_email && sm_email)
            gtk_entry_set_buffer(GTK_ENTRY(sm_email), gtk_entry_get_buffer(GTK_ENTRY(gs_email)));

        /* Share smtp_server buffer: gs-smtp ↔ sm-smtp */
        GtkWidget *gs_smtp = g_object_get_data(G_OBJECT(gs_page), "gs-smtp");
        GtkWidget *sm_smtp = g_object_get_data(G_OBJECT(sm_page), "sm-smtp");
        if (gs_smtp && sm_smtp)
            gtk_entry_set_buffer(GTK_ENTRY(sm_smtp), gtk_entry_get_buffer(GTK_ENTRY(gs_smtp)));
    }

    gtk_paned_set_end_child(GTK_PANED(hpaned), dialog->content_area);
    gtk_paned_set_resize_end_child(GTK_PANED(hpaned), TRUE);
    gtk_box_append(GTK_BOX(main_vbox), hpaned);

    g_signal_connect(dialog->section_list, "row-selected",
                     G_CALLBACK(on_section_selected), dialog);

    /* Bottom button bar */
    gtk_box_append(GTK_BOX(main_vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    GtkWidget *button_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(button_bar, GTK_ALIGN_END);
    gtk_widget_set_margin_start(button_bar, 12); gtk_widget_set_margin_end(button_bar, 12);
    gtk_widget_set_margin_top(button_bar, 10); gtk_widget_set_margin_bottom(button_bar, 10);
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    GtkWidget *ok_btn = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(ok_btn, "suggested-action");
    gtk_box_append(GTK_BOX(button_bar), cancel_btn);
    gtk_box_append(GTK_BOX(button_bar), ok_btn);
    gtk_box_append(GTK_BOX(main_vbox), button_bar);

    g_signal_connect(ok_btn, "clicked", G_CALLBACK(on_ok_clicked), dialog);
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_cancel_clicked), dialog);
    g_object_set_data(G_OBJECT(dialog->dialog), "settings-dialog", dialog);

    GtkListBoxRow *first = gtk_list_box_get_row_at_index(dialog->section_list, 0);
    if (first) gtk_list_box_select_row(dialog->section_list, first);

    return dialog;
}

/* ── Public API ── */

void show_settings_section(SettingsDialog *dialog, SettingsSection section)
{
    if (!dialog) return;
    for (int i = 0; i < SETTINGS_COUNT; i++)
        gtk_widget_set_visible(dialog->section_widgets[i], (i == section));
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(dialog->section_list, section);
    if (row) gtk_list_box_select_row(dialog->section_list, row);
}

GtkWidget* get_settings_dialog_widget(SettingsDialog *dialog)
{
    return dialog ? dialog->dialog : NULL;
}

AppSettings* get_settings_from_dialog(SettingsDialog *dialog)
{
    if (!dialog || !dialog->settings) return NULL;
    sync_all_pages(dialog);
    return dialog->settings;
}

void free_settings_dialog(SettingsDialog *dialog)
{
    if (!dialog) return;
    if (dialog->settings) g_free(dialog->settings);
    g_free(dialog);
}
