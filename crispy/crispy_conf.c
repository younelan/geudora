/* crispy_conf.c — Simple key=value config file parser */

#include "crispy_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *trim(char *s) {
  while (*s && isspace((unsigned char)*s)) s++;
  char *end = s + strlen(s) - 1;
  while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
  return s;
}

int crispy_conf_load(CrispyConf *conf, const char *path) {
  memset(conf, 0, sizeof(*conf));
  FILE *f = fopen(path, "r");
  if (!f) return -1;

  char line[512];
  while (fgets(line, sizeof(line), f) && conf->count < CRISPY_CONF_MAX) {
    char *p = trim(line);
    if (!*p || *p == '#') continue;

    char *eq = strchr(p, '=');
    if (!eq) continue;
    *eq = '\0';

    char *key = trim(p);
    char *val = trim(eq + 1);

    snprintf(conf->entries[conf->count].key, 64, "%s", key);
    snprintf(conf->entries[conf->count].value, 256, "%s", val);
    conf->count++;
  }
  fclose(f);
  return 0;
}

const char *crispy_conf_get(const CrispyConf *conf, const char *key,
                            const char *fallback) {
  for (int i = 0; i < conf->count; i++)
    if (strcasecmp(conf->entries[i].key, key) == 0)
      return conf->entries[i].value;
  return fallback;
}

int crispy_conf_int(const CrispyConf *conf, const char *key, int fallback) {
  const char *v = crispy_conf_get(conf, key, NULL);
  return v ? atoi(v) : fallback;
}
