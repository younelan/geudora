/*
 * Compose Window for gEudora
 * Creates a new message composition window matching original Eudora design
 * with headers (To, Cc, Bcc, Subject), formatting toolbar, and gEditCtrl editor
 */

#include "compose_window.h"
#include "../gEditCtrl/geditctrl.h"
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* Compose window structure */
typedef struct {
    GtkWidget *window;
    GtkWidget *to_entry;
    GtkWidget *cc_entry;
    GtkWidget *bcc_entry;
    GtkWidget *subject_entry;
    GtkWidget *editor;  /* GtkTextView widget */
    
    /* Formatting toolbar buttons */
    GtkWidget *bold_button;
    GtkWidget *italic_button;
    GtkWidget *underline_button;
    
    /* Compose toolbar buttons */
    GtkWidget *queue_button;
    GtkWidget *send_button;
    GtkWidget *save_stationery_button;
    GtkWidget *format_toolbar_toggle;
    GtkWidget *mime_toggle;
    GtkWidget *attachments_toggle;
    GtkWidget *word_wrap_toggle;
    GtkWidget *keep_copy_toggle;
    GtkWidget *receipt_toggle;
    
    /* Formatting toolbar */
    GtkWidget *format_toolbar;
    gboolean format_toolbar_visible;
} ComposeWindowData;

/* Cleanup callback when window is destroyed */
static void on_compose_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (data) {
        g_free(data);
    }
}

/* Queue button clicked */
static void on_queue_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    
    if (!data) return;
    
    g_print("Queueing message...\n");
    /* TODO: Queue the message */
    gtk_window_close(GTK_WINDOW(data->window));
}

/* Send button clicked */
static void on_send_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    
    if (!data) return;
    
    const char *to = gtk_editable_get_text(GTK_EDITABLE(data->to_entry));
    const char *subject = gtk_editable_get_text(GTK_EDITABLE(data->subject_entry));
    
    if (!to || strlen(to) == 0) {
        GtkAlertDialog *dialog = gtk_alert_dialog_new("Please enter a recipient");
        gtk_alert_dialog_show(dialog, GTK_WINDOW(data->window));
        g_object_unref(dialog);
        return;
    }
    
    g_print("Sending message to: %s\n", to);
    g_print("Subject: %s\n", subject);
    
    /* TODO: Actually send the message */
    
    gtk_window_close(GTK_WINDOW(data->window));
}

/* Save stationery button clicked */
static void on_save_stationery_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    
    if (!data) return;
    
    g_print("Saving as stationery...\n");
    /* TODO: Save as stationery */
}

/* Format toolbar toggle */
static void on_format_toolbar_toggle(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    
    if (!data || !data->format_toolbar) return;
    
    data->format_toolbar_visible = !data->format_toolbar_visible;
    if (data->format_toolbar_visible) {
        gtk_widget_show(data->format_toolbar);
    } else {
        gtk_widget_hide(data->format_toolbar);
    }
}

/* MIME toggle */
static void on_mime_toggle(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_print("MIME toggle\n");
}

/* Attachments toggle */
static void on_attachments_toggle(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_print("Attachments toggle\n");
}

/* Word wrap toggle */
static void on_word_wrap_toggle(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    
    if (!data || !data->editor) return;
    
    GtkWrapMode mode = gtk_text_view_get_wrap_mode(GTK_TEXT_VIEW(data->editor));
    if (mode == GTK_WRAP_WORD) {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(data->editor), GTK_WRAP_NONE);
    } else {
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(data->editor), GTK_WRAP_WORD);
    }
}

/* Keep copy toggle */
static void on_keep_copy_toggle(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_print("Keep copy toggle\n");
}

/* Receipt toggle */
static void on_receipt_toggle(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_print("Receipt toggle\n");
}

/* Bold button clicked */
static void on_bold_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_apply_tag_by_name(buffer, "bold", &start, &end);
    }
}

/* Italic button clicked */
static void on_italic_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_apply_tag_by_name(buffer, "italic", &start, &end);
    }
}

/* Underline button clicked */
static void on_underline_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gtk_text_buffer_apply_tag_by_name(buffer, "underline", &start, &end);
    }
}

/* Color changed callback - applies color to selected text */
static void on_color_changed(GtkColorDialogButton *button, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    GtkWidget *editor = GTK_WIDGET(user_data);
    
    if (!editor || !GTK_IS_TEXT_VIEW(editor)) {
        g_warning("Invalid editor widget");
        return;
    }
    
    const GdkRGBA *color = gtk_color_dialog_button_get_rgba(button);
    if (!color) {
        g_warning("No color selected");
        return;
    }
    
    g_print("Color selected: R=%.2f G=%.2f B=%.2f A=%.2f\n", color->red, color->green, color->blue, color->alpha);
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        g_print("Applying color to selected text\n");
        
        /* Create a unique tag name for this color */
        gchar tag_name[128];
        g_snprintf(tag_name, sizeof(tag_name), "color_%d_%d_%d",
                   (int)(color->red * 255),
                   (int)(color->green * 255),
                   (int)(color->blue * 255));
        
        /* Check if tag already exists, if not create it */
        GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *tag = gtk_text_tag_table_lookup(tag_table, tag_name);
        
        if (!tag) {
            g_print("Creating new color tag: %s\n", tag_name);
            tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                            "foreground-rgba", color,
                                            NULL);
        }
        
        if (tag) {
            gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
            g_print("Color tag applied\n");
        }
    } else {
        g_print("No text selected\n");
    }
}

/* Font selector changed */
static void on_font_changed(GtkComboBox *combo, gpointer user_data) {
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    gchar *font_family = gtk_combo_box_get_active_id(combo);
    if (!font_family) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gchar tag_name[128];
        g_snprintf(tag_name, sizeof(tag_name), "font_%s", font_family);
        
        GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *tag = gtk_text_tag_table_lookup(tag_table, tag_name);
        
        if (!tag) {
            tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                            "family", font_family,
                                            NULL);
        }
        
        if (tag) {
            gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
        }
    }
}

/* Size selector changed */
static void on_size_changed(GtkComboBox *combo, gpointer user_data) {
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    gchar *size_str = gtk_combo_box_get_active_id(combo);
    if (!size_str) return;
    
    int size = atoi(size_str) * 1024;  /* Pango uses 1/1024 of a point */
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gchar tag_name[128];
        g_snprintf(tag_name, sizeof(tag_name), "size_%s", size_str);
        
        GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table(buffer);
        GtkTextTag *tag = gtk_text_tag_table_lookup(tag_table, tag_name);
        
        if (!tag) {
            tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                            "size", size,
                                            NULL);
        }
        
        if (tag) {
            gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
        }
    }
}

/* Image file selected callback */
static void on_image_file_selected(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    
    if (!data || !data->editor) return;
    
    GFile *file = gtk_file_dialog_open_finish(dialog, res, NULL);
    if (!file) return;
    
    gchar *path = g_file_get_path(file);
    if (!path) {
        g_object_unref(file);
        return;
    }
    
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(path, 300, 300, TRUE, &error);
    
    if (error) {
        g_warning("Failed to load image: %s", error->message);
        g_error_free(error);
        g_free(path);
        g_object_unref(file);
        return;
    }
    
    if (!pixbuf) {
        g_warning("Failed to create pixbuf from image");
        g_free(path);
        g_object_unref(file);
        return;
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter cursor;
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
    
    /* Create a tag for this image */
    gchar tag_name[128];
    g_snprintf(tag_name, sizeof(tag_name), "image_%p", pixbuf);
    
    GtkTextTag *tag = gtk_text_buffer_create_tag(buffer, tag_name,
                                                  "pixbuf", pixbuf,
                                                  NULL);
    
    /* Insert the image using the tag */
    GtkTextIter start = cursor;
    gtk_text_buffer_insert_with_tags(buffer, &cursor, " ", 1, tag, NULL);
    gtk_text_buffer_insert(buffer, &cursor, "\n", -1);
    
    g_print("Image inserted: %s\n", path);
    
    g_object_unref(pixbuf);
    g_free(path);
    g_object_unref(file);
}

/* Bullet list button clicked */
static void on_bullet_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        /* Add bullet point at start of line */
        GtkTextIter line_start = start;
        gtk_text_iter_set_line_offset(&line_start, 0);
        gtk_text_buffer_insert(buffer, &line_start, "• ", -1);
    } else {
        /* Insert bullet at cursor */
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
        GtkTextIter line_start = cursor;
        gtk_text_iter_set_line_offset(&line_start, 0);
        gtk_text_buffer_insert(buffer, &line_start, "• ", -1);
    }
}

/* Indent button clicked */
static void on_indent_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        /* Indent selected lines */
        gtk_text_iter_set_line_offset(&start, 0);
        while (gtk_text_iter_get_line(&start) <= gtk_text_iter_get_line(&end)) {
            gtk_text_buffer_insert(buffer, &start, "    ", -1);
            if (!gtk_text_iter_forward_line(&start)) break;
        }
    } else {
        /* Indent current line */
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
        GtkTextIter line_start = cursor;
        gtk_text_iter_set_line_offset(&line_start, 0);
        gtk_text_buffer_insert(buffer, &line_start, "    ", -1);
    }
}

/* Outdent button clicked */
static void on_outdent_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        /* Outdent selected lines */
        gtk_text_iter_set_line_offset(&start, 0);
        while (gtk_text_iter_get_line(&start) <= gtk_text_iter_get_line(&end)) {
            GtkTextIter line_end = start;
            if (gtk_text_iter_forward_chars(&line_end, 4)) {
                gchar *text = gtk_text_buffer_get_text(buffer, &start, &line_end, FALSE);
                if (g_str_has_prefix(text, "    ")) {
                    gtk_text_buffer_delete(buffer, &start, &line_end);
                }
                g_free(text);
            }
            if (!gtk_text_iter_forward_line(&start)) break;
        }
    } else {
        /* Outdent current line */
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
        GtkTextIter line_start = cursor;
        gtk_text_iter_set_line_offset(&line_start, 0);
        GtkTextIter line_end = line_start;
        if (gtk_text_iter_forward_chars(&line_end, 4)) {
            gchar *text = gtk_text_buffer_get_text(buffer, &line_start, &line_end, FALSE);
            if (g_str_has_prefix(text, "    ")) {
                gtk_text_buffer_delete(buffer, &line_start, &line_end);
            }
            g_free(text);
        }
    }
}

/* Align left button clicked */
static void on_align_left_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    gtk_text_view_set_justification(GTK_TEXT_VIEW(data->editor), GTK_JUSTIFY_LEFT);
}

/* Align center button clicked */
static void on_align_center_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    gtk_text_view_set_justification(GTK_TEXT_VIEW(data->editor), GTK_JUSTIFY_CENTER);
}

/* Align right button clicked */
static void on_align_right_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    gtk_text_view_set_justification(GTK_TEXT_VIEW(data->editor), GTK_JUSTIFY_RIGHT);
}

/* Insert image button clicked */
static void on_insert_image_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->window) return;
    
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Insert Image");
    
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.gif");
    gtk_file_filter_add_pattern(filter, "*.bmp");
    
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    
    gtk_file_dialog_open(dialog, GTK_WINDOW(data->window), NULL, 
                        on_image_file_selected, data);
    
    g_object_unref(filter);
    g_object_unref(filters);
    g_object_unref(dialog);
}

/* Insert link button clicked */
static void on_insert_link_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    ComposeWindowData *data = (ComposeWindowData *)user_data;
    if (!data || !data->editor) return;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(data->editor));
    GtkTextIter start, end;
    
    if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
        gchar *selected_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        
        /* Create a simple link format: [text](url) */
        gchar *link_text = g_strdup_printf("[%s](http://)", selected_text);
        gtk_text_buffer_delete(buffer, &start, &end);
        gtk_text_buffer_insert(buffer, &start, link_text, -1);
        
        g_free(selected_text);
        g_free(link_text);
    } else {
        /* Insert empty link at cursor */
        GtkTextIter cursor;
        gtk_text_buffer_get_iter_at_mark(buffer, &cursor, gtk_text_buffer_get_insert(buffer));
        gtk_text_buffer_insert(buffer, &cursor, "[link](http://)", -1);
    }
}

/* Create a new compose window */
GtkWidget* create_compose_window(GtkWindow *parent) {
    ComposeWindowData *data = g_new0(ComposeWindowData, 1);
    
    /* Create main window */
    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "New Message");
    gtk_window_set_modal(GTK_WINDOW(window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(window), parent);
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 700);
    
    data->window = window;
    
    /* Create main vertical box */
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);
    
    /* Create header area with To, Cc, Bcc, Subject fields */
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(header_box, 12);
    gtk_widget_set_margin_end(header_box, 12);
    gtk_widget_set_margin_top(header_box, 12);
    gtk_widget_set_margin_bottom(header_box, 6);
    gtk_box_append(GTK_BOX(main_box), header_box);
    
    /* To field */
    GtkWidget *to_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(to_box), gtk_label_new("To:"));
    data->to_entry = gtk_entry_new();
    gtk_widget_set_hexpand(data->to_entry, TRUE);
    gtk_widget_set_focus_on_click(data->to_entry, FALSE);  /* Don't grab focus on click */
    gtk_box_append(GTK_BOX(to_box), data->to_entry);
    gtk_box_append(GTK_BOX(header_box), to_box);
    
    /* Cc field */
    GtkWidget *cc_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(cc_box), gtk_label_new("Cc:"));
    data->cc_entry = gtk_entry_new();
    gtk_widget_set_hexpand(data->cc_entry, TRUE);
    gtk_widget_set_focus_on_click(data->cc_entry, FALSE);
    gtk_box_append(GTK_BOX(cc_box), data->cc_entry);
    gtk_box_append(GTK_BOX(header_box), cc_box);
    
    /* Bcc field */
    GtkWidget *bcc_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(bcc_box), gtk_label_new("Bcc:"));
    data->bcc_entry = gtk_entry_new();
    gtk_widget_set_hexpand(data->bcc_entry, TRUE);
    gtk_widget_set_focus_on_click(data->bcc_entry, FALSE);
    gtk_box_append(GTK_BOX(bcc_box), data->bcc_entry);
    gtk_box_append(GTK_BOX(header_box), bcc_box);
    
    /* Subject field */
    GtkWidget *subject_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(subject_box), gtk_label_new("Subject:"));
    data->subject_entry = gtk_entry_new();
    gtk_widget_set_hexpand(data->subject_entry, TRUE);
    gtk_widget_set_focus_on_click(data->subject_entry, FALSE);
    gtk_box_append(GTK_BOX(subject_box), data->subject_entry);
    gtk_box_append(GTK_BOX(header_box), subject_box);
    
    /* Separator */
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    /* Create compose toolbar (top toolbar with Queue, Send, Save Stationery, etc.) */
    GtkWidget *compose_toolbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(compose_toolbar_box, 12);
    gtk_widget_set_margin_end(compose_toolbar_box, 12);
    gtk_widget_set_margin_top(compose_toolbar_box, 8);
    gtk_widget_set_margin_bottom(compose_toolbar_box, 8);
    gtk_box_append(GTK_BOX(main_box), compose_toolbar_box);
    
    /* Queue button */
    data->queue_button = gtk_button_new_with_label("Queue");
    g_signal_connect(data->queue_button, "clicked", G_CALLBACK(on_queue_clicked), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->queue_button);
    
    /* Send button */
    data->send_button = gtk_button_new_with_label("Send");
    g_signal_connect(data->send_button, "clicked", G_CALLBACK(on_send_clicked), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->send_button);
    
    /* Save Stationery button */
    data->save_stationery_button = gtk_button_new_with_label("Save Stationery");
    g_signal_connect(data->save_stationery_button, "clicked", G_CALLBACK(on_save_stationery_clicked), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->save_stationery_button);
    
    /* Separator */
    gtk_box_append(GTK_BOX(compose_toolbar_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    /* Format toolbar toggle */
    data->format_toolbar_toggle = gtk_toggle_button_new_with_label("Format");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->format_toolbar_toggle), TRUE);
    g_signal_connect(data->format_toolbar_toggle, "clicked", G_CALLBACK(on_format_toolbar_toggle), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->format_toolbar_toggle);
    
    /* MIME toggle */
    data->mime_toggle = gtk_toggle_button_new_with_label("MIME");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->mime_toggle), TRUE);
    g_signal_connect(data->mime_toggle, "clicked", G_CALLBACK(on_mime_toggle), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->mime_toggle);
    
    /* Attachments toggle */
    data->attachments_toggle = gtk_toggle_button_new_with_label("Attachments");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->attachments_toggle), TRUE);
    g_signal_connect(data->attachments_toggle, "clicked", G_CALLBACK(on_attachments_toggle), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->attachments_toggle);
    
    /* Word wrap toggle */
    data->word_wrap_toggle = gtk_toggle_button_new_with_label("Wrap");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->word_wrap_toggle), TRUE);
    g_signal_connect(data->word_wrap_toggle, "clicked", G_CALLBACK(on_word_wrap_toggle), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->word_wrap_toggle);
    
    /* Keep copy toggle */
    data->keep_copy_toggle = gtk_toggle_button_new_with_label("Keep Copy");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->keep_copy_toggle), TRUE);
    g_signal_connect(data->keep_copy_toggle, "clicked", G_CALLBACK(on_keep_copy_toggle), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->keep_copy_toggle);
    
    /* Receipt toggle */
    data->receipt_toggle = gtk_toggle_button_new_with_label("Receipt");
    g_signal_connect(data->receipt_toggle, "clicked", G_CALLBACK(on_receipt_toggle), data);
    gtk_box_append(GTK_BOX(compose_toolbar_box), data->receipt_toggle);
    
    /* Add spacer */
    GtkWidget *spacer2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer2, TRUE);
    gtk_box_append(GTK_BOX(compose_toolbar_box), spacer2);
    
    /* Separator */
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    /* Create formatting toolbar */
    data->format_toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *toolbar_box = data->format_toolbar;
    gtk_widget_set_margin_start(toolbar_box, 12);
    gtk_widget_set_margin_end(toolbar_box, 12);
    gtk_widget_set_margin_top(toolbar_box, 8);
    gtk_widget_set_margin_bottom(toolbar_box, 8);
    gtk_box_append(GTK_BOX(main_box), toolbar_box);
    
    /* Bold button */
    data->bold_button = gtk_toggle_button_new_with_label("B");
    gtk_widget_set_can_focus(data->bold_button, FALSE);
    gtk_widget_set_tooltip_text(data->bold_button, "Bold (Ctrl+B)");
    g_signal_connect(data->bold_button, "clicked", G_CALLBACK(on_bold_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), data->bold_button);
    
    /* Italic button */
    data->italic_button = gtk_toggle_button_new_with_label("I");
    gtk_widget_set_can_focus(data->italic_button, FALSE);
    gtk_widget_set_tooltip_text(data->italic_button, "Italic (Ctrl+I)");
    g_signal_connect(data->italic_button, "clicked", G_CALLBACK(on_italic_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), data->italic_button);
    
    /* Underline button */
    data->underline_button = gtk_toggle_button_new_with_label("U");
    gtk_widget_set_can_focus(data->underline_button, FALSE);
    gtk_widget_set_tooltip_text(data->underline_button, "Underline (Ctrl+U)");
    g_signal_connect(data->underline_button, "clicked", G_CALLBACK(on_underline_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), data->underline_button);
    
    /* Separator in toolbar */
    gtk_box_append(GTK_BOX(toolbar_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    /* Color picker button - using exact main.c pattern */
    GtkColorDialog *color_dialog = gtk_color_dialog_new();
    GtkWidget *btn_color = gtk_color_dialog_button_new(color_dialog);
    gtk_widget_set_can_focus(btn_color, FALSE);
    gtk_widget_set_tooltip_text(btn_color, "Text Color");
    gtk_box_append(GTK_BOX(toolbar_box), btn_color);
    g_signal_connect(btn_color, "notify::rgba", G_CALLBACK(on_color_changed), data->editor);
    /* Don't unref - the button holds a reference */
    
    /* Font selector */
    GtkWidget *font_label = gtk_label_new("Font:");
    gtk_box_append(GTK_BOX(toolbar_box), font_label);
    
    GtkWidget *font_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(font_combo), "sans", "Sans");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(font_combo), "serif", "Serif");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(font_combo), "monospace", "Monospace");
    gtk_combo_box_set_active(GTK_COMBO_BOX(font_combo), 0);
    gtk_widget_set_can_focus(font_combo, FALSE);
    g_signal_connect(font_combo, "changed", G_CALLBACK(on_font_changed), data);
    gtk_box_append(GTK_BOX(toolbar_box), font_combo);
    
    /* Size selector */
    GtkWidget *size_label = gtk_label_new("Size:");
    gtk_box_append(GTK_BOX(toolbar_box), size_label);
    
    GtkWidget *size_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "8", "8pt");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "10", "10pt");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "12", "12pt");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "14", "14pt");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "16", "16pt");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "18", "18pt");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(size_combo), "20", "20pt");
    gtk_combo_box_set_active(GTK_COMBO_BOX(size_combo), 2);  /* Default to 12pt */
    gtk_widget_set_can_focus(size_combo, FALSE);
    g_signal_connect(size_combo, "changed", G_CALLBACK(on_size_changed), data);
    gtk_box_append(GTK_BOX(toolbar_box), size_combo);
    
    /* Separator */
    gtk_box_append(GTK_BOX(toolbar_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    /* Bullet list button */
    GtkWidget *bullet_button = gtk_toggle_button_new_with_label("•");
    gtk_widget_set_can_focus(bullet_button, FALSE);
    gtk_widget_set_tooltip_text(bullet_button, "Bullet List");
    g_signal_connect(bullet_button, "clicked", G_CALLBACK(on_bullet_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), bullet_button);
    
    /* Indent button */
    GtkWidget *indent_button = gtk_button_new_with_label("→");
    gtk_widget_set_can_focus(indent_button, FALSE);
    gtk_widget_set_tooltip_text(indent_button, "Increase Indent");
    g_signal_connect(indent_button, "clicked", G_CALLBACK(on_indent_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), indent_button);
    
    /* Outdent button */
    GtkWidget *outdent_button = gtk_button_new_with_label("←");
    gtk_widget_set_can_focus(outdent_button, FALSE);
    gtk_widget_set_tooltip_text(outdent_button, "Decrease Indent");
    g_signal_connect(outdent_button, "clicked", G_CALLBACK(on_outdent_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), outdent_button);
    
    /* Separator */
    gtk_box_append(GTK_BOX(toolbar_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    /* Align left button */
    GtkWidget *align_left_button = gtk_toggle_button_new_with_label("⬅");
    gtk_widget_set_can_focus(align_left_button, FALSE);
    gtk_widget_set_tooltip_text(align_left_button, "Align Left");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(align_left_button), TRUE);
    g_signal_connect(align_left_button, "clicked", G_CALLBACK(on_align_left_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), align_left_button);
    
    /* Align center button */
    GtkWidget *align_center_button = gtk_toggle_button_new_with_label("⬇");
    gtk_widget_set_can_focus(align_center_button, FALSE);
    gtk_widget_set_tooltip_text(align_center_button, "Align Center");
    g_signal_connect(align_center_button, "clicked", G_CALLBACK(on_align_center_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), align_center_button);
    
    /* Align right button */
    GtkWidget *align_right_button = gtk_toggle_button_new_with_label("➡");
    gtk_widget_set_can_focus(align_right_button, FALSE);
    gtk_widget_set_tooltip_text(align_right_button, "Align Right");
    g_signal_connect(align_right_button, "clicked", G_CALLBACK(on_align_right_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), align_right_button);
    
    /* Separator */
    gtk_box_append(GTK_BOX(toolbar_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL));
    
    /* Insert image button */
    GtkWidget *image_button = gtk_button_new_with_label("🖼");
    gtk_widget_set_can_focus(image_button, FALSE);
    gtk_widget_set_tooltip_text(image_button, "Insert Image");
    g_signal_connect(image_button, "clicked", G_CALLBACK(on_insert_image_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), image_button);
    
    /* Insert link button */
    GtkWidget *link_button = gtk_button_new_with_label("🔗");
    gtk_widget_set_can_focus(link_button, FALSE);
    gtk_widget_set_tooltip_text(link_button, "Insert Link");
    g_signal_connect(link_button, "clicked", G_CALLBACK(on_insert_link_clicked), data);
    gtk_box_append(GTK_BOX(toolbar_box), link_button);
    
    /* Add spacer */
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(toolbar_box), spacer);
    
    /* Separator */
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    /* Create editor area with GtkTextView */
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_widget_set_hexpand(text_view, TRUE);
    gtk_widget_set_vexpand(text_view, TRUE);
    gtk_widget_set_size_request(text_view, 600, 400);
    
    /* Store the text view as the editor */
    data->editor = text_view;
    
    /* Add to main box */
    gtk_box_append(GTK_BOX(main_box), text_view);
    
    /* Grab focus on the text view */
    gtk_widget_grab_focus(text_view);
    
    /* Set initial text */
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    
    /* Create text tags for formatting */
    gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);
    gtk_text_buffer_create_tag(buffer, "underline", "underline", PANGO_UNDERLINE_SINGLE, NULL);
    
    /* Start with empty text */
    gtk_text_buffer_set_text(buffer, "", -1);
    
    /* Separator */
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    
    /* Store data on window for later retrieval */
    g_object_set_data_full(G_OBJECT(window), "compose-data", data, (GDestroyNotify)g_free);
    
    /* Connect destroy signal */
    g_signal_connect(window, "destroy", G_CALLBACK(on_compose_window_destroy), data);
    
    return window;
}

/* Get the editor control from a compose window */
GtkWidget* compose_window_get_editor(GtkWidget *compose_window) {
    if (!GTK_IS_WINDOW(compose_window)) {
        return NULL;
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    return data ? data->editor : NULL;
}

/* Get the message text from a compose window */
gchar* compose_window_get_text(GtkWidget *compose_window) {
    GtkWidget *editor = compose_window_get_editor(compose_window);
    if (!editor) {
        return g_strdup("");
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    return text ? text : g_strdup("");
}

/* Set the message text in a compose window */
void compose_window_set_text(GtkWidget *compose_window, const gchar *text) {
    GtkWidget *editor = compose_window_get_editor(compose_window);
    if (!editor || !text) {
        return;
    }
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor));
    gtk_text_buffer_set_text(buffer, text, -1);
}

/* Get the To field from a compose window */
gchar* compose_window_get_to(GtkWidget *compose_window) {
    if (!GTK_IS_WINDOW(compose_window)) {
        return g_strdup("");
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->to_entry) {
        return g_strdup("");
    }
    
    const char *text = gtk_editable_get_text(GTK_EDITABLE(data->to_entry));
    return g_strdup(text ? text : "");
}

/* Set the To field in a compose window */
void compose_window_set_to(GtkWidget *compose_window, const gchar *to) {
    if (!GTK_IS_WINDOW(compose_window) || !to) {
        return;
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->to_entry) {
        return;
    }
    
    gtk_editable_set_text(GTK_EDITABLE(data->to_entry), to);
}

/* Get the Subject field from a compose window */
gchar* compose_window_get_subject(GtkWidget *compose_window) {
    if (!GTK_IS_WINDOW(compose_window)) {
        return g_strdup("");
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->subject_entry) {
        return g_strdup("");
    }
    
    const char *text = gtk_editable_get_text(GTK_EDITABLE(data->subject_entry));
    return g_strdup(text ? text : "");
}

/* Set the Subject field in a compose window */
void compose_window_set_subject(GtkWidget *compose_window, const gchar *subject) {
    if (!GTK_IS_WINDOW(compose_window) || !subject) {
        return;
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->subject_entry) {
        return;
    }
    
    gtk_editable_set_text(GTK_EDITABLE(data->subject_entry), subject);
}

/* Get the Cc field from a compose window */
gchar* compose_window_get_cc(GtkWidget *compose_window) {
    if (!GTK_IS_WINDOW(compose_window)) {
        return g_strdup("");
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->cc_entry) {
        return g_strdup("");
    }
    
    const char *text = gtk_editable_get_text(GTK_EDITABLE(data->cc_entry));
    return g_strdup(text ? text : "");
}

/* Set the Cc field in a compose window */
void compose_window_set_cc(GtkWidget *compose_window, const gchar *cc) {
    if (!GTK_IS_WINDOW(compose_window) || !cc) {
        return;
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->cc_entry) {
        return;
    }
    
    gtk_editable_set_text(GTK_EDITABLE(data->cc_entry), cc);
}

/* Get the Bcc field from a compose window */
gchar* compose_window_get_bcc(GtkWidget *compose_window) {
    if (!GTK_IS_WINDOW(compose_window)) {
        return g_strdup("");
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->bcc_entry) {
        return g_strdup("");
    }
    
    const char *text = gtk_editable_get_text(GTK_EDITABLE(data->bcc_entry));
    return g_strdup(text ? text : "");
}

/* Set the Bcc field in a compose window */
void compose_window_set_bcc(GtkWidget *compose_window, const gchar *bcc) {
    if (!GTK_IS_WINDOW(compose_window) || !bcc) {
        return;
    }
    
    ComposeWindowData *data = (ComposeWindowData *)g_object_get_data(G_OBJECT(compose_window), "compose-data");
    if (!data || !data->bcc_entry) {
        return;
    }
    
    gtk_editable_set_text(GTK_EDITABLE(data->bcc_entry), bcc);
}
