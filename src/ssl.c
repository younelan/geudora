/* ssl.c — OpenSSL transport wrapper for gEudora
 *
 * Port of the original Mac Eudora ssl.c. Uses system OpenSSL directly
 * (no CFBundle loading, no Keychain — just standard POSIX + OpenSSL).
 *
 * Architecture: ESSLSetupVector() wraps the plain TCP TransVector.
 * When SSL is active (esslSSLInUse), send/recv go through OpenSSL;
 * otherwise they fall through to the underlying TCP functions.
 *
 * Certificate handling:
 *   - System CA store for standard servers
 *   - Local trust store (~/.local/share/geudora/certs/) for accepted certs
 *   - Custom verify callback prompts user for untrusted/self-signed certs
 *
 * Copyright (c) 2017, Computer History Museum — All rights reserved.
 * Original code under BSD license. Port modifications for GTK/POSIX.
 */

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#include "myssl.h"
#include "mydefs.h"
#include "TransStream.h"
#include "threading.h"

/* Forward declarations — ESSL transport wrappers */
static int ESSLConnectTrans(TransStream stream, const char *serverName, long port,
                              bool silently, uLong timeout);
static int ESSLSendTrans(TransStream stream, const char *text, long size, ...);
static int ESSLReceiveTrans(TransStream stream, char * line, long *size);
static int ESSLDisTrans(TransStream stream);
static int ESSLDestroyTrans(TransStream stream);
static int ESSLTransErr(TransStream stream);
static void ESSLSilenceTrans(TransStream stream, bool silence);
static char *ESSLWhoAmI(TransStream stream, char *who);

/* Forward declarations — certificate management */
static int ssl_verify_callback(int preverify_ok, X509_STORE_CTX *ctx);
static char *get_certs_dir(void);
static char *cert_fingerprint(X509 *cert);
static int is_cert_trusted_locally(X509 *cert);
static int save_cert_locally(X509 *cert);

/* Defined in tcp.c */
extern int NetRecvLine(TransStream stream, char * line, long *size);

/* The underlying (unwrapped) TCP transport vector */
TransVector ESSLSubTrans;

/* The SSL-wrapped transport vector */
static TransVector ESSLTrans = {
    ESSLConnectTrans,
    ESSLSendTrans,
    ESSLReceiveTrans,
    ESSLDisTrans,
    ESSLDestroyTrans,
    ESSLTransErr,
    ESSLSilenceTrans,
    NULL, /* vSendWDS */
    (char *(*)(TransStream, char *))ESSLWhoAmI,
    NetRecvLine, /* vRecvLine — line-buffered recv calls RecvTrans internally */
    NULL  /* vAsyncSendTrans */
};

/* One-time OpenSSL library init */
static pthread_once_t ssl_init_once = PTHREAD_ONCE_INIT;

static void ssl_lib_init(void) {
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS |
                     OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
}

/* ===================================================================
 * Certificate trust store — local file-based store
 * Certs saved to ~/.local/share/geudora/certs/<sha256>.pem
 * =================================================================== */

static char *get_certs_dir(void) {
    const char *data_home = g_get_user_data_dir();
    char *dir = g_build_filename(data_home, "geudora", "certs", NULL);
    g_mkdir_with_parents(dir, 0700);
    return dir;
}

/* Return hex SHA-256 fingerprint of a certificate (caller must g_free) */
static char *cert_fingerprint(X509 *cert) {
    unsigned char md[SHA256_DIGEST_LENGTH];
    unsigned int len = sizeof(md);
    if (!X509_digest(cert, EVP_sha256(), md, &len))
        return NULL;

    char *hex = g_malloc(len * 3);
    char *p = hex;
    for (unsigned int i = 0; i < len; i++) {
        if (i > 0) *p++ = ':';
        p += sprintf(p, "%02X", md[i]);
    }
    return hex;
}

/* File-safe fingerprint for filename (no colons) */
static char *cert_filename_hash(X509 *cert) {
    unsigned char md[SHA256_DIGEST_LENGTH];
    unsigned int len = sizeof(md);
    if (!X509_digest(cert, EVP_sha256(), md, &len))
        return NULL;

    char *hex = g_malloc(len * 2 + 1);
    for (unsigned int i = 0; i < len; i++)
        sprintf(hex + i * 2, "%02x", md[i]);
    return hex;
}

/* Check if certificate is in local trust store */
static int is_cert_trusted_locally(X509 *cert) {
    char *hash = cert_filename_hash(cert);
    if (!hash) return 0;

    char *dir = get_certs_dir();
    char *path = g_build_filename(dir, hash, NULL);
    g_free(dir);
    g_free(hash);

    /* Append .pem */
    char *pempath = g_strdup_printf("%s.pem", path);
    g_free(path);

    int exists = g_file_test(pempath, G_FILE_TEST_EXISTS);
    g_free(pempath);
    return exists;
}

/* Save certificate to local trust store */
static int save_cert_locally(X509 *cert) {
    char *hash = cert_filename_hash(cert);
    if (!hash) return -1;

    char *dir = get_certs_dir();
    char *pempath = g_strdup_printf("%s/%s.pem", dir, hash);
    g_free(dir);
    g_free(hash);

    FILE *f = fopen(pempath, "w");
    g_free(pempath);
    if (!f) return -1;

    PEM_write_X509(f, cert);
    fclose(f);
    return 0;
}

/* Load all locally trusted certs into an SSL_CTX */
static void load_local_trust_store(SSL_CTX *ctx) {
    char *dir = get_certs_dir();
    GDir *gdir = g_dir_open(dir, 0, NULL);
    if (!gdir) {
        g_free(dir);
        return;
    }

    X509_STORE *store = SSL_CTX_get_cert_store(ctx);
    const char *name;
    while ((name = g_dir_read_name(gdir)) != NULL) {
        if (!g_str_has_suffix(name, ".pem"))
            continue;

        char *path = g_build_filename(dir, name, NULL);
        FILE *f = fopen(path, "r");
        g_free(path);
        if (!f) continue;

        X509 *cert = PEM_read_X509(f, NULL, NULL, NULL);
        fclose(f);
        if (cert) {
            X509_STORE_add_cert(store, cert);
            X509_free(cert);
        }
    }

    g_dir_close(gdir);
    g_free(dir);
}

/* ===================================================================
 * SSL certificate prompt — ask user about untrusted certs
 * Marshals a GTK dialog from the background thread to the main thread.
 * =================================================================== */

/* User responses from the cert dialog */
enum {
    CERT_REJECT = 0,
    CERT_ACCEPT_ONCE = 1,
    CERT_ACCEPT_ALWAYS = 2
};

/* Data passed between background thread and main thread for cert prompt */
typedef struct {
    char subject[256];
    char issuer[256];
    char *fingerprint;
    const char *error_str;
    int error_code;
    int user_response;        /* filled by main thread */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    gboolean done;
} CertPromptData;

/* Main window accessor (defined in main_eudora.c) */
extern GtkWidget *get_main_window(void);

/* Button context for cert dialog callbacks */
typedef struct {
    CertPromptData *d;
    GMainLoop *l;
    GtkWidget *w;
    int resp;
} BtnCtx;

static void cert_btn_clicked(GtkWidget *widget, gpointer ud) {
    (void)widget;
    BtnCtx *c = (BtnCtx *)ud;
    c->d->user_response = c->resp;
    gtk_window_destroy(GTK_WINDOW(c->w));
    g_main_loop_quit(c->l);
}

static gboolean cert_dlg_close(GtkWindow *window, gpointer ud) {
    (void)window;
    BtnCtx *c = (BtnCtx *)ud;
    c->d->user_response = CERT_REJECT;
    g_main_loop_quit(c->l);
    return FALSE;
}

/* Called on the GTK main thread via g_idle_add */
static gboolean cert_prompt_on_main_thread(gpointer user_data) {
    CertPromptData *data = (CertPromptData *)user_data;

    GtkWidget *dlg = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "SSL Certificate Warning");
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 480, -1);
    gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

    GtkWidget *parent = get_main_window();
    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(parent));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 16);
    gtk_widget_set_margin_bottom(vbox, 16);
    gtk_window_set_child(GTK_WINDOW(dlg), vbox);

    /* Warning icon + title */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<b><span size='large'>Untrusted SSL Certificate</span></b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0);
    gtk_box_append(GTK_BOX(vbox), title);

    /* Error description */
    char *err_text = g_strdup_printf(
        "The server's certificate could not be verified.\n\n"
        "<b>Error:</b> %s\n\n"
        "<b>Subject:</b> %s\n"
        "<b>Issuer:</b> %s\n"
        "<b>Fingerprint:</b>\n<tt>%s</tt>",
        data->error_str ? data->error_str : "Unknown error",
        data->subject, data->issuer,
        data->fingerprint ? data->fingerprint : "(unknown)");
    GtkWidget *info = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(info), err_text);
    gtk_label_set_xalign(GTK_LABEL(info), 0);
    gtk_label_set_wrap(GTK_LABEL(info), TRUE);
    gtk_label_set_selectable(GTK_LABEL(info), TRUE);
    gtk_box_append(GTK_BOX(vbox), info);
    g_free(err_text);

    /* Separator */
    gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    /* Buttons */
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    gtk_widget_set_margin_top(btn_box, 8);

    GtkWidget *reject_btn = gtk_button_new_with_label("Reject");
    GtkWidget *once_btn = gtk_button_new_with_label("Accept Once");
    GtkWidget *always_btn = gtk_button_new_with_label("Accept Always");
    gtk_widget_add_css_class(reject_btn, "destructive-action");
    gtk_widget_add_css_class(always_btn, "suggested-action");

    gtk_box_append(GTK_BOX(btn_box), reject_btn);
    gtk_box_append(GTK_BOX(btn_box), once_btn);
    gtk_box_append(GTK_BOX(btn_box), always_btn);
    gtk_box_append(GTK_BOX(vbox), btn_box);

    /* Run a nested main loop for the dialog */
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    /* Context for button callbacks */
    typedef struct { CertPromptData *d; GMainLoop *l; GtkWidget *w; int resp; } BtnCtx;
    BtnCtx ctx_reject = {data, loop, dlg, CERT_REJECT};
    BtnCtx ctx_once   = {data, loop, dlg, CERT_ACCEPT_ONCE};
    BtnCtx ctx_always = {data, loop, dlg, CERT_ACCEPT_ALWAYS};

    g_signal_connect_data(reject_btn, "clicked",
        G_CALLBACK(cert_btn_clicked), &ctx_reject, NULL, 0);
    g_signal_connect_data(once_btn, "clicked",
        G_CALLBACK(cert_btn_clicked), &ctx_once, NULL, 0);
    g_signal_connect_data(always_btn, "clicked",
        G_CALLBACK(cert_btn_clicked), &ctx_always, NULL, 0);
    g_signal_connect_data(dlg, "close-request",
        G_CALLBACK(cert_dlg_close), &ctx_reject, NULL, 0);

    data->user_response = CERT_REJECT; /* default if something goes wrong */
    gtk_window_present(GTK_WINDOW(dlg));
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    /* Wake up the background thread */
    pthread_mutex_lock(&data->mutex);
    data->done = TRUE;
    pthread_cond_signal(&data->cond);
    pthread_mutex_unlock(&data->mutex);

    return G_SOURCE_REMOVE;
}

/* Show cert prompt from any thread — blocks until user responds */
static int prompt_user_for_cert(const char *subject, const char *issuer,
                                 char *fingerprint, const char *error_str,
                                 int error_code) {
    CertPromptData data;
    g_strlcpy(data.subject, subject, sizeof(data.subject));
    g_strlcpy(data.issuer, issuer, sizeof(data.issuer));
    data.fingerprint = fingerprint;
    data.error_str = error_str;
    data.error_code = error_code;
    data.user_response = CERT_REJECT;
    data.done = FALSE;
    pthread_mutex_init(&data.mutex, NULL);
    pthread_cond_init(&data.cond, NULL);

    /* Schedule dialog on main thread */
    g_idle_add(cert_prompt_on_main_thread, &data);

    /* Wait for user response */
    pthread_mutex_lock(&data.mutex);
    while (!data.done)
        pthread_cond_wait(&data.cond, &data.mutex);
    pthread_mutex_unlock(&data.mutex);

    pthread_mutex_destroy(&data.mutex);
    pthread_cond_destroy(&data.cond);

    return data.user_response;
}

/* ===================================================================
 * SSL verify callback — called by OpenSSL during handshake
 * =================================================================== */

static int ssl_verify_callback(int preverify_ok, X509_STORE_CTX *store_ctx) {
    if (preverify_ok)
        return 1; /* system CAs verified it — all good */

    X509 *cert = X509_STORE_CTX_get_current_cert(store_ctx);
    int err = X509_STORE_CTX_get_error(store_ctx);
    int depth = X509_STORE_CTX_get_error_depth(store_ctx);

    /* For intermediate/root certs, check local trust silently */
    if (depth > 0)
        return is_cert_trusted_locally(cert) ? 1 : 0;

    /* Check local trust store first — no prompt needed */
    if (is_cert_trusted_locally(cert))
        return 1;

    /* Get certificate details */
    char subject[256] = {0};
    char issuer_str[256] = {0};
    char *fingerprint = cert_fingerprint(cert);

    X509_NAME_oneline(X509_get_subject_name(cert), subject, sizeof(subject));
    X509_NAME_oneline(X509_get_issuer_name(cert), issuer_str, sizeof(issuer_str));

    const char *err_str = X509_verify_cert_error_string(err);

    g_printerr("\nSSL Certificate Warning:\n"
               "  Subject:     %s\n"
               "  Issuer:      %s\n"
               "  Fingerprint: %s\n"
               "  Error:       %s (code %d)\n",
               subject, issuer_str,
               fingerprint ? fingerprint : "(unknown)",
               err_str ? err_str : "unknown", err);

    /* Prompt the user */
    int response = prompt_user_for_cert(subject, issuer_str, fingerprint,
                                         err_str, err);

    switch (response) {
    case CERT_ACCEPT_ALWAYS:
        g_printerr("  Action:      Accepted (saved to local trust store)\n\n");
        save_cert_locally(cert);
        g_free(fingerprint);
        return 1;

    case CERT_ACCEPT_ONCE:
        g_printerr("  Action:      Accepted (this session only)\n\n");
        g_free(fingerprint);
        return 1;

    default:
        g_printerr("  Action:      REJECTED by user\n\n");
        g_free(fingerprint);
        return 0;
    }
}

/************************************************************************
 * ESSLSetupVector — swap in the SSL transport vector
 ************************************************************************/
TransVector ESSLSetupVector(TransVector theTrans) {
    ESSLSubTrans = theTrans;
    return ESSLTrans;
}

/************************************************************************
 * SetupSSLConnection — create SSL_CTX and SSL, attach to socket via BIO
 ************************************************************************/
static int SetupSSLConnection(TransStream stream) {
    pthread_once(&ssl_init_once, ssl_lib_init);

    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        g_printerr("SSL: SSL_CTX_new failed\n");
        return -1;
    }

    /* Minimum TLS 1.2 — modern security baseline */
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    /* Load system CA certificates */
    SSL_CTX_set_default_verify_paths(ctx);

    /* Load locally accepted certificates */
    load_local_trust_store(ctx);

    /* Set verification with our custom callback */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, ssl_verify_callback);

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        g_printerr("SSL: SSL_new failed\n");
        SSL_CTX_free(ctx);
        return -1;
    }

    /* Attach the existing TCP socket to OpenSSL via BIO_s_socket */
    BIO *bio = BIO_new_socket(stream->sockfd, BIO_NOCLOSE);
    if (!bio) {
        g_printerr("SSL: BIO_new_socket failed\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        return -1;
    }
    SSL_set_bio(ssl, bio, bio);
    SSL_set_connect_state(ssl);

    /* Enable SNI (Server Name Indication) */
    if (stream->serverName[0])
        SSL_set_tlsext_host_name(ssl, (const char *)stream->serverName);

    stream->ctx = ctx;
    stream->ssl = ssl;

    return 0;
}

/************************************************************************
 * CleanupSSLConnection — free SSL and SSL_CTX
 ************************************************************************/
static int CleanupSSLConnection(TransStream stream) {
    if (stream->ssl) {
        SSL_shutdown((SSL *)stream->ssl);
        SSL_free((SSL *)stream->ssl);
        stream->ssl = NULL;
    }
    if (stream->ctx) {
        SSL_CTX_free((SSL_CTX *)stream->ctx);
        stream->ctx = NULL;
    }
    return 0;
}

/************************************************************************
 * ESSLStartSSLLo — perform SSL/TLS handshake
 ************************************************************************/
static OSStatus ESSLStartSSLLo(TransStream stream) {
    if (stream->ESSLSetting & esslSSLInUse)
        return noErr; /* already done */

    /* If SSL objects not set up yet (STARTTLS case), set them up now */
    if (!stream->ssl) {
        int err = SetupSSLConnection(stream);
        if (err) return paramErr;

        /* Re-attach BIO since socket is already connected */
        BIO *bio = BIO_new_socket(stream->sockfd, BIO_NOCLOSE);
        if (!bio) return paramErr;
        SSL_set_bio((SSL *)stream->ssl, bio, bio);

        if (stream->serverName[0])
            SSL_set_tlsext_host_name((SSL *)stream->ssl,
                                     (const char *)stream->serverName);
    }

    int hsErr = SSL_do_handshake((SSL *)stream->ssl);
    if (hsErr == 1) {
        stream->ESSLSetting |= esslSSLInUse;
        g_print("SSL: handshake succeeded (%s)\n",
                SSL_get_version((SSL *)stream->ssl));
        return noErr;
    }

    int sslErr = SSL_get_error((SSL *)stream->ssl, hsErr);
    int retries = 0;
    while (hsErr != 1 && retries < 50) {
        switch (sslErr) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
            hsErr = SSL_do_handshake((SSL *)stream->ssl);
            sslErr = SSL_get_error((SSL *)stream->ssl, hsErr);
            retries++;
            break;
        default:
            goto handshake_failed;
        }
    }

    if (hsErr == 1) {
        stream->ESSLSetting |= esslSSLInUse;
        g_print("SSL: handshake succeeded (%s)\n",
                SSL_get_version((SSL *)stream->ssl));
        return noErr;
    }

handshake_failed:
    g_printerr("SSL: handshake failed (err=%d ssl_err=%d)\n", hsErr, sslErr);
    ERR_print_errors_fp(stderr);

    if (stream->ESSLSetting & esslOptional)
        return noErr; /* optional SSL — continue unencrypted */

    return paramErr;
}

/************************************************************************
 * ESSLStartSSL — thread-safe wrapper around handshake
 ************************************************************************/
static pthread_mutex_t ssl_handshake_mutex = PTHREAD_MUTEX_INITIALIZER;

OSStatus ESSLStartSSL(TransStream stream) {
    pthread_mutex_lock(&ssl_handshake_mutex);
    OSStatus err = ESSLStartSSLLo(stream);
    pthread_mutex_unlock(&ssl_handshake_mutex);
    return err;
}

/************************************************************************
 * ESSLConnectTrans — connect, then optionally start SSL
 ************************************************************************/
static int ESSLConnectTrans(TransStream stream, const char *serverName, long port,
                              bool silently, uLong timeout) {
    /* Connect via the underlying TCP transport first */
    int err = (*ESSLSubTrans.vConnectTrans)(stream, serverName, port,
                                              silently, timeout);
    if (err)
        return err;

    /* Store server info */
    stream->port = port;
    if (serverName)
        g_strlcpy((char *)stream->serverName, (const char *)serverName,
                  sizeof(stream->serverName));

    /* Set up SSL context if SSL is requested */
    if (stream->ESSLSetting > 1) {
        err = SetupSSLConnection(stream);
        if (err && !(stream->ESSLSetting && esslOptional))
            return err;

        /* For alternate port (implicit SSL), handshake immediately */
        if (stream->ESSLSetting & esslUseAltPort)
            err = ESSLStartSSL(stream);
    }

    return err;
}

/************************************************************************
 * ESSLSendTrans — send data, using SSL if active
 ************************************************************************/
static int ESSLSendTrans(TransStream stream, const char *text, long size, ...) {
    int err = noErr;
    va_list ap;

    if (size == 0)
        return noErr;

    va_start(ap, size);

    do {
        if (!(stream->ESSLSetting & esslSSLInUse)) {
            err = (*ESSLSubTrans.vSendTrans)(stream, text, size, NULL);
        } else {
            long offset = 0;
            while (offset < size) {
                int written = SSL_write((SSL *)stream->ssl,
                                        text + offset, (int)(size - offset));
                if (written <= 0) {
                    int sslErr = SSL_get_error((SSL *)stream->ssl, written);
                    if (sslErr == SSL_ERROR_WANT_WRITE)
                        continue;
                    err = -1;
                    break;
                }
                offset += written;
            }
        }

        text = va_arg(ap, char *);
        if (text)
            size = va_arg(ap, long);
    } while (!err && text);

    va_end(ap);
    return err;
}

/************************************************************************
 * ESSLReceiveTrans — receive data, using SSL if active
 ************************************************************************/
static int ESSLReceiveTrans(TransStream stream, char * line, long *size) {
    if (!(stream->ESSLSetting & esslSSLInUse))
        return (*ESSLSubTrans.vRecvTrans)(stream, line, size);

    int bytesRead = SSL_read((SSL *)stream->ssl, line, (int)*size);
    if (bytesRead > 0) {
        *size = bytesRead;
        return noErr;
    }

    *size = 0;
    int sslErr = SSL_get_error((SSL *)stream->ssl, bytesRead);
    if (bytesRead == 0)
        return stream->streamErr ? stream->streamErr : -1;
    if (sslErr == SSL_ERROR_WANT_READ)
        return noErr;
    return stream->streamErr ? stream->streamErr : -1;
}

/************************************************************************
 * Passthrough functions — delegate to the underlying TCP transport
 ************************************************************************/
static int ESSLDisTrans(TransStream stream) {
    return (*ESSLSubTrans.vDisTrans)(stream);
}

static int ESSLDestroyTrans(TransStream stream) {
    CleanupSSLConnection(stream);
    stream->ESSLSetting = 0;
    return (*ESSLSubTrans.vDestroyTrans)(stream);
}

static int ESSLTransErr(TransStream stream) {
    return (*ESSLSubTrans.vTransError)(stream);
}

static void ESSLSilenceTrans(TransStream stream, bool silence) {
    (*ESSLSubTrans.vSilenceTrans)(stream, silence);
}

static char *ESSLWhoAmI(TransStream stream, char *who) {
    return (*ESSLSubTrans.vWhoAmI)(stream, who);
}
