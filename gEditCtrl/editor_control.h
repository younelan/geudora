#ifndef EDITOR_CONTROL_H
#define EDITOR_CONTROL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Creates the editor control. Returns a GtkWidget pointer to a GtkTextView
 * pre-filled with demo text. Caller may treat the returned widget as a
 * `GtkTextView*` for connecting actions/tags.
 */
GtkWidget *editor_control_new(void);

G_END_DECLS

#endif /* EDITOR_CONTROL_H */
