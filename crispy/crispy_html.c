/* crispy_html.c — HTML tokenizer and parser
 * Part of crispy: standalone mail library.
 * Portable: C99, no external dependencies.
 */

#include "crispy_html.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ================================================================
 * Tag name → enum mapping
 * ================================================================ */

static const struct { const char *name; CrispyHtmlTag tag; } tag_map[] = {
  {"html", CRISPY_TAG_HTML}, {"head", CRISPY_TAG_HEAD}, {"body", CRISPY_TAG_BODY},
  {"title", CRISPY_TAG_TITLE}, {"p", CRISPY_TAG_P}, {"br", CRISPY_TAG_BR},
  {"hr", CRISPY_TAG_HR}, {"div", CRISPY_TAG_DIV}, {"span", CRISPY_TAG_SPAN},
  {"a", CRISPY_TAG_A}, {"img", CRISPY_TAG_IMG}, {"b", CRISPY_TAG_B},
  {"i", CRISPY_TAG_I}, {"u", CRISPY_TAG_U}, {"strong", CRISPY_TAG_STRONG},
  {"em", CRISPY_TAG_EM}, {"font", CRISPY_TAG_FONT}, {"big", CRISPY_TAG_BIG},
  {"small", CRISPY_TAG_SMALL}, {"h1", CRISPY_TAG_H1}, {"h2", CRISPY_TAG_H2},
  {"h3", CRISPY_TAG_H3}, {"h4", CRISPY_TAG_H4}, {"h5", CRISPY_TAG_H5},
  {"h6", CRISPY_TAG_H6}, {"ul", CRISPY_TAG_UL}, {"ol", CRISPY_TAG_OL},
  {"li", CRISPY_TAG_LI}, {"dl", CRISPY_TAG_DL}, {"dt", CRISPY_TAG_DT},
  {"dd", CRISPY_TAG_DD}, {"table", CRISPY_TAG_TABLE}, {"tr", CRISPY_TAG_TR},
  {"td", CRISPY_TAG_TD}, {"th", CRISPY_TAG_TH}, {"caption", CRISPY_TAG_CAPTION},
  {"pre", CRISPY_TAG_PRE}, {"code", CRISPY_TAG_CODE},
  {"blockquote", CRISPY_TAG_BLOCKQUOTE}, {"style", CRISPY_TAG_STYLE},
  {"script", CRISPY_TAG_SCRIPT}, {"meta", CRISPY_TAG_META},
  {"link", CRISPY_TAG_LINK}, {"sup", CRISPY_TAG_SUP}, {"sub", CRISPY_TAG_SUB},
  {"s", CRISPY_TAG_S}, {"strike", CRISPY_TAG_STRIKE}, {"center", CRISPY_TAG_CENTER},
  {"thead", CRISPY_TAG_THEAD}, {"tbody", CRISPY_TAG_TBODY}, {"tfoot", CRISPY_TAG_TFOOT},
  {NULL, CRISPY_TAG_UNKNOWN}
};

CrispyHtmlTag crispy_html_tag_from_name(const char *name) {
  if (!name) return CRISPY_TAG_UNKNOWN;
  for (int i = 0; tag_map[i].name; i++)
    if (strcasecmp(name, tag_map[i].name) == 0) return tag_map[i].tag;
  return CRISPY_TAG_UNKNOWN;
}

bool crispy_html_is_block(CrispyHtmlTag tag) {
  switch (tag) {
    case CRISPY_TAG_P: case CRISPY_TAG_DIV: case CRISPY_TAG_BLOCKQUOTE:
    case CRISPY_TAG_H1: case CRISPY_TAG_H2: case CRISPY_TAG_H3:
    case CRISPY_TAG_H4: case CRISPY_TAG_H5: case CRISPY_TAG_H6:
    case CRISPY_TAG_UL: case CRISPY_TAG_OL: case CRISPY_TAG_LI:
    case CRISPY_TAG_TABLE: case CRISPY_TAG_TR: case CRISPY_TAG_TD:
    case CRISPY_TAG_TH: case CRISPY_TAG_PRE: case CRISPY_TAG_HR:
    case CRISPY_TAG_DL: case CRISPY_TAG_DT: case CRISPY_TAG_DD:
    case CRISPY_TAG_CENTER: case CRISPY_TAG_FORM:
      return true;
    default: return false;
  }
}

bool crispy_html_is_void(CrispyHtmlTag tag) {
  switch (tag) {
    case CRISPY_TAG_BR: case CRISPY_TAG_HR: case CRISPY_TAG_IMG:
    case CRISPY_TAG_INPUT: case CRISPY_TAG_META: case CRISPY_TAG_LINK:
    case CRISPY_TAG_COL: case CRISPY_TAG_EMBED:
      return true;
    default: return false;
  }
}

/* ================================================================
 * Tokenizer
 * ================================================================ */

void crispy_html_init(CrispyHtmlTokenizer *t, const char *html, long len) {
  t->html = html;
  t->len = len < 0 ? (long)strlen(html) : len;
  t->pos = 0;
}

bool crispy_html_next(CrispyHtmlTokenizer *t, CrispyHtmlToken *tok) {
  if (t->pos >= t->len) {
    tok->type = CRISPY_HTML_EOF;
    tok->start = t->html + t->len;
    tok->len = 0;
    tok->tag = CRISPY_TAG_UNKNOWN;
    tok->tag_name[0] = '\0';
    return false;
  }

  const char *p = t->html + t->pos;
  const char *end = t->html + t->len;

  if (*p == '<') {
    const char *tag_start = p;
    p++;

    /* Comment */
    if (p + 2 < end && p[0] == '!' && p[1] == '-' && p[2] == '-') {
      p += 3;
      while (p + 2 < end && !(p[0] == '-' && p[1] == '-' && p[2] == '>')) p++;
      if (p + 2 < end) p += 3;
      tok->type = CRISPY_HTML_COMMENT;
      tok->start = tag_start;
      tok->len = p - tag_start;
      tok->tag = CRISPY_TAG_UNKNOWN;
      tok->tag_name[0] = '\0';
      t->pos = p - t->html;
      return true;
    }

    /* DOCTYPE */
    if (p < end && *p == '!') {
      while (p < end && *p != '>') p++;
      if (p < end) p++;
      tok->type = CRISPY_HTML_DOCTYPE;
      tok->start = tag_start;
      tok->len = p - tag_start;
      tok->tag = CRISPY_TAG_UNKNOWN;
      tok->tag_name[0] = '\0';
      t->pos = p - t->html;
      return true;
    }

    /* Close tag */
    bool is_close = false;
    if (p < end && *p == '/') { is_close = true; p++; }

    /* Tag name */
    while (p < end && isspace((unsigned char)*p)) p++;
    size_t ni = 0;
    while (p < end && *p != '>' && *p != ' ' && *p != '/' && *p != '\t' &&
           *p != '\n' && *p != '\r' && ni < sizeof(tok->tag_name) - 1) {
      tok->tag_name[ni++] = (char)tolower((unsigned char)*p);
      p++;
    }
    tok->tag_name[ni] = '\0';
    tok->tag = crispy_html_tag_from_name(tok->tag_name);

    /* Skip to end of tag (handle quoted attributes) */
    bool self_close = false;
    while (p < end && *p != '>') {
      if (*p == '"') { p++; while (p < end && *p != '"') p++; if (p < end) p++; }
      else if (*p == '\'') { p++; while (p < end && *p != '\'') p++; if (p < end) p++; }
      else { if (*p == '/') self_close = true; p++; }
    }
    if (p < end) p++; /* skip '>' */

    tok->type = is_close ? CRISPY_HTML_TAG_CLOSE :
                (self_close || crispy_html_is_void(tok->tag)) ? CRISPY_HTML_TAG_SELF :
                CRISPY_HTML_TAG_OPEN;
    tok->start = tag_start;
    tok->len = p - tag_start;
    t->pos = p - t->html;
    return true;
  }

  /* Entity */
  if (*p == '&') {
    const char *ent_start = p;
    p++;
    while (p < end && *p != ';' && *p != '<' && *p != ' ' && (p - ent_start) < 12) p++;
    if (p < end && *p == ';') p++;
    tok->type = CRISPY_HTML_ENTITY;
    tok->start = ent_start;
    tok->len = p - ent_start;
    tok->tag = CRISPY_TAG_UNKNOWN;
    tok->tag_name[0] = '\0';
    t->pos = p - t->html;
    return true;
  }

  /* Plain text */
  const char *text_start = p;
  while (p < end && *p != '<' && *p != '&') p++;
  tok->type = CRISPY_HTML_TEXT;
  tok->start = text_start;
  tok->len = p - text_start;
  tok->tag = CRISPY_TAG_UNKNOWN;
  tok->tag_name[0] = '\0';
  t->pos = p - t->html;
  return true;
}

/* ================================================================
 * Entity decoding
 * ================================================================ */

static const struct { const char *name; int ch; } entities[] = {
  {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''},
  {"nbsp", 0xA0}, {"copy", 0xA9}, {"reg", 0xAE}, {"trade", 0x2122},
  {"ndash", 0x2013}, {"mdash", 0x2014}, {"lsquo", 0x2018}, {"rsquo", 0x2019},
  {"ldquo", 0x201C}, {"rdquo", 0x201D}, {"bull", 0x2022}, {"hellip", 0x2026},
  {"euro", 0x20AC}, {"pound", 0xA3}, {"yen", 0xA5}, {"cent", 0xA2},
  {"laquo", 0xAB}, {"raquo", 0xBB}, {"deg", 0xB0}, {"micro", 0xB5},
  {"para", 0xB6}, {"middot", 0xB7}, {"frac14", 0xBC}, {"frac12", 0xBD},
  {"frac34", 0xBE}, {"times", 0xD7}, {"divide", 0xF7},
  {NULL, 0}
};

int crispy_html_decode_entity(const char *entity, int *advance) {
  if (!entity || *entity != '&') { *advance = 0; return -1; }
  const char *p = entity + 1;

  /* Numeric: &#123; or &#x1F; */
  if (*p == '#') {
    p++;
    int val = 0;
    if (*p == 'x' || *p == 'X') {
      p++;
      while (*p && *p != ';') {
        int d = 0;
        if (*p >= '0' && *p <= '9') d = *p - '0';
        else if (*p >= 'a' && *p <= 'f') d = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') d = *p - 'A' + 10;
        else break;
        val = val * 16 + d;
        p++;
      }
    } else {
      while (*p >= '0' && *p <= '9') { val = val * 10 + (*p - '0'); p++; }
    }
    if (*p == ';') p++;
    *advance = (int)(p - entity);
    return val > 0 ? val : -1;
  }

  /* Named */
  char name[16]; size_t ni = 0;
  while (*p && *p != ';' && ni < sizeof(name) - 1) name[ni++] = *p++;
  name[ni] = '\0';
  if (*p == ';') p++;
  *advance = (int)(p - entity);

  for (int i = 0; entities[i].name; i++)
    if (strcasecmp(name, entities[i].name) == 0) return entities[i].ch;

  return -1;
}

/* Encode a Unicode codepoint as UTF-8 into buf. Returns bytes written. */
static int utf8_encode(int cp, char *buf) {
  if (cp < 0) return 0;
  if (cp < 0x80) { buf[0] = (char)cp; return 1; }
  if (cp < 0x800) { buf[0] = (char)(0xC0 | (cp >> 6)); buf[1] = (char)(0x80 | (cp & 0x3F)); return 2; }
  if (cp < 0x10000) { buf[0] = (char)(0xE0 | (cp >> 12)); buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); buf[2] = (char)(0x80 | (cp & 0x3F)); return 3; }
  buf[0] = (char)(0xF0 | (cp >> 18)); buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); buf[3] = (char)(0x80 | (cp & 0x3F)); return 4;
}

char *crispy_html_decode_entities(const char *text, long len) {
  if (!text) return strdup("");
  if (len < 0) len = (long)strlen(text);
  char *out = (char *)malloc(len * 4 + 1); /* worst case: all entities expand to 4-byte UTF-8 */
  if (!out) return NULL;
  long oi = 0;
  const char *p = text, *end = text + len;
  while (p < end) {
    if (*p == '&') {
      int adv = 0;
      int ch = crispy_html_decode_entity(p, &adv);
      if (ch > 0) {
        oi += utf8_encode(ch, out + oi);
        p += adv;
      } else {
        out[oi++] = *p++;
      }
    } else {
      out[oi++] = *p++;
    }
  }
  out[oi] = '\0';
  return out;
}

/* ================================================================
 * Attribute parsing
 * ================================================================ */

char *crispy_html_get_attr(const char *tag_text, long tag_len, const char *attr_name) {
  if (!tag_text || !attr_name) return NULL;
  const char *p = tag_text, *end = tag_text + tag_len;

  /* Skip past tag name */
  if (*p == '<') p++;
  if (*p == '/') p++;
  while (p < end && !isspace((unsigned char)*p) && *p != '>') p++;

  size_t alen = strlen(attr_name);
  while (p < end) {
    while (p < end && isspace((unsigned char)*p)) p++;
    if (p >= end || *p == '>' || *p == '/') break;

    /* Attribute name */
    const char *astart = p;
    while (p < end && *p != '=' && !isspace((unsigned char)*p) && *p != '>') p++;
    size_t nlen = (size_t)(p - astart);

    while (p < end && isspace((unsigned char)*p)) p++;
    if (p >= end || *p != '=') continue;
    p++; /* skip = */
    while (p < end && isspace((unsigned char)*p)) p++;

    /* Value */
    const char *vstart;
    size_t vlen;
    if (p < end && (*p == '"' || *p == '\'')) {
      char q = *p++;
      vstart = p;
      while (p < end && *p != q) p++;
      vlen = (size_t)(p - vstart);
      if (p < end) p++;
    } else {
      vstart = p;
      while (p < end && !isspace((unsigned char)*p) && *p != '>') p++;
      vlen = (size_t)(p - vstart);
    }

    if (nlen == alen && strncasecmp(astart, attr_name, alen) == 0) {
      char *val = (char *)malloc(vlen + 1);
      if (val) { memcpy(val, vstart, vlen); val[vlen] = '\0'; }
      return val;
    }
  }
  return NULL;
}

/* ================================================================
 * Color parsing
 * ================================================================ */

static const struct { const char *name; unsigned char r, g, b; } named_colors[] = {
  {"black", 0, 0, 0}, {"white", 255, 255, 255}, {"red", 255, 0, 0},
  {"green", 0, 128, 0}, {"blue", 0, 0, 255}, {"yellow", 255, 255, 0},
  {"cyan", 0, 255, 255}, {"magenta", 255, 0, 255}, {"gray", 128, 128, 128},
  {"grey", 128, 128, 128}, {"silver", 192, 192, 192}, {"maroon", 128, 0, 0},
  {"olive", 128, 128, 0}, {"lime", 0, 255, 0}, {"aqua", 0, 255, 255},
  {"teal", 0, 128, 128}, {"navy", 0, 0, 128}, {"fuchsia", 255, 0, 255},
  {"purple", 128, 0, 128}, {"orange", 255, 165, 0},
  {NULL, 0, 0, 0}
};

static int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

bool crispy_html_parse_color(const char *color_str, CrispyColor *out) {
  if (!color_str || !out) return false;
  while (*color_str == ' ') color_str++;

  if (*color_str == '#') {
    color_str++;
    size_t len = strlen(color_str);
    if (len == 6) {
      out->r = (unsigned char)(hex_digit(color_str[0]) * 16 + hex_digit(color_str[1]));
      out->g = (unsigned char)(hex_digit(color_str[2]) * 16 + hex_digit(color_str[3]));
      out->b = (unsigned char)(hex_digit(color_str[4]) * 16 + hex_digit(color_str[5]));
      return true;
    }
    if (len == 3) {
      out->r = (unsigned char)(hex_digit(color_str[0]) * 17);
      out->g = (unsigned char)(hex_digit(color_str[1]) * 17);
      out->b = (unsigned char)(hex_digit(color_str[2]) * 17);
      return true;
    }
    return false;
  }

  for (int i = 0; named_colors[i].name; i++) {
    if (strcasecmp(color_str, named_colors[i].name) == 0) {
      out->r = named_colors[i].r;
      out->g = named_colors[i].g;
      out->b = named_colors[i].b;
      return true;
    }
  }
  return false;
}

/* ================================================================
 * Charset detection
 * ================================================================ */

char *crispy_html_detect_charset(const char *html, long len) {
  if (!html) return NULL;
  if (len < 0) len = (long)strlen(html);

  CrispyHtmlTokenizer t;
  CrispyHtmlToken tok;
  crispy_html_init(&t, html, len);

  while (crispy_html_next(&t, &tok)) {
    if (tok.type == CRISPY_HTML_TAG_OPEN || tok.type == CRISPY_HTML_TAG_SELF) {
      if (tok.tag == CRISPY_TAG_META) {
        /* <meta charset="utf-8"> */
        char *cs = crispy_html_get_attr(tok.start, tok.len, "charset");
        if (cs) return cs;

        /* <meta http-equiv="Content-Type" content="text/html; charset=utf-8"> */
        char *he = crispy_html_get_attr(tok.start, tok.len, "http-equiv");
        if (he && strcasecmp(he, "Content-Type") == 0) {
          char *content = crispy_html_get_attr(tok.start, tok.len, "content");
          if (content) {
            char *cp = strcasestr(content, "charset=");
            if (cp) {
              cp += 8;
              char *end_cp = cp;
              while (*end_cp && *end_cp != ';' && *end_cp != ' ' && *end_cp != '"') end_cp++;
              char *result = (char *)malloc(end_cp - cp + 1);
              if (result) { memcpy(result, cp, end_cp - cp); result[end_cp - cp] = '\0'; }
              free(content); free(he);
              return result;
            }
            free(content);
          }
        }
        free(he);
      }
      /* Stop after </head> or <body> */
      if (tok.tag == CRISPY_TAG_BODY) break;
    }
    if (tok.type == CRISPY_HTML_TAG_CLOSE && tok.tag == CRISPY_TAG_HEAD) break;
  }
  return NULL;
}
