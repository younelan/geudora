/*
 * GTK Menu Definitions for gEudora
 * Ported from Eudora Carbon resource files (Common.r, two.rsrc.r, etc.)
 *
 * This file defines the menu structure for the GTK version of Eudora.
 * Menus are built programmatically using GTK's menu API.
 */

#ifndef GTK_MENUS_H
#define GTK_MENUS_H

#include <gtk/gtk.h>

typedef GMenuModel *MenuHandle;

/* Menu IDs (from original Carbon resources) */
#define MENU_FILE 1502
#define MENU_EDIT 1503
#define MENU_MESSAGE 1505
#define MENU_TRANSFER 1506
#define MENU_MAILBOX 1507
#define MENU_WINDOW 1514
#define MENU_SCRIPTS 1554


/* Menu item action identifiers */
typedef enum {
  /* File menu */
  ACTION_NEW_MESSAGE,
  ACTION_OPEN,
  ACTION_OPEN_SELECTION,
  ACTION_OPEN_IN_BROWSER,
  ACTION_CLOSE,
  ACTION_SAVE,
  ACTION_SAVE_AS,
  ACTION_REVERT,
  ACTION_PAGE_SETUP,
  ACTION_PRINT,
  ACTION_QUIT,

  /* Edit menu */
  ACTION_UNDO,
  ACTION_CUT,
  ACTION_COPY,
  ACTION_PASTE,
  ACTION_PASTE_QUOTATION,
  ACTION_CLEAR,
  ACTION_SELECT_ALL,
  ACTION_WRAP_SELECTION,
  ACTION_FINISH_NICKNAME,
  ACTION_INSERT_RECIPIENT,
  ACTION_INSERT_EMOTICON,
  ACTION_SPEAK,
  ACTION_SPELLING,

  /* Message menu */
  ACTION_NEW_MSG,
  ACTION_REPLY,
  ACTION_REPLY_ALL,
  ACTION_FORWARD,
  ACTION_REDIRECT,
  ACTION_SEND_AGAIN,
  ACTION_QUEUE_FOR_DELIVERY,
  ACTION_DELETE,
  ACTION_MARK_AS_READ,
  ACTION_MARK_AS_UNREAD,
  ACTION_MARK_AS_JUNK,
  ACTION_MARK_AS_NOT_JUNK,
  ACTION_CHANGE_STATUS,
  ACTION_CHANGE_PRIORITY,
  ACTION_CHANGE_LABEL,
  ACTION_CHANGE_PERSONALITY,

  /* Transfer menu */
  ACTION_TRANSFER_IN,
  ACTION_TRANSFER_OUT,

  /* Window menu */
  ACTION_MINIMIZE,
  ACTION_BRING_ALL_TO_FRONT,
  ACTION_SEND_TO_BACK,
  ACTION_TABS,
  ACTION_DRAWER,

  /* Scripts menu */
  ACTION_OPEN_SCRIPTS_FOLDER,

  /* Settings menu */
  ACTION_PREFERENCES,

  ACTION_COUNT
} MenuAction;

/* Menu structure */
typedef struct {
  const char *label;
  const char *accelerator;
  MenuAction action;
  gboolean is_separator;
  gboolean is_submenu;
} MenuItem;

/* Function declarations */
GtkWidget *create_menu_bar(GtkWidget *window);
GtkWidget *create_file_menu(void);
GtkWidget *create_edit_menu(void);
GtkWidget *create_message_menu(void);
GtkWidget *create_transfer_menu(void);
GtkWidget *create_window_menu(void);
GtkWidget *create_scripts_menu(void);

/* Callback function type */
typedef void (*MenuCallback)(GtkWidget *widget, gpointer data);

/* Registry functions */
void register_menu_model(int menu_id, GMenuModel *model);
MenuHandle GetMHandle(short menu_id);
int GetMenuID(GMenuModel *mh);
GMenuModel *ParentMailboxMenu(GMenuModel *mh, short *item);
short SubmenuId(MenuHandle mh, short item);
void DeleteMenuItem(MenuHandle mh, short item);
void EnableItem(MenuHandle mh, short item);
void DisableItem(MenuHandle mh, short item);
void SetItemMark(MenuHandle mh, short item, short mark);
void CalcMenuSize(MenuHandle mh);
bool HasSubmenu(MenuHandle mh, short item);
void FixMenuUnread(MenuHandle mh, int item, bool unread);

/* Mark character constants (Mac compatibility) */
#ifndef checkMark
#define checkMark  0x12
#endif
#ifndef noMark
#define noMark     0x00
#endif

/* Legacy-compatible helper functions for GMenuModel */
int CountMenuItems(GMenuModel *mh);
unsigned char *MyGetItem(MenuHandle mh, short item, unsigned char *name);
short FindItemByName(MenuHandle mh, unsigned char *name);
void SetItemStyle(GMenuModel *mh, int item, int style);
int GetItemStyle(GMenuModel *mh, int item);

/* Register menu callbacks */
void register_menu_callbacks(MenuCallback callbacks[ACTION_COUNT]);

#endif /* GTK_MENUS_H */

/* Mac Menu Manager compatibility APIs (implemented in gtk_menus.c) */
void GetItemCmd(MenuHandle mh, short item, short *cmd);
void GetItemMark(MenuHandle mh, short item, short *mark);
void SetMenuItemText(MenuHandle mh, short item, unsigned char *text);
int  GetMenuItemHierarchicalID(MenuHandle mh, short item, short *outID);
void EnableMenuItems(int enable);
long MenuEvent(void *event);
void RenameItem(short menuId, unsigned char *oldName, unsigned char *newName);
void NukeMenuItemByName(short menuId, unsigned char *itemName);
void NukeMenuItem(MenuHandle mh, short item);
