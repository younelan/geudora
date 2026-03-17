/* Portable regex wrapper using GLib GRegex.
 * Replaces the original Mac/Henry Spencer regexp implementation.
 * Copyright (c) 2017, Computer History Museum - All rights reserved. */

#ifndef REGEXP_H
#define REGEXP_H

#include <glib.h>

/* Compiled regex object - replaces the old regexp struct + void **/
typedef struct {
    GRegex *re;         /* compiled GRegex */
    char *pattern;      /* original pattern string */
} PortableRegexp;

/* Compile a regex pattern. Returns allocated PortableRegexp* or NULL on error. */
static inline PortableRegexp *eudora_regcomp(const char *pattern) {
    GError *err = NULL;
    GRegex *re = g_regex_new(pattern, G_REGEX_CASELESS, 0, &err);
    if (!re) {
        if (err) g_error_free(err);
        return NULL;
    }
    PortableRegexp *pr = g_new0(PortableRegexp, 1);
    pr->re = re;
    pr->pattern = g_strdup(pattern);
    return pr;
}

/* Free a compiled regex. */
static inline void eudora_regfree(PortableRegexp *pr) {
    if (pr) {
        if (pr->re) g_regex_unref(pr->re);
        g_free(pr->pattern);
        g_free(pr);
    }
}

/* Search for regex match in text at given offset.
 * Returns byte offset of match (>= 0), or -1 if not found. */
static inline long eudora_regex_search(PortableRegexp *pr, const char *text,
                                        long offset, long len) {
    if (!pr || !pr->re || !text || offset < 0 || offset >= len) return -1;
    GMatchInfo *match_info = NULL;
    gboolean found = g_regex_match_full(pr->re, text + offset, len - offset,
                                         0, 0, &match_info, NULL);
    long result = -1;
    if (found) {
        int start_pos;
        g_match_info_fetch_pos(match_info, 0, &start_pos, NULL);
        result = offset + start_pos;
    }
    if (match_info) g_match_info_free(match_info);
    return result;
}

/* Legacy name compatibility — old code used RegexpHandle as the compiled type */
typedef PortableRegexp *RegexpHandle;

#endif /* REGEXP_H */
