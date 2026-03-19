/* transport_posix.h — POSIX socket + OpenSSL transport for maillib
 * Provides SmtpTransport that works with smtp.h and pop3.h.
 */

#ifndef CRISPY_TRANSPORT_H
#define CRISPY_TRANSPORT_H

#include "crispy_smtp.h" /* for SmtpTransport */

/* Create a POSIX/OpenSSL transport.
 * The returned SmtpTransport can be used with smtp_init() or pop3_init().
 * Call the transport's destroy() when done. */
SmtpTransport maillib_transport_posix_new(void);

#endif /* CRISPY_TRANSPORT_H */
