/*
 * GTK4 Docking System for gEudora
 * Implements dockable toolbars similar to Eudora's Wazoo
 */

#include "gtk_docking.h"
#include <stdlib.h>
#include <string.h>

/* Create a dockable panel */
DockablePanel* create_dockable_panel(const char *title, GtkWidget *content)
{
    DockablePanel *panel = g_new0(DockablePanel, 1);
    panel->title = g_strdup(title);
    panel->widget = content;
    panel->position = DOCK_LEFT;
    panel->visible = TRUE;
    
    /* Create container box - just the content, no header */
    panel->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(panel->container, "dockable-panel");
    
    /* Add content directly */
    gtk_box_append(GTK_BOX(panel->container), content);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_widget_set_hexpand(content, TRUE);
    
    return panel;
}

/* Add panel to dock area */
void add_panel_to_dock(GtkBox *dock_area, DockablePanel *panel, DockPosition position)
{
    if (!dock_area || !panel) {
        return;
    }
    
    panel->position = position;
    gtk_box_append(GTK_BOX(dock_area), panel->container);
    
    /* Set expansion based on position */
    if (position == DOCK_LEFT || position == DOCK_RIGHT) {
        gtk_widget_set_hexpand(panel->container, TRUE);
        gtk_widget_set_vexpand(panel->container, TRUE);
    } else {
        gtk_widget_set_hexpand(panel->container, TRUE);
        gtk_widget_set_vexpand(panel->container, FALSE);
    }
}

/* Toggle panel visibility */
void toggle_panel_visibility(DockablePanel *panel)
{
    if (!panel) {
        return;
    }
    
    panel->visible = !panel->visible;
    gtk_widget_set_visible(panel->container, panel->visible);
}

/* Set panel position */
void set_panel_position(DockablePanel *panel, DockPosition position)
{
    if (!panel) {
        return;
    }
    
    panel->position = position;
    
    /* Update expansion based on new position */
    if (position == DOCK_LEFT || position == DOCK_RIGHT) {
        gtk_widget_set_hexpand(panel->container, TRUE);
        gtk_widget_set_vexpand(panel->container, TRUE);
    } else if (position == DOCK_TOP || position == DOCK_BOTTOM) {
        gtk_widget_set_hexpand(panel->container, TRUE);
        gtk_widget_set_vexpand(panel->container, FALSE);
    }
}

/* Free dockable panel */
void free_dockable_panel(DockablePanel *panel)
{
    if (!panel) {
        return;
    }
    
    g_free(panel->title);
    g_free(panel);
}
