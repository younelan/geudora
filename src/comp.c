/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "fileutil.h"
#include "comp.h"
#include "sendmail.h"
#include "MyRes.h"
#define SEND_ITEM 100
#define SAVE_ITEM 101
#ifndef PREF_188
#define PREF_188 188
#endif
#include "../gEditCtrl/geditctrl.h"
#include "../gEditCtrl/gedit-state.h"
#include "Globals.h"
#include "theme.h"
#include "StringDefs.h"
#include "gtk_prefs.h"
#include "mailbox.h"
#include "message.h"
#include "prefdefs.h"
#include "toc.h"
#include "emoticon.h"
#include "util.h"
#include "threading.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define FILE_NUM 7
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

/* Missing function declarations - use compatible signatures */
void SetMyWindowPrivateData(MyWindowPtr win, void *privateData);
/* CloseMyWindow, InvalContent, UpdateMyWindow, GetSumColor declared in headers */
void AttachSelect(MessHandle messH);
int SetWinMinSize(MyWindowPtr win, int width, int height);
bool IsColorWin(void *winWP);
int MessFind(MyWindowPtr win);
int MessagePosition(MyWindowPtr win);
int GetMessageLength(TOCType * tocH, int sumNum);
int ReadMessage(TOCType * tocH, int sumNum, unsigned char *buffer);
bool ShowMyWindow(void *winWP);
/* AWrite renamed to file_write — included via fileutil.h */

#ifndef OPT_COMP_TOOLBAR_VISIBLE
#define OPT_COMP_TOOLBAR_VISIBLE (1 << 7)
#endif
#ifndef PREF_COMP_TOOLBAR
#define PREF_COMP_TOOLBAR PREF_188
#endif
#ifndef MessZoomSize
#define MessZoomSize 0
#endif
#ifndef CompZoomSize
#define CompZoomSize 1
#endif

/************************************************************************
 * private function declarations - ported to use gEditCtrl instead of Pete
 ************************************************************************/
int GetCompTexts(MessHandle messH, bool new);
void MakeCompTitle(char *string, TOCType * tocH, MessHandle messH, int sumNum);
int WriteComp(MessHandle messH, short refN, long offset);
char *GetMyHostname(char *hostname);
int CompStripHeaderReturns(MessHandle messH);
int SuckDragAddresses(void *drag, char **addresses, bool leadingComma,
                      bool trailingComma);
int FindAndMarkSigSep(GtkWidget *pte);
long FindSigSep(GtkWidget *pte);
void SepStyle(void *pip, void *tsp, void **graphic, int pgt);
int CompGetDragContents(GtkWidget *pte, char **theText, void **theStyles,
                        void **theParas, void *drag, long dropLocation);
void CompBeautifyFrom(char *name);
char *CompCurAddr(MyWindowPtr win, char *addr);

/* Forward declarations for window management functions */
bool CompClose(MyWindowPtr win);
static gboolean on_comp_close_request(GtkWindow *window, gpointer user_data);
static void on_comp_body_changed(geditDocument *doc, gpointer user_data);
void CompDidResize(MyWindowPtr win);
bool CompClick(MyWindowPtr win, GdkEvent *event);
bool CompMenu(MyWindowPtr win, int menuItem);
bool CompKey(MyWindowPtr win, GdkEvent *event);
bool CompButton(MyWindowPtr win, GtkWidget *button, GdkEvent *event);
void CompHelp(MyWindowPtr win, int helpType);
void CompGonnaShow(MyWindowPtr win);
bool CompDragHandler(MyWindowPtr win, void *dragEvent);
void CompIdle(MyWindowPtr win);
bool CompSend(MessHandle messH);
bool CompSave(MessHandle messH);
int CreateMessageBody(char *buffer, unsigned long *uidHash);
int GatherCompAddresses(MyWindowPtr win, char *addrList);

/* ── Style tracking: update toolbar to reflect caret style ── */
static void on_comp_selection_changed(geditDocument *doc, gpointer ud) {
  GtkWidget *body_ctrl = GTK_WIDGET(ud);

  GtkWidget *bold_tb = g_object_get_data(G_OBJECT(body_ctrl), "fmt-bold");
  GtkWidget *italic_tb = g_object_get_data(G_OBJECT(body_ctrl), "fmt-italic");
  GtkWidget *underline_tb = g_object_get_data(G_OBJECT(body_ctrl), "fmt-underline");
  GtkWidget *color_btn = g_object_get_data(G_OBJECT(body_ctrl), "fmt-color");
  GtkWidget *font_dd = g_object_get_data(G_OBJECT(body_ctrl), "fmt-font");
  GtkWidget *size_dd = g_object_get_data(G_OBJECT(body_ctrl), "fmt-size");

  gint caret = geditctrl_get_caret_offset(body_ctrl);
  geditStyleRun style = {0};
  gedit_document_get_style_at(doc, caret > 0 ? caret - 1 : 0, &style);

  /* Block signals to avoid feedback loops */
  if (bold_tb) {
    g_signal_handlers_block_matched(bold_tb, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bold_tb), style.bold);
    g_signal_handlers_unblock_matched(bold_tb, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
  }
  if (italic_tb) {
    g_signal_handlers_block_matched(italic_tb, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(italic_tb), style.italic);
    g_signal_handlers_unblock_matched(italic_tb, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
  }
  if (underline_tb) {
    g_signal_handlers_block_matched(underline_tb, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(underline_tb), style.underline);
    g_signal_handlers_unblock_matched(underline_tb, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
  }
  if (color_btn) {
    gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(color_btn), &style.color);
  }
  if (size_dd) {
    static const gint sizes[] = {8, 10, 12, 14, 16, 18, 20, 24, 28, 36};
    gint sz = style.font_size > 0 ? style.font_size : 12;
    guint best = 2; /* default to 12pt */
    for (guint i = 0; i < G_N_ELEMENTS(sizes); i++) {
      if (sizes[i] == sz) { best = i; break; }
    }
    g_signal_handlers_block_matched(size_dd, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(size_dd), best);
    g_signal_handlers_unblock_matched(size_dd, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
  }
  if (font_dd) {
    static const char *families[] = {"Sans", "Serif", "Monospace",
                                     "Helvetica", "Times", "Courier"};
    guint best = 0;
    if (style.font_family) {
      for (guint i = 0; i < G_N_ELEMENTS(families); i++) {
        if (g_ascii_strcasecmp(style.font_family, families[i]) == 0) {
          best = i; break;
        }
      }
    }
    g_signal_handlers_block_matched(font_dd, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(font_dd), best);
    g_signal_handlers_unblock_matched(font_dd, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, body_ctrl);
  }
}

/* ── Formatting toolbar callbacks ── */
static void on_comp_bold(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_toggle_style((GtkWidget *)ud, TRUE, FALSE, FALSE);
}
static void on_comp_italic(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_toggle_style((GtkWidget *)ud, FALSE, TRUE, FALSE);
}
static void on_comp_underline(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_toggle_style((GtkWidget *)ud, FALSE, FALSE, TRUE);
}
static void on_comp_clear_style(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_clear_style((GtkWidget *)ud);
}
static void on_comp_zoom_in(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_change_font_size((GtkWidget *)ud, 2);
}
static void on_comp_zoom_out(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_change_font_size((GtkWidget *)ud, -2);
}
static void on_comp_align_left(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_set_alignment((GtkWidget *)ud, gedit_ALIGN_LEFT);
}
static void on_comp_align_center(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_set_alignment((GtkWidget *)ud, gedit_ALIGN_CENTER);
}
static void on_comp_align_right(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_set_alignment((GtkWidget *)ud, gedit_ALIGN_RIGHT);
}
static void on_comp_indent(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_indent((GtkWidget *)ud, 20);
}
static void on_comp_outdent(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_indent((GtkWidget *)ud, -20);
}
static void on_comp_bullet(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_toggle_bullet((GtkWidget *)ud);
}
static void on_comp_insert_hr(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_insert_hr((GtkWidget *)ud);
}
static void on_comp_quote_inc(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_change_quote_level((GtkWidget *)ud, 1);
}
static void on_comp_quote_dec(GtkButton *btn, gpointer ud) {
  (void)btn;
  geditctrl_change_quote_level((GtkWidget *)ud, -1);
}
static void on_comp_color_changed(GtkColorDialogButton *btn, GParamSpec *pspec,
                                  gpointer ud) {
  (void)pspec;
  const GdkRGBA *color = gtk_color_dialog_button_get_rgba(btn);
  if (color)
    geditctrl_set_color((GtkWidget *)ud, color);
}
static void on_comp_font_size_changed(GtkDropDown *dd, GParamSpec *pspec,
                                      gpointer ud) {
  (void)pspec;
  static const gint sizes[] = {8, 10, 12, 14, 16, 18, 20, 24, 28, 36};
  guint sel = gtk_drop_down_get_selected(dd);
  if (sel < G_N_ELEMENTS(sizes))
    geditctrl_set_font_size((GtkWidget *)ud, sizes[sel]);
}
static void on_comp_font_family_changed(GtkDropDown *dd, GParamSpec *pspec,
                                        gpointer ud) {
  (void)pspec;
  static const char *families[] = {"Sans", "Serif", "Monospace",
                                   "Helvetica", "Times", "Courier"};
  guint sel = gtk_drop_down_get_selected(dd);
  if (sel < G_N_ELEMENTS(families))
    geditctrl_set_font_family((GtkWidget *)ud, families[sel]);
}
/* Insert image via file dialog */
static void on_comp_image_selected(GObject *src, GAsyncResult *res, gpointer ud) {
  GtkFileDialog *dlg = GTK_FILE_DIALOG(src);
  GError *err = NULL;
  GFile *file = gtk_file_dialog_open_finish(dlg, res, &err);
  if (file) {
    char *path = g_file_get_path(file);
    if (path) {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, NULL);
      if (pixbuf) {
        int w = gdk_pixbuf_get_width(pixbuf);
        int h = gdk_pixbuf_get_height(pixbuf);
        /* Scale down if too large */
        if (w > 640) { h = h * 640 / w; w = 640; }
        geditctrl_insert_image((GtkWidget *)ud, pixbuf, w, h);
        g_object_unref(pixbuf);
      }
      g_free(path);
    }
    g_object_unref(file);
  }
  if (err) g_error_free(err);
}
static void on_comp_insert_image(GtkButton *btn, gpointer ud) {
  (void)btn;
  GtkWidget *ctrl = (GtkWidget *)ud;
  GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(ctrl));
  GtkFileDialog *dlg = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dlg, "Insert Image");
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, "Images");
  gtk_file_filter_add_pattern(filter, "*.png");
  gtk_file_filter_add_pattern(filter, "*.jpg");
  gtk_file_filter_add_pattern(filter, "*.jpeg");
  gtk_file_filter_add_pattern(filter, "*.gif");
  gtk_file_filter_add_pattern(filter, "*.bmp");
  GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
  g_list_store_append(filters, filter);
  gtk_file_dialog_set_filters(dlg, G_LIST_MODEL(filters));
  gtk_file_dialog_open(dlg, GTK_WINDOW(toplevel), NULL,
                       on_comp_image_selected, ctrl);
  g_object_unref(filter);
  g_object_unref(filters);
  g_object_unref(dlg);
}

/* Insert/edit link via a small dialog */
static void on_comp_link_ok(GtkWidget *btn, gpointer ud) {
  (void)btn;
  GtkWidget *dlg = GTK_WIDGET(ud);
  GtkWidget *url_entry = g_object_get_data(G_OBJECT(dlg), "url-entry");
  GtkWidget *text_entry = g_object_get_data(G_OBJECT(dlg), "text-entry");
  GtkWidget *ctrl = g_object_get_data(G_OBJECT(dlg), "body-ctrl");
  const gchar *url = gtk_editable_get_text(GTK_EDITABLE(url_entry));
  const gchar *text = gtk_editable_get_text(GTK_EDITABLE(text_entry));

  if (!url || !url[0]) {
    gtk_window_destroy(GTK_WINDOW(dlg));
    return;
  }

  /* If text is empty, use the URL as display text */
  if (!text || !text[0])
    text = url;

  geditctrl_insert_link(ctrl, url, text);
  gtk_window_destroy(GTK_WINDOW(dlg));
}
/* ── Emoji picker popover ── */
static void on_emoji_clicked(GtkButton *btn, gpointer ud) {
  GtkWidget *body_ctrl = (GtkWidget *)ud;
  int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "emo-idx"));

  /* Use gEditCtrl's proper insert path (undo, state, redraw) */
  geditctrl_insert_emoji(body_ctrl, EmoGetEmoji(idx));

  /* Close the popover */
  GtkWidget *popover = gtk_widget_get_ancestor(GTK_WIDGET(btn), GTK_TYPE_POPOVER);
  if (popover) gtk_popover_popdown(GTK_POPOVER(popover));

  /* Return focus to the editor drawing area so typing works */
  GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(body_ctrl));
  if (area) gtk_widget_grab_focus(area);
}

static GtkWidget *create_emoji_popover(GtkWidget *body_ctrl) {
  EmoInit();
  int count = EmoCount();

  GtkWidget *popover = gtk_popover_new();
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scroll, 280, 240);

  GtkWidget *grid = gtk_flow_box_new();
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(grid), 8);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(grid), GTK_SELECTION_NONE);
  gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(grid), TRUE);

  /* De-duplicate: only show each unique emoji once */
  int shown = 0;
  for (int i = 0; i < count && shown < 40; i++) {
    if (i > 0 && strcmp(EmoGetEmoji(i), EmoGetEmoji(i - 1)) == 0)
      continue;
    GtkWidget *btn = gtk_button_new_with_label(EmoGetEmoji(i));
    gtk_widget_set_tooltip_text(btn, EmoGetMeaning(i));
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_margin_start(btn, 1);
    gtk_widget_set_margin_end(btn, 1);
    g_object_set_data(G_OBJECT(btn), "emo-idx", GINT_TO_POINTER(i));
    g_signal_connect(btn, "clicked", G_CALLBACK(on_emoji_clicked), body_ctrl);
    gtk_flow_box_append(GTK_FLOW_BOX(grid), btn);
    shown++;
  }

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), grid);
  gtk_popover_set_child(GTK_POPOVER(popover), scroll);

  return popover;
}

static void on_comp_insert_link(GtkButton *btn, gpointer ud) {
  (void)btn;
  GtkWidget *ctrl = (GtkWidget *)ud;
  GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(ctrl));

  /* Check if caret is already on a link */
  gint caret = geditctrl_get_caret_offset(ctrl);
  gchar *existing_url = geditctrl_get_link_at(ctrl, caret);

  /* Get selected text if any */
  gchar *sel_text = NULL;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (doc) {
    GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(ctrl));
    GEditCtrlState *s = area ? gedit_state_for_area(area) : NULL;
    if (s && s->sel_start != s->sel_end) {
      gint a = MIN(s->sel_start, s->sel_end);
      gint b = MAX(s->sel_start, s->sel_end);
      sel_text = gedit_document_get_text_range(doc, a, b - a);
    }
  }

  GtkWidget *dlg = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Insert Link");
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(toplevel));
  gtk_window_set_default_size(GTK_WINDOW(dlg), 400, -1);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  gtk_window_set_child(GTK_WINDOW(dlg), box);

  GtkWidget *text_label = gtk_label_new("Text:");
  gtk_widget_set_halign(text_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), text_label);

  GtkWidget *text_entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(text_entry), sel_text ? sel_text : "");
  gtk_entry_set_placeholder_text(GTK_ENTRY(text_entry), "Link text (leave empty to use URL)");
  gtk_box_append(GTK_BOX(box), text_entry);

  GtkWidget *url_label = gtk_label_new("URL:");
  gtk_widget_set_halign(url_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), url_label);

  GtkWidget *url_entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(url_entry),
                        existing_url ? existing_url : "https://");
  gtk_box_append(GTK_BOX(box), url_entry);

  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(box), btn_box);

  GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
  GtkWidget *ok_btn = gtk_button_new_with_label("OK");
  gtk_box_append(GTK_BOX(btn_box), cancel_btn);
  gtk_box_append(GTK_BOX(btn_box), ok_btn);

  g_object_set_data(G_OBJECT(dlg), "url-entry", url_entry);
  g_object_set_data(G_OBJECT(dlg), "text-entry", text_entry);
  g_object_set_data(G_OBJECT(dlg), "body-ctrl", ctrl);

  g_signal_connect_swapped(cancel_btn, "clicked",
                           G_CALLBACK(gtk_window_destroy), dlg);
  g_signal_connect(ok_btn, "clicked", G_CALLBACK(on_comp_link_ok), dlg);

  g_free(existing_url);
  g_free(sel_text);
  gtk_window_present(GTK_WINDOW(dlg));
}

/* ── Icon bar toggle callbacks ── */
static void on_comp_qp_toggled(GtkToggleButton *btn, gpointer ud) {
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  if (gtk_toggle_button_get_active(btn))
    SetMessFlag(messH, FLAG_CAN_ENC);
  else
    ClearMessFlag(messH, FLAG_CAN_ENC);
}
static void on_comp_textonly_toggled(GtkToggleButton *btn, gpointer ud) {
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  if (gtk_toggle_button_get_active(btn))
    SetMessFlag(messH, FLAG_BX_TEXT);
  else
    ClearMessFlag(messH, FLAG_BX_TEXT);
}
static void on_comp_wrap_toggled(GtkToggleButton *btn, gpointer ud) {
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  if (gtk_toggle_button_get_active(btn))
    SetMessFlag(messH, FLAG_WRAP_OUT);
  else
    ClearMessFlag(messH, FLAG_WRAP_OUT);
}
static void on_comp_keep_toggled(GtkToggleButton *btn, gpointer ud) {
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  if (gtk_toggle_button_get_active(btn))
    SetMessFlag(messH, FLAG_KEEP_COPY);
  else
    ClearMessFlag(messH, FLAG_KEEP_COPY);
}
static void on_comp_rr_toggled(GtkToggleButton *btn, gpointer ud) {
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  if (gtk_toggle_button_get_active(btn))
    SetMessFlag(messH, FLAG_RR);
  else
    ClearMessFlag(messH, FLAG_RR);
}

/* ── Dropdown changed callbacks ── */
static void on_comp_priority_changed(GObject *dd, GParamSpec *pspec, gpointer ud) {
  (void)pspec;
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(dd));
  /* 0=Highest(1) 1=High(2) 2=Normal(3) 3=Low(4) 4=Lowest(5) */
  SumOf(messH)->priority = (short)(sel + 1);
}
static void on_comp_encoding_changed(GObject *dd, GParamSpec *pspec, gpointer ud) {
  (void)pspec;
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(dd));
  /* 0=MIME 1=BinHex 2=Uuencode */
  SumOf(messH)->tableId = (short)sel;
}
static void on_comp_signature_changed(GObject *dd, GParamSpec *pspec, gpointer ud) {
  (void)pspec;
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(dd));
  /* 0=None 1=Standard 2=Alternate */
  SumOf(messH)->sigId = (short)sel;
}

/* ── Attachment management ── */

/* Remove a single attachment chip from the list */
static void on_attach_remove(GtkButton *btn, gpointer ud) {
  GtkWidget *chip = gtk_widget_get_parent(GTK_WIDGET(btn));
  if (!chip) return;
  GtkWidget *flow = gtk_widget_get_parent(chip);
  if (!flow) return;
  gtk_flow_box_remove(GTK_FLOW_BOX(flow), chip);
}

/* Add a file path as an attachment chip to the flow box */
static void comp_add_attachment(GtkWidget *attach_flow, const char *path) {
  if (!attach_flow || !path || !*path) return;

  /* Create a chip: [icon] filename [x] */
  GtkWidget *chip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_margin_start(chip, 2);
  gtk_widget_set_margin_end(chip, 2);
  gtk_widget_set_margin_top(chip, 1);
  gtk_widget_set_margin_bottom(chip, 1);

  GtkWidget *icon = gtk_image_new_from_icon_name("mail-attachment-symbolic");
  gtk_box_append(GTK_BOX(chip), icon);

  /* Show just the filename, store full path */
  const char *basename = strrchr(path, '/');
  basename = basename ? basename + 1 : path;
  GtkWidget *lbl = gtk_label_new(basename);
  gtk_widget_set_tooltip_text(lbl, path);
  gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_max_width_chars(GTK_LABEL(lbl), 30);
  gtk_box_append(GTK_BOX(chip), lbl);

  /* Store full path on the chip for later retrieval */
  g_object_set_data_full(G_OBJECT(chip), "attach-path", g_strdup(path), g_free);

  GtkWidget *rm_btn = gtk_button_new_from_icon_name("window-close-symbolic");
  gtk_widget_add_css_class(rm_btn, "flat");
  gtk_widget_add_css_class(rm_btn, "circular");
  gtk_widget_set_tooltip_text(rm_btn, "Remove attachment");
  g_signal_connect(rm_btn, "clicked", G_CALLBACK(on_attach_remove), NULL);
  gtk_box_append(GTK_BOX(chip), rm_btn);

  gtk_flow_box_insert(GTK_FLOW_BOX(attach_flow), chip, -1);
}

/* File dialog callback — add selected file as attachment */
static void on_attach_response(GObject *source, GAsyncResult *res, gpointer ud) {
  GtkFileDialog *dlg = GTK_FILE_DIALOG(source);
  GtkWidget *attach_flow = (GtkWidget *)ud;
  GError *err = NULL;
  GFile *file = gtk_file_dialog_open_finish(dlg, res, &err);
  if (file) {
    char *path = g_file_get_path(file);
    if (path) {
      comp_add_attachment(attach_flow, path);
      g_free(path);
    }
    g_object_unref(file);
  }
  if (err) g_error_free(err);
}

static void on_comp_attach_clicked(GtkButton *btn, gpointer ud) {
  (void)btn;
  GtkWidget *attach_flow = (GtkWidget *)ud;
  if (!attach_flow) return;
  GtkWidget *toplevel = GTK_WIDGET(gtk_widget_get_root(attach_flow));
  GtkFileDialog *dlg = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dlg, "Attach File");
  gtk_file_dialog_open(dlg, GTK_WINDOW(toplevel), NULL,
                       on_attach_response, attach_flow);
  g_object_unref(dlg);
}

/* Send button callback for old-style compose */
static void on_comp_send_clicked(GtkButton *btn, gpointer ud) {
  (void)btn;
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  CompSend(messH);
}

/* Save draft callback — saves and keeps window open */
static void on_comp_save_clicked(GtkButton *btn, gpointer ud) {
  (void)btn;
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  MyWindowPtr win = messH->win;
  if (CompSave(messH)) {
    /* Show saved status in title bar */
    if (win && win->window) {
      const char *title = gtk_window_get_title(GTK_WINDOW(win->window));
      if (title && !g_str_has_suffix(title, " [Saved]")) {
        char *newTitle = g_strdup_printf("%s [Saved]", title);
        theme_update_headerbar_title(win->window, newTitle);
        g_free(newTitle);
      }
    }
  }
}

/* Queue button callback — save, mark as queued, close window */
static void on_comp_queue_clicked(GtkButton *btn, gpointer ud) {
  (void)btn;
  MessHandle messH = (MessHandle)ud;
  if (!messH) return;
  if (CompSave(messH)) {
    TOCType *tocH = messH->tocH;
    int sumNum = messH->sumNum;
    tocH->sums[sumNum].state = QUEUED;
    TOCSetDirty(tocH, true);
    MyWindowPtr win = messH->win;
    if (win && win->window)
      gtk_window_close(GTK_WINDOW(win->window));
  }
}

/**********************************************************************
 * OpenComp - open an outgoing message - ported to use gEditCtrl
 *
 * Original Eudora layout (top to bottom):
 *   1. Compose toolbar: [Send Now] [Queue] [Save] ... Priority dropdown
 *   2. Formatting bar:  B I U  font-size  color  alignment  quote
 *   3. Header area (GtkGrid): To: [entry]  From: [entry]  Subject: [entry]
 *                              Cc: [entry]  Bcc: [entry]  Attachments: [entry]
 *   4. Separator
 *   5. gEditCtrl body editor (scrolled, fills rest of window)
 *
 * Headers are separate GtkEntry fields (not inline in the editor) so
 * they can be locked/unlocked individually, matching original behaviour.
 * The gEditCtrl holds only the message body.
 **********************************************************************/
MyWindowPtr OpenComp(TOCType * tocH, int sumNum, GtkWidget *winWP,
                     MyWindowPtr win, bool showIt, bool new) {
  char title[256];
  MessHandle messH;

  if ((messH = g_malloc0(sizeof(MessType))) == NULL)
    return (NULL);

  /* Create window and MyWindowPtr */
  if (!win) {
    win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
    if (!win) {
      g_free(messH);
      return NULL;
    }
    win->window = gtk_window_new();
    win->pte = NULL;
  }

  winWP = (GtkWidget *)GetMyWindowWindowPtr(win);

  tocH->sums[sumNum].messH = messH;
  MakeCompTitle(title, tocH, messH, sumNum);

  messH->win = win;
  messH->sumNum = sumNum;
  messH->tocH = tocH;

  SetMyWindowPrivateData(win, (void *)messH);
  win->close = CompClose;

  messH->next = MessList; MessList = messH;

  tocH->sums[sumNum].flags |= FLAG_ICON_BAR;
  if (PrefIsSet(PREF_COMP_TOOLBAR))
    SetMessOpt(messH, OPT_COMP_TOOLBAR_VISIBLE);

  bool isSent = (tocH->sums[sumNum].state == SENT ||
                 tocH->sums[sumNum].state == BUSY_SENDING);

  /* ── CSS for flat toolbar buttons ── */
  {
    static gboolean css_loaded = FALSE;
    if (!css_loaded) {
      GtkCssProvider *css = gtk_css_provider_new();
      gtk_css_provider_load_from_string(css,
        ".comp-toolbar button { min-height: 24px; min-width: 24px; padding: 2px 6px; }\n"
        ".comp-fmt-bar button { min-height: 22px; min-width: 22px; padding: 1px 4px; }\n"
        ".comp-header-grid label { font-size: 13px; color: alpha(currentColor, 0.7); }\n"
        ".comp-header-grid entry { min-height: 28px; }\n"
      );
      gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
      g_object_unref(css);
      css_loaded = TRUE;
    }
  }

  /* ── Build the window layout ── */
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* ── 1. Compose toolbar ── */
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_add_css_class(toolbar, "comp-toolbar");
  gtk_widget_set_margin_start(toolbar, 6);
  gtk_widget_set_margin_end(toolbar, 6);
  gtk_widget_set_margin_top(toolbar, 4);
  gtk_widget_set_margin_bottom(toolbar, 4);

  GtkWidget *send_btn = gtk_button_new_from_icon_name("mail-send-symbolic");
  gtk_widget_set_tooltip_text(send_btn, "Send Now");
  gtk_widget_set_sensitive(send_btn, !isSent);
  g_signal_connect(send_btn, "clicked", G_CALLBACK(on_comp_send_clicked), messH);
  gtk_box_append(GTK_BOX(toolbar), send_btn);
  messH->sendButton = send_btn;

  GtkWidget *queue_btn = gtk_button_new_from_icon_name("mail-send-receive-symbolic");
  gtk_widget_set_tooltip_text(queue_btn, "Queue for later");
  gtk_widget_set_sensitive(queue_btn, !isSent);
  g_signal_connect(queue_btn, "clicked", G_CALLBACK(on_comp_queue_clicked), messH);
  gtk_box_append(GTK_BOX(toolbar), queue_btn);

  GtkWidget *save_btn = gtk_button_new_from_icon_name("document-save-symbolic");
  gtk_widget_set_tooltip_text(save_btn, "Save draft");
  g_signal_connect(save_btn, "clicked", G_CALLBACK(on_comp_save_clicked), messH);
  gtk_box_append(GTK_BOX(toolbar), save_btn);

  /* Attach button — wired to attach_flow below after layout is built */
  GtkWidget *attach_btn = gtk_button_new_from_icon_name("mail-attachment-symbolic");
  gtk_widget_set_tooltip_text(attach_btn, "Attach file");
  gtk_box_append(GTK_BOX(toolbar), attach_btn);

  /* Insert image — signal connected below after body_ctrl is created */
  GtkWidget *img_b = gtk_button_new_from_icon_name("image-x-generic-symbolic");
  gtk_widget_set_tooltip_text(img_b, "Insert Image");
  gtk_box_append(GTK_BOX(toolbar), img_b);

  /* Insert link — signal connected below after body_ctrl is created */
  GtkWidget *link_b = gtk_button_new_from_icon_name("insert-link-symbolic");
  gtk_widget_set_tooltip_text(link_b, "Insert Link");
  gtk_box_append(GTK_BOX(toolbar), link_b);

  /* Insert emoji — popover attached below after body_ctrl is created */
  GtkWidget *emoji_btn = gtk_menu_button_new();
  gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(emoji_btn), "face-smile-symbolic");
  gtk_widget_set_tooltip_text(emoji_btn, "Insert Emoticon");
  gtk_widget_set_can_focus(emoji_btn, FALSE);
  gtk_box_append(GTK_BOX(toolbar), emoji_btn);

  gtk_box_append(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  /* ── Icon bar toggle buttons (matching original Eudora compose icon bar) ── */
  struct { const char *label; const char *tip; GCallback cb; long flag; } toggles[] = {
    {"QP",   "Allow Quoted-Printable encoding",   G_CALLBACK(on_comp_qp_toggled),      FLAG_CAN_ENC},
    {"Text", "Send attachments as plain data",     G_CALLBACK(on_comp_textonly_toggled), FLAG_BX_TEXT},
    {"Wrap", "Word-wrap message when sent",        G_CALLBACK(on_comp_wrap_toggled),     FLAG_WRAP_OUT},
    {"Keep", "Keep copy after sending",            G_CALLBACK(on_comp_keep_toggled),     FLAG_KEEP_COPY},
    {"RR",   "Request return receipt",             G_CALLBACK(on_comp_rr_toggled),       FLAG_RR},
    {NULL, NULL, NULL, 0}
  };
  for (int t = 0; toggles[t].label; t++) {
    GtkWidget *tb = gtk_toggle_button_new_with_label(toggles[t].label);
    gtk_widget_set_tooltip_text(tb, toggles[t].tip);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb),
      MessFlagIsSet(messH, toggles[t].flag) ? TRUE : FALSE);
    gtk_widget_set_can_focus(tb, FALSE);
    g_signal_connect(tb, "toggled", toggles[t].cb, messH);
    gtk_box_append(GTK_BOX(toolbar), tb);
  }

  gtk_box_append(GTK_BOX(toolbar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  /* Spacer pushes dropdowns to right */
  GtkWidget *spacer = gtk_label_new("");
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(toolbar), spacer);

  /* Encoding dropdown */
  const char *enc_types[] = {"MIME", "BinHex", "Uuencode", NULL};
  GtkWidget *enc_dd = gtk_drop_down_new_from_strings(enc_types);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(enc_dd), SumOf(messH)->tableId);
  gtk_widget_set_tooltip_text(enc_dd, "Attachment encoding");
  g_signal_connect(enc_dd, "notify::selected", G_CALLBACK(on_comp_encoding_changed), messH);
  gtk_box_append(GTK_BOX(toolbar), enc_dd);

  /* Priority dropdown */
  const char *priorities[] = {"Highest", "High", "Normal", "Low", "Lowest", NULL};
  GtkWidget *priority_dd = gtk_drop_down_new_from_strings(priorities);
  { short pri = SumOf(messH)->priority;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(priority_dd),
      (pri >= 1 && pri <= 5) ? (guint)(pri - 1) : 2); }
  gtk_widget_set_tooltip_text(priority_dd, "Priority");
  g_signal_connect(priority_dd, "notify::selected", G_CALLBACK(on_comp_priority_changed), messH);
  gtk_box_append(GTK_BOX(toolbar), priority_dd);

  /* Signature dropdown */
  const char *sigs[] = {"None", "Standard", "Alternate", NULL};
  GtkWidget *sig_dd = gtk_drop_down_new_from_strings(sigs);
  { short sig = SumOf(messH)->sigId;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(sig_dd),
      (sig >= 0 && sig <= 2) ? (guint)sig : 1); }
  gtk_widget_set_tooltip_text(sig_dd, "Signature");
  g_signal_connect(sig_dd, "notify::selected", G_CALLBACK(on_comp_signature_changed), messH);
  gtk_box_append(GTK_BOX(toolbar), sig_dd);

  gtk_box_append(GTK_BOX(vbox), toolbar);
  gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  /* ── 2. Formatting toolbar ──
   * Create the gEditCtrl early so formatting buttons can reference it. */
  GtkWidget *body_ctrl = geditctrl_new();
  theme_apply_to_editor(body_ctrl);

  GtkWidget *fmt_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_add_css_class(fmt_bar, "comp-fmt-bar");
  gtk_widget_set_margin_start(fmt_bar, 6);
  gtk_widget_set_margin_end(fmt_bar, 6);
  gtk_widget_set_margin_top(fmt_bar, 3);
  gtk_widget_set_margin_bottom(fmt_bar, 3);

  /* B / I / U as toggle buttons */
  GtkWidget *bold_tb = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(bold_tb), "format-text-bold-symbolic");
  gtk_widget_set_tooltip_text(bold_tb, "Bold (Ctrl+B)");
  gtk_widget_set_can_focus(bold_tb, FALSE);
  g_signal_connect(bold_tb, "clicked", G_CALLBACK(on_comp_bold), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), bold_tb);

  GtkWidget *italic_tb = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(italic_tb), "format-text-italic-symbolic");
  gtk_widget_set_tooltip_text(italic_tb, "Italic (Ctrl+I)");
  gtk_widget_set_can_focus(italic_tb, FALSE);
  g_signal_connect(italic_tb, "clicked", G_CALLBACK(on_comp_italic), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), italic_tb);

  GtkWidget *underline_tb = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(underline_tb), "format-text-underline-symbolic");
  gtk_widget_set_tooltip_text(underline_tb, "Underline (Ctrl+U)");
  gtk_widget_set_can_focus(underline_tb, FALSE);
  g_signal_connect(underline_tb, "clicked", G_CALLBACK(on_comp_underline), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), underline_tb);

  /* Clear formatting */
  GtkWidget *clear_b = gtk_button_new_from_icon_name("edit-clear-symbolic");
  gtk_widget_set_tooltip_text(clear_b, "Clear Formatting");
  gtk_widget_set_can_focus(clear_b, FALSE);
  g_signal_connect(clear_b, "clicked", G_CALLBACK(on_comp_clear_style), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), clear_b);

  gtk_box_append(GTK_BOX(fmt_bar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  /* Color picker */
  GtkColorDialog *color_dlg = gtk_color_dialog_new();
  GtkWidget *color_btn = gtk_color_dialog_button_new(color_dlg);
  gtk_widget_set_can_focus(color_btn, FALSE);
  gtk_widget_set_tooltip_text(color_btn, "Text Color");
  g_signal_connect(color_btn, "notify::rgba", G_CALLBACK(on_comp_color_changed), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), color_btn);

  /* Font family dropdown */
  const char *font_families[] = {"Sans", "Serif", "Monospace",
                                 "Helvetica", "Times", "Courier", NULL};
  GtkWidget *font_dd = gtk_drop_down_new_from_strings(font_families);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(font_dd), 0); /* Sans default */
  gtk_widget_set_tooltip_text(font_dd, "Font");
  gtk_widget_set_can_focus(font_dd, FALSE);
  g_signal_connect(font_dd, "notify::selected", G_CALLBACK(on_comp_font_family_changed), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), font_dd);

  /* Font size dropdown */
  const char *sizes_str[] = {"8", "10", "12", "14", "16", "18", "20", "24", "28", "36", NULL};
  GtkWidget *size_dd = gtk_drop_down_new_from_strings(sizes_str);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(size_dd), 2); /* 12pt default */
  gtk_widget_set_tooltip_text(size_dd, "Font Size");
  gtk_widget_set_can_focus(size_dd, FALSE);
  g_signal_connect(size_dd, "notify::selected", G_CALLBACK(on_comp_font_size_changed), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), size_dd);

  gtk_box_append(GTK_BOX(fmt_bar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  /* Alignment */
  struct { const char *icon; const char *tip; GCallback cb; } align[] = {
    {"format-justify-left-symbolic",   "Align Left",   G_CALLBACK(on_comp_align_left)},
    {"format-justify-center-symbolic", "Center",        G_CALLBACK(on_comp_align_center)},
    {"format-justify-right-symbolic",  "Align Right",   G_CALLBACK(on_comp_align_right)},
    {NULL, NULL, NULL}
  };
  for (int i = 0; align[i].icon; i++) {
    GtkWidget *b = gtk_button_new_from_icon_name(align[i].icon);
    gtk_widget_set_tooltip_text(b, align[i].tip);
    gtk_widget_set_can_focus(b, FALSE);
    g_signal_connect(b, "clicked", align[i].cb, body_ctrl);
    gtk_box_append(GTK_BOX(fmt_bar), b);
  }

  gtk_box_append(GTK_BOX(fmt_bar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  /* Indent / Outdent / Bullets */
  GtkWidget *indent_b = gtk_button_new_from_icon_name("format-indent-more-symbolic");
  gtk_widget_set_tooltip_text(indent_b, "Indent");
  gtk_widget_set_can_focus(indent_b, FALSE);
  g_signal_connect(indent_b, "clicked", G_CALLBACK(on_comp_indent), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), indent_b);

  GtkWidget *outdent_b = gtk_button_new_from_icon_name("format-indent-less-symbolic");
  gtk_widget_set_tooltip_text(outdent_b, "Outdent");
  gtk_widget_set_can_focus(outdent_b, FALSE);
  g_signal_connect(outdent_b, "clicked", G_CALLBACK(on_comp_outdent), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), outdent_b);

  GtkWidget *bullet_b = gtk_button_new_from_icon_name("view-list-symbolic");
  gtk_widget_set_tooltip_text(bullet_b, "Bullet List");
  gtk_widget_set_can_focus(bullet_b, FALSE);
  g_signal_connect(bullet_b, "clicked", G_CALLBACK(on_comp_bullet), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), bullet_b);

  gtk_box_append(GTK_BOX(fmt_bar), gtk_separator_new(GTK_ORIENTATION_VERTICAL));

  /* Quote level increase / decrease */
  GtkWidget *qi = gtk_button_new_from_icon_name("format-text-direction-ltr-symbolic");
  gtk_widget_set_tooltip_text(qi, "Increase Quote Level");
  gtk_widget_set_can_focus(qi, FALSE);
  g_signal_connect(qi, "clicked", G_CALLBACK(on_comp_quote_inc), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), qi);

  GtkWidget *qd = gtk_button_new_from_icon_name("format-text-direction-rtl-symbolic");
  gtk_widget_set_tooltip_text(qd, "Decrease Quote Level");
  gtk_widget_set_can_focus(qd, FALSE);
  g_signal_connect(qd, "clicked", G_CALLBACK(on_comp_quote_dec), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), qd);

  /* Horizontal rule */
  GtkWidget *hr_b = gtk_button_new_from_icon_name("object-select-symbolic");
  gtk_widget_set_tooltip_text(hr_b, "Insert Horizontal Rule");
  gtk_widget_set_can_focus(hr_b, FALSE);
  g_signal_connect(hr_b, "clicked", G_CALLBACK(on_comp_insert_hr), body_ctrl);
  gtk_box_append(GTK_BOX(fmt_bar), hr_b);

  /* Store toolbar widget refs on body_ctrl for style tracking */
  g_object_set_data(G_OBJECT(body_ctrl), "fmt-bold", bold_tb);
  g_object_set_data(G_OBJECT(body_ctrl), "fmt-italic", italic_tb);
  g_object_set_data(G_OBJECT(body_ctrl), "fmt-underline", underline_tb);
  g_object_set_data(G_OBJECT(body_ctrl), "fmt-color", color_btn);
  g_object_set_data(G_OBJECT(body_ctrl), "fmt-font", font_dd);
  g_object_set_data(G_OBJECT(body_ctrl), "fmt-size", size_dd);

  /* Connect to document's selection-changed to update toolbar */
  geditDocument *fmt_doc = geditctrl_get_document(body_ctrl);
  if (fmt_doc)
    g_signal_connect(fmt_doc, "selection-changed",
                     G_CALLBACK(on_comp_selection_changed), body_ctrl);

  gtk_box_append(GTK_BOX(vbox), fmt_bar);
  gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  /* ── 3. Header area + Body in a GtkPaned splitter ── */
  char *buffer = NULL;
  long bufferSize;
  unsigned long uidHash;

  if (new) {
    bufferSize = 1024;
    buffer = g_malloc(bufferSize);
    if (buffer) {
      short len = CreateMessageBody(buffer, &uidHash);
      buffer = g_realloc(buffer, len + 1);
      SumOf(messH)->uidHash = SumOf(messH)->msgIdHash = uidHash;
    }
  } else {
    bufferSize = GetMessageLength(tocH, sumNum) + 1;
    buffer = g_malloc0(bufferSize);  /* zero-fill so it's null-terminated */
    if (buffer)
      ReadMessage(tocH, sumNum, (unsigned char *)buffer);
  }

  /* Parse headers from the buffer */
  static const char *header_labels[] = {
    "To:", "From:", "Subject:", "Cc:", "Bcc:", "Attachments:", NULL
  };
  char *header_values[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
  char *body_start = NULL;

  if (buffer) {
    char *cp = buffer;
    char *stop = cp + strlen(buffer);
    /* Skip sendmail "From " envelope line */
    while (cp < stop && *cp != '\r' && *cp != '\n') cp++;
    while (cp < stop && (*cp == '\r' || *cp == '\n')) cp++;
    /* Parse header lines until blank line */
    while (cp < stop) {
      /* Blank line = end of headers */
      if (*cp == '\r' || *cp == '\n') {
        /* Blank line = end of headers; skip one CRLF */
        if (cp < stop && *cp == '\r') cp++;
        if (cp < stop && *cp == '\n') cp++;
        break;
      }
      char *eol = cp;
      while (eol < stop && *eol != '\r' && *eol != '\n') eol++;
      char *colon = memchr(cp, ':', eol - cp);
      if (colon) {
        size_t name_len = colon - cp + 1;
        for (int h = 0; header_labels[h]; h++) {
          if (strlen(header_labels[h]) == name_len &&
              g_ascii_strncasecmp(cp, header_labels[h], name_len) == 0) {
            char *val = colon + 1;
            while (val < eol && *val == ' ') val++;
            header_values[h] = g_strndup(val, eol - val);
            break;
          }
        }
      }
      cp = eol;
      /* Skip exactly one line ending (CRLF, CR, or LF) */
      if (cp < stop && *cp == '\r') cp++;
      if (cp < stop && *cp == '\n') cp++;
    }
    body_start = cp;
  }

  /* Header grid */
  GtkWidget *header_grid = gtk_grid_new();
  gtk_widget_add_css_class(header_grid, "comp-header-grid");
  gtk_grid_set_row_spacing(GTK_GRID(header_grid), 4);
  gtk_grid_set_column_spacing(GTK_GRID(header_grid), 8);
  gtk_widget_set_margin_start(header_grid, 8);
  gtk_widget_set_margin_end(header_grid, 8);
  gtk_widget_set_margin_top(header_grid, 6);
  gtk_widget_set_margin_bottom(header_grid, 6);

  /* Text entry headers: To, From, Subject, Cc, Bcc (indices 0-4) */
  GtkWidget *header_entries[5];
  for (int h = 0; h < 5; h++) {
    GtkWidget *lbl = gtk_label_new(header_labels[h]);
    gtk_widget_set_halign(lbl, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(header_grid), lbl, 0, h, 1, 1);

    if (h == 1) {
      /* From: field is a dropdown populated from accounts/personalities */
      PrefsAccount accounts[16];
      int num_accounts = prefs_load_accounts(accounts, 16);

      /* Build "Real Name <email>" strings for each account */
      GtkStringList *from_model = gtk_string_list_new(NULL);
      int active_idx = 0;
      gchar *default_email = prefs_get_string(PREFS_GROUP_SENDING_MAIL,
                                               "email_address", "");
      gchar *default_name = prefs_get_string(PREFS_GROUP_SENDING_MAIL,
                                              "real_name", "");

      /* Dominant personality always from global prefs */
      if (default_email && default_email[0]) {
        gchar *from_str;
        if (default_name && default_name[0])
          from_str = g_strdup_printf("%s <%s>", default_name, default_email);
        else
          from_str = g_strdup(default_email);
        gtk_string_list_append(from_model, from_str);
        g_free(from_str);
      }

      /* Additional personalities (all loaded accounts are non-dominant) */
      for (int a = 0; a < num_accounts; a++) {
        if (!accounts[a].enabled || !accounts[a].email[0]) continue;
        const char *name = accounts[a].real_name[0] ? accounts[a].real_name : accounts[a].name;
        gchar *from_str;
        if (name && name[0])
          from_str = g_strdup_printf("%s <%s>", name, accounts[a].email);
        else
          from_str = g_strdup(accounts[a].email);
        gtk_string_list_append(from_model, from_str);
        g_free(from_str);
      }

      /* If no accounts at all, add a blank entry */
      if (g_list_model_get_n_items(G_LIST_MODEL(from_model)) == 0)
        gtk_string_list_append(from_model, "");

      /* If we have a value from the message, find it in the list */
      if (header_values[h] && header_values[h][0]) {
        guint n = g_list_model_get_n_items(G_LIST_MODEL(from_model));
        for (guint i = 0; i < n; i++) {
          const char *s = gtk_string_list_get_string(from_model, i);
          if (s && strstr(s, header_values[h])) { active_idx = (int)i; break; }
        }
      }

      GtkWidget *from_dd = gtk_drop_down_new(G_LIST_MODEL(from_model), NULL);
      gtk_drop_down_set_selected(GTK_DROP_DOWN(from_dd), active_idx);
      gtk_widget_set_hexpand(from_dd, TRUE);
      if (isSent)
        gtk_widget_set_sensitive(from_dd, FALSE);
      gtk_grid_attach(GTK_GRID(header_grid), from_dd, 1, h, 1, 1);
      header_entries[h] = from_dd;

      g_free(default_email);
      g_free(default_name);
      if (header_values[h]) g_free(header_values[h]);
    } else {
      GtkWidget *entry = gtk_entry_new();
      gtk_widget_set_hexpand(entry, TRUE);
      if (header_values[h]) {
        gtk_editable_set_text(GTK_EDITABLE(entry), header_values[h]);
        g_free(header_values[h]);
      }
      if (isSent)
        gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);

      gtk_grid_attach(GTK_GRID(header_grid), entry, 1, h, 1, 1);
      header_entries[h] = entry;
    }
  }

  /* Store header widgets in messH for CompHead* API access */
  memset(messH->headerWidgets, 0, sizeof(messH->headerWidgets));
  messH->headerWidgets[TO_HEAD] = header_entries[0];   /* To: */
  messH->headerWidgets[FROM_HEAD] = header_entries[1];  /* From: (GtkDropDown) */
  messH->headerWidgets[SUBJ_HEAD] = header_entries[2];  /* Subject: */
  messH->headerWidgets[CC_HEAD] = header_entries[3];    /* Cc: */
  messH->headerWidgets[BCC_HEAD] = header_entries[4];   /* Bcc: */
  messH->headerGrid = header_grid;

  /* Attachments row: label + FlowBox with add button */
  GtkWidget *attach_lbl = gtk_label_new("Attachments:");
  gtk_widget_set_halign(attach_lbl, GTK_ALIGN_END);
  gtk_widget_set_valign(attach_lbl, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(header_grid), attach_lbl, 0, 5, 1, 1);

  GtkWidget *attach_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_hexpand(attach_box, TRUE);

  GtkWidget *attach_flow = gtk_flow_box_new();
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(attach_flow), GTK_SELECTION_NONE);
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(attach_flow), 20);
  gtk_widget_set_hexpand(attach_flow, TRUE);
  gtk_box_append(GTK_BOX(attach_box), attach_flow);

  /* Small "+" button to add more attachments inline */
  GtkWidget *add_attach_btn = gtk_button_new_from_icon_name("list-add-symbolic");
  gtk_widget_add_css_class(add_attach_btn, "flat");
  gtk_widget_set_tooltip_text(add_attach_btn, "Add attachment");
  gtk_widget_set_valign(add_attach_btn, GTK_ALIGN_START);
  g_signal_connect(add_attach_btn, "clicked",
                   G_CALLBACK(on_comp_attach_clicked), attach_flow);
  gtk_box_append(GTK_BOX(attach_box), add_attach_btn);

  gtk_grid_attach(GTK_GRID(header_grid), attach_box, 1, 5, 1, 1);

  /* Populate existing attachments from header */
  if (header_values[5]) {
    char **parts = g_strsplit(header_values[5], ",", -1);
    for (int i = 0; parts && parts[i]; i++) {
      char *trimmed = g_strstrip(g_strdup(parts[i]));
      if (*trimmed)
        comp_add_attachment(attach_flow, trimmed);
      g_free(trimmed);
    }
    g_strfreev(parts);
    g_free(header_values[5]);
  }

  /* Wire the toolbar attach button to the same flow */
  g_signal_connect(attach_btn, "clicked",
                   G_CALLBACK(on_comp_attach_clicked), attach_flow);

  /* Wire image, link, and emoji buttons now that body_ctrl exists */
  g_signal_connect(img_b, "clicked", G_CALLBACK(on_comp_insert_image), body_ctrl);
  g_signal_connect(link_b, "clicked", G_CALLBACK(on_comp_insert_link), body_ctrl);

  /* Attach emoji popover to the menu button */
  GtkWidget *emo_popover = create_emoji_popover(body_ctrl);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(emoji_btn), emo_popover);

  g_object_set_data(G_OBJECT(winWP), "comp-to", header_entries[0]);
  g_object_set_data(G_OBJECT(winWP), "comp-from", header_entries[1]);
  g_object_set_data(G_OBJECT(winWP), "comp-subject", header_entries[2]);
  g_object_set_data(G_OBJECT(winWP), "comp-cc", header_entries[3]);
  g_object_set_data(G_OBJECT(winWP), "comp-bcc", header_entries[4]);
  g_object_set_data(G_OBJECT(winWP), "comp-attach-flow", attach_flow);

  /* Vertical paned: headers on top, body below */
  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_start_child(GTK_PANED(paned), header_grid);
  gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
  gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
  gtk_widget_set_vexpand(paned, TRUE);
  gtk_widget_set_hexpand(paned, TRUE);

  /* ── 4. gEditCtrl body editor (created above for formatting toolbar) ── */
  win->pte = body_ctrl;
  if (win->pte) {
    geditDocument *doc = geditctrl_get_document(win->pte);

    /* Insert body text if available */
    g_print("OpenComp: bufferSize=%ld body_start=%p buffer=%p\n",
            bufferSize, (void*)body_start, (void*)buffer);
    if (body_start)
      g_print("OpenComp: body_start offset=%ld body='%.80s'\n",
              (long)(body_start - buffer), body_start);
    if (body_start && *body_start) {
      /* Skip any remaining line endings after blank line */
      while (*body_start == '\r' || *body_start == '\n') body_start++;
      if (*body_start) {
        char *stop = buffer + strlen(buffer);
        char *bodyText = g_strndup(body_start, stop - body_start);
        gedit_document_insert_text(doc, 0, bodyText);
        g_free(bodyText);
      }
    }

    gtk_widget_set_vexpand(win->pte, TRUE);
    gtk_widget_set_hexpand(win->pte, TRUE);

    /* Put body into the paned bottom half */
    gtk_paned_set_end_child(GTK_PANED(paned), win->pte);
    gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);

    /* Set initial split position (headers get ~200px) */
    gtk_paned_set_position(GTK_PANED(paned), 200);

    if (isSent)
      geditctrl_set_editable(win->pte, false);
  }

  if (buffer) g_free(buffer);

  /* Add the paned (headers + body) to main vbox */
  gtk_box_append(GTK_BOX(vbox), paned);

  /* Set the vbox as the window child */
  gtk_window_set_child(GTK_WINDOW(winWP), vbox);

  win->dontControl = true;
  if (IsColorWin(winWP))
    win->label = GetSumColor(messH->tocH, messH->sumNum);

  theme_setup_headerbar(winWP, title);
  gtk_window_set_default_size(GTK_WINDOW(winWP), 640, 520);
  AttachSelect(messH);

  /* Connect close-request for WannaSave dialog (like original CompClose) */
  g_signal_connect(winWP, "close-request",
                   G_CALLBACK(on_comp_close_request), win);

  /* Track dirty state when body text changes */
  if (win->pte) {
    geditDocument *body_doc = geditctrl_get_document(win->pte);
    if (body_doc)
      g_signal_connect(body_doc, "document-changed",
                       G_CALLBACK(on_comp_body_changed), win);
  }
  win->isDirty = false; /* start clean */

  if (showIt)
    ShowMyWindow(winWP);

  /* Initialize toolbar to reflect style at caret position */
  if (win->pte && fmt_doc)
    on_comp_selection_changed(fmt_doc, win->pte);

  /* Grab focus on the body editor so key events work immediately */
  if (win->pte) {
    GtkWidget *area = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(win->pte));
    if (area)
      gtk_widget_grab_focus(area);
  }

  return (win);
}

/**********************************************************************
 * DoComposeNew - start a new outgoing message
 * Ported from functions.c - gets Out TOC, creates blank summary, opens OpenComp
 **********************************************************************/
MyWindowPtr DoComposeNew(int type) {
  (void)type;
  TOCType *tocH;
  MSumType sum;
  MyWindowPtr newWin;
  bool oldReallyDirty;

  /* Always use the real Out TOC (threading is always on) */
  tocH = GetRealOutTOC();
  g_print("DoComposeNew: GetRealOutTOC=%p\n", (void*)tocH);
  if (!tocH) return NULL;

  memset(&sum, 0, sizeof(sum));
  sum.state = UNSENDABLE;
  sum.flags = 0;
  sum.tableId = 0;
  sum.origZone = ZoneSecs() / 60;
  sum.seconds = GMTDateTime();
  sum.persId = CurPers->persId;
  sum.sigId = 0;

  oldReallyDirty = tocH->reallyDirty;
  if (!SaveMessageSum(&sum, &tocH)) {
    g_print("DoComposeNew: SaveMessageSum failed\n");
    return NULL;
  }
  g_print("DoComposeNew: SaveMessageSum OK, count=%d\n", tocH->count);

  newWin = OpenComp(tocH, tocH->count - 1, NULL, NULL, true, true);
  g_print("DoComposeNew: OpenComp returned %p\n", (void*)newWin);
  return newWin;
}

/**********************************************************************
 * CompCurAddr - return the address most closely associated with this message
 * Ported to use standard C strings instead of Pascal strings
 **********************************************************************/
char *CompCurAddr(MyWindowPtr win, char *addr) {
  char *addrList =
      g_malloc(1024); // Replace BinAddrHandle with standard allocation
  *addr = 0;

  if (win->hasSelection)
    return CurAddrSel(win, addr);

  if (!GatherCompAddresses(win, addrList)) {
    g_strlcpy((char *)addr, addrList, 256); // Replace PCopy with g_strlcpy
    ShortAddr(addr, addr);
  }

  g_free(addrList);
  return *addr ? addr : NULL;
}

/**********************************************************************
 * BodyOffset - return the offset to the first character of the body
 * of a message - ported to use char* instead of void ***********************************************************************/
long BodyOffset(char *text) {
  char *spot;
  long size = strlen(text);
  char *end = text + size;

  for (spot = text + 2; spot < end; spot++)
    if (spot[-1] != '\015')
      spot++;
    else if (spot[-2] == '\015')
      break;

  return (spot - text);
}

/************************************************************************
 * NewMessageId - create a new message id - ported to use standard C
 ************************************************************************/
char *NewMessageId(char *id) {
  char hostname[128];
  char scratch[256];
  static short seq;
  char *vers;
  struct {
    unsigned char four[4];
    long seconds;
    short ticks;
  } rawStuff;

  // Get version info (simplified)
  vers = "Eudora-GTK";

  // Get hostname
  GetMyHostname(hostname);

  // Create unique ID components
  rawStuff.seconds = time(NULL);
  rawStuff.ticks = rand() % 60;
  seq++;

  // Format message ID
  snprintf(id, 256, "<%08lX.%04X.%s@%s>", rawStuff.seconds, seq, vers,
           hostname);

  return id;
}

/**********************************************************************
 * GetCompTexts - get the fields of an under-composition message
 * PORTED FROM PETE TO gEditCtrl
 * First, we read ALL the message into a buffer. Then, we grab the
 * header items, stuff them one by one into appropriate gEditCtrl widgets.
 * After that, we stuff the body into the main gEditCtrl widget.
 *
 * the "new" item means not to read the text, but to create it instead
 **********************************************************************/
int GetCompTexts(MessHandle messH, bool new) {
  MyWindowPtr messWin = messH->win;
  GtkWidget *messWinWP = (GtkWidget *)GetMyWindowWindowPtr(messWin);
  int sumNum = messH->sumNum;
  TOCType * tocH = messH->tocH;
  char *buffer = NULL;
  Accumulator extras;
  int which;
  int err = 0;
  char *cp, *ep;
  char *stop;
  unsigned long uidHash;
  short len;
  geditDocument *doc; // Replace PETEDocInitInfo with geditDocument
  char headerName[64];
  long width;
  bool locked;
  short baseLock;
  long bo;
  void *grumble;
  bool xDash;
  long baseWidth = 100; // Simplified base width calculation

  memset(&extras, 0, sizeof(extras));

  /*
   * allocate space for the text
   */
  long bufferSize = new ? 1024 : GetMessageLength(tocH, sumNum) + 1;
  if ((buffer = g_malloc(bufferSize)) == NULL) {
    return (WarnUser(NO_MESS_BUF, errno));
  }

  /*
   * read or create
   */
  if (!new) {
    /*
     * read it
     */
    if ((err = ReadMessage(tocH, sumNum, (unsigned char *)buffer))) {
      g_free(buffer);
      return (err);
    }
  } else {
    len = CreateMessageBody(buffer, &uidHash);
    buffer = g_realloc(buffer, len + 1);
    SumOf(messH)->uidHash = SumOf(messH)->msgIdHash = uidHash;
  }

  /*
   * now, set up the gEditCtrl widget instead of Pete TERec's
   */
  messWin->pte = geditctrl_new();
  if (!messWin->pte) {
    err = -1;
    goto failure;
  }
  theme_apply_to_editor(messWin->pte);

  // Get the document from gEditCtrl
  doc = geditctrl_get_document(messWin->pte);

  // Set drag callback for gEditCtrl (simplified)
  // geditctrl_set_drag_callback(messWin->pte, CompGetDragContents);

  /*
   * put in the text...
   */
  cp = buffer;
  stop = cp + strlen(buffer);
  while (*cp++ != '\015' && cp < stop)
    ; // skip sendmail from line

  /*
   * the headers - ported to gEditCtrl
   */
  // Remove CycleBalls() - Mac-specific
  baseLock =
      (SumOf(messH)->state == SENT || SumOf(messH)->state == BUSY_SENDING) ? 1
                                                                           : 0;

  for (which = TO_HEAD; which < BODY_HEAD; which++) {
    locked = baseLock || which == ATTACH_HEAD ||
             (which == FROM_HEAD && !PrefIsSet(PREF_EDIT_FROM));

    GetRString(headerName, HeaderStrn + which);
    xDash =
        strlen(headerName) > 2 && headerName[0] == 'X' && headerName[1] == '-';

    // Find header content
    ep = cp;
    while (ep < stop && *ep != '\015')
      ep++;

    if (ep > cp) {
      // Insert header name
      char headerLine[512];
      snprintf(headerLine, sizeof(headerLine), "%s ", headerName);
      gedit_document_insert_text(doc, gedit_document_get_length(doc),
                                 headerLine);

      // Insert header content
      char *headerContent = g_strndup(cp, ep - cp);
      gedit_document_insert_text(doc, gedit_document_get_length(doc),
                                 headerContent);
      g_free(headerContent);

      // Add newline
      gedit_document_insert_text(doc, gedit_document_get_length(doc), "\n");
    }

    cp = ep + 1;
  }

  // Store header count in gEditCtrl metadata (simplified)
  // geditctrl_set_header_count(messWin->pte, which - 1);

  // Insert body separator
  gedit_document_insert_text(doc, gedit_document_get_length(doc), "\n");

  // Get body offset
  bo = gedit_document_get_length(doc);

  if (ep < stop) {
    // Insert body text
    if (MessFlagIsSet(messH, FLAG_RICH)) {
      // Insert rich text (simplified - would need full rich text parsing)
      char *bodyText = g_strndup(ep, stop - ep);
      gedit_document_insert_markup(doc, bo, bodyText);
      g_free(bodyText);
    } else {
      // Insert plain text
      char *bodyText = g_strndup(ep, stop - ep);
      gedit_document_insert_text(doc, bo, bodyText);
      g_free(bodyText);

      if (MessFlagIsSet(messH, FLAG_RICH)) {
        // Mark as rich text in gEditCtrl
        geditctrl_set_rich_text(messWin->pte, bo, true);
      }
    }

    // void *inline signature
    if (MessOptIsSet(messH, OPT_INLINE_SIG)) {
      FindAndMarkSigSep(messWin->pte);
    }
  } else {
    // Insert empty body
    gedit_document_insert_text(doc, bo, "");
  }

  if (buffer)
    g_free(buffer);

  // void *signature insertion (simplified)
  if (!new && !MessOptIsSet(messH, OPT_INLINE_SIG)) {
    // Add signature logic here
    if (MessOptIsSet(messH, OPT_INLINE_SIG)) {
      FindAndMarkSigSep(messWin->pte);
    }
  }

  // Lock text if message is sent
  if (baseLock) {
    // Lock the entire document in gEditCtrl
    geditctrl_set_editable(messWin->pte, false);
  }

  // Set change callback for gEditCtrl
  // geditctrl_set_change_callback(messWin->pte, PeteChanged);

  return (0);

failure:
  if (buffer)
    g_free(buffer);
  return (err);
}

/**********************************************************************
 * MakeCompTitle - make a reasonable composition title
 * Ported to use standard C strings instead of Pascal strings
 **********************************************************************/
void MakeCompTitle(char *string, TOCType * tocH, MessHandle messH, int sumNum) {
  char subject[256] = "";
  char to[256] = "";
  char pattern[64];

  // Get subject from message
  if (messH && messH->win && messH->win->pte) {
    // Extract subject from headers (simplified)
    g_strlcpy(subject, tocH->sums[sumNum].subj, sizeof(subject));
  }

  // Get To address (simplified)
  // This would need proper header parsing in a full implementation

  // Format title
  if (strlen(subject) > 0) {
    snprintf(string, 256, "Compose: %s", subject);
  } else {
    g_strlcpy(string, "Compose Message", 256);
  }
}

/**********************************************************************
 * FindAndMarkSigSep - find and mark signature separator in gEditCtrl
 * Ported from Pete to gEditCtrl
 **********************************************************************/
int FindAndMarkSigSep(GtkWidget *pte) {
  long sigPos = FindSigSep(pte);
  if (sigPos >= 0) {
    // Mark signature separator in gEditCtrl
    geditDocument *doc = geditctrl_get_document(pte);
    // This would need gEditCtrl-specific signature marking
    return 0;
  }
  return -1;
}

/**********************************************************************
 * FindSigSep - find signature separator in gEditCtrl text
 * Ported from Pete to gEditCtrl
 **********************************************************************/
long FindSigSep(GtkWidget *pte) {
  geditDocument *doc = geditctrl_get_document(pte);
  char *text = gedit_document_get_text(doc);
  char *sigSep = strstr(text, "\n-- \n");

  if (sigSep) {
    long pos = sigSep - text;
    g_free(text);
    return pos;
  }

  g_free(text);
  return -1;
}

/**********************************************************************
 * CompGetDragContents - handle drag contents for gEditCtrl
 * Ported from Pete drag handling to gEditCtrl
 **********************************************************************/
int CompGetDragContents(GtkWidget *pte, char **theText, void **theStyles,
                        void **theParas, void *drag, long dropLocation) {
  // Simplified drag handling for gEditCtrl
  // This would need full implementation based on gEditCtrl's drag API
  *theText = NULL;
  *theStyles = NULL;
  *theParas = NULL;
  return 0;
}

/**********************************************************************
 * WriteComp - write composition to file
 * Ported to work with gEditCtrl instead of Pete
 **********************************************************************/
int WriteComp(MessHandle messH, short refN, long offset) {
  MyWindowPtr win = messH->win;
  int err = 0;

  if (!win || !win->pte || !win->window)
    return -1;

  geditDocument *doc = geditctrl_get_document(win->pte);
  char *body = gedit_document_get_text(doc);

  /* Gather headers from the widgets */
  GtkWidget *winWP = win->window;
  const char *to = "", *from = "", *subject = "", *cc = "", *bcc = "";
  GtkWidget *e;
  if ((e = g_object_get_data(G_OBJECT(winWP), "comp-to")))
    to = gtk_editable_get_text(GTK_EDITABLE(e));
  if ((e = g_object_get_data(G_OBJECT(winWP), "comp-from"))) {
    /* From is a GtkDropDown — get selected string */
    GtkStringObject *sel = GTK_STRING_OBJECT(
        gtk_drop_down_get_selected_item(GTK_DROP_DOWN(e)));
    if (sel)
      from = gtk_string_object_get_string(sel);
  }
  if ((e = g_object_get_data(G_OBJECT(winWP), "comp-subject")))
    subject = gtk_editable_get_text(GTK_EDITABLE(e));
  if ((e = g_object_get_data(G_OBJECT(winWP), "comp-cc")))
    cc = gtk_editable_get_text(GTK_EDITABLE(e));
  if ((e = g_object_get_data(G_OBJECT(winWP), "comp-bcc")))
    bcc = gtk_editable_get_text(GTK_EDITABLE(e));

  /* Build complete message: envelope + headers + blank line + body */
  time_t now = time(NULL);
  char dateStr[64];
  strftime(dateStr, sizeof(dateStr), "%a %b %d %H:%M:%S %Y", localtime(&now));

  char *msg = g_strdup_printf(
      "From %s %s\r\n"
      "To: %s\r\n"
      "From: %s\r\n"
      "Subject: %s\r\n"
      "Cc: %s\r\n"
      "Bcc: %s\r\n"
      "\r\n"
      "%s\r\n",
      from[0] ? from : "user", dateStr,
      to, from, subject, cc, bcc,
      body ? body : "");

  long count = strlen(msg);

  /* Seek to the write position */
  if (lseek(refN, offset, SEEK_SET) < 0) {
    g_free(msg);
    g_free(body);
    return -1;
  }

  err = file_write(refN, &count, (unsigned char *)msg);

  /* Update the summary with the actual message length */
  if (!err) {
    TOCType *tocH = messH->tocH;
    int sumNum = messH->sumNum;
    tocH->sums[sumNum].offset = offset;
    tocH->sums[sumNum].length = count;

    /* Update summary fields for display in message list */
    if (subject && subject[0])
      g_strlcpy(tocH->sums[sumNum].subj, subject,
                 sizeof(tocH->sums[sumNum].subj));
    if (to && to[0])
      g_strlcpy(tocH->sums[sumNum].from, to,
                 sizeof(tocH->sums[sumNum].from));
    tocH->sums[sumNum].seconds = (unsigned long)time(NULL);
    InvalSum(tocH, sumNum);
  }

  g_free(msg);
  g_free(body);

  return err;
}

/**********************************************************************
 * GetMyHostname - get hostname for message ID
 * Ported to use standard C networking
 **********************************************************************/
char *GetMyHostname(char *hostname) {
  if (gethostname(hostname, 127) != 0) {
    g_strlcpy(hostname, "localhost", 128);
  }
  return hostname;
}

/**********************************************************************
 * CompStripHeaderReturns - strip returns from headers
 * Ported to work with gEditCtrl
 **********************************************************************/
int CompStripHeaderReturns(MessHandle messH) {
  MyWindowPtr win = messH->win;
  geditDocument *doc;
  char *text;

  if (!win || !win->pte)
    return -1;

  doc = geditctrl_get_document(win->pte);
  text = gedit_document_get_text(doc);

  if (text) {
    // Strip returns from header section (simplified)
    // This would need proper header parsing
    g_free(text);
  }

  return 0;
}
/**********************************************************************
 * SuckDragAddresses - extract addresses from drag operation
 * Ported to work with GTK drag and drop
 **********************************************************************/
int SuckDragAddresses(void *drag, char **addresses, bool leadingComma,
                      bool trailingComma) {
  // Simplified drag address extraction for GTK
  // This would need full implementation based on GTK's drag and drop API
  *addresses = g_strdup("");
  return 0;
}

/**********************************************************************
 * SepStyle - set separator style
 * Ported from Pete styling to gEditCtrl styling
 **********************************************************************/
void SepStyle(void *pip, void *tsp, void **graphic, int pgt) {
  // Simplified styling for gEditCtrl
  // This would need gEditCtrl-specific style implementation
}

/**********************************************************************
 * CompBeautifyFrom - beautify the From address
 * Ported to use standard C strings
 **********************************************************************/
void CompBeautifyFrom(char *name) {
  // Beautify from address (simplified)
  // Remove quotes, clean up formatting, etc.
  if (name && strlen(name) > 0) {
    // Basic cleanup - remove surrounding quotes
    if (name[0] == '"' && name[strlen(name) - 1] == '"') {
      memmove(name, name + 1, strlen(name) - 1);
      name[strlen(name) - 2] = '\0';
    }
  }
}

/**********************************************************************
 * Additional utility functions needed for composition
 **********************************************************************/

/**********************************************************************
 * CompDidResize - handle window resize for composition
 * Ported to work with GTK window resizing
 **********************************************************************/
void CompDidResize(MyWindowPtr win) {
  if (win && win->pte) {
    // Resize gEditCtrl widget to fit window
    // This would need proper GTK widget resizing
  }
}

/**********************************************************************
 * CompClick - handle mouse clicks in composition window
 * Ported from Mac mouse handling to GTK
 **********************************************************************/
bool CompClick(MyWindowPtr win, GdkEvent *event) {
  if (win && win->pte) {
    // void *click in gEditCtrl
    // This would need GTK event handling
    return true;
  }
  return false;
}

/**********************************************************************
 * CompMenu - handle menu commands for composition
 * Ported to work with GTK menus
 **********************************************************************/
bool CompMenu(MyWindowPtr win, int menuItem) {
  MessHandle messH = Win2MessH(win);

  if (!messH)
    return false;

  switch (menuItem) {
  case SEND_ITEM:
    // Send message
    return CompSend(messH);

  case SAVE_ITEM:
    // Save message
    return CompSave(messH);

  default:
    return false;
  }
}

/**********************************************************************
 * CompKey - handle keyboard input for composition
 * Ported from Mac key handling to GTK
 **********************************************************************/
bool CompKey(MyWindowPtr win, GdkEvent *event) {
  if (win && win->pte) {
    // void *key events in gEditCtrl
    // This would need GTK key event handling
    return true;
  }
  return false;
}

/**********************************************************************
 * CompButton - handle button clicks in composition
 * Ported to work with GTK buttons
 **********************************************************************/
bool CompButton(MyWindowPtr win, GtkWidget *button, GdkEvent *event) {
  MessHandle messH = Win2MessH(win);

  if (!messH)
    return false;

  if (button == messH->sendButton) {
    // Send button clicked
    return CompSend(messH);
  }

  return false;
}

/**********************************************************************
 * CompHelp - provide help for composition window
 **********************************************************************/
void CompHelp(MyWindowPtr win, int helpType) {
  // Show help for composition window
  // This would integrate with GTK help system
}

/**********************************************************************
 * CompGonnaShow - prepare composition window for display
 * Ported from Mac window showing to GTK
 **********************************************************************/
void CompGonnaShow(MyWindowPtr win) {
  if (win && win->pte) {
    // Prepare gEditCtrl for display
    gtk_widget_show(win->pte);
  }
}

/**********************************************************************
 * CompDragHandler - handle drag operations in composition
 * Ported from Mac drag handling to GTK
 **********************************************************************/
bool CompDragHandler(MyWindowPtr win, void *dragEvent) {
  // void *drag operations in gEditCtrl
  // This would need GTK drag and drop implementation
  return false;
}

/**********************************************************************
 * CompIdle - handle idle processing for composition
 * Ported from Mac idle processing to GTK
 **********************************************************************/
void CompIdle(MyWindowPtr win) {
  MessHandle messH = Win2MessH(win);

  if (messH && win->pte) {
    // void *idle processing for gEditCtrl
    // Auto-save, spell check, etc.
  }
}

/**********************************************************************
 * on_wanna_save_response - callback for the Save/Cancel/Discard dialog
 * Matches original Eudora WannaSave behavior from compact.c
 **********************************************************************/
static void on_wanna_save_response(GObject *source, GAsyncResult *res,
                                   gpointer user_data) {
  (void)source;
  MyWindowPtr win = (MyWindowPtr)user_data;
  GError *err = NULL;
  int choice = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source),
                                               res, &err);
  if (err) {
    g_error_free(err);
    return; /* dialog cancelled/error — keep window open */
  }

  switch (choice) {
  case 0: { /* Save */
    MessHandle messH = Win2MessH(win);
    if (messH && CompSave(messH)) {
      win->isDirty = false;
      gtk_window_close(GTK_WINDOW(win->window));
    }
    break;
  }
  case 1: /* Cancel */
    break; /* do nothing, window stays open */
  case 2: /* Discard */
    win->isDirty = false;
    gtk_window_close(GTK_WINDOW(win->window));
    break;
  }
}

/**********************************************************************
 * on_comp_close_request - GTK close-request handler for compose window
 * Intercepts the close and shows WannaSave dialog if dirty.
 **********************************************************************/
static gboolean on_comp_close_request(GtkWindow *window, gpointer user_data) {
  (void)window;
  MyWindowPtr win = (MyWindowPtr)user_data;
  if (!win)
    return FALSE; /* allow close */

  MessHandle messH = Win2MessH(win);
  if (!messH)
    return FALSE; /* allow close */

  if (!win->isDirty)
    return FALSE; /* not dirty — allow close */

  /* Show Save/Cancel/Discard dialog (WannaSave) */
  const char *title_str = gtk_window_get_title(GTK_WINDOW(win->window));
  gchar *msg = g_strdup_printf("Save changes to \"%s\"?",
                                title_str ? title_str : "New Message");

  GtkAlertDialog *dlg = gtk_alert_dialog_new("%s", msg);
  g_free(msg);

  const char *buttons[] = {"Save", "Cancel", "Don't Save", NULL};
  gtk_alert_dialog_set_buttons(dlg, buttons);
  gtk_alert_dialog_set_cancel_button(dlg, 1);
  gtk_alert_dialog_set_default_button(dlg, 0);

  gtk_alert_dialog_choose(dlg, GTK_WINDOW(win->window), NULL,
                          on_wanna_save_response, win);
  g_object_unref(dlg);

  return TRUE; /* prevent close until dialog answered */
}

/**********************************************************************
 * on_comp_body_changed - track dirty state when body text changes
 **********************************************************************/
static void on_comp_body_changed(geditDocument *doc, gpointer user_data) {
  (void)doc;
  MyWindowPtr win = (MyWindowPtr)user_data;
  if (win)
    win->isDirty = true;
}

/**********************************************************************
 * CompClose - close composition window (cleanup after dialog resolved)
 * Ported to work with GTK window closing
 **********************************************************************/
bool CompClose(MyWindowPtr win) {
  MessHandle messH = Win2MessH(win);

  if (!messH)
    return true;

  // Clean up gEditCtrl
  if (win->pte) {
    g_object_unref(win->pte);
    win->pte = NULL;
  }

  // Remove from message list
  LL_Remove(MessList, messH, (MessHandle));

  // Free message handle
  g_free(messH);

  return true;
}

/**********************************************************************
 * CompSend - send the composition
 **********************************************************************/
bool CompSend(MessHandle messH) {
  MyWindowPtr win = messH->win;
  if (!win || !win->pte)
    return false;

  /* Save the message first */
  if (!CompSave(messH))
    return false;

  TOCType *tocH = messH->tocH;
  int sumNum = messH->sumNum;

  /* Set persId from the From dropdown selection */
  {
    GtkWidget *fromDD = messH->headerWidgets[FROM_HEAD];
    if (fromDD && GTK_IS_DROP_DOWN(fromDD)) {
      guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(fromDD));
      if (sel == 0) {
        /* Dominant personality */
        tocH->sums[sumNum].persId = 0;
      } else {
        /* Non-dominant: find matching personality by index */
        PrefsAccount accounts[16];
        int n = prefs_load_accounts(accounts, 16);
        if (sel - 1 < (guint)n) {
          tocH->sums[sumNum].persId = Hash(accounts[sel - 1].name);
        }
      }
    }
  }

  /* Mark as queued for sending */
  tocH->sums[sumNum].state = QUEUED;
  TOCSetDirty(tocH, true);

  extern int WriteTOC(TOCType *tocH);
  WriteTOC(tocH);

  g_print("CompSend: queued sum %d, persId=%u, tocH=%p count=%d\n",
          sumNum, tocH->sums[sumNum].persId, (void*)tocH, tocH->count);

  /* Detach messH from summary so send thread can pick it up */
  tocH->sums[sumNum].messH = NULL;

  /* Close the compose window */
  if (win->window)
    gtk_window_close(GTK_WINDOW(win->window));

  /* Trigger immediate mail transfer for queued messages */
  extern short XferMail(bool check, bool send, bool manual, bool scripted,
                        bool thread, short modifiers);
  XferMail(false, true, true, false, true, 0);

  return true;
}

/**********************************************************************
 * CompSave - save the composition
 **********************************************************************/
bool CompSave(MessHandle messH) {
  MyWindowPtr win = messH->win;
  TOCType * tocH = messH->tocH;
  int sumNum = messH->sumNum;

  if (!win || !win->pte)
    return false;

  /* Ensure the mailbox file is open */
  extern int BoxFOpen(TOCType *tocH);
  extern int WriteTOC(TOCType *tocH);
  if (tocH->refN == 0)
    BoxFOpen(tocH);
  if (tocH->refN <= 0)
    return false;

  /* For new/unsaved messages, append at end of mailbox file */
  long offset = tocH->sums[sumNum].offset;
  if (offset == 0 && tocH->sums[sumNum].length == 0) {
    off_t end = lseek(tocH->refN, 0, SEEK_END);
    if (end < 0) return false;
    offset = (long)end;
  }

  int err = WriteComp(messH, tocH->refN, offset);

  if (!err) {
    TOCSetDirty(tocH, true);
    tocH->reallyDirty = true;
    WriteTOC(tocH);
    win->isDirty = false;

    /* Update the mailbox list to reflect the saved message */
    extern void InvalSum(TOCType *tocH, short sumNum);
    InvalSum(tocH, sumNum);

    /* Also refresh the Out mailbox TreeView if it's open */
    if (tocH->win && tocH->win->window) {
      GtkWidget *tree = g_object_get_data(G_OBJECT(tocH->win->window), "mbox-tree");
      if (tree && GTK_IS_TREE_VIEW(tree)) {
        GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
        if (store) {
          /* Check if this sumNum already has a row */
          GtkTreeIter iter;
          gboolean found = FALSE;
          gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
          while (valid) {
            int idx = -1;
            gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &idx, -1);
            if (idx == sumNum) { found = TRUE; break; }
            valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
          }
          if (!found) {
            /* New message — append a row */
            gtk_list_store_append(store, &iter);
          }
          /* InvalSum idle callback will fill in the columns */
        }
      }
    }

    return true;
  }

  return false;
}

/**********************************************************************
 * Additional helper functions
 **********************************************************************/

/**********************************************************************
 * CreateMessageBody - create a new message body
 **********************************************************************/
int CreateMessageBody(char *buffer, unsigned long *uidHash) {
  char msgId[256];
  time_t now = time(NULL);

  NewMessageId(msgId);
  *uidHash = Hash((unsigned char *)msgId);

  /* Get configured return address */
  gchar *email = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "email_address", "");
  gchar *name = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "real_name", "");
  gchar *from_addr;
  if (name && name[0] && email && email[0])
    from_addr = g_strdup_printf("%s <%s>", name, email);
  else if (email && email[0])
    from_addr = g_strdup(email);
  else
    from_addr = g_strdup("");

  /* Sendmail envelope line (expected by header parser), then headers */
  char dateStr[64];
  strftime(dateStr, sizeof(dateStr), "%a %b %d %H:%M:%S %Y", localtime(&now));
  int len = snprintf(buffer, 1024,
                     "From %s %s\r\n"
                     "To: \r\n"
                     "From: %s\r\n"
                     "Subject: \r\n"
                     "Cc: \r\n"
                     "Bcc: \r\n"
                     "Attachments: \r\n"
                     "Message-ID: %s\r\n"
                     "\r\n",
                     email[0] ? email : "user", dateStr,
                     from_addr, msgId);

  g_free(from_addr);
  g_free(email);
  g_free(name);
  return len;
}

/**********************************************************************
 * GatherCompAddresses - implemented in nickmng.c
 **********************************************************************/
/**********************************************************************
 * CompHead* — GTK4 compose header field management
 *
 * On Mac these manipulated text ranges in a single PETE editor.
 * On GTK each header is a separate GtkEntry widget stored in
 * messH->headerWidgets[TO_HEAD..BCC_HEAD], plus the body in bodyPTE.
 **********************************************************************/

/* Helper: get the GtkEntry text for a header index */
static const char *comp_header_get(MessHandle messH, short index) {
  if (!messH || index < 1 || index > 15) return "";
  GtkWidget *w = messH->headerWidgets[index];
  if (!w) return "";
  if (GTK_IS_EDITABLE(w))
    return gtk_editable_get_text(GTK_EDITABLE(w));
  if (GTK_IS_DROP_DOWN(w)) {
    GtkStringList *model = GTK_STRING_LIST(gtk_drop_down_get_model(GTK_DROP_DOWN(w)));
    guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(w));
    return gtk_string_list_get_string(model, sel);
  }
  return "";
}

/* Helper: set the GtkEntry text for a header index */
static void comp_header_set(MessHandle messH, short index, const char *text) {
  if (!messH || index < 1 || index > 15 || !text) return;
  GtkWidget *w = messH->headerWidgets[index];
  if (!w) return;
  if (GTK_IS_EDITABLE(w))
    gtk_editable_set_text(GTK_EDITABLE(w), text);
  /* Can't set text on GtkDropDown directly */
}

/**********************************************************************
 * CompHeadFind — find a header by index in a compose message
 **********************************************************************/
HSPtr CompHeadFind(MessHandle messH, short index, HSPtr hSpec) {
  if (!messH || !hSpec) return NULL;
  memset(hSpec, 0, sizeof(HeadSpec));
  if (index < 0 || index > 15) return NULL;
  /* For GTK, HeadSpec offset/length refer to the widget content */
  hSpec->index = index;
  if (index == BODY_HEAD || index == 0) {
    /* Body — return info about the body PTE */
    if (messH->bodyPTE) {
      long len = gedit_document_get_length(
          geditctrl_get_document(messH->bodyPTE));
      hSpec->value = 0;
      hSpec->stop = len;
      hSpec->length = len;
    }
  } else {
    const char *txt = comp_header_get(messH, index);
    hSpec->value = 0;
    hSpec->length = txt ? strlen(txt) : 0;
    hSpec->stop = hSpec->length;
  }
  return hSpec;
}

/**********************************************************************
 * CompHeadFindStr — find a header by name string
 **********************************************************************/
HSPtr CompHeadFindStr(MessHandle messH, char *name, HSPtr hSpec) {
  if (!messH || !name || !hSpec) return NULL;
  /* Map name to index */
  static const struct { const char *n; short idx; } map[] = {
    {"To:", TO_HEAD}, {"From:", FROM_HEAD}, {"Subject:", SUBJ_HEAD},
    {"Cc:", CC_HEAD}, {"Bcc:", BCC_HEAD}, {"Attachments:", ATTACH_HEAD},
    {NULL, 0}
  };
  for (int i = 0; map[i].n; i++) {
    if (g_ascii_strncasecmp(name, map[i].n, strlen(map[i].n)) == 0)
      return CompHeadFind(messH, map[i].idx, hSpec);
  }
  return NULL;
}

/**********************************************************************
 * CompHeadGetText — get the text of a header field (allocates)
 **********************************************************************/
int CompHeadGetText(GtkWidget *pte, HSPtr hSpec, char **text) {
  if (!text) return -1;
  *text = NULL;
  if (!pte || !hSpec) return -1;
  /* Find the messH from the pte widget's window */
  GtkWidget *toplevel = gtk_widget_get_ancestor(pte, GTK_TYPE_WINDOW);
  if (!toplevel) return -1;
  /* Get messH from window private data */
  MyWindowPtr win = (MyWindowPtr)g_object_get_data(G_OBJECT(toplevel), "mywindow");
  if (!win) return -1;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return -1;

  if (hSpec->index == BODY_HEAD || hSpec->index == 0) {
    /* Body text from gEditCtrl */
    if (messH->bodyPTE) {
      GtkTextBuffer *buf = geditctrl_get_document(messH->bodyPTE)
          ? gtk_text_view_get_buffer(GTK_TEXT_VIEW(messH->bodyPTE))
          : NULL;
      if (buf) {
        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(buf, &start, &end);
        *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
        return 0;
      }
    }
    return -1;
  }

  const char *val = comp_header_get(messH, hSpec->index);
  if (val) {
    *text = g_strdup(val);
    return 0;
  }
  return -1;
}

/**********************************************************************
 * CompHeadGetStrLo — get header text into a fixed buffer
 **********************************************************************/
int CompHeadGetStrLo(MessHandle messH, short index, char *string, short size) {
  if (!messH || !string || size <= 0) return -1;
  string[0] = '\0';
  const char *val = comp_header_get(messH, index);
  if (val) {
    g_strlcpy(string, val, size);
    return 0;
  }
  return -1;
}

/**********************************************************************
 * CompHeadSet — set the text of a header field
 **********************************************************************/
int CompHeadSet(GtkWidget *pte, HSPtr hSpec, char *text) {
  if (!pte || !hSpec || !text) return -1;
  GtkWidget *toplevel = gtk_widget_get_ancestor(pte, GTK_TYPE_WINDOW);
  if (!toplevel) return -1;
  MyWindowPtr win = (MyWindowPtr)g_object_get_data(G_OBJECT(toplevel), "mywindow");
  if (!win) return -1;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return -1;

  if (hSpec->index == BODY_HEAD || hSpec->index == 0) {
    if (messH->bodyPTE) {
      GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(messH->bodyPTE));
      if (buf) {
        gtk_text_buffer_set_text(buf, text, -1);
        return 0;
      }
    }
    return -1;
  }
  comp_header_set(messH, hSpec->index, text);
  return 0;
}

/**********************************************************************
 * CompHeadSetPtr — set header from ptr + size
 **********************************************************************/
int CompHeadSetPtr(GtkWidget *pte, HSPtr hSpec, char *text, long size) {
  if (!text || size < 0) return -1;
  char *tmp = g_strndup(text, size);
  int err = CompHeadSet(pte, hSpec, tmp);
  g_free(tmp);
  return err;
}

/**********************************************************************
 * CompHeadAppendPtr — append text to a header field
 **********************************************************************/
int CompHeadAppendPtr(GtkWidget *pte, HSPtr hSpec, char *text, long size) {
  if (!pte || !hSpec || !text) return -1;
  GtkWidget *toplevel = gtk_widget_get_ancestor(pte, GTK_TYPE_WINDOW);
  if (!toplevel) return -1;
  MyWindowPtr win = (MyWindowPtr)g_object_get_data(G_OBJECT(toplevel), "mywindow");
  if (!win) return -1;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return -1;

  char *append = g_strndup(text, size);

  if (hSpec->index == BODY_HEAD || hSpec->index == 0) {
    if (messH->bodyPTE) {
      GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(messH->bodyPTE));
      if (buf) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buf, &end);
        gtk_text_buffer_insert(buf, &end, append, -1);
        g_free(append);
        return 0;
      }
    }
    g_free(append);
    return -1;
  }

  const char *existing = comp_header_get(messH, hSpec->index);
  char *combined = g_strconcat(existing ? existing : "", append, NULL);
  comp_header_set(messH, hSpec->index, combined);
  g_free(combined);
  g_free(append);
  return 0;
}

/**********************************************************************
 * CompHeadPrependPtr — prepend text to a header field
 **********************************************************************/
int CompHeadPrependPtr(GtkWidget *pte, HSPtr hSpec, char *text, long size) {
  if (!pte || !hSpec || !text) return -1;
  GtkWidget *toplevel = gtk_widget_get_ancestor(pte, GTK_TYPE_WINDOW);
  if (!toplevel) return -1;
  MyWindowPtr win = (MyWindowPtr)g_object_get_data(G_OBJECT(toplevel), "mywindow");
  if (!win) return -1;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return -1;

  char *prepend = g_strndup(text, size);
  const char *existing = comp_header_get(messH, hSpec->index);
  char *combined = g_strconcat(prepend, existing ? existing : "", NULL);
  comp_header_set(messH, hSpec->index, combined);
  g_free(combined);
  g_free(prepend);
  return 0;
}

/**********************************************************************
 * CompHeadActivate — focus a header field
 **********************************************************************/
int CompHeadActivate(GtkWidget *pte, HSPtr hSpec) {
  if (!pte || !hSpec) return -1;
  GtkWidget *toplevel = gtk_widget_get_ancestor(pte, GTK_TYPE_WINDOW);
  if (!toplevel) return -1;
  MyWindowPtr win = (MyWindowPtr)g_object_get_data(G_OBJECT(toplevel), "mywindow");
  if (!win) return -1;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return -1;

  GtkWidget *w = messH->headerWidgets[hSpec->index];
  if (w && gtk_widget_get_visible(w))
    gtk_widget_grab_focus(w);
  return 0;
}

/**********************************************************************
 * CompHeadCurrent — return which header field is currently focused
 **********************************************************************/
short CompHeadCurrent(GtkWidget *pte) {
  if (!pte) return -1;
  GtkWidget *toplevel = gtk_widget_get_ancestor(pte, GTK_TYPE_WINDOW);
  if (!toplevel) return -1;
  MyWindowPtr win = (MyWindowPtr)g_object_get_data(G_OBJECT(toplevel), "mywindow");
  if (!win) return -1;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return -1;

  GtkWidget *focused = gtk_window_get_focus(GTK_WINDOW(toplevel));
  for (int i = 1; i <= 15; i++) {
    if (messH->headerWidgets[i] == focused)
      return i;
  }
  if (focused == messH->bodyPTE)
    return BODY_HEAD;
  return -1;
}

/**********************************************************************
 * CompSwitchFields — tab between header fields and body
 **********************************************************************/
void CompSwitchFields(MyWindowPtr win, bool forward) {
  if (!win) return;
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  if (!messH) return;

  short cur = CompHeadCurrent(messH->bodyPTE);
  /* Build ordered list of focusable widgets */
  short order[] = { TO_HEAD, FROM_HEAD, SUBJ_HEAD, CC_HEAD, BCC_HEAD, BODY_HEAD };
  int nfields = 6;
  int curIdx = -1;
  for (int i = 0; i < nfields; i++) {
    if (order[i] == cur) { curIdx = i; break; }
  }
  if (curIdx < 0) curIdx = 0;

  int nextIdx = forward ? (curIdx + 1) % nfields : (curIdx - 1 + nfields) % nfields;
  short nextHead = order[nextIdx];

  if (nextHead == BODY_HEAD) {
    if (messH->bodyPTE) gtk_widget_grab_focus(messH->bodyPTE);
  } else {
    GtkWidget *w = messH->headerWidgets[nextHead];
    if (w) gtk_widget_grab_focus(w);
  }
}

/**********************************************************************
 * HandleHeadFindStr — find a header by name in raw message text
 * Searches for "name: value\r\n" pattern in text
 **********************************************************************/
HSPtr HandleHeadFindStr(char *text, char *name, HSPtr hSpec) {
  if (!text || !name || !hSpec) return NULL;
  memset(hSpec, 0, sizeof(HeadSpec));

  size_t nameLen = strlen(name);
  char *p = text;

  while (*p) {
    /* Match header name at start of line (case-insensitive) */
    if (g_ascii_strncasecmp(p, name, nameLen) == 0) {
      char *valStart = p + nameLen;
      /* Skip optional whitespace after colon */
      while (*valStart == ' ' || *valStart == '\t') valStart++;

      /* Find end of header value (handles continuation lines) */
      char *valEnd = valStart;
      while (*valEnd) {
        if (*valEnd == '\r' || *valEnd == '\n') {
          char *next = valEnd;
          if (*next == '\r') next++;
          if (*next == '\n') next++;
          /* Continuation line? (starts with space/tab) */
          if (*next == ' ' || *next == '\t') {
            valEnd = next;
            continue;
          }
          break;
        }
        valEnd++;
      }

      hSpec->start = hSpec->offset = p - text;
      hSpec->value = valStart - text;
      hSpec->stop = valEnd - text;
      hSpec->length = valEnd - p;
      return hSpec;
    }

    /* Skip to next line */
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;
    /* Empty line = end of headers */
    if (*p == '\r' || *p == '\n' || *p == '\0') break;
  }
  return NULL;
}

/**********************************************************************
 * HandleHeadGetText — extract header value text (allocates copy)
 **********************************************************************/
int HandleHeadGetText(char *textIn, HSPtr hSpec, char **text) {
  if (!textIn || !hSpec || !text) return -1;
  *text = NULL;

  long valLen = hSpec->stop - hSpec->value;
  if (valLen <= 0) return -1;

  *text = g_strndup(textIn + hSpec->value, valLen);
  return *text ? 0 : -1;
}

/**********************************************************************
 * HandleHeadGetIdText — find header by resource ID, extract value
 **********************************************************************/
int HandleHeadGetIdText(char *textIn, short id, char **text) {
  HeadSpec hs;
  char headerName[64];

  if (!textIn || !text) return -1;
  *text = NULL;

  GetRString(headerName, HEADER_STRN + id);
  if (!HandleHeadFindStr(textIn, headerName, &hs))
    return -1;

  return HandleHeadGetText(textIn, &hs, text);
}

/**********************************************************************
 * HandleHeadGetPStr — find header by ID, copy value into buffer
 **********************************************************************/
char *HandleHeadGetPStr(char *text, short head, char *val) {
  HeadSpec hs;
  char headerName[64];

  if (!text || !val) return val;
  val[0] = '\0';

  GetRString(headerName, HEADER_STRN + head);
  if (HandleHeadFindStr(text, headerName, &hs)) {
    long len = hs.stop - hs.value;
    if (len > 254) len = 254;
    if (len > 0)
      memcpy(val, text + hs.value, len);
    val[len] = '\0';
  }
  return val;
}
