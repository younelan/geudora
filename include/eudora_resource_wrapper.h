/* Minimal wrapper to expose embedded Eudora GResource bundle.
 * Provides routines to register the compiled resource and lookup
 * data or strings by resource path.
 */

#ifndef EUDORA_RESOURCE_WRAPPER_H
#define EUDORA_RESOURCE_WRAPPER_H

#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Ensure embedded resources are registered. Safe to call multiple times. */
void eudora_resources_register(void);

/* Lookup raw bytes for a resource path. Returns a new reference to GBytes
 * (caller should g_bytes_unref). Returns NULL on failure. */
GBytes *eudora_resources_lookup_data(const char *path);

/* Lookup a resource and return a NUL-terminated heap string. Caller must free(). */
char *eudora_resources_lookup_string(const char *path);

/* Load an image from the resource path as a GdkPixbuf; caller must g_object_unref(). */
GdkPixbuf *eudora_resources_load_pixbuf(const char *path);

#endif /* EUDORA_RESOURCE_WRAPPER_H */
