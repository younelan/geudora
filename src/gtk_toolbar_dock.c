/*
 * GTK4 Dockable Toolbar System for gEudora
 * Implements Eudora-style dockable toolbars (Wazoo)
 */

#include "gtk_toolbar_dock.h"
#include <stdlib.h>
#include <string.h>

/* Create a dockable toolbar */
DockableToolbar* create_dockable_toolbar(const char *name)
{
    DockableToolbar *toolbar = g_new0(DockableToolbar, 1);
    toolbar->name = g_strdup(name);
    toolbar->position = TOOLBAR_DOCK_TOP;
    toolbar->visible = TRUE;
    toolbar->floating = FALSE;
    
    /* Create main container */
    toolbar->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(toolbar->container, "dockable-toolbar");
    
    /* Create title bar for floating mode */
    toolbar->title_bar = gtk_header_bar_new();
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(toolbar->title_bar), gtk_label_new(name));
    gtk_widget_set_visible(toolbar->title_bar, FALSE);  /* Hidden by default */
    
    /* Create the actual toolbar */
    toolbar->toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_add_css_class(toolbar->toolbar, "toolbar");
    gtk_widget_set_margin_start(toolbar->toolbar, 4);
    gtk_widget_set_margin_end(toolbar->toolbar, 4);
    gtk_widget_set_margin_top(toolbar->toolbar, 4);
    gtk_widget_set_margin_bottom(toolbar->toolbar, 4);
    
    /* Add title bar and toolbar to container */
    gtk_box_append(GTK_BOX(toolbar->container), toolbar->title_bar);
    gtk_box_append(GTK_BOX(toolbar->container), toolbar->toolbar);
    
    return toolbar;
}

/* Add button to toolbar */
void toolbar_add_button(DockableToolbar *toolbar, GtkWidget *button)
{
    if (!toolbar || !button) {
        return;
    }
    
    gtk_box_append(GTK_BOX(toolbar->toolbar), button);
}

/* Add separator to toolbar */
void toolbar_add_separator(DockableToolbar *toolbar)
{
    if (!toolbar) {
        return;
    }
    
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(toolbar->toolbar), separator);
}

/* Set toolbar position */
void set_toolbar_position(DockableToolbar *toolbar, ToolbarDockPosition position)
{
    if (!toolbar) {
        return;
    }
    
    toolbar->position = position;
    
    /* Update orientation based on position */
    if (position == TOOLBAR_DOCK_LEFT || position == TOOLBAR_DOCK_RIGHT) {
        gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar->toolbar), GTK_ORIENTATION_VERTICAL);
        gtk_widget_set_hexpand(toolbar->toolbar, FALSE);
        gtk_widget_set_vexpand(toolbar->toolbar, TRUE);
    } else {
        gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar->toolbar), GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_hexpand(toolbar->toolbar, TRUE);
        gtk_widget_set_vexpand(toolbar->toolbar, FALSE);
    }
    
    /* Show title bar if floating */
    if (position == TOOLBAR_DOCK_FLOATING) {
        gtk_widget_set_visible(toolbar->title_bar, TRUE);
        toolbar->floating = TRUE;
    } else {
        gtk_widget_set_visible(toolbar->title_bar, FALSE);
        toolbar->floating = FALSE;
    }
}

/* Toggle toolbar visibility */
void toggle_toolbar_visibility(DockableToolbar *toolbar)
{
    if (!toolbar) {
        return;
    }
    
    toolbar->visible = !toolbar->visible;
    gtk_widget_set_visible(toolbar->container, toolbar->visible);
}

/* Get toolbar widget */
GtkWidget* get_toolbar_widget(DockableToolbar *toolbar)
{
    if (!toolbar) {
        return NULL;
    }
    
    return toolbar->container;
}

/* Free dockable toolbar */
void free_dockable_toolbar(DockableToolbar *toolbar)
{
    if (!toolbar) {
        return;
    }
    
    g_free(toolbar->name);
    g_free(toolbar);
}
