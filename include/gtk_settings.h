/*
 * GTK4 Settings Dialog for gEudora
 * Multi-section preferences dialog matching original Mac Eudora 6.2.4
 */

#ifndef GTK_SETTINGS_H
#define GTK_SETTINGS_H

#include <gtk/gtk.h>

/* Settings sections — matches original Mac Eudora panel order */
typedef enum {
    SETTINGS_GETTING_STARTED,
    SETTINGS_CHECKING_MAIL,
    SETTINGS_SENDING_MAIL,
    SETTINGS_COMPOSING,
    SETTINGS_MAILBOX_DISPLAY,
    SETTINGS_DATE_DISPLAY,
    SETTINGS_STYLED_TEXT,
    SETTINGS_FONTS,
    SETTINGS_LABELS,
    SETTINGS_ATTACHMENTS,
    SETTINGS_REPLYING,
    SETTINGS_JUNK_MAIL,
    SETTINGS_TOOLBAR,
    SETTINGS_GETTING_ATTENTION,
    SETTINGS_HOSTS,
    SETTINGS_MOVING_AROUND,
    SETTINGS_EXTRA_WARNINGS,
    SETTINGS_MISCELLANEOUS,
    SETTINGS_ACCOUNTS,
    SETTINGS_SSL,
    SETTINGS_SPELL_CHECKING,
    SETTINGS_COUNT
} SettingsSection;

/* Application settings structure — all preferences from original Eudora */
typedef struct {
    /* Getting Started */
    char pop_username[256];
    char pop_server[256];
    char real_name[256];
    char smtp_server[256];
    char email_address[256];
    gboolean default_mailer;

    /* Checking Mail */
    gboolean use_pop;
    gboolean use_imap;
    gboolean use_passwords;
    gboolean use_kerberos;
    gboolean use_apop;
    gboolean overlap_commands;
    int check_interval;
    gboolean check_battery;
    gboolean send_on_check;
    gboolean leave_on_server;
    int leave_days;
    gboolean delete_from_trash;
    int skip_size_kb;

    /* Sending Mail */
    char default_domain[256];
    gboolean send_immediately;
    gboolean keep_sent_copy;
    gboolean wrap_outgoing;
    gboolean include_signature;
    gboolean use_submission_port;
    gboolean allow_smtp_auth;
    gboolean fix_curly_quotes;
    gboolean auto_fcc_original;

    /* Composing */
    gboolean auto_complete_nicknames;
    gboolean expand_nicknames_immediately;
    gboolean may_use_qp;
    int word_wrap_column;

    /* Mailbox Display */
    gboolean show_col_status;
    gboolean show_col_priority;
    gboolean show_col_attachments;
    gboolean show_col_label;
    gboolean show_col_who;
    gboolean show_col_date;
    gboolean show_col_size;
    gboolean show_col_server;
    gboolean show_col_mood;
    gboolean show_col_junk;
    gboolean draw_horiz_lines;
    gboolean draw_vert_lines;
    gboolean show_selected_count;
    gboolean show_preview;
    int mark_read_delay;
    gboolean mark_read_on_scroll;
    gboolean mark_read_on_delete;

    /* Date Display */
    gboolean date_age_sensitive;
    gboolean date_local_timezone;

    /* Styled Text */
    int styled_send_mode;   /* 0=both, 1=styled only, 2=plain only, 3=ask */
    gboolean styled_bold;
    gboolean styled_italic;
    gboolean styled_underline;
    gboolean styled_color;
    gboolean styled_size;
    gboolean styled_font;
    gboolean styled_margins;
    gboolean show_format_toolbar;

    /* Fonts */
    char screen_font[256];
    int screen_font_size;
    char fixed_font[256];
    int fixed_font_size;
    char print_font[256];
    int print_font_size;
    char message_font[256];
    int message_font_size;
    char compose_font[256];
    int compose_font_size;

    /* Labels */
    char label_names[8][64];
    double label_colors[8][3];  /* RGB 0.0-1.0 */

    /* Attachments */
    int attach_encoding;    /* 0=MIME, 1=BinHex, 2=UUencode */
    char attach_folder[512];
    gboolean trash_attachments_with_msg;
    gboolean receive_digests_as_attach;

    /* Replying */
    gboolean reply_to_all_default;
    gboolean reply_include_self;
    gboolean reply_to_in_cc;
    gboolean copy_original_priority;

    /* Junk Mail */
    int junk_threshold;
    gboolean junk_check_addressbook;
    gboolean junk_hold_mailbox;
    gboolean junk_never_unread;
    int junk_remove_days;
    gboolean junk_warn_remove;

    /* Toolbar */
    gboolean show_toolbar;
    gboolean show_search_field;
    int toolbar_button_style;   /* 0=big+names, 1=big, 2=small+names, 3=small, 4=names */

    /* Getting Attention */
    gboolean alert_on_new;
    gboolean bounce_dock;
    gboolean open_mailbox_on_new;
    gboolean play_sound_on_new;
    char new_mail_sound[256];
    gboolean show_task_progress;

    /* Hosts */
    char ph_server[256];
    char finger_server[256];
    gboolean dns_load_balance;
    gboolean offline_mode;

    /* Moving Around */
    int after_message;  /* 0=nothing, 1=next, 2=next unread, 3=next if unread, 4=same subject */
    gboolean tab_switches_fields;
    gboolean return_switches_fields;

    /* Extra Warnings */
    gboolean warn_delete_unread;
    gboolean warn_delete_queued;
    gboolean warn_delete_unsent;
    gboolean warn_queue_no_subject;
    gboolean warn_queue_styled;
    gboolean warn_quit_queued;
    gboolean warn_empty_trash;
    int warn_send_size_kb;

    /* Miscellaneous */
    gboolean close_msg_with_mailbox;
    gboolean empty_trash_on_quit;
    gboolean turbo_redirect;
    gboolean resort_less_often;
    gboolean use_old_toc;
    gboolean generate_filter_reports;
    gboolean use_keychain;

    /* Display (legacy) */
    gboolean show_toolbars;
    gboolean zoom_on_open;
    gboolean display_graphics;
    gboolean display_emoticons;

    /* Security / SSL */
    gboolean save_password;
    gboolean use_ssl;
    int ssl_pop_mode;       /* 0=off, 1=required, 2=alternate port */
    int ssl_smtp_mode;
    int ssl_imap_mode;

    /* Advanced (legacy) */
    gboolean case_sensitive_search;
    gboolean expert_mode;

    /* Spell Checking */
    gboolean spell_auto_check;
    gboolean spell_warn_on_send;
    gboolean spell_ignore_caps;
    gboolean spell_ignore_mixed_case;
    gboolean spell_ignore_numbers;
    gboolean spell_ignore_all_caps;
    int spell_suggest_mode;  /* 0=look, 1=sound, 2=both, 3=never */

    /* Accounts - support multiple personalities */
    char account_name[256];
    char account_type[256];
    gboolean account_enabled;
} AppSettings;

/* Account structure for storing multiple accounts */
typedef struct {
    char name[256];
    char email[256];
    char type[256];
    char server[256];
    char smtp_server[256];
    char username[256];
    gboolean enabled;
} EmailAccount;

#define MAX_ACCOUNTS 10

/* Settings dialog structure */
typedef struct {
    GtkWidget *dialog;
    GtkWidget *sidebar;
    GtkWidget *content_area;
    GtkWidget *section_widgets[SETTINGS_COUNT];
    GtkListBox *section_list;
    AppSettings *settings;
    void *widgets;  /* DialogWidgets* */
} SettingsDialog;

SettingsDialog* create_settings_dialog(GtkWindow *parent, AppSettings *settings);
void show_settings_section(SettingsDialog *dialog, SettingsSection section);
GtkWidget* get_settings_dialog_widget(SettingsDialog *dialog);
AppSettings* get_settings_from_dialog(SettingsDialog *dialog);
void free_settings_dialog(SettingsDialog *dialog);

#endif /* GTK_SETTINGS_H */
