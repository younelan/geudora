/* Copyright (c) 2017, Computer History Museum — see keychain.h for license */

/*
 * keychain.c — Cross-platform secure credential storage.
 *
 * Platform dispatch (compile-time):
 *   macOS   (__APPLE__)          Security.framework
 *   Windows (_WIN32)             Windows Credential Manager
 *   Linux with libsecret         libsecret / GNOME Keyring
 *   Fallback                     Plaintext file, chmod 600
 *
 * Build notes:
 *   macOS:   link -framework Security -framework CoreFoundation
 *   Windows: link -lcredui (or Advapi32.lib)
 *   Linux:   pkg-config --cflags --libs libsecret-1
 *            add -DHAVE_LIBSECRET to CFLAGS if libsecret is available
 */

#include "keychain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================
 * macOS — Security.framework
 * ====================================================================== */
#if defined(__APPLE__)

#include <Security/Security.h>

int keychain_store(const char *service, const char *account,
                   const char *password) {
    /* Delete any existing entry first so SecItemAdd doesn't return errSecDuplicateItem */
    keychain_delete(service, account);

    CFStringRef svc  = CFStringCreateWithCString(NULL, service,  kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithCString(NULL, account,  kCFStringEncodingUTF8);
    CFDataRef   data = CFDataCreate(NULL, (const UInt8 *)password, (CFIndex)strlen(password));

    CFDictionaryRef attrs = CFDictionaryCreate(NULL,
        (const void *[]){
            kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData
        },
        (const void *[]){ kSecClassGenericPassword, svc, acct, data },
        4,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    OSStatus st = SecItemAdd(attrs, NULL);

    CFRelease(attrs);
    CFRelease(data);
    CFRelease(acct);
    CFRelease(svc);

    return (st == errSecSuccess) ? KEYCHAIN_OK : KEYCHAIN_ERR;
}

int keychain_find(const char *service, const char *account,
                  char *out, size_t size) {
    CFStringRef svc  = CFStringCreateWithCString(NULL, service,  kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithCString(NULL, account,  kCFStringEncodingUTF8);

    CFDictionaryRef query = CFDictionaryCreate(NULL,
        (const void *[]){
            kSecClass, kSecAttrService, kSecAttrAccount,
            kSecReturnData, kSecMatchLimit
        },
        (const void *[]){
            kSecClassGenericPassword, svc, acct,
            kCFBooleanTrue, kSecMatchLimitOne
        },
        5,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    CFDataRef result = NULL;
    OSStatus st = SecItemCopyMatching(query, (CFTypeRef *)&result);

    CFRelease(query);
    CFRelease(acct);
    CFRelease(svc);

    if (st == errSecItemNotFound) return KEYCHAIN_NOT_FOUND;
    if (st != errSecSuccess)      return KEYCHAIN_ERR;

    CFIndex len = CFDataGetLength(result);
    if ((size_t)len >= size) len = (CFIndex)(size - 1);
    memcpy(out, CFDataGetBytePtr(result), len);
    out[len] = '\0';
    CFRelease(result);
    return KEYCHAIN_OK;
}

int keychain_delete(const char *service, const char *account) {
    CFStringRef svc  = CFStringCreateWithCString(NULL, service,  kCFStringEncodingUTF8);
    CFStringRef acct = CFStringCreateWithCString(NULL, account,  kCFStringEncodingUTF8);

    CFDictionaryRef query = CFDictionaryCreate(NULL,
        (const void *[]){ kSecClass, kSecAttrService, kSecAttrAccount },
        (const void *[]){ kSecClassGenericPassword, svc, acct },
        3,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks);

    OSStatus st = SecItemDelete(query);

    CFRelease(query);
    CFRelease(acct);
    CFRelease(svc);

    if (st == errSecItemNotFound) return KEYCHAIN_NOT_FOUND;
    return (st == errSecSuccess) ? KEYCHAIN_OK : KEYCHAIN_ERR;
}

/* ======================================================================
 * Windows — Credential Manager
 * ====================================================================== */
#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincred.h>

/* Build a composite target name: "service/account" */
static void make_target(const char *service, const char *account,
                        char *buf, size_t size) {
    snprintf(buf, size, "%s/%s", service, account);
}

int keychain_store(const char *service, const char *account,
                   const char *password) {
    char target[512];
    make_target(service, account, target, sizeof(target));

    CREDENTIALA cred = {0};
    cred.Type                 = CRED_TYPE_GENERIC;
    cred.TargetName           = target;
    cred.UserName             = (LPSTR)account;
    cred.CredentialBlob       = (LPBYTE)password;
    cred.CredentialBlobSize   = (DWORD)strlen(password);
    cred.Persist              = CRED_PERSIST_LOCAL_MACHINE;

    return CredWriteA(&cred, 0) ? KEYCHAIN_OK : KEYCHAIN_ERR;
}

int keychain_find(const char *service, const char *account,
                  char *out, size_t size) {
    char target[512];
    make_target(service, account, target, sizeof(target));

    PCREDENTIALA cred = NULL;
    if (!CredReadA(target, CRED_TYPE_GENERIC, 0, &cred))
        return KEYCHAIN_NOT_FOUND;

    DWORD len = cred->CredentialBlobSize;
    if (len >= (DWORD)size) len = (DWORD)(size - 1);
    memcpy(out, cred->CredentialBlob, len);
    out[len] = '\0';
    CredFree(cred);
    return KEYCHAIN_OK;
}

int keychain_delete(const char *service, const char *account) {
    char target[512];
    make_target(service, account, target, sizeof(target));

    if (!CredDeleteA(target, CRED_TYPE_GENERIC, 0)) {
        DWORD err = GetLastError();
        return (err == ERROR_NOT_FOUND) ? KEYCHAIN_NOT_FOUND : KEYCHAIN_ERR;
    }
    return KEYCHAIN_OK;
}

/* ======================================================================
 * Linux — libsecret
 * ====================================================================== */
#elif defined(HAVE_LIBSECRET)

#include <libsecret/secret.h>

/* Schema: two attributes — "service" and "account" */
static const SecretSchema *get_schema(void) {
    static const SecretSchema schema = {
        "org.geudora.password", SECRET_SCHEMA_NONE,
        {
            { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
            { "account", SECRET_SCHEMA_ATTRIBUTE_STRING },
            { NULL, 0 }
        }
    };
    return &schema;
}

int keychain_store(const char *service, const char *account,
                   const char *password) {
    GError *err = NULL;
    char label[256];
    snprintf(label, sizeof(label), "gEudora password for %s", account);

    secret_password_store_sync(get_schema(), SECRET_COLLECTION_DEFAULT,
                               label, password, NULL, &err,
                               "service", service,
                               "account", account,
                               NULL);
    if (err) { g_error_free(err); return KEYCHAIN_ERR; }
    return KEYCHAIN_OK;
}

int keychain_find(const char *service, const char *account,
                  char *out, size_t size) {
    GError *err = NULL;
    gchar *pw = secret_password_lookup_sync(get_schema(), NULL, &err,
                                            "service", service,
                                            "account", account,
                                            NULL);
    if (err)  { g_error_free(err); return KEYCHAIN_ERR; }
    if (!pw)  return KEYCHAIN_NOT_FOUND;

    strncpy(out, pw, size - 1);
    out[size - 1] = '\0';
    secret_password_free(pw);
    return KEYCHAIN_OK;
}

int keychain_delete(const char *service, const char *account) {
    GError *err = NULL;
    gboolean found = secret_password_clear_sync(get_schema(), NULL, &err,
                                                "service", service,
                                                "account", account,
                                                NULL);
    if (err)   { g_error_free(err); return KEYCHAIN_ERR; }
    if (!found) return KEYCHAIN_NOT_FOUND;
    return KEYCHAIN_OK;
}

/* ======================================================================
 * Fallback — plaintext file, chmod 600
 * ====================================================================== */
#else

#include <sys/stat.h>
#include <glib.h>

/*
 * Storage: ~/.config/geudora/credentials (GKeyFile, mode 0600)
 * Group = service, key = account, value = password (plaintext)
 */

static gchar *cred_path(void) {
    return g_build_filename(g_get_home_dir(), ".config", "geudora",
                            "credentials", NULL);
}

static GKeyFile *cred_load(void) {
    GKeyFile *kf = g_key_file_new();
    gchar *path = cred_path();
    g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL);
    g_free(path);
    return kf;
}

static int cred_save(GKeyFile *kf) {
    gchar *path = cred_path();
    /* Ensure directory exists */
    gchar *dir = g_path_get_dirname(path);
    g_mkdir_with_parents(dir, 0700);
    g_free(dir);

    gsize len;
    gchar *data = g_key_file_to_data(kf, &len, NULL);
    gboolean ok = g_file_set_contents(path, data, (gssize)len, NULL);
    g_free(data);

    /* Restrict permissions: owner read/write only */
    chmod(path, 0600);
    g_free(path);
    return ok ? KEYCHAIN_OK : KEYCHAIN_ERR;
}

int keychain_store(const char *service, const char *account,
                   const char *password) {
    GKeyFile *kf = cred_load();
    g_key_file_set_string(kf, service, account, password);
    int r = cred_save(kf);
    g_key_file_free(kf);
    return r;
}

int keychain_find(const char *service, const char *account,
                  char *out, size_t size) {
    GKeyFile *kf = cred_load();
    gchar *val = g_key_file_get_string(kf, service, account, NULL);
    g_key_file_free(kf);
    if (!val) return KEYCHAIN_NOT_FOUND;
    strncpy(out, val, size - 1);
    out[size - 1] = '\0';
    g_free(val);
    return KEYCHAIN_OK;
}

int keychain_delete(const char *service, const char *account) {
    GKeyFile *kf = cred_load();
    gboolean found = g_key_file_remove_key(kf, service, account, NULL);
    if (found) cred_save(kf);
    g_key_file_free(kf);
    return found ? KEYCHAIN_OK : KEYCHAIN_NOT_FOUND;
}

#endif /* platform */
