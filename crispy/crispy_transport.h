/* transport_posix.h — POSIX socket + OpenSSL transport for maillib
 * Provides SmtpTransport that works with smtp.h and pop3.h.
 */

#ifndef CRISPY_TRANSPORT_H
#define CRISPY_TRANSPORT_H

#include "crispy_smtp.h" /* for SmtpTransport */

/* Create a POSIX/OpenSSL transport.
 * The returned SmtpTransport can be used with crispy_smtp_init() or crispy_pop3_init().
 * Call the transport's destroy() when done. */
SmtpTransport crispy_transport_new(void);

#endif /* CRISPY_TRANSPORT_H */
