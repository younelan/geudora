/* macmbx_vcard.c — vCard 2.1/3.0 parser and builder
 * Part of macmbx: standalone mail data management library.
 * Ported from Eudora vcard.c, rewritten standalone.
 */

#include "macmbx_vcard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* ================================================================
 * String buffer
 * ================================================================ */

typedef struct { char *d; size_t l, c; } SB;
static void sb_init(SB *s) { s->c = 256; s->d = malloc(s->c); s->l = 0; s->d[0] = '\0'; }
static void sb_add(SB *s, const char *t, size_t n) {
  if (s->l + n + 1 > s->c) { while (s->l + n + 1 > s->c) s->c *= 2; s->d = realloc(s->d, s->c); }
  memcpy(s->d + s->l, t, n); s->l += n; s->d[s->l] = '\0';
}
static void sb_adds(SB *s, const char *t) { sb_add(s, t, strlen(t)); }
static void sb_addc(SB *s, char c) { sb_add(s, &c, 1); }
static char *sb_detach(SB *s) { char *r = s->d; s->d = NULL; s->l = s->c = 0; return r; }

/* ================================================================
 * Base64 decode (self-contained, no crispy dependency)
 * ================================================================ */

static const signed char b64v[256] = {
  ['A']=0,['B']=1,['C']=2,['D']=3,['E']=4,['F']=5,['G']=6,['H']=7,
  ['I']=8,['J']=9,['K']=10,['L']=11,['M']=12,['N']=13,['O']=14,['P']=15,
  ['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,
  ['Y']=24,['Z']=25,['a']=26,['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,
  ['g']=32,['h']=33,['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,
  ['o']=40,['p']=41,['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,
  ['w']=48,['x']=49,['y']=50,['z']=51,['0']=52,['1']=53,['2']=54,['3']=55,
  ['4']=56,['5']=57,['6']=58,['7']=59,['8']=60,['9']=61,['+']=62,['/']=63,
};

static unsigned char *b64_decode(const char *in, long inLen, long *outLen) {
  unsigned char *out = malloc(inLen);
  if (!out) return NULL;
  long o = 0;
  for (long i = 0; i < inLen; ) {
    while (i < inLen && (in[i] == '\r' || in[i] == '\n' || in[i] == ' ')) i++;
    if (i >= inLen) break;
    unsigned char a = b64v[(unsigned char)in[i++]];
    unsigned char b = (i < inLen) ? b64v[(unsigned char)in[i++]] : 0;
    unsigned char c = (i < inLen) ? b64v[(unsigned char)in[i++]] : 0;
    unsigned char d = (i < inLen) ? b64v[(unsigned char)in[i++]] : 0;
    out[o++] = (a << 2) | (b >> 4);
    if (i >= 3 && in[i-2] != '=') out[o++] = (b << 4) | (c >> 2);
    if (i >= 4 && in[i-1] != '=') out[o++] = (c << 6) | d;
  }
  if (outLen) *outLen = o;
  return out;
}

static char *b64_encode(const unsigned char *in, long inLen) {
  static const char t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  long outLen = ((inLen + 2) / 3) * 4 + (inLen / 54) + 4;
  char *out = malloc(outLen);
  if (!out) return NULL;
  long o = 0, col = 0;
  for (long i = 0; i < inLen; i += 3) {
    unsigned char a = in[i];
    unsigned char b = (i + 1 < inLen) ? in[i + 1] : 0;
    unsigned char c = (i + 2 < inLen) ? in[i + 2] : 0;
    out[o++] = t[a >> 2];
    out[o++] = t[((a & 3) << 4) | (b >> 4)];
    out[o++] = (i + 1 < inLen) ? t[((b & 0xF) << 2) | (c >> 6)] : '=';
    out[o++] = (i + 2 < inLen) ? t[c & 0x3F] : '=';
    col += 4;
    if (col >= 76) { out[o++] = '\r'; out[o++] = '\n'; col = 0; }
  }
  out[o] = '\0';
  return out;
}

/* ================================================================
 * QP decode
 * ================================================================ */

static int hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static char *qp_decode(const char *in, long inLen, long *outLen) {
  char *out = malloc(inLen + 1);
  if (!out) return NULL;
  long o = 0;
  for (long i = 0; i < inLen; i++) {
    if (in[i] == '=' && i + 2 < inLen) {
      if (in[i+1] == '\r' || in[i+1] == '\n') {
        i++; if (i < inLen && in[i] == '\n') i++;
        continue; /* soft line break */
      }
      int h = hexval(in[i+1]), l = hexval(in[i+2]);
      if (h >= 0 && l >= 0) { out[o++] = (char)((h << 4) | l); i += 2; continue; }
    }
    out[o++] = in[i];
  }
  out[o] = '\0';
  if (outLen) *outLen = o;
  return out;
}

/* ================================================================
 * Line unfolding: CRLF followed by space/tab = continuation
 * ================================================================ */

static char *unfold_lines(const char *text, long len) {
  char *out = malloc(len + 1);
  if (!out) return NULL;
  long o = 0;
  for (long i = 0; i < len; i++) {
    if (text[i] == '\r' && i + 1 < len && text[i+1] == '\n' &&
        i + 2 < len && (text[i+2] == ' ' || text[i+2] == '\t')) {
      i += 2; /* skip CRLF + space/tab */
      continue;
    }
    if (text[i] == '\n' &&
        i + 1 < len && (text[i+1] == ' ' || text[i+1] == '\t')) {
      i++; /* skip LF + space/tab */
      continue;
    }
    out[o++] = text[i];
  }
  out[o] = '\0';
  return out;
}

/* ================================================================
 * Property memory
 * ================================================================ */

static MacmbxVcardProp *prop_new(void) {
  return (MacmbxVcardProp *)calloc(1, sizeof(MacmbxVcardProp));
}

void macmbx_vcard_free_prop(MacmbxVcardProp *prop) {
  if (!prop) return;
  for (int i = 0; i < prop->value_count; i++) free(prop->values[i]);
  free(prop->raw_value);
  free(prop->binary_data);
  free(prop);
}

void macmbx_vcard_free(MacmbxVcard *vc) {
  if (!vc) return;
  MacmbxVcardProp *p = vc->props;
  while (p) { MacmbxVcardProp *next = p->next; macmbx_vcard_free_prop(p); p = next; }
  free(vc);
}

static void vc_append_prop(MacmbxVcard *vc, MacmbxVcardProp *prop) {
  prop->next = NULL;
  if (!vc->props) { vc->props = prop; return; }
  MacmbxVcardProp *tail = vc->props;
  while (tail->next) tail = tail->next;
  tail->next = prop;
}

/* ================================================================
 * Parser
 * ================================================================ */

/* Parse parameters from "name;PARAM1=val1;PARAM2=val2:value" */
static void parse_params(char *paramStr, MacmbxVcardProp *prop) {
  char *tok = strtok(paramStr, ";");
  while (tok) {
    char *eq = strchr(tok, '=');
    if (eq) {
      *eq = '\0';
      if (prop->param_count < MACMBX_VCARD_MAX_PARAMS) {
        MacmbxVcardParam *p = &prop->params[prop->param_count++];
        snprintf(p->name, sizeof(p->name), "%s", tok);
        snprintf(p->value, sizeof(p->value), "%s", eq + 1);
        /* Uppercase param name */
        for (char *c = p->name; *c; c++) *c = toupper((unsigned char)*c);
      }
    } else {
      /* vCard 2.1 style: bare parameter like "WORK" or "HOME" = TYPE=WORK */
      if (strcasecmp(tok, "WORK") == 0 || strcasecmp(tok, "HOME") == 0 ||
          strcasecmp(tok, "CELL") == 0 || strcasecmp(tok, "FAX") == 0 ||
          strcasecmp(tok, "PREF") == 0 || strcasecmp(tok, "VOICE") == 0 ||
          strcasecmp(tok, "INTERNET") == 0) {
        if (prop->param_count < MACMBX_VCARD_MAX_PARAMS) {
          MacmbxVcardParam *p = &prop->params[prop->param_count++];
          snprintf(p->name, sizeof(p->name), "TYPE");
          snprintf(p->value, sizeof(p->value), "%s", tok);
          for (char *c = p->value; *c; c++) *c = toupper((unsigned char)*c);
        }
      } else if (strcasecmp(tok, "QUOTED-PRINTABLE") == 0) {
        if (prop->param_count < MACMBX_VCARD_MAX_PARAMS) {
          MacmbxVcardParam *p = &prop->params[prop->param_count++];
          snprintf(p->name, sizeof(p->name), "ENCODING");
          snprintf(p->value, sizeof(p->value), "QUOTED-PRINTABLE");
        }
      } else if (strcasecmp(tok, "BASE64") == 0 || strcasecmp(tok, "b") == 0) {
        if (prop->param_count < MACMBX_VCARD_MAX_PARAMS) {
          MacmbxVcardParam *p = &prop->params[prop->param_count++];
          snprintf(p->name, sizeof(p->name), "ENCODING");
          snprintf(p->value, sizeof(p->value), "b");
        }
      }
    }
    tok = strtok(NULL, ";");
  }
}

/* Check if property uses a specific encoding */
static bool has_encoding(MacmbxVcardProp *prop, const char *enc) {
  for (int i = 0; i < prop->param_count; i++)
    if (strcasecmp(prop->params[i].name, "ENCODING") == 0 &&
        strcasecmp(prop->params[i].value, enc) == 0) return true;
  return false;
}

/* Structured properties: split by semicolon into parts */
static bool is_structured(const char *name) {
  return strcasecmp(name, "N") == 0 || strcasecmp(name, "ADR") == 0 ||
         strcasecmp(name, "ORG") == 0;
}

static void split_structured(MacmbxVcardProp *prop, const char *value) {
  char *copy = strdup(value);
  char *p = copy;
  while (p && prop->value_count < MACMBX_VCARD_MAX_VALUES) {
    char *semi = strchr(p, ';');
    if (semi) *semi = '\0';
    prop->values[prop->value_count++] = strdup(p);
    p = semi ? semi + 1 : NULL;
  }
  free(copy);
}

MacmbxVcard *macmbx_vcard_parse(const char *text, long len) {
  if (!text) return NULL;
  if (len < 0) len = (long)strlen(text);

  /* Unfold continuation lines */
  char *unfolded = unfold_lines(text, len);
  if (!unfolded) return NULL;

  MacmbxVcard *vc = (MacmbxVcard *)calloc(1, sizeof(MacmbxVcard));
  if (!vc) { free(unfolded); return NULL; }
  snprintf(vc->version, sizeof(vc->version), "3.0");

  char *line = unfolded;
  bool in_vcard = false;

  while (line && *line) {
    /* Get one line */
    char *eol = strpbrk(line, "\r\n");
    if (eol) {
      *eol = '\0';
      eol++;
      if (*eol == '\n') eol++;
    }

    if (strcasecmp(line, "BEGIN:VCARD") == 0) { in_vcard = true; line = eol; continue; }
    if (strcasecmp(line, "END:VCARD") == 0) break;
    if (!in_vcard) { line = eol; continue; }

    /* Parse: [group.]NAME[;PARAMS]:VALUE */
    char *colon = strchr(line, ':');
    if (!colon) { line = eol; continue; }
    *colon = '\0';
    char *raw_name = line;
    char *raw_value = colon + 1;

    MacmbxVcardProp *prop = prop_new();
    if (!prop) { line = eol; continue; }

    /* Check for group prefix */
    char *dot = strchr(raw_name, '.');
    if (dot) {
      *dot = '\0';
      snprintf(prop->group, sizeof(prop->group), "%s", raw_name);
      raw_name = dot + 1;
    }

    /* Split name from params */
    char *semi = strchr(raw_name, ';');
    if (semi) {
      *semi = '\0';
      parse_params(semi + 1, prop);
    }
    snprintf(prop->name, sizeof(prop->name), "%s", raw_name);
    /* Uppercase property name */
    for (char *c = prop->name; *c; c++) *c = toupper((unsigned char)*c);

    /* VERSION */
    if (strcmp(prop->name, "VERSION") == 0) {
      snprintf(vc->version, sizeof(vc->version), "%s", raw_value);
      macmbx_vcard_free_prop(prop);
      line = eol; continue;
    }

    /* Decode value based on encoding */
    char *decoded_value = NULL;
    if (has_encoding(prop, "QUOTED-PRINTABLE") || has_encoding(prop, "quoted-printable")) {
      long dlen;
      decoded_value = qp_decode(raw_value, (long)strlen(raw_value), &dlen);
    } else if (has_encoding(prop, "b") || has_encoding(prop, "BASE64") || has_encoding(prop, "base64")) {
      long blen;
      prop->binary_data = b64_decode(raw_value, (long)strlen(raw_value), &blen);
      prop->binary_len = blen;
      /* Also store raw for text access */
      decoded_value = strdup(raw_value);
    } else {
      decoded_value = strdup(raw_value);
    }

    /* Store value */
    prop->raw_value = decoded_value;
    if (is_structured(prop->name) && decoded_value) {
      split_structured(prop, decoded_value);
    }

    vc_append_prop(vc, prop);
    line = eol;
  }

  free(unfolded);
  return vc;
}

int macmbx_vcard_parse_file(const char *path, MacmbxVcard ***cards) {
  if (!path || !cards) return 0;
  *cards = NULL;

  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  fseek(f, 0, SEEK_END);
  long flen = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *data = malloc(flen + 1);
  if (!data) { fclose(f); return 0; }
  flen = (long)fread(data, 1, flen, f);
  data[flen] = '\0';
  fclose(f);

  /* Count vCards */
  int count = 0;
  const char *p = data;
  while ((p = strcasestr(p, "BEGIN:VCARD")) != NULL) { count++; p += 11; }

  if (count == 0) { free(data); return 0; }

  *cards = (MacmbxVcard **)calloc(count, sizeof(MacmbxVcard *));
  int idx = 0;
  p = data;
  while ((p = strcasestr(p, "BEGIN:VCARD")) != NULL) {
    const char *end = strcasestr(p + 11, "END:VCARD");
    if (!end) break;
    end += 9; /* include "END:VCARD" */
    long vclen = end - p;
    (*cards)[idx] = macmbx_vcard_parse(p, vclen);
    if ((*cards)[idx]) idx++;
    p = end;
  }

  free(data);
  return idx;
}

/* ================================================================
 * Building
 * ================================================================ */

MacmbxVcard *macmbx_vcard_new(void) {
  MacmbxVcard *vc = (MacmbxVcard *)calloc(1, sizeof(MacmbxVcard));
  if (vc) snprintf(vc->version, sizeof(vc->version), "3.0");
  return vc;
}

MacmbxVcardProp *macmbx_vcard_add(MacmbxVcard *vc, const char *name, const char *value) {
  if (!vc || !name) return NULL;
  MacmbxVcardProp *p = prop_new();
  snprintf(p->name, sizeof(p->name), "%s", name);
  for (char *c = p->name; *c; c++) *c = toupper((unsigned char)*c);
  p->raw_value = value ? strdup(value) : strdup("");
  if (is_structured(p->name)) split_structured(p, p->raw_value);
  vc_append_prop(vc, p);
  return p;
}

MacmbxVcardProp *macmbx_vcard_add_typed(MacmbxVcard *vc, const char *name,
                                          const char *value, const char *type) {
  MacmbxVcardProp *p = macmbx_vcard_add(vc, name, value);
  if (p && type) macmbx_vcard_prop_add_param(p, "TYPE", type);
  return p;
}

MacmbxVcardProp *macmbx_vcard_add_structured(MacmbxVcard *vc, const char *name,
                                               const char **parts, int part_count) {
  if (!vc || !name) return NULL;
  SB sb; sb_init(&sb);
  for (int i = 0; i < part_count; i++) {
    if (i > 0) sb_addc(&sb, ';');
    if (parts[i]) sb_adds(&sb, parts[i]);
  }
  MacmbxVcardProp *p = macmbx_vcard_add(vc, name, sb.d);
  free(sb.d);
  return p;
}

MacmbxVcardProp *macmbx_vcard_add_binary(MacmbxVcard *vc, const char *name,
                                           const char *media_type,
                                           const unsigned char *data, long len) {
  if (!vc || !name || !data) return NULL;
  MacmbxVcardProp *p = prop_new();
  snprintf(p->name, sizeof(p->name), "%s", name);
  for (char *c = p->name; *c; c++) *c = toupper((unsigned char)*c);
  p->binary_data = malloc(len);
  if (p->binary_data) { memcpy(p->binary_data, data, len); p->binary_len = len; }
  p->raw_value = b64_encode(data, len);
  macmbx_vcard_prop_add_param(p, "ENCODING", "b");
  if (media_type) macmbx_vcard_prop_add_param(p, "TYPE", media_type);
  vc_append_prop(vc, p);
  return p;
}

int macmbx_vcard_prop_add_param(MacmbxVcardProp *prop, const char *name, const char *value) {
  if (!prop || !name || prop->param_count >= MACMBX_VCARD_MAX_PARAMS) return -1;
  MacmbxVcardParam *p = &prop->params[prop->param_count++];
  snprintf(p->name, sizeof(p->name), "%s", name);
  snprintf(p->value, sizeof(p->value), "%s", value ? value : "");
  return 0;
}

/* ================================================================
 * Serializer
 * ================================================================ */

/* Fold long lines at 75 chars */
static void fold_line(SB *sb, const char *line) {
  size_t len = strlen(line);
  if (len <= 75) { sb_adds(sb, line); sb_adds(sb, "\r\n"); return; }
  size_t i = 0;
  while (i < len) {
    size_t chunk = (i == 0) ? 75 : 74; /* first line 75, continuation 74 (1 for space) */
    if (i + chunk > len) chunk = len - i;
    if (i > 0) sb_addc(sb, ' '); /* continuation space */
    sb_add(sb, line + i, chunk);
    sb_adds(sb, "\r\n");
    i += chunk;
  }
}

char *macmbx_vcard_serialize(MacmbxVcard *vc) {
  if (!vc) return strdup("");
  SB sb; sb_init(&sb);
  sb_adds(&sb, "BEGIN:VCARD\r\n");

  char line[4096];
  snprintf(line, sizeof(line), "VERSION:%s", vc->version);
  fold_line(&sb, line);

  for (MacmbxVcardProp *p = vc->props; p; p = p->next) {
    if (strcasecmp(p->name, "VERSION") == 0) continue;

    SB ln; sb_init(&ln);
    /* Group prefix */
    if (p->group[0]) { sb_adds(&ln, p->group); sb_addc(&ln, '.'); }
    sb_adds(&ln, p->name);
    /* Parameters */
    for (int i = 0; i < p->param_count; i++) {
      sb_addc(&ln, ';');
      sb_adds(&ln, p->params[i].name);
      sb_addc(&ln, '=');
      sb_adds(&ln, p->params[i].value);
    }
    sb_addc(&ln, ':');
    if (p->raw_value) sb_adds(&ln, p->raw_value);
    fold_line(&sb, ln.d);
    free(ln.d);
  }

  sb_adds(&sb, "END:VCARD\r\n");
  return sb_detach(&sb);
}

int macmbx_vcard_write_file(MacmbxVcard *vc, const char *path) {
  char *text = macmbx_vcard_serialize(vc);
  if (!text) return -1;
  FILE *f = fopen(path, "wb");
  if (!f) { free(text); return -1; }
  fwrite(text, 1, strlen(text), f);
  fclose(f);
  free(text);
  return 0;
}

int macmbx_vcard_write_file_multi(MacmbxVcard **cards, int count, const char *path) {
  if (!cards || !path) return -1;
  FILE *f = fopen(path, "wb");
  if (!f) return -1;
  for (int i = 0; i < count; i++) {
    char *text = macmbx_vcard_serialize(cards[i]);
    if (text) { fwrite(text, 1, strlen(text), f); free(text); }
  }
  fclose(f);
  return 0;
}

/* ================================================================
 * Property access
 * ================================================================ */

MacmbxVcardProp *macmbx_vcard_find(MacmbxVcard *vc, const char *name) {
  if (!vc || !name) return NULL;
  for (MacmbxVcardProp *p = vc->props; p; p = p->next)
    if (strcasecmp(p->name, name) == 0) return p;
  return NULL;
}

int macmbx_vcard_find_all(MacmbxVcard *vc, const char *name,
                            MacmbxVcardProp **props, int max) {
  if (!vc || !name || !props) return 0;
  int count = 0;
  for (MacmbxVcardProp *p = vc->props; p && count < max; p = p->next)
    if (strcasecmp(p->name, name) == 0) props[count++] = p;
  return count;
}

const char *macmbx_vcard_get(MacmbxVcard *vc, const char *name) {
  MacmbxVcardProp *p = macmbx_vcard_find(vc, name);
  return p ? p->raw_value : NULL;
}

const char *macmbx_vcard_get_part(MacmbxVcard *vc, const char *name, int part) {
  MacmbxVcardProp *p = macmbx_vcard_find(vc, name);
  if (!p || part < 0 || part >= p->value_count) return NULL;
  return p->values[part];
}

bool macmbx_vcard_has_type(MacmbxVcardProp *prop, const char *type) {
  if (!prop || !type) return false;
  for (int i = 0; i < prop->param_count; i++)
    if (strcasecmp(prop->params[i].name, "TYPE") == 0 &&
        strcasestr(prop->params[i].value, type)) return true;
  return false;
}

const char *macmbx_vcard_get_type(MacmbxVcardProp *prop) {
  if (!prop) return NULL;
  for (int i = 0; i < prop->param_count; i++)
    if (strcasecmp(prop->params[i].name, "TYPE") == 0) return prop->params[i].value;
  return NULL;
}

/* ================================================================
 * Convenience accessors
 * ================================================================ */

const char *macmbx_vcard_fn(MacmbxVcard *vc) { return macmbx_vcard_get(vc, "FN"); }
const char *macmbx_vcard_family(MacmbxVcard *vc) { return macmbx_vcard_get_part(vc, "N", 0); }
const char *macmbx_vcard_given(MacmbxVcard *vc) { return macmbx_vcard_get_part(vc, "N", 1); }

const char *macmbx_vcard_email(MacmbxVcard *vc) { return macmbx_vcard_get(vc, "EMAIL"); }

int macmbx_vcard_emails(MacmbxVcard *vc, const char **addrs, int max) {
  MacmbxVcardProp *props[16];
  int count = macmbx_vcard_find_all(vc, "EMAIL", props, max < 16 ? max : 16);
  for (int i = 0; i < count && i < max; i++) addrs[i] = props[i]->raw_value;
  return count;
}

const char *macmbx_vcard_phone(MacmbxVcard *vc, const char *type) {
  for (MacmbxVcardProp *p = vc ? vc->props : NULL; p; p = p->next) {
    if (strcasecmp(p->name, "TEL") != 0) continue;
    if (!type || macmbx_vcard_has_type(p, type)) return p->raw_value;
  }
  return NULL;
}

const char *macmbx_vcard_org(MacmbxVcard *vc) { return macmbx_vcard_get(vc, "ORG"); }

const unsigned char *macmbx_vcard_photo(MacmbxVcard *vc, long *outLen,
                                          const char **media_type) {
  MacmbxVcardProp *p = macmbx_vcard_find(vc, "PHOTO");
  if (!p || !p->binary_data) return NULL;
  if (outLen) *outLen = p->binary_len;
  if (media_type) *media_type = macmbx_vcard_get_type(p);
  return p->binary_data;
}

/* strcasestr portability */
#if !defined(_GNU_SOURCE) && !defined(__APPLE__)
static const char *macmbx_vc_strcasestr(const char *h, const char *n) {
  if (!h || !n) return NULL;
  size_t nl = strlen(n);
  for (; *h; h++) if (strncasecmp(h, n, nl) == 0) return h;
  return NULL;
}
#undef strcasestr
#define strcasestr macmbx_vc_strcasestr
#endif
