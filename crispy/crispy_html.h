/* crispy_html.h — HTML tokenizer and parser
 * Part of crispy: standalone mail library.
 *
 * Full HTML tokenizer for email messages:
 *   - Token-based parser (tags, text, entities, comments)
 *   - Entity decoding (named + numeric)
 *   - Attribute parsing
 *   - Color parsing (named colors + #hex)
 *   - Charset detection from <meta>
 *   - Tag enumeration
 *
 * Standalone: no GTK, no Eudora dependency.
 */

#ifndef CRISPY_HTML_H
#define CRISPY_HTML_H

#include <stddef.h>
#include <stdbool.h>

/* ================================================================
 * Token types
 * ================================================================ */

typedef enum {
  CRISPY_HTML_TEXT,          /* plain text between tags */
  CRISPY_HTML_TAG_OPEN,     /* <tag ...> */
  CRISPY_HTML_TAG_CLOSE,    /* </tag> */
  CRISPY_HTML_TAG_SELF,     /* <tag ... /> */
  CRISPY_HTML_COMMENT,      /* <!-- ... --> */
  CRISPY_HTML_DOCTYPE,      /* <!DOCTYPE ...> */
  CRISPY_HTML_ENTITY,       /* &entity; or &#123; */
  CRISPY_HTML_EOF,          /* end of input */
} CrispyHtmlTokenType;

/* ================================================================
 * Tag enumeration (common HTML tags in email)
 * ================================================================ */

typedef enum {
  CRISPY_TAG_UNKNOWN = 0,
  CRISPY_TAG_HTML, CRISPY_TAG_HEAD, CRISPY_TAG_BODY, CRISPY_TAG_TITLE,
  CRISPY_TAG_P, CRISPY_TAG_BR, CRISPY_TAG_HR, CRISPY_TAG_DIV, CRISPY_TAG_SPAN,
  CRISPY_TAG_A, CRISPY_TAG_IMG, CRISPY_TAG_B, CRISPY_TAG_I, CRISPY_TAG_U,
  CRISPY_TAG_STRONG, CRISPY_TAG_EM, CRISPY_TAG_FONT, CRISPY_TAG_BIG, CRISPY_TAG_SMALL,
  CRISPY_TAG_H1, CRISPY_TAG_H2, CRISPY_TAG_H3, CRISPY_TAG_H4, CRISPY_TAG_H5, CRISPY_TAG_H6,
  CRISPY_TAG_UL, CRISPY_TAG_OL, CRISPY_TAG_LI, CRISPY_TAG_DL, CRISPY_TAG_DT, CRISPY_TAG_DD,
  CRISPY_TAG_TABLE, CRISPY_TAG_TR, CRISPY_TAG_TD, CRISPY_TAG_TH, CRISPY_TAG_CAPTION,
  CRISPY_TAG_PRE, CRISPY_TAG_CODE, CRISPY_TAG_BLOCKQUOTE,
  CRISPY_TAG_STYLE, CRISPY_TAG_SCRIPT, CRISPY_TAG_META, CRISPY_TAG_LINK,
  CRISPY_TAG_SUP, CRISPY_TAG_SUB, CRISPY_TAG_S, CRISPY_TAG_STRIKE, CRISPY_TAG_CENTER,
  CRISPY_TAG_FORM, CRISPY_TAG_INPUT, CRISPY_TAG_TEXTAREA, CRISPY_TAG_SELECT, CRISPY_TAG_OPTION,
  CRISPY_TAG_IFRAME, CRISPY_TAG_OBJECT, CRISPY_TAG_EMBED,
  CRISPY_TAG_THEAD, CRISPY_TAG_TBODY, CRISPY_TAG_TFOOT, CRISPY_TAG_COL, CRISPY_TAG_COLGROUP,
} CrispyHtmlTag;

/* ================================================================
 * Token
 * ================================================================ */

typedef struct {
  CrispyHtmlTokenType type;
  CrispyHtmlTag tag;          /* parsed tag enum (for tag tokens) */
  const char *start;          /* pointer into source text */
  long len;                   /* length of token text */
  char tag_name[32];          /* lowercase tag name */
} CrispyHtmlToken;

/* ================================================================
 * Tokenizer context
 * ================================================================ */

typedef struct {
  const char *html;           /* source text */
  long len;                   /* total length */
  long pos;                   /* current position */
} CrispyHtmlTokenizer;

/* ================================================================
 * Tokenizer API
 * ================================================================ */

/* Initialize tokenizer */
void crispy_html_init(CrispyHtmlTokenizer *t, const char *html, long len);

/* Get next token. Returns false at EOF. */
bool crispy_html_next(CrispyHtmlTokenizer *t, CrispyHtmlToken *tok);

/* ================================================================
 * Entity decoding
 * ================================================================ */

/* Decode a single HTML entity (e.g. "&amp;" → "&", "&#65;" → "A").
 * Input points to the '&'. Returns decoded char, sets *advance to
 * bytes consumed from input. */
int crispy_html_decode_entity(const char *entity, int *advance);

/* Decode all HTML entities in a string. Returns malloc'd result. */
char *crispy_html_decode_entities(const char *text, long len);

/* ================================================================
 * Attribute parsing
 * ================================================================ */

/* Get an attribute value from a tag token.
 * Returns malloc'd string, or NULL if not found. */
char *crispy_html_get_attr(const char *tag_text, long tag_len, const char *attr_name);

/* ================================================================
 * Color parsing
 * ================================================================ */

typedef struct { unsigned char r, g, b; } CrispyColor;

/* Parse an HTML color (named or #hex). Returns true on success. */
bool crispy_html_parse_color(const char *color_str, CrispyColor *out);

/* ================================================================
 * Charset detection
 * ================================================================ */

/* Scan HTML for <meta charset> or <meta http-equiv="Content-Type">.
 * Returns malloc'd charset string (e.g. "utf-8"), or NULL. */
char *crispy_html_detect_charset(const char *html, long len);

/* ================================================================
 * Tag lookup
 * ================================================================ */

/* Map tag name string to enum. */
CrispyHtmlTag crispy_html_tag_from_name(const char *name);

/* Is this tag a block-level element? */
bool crispy_html_is_block(CrispyHtmlTag tag);

/* Is this tag a void/self-closing element? */
bool crispy_html_is_void(CrispyHtmlTag tag);

#endif /* CRISPY_HTML_H */
