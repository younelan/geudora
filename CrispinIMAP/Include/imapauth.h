/* imapauth.h - IMAP authentication handler declarations
 * Ported from original Mac imapauth.h for the GTK/POSIX port.
 *
 * The three authenticators below are registered in mail.c's AUTHENTICATOR
 * structs (c-client layer).  Implementations live in src/imapauth.c.
 */
#ifndef IMAPAUTH_H
#define IMAPAUTH_H

#include <stdbool.h>
#include "mail.h"

/* c-client authenticator callbacks - registered in mail.c */
long CramMD5Authenticator(authchallenge_t challenger, authrespond_t responder,
                          NETMBX *mb, void *s, unsigned long *trial,
                          char *user);
long KrbV4Authenticator(authchallenge_t challenger, authrespond_t responder,
                        NETMBX *mb, void *s, unsigned long *trial, char *user);
long GssapiAuthenticator(authchallenge_t challenger, authrespond_t responder,
                         NETMBX *mb, void *s, unsigned long *trial, char *user);

/* Kerberos usage tracking (GSSAPI/IMAP only) */
void UsedKerberos(void);
bool KerberosWasUsed(void);

#endif /* IMAPAUTH_H */
