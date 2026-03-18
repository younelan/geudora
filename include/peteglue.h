/*
 * peteglue.h — PETE glue layer ported to gEditCtrl.
 *
 * Original peteglue.c (3440 lines) wrapped the PETE editor library.
 * This provides the same API using geditDocument / GtkTextView.
 * PETEHandle = GtkWidget* (the gEditCtrl widget).
 */
#ifndef PETEGLUE_H
#define PETEGLUE_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "geditctrl.h"

G_BEGIN_DECLS

/* Forward declare MyWindow so we don't pull in message.h */
struct MyWindow;
/* MyWindowPtr defined in mailbox.h */

/* --- Raw text / length --- */
int PETEGetRawText(void *unused, GtkWidget *ctrl, void **out_text);
int PeteGetRawText(GtkWidget *ctrl, void **out_text);
int PETEGetTextLen(void *unused, GtkWidget *ctrl);
int PETEGetTextLen2(GtkWidget *ctrl);
int PeteLen(GtkWidget *ctrl);
int PeteGetTextAndSelection(GtkWidget *ctrl, void **out_text,
                            long *selStart, long *selEnd);

/* --- Insertion --- */
int PETEInsertTextPtr(void *unused, GtkWidget *ctrl, int pos,
                      const char *ptr, int len, void *opt);
int PETEInsertParaPtr(void *unused, GtkWidget *ctrl, int pos,
                      void *a, void *b, int c, void *d);
int PeteInsertPtr(GtkWidget *ctrl, int offset, const char *ptr, int len);
int PeteInsertChar(GtkWidget *ctrl, int offset, char ch, void *opt);
int PeteSetTextPtr(GtkWidget *ctrl, const char *text, int len);

/* --- Edit operations (cut/copy/paste/key/mouse) --- */
int PeteEdit(MyWindowPtr win, GtkWidget *pte, int what, void *event);

/* --- Selection --- */
void PeteSelect(MyWindowPtr win, GtkWidget *pte, long start, long stop);

/* --- Focus --- */
void PeteFocus(MyWindowPtr win, GtkWidget *pte, bool focus);

/* --- Scrolling --- */
int PeteScroll(GtkWidget *pte, short horizontal, short vertical);

/* --- Undo --- */
void PETEAllowUndo(void *unused, GtkWidget *ctrl, int a, int b);
int PetePrepareUndo(GtkWidget *pte, short undoWhat, long start, long stop,
                    long *uStart, long *uStop);
int PeteFinishUndo(GtkWidget *pte, short undoWhat, long start, long stop);

/* --- Calc on/off (layout freeze/thaw) --- */
void PETECalcOn(GtkWidget *ctrl);
void PETECalcOff(GtkWidget *ctrl);
#define PeteCalcOn(pte)  PETECalcOn(pte)
#define PeteCalcOff(pte) PETECalcOff(pte)

/* --- URL scanning --- */
void PeteURLScan(MyWindowPtr win, GtkWidget *pte);
void PeteSetURLRescan(GtkWidget *pte, long spot);

/* --- Dirty state --- */
long PeteIsDirty(GtkWidget *pte);
void PeteSetDirty(GtkWidget *pte, bool dirty);
bool PeteIsDirtyList(GtkWidget *pte);
void PeteCleanList(GtkWidget *pte);

/* --- Validation --- */
bool PeteIsValid(GtkWidget *pte);

/* --- Delete --- */
int PeteDelete(GtkWidget *pte, long start, long stop);

/* --- PeteExtra — associates owning window with editor widget --- */
typedef struct {
  struct MyWindow *win;
  long urlScanned;   /* offset up to which URLs have been scanned */
} PeteExtraStruct, *PeteExtraHandle;

PeteExtraHandle PeteExtra(GtkWidget *ctrl);
void PeteSetWin(GtkWidget *ctrl, struct MyWindow *win);

/* --- Linked list traversal (editor list per window) --- */
GtkWidget *PeteNext(GtkWidget *pte);
void PeteLink(GtkWidget *pte, GtkWidget **list);
void PeteRemove(GtkWidget *pte, GtkWidget **list);

G_END_DECLS

#endif /* PETEGLUE_H */
