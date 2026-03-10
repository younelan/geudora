/*
 * GTK4 Icon Management for gEudora
 * Handles loading and managing toolbar icons from sprite sheets
 */

#include "gtk_icons.h"
#include <string.h>
#include <stdlib.h>

/* Global sprite sheet cache */
static struct {
    GdkPixbuf *sprite_16;
    GdkPixbuf *sprite_24;
    GdkPixbuf *sprite_32;
    char resource_dir[512];
} icon_cache = {NULL, NULL, NULL, ""};

/* Load sprite sheet from file */
static GdkPixbuf* load_sprite_sheet(const char *filename, int size)
{
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    
    if (error != NULL) {
        g_warning("Failed to load sprite sheet %s: %s", filename, error->message);
        g_error_free(error);
        return NULL;
    }
    
    return pixbuf;
}

/* Initialize icon system - load sprite sheets */
void init_icon_system(const char *resource_dir)
{
    if (!resource_dir) {
        g_warning("Resource directory not specified");
        return;
    }
    
    strncpy(icon_cache.resource_dir, resource_dir, sizeof(icon_cache.resource_dir) - 1);
    
    /* Load sprite sheets */
    char path[1024];
    
    snprintf(path, sizeof(path), "%s/tbar16.png", resource_dir);
    icon_cache.sprite_16 = load_sprite_sheet(path, 16);
    
    snprintf(path, sizeof(path), "%s/tbar32.png", resource_dir);
    icon_cache.sprite_32 = load_sprite_sheet(path, 32);
    
    /* tbar24 might not exist, use 32 as fallback */
    icon_cache.sprite_24 = icon_cache.sprite_32;
    
    if (!icon_cache.sprite_32) {
        g_warning("Failed to load toolbar sprite sheets");
    }
}

/* Extract icon from sprite sheet */
GdkPixbuf* get_toolbar_icon(ToolbarIcon icon, IconSize size)
{
    GdkPixbuf *sprite = NULL;
    int icon_size = size;
    
    /* Select appropriate sprite sheet */
    switch (size) {
        case ICON_SIZE_SMALL:
            sprite = icon_cache.sprite_16;
            icon_size = 16;
            break;
        case ICON_SIZE_MEDIUM:
            sprite = icon_cache.sprite_24 ? icon_cache.sprite_24 : icon_cache.sprite_32;
            icon_size = 24;
            break;
        case ICON_SIZE_LARGE:
            sprite = icon_cache.sprite_32;
            icon_size = 32;
            break;
        default:
            sprite = icon_cache.sprite_32;
            icon_size = 32;
    }
    
    if (!sprite) {
        g_warning("Sprite sheet not loaded for size %d", size);
        return NULL;
    }
    
    /* Extract icon from sprite sheet */
    /* Each sprite sheet is icon_size x (num_icons * icon_size) */
    int x = icon * icon_size;
    int y = 0;
    
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_subpixbuf(sprite, x, y, icon_size, icon_size);
    
    return icon_pixbuf;
}

/* Create a button with icon and label from sprite sheet */
GtkWidget* create_toolbar_button(ToolbarIcon icon, const char *label, IconSize size)
{
    GtkWidget *button = gtk_button_new_with_label(label);
    
    /* Get icon from sprite sheet */
    GdkPixbuf *pixbuf = get_toolbar_icon(icon, size);
    if (!pixbuf) {
        return button;  /* Return button without icon if loading fails */
    }
    
    /* Create texture from pixbuf */
    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
    
    /* Create box to hold both icon and label */
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_append(GTK_BOX(box), image);
    gtk_box_append(GTK_BOX(box), gtk_label_new(label));
    
    gtk_button_set_child(GTK_BUTTON(button), box);
    
    g_object_unref(pixbuf);
    g_object_unref(texture);
    
    return button;
}

/* Create a button with icon only from sprite sheet */
GtkWidget* create_toolbar_button_no_label(ToolbarIcon icon, IconSize size)
{
    GtkWidget *button = gtk_button_new();
    
    /* Get icon from sprite sheet */
    GdkPixbuf *pixbuf = get_toolbar_icon(icon, size);
    if (!pixbuf) {
        return button;  /* Return button without icon if loading fails */
    }
    
    /* Create texture from pixbuf */
    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
    
    gtk_button_set_child(GTK_BUTTON(button), image);
    
    g_object_unref(pixbuf);
    g_object_unref(texture);
    
    return button;
}

/* Set icon on existing button from sprite sheet */
void set_toolbar_button_icon(GtkWidget *button, ToolbarIcon icon, IconSize size)
{
    if (!GTK_IS_BUTTON(button)) {
        return;
    }
    
    /* Get icon from sprite sheet */
    GdkPixbuf *pixbuf = get_toolbar_icon(icon, size);
    if (!pixbuf) {
        return;
    }
    
    /* Create texture from pixbuf */
    GdkTexture *texture = gdk_texture_new_for_pixbuf(pixbuf);
    GtkWidget *image = gtk_image_new_from_paintable(GDK_PAINTABLE(texture));
    
    gtk_button_set_child(GTK_BUTTON(button), image);
    
    g_object_unref(pixbuf);
    g_object_unref(texture);
}

/* Cleanup icon system */
void cleanup_icon_system(void)
{
    if (icon_cache.sprite_16) {
        g_object_unref(icon_cache.sprite_16);
        icon_cache.sprite_16 = NULL;
    }
    if (icon_cache.sprite_32) {
        g_object_unref(icon_cache.sprite_32);
        icon_cache.sprite_32 = NULL;
    }
    /* Don't unref sprite_24 if it's the same as sprite_32 */
    icon_cache.sprite_24 = NULL;
}
