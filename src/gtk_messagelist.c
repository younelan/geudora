/*
 * GtkMessageListItem GObject wrapper implementation
 */

#include "gtk_messagelist.h"
#include "StringUtil.h"
#include <stdio.h>


struct _GtkMessageListItem {
  GObject parent_instance;
  char *from;
  char *subject;
  char *date;
  char *size;
  int index;
};

G_DEFINE_TYPE(GtkMessageListItem, gtk_messagelist_item, G_TYPE_OBJECT)

static void gtk_messagelist_item_finalize(GObject *object) {
  GtkMessageListItem *self = GTK_MESSAGELIST_ITEM(object);
  g_free(self->from);
  g_free(self->subject);
  g_free(self->date);
  g_free(self->size);
  G_OBJECT_CLASS(gtk_messagelist_item_parent_class)->finalize(object);
}

static void gtk_messagelist_item_init(GtkMessageListItem *self) {}

static void gtk_messagelist_item_class_init(GtkMessageListItemClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = gtk_messagelist_item_finalize;
}

GtkMessageListItem *gtk_messagelist_item_new(MessageSummary *summary,
                                             int index) {
  GtkMessageListItem *msg = g_object_new(GTK_TYPE_MESSAGELIST_ITEM, NULL);

  if (summary) {
    msg->from = ensure_utf8(summary->from);
    msg->subject = ensure_utf8(summary->subj);


    char date_str[64];
    time_t t = summary->seconds;
    struct tm *tm_info = localtime(&t);
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);
    msg->date = g_strdup(date_str);

    char size_str[32];
    if (summary->length < 1024) {
      snprintf(size_str, sizeof(size_str), "%ld B", summary->length);
    } else if (summary->length < 1024 * 1024) {
      snprintf(size_str, sizeof(size_str), "%.1f KB", summary->length / 1024.0);
    } else {
      snprintf(size_str, sizeof(size_str), "%.1f MB",
               summary->length / (1024.0 * 1024.0));
    }
    msg->size = g_strdup(size_str);

    msg->index = index;
  }

  return msg;
}

const char *gtk_messagelist_item_get_from(GtkMessageListItem *msg) {
  return msg->from;
}
const char *gtk_messagelist_item_get_subject(GtkMessageListItem *msg) {
  return msg->subject;
}
const char *gtk_messagelist_item_get_date(GtkMessageListItem *msg) {
  return msg->date;
}
const char *gtk_messagelist_item_get_size(GtkMessageListItem *msg) {
  return msg->size;
}
int gtk_messagelist_item_get_index(GtkMessageListItem *msg) {
  return msg->index;
}
