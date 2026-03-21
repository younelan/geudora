/* macmbx_mailer.h — Mail send/receive engine
 * Part of macmbx: standalone mail data management library.
 *
 * Complete mail transfer: spool, send, receive, deliver, filter.
 * Uses crispy for network (SMTP/POP3/IMAP) and macmbx for storage.
 * Standalone — works without Eudora UI.
 *
 * Portable: POSIX + Windows. No GTK, no GLib.
 */

#ifndef MACMBX_MAILER_H
#define MACMBX_MAILER_H

#include "macmbx.h"
#include "macmbx_conf.h"

/* Forward declarations for crispy types (opaque to callers) */
typedef struct SmtpSession SmtpSession;
typedef struct SmtpTransport SmtpTransport;

/* ================================================================
 * Mailer instance
 * ================================================================ */

typedef struct MacmbxMailer MacmbxMailer;

/* ================================================================
 * Callbacks — optional, for UI integration
 * ================================================================ */

/* Progress callback during send/receive */
typedef void (*MacmbxMailerProgressFn)(const char *status, int current,
                                        int total, void *ctx);

/* Credential callback — called if password not in config.
 * Fill password buffer, return 0 on success, -1 to cancel. */
typedef int (*MacmbxMailerCredentialFn)(const char *account_name,
                                         const char *server,
                                         char *password, int pw_size,
                                         void *ctx);

/* Certificate callback — called on unknown/untrusted cert.
 * Return 0 to accept, 1 to accept+save, -1 to reject. */
typedef int (*MacmbxMailerCertFn)(const char *host, const char *cert_info,
                                   void *ctx);

/* ================================================================
 * Lifecycle
 * ================================================================ */

/* Create mailer. conf and store are borrowed (not freed by mailer). */
MacmbxMailer *macmbx_mailer_new(MacmbxConf *conf, MacmbxStore *store);

/* Free mailer. Closes any open connections. */
void macmbx_mailer_free(MacmbxMailer *m);

/* Register optional callbacks. */
void macmbx_mailer_set_progress(MacmbxMailer *m, MacmbxMailerProgressFn fn, void *ctx);
void macmbx_mailer_set_credentials(MacmbxMailer *m, MacmbxMailerCredentialFn fn, void *ctx);
void macmbx_mailer_set_cert_callback(MacmbxMailer *m, MacmbxMailerCertFn fn, void *ctx);

/* Set filter set (applied after receive). Optional. */
void macmbx_mailer_set_filters(MacmbxMailer *m, MacmbxFilterSet *filters);

/* Set junk config (scoring after receive). Optional. */
void macmbx_mailer_set_junk(MacmbxMailer *m, MacmbxJunkConfig *junk);

/* Set signature text (appended to outgoing). Optional. */
void macmbx_mailer_set_signature(MacmbxMailer *m, const char *sig);

/* ================================================================
 * Outbound — queue and send
 * ================================================================ */

/* Queue a message for later sending.
 * Writes to the Out spool mailbox with state QUEUED.
 * Returns message index in Out TOC, or -1 on error. */
int macmbx_mailer_queue(MacmbxMailer *m, const char *message, long len,
                          const char *from, const char **rcpts);

/* Queue a message for a specific account/personality. */
int macmbx_mailer_queue_as(MacmbxMailer *m, int account_index,
                             const char *message, long len,
                             const char *from, const char **rcpts);

/* Send all queued messages.
 * Connects to SMTP server(s), sends each queued message, marks as SENT.
 * Returns count sent, or negative on error. */
int macmbx_mailer_send(MacmbxMailer *m);

/* Send queued messages for a specific account only. */
int macmbx_mailer_send_account(MacmbxMailer *m, int account_index);

/* Send a message immediately (no queue, direct send).
 * Returns 0 on success. */
int macmbx_mailer_send_now(MacmbxMailer *m, const char *message, long len,
                             const char *from, const char **rcpts);

/* Send immediately via a specific account. */
int macmbx_mailer_send_now_as(MacmbxMailer *m, int account_index,
                                const char *message, long len,
                                const char *from, const char **rcpts);

/* Cancel a queued message (remove from spool). */
int macmbx_mailer_cancel(MacmbxMailer *m, int queue_index);

/* Get count of queued messages. */
int macmbx_mailer_queued_count(MacmbxMailer *m);

/* ================================================================
 * Inbound — check and receive
 * ================================================================ */

/* Check mail for all accounts.
 * Downloads new messages to inbox, runs filters, scores junk.
 * Returns total new message count, or negative on error. */
int macmbx_mailer_check(MacmbxMailer *m);

/* Check mail for a specific account. */
int macmbx_mailer_check_account(MacmbxMailer *m, int account_index);

/* Check mail for the dominant (main) account. */
int macmbx_mailer_check_dominant(MacmbxMailer *m);

/* ================================================================
 * LMOS — Leave Mail On Server
 * ================================================================ */

/* Get LMOS tracking file path for an account. */
const char *macmbx_mailer_lmos_path(MacmbxMailer *m, int account_index);

/* Check if a message UIDL has been downloaded already. */
bool macmbx_mailer_lmos_seen(MacmbxMailer *m, int account_index,
                               const char *uidl);

/* Mark a UIDL as downloaded. */
void macmbx_mailer_lmos_mark(MacmbxMailer *m, int account_index,
                               const char *uidl);

/* ================================================================
 * On-demand fetch — for headers-only / leave-on-server mode
 *
 * When configured to download headers only, message bodies and
 * attachments are fetched on demand when the user views them.
 * macmbx_mailer handles the crispy_imap connection internally.
 * ================================================================ */

/* Ensure a message body is fully downloaded.
 * If the message is a stub (headers only), fetches the full body
 * from the server via IMAP and updates the local mbox + TOC.
 * Returns 0 if body is available, -1 on error. */
int macmbx_mailer_ensure_body(MacmbxMailer *m, MacmbxTOC *toc, int index);

/* Fetch a specific MIME attachment by part number.
 * Downloads the attachment from the server, decodes it,
 * saves to the attachments directory.
 * out_path: filled with the local file path (caller provides buffer).
 * Returns 0 on success. */
int macmbx_mailer_fetch_attachment(MacmbxMailer *m, MacmbxTOC *toc,
                                     int index, const char *part_id,
                                     char *out_path, int path_size);

/* Check if a message is a stub (headers only, body not downloaded). */
bool macmbx_mailer_is_stub(MacmbxMailer *m, MacmbxTOC *toc, int index);

/* ================================================================
 * Connection management
 * ================================================================ */

/* Test connection to an account's servers (SMTP + POP/IMAP).
 * Returns 0 if both succeed. */
int macmbx_mailer_test_connection(MacmbxMailer *m, int account_index);

/* Disconnect all open connections. */
void macmbx_mailer_disconnect(MacmbxMailer *m);

#endif /* MACMBX_MAILER_H */
