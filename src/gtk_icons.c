/*
 * GTK4 Icon Management for gEudora
 * Loads individual SVG icon files for toolbar buttons
 */

#include "gtk_icons.h"
#include <string.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Icon filename mapping - indexed by ToolbarIcon enum */
static const char *icon_filenames[] = {
    "new_message.svg",   /* ICON_NEW_MESSAGE = 0  */
    "open.svg",          /* ICON_OPEN = 1         */
    "save.svg",          /* ICON_SAVE = 2         */
    "print.svg",         /* ICON_PRINT = 3        */
    "cut.svg",           /* ICON_CUT = 4          */
    "copy.svg",          /* ICON_COPY = 5         */
    "paste.svg",         /* ICON_PASTE = 6        */
    "undo.svg",          /* ICON_UNDO = 7         */
    "reply.svg",         /* ICON_REPLY = 8        */
    "reply_all.svg",     /* ICON_REPLY_ALL = 9    */
    "forward.svg",       /* ICON_FORWARD = 10     */
    "delete.svg",        /* ICON_DELETE = 11       */
    "check_mail.svg",    /* ICON_CHECK_MAIL = 12  */
    "send_queued.svg",   /* ICON_SEND_QUEUED = 13 */
    "address_book.svg",  /* ICON_ADDRESS_BOOK = 14*/
    "filters.svg",       /* ICON_FILTERS = 15     */
    "search.svg",        /* ICON_SEARCH = 16      */
    "junk.svg",          /* ICON_JUNK = 17        */
    "not_junk.svg",      /* ICON_NOT_JUNK = 18    */
    "redirect.svg",      /* ICON_REDIRECT = 19    */
    "attach.svg",        /* ICON_ATTACH = 20      */
    "signature.svg",     /* ICON_SIGNATURE = 21   */
    "stationery.svg",    /* ICON_STATIONERY = 22  */
    "personality.svg",   /* ICON_PERSONALITY = 23 */
    "settings.svg",      /* ICON_SETTINGS = 24    */
    "help.svg",          /* ICON_HELP = 25        */
    "quit.svg",          /* ICON_QUIT = 26        */
    "empty_trash.svg",   /* ICON_EMPTY_TRASH = 27 */
    "transfer_in.svg",   /* ICON_TRANSFER_IN = 28 */
    "transfer_out.svg",  /* ICON_TRANSFER_OUT = 29*/
    "mailbox.svg",       /* ICON_MAILBOX = 30     */
    "folder.svg",        /* ICON_FOLDER = 31      */
};

#define NUM_ICONS (sizeof(icon_filenames) / sizeof(icon_filenames[0]))

/* Global icon cache */
static struct {
    GdkPixbuf *icons[32][3];  /* [icon_index][size_index: 0=16, 1=24, 2=32] */
    char resource_dir[512];
} icon_cache;

static int size_to_index(IconSize size)
{
    switch (size) {
        case ICON_SIZE_SMALL:  return 0;
        case ICON_SIZE_MEDIUM: return 1;
        case ICON_SIZE_LARGE:  return 2;
        default: return 1;
    }
}

/* Load a single SVG icon at a given size */
static GdkPixbuf* load_svg_icon(const char *path, int size)
{
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_size(path, size, size, &error);

    if (error != NULL) {
        g_warning("Failed to load icon %s: %s", path, error->message);
        g_error_free(error);
        return NULL;
    }

    return pixbuf;
}

/* Initialize icon system - load all icons from SVG files */
void init_icon_system(const char *resource_dir)
{
    if (!resource_dir) {
        g_warning("Resource directory not specified");
        return;
    }

    strncpy(icon_cache.resource_dir, resource_dir, sizeof(icon_cache.resource_dir) - 1);
    memset(icon_cache.icons, 0, sizeof(icon_cache.icons));

    /* Icons are loaded on demand in get_toolbar_icon() */
}

/* Extract icon - loads from SVG on first access, caches result */
GdkPixbuf* get_toolbar_icon(ToolbarIcon icon, IconSize size)
{
    if (icon < 0 || (size_t)icon >= NUM_ICONS) {
        g_warning("Invalid icon index %d", icon);
        return NULL;
    }

    int si = size_to_index(size);
    int px = (int)size;

    /* Check cache */
    if (icon_cache.icons[icon][si])
        return icon_cache.icons[icon][si];

    /* Build path: resource_dir/icons/filename.svg */
    char path[1024];
    snprintf(path, sizeof(path), "%s/icons/%s", icon_cache.resource_dir, icon_filenames[icon]);

    GdkPixbuf *pixbuf = load_svg_icon(path, px);
    if (pixbuf) {
        icon_cache.icons[icon][si] = pixbuf;
    }

    return pixbuf;
}

/* Create a button with icon and label */
GtkWidget* create_toolbar_button(ToolbarIcon icon, const char *label, IconSize size)
{
    GtkWidget *button = gtk_button_new();

    GdkPixbuf *pixbuf = get_toolbar_icon(icon, size);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    if (pixbuf) {
        GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
        GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
        gtk_box_append(GTK_BOX(box), image);
        g_object_unref(texture);
    }

    gtk_box_append(GTK_BOX(box), gtk_label_new(label));
    gtk_button_set_child(GTK_BUTTON(button), box);

    return button;
}

/* Create a button with icon only */
GtkWidget* create_toolbar_button_no_label(ToolbarIcon icon, IconSize size)
{
    GtkWidget *button = gtk_button_new();

    GdkPixbuf *pixbuf = get_toolbar_icon(icon, size);
    if (!pixbuf) {
        return button;
    }

    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));

    gtk_button_set_child(GTK_BUTTON(button), image);
    g_object_unref(texture);

    return button;
}

/* Set icon on existing button */
void set_toolbar_button_icon(GtkWidget *button, ToolbarIcon icon, IconSize size)
{
    if (!GTK_IS_BUTTON(button))
        return;

    GdkPixbuf *pixbuf = get_toolbar_icon(icon, size);
    if (!pixbuf)
        return;

    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));

    gtk_button_set_child(GTK_BUTTON(button), image);
    g_object_unref(texture);
}

/* Cleanup icon system */
void cleanup_icon_system(void)
{
    for (size_t i = 0; i < NUM_ICONS; i++) {
        for (int s = 0; s < 3; s++) {
            if (icon_cache.icons[i][s]) {
                g_object_unref(icon_cache.icons[i][s]);
                icon_cache.icons[i][s] = NULL;
            }
        }
    }
}
