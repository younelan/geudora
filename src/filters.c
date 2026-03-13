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

#include "filters.h"
#include "filtrun.h"
#include "scripting.h"
#include <string.h>
#include <stdlib.h>

/************************************************************************
 * Filters — scripting CRUD operations
 *
 * Original Mac code used Apple Events to expose filter management to
 * external scripts. This file now provides platform-neutral CRUD
 * functions that any scripting backend (D-Bus, Apple Events, COM)
 * can call.
 *
 * Filter runtime logic (matching, actions) lives in filtrun.c.
 * Filter UI (editing dialog) will live in filtwin.c.
 ************************************************************************/
#define FILE_NUM 55

/*======================================================================
 * Internal helpers
 *====================================================================*/

/* Resolve a filter ID or 1-based index to a 0-based array index.
 * Returns -1 if not found. */
static int ResolveFilter(long idOrIndex, bool byId)
{
	int i;
	if (byId) {
		for (i = 0; i < gNFilters; i++)
			if (gFilterArray[i].fu.id == idOrIndex)
				return i;
		return -1;
	}
	/* 1-based index */
	if (idOrIndex < 1 || idOrIndex > gNFilters)
		return -1;
	return (int)(idOrIndex - 1);
}

/* Allocate a new unique filter ID by finding the max existing ID + 1 */
static long FilterNewId(void)
{
	long maxId = 0;
	int i;
	for (i = 0; i < gNFilters; i++)
		if (gFilterArray[i].fu.id > maxId)
			maxId = gFilterArray[i].fu.id;
	return maxId + 1;
}

/*======================================================================
 * ScriptCountFilters
 *====================================================================*/
int ScriptCountFilters(long *count)
{
	if (!count) return -50;  /* paramErr */
	if (RegenerateFilters()) return -1;
	*count = gNFilters;
	return 0;
}

/*======================================================================
 * ScriptFilterExists
 *====================================================================*/
bool ScriptFilterExists(long idOrIndex, bool byId)
{
	if (RegenerateFilters()) return false;
	return ResolveFilter(idOrIndex, byId) >= 0;
}

/*======================================================================
 * ScriptCreateFilter — insert a new empty filter at position
 *====================================================================*/
int ScriptCreateFilter(int position, long *outId)
{
	int err;
	FilterRecord newFR;

	if ((err = RegenerateFilters())) return err;

	/* Initialize a blank filter */
	memset(&newFR, 0, sizeof(newFR));
	newFR.fu.id = FilterNewId();
	newFR.incoming = true;  /* default: apply to incoming */

	/* Clamp position */
	if (position < 0 || position > gNFilters)
		position = gNFilters;

	/* Grow the array */
	FilterRecord *grown = (FilterRecord *)realloc(
		gFilterArray, (gNFilters + 1) * sizeof(FilterRecord));
	if (!grown) return -108;  /* memFullErr */
	gFilterArray = grown;

	/* Shift filters after insertion point */
	if (position < gNFilters)
		memmove(&gFilterArray[position + 1], &gFilterArray[position],
		        (gNFilters - position) * sizeof(FilterRecord));

	gFilterArray[position] = newFR;
	gNFilters++;

	if (outId) *outId = newFR.fu.id;

	err = SaveFilters();
	return err;
}

/*======================================================================
 * ScriptDeleteFilter
 *====================================================================*/
int ScriptDeleteFilter(long idOrIndex, bool byId)
{
	int err, idx;

	if ((err = RegenerateFilters())) return err;

	idx = ResolveFilter(idOrIndex, byId);
	if (idx < 0) return -1723;  /* errAENoSuchObject */

	/* Shift remaining filters down */
	if (idx < gNFilters - 1)
		memmove(&gFilterArray[idx], &gFilterArray[idx + 1],
		        (gNFilters - idx - 1) * sizeof(FilterRecord));
	gNFilters--;

	/* Shrink (or free if empty) */
	if (gNFilters > 0) {
		FilterRecord *shrunk = (FilterRecord *)realloc(
			gFilterArray, gNFilters * sizeof(FilterRecord));
		if (shrunk) gFilterArray = shrunk;
	} else {
		free(gFilterArray);
		gFilterArray = NULL;
	}

	err = SaveFilters();
	return err;
}

/*======================================================================
 * ScriptGetFilterProperty
 *====================================================================*/
int ScriptGetFilterProperty(long idOrIndex, bool byId,
                            ScriptPropertyID prop, ScriptValue *out)
{
	int err, idx;
	FilterRecord *fr;

	if (!out) return -50;
	if ((err = RegenerateFilters())) return err;

	idx = ResolveFilter(idOrIndex, byId);
	if (idx < 0) return -1723;
	fr = &gFilterArray[idx];

	switch (prop) {
	case kScriptPropName:
		*out = ScriptString(fr->name);
		break;
	case kScriptPropId:
		*out = ScriptLong(fr->fu.id);
		break;
	case kScriptPropLastMatch:
		*out = ScriptLong((long)fr->fu.lastMatch);
		break;
	case kScriptPropIncoming:
		*out = ScriptBool(fr->incoming);
		break;
	case kScriptPropOutgoing:
		*out = ScriptBool(fr->outgoing);
		break;
	case kScriptPropManual:
		*out = ScriptBool(fr->manual);
		break;
	case kScriptPropConjunction:
		*out = ScriptLong((long)fr->conjunction);
		break;
	case kScriptPropTransferMailbox:
		*out = ScriptString(fr->transferSpec.name);
		break;
	case kScriptPropCopyInstead:
		*out = ScriptBool(fr->kill);  /* copyInstead mapped to kill flag */
		break;
	default:
		return -1723;
	}
	return 0;
}

/*======================================================================
 * ScriptSetFilterProperty
 *====================================================================*/
int ScriptSetFilterProperty(long idOrIndex, bool byId,
                            ScriptPropertyID prop, const ScriptValue *in)
{
	int err, idx;
	FilterRecord *fr;

	if (!in) return -50;
	if ((err = RegenerateFilters())) return err;

	idx = ResolveFilter(idOrIndex, byId);
	if (idx < 0) return -1723;
	fr = &gFilterArray[idx];

	switch (prop) {
	case kScriptPropName:
		if (in->type != kScriptValString) return -50;
		strncpy(fr->name, in->u.str, sizeof(fr->name) - 1);
		fr->name[sizeof(fr->name) - 1] = '\0';
		break;
	case kScriptPropId:
		return -1723;  /* read-only */
	case kScriptPropLastMatch:
		if (in->type != kScriptValLong) return -50;
		fr->fu.lastMatch = (uint32_t)in->u.num;
		FilterNoteMatch(idx, in->u.num);
		break;
	case kScriptPropIncoming:
		if (in->type != kScriptValBool) return -50;
		fr->incoming = in->u.flag;
		break;
	case kScriptPropOutgoing:
		if (in->type != kScriptValBool) return -50;
		fr->outgoing = in->u.flag;
		break;
	case kScriptPropManual:
		if (in->type != kScriptValBool) return -50;
		fr->manual = in->u.flag;
		break;
	case kScriptPropConjunction:
		if (in->type != kScriptValLong) return -50;
		fr->conjunction = (ConjunctionEnum)(in->u.num & 0xff);
		break;
	case kScriptPropTransferMailbox:
		if (in->type != kScriptValString) return -50;
		strncpy(fr->transferSpec.name, in->u.str,
		        sizeof(fr->transferSpec.name) - 1);
		fr->transferSpec.name[sizeof(fr->transferSpec.name) - 1] = '\0';
		break;
	case kScriptPropCopyInstead:
		if (in->type != kScriptValBool) return -50;
		fr->kill = in->u.flag;
		break;
	default:
		return -1723;
	}

	err = SaveFilters();
	return err;
}

/*======================================================================
 * ScriptGetTermProperty
 *====================================================================*/
int ScriptGetTermProperty(long idOrIndex, bool byId, int termIndex,
                          ScriptPropertyID prop, ScriptValue *out)
{
	int err, idx;
	FilterRecord *fr;
	MatchTerm *mt;

	if (!out || termIndex < 0 || termIndex > 1) return -50;
	if ((err = RegenerateFilters())) return err;

	idx = ResolveFilter(idOrIndex, byId);
	if (idx < 0) return -1723;
	fr = &gFilterArray[idx];
	mt = &fr->terms[termIndex];

	switch (prop) {
	case kScriptPropTermHeader:
		*out = ScriptString(mt->header);
		break;
	case kScriptPropTermVerb:
		*out = ScriptLong((long)mt->verb);
		break;
	case kScriptPropTermValue:
		*out = ScriptString(mt->value);
		break;
	default:
		return -1723;
	}
	return 0;
}

/*======================================================================
 * ScriptSetTermProperty
 *====================================================================*/
int ScriptSetTermProperty(long idOrIndex, bool byId, int termIndex,
                          ScriptPropertyID prop, const ScriptValue *in)
{
	int err, idx;
	FilterRecord *fr;
	MatchTerm *mt;

	if (!in || termIndex < 0 || termIndex > 1) return -50;
	if ((err = RegenerateFilters())) return err;

	idx = ResolveFilter(idOrIndex, byId);
	if (idx < 0) return -1723;
	fr = &gFilterArray[idx];
	mt = &fr->terms[termIndex];

	switch (prop) {
	case kScriptPropTermHeader:
		if (in->type != kScriptValString) return -50;
		strncpy(mt->header, in->u.str, sizeof(mt->header) - 1);
		mt->header[sizeof(mt->header) - 1] = '\0';
		break;
	case kScriptPropTermVerb:
		if (in->type != kScriptValLong) return -50;
		mt->verb = (MatchEnum)(in->u.num & 0xff);
		break;
	case kScriptPropTermValue:
		if (in->type != kScriptValString) return -50;
		strncpy(mt->value, in->u.str, sizeof(mt->value) - 1);
		mt->value[sizeof(mt->value) - 1] = '\0';
		break;
	default:
		return -1723;
	}

	err = SaveFilters();
	return err;
}
