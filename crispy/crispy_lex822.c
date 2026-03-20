/* crispy_lex822.c — RFC 822 header lexical analyzer
 * Part of crispy: standalone mail library.
 *
 * Ported from Eudora lex822.c, rewritten standalone.
 * No transport dependency — works on in-memory buffers.
 */

#include "crispy_lex822.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* ================================================================
 * Character classification (RFC 822)
 * ================================================================ */

static bool is_special(unsigned char c) {
  return c == '(' || c == ')' || c == '<' || c == '>' ||
         c == '@' || c == ',' || c == ';' || c == ':' ||
         c == '\\' || c == '"' || c == '.' || c == '[' || c == ']';
}

static bool is_lwsp(unsigned char c) {
  return c == ' ' || c == '\t';
}

static bool is_atom_char(unsigned char c) {
  return c > 32 && c != 127 && !is_special(c);
}

bool crispy_lex_is_special(char c) {
  return is_special((unsigned char)c);
}

bool crispy_lex_needs_quoting(const char *s) {
  if (!s) return false;
  while (*s) {
    unsigned char c = (unsigned char)*s;
    if (is_special(c) || is_lwsp(c)) return true;
    s++;
  }
  return false;
}

/* ================================================================
 * Lexer
 * ================================================================ */

void crispy_lex_init(CrispyLexer *lex, const char *header_value, bool structured) {
  if (!lex) return;
  lex->input = header_value;
  lex->pos = header_value;
  lex->end = header_value ? header_value + strlen(header_value) : header_value;
  lex->structured = structured;
}

void crispy_lex_reset(CrispyLexer *lex) {
  if (lex) lex->pos = lex->input;
}

static void tok_init(CrispyToken *tok, CrispyTokenType type, const char *start) {
  tok->type = type;
  tok->start = start;
  tok->len = 0;
  tok->value[0] = '\0';
  tok->value_len = 0;
}

static void tok_add(CrispyToken *tok, char c) {
  if (tok->value_len < (int)sizeof(tok->value) - 1)
    tok->value[tok->value_len++] = c;
  tok->value[tok->value_len] = '\0';
}

/* Skip folding whitespace: CRLF followed by space/tab */
static void skip_fws(CrispyLexer *lex) {
  while (lex->pos < lex->end) {
    if (*lex->pos == '\r' && lex->pos + 1 < lex->end && lex->pos[1] == '\n' &&
        lex->pos + 2 < lex->end && is_lwsp((unsigned char)lex->pos[2])) {
      lex->pos += 2; /* skip CRLF, the space/tab will be consumed as LWSP */
    } else if (*lex->pos == '\n' &&
               lex->pos + 1 < lex->end && is_lwsp((unsigned char)lex->pos[1])) {
      lex->pos++; /* skip LF, space/tab next */
    } else {
      break;
    }
  }
}

CrispyTokenType crispy_lex_next(CrispyLexer *lex, CrispyToken *tok) {
  if (!lex || !tok || !lex->pos || lex->pos >= lex->end) {
    if (tok) { tok->type = CRISPY_TOK_END; tok->len = 0; tok->value[0] = '\0'; tok->value_len = 0; }
    return CRISPY_TOK_END;
  }

  skip_fws(lex);
  if (lex->pos >= lex->end) {
    tok->type = CRISPY_TOK_END;
    return CRISPY_TOK_END;
  }

  unsigned char c = (unsigned char)*lex->pos;
  const char *start = lex->pos;

  /* Whitespace */
  if (is_lwsp(c)) {
    tok_init(tok, CRISPY_TOK_LWSP, start);
    while (lex->pos < lex->end && is_lwsp((unsigned char)*lex->pos)) {
      tok_add(tok, ' '); /* normalize to single space */
      lex->pos++;
      skip_fws(lex);
    }
    tok->value_len = 1; tok->value[0] = ' '; tok->value[1] = '\0';
    tok->len = (int)(lex->pos - start);
    return CRISPY_TOK_LWSP;
  }

  /* Quoted string */
  if (c == '"') {
    tok_init(tok, CRISPY_TOK_QSTRING, start);
    lex->pos++; /* skip opening quote */
    while (lex->pos < lex->end && *lex->pos != '"') {
      if (*lex->pos == '\\' && lex->pos + 1 < lex->end) {
        lex->pos++; /* skip backslash */
        tok_add(tok, *lex->pos);
      } else {
        tok_add(tok, *lex->pos);
      }
      lex->pos++;
    }
    if (lex->pos < lex->end) lex->pos++; /* skip closing quote */
    tok->len = (int)(lex->pos - start);
    return CRISPY_TOK_QSTRING;
  }

  /* Comment */
  if (c == '(' && lex->structured) {
    tok_init(tok, CRISPY_TOK_COMMENT, start);
    lex->pos++; /* skip ( */
    int depth = 1;
    while (lex->pos < lex->end && depth > 0) {
      if (*lex->pos == '(') depth++;
      else if (*lex->pos == ')') { depth--; if (depth == 0) break; }
      else if (*lex->pos == '\\' && lex->pos + 1 < lex->end) {
        lex->pos++;
        tok_add(tok, *lex->pos);
        lex->pos++;
        continue;
      }
      if (depth > 0) tok_add(tok, *lex->pos);
      lex->pos++;
    }
    if (lex->pos < lex->end && *lex->pos == ')') lex->pos++;
    tok->len = (int)(lex->pos - start);
    return CRISPY_TOK_COMMENT;
  }

  /* Domain literal */
  if (c == '[' && lex->structured) {
    tok_init(tok, CRISPY_TOK_DOMAIN_LIT, start);
    lex->pos++; /* skip [ */
    while (lex->pos < lex->end && *lex->pos != ']') {
      if (*lex->pos == '\\' && lex->pos + 1 < lex->end) {
        lex->pos++;
        tok_add(tok, *lex->pos);
      } else {
        tok_add(tok, *lex->pos);
      }
      lex->pos++;
    }
    if (lex->pos < lex->end) lex->pos++; /* skip ] */
    tok->len = (int)(lex->pos - start);
    return CRISPY_TOK_DOMAIN_LIT;
  }

  /* Special characters (structured mode) */
  if (lex->structured && is_special(c) && c != '"' && c != '(' && c != '[') {
    tok_init(tok, CRISPY_TOK_SPECIAL, start);
    tok_add(tok, c);
    lex->pos++;
    tok->len = 1;
    return CRISPY_TOK_SPECIAL;
  }

  /* Atom (structured) or text (unstructured) */
  if (lex->structured) {
    tok_init(tok, CRISPY_TOK_ATOM, start);
    while (lex->pos < lex->end && is_atom_char((unsigned char)*lex->pos)) {
      tok_add(tok, *lex->pos);
      lex->pos++;
    }
    tok->len = (int)(lex->pos - start);
    return CRISPY_TOK_ATOM;
  }

  /* Unstructured: collect everything as text until end */
  tok_init(tok, CRISPY_TOK_TEXT, start);
  while (lex->pos < lex->end) {
    unsigned char ch = (unsigned char)*lex->pos;
    if (ch == '\r' || ch == '\n') {
      /* Check for folding */
      const char *save = lex->pos;
      skip_fws(lex);
      if (lex->pos > save && lex->pos < lex->end && is_lwsp((unsigned char)*lex->pos)) {
        tok_add(tok, ' ');
        lex->pos++;
        continue;
      }
      if (lex->pos == save) break; /* real end of header */
      continue;
    }
    tok_add(tok, *lex->pos);
    lex->pos++;
  }
  tok->len = (int)(lex->pos - start);
  return CRISPY_TOK_TEXT;
}

CrispyTokenType crispy_lex_peek(CrispyLexer *lex) {
  if (!lex) return CRISPY_TOK_END;
  const char *saved = lex->pos;
  CrispyToken tok;
  CrispyTokenType type = crispy_lex_next(lex, &tok);
  lex->pos = saved;
  return type;
}

/* ================================================================
 * Tokenize entire header into array
 * ================================================================ */

int crispy_lex_tokenize(const char *header_value, bool structured,
                         CrispyToken **tokens) {
  if (!header_value || !tokens) return 0;
  *tokens = NULL;

  CrispyLexer lex;
  crispy_lex_init(&lex, header_value, structured);

  int count = 0, cap = 32;
  *tokens = (CrispyToken *)calloc(cap, sizeof(CrispyToken));

  CrispyToken tok;
  while (crispy_lex_next(&lex, &tok) != CRISPY_TOK_END) {
    if (count >= cap) {
      cap *= 2;
      *tokens = (CrispyToken *)realloc(*tokens, cap * sizeof(CrispyToken));
    }
    (*tokens)[count++] = tok;
  }
  return count;
}

/* ================================================================
 * RFC 822 quoting
 * ================================================================ */

char *crispy_lex_quote(char *dst, size_t dstSize,
                        const char *src, bool escape_spaces) {
  if (!dst || !src || dstSize < 3) return dst;
  size_t o = 0;
  dst[o++] = '"';
  while (*src && o < dstSize - 2) {
    if (*src == '"' || *src == '\\') {
      if (o < dstSize - 3) { dst[o++] = '\\'; dst[o++] = *src; }
    } else if (escape_spaces && *src == ' ') {
      if (o < dstSize - 3) { dst[o++] = '\\'; dst[o++] = ' '; }
    } else {
      dst[o++] = *src;
    }
    src++;
  }
  dst[o++] = '"';
  dst[o] = '\0';
  return dst;
}

/* ================================================================
 * PseudoQP — fix broken QP in headers
 * Some spam has =XX sequences in unencoded headers.
 * ================================================================ */

static int hex_val(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

void crispy_lex_fix_pseudo_qp(char *text) {
  if (!text) return;
  char *src = text, *dst = text;
  while (*src) {
    if (*src == '=' && src[1] && src[2]) {
      int h = hex_val((unsigned char)src[1]);
      int l = hex_val((unsigned char)src[2]);
      if (h >= 0 && l >= 0) {
        unsigned char decoded = (unsigned char)((h << 4) | l);
        if (decoded >= 32 || decoded == '\t') {
          *dst++ = (char)decoded;
          src += 3;
          continue;
        }
      }
    }
    *dst++ = *src++;
  }
  *dst = '\0';
}

/* ================================================================
 * Inline base64 decode (for RFC 2047 B-encoded fragments in headers)
 * ================================================================ */

static const unsigned char b64_table[256] = {
  ['A']=0,['B']=1,['C']=2,['D']=3,['E']=4,['F']=5,['G']=6,['H']=7,
  ['I']=8,['J']=9,['K']=10,['L']=11,['M']=12,['N']=13,['O']=14,['P']=15,
  ['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,
  ['Y']=24,['Z']=25,['a']=26,['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,
  ['g']=32,['h']=33,['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,
  ['o']=40,['p']=41,['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,
  ['w']=48,['x']=49,['y']=50,['z']=51,['0']=52,['1']=53,['2']=54,['3']=55,
  ['4']=56,['5']=57,['6']=58,['7']=59,['8']=60,['9']=61,['+']=62,['/']=63,
};

int crispy_lex_decode_b64_inline(char *s) {
  if (!s) return 0;
  int len = (int)strlen(s);
  unsigned char *src = (unsigned char *)s;
  unsigned char *dst = (unsigned char *)s;
  int out = 0;

  int i = 0;
  while (i < len) {
    /* Skip non-base64 chars */
    while (i < len && src[i] != '+' && src[i] != '/' &&
           !isalnum((unsigned char)src[i]) && src[i] != '=') i++;
    if (i >= len) break;

    unsigned char a = (i < len) ? b64_table[src[i++]] : 0;
    unsigned char b = (i < len) ? b64_table[src[i++]] : 0;
    unsigned char c = (i < len) ? b64_table[src[i++]] : 0;
    unsigned char d = (i < len) ? b64_table[src[i++]] : 0;

    dst[out++] = (a << 2) | (b >> 4);
    if (i >= 3 && src[i-2] != '=') dst[out++] = (b << 4) | (c >> 2);
    if (i >= 4 && src[i-1] != '=') dst[out++] = (c << 6) | d;
  }
  dst[out] = '\0';
  return out;
}
