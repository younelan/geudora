#include "task_ldef.h"
#include "util.h" /* For ASSERT and other utils */
#include <gtk/gtk.h>

#define FILE_NUM 109

/* Helper structure to store cell data in GObject row */
typedef struct {
  drawCellType draw;
  void *data;
} TaskCellData;

static void task_cell_data_free(TaskCellData *tcd) { g_free(tcd); }

/* The actual drawing function connected to GtkDrawingArea */
static void task_cell_draw(GtkDrawingArea *drawing_area, cairo_t *cr, int width,
                           int height, gpointer user_data) {
  GtkWidget *row = GTK_WIDGET(user_data);
  TaskCellData *tcd = g_object_get_data(G_OBJECT(row), "task-cell-data");
  if (tcd && tcd->draw) {
    MyWindowPtr win = NULL;
    GtkWidget *parent = gtk_widget_get_ancestor(row, GTK_TYPE_WINDOW);
    if (parent) {
      win = g_object_get_data(G_OBJECT(parent), "my-window-ptr");
    }
    (*tcd->draw)(win, row, cr, tcd->data);
  }
}

/* Add an entry to the GtkListBox */
int AddListItemEntry(short where, drawCellType draw, void *data,
                     ListHandle lHandle) {
  if (!GTK_IS_LIST_BOX(lHandle))
    return -1;

  GtkWidget *row = gtk_list_box_row_new();
  TaskCellData *tcd = g_new0(TaskCellData, 1);
  tcd->draw = draw;
  tcd->data = data;

  g_object_set_data_full(G_OBJECT(row), "task-cell-data", tcd,
                         (GDestroyNotify)task_cell_data_free);

  GtkWidget *da = gtk_drawing_area_new();
  gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(da), 300);
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(da), 50);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(da), task_cell_draw, row,
                                 NULL);
  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), da);

  gtk_widget_set_visible(row, TRUE);

  if (where == -1 || where >= 32767) // append
    gtk_list_box_append(GTK_LIST_BOX(lHandle), row);
  else
    gtk_list_box_insert(GTK_LIST_BOX(lHandle), row, where);

  return 0; // 0
}

/* Remove an entry matching data pointer */
void RemoveListItemEntry(void *data, ListHandle lHandle) {
  if (!GTK_IS_LIST_BOX(lHandle))
    return;

  GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(lHandle));
  while (child != NULL) {
    if (GTK_IS_LIST_BOX_ROW(child)) {
      TaskCellData *tcd = g_object_get_data(G_OBJECT(child), "task-cell-data");
      if (tcd && tcd->data == data) {
        gtk_list_box_remove(GTK_LIST_BOX(lHandle), child);
        return;
      }
    }
    child = gtk_widget_get_next_sibling(child);
  }
}

/* ListItemDraw wrapper for GTK - trigger custom drawing if necessary */
void ListItemDraw(bool lSelect, GtkWidget *row, Cell lCell,
                  ListHandle lHandle) {
  /*
   * In a true GTK port, drawing is handled by GtkWidget::snapshot or by child
   * widgets. For the initial "shim" port, we allow triggering the draw
   * callback.
   */
  TaskCellData *tcd = g_object_get_data(G_OBJECT(row), "task-cell-data");
  if (tcd && tcd->draw) {
    /* In gEudora port, MyWindowPtr is often stored in the parent window widget
     */
    MyWindowPtr win = NULL;
    GtkWidget *parent =
        gtk_widget_get_ancestor(GTK_WIDGET(lHandle), GTK_TYPE_WINDOW);
    if (parent) {
      /* Attempt to find win handle if stored in widget data */
      win = g_object_get_data(G_OBJECT(parent), "my-window-ptr");
    }
    (*tcd->draw)(win, row, NULL, tcd->data);
  }
}
