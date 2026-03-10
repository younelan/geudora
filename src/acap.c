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
OSErr ACAPLoad(bool giveQuit) {
    // TODO: Implement ACAP settings loading
    // For now, just return success and use local settings
    return noErr;
}

/************************************************************************
 * GetACAPLogin - get ACAP login credentials (STUB)
 ************************************************************************/
OSErr GetACAPLogin(PStr server, PStr user, PStr password, bool giveQuit) {
    // TODO: Implement ACAP login dialog
    // For now, return error to indicate ACAP is not available
    return -1;
}

/************************************************************************
 * ACAPLogin - login to ACAP server (STUB)
 ************************************************************************/
OSErr ACAPLogin(PStr server, PStr user, PStr password, ACAPStateHandle state) {
    // TODO: Implement ACAP server connection and authentication
    // For now, return error to indicate ACAP is not available
    return -1;
}

/************************************************************************
 * GetPOPInfo - get POP server info (STUB)
 ************************************************************************/
void GetPOPInfo(void *user, void *password) {
    // TODO: Implement POP info retrieval
    // For now, do nothing
}

/************************************************************************
 * GetPOPPref - get POP preference string (STUB)
 ************************************************************************/
PStr GetPOPPref(PStr dest) {
    // TODO: Implement POP preference retrieval
    // For now, return empty string
    if (dest) {
        dest[0] = 0;
    }
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
