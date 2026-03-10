/*
 * GTK4 Mailbox Management for gEudora
 * Provides mailbox tree view and mailbox operations
 */

#ifndef GTK_MAILBOX_H
#define GTK_MAILBOX_H

#include <gtk/gtk.h>

/* Mailbox structure */
typedef struct {
    gchar *name;           /* Mailbox name */
    gchar *path;           /* Full path to mailbox directory */
    gchar *toc_path;       /* Path to TOC (table of contents) file */
    gboolean is_folder;    /* TRUE if this is a folder containing mailboxes */
    gboolean is_special;   /* TRUE if this is a special mailbox (In, Out, Trash, Junk) */
    int message_count;     /* Number of messages in this mailbox */
    int unread_count;      /* Number of unread messages */
} GtkMailbox;

/* Mailbox tree operations */
GtkWidget* gtk_mailbox_tree_new(void);
void gtk_mailbox_tree_load(GtkWidget *tree);
void gtk_mailbox_tree_refresh(GtkWidget *tree);

/* Mailbox file operations */
void gtk_mailbox_create_default(void);
gboolean gtk_mailbox_create(const gchar *name, gboolean is_folder);
gboolean gtk_mailbox_delete(const gchar *path);
gboolean gtk_mailbox_rename(const gchar *old_path, const gchar *new_name);

/* Mailbox queries */
GtkMailbox* gtk_mailbox_get_by_name(const gchar *name);
GtkMailbox* gtk_mailbox_get_by_path(const gchar *path);
gchar* gtk_mailbox_get_path(const gchar *name);
gchar* gtk_mailbox_get_toc_path(const gchar *name);

/* Message operations */
int gtk_mailbox_get_message_count(const gchar *mailbox_path);
int gtk_mailbox_get_unread_count(const gchar *mailbox_path);
void gtk_mailbox_add_message(const gchar *mailbox_path, const gchar *message_data);
void gtk_mailbox_delete_message(const gchar *mailbox_path, int message_id);
void gtk_mailbox_transfer_message(const gchar *from_path, const gchar *to_path, int message_id);

/* Cleanup */
void gtk_mailbox_free(GtkMailbox *mailbox);

#endif /* GTK_MAILBOX_H */
