/* crispy_rfc822.h — RFC822 message utilities
 * Part of crispy: standalone mail library.
 *
 * Address parsing/formatting, header building, date parsing,
 * BODYSTRUCTURE navigation, subject stripping, sequence parsing.
 *
 * Portable: POSIX + Windows. No Eudora dependency.
 */

#ifndef CRISPY_RFC822_H
#define CRISPY_RFC822_H

#include <stddef.h>
#include <stdbool.h>
#include "crispy_imap.h"  /* for ImapAddress, ImapEnvelope, ImapBodyPart, etc. */

/* ================================================================
 * Parsed date
 * ================================================================ */

typedef struct {
  int year;          /* 4-digit year */
  int month;         /* 1-12 */
  int day;           /* 1-31 */
  int hours;         /* 0-23 */
  int minutes;       /* 0-59 */
  int seconds;       /* 0-59 */
  int tz_offset_min; /* timezone offset in minutes, e.g. +120 = UTC+2 */
} CrispyDate;

/* ================================================================
 * RFC822 address parsing — parse text address strings
 * ================================================================ */

/* Parse RFC822 address list: "Name <addr>, Name <addr>, ..."
 * Returns linked list of ImapAddress. Caller frees with crispy_imap_free_addresses(). */
ImapAddress *crispy_rfc822_parse_adrlist(const char *text, const char *default_host);

/* Parse a single mailbox: "Name <user@host>" or "user@host" or "<user@host>"
 * Returns single ImapAddress or NULL. Caller frees. */
ImapAddress *crispy_rfc822_parse_mailbox(const char *text, const char *default_host);

/* ================================================================
 * RFC822 address formatting — ImapAddress to display strings
 * ================================================================ */

/* Format address list to string: "Name <user@host>, Name2 <user2@host2>"
 * Returns malloc'd string. Caller frees. */
char *crispy_rfc822_write_address(ImapAddress *adr);

/* Format single address to "user@host" (no name). buf must be >= 512. */
void crispy_rfc822_address(char *buf, size_t bufsize, ImapAddress *adr);

/* Format single address with name: "Name <user@host>". Returns malloc'd. */
char *crispy_rfc822_format_address(ImapAddress *adr);

/* Copy an address list (deep copy). Caller frees with crispy_imap_free_addresses(). */
ImapAddress *crispy_rfc822_copy_adrlist(ImapAddress *adr);

/* ================================================================
 * RFC822 quoting and comment handling
 * ================================================================ */

/* Quote a string for RFC822 if it contains specials. Returns malloc'd. */
char *crispy_rfc822_quote(const char *src);

/* Unquote an RFC822 quoted string in place. Returns same pointer. */
char *crispy_rfc822_unquote(char *src);

/* Skip whitespace and RFC822 comments. Advances *s past them. */
void crispy_rfc822_skipws(const char **s);

/* Skip an RFC822 comment starting at '('. Returns pointer past ')' or NULL. */
const char *crispy_rfc822_skip_comment(const char *s);

/* ================================================================
 * RFC822 header building — construct headers from envelope/body
 * ================================================================ */

/* Build complete RFC822 headers from envelope and body structure.
 * Returns malloc'd string with headers (including trailing blank line).
 * body may be NULL (no MIME headers). Caller frees. */
char *crispy_rfc822_build_header(ImapEnvelope *env, ImapBodyPart *body);

/* Write Content-Type/Encoding/Disposition headers for a body part.
 * Returns malloc'd string. Caller frees. */
char *crispy_rfc822_body_header(ImapBodyPart *body);

/* ================================================================
 * RFC822 message output — serialize envelope+body to full message
 * ================================================================ */

/* Build complete RFC822 message (headers + body text).
 * body_text is the raw body content (already encoded).
 * Returns malloc'd string. Caller frees. */
char *crispy_rfc822_build_message(ImapEnvelope *env, ImapBodyPart *body,
                                   const char *body_text, long body_len);

/* ================================================================
 * Date parsing and formatting
 * ================================================================ */

/* Parse IMAP/RFC822 date string into components.
 * Accepts formats: "dd-Mon-yyyy hh:mm:ss +zzzz",
 * "Mon, dd Mon yyyy hh:mm:ss +zzzz", "mm/dd/yyyy", etc.
 * Returns true on success. */
bool crispy_rfc822_parse_date(const char *str, CrispyDate *d);

/* Format date to IMAP INTERNALDATE format: "dd-Mon-yyyy hh:mm:ss +zzzz".
 * buf must be >= 32 bytes. Returns buf. */
char *crispy_rfc822_format_imap_date(char *buf, size_t bufsize, CrispyDate *d);

/* Format date to RFC822 format: "Mon, dd Mon yyyy hh:mm:ss +zzzz".
 * buf must be >= 48 bytes. Returns buf. */
char *crispy_rfc822_format_date(char *buf, size_t bufsize, CrispyDate *d);

/* Format date to C ctime-style: "Mon Mon dd hh:mm:ss yyyy\n".
 * buf must be >= 32 bytes. Returns buf. */
char *crispy_rfc822_format_cdate(char *buf, size_t bufsize, CrispyDate *d);

/* ================================================================
 * BODYSTRUCTURE navigation
 * ================================================================ */

/* Navigate body tree by IMAP section string (e.g. "1.2.3").
 * Returns pointer to matching part, or NULL if not found.
 * Does NOT allocate — returns pointer into existing tree. */
ImapBodyPart *crispy_rfc822_sub_body(ImapBodyPart *body, const char *section);

/* ================================================================
 * Subject utilities for threading
 * ================================================================ */

/* Strip leading "Re:", "RE:", "re:" prefixes (repeating).
 * Returns pointer into original string (no allocation). */
const char *crispy_rfc822_skip_re(const char *subject);

/* Strip trailing "(fwd)", "(Fwd)", "(FWD)" suffixes (repeating).
 * Returns pointer into original string. */
const char *crispy_rfc822_skip_fwd(const char *subject);

/* Strip both Re: and (fwd). Returns pointer into original string. */
const char *crispy_rfc822_base_subject(const char *subject);

/* ================================================================
 * IMAP sequence set parsing
 * ================================================================ */

/* Parse IMAP sequence set "1,3:5,7,10:*" into array of UIDs.
 * max_uid is used for '*'. Returns count, allocates *uids. Caller frees. */
int crispy_rfc822_parse_sequence(const char *seqset, unsigned long max_uid,
                                  unsigned long **uids);

/* ================================================================
 * Client-side search — offline text/address matching
 * ================================================================ */

/* Case-insensitive substring search in text.
 * Returns true if pattern is found anywhere in text. */
bool crispy_rfc822_search_text(const char *text, long textLen,
                                const char *pattern);

/* Search within an address list for a pattern.
 * Matches against name, mailbox, and host fields.
 * Returns true if pattern found in any address. */
bool crispy_rfc822_search_addr(ImapAddress *adr, const char *pattern);

/* Search within a header value. Handles RFC2047-encoded words
 * by searching the raw text (caller should decode first if needed).
 * Returns true if pattern found. */
bool crispy_rfc822_search_header(const char *header_value, const char *pattern);

/* Full message search — searches headers and body text.
 * Returns true if pattern found anywhere. */
bool crispy_rfc822_search_message(const char *message, long msgLen,
                                   const char *pattern);

/* Compare two subjects for threading (strips Re:/Fwd:, case-insensitive).
 * Returns true if base subjects match. */
bool crispy_rfc822_subjects_match(const char *subj1, const char *subj2);

/* Compare messages for client-side sorting.
 * field: "DATE", "FROM", "SUBJECT", "SIZE".
 * Returns <0, 0, >0 like strcmp. */
int crispy_rfc822_sort_compare(const char *field,
                                ImapFetchResult *a, ImapFetchResult *b);

/* ================================================================
 * Address and date display formatting
 * ================================================================ */

/* Beautify a From address in place: "Name <addr>" → "Name",
 * strip quotes, trim whitespace. For display in message lists. */
void crispy_rfc822_beautify_from(char *fromStr);

/* Beautify a date string in place for display.
 * Reformats to a clean short form. */
void crispy_rfc822_beautify_date(char *dateStr);

/* Format seconds-since-epoch as RFC822 date into buf.
 * Returns buf. */
char *crispy_rfc822_date_from_secs(char *buf, size_t bufsize, unsigned long secs);

/* Check if a line is an mbox "From " separator line. */
bool crispy_rfc822_is_from_line(const char *line);

/* Write an mbox "From " separator line into buf. */
void crispy_rfc822_write_from_line(char *buf, size_t bufsize, const char *sender);

#endif /* CRISPY_RFC822_H */
