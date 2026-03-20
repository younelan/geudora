/* macmbx_vcard.h — vCard 2.1/3.0/4.0 parser and builder
 * Part of macmbx: standalone mail data management library.
 *
 * Full RFC 2426 (vCard 3.0) + RFC 2425 (MIME directory) support:
 *   - Structured properties: N, ADR, ORG
 *   - Parameters: TYPE, VALUE, ENCODING, CHARSET, PREF
 *   - Encodings: 7bit, 8bit, quoted-printable, base64
 *   - Line unfolding (continuation lines)
 *   - Multiple values per property
 *   - X-DASH custom properties
 *   - PHOTO with base64 or URI
 *   - vCard 2.1 compatibility
 *
 * No Eudora dependency. Portable C99.
 */

#ifndef MACMBX_VCARD_H
#define MACMBX_VCARD_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Maximum values per property (ADR has 7 parts, N has 5) */
#define MACMBX_VCARD_MAX_VALUES 8
#define MACMBX_VCARD_MAX_PARAMS 8

/* ================================================================
 * Types
 * ================================================================ */

/* Parameter on a property (e.g. TYPE=WORK, ENCODING=b) */
typedef struct {
  char name[32];              /* parameter name: TYPE, VALUE, ENCODING, etc. */
  char value[128];            /* parameter value: WORK, HOME, b, etc. */
} MacmbxVcardParam;

/* A single vCard property (one line like TEL;TYPE=WORK:+1234) */
typedef struct MacmbxVcardProp {
  char name[64];              /* property name: FN, N, TEL, ADR, etc. */
  char group[32];             /* group prefix: item1, item2, etc. (empty if none) */
  MacmbxVcardParam params[MACMBX_VCARD_MAX_PARAMS];
  int param_count;
  /* Value(s) — semicolon-separated for structured (N, ADR, ORG) */
  char *values[MACMBX_VCARD_MAX_VALUES];  /* malloc'd strings */
  int value_count;
  /* Raw value for non-structured properties */
  char *raw_value;            /* malloc'd, full value after decoding */
  /* Binary data (PHOTO, LOGO, KEY) */
  unsigned char *binary_data; /* malloc'd decoded bytes */
  long binary_len;
  struct MacmbxVcardProp *next;
} MacmbxVcardProp;

/* A complete vCard */
typedef struct {
  char version[8];            /* "2.1", "3.0", "4.0" */
  MacmbxVcardProp *props;     /* linked list of properties */
} MacmbxVcard;

/* ================================================================
 * Parsing
 * ================================================================ */

/* Parse a single vCard from text. Returns NULL on error.
 * Handles line unfolding, QP/base64 decoding, structured values.
 * Caller must free with macmbx_vcard_free(). */
MacmbxVcard *macmbx_vcard_parse(const char *text, long len);

/* Parse a file containing one or more vCards.
 * Returns count, allocates *cards array. Caller frees each + array. */
int macmbx_vcard_parse_file(const char *path, MacmbxVcard ***cards);

/* ================================================================
 * Building
 * ================================================================ */

/* Create a new empty vCard (version 3.0). */
MacmbxVcard *macmbx_vcard_new(void);

/* Add a simple property: name=value. */
MacmbxVcardProp *macmbx_vcard_add(MacmbxVcard *vc, const char *name,
                                    const char *value);

/* Add a property with TYPE parameter. */
MacmbxVcardProp *macmbx_vcard_add_typed(MacmbxVcard *vc, const char *name,
                                          const char *value, const char *type);

/* Add a structured property (N, ADR, ORG) with semicolon-separated parts. */
MacmbxVcardProp *macmbx_vcard_add_structured(MacmbxVcard *vc, const char *name,
                                               const char **parts, int part_count);

/* Add a binary property (PHOTO, LOGO, KEY) with base64 encoding. */
MacmbxVcardProp *macmbx_vcard_add_binary(MacmbxVcard *vc, const char *name,
                                           const char *media_type,
                                           const unsigned char *data, long len);

/* Add a parameter to an existing property. */
int macmbx_vcard_prop_add_param(MacmbxVcardProp *prop,
                                  const char *name, const char *value);

/* Serialize a vCard to text (RFC 2426 format).
 * Returns malloc'd string. Caller frees. */
char *macmbx_vcard_serialize(MacmbxVcard *vc);

/* Write a vCard to a file. */
int macmbx_vcard_write_file(MacmbxVcard *vc, const char *path);

/* Write multiple vCards to a file. */
int macmbx_vcard_write_file_multi(MacmbxVcard **cards, int count, const char *path);

/* ================================================================
 * Property access
 * ================================================================ */

/* Find first property by name. Returns NULL if not found. */
MacmbxVcardProp *macmbx_vcard_find(MacmbxVcard *vc, const char *name);

/* Find all properties by name. Returns count, fills props array (up to max). */
int macmbx_vcard_find_all(MacmbxVcard *vc, const char *name,
                            MacmbxVcardProp **props, int max);

/* Get the value of a simple property. Returns NULL if not found. */
const char *macmbx_vcard_get(MacmbxVcard *vc, const char *name);

/* Get a specific part of a structured property (N, ADR).
 * part: 0-based index. Returns NULL if not found. */
const char *macmbx_vcard_get_part(MacmbxVcard *vc, const char *name, int part);

/* Check if a property has a TYPE parameter matching a value. */
bool macmbx_vcard_has_type(MacmbxVcardProp *prop, const char *type);

/* Get the TYPE parameter value(s) as a string. Returns NULL if none. */
const char *macmbx_vcard_get_type(MacmbxVcardProp *prop);

/* ================================================================
 * Convenience: common fields
 * ================================================================ */

/* Get formatted name (FN). */
const char *macmbx_vcard_fn(MacmbxVcard *vc);

/* Get name parts: N property = family;given;middle;prefix;suffix */
const char *macmbx_vcard_family(MacmbxVcard *vc);
const char *macmbx_vcard_given(MacmbxVcard *vc);

/* Get first email address. */
const char *macmbx_vcard_email(MacmbxVcard *vc);

/* Get all email addresses. Returns count, fills array (up to max). */
int macmbx_vcard_emails(MacmbxVcard *vc, const char **addrs, int max);

/* Get first phone number. type: "WORK", "HOME", "CELL", NULL for any. */
const char *macmbx_vcard_phone(MacmbxVcard *vc, const char *type);

/* Get organization name. */
const char *macmbx_vcard_org(MacmbxVcard *vc);

/* Get photo binary data. Returns NULL if no PHOTO. */
const unsigned char *macmbx_vcard_photo(MacmbxVcard *vc, long *outLen,
                                          const char **media_type);

/* ================================================================
 * Memory management
 * ================================================================ */

void macmbx_vcard_free(MacmbxVcard *vc);
void macmbx_vcard_free_prop(MacmbxVcardProp *prop);

#endif /* MACMBX_VCARD_H */
