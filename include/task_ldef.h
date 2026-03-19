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

/* Draw callback type */
typedef void (*drawCellType)(void *win, GtkWidget *row, void *cr,
                             void *data);

int AddListItemEntry(short where, drawCellType draw, void *data,
                     ListHandle lHandle);
void RemoveListItemEntry(void *data, ListHandle lHandle);
void ListItemDraw(bool lSelect, GtkWidget *row, Cell lCell, ListHandle lHandle);

#endif