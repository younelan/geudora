/*
 * GTK4 Preferences System for gEudora
 * GLib-based INI file preferences, similar to Mac prefs.c
 * Uses GKeyFile for cross-platform INI file support
 */

#ifndef GTK_PREFS_H
#define GTK_PREFS_H

#include <glib.h>
#include "gtk_settings.h"

/* Preference groups (sections in INI file) */
#define PREFS_GROUP_GETTING_STARTED "getting_started"
#define PREFS_GROUP_CHECKING_MAIL   "checking_mail"
#define PREFS_GROUP_SENDING_MAIL    "sending_mail"
#define PREFS_GROUP_DISPLAY         "display"
#define PREFS_GROUP_ADVANCED        "advanced"
#define PREFS_GROUP_SECURITY        "security"
#define PREFS_GROUP_FONTS           "fonts"
#define PREFS_GROUP_ACCOUNTS        "accounts"

/* Initialize preferences system */
void prefs_init(const char *config_dir);

/* Load preferences from INI file */
AppSettings* prefs_load(void);

/* Save preferences to INI file */
gboolean prefs_save(AppSettings *settings);

/* Get string preference */
gchar* prefs_get_string(const char *group, const char *key, const char *default_value);

/* Get integer preference */
gint prefs_get_int(const char *group, const char *key, gint default_value);

/* Get boolean preference */
gboolean prefs_get_bool(const char *group, const char *key, gboolean default_value);

/* Set string preference */
void prefs_set_string(const char *group, const char *key, const char *value);

/* Set integer preference */
void prefs_set_int(const char *group, const char *key, gint value);

/* Set boolean preference */
void prefs_set_bool(const char *group, const char *key, gboolean value);

/* Cleanup preferences system */
void prefs_cleanup(void);

/* Get Eudora data directory path */
const char* prefs_get_data_path(void);

/* Get addressbook file path */
const char* prefs_get_addressbook_path(void);

/* Get mailboxes directory path */
const char* prefs_get_mailboxes_path(void);

/* Account management */
typedef struct {
    char name[256];
    char email[256];
    char type[256];
    char server[256];
    char smtp_server[256];
    char username[256];
    gboolean enabled;
} PrefsAccount;

/* Load all accounts from INI file */
int prefs_load_accounts(PrefsAccount *accounts, int max_accounts);

/* Save all accounts to INI file */
void prefs_save_accounts(PrefsAccount *accounts, int num_accounts);

/* Get account by index */
gboolean prefs_get_account(int index, PrefsAccount *account);

/* Set account by index */
void prefs_set_account(int index, PrefsAccount *account);

#endif /* GTK_PREFS_H */
