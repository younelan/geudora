/* resource_manager.h - minimal wrapper for embedded GResource bundle
 * Provides helpers to register the compiled resource and lookup
 * data, strings, and images by resource path.
 */

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Register the embedded Eudora GResource bundle. Safe to call repeatedly. */
void resource_manager_register(void);

/* Lookup raw bytes for a resource path. Returns a new reference to GBytes
 * (caller should g_bytes_unref). Returns NULL on failure. */
GBytes *resource_manager_lookup_data(const char *path);

/* Lookup a resource and return a NUL-terminated heap string. Caller must free(). */
char *resource_manager_lookup_string(const char *path);

/* Load an image from the resource path as a GdkPixbuf; caller must g_object_unref(). */
GdkPixbuf *resource_manager_load_pixbuf(const char *path);

#endif /* RESOURCE_MANAGER_H */
