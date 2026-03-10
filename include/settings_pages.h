/*
 * settings_pages.h — declarations for all settings section page builders
 * Each returns a GtkWidget* page, and stores widget pointers in GObject data
 * on the returned widget for sync_widgets_to_settings to retrieve.
 */
#ifndef SETTINGS_PAGES_H
#define SETTINGS_PAGES_H

#include "gtk_settings.h"
#include <gtk/gtk.h>

GtkWidget *create_getting_started_page(AppSettings *s);
GtkWidget *create_checking_mail_page(AppSettings *s);
GtkWidget *create_sending_mail_page(AppSettings *s);
GtkWidget *create_composing_page(AppSettings *s);
GtkWidget *create_mailbox_display_page(AppSettings *s);
GtkWidget *create_date_display_page(AppSettings *s);
GtkWidget *create_styled_text_page(AppSettings *s);
GtkWidget *create_fonts_page(AppSettings *s);
GtkWidget *create_labels_page(AppSettings *s);
GtkWidget *create_attachments_page(AppSettings *s);
GtkWidget *create_replying_page(AppSettings *s);
GtkWidget *create_junk_mail_page(AppSettings *s);
GtkWidget *create_toolbar_page(AppSettings *s);
GtkWidget *create_getting_attention_page(AppSettings *s);
GtkWidget *create_hosts_page(AppSettings *s);
GtkWidget *create_moving_around_page(AppSettings *s);
GtkWidget *create_extra_warnings_page(AppSettings *s);
GtkWidget *create_miscellaneous_page(AppSettings *s);
GtkWidget *create_accounts_page(AppSettings *s, GtkWindow *parent);
GtkWidget *create_ssl_page(AppSettings *s);
GtkWidget *create_spell_checking_page(AppSettings *s);

/* Sync all widget values from a page back to settings */
void sync_page_to_settings(GtkWidget *page, AppSettings *s, SettingsSection section);

#endif
