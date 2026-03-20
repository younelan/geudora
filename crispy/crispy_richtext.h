/* crispy_richtext.h — Rich text format conversions
 * Part of crispy: standalone mail library.
 *
 * Conversions between text/enriched (RFC 1896), text/html,
 * text/plain, and format=flowed (RFC 3676).
 *
 * Portable: no UI dependency, pure text transforms.
 */

#ifndef CRISPY_RICHTEXT_H
#define CRISPY_RICHTEXT_H

#include <stddef.h>
#include <stdbool.h>

/* ================================================================
 * Text/enriched (RFC 1896) — parse and convert
 *
 * Tags: <bold>, <italic>, <underline>, <fixed>, <bigger>, <smaller>,
 *       <color><param>RRGGBB</param></color>, <fontfamily><param>name</param>,
 *       <center>, <flushleft>, <flushright>, <excerpt>, <nofill>, <param>
 * ================================================================ */

/* Convert text/enriched to HTML.
 * Maps enriched tags to HTML equivalents.
 * Returns malloc'd HTML string. Caller frees. */
char *crispy_enriched_to_html(const char *enriched, long len);

/* Convert text/enriched to plain text (strip all tags).
 * Returns malloc'd string. Caller frees. */
char *crispy_enriched_to_plain(const char *enriched, long len);

/* Convert HTML to text/enriched.
 * Maps common HTML tags to enriched equivalents.
 * Returns malloc'd string. Caller frees. */
char *crispy_html_to_enriched(const char *html, long len);

/* Convert plain text to text/enriched.
 * Escapes '<' as '<<', preserves line breaks with <nofill>.
 * Returns malloc'd string. Caller frees. */
char *crispy_plain_to_enriched(const char *text, long len);

/* ================================================================
 * Format=flowed (RFC 3676)
 *
 * Soft line breaks: lines ending with space are reflowable.
 * Quote levels: lines starting with ">" are quoted.
 * delsp=yes: delete trailing space on soft breaks.
 * ================================================================ */

/* Parse format=flowed text into reflowed plain text.
 * Joins soft-wrapped lines, preserves hard breaks and quote levels.
 * Returns malloc'd string. Caller frees. */
char *crispy_flowed_to_plain(const char *flowed, long len, bool delsp);

/* Parse format=flowed text into HTML with <blockquote> for quotes.
 * Returns malloc'd string. Caller frees. */
char *crispy_flowed_to_html(const char *flowed, long len, bool delsp);

/* Convert plain text to format=flowed.
 * Wraps lines at max_col (default 72), adds soft break spaces.
 * Preserves quote prefixes.
 * Returns malloc'd string. Caller frees. */
char *crispy_plain_to_flowed(const char *text, long len, int max_col);

/* ================================================================
 * HTML utilities
 * ================================================================ */

/* Convert HTML to plain text (strip tags, decode entities).
 * Returns malloc'd string. Caller frees. */
char *crispy_html_to_plain(const char *html, long len);

/* Convert plain text to minimal HTML (escape entities, <br> for newlines).
 * Returns malloc'd string. Caller frees. */
char *crispy_plain_to_html(const char *text, long len);

#endif /* CRISPY_RICHTEXT_H */
