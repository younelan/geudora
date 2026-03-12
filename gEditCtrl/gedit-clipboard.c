#include "gedit-clipboard.h"
#include "gedit-state.h"
#include <gdk/gdk.h>
#include <json-glib/json-glib.h>
#include <string.h>

/* Forward declarations */
static geditGraphic *gedit_graphic_copy(const geditGraphic *src);
static void gedit_graphic_free(geditGraphic *g);
static void gedit_style_run_free(geditStyleRun *run);
static gchar *gedit_add_intelligent_paste_space(geditDocument *doc, gint caret,
                                                const gchar *text,
                                                gint *out_added_before,
                                                gint *out_added_after);

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

static void gedit_style_run_free(geditStyleRun *run) {
  if (run->is_graphic && run->graphic) {
    gedit_graphic_free(run->graphic);
  }
  g_free(run);
}

/* Collapse consecutive runs with identical formatting (from Carbon convert.c pattern) */
static GList *gedit_merge_style_runs(GList *runs) {
  if (!runs)
    return NULL;

  GList *merged = NULL;
  geditStyleRun *prev = NULL;

  for (GList *l = runs; l; l = l->next) {
    geditStyleRun *curr = (geditStyleRun *)l->data;

    if (prev && prev->bold == curr->bold && prev->italic == curr->italic &&
        prev->underline == curr->underline && prev->font_size == curr->font_size &&
        prev->is_graphic == curr->is_graphic &&
        prev->color.red == curr->color.red &&
        prev->color.green == curr->color.green &&
        prev->color.blue == curr->color.blue &&
        prev->color.alpha == curr->color.alpha) {
      /* Merge with previous */
      geditStyleRun *last = (geditStyleRun *)g_list_last(merged)->data;
      last->length += curr->length;
    } else {
      /* Add as new run */
      geditStyleRun *copy = g_new0(geditStyleRun, 1);
      memcpy(copy, curr, sizeof(geditStyleRun));
      if (copy->is_graphic && copy->graphic) {
        copy->graphic = gedit_graphic_copy(copy->graphic);
      }
      merged = g_list_append(merged, copy);
      prev = copy;
    }
  }

  return merged;
}

/* Serialize formatted content to JSON for clipboard */
static gchar *gedit_serialize_content(geditDocument *doc, gint offset,
                                      gint length) {
  JsonBuilder *builder = json_builder_new();
  json_builder_begin_object(builder);

  /* Extract text range */
  gchar *full_text = gedit_document_get_text(doc);
  if (!full_text) {
    g_object_unref(builder);
    return g_strdup("");
  }

  const gchar *start_ptr = g_utf8_offset_to_pointer(full_text, offset);
  const gchar *end_ptr = g_utf8_offset_to_pointer(full_text, offset + length);
  gint byte_len = (gint)(end_ptr - start_ptr);
  gchar *text_range = g_strndup(start_ptr, byte_len);

  json_builder_set_member_name(builder, "text");
  json_builder_add_string_value(builder, text_range);

  /* Serialize style runs that overlap with selection */
  json_builder_set_member_name(builder, "styles");
  json_builder_begin_array(builder);

  GList *style_runs = gedit_document_get_style_runs(doc);
  GList *overlapping = NULL;

  /* Extract overlapping runs */
  for (GList *l = style_runs; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;

    /* Check if this run overlaps with selection */
    gint run_end = run->offset + run->length;
    gint sel_end = offset + length;
    if (run_end <= offset || run->offset >= sel_end)
      continue;

    /* Calculate overlap */
    gint overlap_start = MAX(run->offset, offset);
    gint overlap_end = MIN(run_end, sel_end);
    gint overlap_len = overlap_end - overlap_start;
    gint rel_offset = overlap_start - offset;

    geditStyleRun *overlap_run = g_new0(geditStyleRun, 1);
    memcpy(overlap_run, run, sizeof(geditStyleRun));
    overlap_run->offset = rel_offset;
    overlap_run->length = overlap_len;
    if (overlap_run->is_graphic && overlap_run->graphic) {
      overlap_run->graphic = gedit_graphic_copy(overlap_run->graphic);
    }
    overlapping = g_list_append(overlapping, overlap_run);
  }

  /* Merge consecutive runs with identical formatting */
  GList *merged = gedit_merge_style_runs(overlapping);
  g_list_free_full(overlapping, (GDestroyNotify)gedit_style_run_free);

  /* Serialize merged runs */
  for (GList *l = merged; l; l = l->next) {
    geditStyleRun *run = (geditStyleRun *)l->data;

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "offset");
    json_builder_add_int_value(builder, run->offset);
    json_builder_set_member_name(builder, "length");
    json_builder_add_int_value(builder, run->length);
    json_builder_set_member_name(builder, "bold");
    json_builder_add_boolean_value(builder, run->bold);
    json_builder_set_member_name(builder, "italic");
    json_builder_add_boolean_value(builder, run->italic);
    json_builder_set_member_name(builder, "underline");
    json_builder_add_boolean_value(builder, run->underline);
    json_builder_set_member_name(builder, "font_size");
    json_builder_add_int_value(builder, run->font_size);

    if (run->is_graphic && run->graphic) {
      json_builder_set_member_name(builder, "is_graphic");
      json_builder_add_boolean_value(builder, TRUE);
      json_builder_set_member_name(builder, "graphic_width");
      json_builder_add_int_value(builder, run->graphic->width);
      json_builder_set_member_name(builder, "graphic_height");
      json_builder_add_int_value(builder, run->graphic->height);
    } else {
      json_builder_set_member_name(builder, "color");
      json_builder_begin_object(builder);
      json_builder_set_member_name(builder, "r");
      json_builder_add_double_value(builder, run->color.red);
      json_builder_set_member_name(builder, "g");
      json_builder_add_double_value(builder, run->color.green);
      json_builder_set_member_name(builder, "b");
      json_builder_add_double_value(builder, run->color.blue);
      json_builder_set_member_name(builder, "a");
      json_builder_add_double_value(builder, run->color.alpha);
      json_builder_end_object(builder);
    }

    json_builder_end_object(builder);
  }
  g_list_free_full(merged, (GDestroyNotify)gedit_style_run_free);
  json_builder_end_array(builder);

  json_builder_end_object(builder);

  JsonNode *root = json_builder_get_root(builder);
  JsonGenerator *gen = json_generator_new();
  json_generator_set_root(gen, root);
  gchar *json_str = json_generator_to_data(gen, NULL);

  g_object_unref(gen);
  json_node_free(root);
  g_object_unref(builder);
  g_free(text_range);
  g_free(full_text);

  return json_str;
}

/* Deserialize JSON content from clipboard */
static gboolean gedit_deserialize_content(const gchar *json_str,
                                          gchar **out_text,
                                          GList **out_styles) {
  if (!json_str || !out_text || !out_styles)
    return FALSE;

  JsonParser *parser = json_parser_new();
  GError *error = NULL;

  if (!json_parser_load_from_data(parser, json_str, -1, &error)) {
    g_error_free(error);
    g_object_unref(parser);
    return FALSE;
  }

  JsonNode *root = json_parser_get_root(parser);
  if (!JSON_NODE_HOLDS_OBJECT(root)) {
    g_object_unref(parser);
    return FALSE;
  }

  JsonObject *obj = json_node_get_object(root);

  /* Extract text */
  const gchar *text = json_object_get_string_member(obj, "text");
  if (!text) {
    g_object_unref(parser);
    return FALSE;
  }
  *out_text = g_strdup(text);

  /* Extract styles */
  JsonArray *styles_array = json_object_get_array_member(obj, "styles");
  if (styles_array) {
    guint len = json_array_get_length(styles_array);
    for (guint i = 0; i < len; i++) {
      JsonNode *style_node = json_array_get_element(styles_array, i);
      if (!JSON_NODE_HOLDS_OBJECT(style_node))
        continue;

      JsonObject *style_obj = json_node_get_object(style_node);
      geditStyleRun *run = g_new0(geditStyleRun, 1);

      run->offset = json_object_get_int_member(style_obj, "offset");
      run->length = json_object_get_int_member(style_obj, "length");
      run->bold = json_object_get_boolean_member(style_obj, "bold");
      run->italic = json_object_get_boolean_member(style_obj, "italic");
      run->underline = json_object_get_boolean_member(style_obj, "underline");
      run->font_size = json_object_get_int_member(style_obj, "font_size");
      run->is_graphic = json_object_get_boolean_member(style_obj, "is_graphic");

      if (!run->is_graphic) {
        JsonObject *color_obj = json_object_get_object_member(style_obj, "color");
        if (color_obj) {
          run->color.red = json_object_get_double_member(color_obj, "r");
          run->color.green = json_object_get_double_member(color_obj, "g");
          run->color.blue = json_object_get_double_member(color_obj, "b");
          run->color.alpha = json_object_get_double_member(color_obj, "a");
        }
      }

      *out_styles = g_list_append(*out_styles, run);
    }
  }

  g_object_unref(parser);
  return TRUE;
}

/* Callback for paste operation completion */
static void gedit_clipboard_paste_cb(GObject *source_object, GAsyncResult *res,
                                     gpointer user_data) {
  GdkClipboard *clipboard = GDK_CLIPBOARD(source_object);
  struct {
    GtkWidget *area;
    gint caret_pos;
    geditDocument *doc;
    GEditCtrlState *state;
    gboolean plain_text;
  } *data = user_data;

  GError *error = NULL;
  gchar *text = gdk_clipboard_read_text_finish(clipboard, res, &error);

  if (error) {
    g_error_free(error);
    g_free(data);
    return;
  }

  if (text) {
    gchar *plain_text = NULL;
    GList *styles = NULL;
    gint inserted_len = 0;

    /* If plain text mode, skip JSON parsing and use text as-is */
    if (data->plain_text) {
      
      /* Add intelligent spacing */
      gint added_before = 0, added_after = 0;
      gchar *spaced_text = gedit_add_intelligent_paste_space(data->doc, data->caret_pos,
                                                             text, &added_before, &added_after);
      
      inserted_len = g_utf8_strlen(spaced_text, -1);
      gedit_document_paste_formatted(data->doc, data->caret_pos, spaced_text, NULL);
      g_free(spaced_text);
    } else {
      /* Try to parse as JSON (formatted content) */
      if (gedit_deserialize_content(text, &plain_text, &styles)) {

        /* Add intelligent spacing */
        gint added_before = 0, added_after = 0;
        gchar *spaced_text = gedit_add_intelligent_paste_space(data->doc, data->caret_pos,
                                                               plain_text, &added_before, &added_after);
        
        inserted_len = g_utf8_strlen(spaced_text, -1);
        
        /* Adjust style run offsets for added spaces */
        if (added_before > 0) {
          for (GList *l = styles; l; l = l->next) {
            geditStyleRun *run = (geditStyleRun *)l->data;
            run->offset += added_before;
          }
        }
        
        /* Use batch paste for atomic undo */
        gedit_document_paste_formatted(data->doc, data->caret_pos, spaced_text,
                                       styles);

        /* Clean up styles list (paste_formatted copies them) */
        for (GList *l = styles; l; l = l->next) {
          g_free(l->data);
        }
        g_list_free(styles);
        g_free(spaced_text);
        g_free(plain_text);
      } else {
        /* Plain text fallback */
        
        /* Add intelligent spacing */
        gint added_before = 0, added_after = 0;
        gchar *spaced_text = gedit_add_intelligent_paste_space(data->doc, data->caret_pos,
                                                               text, &added_before, &added_after);
        
        inserted_len = g_utf8_strlen(spaced_text, -1);
        gedit_document_paste_formatted(data->doc, data->caret_pos, spaced_text, NULL);
        g_free(spaced_text);
        gedit_document_paste_formatted(data->doc, data->caret_pos, text, NULL);
      }
    }

    /* Update caret to end of pasted text */
    if (data->state) {
      data->state->caret = data->caret_pos + inserted_len;
      data->state->sel_start = data->state->caret;
      data->state->sel_end = data->state->caret;
      data->state->sel_anchor = -1;
      g_signal_emit_by_name(data->doc, "selection-changed");
    }

    gtk_widget_queue_draw(data->area);
    g_free(text);
  }

  g_free(data);
}

G_GNUC_INTERNAL void gedit_clipboard_copy(GtkWidget *area, gint sel_start,
                                          gint sel_end, geditDocument *doc) {
  if (sel_start == sel_end || !doc)
    return;

  gint offset = MIN(sel_start, sel_end);
  gint length = ABS(sel_end - sel_start);

  GdkClipboard *clipboard = gtk_widget_get_clipboard(area);
  if (!clipboard)
    return;

  /* Serialize formatted content */
  gchar *json_content = gedit_serialize_content(doc, offset, length);

  /* Also get plain text for fallback */
  gchar *full_text = gedit_document_get_text(doc);
  const gchar *start_ptr = g_utf8_offset_to_pointer(full_text, offset);
  const gchar *end_ptr = g_utf8_offset_to_pointer(full_text, offset + length);
  gint byte_len = (gint)(end_ptr - start_ptr);
  gchar *plain_text = g_strndup(start_ptr, byte_len);

  /* Set clipboard with both formats */
  GValue value = G_VALUE_INIT;
  g_value_init(&value, G_TYPE_STRING);
  g_value_set_string(&value, json_content);
  gdk_clipboard_set_value(clipboard, &value);
  g_value_unset(&value);

  g_free(json_content);
  g_free(plain_text);
  g_free(full_text);
}

G_GNUC_INTERNAL void gedit_clipboard_cut(GtkWidget *area, gint sel_start,
                                         gint sel_end, geditDocument *doc) {
  if (sel_start == sel_end || !doc)
    return;

  /* Copy first */
  gedit_clipboard_copy(area, sel_start, sel_end, doc);

  /* Then delete */
  gint offset = MIN(sel_start, sel_end);
  gint length = ABS(sel_end - sel_start);
  gedit_document_delete_range(doc, offset, length);

  gtk_widget_queue_draw(area);
}

G_GNUC_INTERNAL void gedit_clipboard_paste(GtkWidget *area, gint caret_pos,
                                           geditDocument *doc) {
  if (!area || !doc)
    return;

  GdkClipboard *clipboard = gtk_widget_get_clipboard(area);
  if (!clipboard)
    return;

  GEditCtrlState *state = gedit_state_for_area(area);

  struct {
    GtkWidget *area;
    gint caret_pos;
    geditDocument *doc;
    GEditCtrlState *state;
    gboolean plain_text;
  } *data = g_new(typeof(*data), 1);
  data->area = area;
  data->caret_pos = caret_pos;
  data->doc = doc;
  data->state = state;
  data->plain_text = FALSE;

  gdk_clipboard_read_text_async(clipboard, NULL, gedit_clipboard_paste_cb,
                                data);
}

G_GNUC_INTERNAL void gedit_clipboard_intelligent_copy(GtkWidget *area, gint caret,
                                                      geditDocument *doc) {
  if (!doc)
    return;

  gint start, end;
  gedit_document_get_word_bounds(doc, caret, &start, &end);
  
  if (start < end) {
    gedit_clipboard_copy(area, start, end, doc);
  }
}

G_GNUC_INTERNAL void gedit_clipboard_intelligent_cut(GtkWidget *area, gint caret,
                                                     geditDocument *doc) {
  if (!doc)
    return;

  gint start, end;
  gedit_document_get_word_bounds(doc, caret, &start, &end);
  
  if (start < end) {
    /* Extend selection to include surrounding spaces intelligently */
    gchar *text = gedit_document_get_text(doc);
    if (text) {
      gint len = g_utf8_strlen(text, -1);
      
      /* Check if we should include space before */
      if (start > 0) {
        const gchar *p = g_utf8_offset_to_pointer(text, start - 1);
        gunichar uc = g_utf8_get_char(p);
        if (g_unichar_isspace(uc)) {
          start--;
        }
      }
      
      /* Check if we should include space after */
      if (end < len) {
        const gchar *p = g_utf8_offset_to_pointer(text, end);
        gunichar uc = g_utf8_get_char(p);
        if (g_unichar_isspace(uc)) {
          end++;
        }
      }
      
      g_free(text);
    }
    
    gedit_clipboard_cut(area, start, end, doc);
  }
}


G_GNUC_INTERNAL void gedit_clipboard_paste_plain(GtkWidget *area, gint caret_pos,
                                                 geditDocument *doc) {
  if (!area || !doc)
    return;

  GdkClipboard *clipboard = gtk_widget_get_clipboard(area);
  if (!clipboard)
    return;

  GEditCtrlState *state = gedit_state_for_area(area);

  struct {
    GtkWidget *area;
    gint caret_pos;
    geditDocument *doc;
    GEditCtrlState *state;
    gboolean plain_text;
  } *data = g_new(typeof(*data), 1);
  data->area = area;
  data->caret_pos = caret_pos;
  data->doc = doc;
  data->state = state;
  data->plain_text = TRUE;

  gdk_clipboard_read_text_async(clipboard, NULL, gedit_clipboard_paste_cb,
                                data);
}


/* Add intelligent spacing around pasted text */
static gchar *gedit_add_intelligent_paste_space(geditDocument *doc, gint caret,
                                                const gchar *text,
                                                gint *out_added_before,
                                                gint *out_added_after) {
  if (!text || !doc) {
    *out_added_before = 0;
    *out_added_after = 0;
    return g_strdup(text);
  }

  gchar *full_text = gedit_document_get_text(doc);
  if (!full_text) {
    *out_added_before = 0;
    *out_added_after = 0;
    return g_strdup(text);
  }

  gint doc_len = g_utf8_strlen(full_text, -1);
  gboolean add_space_before = FALSE;
  gboolean add_space_after = FALSE;
  
  *out_added_before = 0;
  *out_added_after = 0;

  /* Check if we need space before */
  if (caret > 0 && caret < doc_len) {
    /* Check character before caret */
    const gchar *p = g_utf8_offset_to_pointer(full_text, caret - 1);
    gunichar uc = g_utf8_get_char(p);
    
    /* Check first character of pasted text */
    gunichar first_char = g_utf8_get_char(text);
    
    /* Add space if: char before is not space AND first char is not space/punctuation */
    if (!g_unichar_isspace(uc) && !g_unichar_ispunct(uc) &&
        !g_unichar_isspace(first_char) && !g_unichar_ispunct(first_char)) {
      add_space_before = TRUE;
    }
  }

  /* Check if we need space after */
  if (caret < doc_len) {
    /* Check character at caret */
    const gchar *p = g_utf8_offset_to_pointer(full_text, caret);
    gunichar uc = g_utf8_get_char(p);
    
    /* Check last character of pasted text */
    gint text_len = g_utf8_strlen(text, -1);
    if (text_len > 0) {
      const gchar *last_p = g_utf8_offset_to_pointer(text, text_len - 1);
      gunichar last_char = g_utf8_get_char(last_p);
      
      /* Add space if: char at caret is not space AND last char is not space/punctuation */
      if (!g_unichar_isspace(uc) && !g_unichar_ispunct(uc) &&
          !g_unichar_isspace(last_char) && !g_unichar_ispunct(last_char)) {
        add_space_after = TRUE;
      }
    }
  }

  g_free(full_text);

  /* Build result string with spaces */
  GString *result = g_string_new("");
  if (add_space_before) {
    g_string_append_c(result, ' ');
    *out_added_before = 1;
  }
  g_string_append(result, text);
  if (add_space_after) {
    g_string_append_c(result, ' ');
    *out_added_after = 1;
  }

  gchar *ret = result->str;
  g_string_free(result, FALSE);
  return ret;
}
