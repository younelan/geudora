/* Copyright (c) 2017, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

#include "acap.h"
#include "gtk_prefs.h"
#include "schizo.h"
#include "Globals.h"
#include <string.h>
#include <stdio.h>

/************************************************************************
 * GetCurPersAccount - find the PrefsAccount for the current personality.
 * Returns true if found, fills acct. For dominant (persId=0), returns
 * account_0. For others, matches by name.
 ************************************************************************/
bool GetCurPersAccount(PrefsAccount *acct) {
  if (!acct) return false;

  /* Dominant personality uses global prefs, not account_N */
  if (!CurPers || CurPers->persId == 0 || CurPers == PersList)
    return false;

  /* Non-dominant: find matching account by name */
  PrefsAccount accounts[16];
  int n = prefs_load_accounts(accounts, 16);

  for (int i = 0; i < n; i++) {
    if (strcmp(accounts[i].name, CurPers->name) == 0) {
      *acct = accounts[i];
      return true;
    }
  }

  return false; /* not found — caller uses global prefs */
}

/************************************************************************
 * ACAP STUB IMPLEMENTATION
 * 
 * ACAP (Application Configuration Access Protocol) was used for 
 * centralized settings management. This is stubbed out for the 
 * portable GTK build as it requires extensive Mac-specific networking
 * and UI code.
 * 
 * TODO: Implement ACAP support using portable networking libraries
 ************************************************************************/

/************************************************************************
 * ACAPLoad - load initial settings with acap (STUB)
 ************************************************************************/
int ACAPLoad(bool giveQuit) {
    // TODO: Implement ACAP settings loading
    // For now, just return success and use local settings
    return 0;
}

/************************************************************************
 * GetACAPLogin - get ACAP login credentials (STUB)
 ************************************************************************/
int GetACAPLogin(char *server, char *user, char *password, bool giveQuit) {
    // TODO: Implement ACAP login dialog
    // For now, return error to indicate ACAP is not available
    return -1;
}

/************************************************************************
 * ACAPLogin - login to ACAP server (STUB)
 ************************************************************************/
int ACAPLogin(char *server, char *user, char *password, ACAPStateHandle state) {
    // TODO: Implement ACAP server connection and authentication
    // For now, return error to indicate ACAP is not available
    return -1;
}

/************************************************************************
 * GetPOPInfo - get POP username and server from INI settings
 * Populates C strings: user = pop_username, host = pop_server
 ************************************************************************/
void GetPOPInfo(void *user, void *host) {
    char *u = (char *)user;
    char *h = (char *)host;

    /* Try per-personality account first */
    PrefsAccount acct;
    if (GetCurPersAccount(&acct) && acct.server[0]) {
      if (u) { strncpy(u, acct.username, 255); u[255] = '\0'; }
      if (h) { strncpy(h, acct.server, 255); h[255] = '\0'; }
      return;
    }

    /* Fallback to global prefs */
    gchar *username = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_username", "");
    gchar *server = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_server", "");

    if (u) {
        strncpy(u, username, 255);
        u[255] = '\0';
    }
    if (h) {
        strncpy(h, server, 255);
        h[255] = '\0';
    }

    g_free(username);
    g_free(server);
}

/************************************************************************
 * GetPOPPref - get POP account string "username@server" as C string
 * Used by mail engine to identify the POP account.
 * Returns dest, which is non-empty if account is configured.
 ************************************************************************/
char *GetPOPPref(char *dest) {
    if (!dest) return dest;

    /* Try per-personality account */
    PrefsAccount acct;
    if (GetCurPersAccount(&acct) && acct.username[0] && acct.server[0]) {
      snprintf(dest, 256, "%s@%s", acct.username, acct.server);
      return dest;
    }

    /* Fallback to global */
    gchar *username = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_username", "");
    gchar *server = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_server", "");

    if (username[0] && server[0]) {
        snprintf(dest, 256, "%s@%s", username, server);
    } else {
        dest[0] = 0;
    }

    g_free(username);
    g_free(server);
    return dest;
}

/************************************************************************
 * Cleanup - cleanup before exit (STUB)
 ************************************************************************/
/* Cleanup is implemented in ends.c */

/************************************************************************
 * ExitToShell - exit application (STUB)
 ************************************************************************/
void ExitToShell(void) {
    // TODO: Implement proper application exit
    exit(0);
}
