/* macmbx_xml.c — Portable XML parser and generator
 * Part of macmbx: standalone mail data management library.
 *
 * Ported from original Eudora xml.c. Tokenizer + recursive descent parser.
 * Standalone: no GLib, no libxml2, no GTK.
 */

#include "macmbx_xml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/* ================================================================
 * Internal helpers
 * ================================================================ */

static void writer_grow(MacmbxXmlWriter *w, int need) {
  if (w->len + need < w->cap) return;
  int newcap = w->cap * 2;
  if (newcap < w->len + need + 64) newcap = w->len + need + 64;
  w->buf = (char *)realloc(w->buf, newcap);
  w->cap = newcap;
}

static void writer_append(MacmbxXmlWriter *w, const char *s, int slen) {
  if (slen < 0) slen = (int)strlen(s);
  writer_grow(w, slen + 1);
  memcpy(w->buf + w->len, s, slen);
  w->len += slen;
  w->buf[w->len] = '\0';
}

static void writer_char(MacmbxXmlWriter *w, char c) {
  writer_grow(w, 2);
  w->buf[w->len++] = c;
  w->buf[w->len] = '\0';
}

static void writer_indent(MacmbxXmlWriter *w) {
  for (int i = 0; i < w->indent; i++)
    writer_char(w, '\t');
}

/* HTML named entity table (beyond the 5 XML entities) */
typedef struct { const char *name; uint32_t codepoint; } HtmlEntity;
static const HtmlEntity html_entities[] = {
  { "nbsp",    0x00A0 }, { "iexcl",   0x00A1 }, { "cent",    0x00A2 },
  { "pound",   0x00A3 }, { "yen",     0x00A5 }, { "sect",    0x00A7 },
  { "copy",    0x00A9 }, { "laquo",   0x00AB }, { "reg",     0x00AE },
  { "deg",     0x00B0 }, { "micro",   0x00B5 }, { "para",    0x00B6 },
  { "middot",  0x00B7 }, { "raquo",   0x00BB }, { "frac14",  0x00BC },
  { "frac12",  0x00BD }, { "frac34",  0x00BE }, { "iquest",  0x00BF },
  { "times",   0x00D7 }, { "divide",  0x00F7 }, { "ndash",   0x2013 },
  { "mdash",   0x2014 }, { "lsquo",   0x2018 }, { "rsquo",   0x2019 },
  { "ldquo",   0x201C }, { "rdquo",   0x201D }, { "bull",    0x2022 },
  { "hellip",  0x2026 }, { "euro",    0x20AC }, { "trade",   0x2122 },
  { NULL, 0 }
};

/* Encode a Unicode codepoint as UTF-8 into buf. Returns bytes written. */
static int entity_utf8_encode(uint32_t cp, char *buf) {
  int n = 0;
  if (cp < 0x80) {
    buf[n++] = (char)cp;
  } else if (cp < 0x800) {
    buf[n++] = (char)(0xC0 | (cp >> 6));
    buf[n++] = (char)(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    buf[n++] = (char)(0xE0 | (cp >> 12));
    buf[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
    buf[n++] = (char)(0x80 | (cp & 0x3F));
  } else if (cp <= 0x10FFFF) {
    buf[n++] = (char)(0xF0 | (cp >> 18));
    buf[n++] = (char)(0x80 | ((cp >> 12) & 0x3F));
    buf[n++] = (char)(0x80 | ((cp >> 6) & 0x3F));
    buf[n++] = (char)(0x80 | (cp & 0x3F));
  }
  return n;
}

/* Expand XML character/entity references in place.
 * Handles: &lt; &gt; &amp; &quot; &apos; &#123; &#x1F;
 * Also handles common HTML named entities. */
static int expand_entities(const char *in, int inLen, char *out, int maxOut) {
  int o = 0;
  for (int i = 0; i < inLen && o < maxOut - 1; i++) {
    if (in[i] == '&') {
      /* Entity reference */
      const char *semi = memchr(in + i, ';', inLen - i);
      if (!semi) { out[o++] = in[i]; continue; }
      int elen = (int)(semi - (in + i) - 1); /* length of entity name */
      const char *ename = in + i + 1;
      char ch = 0;
      if (elen == 2 && !memcmp(ename, "lt", 2)) ch = '<';
      else if (elen == 2 && !memcmp(ename, "gt", 2)) ch = '>';
      else if (elen == 3 && !memcmp(ename, "amp", 3)) ch = '&';
      else if (elen == 4 && !memcmp(ename, "quot", 4)) ch = '"';
      else if (elen == 4 && !memcmp(ename, "apos", 4)) ch = '\'';
      else if (ename[0] == '#') {
        /* Numeric reference */
        unsigned long cp;
        if (ename[1] == 'x' || ename[1] == 'X')
          cp = strtoul(ename + 2, NULL, 16);
        else
          cp = strtoul(ename + 1, NULL, 10);
        if (cp < 0x80) {
          ch = (char)cp;
        } else {
          char u8[4];
          int u8len = entity_utf8_encode((uint32_t)cp, u8);
          for (int j = 0; j < u8len && o < maxOut - 1; j++)
            out[o++] = u8[j];
          i = (int)(semi - in);
          continue;
        }
      }
      /* Try HTML named entities if not matched yet */
      if (!ch) {
        for (const HtmlEntity *e = html_entities; e->name; e++) {
          int nlen = (int)strlen(e->name);
          if (nlen == elen && !memcmp(ename, e->name, elen)) {
            char u8[4];
            int u8len = entity_utf8_encode(e->codepoint, u8);
            for (int j = 0; j < u8len && o < maxOut - 1; j++)
              out[o++] = u8[j];
            i = (int)(semi - in);
            goto next_char;
          }
        }
      }
      if (ch) { out[o++] = ch; i = (int)(semi - in); }
      else out[o++] = in[i]; /* unknown entity, keep & */
    } else {
      out[o++] = in[i];
    }
    next_char:;
  }
  out[o] = '\0';
  return o;
}

/* XML-escape text for output */
static void write_escaped(MacmbxXmlWriter *w, const char *text) {
  if (!text) return;
  for (const char *p = text; *p; p++) {
    switch (*p) {
    case '<': writer_append(w, "&lt;", 4); break;
    case '>': writer_append(w, "&gt;", 4); break;
    case '&': writer_append(w, "&amp;", 5); break;
    case '"': writer_append(w, "&quot;", 6); break;
    default: writer_char(w, *p); break;
    }
  }
}

/* ================================================================
 * Tokenizer
 * ================================================================ */

void macmbx_xml_tokenizer_init(MacmbxXmlTokenizer *t, const char *text, int len) {
  memset(t, 0, sizeof(*t));
  t->text = text;
  t->textLen = len;
}

static char peek(MacmbxXmlTokenizer *t) {
  return (t->offset < t->textLen) ? t->text[t->offset] : '\0';
}

static char advance(MacmbxXmlTokenizer *t) {
  return (t->offset < t->textLen) ? t->text[t->offset++] : '\0';
}

static void skip_ws(MacmbxXmlTokenizer *t) {
  while (t->offset < t->textLen) {
    char c = t->text[t->offset];
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
    t->offset++;
  }
}

int macmbx_xml_next_token(MacmbxXmlTokenizer *t) {
  if (t->offset >= t->textLen) {
    t->tokenType = MACMBX_XML_DONE;
    return MACMBX_XML_DONE;
  }

  if (peek(t) == '<') {
    int start = t->offset;
    advance(t); /* skip < */

    /* Comment: <!-- ... --> */
    if (t->offset + 2 < t->textLen &&
        t->text[t->offset] == '!' && t->text[t->offset+1] == '-' && t->text[t->offset+2] == '-') {
      t->offset += 3;
      while (t->offset + 2 < t->textLen) {
        if (t->text[t->offset] == '-' && t->text[t->offset+1] == '-' && t->text[t->offset+2] == '>') {
          t->offset += 3;
          break;
        }
        t->offset++;
      }
      t->tokenType = MACMBX_XML_COMMENT;
      t->tokenStart = start;
      t->tokenLen = t->offset - start;
      return MACMBX_XML_COMMENT;
    }

    /* CDATA: <![CDATA[ ... ]]> */
    if (t->offset + 7 < t->textLen && !memcmp(t->text + t->offset, "![CDATA[", 8)) {
      t->offset += 8;
      int cdata_start = t->offset;
      while (t->offset + 2 < t->textLen) {
        if (t->text[t->offset] == ']' && t->text[t->offset+1] == ']' && t->text[t->offset+2] == '>') {
          t->tokenType = MACMBX_XML_CDATA;
          t->tokenStart = cdata_start;
          t->tokenLen = t->offset - cdata_start;
          t->offset += 3;
          return MACMBX_XML_CDATA;
        }
        t->offset++;
      }
    }

    /* PI: <? ... ?> */
    if (peek(t) == '?') {
      t->offset++;
      while (t->offset + 1 < t->textLen) {
        if (t->text[t->offset] == '?' && t->text[t->offset+1] == '>') {
          t->offset += 2;
          break;
        }
        t->offset++;
      }
      t->tokenType = MACMBX_XML_PI;
      t->tokenStart = start;
      t->tokenLen = t->offset - start;
      return MACMBX_XML_PI;
    }

    /* End tag: </name> */
    if (peek(t) == '/') {
      advance(t);
      int name_start = t->offset;
      while (t->offset < t->textLen && t->text[t->offset] != '>') t->offset++;
      t->tokenType = MACMBX_XML_END_TAG;
      t->tokenStart = name_start;
      t->tokenLen = t->offset - name_start;
      /* Trim whitespace from name */
      while (t->tokenLen > 0 && (t->text[t->tokenStart + t->tokenLen - 1] == ' ' ||
             t->text[t->tokenStart + t->tokenLen - 1] == '\t'))
        t->tokenLen--;
      if (t->offset < t->textLen) t->offset++; /* skip > */
      t->attrStart = 0; t->attrLen = 0;
      return MACMBX_XML_END_TAG;
    }

    /* Element tag: <name attrs> or <name attrs /> */
    int name_start = t->offset;
    while (t->offset < t->textLen && t->text[t->offset] != '>' &&
           t->text[t->offset] != ' ' && t->text[t->offset] != '\t' &&
           t->text[t->offset] != '\r' && t->text[t->offset] != '\n' &&
           t->text[t->offset] != '/')
      t->offset++;
    t->tokenStart = name_start;
    t->tokenLen = t->offset - name_start;

    /* Attributes area */
    skip_ws(t);
    t->attrStart = t->offset;

    /* Scan to end of tag */
    bool empty = false;
    while (t->offset < t->textLen && t->text[t->offset] != '>') {
      if (t->text[t->offset] == '/' && t->offset + 1 < t->textLen && t->text[t->offset+1] == '>') {
        empty = true;
        t->attrLen = t->offset - t->attrStart;
        t->offset += 2; /* skip /> */
        t->tokenType = MACMBX_XML_EMPTY_TAG;
        return MACMBX_XML_EMPTY_TAG;
      }
      /* Skip quoted attribute values */
      if (t->text[t->offset] == '"' || t->text[t->offset] == '\'') {
        char q = t->text[t->offset++];
        while (t->offset < t->textLen && t->text[t->offset] != q) t->offset++;
        if (t->offset < t->textLen) t->offset++;
      } else {
        t->offset++;
      }
    }
    t->attrLen = t->offset - t->attrStart;
    if (t->offset < t->textLen) t->offset++; /* skip > */
    t->tokenType = MACMBX_XML_ELEMENT;
    (void)empty;
    return MACMBX_XML_ELEMENT;
  }

  /* Text content */
  int start = t->offset;
  while (t->offset < t->textLen && t->text[t->offset] != '<')
    t->offset++;
  t->tokenType = MACMBX_XML_CONTENT;
  t->tokenStart = start;
  t->tokenLen = t->offset - start;
  t->attrStart = 0; t->attrLen = 0;
  return MACMBX_XML_CONTENT;
}

int macmbx_xml_token_text(MacmbxXmlTokenizer *t, char *buf, int bufLen) {
  if (t->tokenLen <= 0 || !buf || bufLen <= 0) { if (buf) buf[0] = '\0'; return 0; }
  return expand_entities(t->text + t->tokenStart, t->tokenLen, buf, bufLen);
}

int macmbx_xml_tag_name(MacmbxXmlTokenizer *t, char *buf, int bufLen) {
  if (t->tokenLen <= 0 || !buf || bufLen <= 0) { if (buf) buf[0] = '\0'; return 0; }
  int n = t->tokenLen < bufLen - 1 ? t->tokenLen : bufLen - 1;
  memcpy(buf, t->text + t->tokenStart, n);
  buf[n] = '\0';
  return n;
}

/* ================================================================
 * Attribute parsing
 * ================================================================ */

void macmbx_xml_reset_attrs(MacmbxXmlTokenizer *t) {
  t->attrParsePos = t->attrStart;
}

int macmbx_xml_next_attr(MacmbxXmlTokenizer *t, MacmbxXmlAttr *attr) {
  if (!t || !attr) return 0;
  const char *text = t->text;
  int end = t->attrStart + t->attrLen;
  int pos = t->attrParsePos;

  /* Skip whitespace */
  while (pos < end && (text[pos] == ' ' || text[pos] == '\t' ||
         text[pos] == '\r' || text[pos] == '\n'))
    pos++;
  if (pos >= end) { t->attrParsePos = pos; return 0; }

  /* Read name */
  int ns = pos;
  while (pos < end && text[pos] != '=' && text[pos] != ' ' &&
         text[pos] != '\t' && text[pos] != '/' && text[pos] != '>')
    pos++;
  int nlen = pos - ns;
  if (nlen <= 0) { t->attrParsePos = pos; return 0; }
  if (nlen >= (int)sizeof(attr->name)) nlen = sizeof(attr->name) - 1;
  memcpy(attr->name, text + ns, nlen);
  attr->name[nlen] = '\0';

  /* Skip whitespace around = */
  while (pos < end && (text[pos] == ' ' || text[pos] == '\t')) pos++;
  if (pos < end && text[pos] == '=') {
    pos++;
    while (pos < end && (text[pos] == ' ' || text[pos] == '\t')) pos++;
  } else {
    /* Boolean attribute (no value) */
    attr->value[0] = '\0';
    t->attrParsePos = pos;
    return 1;
  }

  /* Read value */
  if (pos < end && (text[pos] == '"' || text[pos] == '\'')) {
    char q = text[pos++];
    int vs = pos;
    while (pos < end && text[pos] != q) pos++;
    int vlen = pos - vs;
    if (pos < end) pos++; /* skip closing quote */
    if (vlen >= (int)sizeof(attr->value)) vlen = sizeof(attr->value) - 1;
    expand_entities(text + vs, vlen, attr->value, sizeof(attr->value));
  } else {
    /* Unquoted value */
    int vs = pos;
    while (pos < end && text[pos] != ' ' && text[pos] != '\t' &&
           text[pos] != '>' && text[pos] != '/')
      pos++;
    int vlen = pos - vs;
    if (vlen >= (int)sizeof(attr->value)) vlen = sizeof(attr->value) - 1;
    expand_entities(text + vs, vlen, attr->value, sizeof(attr->value));
  }
  t->attrParsePos = pos;
  return 1;
}

char *macmbx_xml_get_attr(MacmbxXmlTokenizer *t, const char *name,
                           char *buf, int bufLen) {
  macmbx_xml_reset_attrs(t);
  MacmbxXmlAttr attr;
  while (macmbx_xml_next_attr(t, &attr)) {
    if (strcasecmp(attr.name, name) == 0) {
      int n = (int)strlen(attr.value);
      if (n >= bufLen) n = bufLen - 1;
      memcpy(buf, attr.value, n);
      buf[n] = '\0';
      return buf;
    }
  }
  return NULL;
}

/* ================================================================
 * DOM-like tree parser (recursive descent)
 * ================================================================ */

static void node_init(MacmbxXmlNode *n) { memset(n, 0, sizeof(*n)); }

static void node_add_child(MacmbxXmlNode *parent, MacmbxXmlNode *child) {
  if (parent->childCount >= parent->childCapacity) {
    parent->childCapacity = parent->childCapacity ? parent->childCapacity * 2 : 8;
    parent->children = (MacmbxXmlNode *)realloc(parent->children,
                        parent->childCapacity * sizeof(MacmbxXmlNode));
  }
  parent->children[parent->childCount++] = *child;
}

/* Fix parent pointers after tree construction (realloc invalidates them) */
static void fixup_parents(MacmbxXmlNode *node) {
  for (int i = 0; i < node->childCount; i++) {
    node->children[i].parent = node;
    fixup_parents(&node->children[i]);
  }
}

static MacmbxXmlNode *parse_node(MacmbxXmlTokenizer *t);

static MacmbxXmlNode *parse_element(MacmbxXmlTokenizer *t) {
  MacmbxXmlNode node;
  node_init(&node);

  /* Get tag name */
  char name[256];
  macmbx_xml_tag_name(t, name, sizeof(name));
  node.name = strdup(name);

  /* Parse attributes — single pass with growable array */
  macmbx_xml_reset_attrs(t);
  MacmbxXmlAttr tmp;
  int attr_cap = 0;
  while (macmbx_xml_next_attr(t, &tmp)) {
    if (node.attrCount >= attr_cap) {
      attr_cap = attr_cap ? attr_cap * 2 : 8;
      node.attrs = (MacmbxXmlAttr *)realloc(node.attrs, attr_cap * sizeof(MacmbxXmlAttr));
    }
    node.attrs[node.attrCount++] = tmp;
  }

  /* If empty element, we're done */
  if (t->tokenType == MACMBX_XML_EMPTY_TAG) {
    MacmbxXmlNode *heap = (MacmbxXmlNode *)malloc(sizeof(MacmbxXmlNode));
    *heap = node;
    return heap;
  }

  /* Parse children until matching end tag */
  while (1) {
    int tok = macmbx_xml_next_token(t);
    if (tok == MACMBX_XML_DONE) break;
    if (tok == MACMBX_XML_END_TAG) {
      char ename[256];
      macmbx_xml_tag_name(t, ename, sizeof(ename));
      if (strcasecmp(ename, name) == 0) break;
      /* Mismatched end tag — keep going */
      continue;
    }
    if (tok == MACMBX_XML_COMMENT || tok == MACMBX_XML_PI) continue;

    if (tok == MACMBX_XML_CONTENT || tok == MACMBX_XML_CDATA) {
      MacmbxXmlNode text_node;
      node_init(&text_node);
      char *buf = (char *)malloc(t->tokenLen + 1);
      if (tok == MACMBX_XML_CDATA) {
        memcpy(buf, t->text + t->tokenStart, t->tokenLen);
        buf[t->tokenLen] = '\0';
      } else {
        expand_entities(t->text + t->tokenStart, t->tokenLen, buf, t->tokenLen + 1);
      }
      text_node.text = buf;
      node_add_child(&node, &text_node);
    } else if (tok == MACMBX_XML_ELEMENT || tok == MACMBX_XML_EMPTY_TAG) {
      MacmbxXmlNode *child = parse_element(t);
      if (child) {
        node_add_child(&node, child);
        free(child); /* copied into children array */
      }
    }
  }

  MacmbxXmlNode *heap = (MacmbxXmlNode *)malloc(sizeof(MacmbxXmlNode));
  *heap = node;
  return heap;
}

MacmbxXmlNode *macmbx_xml_parse(const char *xml, int len) {
  if (!xml || len <= 0) return NULL;
  MacmbxXmlTokenizer t;
  macmbx_xml_tokenizer_init(&t, xml, len);

  /* Create root container */
  MacmbxXmlNode *root = (MacmbxXmlNode *)calloc(1, sizeof(MacmbxXmlNode));
  root->name = strdup("_root");

  while (1) {
    int tok = macmbx_xml_next_token(&t);
    if (tok == MACMBX_XML_DONE) break;
    if (tok == MACMBX_XML_COMMENT || tok == MACMBX_XML_PI) continue;
    if (tok == MACMBX_XML_CONTENT || tok == MACMBX_XML_CDATA) {
      MacmbxXmlNode text_node;
      node_init(&text_node);
      char *buf = (char *)malloc(t.tokenLen + 1);
      expand_entities(t.text + t.tokenStart, t.tokenLen, buf, t.tokenLen + 1);
      text_node.text = buf;
      node_add_child(root, &text_node);
    } else if (tok == MACMBX_XML_ELEMENT || tok == MACMBX_XML_EMPTY_TAG) {
      MacmbxXmlNode *child = parse_element(&t);
      if (child) { node_add_child(root, child); free(child); }
    }
  }
  fixup_parents(root);
  return root;
}

void macmbx_xml_free(MacmbxXmlNode *node) {
  if (!node) return;
  free(node->name);
  free(node->text);
  free(node->attrs);
  for (int i = 0; i < node->childCount; i++)
    macmbx_xml_free(&node->children[i]);
  free(node->children);
  /* Only free the root — children are in the array */
}

MacmbxXmlNode *macmbx_xml_find(MacmbxXmlNode *parent, const char *name) {
  if (!parent || !name) return NULL;
  for (int i = 0; i < parent->childCount; i++) {
    if (parent->children[i].name && strcasecmp(parent->children[i].name, name) == 0)
      return &parent->children[i];
  }
  return NULL;
}

/* Check if string is whitespace-only */
static bool is_ws_only(const char *s) {
  if (!s) return true;
  while (*s) {
    if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') return false;
    s++;
  }
  return true;
}

const char *macmbx_xml_text(MacmbxXmlNode *node) {
  if (!node) return "";
  /* Return first non-whitespace text child */
  for (int i = 0; i < node->childCount; i++) {
    if (node->children[i].text && !is_ws_only(node->children[i].text))
      return node->children[i].text;
  }
  /* Fall back to first text child even if whitespace */
  for (int i = 0; i < node->childCount; i++) {
    if (node->children[i].text) return node->children[i].text;
  }
  return node->text ? node->text : "";
}

char *macmbx_xml_all_text(MacmbxXmlNode *node) {
  if (!node) return strdup("");
  /* Concatenate all text from this node and all descendants (depth-first).
   * Handles mixed content: <p>Hello <b>world</b> foo</p> → "Hello world foo" */
  int cap = 256, len = 0;
  char *buf = (char *)malloc(cap);
  buf[0] = '\0';

  if (node->text) {
    int tlen = (int)strlen(node->text);
    if (len + tlen >= cap) { cap = len + tlen + 256; buf = realloc(buf, cap); }
    memcpy(buf + len, node->text, tlen);
    len += tlen;
    buf[len] = '\0';
  }

  for (int i = 0; i < node->childCount; i++) {
    char *child_text = macmbx_xml_all_text(&node->children[i]);
    if (child_text) {
      int clen = (int)strlen(child_text);
      if (clen > 0) {
        if (len + clen >= cap) { cap = len + clen + 256; buf = realloc(buf, cap); }
        memcpy(buf + len, child_text, clen);
        len += clen;
        buf[len] = '\0';
      }
      free(child_text);
    }
  }
  return buf;
}

const char *macmbx_xml_attr(MacmbxXmlNode *node, const char *name) {
  if (!node || !name) return NULL;
  for (int i = 0; i < node->attrCount; i++) {
    if (strcasecmp(node->attrs[i].name, name) == 0)
      return node->attrs[i].value;
  }
  return NULL;
}

/* ================================================================
 * Namespace support
 * ================================================================ */

const char *macmbx_xml_local_name(const char *qname) {
  if (!qname) return "";
  const char *colon = strchr(qname, ':');
  return colon ? colon + 1 : qname;
}

char *macmbx_xml_prefix(const char *qname, char *buf, int bufLen) {
  if (!buf || bufLen <= 0) return buf;
  buf[0] = '\0';
  if (!qname) return buf;
  const char *colon = strchr(qname, ':');
  if (!colon) return buf;
  int plen = (int)(colon - qname);
  if (plen >= bufLen) plen = bufLen - 1;
  memcpy(buf, qname, plen);
  buf[plen] = '\0';
  return buf;
}

const char *macmbx_xml_ns_uri(MacmbxXmlNode *node, const char *prefix) {
  if (!node) return NULL;

  /* Walk up the tree looking for xmlns declarations */
  for (MacmbxXmlNode *n = node; n != NULL; n = n->parent) {
    for (int i = 0; i < n->attrCount; i++) {
      const char *aname = n->attrs[i].name;
      if (prefix && prefix[0]) {
        /* Looking for xmlns:prefix="uri" */
        if (strncmp(aname, "xmlns:", 6) == 0 &&
            strcasecmp(aname + 6, prefix) == 0)
          return n->attrs[i].value;
      } else {
        /* Looking for default namespace xmlns="uri" */
        if (strcasecmp(aname, "xmlns") == 0)
          return n->attrs[i].value;
      }
    }
  }
  return NULL;
}

MacmbxXmlNode *macmbx_xml_find_ns(MacmbxXmlNode *parent,
                                    const char *localName,
                                    const char *nsUri) {
  if (!parent || !localName) return NULL;
  for (int i = 0; i < parent->childCount; i++) {
    MacmbxXmlNode *child = &parent->children[i];
    if (!child->name) continue;

    /* Compare local name */
    const char *clocal = macmbx_xml_local_name(child->name);
    if (strcasecmp(clocal, localName) != 0) continue;

    /* If nsUri specified, check namespace */
    if (nsUri) {
      char pfx[64];
      macmbx_xml_prefix(child->name, pfx, sizeof(pfx));
      const char *uri = macmbx_xml_ns_uri(child, pfx);
      if (!uri || strcmp(uri, nsUri) != 0) continue;
    }
    return child;
  }
  return NULL;
}

/* ================================================================
 * XML generation (writer)
 * ================================================================ */

void macmbx_xml_writer_init(MacmbxXmlWriter *w) {
  memset(w, 0, sizeof(*w));
  w->cap = 256;
  w->buf = (char *)malloc(w->cap);
  w->buf[0] = '\0';
}

void macmbx_xml_writer_free(MacmbxXmlWriter *w) {
  free(w->buf);
  memset(w, 0, sizeof(*w));
}

const char *macmbx_xml_writer_str(MacmbxXmlWriter *w) {
  return w->buf ? w->buf : "";
}

char *macmbx_xml_writer_take(MacmbxXmlWriter *w) {
  char *s = w->buf;
  w->buf = NULL;
  w->len = w->cap = 0;
  return s;
}

void macmbx_xml_open_tag(MacmbxXmlWriter *w, const char *name) {
  writer_indent(w);
  writer_char(w, '<');
  writer_append(w, name, -1);
  writer_char(w, '>');
  writer_char(w, '\n');
  w->indent++;
}

void macmbx_xml_open_tag_attrs(MacmbxXmlWriter *w, const char *name, ...) {
  writer_indent(w);
  writer_char(w, '<');
  writer_append(w, name, -1);

  va_list ap;
  va_start(ap, name);
  while (1) {
    const char *attr_name = va_arg(ap, const char *);
    if (!attr_name) break;
    const char *attr_val = va_arg(ap, const char *);
    if (!attr_val) break;
    writer_char(w, ' ');
    writer_append(w, attr_name, -1);
    writer_append(w, "=\"", 2);
    write_escaped(w, attr_val);
    writer_char(w, '"');
  }
  va_end(ap);

  writer_char(w, '>');
  writer_char(w, '\n');
  w->indent++;
}

void macmbx_xml_close_tag(MacmbxXmlWriter *w, const char *name) {
  if (w->indent > 0) w->indent--;
  writer_indent(w);
  writer_append(w, "</", 2);
  writer_append(w, name, -1);
  writer_char(w, '>');
  writer_char(w, '\n');
}

void macmbx_xml_empty_tag(MacmbxXmlWriter *w, const char *name) {
  writer_indent(w);
  writer_char(w, '<');
  writer_append(w, name, -1);
  writer_append(w, " />\n", 4);
}

void macmbx_xml_write_text(MacmbxXmlWriter *w, const char *text) {
  write_escaped(w, text);
}

void macmbx_xml_write_element(MacmbxXmlWriter *w, const char *name, const char *value) {
  writer_indent(w);
  writer_char(w, '<');
  writer_append(w, name, -1);
  writer_char(w, '>');
  if (value) write_escaped(w, value);
  writer_append(w, "</", 2);
  writer_append(w, name, -1);
  writer_char(w, '>');
  writer_char(w, '\n');
}

void macmbx_xml_write_int(MacmbxXmlWriter *w, const char *name, long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%ld", value);
  macmbx_xml_write_element(w, name, buf);
}

void macmbx_xml_write_raw(MacmbxXmlWriter *w, const char *text) {
  if (text) writer_append(w, text, -1);
}

void macmbx_xml_write_crlf(MacmbxXmlWriter *w) {
  writer_append(w, "\r\n", 2);
}

void macmbx_xml_indent_inc(MacmbxXmlWriter *w) { w->indent++; }
void macmbx_xml_indent_dec(MacmbxXmlWriter *w) { if (w->indent > 0) w->indent--; }
