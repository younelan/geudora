#include "gedit-print.h"
#include "gedit-state.h"
#include <pango/pangocairo.h>

typedef struct {
  geditDocument *doc;
  gchar *title;
  gchar **lines;
  gint num_lines;
  gdouble line_height;
  PangoContext *pango_ctx;
  PangoFontDescription *font;
} PrintContext;

static void gedit_print_draw_page(GtkPrintOperation *operation,
                                  GtkPrintContext *print_context, gint page_num,
                                  gpointer user_data) {
  (void)operation;
  PrintContext *ctx = (PrintContext *)user_data;
  cairo_t *cr = gtk_print_context_get_cairo_context(print_context);
  
  gdouble page_width = gtk_print_context_get_width(print_context);
  gdouble page_height = gtk_print_context_get_height(print_context);
  
  /* Set up margins */
  gdouble margin_left = 36.0;   /* 0.5 inch */
  gdouble margin_right = 36.0;
  gdouble margin_top = 36.0;
  gdouble margin_bottom = 36.0;
  
  gdouble content_width = page_width - margin_left - margin_right;
  gdouble content_height = page_height - margin_top - margin_bottom;
  
  /* Create layout using our pre-created Pango context (not print context) */
  PangoLayout *layout = pango_layout_new(ctx->pango_ctx);
  pango_layout_set_font_description(layout, ctx->font);
  pango_layout_set_width(layout, (int)(content_width * PANGO_SCALE));
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  
  gint lines_per_page = (gint)(content_height / ctx->line_height);
  if (lines_per_page <= 0)
    lines_per_page = 1;
  
  gint start_line = page_num * lines_per_page;
  gint end_line = MIN(start_line + lines_per_page, ctx->num_lines);
  
  /* Draw header */
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_font_size(cr, 10);
  gchar *header_text = g_strdup_printf("%s - Page %d", ctx->title, page_num + 1);
  cairo_move_to(cr, margin_left, margin_top - 20);
  cairo_show_text(cr, header_text);
  g_free(header_text);
  
  /* Draw content lines */
  gdouble y = margin_top;
  for (gint i = start_line; i < end_line; i++) {
    pango_layout_set_text(layout, ctx->lines[i], -1);
    cairo_move_to(cr, margin_left, y);
    pango_cairo_show_layout(cr, layout);
    y += ctx->line_height;
  }
  
  /* Draw footer */
  cairo_set_font_size(cr, 9);
  gchar *footer_text = g_strdup_printf("%d", page_num + 1);
  cairo_move_to(cr, page_width / 2 - 10, page_height - margin_bottom + 10);
  cairo_show_text(cr, footer_text);
  g_free(footer_text);
  
  /* Clean up layout (but NOT the context or font - those are in ctx) */
  g_object_unref(layout);
}

static gint gedit_print_get_num_pages(GtkPrintOperation *operation,
                                      GtkPrintContext *print_context,
                                      gpointer user_data) {
  PrintContext *ctx = (PrintContext *)user_data;
  
  gdouble page_height = gtk_print_context_get_height(print_context);
  gdouble margin_top = 36.0;
  gdouble margin_bottom = 36.0;
  gdouble content_height = page_height - margin_top - margin_bottom;
  
  gint lines_per_page = (gint)(content_height / ctx->line_height);
  if (lines_per_page <= 0)
    lines_per_page = 1;
  
  gint num_pages = (ctx->num_lines + lines_per_page - 1) / lines_per_page;
  
  g_print("gedit: Print - %d lines, %d lines per page, %d pages\n", ctx->num_lines,
          lines_per_page, num_pages);
  
  return num_pages;
}

static void gedit_print_begin(GtkPrintOperation *operation,
                              GtkPrintContext *print_context, gpointer user_data) {
  PrintContext *ctx = (PrintContext *)user_data;
  
  /* Create a Pango context from the print context's Cairo context */
  PangoFontMap *fontmap = pango_cairo_font_map_get_default();
  ctx->pango_ctx = pango_font_map_create_context(fontmap);
  
  /* Set up font */
  ctx->font = pango_font_description_from_string("Monospace 11");
  
  /* Calculate line height using a temporary layout */
  PangoLayout *temp_layout = pango_layout_new(ctx->pango_ctx);
  pango_layout_set_font_description(temp_layout, ctx->font);
  pango_layout_set_text(temp_layout, "X", -1);
  
  PangoRectangle ink_rect, logical_rect;
  pango_layout_get_extents(temp_layout, &ink_rect, &logical_rect);
  ctx->line_height = logical_rect.height / (gdouble)PANGO_SCALE;
  
  g_object_unref(temp_layout);
  
  /* Calculate number of pages */
  gint num_pages = gedit_print_get_num_pages(operation, print_context, user_data);
  gtk_print_operation_set_n_pages(operation, num_pages);
  
  g_print("gedit: Print begin - %d pages\n", num_pages);
}

static void gedit_print_done(GtkPrintOperation *operation,
                             GtkPrintOperationResult result, gpointer user_data) {
  PrintContext *ctx = (PrintContext *)user_data;
  (void)operation;
  
  if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
    GError *error = NULL;
    gtk_print_operation_get_error(GTK_PRINT_OPERATION(operation), &error);
    if (error) {
      g_printerr("gedit: Print error: %s\n", error->message);
      g_error_free(error);
    }
  } else if (result == GTK_PRINT_OPERATION_RESULT_APPLY) {
    g_print("gedit: Print completed successfully\n");
  } else if (result == GTK_PRINT_OPERATION_RESULT_CANCEL) {
    g_print("gedit: Print cancelled\n");
  }
  
  /* Clean up */
  if (ctx->lines) {
    for (gint i = 0; i < ctx->num_lines; i++) {
      g_free(ctx->lines[i]);
    }
    g_free(ctx->lines);
  }
  g_free(ctx->title);
  
  /* Clean up Pango resources */
  if (ctx->pango_ctx) {
    g_object_unref(ctx->pango_ctx);
  }
  if (ctx->font) {
    pango_font_description_free(ctx->font);
  }
  
  g_free(ctx);
}

void gedit_print_document(GtkWidget *parent_window, geditDocument *doc,
                          const gchar *document_title) {
  g_return_if_fail(gedit_DOCUMENT(doc));
  
  /* Get document text */
  gchar *full_text = gedit_document_get_text(doc);
  if (!full_text) {
    g_warning("gedit: No text to print");
    return;
  }
  
  /* Split text into lines */
  gchar **lines = g_strsplit(full_text, "\n", -1);
  gint num_lines = 0;
  for (gint i = 0; lines[i] != NULL; i++) {
    num_lines++;
  }
  
  g_print("gedit: Printing document with %d lines\n", num_lines);
  
  /* Create print context */
  PrintContext *ctx = g_new0(PrintContext, 1);
  ctx->doc = doc;
  ctx->title = g_strdup(document_title ? document_title : "Document");
  ctx->lines = lines;
  ctx->num_lines = num_lines;
  
  /* Create print operation */
  GtkPrintOperation *print_op = gtk_print_operation_new();
  gtk_print_operation_set_allow_async(print_op, FALSE);
  
  /* Connect signals */
  g_signal_connect(print_op, "begin-print", G_CALLBACK(gedit_print_begin), ctx);
  g_signal_connect(print_op, "draw-page", G_CALLBACK(gedit_print_draw_page), ctx);
  g_signal_connect(print_op, "done", G_CALLBACK(gedit_print_done), ctx);
  
  /* Export to PDF instead of preview (avoids app launch issues on macOS) */
  GtkWindow *parent = GTK_WINDOW(gtk_widget_get_root(parent_window));
  
  /* Set default PDF filename */
  gchar *pdf_filename = g_strdup_printf("%s.pdf", document_title ? document_title : "document");
  gtk_print_operation_set_export_filename(print_op, pdf_filename);
  
  GtkPrintOperationResult result = gtk_print_operation_run(
      print_op, GTK_PRINT_OPERATION_ACTION_EXPORT, parent, NULL);
  
  if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
    g_printerr("gedit: Print operation failed\n");
  } else if (result == GTK_PRINT_OPERATION_RESULT_APPLY) {
    g_print("gedit: PDF exported to %s\n", pdf_filename);
  } else if (result == GTK_PRINT_OPERATION_RESULT_CANCEL) {
    g_print("gedit: Print operation cancelled\n");
  }
  
  g_free(pdf_filename);
  g_object_unref(print_op);
  g_free(full_text);
}
