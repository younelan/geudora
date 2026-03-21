/*
 * GTK4 Mailbox Management for gEudora
 * Provides mailbox tree view and mailbox operations.
 * Backed by macmbx library for storage and TOC management.
 */

#ifndef GTK_MAILBOX_H
#define GTK_MAILBOX_H

#include <gtk/gtk.h>
#include "macmbx.h"

/* Mailbox tree operations — backed by MacmbxStore */
GtkWidget* gtk_mailbox_tree_new(void);
void gtk_mailbox_tree_load(GtkWidget *tree, MacmbxStore *store);
void gtk_mailbox_tree_refresh(GtkWidget *tree);

/* Mailbox file operations via MacmbxStore */
void gtk_mailbox_ensure_defaults(MacmbxStore *store);

/* Mailbox queries */
gchar* gtk_mailbox_get_path(const gchar *name);

/* Global store accessor (set by main_eudora.c at startup) */
void        gtk_mailbox_set_store(MacmbxStore *store);
MacmbxStore *gtk_mailbox_get_store(void);

#endif /* GTK_MAILBOX_H */
