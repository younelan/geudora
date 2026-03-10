/* Simple per-mailbox JSON sidecar store for message errors. */
#ifndef MESG_ERROR_STORE_H
#define MESG_ERROR_STORE_H

#include "mailbox.h"

/* Load message-error sidecar for `tocH` and populate
 * `(*tocH)->sums[].mesgErrH`. Returns 0 on success, non-zero on error (file
 * missing is success with no data).
 */
int mesg_error_store_load(TOCHandle tocH);

/* Save all in-memory message-error records for `tocH` to a sidecar next to
 * mailbox. Returns 0 on success, non-zero on error.
 */
int mesg_error_store_save_all(TOCHandle tocH);

#endif
