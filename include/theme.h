/* gEudora Theme Engine
 * Provides 5 themes: Light, Dark, Nord, Solarized, Monokai
 * All panels use CSS variables via theme classes on the root window. */

#ifndef GEUDORA_THEME_H
#define GEUDORA_THEME_H

#include <gtk/gtk.h>

typedef enum {
  THEME_LIGHT = 0,
  THEME_DARK,
  THEME_NORD,
  THEME_SOLARIZED,
  THEME_MONOKAI,
  THEME_COUNT
} GeudoraTheme;

/* Initialize the theme system — call once at startup */
void theme_init(GtkWidget *root_window);

/* Apply a theme by ID */
void theme_apply(GeudoraTheme theme);

/* Get current theme */
GeudoraTheme theme_get_current(void);

/* Get theme display name */
const char *theme_get_name(GeudoraTheme theme);

/* Cycle to next theme */
void theme_cycle(void);

/* Get the root window (for external callers) */
GtkWidget *theme_get_root(void);

/* Apply current theme colors to a gEditCtrl widget */
void theme_apply_to_editor(GtkWidget *editor_ctrl);

/* Create a themed GtkHeaderBar and set it as the window's titlebar.
 * Returns the headerbar widget (e.g. to add buttons to it). */
GtkWidget *theme_setup_headerbar(GtkWidget *window, const char *title);

/* Update the headerbar title (also sets gtk_window_set_title for accessibility) */
void theme_update_headerbar_title(GtkWidget *window, const char *title);

#endif /* GEUDORA_THEME_H */
