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
 * functions of which I am not proud — ported to GTK4/GLib
 **********************************************************************/

/* Prevent legacy_shim.h from providing a conflicting static-inline Dprintf */
#define Dprintf Dprintf

#include "shame.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "Globals.h"
#include "fileutil.h"
#include "task_types.h"
#include "taskProgress.h"
#include "threading.h"

/* DlgFilterUPP: Mac modal-dialog filter; unused on GTK — defined here for
 * the few callers that still reference it via extern.                       */
void *DlgFilterUPP = NULL;

static bool AlertBeep = true;

/************************************************************************
 * Dprintf - debug log
 ************************************************************************/
void Dprintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	g_logv("Eudora", G_LOG_LEVEL_DEBUG, fmt, args);
	va_end(args);
}

/************************************************************************
 * Aprintf - formatted alert (resource-based on Mac; log on GTK)
 ************************************************************************/
void Aprintf(short templ, short which, short rFormat, ...)
{
	(void)templ; (void)which; (void)rFormat;
	g_warning("Aprintf: template=%d", templ);
}

/************************************************************************
 * ReallyDoAnAlert - show a dialog (Mac ALRT template; log on GTK)
 ************************************************************************/
int ReallyDoAnAlert(int templ, int which)
{
	(void)templ; (void)which;
	g_warning("ReallyDoAnAlert: template=%d", templ);

	if (InAThread()) {
		char buf[64];
		snprintf(buf, sizeof(buf), "Alert %d", templ);
		AddTaskErrorsS(buf, "", CheckingTask, CurPers->persId);
	}
	return 1;
}

/************************************************************************
 * ReallyStandardAlert - standard alert dialog
 ************************************************************************/
short ReallyStandardAlert(int alertType, const char *error, const char *explanation, void *alertParam)
{
	(void)alertType; (void)alertParam;
	if (error && *error)
		g_warning("Alert: %s", error);
	if (explanation && *explanation)
		g_warning("Alert: %s", explanation);

	if (InAThread()) {
		AddTaskErrorsS(error, explanation, CheckingTask, CurPers->persId);
	}

	ActiveTicks = (long)(g_get_monotonic_time() / 16667);
	return 1;
}

/************************************************************************
 * MyStandardAlert - wrapper around ReallyStandardAlert
 ************************************************************************/
OSErr MyStandardAlert(int inAlertType, const char *inError, const char *inExplanation,
                      void *inAlertParam, short *outItemHit)
{
	(void)inAlertParam;
	ReallyStandardAlert(inAlertType, inError, inExplanation, NULL);
	if (outItemHit)
		*outItemHit = 1;
	return 0;
}

/************************************************************************
 * WarnUser - show a warning for an error condition
 ************************************************************************/
int WarnUser(short stringId, int err)
{
	char msg[256];
	GetRString(msg, stringId);
	g_warning("WarnUser: [%d] '%s' err=%d", (int)stringId, msg, err);
	return err;
}

/************************************************************************
 * GoGetHelp - open help URL (stub)
 ************************************************************************/
OSErr GoGetHelp(const char *error, const char *explanation)
{
	(void)error; (void)explanation;
	return 0;
}

/************************************************************************
 * MemoryPreflight - on Linux malloc doesn't fail this way; always OK
 ************************************************************************/
OSErr MemoryPreflight(long size)
{
	(void)size;
	return 0;
}

/************************************************************************
 * DeepTrouble - fatal error, log and exit
 ************************************************************************/
void DeepTrouble(const char *str)
{
	g_critical("Fatal: %s", str ? str : "(null)");
	exit(1);
}

/************************************************************************
 * SetAlertBeep - control whether alerts beep
 ************************************************************************/
void SetAlertBeep(bool onOrOff)
{
	AlertBeep = onOrOff;
}

/************************************************************************
 * Switch - yield events / cooperative switch (no-op on GTK main loop)
 ************************************************************************/
bool Switch(void)
{
	while (g_main_context_iteration(NULL, FALSE))
		;
	return false;
}

/************************************************************************
 * MyHandToHand - replace *inHandle with a freshly allocated copy
 ************************************************************************/
OSErr MyHandToHand(Handle *inHandle)
{
	if (!inHandle || !*inHandle)
		return -108;
	long len = GetHandleSize(*inHandle);
	Handle result = NewHandle(len);
	if (!result)
		return -108;
	if (len)
		memcpy(*result, **inHandle, (size_t)len);
	*inHandle = result;
	return 0;
}
