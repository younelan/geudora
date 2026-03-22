/*
 * gedit-table.c - Table support for gEditCtrl
 *
 * Standalone GTK4 table widget: creates a GtkGrid embedded as a child
 * widget in a GtkTextView via text buffer child anchors.
 *
 * Ported from original Eudora table.c, adapted to GTK4.
 * No Eudora dependencies - only GTK4/GLib and standard C.
 */

#include "gedit-table.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

/* ================================================================
 * Table data structure management
 * ================================================================ */

GEditTable *gedit_table_new(int rows, int columns) {
    GEditTable *t = g_new0(GEditTable, 1);
    if (!t) return NULL;
    t->rows = rows;
    t->columns = columns;
    t->width = 0;
    t->border = 1;
    t->cellPadding = 2;
    t->cellSpacing = 2;
    t->bgColor = 0;
    t->cellCount = 0;
    t->cellAlloc = rows * columns;
    if (t->cellAlloc < 4) t->cellAlloc = 4;
    t->cells = g_new0(GEditTableCell, t->cellAlloc);
    if (!t->cells) { g_free(t); return NULL; }
    return t;
}

void gedit_table_free(GEditTable *table) {
    if (!table) return;
    for (int i = 0; i < table->cellCount; i++)
        g_free(table->cells[i].text);
    g_free(table->cells);
    g_free(table);
}

int gedit_table_add_cell(GEditTable *table, int row, int col,
                         const char *text) {
    if (!table) return -1;
    if (table->cellCount >= table->cellAlloc) {
        int newAlloc = table->cellAlloc * 2;
        GEditTableCell *newCells = g_renew(GEditTableCell, table->cells, newAlloc);
        if (!newCells) return -1;
        memset(newCells + table->cellAlloc, 0,
               (newAlloc - table->cellAlloc) * sizeof(GEditTableCell));
        table->cells = newCells;
        table->cellAlloc = newAlloc;
    }
    int idx = table->cellCount++;
    GEditTableCell *c = &table->cells[idx];
    memset(c, 0, sizeof(*c));
    c->row = row;
    c->column = col;
    c->rowSpan = 1;
    c->colSpan = 1;
    c->text = text ? g_strdup(text) : g_strdup("");
    return idx;
}

void gedit_table_cell_set_span(GEditTable *table, int cellIdx,
                               int rowSpan, int colSpan) {
    if (!table || cellIdx < 0 || cellIdx >= table->cellCount) return;
    table->cells[cellIdx].rowSpan = rowSpan > 0 ? rowSpan : 1;
    table->cells[cellIdx].colSpan = colSpan > 0 ? colSpan : 1;
}

void gedit_table_cell_set_align(GEditTable *table, int cellIdx,
                                int hAlign, int vAlign) {
    if (!table || cellIdx < 0 || cellIdx >= table->cellCount) return;
    table->cells[cellIdx].hAlign = hAlign;
    table->cells[cellIdx].vAlign = vAlign;
}

void gedit_table_cell_set_bg(GEditTable *table, int cellIdx, uint32_t color) {
    if (!table || cellIdx < 0 || cellIdx >= table->cellCount) return;
    table->cells[cellIdx].bgColor = color;
}

void gedit_table_cell_set_header(GEditTable *table, int cellIdx, bool isHeader) {
    if (!table || cellIdx < 0 || cellIdx >= table->cellCount) return;
    table->cells[cellIdx].isHeader = isHeader;
}

void gedit_table_cell_set_size(GEditTable *table, int cellIdx,
                               int width, int height) {
    if (!table || cellIdx < 0 || cellIdx >= table->cellCount) return;
    table->cells[cellIdx].width = width;
    table->cells[cellIdx].height = height;
}

/* ================================================================
 * Column width algorithm (ported from original GetColWidths)
 * ================================================================ */

/* Column width status */
enum { COL_NO_WIDTH = 0, COL_FIXED_WIDTH, COL_PROP_WIDTH };

/*
 * Measure the natural width of text content using Pango.
 * Returns width in pixels.
 */
static int measure_text_width(const char *text, bool isHeader) {
    if (!text || !*text) return 20; /* minimum cell width */

    PangoContext *ctx = pango_font_map_create_context(
        pango_cairo_font_map_get_default());
    PangoLayout *layout = pango_layout_new(ctx);
    if (isHeader) {
        PangoFontDescription *fd = pango_font_description_from_string("Sans Bold 10");
        pango_layout_set_font_description(layout, fd);
        pango_font_description_free(fd);
    }
    pango_layout_set_text(layout, text, -1);
    int w, h;
    pango_layout_get_pixel_size(layout, &w, &h);
    g_object_unref(layout);
    g_object_unref(ctx);
    return w + 8; /* add some padding */
}

/*
 * Measure the height of text content at a given column width.
 * Returns height in pixels.
 */
static int measure_text_height(const char *text, int colWidth, bool isHeader) {
    if (!text || !*text) return 18; /* minimum cell height */
    if (colWidth < 10) colWidth = 10;

    PangoContext *ctx = pango_font_map_create_context(
        pango_cairo_font_map_get_default());
    PangoLayout *layout = pango_layout_new(ctx);
    if (isHeader) {
        PangoFontDescription *fd = pango_font_description_from_string("Sans Bold 10");
        pango_layout_set_font_description(layout, fd);
        pango_font_description_free(fd);
    }
    pango_layout_set_width(layout, colWidth * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_text(layout, text, -1);
    int w, h;
    pango_layout_get_pixel_size(layout, &w, &h);
    g_object_unref(layout);
    g_object_unref(ctx);
    return h + 4;
}

/*
 * Compute column widths for all columns in the table.
 * Follows the original Eudora algorithm:
 *   1. Fixed widths from cell/column specs
 *   2. Percentage widths relative to table width
 *   3. Proportional widths (star-sizing)
 *   4. Auto-width: measure text content to determine natural width
 *   5. Distribute excess width to unspecified columns
 *   6. Handle colSpan distribution
 *   7. Clamp overly wide columns
 *
 * Returns an allocated array of column widths. Caller must g_free().
 * Sets *sumWidths to the total of all column widths.
 */
static int *get_col_widths(GEditTable *table, int *sumWidths,
                           int tableWidth, bool tableWidthSpecified) {
    int colCount = table->columns;
    int *widths = g_new0(int, colCount);
    int *colStat = g_new0(int, colCount); /* COL_NO_WIDTH / COL_FIXED_WIDTH */

    /* Pass 1: Collect explicit cell widths (fixed & percentage) */
    for (int ci = 0; ci < table->cellCount; ci++) {
        GEditTableCell *cell = &table->cells[ci];
        int col = cell->column;
        if (col < 0 || col >= colCount) continue;
        if (cell->colSpan > 1) continue; /* handle spans later */

        int w = cell->width;
        if (w < 0) {
            /* Percentage: convert to pixels */
            w = tableWidth * (-w) / 100;
        }
        if (w > 0 && w > widths[col]) {
            widths[col] = w;
            colStat[col] = COL_FIXED_WIDTH;
        }
    }

    /* Pass 2: Auto-width for columns that have no explicit width */
    bool doSpan = false;
    for (int ci = 0; ci < table->cellCount; ci++) {
        GEditTableCell *cell = &table->cells[ci];
        int col = cell->column;
        if (col < 0 || col >= colCount) continue;
        if (colStat[col] != COL_NO_WIDTH) continue;

        if (cell->colSpan < 2) {
            int w = measure_text_width(cell->text, cell->isHeader);
            if (w > widths[col]) widths[col] = w;
        } else {
            doSpan = true;
        }
    }

    /* Pass 3: Handle column spans */
    if (doSpan) {
        int cellBorderExtra = table->cellSpacing +
            2 * (table->cellPadding + (table->border ? 1 : 0));

        for (int ci = 0; ci < table->cellCount; ci++) {
            GEditTableCell *cell = &table->cells[ci];
            int col = cell->column;
            int span = cell->colSpan;
            if (col < 0 || col >= colCount) continue;
            if (span < 2) continue;

            int w = measure_text_width(cell->text, cell->isHeader);
            /* Sum current widths in span */
            int sum = 0;
            int spanEnd = col + span;
            if (spanEnd > colCount) spanEnd = colCount;
            for (int i = col; i < spanEnd; i++)
                sum += widths[i];
            sum += (span - 1) * cellBorderExtra;

            if (w > sum) {
                /* Distribute excess equally among spanned columns */
                int addEach = (w - sum + span - 1) / span;
                for (int i = col; i < spanEnd; i++)
                    widths[i] += addEach;
            }
        }
    }

    /* Pass 4: Clamp overly wide columns when total exceeds available width */
    if (colCount > 0) {
        int sumWidth = 0;
        int leftoverWidth = 0;
        int overCount = 0;
        int aveWidth = tableWidth / colCount;

        for (int col = 0; col < colCount; col++) {
            sumWidth += widths[col];
            if (widths[col] <= aveWidth)
                leftoverWidth += widths[col];
            else
                overCount++;
        }

        if (sumWidth > tableWidth && overCount > 0) {
            int bigWidth = (tableWidth - leftoverWidth);
            if (overCount > 0) bigWidth /= overCount;
            if (bigWidth < aveWidth) bigWidth = aveWidth;
            for (int col = 0; col < colCount; col++) {
                if (widths[col] > aveWidth)
                    widths[col] = bigWidth;
            }
        }
    }

    /* Pass 5: If table width is specified, distribute remaining space
     * among columns with no explicit width */
    if (tableWidthSpecified) {
        int leftoverWidth = tableWidth;
        int noSpecCount = 0;
        for (int col = 0; col < colCount; col++) {
            leftoverWidth -= widths[col];
            if (colStat[col] == COL_NO_WIDTH)
                noSpecCount++;
        }
        if (leftoverWidth > 0 && noSpecCount > 0) {
            int lastNoSpec = 0;
            for (int col = 0; col < colCount; col++) {
                if (colStat[col] == COL_NO_WIDTH) {
                    widths[col] += leftoverWidth / noSpecCount;
                    lastNoSpec = col;
                }
            }
            widths[lastNoSpec] += leftoverWidth % noSpecCount;
        }
    }

    /* Calculate sum */
    *sumWidths = 0;
    for (int col = 0; col < colCount; col++)
        *sumWidths += widths[col];

    g_free(colStat);
    return widths;
}

/* ================================================================
 * Row height algorithm (ported from original GetRowHeights)
 * ================================================================ */

/*
 * Compute row heights for all rows.
 * Auto-height: measures text at final column width.
 * Handles rowSpan similarly to colSpan.
 *
 * Returns an allocated array of row heights. Caller must g_free().
 * Sets *sumHeights to the total of all row heights.
 */
static int *get_row_heights(GEditTable *table, int *sumHeights,
                            int *colWidths) {
    int rowCount = table->rows;
    int *heights = g_new0(int, rowCount);
    int cellBorderExtra = table->cellSpacing +
        2 * (table->cellPadding + (table->border ? 1 : 0));

    for (int ci = 0; ci < table->cellCount; ci++) {
        GEditTableCell *cell = &table->cells[ci];
        int row = cell->row;
        int col = cell->column;
        if (row < 0 || row >= rowCount) continue;
        if (col < 0 || col >= table->columns) continue;

        /* Compute cell width (including column span) */
        int cellWidth = colWidths[col];
        int colSpan = cell->colSpan;
        if (colSpan > 1) {
            int spanEnd = col + colSpan;
            if (spanEnd > table->columns) spanEnd = table->columns;
            for (int i = col + 1; i < spanEnd; i++)
                cellWidth += colWidths[i];
            cellWidth += (colSpan - 1) * cellBorderExtra;
        }

        /* Get cell height */
        int h;
        if (cell->height > 0) {
            h = cell->height;
        } else if (cell->height < 0) {
            /* Percentage height not meaningful without container; use auto */
            h = measure_text_height(cell->text, cellWidth, cell->isHeader);
        } else {
            h = measure_text_height(cell->text, cellWidth, cell->isHeader);
        }

        /* For rowSpan == 1, assign directly; for rowSpan > 1,
         * distribute among the spanned rows later */
        if (cell->rowSpan <= 1) {
            if (h > heights[row]) heights[row] = h;
        }
    }

    /* Second pass: handle rowSpan > 1 */
    for (int ci = 0; ci < table->cellCount; ci++) {
        GEditTableCell *cell = &table->cells[ci];
        if (cell->rowSpan <= 1) continue;

        int row = cell->row;
        int col = cell->column;
        if (row < 0 || row >= rowCount) continue;
        if (col < 0 || col >= table->columns) continue;

        int cellWidth = colWidths[col];
        int colSpan = cell->colSpan;
        if (colSpan > 1) {
            int spanEnd = col + colSpan;
            if (spanEnd > table->columns) spanEnd = table->columns;
            for (int i = col + 1; i < spanEnd; i++)
                cellWidth += colWidths[i];
            cellWidth += (colSpan - 1) * cellBorderExtra;
        }

        int h = cell->height > 0 ? cell->height :
                measure_text_height(cell->text, cellWidth, cell->isHeader);

        /* Sum current heights of spanned rows */
        int rowSpan = cell->rowSpan;
        int spanEnd = row + rowSpan;
        if (spanEnd > rowCount) spanEnd = rowCount;
        int sumSpanned = 0;
        for (int r = row; r < spanEnd; r++)
            sumSpanned += heights[r];
        sumSpanned += (spanEnd - row - 1) * cellBorderExtra;

        if (h > sumSpanned) {
            /* Distribute excess equally */
            int actualSpan = spanEnd - row;
            int addEach = (h - sumSpanned + actualSpan - 1) / actualSpan;
            for (int r = row; r < spanEnd; r++)
                heights[r] += addEach;
        }
    }

    *sumHeights = 0;
    for (int row = 0; row < rowCount; row++)
        *sumHeights += heights[row];
    return heights;
}

/* ================================================================
 * CSS styling helpers
 * ================================================================ */

/* Build a CSS string for the table grid and apply it via a CssProvider. */
static void apply_table_css(GtkWidget *grid, GEditTable *table) {
    GString *css = g_string_new(NULL);

    /* Table-level style */
    g_string_append(css, ".gedit-table {\n");
    if (table->bgColor) {
        g_string_append_printf(css, "  background-color: #%06x;\n", table->bgColor);
    }
    if (table->border > 0) {
        g_string_append_printf(css,
            "  border: %dpx solid #888888;\n", table->border);
    }
    g_string_append(css, "}\n");

    /* Cell base style */
    g_string_append(css, ".gedit-table-cell {\n");
    g_string_append_printf(css, "  padding: %dpx;\n", table->cellPadding);
    if (table->border > 0) {
        g_string_append(css, "  border: 1px solid #aaaaaa;\n");
    }
    g_string_append(css, "}\n");

    /* Header cell style */
    g_string_append(css, ".gedit-table-header {\n");
    g_string_append(css, "  font-weight: bold;\n");
    g_string_append(css, "}\n");

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css->str);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
    g_string_free(css, TRUE);
}

/* Set per-cell inline CSS (background color) */
static void apply_cell_css(GtkWidget *widget, GEditTableCell *cell) {
    if (!cell->bgColor) return;

    char css_buf[128];
    snprintf(css_buf, sizeof(css_buf),
             "* { background-color: #%06x; }", cell->bgColor);

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, css_buf);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);
    g_object_unref(provider);
}

/* ================================================================
 * GtkGrid rendering
 * ================================================================ */

/*
 * Build a GtkGrid from a GEditTable.
 * Each cell becomes a GtkLabel (or bold label for headers) inside the grid.
 * Column/row spans via gtk_grid_attach with span parameters.
 * Alignment via GtkWidget alignment properties.
 * Background color via CSS inline styles.
 */
static GtkWidget *build_grid(GEditTable *table, int availableWidth) {
    GtkWidget *grid = gtk_grid_new();
    gtk_widget_add_css_class(grid, "gedit-table");

    /* Cell spacing via GtkGrid properties */
    gtk_grid_set_row_spacing(GTK_GRID(grid), table->cellSpacing);
    gtk_grid_set_column_spacing(GTK_GRID(grid), table->cellSpacing);

    /* Apply CSS for borders, padding, etc. */
    apply_table_css(grid, table);

    /* Compute table width */
    int tableWidth = availableWidth;
    if (table->width > 0)
        tableWidth = table->width;
    else if (table->width < 0)
        tableWidth = availableWidth * (-table->width) / 100;

    /* Compute column widths */
    int sumWidths = 0;
    int *colWidths = get_col_widths(table, &sumWidths,
                                    tableWidth, table->width != 0);

    /* Compute row heights */
    int sumHeights = 0;
    int *rowHeights = get_row_heights(table, &sumHeights, colWidths);

    /* Set column widths on the grid via minimum width requests */
    for (int col = 0; col < table->columns; col++) {
        /* Insert invisible sizing widgets to enforce column widths */
        GtkWidget *sizer = gtk_label_new(NULL);
        gtk_widget_set_size_request(sizer, colWidths[col], 0);
        gtk_widget_set_visible(sizer, FALSE);
        gtk_grid_attach(GTK_GRID(grid), sizer, col, table->rows, 1, 1);
    }

    /* Populate cells */
    for (int ci = 0; ci < table->cellCount; ci++) {
        GEditTableCell *cell = &table->cells[ci];

        /* Create cell widget */
        GtkWidget *label = gtk_label_new(cell->text);
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
        gtk_widget_add_css_class(label, "gedit-table-cell");

        if (cell->isHeader) {
            gtk_widget_add_css_class(label, "gedit-table-header");
        }

        /* Horizontal alignment */
        switch (cell->hAlign) {
        case GEDIT_TABLE_HALIGN_LEFT:
            gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
            gtk_widget_set_halign(label, GTK_ALIGN_START);
            break;
        case GEDIT_TABLE_HALIGN_CENTER:
            gtk_label_set_xalign(GTK_LABEL(label), 0.5f);
            gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
            break;
        case GEDIT_TABLE_HALIGN_RIGHT:
            gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
            gtk_widget_set_halign(label, GTK_ALIGN_END);
            break;
        case GEDIT_TABLE_HALIGN_JUSTIFY:
            gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_FILL);
            gtk_widget_set_halign(label, GTK_ALIGN_FILL);
            break;
        default:
            /* Default: left for <td>, center for <th> */
            if (cell->isHeader) {
                gtk_label_set_xalign(GTK_LABEL(label), 0.5f);
                gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
            } else {
                gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
                gtk_widget_set_halign(label, GTK_ALIGN_START);
            }
            break;
        }

        /* Vertical alignment */
        switch (cell->vAlign) {
        case GEDIT_TABLE_VALIGN_MIDDLE:
            gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
            break;
        case GEDIT_TABLE_VALIGN_BOTTOM:
            gtk_widget_set_valign(label, GTK_ALIGN_END);
            break;
        default:
            gtk_widget_set_valign(label, GTK_ALIGN_START);
            break;
        }

        /* Cell size: use explicit height if set */
        int cellW = -1; /* let grid handle via column widths */
        int cellH = -1;
        if (cell->height > 0) cellH = cell->height;
        if (cellW > 0 || cellH > 0)
            gtk_widget_set_size_request(label, cellW, cellH);

        /* Make the cell expand to fill its grid space */
        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_vexpand(label, TRUE);

        /* Background color */
        apply_cell_css(label, cell);

        /* Attach to grid with span */
        int colSpan = cell->colSpan > 0 ? cell->colSpan : 1;
        int rowSpan = cell->rowSpan > 0 ? cell->rowSpan : 1;
        gtk_grid_attach(GTK_GRID(grid), label,
                        cell->column, cell->row, colSpan, rowSpan);
    }

    /* Store the table data on the widget for later retrieval */
    g_object_set_data(G_OBJECT(grid), "gedit-table-rows",
                      GINT_TO_POINTER(table->rows));
    g_object_set_data(G_OBJECT(grid), "gedit-table-cols",
                      GINT_TO_POINTER(table->columns));
    g_object_set_data(G_OBJECT(grid), "gedit-table-border",
                      GINT_TO_POINTER(table->border));
    g_object_set_data(G_OBJECT(grid), "gedit-table-cellpadding",
                      GINT_TO_POINTER(table->cellPadding));
    g_object_set_data(G_OBJECT(grid), "gedit-table-cellspacing",
                      GINT_TO_POINTER(table->cellSpacing));

    g_free(colWidths);
    g_free(rowHeights);
    return grid;
}

/* ================================================================
 * Public API: insert table into GtkTextView
 * ================================================================ */

GtkWidget *gedit_table_insert_at(GtkTextView *text_view, GEditTable *table,
                                 GtkTextIter *iter) {
    if (!text_view || !table || table->cellCount == 0) return NULL;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);

    /* Get available width from the text view */
    int availableWidth = gtk_widget_get_width(GTK_WIDGET(text_view));
    if (availableWidth <= 0) availableWidth = 600; /* fallback */

    /* Build the grid widget */
    GtkWidget *grid = build_grid(table, availableWidth);

    /* Create a child anchor in the text buffer */
    GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor(buffer, iter);

    /* Insert the grid as a child widget at the anchor */
    gtk_text_view_add_child_at_anchor(text_view, grid, anchor);

    return grid;
}

GtkWidget *gedit_table_insert(GtkTextView *text_view, GEditTable *table) {
    if (!text_view || !table) return NULL;

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter iter;
    GtkTextMark *insert = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, insert);

    return gedit_table_insert_at(text_view, table, &iter);
}

/* ================================================================
 * HTML parsing helpers
 * ================================================================ */

/* Case-insensitive prefix match */
static bool ci_starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (tolower((unsigned char)*str) != tolower((unsigned char)*prefix))
            return false;
        str++;
        prefix++;
    }
    return true;
}

/* Find the next occurrence of a tag (case-insensitive) in html.
 * Returns pointer to '<' of the found tag, or NULL. */
static const char *find_tag(const char *html, const char *end,
                            const char *tagName) {
    size_t tagLen = strlen(tagName);
    const char *p = html;
    while (p < end) {
        p = memchr(p, '<', end - p);
        if (!p) return NULL;
        if (p + 1 + tagLen <= end) {
            if (ci_starts_with(p + 1, tagName)) {
                char after = p[1 + tagLen];
                if (after == '>' || after == ' ' || after == '/' ||
                    after == '\t' || after == '\n' || after == '\r')
                    return p;
            }
        }
        p++;
    }
    return NULL;
}

/* Find the closing '>' for a tag starting at '<'. */
static const char *find_tag_end(const char *tag, const char *htmlEnd) {
    const char *p = memchr(tag, '>', htmlEnd - tag);
    return p;
}

/* Extract an attribute value from a tag string.
 * tag points to the opening '<', tagEnd to '>'.
 * Returns a newly allocated string, or NULL. Caller frees with g_free(). */
static char *get_attr(const char *tag, const char *tagEnd, const char *attr) {
    size_t attrLen = strlen(attr);
    const char *p = tag;
    while (p < tagEnd) {
        /* Skip whitespace */
        while (p < tagEnd && isspace((unsigned char)*p)) p++;
        if (p >= tagEnd) break;

        /* Check if this is our attribute */
        if (ci_starts_with(p, attr) && (p[attrLen] == '=' || isspace((unsigned char)p[attrLen]))) {
            p += attrLen;
            while (p < tagEnd && isspace((unsigned char)*p)) p++;
            if (p < tagEnd && *p == '=') {
                p++;
                while (p < tagEnd && isspace((unsigned char)*p)) p++;
                if (p < tagEnd) {
                    if (*p == '"' || *p == '\'') {
                        char quote = *p++;
                        const char *start = p;
                        while (p < tagEnd && *p != quote) p++;
                        return g_strndup(start, p - start);
                    } else {
                        const char *start = p;
                        while (p < tagEnd && !isspace((unsigned char)*p) &&
                               *p != '>' && *p != '/')
                            p++;
                        return g_strndup(start, p - start);
                    }
                }
            }
        }
        /* Skip to next whitespace or end */
        while (p < tagEnd && !isspace((unsigned char)*p) && *p != '>') p++;
    }
    return NULL;
}

/* Parse an integer attribute. Returns defaultVal if not found. */
static int get_int_attr(const char *tag, const char *tagEnd,
                        const char *attr, int defaultVal) {
    char *val = get_attr(tag, tagEnd, attr);
    if (!val) return defaultVal;
    int result = atoi(val);
    g_free(val);
    return result;
}

/* Parse a width attribute: can be "100" (pixels), "50%" (percentage).
 * Percentages returned as negative values. */
static int get_width_attr(const char *tag, const char *tagEnd,
                          const char *attr) {
    char *val = get_attr(tag, tagEnd, attr);
    if (!val) return 0;
    int result;
    size_t len = strlen(val);
    if (len > 0 && val[len - 1] == '%') {
        val[len - 1] = '\0';
        result = -atoi(val); /* negative = percentage */
    } else {
        result = atoi(val);
    }
    g_free(val);
    return result;
}

/* Parse bgcolor attribute: "#rrggbb" or simple color names.
 * Returns 0xRRGGBB or 0 if not found/parseable. */
static uint32_t get_color_attr(const char *tag, const char *tagEnd,
                               const char *attr) {
    char *val = get_attr(tag, tagEnd, attr);
    if (!val) return 0;

    uint32_t color = 0;
    if (val[0] == '#' && strlen(val) >= 7) {
        color = (uint32_t)strtoul(val + 1, NULL, 16);
    } else {
        /* Try named colors via GdkRGBA */
        GdkRGBA rgba;
        if (gdk_rgba_parse(&rgba, val)) {
            uint32_t r = (uint32_t)(rgba.red * 255) & 0xFF;
            uint32_t g = (uint32_t)(rgba.green * 255) & 0xFF;
            uint32_t b = (uint32_t)(rgba.blue * 255) & 0xFF;
            color = (r << 16) | (g << 8) | b;
        }
    }
    g_free(val);
    return color;
}

/* Parse align attribute: returns GEDIT_TABLE_HALIGN_* */
static int parse_halign(const char *val) {
    if (!val) return GEDIT_TABLE_HALIGN_DEFAULT;
    if (g_ascii_strcasecmp(val, "left") == 0) return GEDIT_TABLE_HALIGN_LEFT;
    if (g_ascii_strcasecmp(val, "center") == 0) return GEDIT_TABLE_HALIGN_CENTER;
    if (g_ascii_strcasecmp(val, "right") == 0) return GEDIT_TABLE_HALIGN_RIGHT;
    if (g_ascii_strcasecmp(val, "justify") == 0) return GEDIT_TABLE_HALIGN_JUSTIFY;
    return GEDIT_TABLE_HALIGN_DEFAULT;
}

/* Parse valign attribute: returns GEDIT_TABLE_VALIGN_* */
static int parse_valign(const char *val) {
    if (!val) return GEDIT_TABLE_VALIGN_DEFAULT;
    if (g_ascii_strcasecmp(val, "middle") == 0) return GEDIT_TABLE_VALIGN_MIDDLE;
    if (g_ascii_strcasecmp(val, "bottom") == 0) return GEDIT_TABLE_VALIGN_BOTTOM;
    return GEDIT_TABLE_VALIGN_DEFAULT;
}

/* Strip HTML tags from content, keeping only text.
 * Also decode basic entities. Returns newly allocated string. */
static char *strip_html_tags(const char *html, int len) {
    GString *out = g_string_new(NULL);
    const char *p = html;
    const char *end = html + len;

    while (p < end) {
        if (*p == '<') {
            /* Skip tag */
            const char *close = memchr(p, '>', end - p);
            if (close) {
                /* Check for <br> */
                if (ci_starts_with(p + 1, "br"))
                    g_string_append_c(out, '\n');
                p = close + 1;
            } else {
                p++;
            }
        } else if (*p == '&') {
            /* Basic entity decoding */
            if (ci_starts_with(p, "&amp;"))    { g_string_append_c(out, '&');  p += 5; }
            else if (ci_starts_with(p, "&lt;"))  { g_string_append_c(out, '<');  p += 4; }
            else if (ci_starts_with(p, "&gt;"))  { g_string_append_c(out, '>');  p += 4; }
            else if (ci_starts_with(p, "&quot;")){ g_string_append_c(out, '"');  p += 6; }
            else if (ci_starts_with(p, "&nbsp;")){ g_string_append_c(out, ' ');  p += 6; }
            else if (ci_starts_with(p, "&apos;")){ g_string_append_c(out, '\''); p += 6; }
            else if (ci_starts_with(p, "&#")) {
                const char *semi = memchr(p, ';', end - p);
                if (semi && semi - p < 10) {
                    gunichar ch;
                    if (p[2] == 'x' || p[2] == 'X')
                        ch = (gunichar)strtoul(p + 3, NULL, 16);
                    else
                        ch = (gunichar)strtoul(p + 2, NULL, 10);
                    if (ch > 0) {
                        char buf[6];
                        int n = g_unichar_to_utf8(ch, buf);
                        g_string_append_len(out, buf, n);
                    }
                    p = semi + 1;
                } else {
                    g_string_append_c(out, *p++);
                }
            } else {
                g_string_append_c(out, *p++);
            }
        } else {
            g_string_append_c(out, *p++);
        }
    }

    return g_string_free(out, FALSE);
}

/* ================================================================
 * HTML table parser
 * ================================================================ */

GtkWidget *gedit_table_insert_html(GtkTextView *text_view,
                                   const char *html, int htmlLen) {
    if (!text_view || !html || htmlLen <= 0) return NULL;

    const char *end = html + htmlLen;

    /* Find <table ...> */
    const char *tableTag = find_tag(html, end, "table");
    if (!tableTag) return NULL;
    const char *tableTagEnd = find_tag_end(tableTag, end);
    if (!tableTagEnd) return NULL;

    /* Parse table attributes */
    int tblWidth = get_width_attr(tableTag, tableTagEnd, "width");
    int tblBorder = get_int_attr(tableTag, tableTagEnd, "border", 1);
    int tblPadding = get_int_attr(tableTag, tableTagEnd, "cellpadding", 2);
    int tblSpacing = get_int_attr(tableTag, tableTagEnd, "cellspacing", 2);
    uint32_t tblBgColor = get_color_attr(tableTag, tableTagEnd, "bgcolor");

    /* First pass: count rows and max columns */
    int rowCount = 0;
    int maxCols = 0;
    {
        const char *scan = tableTagEnd + 1;
        while (scan < end) {
            const char *tr = find_tag(scan, end, "tr");
            if (!tr) break;

            /* Find </tr> */
            const char *trClose = find_tag(tr + 1, end, "/tr");
            if (!trClose) trClose = end;

            rowCount++;
            int cols = 0;
            const char *cellScan = find_tag_end(tr, end);
            if (cellScan) cellScan++;
            else break;

            while (cellScan && cellScan < trClose) {
                const char *td = find_tag(cellScan, trClose, "td");
                const char *th = find_tag(cellScan, trClose, "th");
                const char *cellTag = NULL;
                if (td && th) cellTag = (td < th) ? td : th;
                else if (td) cellTag = td;
                else if (th) cellTag = th;
                else break;

                const char *cellTagEnd = find_tag_end(cellTag, end);
                if (!cellTagEnd) break;

                int colspan = get_int_attr(cellTag, cellTagEnd, "colspan", 1);
                if (colspan < 1) colspan = 1;
                cols += colspan;
                cellScan = cellTagEnd + 1;
            }
            if (cols > maxCols) maxCols = cols;

            scan = trClose + 1;
        }
    }

    if (rowCount == 0 || maxCols == 0) return NULL;

    /* Create table */
    GEditTable *table = gedit_table_new(rowCount, maxCols);
    if (!table) return NULL;
    table->width = tblWidth;
    table->border = tblBorder;
    table->cellPadding = tblPadding;
    table->cellSpacing = tblSpacing;
    table->bgColor = tblBgColor;

    /* Second pass: parse cells */
    int currentRow = 0;
    const char *scan = tableTagEnd + 1;
    while (scan < end && currentRow < rowCount) {
        const char *tr = find_tag(scan, end, "tr");
        if (!tr) break;
        const char *trTagEnd = find_tag_end(tr, end);
        if (!trTagEnd) break;

        const char *trClose = find_tag(tr + 1, end, "/tr");
        if (!trClose) trClose = end;

        int currentCol = 0;
        const char *cellScan = trTagEnd + 1;

        while (cellScan && cellScan < trClose) {
            const char *td = find_tag(cellScan, trClose, "td");
            const char *th = find_tag(cellScan, trClose, "th");
            const char *cellTag = NULL;
            bool isHeader = false;
            if (td && th) {
                if (td < th) { cellTag = td; isHeader = false; }
                else { cellTag = th; isHeader = true; }
            } else if (td) { cellTag = td; isHeader = false; }
            else if (th) { cellTag = th; isHeader = true; }
            else break;

            const char *cellTagEnd = find_tag_end(cellTag, end);
            if (!cellTagEnd) break;

            /* Parse cell attributes */
            int colspan = get_int_attr(cellTag, cellTagEnd, "colspan", 1);
            int rowspan = get_int_attr(cellTag, cellTagEnd, "rowspan", 1);
            if (colspan < 1) colspan = 1;
            if (rowspan < 1) rowspan = 1;
            int cellWidth = get_width_attr(cellTag, cellTagEnd, "width");
            int cellHeight = get_int_attr(cellTag, cellTagEnd, "height", 0);
            uint32_t cellBg = get_color_attr(cellTag, cellTagEnd, "bgcolor");

            char *alignStr = get_attr(cellTag, cellTagEnd, "align");
            int hAlign = parse_halign(alignStr);
            g_free(alignStr);

            char *valignStr = get_attr(cellTag, cellTagEnd, "valign");
            int vAlign = parse_valign(valignStr);
            g_free(valignStr);

            /* Extract cell text content (between > and next </td> or </th>) */
            const char *contentStart = cellTagEnd + 1;
            const char *cellClose;
            if (isHeader)
                cellClose = find_tag(contentStart, trClose, "/th");
            else
                cellClose = find_tag(contentStart, trClose, "/td");
            if (!cellClose) cellClose = trClose;

            int contentLen = (int)(cellClose - contentStart);
            char *cellText = strip_html_tags(contentStart, contentLen);

            /* Trim leading/trailing whitespace */
            g_strstrip(cellText);

            /* Add cell */
            int idx = gedit_table_add_cell(table, currentRow, currentCol, cellText);
            g_free(cellText);

            if (idx >= 0) {
                gedit_table_cell_set_span(table, idx, rowspan, colspan);
                gedit_table_cell_set_align(table, idx, hAlign, vAlign);
                gedit_table_cell_set_bg(table, idx, cellBg);
                gedit_table_cell_set_header(table, idx, isHeader);
                gedit_table_cell_set_size(table, idx, cellWidth, cellHeight);
            }

            currentCol += colspan;

            /* Move past the closing tag */
            const char *closeEnd = find_tag_end(cellClose, end);
            cellScan = closeEnd ? closeEnd + 1 : cellClose + 1;
        }

        currentRow++;
        const char *trCloseEnd = find_tag_end(trClose, end);
        scan = trCloseEnd ? trCloseEnd + 1 : trClose + 1;
    }

    /* Update actual row/column counts */
    table->rows = rowCount;
    table->columns = maxCols;

    /* Insert the table */
    GtkWidget *grid = gedit_table_insert(text_view, table);
    gedit_table_free(table);
    return grid;
}

/* ================================================================
 * Table from widget (reverse: extract data from GtkGrid)
 * ================================================================ */

GEditTable *gedit_table_from_widget(GtkWidget *grid) {
    if (!grid || !GTK_IS_GRID(grid)) return NULL;
    if (!gtk_widget_has_css_class(grid, "gedit-table")) return NULL;

    int rows = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(grid), "gedit-table-rows"));
    int cols = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(grid), "gedit-table-cols"));
    if (rows <= 0 || cols <= 0) return NULL;

    GEditTable *table = gedit_table_new(rows, cols);
    if (!table) return NULL;

    table->border = GPOINTER_TO_INT(
        g_object_get_data(G_OBJECT(grid), "gedit-table-border"));
    table->cellPadding = GPOINTER_TO_INT(
        g_object_get_data(G_OBJECT(grid), "gedit-table-cellpadding"));
    table->cellSpacing = GPOINTER_TO_INT(
        g_object_get_data(G_OBJECT(grid), "gedit-table-cellspacing"));

    /* Iterate over children of the grid */
    for (GtkWidget *child = gtk_widget_get_first_child(grid);
         child != NULL;
         child = gtk_widget_get_next_sibling(child))
    {
        if (!GTK_IS_LABEL(child)) continue;
        if (!gtk_widget_get_visible(child)) continue; /* skip sizer widgets */

        /* GTK4 does not expose grid child attach properties directly;
         * we query via gtk_grid_query_child */
        int col, row, colSpan, rowSpan;
        gtk_grid_query_child(GTK_GRID(grid), child,
                             &col, &row, &colSpan, &rowSpan);

        const char *text = gtk_label_get_text(GTK_LABEL(child));
        int idx = gedit_table_add_cell(table, row, col, text);
        if (idx >= 0) {
            gedit_table_cell_set_span(table, idx, rowSpan, colSpan);

            bool isHeader = gtk_widget_has_css_class(child, "gedit-table-header");
            gedit_table_cell_set_header(table, idx, isHeader);

            /* Extract alignment from widget properties */
            GtkAlign ha = gtk_widget_get_halign(child);
            int hAlign = GEDIT_TABLE_HALIGN_DEFAULT;
            if (ha == GTK_ALIGN_START) hAlign = GEDIT_TABLE_HALIGN_LEFT;
            else if (ha == GTK_ALIGN_CENTER) hAlign = GEDIT_TABLE_HALIGN_CENTER;
            else if (ha == GTK_ALIGN_END) hAlign = GEDIT_TABLE_HALIGN_RIGHT;
            else if (ha == GTK_ALIGN_FILL) hAlign = GEDIT_TABLE_HALIGN_JUSTIFY;

            GtkAlign va = gtk_widget_get_valign(child);
            int vAlign = GEDIT_TABLE_VALIGN_DEFAULT;
            if (va == GTK_ALIGN_CENTER) vAlign = GEDIT_TABLE_VALIGN_MIDDLE;
            else if (va == GTK_ALIGN_END) vAlign = GEDIT_TABLE_VALIGN_BOTTOM;

            gedit_table_cell_set_align(table, idx, hAlign, vAlign);
        }
    }

    return table;
}

/* ================================================================
 * Export table to HTML
 * ================================================================ */

char *gedit_table_to_html(GEditTable *table) {
    if (!table) return NULL;

    GString *html = g_string_new("<table");

    if (table->width > 0)
        g_string_append_printf(html, " width=\"%d\"", table->width);
    else if (table->width < 0)
        g_string_append_printf(html, " width=\"%d%%\"", -table->width);

    if (table->border >= 0)
        g_string_append_printf(html, " border=\"%d\"", table->border);
    if (table->cellPadding > 0)
        g_string_append_printf(html, " cellpadding=\"%d\"", table->cellPadding);
    if (table->cellSpacing > 0)
        g_string_append_printf(html, " cellspacing=\"%d\"", table->cellSpacing);
    if (table->bgColor)
        g_string_append_printf(html, " bgcolor=\"#%06x\"", table->bgColor);

    g_string_append(html, ">\n");

    /* Organize cells by row */
    for (int row = 0; row < table->rows; row++) {
        g_string_append(html, "  <tr>\n");
        for (int ci = 0; ci < table->cellCount; ci++) {
            GEditTableCell *cell = &table->cells[ci];
            if (cell->row != row) continue;

            const char *tag = cell->isHeader ? "th" : "td";
            g_string_append_printf(html, "    <%s", tag);

            if (cell->colSpan > 1)
                g_string_append_printf(html, " colspan=\"%d\"", cell->colSpan);
            if (cell->rowSpan > 1)
                g_string_append_printf(html, " rowspan=\"%d\"", cell->rowSpan);

            if (cell->width > 0)
                g_string_append_printf(html, " width=\"%d\"", cell->width);
            else if (cell->width < 0)
                g_string_append_printf(html, " width=\"%d%%\"", -cell->width);
            if (cell->height > 0)
                g_string_append_printf(html, " height=\"%d\"", cell->height);

            switch (cell->hAlign) {
            case GEDIT_TABLE_HALIGN_LEFT:    g_string_append(html, " align=\"left\""); break;
            case GEDIT_TABLE_HALIGN_CENTER:  g_string_append(html, " align=\"center\""); break;
            case GEDIT_TABLE_HALIGN_RIGHT:   g_string_append(html, " align=\"right\""); break;
            case GEDIT_TABLE_HALIGN_JUSTIFY: g_string_append(html, " align=\"justify\""); break;
            default: break;
            }

            switch (cell->vAlign) {
            case GEDIT_TABLE_VALIGN_MIDDLE: g_string_append(html, " valign=\"middle\""); break;
            case GEDIT_TABLE_VALIGN_BOTTOM: g_string_append(html, " valign=\"bottom\""); break;
            default: break;
            }

            if (cell->bgColor)
                g_string_append_printf(html, " bgcolor=\"#%06x\"", cell->bgColor);

            g_string_append_c(html, '>');

            /* Escape cell text for HTML */
            if (cell->text) {
                for (const char *t = cell->text; *t; t++) {
                    switch (*t) {
                    case '&': g_string_append(html, "&amp;"); break;
                    case '<': g_string_append(html, "&lt;"); break;
                    case '>': g_string_append(html, "&gt;"); break;
                    case '"': g_string_append(html, "&quot;"); break;
                    default:  g_string_append_c(html, *t); break;
                    }
                }
            }

            g_string_append_printf(html, "</%s>\n", tag);
        }
        g_string_append(html, "  </tr>\n");
    }

    g_string_append(html, "</table>\n");

    return g_string_free(html, FALSE);
}
