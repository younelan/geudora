/*
 * GTK4 Dockable Toolbar System for gEudora
 *
 * Port of Eudora's floating/dockable toolbar system.
 * Original Mac version (toolbar.c) supported two modes:
 *   - kDockable: toolbar docked to main window edge
 *   - kFloating: toolbar in its own floating window (kFloatingWindowClass)
 * The mode was controlled by PREF_TB_FLOATING.
 *
 * This GTK4 port implements both modes:
 *   - Docked: toolbar is a child widget inside the main window
 *   - Floating: toolbar is reparented into a separate GtkWindow
 * Right-click on the toolbar brings up a context menu to toggle mode.
 */

#include "gtk_toolbar_dock.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static void toolbar_dock_to_window(DockableToolbar *toolbar);
static void toolbar_float(DockableToolbar *toolbar);
static gboolean on_float_window_close(GtkWindow *window, gpointer user_data);

/* Right-click context menu handler */
static void on_toolbar_dock_toggle(GSimpleAction *action, GVariant *parameter,
                                    gpointer user_data) {
  (void)action;
  (void)parameter;
  DockableToolbar *toolbar = (DockableToolbar *)user_data;

  if (toolbar->floating)
    toolbar_dock_to_window(toolbar);
  else
    toolbar_float(toolbar);
}

/* Build the right-click popover menu for the toolbar */
static GtkWidget *create_toolbar_context_menu(DockableToolbar *toolbar) {
  GSimpleActionGroup *group = g_simple_action_group_new();
  GSimpleAction *toggle =
      g_simple_action_new("toggle-float", NULL);
  g_signal_connect(toggle, "activate",
                   G_CALLBACK(on_toolbar_dock_toggle), toolbar);
  g_action_map_add_action(G_ACTION_MAP(group), G_ACTION(toggle));
  g_object_unref(toggle);

  GMenu *menu = g_menu_new();
  g_menu_append(menu, "Toggle Floating", "toolbar.toggle-float");

  GtkWidget *popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
  g_object_unref(menu);

  gtk_widget_insert_action_group(toolbar->toolbar, "toolbar",
                                 G_ACTION_GROUP(group));
  g_object_unref(group);

  return popover;
}

/* Right-click gesture handler */
static void on_toolbar_right_click(GtkGestureClick *gesture, int n_press,
                                    double x, double y, gpointer user_data) {
  (void)n_press;
  DockableToolbar *toolbar = (DockableToolbar *)user_data;

  if (!toolbar->context_menu) {
    toolbar->context_menu = create_toolbar_context_menu(toolbar);
    gtk_widget_set_parent(toolbar->context_menu, toolbar->toolbar);
  }

  GdkRectangle rect = {(int)x, (int)y, 1, 1};
  gtk_popover_set_pointing_to(GTK_POPOVER(toolbar->context_menu), &rect);
  gtk_popover_popup(GTK_POPOVER(toolbar->context_menu));

  gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

/* Create a dockable toolbar */
DockableToolbar *create_dockable_toolbar(const char *name) {
  DockableToolbar *toolbar = g_new0(DockableToolbar, 1);
  toolbar->name = g_strdup(name);
  toolbar->position = TOOLBAR_DOCK_TOP;
  toolbar->visible = TRUE;
  toolbar->floating = FALSE;
  toolbar->float_window = NULL;
  toolbar->dock_parent = NULL;
  toolbar->context_menu = NULL;

  /* Create main container — this is what gets added to the main window
   * or reparented into the floating window */
  toolbar->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(toolbar->container, "dockable-toolbar");

  /* Create the actual toolbar box with buttons */
  toolbar->toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_add_css_class(toolbar->toolbar, "toolbar");
  gtk_widget_set_margin_start(toolbar->toolbar, 4);
  gtk_widget_set_margin_end(toolbar->toolbar, 4);
  gtk_widget_set_margin_top(toolbar->toolbar, 2);
  gtk_widget_set_margin_bottom(toolbar->toolbar, 2);

  gtk_box_append(GTK_BOX(toolbar->container), toolbar->toolbar);

  /* Right-click gesture for context menu */
  GtkGesture *right_click = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(right_click), 3);
  g_signal_connect(right_click, "pressed",
                   G_CALLBACK(on_toolbar_right_click), toolbar);
  gtk_widget_add_controller(toolbar->toolbar,
                            GTK_EVENT_CONTROLLER(right_click));

  return toolbar;
}

/* Add button to toolbar */
void toolbar_add_button(DockableToolbar *toolbar, GtkWidget *button) {
  if (!toolbar || !button)
    return;
  gtk_box_append(GTK_BOX(toolbar->toolbar), button);
}

/* Add separator to toolbar */
void toolbar_add_separator(DockableToolbar *toolbar) {
  if (!toolbar)
    return;
  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
  gtk_box_append(GTK_BOX(toolbar->toolbar), separator);
}

/* Set toolbar position (orientation) */
void set_toolbar_position(DockableToolbar *toolbar,
                          ToolbarDockPosition position) {
  if (!toolbar)
    return;

  toolbar->position = position;

  if (position == TOOLBAR_DOCK_LEFT || position == TOOLBAR_DOCK_RIGHT) {
    gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar->toolbar),
                                  GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_hexpand(toolbar->toolbar, FALSE);
    gtk_widget_set_vexpand(toolbar->toolbar, TRUE);
  } else {
    gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar->toolbar),
                                  GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand(toolbar->toolbar, TRUE);
    gtk_widget_set_vexpand(toolbar->toolbar, FALSE);
  }

  if (position == TOOLBAR_DOCK_FLOATING) {
    toolbar_float(toolbar);
  } else if (toolbar->floating) {
    toolbar_dock_to_window(toolbar);
  }
}

/**********************************************************************
 * toolbar_float - tear off toolbar into its own floating window.
 * Like original Eudora's kFloatingWindowClass mode.
 **********************************************************************/
static void toolbar_float(DockableToolbar *toolbar) {
  if (!toolbar || toolbar->floating)
    return;

  /* Remember the dock parent so we can re-dock later */
  toolbar->dock_parent = gtk_widget_get_parent(toolbar->container);

  /* Remove from dock parent */
  if (toolbar->dock_parent) {
    g_object_ref(toolbar->container); /* prevent destruction */
    gtk_box_remove(GTK_BOX(toolbar->dock_parent), toolbar->container);
  }

  /* Create floating window */
  toolbar->float_window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(toolbar->float_window), toolbar->name);
  gtk_window_set_decorated(GTK_WINDOW(toolbar->float_window), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(toolbar->float_window), FALSE);
  gtk_window_set_deletable(GTK_WINDOW(toolbar->float_window), TRUE);

  /* Set the toolbar as the window content */
  gtk_window_set_child(GTK_WINDOW(toolbar->float_window), toolbar->container);
  if (toolbar->dock_parent)
    g_object_unref(toolbar->container);

  /* When the floating window is closed, re-dock the toolbar */
  g_signal_connect(toolbar->float_window, "close-request",
                   G_CALLBACK(on_float_window_close), toolbar);

  toolbar->floating = TRUE;
  toolbar->position = TOOLBAR_DOCK_FLOATING;

  gtk_window_present(GTK_WINDOW(toolbar->float_window));
}

/**********************************************************************
 * toolbar_dock_to_window - re-dock a floating toolbar back to its
 * parent container. Like switching from kFloating to kDockable.
 **********************************************************************/
static void toolbar_dock_to_window(DockableToolbar *toolbar) {
  if (!toolbar || !toolbar->floating)
    return;

  if (toolbar->float_window) {
    /* Remove container from floating window */
    g_object_ref(toolbar->container);
    gtk_window_set_child(GTK_WINDOW(toolbar->float_window), NULL);

    /* Destroy the floating window */
    gtk_window_destroy(GTK_WINDOW(toolbar->float_window));
    toolbar->float_window = NULL;
  }

  /* Re-add to dock parent */
  if (toolbar->dock_parent) {
    gtk_box_prepend(GTK_BOX(toolbar->dock_parent), toolbar->container);
    g_object_unref(toolbar->container);
  }

  toolbar->floating = FALSE;
  toolbar->position = TOOLBAR_DOCK_TOP;
}

/* void *floating window close — re-dock instead of destroying */
static gboolean on_float_window_close(GtkWindow *window, gpointer user_data) {
  (void)window;
  DockableToolbar *toolbar = (DockableToolbar *)user_data;
  toolbar_dock_to_window(toolbar);
  return TRUE; /* prevent default close (we handled it) */
}

/* Toggle toolbar visibility */
void toggle_toolbar_visibility(DockableToolbar *toolbar) {
  if (!toolbar)
    return;
  toolbar->visible = !toolbar->visible;

  if (toolbar->floating && toolbar->float_window) {
    gtk_widget_set_visible(toolbar->float_window, toolbar->visible);
  } else {
    gtk_widget_set_visible(toolbar->container, toolbar->visible);
  }
}

/* Get toolbar widget (the container for embedding in main window) */
GtkWidget *get_toolbar_widget(DockableToolbar *toolbar) {
  if (!toolbar)
    return NULL;
  return toolbar->container;
}

/* Check if toolbar is floating */
gboolean is_toolbar_floating(DockableToolbar *toolbar) {
  if (!toolbar)
    return FALSE;
  return toolbar->floating;
}

/* Free dockable toolbar */
void free_dockable_toolbar(DockableToolbar *toolbar) {
  if (!toolbar)
    return;

  if (toolbar->floating && toolbar->float_window)
    gtk_window_destroy(GTK_WINDOW(toolbar->float_window));

  if (toolbar->context_menu)
    gtk_widget_unparent(toolbar->context_menu);

  g_free(toolbar->name);
  g_free(toolbar);
}
