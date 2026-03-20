/* crispy_richtext.c — Rich text format conversions
 * Part of crispy: standalone mail library.
 *
 * Ported from Eudora rich.c, rewritten standalone.
 * No PETE, no UI — pure text transforms.
 */

#include "crispy_richtext.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ================================================================
 * Dynamic string buffer
 * ================================================================ */

typedef struct {
  char *data;
  size_t len, cap;
} Buf;

static void buf_init(Buf *b) { b->cap = 1024; b->data = malloc(b->cap); b->len = 0; b->data[0] = '\0'; }
static void buf_grow(Buf *b, size_t need) {
  if (b->len + need + 1 > b->cap) {
    while (b->len + need + 1 > b->cap) b->cap *= 2;
    b->data = realloc(b->data, b->cap);
  }
}
static void buf_add(Buf *b, const char *s, size_t n) { buf_grow(b, n); memcpy(b->data + b->len, s, n); b->len += n; b->data[b->len] = '\0'; }
static void buf_adds(Buf *b, const char *s) { buf_add(b, s, strlen(s)); }
static void buf_addc(Buf *b, char c) { buf_grow(b, 1); b->data[b->len++] = c; b->data[b->len] = '\0'; }
static char *buf_detach(Buf *b) { char *r = b->data; b->data = NULL; b->len = b->cap = 0; return r; }

/* ================================================================
 * Text/enriched → HTML
 *
 * RFC 1896 enriched tags → HTML mapping:
 *   <bold> → <b>, <italic> → <i>, <underline> → <u>
 *   <fixed> → <tt>, <bigger> → <big>, <smaller> → <small>
 *   <center> → <center>, <flushleft> → <div align=left>
 *   <flushright> → <div align=right>
 *   <excerpt> → <blockquote>, <nofill> → <pre>
 *   <color><param>RRGGBB</param> → <span style="color:#RRGGBB">
 *   <fontfamily><param>name</param> → <span style="font-family:name">
 *   << → literal <
 * ================================================================ */

/* Find enriched tag: returns tag name (without < >), advances *p past > */
static const char *parse_enriched_tag(const char **p, const char *end,
                                       char *tag, size_t tagSz, bool *is_close) {
  if (*p >= end || **p != '<') return NULL;
  (*p)++; /* skip < */

  /* Check for << (literal <) */
  if (*p < end && **p == '<') { (*p)++; return NULL; /* caller handles << */ }

  *is_close = false;
  if (*p < end && **p == '/') { *is_close = true; (*p)++; }

  size_t i = 0;
  while (*p < end && **p != '>' && i < tagSz - 1) {
    tag[i++] = **p;
    (*p)++;
  }
  tag[i] = '\0';
  if (*p < end && **p == '>') (*p)++;

  /* Lowercase tag name */
  for (size_t j = 0; j < i; j++) tag[j] = tolower((unsigned char)tag[j]);
  return tag;
}

char *crispy_enriched_to_html(const char *enriched, long len) {
  if (!enriched) return strdup("");
  if (len < 0) len = (long)strlen(enriched);
  const char *p = enriched, *end = enriched + len;
  Buf b; buf_init(&b);

  buf_adds(&b, "<html><body>");
  bool in_nofill = false;
  bool in_param = false;
  char param_buf[256]; int param_len = 0;
  char pending_tag[64]; pending_tag[0] = '\0';

  while (p < end) {
    if (*p == '<') {
      char tag[64]; bool is_close;

      /* Check << (literal <) */
      if (p + 1 < end && p[1] == '<') {
        if (in_param) { if (param_len < 255) param_buf[param_len++] = '<'; }
        else buf_adds(&b, "&lt;");
        p += 2;
        continue;
      }

      parse_enriched_tag(&p, end, tag, sizeof(tag), &is_close);

      if (strcmp(tag, "param") == 0) {
        if (!is_close) { in_param = true; param_len = 0; param_buf[0] = '\0'; }
        else {
          in_param = false; param_buf[param_len] = '\0';
          /* Apply param to pending tag */
          if (strcmp(pending_tag, "color") == 0) {
            /* param is RRGG,BBXX or RRGGBB */
            char color[8] = "#000000";
            if (param_len >= 4 && strchr(param_buf, ',')) {
              /* RRRR,GGGG,BBBB format */
              unsigned r = 0, g = 0, b2 = 0;
              sscanf(param_buf, "%x,%x,%x", &r, &g, &b2);
              snprintf(color, sizeof(color), "#%02x%02x%02x", r>>8, g>>8, b2>>8);
            } else if (param_len >= 6) {
              snprintf(color, sizeof(color), "#%.6s", param_buf);
            }
            char span[80];
            snprintf(span, sizeof(span), "<span style=\"color:%s\">", color);
            buf_adds(&b, span);
          } else if (strcmp(pending_tag, "fontfamily") == 0) {
            char span[128];
            snprintf(span, sizeof(span), "<span style=\"font-family:%s\">", param_buf);
            buf_adds(&b, span);
          }
          pending_tag[0] = '\0';
        }
        continue;
      }

      if (in_param) {
        /* Collect param content */
        /* Already advanced past tag — this shouldn't happen normally */
        continue;
      }

      /* Map enriched tags to HTML */
      if (strcmp(tag, "bold") == 0)
        buf_adds(&b, is_close ? "</b>" : "<b>");
      else if (strcmp(tag, "italic") == 0)
        buf_adds(&b, is_close ? "</i>" : "<i>");
      else if (strcmp(tag, "underline") == 0)
        buf_adds(&b, is_close ? "</u>" : "<u>");
      else if (strcmp(tag, "fixed") == 0)
        buf_adds(&b, is_close ? "</tt>" : "<tt>");
      else if (strcmp(tag, "bigger") == 0)
        buf_adds(&b, is_close ? "</big>" : "<big>");
      else if (strcmp(tag, "smaller") == 0)
        buf_adds(&b, is_close ? "</small>" : "<small>");
      else if (strcmp(tag, "center") == 0)
        buf_adds(&b, is_close ? "</center>" : "<center>");
      else if (strcmp(tag, "flushleft") == 0)
        buf_adds(&b, is_close ? "</div>" : "<div style=\"text-align:left\">");
      else if (strcmp(tag, "flushright") == 0)
        buf_adds(&b, is_close ? "</div>" : "<div style=\"text-align:right\">");
      else if (strcmp(tag, "flushboth") == 0)
        buf_adds(&b, is_close ? "</div>" : "<div style=\"text-align:justify\">");
      else if (strcmp(tag, "excerpt") == 0)
        buf_adds(&b, is_close ? "</blockquote>" : "<blockquote>");
      else if (strcmp(tag, "nofill") == 0) {
        buf_adds(&b, is_close ? "</pre>" : "<pre>");
        in_nofill = is_close ? false : true;
      }
      else if (strcmp(tag, "color") == 0) {
        if (is_close) buf_adds(&b, "</span>");
        else snprintf(pending_tag, sizeof(pending_tag), "color");
      }
      else if (strcmp(tag, "fontfamily") == 0) {
        if (is_close) buf_adds(&b, "</span>");
        else snprintf(pending_tag, sizeof(pending_tag), "fontfamily");
      }
      /* Unknown tags: ignore */
      continue;
    }

    if (in_param) {
      if (param_len < 255) param_buf[param_len++] = *p;
      p++;
      continue;
    }

    /* Regular text */
    if (*p == '&') buf_adds(&b, "&amp;");
    else if (*p == '>') buf_adds(&b, "&gt;");
    else if (*p == '\n') {
      if (in_nofill) buf_addc(&b, '\n');
      else {
        /* In enriched, single newline = space, double newline = <br> */
        if (p + 1 < end && p[1] == '\n') {
          buf_adds(&b, "<br>\n");
          p++; /* skip second newline */
        } else {
          buf_addc(&b, ' ');
        }
      }
    }
    else if (*p == '\r') { /* skip CR */ }
    else buf_addc(&b, *p);
    p++;
  }

  buf_adds(&b, "</body></html>");
  return buf_detach(&b);
}

/* ================================================================
 * Text/enriched → plain text
 * ================================================================ */

char *crispy_enriched_to_plain(const char *enriched, long len) {
  if (!enriched) return strdup("");
  if (len < 0) len = (long)strlen(enriched);
  const char *p = enriched, *end = enriched + len;
  Buf b; buf_init(&b);
  bool in_param = false;

  while (p < end) {
    if (*p == '<') {
      if (p + 1 < end && p[1] == '<') {
        if (!in_param) buf_addc(&b, '<');
        p += 2; continue;
      }
      char tag[64]; bool is_close;
      parse_enriched_tag(&p, end, tag, sizeof(tag), &is_close);
      if (strcmp(tag, "param") == 0) in_param = !is_close;
      else if (strcmp(tag, "excerpt") == 0 && !is_close) buf_adds(&b, "> ");
      continue;
    }
    if (in_param) { p++; continue; }
    if (*p == '\r') { p++; continue; }
    buf_addc(&b, *p);
    p++;
  }
  return buf_detach(&b);
}

/* ================================================================
 * HTML → text/enriched
 * ================================================================ */

char *crispy_html_to_enriched(const char *html, long len) {
  if (!html) return strdup("");
  if (len < 0) len = (long)strlen(html);
  const char *p = html, *end = html + len;
  Buf b; buf_init(&b);

  while (p < end) {
    if (*p == '<') {
      p++;
      bool is_close = false;
      if (p < end && *p == '/') { is_close = true; p++; }
      char tag[64]; size_t i = 0;
      while (p < end && *p != '>' && *p != ' ' && i < sizeof(tag) - 1)
        tag[i++] = tolower((unsigned char)*p++);
      tag[i] = '\0';
      /* Skip attributes and closing > */
      while (p < end && *p != '>') p++;
      if (p < end) p++;

      /* Map HTML to enriched */
      if (strcmp(tag, "b") == 0 || strcmp(tag, "strong") == 0)
        buf_adds(&b, is_close ? "</bold>" : "<bold>");
      else if (strcmp(tag, "i") == 0 || strcmp(tag, "em") == 0)
        buf_adds(&b, is_close ? "</italic>" : "<italic>");
      else if (strcmp(tag, "u") == 0)
        buf_adds(&b, is_close ? "</underline>" : "<underline>");
      else if (strcmp(tag, "tt") == 0 || strcmp(tag, "code") == 0)
        buf_adds(&b, is_close ? "</fixed>" : "<fixed>");
      else if (strcmp(tag, "blockquote") == 0)
        buf_adds(&b, is_close ? "</excerpt>" : "<excerpt>");
      else if (strcmp(tag, "pre") == 0)
        buf_adds(&b, is_close ? "</nofill>" : "<nofill>");
      else if (strcmp(tag, "center") == 0)
        buf_adds(&b, is_close ? "</center>" : "<center>");
      else if (strcmp(tag, "br") == 0)
        buf_adds(&b, "\n\n");
      else if (strcmp(tag, "p") == 0 && !is_close)
        buf_adds(&b, "\n\n");
      /* Skip other tags */
      continue;
    }

    /* HTML entities */
    if (*p == '&') {
      if (strncmp(p, "&amp;", 5) == 0) { buf_addc(&b, '&'); p += 5; }
      else if (strncmp(p, "&lt;", 4) == 0) { buf_adds(&b, "<<"); p += 4; }
      else if (strncmp(p, "&gt;", 4) == 0) { buf_addc(&b, '>'); p += 4; }
      else if (strncmp(p, "&quot;", 6) == 0) { buf_addc(&b, '"'); p += 6; }
      else if (strncmp(p, "&nbsp;", 6) == 0) { buf_addc(&b, ' '); p += 6; }
      else if (strncmp(p, "&#", 2) == 0) {
        p += 2;
        unsigned int cp = 0;
        if (*p == 'x' || *p == 'X') { p++; cp = (unsigned)strtoul(p, (char **)&p, 16); }
        else cp = (unsigned)strtoul(p, (char **)&p, 10);
        if (*p == ';') p++;
        /* Simple: ASCII chars only, skip others */
        if (cp < 128) buf_addc(&b, (char)cp);
      }
      else { buf_addc(&b, '&'); p++; }
      continue;
    }

    buf_addc(&b, *p);
    p++;
  }
  return buf_detach(&b);
}

/* ================================================================
 * Plain text → text/enriched
 * ================================================================ */

char *crispy_plain_to_enriched(const char *text, long len) {
  if (!text) return strdup("");
  if (len < 0) len = (long)strlen(text);
  Buf b; buf_init(&b);
  buf_adds(&b, "<nofill>");
  for (long i = 0; i < len; i++) {
    if (text[i] == '<') buf_adds(&b, "<<");
    else buf_addc(&b, text[i]);
  }
  buf_adds(&b, "</nofill>");
  return buf_detach(&b);
}

/* ================================================================
 * Format=flowed (RFC 3676) → plain text
 *
 * Rules:
 * - Lines ending with space (before CRLF) are "soft" — join with next
 * - Lines NOT ending with space are "hard" — keep line break
 * - Lines starting with ">" are quoted, nesting by count
 * - "-- " is a signature separator (always hard break)
 * - delsp=yes: delete the trailing space on soft lines
 * ================================================================ */

/* Count leading ">" and advance past them + optional space */
static int count_quote_depth(const char **line) {
  int depth = 0;
  const char *p = *line;
  while (*p == '>') { depth++; p++; }
  if (*p == ' ') p++; /* space-stuffing */
  *line = p;
  return depth;
}

char *crispy_flowed_to_plain(const char *flowed, long len, bool delsp) {
  if (!flowed) return strdup("");
  if (len < 0) len = (long)strlen(flowed);
  Buf b; buf_init(&b);
  const char *p = flowed, *end = flowed + len;
  int prev_depth = 0;

  while (p < end) {
    /* Get one line */
    const char *line_start = p;
    while (p < end && *p != '\n' && *p != '\r') p++;
    long line_len = p - line_start;
    /* Skip CRLF */
    if (p < end && *p == '\r') p++;
    if (p < end && *p == '\n') p++;

    /* Make a copy to work with */
    char *line = malloc(line_len + 1);
    memcpy(line, line_start, line_len); line[line_len] = '\0';
    /* Strip trailing CR */
    while (line_len > 0 && (line[line_len-1] == '\r')) line[--line_len] = '\0';

    /* Parse quote depth */
    const char *content = line;
    int depth = count_quote_depth(&content);

    /* Quote depth changed? Close/open quote levels */
    if (depth != prev_depth) {
      buf_addc(&b, '\n');
      for (int d = 0; d < depth; d++) buf_addc(&b, '>');
      if (depth > 0) buf_addc(&b, ' ');
      prev_depth = depth;
    }

    /* Check for signature separator */
    if (strcmp(content, "-- ") == 0) {
      buf_adds(&b, "-- \n");
      prev_depth = depth;
      free(line);
      continue;
    }

    /* Check if soft-wrapped (trailing space) */
    long clen = (long)strlen(content);
    bool soft = (clen > 0 && content[clen - 1] == ' ');

    if (soft && delsp) {
      /* Delete trailing space */
      buf_add(&b, content, clen - 1);
    } else if (soft) {
      /* Keep trailing space (it's the join point) */
      buf_add(&b, content, clen);
    } else {
      buf_adds(&b, content);
      buf_addc(&b, '\n');
    }

    free(line);
  }
  return buf_detach(&b);
}

/* ================================================================
 * Format=flowed → HTML
 * ================================================================ */

char *crispy_flowed_to_html(const char *flowed, long len, bool delsp) {
  if (!flowed) return strdup("");
  if (len < 0) len = (long)strlen(flowed);
  Buf b; buf_init(&b);
  buf_adds(&b, "<html><body>");
  const char *p = flowed, *end = flowed + len;
  int cur_depth = 0;

  while (p < end) {
    const char *line_start = p;
    while (p < end && *p != '\n' && *p != '\r') p++;
    long line_len = p - line_start;
    if (p < end && *p == '\r') p++;
    if (p < end && *p == '\n') p++;

    char *line = malloc(line_len + 1);
    memcpy(line, line_start, line_len); line[line_len] = '\0';
    while (line_len > 0 && line[line_len-1] == '\r') line[--line_len] = '\0';

    const char *content = line;
    int depth = count_quote_depth(&content);

    /* Adjust blockquote nesting */
    while (cur_depth < depth) { buf_adds(&b, "<blockquote>"); cur_depth++; }
    while (cur_depth > depth) { buf_adds(&b, "</blockquote>"); cur_depth--; }

    long clen = (long)strlen(content);
    bool soft = (clen > 0 && content[clen - 1] == ' ');

    /* Output content with HTML escaping */
    for (long i = 0; i < clen; i++) {
      if (soft && delsp && i == clen - 1) continue; /* skip trailing space */
      if (content[i] == '<') buf_adds(&b, "&lt;");
      else if (content[i] == '>') buf_adds(&b, "&gt;");
      else if (content[i] == '&') buf_adds(&b, "&amp;");
      else buf_addc(&b, content[i]);
    }

    if (!soft) buf_adds(&b, "<br>\n");

    free(line);
  }

  while (cur_depth > 0) { buf_adds(&b, "</blockquote>"); cur_depth--; }
  buf_adds(&b, "</body></html>");
  return buf_detach(&b);
}

/* ================================================================
 * Plain text → format=flowed
 * ================================================================ */

char *crispy_plain_to_flowed(const char *text, long len, int max_col) {
  if (!text) return strdup("");
  if (len < 0) len = (long)strlen(text);
  if (max_col <= 0) max_col = 72;
  Buf b; buf_init(&b);
  const char *p = text, *end = text + len;

  while (p < end) {
    const char *line_start = p;
    while (p < end && *p != '\n') p++;
    long line_len = p - line_start;
    if (p < end) p++; /* skip \n */

    /* Count quote prefix */
    const char *lp = line_start;
    int depth = 0;
    while (lp < line_start + line_len && *lp == '>') { depth++; lp++; }
    if (lp < line_start + line_len && *lp == ' ') lp++;

    long content_len = line_len - (lp - line_start);
    int prefix_len = depth + (depth > 0 ? 1 : 0); /* "> " */
    int wrap_at = max_col - prefix_len;
    if (wrap_at < 20) wrap_at = 20;

    if (content_len <= wrap_at) {
      /* Short line — output as-is (hard break) */
      for (int d = 0; d < depth; d++) buf_addc(&b, '>');
      if (depth > 0) buf_addc(&b, ' ');
      buf_add(&b, lp, content_len);
      buf_adds(&b, "\r\n");
    } else {
      /* Long line — wrap with soft breaks */
      const char *wp = lp;
      long remaining = content_len;
      while (remaining > 0) {
        int chunk = (remaining > wrap_at) ? wrap_at : (int)remaining;
        /* Find last space within chunk for word wrap */
        if (remaining > wrap_at) {
          int last_space = -1;
          for (int i = chunk - 1; i > 0; i--) {
            if (wp[i] == ' ') { last_space = i; break; }
          }
          if (last_space > 0) chunk = last_space + 1; /* include the space */
        }

        for (int d = 0; d < depth; d++) buf_addc(&b, '>');
        if (depth > 0) buf_addc(&b, ' ');
        buf_add(&b, wp, chunk);

        wp += chunk;
        remaining -= chunk;

        if (remaining > 0) {
          /* Soft break: line ends with space (already included from word wrap) */
          buf_adds(&b, "\r\n");
        } else {
          buf_adds(&b, "\r\n");
        }
      }
    }
  }
  return buf_detach(&b);
}

/* ================================================================
 * HTML → plain text
 * ================================================================ */

char *crispy_html_to_plain(const char *html, long len) {
  if (!html) return strdup("");
  if (len < 0) len = (long)strlen(html);
  const char *p = html, *end = html + len;
  Buf b; buf_init(&b);
  bool in_pre = false;

  while (p < end) {
    if (*p == '<') {
      p++;
      /* Get tag name */
      bool is_close = false;
      if (p < end && *p == '/') { is_close = true; p++; }
      char tag[32]; size_t i = 0;
      while (p < end && *p != '>' && *p != ' ' && i < sizeof(tag) - 1)
        tag[i++] = tolower((unsigned char)*p++);
      tag[i] = '\0';
      while (p < end && *p != '>') p++;
      if (p < end) p++;

      if (strcmp(tag, "br") == 0) buf_addc(&b, '\n');
      else if (strcmp(tag, "p") == 0 && !is_close) buf_adds(&b, "\n\n");
      else if (strcmp(tag, "div") == 0 && !is_close) buf_addc(&b, '\n');
      else if (strcmp(tag, "li") == 0 && !is_close) buf_adds(&b, "\n- ");
      else if (strcmp(tag, "hr") == 0) buf_adds(&b, "\n---\n");
      else if (strcmp(tag, "pre") == 0) in_pre = !is_close;
      else if (strcmp(tag, "h1") == 0 || strcmp(tag, "h2") == 0 ||
               strcmp(tag, "h3") == 0) {
        if (!is_close) buf_adds(&b, "\n\n");
        else buf_addc(&b, '\n');
      }
      continue;
    }

    if (*p == '&') {
      if (strncmp(p, "&amp;", 5) == 0) { buf_addc(&b, '&'); p += 5; }
      else if (strncmp(p, "&lt;", 4) == 0) { buf_addc(&b, '<'); p += 4; }
      else if (strncmp(p, "&gt;", 4) == 0) { buf_addc(&b, '>'); p += 4; }
      else if (strncmp(p, "&quot;", 6) == 0) { buf_addc(&b, '"'); p += 6; }
      else if (strncmp(p, "&apos;", 6) == 0) { buf_addc(&b, '\''); p += 6; }
      else if (strncmp(p, "&nbsp;", 6) == 0) { buf_addc(&b, ' '); p += 6; }
      else if (strncmp(p, "&#", 2) == 0) {
        p += 2;
        unsigned int cp = 0;
        if (*p == 'x' || *p == 'X') { p++; cp = (unsigned)strtoul(p, (char **)&p, 16); }
        else cp = (unsigned)strtoul(p, (char **)&p, 10);
        if (*p == ';') p++;
        if (cp < 128) buf_addc(&b, (char)cp);
        else if (cp < 0x800) {
          buf_addc(&b, (char)(0xC0 | (cp >> 6)));
          buf_addc(&b, (char)(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
          buf_addc(&b, (char)(0xE0 | (cp >> 12)));
          buf_addc(&b, (char)(0x80 | ((cp >> 6) & 0x3F)));
          buf_addc(&b, (char)(0x80 | (cp & 0x3F)));
        }
      }
      else { p++; } /* skip unknown entity */
      continue;
    }

    if (!in_pre && (*p == '\r' || *p == '\n')) {
      /* Collapse whitespace in non-pre */
      while (p < end && (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t')) p++;
      buf_addc(&b, ' ');
      continue;
    }

    buf_addc(&b, *p);
    p++;
  }
  return buf_detach(&b);
}

/* ================================================================
 * Plain text → HTML
 * ================================================================ */

char *crispy_plain_to_html(const char *text, long len) {
  if (!text) return strdup("");
  if (len < 0) len = (long)strlen(text);
  Buf b; buf_init(&b);
  buf_adds(&b, "<html><body><pre>");
  for (long i = 0; i < len; i++) {
    if (text[i] == '<') buf_adds(&b, "&lt;");
    else if (text[i] == '>') buf_adds(&b, "&gt;");
    else if (text[i] == '&') buf_adds(&b, "&amp;");
    else if (text[i] == '\r') continue;
    else buf_addc(&b, text[i]);
  }
  buf_adds(&b, "</pre></body></html>");
  return buf_detach(&b);
}
