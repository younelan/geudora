/* macmbx_concentrator.h — Message structure analysis and content concentrator
 * Part of macmbx: standalone mail data management library.
 *
 * Analyzes message text paragraph-by-paragraph to identify:
 *   - Body text, quoted text (with nesting depth)
 *   - Attributions ("On ... wrote:")
 *   - Forwarded sections ("--- Begin forwarded message ---")
 *   - Signature blocks ("-- ")
 *   - Attachments
 *   - Digest separators
 *
 * Also: message summarization with configurable rules (show/hide/truncate
 * each element type).
 *
 * Ported from Eudora's Content Concentrator. No UI dependency.
 */

#ifndef MACMBX_CONCENTRATOR_H
#define MACMBX_CONCENTRATOR_H

#include <stddef.h>
#include <stdbool.h>

/* ================================================================
 * Paragraph types
 * ================================================================ */

typedef enum {
  MACMBX_PARA_BODY,          /* regular body text */
  MACMBX_PARA_QUOTED,        /* quoted text (>-prefixed) */
  MACMBX_PARA_ATTRIBUTION,   /* "On Mon, John wrote:" */
  MACMBX_PARA_FORWARD_BEGIN, /* "--- Begin forwarded message ---" */
  MACMBX_PARA_FORWARD_END,   /* "--- End forwarded message ---" */
  MACMBX_PARA_SIGNATURE,     /* text after "-- " separator */
  MACMBX_PARA_SIG_SEPARATOR, /* the "-- " line itself */
  MACMBX_PARA_ATTACHMENT,    /* "Attachment converted: ..." */
  MACMBX_PARA_DIGEST_SEP,    /* "----" digest separator */
  MACMBX_PARA_BLANK,         /* empty line */
  MACMBX_PARA_HEADER,        /* header line (in forwarded section) */
} MacmbxParaType;

/* A single analyzed paragraph */
typedef struct {
  MacmbxParaType type;
  int quote_depth;            /* 0 = not quoted, 1 = "> ", 2 = ">> ", etc. */
  long offset;                /* byte offset in original text */
  long length;                /* bytes including newline */
  const char *text;           /* pointer into original text (not malloc'd) */
  int text_len;               /* length of text content (without trailing newline) */
} MacmbxParagraph;

/* Analyzed message structure */
typedef struct {
  MacmbxParagraph *paras;
  int count;
  int capacity;
  /* Summary stats */
  int body_paras;
  int quoted_paras;
  int max_quote_depth;
  bool has_signature;
  bool has_forwarded;
  bool has_attachments;
} MacmbxMsgStructure;

/* ================================================================
 * Analysis
 * ================================================================ */

/* Analyze message body text into structured paragraphs.
 * text: plain text message body (after headers).
 * Returns structure. Caller frees with macmbx_concentrator_free(). */
MacmbxMsgStructure *macmbx_concentrator_analyze(const char *text, long len);

/* Free analyzed structure. */
void macmbx_concentrator_free(MacmbxMsgStructure *ms);

/* ================================================================
 * Summarization / Concentration
 * ================================================================ */

/* Summary profile — what to include in the concentrated output */
typedef struct {
  bool show_body;             /* show body text */
  bool show_quoted;           /* show quoted text */
  int max_quote_depth;        /* hide quotes deeper than this (0 = show all) */
  bool show_attribution;      /* show "X wrote:" lines */
  bool show_signature;        /* show signature block */
  bool show_forwarded;        /* show forwarded sections */
  bool show_attachments;      /* show attachment references */
  int max_body_chars;         /* truncate body at N chars (0 = no limit) */
  int max_body_paras;         /* max body paragraphs (0 = no limit) */
  int max_quoted_chars;       /* truncate quoted at N chars (0 = no limit) */
  bool ellipsis;              /* add "..." when truncating */
} MacmbxConcentratorProfile;

/* Initialize profile with sensible defaults (show everything). */
void macmbx_concentrator_profile_default(MacmbxConcentratorProfile *profile);

/* Initialize profile for preview (short summary: body only, truncated). */
void macmbx_concentrator_profile_preview(MacmbxConcentratorProfile *profile, int max_chars);

/* Initialize profile for notification (first paragraph only). */
void macmbx_concentrator_profile_notify(MacmbxConcentratorProfile *profile, int max_chars);

/* Concentrate: apply profile to analyzed structure, produce summary text.
 * Returns malloc'd string. Caller frees. */
char *macmbx_concentrator_concentrate(MacmbxMsgStructure *ms, MacmbxConcentratorProfile *profile);

/* Shortcut: analyze + concentrate in one call.
 * Returns malloc'd summary text. Caller frees. */
char *macmbx_concentrator_summarize(const char *text, long len,
                                MacmbxConcentratorProfile *profile);

/* ================================================================
 * Paragraph detection utilities (usable standalone)
 * ================================================================ */

/* Count leading ">" quote characters. Returns depth. */
int macmbx_concentrator_quote_depth(const char *line, int len);

/* Check if line is an attribution ("On Mon, John wrote:"). */
bool macmbx_concentrator_is_attribution(const char *line, int len);

/* Check if line is a forward begin marker. */
bool macmbx_concentrator_is_forward_begin(const char *line, int len);

/* Check if line is a forward end marker. */
bool macmbx_concentrator_is_forward_end(const char *line, int len);

/* Check if line is a signature separator "-- ". */
bool macmbx_concentrator_is_sig_separator(const char *line, int len);

/* Check if line is a digest separator "----...". */
bool macmbx_concentrator_is_digest_separator(const char *line, int len);

/* Check if line is an attachment reference. */
bool macmbx_concentrator_is_attachment(const char *line, int len);

/* Check if line is all whitespace. */
bool macmbx_concentrator_is_blank(const char *line, int len);

#endif /* MACMBX_CONCENTRATOR_H */
