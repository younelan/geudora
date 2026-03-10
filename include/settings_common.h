/*
 * settings_common.h — shared UI helpers for settings pages
 */
#ifndef SETTINGS_COMMON_H
#define SETTINGS_COMMON_H

#include "gtk_settings.h"
#include <gtk/gtk.h>
#include <pango/pango.h>

/* Create a form row: right-aligned dim label + expanding widget */
GtkWidget *form_row(const char *label_text, GtkWidget *widget);

/* Create a group frame with optional bold header */
GtkWidget *group_box(const char *title);

/* Append a child widget into a group_box */
void group_add(GtkWidget *frame, GtkWidget *child);

/* Create a page title with optional subtitle */
GtkWidget *page_title(const char *text, const char *subtitle);

/* Helper: create a checkbox with margins already set for group_add */
GtkWidget *group_check(const char *label, gboolean active);

/* Helper: create a spin row inside an hbox with label suffix */
GtkWidget *spin_row(const char *prefix, GtkWidget **spin_out,
                    double min, double max, double val, const char *suffix);

#endif
