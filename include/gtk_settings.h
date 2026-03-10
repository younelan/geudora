/*
 * GTK4 Settings Dialog for gEudora
 * Multi-section preferences dialog similar to Mac Eudora
 */

#ifndef GTK_SETTINGS_H
#define GTK_SETTINGS_H

#include <gtk/gtk.h>

/* Settings sections */
typedef enum {
    SETTINGS_GETTING_STARTED,
    SETTINGS_CHECKING_MAIL,
    SETTINGS_SENDING_MAIL,
    SETTINGS_ATTACHMENTS,
    SETTINGS_DISPLAY,
    SETTINGS_FONTS,
    SETTINGS_ADVANCED,
    SETTINGS_SECURITY,
    SETTINGS_ACCOUNTS,
    SETTINGS_COUNT
} SettingsSection;

/* Application settings structure */
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
    
    /* Display */
    gboolean show_preview;
    gboolean show_toolbars;
    gboolean zoom_on_open;
    
    /* Advanced */
    gboolean case_sensitive_search;
    gboolean offline_mode;
    gboolean expert_mode;
    
    /* Security */
    gboolean save_password;
    gboolean use_ssl;
    
    /* Fonts */
    char message_font[256];
    int message_font_size;
    char compose_font[256];
    int compose_font_size;
    
    /* Accounts - support multiple personalities */
    char account_name[256];
    char account_type[256];  /* POP, IMAP, etc */
    gboolean account_enabled;
} AppSettings;

/* Account structure for storing multiple accounts */
typedef struct {
    char name[256];
    char email[256];
    char type[256];      /* IMAP, POP, SMTP */
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
    
    /* Widget references for value retrieval */
    void *widgets;  /* DialogWidgets* - opaque pointer to avoid circular includes */
} SettingsDialog;

/* Create settings dialog */
SettingsDialog* create_settings_dialog(GtkWindow *parent, AppSettings *settings);

/* Show specific settings section */
void show_settings_section(SettingsDialog *dialog, SettingsSection section);

/* Get settings dialog widget */
GtkWidget* get_settings_dialog_widget(SettingsDialog *dialog);

/* Get settings from dialog */
AppSettings* get_settings_from_dialog(SettingsDialog *dialog);

/* Free settings dialog */
void free_settings_dialog(SettingsDialog *dialog);

#endif /* GTK_SETTINGS_H */
