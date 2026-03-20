/* macmbx_conf.h — INI configuration file + personality management
 * Part of macmbx: standalone mail data management library.
 *
 * Full INI file support: [sections], key=value, # comments.
 * Personality/account management: enumerate, add, remove, get/set.
 * Eudora geudora.ini compatible.
 *
 * No Eudora dependency. Portable C99.
 */

#ifndef MACMBX_CONF_H
#define MACMBX_CONF_H

#include <stddef.h>
#include <stdbool.h>

/* ================================================================
 * INI file types
 * ================================================================ */

typedef struct MacmbxConfEntry {
  char *key;
  char *value;
  char *comment;              /* inline comment, or full-line comment if key==NULL */
  struct MacmbxConfEntry *next;
} MacmbxConfEntry;

typedef struct MacmbxConfSection {
  char *name;                 /* section name (empty string for global) */
  MacmbxConfEntry *entries;
  struct MacmbxConfSection *next;
} MacmbxConfSection;

typedef struct {
  char path[1024];
  MacmbxConfSection *sections;
  bool dirty;
} MacmbxConf;

/* ================================================================
 * Lifecycle
 * ================================================================ */

/* Load INI file. Returns NULL on error. Creates empty conf if file missing. */
MacmbxConf *macmbx_conf_load(const char *path);

/* Save INI file. Preserves comments and section order. Returns 0 on success. */
int macmbx_conf_save(MacmbxConf *conf);

/* Free all memory. Does NOT save. */
void macmbx_conf_free(MacmbxConf *conf);

/* Create an empty config (no file). */
MacmbxConf *macmbx_conf_new(void);

/* ================================================================
 * Get values
 * ================================================================ */

/* Get string value. Returns fallback if not found. */
const char *macmbx_conf_get(MacmbxConf *conf, const char *section,
                              const char *key, const char *fallback);

/* Get integer value. */
int macmbx_conf_get_int(MacmbxConf *conf, const char *section,
                          const char *key, int fallback);

/* Get long value. */
long macmbx_conf_get_long(MacmbxConf *conf, const char *section,
                            const char *key, long fallback);

/* Get boolean value (true/false, yes/no, 1/0). */
bool macmbx_conf_get_bool(MacmbxConf *conf, const char *section,
                            const char *key, bool fallback);

/* ================================================================
 * Set values
 * ================================================================ */

/* Set string value. Creates section/key if not exists. */
int macmbx_conf_set(MacmbxConf *conf, const char *section,
                      const char *key, const char *value);

/* Set integer value. */
int macmbx_conf_set_int(MacmbxConf *conf, const char *section,
                          const char *key, int value);

/* Set long value. */
int macmbx_conf_set_long(MacmbxConf *conf, const char *section,
                           const char *key, long value);

/* Set boolean value. */
int macmbx_conf_set_bool(MacmbxConf *conf, const char *section,
                           const char *key, bool value);

/* ================================================================
 * Section management
 * ================================================================ */

/* Check if a section exists. */
bool macmbx_conf_has_section(MacmbxConf *conf, const char *section);

/* Check if a key exists in a section. */
bool macmbx_conf_has_key(MacmbxConf *conf, const char *section, const char *key);

/* Remove a key from a section. */
int macmbx_conf_remove_key(MacmbxConf *conf, const char *section, const char *key);

/* Remove an entire section. */
int macmbx_conf_remove_section(MacmbxConf *conf, const char *section);

/* List all section names. Returns count, allocates *names. Caller frees each + array. */
int macmbx_conf_list_sections(MacmbxConf *conf, char ***names);

/* List all keys in a section. Returns count, allocates *keys. Caller frees each + array. */
int macmbx_conf_list_keys(MacmbxConf *conf, const char *section, char ***keys);

/* ================================================================
 * Personality / Account management
 *
 * Accounts are stored as [account_1], [account_2], etc.
 * The dominant personality uses global sections
 * (checking_mail, sending_mail, ssl).
 * ================================================================ */

/* Account info — extracted from account_N section */
typedef struct {
  int index;                  /* 1-based: account_1, account_2, ... */
  char name[128];             /* display name */
  char real_name[128];
  char email[256];
  char type[16];              /* "POP" or "IMAP" */
  char server[256];
  char smtp_server[256];
  char username[128];
  int check_interval;
  int ssl_mode;               /* 0=none, 1=SSL, 2=STARTTLS */
  bool leave_on_server;
  bool enabled;
} MacmbxAccount;

/* Get the dominant (main) account info from global sections. */
int macmbx_conf_get_dominant(MacmbxConf *conf, MacmbxAccount *acct);

/* Count personality accounts (account_1, account_2, ...). */
int macmbx_conf_count_accounts(MacmbxConf *conf);

/* Get account by 1-based index. Returns 0 on success. */
int macmbx_conf_get_account(MacmbxConf *conf, int index, MacmbxAccount *acct);

/* Get all accounts. Returns count, allocates *accts. Caller frees. */
int macmbx_conf_get_all_accounts(MacmbxConf *conf, MacmbxAccount **accts);

/* Add a new account. Returns the new index (account_N). */
int macmbx_conf_add_account(MacmbxConf *conf, const MacmbxAccount *acct);

/* Update an existing account. */
int macmbx_conf_update_account(MacmbxConf *conf, int index, const MacmbxAccount *acct);

/* Remove an account. Renumbers remaining accounts. */
int macmbx_conf_remove_account(MacmbxConf *conf, int index);

/* Find account by email address. Returns index or -1. */
int macmbx_conf_find_account_by_email(MacmbxConf *conf, const char *email);

/* Find account by name. Returns index or -1. */
int macmbx_conf_find_account_by_name(MacmbxConf *conf, const char *name);

#endif /* MACMBX_CONF_H */
