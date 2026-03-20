/* macmbx_concentrator.c — Message structure analysis and content concentrator
 * Part of macmbx: standalone mail data management library.
 * Ported from Eudora concentrator.c, rewritten standalone.
 */

#include "macmbx_concentrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/* ================================================================
 * Paragraph detection
 * ================================================================ */

int macmbx_concentrator_quote_depth(const char *line, int len) {
  int depth = 0;
  int i = 0;
  while (i < len && line[i] == '>') { depth++; i++; }
  return depth;
}

bool macmbx_concentrator_is_blank(const char *line, int len) {
  for (int i = 0; i < len; i++)
    if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n')
      return false;
  return true;
}

bool macmbx_concentrator_is_sig_separator(const char *line, int len) {
  /* "-- " or "-- \n" or "-- \r\n" */
  if (len < 3) return false;
  if (line[0] == '-' && line[1] == '-' && line[2] == ' ') {
    /* Rest should be whitespace/newline only */
    for (int i = 3; i < len; i++)
      if (line[i] != ' ' && line[i] != '\r' && line[i] != '\n') return false;
    return true;
  }
  return false;
}

bool macmbx_concentrator_is_attribution(const char *line, int len) {
  /* Pattern: starts with word char, ends with ":" before newline,
   * length 10-200, contains "wrote" or "said" or "writes" or "@" */
  if (len < 10 || len > 200) return false;

  /* Must start with word char */
  if (!isalnum((unsigned char)line[0])) return false;

  /* Must end with colon (before trailing whitespace/newline) */
  int end = len - 1;
  while (end > 0 && (line[end] == '\r' || line[end] == '\n' || line[end] == ' '))
    end--;
  if (end < 5 || line[end] != ':') return false;

  /* Should contain common attribution words */
  /* Case-insensitive search for key phrases */
  for (int i = 0; i < len - 4; i++) {
    if (strncasecmp(line + i, "wrote", 5) == 0) return true;
    if (strncasecmp(line + i, "said", 4) == 0) return true;
    if (strncasecmp(line + i, "writes", 6) == 0) return true;
    if (strncasecmp(line + i, "schrieb", 7) == 0) return true;  /* German */
    if (strncasecmp(line + i, "a ecrit", 7) == 0) return true;  /* French */
    if (strncasecmp(line + i, "escribi", 7) == 0) return true;  /* Spanish */
  }

  /* Also match "On <date>, <name> <addr>:" pattern */
  if (len > 3 && strncasecmp(line, "On ", 3) == 0 && line[end] == ':')
    return true;

  return false;
}

bool macmbx_concentrator_is_forward_begin(const char *line, int len) {
  if (len < 10 || len > 60) return false;
  /* Must start with dashes */
  if (line[0] != '-' || line[1] != '-' || line[2] != '-') return false;

  /* Common patterns */
  static const char *patterns[] = {
    "begin forwarded",
    "original message",
    "forwarded message",
    "forwarded by",
    "start of forwarded",
    NULL
  };
  for (int p = 0; patterns[p]; p++) {
    for (int i = 3; i < len - (int)strlen(patterns[p]); i++) {
      if (strncasecmp(line + i, patterns[p], strlen(patterns[p])) == 0)
        return true;
    }
  }
  return false;
}

bool macmbx_concentrator_is_forward_end(const char *line, int len) {
  if (len < 10 || len > 60) return false;
  if (line[0] != '-' || line[1] != '-' || line[2] != '-') return false;

  static const char *patterns[] = {
    "end forwarded",
    "end of forwarded",
    "end original",
    NULL
  };
  for (int p = 0; patterns[p]; p++) {
    for (int i = 3; i < len - (int)strlen(patterns[p]); i++) {
      if (strncasecmp(line + i, patterns[p], strlen(patterns[p])) == 0)
        return true;
    }
  }
  return false;
}

bool macmbx_concentrator_is_digest_separator(const char *line, int len) {
  if (len < 4) return false;
  /* At least 4 dashes */
  int dashes = 0;
  for (int i = 0; i < len && line[i] == '-'; i++) dashes++;
  if (dashes < 4) return false;
  /* Rest should be dashes or whitespace */
  for (int i = dashes; i < len; i++)
    if (line[i] != '-' && line[i] != ' ' && line[i] != '\r' && line[i] != '\n')
      return false;
  return true;
}

bool macmbx_concentrator_is_attachment(const char *line, int len) {
  if (len < 15) return false;
  /* Eudora: "Attachment converted: ..." */
  if (strncasecmp(line, "Attachment converted:", 21) == 0) return true;
  if (strncasecmp(line, "Attachment ", 11) == 0) return true;
  /* Also: "[image: ...]" inline image markers */
  if (line[0] == '[' && strncasecmp(line + 1, "image:", 6) == 0) return true;
  /* MIME attachment markers */
  if (strncasecmp(line, "<<", 2) == 0 && len < 80) {
    /* <<filename.pdf>> */
    for (int i = 2; i < len; i++)
      if (line[i] == '>' && i + 1 < len && line[i+1] == '>') return true;
  }
  return false;
}

/* ================================================================
 * Analyze message structure
 * ================================================================ */

static void ensure_para_capacity(MacmbxMsgStructure *ms, int needed) {
  if (needed <= ms->capacity) return;
  int newCap = ms->capacity ? ms->capacity * 2 : 64;
  while (newCap < needed) newCap *= 2;
  ms->paras = (MacmbxParagraph *)realloc(ms->paras, newCap * sizeof(MacmbxParagraph));
  ms->capacity = newCap;
}

MacmbxMsgStructure *macmbx_concentrator_analyze(const char *text, long len) {
  if (!text) return NULL;
  if (len < 0) len = (long)strlen(text);

  MacmbxMsgStructure *ms = (MacmbxMsgStructure *)calloc(1, sizeof(MacmbxMsgStructure));
  if (!ms) return NULL;
  ms->capacity = 64;
  ms->paras = (MacmbxParagraph *)calloc(ms->capacity, sizeof(MacmbxParagraph));

  const char *p = text;
  const char *end = text + len;
  bool in_signature = false;
  bool in_forward = false;

  while (p < end) {
    /* Find end of line */
    const char *line_start = p;
    while (p < end && *p != '\n') p++;
    int line_len = (int)(p - line_start);
    if (p < end) p++; /* skip \n */

    /* Strip trailing CR */
    int content_len = line_len;
    while (content_len > 0 && line_start[content_len - 1] == '\r') content_len--;

    ensure_para_capacity(ms, ms->count + 1);
    MacmbxParagraph *para = &ms->paras[ms->count];
    memset(para, 0, sizeof(*para));
    para->offset = line_start - text;
    para->length = (p - line_start);
    para->text = line_start;
    para->text_len = content_len;

    /* Classify */
    if (macmbx_concentrator_is_blank(line_start, content_len)) {
      para->type = MACMBX_PARA_BLANK;
    } else if (in_signature) {
      para->type = MACMBX_PARA_SIGNATURE;
    } else if (macmbx_concentrator_is_sig_separator(line_start, content_len)) {
      para->type = MACMBX_PARA_SIG_SEPARATOR;
      in_signature = true;
      ms->has_signature = true;
    } else if (macmbx_concentrator_is_forward_begin(line_start, content_len)) {
      para->type = MACMBX_PARA_FORWARD_BEGIN;
      in_forward = true;
      ms->has_forwarded = true;
    } else if (macmbx_concentrator_is_forward_end(line_start, content_len)) {
      para->type = MACMBX_PARA_FORWARD_END;
      in_forward = false;
    } else if (macmbx_concentrator_is_attachment(line_start, content_len)) {
      para->type = MACMBX_PARA_ATTACHMENT;
      ms->has_attachments = true;
    } else if (macmbx_concentrator_is_digest_separator(line_start, content_len)) {
      para->type = MACMBX_PARA_DIGEST_SEP;
    } else if (in_forward && content_len > 0 && strchr(line_start, ':') &&
               (content_len - (strchr(line_start, ':') - line_start)) > 1 &&
               !isspace((unsigned char)line_start[0]) &&
               ms->count > 0 && ms->paras[ms->count - 1].type == MACMBX_PARA_FORWARD_BEGIN) {
      /* Header line right after forward-begin marker */
      para->type = MACMBX_PARA_HEADER;
    } else {
      /* Check quoting */
      int depth = macmbx_concentrator_quote_depth(line_start, content_len);
      if (depth > 0) {
        para->type = MACMBX_PARA_QUOTED;
        para->quote_depth = depth;
        if (depth > ms->max_quote_depth) ms->max_quote_depth = depth;
        ms->quoted_paras++;
      } else if (macmbx_concentrator_is_attribution(line_start, content_len)) {
        para->type = MACMBX_PARA_ATTRIBUTION;
      } else {
        para->type = MACMBX_PARA_BODY;
        ms->body_paras++;
      }
    }

    ms->count++;
  }

  return ms;
}

void macmbx_concentrator_free(MacmbxMsgStructure *ms) {
  if (!ms) return;
  free(ms->paras);
  free(ms);
}

/* ================================================================
 * Profiles
 * ================================================================ */

void macmbx_concentrator_profile_default(MacmbxConcentratorProfile *profile) {
  if (!profile) return;
  memset(profile, 0, sizeof(*profile));
  profile->show_body = true;
  profile->show_quoted = true;
  profile->show_attribution = true;
  profile->show_signature = true;
  profile->show_forwarded = true;
  profile->show_attachments = true;
  profile->ellipsis = true;
}

void macmbx_concentrator_profile_preview(MacmbxConcentratorProfile *profile, int max_chars) {
  if (!profile) return;
  memset(profile, 0, sizeof(*profile));
  profile->show_body = true;
  profile->show_quoted = false;
  profile->show_attribution = false;
  profile->show_signature = false;
  profile->show_forwarded = false;
  profile->show_attachments = false;
  profile->max_body_chars = max_chars > 0 ? max_chars : 200;
  profile->max_body_paras = 3;
  profile->ellipsis = true;
}

void macmbx_concentrator_profile_notify(MacmbxConcentratorProfile *profile, int max_chars) {
  if (!profile) return;
  memset(profile, 0, sizeof(*profile));
  profile->show_body = true;
  profile->max_body_chars = max_chars > 0 ? max_chars : 100;
  profile->max_body_paras = 1;
  profile->ellipsis = true;
}

/* ================================================================
 * Concentrate: apply profile to produce summary
 * ================================================================ */

char *macmbx_concentrator_concentrate(MacmbxMsgStructure *ms,
                                        MacmbxConcentratorProfile *profile) {
  if (!ms || !profile) return strdup("");

  size_t cap = 2048;
  char *out = (char *)malloc(cap);
  if (!out) return strdup("");
  size_t o = 0;
  int body_chars = 0, body_paras = 0;
  int quoted_chars = 0;
  bool truncated = false;

  for (int i = 0; i < ms->count && !truncated; i++) {
    MacmbxParagraph *para = &ms->paras[i];
    bool include = false;

    switch (para->type) {
    case MACMBX_PARA_BODY:
      if (!profile->show_body) break;
      if (profile->max_body_paras > 0 && body_paras >= profile->max_body_paras) {
        truncated = true; break;
      }
      if (profile->max_body_chars > 0 && body_chars >= profile->max_body_chars) {
        truncated = true; break;
      }
      include = true;
      body_paras++;
      break;

    case MACMBX_PARA_QUOTED:
      if (!profile->show_quoted) break;
      if (profile->max_quote_depth > 0 && para->quote_depth > profile->max_quote_depth) break;
      if (profile->max_quoted_chars > 0 && quoted_chars >= profile->max_quoted_chars) break;
      include = true;
      break;

    case MACMBX_PARA_ATTRIBUTION:
      include = profile->show_attribution;
      break;

    case MACMBX_PARA_FORWARD_BEGIN:
    case MACMBX_PARA_FORWARD_END:
      include = profile->show_forwarded;
      break;

    case MACMBX_PARA_SIG_SEPARATOR:
    case MACMBX_PARA_SIGNATURE:
      include = profile->show_signature;
      break;

    case MACMBX_PARA_ATTACHMENT:
      include = profile->show_attachments;
      break;

    case MACMBX_PARA_BLANK:
      /* Include blanks only between included content */
      if (o > 0 && out[o-1] != '\n') include = true;
      break;

    case MACMBX_PARA_DIGEST_SEP:
      include = profile->show_forwarded;
      break;

    case MACMBX_PARA_HEADER:
      include = profile->show_forwarded;
      break;
    }

    if (include) {
      int add_len = para->text_len;

      /* Check truncation limits */
      if (para->type == MACMBX_PARA_BODY && profile->max_body_chars > 0) {
        int remaining = profile->max_body_chars - body_chars;
        if (add_len > remaining) {
          add_len = remaining;
          /* Truncate at word boundary */
          while (add_len > 0 && para->text[add_len - 1] != ' ') add_len--;
          if (add_len == 0) add_len = remaining;
          truncated = true;
        }
        body_chars += add_len;
      }
      if (para->type == MACMBX_PARA_QUOTED && profile->max_quoted_chars > 0) {
        int remaining = profile->max_quoted_chars - quoted_chars;
        if (add_len > remaining) add_len = remaining;
        quoted_chars += add_len;
      }

      /* Grow buffer */
      if (o + add_len + 4 > cap) {
        while (o + add_len + 4 > cap) cap *= 2;
        out = (char *)realloc(out, cap);
      }
      memcpy(out + o, para->text, add_len);
      o += add_len;
      out[o++] = '\n';
    }
  }

  /* Add ellipsis if truncated */
  if (truncated && profile->ellipsis) {
    if (o + 4 > cap) { cap += 16; out = realloc(out, cap); }
    memcpy(out + o, "...", 3);
    o += 3;
  }

  out[o] = '\0';

  /* Trim trailing blank lines */
  while (o > 1 && out[o-1] == '\n' && out[o-2] == '\n') o--;
  out[o] = '\0';

  return out;
}

/* ================================================================
 * Shortcut: analyze + concentrate
 * ================================================================ */

char *macmbx_concentrator_summarize(const char *text, long len,
                                      MacmbxConcentratorProfile *profile) {
  MacmbxMsgStructure *ms = macmbx_concentrator_analyze(text, len);
  if (!ms) return strdup("");
  char *result = macmbx_concentrator_concentrate(ms, profile);
  macmbx_concentrator_free(ms);
  return result;
}
