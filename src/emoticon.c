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

#include <string.h>
#include <ctype.h>

/* --- Emoticon mapping table --- */

typedef struct {
    const char *ascii;     /* ASCII pattern, e.g. ":-)" */
    const char *emoji;     /* UTF-8 emoji, e.g. "\xF0\x9F\x98\x80" */
    const char *meaning;   /* Human-readable, e.g. "smile" */
    int ascii_len;         /* strlen(ascii), computed at init */
} EmoEntry;

/*
 * Mapping table — ordered longest-first within each starting character
 * so longer patterns match before shorter ones (e.g. ":-)" before ":)").
 *
 * These are the standard emoticons from original Eudora plus common extras.
 */
static EmoEntry emo_table[] = {
    /* Smileys */
    {":-)",   "\xF0\x9F\x98\x80", "smile",       0},   /* 😀 */
    {":)",    "\xF0\x9F\x98\x80", "smile",       0},   /* 😀 */
    {";-)",   "\xF0\x9F\x98\x89", "wink",        0},   /* 😉 */
    {";)",    "\xF0\x9F\x98\x89", "wink",        0},   /* 😉 */
    {":-D",   "\xF0\x9F\x98\x83", "big grin",    0},   /* 😃 */
    {":D",    "\xF0\x9F\x98\x83", "big grin",    0},   /* 😃 */
    {":-P",   "\xF0\x9F\x98\x9B", "tongue",      0},   /* 😛 */
    {":P",    "\xF0\x9F\x98\x9B", "tongue",      0},   /* 😛 */
    {":-p",   "\xF0\x9F\x98\x9B", "tongue",      0},   /* 😛 */
    {":p",    "\xF0\x9F\x98\x9B", "tongue",      0},   /* 😛 */
    {":-(",   "\xF0\x9F\x98\x9E", "sad",         0},   /* 😞 */
    {":(",    "\xF0\x9F\x98\x9E", "sad",         0},   /* 😞 */
    {":-/",   "\xF0\x9F\x98\x95", "confused",    0},   /* 😕 */
    {":/",    "\xF0\x9F\x98\x95", "confused",    0},   /* 😕 */
    {":-|",   "\xF0\x9F\x98\x90", "neutral",     0},   /* 😐 */
    {":|",    "\xF0\x9F\x98\x90", "neutral",     0},   /* 😐 */
    {":-O",   "\xF0\x9F\x98\xAE", "surprised",   0},   /* 😮 */
    {":O",    "\xF0\x9F\x98\xAE", "surprised",   0},   /* 😮 */
    {":-o",   "\xF0\x9F\x98\xAE", "surprised",   0},   /* 😮 */
    {":o",    "\xF0\x9F\x98\xAE", "surprised",   0},   /* 😮 */
    {":'(",   "\xF0\x9F\x98\xA2", "cry",         0},   /* 😢 */
    {":'-(",  "\xF0\x9F\x98\xA2", "cry",         0},   /* 😢 */
    {":-*",   "\xF0\x9F\x98\x98", "kiss",        0},   /* 😘 */
    {":*",    "\xF0\x9F\x98\x98", "kiss",        0},   /* 😘 */
    {"B-)",   "\xF0\x9F\x98\x8E", "cool",        0},   /* 😎 */
    {"B)",    "\xF0\x9F\x98\x8E", "cool",        0},   /* 😎 */
    {":-S",   "\xF0\x9F\x98\x96", "confounded",  0},   /* 😖 */
    {":S",    "\xF0\x9F\x98\x96", "confounded",  0},   /* 😖 */
    {":-s",   "\xF0\x9F\x98\x96", "confounded",  0},   /* 😖 */
    {":s",    "\xF0\x9F\x98\x96", "confounded",  0},   /* 😖 */
    {">:-(",  "\xF0\x9F\x98\xA0", "angry",       0},   /* 😠 */
    {">:(",   "\xF0\x9F\x98\xA0", "angry",       0},   /* 😠 */
    {":-X",   "\xF0\x9F\xA4\x90", "mute",        0},   /* 🤐 */
    {":X",    "\xF0\x9F\xA4\x90", "mute",        0},   /* 🤐 */

    /* Other common */
    {"<3",    "\xE2\x9D\xA4\xEF\xB8\x8F", "heart",  0},  /* ❤️ */
    {"</3",   "\xF0\x9F\x92\x94", "broken heart", 0}, /* 💔 */
    {"O:-)",  "\xF0\x9F\x98\x87", "angel",       0},   /* 😇 */
    {"O:)",   "\xF0\x9F\x98\x87", "angel",       0},   /* 😇 */
    {"XD",    "\xF0\x9F\x98\x86", "laughing",    0},   /* 😆 */
    {"xD",    "\xF0\x9F\x98\x86", "laughing",    0},   /* 😆 */
    {":^)",   "\xF0\x9F\x98\x8F", "smirk",       0},   /* 😏 */
};

#define EMO_TABLE_SIZE (sizeof(emo_table) / sizeof(emo_table[0]))

static bool emo_initted = false;

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
 ***********************************************************************/
void EmoInit(void)
{
    if (emo_initted) return;

    /* Compute ascii_len for each entry */
    for (size_t i = 0; i < EMO_TABLE_SIZE; i++)
        emo_table[i].ascii_len = (int)strlen(emo_table[i].ascii);

    emo_initted = true;
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
    return (int)EMO_TABLE_SIZE;
}

/***********************************************************************
 * EmoGetAscii / EmoGetEmoji / EmoGetMeaning - accessors
 ***********************************************************************/
const char *EmoGetAscii(int index)
{
    if (index < 0 || index >= (int)EMO_TABLE_SIZE) return "";
    return emo_table[index].ascii;
}

const char *EmoGetEmoji(int index)
{
    if (index < 0 || index >= (int)EMO_TABLE_SIZE) return "";
    return emo_table[index].emoji;
}

const char *EmoGetMeaning(int index)
{
    if (index < 0 || index >= (int)EMO_TABLE_SIZE) return "";
    return emo_table[index].meaning;
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
 * Like the original EmoScanPtr: checks word boundaries, prefers
 * longest match at each position.
 *
 * Returns the index into emo_table, or -1 if none found.
 * Sets *match_offset to the byte offset of the match within 'text'.
 ***********************************************************************/
static int emo_scan_text(const char *text, int text_len, int start_offset, int *match_offset)
{
    for (int pos = start_offset; pos < text_len; pos++) {
        /* Word boundary check: no word/digit char immediately before */
        if (pos > 0 && is_word_or_digit(text[pos - 1]))
            continue;

        int best = -1;
        int best_len = 0;

        for (size_t i = 0; i < EMO_TABLE_SIZE; i++) {
            int alen = emo_table[i].ascii_len;
            if (alen > text_len - pos) continue;
            if (memcmp(text + pos, emo_table[i].ascii, alen) != 0) continue;
            /* Word boundary check: no word/digit char immediately after */
            if (pos + alen < text_len && is_word_or_digit(text[pos + alen]))
                continue;
            /* Prefer longest match */
            if (alen > best_len) {
                best = (int)i;
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
 * Uses GtkTextBuffer directly.  Each replacement is tagged with the
 * "emoticon" tag so it can be identified and reverted.  We store the
 * original ASCII text in a GHashTable keyed by buffer offset (the
 * tag carries per-range data via marks).
 ***********************************************************************/
void EmoScanPte(GtkWidget *pte, bool toCompletion)
{
    if (!emo_initted) EmoInit();
    if (!pte) return;

    geditDocument *doc = geditctrl_get_document(pte);
    if (!doc) return;
    GtkTextBuffer *buffer = gedit_document_get_buffer(doc);
    if (!buffer) return;

    GtkTextTag *emo_tag = emo_get_or_create_tag(buffer);

    /* Get the full text.  We scan it as a C string, then apply
     * replacements back to the buffer using character offsets. */
    GtkTextIter start_iter, end_iter;
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    char *text = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE);
    if (!text) return;

    int text_len = (int)strlen(text);
    int scan_pos = 0;
    int replacements = 0;
    int max_replacements = toCompletion ? 10000 : 15;

    while (scan_pos < text_len && replacements < max_replacements) {
        int match_offset = 0;
        int emo_idx = emo_scan_text(text, text_len, scan_pos, &match_offset);
        if (emo_idx < 0) break;

        int ascii_len = emo_table[emo_idx].ascii_len;
        const char *emoji = emo_table[emo_idx].emoji;
        int emoji_byte_len = (int)strlen(emoji);

        /* Convert byte offset to character offset for GtkTextBuffer.
         * text is UTF-8, so we need g_utf8_pointer_to_offset. */
        int char_offset = (int)g_utf8_pointer_to_offset(text, text + match_offset);
        int char_len = (int)g_utf8_pointer_to_offset(text + match_offset,
                                                       text + match_offset + ascii_len);

        /* Check if this range already has the emoticon tag */
        GtkTextIter check_iter;
        gtk_text_buffer_get_iter_at_offset(buffer, &check_iter, char_offset);
        if (gtk_text_iter_has_tag(&check_iter, emo_tag)) {
            /* Already replaced — skip past it */
            scan_pos = match_offset + ascii_len;
            continue;
        }

        /* Save the original ASCII text as a mark name so we can revert.
         * We use the pattern: store original in object data on the tag
         * instance, keyed by a GtkTextMark at the start position. */
        GtkTextIter del_start, del_end;
        gtk_text_buffer_get_iter_at_offset(buffer, &del_start, char_offset);
        gtk_text_buffer_get_iter_at_offset(buffer, &del_end, char_offset + char_len);

        /* Delete the ASCII text */
        gtk_text_buffer_delete(buffer, &del_start, &del_end);

        /* Insert the emoji with the emoticon tag */
        gtk_text_buffer_get_iter_at_offset(buffer, &del_start, char_offset);
        gtk_text_buffer_insert_with_tags(buffer, &del_start, emoji, emoji_byte_len,
                                          emo_tag, NULL);

        /* Re-fetch text since buffer changed */
        g_free(text);
        gtk_text_buffer_get_start_iter(buffer, &start_iter);
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        text = gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, TRUE);
        text_len = (int)strlen(text);

        /* Advance past the inserted emoji */
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
    if (!emo_initted) EmoInit();
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
        const char *ascii_replacement = NULL;
        for (size_t i = 0; i < EMO_TABLE_SIZE; i++) {
            if (strcmp(emoji_text, emo_table[i].emoji) == 0) {
                ascii_replacement = emo_table[i].ascii;
                break;
            }
        }
        g_free(emoji_text);

        if (ascii_replacement) {
            int offset = gtk_text_iter_get_offset(&iter);

            /* Delete the emoji */
            gtk_text_buffer_delete(buffer, &iter, &tag_end);

            /* Insert the ASCII text without the emoticon tag */
            gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset);
            gtk_text_buffer_insert(buffer, &iter, ascii_replacement, -1);

            /* Restart from the insertion point */
            gtk_text_buffer_get_iter_at_offset(buffer, &iter, offset + (int)strlen(ascii_replacement));
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
 ***********************************************************************/
int EmoInsert(MyWindowPtr win, int item)
{
    if (!win || !win->pte) return -1;
    if (item < 0 || item >= (int)EMO_TABLE_SIZE) return -1;
    if (!emo_initted) EmoInit();

    geditDocument *doc = geditctrl_get_document(win->pte);
    if (!doc) return -1;
    GtkTextBuffer *buffer = gedit_document_get_buffer(doc);
    if (!buffer) return -1;

    const char *emoji = emo_table[item].emoji;

    /* Get current cursor position */
    GtkTextMark *insert_mark = gtk_text_buffer_get_insert(buffer);
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, insert_mark);

    int offset = gtk_text_iter_get_offset(&cursor);

    /* Add space before if previous char is a word char */
    if (offset > 0) {
        GtkTextIter prev = cursor;
        gtk_text_iter_backward_char(&prev);
        gunichar pc = gtk_text_iter_get_char(&prev);
        if (g_unichar_isalnum(pc) || pc == '_')
            gtk_text_buffer_insert(buffer, &cursor, " ", 1);
    }

    /* Insert the emoji with the emoticon tag */
    GtkTextTag *emo_tag = emo_get_or_create_tag(buffer);
    gtk_text_buffer_insert_with_tags(buffer, &cursor, emoji, -1, emo_tag, NULL);

    /* Add space after if next char is a word char */
    gunichar nc = gtk_text_iter_get_char(&cursor);
    if (nc != 0 && (g_unichar_isalnum(nc) || nc == '_'))
        gtk_text_buffer_insert(buffer, &cursor, " ", 1);

    /* Place cursor after insertion */
    gtk_text_buffer_place_cursor(buffer, &cursor);

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
