/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

/* Copyright (c) 1999 by QUALCOMM Incorporated */

/*
 * imapauth.c - IMAP SASL authentication handlers
 *
 * Ported from the original Mac imapauth.c to GTK/POSIX.
 *
 * Supported:
 *   CRAM-MD5  (RFC 2195, via GLib GHmac)
 *   GSSAPI    (Kerberos V5, via system GSSAPI)
 *
 * Kerberos V4 is not supported (obsolete since 2012).
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <glib.h>
#include <gssapi/gssapi.h>

#include "imapauth.h"

/* ============================================================
 * CRAM-MD5 Authentication (RFC 2195)
 * ============================================================ */

long CramMD5Authenticator(authchallenge_t challenger, authrespond_t responder,
                           NETMBX *mb, void *s, unsigned long *trial, char *user)
{
    MAILSTREAM     *stream = (MAILSTREAM *)s;
    char            password[MAILTMPLEN];
    char            response[MAILTMPLEN];
    unsigned long   len;
    unsigned char  *pChallenge;
    GHmac          *hmac;
    long            ret = NIL;

    if (!(challenger && responder && mb && s && trial && user))
        return NIL;

    password[0] = '\0';
    mm_login(mb, user, password, *trial);

    if (!(password[0] && user[0])) {
        *trial = MAXLOGINTRIALS + 1;
        mm_log("CRAM-MD5 authenticator aborted", IMAP_ERROR);
        return NIL;
    }

    len = 0;
    pChallenge = (unsigned char *)(*challenger)(stream, &len);
    if (!pChallenge || len == 0)
        return NIL;

    /* Compute HMAC-MD5(challenge, password) per RFC 2195 */
    hmac = g_hmac_new(G_CHECKSUM_MD5, (guchar *)password, strlen(password));
    g_hmac_update(hmac, pChallenge, len);

    /* Response: "user hexdigest" */
    snprintf(response, sizeof(response), "%s %s", user, g_hmac_get_string(hmac));
    g_hmac_unref(hmac);

    fs_give((void **)&pChallenge);

    (*trial)++;
    if ((*responder)(stream, response, strlen(response)))
        ret = 1;

    return ret;
}

/* ============================================================
 * Kerberos V4 - not supported (obsolete)
 * ============================================================ */

long KrbV4Authenticator(authchallenge_t challenger, authrespond_t responder,
                         NETMBX *mb, void *s, unsigned long *trial, char *user)
{
    (void)challenger; (void)responder; (void)mb;
    (void)s; (void)trial; (void)user;
    mm_log("Kerberos V4 authentication is not supported", IMAP_ERROR);
    return NIL;
}

/* ============================================================
 * GSSAPI (Kerberos V5) Authentication
 * ============================================================ */

#define AUTH_GSSAPI_P_NONE 1

/* Per-authentication context (no Mac KClient dependency) */
typedef struct {
    authchallenge_t  challenger;
    authrespond_t    responder;
    MAILSTREAM      *mailstream;
    char             cService[255];
    char             cUser[255];
    char             cHost[255];
} CGSSAPIAuthData;

static bool gIMAPAuthedKerberos = false;

static long GSSAPIAuthenticate(CGSSAPIAuthData *gd);

long GssapiAuthenticator(authchallenge_t challenger, authrespond_t responder,
                          NETMBX *mb, void *s, unsigned long *trial, char *user)
{
    MAILSTREAM    *stream = (MAILSTREAM *)s;
    CGSSAPIAuthData authData;
    long            result = 0;

    if (!challenger || !responder || !mb || !s || !user)
        return 0;

    memset(&authData, 0, sizeof(authData));
    authData.challenger = challenger;
    authData.responder  = responder;
    authData.mailstream = stream;

    strncpy(authData.cUser, mb->user, sizeof(authData.cUser) - 1);
    strncpy(authData.cHost, mb->host, sizeof(authData.cHost) - 1);
    strncpy(authData.cService, "imap", sizeof(authData.cService) - 1);

    strncpy(user, mb->user, NETMAXUSER - 1);
    user[NETMAXUSER - 1] = '\0';
    *trial = 0;

    result = GSSAPIAuthenticate(&authData);
    if (!result)
        mm_log("GSSAPI authentication failed", IMAP_ERROR);
    else
        gIMAPAuthedKerberos = true;

    return result;
}

static long GSSAPIAuthenticate(CGSSAPIAuthData *gd)
{
    long            ret = NIL;
    char            tmp[MAILTMPLEN];
    OM_uint32       maj, min, mmaj, mmin;
    OM_uint32       mctx = 0;
    gss_ctx_id_t    ctx = GSS_C_NO_CONTEXT;
    gss_buffer_desc chal, resp, buf;
    gss_name_t      crname = GSS_C_NO_NAME;
    long            i;
    int             conf;
    gss_qop_t       qop;

    memset(&chal, 0, sizeof(chal));
    memset(&resp, 0, sizeof(resp));

    /* Get initial server challenge (may be empty for GSSAPI) */
    chal.value = (*gd->challenger)(gd->mailstream, (unsigned long *)&chal.length);
    if (!chal.value)
        return NIL;

    /* Build "service@host" principal name */
    snprintf(tmp, sizeof(tmp), "%s@%s", gd->cService, gd->cHost);
    buf.value  = tmp;
    buf.length = strlen(tmp) + 1;

    if (gss_import_name(&min, &buf, GSS_C_NT_HOSTBASED_SERVICE, &crname) != GSS_S_COMPLETE) {
        (*gd->responder)(gd->mailstream, NIL, 0);
        fs_give((void **)&chal.value);
        return NIL;
    }

    /* Initiate GSSAPI security context */
    maj = gss_init_sec_context(&min, GSS_C_NO_CREDENTIAL, &ctx,
                                crname, GSS_C_NO_OID,
                                GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,
                                0, GSS_C_NO_CHANNEL_BINDINGS,
                                GSS_C_NO_BUFFER, NULL, &resp, NULL, NULL);

    switch (maj) {
    case GSS_S_CONTINUE_NEEDED:
        do {
            if (chal.value)
                fs_give((void **)&chal.value);
            i = (*gd->responder)(gd->mailstream, (char *)resp.value, resp.length);
            gss_release_buffer(&min, &resp);
        } while (i &&
            (chal.value = (*gd->challenger)(gd->mailstream, (unsigned long *)&chal.length)) &&
            (maj = gss_init_sec_context(&min, GSS_C_NO_CREDENTIAL, &ctx,
                                         crname, GSS_C_NO_OID,
                                         GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,
                                         0, GSS_C_NO_CHANNEL_BINDINGS,
                                         &chal, NULL, &resp, NULL, NULL)) == GSS_S_CONTINUE_NEEDED);
        /* fall through */
    case GSS_S_COMPLETE:
        if (chal.value) {
            fs_give((void **)&chal.value);
            if (maj != GSS_S_COMPLETE)
                (*gd->responder)(gd->mailstream, NIL, 0);
        }
        if ((maj == GSS_S_COMPLETE) &&
            (*gd->responder)(gd->mailstream, resp.value ? (char *)resp.value : "", resp.length) &&
            (chal.value = (*gd->challenger)(gd->mailstream, (unsigned long *)&chal.length)) &&
            (gss_unwrap(&min, ctx, &chal, &resp, &conf, &qop) == GSS_S_COMPLETE) &&
            (resp.length >= 4) &&
            (*((char *)resp.value) & AUTH_GSSAPI_P_NONE))
        {
            memcpy(tmp, resp.value, 4);
            gss_release_buffer(&min, &resp);
            tmp[0] = AUTH_GSSAPI_P_NONE;
            strcpy(tmp + 4, gd->cUser);
            buf.value  = tmp;
            buf.length = strlen(gd->cUser) + 4;
            if (gss_wrap(&min, ctx, 0, qop, &buf, &conf, &resp) == GSS_S_COMPLETE) {
                if ((*gd->responder)(gd->mailstream, (char *)resp.value, resp.length))
                    ret = T;
                gss_release_buffer(&min, &resp);
            } else {
                (*gd->responder)(gd->mailstream, NIL, 0);
            }
        }
        if (chal.value)
            fs_give((void **)&chal.value);
        gss_delete_sec_context(&min, &ctx, GSS_C_NO_BUFFER);
        break;

    case GSS_S_CREDENTIALS_EXPIRED:
        if (chal.value) fs_give((void **)&chal.value);
        mm_log("GSSAPI: credentials expired", IMAP_ERROR);
        (*gd->responder)(gd->mailstream, NIL, 0);
        break;

    case GSS_S_FAILURE:
        if (chal.value) fs_give((void **)&chal.value);
        do {
            switch (mmaj = gss_display_status(&mmin, min, GSS_C_MECH_CODE,
                                               GSS_C_NULL_OID, &mctx, &resp)) {
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
                mm_log((char *)resp.value, IMAP_ERROR);
                gss_release_buffer(&mmin, &resp);
            }
        } while (mmaj == GSS_S_CONTINUE_NEEDED);
        (*gd->responder)(gd->mailstream, NIL, 0);
        break;

    default:
        if (chal.value) fs_give((void **)&chal.value);
        do {
            switch (mmaj = gss_display_status(&mmin, maj, GSS_C_GSS_CODE,
                                               GSS_C_NULL_OID, &mctx, &resp)) {
            case GSS_S_COMPLETE:
                mctx = 0;
                /* fall through */
            case GSS_S_CONTINUE_NEEDED:
                mm_log((char *)resp.value, IMAP_ERROR);
                gss_release_buffer(&mmin, &resp);
            }
        } while (mmaj == GSS_S_CONTINUE_NEEDED);
        mctx = 0;
        do {
            switch (mmaj = gss_display_status(&mmin, min, GSS_C_MECH_CODE,
                                               GSS_C_NULL_OID, &mctx, &resp)) {
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
                mm_log((char *)resp.value, IMAP_ERROR);
                gss_release_buffer(&mmin, &resp);
            }
        } while (mmaj == GSS_S_CONTINUE_NEEDED);
        (*gd->responder)(gd->mailstream, NIL, 0);
        break;
    }

    if (crname != GSS_C_NO_NAME)
        gss_release_name(&min, &crname);

    return ret;
}

/* ============================================================
 * Kerberos usage tracking (GSSAPI/IMAP only)
 * ============================================================ */

void UsedKerberos(void)
{
    gIMAPAuthedKerberos = true;
}

bool KerberosWasUsed(void)
{
    return gIMAPAuthedKerberos;
}
