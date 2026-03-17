/*
 * GTK4 Menu Implementation for gEudora
 * Ported from Eudora Carbon resource files
 */

#include "gtk_menus.h"
#include <string.h>

/* Global callback array */
static MenuCallback g_menu_callbacks[ACTION_COUNT] = {NULL};

/* File Menu Items */
static const MenuItem file_menu_items[] = {
    {"_New Message", "<Control>n", ACTION_NEW_MESSAGE, FALSE, FALSE},
    {"_Open...", "<Control>o", ACTION_OPEN, FALSE, FALSE},
    {"Open _Selection", NULL, ACTION_OPEN_SELECTION, FALSE, FALSE},
    {"Open in _Browser", NULL, ACTION_OPEN_IN_BROWSER, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"_Close", "<Control>w", ACTION_CLOSE, FALSE, FALSE},
    {"_Save", "<Control>s", ACTION_SAVE, FALSE, FALSE},
    {"Save _As...", "<Control><Shift>s", ACTION_SAVE_AS, FALSE, FALSE},
    {"_Revert", NULL, ACTION_REVERT, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"Page Set_up...", NULL, ACTION_PAGE_SETUP, FALSE, FALSE},
    {"_Print...", "<Control>p", ACTION_PRINT, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"_Quit", "<Control>q", ACTION_QUIT, FALSE, FALSE},
    {NULL, NULL, 0, FALSE, FALSE} /* End marker */
};

/* Edit Menu Items */
static const MenuItem edit_menu_items[] = {
    {"_Undo", "<Control>z", ACTION_UNDO, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"Cu_t", "<Control>x", ACTION_CUT, FALSE, FALSE},
    {"_Copy", "<Control>c", ACTION_COPY, FALSE, FALSE},
    {"_Paste", "<Control>v", ACTION_PASTE, FALSE, FALSE},
    {"Paste as _Quotation", NULL, ACTION_PASTE_QUOTATION, FALSE, FALSE},
    {"_Clear", NULL, ACTION_CLEAR, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"_Select All", "<Control>a", ACTION_SELECT_ALL, FALSE, FALSE},
    {"_Wrap Selection", NULL, ACTION_WRAP_SELECTION, FALSE, FALSE},
    {"_Finish Nickname", "<Control>comma", ACTION_FINISH_NICKNAME, FALSE,
     FALSE},
    {"_Insert Recipient", NULL, ACTION_INSERT_RECIPIENT, FALSE, FALSE},
    {"Insert _Emoticon", NULL, ACTION_INSERT_EMOTICON, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"_Speak", NULL, ACTION_SPEAK, FALSE, FALSE},
    {"_Spelling", NULL, ACTION_SPELLING, FALSE, FALSE},
    {NULL, NULL, 0, FALSE, FALSE} /* End marker */
};

/* Message Menu Items */
static const MenuItem message_menu_items[] = {
    {"_New Message", "<Control>n", ACTION_NEW_MSG, FALSE, FALSE},
    {"_Reply", "<Control>r", ACTION_REPLY, FALSE, FALSE},
    {"Reply _All", "<Control><Shift>r", ACTION_REPLY_ALL, FALSE, FALSE},
    {"_Forward", NULL, ACTION_FORWARD, FALSE, FALSE},
    {"_Redirect", NULL, ACTION_REDIRECT, FALSE, FALSE},
    {"Send A_gain", NULL, ACTION_SEND_AGAIN, FALSE, FALSE},
    {"_Queue for Delivery", NULL, ACTION_QUEUE_FOR_DELIVERY, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"_Delete", "Delete", ACTION_DELETE, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"Mark as _Read", NULL, ACTION_MARK_AS_READ, FALSE, FALSE},
    {"Mark as _Unread", NULL, ACTION_MARK_AS_UNREAD, FALSE, FALSE},
    {"Mark as _Junk", NULL, ACTION_MARK_AS_JUNK, FALSE, FALSE},
    {"Mark as _Not Junk", NULL, ACTION_MARK_AS_NOT_JUNK, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"Change _Status", NULL, ACTION_CHANGE_STATUS, FALSE, TRUE},
    {"Change _Priority", NULL, ACTION_CHANGE_PRIORITY, FALSE, TRUE},
    {"Change _Label", NULL, ACTION_CHANGE_LABEL, FALSE, TRUE},
    {"Change _Personality", NULL, ACTION_CHANGE_PERSONALITY, FALSE, TRUE},
    {NULL, NULL, 0, FALSE, FALSE} /* End marker */
};

/* Transfer Menu Items */
static const MenuItem transfer_menu_items[] = {
    {"→ _In", NULL, ACTION_TRANSFER_IN, FALSE, TRUE},
    {"→ _Out", NULL, ACTION_TRANSFER_OUT, FALSE, TRUE},
    {NULL, NULL, 0, FALSE, FALSE} /* End marker */
};

/* Window Menu Items */
static const MenuItem window_menu_items[] = {
    {"_Minimize Window", "<Control>m", ACTION_MINIMIZE, FALSE, FALSE},
    {"_Bring All to Front", NULL, ACTION_BRING_ALL_TO_FRONT, FALSE, FALSE},
    {"_Send to Back", "<Control>backslash", ACTION_SEND_TO_BACK, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {"_Tabs", NULL, ACTION_TABS, FALSE, FALSE},
    {"_Drawer", NULL, ACTION_DRAWER, FALSE, FALSE},
    {NULL, NULL, 0, FALSE, FALSE} /* End marker */
};

/* Scripts Menu Items */
static const MenuItem scripts_menu_items[] = {
    {"_Open Scripts Folder", NULL, ACTION_OPEN_SCRIPTS_FOLDER, FALSE, FALSE},
    {NULL, NULL, 0, TRUE, FALSE}, /* Separator */
    {NULL, NULL, 0, FALSE, FALSE} /* End marker */
};

/* Helper function to create a menu from menu items - GTK4 version */
static GtkWidget *create_menu_from_items(const MenuItem *items) {
  GtkWidget *menu = gtk_popover_menu_new_from_model(NULL);
  GMenu *gmenu = g_menu_new();

  for (int i = 0; items[i].label != NULL || items[i].is_separator; i++) {
    if (items[i].is_separator) {
      /* GTK4 doesn't use separator items in menus, just skip */
      continue;
    } else {
      GMenuItem *item = g_menu_item_new(items[i].label, NULL);
      g_menu_append_item(gmenu, item);
      g_object_unref(item);
    }
  }

  gtk_popover_menu_set_menu_model(GTK_POPOVER_MENU(menu), G_MENU_MODEL(gmenu));
  g_object_unref(gmenu);

  return menu;
}

/* Create individual menus */
GtkWidget *create_file_menu(void) {
  return create_menu_from_items(file_menu_items);
}

GtkWidget *create_edit_menu(void) {
  return create_menu_from_items(edit_menu_items);
}

GtkWidget *create_message_menu(void) {
  return create_menu_from_items(message_menu_items);
}

GtkWidget *create_transfer_menu(void) {
  return create_menu_from_items(transfer_menu_items);
}

GtkWidget *create_window_menu(void) {
  return create_menu_from_items(window_menu_items);
}

GtkWidget *create_scripts_menu(void) {
  return create_menu_from_items(scripts_menu_items);
}

/* Create the complete menu bar - GTK4 system menu */
GtkWidget *create_menu_bar(GtkWidget *window) {
  GApplication *app = g_application_get_default();

  /* Build the main menu structure */
  GMenu *main_menu = g_menu_new();

  /* File menu */
  GMenu *file_menu = g_menu_new();
  g_menu_append(file_menu, "_New Message", "app.new-message");
  g_menu_append(file_menu, "_Open...", "app.open");
  g_menu_append(file_menu, "Open _Selection", "app.open-selection");
  g_menu_append(file_menu, "Open in _Browser", "app.open-browser");
  g_menu_append_section(file_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(file_menu, "_Close", "app.close");
  g_menu_append(file_menu, "_Save", "app.save");
  g_menu_append(file_menu, "Save _As...", "app.save-as");
  g_menu_append(file_menu, "_Revert", "app.revert");
  g_menu_append_section(file_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(file_menu, "Page Set_up...", "app.page-setup");
  g_menu_append(file_menu, "_Print...", "app.print");
  g_menu_append_section(file_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(file_menu, "_Quit", "app.quit");
  g_menu_append_submenu(main_menu, "_File", G_MENU_MODEL(file_menu));

  /* Edit menu */
  GMenu *edit_menu = g_menu_new();
  g_menu_append(edit_menu, "_Undo", "app.undo");
  g_menu_append_section(edit_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(edit_menu, "Cu_t", "app.cut");
  g_menu_append(edit_menu, "_Copy", "app.copy");
  g_menu_append(edit_menu, "_Paste", "app.paste");
  g_menu_append(edit_menu, "Paste as _Quotation", "app.paste-quotation");
  g_menu_append(edit_menu, "_Clear", "app.clear");
  g_menu_append_section(edit_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(edit_menu, "_Select All", "app.select-all");
  g_menu_append(edit_menu, "_Wrap Selection", "app.wrap-selection");
  g_menu_append(edit_menu, "_Finish Nickname", "app.finish-nickname");
  g_menu_append(edit_menu, "_Insert Recipient", "app.insert-recipient");
  /* Build emoticon submenu dynamically from the emoticon table */
  {
    extern int EmoCount(void);
    extern const char *EmoGetAscii(int index);
    extern const char *EmoGetEmoji(int index);
    extern const char *EmoGetMeaning(int index);
    extern void EmoInit(void);
    EmoInit();
    int emo_count = EmoCount();
    if (emo_count > 0) {
      GMenu *emo_submenu = g_menu_new();
      /* Show a reasonable subset — group by category later if desired */
      int shown = 0;
      for (int i = 0; i < emo_count && shown < 40; i++) {
        /* Skip duplicates (shorter patterns for same emoji) */
        if (i > 0 && strcmp(EmoGetEmoji(i), EmoGetEmoji(i - 1)) == 0)
          continue;
        char label[128];
        const char *meaning = EmoGetMeaning(i);
        snprintf(label, sizeof(label), "%s  %s  (%s)",
                 EmoGetEmoji(i), EmoGetAscii(i), meaning);
        char action_name[64];
        snprintf(action_name, sizeof(action_name), "app.insert-emoticon(%d)", i);
        g_menu_append(emo_submenu, label, action_name);
        shown++;
      }
      g_menu_append_submenu(edit_menu, "Insert _Emoticon",
                            G_MENU_MODEL(emo_submenu));
    } else {
      g_menu_append(edit_menu, "Insert _Emoticon", "app.insert-emoticon");
    }
  }
  g_menu_append_section(edit_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(edit_menu, "_Speak", "app.speak");
  g_menu_append(edit_menu, "_Spelling", "app.spelling");
  g_menu_append_submenu(main_menu, "_Edit", G_MENU_MODEL(edit_menu));

  /* Message menu */
  GMenu *message_menu = g_menu_new();
  g_menu_append(message_menu, "_New Message", "app.new-message");
  g_menu_append(message_menu, "_Reply", "app.reply");
  g_menu_append(message_menu, "Reply _All", "app.reply-all");
  g_menu_append(message_menu, "_Forward", "app.forward");
  g_menu_append(message_menu, "_Redirect", "app.redirect");
  g_menu_append(message_menu, "Send A_gain", "app.send-again");
  g_menu_append(message_menu, "_Queue for Delivery", "app.queue");
  g_menu_append_section(message_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(message_menu, "_Delete", "app.delete");
  g_menu_append_section(message_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(message_menu, "Mark as _Read", "app.mark-read");
  g_menu_append(message_menu, "Mark as _Unread", "app.mark-unread");
  g_menu_append(message_menu, "Mark as _Junk", "app.mark-junk");
  g_menu_append(message_menu, "Mark as _Not Junk", "app.mark-not-junk");
  g_menu_append_section(message_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(message_menu, "Change _Status", "app.change-status");
  g_menu_append(message_menu, "Change _Priority", "app.change-priority");
  g_menu_append(message_menu, "Change _Label", "app.change-label");
  g_menu_append(message_menu, "Change _Personality", "app.change-personality");
  g_menu_append_submenu(main_menu, "_Message", G_MENU_MODEL(message_menu));

  /* Transfer menu */
  GMenu *transfer_menu = g_menu_new();
  g_menu_append(transfer_menu, "→ _In", "app.transfer-in");
  g_menu_append(transfer_menu, "→ _Out", "app.transfer-out");
  g_menu_append_submenu(main_menu, "_Transfer", G_MENU_MODEL(transfer_menu));

  /* Window menu */
  GMenu *window_menu = g_menu_new();
  g_menu_append(window_menu, "_Minimize Window", "app.minimize");
  g_menu_append(window_menu, "_Bring All to Front", "app.bring-to-front");
  g_menu_append(window_menu, "_Send to Back", "app.send-to-back");
  g_menu_append_section(window_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(window_menu, "_Tabs", "app.tabs");
  g_menu_append(window_menu, "_Drawer", "app.drawer");
  g_menu_append_submenu(main_menu, "_Window", G_MENU_MODEL(window_menu));

  /* Tools menu — wazoo windows */
  GMenu *tools_menu = g_menu_new();
  g_menu_append(tools_menu, "_Address Book", "app.address-book");
  g_menu_append(tools_menu, "_Filters", "app.filters");
  g_menu_append(tools_menu, "_Personalities", "app.personalities");
  g_menu_append(tools_menu, "Si_gnatures", "app.signatures");
  g_menu_append(tools_menu, "_Statistics", "app.statistics");
  g_menu_append_section(tools_menu, NULL, G_MENU_MODEL(g_menu_new()));
  g_menu_append(tools_menu, "_Check Mail", "app.check-mail");
  g_menu_append(tools_menu, "_Send Queued", "app.send-queued");
  g_menu_append_submenu(main_menu, "_Tools", G_MENU_MODEL(tools_menu));

  /* Scripts menu */
  GMenu *scripts_menu = g_menu_new();
  g_menu_append(scripts_menu, "_Open Scripts Folder", "app.open-scripts");
  g_menu_append_submenu(main_menu, "_Scripts", G_MENU_MODEL(scripts_menu));

  /* Settings/Preferences menu */
  GMenu *prefs_menu = g_menu_new();
  g_menu_append(prefs_menu, "_Preferences...", "app.preferences");
  g_menu_append_submenu(main_menu, "_Settings", G_MENU_MODEL(prefs_menu));

  /* Set as application menu */
  gtk_application_set_menubar(GTK_APPLICATION(app), G_MENU_MODEL(main_menu));

  g_object_unref(file_menu);
  g_object_unref(edit_menu);
  g_object_unref(message_menu);
  g_object_unref(transfer_menu);
  g_object_unref(tools_menu);
  g_object_unref(window_menu);
  g_object_unref(scripts_menu);
  g_object_unref(prefs_menu);
  g_object_unref(main_menu);

  /* Return NULL since system menu is handled by the application */
  return NULL;
}

/* Global registry for menus */
static GHashTable *g_menu_registry = NULL;
static GHashTable *g_menu_id_registry = NULL;

void register_menu_model(int menu_id, GMenuModel *model) {
  if (!g_menu_registry) {
    g_menu_registry = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
                                            g_object_unref);
    g_menu_id_registry =
        g_hash_table_new(g_direct_hash, g_direct_equal); /* value is int */
  }
  g_hash_table_insert(g_menu_registry, GINT_TO_POINTER(menu_id),
                      g_object_ref(model));
  g_hash_table_insert(g_menu_id_registry, model, GINT_TO_POINTER(menu_id));
}

MenuHandle GetMHandle(short menu_id) {
  if (!g_menu_registry)
    return NULL;
  return G_MENU_MODEL(
      g_hash_table_lookup(g_menu_registry, GINT_TO_POINTER(menu_id)));
}

int GetMenuID(GMenuModel *mh) {
  if (!g_menu_id_registry || !mh)
    return 0;
  return GPOINTER_TO_INT(g_hash_table_lookup(g_menu_id_registry, mh));
}

GMenuModel *ParentMailboxMenu(GMenuModel *mh, short *item) {
  if (!g_menu_registry || !mh)
    return NULL;

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, g_menu_registry);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    GMenuModel *parent = G_MENU_MODEL(value);
    int n = g_menu_model_get_n_items(parent);
    for (int i = 0; i < n; i++) {
      GMenuModel *submenu =
          g_menu_model_get_item_link(parent, i, G_MENU_LINK_SUBMENU);
      if (submenu) {
        if (submenu == mh) {
          if (item)
            *item = i + 1; /* 1-based legacy index */
          g_object_unref(submenu);
          return parent;
        }
        /* Recurse for nested submenus */
        GMenuModel *found = ParentMailboxMenu(submenu, item);
        g_object_unref(submenu);
        if (found)
          return found;
      }
    }
  }
  return NULL;
}

short SubmenuId(MenuHandle mh, short item) {
  int index = item - 1;
  if (!mh || index < 0 || index >= g_menu_model_get_n_items(mh))
    return 0;

  GMenuModel *submenu =
      g_menu_model_get_item_link(mh, index, G_MENU_LINK_SUBMENU);
  if (submenu) {
    int id = GetMenuID(submenu);
    g_object_unref(submenu);
    return (short)id;
  }
  return 0;
}

void DeleteMenuItem(MenuHandle mh, short item) {
  int index = item - 1;
  if (!G_IS_MENU(mh) || index < 0 || index >= g_menu_model_get_n_items(mh))
    return;
  g_menu_remove(G_MENU(mh), index);
}

/* Helper function to get label from menu item */
static gchar *get_menu_item_label(GMenuModel *model, int index) {
  gchar *label = NULL;
  if (g_menu_model_get_item_attribute(model, index, G_MENU_ATTRIBUTE_LABEL, "s",
                                      &label)) {
    return label;
  }
  return NULL;
}

void EnableItem(MenuHandle mh, short item) {
  /* In GTK4 GMenuModel, enabling/disabling is typically handled via actions.
   * If this item is tied to an action, we should enable the action.
   */
  int index = item - 1;
  if (!G_IS_MENU(mh) || index < 0 || index >= g_menu_model_get_n_items(mh))
    return;

  /* For now, just set an 'enabled' attribute that the UI can pick up */
  GMenuItem *gitem = g_menu_item_new(NULL, NULL);
  gchar *label = get_menu_item_label(mh, index);
  if (label) {
    g_menu_item_set_label(gitem, label);
    g_free(label);
  }
  g_menu_item_set_attribute(gitem, "enabled", "b", TRUE);
  g_menu_remove(G_MENU(mh), index);
  g_menu_insert_item(G_MENU(mh), index, gitem);
  g_object_unref(gitem);
}

void DisableItem(MenuHandle mh, short item) {
  int index = item - 1;
  if (!G_IS_MENU(mh) || index < 0 || index >= g_menu_model_get_n_items(mh))
    return;
  GMenuItem *gitem = g_menu_item_new(NULL, NULL);
  gchar *label = get_menu_item_label(mh, index);
  if (label) {
    g_menu_item_set_label(gitem, label);
    g_free(label);
  }
  g_menu_item_set_attribute(gitem, "enabled", "b", FALSE);
  g_menu_remove(G_MENU(mh), index);
  g_menu_insert_item(G_MENU(mh), index, gitem);
  g_object_unref(gitem);
}

void SetItemMark(MenuHandle mh, short item, short mark) {
  /* In GTK4, menu checkmarks are handled via stateful actions.
   * Store the mark as an attribute for menu rendering to pick up. */
  int index = item - 1;
  if (!G_IS_MENU(mh) || index < 0 || index >= g_menu_model_get_n_items(mh))
    return;
  GMenuItem *gitem = g_menu_item_new(NULL, NULL);
  gchar *label = get_menu_item_label(mh, index);
  if (label) {
    g_menu_item_set_label(gitem, label);
    g_free(label);
  }
  g_menu_item_set_attribute(gitem, "mark", "i", (gint)mark);
  g_menu_remove(G_MENU(mh), index);
  g_menu_insert_item(G_MENU(mh), index, gitem);
  g_object_unref(gitem);
}

void CalcMenuSize(MenuHandle mh) {
  /* GTK handles menu layout automatically; nothing to do. */
  (void)mh;
}

/* Helper function to get label from menu item */

int CountMenuItems(GMenuModel *mh) {
  if (!mh)
    return 0;
  return g_menu_model_get_n_items(mh);
}

char *MyGetItem(MenuHandle mh, short item, char *name) {
  /* items are 1-based in legacy Eudora, 0-based in GLib */
  int index = item - 1;
  if (!mh || index < 0 || index >= g_menu_model_get_n_items(mh)) {
    if (name)
      *name = 0;
    return name;
  }

  gchar *label = get_menu_item_label(mh, index);
  if (label) {
    /* Copy to C string if name is provided */
    if (name) {
      size_t len = strlen(label);
      if (len > 255)
        len = 255;
      memcpy(name, label, len);
      name[len] = '\0';
    }
    g_free(label);
  } else if (name) {
    *name = 0;
  }
  return name;
}

short FindItemByName(MenuHandle mh, char *name) {
  if (!mh || !name)
    return 0;

  int count = g_menu_model_get_n_items(mh);
  const char *p_name = name;

  for (int i = 0; i < count; i++) {
    gchar *label = get_menu_item_label(mh, i);
    if (label) {
      if (strcmp(label, p_name) == 0) {
        g_free(label);
        return i + 1; /* Return 1-based index */
      }
      g_free(label);
    }
  }
  return 0;
}

void SetItemStyle(GMenuModel *mh, int item, int style) {
  /* GMenuModel is immutable. To change style, we need the underlying GMenu.
   * For the port, we assume the model is a GMenu if it's being modified.
   */
  int index = item - 1;
  if (!G_IS_MENU(mh) || index < 0 || index >= g_menu_model_get_n_items(mh))
    return;

  GMenu *menu = G_MENU(mh);
  /* We can't easily modify a GMenuItem in-place in a GMenu.
   * We need to recreate the item and replace it.
   */
  GMenuItem *gitem = g_menu_item_new(NULL, NULL);
  /* Copy existing attributes would be better, but for now we'll just set the
   * style */
  gchar *label = get_menu_item_label(mh, index);
  if (label) {
    g_menu_item_set_label(gitem, label);
    g_free(label);
  }

  /* Map legacy Style to GTK attributes */
  /* For unread (italic), we can set a custom attribute or use Pango markup if
   * supported by the renderer */
  if (style & (1 << 1)) { /* fontItalic/UnreadStyle */
    g_menu_item_set_attribute(gitem, "style", "s", "italic");
  } else {
    g_menu_item_set_attribute(gitem, "style", "s", "normal");
  }

  if (style & (1 << 0)) { /* fontBold */
    g_menu_item_set_attribute(gitem, "weight", "s", "bold");
  }

  g_menu_remove(menu, index);
  g_menu_insert_item(menu, index, gitem);
  g_object_unref(gitem);
}

int GetItemStyle(GMenuModel *mh, int item) {
  int index = item - 1;
  if (!mh || index < 0 || index >= g_menu_model_get_n_items(mh))
    return 0;

  int style = 0;
  gchar *s_val = NULL;
  if (g_menu_model_get_item_attribute(mh, index, "style", "s", &s_val)) {
    if (strcmp(s_val, "italic") == 0)
      style |= (1 << 1);
    g_free(s_val);
  }
  if (g_menu_model_get_item_attribute(mh, index, "weight", "s", &s_val)) {
    if (strcmp(s_val, "bold") == 0)
      style |= (1 << 0);
    g_free(s_val);
  }
  return style;
}

bool HasSubmenu(MenuHandle mh, short item) {
  int index = item - 1;
  if (!mh || index < 0 || index >= g_menu_model_get_n_items(mh))
    return false;
  GMenuModel *sub = g_menu_model_get_item_link(mh, index, G_MENU_LINK_SUBMENU);
  if (sub) {
    g_object_unref(sub);
    return true;
  }
  return false;
}

/* Register menu callbacks */
void register_menu_callbacks(MenuCallback callbacks[ACTION_COUNT]) {
  memcpy(g_menu_callbacks, callbacks, sizeof(g_menu_callbacks));
}
