/* crispy_transport.h — POSIX socket + OpenSSL transport
 * Provides SmtpTransport that works with crispy_smtp and crispy_pop3.
 */

#ifndef CRISPY_TRANSPORT_H
#define CRISPY_TRANSPORT_H

#include "crispy_smtp.h" /* for SmtpTransport */

/* --- Certificate verification callback ---
 *
 * Called when a server certificate fails validation.
 *
 * Parameters:
 *   host      — the server hostname
 *   error     — human-readable error string (e.g. "self-signed certificate")
 *   cert_pem  — the certificate in PEM format (for display/storage)
 *   userdata  — opaque pointer set via crispy_transport_set_cert_callback
 *
 * Return:
 *   true  — accept the certificate (connection proceeds)
 *   false — reject (connection fails)
 */
typedef bool (*CrispyCertCallback)(const char *host, const char *error,
                                   const char *cert_pem, void *userdata);

/* --- Transport creation --- */

/* Create a POSIX/OpenSSL transport. */
SmtpTransport crispy_transport_new(void);

/* Set certificate verification callback.
 * Must be called after crispy_transport_new(), before connect.
 * If not set, invalid certs are rejected. */
void crispy_transport_set_cert_callback(SmtpTransport *tp,
                                        CrispyCertCallback cb,
                                        void *userdata);

/* Load a previously trusted cert for a host into the transport.
 * Must be called before connect. Takes ownership of pem (will be freed). */
void crispy_transport_load_trusted(SmtpTransport *tp, char *pem);

/* --- Certificate store ---
 * Simple file-based store: one PEM file per host in a directory.
 * certDir is e.g. "~/.local/share/geudora/certs/"
 */

/* Store a trusted certificate PEM for a host. Returns 0 on success. */
int crispy_cert_store_save(const char *certDir, const char *host,
                           const char *cert_pem);

/* Load a trusted cert for a host. Returns PEM (caller frees) or NULL. */
char *crispy_cert_store_load(const char *certDir, const char *host);

/* Delete a trusted cert for a host. Returns 0 on success. */
int crispy_cert_store_delete(const char *certDir, const char *host);

/* List all trusted hosts. Returns NULL-terminated array of hostnames.
 * Caller must free each string and the array. */
char **crispy_cert_store_list(const char *certDir, int *count);

/* Get cert info as human-readable text (subject, issuer, dates, fingerprint).
 * Returns malloc'd string. Caller frees. */
char *crispy_cert_info(const char *cert_pem);

#endif /* CRISPY_TRANSPORT_H */
