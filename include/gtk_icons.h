/*
 * GTK4 Icon Management for gEudora
 * Handles loading and managing toolbar icons from sprite sheets
 */

#ifndef GTK_ICONS_H
#define GTK_ICONS_H

#include <gtk/gtk.h>

/* Icon size constants */
typedef enum {
    ICON_SIZE_SMALL = 16,
    ICON_SIZE_MEDIUM = 24,
    ICON_SIZE_LARGE = 32
} IconSize;

/* Icon indices in the toolbar sprite sheet */
typedef enum {
    ICON_NEW_MESSAGE = 0,
    ICON_OPEN = 1,
    ICON_SAVE = 2,
    ICON_PRINT = 3,
    ICON_CUT = 4,
    ICON_COPY = 5,
    ICON_PASTE = 6,
    ICON_UNDO = 7,
    ICON_REPLY = 8,
    ICON_REPLY_ALL = 9,
    ICON_FORWARD = 10,
    ICON_DELETE = 11,
    ICON_CHECK_MAIL = 12,
    ICON_SEND_QUEUED = 13,
    ICON_ADDRESS_BOOK = 14,
    ICON_FILTERS = 15,
    ICON_SEARCH = 16,
    ICON_JUNK = 17,
    ICON_NOT_JUNK = 18,
    ICON_REDIRECT = 19,
    ICON_ATTACH = 20,
    ICON_SIGNATURE = 21,
    ICON_STATIONERY = 22,
    ICON_PERSONALITY = 23,
    ICON_SETTINGS = 24,
    ICON_HELP = 25,
    ICON_QUIT = 26,
    ICON_EMPTY_TRASH = 27,
    ICON_TRANSFER_IN = 28,
    ICON_TRANSFER_OUT = 29,
    ICON_MAILBOX = 30,
    ICON_FOLDER = 31
} ToolbarIcon;

/* Initialize icon system - load sprite sheets */
void init_icon_system(const char *resource_dir);

/* Create a button with icon and label from sprite sheet */
GtkWidget* create_toolbar_button(ToolbarIcon icon, const char *label, IconSize size);

/* Create a button with icon only from sprite sheet */
GtkWidget* create_toolbar_button_no_label(ToolbarIcon icon, IconSize size);

/* Extract icon from sprite sheet */
GdkPixbuf* get_toolbar_icon(ToolbarIcon icon, IconSize size);

/* Set icon on existing button from sprite sheet */
void set_toolbar_button_icon(GtkWidget *button, ToolbarIcon icon, IconSize size);

/* Cleanup icon system */
void cleanup_icon_system(void);

#endif /* GTK_ICONS_H */
