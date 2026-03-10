/*
 * GTK4 Docking System for gEudora
 * Implements dockable toolbars/panels similar to Eudora's Wazoo
 */

#ifndef GTK_DOCKING_H
#define GTK_DOCKING_H

#include <gtk/gtk.h>

/* Dock position */
typedef enum {
    DOCK_LEFT,
    DOCK_RIGHT,
    DOCK_TOP,
    DOCK_BOTTOM,
    DOCK_FLOATING
} DockPosition;

/* Dockable panel structure */
typedef struct {
    char *title;
    GtkWidget *widget;
    GtkWidget *container;
    GtkWidget *header_bar;
    GtkWidget *close_button;
    DockPosition position;
    gboolean visible;
} DockablePanel;

/* Create a dockable panel */
DockablePanel* create_dockable_panel(const char *title, GtkWidget *content);

/* Add panel to dock area */
void add_panel_to_dock(GtkBox *dock_area, DockablePanel *panel, DockPosition position);

/* Toggle panel visibility */
void toggle_panel_visibility(DockablePanel *panel);

/* Set panel position */
void set_panel_position(DockablePanel *panel, DockPosition position);

/* Free dockable panel */
void free_dockable_panel(DockablePanel *panel);

#endif /* GTK_DOCKING_H */
