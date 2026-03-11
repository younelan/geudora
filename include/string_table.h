/* string_table.h - Portable string resource table
 * Replaces Mac STR# resources with a compiled-in lookup table.
 * String values extracted from StringDefs.h comments.
 */
#ifndef STRING_TABLE_H
#define STRING_TABLE_H

#include <stdint.h>

/* Look up a string by its Mac STR# resource ID.
 * Returns the C string value, or NULL if not found. */
const char *string_table_lookup(uint16_t id);

/* GetIndString replacement: copies the string for the given ID into dest
 * as a Pascal string (length byte + data). dest must be >= 256 bytes. */
void GetIndString_impl(unsigned char *dest, short strListID, short index);

#endif /* STRING_TABLE_H */
