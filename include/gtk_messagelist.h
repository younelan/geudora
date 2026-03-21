/*
 * EudoraMessage GObject wrapper for MacmbxMsgSum
 * Required for GtkColumnView/GListModel
 */

#ifndef GTK_MESSAGELIST_ITEM_H
#define GTK_MESSAGELIST_ITEM_H

#include "toc.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define GTK_TYPE_MESSAGELIST_ITEM (gtk_messagelist_item_get_type())

G_DECLARE_FINAL_TYPE(GtkMessageListItem, gtk_messagelist_item, GTK,
                     MESSAGELIST_ITEM, GObject)

GtkMessageListItem *gtk_messagelist_item_new(MacmbxMsgSum *summary,
                                             int index);

/* Getters - matches original Eudora mailbox columns */
const char *gtk_messagelist_item_get_status(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_priority(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_attach(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_label(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_from(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_subject(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_date(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_size(GtkMessageListItem *msg);
const char *gtk_messagelist_item_get_junk(GtkMessageListItem *msg);
int gtk_messagelist_item_get_index(GtkMessageListItem *msg);

G_END_DECLS

#endif
