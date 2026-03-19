/* crispy_uudecode.h — UUencode/AppleSingle decode
 * Part of crispy: standalone, no Eudora dependency.
 */

#ifndef CRISPY_UUDECODE_H
#define CRISPY_UUDECODE_H

#include <stddef.h>
#include <stdbool.h>

/* Detect if text contains UUencoded content ("begin NNN filename"). */
bool crispy_uudecode_detect(const char *data, long len);

/* Decode UUencoded data.
 * Input: raw text containing "begin NNN filename\n...data...\nend"
 * Output: malloc'd binary data, *outLen = length
 *         *outFilename = malloc'd filename (caller frees both)
 * Returns NULL if not valid UUencode. */
char *crispy_uudecode(const char *data, long len, long *outLen,
                      char **outFilename);

/* Encode binary data as UUencode.
 * filename: name to put in the "begin" line
 * data: raw binary data
 * dataLen: length of data
 * outLen: receives length of encoded output
 * Returns malloc'd UUencoded text (caller frees).
 * Format: "begin 644 filename\n<encoded lines>\n`\nend\n" */
char *crispy_uuencode(const char *filename, const char *data,
                      long dataLen, long *outLen);

#endif /* CRISPY_UUDECODE_H */
