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

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * functions of which I am not proud
 **********************************************************************/

#ifndef SHAME_H
#define SHAME_H

#include <stdbool.h>
#include "mailbox.h"

/* Aprintf: formatted alert via resource string IDs */
void Aprintf(short templ, short which, short rFormat, ...);

/* MemoryPreflight: check if enough memory is available */
OSErr MemoryPreflight(long size);

/* MyHandToHand: duplicate a Handle in place */
OSErr MyHandToHand(Handle *inHandle);

/* WarnUser: show a warning dialog by string resource ID + error code */
int WarnUser(short stringId, int err);

/* DeepTrouble: fatal error — log and exit */
void DeepTrouble(const char *str);

/* SetAlertBeep: control whether alerts produce a beep */
void SetAlertBeep(bool onOrOff);

/* ReallyStandardAlert / MyStandardAlert: GTK-ported alert helpers */
short ReallyStandardAlert(int alertType, const char *error, const char *explanation, void *alertParam);
OSErr MyStandardAlert(int inAlertType, const char *inError, const char *inExplanation, void *inAlertParam, short *outItemHit);

/* GoGetHelp: open help URL — stub for now */
OSErr GoGetHelp(const char *error, const char *explanation);

/* Switch: yield to other threads/events */
bool Switch(void);

#define RANDOM_FAILURE_PROC ;
#define RANDOM_FAILURE ;

#endif /* SHAME_H */
