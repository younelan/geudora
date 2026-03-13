/*
 * GTK4 Preferences System for gEudora
 * GLib-based INI file preferences, similar to Mac prefs.c
 * Uses GKeyFile for cross-platform INI file support
 */

#include "gtk_prefs.h"
#include <stdio.h>
#include <string.h>

/* Global preferences state */
static struct {
    GKeyFile *keyfile;
    gchar *data_path;      /* ~/.local/share/geudora */
    gchar *config_file;    /* ~/.local/share/geudora/geudora.ini */
    gchar *addressbook_file;
    gchar *mailboxes_dir;
} prefs_state = {NULL, NULL, NULL, NULL, NULL};

/* Initialize preferences system */
void prefs_init(const char *config_dir)
{
    (void)config_dir;  /* Ignore parameter, use standard XDG paths */
    
    /* Use XDG data directory: ~/.local/share/geudora */
    const char *data_home = g_get_user_data_dir();
    prefs_state.data_path = g_build_filename(data_home, "geudora", NULL);
    prefs_state.config_file = g_build_filename(prefs_state.data_path, "geudora.ini", NULL);
    prefs_state.addressbook_file = g_build_filename(prefs_state.data_path, "addressbook.txt", NULL);
    prefs_state.mailboxes_dir = g_build_filename(prefs_state.data_path, "mailboxes", NULL);
    
    g_print("DEBUG: Eudora data path: %s\n", prefs_state.data_path);
    g_print("DEBUG: Config file: %s\n", prefs_state.config_file);
    g_print("DEBUG: Addressbook: %s\n", prefs_state.addressbook_file);
    g_print("DEBUG: Mailboxes dir: %s\n", prefs_state.mailboxes_dir);
    
    /* Create data directory structure if it doesn't exist */
    if (!g_file_test(prefs_state.data_path, G_FILE_TEST_EXISTS)) {
        g_print("Creating Eudora data directory: %s\n", prefs_state.data_path);
        g_mkdir_with_parents(prefs_state.data_path, 0755);
    }
    
    if (!g_file_test(prefs_state.mailboxes_dir, G_FILE_TEST_EXISTS)) {
        g_print("Creating mailboxes directory: %s\n", prefs_state.mailboxes_dir);
        g_mkdir_with_parents(prefs_state.mailboxes_dir, 0755);
    }
    
    /* Initialize keyfile */
    prefs_state.keyfile = g_key_file_new();
    
    g_print("✓ Eudora data initialized at: %s\n", prefs_state.data_path);
}

/* Helper macros for load/save to reduce repetition */
#define LOAD_STR(grp, key, field, def) do { \
    gchar *_v = prefs_get_string(grp, key, def); \
    strncpy(s->field, _v, sizeof(s->field) - 1); \
    g_free(_v); \
} while(0)
#define LOAD_BOOL(grp, key, field, def) s->field = prefs_get_bool(grp, key, def)
#define LOAD_INT(grp, key, field, def)  s->field = prefs_get_int(grp, key, def)

#define SAVE_STR(grp, key, field)  prefs_set_string(grp, key, s->field)
#define SAVE_BOOL(grp, key, field) prefs_set_bool(grp, key, s->field)
#define SAVE_INT(grp, key, field)  prefs_set_int(grp, key, s->field)

/* Load preferences from INI file */
AppSettings* prefs_load(void)
{
    AppSettings *s = g_new0(AppSettings, 1);
    GError *error = NULL;

    if (!prefs_state.keyfile) {
        g_warning("Preferences not initialized");
        return s;
    }

    if (g_file_test(prefs_state.config_file, G_FILE_TEST_EXISTS)) {
        if (!g_key_file_load_from_file(prefs_state.keyfile, prefs_state.config_file,
                                       G_KEY_FILE_KEEP_COMMENTS, &error)) {
            g_warning("Failed to load preferences: %s", error->message);
            g_error_free(error);
        } else {
            g_print("Preferences loaded from %s\n", prefs_state.config_file);
        }
    }

    /* Getting Started */
    LOAD_STR(PREFS_GROUP_CHECKING_MAIL, "pop_username", pop_username, "");
    LOAD_STR(PREFS_GROUP_CHECKING_MAIL, "pop_server", pop_server, "");
    LOAD_STR(PREFS_GROUP_SENDING_MAIL, "real_name", real_name, "");
    LOAD_STR(PREFS_GROUP_SENDING_MAIL, "smtp_server", smtp_server, "");
    LOAD_STR(PREFS_GROUP_SENDING_MAIL, "email_address", email_address, "");
    LOAD_BOOL(PREFS_GROUP_GETTING_STARTED, "default_mailer", default_mailer, FALSE);

    /* Checking Mail */
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_pop", use_pop, FALSE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_imap", use_imap, TRUE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_passwords", use_passwords, TRUE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_kerberos", use_kerberos, FALSE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_apop", use_apop, FALSE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "overlap_commands", overlap_commands, FALSE);
    LOAD_INT(PREFS_GROUP_CHECKING_MAIL, "check_interval", check_interval, 5);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "check_battery", check_battery, FALSE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "send_on_check", send_on_check, FALSE);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "leave_on_server", leave_on_server, FALSE);
    LOAD_INT(PREFS_GROUP_CHECKING_MAIL, "leave_days", leave_days, 0);
    LOAD_BOOL(PREFS_GROUP_CHECKING_MAIL, "delete_from_trash", delete_from_trash, FALSE);
    LOAD_INT(PREFS_GROUP_CHECKING_MAIL, "skip_size_kb", skip_size_kb, 0);

    /* Sending Mail */
    LOAD_STR(PREFS_GROUP_SENDING_MAIL, "default_domain", default_domain, "");
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "send_immediately", send_immediately, FALSE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "keep_sent_copy", keep_sent_copy, TRUE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "wrap_outgoing", wrap_outgoing, TRUE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "include_signature", include_signature, TRUE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "use_submission_port", use_submission_port, FALSE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "allow_smtp_auth", allow_smtp_auth, FALSE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "fix_curly_quotes", fix_curly_quotes, FALSE);
    LOAD_BOOL(PREFS_GROUP_SENDING_MAIL, "auto_fcc_original", auto_fcc_original, FALSE);

    /* Composing */
    LOAD_BOOL(PREFS_GROUP_COMPOSING, "auto_complete_nicknames", auto_complete_nicknames, TRUE);
    LOAD_BOOL(PREFS_GROUP_COMPOSING, "expand_nicknames_immediately", expand_nicknames_immediately, FALSE);
    LOAD_BOOL(PREFS_GROUP_COMPOSING, "may_use_qp", may_use_qp, TRUE);
    LOAD_INT(PREFS_GROUP_COMPOSING, "word_wrap_column", word_wrap_column, 76);

    /* Mailbox Display */
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_status", show_col_status, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_priority", show_col_priority, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_attachments", show_col_attachments, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_label", show_col_label, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_who", show_col_who, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_date", show_col_date, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_size", show_col_size, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_server", show_col_server, FALSE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_mood", show_col_mood, FALSE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_junk", show_col_junk, FALSE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "draw_horiz_lines", draw_horiz_lines, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "draw_vert_lines", draw_vert_lines, FALSE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_selected_count", show_selected_count, TRUE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_preview", show_preview, TRUE);
    LOAD_INT(PREFS_GROUP_MAILBOX_DISPLAY, "mark_read_delay", mark_read_delay, 0);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "mark_read_on_scroll", mark_read_on_scroll, FALSE);
    LOAD_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "mark_read_on_delete", mark_read_on_delete, TRUE);

    /* Date Display */
    LOAD_BOOL(PREFS_GROUP_DATE_DISPLAY, "date_age_sensitive", date_age_sensitive, TRUE);
    LOAD_BOOL(PREFS_GROUP_DATE_DISPLAY, "date_local_timezone", date_local_timezone, TRUE);

    /* Styled Text */
    LOAD_INT(PREFS_GROUP_STYLED_TEXT, "styled_send_mode", styled_send_mode, 0);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_bold", styled_bold, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_italic", styled_italic, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_underline", styled_underline, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_color", styled_color, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_size", styled_size, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_font", styled_font, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_margins", styled_margins, TRUE);
    LOAD_BOOL(PREFS_GROUP_STYLED_TEXT, "show_format_toolbar", show_format_toolbar, TRUE);

    /* Fonts */
    LOAD_STR(PREFS_GROUP_FONTS, "screen_font", screen_font, "Sans");
    LOAD_INT(PREFS_GROUP_FONTS, "screen_font_size", screen_font_size, 12);
    LOAD_STR(PREFS_GROUP_FONTS, "fixed_font", fixed_font, "Monospace");
    LOAD_INT(PREFS_GROUP_FONTS, "fixed_font_size", fixed_font_size, 12);
    LOAD_STR(PREFS_GROUP_FONTS, "print_font", print_font, "Serif");
    LOAD_INT(PREFS_GROUP_FONTS, "print_font_size", print_font_size, 12);
    LOAD_STR(PREFS_GROUP_FONTS, "message_font", message_font, "Monospace");
    LOAD_INT(PREFS_GROUP_FONTS, "message_font_size", message_font_size, 12);
    LOAD_STR(PREFS_GROUP_FONTS, "compose_font", compose_font, "Monospace");
    LOAD_INT(PREFS_GROUP_FONTS, "compose_font_size", compose_font_size, 12);

    /* Labels */
    {
        const char *default_names[] = {"Label 1","Label 2","Label 3","Label 4",
                                       "Label 5","Label 6","Label 7","Label 8"};
        const double default_colors[][3] = {
            {1,0,0},{0,0,1},{0,0.5,0},{1,0.5,0},
            {0.5,0,0.5},{0,0.5,0.5},{0.5,0.5,0},{0.3,0.3,0.3}
        };
        for (int i = 0; i < 8; i++) {
            gchar key[32];
            g_snprintf(key, sizeof(key), "name_%d", i);
            gchar *v = prefs_get_string(PREFS_GROUP_LABELS, key, default_names[i]);
            strncpy(s->label_names[i], v, sizeof(s->label_names[i]) - 1);
            g_free(v);
            g_snprintf(key, sizeof(key), "color_r_%d", i);
            s->label_colors[i][0] = prefs_get_int(PREFS_GROUP_LABELS, key,
                                    (int)(default_colors[i][0]*1000)) / 1000.0;
            g_snprintf(key, sizeof(key), "color_g_%d", i);
            s->label_colors[i][1] = prefs_get_int(PREFS_GROUP_LABELS, key,
                                    (int)(default_colors[i][1]*1000)) / 1000.0;
            g_snprintf(key, sizeof(key), "color_b_%d", i);
            s->label_colors[i][2] = prefs_get_int(PREFS_GROUP_LABELS, key,
                                    (int)(default_colors[i][2]*1000)) / 1000.0;
        }
    }

    /* Attachments */
    LOAD_INT(PREFS_GROUP_ATTACHMENTS, "attach_encoding", attach_encoding, 0);
    LOAD_STR(PREFS_GROUP_ATTACHMENTS, "attach_folder", attach_folder, "");
    LOAD_BOOL(PREFS_GROUP_ATTACHMENTS, "trash_attachments_with_msg", trash_attachments_with_msg, FALSE);
    LOAD_BOOL(PREFS_GROUP_ATTACHMENTS, "receive_digests_as_attach", receive_digests_as_attach, FALSE);

    /* Replying */
    LOAD_BOOL(PREFS_GROUP_REPLYING, "reply_to_all_default", reply_to_all_default, FALSE);
    LOAD_BOOL(PREFS_GROUP_REPLYING, "reply_include_self", reply_include_self, FALSE);
    LOAD_BOOL(PREFS_GROUP_REPLYING, "reply_to_in_cc", reply_to_in_cc, FALSE);
    LOAD_BOOL(PREFS_GROUP_REPLYING, "copy_original_priority", copy_original_priority, FALSE);

    /* Junk Mail */
    LOAD_INT(PREFS_GROUP_JUNK_MAIL, "junk_threshold", junk_threshold, 50);
    LOAD_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_check_addressbook", junk_check_addressbook, TRUE);
    LOAD_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_hold_mailbox", junk_hold_mailbox, TRUE);
    LOAD_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_never_unread", junk_never_unread, FALSE);
    LOAD_INT(PREFS_GROUP_JUNK_MAIL, "junk_remove_days", junk_remove_days, 14);
    LOAD_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_warn_remove", junk_warn_remove, TRUE);

    /* Toolbar */
    LOAD_BOOL(PREFS_GROUP_TOOLBAR, "show_toolbar", show_toolbar, TRUE);
    LOAD_BOOL(PREFS_GROUP_TOOLBAR, "show_search_field", show_search_field, TRUE);
    LOAD_INT(PREFS_GROUP_TOOLBAR, "toolbar_button_style", toolbar_button_style, 0);

    /* Getting Attention */
    LOAD_BOOL(PREFS_GROUP_GETTING_ATTENTION, "alert_on_new", alert_on_new, TRUE);
    LOAD_BOOL(PREFS_GROUP_GETTING_ATTENTION, "bounce_dock", bounce_dock, TRUE);
    LOAD_BOOL(PREFS_GROUP_GETTING_ATTENTION, "open_mailbox_on_new", open_mailbox_on_new, FALSE);
    LOAD_BOOL(PREFS_GROUP_GETTING_ATTENTION, "play_sound_on_new", play_sound_on_new, TRUE);
    LOAD_STR(PREFS_GROUP_GETTING_ATTENTION, "new_mail_sound", new_mail_sound, "");
    LOAD_BOOL(PREFS_GROUP_GETTING_ATTENTION, "show_task_progress", show_task_progress, TRUE);

    /* Hosts */
    LOAD_STR(PREFS_GROUP_HOSTS, "ph_server", ph_server, "");
    LOAD_STR(PREFS_GROUP_HOSTS, "finger_server", finger_server, "");
    LOAD_BOOL(PREFS_GROUP_HOSTS, "dns_load_balance", dns_load_balance, FALSE);
    LOAD_BOOL(PREFS_GROUP_HOSTS, "offline_mode", offline_mode, FALSE);

    /* Moving Around */
    LOAD_INT(PREFS_GROUP_MOVING_AROUND, "after_message", after_message, 1);
    LOAD_BOOL(PREFS_GROUP_MOVING_AROUND, "tab_switches_fields", tab_switches_fields, TRUE);
    LOAD_BOOL(PREFS_GROUP_MOVING_AROUND, "return_switches_fields", return_switches_fields, FALSE);

    /* Extra Warnings */
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_delete_unread", warn_delete_unread, TRUE);
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_delete_queued", warn_delete_queued, TRUE);
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_delete_unsent", warn_delete_unsent, TRUE);
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_queue_no_subject", warn_queue_no_subject, TRUE);
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_queue_styled", warn_queue_styled, FALSE);
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_quit_queued", warn_quit_queued, TRUE);
    LOAD_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_empty_trash", warn_empty_trash, TRUE);
    LOAD_INT(PREFS_GROUP_EXTRA_WARNINGS, "warn_send_size_kb", warn_send_size_kb, 0);

    /* Miscellaneous */
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "close_msg_with_mailbox", close_msg_with_mailbox, TRUE);
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "empty_trash_on_quit", empty_trash_on_quit, FALSE);
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "turbo_redirect", turbo_redirect, FALSE);
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "resort_less_often", resort_less_often, FALSE);
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "use_old_toc", use_old_toc, FALSE);
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "generate_filter_reports", generate_filter_reports, FALSE);
    LOAD_BOOL(PREFS_GROUP_MISCELLANEOUS, "use_keychain", use_keychain, FALSE);

    /* Display (legacy) */
    LOAD_BOOL(PREFS_GROUP_DISPLAY, "show_toolbars", show_toolbars, TRUE);
    LOAD_BOOL(PREFS_GROUP_DISPLAY, "zoom_on_open", zoom_on_open, FALSE);
    LOAD_BOOL(PREFS_GROUP_DISPLAY, "display_graphics", display_graphics, TRUE);
    LOAD_BOOL(PREFS_GROUP_DISPLAY, "display_emoticons", display_emoticons, TRUE);

    /* SSL */
    LOAD_BOOL(PREFS_GROUP_SSL, "save_password", save_password, FALSE);
    LOAD_BOOL(PREFS_GROUP_SSL, "use_ssl", use_ssl, TRUE);
    LOAD_INT(PREFS_GROUP_SSL, "ssl_pop_mode", ssl_pop_mode, 0);
    LOAD_INT(PREFS_GROUP_SSL, "ssl_smtp_mode", ssl_smtp_mode, 0);
    LOAD_INT(PREFS_GROUP_SSL, "ssl_imap_mode", ssl_imap_mode, 0);

    /* Advanced (legacy) */
    LOAD_BOOL(PREFS_GROUP_ADVANCED, "case_sensitive_search", case_sensitive_search, FALSE);
    LOAD_BOOL(PREFS_GROUP_ADVANCED, "expert_mode", expert_mode, FALSE);

    /* Spell Checking */
    LOAD_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_auto_check", spell_auto_check, FALSE);
    LOAD_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_warn_on_send", spell_warn_on_send, TRUE);
    LOAD_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_caps", spell_ignore_caps, FALSE);
    LOAD_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_mixed_case", spell_ignore_mixed_case, FALSE);
    LOAD_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_numbers", spell_ignore_numbers, FALSE);
    LOAD_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_all_caps", spell_ignore_all_caps, FALSE);
    LOAD_INT(PREFS_GROUP_SPELL_CHECKING, "spell_suggest_mode", spell_suggest_mode, 0);

    /* Accounts (legacy single-account fields) */
    LOAD_STR(PREFS_GROUP_ACCOUNTS, "account_name", account_name, "");
    LOAD_STR(PREFS_GROUP_ACCOUNTS, "account_type", account_type, "IMAP");
    LOAD_BOOL(PREFS_GROUP_ACCOUNTS, "account_enabled", account_enabled, TRUE);

    return s;
}

/* Save preferences to INI file */
gboolean prefs_save(AppSettings *s)
{
    GError *error = NULL;
    gchar *data;
    gsize length;

    if (!prefs_state.keyfile || !s) {
        g_warning("Cannot save preferences: invalid state");
        return FALSE;
    }
    if (!prefs_state.config_file) {
        g_warning("Cannot save preferences: config_file is NULL");
        return FALSE;
    }

    /* Getting Started */
    SAVE_STR(PREFS_GROUP_CHECKING_MAIL, "pop_username", pop_username);
    SAVE_STR(PREFS_GROUP_CHECKING_MAIL, "pop_server", pop_server);
    SAVE_STR(PREFS_GROUP_SENDING_MAIL, "real_name", real_name);
    SAVE_STR(PREFS_GROUP_SENDING_MAIL, "smtp_server", smtp_server);
    SAVE_STR(PREFS_GROUP_SENDING_MAIL, "email_address", email_address);
    SAVE_BOOL(PREFS_GROUP_GETTING_STARTED, "default_mailer", default_mailer);

    /* Checking Mail */
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_pop", use_pop);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_imap", use_imap);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_passwords", use_passwords);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_kerberos", use_kerberos);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "use_apop", use_apop);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "overlap_commands", overlap_commands);
    SAVE_INT(PREFS_GROUP_CHECKING_MAIL, "check_interval", check_interval);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "check_battery", check_battery);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "send_on_check", send_on_check);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "leave_on_server", leave_on_server);
    SAVE_INT(PREFS_GROUP_CHECKING_MAIL, "leave_days", leave_days);
    SAVE_BOOL(PREFS_GROUP_CHECKING_MAIL, "delete_from_trash", delete_from_trash);
    SAVE_INT(PREFS_GROUP_CHECKING_MAIL, "skip_size_kb", skip_size_kb);

    /* Sending Mail */
    SAVE_STR(PREFS_GROUP_SENDING_MAIL, "default_domain", default_domain);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "send_immediately", send_immediately);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "keep_sent_copy", keep_sent_copy);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "wrap_outgoing", wrap_outgoing);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "include_signature", include_signature);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "use_submission_port", use_submission_port);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "allow_smtp_auth", allow_smtp_auth);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "fix_curly_quotes", fix_curly_quotes);
    SAVE_BOOL(PREFS_GROUP_SENDING_MAIL, "auto_fcc_original", auto_fcc_original);

    /* Composing */
    SAVE_BOOL(PREFS_GROUP_COMPOSING, "auto_complete_nicknames", auto_complete_nicknames);
    SAVE_BOOL(PREFS_GROUP_COMPOSING, "expand_nicknames_immediately", expand_nicknames_immediately);
    SAVE_BOOL(PREFS_GROUP_COMPOSING, "may_use_qp", may_use_qp);
    SAVE_INT(PREFS_GROUP_COMPOSING, "word_wrap_column", word_wrap_column);

    /* Mailbox Display */
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_status", show_col_status);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_priority", show_col_priority);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_attachments", show_col_attachments);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_label", show_col_label);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_who", show_col_who);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_date", show_col_date);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_size", show_col_size);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_server", show_col_server);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_mood", show_col_mood);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_col_junk", show_col_junk);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "draw_horiz_lines", draw_horiz_lines);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "draw_vert_lines", draw_vert_lines);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_selected_count", show_selected_count);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "show_preview", show_preview);
    SAVE_INT(PREFS_GROUP_MAILBOX_DISPLAY, "mark_read_delay", mark_read_delay);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "mark_read_on_scroll", mark_read_on_scroll);
    SAVE_BOOL(PREFS_GROUP_MAILBOX_DISPLAY, "mark_read_on_delete", mark_read_on_delete);

    /* Date Display */
    SAVE_BOOL(PREFS_GROUP_DATE_DISPLAY, "date_age_sensitive", date_age_sensitive);
    SAVE_BOOL(PREFS_GROUP_DATE_DISPLAY, "date_local_timezone", date_local_timezone);

    /* Styled Text */
    SAVE_INT(PREFS_GROUP_STYLED_TEXT, "styled_send_mode", styled_send_mode);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_bold", styled_bold);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_italic", styled_italic);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_underline", styled_underline);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_color", styled_color);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_size", styled_size);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_font", styled_font);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "styled_margins", styled_margins);
    SAVE_BOOL(PREFS_GROUP_STYLED_TEXT, "show_format_toolbar", show_format_toolbar);

    /* Fonts */
    SAVE_STR(PREFS_GROUP_FONTS, "screen_font", screen_font);
    SAVE_INT(PREFS_GROUP_FONTS, "screen_font_size", screen_font_size);
    SAVE_STR(PREFS_GROUP_FONTS, "fixed_font", fixed_font);
    SAVE_INT(PREFS_GROUP_FONTS, "fixed_font_size", fixed_font_size);
    SAVE_STR(PREFS_GROUP_FONTS, "print_font", print_font);
    SAVE_INT(PREFS_GROUP_FONTS, "print_font_size", print_font_size);
    SAVE_STR(PREFS_GROUP_FONTS, "message_font", message_font);
    SAVE_INT(PREFS_GROUP_FONTS, "message_font_size", message_font_size);
    SAVE_STR(PREFS_GROUP_FONTS, "compose_font", compose_font);
    SAVE_INT(PREFS_GROUP_FONTS, "compose_font_size", compose_font_size);

    /* Labels */
    for (int i = 0; i < 8; i++) {
        gchar key[32];
        g_snprintf(key, sizeof(key), "name_%d", i);
        prefs_set_string(PREFS_GROUP_LABELS, key, s->label_names[i]);
        g_snprintf(key, sizeof(key), "color_r_%d", i);
        prefs_set_int(PREFS_GROUP_LABELS, key, (int)(s->label_colors[i][0] * 1000));
        g_snprintf(key, sizeof(key), "color_g_%d", i);
        prefs_set_int(PREFS_GROUP_LABELS, key, (int)(s->label_colors[i][1] * 1000));
        g_snprintf(key, sizeof(key), "color_b_%d", i);
        prefs_set_int(PREFS_GROUP_LABELS, key, (int)(s->label_colors[i][2] * 1000));
    }

    /* Attachments */
    SAVE_INT(PREFS_GROUP_ATTACHMENTS, "attach_encoding", attach_encoding);
    SAVE_STR(PREFS_GROUP_ATTACHMENTS, "attach_folder", attach_folder);
    SAVE_BOOL(PREFS_GROUP_ATTACHMENTS, "trash_attachments_with_msg", trash_attachments_with_msg);
    SAVE_BOOL(PREFS_GROUP_ATTACHMENTS, "receive_digests_as_attach", receive_digests_as_attach);

    /* Replying */
    SAVE_BOOL(PREFS_GROUP_REPLYING, "reply_to_all_default", reply_to_all_default);
    SAVE_BOOL(PREFS_GROUP_REPLYING, "reply_include_self", reply_include_self);
    SAVE_BOOL(PREFS_GROUP_REPLYING, "reply_to_in_cc", reply_to_in_cc);
    SAVE_BOOL(PREFS_GROUP_REPLYING, "copy_original_priority", copy_original_priority);

    /* Junk Mail */
    SAVE_INT(PREFS_GROUP_JUNK_MAIL, "junk_threshold", junk_threshold);
    SAVE_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_check_addressbook", junk_check_addressbook);
    SAVE_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_hold_mailbox", junk_hold_mailbox);
    SAVE_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_never_unread", junk_never_unread);
    SAVE_INT(PREFS_GROUP_JUNK_MAIL, "junk_remove_days", junk_remove_days);
    SAVE_BOOL(PREFS_GROUP_JUNK_MAIL, "junk_warn_remove", junk_warn_remove);

    /* Toolbar */
    SAVE_BOOL(PREFS_GROUP_TOOLBAR, "show_toolbar", show_toolbar);
    SAVE_BOOL(PREFS_GROUP_TOOLBAR, "show_search_field", show_search_field);
    SAVE_INT(PREFS_GROUP_TOOLBAR, "toolbar_button_style", toolbar_button_style);

    /* Getting Attention */
    SAVE_BOOL(PREFS_GROUP_GETTING_ATTENTION, "alert_on_new", alert_on_new);
    SAVE_BOOL(PREFS_GROUP_GETTING_ATTENTION, "bounce_dock", bounce_dock);
    SAVE_BOOL(PREFS_GROUP_GETTING_ATTENTION, "open_mailbox_on_new", open_mailbox_on_new);
    SAVE_BOOL(PREFS_GROUP_GETTING_ATTENTION, "play_sound_on_new", play_sound_on_new);
    SAVE_STR(PREFS_GROUP_GETTING_ATTENTION, "new_mail_sound", new_mail_sound);
    SAVE_BOOL(PREFS_GROUP_GETTING_ATTENTION, "show_task_progress", show_task_progress);

    /* Hosts */
    SAVE_STR(PREFS_GROUP_HOSTS, "ph_server", ph_server);
    SAVE_STR(PREFS_GROUP_HOSTS, "finger_server", finger_server);
    SAVE_BOOL(PREFS_GROUP_HOSTS, "dns_load_balance", dns_load_balance);
    SAVE_BOOL(PREFS_GROUP_HOSTS, "offline_mode", offline_mode);

    /* Moving Around */
    SAVE_INT(PREFS_GROUP_MOVING_AROUND, "after_message", after_message);
    SAVE_BOOL(PREFS_GROUP_MOVING_AROUND, "tab_switches_fields", tab_switches_fields);
    SAVE_BOOL(PREFS_GROUP_MOVING_AROUND, "return_switches_fields", return_switches_fields);

    /* Extra Warnings */
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_delete_unread", warn_delete_unread);
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_delete_queued", warn_delete_queued);
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_delete_unsent", warn_delete_unsent);
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_queue_no_subject", warn_queue_no_subject);
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_queue_styled", warn_queue_styled);
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_quit_queued", warn_quit_queued);
    SAVE_BOOL(PREFS_GROUP_EXTRA_WARNINGS, "warn_empty_trash", warn_empty_trash);
    SAVE_INT(PREFS_GROUP_EXTRA_WARNINGS, "warn_send_size_kb", warn_send_size_kb);

    /* Miscellaneous */
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "close_msg_with_mailbox", close_msg_with_mailbox);
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "empty_trash_on_quit", empty_trash_on_quit);
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "turbo_redirect", turbo_redirect);
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "resort_less_often", resort_less_often);
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "use_old_toc", use_old_toc);
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "generate_filter_reports", generate_filter_reports);
    SAVE_BOOL(PREFS_GROUP_MISCELLANEOUS, "use_keychain", use_keychain);

    /* Display (legacy) */
    SAVE_BOOL(PREFS_GROUP_DISPLAY, "show_toolbars", show_toolbars);
    SAVE_BOOL(PREFS_GROUP_DISPLAY, "zoom_on_open", zoom_on_open);
    SAVE_BOOL(PREFS_GROUP_DISPLAY, "display_graphics", display_graphics);
    SAVE_BOOL(PREFS_GROUP_DISPLAY, "display_emoticons", display_emoticons);

    /* SSL */
    SAVE_BOOL(PREFS_GROUP_SSL, "save_password", save_password);
    SAVE_BOOL(PREFS_GROUP_SSL, "use_ssl", use_ssl);
    SAVE_INT(PREFS_GROUP_SSL, "ssl_pop_mode", ssl_pop_mode);
    SAVE_INT(PREFS_GROUP_SSL, "ssl_smtp_mode", ssl_smtp_mode);
    SAVE_INT(PREFS_GROUP_SSL, "ssl_imap_mode", ssl_imap_mode);

    /* Advanced (legacy) */
    SAVE_BOOL(PREFS_GROUP_ADVANCED, "case_sensitive_search", case_sensitive_search);
    SAVE_BOOL(PREFS_GROUP_ADVANCED, "expert_mode", expert_mode);

    /* Spell Checking */
    SAVE_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_auto_check", spell_auto_check);
    SAVE_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_warn_on_send", spell_warn_on_send);
    SAVE_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_caps", spell_ignore_caps);
    SAVE_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_mixed_case", spell_ignore_mixed_case);
    SAVE_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_numbers", spell_ignore_numbers);
    SAVE_BOOL(PREFS_GROUP_SPELL_CHECKING, "spell_ignore_all_caps", spell_ignore_all_caps);
    SAVE_INT(PREFS_GROUP_SPELL_CHECKING, "spell_suggest_mode", spell_suggest_mode);

    /* Accounts (legacy single-account fields) */
    SAVE_STR(PREFS_GROUP_ACCOUNTS, "account_name", account_name);
    SAVE_STR(PREFS_GROUP_ACCOUNTS, "account_type", account_type);
    SAVE_BOOL(PREFS_GROUP_ACCOUNTS, "account_enabled", account_enabled);

    g_print("DEBUG prefs_save: pop_username='%s' pop_server='%s' smtp='%s' email='%s' real_name='%s'\n",
            s->pop_username, s->pop_server, s->smtp_server, s->email_address, s->real_name);

    /* Write to file */
    data = g_key_file_to_data(prefs_state.keyfile, &length, &error);
    if (!data) {
        g_warning("Failed to serialize preferences: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    if (!g_file_set_contents(prefs_state.config_file, data, length, &error)) {
        g_warning("Failed to save preferences to %s: %s", prefs_state.config_file, error->message);
        g_error_free(error);
        g_free(data);
        return FALSE;
    }

    g_free(data);
    g_print("Preferences saved to %s\n", prefs_state.config_file);
    return TRUE;
}

/* Get string preference */
gchar* prefs_get_string(const char *group, const char *key, const char *default_value)
{
    GError *error = NULL;
    gchar *value;
    
    if (!prefs_state.keyfile) {
        return g_strdup(default_value ? default_value : "");
    }
    
    value = g_key_file_get_string(prefs_state.keyfile, group, key, &error);
    if (error) {
        g_error_free(error);
        return g_strdup(default_value ? default_value : "");
    }
    
    return value ? value : g_strdup(default_value ? default_value : "");
}

/* Get integer preference */
gint prefs_get_int(const char *group, const char *key, gint default_value)
{
    GError *error = NULL;
    gint value;
    
    if (!prefs_state.keyfile) {
        return default_value;
    }
    
    value = g_key_file_get_integer(prefs_state.keyfile, group, key, &error);
    if (error) {
        g_error_free(error);
        return default_value;
    }
    
    return value;
}

/* Get boolean preference */
gboolean prefs_get_bool(const char *group, const char *key, gboolean default_value)
{
    GError *error = NULL;
    gboolean value;
    
    if (!prefs_state.keyfile) {
        return default_value;
    }
    
    value = g_key_file_get_boolean(prefs_state.keyfile, group, key, &error);
    if (error) {
        g_error_free(error);
        return default_value;
    }
    
    return value;
}

/* Set string preference */
void prefs_set_string(const char *group, const char *key, const char *value)
{
    if (!prefs_state.keyfile) {
        return;
    }
    
    g_key_file_set_string(prefs_state.keyfile, group, key, value ? value : "");
}

/* Set integer preference */
void prefs_set_int(const char *group, const char *key, gint value)
{
    if (!prefs_state.keyfile) {
        return;
    }
    
    g_key_file_set_integer(prefs_state.keyfile, group, key, value);
}

/* Set boolean preference */
void prefs_set_bool(const char *group, const char *key, gboolean value)
{
    if (!prefs_state.keyfile) {
        return;
    }
    
    g_key_file_set_boolean(prefs_state.keyfile, group, key, value);
}

/* Flush in-memory preferences to disk */
void prefs_flush(void)
{
    if (!prefs_state.keyfile || !prefs_state.config_file) return;
    GError *error = NULL;
    gsize length;
    gchar *data = g_key_file_to_data(prefs_state.keyfile, &length, &error);
    if (!data) {
        if (error) g_error_free(error);
        return;
    }
    g_file_set_contents(prefs_state.config_file, data, length, &error);
    if (error) g_error_free(error);
    g_free(data);
}

/* Cleanup preferences system */
void prefs_cleanup(void)
{
    if (prefs_state.keyfile) {
        g_key_file_free(prefs_state.keyfile);
        prefs_state.keyfile = NULL;
    }
    
    if (prefs_state.data_path) {
        g_free(prefs_state.data_path);
        prefs_state.data_path = NULL;
    }
    
    if (prefs_state.config_file) {
        g_free(prefs_state.config_file);
        prefs_state.config_file = NULL;
    }
    
    if (prefs_state.addressbook_file) {
        g_free(prefs_state.addressbook_file);
        prefs_state.addressbook_file = NULL;
    }
    
    if (prefs_state.mailboxes_dir) {
        g_free(prefs_state.mailboxes_dir);
        prefs_state.mailboxes_dir = NULL;
    }
}

/* Get Eudora data directory path */
const char* prefs_get_data_path(void)
{
    return prefs_state.data_path;
}

/* Get addressbook file path */
const char* prefs_get_addressbook_path(void)
{
    return prefs_state.addressbook_file;
}

/* Get mailboxes directory path */
const char* prefs_get_mailboxes_path(void)
{
    return prefs_state.mailboxes_dir;
}

/* Load all accounts from INI file */
int prefs_load_accounts(PrefsAccount *accounts, int max_accounts)
{
    int count = 0;
    
    if (!prefs_state.keyfile || !accounts) {
        return 0;
    }
    
    for (int i = 0; i < max_accounts; i++) {
        gchar *group = g_strdup_printf("account_%d", i);
        
        if (!g_key_file_has_group(prefs_state.keyfile, group)) {
            g_free(group);
            break;
        }
        
        gchar *val = prefs_get_string(group, "name", "");
        strncpy(accounts[i].name, val, sizeof(accounts[i].name) - 1);
        g_free(val);
        
        val = prefs_get_string(group, "email", "");
        strncpy(accounts[i].email, val, sizeof(accounts[i].email) - 1);
        g_free(val);
        
        val = prefs_get_string(group, "type", "IMAP");
        strncpy(accounts[i].type, val, sizeof(accounts[i].type) - 1);
        g_free(val);
        
        val = prefs_get_string(group, "server", "");
        strncpy(accounts[i].server, val, sizeof(accounts[i].server) - 1);
        g_free(val);
        
        val = prefs_get_string(group, "smtp_server", "");
        strncpy(accounts[i].smtp_server, val, sizeof(accounts[i].smtp_server) - 1);
        g_free(val);
        
        val = prefs_get_string(group, "username", "");
        strncpy(accounts[i].username, val, sizeof(accounts[i].username) - 1);
        g_free(val);
        
        accounts[i].enabled = prefs_get_bool(group, "enabled", TRUE);
        
        count++;
        g_free(group);
    }
    
    return count;
}

/* Save all accounts to INI file */
void prefs_save_accounts(PrefsAccount *accounts, int num_accounts)
{
    if (!prefs_state.keyfile || !accounts) {
        return;
    }
    
    /* Clear old accounts */
    for (int i = 0; i < 100; i++) {
        gchar *group = g_strdup_printf("account_%d", i);
        if (g_key_file_has_group(prefs_state.keyfile, group)) {
            g_key_file_remove_group(prefs_state.keyfile, group, NULL);
        }
        g_free(group);
    }
    
    /* Save new accounts */
    for (int i = 0; i < num_accounts; i++) {
        gchar *group = g_strdup_printf("account_%d", i);
        
        prefs_set_string(group, "name", accounts[i].name);
        prefs_set_string(group, "email", accounts[i].email);
        prefs_set_string(group, "type", accounts[i].type);
        prefs_set_string(group, "server", accounts[i].server);
        prefs_set_string(group, "smtp_server", accounts[i].smtp_server);
        prefs_set_string(group, "username", accounts[i].username);
        prefs_set_bool(group, "enabled", accounts[i].enabled);
        
        g_free(group);
    }
}

/* Get account by index */
gboolean prefs_get_account(int index, PrefsAccount *account)
{
    if (!prefs_state.keyfile || !account) {
        return FALSE;
    }
    
    gchar *group = g_strdup_printf("account_%d", index);
    
    if (!g_key_file_has_group(prefs_state.keyfile, group)) {
        g_free(group);
        return FALSE;
    }
    
    gchar *val = prefs_get_string(group, "name", "");
    strncpy(account->name, val, sizeof(account->name) - 1);
    g_free(val);
    
    val = prefs_get_string(group, "email", "");
    strncpy(account->email, val, sizeof(account->email) - 1);
    g_free(val);
    
    val = prefs_get_string(group, "type", "IMAP");
    strncpy(account->type, val, sizeof(account->type) - 1);
    g_free(val);
    
    val = prefs_get_string(group, "server", "");
    strncpy(account->server, val, sizeof(account->server) - 1);
    g_free(val);
    
    val = prefs_get_string(group, "smtp_server", "");
    strncpy(account->smtp_server, val, sizeof(account->smtp_server) - 1);
    g_free(val);
    
    val = prefs_get_string(group, "username", "");
    strncpy(account->username, val, sizeof(account->username) - 1);
    g_free(val);
    
    account->enabled = prefs_get_bool(group, "enabled", TRUE);
    
    g_free(group);
    return TRUE;
}

/* Set account by index */
void prefs_set_account(int index, PrefsAccount *account)
{
    if (!prefs_state.keyfile || !account) {
        return;
    }
    
    gchar *group = g_strdup_printf("account_%d", index);
    
    prefs_set_string(group, "name", account->name);
    prefs_set_string(group, "email", account->email);
    prefs_set_string(group, "type", account->type);
    prefs_set_string(group, "server", account->server);
    prefs_set_string(group, "smtp_server", account->smtp_server);
    prefs_set_string(group, "username", account->username);
    prefs_set_bool(group, "enabled", account->enabled);
    
    g_free(group);
}
