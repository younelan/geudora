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
	if (!count) return -50;  /* EINVAL */
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
	if (!grown) return -108;  /* ENOMEM */
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
		*out = ScriptString(spec_name(fr));
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
		*out = ScriptString(spec_name(fr->transferSpec));
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
		g_strlcpy(fr->transferSpec, in->u.str, sizeof(fr->transferSpec));
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

/*======================================================================
 * Personality scripting — platform-neutral CRUD
 *
 * Original Mac code in schizo.c exposed personality management via
 * Apple Events (SetPersProperty, GetPersProperty, AECreatePersonality).
 * These functions provide the same operations in a platform-neutral way
 * that any scripting backend (D-Bus, AE, COM) can call.
 *====================================================================*/

#include "schizo.h"
#include "threading.h"

/*======================================================================
 * ScriptCountPersonalities
 *====================================================================*/
int ScriptCountPersonalities(long *count)
{
	if (!count) return -50;
	*count = PersCount();
	return 0;
}

/*======================================================================
 * ScriptGetPersonalityProperty
 *====================================================================*/
int ScriptGetPersonalityProperty(long index, ScriptPropertyID prop,
                                  ScriptValue *out)
{
	PersHandle pers;

	if (!out) return -50;
	pers = Index2Pers((short)index);
	if (!pers) return -1723;

	switch (prop) {
	case kScriptPropName:
		*out = ScriptString(spec_name(pers));
		break;
	case kScriptPropId:
		*out = ScriptLong((long)pers->persId);
		break;
	default:
		return -1723;
	}
	return 0;
}

/*======================================================================
 * ScriptSetPersonalityProperty
 *====================================================================*/
int ScriptSetPersonalityProperty(long index, ScriptPropertyID prop,
                                  const ScriptValue *in)
{
	PersHandle pers;

	if (!in) return -50;
	pers = Index2Pers((short)index);
	if (!pers) return -1723;

	switch (prop) {
	case kScriptPropName:
		if (in->type != kScriptValString) return -50;
		/* Cannot rename the dominant personality */
		if (pers == PersList) return -1723;
		PersSetName(pers, (unsigned char *)in->u.str);
		break;
	case kScriptPropId:
		return -1723;  /* read-only */
	default:
		return -1723;
	}
	return 0;
}

/*======================================================================
 * ScriptCreatePersonality
 *====================================================================*/
int ScriptCreatePersonality(long *outIndex)
{
	PersHandle pers = PersNew();
	if (!pers) return -108;

	if (outIndex)
		*outIndex = PersCount();  /* new personality is last */

	return 0;
}

/*======================================================================
 * ScriptDeletePersonality
 *====================================================================*/
int ScriptDeletePersonality(long index)
{
	PersHandle pers = Index2Pers((short)index);
	if (!pers) return -1723;
	if (pers == PersList) return -1723;  /* cannot delete dominant */

	return (int)PersDelete(pers);
}

/*======================================================================
 * Mail transfer scripting
 *====================================================================*/

#include "mailxfer.h"
#include "comp.h"
#include "compact.h"
#include "toc.h"
#include "message.h"
#include "uudecode.h"
#include "StringDefs.h"

/* Forward declarations for message operations (defined in message.c) */
extern MyWindowPtr DoReplyMessage(MyWindowPtr win, bool all, bool self,
                                   bool quote, bool doFcc, short withWhich,
                                   bool vis, bool station, bool caching);
extern MyWindowPtr DoForwardMessage(MyWindowPtr win, void *toWhom, bool turbo);
extern MyWindowPtr DoRedistributeMessage(MyWindowPtr win, void *toWhom,
                                          bool turbo, bool andDelete,
                                          bool showIt);

/*======================================================================
 * ScriptCheckMail — trigger a mail check and/or send from scripting
 *====================================================================*/
int ScriptCheckMail(bool check, bool send)
{
	return (int)XferMail(check, send, false, true, true, 0);
}

/* Use TOCByPath from toc.c for mailbox path → TOC lookup */

/*======================================================================
 * ScriptCountMessages — count messages in a mailbox
 *====================================================================*/
int ScriptCountMessages(const char *mailboxPath, long *count)
{
	if (!count) return -50;
	TOCType *tocH = TOCByPath(mailboxPath);
	if (!tocH) return -1;
	*count = tocH->count;
	return 0;
}

/*======================================================================
 * ScriptGetMessageProperty — get a message property by mailbox + index
 *====================================================================*/
int ScriptGetMessageProperty(const char *mailboxPath, long index,
                              ScriptPropertyID prop, ScriptValue *out)
{
	if (!out) return -50;
	TOCType *tocH = TOCByPath(mailboxPath);
	if (!tocH) return -1;
	if (index < 0 || index >= tocH->count) return -1723;

	MSumPtr sum = &tocH->sums[index];

	switch (prop) {
	case kScriptPropPriority:
		*out = ScriptLong(sum->priority);
		break;
	case kScriptPropStatus:
		*out = ScriptLong(sum->state);
		break;
	case kScriptPropSender:
		*out = ScriptString(sum->from);
		break;
	case kScriptPropDate:
		*out = ScriptLong((long)sum->seconds);
		break;
	case kScriptPropSubject:
		*out = ScriptString(sum->subj);
		break;
	case kScriptPropSize:
		*out = ScriptLong(sum->length);
		break;
	case kScriptPropIsOutgoing:
		*out = ScriptBool(tocH->which == OUT);
		break;
	case kScriptPropLabel:
		*out = ScriptLong(GetSumColor(tocH, (short)index));
		break;
	case kScriptPropWrap:
		*out = ScriptBool(0 != (sum->flags & FLAG_WRAP_OUT));
		break;
	case kScriptPropKeepCopy:
		*out = ScriptBool(0 != (sum->flags & FLAG_KEEP_COPY));
		break;
	case kScriptPropReturnReceipt:
		*out = ScriptBool(0 != (sum->flags & FLAG_RR));
		break;
	case kScriptPropName:
		*out = ScriptString(sum->subj); /* name = subject for messages */
		break;
	case kScriptPropId:
		*out = ScriptLong((long)sum->uidHash);
		break;
	default:
		return -1723;
	}
	return 0;
}

/*======================================================================
 * ScriptSetMessageProperty — set a message property
 *====================================================================*/
int ScriptSetMessageProperty(const char *mailboxPath, long index,
                              ScriptPropertyID prop, const ScriptValue *in)
{
	if (!in) return -50;
	TOCType *tocH = TOCByPath(mailboxPath);
	if (!tocH) return -1;
	if (index < 0 || index >= tocH->count) return -1723;

	MSumPtr sum = &tocH->sums[index];

	switch (prop) {
	case kScriptPropPriority:
		if (in->type != kScriptValLong) return -50;
		sum->priority = (unsigned char)in->u.num;
		TOCSetDirty(tocH, true);
		break;
	case kScriptPropStatus:
		if (in->type != kScriptValLong) return -50;
		sum->state = (StateEnum)in->u.num;
		TOCSetDirty(tocH, true);
		break;
	case kScriptPropSubject:
		if (in->type != kScriptValString) return -50;
		strncpy(sum->subj, in->u.str, sizeof(sum->subj) - 1);
		sum->subj[sizeof(sum->subj) - 1] = '\0';
		TOCSetDirty(tocH, true);
		break;
	case kScriptPropLabel:
		if (in->type != kScriptValLong) return -50;
		SetSumColor(tocH, (short)index, (short)in->u.num);
		break;
	case kScriptPropWrap:
		if (in->type != kScriptValBool) return -50;
		if (in->u.flag) sum->flags |= FLAG_WRAP_OUT;
		else sum->flags &= ~FLAG_WRAP_OUT;
		TOCSetDirty(tocH, true);
		break;
	case kScriptPropKeepCopy:
		if (in->type != kScriptValBool) return -50;
		if (in->u.flag) sum->flags |= FLAG_KEEP_COPY;
		else sum->flags &= ~FLAG_KEEP_COPY;
		TOCSetDirty(tocH, true);
		break;
	case kScriptPropReturnReceipt:
		if (in->type != kScriptValBool) return -50;
		if (in->u.flag) sum->flags |= FLAG_RR;
		else sum->flags &= ~FLAG_RR;
		TOCSetDirty(tocH, true);
		break;
	case kScriptPropId:
		return -1723; /* read-only */
	default:
		return -1723;
	}
	return 0;
}

/*======================================================================
 * ScriptCreateMessage — create a new outgoing message (compose)
 *====================================================================*/
int ScriptCreateMessage(long *outIndex)
{
	MyWindowPtr win = DoComposeNew(0);
	if (!win) return -108;

	MessHandle messH = Win2MessH(win);
	if (!messH) return -108;

	if (outIndex)
		*outIndex = messH->sumNum;

	return 0;
}

/*======================================================================
 * ScriptReplyMessage — reply to a message
 *====================================================================*/
int ScriptReplyMessage(const char *mailboxPath, long index,
                        bool replyAll, bool includeSelf, bool quoteText,
                        long *outIndex)
{
	TOCType *tocH = TOCByPath(mailboxPath);
	if (!tocH) return -1;
	if (index < 0 || index >= tocH->count) return -1723;

	MyWindowPtr win = GetAMessage(tocH, (short)index, NULL, NULL, false);
	if (!win) return -1;

	MessHandle messH = Win2MessH(win);
	if (!messH) return -1;

	MyWindowPtr replyWin = DoReplyMessage(win, replyAll, includeSelf,
	                                       quoteText, true, 0, true, true, true);
	if (!replyWin) return -1;

	MessHandle replyMessH = Win2MessH(replyWin);
	if (outIndex && replyMessH)
		*outIndex = replyMessH->sumNum;

	return 0;
}

/*======================================================================
 * ScriptForwardMessage — forward a message
 *====================================================================*/
int ScriptForwardMessage(const char *mailboxPath, long index,
                          long *outIndex)
{
	TOCType *tocH = TOCByPath(mailboxPath);
	if (!tocH) return -1;
	if (index < 0 || index >= tocH->count) return -1723;

	MyWindowPtr win = GetAMessage(tocH, (short)index, NULL, NULL, false);
	if (!win) return -1;

	MyWindowPtr fwdWin = DoForwardMessage(win, NULL, true);
	if (!fwdWin) return -1;

	MessHandle fwdMessH = Win2MessH(fwdWin);
	if (outIndex && fwdMessH)
		*outIndex = fwdMessH->sumNum;

	return 0;
}

/*======================================================================
 * ScriptRedirectMessage — redirect a message
 *====================================================================*/
int ScriptRedirectMessage(const char *mailboxPath, long index,
                           long *outIndex)
{
	TOCType *tocH = TOCByPath(mailboxPath);
	if (!tocH) return -1;
	if (index < 0 || index >= tocH->count) return -1723;

	MyWindowPtr win = GetAMessage(tocH, (short)index, NULL, NULL, false);
	if (!win) return -1;

	MyWindowPtr redirWin = DoRedistributeMessage(win, NULL, false, false, true);
	if (!redirWin) return -1;

	MessHandle redirMessH = Win2MessH(redirWin);
	if (outIndex && redirMessH)
		*outIndex = redirMessH->sumNum;

	return 0;
}

/*======================================================================
 * ScriptQueueMessage — queue a message for sending
 *====================================================================*/
int ScriptQueueMessage(long index)
{
	/* Messages must be in the Out mailbox to be queued */
	TOCType *tocH = GetRealOutTOC();
	if (!tocH) return -1;
	if (index < 0 || index >= tocH->count) return -1723;

	return QueueMessage(tocH, (short)index, kEuSendNow, 0, true, false);
}

/*======================================================================
 * ScriptMoveMessage — move or copy a message between mailboxes
 *====================================================================*/
int ScriptMoveMessage(const char *fromMailbox, long index,
                       const char *toMailbox, bool copy)
{
	TOCType *fromTocH = TOCByPath(fromMailbox);
	if (!fromTocH) return -1;
	if (index < 0 || index >= fromTocH->count) return -1723;

	TOCType *toTocH = TOCByPath(toMailbox);
	if (!toTocH) return -1;

	return AppendMessage(fromTocH, (int)index, &toTocH, copy, false, false);
}
