/*
 * Compose Window for gEudora
 * Creates a new message composition window with gEditCtrl editor
 */

#ifndef COMPOSE_WINDOW_H
#define COMPOSE_WINDOW_H

#include <gtk/gtk.h>

/* Create a new compose window for message composition */
GtkWidget* create_compose_window(GtkWindow *parent);

/* Get the editor control from a compose window */
GtkWidget* compose_window_get_editor(GtkWidget *compose_window);

/* Get the message text from a compose window */
gchar* compose_window_get_text(GtkWidget *compose_window);

/* Set the message text in a compose window */
void compose_window_set_text(GtkWidget *compose_window, const gchar *text);

/* Get the To field from a compose window */
gchar* compose_window_get_to(GtkWidget *compose_window);

/* Set the To field in a compose window */
void compose_window_set_to(GtkWidget *compose_window, const gchar *to);

/* Get the Subject field from a compose window */
gchar* compose_window_get_subject(GtkWidget *compose_window);

/* Set the Subject field in a compose window */
void compose_window_set_subject(GtkWidget *compose_window, const gchar *subject);

/* Get the Cc field from a compose window */
gchar* compose_window_get_cc(GtkWidget *compose_window);

/* Set the Cc field in a compose window */
void compose_window_set_cc(GtkWidget *compose_window, const gchar *cc);

/* Get the Bcc field from a compose window */
gchar* compose_window_get_bcc(GtkWidget *compose_window);

/* Set the Bcc field in a compose window */
void compose_window_set_bcc(GtkWidget *compose_window, const gchar *bcc);

#endif /* COMPOSE_WINDOW_H */
