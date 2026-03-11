#ifndef gedit_DOCUMENT_H
#define gedit_DOCUMENT_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _geditDocument geditDocument;
typedef struct _geditDocumentClass geditDocumentClass;

typedef struct {
  gint offset; /* char offset */
  gint width;  /* pixels */
  gint height; /* pixels */
  GdkPixbuf *texture;
} geditGraphic;

typedef struct {
  gint offset; /* char offset */
  gint length; /* number of chars */
  gboolean bold;
  gboolean italic;
  gboolean underline;
  GdkRGBA color;
  gint font_size;      /* points, 0 == default */
  gboolean is_graphic; /* TRUE if this style run is a graphic */
  geditGraphic *graphic;
} geditStyleRun;

typedef enum {
  gedit_ALIGN_LEFT,
  gedit_ALIGN_CENTER,
  gedit_ALIGN_RIGHT
} geditAlignment;

typedef enum {
  gedit_DIR_LTR = 0,  /* Left-to-right */
  gedit_DIR_RTL = 1   /* Right-to-left */
} geditDirection;

typedef enum {
  gedit_BULLET_NONE = 0,
  gedit_BULLET_CIRCLE = 1,
  gedit_BULLET_SQUARE = 2,
  gedit_BULLET_DISK = 3
} geditBulletType;

typedef enum {
  gedit_PARA_ATTR_ALIGNMENT = 1 << 0,
  gedit_PARA_ATTR_INDENT = 1 << 1,
  gedit_PARA_ATTR_BULLET = 1 << 2,
  gedit_PARA_ATTR_QUOTE_LEVEL = 1 << 3,
  gedit_PARA_ATTR_HR = 1 << 4,
  gedit_PARA_ATTR_TAB_STOPS = 1 << 5,
  gedit_PARA_ATTR_DIRECTION = 1 << 6,
  gedit_PARA_ATTR_PAGE_BREAK = 1 << 7
} geditParaAttrFlags;

typedef struct {
  gint position; /* pixels from left */
} geditTabStop;

typedef struct {
  gint offset; /* char offset */
  gint length; /* chars */
  geditAlignment alignment;
  gint indent; /* pixels */
  geditBulletType bullet; /* bullet type (0 = none) */
  gint quote_level;
  gboolean is_hr; /* horizontal rule */
  gboolean page_break; /* page break / form feed */
  GArray *tab_stops; /* array of geditTabStop */
  geditDirection direction; /* LTR or RTL */
  guint32 mask; /* bits from geditParaAttrFlags */
} geditParaAttr;

#define gedit_TYPE_DOCUMENT (gedit_document_get_type())
#define gedit_DOCUMENT(obj)                                                    \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), gedit_TYPE_DOCUMENT, geditDocument))

GType gedit_document_get_type(void) G_GNUC_CONST;
geditDocument *gedit_document_new(void);
GtkTextBuffer *gedit_document_get_buffer(geditDocument *self);
void gedit_document_insert_text(geditDocument *self, gint offset,
                                const gchar *text);
void gedit_document_delete_range(geditDocument *self, gint offset, gint length);
gchar *gedit_document_get_text(geditDocument *self);
gint gedit_document_get_length(geditDocument *self);
/* Return a newly-allocated UTF-8 string containing the substring of the
 * document starting at `offset` for `length` characters. Caller must free
 * with g_free(). */
gchar *gedit_document_get_text_range(geditDocument *self, gint offset,
                                     gint length);

/* Style run APIs */
void gedit_document_add_style_run(geditDocument *self, gint offset, gint length,
                                  gboolean bold, gboolean italic,
                                  gboolean underline, const GdkRGBA *color,
                                  gint font_size);
void gedit_document_insert_graphic(geditDocument *self, gint offset,
                                   GdkPixbuf *texture, gint width,
                                   gint height);
void gedit_document_toggle_style(geditDocument *self, gint offset, gint length,
                                 gboolean bold, gboolean italic,
                                 gboolean underline);
void gedit_document_get_style_at(geditDocument *self, gint offset,
                                 geditStyleRun *out_style);
PangoAttrList *gedit_document_get_attr_list(geditDocument *self);
/* Paragraph-level APIs */
void gedit_document_set_alignment(geditDocument *self, gint offset, gint length,
                                  geditAlignment align);
void gedit_document_toggle_bullet(geditDocument *self, gint offset,
                                  gint length);
void gedit_document_indent(geditDocument *self, gint offset, gint length,
                           gint delta_pixels);
void gedit_document_set_quote_level(geditDocument *self, gint offset,
                                    gint length, gint level);
/* Horizontal rule: insert a paragraph-level rule */
void gedit_document_insert_hr(geditDocument *self, gint offset);
/* Tab stops: set tab stops for a paragraph range */
void gedit_document_set_tab_stops(geditDocument *self, gint offset, gint length,
                                  GArray *tab_stops);
/* Remove paragraph attributes in a range (used when deleting text) */
void gedit_document_remove_para_attrs_in_range(geditDocument *self, gint offset,
                                               gint length);

/* Navigation helpers */
gint gedit_document_find_word_boundary_left(geditDocument *self, gint offset);
gint gedit_document_find_word_boundary_right(geditDocument *self, gint offset);
gint gedit_document_find_line_start(geditDocument *self, gint offset);
gint gedit_document_find_line_end(geditDocument *self, gint offset);
/* Set text direction (LTR or RTL) for a paragraph range */
void gedit_document_set_direction(geditDocument *self, gint offset, gint length,
                                  geditDirection direction);
/* Insert a page break at the current offset */
void gedit_document_insert_page_break(geditDocument *self, gint offset);
/* Get word boundaries for intelligent cut/paste */
void gedit_document_get_word_bounds(geditDocument *self, gint offset,
                                    gint *out_start, gint *out_end);
/* Check if character at offset is whitespace or punctuation */
gboolean gedit_document_is_word_boundary_char(geditDocument *self, gint offset);
/* Find text in document, returns offset or -1 if not found */
gint gedit_document_find_text(geditDocument *self, const gchar *search_text,
                              gint start_offset, gint end_offset,
                              gboolean case_sensitive);
/* Replace text in document, returns number of replacements */
gint gedit_document_replace_text(geditDocument *self, const gchar *search_text,
                                 const gchar *replace_text, gint start_offset,
                                 gint end_offset, gboolean replace_all,
                                 gboolean case_sensitive);
/* get attr list for a specific paragraph range (char offsets) */
PangoAttrList *gedit_document_get_attr_list_for_range(geditDocument *self,
                                                      gint range_offset,
                                                      gint range_length);
/* Get merged paragraph attributes for a given char range (fills out_attr). */
void gedit_document_get_para_attr(geditDocument *self, gint offset, gint length,
                                  geditParaAttr *out_attr);

GList *gedit_document_get_style_runs(geditDocument *self);

/* Undo/redo */
void gedit_document_undo(geditDocument *self);
void gedit_document_redo(geditDocument *self);

/* Batch paste: insert text with styles as a single undo operation */
void gedit_document_paste_formatted(geditDocument *self, gint offset,
                                    const gchar *text, GList *style_runs);

/* Copy a range of text+style runs from `src` to `dst` at `dst_offset`.
 * If `preserve_labels` is FALSE, paragraph-level label attributes may be
 * ignored by the implementation. Returns 0 on success or non-zero on error.
 */
gint gedit_document_copy_range(geditDocument *src, gint src_offset,
                               gint src_length, geditDocument *dst,
                               gint dst_offset, gboolean preserve_labels);

/* Insert a small subset of HTML-like markup into the document at `offset`.
 * Supported tags: <b>, <i>, <u>, <color=#rrggbb>, <hr/>
 * The function inserts plain text (tags stripped) and creates style runs
 * for the ranges covered by tags.
 */
void gedit_document_insert_markup(geditDocument *self, gint offset,
                                  const gchar *markup);

G_END_DECLS

#endif /* gedit_DOCUMENT_H */
