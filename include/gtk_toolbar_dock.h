/*
 * GTK4 Dockable Toolbar System for gEudora
 *
 * Port of Eudora's floating/dockable toolbar (toolbar.c).
 * Supports docked mode (in main window) and floating mode
 * (separate window), matching the original PREF_TB_FLOATING behavior.
 */

#ifndef GTK_TOOLBAR_DOCK_H
#define GTK_TOOLBAR_DOCK_H

#include <gtk/gtk.h>

/* Toolbar dock position */
typedef enum {
  TOOLBAR_DOCK_TOP,
  TOOLBAR_DOCK_BOTTOM,
  TOOLBAR_DOCK_LEFT,
  TOOLBAR_DOCK_RIGHT,
  TOOLBAR_DOCK_FLOATING
} ToolbarDockPosition;

/* Dockable toolbar structure */
typedef struct {
  char *name;
  GtkWidget *toolbar;       /* The HBox/VBox containing buttons */
  GtkWidget *container;     /* Outer container (holds toolbar) */
  GtkWidget *float_window;  /* Floating window (NULL when docked) */
  GtkWidget *dock_parent;   /* Saved parent for re-docking */
  GtkWidget *context_menu;  /* Right-click popover menu */
  ToolbarDockPosition position;
  gboolean visible;
  gboolean floating;
} DockableToolbar;

/* Create a dockable toolbar */
DockableToolbar *create_dockable_toolbar(const char *name);

/* Add button to toolbar */
void toolbar_add_button(DockableToolbar *toolbar, GtkWidget *button);

/* Add separator to toolbar */
void toolbar_add_separator(DockableToolbar *toolbar);

/* Set toolbar position */
void set_toolbar_position(DockableToolbar *toolbar,
                          ToolbarDockPosition position);

/* Toggle toolbar visibility */
void toggle_toolbar_visibility(DockableToolbar *toolbar);

/* Get toolbar widget (for embedding in main window) */
GtkWidget *get_toolbar_widget(DockableToolbar *toolbar);

/* Check if toolbar is currently floating */
gboolean is_toolbar_floating(DockableToolbar *toolbar);

/* Free dockable toolbar */
void free_dockable_toolbar(DockableToolbar *toolbar);

#endif /* GTK_TOOLBAR_DOCK_H */
