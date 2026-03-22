/*
 * gedit-table.h - Table support for gEditCtrl
 *
 * Standalone GTK4 table widget: creates a GtkGrid embedded as a child
 * widget in a GtkTextView via text buffer child anchors.
 *
 * Ported from original Eudora table.c, adapted to GTK4.
 * No Eudora dependencies - only GTK4/GLib and standard C.
 */

#ifndef GEDIT_TABLE_H
#define GEDIT_TABLE_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdint.h>

G_BEGIN_DECLS

/* Horizontal alignment */
enum {
    GEDIT_TABLE_HALIGN_DEFAULT  = 0,
    GEDIT_TABLE_HALIGN_LEFT     = 1,
    GEDIT_TABLE_HALIGN_CENTER   = 2,
    GEDIT_TABLE_HALIGN_RIGHT    = 3,
    GEDIT_TABLE_HALIGN_JUSTIFY  = 4
};

/* Vertical alignment */
enum {
    GEDIT_TABLE_VALIGN_DEFAULT  = 0,  /* same as top */
    GEDIT_TABLE_VALIGN_MIDDLE   = 1,
    GEDIT_TABLE_VALIGN_BOTTOM   = 2
};

/* Table cell data */
typedef struct {
    int row, column;
    int rowSpan, colSpan;       /* 1 = no span */
    int width, height;          /* 0 = auto, <0 = percentage, >0 = pixels */
    int hAlign;                 /* GEDIT_TABLE_HALIGN_* */
    int vAlign;                 /* GEDIT_TABLE_VALIGN_* */
    char *text;                 /* cell text content (may include simple markup) */
    uint32_t bgColor;           /* 0 = transparent, else 0xRRGGBB */
    bool isHeader;              /* <th> vs <td> */
} GEditTableCell;

/* Table data */
typedef struct {
    int rows, columns;
    int width;                  /* 0=auto, <0=percentage, >0=pixels */
    int border;                 /* border thickness in pixels */
    int cellPadding;            /* padding inside each cell */
    int cellSpacing;            /* spacing between cells */
    uint32_t bgColor;           /* table background, 0 = transparent */
    GEditTableCell *cells;
    int cellCount;
    int cellAlloc;              /* allocated capacity for cells array */
} GEditTable;

/* Create a table data structure. Caller populates cells. */
GEditTable *gedit_table_new(int rows, int columns);

/* Free a table and all its cells. */
void gedit_table_free(GEditTable *table);

/* Add a cell to the table. Returns cell index, or -1 on error. */
int gedit_table_add_cell(GEditTable *table, int row, int col,
                         const char *text);

/* Set cell span (rowSpan, colSpan). */
void gedit_table_cell_set_span(GEditTable *table, int cellIdx,
                               int rowSpan, int colSpan);

/* Set cell alignment (hAlign, vAlign). */
void gedit_table_cell_set_align(GEditTable *table, int cellIdx,
                                int hAlign, int vAlign);

/* Set cell background color (0xRRGGBB, or 0 for transparent). */
void gedit_table_cell_set_bg(GEditTable *table, int cellIdx, uint32_t color);

/* Mark cell as header (<th>) or data (<td>). */
void gedit_table_cell_set_header(GEditTable *table, int cellIdx, bool isHeader);

/* Set cell explicit size (0=auto, <0=percentage, >0=pixels). */
void gedit_table_cell_set_size(GEditTable *table, int cellIdx,
                               int width, int height);

/* Insert a table into a GtkTextView at the current cursor position.
 * Creates a GtkGrid child widget anchored in the text buffer.
 * Returns the GtkWidget* of the created grid (for later reference). */
GtkWidget *gedit_table_insert(GtkTextView *text_view, GEditTable *table);

/* Insert a table at a specific text buffer iterator. */
GtkWidget *gedit_table_insert_at(GtkTextView *text_view, GEditTable *table,
                                 GtkTextIter *iter);

/* Parse an HTML <table> string and insert into text view.
 * Handles: <table>, <tr>, <td>, <th>, colspan, rowspan,
 * align, valign, width, height, bgcolor, border, cellpadding, cellspacing.
 * Returns the created GtkWidget or NULL on parse error. */
GtkWidget *gedit_table_insert_html(GtkTextView *text_view,
                                   const char *html, int htmlLen);

/* Get table data from a GtkGrid widget previously created by
 * gedit_table_insert (for export/copy). Caller must free with
 * gedit_table_free(). Returns NULL if widget is not a gedit table. */
GEditTable *gedit_table_from_widget(GtkWidget *grid);

/* Export table to HTML string. Caller frees result with g_free(). */
char *gedit_table_to_html(GEditTable *table);

G_END_DECLS

#endif /* GEDIT_TABLE_H */
