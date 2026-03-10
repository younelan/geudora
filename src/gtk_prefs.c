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

/* Load preferences from INI file */
AppSettings* prefs_load(void)
{
    AppSettings *settings = g_new0(AppSettings, 1);
    GError *error = NULL;
    gchar *val;
    
    if (!prefs_state.keyfile) {
        g_warning("Preferences not initialized");
        return settings;
    }
    
    /* Load existing file if it exists */
    if (g_file_test(prefs_state.config_file, G_FILE_TEST_EXISTS)) {
        if (!g_key_file_load_from_file(prefs_state.keyfile, prefs_state.config_file, 
                                       G_KEY_FILE_KEEP_COMMENTS, &error)) {
            g_warning("Failed to load preferences: %s", error->message);
            g_error_free(error);
        } else {
            g_print("Preferences loaded from %s\n", prefs_state.config_file);
        }
    }
    
    /* Load Checking Mail settings (source of truth for user/server) */
    val = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_username", "");
    strncpy(settings->pop_username, val, sizeof(settings->pop_username) - 1);
    g_free(val);
    
    val = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_server", "");
    strncpy(settings->pop_server, val, sizeof(settings->pop_server) - 1);
    g_free(val);
    
    /* Load Sending Mail settings (source of truth for email/smtp/domain) */
    val = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "email_address", "");
    strncpy(settings->email_address, val, sizeof(settings->email_address) - 1);
    g_free(val);
    
    val = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "smtp_server", "");
    strncpy(settings->smtp_server, val, sizeof(settings->smtp_server) - 1);
    g_free(val);
    
    val = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "real_name", "");
    strncpy(settings->real_name, val, sizeof(settings->real_name) - 1);
    g_free(val);
    
    /* Load Checking Mail settings */
    settings->use_pop = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_pop", FALSE);
    settings->use_imap = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_imap", TRUE);  /* Default to IMAP */
    settings->use_passwords = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_passwords", TRUE);
    settings->use_kerberos = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_kerberos", FALSE);
    settings->use_apop = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "use_apop", FALSE);
    settings->overlap_commands = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "overlap_commands", FALSE);
    settings->check_interval = prefs_get_int(PREFS_GROUP_CHECKING_MAIL, "check_interval", 5);
    settings->check_battery = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "check_battery", FALSE);
    settings->send_on_check = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "send_on_check", FALSE);
    settings->leave_on_server = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "leave_on_server", FALSE);
    settings->leave_days = prefs_get_int(PREFS_GROUP_CHECKING_MAIL, "leave_days", 0);
    settings->delete_from_trash = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "delete_from_trash", FALSE);
    settings->skip_size_kb = prefs_get_int(PREFS_GROUP_CHECKING_MAIL, "skip_size_kb", 0);
    
    /* Load Sending Mail settings */
    val = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "default_domain", "");
    strncpy(settings->default_domain, val, sizeof(settings->default_domain) - 1);
    g_free(val);
    
    settings->send_immediately = prefs_get_bool(PREFS_GROUP_SENDING_MAIL, "send_immediately", FALSE);
    settings->keep_sent_copy = prefs_get_bool(PREFS_GROUP_SENDING_MAIL, "keep_sent_copy", TRUE);
    settings->wrap_outgoing = prefs_get_bool(PREFS_GROUP_SENDING_MAIL, "wrap_outgoing", TRUE);
    settings->include_signature = prefs_get_bool(PREFS_GROUP_SENDING_MAIL, "include_signature", TRUE);
    
    /* Load Display settings */
    settings->show_preview = prefs_get_bool(PREFS_GROUP_DISPLAY, "show_preview", TRUE);
    settings->show_toolbars = prefs_get_bool(PREFS_GROUP_DISPLAY, "show_toolbars", TRUE);
    settings->zoom_on_open = prefs_get_bool(PREFS_GROUP_DISPLAY, "zoom_on_open", FALSE);
    
    /* Load Advanced settings */
    settings->case_sensitive_search = prefs_get_bool(PREFS_GROUP_ADVANCED, "case_sensitive_search", FALSE);
    settings->offline_mode = prefs_get_bool(PREFS_GROUP_ADVANCED, "offline_mode", FALSE);
    settings->expert_mode = prefs_get_bool(PREFS_GROUP_ADVANCED, "expert_mode", FALSE);
    
    /* Load Security settings */
    settings->save_password = prefs_get_bool(PREFS_GROUP_SECURITY, "save_password", FALSE);
    settings->use_ssl = prefs_get_bool(PREFS_GROUP_SECURITY, "use_ssl", TRUE);
    
    /* Load Fonts settings */
    val = prefs_get_string(PREFS_GROUP_FONTS, "message_font", "Monospace");
    strncpy(settings->message_font, val, sizeof(settings->message_font) - 1);
    g_free(val);
    settings->message_font_size = prefs_get_int(PREFS_GROUP_FONTS, "message_font_size", 12);
    
    val = prefs_get_string(PREFS_GROUP_FONTS, "compose_font", "Monospace");
    strncpy(settings->compose_font, val, sizeof(settings->compose_font) - 1);
    g_free(val);
    settings->compose_font_size = prefs_get_int(PREFS_GROUP_FONTS, "compose_font_size", 12);
    
    /* Load Accounts settings */
    val = prefs_get_string(PREFS_GROUP_ACCOUNTS, "account_name", "");
    strncpy(settings->account_name, val, sizeof(settings->account_name) - 1);
    g_free(val);
    
    val = prefs_get_string(PREFS_GROUP_ACCOUNTS, "account_type", "IMAP");
    strncpy(settings->account_type, val, sizeof(settings->account_type) - 1);
    g_free(val);
    
    settings->account_enabled = prefs_get_bool(PREFS_GROUP_ACCOUNTS, "account_enabled", TRUE);
    
    return settings;
}

/* Save preferences to INI file */
gboolean prefs_save(AppSettings *settings)
{
    GError *error = NULL;
    gchar *data;
    gsize length;
    
    if (!prefs_state.keyfile || !settings) {
        g_warning("Cannot save preferences: invalid state (keyfile=%p, settings=%p)", prefs_state.keyfile, settings);
        return FALSE;
    }
    
    if (!prefs_state.config_file) {
        g_warning("Cannot save preferences: config_file is NULL");
        return FALSE;
    }
    
    g_print("DEBUG: Saving to %s\n", prefs_state.config_file);
    
    /* Save to Checking Mail section (source of truth for user/server) */
    prefs_set_string(PREFS_GROUP_CHECKING_MAIL, "pop_username", settings->pop_username);
    prefs_set_string(PREFS_GROUP_CHECKING_MAIL, "pop_server", settings->pop_server);
    
    /* Also save to Sending Mail section (source of truth for email/smtp/domain) */
    prefs_set_string(PREFS_GROUP_SENDING_MAIL, "email_address", settings->email_address);
    prefs_set_string(PREFS_GROUP_SENDING_MAIL, "smtp_server", settings->smtp_server);
    prefs_set_string(PREFS_GROUP_SENDING_MAIL, "real_name", settings->real_name);
    
    /* Save Checking Mail settings */
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "use_pop", settings->use_pop);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "use_imap", settings->use_imap);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "use_passwords", settings->use_passwords);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "use_kerberos", settings->use_kerberos);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "use_apop", settings->use_apop);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "overlap_commands", settings->overlap_commands);
    prefs_set_int(PREFS_GROUP_CHECKING_MAIL, "check_interval", settings->check_interval);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "check_battery", settings->check_battery);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "send_on_check", settings->send_on_check);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "leave_on_server", settings->leave_on_server);
    prefs_set_int(PREFS_GROUP_CHECKING_MAIL, "leave_days", settings->leave_days);
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "delete_from_trash", settings->delete_from_trash);
    prefs_set_int(PREFS_GROUP_CHECKING_MAIL, "skip_size_kb", settings->skip_size_kb);
    
    /* Save Sending Mail settings */
    prefs_set_string(PREFS_GROUP_SENDING_MAIL, "default_domain", settings->default_domain);
    prefs_set_bool(PREFS_GROUP_SENDING_MAIL, "send_immediately", settings->send_immediately);
    prefs_set_bool(PREFS_GROUP_SENDING_MAIL, "keep_sent_copy", settings->keep_sent_copy);
    prefs_set_bool(PREFS_GROUP_SENDING_MAIL, "wrap_outgoing", settings->wrap_outgoing);
    prefs_set_bool(PREFS_GROUP_SENDING_MAIL, "include_signature", settings->include_signature);
    
    /* Save Display settings */
    prefs_set_bool(PREFS_GROUP_DISPLAY, "show_preview", settings->show_preview);
    prefs_set_bool(PREFS_GROUP_DISPLAY, "show_toolbars", settings->show_toolbars);
    prefs_set_bool(PREFS_GROUP_DISPLAY, "zoom_on_open", settings->zoom_on_open);
    
    /* Save Advanced settings */
    prefs_set_bool(PREFS_GROUP_ADVANCED, "case_sensitive_search", settings->case_sensitive_search);
    prefs_set_bool(PREFS_GROUP_ADVANCED, "offline_mode", settings->offline_mode);
    prefs_set_bool(PREFS_GROUP_ADVANCED, "expert_mode", settings->expert_mode);
    
    /* Save Security settings */
    prefs_set_bool(PREFS_GROUP_SECURITY, "save_password", settings->save_password);
    prefs_set_bool(PREFS_GROUP_SECURITY, "use_ssl", settings->use_ssl);
    
    /* Save Fonts settings */
    prefs_set_string(PREFS_GROUP_FONTS, "message_font", settings->message_font);
    prefs_set_int(PREFS_GROUP_FONTS, "message_font_size", settings->message_font_size);
    prefs_set_string(PREFS_GROUP_FONTS, "compose_font", settings->compose_font);
    prefs_set_int(PREFS_GROUP_FONTS, "compose_font_size", settings->compose_font_size);
    
    /* Save Accounts settings */
    prefs_set_string(PREFS_GROUP_ACCOUNTS, "account_name", settings->account_name);
    prefs_set_string(PREFS_GROUP_ACCOUNTS, "account_type", settings->account_type);
    prefs_set_bool(PREFS_GROUP_ACCOUNTS, "account_enabled", settings->account_enabled);
    
    /* Write to file */
    data = g_key_file_to_data(prefs_state.keyfile, &length, &error);
    if (!data) {
        g_warning("Failed to convert preferences to data: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    g_print("DEBUG: Writing %zu bytes to %s\n", length, prefs_state.config_file);
    
    if (!g_file_set_contents(prefs_state.config_file, data, length, &error)) {
        g_warning("Failed to save preferences to %s: %s", prefs_state.config_file, error->message);
        g_error_free(error);
        g_free(data);
        return FALSE;
    }
    
    g_free(data);
    g_print("✓ Preferences saved to %s\n", prefs_state.config_file);
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
