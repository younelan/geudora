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

/* Portable ACAP header for builds that don't use Rez/Carbon types */

#ifndef ACAP_H
#define ACAP_H

#include "mydefs.h"
#include <stdbool.h>

/* Forward declarations for types used in ACAP */
typedef struct Personality *PersHandle;
typedef struct ACAPState *ACAPStateHandle;  // ACAP state handle
/* TransStream is defined in TransStream.h via mydefs.h */
struct TransStreamStruct;  // Forward declaration
typedef struct TransStreamStruct *TransStream;

/* Minimal portable declarations used by acap.c
  Project-wide `char *`/string types are defined centrally (see `mailbox.h`/`mydefs.h`).
  Do not redefine `char *` here to avoid conflicting typedefs. */

/* ACAP entry points */
int ACAPLoad(bool giveQuit);
/* Use portable C strings for textual parameters */
int GetACAPLogin(char *server, char *user, char *password, bool giveQuit);
int ACAPLogin(char *server, char *user, char *password, ACAPStateHandle state);

/* Stub declarations for functions not yet ported */
void GetPOPInfo(void *user, void *password);
/* GetPOPPref fills `dest` and returns it; use portable `char *` */
char *GetPOPPref(char *dest);
/* DisTrans is a macro defined in mydefs.h */
/* ReallyDoAnAlert is defined in legacy_shim.h */
void Cleanup(void);
void ExitToShell(void);

/* Alert/dialog constants */
#define Normal 0
#define ECANCELED -128

#endif /* ACAP_H */
