/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED. */

#ifndef KEYCHAIN_H
#define KEYCHAIN_H

#include <stddef.h>

/*
 * Cross-platform secure credential storage.
 *
 * Backends (selected at compile time):
 *   macOS   — Security.framework (Keychain)
 *   Windows — Windows Credential Manager (wincred)
 *   Linux   — libsecret (GNOME Keyring / KDE Wallet)
 *   Fallback— plaintext file, chmod 600
 *
 * All functions return 0 on success, non-zero on error.
 *
 * 'service'  — application name or protocol, e.g. "gEudora-POP3"
 * 'account'  — username or unique identifier, e.g. "youness@host"
 * 'password' — null-terminated UTF-8 string
 */

/* Store or replace a credential. */
int keychain_store(const char *service, const char *account,
                   const char *password);

/* Retrieve a credential into 'out' (null-terminated, up to 'size' bytes).
 * Returns 0 if found, KEYCHAIN_NOT_FOUND if no entry exists. */
int keychain_find(const char *service, const char *account,
                  char *out, size_t size);

/* Delete a credential. Returns 0 if deleted, KEYCHAIN_NOT_FOUND if absent. */
int keychain_delete(const char *service, const char *account);

/* Invalidate cached password for an account. Call after password change/clear. */
void keychain_cache_invalidate(const char *service, const char *account);

/* Invalidate all cached passwords. */
void keychain_cache_clear(void);

/* Error codes */
#define KEYCHAIN_OK          0
#define KEYCHAIN_NOT_FOUND   1
#define KEYCHAIN_ERR         2

#endif /* KEYCHAIN_H */
