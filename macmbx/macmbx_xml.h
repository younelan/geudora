/* macmbx_xml.h — Portable XML parser and generator for macmbx
 *
 * Standalone: no GLib, no libxml2, no Eudora, no GTK.
 * Only standard C.
 *
 * Ported from original Eudora xml.c — tokenizer + recursive descent parser.
 */

#ifndef MACMBX_XML_H
#define MACMBX_XML_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Token types ──────────────────────────────────────────────── */
enum {
  MACMBX_XML_DONE        = 0,
  MACMBX_XML_ELEMENT     = 1,  /* <tag ...>   */
  MACMBX_XML_CONTENT     = 2,  /* text content */
  MACMBX_XML_END_TAG     = 3,  /* </tag>      */
  MACMBX_XML_EMPTY_TAG   = 4,  /* <tag ... /> */
  MACMBX_XML_COMMENT     = 5,  /* <!-- ... --> */
  MACMBX_XML_PI          = 6,  /* <?...?>     */
  MACMBX_XML_CDATA       = 7,  /* <![CDATA[...]]> */
};

/* ── Tokenizer ────────────────────────────────────────────────── */

typedef struct {
  const char *text;      /* input XML text */
  int textLen;           /* total length */
  int offset;            /* current read position */
  /* Last token */
  int tokenType;
  int tokenStart;        /* byte offset of token in text */
  int tokenLen;          /* byte length of token */
  int attrStart;         /* offset where attributes begin (for element tags) */
  int attrLen;           /* length of attribute area */
  int attrParsePos;      /* current position in attribute parsing (per-instance) */
} MacmbxXmlTokenizer;

/* Initialize tokenizer on a buffer. Does not copy — text must outlive tokenizer. */
void macmbx_xml_tokenizer_init(MacmbxXmlTokenizer *t, const char *text, int len);

/* Get next token. Returns token type (MACMBX_XML_DONE when finished). */
int macmbx_xml_next_token(MacmbxXmlTokenizer *t);

/* Copy current token text to buffer (with entity expansion: &lt; → <, etc).
 * Returns bytes written (not including NUL). */
int macmbx_xml_token_text(MacmbxXmlTokenizer *t, char *buf, int bufLen);

/* Get tag name from current element/end tag. NUL-terminated. */
int macmbx_xml_tag_name(MacmbxXmlTokenizer *t, char *buf, int bufLen);

/* ── Attribute parsing ────────────────────────────────────────── */

typedef struct {
  char name[128];
  char value[512];
} MacmbxXmlAttr;

/* Parse next attribute from current element tag.
 * Returns 1 if attribute found, 0 if no more. */
int macmbx_xml_next_attr(MacmbxXmlTokenizer *t, MacmbxXmlAttr *attr);

/* Reset attribute parsing to beginning of current tag's attributes. */
void macmbx_xml_reset_attrs(MacmbxXmlTokenizer *t);

/* Get a specific attribute value by name. Returns NULL if not found.
 * Writes to buf, returns buf or NULL. */
char *macmbx_xml_get_attr(MacmbxXmlTokenizer *t, const char *name,
                           char *buf, int bufLen);

/* ── DOM-like tree parser ─────────────────────────────────────── */

typedef struct MacmbxXmlNode MacmbxXmlNode;

struct MacmbxXmlNode {
  char *name;             /* tag name (NULL for text nodes), may include prefix */
  char *text;             /* text content (NULL for element nodes) */
  MacmbxXmlAttr *attrs;   /* attribute array */
  int attrCount;
  MacmbxXmlNode *children;  /* child nodes array */
  int childCount;
  int childCapacity;
  MacmbxXmlNode *parent;    /* parent node (NULL for root) */
};

/* Parse XML into a tree. Returns root node (caller must free with macmbx_xml_free).
 * Returns NULL on error. */
MacmbxXmlNode *macmbx_xml_parse(const char *xml, int len);

/* Free a node tree. */
void macmbx_xml_free(MacmbxXmlNode *node);

/* Find a child element by name. Returns NULL if not found. */
MacmbxXmlNode *macmbx_xml_find(MacmbxXmlNode *parent, const char *name);

/* Get text content of first non-whitespace child text node. Returns "" if none. */
const char *macmbx_xml_text(MacmbxXmlNode *node);

/* Get ALL text content concatenated from node and descendants (mixed content).
 * For <p>Hello <b>world</b> foo</p> returns "Hello world foo".
 * Returns malloc'd string (caller frees). */
char *macmbx_xml_all_text(MacmbxXmlNode *node);

/* Get attribute value. Returns NULL if not found. */
const char *macmbx_xml_attr(MacmbxXmlNode *node, const char *name);

/* ── Namespace support ────────────────────────────────────────── */

/* Get local name from a possibly-prefixed tag name.
 * "soap:Envelope" → "Envelope", "div" → "div".
 * Returns pointer into the name string (not a copy). */
const char *macmbx_xml_local_name(const char *qname);

/* Get prefix from a possibly-prefixed tag name.
 * "soap:Envelope" → writes "soap" to buf. "div" → writes "" to buf.
 * Returns buf. */
char *macmbx_xml_prefix(const char *qname, char *buf, int bufLen);

/* Get namespace URI for a prefix at a given node.
 * Walks up the tree looking for xmlns:prefix="uri" declarations.
 * For default namespace, pass prefix="" (looks for xmlns="uri").
 * Returns URI string or NULL if not declared. */
const char *macmbx_xml_ns_uri(MacmbxXmlNode *node, const char *prefix);

/* Find child element by local name and namespace URI.
 * Like macmbx_xml_find but namespace-aware. nsUri may be NULL to ignore. */
MacmbxXmlNode *macmbx_xml_find_ns(MacmbxXmlNode *parent,
                                    const char *localName,
                                    const char *nsUri);

/* ── XML generation ───────────────────────────────────────────── */

typedef struct {
  char *buf;
  int len;
  int cap;
  int indent;
} MacmbxXmlWriter;

/* Initialize writer. */
void macmbx_xml_writer_init(MacmbxXmlWriter *w);

/* Free writer buffer. */
void macmbx_xml_writer_free(MacmbxXmlWriter *w);

/* Get result string (NUL-terminated). Writer still owns memory. */
const char *macmbx_xml_writer_str(MacmbxXmlWriter *w);

/* Take ownership of result. Caller must free. Resets writer. */
char *macmbx_xml_writer_take(MacmbxXmlWriter *w);

/* Write opening tag. */
void macmbx_xml_open_tag(MacmbxXmlWriter *w, const char *name);

/* Write opening tag with attributes (NULL-terminated name/value pairs). */
void macmbx_xml_open_tag_attrs(MacmbxXmlWriter *w, const char *name, ...);

/* Write closing tag. */
void macmbx_xml_close_tag(MacmbxXmlWriter *w, const char *name);

/* Write empty element <name/>. */
void macmbx_xml_empty_tag(MacmbxXmlWriter *w, const char *name);

/* Write text content (escaped). */
void macmbx_xml_write_text(MacmbxXmlWriter *w, const char *text);

/* Write <name>value</name> on one line. */
void macmbx_xml_write_element(MacmbxXmlWriter *w, const char *name, const char *value);

/* Write <name>intvalue</name>. */
void macmbx_xml_write_int(MacmbxXmlWriter *w, const char *name, long value);

/* Write raw string (no escaping). */
void macmbx_xml_write_raw(MacmbxXmlWriter *w, const char *text);

/* Write CRLF. */
void macmbx_xml_write_crlf(MacmbxXmlWriter *w);

/* Increase indent. */
void macmbx_xml_indent_inc(MacmbxXmlWriter *w);

/* Decrease indent. */
void macmbx_xml_indent_dec(MacmbxXmlWriter *w);

#ifdef __cplusplus
}
#endif

#endif /* MACMBX_XML_H */
