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

/*
 * emoticon.c — Emoticon system ported to GTK4.
 *
 * Original Mac version used icon suites and PETE graphics to replace
 * ASCII smileys with inline images.  This GTK port uses Unicode emoji
 * characters instead — Pango renders them natively, no image files needed.
 *
 * Each replacement is tagged with a GtkTextTag ("emoticon") so we can
 * identify and revert them.  The original ASCII text is stored in the
 * tag's user data via g_object_set_data().
 */

#include "emoticon.h"
#include "geditctrl.h"
#include "gedit-document.h"
#include "message.h"
#include "gtk_settings.h"
#include "StringUtil.h"
#include "macmbx.h"

#include <string.h>
#include <ctype.h>

/* Emoji table now lives in macmbx; no private table needed. */

/* Access to global settings */
extern AppSettings *prefs_load(void);

/* Check if emoticons are enabled.
 * Original used GetPrefLong(PREF_BITE_ME_EMO) & emoPrefOffBit.
 * GTK port uses the display_emoticons boolean from AppSettings. */
static bool emo_enabled(void)
{
    /* We read the preference at init time and cache it.
     * For dynamic toggling, the settings dialog will call
     * EmoSearchAndDestroy() when turning off. */
    return true;  /* default on; overridden by caller checking EmoDo() */
}

/***********************************************************************
 * EmoInit - initialize the emoticon system
 * macmbx emoji table is static; nothing to do here.
 ***********************************************************************/
void EmoInit(void)
{
}

/***********************************************************************
 * EmoDo - check if emoticon display is enabled
 ***********************************************************************/
bool EmoDo(void)
{
    /* The display_emoticons setting is in AppSettings.
     * We can't easily access app_state.settings from here since it's
     * static in main_eudora.c.  Use GetPrefLong compatibility or
     * just return true as default — the UI toggle in settings will
     * call EmoSearchAndDestroy when turned off.
     * TODO: wire up to settings properly once accessor is available. */
    return true;
}

/***********************************************************************
 * EmoCount - return the number of emoticons in the table
 ***********************************************************************/
int EmoCount(void)
{
    int count = 0;
    macmbx_emoji_table(&count);
    return count;
}

/***********************************************************************
 * EmoGetAscii / EmoGetEmoji / EmoGetMeaning - accessors
 ***********************************************************************/
const char *EmoGetAscii(int index)
{
    int count = 0;
    const MacmbxEmoji *table = macmbx_emoji_table(&count);
    if (index < 0 || index >= count) return "";
    return table[index].ascii;
}

const char *EmoGetEmoji(int index)
{
    int count = 0;
    const MacmbxEmoji *table = macmbx_emoji_table(&count);
    if (index < 0 || index >= count) return "";
    return table[index].emoji;
}

const char *EmoGetMeaning(int index)
{
    int count = 0;
    const MacmbxEmoji *table = macmbx_emoji_table(&count);
    if (index < 0 || index >= count) return "";
    return table[index].name;
}

/***********************************************************************
 * is_word_or_digit - port of IsWordOrDigit for UTF-8 text
 * Only checks ASCII range since emoticon patterns are ASCII.
 ***********************************************************************/
static bool is_word_or_digit(char c)
{
    unsigned char uc = (unsigned char)c;
    if (uc >= 128) return true;  /* treat non-ASCII as word chars */
    return isalnum(uc) || uc == '_';
}

/***********************************************************************
 * emo_get_or_create_tag - get or create the "emoticon" tag for a buffer
 ***********************************************************************/
static GtkTextTag *emo_get_or_create_tag(GtkTextBuffer *buffer)
{
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, "emoticon");
    if (!tag) {
        tag = gtk_text_buffer_create_tag(buffer, "emoticon", NULL);
    }
    return tag;
}

/***********************************************************************
 * emo_scan_text - scan a C string for the first emoticon match.
 *
 * Uses macmbx_emoji_table() to iterate patterns with word-boundary
 * checks, preferring the longest match at each position.
 *
 * Returns the index into the macmbx emoji table, or -1 if none found.
 * Sets *match_offset to the byte offset of the match within 'text'.
 ***********************************************************************/
static int emo_scan_text(const char *text, int text_len, int start_offset, int *match_offset)
{
    int count = 0;
    const MacmbxEmoji *table = macmbx_emoji_table(&count);

    for (int pos = start_offset; pos < text_len; pos++) {
        /* Word boundary check: no word/digit char immediately before */
        if (pos > 0 && is_word_or_digit(text[pos - 1]))
            continue;

        int best = -1;
        int best_len = 0;

        for (int i = 0; i < count; i++) {
            int alen = (int)strlen(table[i].ascii);
            if (alen > text_len - pos) continue;
            if (memcmp(text + pos, table[i].ascii, alen) != 0) continue;
            /* Word boundary check: no word/digit char immediately after */
            if (pos + alen < text_len && is_word_or_digit(text[pos + alen]))
                continue;
            /* Prefer longest match */
            if (alen > best_len) {
                best = i;
                best_len = alen;
            }
        }

        if (best >= 0) {
            *match_offset = pos;
            return best;
        }
    }
    return -1;
}

/***********************************************************************
 * EmoScanPte - scan a single gEditCtrl for emoticons and replace
 * ASCII patterns with Unicode emoji.
 *
 * Uses gedit_document APIs for proper undo/state tracking.
 * Each replacement is tagged with the "emoticon" GtkTextTag so it
 * can be identified and reverted.
 ***********************************************************************/
void EmoScanPte(GtkWidget *pte, bool toCompletion)
{
    if (!pte) return;

    int tbl_count = 0;
    const MacmbxEmoji *tbl = macmbx_emoji_table(&tbl_count);

    geditDocument *doc = geditctrl_get_document(pte);
    if (!doc) return;
    GtkTextBuffer *buffer = gedit_document_get_buffer(doc);
    if (!buffer) return;

    GtkTextTag *emo_tag = emo_get_or_create_tag(buffer);

    char *text = gedit_document_get_text(doc);
    if (!text) return;

    int text_len = (int)strlen(text);
    int scan_pos = 0;
    int replacements = 0;
    int max_replacements = toCompletion ? 10000 : 15;

    while (scan_pos < text_len && replacements < max_replacements) {
        int match_offset = 0;
        int emo_idx = emo_scan_text(text, text_len, scan_pos, &match_offset);
        if (emo_idx < 0) break;

        int ascii_len = (int)strlen(tbl[emo_idx].ascii);
        const char *emoji = tbl[emo_idx].emoji;
        int emoji_byte_len = (int)strlen(emoji);

        /* ASCII emoticon patterns are pure ASCII, so byte offset == char offset */
        int char_offset = match_offset;

        /* Check if this range already has the emoticon tag */
        GtkTextIter check_iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &check_iter, char_offset);
        if (gtk_text_iter_has_tag(&check_iter, emo_tag)) {
            scan_pos = match_offset + ascii_len;
            continue;
        }

        /* Delete the ASCII text via document API */
        gedit_document_delete_range(doc, char_offset, ascii_len);

        /* Insert the emoji via document API */
        gedit_document_insert_text(doc, char_offset, emoji);
        int emoji_char_len = (int)g_utf8_strlen(emoji, -1);

        /* Apply the emoticon tag */
        GtkTextIter tag_start, tag_end;
        gtk_text_buffer_get_iter_at_offset(buffer, &tag_start, char_offset);
        gtk_text_buffer_get_iter_at_offset(buffer, &tag_end, char_offset + emoji_char_len);
        gtk_text_buffer_apply_tag(buffer, emo_tag, &tag_start, &tag_end);

        /* Re-fetch text since document changed */
        g_free(text);
        text = gedit_document_get_text(doc);
        if (!text) return;
        text_len = (int)strlen(text);

        /* Advance past the inserted emoji (in bytes) */
        scan_pos = match_offset + emoji_byte_len;
        replacements++;
    }

    g_free(text);
}

/***********************************************************************
 * EmoScan - scan all open windows for emoticons.
 * Called from idle processing.
 ***********************************************************************/
void EmoScan(void)
{
    /* In GTK port, individual editors call EmoScanPte directly
     * after text changes.  This global scan is a no-op placeholder
     * for now — the idle loop integration will be added when the
     * main event loop is wired up. */
}

/***********************************************************************
 * EmoSearchAndDestroyPte - revert all emoticon emoji back to ASCII
 * in a single gEditCtrl.
 *
 * Walks the buffer looking for text tagged with "emoticon", and
 * replaces the emoji characters with their original ASCII equivalents.
 * Uses gedit_document APIs for proper undo/state tracking.
 ***********************************************************************/
void EmoSearchAndDestroyPte(GtkWidget *pte)
{
    if (!pte) return;

    geditDocument *doc = geditctrl_get_document(pte);
    if (!doc) return;
    GtkTextBuffer *buffer = gedit_document_get_buffer(doc);
    if (!buffer) return;

    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *emo_tag = gtk_text_tag_table_lookup(table, "emoticon");
    if (!emo_tag) return;

    /* Iterate through the buffer finding tagged regions */
    GtkTextIter iter;
    gtk_text_buffer_get_start_iter(buffer, &iter);

    while (!gtk_text_iter_is_end(&iter)) {
        if (!gtk_text_iter_forward_to_tag_toggle(&iter, emo_tag))
            break;

        if (!gtk_text_iter_starts_tag(&iter, emo_tag))
            continue;

        GtkTextIter tag_end = iter;
        if (!gtk_text_iter_forward_to_tag_toggle(&tag_end, emo_tag))
            gtk_text_buffer_get_end_iter(buffer, &tag_end);

        /* Get the emoji text */
        char *emoji_text = gtk_text_buffer_get_text(buffer, &iter, &tag_end, TRUE);
        if (!emoji_text) continue;

        /* Find which emoticon this emoji corresponds to */
        const char *ascii_replacement = macmbx_emoji_reverse(emoji_text);
        g_free(emoji_text);

        if (ascii_replacement) {
            int offset = gtk_text_iter_get_offset(&iter);
            int emoji_char_len = gtk_text_iter_get_offset(&tag_end) - offset;

            /* Delete emoji and insert ASCII via document API */
            gedit_document_delete_range(doc, offset, emoji_char_len);
            gedit_document_insert_text(doc, offset, ascii_replacement);

            /* Restart scan from after the inserted ASCII */
            int ascii_len = (int)g_utf8_strlen(ascii_replacement, -1);
            gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset + ascii_len);
        }
    }
}

/***********************************************************************
 * EmoSearchAndDestroy - revert all emoticon emoji in all open windows.
 ***********************************************************************/
void EmoSearchAndDestroy(void)
{
    /* In GTK port, windows are managed differently.  This will be
     * called from the settings toggle.  Individual window cleanup
     * uses EmoSearchAndDestroyPte directly. */
}

/***********************************************************************
 * EmoInsert - insert an emoticon at the current cursor position.
 * 'item' is the 0-based index into the emoticon table.
 * Delegates to geditctrl_insert_emoji for proper state tracking.
 ***********************************************************************/
int EmoInsert(MyWindowPtr win, int item)
{
    if (!win || !win->pte) return -1;
    int count = 0;
    const MacmbxEmoji *table = macmbx_emoji_table(&count);
    if (item < 0 || item >= count) return -1;

    geditctrl_insert_emoji(win->pte, table[item].emoji);
    return 0;
}

/***********************************************************************
 * IsEmoticonChar - check if the character at offset has the emoticon tag
 ***********************************************************************/
bool IsEmoticonChar(GtkTextBuffer *buffer, int offset)
{
    if (!buffer) return false;
    GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(table, "emoticon");
    if (!tag) return false;

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset);
    return gtk_text_iter_has_tag(&iter, tag) ? true : false;
}
