/* crispy_lex822.h — RFC 822 header lexical analyzer
 * Part of crispy: standalone mail library.
 *
 * Tokenizes RFC 822 header fields into atoms, quoted strings,
 * comments, domain literals, specials, and whitespace.
 * Standalone — works on buffers, no transport dependency.
 *
 * Also: RFC 2047 encoded-word fixing (PseudoQP), RFC 822 quoting.
 */

#ifndef CRISPY_LEX822_H
#define CRISPY_LEX822_H

#include <stddef.h>
#include <stdbool.h>

/* Token types produced by the lexer */
typedef enum {
  CRISPY_TOK_LWSP,       /* linear whitespace (space, tab, folding) */
  CRISPY_TOK_ATOM,       /* unquoted word (alphanumeric + allowed chars) */
  CRISPY_TOK_QSTRING,    /* quoted string (content between quotes, unescaped) */
  CRISPY_TOK_COMMENT,    /* RFC 822 comment (content between parens) */
  CRISPY_TOK_DOMAIN_LIT, /* domain literal [content] */
  CRISPY_TOK_SPECIAL,    /* single special character: ()<>@,;:\".[] */
  CRISPY_TOK_TEXT,       /* unstructured text (body of unstructured headers) */
  CRISPY_TOK_END,        /* end of header */
  CRISPY_TOK_ERROR,      /* parse error */
} CrispyTokenType;

/* A single token */
typedef struct {
  CrispyTokenType type;
  const char *start;     /* pointer into original input */
  int len;               /* length of token text */
  char value[512];       /* processed value (unescaped/decoded) */
  int value_len;
} CrispyToken;

/* Lexer state */
typedef struct {
  const char *input;     /* original header text */
  const char *pos;       /* current position */
  const char *end;       /* end of input */
  bool structured;       /* true = structured header (addresses, content-type) */
                         /* false = unstructured (subject, comments only) */
} CrispyLexer;

/* ================================================================
 * Lexer API
 * ================================================================ */

/* Initialize lexer on a header value string.
 * structured: true for address/content-type headers where specials matter,
 *             false for Subject/Comments where everything is text. */
void crispy_lex_init(CrispyLexer *lex, const char *header_value, bool structured);

/* Get next token. Returns token type. Fills tok with details.
 * Returns CRISPY_TOK_END when no more tokens. */
CrispyTokenType crispy_lex_next(CrispyLexer *lex, CrispyToken *tok);

/* Peek at next token type without consuming it. */
CrispyTokenType crispy_lex_peek(CrispyLexer *lex);

/* Reset lexer to beginning. */
void crispy_lex_reset(CrispyLexer *lex);

/* ================================================================
 * Convenience: tokenize entire header into array
 * ================================================================ */

/* Tokenize a header value into an array of tokens.
 * Returns count, allocates *tokens. Caller frees *tokens. */
int crispy_lex_tokenize(const char *header_value, bool structured,
                         CrispyToken **tokens);

/* ================================================================
 * Utilities
 * ================================================================ */

/* Quote a string for RFC 822 if it contains specials.
 * Adds quotes and escapes internal quotes/backslashes.
 * dst must be >= 2*srcLen + 3. Returns dst. */
char *crispy_lex_quote(char *dst, size_t dstSize,
                        const char *src, bool escape_spaces);

/* Fix broken quoted-printable in headers.
 * Some spam has raw QP in unencoded headers (=20, =0D, etc.).
 * Decodes in place. */
void crispy_lex_fix_pseudo_qp(char *text);

/* Decode a base64-encoded string in place (for inline B64 in headers).
 * Returns decoded length. */
int crispy_lex_decode_b64_inline(char *s);

/* Check if a character is an RFC 822 special: ()<>@,;:\".[] */
bool crispy_lex_is_special(char c);

/* Check if a string needs quoting for RFC 822. */
bool crispy_lex_needs_quoting(const char *s);

#endif /* CRISPY_LEX822_H */
