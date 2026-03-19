/* crispy_binhex.h — BinHex 4.0 decode
 * Part of crispy: standalone, no Eudora dependency.
 */

#ifndef CRISPY_BINHEX_H
#define CRISPY_BINHEX_H

#include <stddef.h>
#include <stdbool.h>

/* Detect if text contains BinHex content.
 * Looks for "(This file must be converted with BinHex" header. */
bool crispy_binhex_detect(const char *data, long len);

/* Decode BinHex 4.0 data.
 * Extracts the data fork only (resource fork ignored — not portable).
 * Input: raw text containing BinHex encoded data
 * Output: malloc'd binary data, *outLen = length
 *         *outFilename = malloc'd filename (caller frees both)
 * Returns NULL if not valid BinHex. */
char *crispy_binhex_decode(const char *data, long len, long *outLen,
                           char **outFilename);

/* Encode binary data as BinHex 4.0.
 * filename: display name for the encoded file
 * data: raw binary data to encode
 * dataLen: length of data
 * outLen: receives length of encoded output
 * Returns malloc'd BinHex encoded text (caller frees).
 * Includes header line and colon delimiters. */
char *crispy_binhex_encode(const char *filename, const char *data,
                           long dataLen, long *outLen);

#endif /* CRISPY_BINHEX_H */
