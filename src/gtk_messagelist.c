/*
 * GtkMessageListItem GObject wrapper implementation
 * Matches original Eudora mailbox list columns:
 * Status, Priority, Attach, Label, From, Subject, Date, Size, Junk
 */

#include "gtk_messagelist.h"
#include "StringUtil.h"
#include <stdio.h>


struct _GtkMessageListItem {
  GObject parent_instance;
  char *status;
  char *priority;
  char *attach;
  char *label;
  char *from;
  char *subject;
  char *date;
  char *size;
  char *junk;
  int index;
};

G_DEFINE_TYPE(GtkMessageListItem, gtk_messagelist_item, G_TYPE_OBJECT)

static void gtk_messagelist_item_finalize(GObject *object) {
  GtkMessageListItem *self = GTK_MESSAGELIST_ITEM(object);
  g_free(self->status);
  g_free(self->priority);
  g_free(self->attach);
  g_free(self->label);
  g_free(self->from);
  g_free(self->subject);
  g_free(self->date);
  g_free(self->size);
  g_free(self->junk);
  G_OBJECT_CLASS(gtk_messagelist_item_parent_class)->finalize(object);
}

static void gtk_messagelist_item_init(GtkMessageListItem *self) {}

static void gtk_messagelist_item_class_init(GtkMessageListItemClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->finalize = gtk_messagelist_item_finalize;
}

/* State enum to display symbol */
static const char *state_to_str(StateEnum s) {
  switch (s) {
    case UNREAD:       return "\xe2\x97\x8f"; /* ● */
    case READ:         return "";
    case REPLIED:      return "\xe2\x86\xa9"; /* ↩ */
    case FORWARDED:    return "\xe2\x86\x92"; /* → */
    case REDIST:       return "\xe2\x87\x89"; /* ⇉ */
    case UNSENDABLE:   return "\xe2\x9c\x8f"; /* ✏ */
    case SENDABLE:     return "\xe2\x9c\x93"; /* ✓ */
    case QUEUED:       return "\xe2\x8f\xb3"; /* ⏳ */
    case SENT:         return "\xe2\x9c\x89"; /* ✉ */
    case UNSENT:       return "\xe2\x9c\x8f"; /* ✏ */
    case TIMED:        return "\xe2\x8f\xb0"; /* ⏰ */
    case BUSY_SENDING: return "\xe2\x87\xa7"; /* ⇧ */
    case MESG_ERR:     return "\xe2\x9a\xa0"; /* ⚠ */
    default:           return "";
  }
}

/* Priority (0-200) to display string */
static const char *priority_to_str(int priority) {
  int p = priority / 40; /* 0-5 */
  switch (p) {
    case 1: return "\xe2\x86\x91\xe2\x86\x91"; /* ↑↑ Highest */
    case 2: return "\xe2\x86\x91";              /* ↑  High */
    case 4: return "\xe2\x86\x93";              /* ↓  Low */
    case 5: return "\xe2\x86\x93\xe2\x86\x93"; /* ↓↓ Lowest */
    default: return "";                          /*    Normal/none */
  }
}

GtkMessageListItem *gtk_messagelist_item_new(MacmbxMsgSum *summary,
                                             int index) {
  GtkMessageListItem *msg = g_object_new(GTK_TYPE_MESSAGELIST_ITEM, NULL);

  if (summary) {
    /* Status */
    msg->status = g_strdup(state_to_str(summary->state));

    /* Priority */
    msg->priority = g_strdup(priority_to_str(summary->priority));

    /* Attachment indicator */
    msg->attach = g_strdup(
        (summary->flags & FLAG_HAS_ATT) ? "\xf0\x9f\x93\x8e" : ""); /* 📎 */

    /* Label (from hue flag bits, 0-15) */
    int hue = 0;
    if (summary->flags & FLAG_HUE1) hue |= 1;
    if (summary->flags & FLAG_HUE2) hue |= 2;
    if (summary->flags & FLAG_HUE3) hue |= 4;
    if (summary->flags & FLAG_HUE4) hue |= 8;
    if (hue > 0) {
      char lbl[4];
      snprintf(lbl, sizeof(lbl), "%d", hue);
      msg->label = g_strdup(lbl);
    } else {
      msg->label = g_strdup("");
    }

    /* From/To */
    msg->from = ensure_utf8(summary->from);

    /* Subject */
    msg->subject = ensure_utf8(summary->subject);

    /* Date */
    char date_str[64];
    time_t t = summary->seconds;
    struct tm *tm_info = localtime(&t);
    if (tm_info)
      strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);
    else
      date_str[0] = '\0';
    msg->date = g_strdup(date_str);

    /* Size */
    char size_str[32];
    if (summary->length < 1024)
      snprintf(size_str, sizeof(size_str), "%ld B", summary->length);
    else if (summary->length < 1024 * 1024)
      snprintf(size_str, sizeof(size_str), "%.1f KB", summary->length / 1024.0);
    else
      snprintf(size_str, sizeof(size_str), "%.1f MB",
               summary->length / (1024.0 * 1024.0));
    msg->size = g_strdup(size_str);

    /* Junk score */
    if (summary->spam_score > 0) {
      char jbuf[8];
      snprintf(jbuf, sizeof(jbuf), "%ld", (long)summary->spam_score);
      msg->junk = g_strdup(jbuf);
    } else {
      msg->junk = g_strdup("");
    }

    msg->index = index;
  }

  return msg;
}

const char *gtk_messagelist_item_get_status(GtkMessageListItem *msg) {
  return msg->status;
}
const char *gtk_messagelist_item_get_priority(GtkMessageListItem *msg) {
  return msg->priority;
}
const char *gtk_messagelist_item_get_attach(GtkMessageListItem *msg) {
  return msg->attach;
}
const char *gtk_messagelist_item_get_label(GtkMessageListItem *msg) {
  return msg->label;
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
const char *gtk_messagelist_item_get_junk(GtkMessageListItem *msg) {
  return msg->junk;
}
int gtk_messagelist_item_get_index(GtkMessageListItem *msg) {
  return msg->index;
}
