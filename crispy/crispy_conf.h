/* crispy_conf.h — Simple key=value config file parser
 * Part of crispy: standalone, no external dependencies.
 */

#ifndef CRISPY_CONF_H
#define CRISPY_CONF_H

#define CRISPY_CONF_MAX 64

typedef struct CrispyConf {
  struct { char key[64]; char value[256]; } entries[CRISPY_CONF_MAX];
  int count;
} CrispyConf;

/* Load a key=value file. Returns 0 on success. */
int crispy_conf_load(CrispyConf *conf, const char *path);

/* Get a value. Returns fallback if not found. */
const char *crispy_conf_get(const CrispyConf *conf, const char *key,
                            const char *fallback);

/* Get a value as int. */
int crispy_conf_int(const CrispyConf *conf, const char *key, int fallback);

#endif
