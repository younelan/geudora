#include "gedit-document.h"
#include "gedit-state.h"
#include <gdk/gdkcairo.h>
#include <pango/pangocairo.h>
#include <string.h>

typedef struct {
  gchar *text;
  GList *style_runs; /* deep copy of geditStyleRun* */
  GList *para_attrs; /* deep copy of geditParaAttr* */
} geditSnapshot;

struct _geditDocument {
  GObject parent_instance;
  GtkTextBuffer *buffer;
  GQueue *undo_stack; /* stores geditSnapshot* snapshots */
  GQueue *redo_stack; /* stores geditSnapshot* snapshots for redo */
  GMutex mutex;
  GList *style_runs; /* list of geditStyleRun* */
  GList *para_attrs; /* list of geditParaAttr* */
};

struct _geditDocumentClass {
  GObjectClass parent_class;
};

enum { DOCUMENT_CHANGED, SELECTION_CHANGED, LAST_SIGNAL };

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(geditDocument, gedit_document, G_TYPE_OBJECT)

/* Forward declarations for internal helpers */
static geditParaAttr *gedit_para_attr_copy(const geditParaAttr *src);
static geditStyleRun *gedit_style_run_copy(const geditStyleRun *src);
static void gedit_style_run_free(geditStyleRun *run);
static void get_para_attr_unlocked(geditDocument *self, gint offset,
                                   gint length, geditParaAttr *out_attr);
static void add_style_run_unlocked(geditDocument *self, gint offset,
                                   gint length, gint bold, gint italic,
                                   gint underline, const GdkRGBA *color,
                                   gint font_size);

static geditParaAttr *gedit_para_attr_copy(const geditParaAttr *src) {
  geditParaAttr *p = g_new0(geditParaAttr, 1);
  memcpy(p, src, sizeof(geditParaAttr));
  if (src->tab_stops) {
    p->tab_stops = g_array_new(FALSE, FALSE, sizeof(geditTabStop));
    g_array_append_vals(p->tab_stops, src->tab_stops->data, src->tab_stops->len);
  }
  return p;
}

static geditGraphic *gedit_graphic_copy(const geditGraphic *src) {
  geditGraphic *g = g_new0(geditGraphic, 1);
  memcpy(g, src, sizeof(geditGraphic));
  if (g->texture)
    g_object_ref(g->texture);
  return g;
}

static void gedit_graphic_free(geditGraphic *g) {
  if (g->texture)
    g_object_unref(g->texture);
  g_free(g);
}

static geditSnapshot *gedit_snapshot_new(geditDocument *self) {
  geditSnapshot *snap = g_new0(geditSnapshot, 1);
  snap->text = gedit_document_get_text(self);
  for (GList *l = self->style_runs; l; l = l->next) {
    snap->style_runs =
        g_list_append(snap->style_runs, gedit_style_run_copy(l->data));
  }
  for (GList *l = self->para_attrs; l; l = l->next) {
    snap->para_attrs =
        g_list_append(snap->para_attrs, gedit_para_attr_copy(l->data));
  }
  return snap;
}

static void gedit_snapshot_free(geditSnapshot *snap) {
  if (!snap)
    return;
  g_free(snap->text);
  g_list_free_full(snap->style_runs, (GDestroyNotify)gedit_style_run_free);
  g_list_free_full(snap->para_attrs, g_free);
  g_free(snap);
}

static void gedit_snapshot_restore(geditDocument *self, geditSnapshot *snap) {
  if (!self || !snap)
    return;

  /* restore text */
  gtk_text_buffer_set_text(self->buffer, snap->text ? snap->text : "", -1);

  /* restore style runs (deep copy) */
  g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
  self->style_runs = NULL;
  for (GList *l = snap->style_runs; l; l = l->next) {
    self->style_runs =
        g_list_append(self->style_runs, gedit_style_run_copy(l->data));
  }

  /* restore para attrs (deep copy) */
  g_list_free_full(self->para_attrs, g_free);
  self->para_attrs = NULL;
  for (GList *l = snap->para_attrs; l; l = l->next) {
    self->para_attrs =
        g_list_append(self->para_attrs, gedit_para_attr_copy(l->data));
  }
}

static void gedit_document_finalize(GObject *object) {
  geditDocument *self = gedit_DOCUMENT(object);
  if (self->buffer)
    g_object_unref(self->buffer);
  if (self->undo_stack)
    g_queue_free_full(self->undo_stack, (GDestroyNotify)gedit_snapshot_free);
  if (self->redo_stack)
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
  if (self->style_runs) {
    g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
    self->style_runs = NULL;
  }
  if (self->para_attrs) {
    g_list_free_full(self->para_attrs, g_free);
    self->para_attrs = NULL;
  }
  g_mutex_clear(&self->mutex);
  G_OBJECT_CLASS(gedit_document_parent_class)->finalize(object);
}

static void gedit_document_class_init(geditDocumentClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = gedit_document_finalize;

  signals[DOCUMENT_CHANGED] = g_signal_new(
      "document-changed", G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST, 0,
      NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  signals[SELECTION_CHANGED] = g_signal_new(
      "selection-changed", G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
      0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void gedit_document_init(geditDocument *self) {
  self->buffer = gtk_text_buffer_new(NULL);
  g_object_ref(self->buffer);
  self->undo_stack = g_queue_new();
  self->redo_stack = g_queue_new();
  g_mutex_init(&self->mutex);
  self->style_runs = NULL;
  self->para_attrs = NULL;
}

geditDocument *gedit_document_new(void) {
  return g_object_new(gedit_TYPE_DOCUMENT, NULL);
}

GtkTextBuffer *gedit_document_get_buffer(geditDocument *self) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);
  return self->buffer;
}

/* Very simple undo: snapshot full buffer text before change. */
void gedit_document_insert_text(geditDocument *self, gint offset,
                                const gchar *text) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  /* New edit invalidates the redo stack */
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &iter, offset);
  gtk_text_buffer_insert(self->buffer, &iter, text, -1);

  /* adjust style run offsets for inserted text (chars) */
  if (text && *text) {
    gint inserted_chars = g_utf8_strlen(text, -1);
    for (GList *l = self->style_runs; l; l = l->next) {
      geditStyleRun *run = (geditStyleRun *)l->data;
      if (run->offset >= offset)
        run->offset += inserted_chars;
    }
  }

  g_mutex_unlock(&self->mutex);

  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_delete_range(geditDocument *self, gint offset,
                                 gint length) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  /* New edit invalidates the redo stack */
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  GtkTextIter start, end;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &start, offset);
  gtk_text_buffer_get_iter_at_offset(self->buffer, &end, offset + length);

  gtk_text_buffer_delete(self->buffer, &start, &end);

  /* adjust or remove style runs that overlap the deleted range */
  if (length > 0) {
    GList *l = self->style_runs;
    while (l) {
      geditStyleRun *run = (geditStyleRun *)l->data;
      gint run_start = run->offset;
      gint run_end = run->offset + run->length;
      if (run_end <= offset) {
        l = l->next; /* before deletion, unchanged */
      } else if (run_start >= offset + length) {
        /* shift earlier runs left */
        run->offset -= length;
        l = l->next;
      } else {
        /* overlapping run: remove it */
        GList *next = l->next;
        self->style_runs = g_list_delete_link(self->style_runs, l);
        gedit_style_run_free(run); // Use the specific free function
        l = next;
      }
    }

    /* Remove paragraph attributes that overlap the deleted range */
    l = self->para_attrs;
    while (l) {
      geditParaAttr *p = (geditParaAttr *)l->data;
      gint a = p->offset;
      gint b = p->offset + p->length;
      if (b <= offset || a >= offset + length) {
        /* no overlap, but adjust offset if after deletion */
        if (a >= offset + length) {
          p->offset -= length;
        }
        l = l->next;
      } else {
        /* overlapping attr: remove it */
        GList *next = l->next;
        self->para_attrs = g_list_delete_link(self->para_attrs, l);
        if (p->tab_stops)
          g_array_unref(p->tab_stops);
        g_free(p);
        l = next;
      }
    }
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_undo(geditDocument *self) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  if (!self->undo_stack || g_queue_is_empty(self->undo_stack)) {
    g_mutex_unlock(&self->mutex);
    return;
  }

  /* current state -> redo */
  geditSnapshot *cur = gedit_snapshot_new(self);
  geditSnapshot *prev = g_queue_pop_head(self->undo_stack);

  if (prev) {
    /* set doc to previous state */
    gedit_snapshot_restore(self, prev);
    /* save current state for redo */
    g_queue_push_head(self->redo_stack, cur);
    gedit_snapshot_free(prev);
  } else {
    gedit_snapshot_free(cur);
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_redo(geditDocument *self) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  if (!self->redo_stack || g_queue_is_empty(self->redo_stack)) {
    g_mutex_unlock(&self->mutex);
    return;
  }

  /* current state -> undo */
  geditSnapshot *cur = gedit_snapshot_new(self);
  geditSnapshot *next = g_queue_pop_head(self->redo_stack);

  if (next) {
    gedit_snapshot_restore(self, next);
    g_queue_push_head(self->undo_stack, cur);
    gedit_snapshot_free(next);
  } else {
    gedit_snapshot_free(cur);
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

/* Paste formatted text with styles as a single atomic undo operation */
void gedit_document_paste_formatted(geditDocument *self, gint offset,
                                    const gchar *text, GList *style_runs) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (!text || !*text)
    return;

  g_mutex_lock(&self->mutex);

  /* Create single undo snapshot BEFORE any modifications */
  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* Insert text into buffer */
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &iter, offset);
  gtk_text_buffer_insert(self->buffer, &iter, text, -1);

  /* Adjust existing style run offsets for inserted text */
  gint inserted_chars = g_utf8_strlen(text, -1);
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    if (run->offset >= offset)
      run->offset += inserted_chars;
  }

  /* Add pasted style runs */
  for (GList *l = style_runs; l; l = l->next) {
    geditStyleRun *src_run = (geditStyleRun *)l->data;
    geditStyleRun *new_run = g_new0(geditStyleRun, 1);
    memcpy(new_run, src_run, sizeof(geditStyleRun));
    new_run->offset = offset + src_run->offset;
    if (new_run->is_graphic && new_run->graphic) {
      new_run->graphic = gedit_graphic_copy(new_run->graphic);
    }
    self->style_runs = g_list_append(self->style_runs, new_run);
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

gchar *gedit_document_get_text_range(geditDocument *self, gint offset,
                                     gint length) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);
  if (length <= 0)
    return g_strdup("");

  GtkTextIter start, end;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &start, offset);
  gtk_text_buffer_get_iter_at_offset(self->buffer, &end, offset + length);
  return gtk_text_buffer_get_text(self->buffer, &start, &end, FALSE);
}

gint gedit_document_copy_range(geditDocument *src, gint src_offset,
                               gint src_length, geditDocument *dst,
                               gint dst_offset, gboolean preserve_labels) {
  g_return_val_if_fail(gedit_DOCUMENT(src) && gedit_DOCUMENT(dst), -1);

  /* Extract text from src */
  gchar *text = gedit_document_get_text_range(src, src_offset, src_length);
  if (!text)
    return -1;

  /* Build a list of style runs overlapping the requested range, adjusting
   * offsets to be relative to the start of the copied text.
   */
  GList *runs = NULL;
  for (GList *l = src->style_runs; l; l = l->next) {
    geditStyleRun *r = (geditStyleRun *)l->data;
    gint run_start = r->offset;
    gint run_end = r->offset + r->length;
    gint copy_start = src_offset;
    gint copy_end = src_offset + src_length;
    if (run_end <= copy_start || run_start >= copy_end)
      continue; /* no overlap */
    gint off = MAX(run_start, copy_start) - copy_start;
    gint len = MIN(run_end, copy_end) - MAX(run_start, copy_start);
    geditStyleRun *nr = g_new0(geditStyleRun, 1);
    memcpy(nr, r, sizeof(geditStyleRun));
    nr->offset = off;
    nr->length = len;
    if (nr->is_graphic && nr->graphic)
      nr->graphic = gedit_graphic_copy(nr->graphic);
    runs = g_list_append(runs, nr);
  }

  /* Paste into destination as a single undoable operation */
  gedit_document_paste_formatted(dst, dst_offset, text, runs);

  /* free temporary text and runs (the paste makes copies as needed) */
  g_free(text);
  g_list_free_full(runs, (GDestroyNotify)gedit_style_run_free);

  return 0;
}

/* Resolve effective style at offset by walking existing runs (caller holds lock) */
static void get_style_at_unlocked(geditDocument *self, gint offset,
                                  geditStyleRun *out) {
  out->offset = offset;
  out->length = 0;
  out->bold = FALSE;
  out->italic = FALSE;
  out->underline = FALSE;
  gdk_rgba_parse(&out->color, "#000000");
  out->font_size = 0;
  out->is_graphic = FALSE;
  out->graphic = NULL;
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *r = l->data;
    if (offset >= r->offset && offset < r->offset + r->length) {
      if (r->bold) out->bold = TRUE;
      if (r->italic) out->italic = TRUE;
      if (r->underline) out->underline = TRUE;
      if (r->font_size > 0) out->font_size = r->font_size;
      out->color = r->color;
      if (r->is_graphic) { out->is_graphic = TRUE; out->graphic = r->graphic; }
    }
  }
}

/* Compare helper for sorting style runs by offset */
static gint style_run_cmp_offset(gconstpointer a, gconstpointer b) {
  const geditStyleRun *ra = a, *rb = b;
  return ra->offset - rb->offset;
}

/* Merge a style change into existing runs, preserving attributes not being changed.
 * bold/italic/underline: -1 = preserve existing, 0 = set FALSE, 1 = set TRUE
 * color: NULL = preserve existing per-run colors
 * font_size: -1 = preserve existing, 0+ = set absolute value */
static void add_style_run_unlocked(geditDocument *self, gint offset,
                                   gint length, gint bold, gint italic,
                                   gint underline, const GdkRGBA *color,
                                   gint font_size) {
  if (length <= 0) return;
  gint range_end = offset + length;
  GList *new_runs = NULL;
  GList *mid_runs = NULL; /* merged runs within the target range */

  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = l->data;
    gint rs = run->offset, re = rs + run->length;

    if (re <= offset || rs >= range_end) {
      /* No overlap — keep unchanged */
      new_runs = g_list_append(new_runs, gedit_style_run_copy(run));
      continue;
    }

    /* Prefix before the target range */
    if (rs < offset) {
      geditStyleRun *pre = gedit_style_run_copy(run);
      pre->length = offset - rs;
      new_runs = g_list_append(new_runs, pre);
    }

    /* Overlapping middle — copy existing run then merge new attributes */
    gint ms = MAX(rs, offset), me = MIN(re, range_end);
    geditStyleRun *mid = gedit_style_run_copy(run);
    mid->offset = ms;
    mid->length = me - ms;
    if (bold >= 0) mid->bold = (gboolean)bold;
    if (italic >= 0) mid->italic = (gboolean)italic;
    if (underline >= 0) mid->underline = (gboolean)underline;
    if (color) mid->color = *color;
    if (font_size >= 0) mid->font_size = font_size;
    mid_runs = g_list_append(mid_runs, mid);

    /* Suffix after the target range */
    if (re > range_end) {
      geditStyleRun *suf = gedit_style_run_copy(run);
      suf->offset = range_end;
      suf->length = re - range_end;
      new_runs = g_list_append(new_runs, suf);
    }
  }

  /* Sort mid_runs by offset so we can detect gaps */
  mid_runs = g_list_sort(mid_runs, style_run_cmp_offset);

  /* Walk the target range, inserting mid_runs and filling gaps with defaults */
  gint cursor = offset;
  for (GList *l = mid_runs; l; l = l->next) {
    geditStyleRun *mr = l->data;
    if (mr->offset > cursor) {
      /* Gap: create a new run with defaults + specified attributes */
      geditStyleRun *gap = g_new0(geditStyleRun, 1);
      gap->offset = cursor;
      gap->length = mr->offset - cursor;
      /* Start from effective style at gap position */
      geditStyleRun eff;
      get_style_at_unlocked(self, cursor, &eff);
      gap->bold = (bold >= 0) ? (gboolean)bold : eff.bold;
      gap->italic = (italic >= 0) ? (gboolean)italic : eff.italic;
      gap->underline = (underline >= 0) ? (gboolean)underline : eff.underline;
      gap->color = color ? *color : eff.color;
      gap->font_size = (font_size >= 0) ? font_size : eff.font_size;
      new_runs = g_list_append(new_runs, gap);
    }
    new_runs = g_list_append(new_runs, mr);
    cursor = mr->offset + mr->length;
  }
  /* Trailing gap */
  if (cursor < range_end) {
    geditStyleRun *gap = g_new0(geditStyleRun, 1);
    gap->offset = cursor;
    gap->length = range_end - cursor;
    geditStyleRun eff;
    get_style_at_unlocked(self, cursor, &eff);
    gap->bold = (bold >= 0) ? (gboolean)bold : eff.bold;
    gap->italic = (italic >= 0) ? (gboolean)italic : eff.italic;
    gap->underline = (underline >= 0) ? (gboolean)underline : eff.underline;
    gap->color = color ? *color : eff.color;
    gap->font_size = (font_size >= 0) ? font_size : eff.font_size;
    new_runs = g_list_append(new_runs, gap);
  }

  g_list_free(mid_runs); /* data moved to new_runs, don't free it */

  /* Replace all runs */
  g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
  self->style_runs = new_runs;
}

void gedit_document_add_style_run(geditDocument *self, gint offset, gint length,
                                  gint bold, gint italic,
                                  gint underline, const GdkRGBA *color,
                                  gint font_size) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  add_style_run_unlocked(self, offset, length, bold, italic, underline, color,
                         font_size);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

/* Helper: duplicate a style run structure */
static geditStyleRun *gedit_style_run_copy(const geditStyleRun *src) {
  geditStyleRun *r = g_new0(geditStyleRun, 1);
  memcpy(r, src, sizeof(geditStyleRun));
  if (r->is_graphic && r->graphic) {
    r->graphic = gedit_graphic_copy(r->graphic);
  }
  r->link_url = g_strdup(src->link_url); /* NULL-safe */
  r->font_family = g_strdup(src->font_family); /* NULL-safe */
  return r;
}

static void gedit_style_run_free(geditStyleRun *run) {
  if (run->is_graphic && run->graphic) {
    gedit_graphic_free(run->graphic);
  }
  g_free(run->link_url);
  g_free(run->font_family);
  g_free(run);
}

/* Check whether all positions in [offset, offset+length) have the requested
 * style flag(s) present (i.e. bold/italic/underline). Returns TRUE only if
 * each character in the range has at least one style run that sets the
 * requested flag(s). */
static gboolean gedit_style_active_across_range(geditDocument *self,
                                                gint offset, gint length,
                                                gboolean bold, gboolean italic,
                                                gboolean underline) {
  if (!gedit_DOCUMENT(self))
    return FALSE;
  if (length <= 0)
    return FALSE;
  gchar *text = gedit_document_get_text(self);
  if (!text)
    return FALSE;

  for (gint i = 0; i < length; i++) {
    gint pos = offset + i;
    gboolean has_flag = FALSE;
    for (GList *l = self->style_runs; l; l = l->next) {
      geditStyleRun *run = (geditStyleRun *)l->data;
      gint rs = run->offset;
      gint re = run->offset + run->length;
      if (pos >= rs && pos < re) {
        if ((bold && run->bold) || (italic && run->italic) ||
            (underline && run->underline)) {
          has_flag = TRUE;
          break;
        }
      }
    }
    if (!has_flag) {
      g_free(text);
      return FALSE;
    }
  }
  g_free(text);
  return TRUE;
}

/* Remove the specified style flags from any style runs that intersect the
 * given range. Runs may be split into prefix/middle/suffix pieces to
 * preserve unaffected attributes. */
static void gedit_remove_style_flags_in_range(geditDocument *self, gint offset,
                                              gint length, gboolean rem_bold,
                                              gboolean rem_italic,
                                              gboolean rem_underline) {
  if (!gedit_DOCUMENT(self))
    return;
  if (length <= 0)
    return;

  GList *new_runs = NULL;
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    gint rs = run->offset;
    gint re = run->offset + run->length;
    if (re <= offset || rs >= offset + length) {
      /* no overlap, preserve as-is */
      new_runs = g_list_append(new_runs, gedit_style_run_copy(run));
      // g_free(run); // This was a bug, run is still in self->style_runs
      continue;
    }

    /* prefix portion before the intersection */
    if (rs < offset) {
      geditStyleRun *pre = gedit_style_run_copy(run);
      pre->length = offset - rs;
      new_runs = g_list_append(new_runs, pre);
    }

    /* middle (intersection) portion: remove requested flags */
    gint mid_s = MAX(rs, offset);
    gint mid_e = MIN(re, offset + length);
    geditStyleRun *mid = gedit_style_run_copy(run);
    if (rem_bold)
      mid->bold = FALSE;
    if (rem_italic)
      mid->italic = FALSE;
    if (rem_underline)
      mid->underline = FALSE;
    mid->offset = mid_s;
    mid->length = mid_e - mid_s;
    new_runs = g_list_append(new_runs, mid);

    /* suffix portion after the intersection */
    if (re > offset + length) {
      geditStyleRun *suf = gedit_style_run_copy(run);
      suf->offset = offset + length;
      suf->length = re - (offset + length);
      new_runs = g_list_append(new_runs, suf);
    }

    // g_free(run); // This was a bug, run is still in self->style_runs
  }

  /* replace runs */
  if (self->style_runs)
    g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
  self->style_runs = new_runs;

  /* Optionally, merge adjacent runs that have identical flags and are
   * contiguous. */
  GList *merged = NULL;
  GList *current_l = self->style_runs;
  while (current_l) {
    geditStyleRun *r = (geditStyleRun *)current_l->data;
    if (merged == NULL) {
      merged = g_list_append(merged, gedit_style_run_copy(r));
    } else {
      geditStyleRun *last = (geditStyleRun *)g_list_last(merged)->data;
      if (last->offset + last->length == r->offset && last->bold == r->bold &&
          last->italic == r->italic && last->underline == r->underline &&
          gdk_rgba_equal(&last->color, &r->color) &&
          last->font_size == r->font_size &&
          last->is_graphic == r->is_graphic &&
          (!last->is_graphic ||
           last->graphic == r->graphic)) { // Graphic runs should not merge
        /* extend last */
        last->length += r->length;
      } else {
        merged = g_list_append(merged, gedit_style_run_copy(r));
      }
    }
    current_l = current_l->next;
  }

  /* replace runs */
  if (self->style_runs)
    g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
  self->style_runs = merged;
}

/* Toggle styles over a range: enable the flags if not uniformly present,
 * otherwise remove the flags across the range. */
void gedit_document_toggle_style(geditDocument *self, gint offset, gint length,
                                 gboolean bold, gboolean italic,
                                 gboolean underline) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (length <= 0)
    return;

  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* Note: gedit_style_active_across_range should be called under lock but it
   * accesses self->style_runs which is protected. */
  gboolean all_have = gedit_style_active_across_range(self, offset, length,
                                                      bold, italic, underline);
  if (all_have) {
    gedit_remove_style_flags_in_range(self, offset, length, bold, italic,
                                      underline);
  } else {
    add_style_run_unlocked(self, offset, length,
                           bold ? 1 : -1,
                           italic ? 1 : -1,
                           underline ? 1 : -1,
                           NULL, -1);
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_get_style_at(geditDocument *self, gint offset,
                                 geditStyleRun *out_style) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);
  /* default */
  out_style->offset = offset;
  out_style->length = 0;
  out_style->bold = FALSE;
  out_style->italic = FALSE;
  out_style->underline = FALSE;
  gdk_rgba_parse(&out_style->color, "#000000");
  out_style->font_size = 0;
  out_style->is_graphic = FALSE;
  out_style->graphic = NULL;

  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    if (offset >= run->offset && offset < run->offset + run->length) {
      /* Merge: if any run has it, it's ON */
      if (run->bold)
        out_style->bold = TRUE;
      if (run->italic)
        out_style->italic = TRUE;
      if (run->underline)
        out_style->underline = TRUE;
      if (run->font_size > 0)
        out_style->font_size = run->font_size;
      /* simplification: last color wins */
      out_style->color = run->color;
      if (run->is_graphic) {
        out_style->is_graphic = TRUE;
        out_style->graphic = run->graphic; // Not copied, just reference
      }
    }
  }
  g_mutex_unlock(&self->mutex);
}

/* Build a PangoAttrList representing all style runs */
PangoAttrList *gedit_document_get_attr_list(geditDocument *self) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);
  gchar *text = gedit_document_get_text(self);
  PangoAttrList *list = pango_attr_list_new();
  if (!text)
    return list;

  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    gint start_byte = 0, end_byte = 0;
    const gchar *start_ptr = g_utf8_offset_to_pointer(text, run->offset);
    const gchar *end_ptr =
        g_utf8_offset_to_pointer(text, run->offset + run->length);
    start_byte = (gint)(start_ptr - text);
    end_byte = (gint)(end_ptr - text);

    if (run->bold) {
      PangoAttribute *a = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
      a->start_index = start_byte;
      a->end_index = end_byte;
      pango_attr_list_insert(list, a);
    }
    if (run->italic) {
      PangoAttribute *a = pango_attr_style_new(PANGO_STYLE_ITALIC);
      a->start_index = start_byte;
      a->end_index = end_byte;
      pango_attr_list_insert(list, a);
    }
    if (run->underline) {
      PangoAttribute *a = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
      a->start_index = start_byte;
      a->end_index = end_byte;
      pango_attr_list_insert(list, a);
    }
    if (run->font_size > 0) {
      PangoAttribute *a = pango_attr_size_new(run->font_size * PANGO_SCALE);
      a->start_index = start_byte;
      a->end_index = end_byte;
      pango_attr_list_insert(list, a);
    }
    if (run->font_family && run->font_family[0]) {
      PangoAttribute *a = pango_attr_family_new(run->font_family);
      a->start_index = start_byte;
      a->end_index = end_byte;
      pango_attr_list_insert(list, a);
    }
    if (run->is_graphic && run->graphic) {
      /* sit on the baseline */
      PangoRectangle rect = {0, -run->graphic->height * PANGO_SCALE,
                             run->graphic->width * PANGO_SCALE,
                             run->graphic->height * PANGO_SCALE};
      PangoAttribute *a = pango_attr_shape_new(&rect, &rect);
      a->start_index = start_byte;
      a->end_index = end_byte;
      pango_attr_list_insert(list, a);
    }
    /* color uses 16-bit components */
    PangoAttribute *col =
        pango_attr_foreground_new((guint16)(run->color.red * 65535.0),
                                  (guint16)(run->color.green * 65535.0),
                                  (guint16)(run->color.blue * 65535.0));
    col->start_index = start_byte;
    col->end_index = end_byte;
    pango_attr_list_insert(list, col);
  }

  g_free(text);
  return list;
}

GList *gedit_document_get_style_runs(geditDocument *self) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);
  g_mutex_lock(&self->mutex);
  GList *res = g_list_copy(self->style_runs);
  g_mutex_unlock(&self->mutex);
  return res;
}

/* Set font family on a range of text. Pass NULL to reset to default. */
void gedit_document_set_font_family(geditDocument *self, gint offset,
                                    gint length, const gchar *family) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (length <= 0) return;

  g_mutex_lock(&self->mutex);
  gint range_end = offset + length;
  GList *new_runs = NULL;

  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = l->data;
    gint rs = run->offset, re = rs + run->length;
    if (re <= offset || rs >= range_end) {
      new_runs = g_list_append(new_runs, gedit_style_run_copy(run));
      continue;
    }
    if (rs < offset) {
      geditStyleRun *pre = gedit_style_run_copy(run);
      pre->length = offset - rs;
      new_runs = g_list_append(new_runs, pre);
    }
    gint ms = MAX(rs, offset), me = MIN(re, range_end);
    geditStyleRun *mid = gedit_style_run_copy(run);
    mid->offset = ms;
    mid->length = me - ms;
    g_free(mid->font_family);
    mid->font_family = g_strdup(family);
    new_runs = g_list_append(new_runs, mid);
    if (re > range_end) {
      geditStyleRun *suf = gedit_style_run_copy(run);
      suf->offset = range_end;
      suf->length = re - range_end;
      new_runs = g_list_append(new_runs, suf);
    }
  }

  if (!self->style_runs) {
    geditStyleRun *nr = g_new0(geditStyleRun, 1);
    nr->offset = offset;
    nr->length = length;
    nr->font_family = g_strdup(family);
    new_runs = g_list_append(new_runs, nr);
  }

  g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
  self->style_runs = new_runs;
  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

/* Set a hyperlink URL on a range of text. Pass NULL to remove link. */
void gedit_document_set_link(geditDocument *self, gint offset, gint length,
                             const gchar *url) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (length <= 0) return;

  g_mutex_lock(&self->mutex);

  gint range_end = offset + length;
  GList *new_runs = NULL;

  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = l->data;
    gint rs = run->offset, re = rs + run->length;

    if (re <= offset || rs >= range_end) {
      new_runs = g_list_append(new_runs, gedit_style_run_copy(run));
      continue;
    }

    /* Prefix before target range */
    if (rs < offset) {
      geditStyleRun *pre = gedit_style_run_copy(run);
      pre->length = offset - rs;
      new_runs = g_list_append(new_runs, pre);
    }

    /* Middle: set the link */
    gint ms = MAX(rs, offset), me = MIN(re, range_end);
    geditStyleRun *mid = gedit_style_run_copy(run);
    mid->offset = ms;
    mid->length = me - ms;
    g_free(mid->link_url);
    mid->link_url = g_strdup(url);
    /* Auto-style links: blue + underline if setting, restore if removing */
    if (url && *url) {
      mid->underline = TRUE;
      mid->color = (GdkRGBA){0.0, 0.0, 0.8, 1.0};
    }
    new_runs = g_list_append(new_runs, mid);

    /* Suffix after target range */
    if (re > range_end) {
      geditStyleRun *suf = gedit_style_run_copy(run);
      suf->offset = range_end;
      suf->length = re - range_end;
      new_runs = g_list_append(new_runs, suf);
    }
  }

  /* If no existing runs covered this range, create one */
  if (!self->style_runs) {
    geditStyleRun *nr = g_new0(geditStyleRun, 1);
    nr->offset = offset;
    nr->length = length;
    nr->link_url = g_strdup(url);
    if (url && *url) {
      nr->underline = TRUE;
      nr->color = (GdkRGBA){0.0, 0.0, 0.8, 1.0};
    }
    new_runs = g_list_append(new_runs, nr);
  }

  g_list_free_full(self->style_runs, (GDestroyNotify)gedit_style_run_free);
  self->style_runs = new_runs;

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

/* Get link URL at a character offset. Returns newly-allocated string or NULL. */
gchar *gedit_document_get_link_at(geditDocument *self, gint offset) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);

  g_mutex_lock(&self->mutex);
  gchar *result = NULL;
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = l->data;
    if (offset >= run->offset && offset < run->offset + run->length &&
        run->link_url && run->link_url[0]) {
      result = g_strdup(run->link_url);
      break;
    }
  }
  g_mutex_unlock(&self->mutex);
  return result;
}

gchar *gedit_document_get_text(geditDocument *self) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);
  GtkTextIter start, end;
  gtk_text_buffer_get_start_iter(self->buffer, &start);
  gtk_text_buffer_get_end_iter(self->buffer, &end);
  return gtk_text_buffer_get_text(self->buffer, &start, &end, FALSE);
}

gint gedit_document_get_length(geditDocument *self) {
  g_return_val_if_fail(gedit_DOCUMENT(self), 0);
  return gtk_text_buffer_get_char_count(self->buffer);
}

/* Paragraph attribute helpers */
static geditParaAttr *para_attr_new(gint offset, gint length) {
  geditParaAttr *p = g_new0(geditParaAttr, 1);
  p->offset = offset;
  p->length = length;
  p->alignment = gedit_ALIGN_LEFT;
  p->indent = 0;
  p->bullet = FALSE;
  p->quote_level = 0;
  p->mask = 0;
  return p;
}

void gedit_document_set_alignment(geditDocument *self, gint offset, gint length,
                                  geditAlignment align) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  geditParaAttr *p = para_attr_new(offset, length);
  p->alignment = align;
  p->mask = gedit_PARA_ATTR_ALIGNMENT;
  self->para_attrs = g_list_append(self->para_attrs, p);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_toggle_bullet(geditDocument *self, gint offset,
                                  gint length) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* get current bullet state */
  geditParaAttr cur_attr;
  get_para_attr_unlocked(self, offset, length, &cur_attr);

  /* Cycle through bullet types: none -> circle -> square -> disk -> none */
  geditBulletType new_bullet = gedit_BULLET_NONE;
  if (cur_attr.bullet == gedit_BULLET_NONE)
    new_bullet = gedit_BULLET_CIRCLE;
  else if (cur_attr.bullet == gedit_BULLET_CIRCLE)
    new_bullet = gedit_BULLET_SQUARE;
  else if (cur_attr.bullet == gedit_BULLET_SQUARE)
    new_bullet = gedit_BULLET_DISK;
  else
    new_bullet = gedit_BULLET_NONE;

  geditParaAttr *p = para_attr_new(offset, length);
  p->bullet = new_bullet;
  p->mask = gedit_PARA_ATTR_BULLET;
  self->para_attrs = g_list_append(self->para_attrs, p);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_indent(geditDocument *self, gint offset, gint length,
                           gint delta_pixels) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* get current indent */
  geditParaAttr cur_attr;
  get_para_attr_unlocked(self, offset, length, &cur_attr);

  geditParaAttr *p = para_attr_new(offset, length);
  p->indent = cur_attr.indent + delta_pixels;
  if (p->indent < 0)
    p->indent = 0;
  p->mask = gedit_PARA_ATTR_INDENT;
  self->para_attrs = g_list_append(self->para_attrs, p);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_set_quote_level(geditDocument *self, gint offset,
                                    gint length, gint level) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  geditParaAttr *p = para_attr_new(offset, length);
  p->quote_level = level;
  p->mask = gedit_PARA_ATTR_QUOTE_LEVEL;
  self->para_attrs = g_list_append(self->para_attrs, p);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_insert_graphic(geditDocument *self, gint offset,
                                   GdkTexture *texture, gint width,
                                   gint height) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* Insert placeholder character U+FFFC (Object Replacement Character) */
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &iter, offset);
  /* The object replacement character is 3 bytes, 1 char */
  gtk_text_buffer_insert(self->buffer, &iter, "\xef\xbf\xbc", -1);

  /* Shift existing runs that are at or after the insertion point */
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *r = (geditStyleRun *)l->data;
    if (r->offset >= offset) {
      r->offset += 1; /* 1 char inserted */
    }
  }

  geditGraphic *gr = g_new0(geditGraphic, 1);
  gr->offset = offset;
  gr->width = width;
  gr->height = height;
  gr->texture = texture;
  if (texture)
    g_object_ref(texture);

  geditStyleRun *run = g_new0(geditStyleRun, 1);
  run->offset = offset;
  run->length = 1;
  run->is_graphic = TRUE;
  run->graphic = gr;

  /* Insert in sorted order (by offset) */
  GList *l = self->style_runs;
  GList *prev = NULL;
  while (l) {
    geditStyleRun *r = (geditStyleRun *)l->data;
    if (r->offset > offset)
      break;
    prev = l;
    l = l->next;
  }
  if (prev) {
    self->style_runs = g_list_insert_before(self->style_runs, l, run);
  } else {
    self->style_runs = g_list_prepend(self->style_runs, run);
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

/* Build a PangoAttrList representing style runs intersecting a specific char
 * range returned list has indices relative to start of range (bytes). */
PangoAttrList *gedit_document_get_attr_list_for_range(geditDocument *self,
                                                      gint range_offset,
                                                      gint range_length) {
  g_return_val_if_fail(gedit_DOCUMENT(self), NULL);
  gchar *text = gedit_document_get_text(self);
  PangoAttrList *list = pango_attr_list_new();
  if (!text)
    return list;

  const gchar *range_start_ptr = g_utf8_offset_to_pointer(text, range_offset);
  const gchar *range_end_ptr =
      g_utf8_offset_to_pointer(text, range_offset + range_length);
  gint range_start_byte = (gint)(range_start_ptr - text);
  gint range_end_byte = (gint)(range_end_ptr - text);

  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    const gchar *run_start_ptr = g_utf8_offset_to_pointer(text, run->offset);
    const gchar *run_end_ptr =
        g_utf8_offset_to_pointer(text, run->offset + run->length);
    gint run_start_byte = (gint)(run_start_ptr - text);
    gint run_end_byte = (gint)(run_end_ptr - text);

    gint inter_start = MAX(run_start_byte, range_start_byte);
    gint inter_end = MIN(run_end_byte, range_end_byte);
    if (inter_end <= inter_start)
      continue;
    gint rel_start = inter_start - range_start_byte;
    gint rel_end = inter_end - range_start_byte;

    if (run->bold) {
      PangoAttribute *a = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
      a->start_index = rel_start;
      a->end_index = rel_end;
      pango_attr_list_insert(list, a);
    }
    if (run->italic) {
      PangoAttribute *a = pango_attr_style_new(PANGO_STYLE_ITALIC);
      a->start_index = rel_start;
      a->end_index = rel_end;
      pango_attr_list_insert(list, a);
    }
    if (run->underline) {
      PangoAttribute *a = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
      a->start_index = rel_start;
      a->end_index = rel_end;
      pango_attr_list_insert(list, a);
    }
    PangoAttribute *col =
        pango_attr_foreground_new((guint16)(run->color.red * 65535.0),
                                  (guint16)(run->color.green * 65535.0),
                                  (guint16)(run->color.blue * 65535.0));
    col->start_index = rel_start;
    col->end_index = rel_end;
    pango_attr_list_insert(list, col);
    if (run->font_size > 0) {
      PangoAttribute *sz = pango_attr_size_new(run->font_size * PANGO_SCALE);
      sz->start_index = rel_start;
      sz->end_index = rel_end;
      pango_attr_list_insert(list, sz);
    }
    if (run->font_family && run->font_family[0]) {
      PangoAttribute *ff = pango_attr_family_new(run->font_family);
      ff->start_index = rel_start;
      ff->end_index = rel_end;
      pango_attr_list_insert(list, ff);
    }
    if (run->is_graphic && run->graphic) {
      /* sit on the baseline */
      PangoRectangle rect = {0, -run->graphic->height * PANGO_SCALE,
                             run->graphic->width * PANGO_SCALE,
                             run->graphic->height * PANGO_SCALE};
      PangoAttribute *a = pango_attr_shape_new(&rect, &rect);
      a->start_index = rel_start;
      a->end_index = rel_end;
      pango_attr_list_insert(list, a);
    }
  }

  g_free(text);
  return list;
}

static void get_para_attr_unlocked(geditDocument *self, gint offset,
                                   gint length, geditParaAttr *out_attr) {
  if (!out_attr)
    return;
  /* default */
  out_attr->offset = offset;
  out_attr->length = length;
  out_attr->alignment = gedit_ALIGN_LEFT;
  out_attr->indent = 0;
  out_attr->bullet = FALSE;
  out_attr->quote_level = 0;
  out_attr->is_hr = FALSE;
  out_attr->page_break = FALSE;
  out_attr->tab_stops = NULL;
  out_attr->direction = gedit_DIR_LTR;
  out_attr->mask = 0;

  for (GList *l = self->para_attrs; l; l = l->next) {
    geditParaAttr *p = (geditParaAttr *)l->data;
    gint a = p->offset;
    gint b = p->offset + p->length;
    gint o = offset;
    gint e = offset + length;
    if (b <= o || a >= e)
      continue; /* no overlap */
    /* merge based on mask */
    if (p->mask & gedit_PARA_ATTR_ALIGNMENT)
      out_attr->alignment = p->alignment;
    if (p->mask & gedit_PARA_ATTR_INDENT)
      out_attr->indent = p->indent;
    if (p->mask & gedit_PARA_ATTR_BULLET)
      out_attr->bullet = p->bullet;
    if (p->mask & gedit_PARA_ATTR_QUOTE_LEVEL)
      out_attr->quote_level = p->quote_level;
    if (p->mask & gedit_PARA_ATTR_HR)
      out_attr->is_hr = p->is_hr;
    if (p->mask & gedit_PARA_ATTR_TAB_STOPS)
      out_attr->tab_stops = p->tab_stops;
    if (p->mask & gedit_PARA_ATTR_DIRECTION)
      out_attr->direction = p->direction;
    if (p->mask & gedit_PARA_ATTR_PAGE_BREAK)
      out_attr->page_break = p->page_break;
  }
}

void gedit_document_get_para_attr(geditDocument *self, gint offset, gint length,
                                  geditParaAttr *out_attr) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);
  get_para_attr_unlocked(self, offset, length, out_attr);
  g_mutex_unlock(&self->mutex);
}

void gedit_document_insert_hr(geditDocument *self, gint offset) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* Insert a newline for the HR */
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &iter, offset);
  gtk_text_buffer_insert(self->buffer, &iter, "\n", -1);

  /* Create HR paragraph attribute */
  geditParaAttr *p = g_new0(geditParaAttr, 1);
  p->offset = offset;
  p->length = 1;
  p->is_hr = TRUE;
  p->mask = gedit_PARA_ATTR_HR;
  self->para_attrs = g_list_append(self->para_attrs, p);

  /* Adjust style run offsets */
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    if (run->offset >= offset)
      run->offset += 1;
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_set_tab_stops(geditDocument *self, gint offset, gint length,
                                  GArray *tab_stops) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (!tab_stops)
    return;

  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  geditParaAttr *p = g_new0(geditParaAttr, 1);
  p->offset = offset;
  p->length = length;
  p->tab_stops = g_array_new(FALSE, FALSE, sizeof(geditTabStop));
  g_array_append_vals(p->tab_stops, tab_stops->data, tab_stops->len);
  p->mask = gedit_PARA_ATTR_TAB_STOPS;
  self->para_attrs = g_list_append(self->para_attrs, p);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_remove_para_attrs_in_range(geditDocument *self, gint offset,
                                               gint length) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (length <= 0)
    return;

  g_mutex_lock(&self->mutex);

  /* Remove any paragraph attributes that overlap the deleted range */
  GList *l = self->para_attrs;
  while (l) {
    geditParaAttr *p = (geditParaAttr *)l->data;
    gint a = p->offset;
    gint b = p->offset + p->length;
    if (b <= offset || a >= offset + length) {
      /* no overlap, keep it */
      l = l->next;
    } else {
      /* overlapping attr: remove it */
      GList *next = l->next;
      self->para_attrs = g_list_delete_link(self->para_attrs, l);
      g_free(p);
      l = next;
    }
  }

  /* Adjust offsets of remaining attributes */
  for (GList *l = self->para_attrs; l; l = l->next) {
    geditParaAttr *p = (geditParaAttr *)l->data;
    if (p->offset >= offset + length) {
      p->offset -= length;
    }
  }

  g_mutex_unlock(&self->mutex);
}

void gedit_document_set_direction(geditDocument *self, gint offset, gint length,
                                  geditDirection direction) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  geditParaAttr *p = para_attr_new(offset, length);
  p->direction = direction;
  p->mask = gedit_PARA_ATTR_DIRECTION;
  self->para_attrs = g_list_append(self->para_attrs, p);

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_insert_page_break(geditDocument *self, gint offset) {
  g_return_if_fail(gedit_DOCUMENT(self));
  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  /* Insert a form feed character (U+000C) */
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_offset(self->buffer, &iter, offset);
  gtk_text_buffer_insert(self->buffer, &iter, "\f", -1);

  /* Create page break paragraph attribute */
  geditParaAttr *p = g_new0(geditParaAttr, 1);
  p->offset = offset;
  p->length = 1;
  p->page_break = TRUE;
  p->mask = gedit_PARA_ATTR_PAGE_BREAK;
  self->para_attrs = g_list_append(self->para_attrs, p);

  /* Adjust style run offsets */
  for (GList *l = self->style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;
    if (run->offset >= offset)
      run->offset += 1;
  }

  g_mutex_unlock(&self->mutex);
  g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
}

void gedit_document_get_word_bounds(geditDocument *self, gint offset,
                                    gint *out_start, gint *out_end) {
  g_return_if_fail(gedit_DOCUMENT(self));
  if (!out_start || !out_end)
    return;

  gchar *text = gedit_document_get_text(self);
  if (!text) {
    *out_start = offset;
    *out_end = offset;
    return;
  }

  gint len = g_utf8_strlen(text, -1);
  gint start = offset;
  gint end = offset;

  /* Move start backwards to beginning of word */
  while (start > 0) {
    const gchar *p = g_utf8_offset_to_pointer(text, start - 1);
    gunichar uc = g_utf8_get_char(p);
    if (g_unichar_isspace(uc) || uc == '\n')
      break;
    start--;
  }

  /* Move end forwards to end of word */
  while (end < len) {
    const gchar *p = g_utf8_offset_to_pointer(text, end);
    gunichar uc = g_utf8_get_char(p);
    if (g_unichar_isspace(uc) || uc == '\n')
      break;
    end++;
  }

  *out_start = start;
  *out_end = end;
  g_free(text);
}


gboolean gedit_document_is_word_boundary_char(geditDocument *self, gint offset) {
  g_return_val_if_fail(gedit_DOCUMENT(self), TRUE);
  
  gchar *text = gedit_document_get_text(self);
  if (!text)
    return TRUE;
  
  gint len = g_utf8_strlen(text, -1);
  if (offset < 0 || offset >= len) {
    g_free(text);
    return TRUE;
  }
  
  const gchar *p = g_utf8_offset_to_pointer(text, offset);
  gunichar uc = g_utf8_get_char(p);
  
  gboolean is_boundary = g_unichar_isspace(uc) || uc == '\n' || 
                         g_unichar_ispunct(uc);
  
  g_free(text);
  return is_boundary;
}


gint gedit_document_find_text(geditDocument *self, const gchar *search_text,
                              gint start_offset, gint end_offset,
                              gboolean case_sensitive) {
  g_return_val_if_fail(gedit_DOCUMENT(self), -1);
  if (!search_text || !*search_text)
    return -1;

  gchar *full_text = gedit_document_get_text(self);
  if (!full_text)
    return -1;

  gint doc_len = g_utf8_strlen(full_text, -1);
  if (end_offset < 0 || end_offset > doc_len)
    end_offset = doc_len;
  if (start_offset < 0)
    start_offset = 0;

  gint search_len = g_utf8_strlen(search_text, -1);
  gint result = -1;

  /* Search from start_offset to end_offset */
  for (gint i = start_offset; i <= end_offset - search_len; i++) {
    const gchar *pos = g_utf8_offset_to_pointer(full_text, i);
    const gchar *search_pos = g_utf8_offset_to_pointer(full_text, i + search_len);
    gint byte_len = (gint)(search_pos - pos);

    gchar *substr = g_strndup(pos, byte_len);
    
    gboolean match = FALSE;
    if (case_sensitive) {
      match = (strcmp(substr, search_text) == 0);
    } else {
      gchar *substr_lower = g_utf8_strdown(substr, -1);
      gchar *search_lower = g_utf8_strdown(search_text, -1);
      match = (strcmp(substr_lower, search_lower) == 0);
      g_free(substr_lower);
      g_free(search_lower);
    }

    if (match) {
      result = i;
      g_free(substr);
      break;
    }
    g_free(substr);
  }

  g_free(full_text);
  return result;
}

gint gedit_document_replace_text(geditDocument *self, const gchar *search_text,
                                 const gchar *replace_text, gint start_offset,
                                 gint end_offset, gboolean replace_all,
                                 gboolean case_sensitive) {
  g_return_val_if_fail(gedit_DOCUMENT(self), 0);
  if (!search_text || !*search_text)
    return 0;

  gint replacements = 0;
  gint search_len = g_utf8_strlen(search_text, -1);
  gint replace_len = replace_text ? g_utf8_strlen(replace_text, -1) : 0;
  gint current_offset = start_offset;

  g_mutex_lock(&self->mutex);

  geditSnapshot *snap = gedit_snapshot_new(self);
  g_queue_push_head(self->undo_stack, snap);
  if (self->redo_stack) {
    g_queue_free_full(self->redo_stack, (GDestroyNotify)gedit_snapshot_free);
    self->redo_stack = g_queue_new();
  }

  while (current_offset >= 0) {
    gint found = gedit_document_find_text(self, search_text, current_offset,
                                          end_offset, case_sensitive);
    if (found < 0)
      break;

    /* Delete the found text */
    gedit_document_delete_range(self, found, search_len);
    
    /* Insert replacement text */
    if (replace_text && *replace_text) {
      GtkTextIter iter;
      gtk_text_buffer_get_iter_at_offset(self->buffer, &iter, found);
      gtk_text_buffer_insert(self->buffer, &iter, replace_text, -1);
    }

    replacements++;
    current_offset = found + replace_len;

    if (!replace_all)
      break;

    /* Update end_offset for next search */
    end_offset += (replace_len - search_len);
  }

  g_mutex_unlock(&self->mutex);

  if (replacements > 0) {
    g_signal_emit(self, signals[DOCUMENT_CHANGED], 0);
  }

  return replacements;
}


/* Navigation helpers */

gint gedit_document_find_word_boundary_left(geditDocument *self, gint offset) {
  g_return_val_if_fail(gedit_DOCUMENT(self), 0);
  
  gchar *text = gedit_document_get_text(self);
  if (!text || offset <= 0)
    return 0;
  
  /* Move back one character to start */
  gint pos = offset - 1;
  if (pos < 0)
    pos = 0;
  
  /* Skip whitespace backwards */
  while (pos > 0) {
    const gchar *p = g_utf8_offset_to_pointer(text, pos);
    gunichar ch = g_utf8_get_char(p);
    if (!g_unichar_isspace(ch))
      break;
    pos--;
  }
  
  /* Skip word characters backwards */
  while (pos > 0) {
    const gchar *p = g_utf8_offset_to_pointer(text, pos - 1);
    gunichar ch = g_utf8_get_char(p);
    if (!g_unichar_isalnum(ch) && ch != '_')
      break;
    pos--;
  }
  
  g_free(text);
  return pos;
}

gint gedit_document_find_word_boundary_right(geditDocument *self, gint offset) {
  g_return_val_if_fail(gedit_DOCUMENT(self), 0);
  
  gchar *text = gedit_document_get_text(self);
  if (!text)
    return offset;
  
  gint len = g_utf8_strlen(text, -1);
  if (offset >= len) {
    g_free(text);
    return len;
  }
  
  gint pos = offset;
  
  /* Skip word characters forward */
  while (pos < len) {
    const gchar *p = g_utf8_offset_to_pointer(text, pos);
    gunichar ch = g_utf8_get_char(p);
    if (!g_unichar_isalnum(ch) && ch != '_')
      break;
    pos++;
  }
  
  /* Skip whitespace forward */
  while (pos < len) {
    const gchar *p = g_utf8_offset_to_pointer(text, pos);
    gunichar ch = g_utf8_get_char(p);
    if (!g_unichar_isspace(ch))
      break;
    pos++;
  }
  
  g_free(text);
  return pos;
}

gint gedit_document_find_line_start(geditDocument *self, gint offset) {
  g_return_val_if_fail(gedit_DOCUMENT(self), 0);
  
  gchar *text = gedit_document_get_text(self);
  if (!text || offset <= 0) {
    g_free(text);
    return 0;
  }
  
  /* Move back to find the newline before this position */
  gint pos = offset;
  while (pos > 0) {
    const gchar *p = g_utf8_offset_to_pointer(text, pos - 1);
    if (*p == '\n')
      break;
    pos--;
  }
  
  g_free(text);
  return pos;
}

gint gedit_document_find_line_end(geditDocument *self, gint offset) {
  g_return_val_if_fail(gedit_DOCUMENT(self), 0);
  
  gchar *text = gedit_document_get_text(self);
  if (!text) {
    g_free(text);
    return offset;
  }
  
  gint len = g_utf8_strlen(text, -1);
  gint pos = offset;
  
  /* Move forward to find the newline after this position */
  while (pos < len) {
    const gchar *p = g_utf8_offset_to_pointer(text, pos);
    if (*p == '\n')
      break;
    pos++;
  }
  
  g_free(text);
  return pos;
}
