/* gtk_autocomplete.h — Nickname autocomplete popover for address entries
 *
 * Attaches to any GtkEntry. On typing, queries macmbx_nick_complete()
 * and shows a dropdown of matching nicknames/addresses.
 * Eudora is just the widget — all matching logic lives in macmbx.
 */

#ifndef GTK_AUTOCOMPLETE_H
#define GTK_AUTOCOMPLETE_H

#include <gtk/gtk.h>
#include "macmbx.h"

/* Attach autocomplete to a GtkEntry.
 * abs: the open address books (borrowed, not freed).
 * The popover is created and managed automatically. */
void gtk_autocomplete_attach(GtkWidget *entry, MacmbxAddressBooks *abs);

/* Detach autocomplete from an entry (optional — cleaned up on widget destroy). */
void gtk_autocomplete_detach(GtkWidget *entry);

#endif /* GTK_AUTOCOMPLETE_H */
