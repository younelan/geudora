/* linkwin.c — Link History Window
 *
 * TODO: Rewrite as GTK window with GtkColumnView.
 * The Mac ListView implementation has been removed.
 * Data model is in linkmng.c (HistoryStruct, gHistories).
 *
 * Columns: Name, Date, Type, Label
 * Sort: by any column, ascending/descending
 * Actions: Open URL, Delete, Remind
 */

#include <gtk/gtk.h>
#include <stdbool.h>
#include <string.h>
#include "mailbox.h"
#include "linkmng.h"
#include "linkwin.h"
#include "gtk_dialogs.h"
#include "fileutil.h"
#include "mydefs.h"

/* LinkTickle — called by linkmng.c when history data changes.
   Will refresh the GTK list when the window is implemented. */
void LinkTickle(void) {
  /* TODO: refresh GtkColumnView when link history window exists */
}

/* OpenLinkWin — open the link history window */
void OpenLinkWin(void) {
  /* TODO: create GTK window with GtkColumnView showing link history */
}

/* NotifyLinkWin — notify window of data changes */
void NotifyLinkWin(void) {
  LinkTickle();
}

/* LinkHasCustomIcons — always false until GTK implementation */
bool LinkHasCustomIcons(void) { return false; }

/* RemindDlogHit — handle remind dialog (stub) */
bool RemindDlogHit(void *event, void *theDialog, short itemHit,
                   short *item) {
  (void)event; (void)theDialog; (void)itemHit; (void)item;
  return false;
}
