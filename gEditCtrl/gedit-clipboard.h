#ifndef gedit_CLIPBOARD_H
#define gedit_CLIPBOARD_H

#include "gedit-document.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Copy selected text with formatting to clipboard */
G_GNUC_INTERNAL void gedit_clipboard_copy(GtkWidget *area, gint sel_start,
                                          gint sel_end, geditDocument *doc);

/* Cut selected text with formatting to clipboard */
G_GNUC_INTERNAL void gedit_clipboard_cut(GtkWidget *area, gint sel_start,
                                         gint sel_end, geditDocument *doc);

/* Paste from clipboard at caret position */
G_GNUC_INTERNAL void gedit_clipboard_paste(GtkWidget *area, gint caret_pos,
                                           geditDocument *doc);

/* Intelligent cut: if no selection, cut the word at caret */
G_GNUC_INTERNAL void gedit_clipboard_intelligent_cut(GtkWidget *area, gint caret,
                                                     geditDocument *doc);

/* Intelligent copy: if no selection, copy the word at caret */
G_GNUC_INTERNAL void gedit_clipboard_intelligent_copy(GtkWidget *area, gint caret,
                                                      geditDocument *doc);

/* Paste as plain text (strips formatting) */
G_GNUC_INTERNAL void gedit_clipboard_paste_plain(GtkWidget *area, gint caret_pos,
                                                 geditDocument *doc);

G_END_DECLS

#endif /* gedit_CLIPBOARD_H */
