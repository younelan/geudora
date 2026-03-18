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

#ifndef EMOTICON_H
#define EMOTICON_H

#include <gtk/gtk.h>
#include <stdbool.h>

/* Forward declarations */
struct MyWindow;
/* MyWindowPtr defined in mailbox.h */

/*
 * EmoInit - initialize the emoticon system.
 * Builds the ASCII-to-Unicode mapping table.
 */
void EmoInit(void);

/*
 * EmoScan - scan all open windows for ASCII emoticons and replace
 * with Unicode emoji.  Called from idle processing.
 */
void EmoScan(void);

/*
 * EmoScanPte - scan a single gEditCtrl widget for emoticons.
 * If toCompletion is true, scan the entire buffer; otherwise scan
 * one line at a time (for incremental idle scanning).
 */
void EmoScanPte(GtkWidget *pte, bool toCompletion);

/*
 * EmoInsert - insert an emoticon at the current cursor position
 * in win->pte.  'item' is the 0-based index into the emoticon table.
 */
int EmoInsert(MyWindowPtr win, int item);

/*
 * EmoSearchAndDestroy - revert all Unicode emoji back to their
 * ASCII equivalents in all open windows.
 */
void EmoSearchAndDestroy(void);

/*
 * EmoSearchAndDestroyPte - revert emoji in a single gEditCtrl.
 */
void EmoSearchAndDestroyPte(GtkWidget *pte);

/*
 * EmoCount - return the number of known emoticons.
 */
int EmoCount(void);

/*
 * EmoGetAscii - return the ASCII pattern for emoticon at index.
 */
const char *EmoGetAscii(int index);

/*
 * EmoGetEmoji - return the UTF-8 emoji string for emoticon at index.
 */
const char *EmoGetEmoji(int index);

/*
 * EmoGetMeaning - return the human-readable meaning for emoticon at index.
 */
const char *EmoGetMeaning(int index);

/*
 * EmoDo - check if emoticon display is enabled in settings.
 */
bool EmoDo(void);

/*
 * IsEmoticonChar - check if the character at the given offset in a
 * GtkTextBuffer is an emoji that was inserted by the emoticon system.
 * Uses GtkTextTag "emoticon" to identify them.
 */
bool IsEmoticonChar(GtkTextBuffer *buffer, int offset);

#define kEmoScanFinished (-1)

#endif /* EMOTICON_H */
