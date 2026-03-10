#ifndef TASK_LDEF_H
#define TASK_LDEF_H

#include "mailbox.h"
#include <gtk/gtk.h>

/* Porting Mac List Manager types to GTK */
#ifndef LIST_HANDLE_DEFINED
#define LIST_HANDLE_DEFINED
typedef GtkWidget *ListHandle; /* GtkListBox */
#endif
typedef GtkWidget *Cell;       /* GtkListBoxRow */

/* Draw callback now takes the row widget and a cairo context */
typedef void (*drawCellType)(MyWindowPtr win, GtkWidget *row, cairo_t *cr,
                             Handle data);

OSErr AddListItemEntry(short where, drawCellType draw, Handle data,
                       ListHandle lHandle);
void RemoveListItemEntry(Handle data, ListHandle lHandle);
void ListItemDraw(bool lSelect, GtkWidget *row, Cell lCell, ListHandle lHandle);

#endif