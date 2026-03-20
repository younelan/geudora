/* macmbx_conf.c — INI configuration file + personality management
 * Part of macmbx: standalone mail data management library.
 */

#include "macmbx_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* ================================================================
 * Internal helpers
 * ================================================================ */

static char *my_strdup(const char *s) { return s ? strdup(s) : NULL; }

static char *trim(char *s) {
  if (!s) return s;
  while (*s == ' ' || *s == '\t') s++;
  char *e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) e--;
  *e = '\0';
  return s;
}

static MacmbxConfSection *find_section(MacmbxConf *conf, const char *name) {
  for (MacmbxConfSection *s = conf->sections; s; s = s->next)
    if (strcasecmp(s->name, name) == 0) return s;
  return NULL;
}

static MacmbxConfSection *ensure_section(MacmbxConf *conf, const char *name) {
  MacmbxConfSection *s = find_section(conf, name);
  if (s) return s;
  s = (MacmbxConfSection *)calloc(1, sizeof(MacmbxConfSection));
  if (!s) return NULL;
  s->name = my_strdup(name);
  /* Append at end */
  if (!conf->sections) { conf->sections = s; }
  else {
    MacmbxConfSection *tail = conf->sections;
    while (tail->next) tail = tail->next;
    tail->next = s;
  }
  return s;
}

static MacmbxConfEntry *find_entry(MacmbxConfSection *sec, const char *key) {
  for (MacmbxConfEntry *e = sec->entries; e; e = e->next)
    if (e->key && strcasecmp(e->key, key) == 0) return e;
  return NULL;
}

static MacmbxConfEntry *ensure_entry(MacmbxConfSection *sec, const char *key) {
  MacmbxConfEntry *e = find_entry(sec, key);
  if (e) return e;
  e = (MacmbxConfEntry *)calloc(1, sizeof(MacmbxConfEntry));
  if (!e) return NULL;
  e->key = my_strdup(key);
  /* Append */
  if (!sec->entries) { sec->entries = e; }
  else {
    MacmbxConfEntry *tail = sec->entries;
    while (tail->next) tail = tail->next;
    tail->next = e;
  }
  return e;
}

/* ================================================================
 * Lifecycle
 * ================================================================ */

MacmbxConf *macmbx_conf_new(void) {
  MacmbxConf *conf = (MacmbxConf *)calloc(1, sizeof(MacmbxConf));
  return conf;
}

MacmbxConf *macmbx_conf_load(const char *path) {
  MacmbxConf *conf = macmbx_conf_new();
  if (!conf) return NULL;
  snprintf(conf->path, sizeof(conf->path), "%s", path);

  FILE *f = fopen(path, "r");
  if (!f) return conf; /* empty config if file missing */

  MacmbxConfSection *cur_sec = ensure_section(conf, ""); /* global section */
  char line[4096];

  while (fgets(line, sizeof(line), f)) {
    /* Strip trailing whitespace/newline */
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n' ||
                        line[len-1] == ' ' || line[len-1] == '\t'))
      line[--len] = '\0';

    /* Skip empty lines */
    char *p = line;
    while (*p == ' ' || *p == '\t') p++;

    /* Comment line */
    if (*p == '#' || *p == ';') {
      MacmbxConfEntry *e = (MacmbxConfEntry *)calloc(1, sizeof(MacmbxConfEntry));
      if (e) {
        e->comment = my_strdup(line);
        if (!cur_sec->entries) cur_sec->entries = e;
        else {
          MacmbxConfEntry *tail = cur_sec->entries;
          while (tail->next) tail = tail->next;
          tail->next = e;
        }
      }
      continue;
    }

    /* Empty line — preserve as empty comment */
    if (!*p) {
      MacmbxConfEntry *e = (MacmbxConfEntry *)calloc(1, sizeof(MacmbxConfEntry));
      if (e) {
        e->comment = my_strdup("");
        if (!cur_sec->entries) cur_sec->entries = e;
        else {
          MacmbxConfEntry *tail = cur_sec->entries;
          while (tail->next) tail = tail->next;
          tail->next = e;
        }
      }
      continue;
    }

    /* Section header */
    if (*p == '[') {
      p++;
      char *end = strchr(p, ']');
      if (end) *end = '\0';
      cur_sec = ensure_section(conf, trim(p));
      continue;
    }

    /* Key = value */
    char *eq = strchr(p, '=');
    if (eq) {
      *eq = '\0';
      char *key = trim(p);
      char *val = trim(eq + 1);
      MacmbxConfEntry *e = ensure_entry(cur_sec, key);
      if (e) {
        free(e->value);
        e->value = my_strdup(val);
      }
    }
  }
  fclose(f);
  return conf;
}

int macmbx_conf_save(MacmbxConf *conf) {
  if (!conf || !conf->path[0]) return -1;

  char tmp[1088];
  snprintf(tmp, sizeof(tmp), "%s.tmp", conf->path);
  FILE *f = fopen(tmp, "w");
  if (!f) return -1;

  for (MacmbxConfSection *s = conf->sections; s; s = s->next) {
    /* Write section header (skip global empty section) */
    if (s->name[0])
      fprintf(f, "[%s]\n", s->name);

    for (MacmbxConfEntry *e = s->entries; e; e = e->next) {
      if (!e->key) {
        /* Comment or blank line */
        if (e->comment && e->comment[0])
          fprintf(f, "%s\n", e->comment);
        else
          fprintf(f, "\n");
      } else {
        fprintf(f, "%s=%s\n", e->key, e->value ? e->value : "");
      }
    }
    if (s->next) fprintf(f, "\n");
  }

  fclose(f);
  if (rename(tmp, conf->path) != 0) { remove(tmp); return -1; }
  conf->dirty = false;
  return 0;
}

void macmbx_conf_free(MacmbxConf *conf) {
  if (!conf) return;
  MacmbxConfSection *s = conf->sections;
  while (s) {
    MacmbxConfSection *ns = s->next;
    MacmbxConfEntry *e = s->entries;
    while (e) {
      MacmbxConfEntry *ne = e->next;
      free(e->key); free(e->value); free(e->comment); free(e);
      e = ne;
    }
    free(s->name); free(s);
    s = ns;
  }
  free(conf);
}

/* ================================================================
 * Get values
 * ================================================================ */

const char *macmbx_conf_get(MacmbxConf *conf, const char *section,
                              const char *key, const char *fallback) {
  if (!conf || !key) return fallback;
  MacmbxConfSection *s = find_section(conf, section ? section : "");
  if (!s) return fallback;
  MacmbxConfEntry *e = find_entry(s, key);
  return (e && e->value) ? e->value : fallback;
}

int macmbx_conf_get_int(MacmbxConf *conf, const char *section,
                          const char *key, int fallback) {
  const char *v = macmbx_conf_get(conf, section, key, NULL);
  return v ? atoi(v) : fallback;
}

long macmbx_conf_get_long(MacmbxConf *conf, const char *section,
                            const char *key, long fallback) {
  const char *v = macmbx_conf_get(conf, section, key, NULL);
  return v ? atol(v) : fallback;
}

bool macmbx_conf_get_bool(MacmbxConf *conf, const char *section,
                            const char *key, bool fallback) {
  const char *v = macmbx_conf_get(conf, section, key, NULL);
  if (!v) return fallback;
  return strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || strcmp(v, "1") == 0;
}

/* ================================================================
 * Set values
 * ================================================================ */

int macmbx_conf_set(MacmbxConf *conf, const char *section,
                      const char *key, const char *value) {
  if (!conf || !key) return -1;
  MacmbxConfSection *s = ensure_section(conf, section ? section : "");
  if (!s) return -1;
  MacmbxConfEntry *e = ensure_entry(s, key);
  if (!e) return -1;
  free(e->value);
  e->value = my_strdup(value);
  conf->dirty = true;
  return 0;
}

int macmbx_conf_set_int(MacmbxConf *conf, const char *section,
                          const char *key, int value) {
  char buf[32]; snprintf(buf, sizeof(buf), "%d", value);
  return macmbx_conf_set(conf, section, key, buf);
}

int macmbx_conf_set_long(MacmbxConf *conf, const char *section,
                           const char *key, long value) {
  char buf[32]; snprintf(buf, sizeof(buf), "%ld", value);
  return macmbx_conf_set(conf, section, key, buf);
}

int macmbx_conf_set_bool(MacmbxConf *conf, const char *section,
                           const char *key, bool value) {
  return macmbx_conf_set(conf, section, key, value ? "true" : "false");
}

/* ================================================================
 * Section management
 * ================================================================ */

bool macmbx_conf_has_section(MacmbxConf *conf, const char *section) {
  return conf && find_section(conf, section ? section : "") != NULL;
}

bool macmbx_conf_has_key(MacmbxConf *conf, const char *section, const char *key) {
  if (!conf || !key) return false;
  MacmbxConfSection *s = find_section(conf, section ? section : "");
  return s && find_entry(s, key) != NULL;
}

int macmbx_conf_remove_key(MacmbxConf *conf, const char *section, const char *key) {
  if (!conf || !key) return -1;
  MacmbxConfSection *s = find_section(conf, section ? section : "");
  if (!s) return -1;
  MacmbxConfEntry **pp = &s->entries;
  while (*pp) {
    if ((*pp)->key && strcasecmp((*pp)->key, key) == 0) {
      MacmbxConfEntry *e = *pp;
      *pp = e->next;
      free(e->key); free(e->value); free(e->comment); free(e);
      conf->dirty = true;
      return 0;
    }
    pp = &(*pp)->next;
  }
  return -1;
}

int macmbx_conf_remove_section(MacmbxConf *conf, const char *section) {
  if (!conf || !section) return -1;
  MacmbxConfSection **pp = &conf->sections;
  while (*pp) {
    if (strcasecmp((*pp)->name, section) == 0) {
      MacmbxConfSection *s = *pp;
      *pp = s->next;
      MacmbxConfEntry *e = s->entries;
      while (e) { MacmbxConfEntry *ne = e->next; free(e->key); free(e->value); free(e->comment); free(e); e = ne; }
      free(s->name); free(s);
      conf->dirty = true;
      return 0;
    }
    pp = &(*pp)->next;
  }
  return -1;
}

int macmbx_conf_list_sections(MacmbxConf *conf, char ***names) {
  if (!conf || !names) return 0;
  *names = NULL;
  int count = 0;
  for (MacmbxConfSection *s = conf->sections; s; s = s->next)
    if (s->name[0]) count++;
  *names = (char **)calloc(count, sizeof(char *));
  int i = 0;
  for (MacmbxConfSection *s = conf->sections; s; s = s->next)
    if (s->name[0]) (*names)[i++] = strdup(s->name);
  return count;
}

int macmbx_conf_list_keys(MacmbxConf *conf, const char *section, char ***keys) {
  if (!conf || !keys) return 0;
  *keys = NULL;
  MacmbxConfSection *s = find_section(conf, section ? section : "");
  if (!s) return 0;
  int count = 0;
  for (MacmbxConfEntry *e = s->entries; e; e = e->next)
    if (e->key) count++;
  *keys = (char **)calloc(count, sizeof(char *));
  int i = 0;
  for (MacmbxConfEntry *e = s->entries; e; e = e->next)
    if (e->key) (*keys)[i++] = strdup(e->key);
  return count;
}

/* ================================================================
 * Personality / Account management
 * ================================================================ */

int macmbx_conf_get_dominant(MacmbxConf *conf, MacmbxAccount *acct) {
  if (!conf || !acct) return -1;
  memset(acct, 0, sizeof(*acct));
  acct->index = 0;
  snprintf(acct->name, sizeof(acct->name), "%s",
    macmbx_conf_get(conf, "sending_mail", "real_name", ""));
  snprintf(acct->real_name, sizeof(acct->real_name), "%s", acct->name);
  snprintf(acct->email, sizeof(acct->email), "%s",
    macmbx_conf_get(conf, "sending_mail", "email_address", ""));
  snprintf(acct->server, sizeof(acct->server), "%s",
    macmbx_conf_get(conf, "checking_mail", "pop_server", ""));
  snprintf(acct->smtp_server, sizeof(acct->smtp_server), "%s",
    macmbx_conf_get(conf, "sending_mail", "smtp_server", ""));
  snprintf(acct->username, sizeof(acct->username), "%s",
    macmbx_conf_get(conf, "checking_mail", "pop_username", ""));
  bool use_imap = macmbx_conf_get_bool(conf, "checking_mail", "use_imap", false);
  snprintf(acct->type, sizeof(acct->type), "%s", use_imap ? "IMAP" : "POP");
  acct->check_interval = macmbx_conf_get_int(conf, "checking_mail", "check_interval", 5);
  acct->ssl_mode = macmbx_conf_get_int(conf, "ssl", "ssl_pop_mode", 0);
  acct->leave_on_server = macmbx_conf_get_bool(conf, "checking_mail", "leave_on_server", false);
  acct->enabled = true;
  return 0;
}

int macmbx_conf_count_accounts(MacmbxConf *conf) {
  if (!conf) return 0;
  int count = 0;
  for (int i = 1; i <= 100; i++) {
    char sec[32]; snprintf(sec, sizeof(sec), "account_%d", i);
    if (macmbx_conf_has_section(conf, sec)) count++;
    else break;
  }
  return count;
}

int macmbx_conf_get_account(MacmbxConf *conf, int index, MacmbxAccount *acct) {
  if (!conf || !acct || index < 1) return -1;
  char sec[32]; snprintf(sec, sizeof(sec), "account_%d", index);
  if (!macmbx_conf_has_section(conf, sec)) return -1;

  memset(acct, 0, sizeof(*acct));
  acct->index = index;
  snprintf(acct->name, sizeof(acct->name), "%s",
    macmbx_conf_get(conf, sec, "name", ""));
  snprintf(acct->real_name, sizeof(acct->real_name), "%s",
    macmbx_conf_get(conf, sec, "real_name", acct->name));
  snprintf(acct->email, sizeof(acct->email), "%s",
    macmbx_conf_get(conf, sec, "email", ""));
  snprintf(acct->type, sizeof(acct->type), "%s",
    macmbx_conf_get(conf, sec, "type", "POP"));
  snprintf(acct->server, sizeof(acct->server), "%s",
    macmbx_conf_get(conf, sec, "server", ""));
  snprintf(acct->smtp_server, sizeof(acct->smtp_server), "%s",
    macmbx_conf_get(conf, sec, "smtp_server", ""));
  snprintf(acct->username, sizeof(acct->username), "%s",
    macmbx_conf_get(conf, sec, "username", ""));
  acct->check_interval = macmbx_conf_get_int(conf, sec, "check_interval", 5);
  acct->ssl_mode = macmbx_conf_get_int(conf, sec, "ssl_mode", 0);
  acct->leave_on_server = macmbx_conf_get_bool(conf, sec, "leave_on_server", false);
  acct->enabled = macmbx_conf_get_bool(conf, sec, "enabled", true);
  return 0;
}

int macmbx_conf_get_all_accounts(MacmbxConf *conf, MacmbxAccount **accts) {
  if (!conf || !accts) return 0;
  int count = macmbx_conf_count_accounts(conf);
  if (count <= 0) { *accts = NULL; return 0; }
  *accts = (MacmbxAccount *)calloc(count, sizeof(MacmbxAccount));
  for (int i = 0; i < count; i++)
    macmbx_conf_get_account(conf, i + 1, &(*accts)[i]);
  return count;
}

int macmbx_conf_add_account(MacmbxConf *conf, const MacmbxAccount *acct) {
  if (!conf || !acct) return -1;
  int idx = macmbx_conf_count_accounts(conf) + 1;
  char sec[32]; snprintf(sec, sizeof(sec), "account_%d", idx);

  macmbx_conf_set(conf, sec, "name", acct->name);
  macmbx_conf_set(conf, sec, "real_name", acct->real_name);
  macmbx_conf_set(conf, sec, "email", acct->email);
  macmbx_conf_set(conf, sec, "type", acct->type);
  macmbx_conf_set(conf, sec, "server", acct->server);
  macmbx_conf_set(conf, sec, "smtp_server", acct->smtp_server);
  macmbx_conf_set(conf, sec, "username", acct->username);
  macmbx_conf_set_int(conf, sec, "check_interval", acct->check_interval);
  macmbx_conf_set_int(conf, sec, "ssl_mode", acct->ssl_mode);
  macmbx_conf_set_bool(conf, sec, "leave_on_server", acct->leave_on_server);
  macmbx_conf_set_bool(conf, sec, "enabled", acct->enabled);

  return idx;
}

int macmbx_conf_update_account(MacmbxConf *conf, int index, const MacmbxAccount *acct) {
  if (!conf || !acct || index < 1) return -1;
  char sec[32]; snprintf(sec, sizeof(sec), "account_%d", index);
  if (!macmbx_conf_has_section(conf, sec)) return -1;

  macmbx_conf_set(conf, sec, "name", acct->name);
  macmbx_conf_set(conf, sec, "real_name", acct->real_name);
  macmbx_conf_set(conf, sec, "email", acct->email);
  macmbx_conf_set(conf, sec, "type", acct->type);
  macmbx_conf_set(conf, sec, "server", acct->server);
  macmbx_conf_set(conf, sec, "smtp_server", acct->smtp_server);
  macmbx_conf_set(conf, sec, "username", acct->username);
  macmbx_conf_set_int(conf, sec, "check_interval", acct->check_interval);
  macmbx_conf_set_int(conf, sec, "ssl_mode", acct->ssl_mode);
  macmbx_conf_set_bool(conf, sec, "leave_on_server", acct->leave_on_server);
  macmbx_conf_set_bool(conf, sec, "enabled", acct->enabled);

  return 0;
}

int macmbx_conf_remove_account(MacmbxConf *conf, int index) {
  if (!conf || index < 1) return -1;
  int count = macmbx_conf_count_accounts(conf);
  if (index > count) return -1;

  /* Remove the section */
  char sec[32]; snprintf(sec, sizeof(sec), "account_%d", index);
  macmbx_conf_remove_section(conf, sec);

  /* Renumber remaining accounts */
  for (int i = index + 1; i <= count; i++) {
    char old_sec[32], new_sec[32];
    snprintf(old_sec, sizeof(old_sec), "account_%d", i);
    snprintf(new_sec, sizeof(new_sec), "account_%d", i - 1);
    MacmbxConfSection *s = find_section(conf, old_sec);
    if (s) { free(s->name); s->name = strdup(new_sec); }
  }
  conf->dirty = true;
  return 0;
}

int macmbx_conf_find_account_by_email(MacmbxConf *conf, const char *email) {
  if (!conf || !email) return -1;
  int count = macmbx_conf_count_accounts(conf);
  for (int i = 1; i <= count; i++) {
    char sec[32]; snprintf(sec, sizeof(sec), "account_%d", i);
    const char *e = macmbx_conf_get(conf, sec, "email", "");
    if (strcasecmp(e, email) == 0) return i;
  }
  return -1;
}

int macmbx_conf_find_account_by_name(MacmbxConf *conf, const char *name) {
  if (!conf || !name) return -1;
  int count = macmbx_conf_count_accounts(conf);
  for (int i = 1; i <= count; i++) {
    char sec[32]; snprintf(sec, sizeof(sec), "account_%d", i);
    const char *n = macmbx_conf_get(conf, sec, "name", "");
    if (strcasecmp(n, name) == 0) return i;
  }
  return -1;
}
