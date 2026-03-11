#ifndef gedit_PRINT_H
#define gedit_PRINT_H

#include "gedit-document.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Print the document to a printer or PDF */
void gedit_print_document(GtkWidget *parent_window, geditDocument *doc,
                          const gchar *document_title);

G_END_DECLS

#endif /* gedit_PRINT_H */
