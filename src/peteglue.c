/*
 * peteglue.c — PETE glue layer ported to gEditCtrl.
 *
 * Original peteglue.c (3440 lines) wrapped the PETE editor library.
 * This reimplements the functions Eudora calls using
 * geditDocument / GtkTextView / GtkTextBuffer.
 *
 * PETEHandle = GtkWidget* (gEditCtrl widget)
 */
#include "peteglue.h"
#include "gedit-document.h"
#include <string.h>
#include <stdlib.h>

/* Edit operation enums matching original PETE constants */
enum {
  peeEvent = 0,
  peeCut = 1,
  peeCopy = 2,
  peePaste = 3,
  peeClear = 4,
  peeSelectAll = 5
};

/* Scroll constants */
enum {
  pseCenterSelection = -2
};

/* ================================================================
 * Validation
 * ================================================================ */
bool PeteIsValid(GtkWidget *pte) {
  return pte != NULL && GTK_IS_WIDGET(pte);
}

/* ================================================================
 * Raw text / length
 * ================================================================ */
int PETEGetRawText(void *unused, GtkWidget *ctrl, void **out_text) {
  (void)unused;
  if (!ctrl || !out_text) return -1;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return -1;
  gchar *txt = gedit_document_get_text(doc);
  if (!txt) { *out_text = NULL; return -1; }
  *out_text = txt; /* caller frees with g_free / free */
  return 0;
}

int PeteGetRawText(GtkWidget *ctrl, void **out_text) {
  return PETEGetRawText(NULL, ctrl, out_text);
}

int PETEGetTextLen(void *unused, GtkWidget *ctrl) {
  (void)unused;
  if (!ctrl) return 0;
  geditDocument *doc = geditctrl_get_document(ctrl);
  return doc ? gedit_document_get_length(doc) : 0;
}

int PETEGetTextLen2(GtkWidget *ctrl) { return PETEGetTextLen(NULL, ctrl); }
int PeteLen(GtkWidget *ctrl) { return PETEGetTextLen(NULL, ctrl); }

int PeteGetTextAndSelection(GtkWidget *ctrl, void **out_text,
                            long *selStart, long *selEnd) {
  if (!ctrl) return -1;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return -1;

  if (out_text) {
    gchar *txt = gedit_document_get_text(doc);
    *out_text = txt; /* may be NULL */
  }

  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  GtkTextIter sI, eI;
  if (gtk_text_buffer_get_selection_bounds(buf, &sI, &eI)) {
    if (selStart) *selStart = gtk_text_iter_get_offset(&sI);
    if (selEnd) *selEnd = gtk_text_iter_get_offset(&eI);
  } else {
    GtkTextMark *ins = gtk_text_buffer_get_insert(buf);
    GtkTextIter cur;
    gtk_text_buffer_get_iter_at_mark(buf, &cur, ins);
    long off = gtk_text_iter_get_offset(&cur);
    if (selStart) *selStart = off;
    if (selEnd) *selEnd = off;
  }
  return 0;
}

/* ================================================================
 * Insertion
 * ================================================================ */
int PETEInsertTextPtr(void *unused, GtkWidget *ctrl, int pos,
                      const char *ptr, int len, void *opt) {
  (void)unused; (void)opt;
  if (!ctrl || !ptr) return -1;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return -1;
  gint offset = (pos == 0x7fffffff)
                    ? gedit_document_get_length(doc)
                    : pos;
  gchar *tmp = g_strndup(ptr, len);
  gedit_document_insert_text(doc, offset, tmp);
  g_free(tmp);
  return 0;
}

int PETEInsertParaPtr(void *unused, GtkWidget *ctrl, int pos,
                      void *a, void *b, int c, void *d) {
  (void)unused; (void)a; (void)b; (void)c; (void)d;
  if (!ctrl) return -1;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return -1;
  gint offset = (pos == 0x7fffffff)
                    ? gedit_document_get_length(doc)
                    : pos;
  gedit_document_insert_text(doc, offset, "\n\n");
  return 0;
}

int PeteInsertPtr(GtkWidget *ctrl, int offset, const char *ptr, int len) {
  return PETEInsertTextPtr(NULL, ctrl, offset, ptr, len, NULL);
}

int PeteInsertChar(GtkWidget *ctrl, int offset, char ch, void *opt) {
  char s[2] = { ch, 0 };
  return PETEInsertTextPtr(NULL, ctrl, offset, s, 1, opt);
}

int PeteSetTextPtr(GtkWidget *ctrl, const char *text, int len) {
  if (!ctrl) return -1;
  geditDocument *doc = geditctrl_get_document(ctrl);
  if (!doc) return -1;
  gint L = gedit_document_get_length(doc);
  if (L > 0) gedit_document_delete_range(doc, 0, L);
  if (text && len > 0) {
    gchar *tmp = g_strndup(text, len);
    gedit_document_insert_text(doc, 0, tmp);
    g_free(tmp);
  }
  return 0;
}

/* ================================================================
 * Delete
 * ================================================================ */
int PeteDelete(GtkWidget *pte, long start, long stop) {
  if (!PeteIsValid(pte)) return -1;
  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return -1;
  gint len = gedit_document_get_length(doc);
  if (stop > len) stop = len;
  if (start >= stop) return 0;
  gedit_document_delete_range(doc, (gint)start, (gint)(stop - start));
  return 0;
}

/* ================================================================
 * PeteEdit — handle edit operations (cut/copy/paste/clear/key/mouse)
 *
 * Original called PETEEdit which dispatched clipboard ops, key events,
 * and mouse events to the PETE library. In GTK4 the text view handles
 * key/mouse natively. We only need to implement clipboard operations
 * and event forwarding.
 * ================================================================ */
int PeteEdit(MyWindowPtr win, GtkWidget *pte, int what, void *event) {
  if (!pte && win) {
    /* win->pte — access through struct offset, but we don't include
       message.h here. The caller should pass pte explicitly. */
    return -1;
  }
  if (!PeteIsValid(pte)) return -1;

  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return -1;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);

  switch (what) {
  case peeCut: {
    GdkClipboard *cb = gdk_display_get_clipboard(gdk_display_get_default());
    GtkTextIter sI, eI;
    if (gtk_text_buffer_get_selection_bounds(buf, &sI, &eI)) {
      gchar *sel = gtk_text_buffer_get_text(buf, &sI, &eI, FALSE);
      gdk_clipboard_set_text(cb, sel);
      g_free(sel);
      gtk_text_buffer_delete(buf, &sI, &eI);
    }
    break;
  }
  case peeCopy: {
    GdkClipboard *cb = gdk_display_get_clipboard(gdk_display_get_default());
    GtkTextIter sI, eI;
    if (gtk_text_buffer_get_selection_bounds(buf, &sI, &eI)) {
      gchar *sel = gtk_text_buffer_get_text(buf, &sI, &eI, FALSE);
      gdk_clipboard_set_text(cb, sel);
      g_free(sel);
    }
    break;
  }
  case peePaste:
    /* GTK handles paste via the text view's default bindings.
       For programmatic paste, emit the paste-clipboard signal. */
    g_signal_emit_by_name(pte, "paste-clipboard");
    break;
  case peeClear: {
    GtkTextIter sI, eI;
    if (gtk_text_buffer_get_selection_bounds(buf, &sI, &eI))
      gtk_text_buffer_delete(buf, &sI, &eI);
    break;
  }
  case peeSelectAll: {
    GtkTextIter sI, eI;
    gtk_text_buffer_get_start_iter(buf, &sI);
    gtk_text_buffer_get_end_iter(buf, &eI);
    gtk_text_buffer_select_range(buf, &sI, &eI);
    break;
  }
  case peeEvent:
    /* Key/mouse events — GTK text view handles these natively.
       If caller passes an event, forward via geditctrl_handle_key
       when applicable. For now, no-op as the event controller
       on the text view already processes input. */
    break;
  default:
    break;
  }
  return 0;
}

/* ================================================================
 * PeteSelect — set the selection range
 *
 * Original also reset style labels at the selection. In gEditCtrl
 * we just set the selection via GtkTextBuffer.
 * ================================================================ */
void PeteSelect(MyWindowPtr win, GtkWidget *pte, long start, long stop) {
  (void)win;
  if (!PeteIsValid(pte)) return;
  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  gint len = gedit_document_get_length(doc);

  if (start < 0) start = 0;
  if (stop < 0) stop = 0;
  if (start > len) start = len;
  if (stop > len) stop = len;

  GtkTextIter sI, eI;
  gtk_text_buffer_get_iter_at_offset(buf, &sI, (gint)start);
  gtk_text_buffer_get_iter_at_offset(buf, &eI, (gint)stop);
  gtk_text_buffer_select_range(buf, &sI, &eI);
}

/* ================================================================
 * PeteFocus — move input focus to an editor widget
 *
 * Original deactivated the previous PTE, set win->pte, activated
 * the new one, and drew focus rings. In GTK4, gtk_widget_grab_focus
 * handles activation and visual focus indication natively.
 * ================================================================ */
void PeteFocus(MyWindowPtr win, GtkWidget *pte, bool focus) {
  (void)win; /* win->pte management done by caller */
  if (!PeteIsValid(pte)) return;
  if (focus)
    gtk_widget_grab_focus(pte);
}

/* ================================================================
 * PeteScroll — scroll the editor view
 *
 * Original called PETEScroll(horizontal, vertical). Vertical values:
 * pseCenterSelection (-2) = scroll selection into center of view.
 * Positive/negative = scroll by lines.
 * ================================================================ */
int PeteScroll(GtkWidget *pte, short horizontal, short vertical) {
  (void)horizontal; /* horizontal scrolling rarely used */
  if (!PeteIsValid(pte)) return -1;

  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return -1;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);

  if (vertical == pseCenterSelection || vertical == 0) {
    /* Scroll the insertion point / selection into view */
    GtkTextMark *ins = gtk_text_buffer_get_insert(buf);
    /* Find the GtkTextView inside the scrolled window */
    GtkWidget *child = gtk_widget_get_first_child(pte);
    while (child && !GTK_IS_TEXT_VIEW(child))
      child = gtk_widget_get_next_sibling(child);
    if (child && GTK_IS_TEXT_VIEW(child))
      gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(child), ins,
                                   0.1, TRUE, 0.0, 0.5);
  }
  /* For line-based scrolling we could adjust the vadjustment,
     but in practice Eudora mostly uses pseCenterSelection */
  return 0;
}

/* ================================================================
 * Undo support
 *
 * Original PetePrepareUndo saved undo state and turned off auto-undo.
 * PeteFinishUndo restored it and inserted the undo record.
 * In GTK4, GtkTextBuffer has built-in undo. We use begin/end
 * user action to group operations.
 * ================================================================ */
void PETEAllowUndo(void *unused, GtkWidget *ctrl, int a, int b) {
  (void)unused; (void)ctrl; (void)a; (void)b;
}

int PetePrepareUndo(GtkWidget *pte, short undoWhat, long start, long stop,
                    long *uStart, long *uStop) {
  (void)undoWhat;
  if (!PeteIsValid(pte)) return -1;

  /* Resolve current-selection markers */
  if (start < 0 || stop < 0)
    PeteGetTextAndSelection(pte, NULL, &start, &stop);

  if (uStart) *uStart = start;
  if (uStop) *uStop = stop;

  /* Begin a user-action group so all subsequent edits are one undo step */
  geditDocument *doc = geditctrl_get_document(pte);
  if (doc) {
    GtkTextBuffer *buf = gedit_document_get_buffer(doc);
    gtk_text_buffer_begin_user_action(buf);
  }
  return 0;
}

int PeteFinishUndo(GtkWidget *pte, short undoWhat, long start, long stop) {
  (void)undoWhat; (void)start; (void)stop;
  if (!PeteIsValid(pte)) return -1;

  geditDocument *doc = geditctrl_get_document(pte);
  if (doc) {
    GtkTextBuffer *buf = gedit_document_get_buffer(doc);
    gtk_text_buffer_end_user_action(buf);
  }
  return 0;
}

/* ================================================================
 * Calc on/off — freeze/thaw layout
 *
 * Original suspended/resumed PETE's layout recalculation.
 * GtkTextView doesn't need explicit freeze/thaw for typical edits,
 * but we could potentially use this for batch operations later.
 * ================================================================ */
void PETECalcOn(GtkWidget *ctrl) { (void)ctrl; }
void PETECalcOff(GtkWidget *ctrl) { (void)ctrl; }

/* ================================================================
 * URL scanning — find and highlight URLs in the text
 *
 * Original scanned from urlScanned offset forward, found protocol
 * colons (http:, ftp:, mailto:, etc.), expanded to full URL
 * boundaries, and applied pURLLabel styling.
 *
 * In GTK4, we use a simple regex-based scan and apply a "url" tag
 * to matched text. The tag can be styled via CSS.
 * ================================================================ */
void PeteSetURLRescan(GtkWidget *pte, long spot) {
  if (!PeteIsValid(pte)) return;
  /* Store the rescan offset via GObject data */
  g_object_set_data(G_OBJECT(pte), "pete-url-scanned",
                    GINT_TO_POINTER((gint)spot));
}

void PeteURLScan(MyWindowPtr win, GtkWidget *pte) {
  (void)win;
  if (!PeteIsValid(pte)) return;

  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  gint textLen = gedit_document_get_length(doc);

  gint scanned = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(pte), "pete-url-scanned"));
  if (scanned == -1 || scanned >= textLen) return;

  /* Ensure we have a "url" tag */
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buf);
  GtkTextTag *urlTag = gtk_text_tag_table_lookup(table, "url");
  if (!urlTag) {
    urlTag = gtk_text_buffer_create_tag(buf, "url",
                                        "foreground", "#0000EE",
                                        "underline", PANGO_UNDERLINE_SINGLE,
                                        NULL);
  }

  /* Get the text from the scanned offset onward */
  GtkTextIter scanStart;
  gtk_text_buffer_get_iter_at_offset(buf, &scanStart, scanned);
  GtkTextIter textEnd;
  gtk_text_buffer_get_end_iter(buf, &textEnd);
  gchar *text = gtk_text_buffer_get_text(buf, &scanStart, &textEnd, FALSE);
  if (!text) return;

  /* Simple URL matching: look for protocol:// or mailto: patterns */
  static const char *protocols[] = {
    "http://", "https://", "ftp://", "mailto:", "file://", NULL
  };

  gchar *pos = text;
  while (*pos) {
    bool found = false;
    for (int i = 0; protocols[i]; i++) {
      size_t plen = strlen(protocols[i]);
      if (g_ascii_strncasecmp(pos, protocols[i], plen) == 0) {
        /* Found a protocol — extend to end of URL */
        gchar *urlStart = pos;
        gchar *urlEnd = pos + plen;
        while (*urlEnd && !g_ascii_isspace(*urlEnd) &&
               *urlEnd != '>' && *urlEnd != '"' && *urlEnd != '\'' &&
               *urlEnd != ')' && *urlEnd != ']')
          urlEnd++;
        /* Strip trailing punctuation */
        while (urlEnd > urlStart + (long)plen &&
               (urlEnd[-1] == '.' || urlEnd[-1] == ',' ||
                urlEnd[-1] == ';' || urlEnd[-1] == ':'))
          urlEnd--;

        gint startOff = scanned + (gint)(urlStart - text);
        gint endOff = scanned + (gint)(urlEnd - text);
        GtkTextIter tS, tE;
        gtk_text_buffer_get_iter_at_offset(buf, &tS, startOff);
        gtk_text_buffer_get_iter_at_offset(buf, &tE, endOff);
        gtk_text_buffer_apply_tag(buf, urlTag, &tS, &tE);

        pos = urlEnd;
        found = true;
        break;
      }
    }
    if (!found) {
      /* Advance by one UTF-8 character */
      pos = g_utf8_next_char(pos);
    }
  }

  g_free(text);

  /* Mark as fully scanned */
  g_object_set_data(G_OBJECT(pte), "pete-url-scanned",
                    GINT_TO_POINTER(-1));
}

/* ================================================================
 * Dirty state
 * ================================================================ */
long PeteIsDirty(GtkWidget *pte) {
  if (!PeteIsValid(pte)) return 0;
  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return 0;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  return gtk_text_buffer_get_modified(buf) ? 1 : 0;
}

void PeteSetDirty(GtkWidget *pte, bool dirty) {
  if (!PeteIsValid(pte)) return;
  geditDocument *doc = geditctrl_get_document(pte);
  if (!doc) return;
  GtkTextBuffer *buf = gedit_document_get_buffer(doc);
  gtk_text_buffer_set_modified(buf, dirty);
}

bool PeteIsDirtyList(GtkWidget *pte) {
  for (; pte; pte = PeteNext(pte))
    if (PeteIsDirty(pte)) return true;
  return false;
}

void PeteCleanList(GtkWidget *pte) {
  for (; pte; pte = PeteNext(pte))
    PeteSetDirty(pte, false);
}

/* ================================================================
 * PeteExtra — per-widget metadata
 *
 * Original used Handle-based PeteExtraStruct attached to each
 * PETEHandle. We use GObject data on the GtkWidget.
 * ================================================================ */
static PeteExtraStruct *pete_extra_get_or_create(GtkWidget *ctrl) {
  PeteExtraStruct *ex =
      g_object_get_data(G_OBJECT(ctrl), "pete-extra");
  if (!ex) {
    ex = g_new0(PeteExtraStruct, 1);
    g_object_set_data_full(G_OBJECT(ctrl), "pete-extra", ex, g_free);
  }
  return ex;
}

PeteExtraHandle PeteExtra(GtkWidget *ctrl) {
  if (!ctrl) return NULL;
  PeteExtraStruct *ex = pete_extra_get_or_create(ctrl);
  /* Callers do (*PeteExtra(pte))->win, so we need PeteExtraStruct **
     (a pointer to a pointer to the struct). We store a heap-allocated
     PeteExtraStruct* that points to the actual struct. */
  PeteExtraStruct **hp =
      g_object_get_data(G_OBJECT(ctrl), "pete-extra-handle");
  if (!hp) {
    hp = g_new(PeteExtraStruct *, 1);
    g_object_set_data_full(G_OBJECT(ctrl), "pete-extra-handle", hp, g_free);
  }
  *hp = ex;
  return hp; /* PeteExtraHandle == PeteExtraStruct ** */
}

void PeteSetWin(GtkWidget *ctrl, struct MyWindow *win) {
  if (!ctrl) return;
  PeteExtraStruct *ex = pete_extra_get_or_create(ctrl);
  ex->win = win;
}

/* ================================================================
 * Linked list traversal
 *
 * Original PETE maintained a singly-linked list of PETEHandle per
 * window. We store next/prev pointers via GObject data.
 * ================================================================ */
GtkWidget *PeteNext(GtkWidget *pte) {
  if (!pte) return NULL;
  return g_object_get_data(G_OBJECT(pte), "pete-next");
}

void PeteLink(GtkWidget *pte, GtkWidget **list) {
  if (!pte || !list) return;
  g_object_set_data(G_OBJECT(pte), "pete-next", *list);
  *list = pte;
}

void PeteRemove(GtkWidget *pte, GtkWidget **list) {
  if (!pte || !list) return;
  if (*list == pte) {
    *list = PeteNext(pte);
    g_object_set_data(G_OBJECT(pte), "pete-next", NULL);
    return;
  }
  for (GtkWidget *p = *list; p; p = PeteNext(p)) {
    if (PeteNext(p) == pte) {
      g_object_set_data(G_OBJECT(p), "pete-next", PeteNext(pte));
      g_object_set_data(G_OBJECT(pte), "pete-next", NULL);
      return;
    }
  }
}
