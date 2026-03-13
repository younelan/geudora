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

/* Prevent legacy_shim.h from providing a conflicting static-inline ZoneSecs;
   util.c contains the real implementation. */
#include "util.h"
#include "Globals.h"
#include "MyRes.h"
#include "StringUtil.h"
#include "fileutil.h"
#include "gtk_menus.h"
#include "schizo.h"
#include "threading.h"
/* Memory manager globals from Globals.h */
extern long MemLastFailed;
extern long LastTotalSpace, LastContigSpace;
/* Forward decl from color.h (which depends on missing mywindow.h) */
typedef struct {
  unsigned short red, green, blue;
} EUDORA_RGBColor;
EUDORA_RGBColor *DarkenColor(EUDORA_RGBColor *color, short percent);
#include "StringDefs.h"
#include "string_table.h"
#include "gtk_prefs.h"

#include <ctype.h>
#define FILE_NUM 41
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/**********************************************************************
 * various useful functions
 **********************************************************************/

typedef struct SoundEntryStruct SoundEntry, *SoundEntryPtr;

struct SoundEntryStruct {
  SoundEntryPtr next;
  unsigned char name[256];
};

#ifdef KERBEROS
#include <krb.h>
#endif

/* AccuAddPtr -> AccuAddPtrVoid mapping is in util.h */

/* Forward-declare Dprintf (defined in shame.c) */
void Dprintf(const char *fmt, ...);

char BitTable[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
bool PasswordFilter(void *dgPtr, void *event, short *item);
void CopyPassword(UPtr password);
int ResetPassword(void);
UHandle PwChars = nil;
void NukeMenuItem(MenuHandle mh, short item);
void CompactTempZone(void);
short FindMenuByName(UPtr name);
UPtr GetRStringLo(PStr theString, int theIndex, PersHandle forPers);
void *TempNewHandleGlue(long size, OSErr *err);
void QueueSound(PStr name);
void PlaySystemSound(PStr name);
void AddSoundsToMenuFrom(MenuHandle mh, short vRef, long dirID);
OSErr FindSystemSound(OSType disk, OSType folder, PStr name, void *spec);

//	Globals

/**********************************************************************
 * initialize all the mac managers
 **********************************************************************/
void MacInitialize(int masterCount, long ensureStack) {}

/**********************************************************************
 * turn a font name into a font id; if the name is not found, use ApplFont
 **********************************************************************/
int GetFontID(UPtr theName) { return 0; }

/**********************************************************************
 * Check or uncheck a font size in the font menu
 **********************************************************************/
void CheckFontSize(int menu, int size, bool check) {}

/**********************************************************************
 * check (or uncheck) a font name in a menu.	If check, also outline sizes
 **********************************************************************/
void CheckFont(int menu, int fontID, bool check) {}

/**********************************************************************
 * find width of largest char in font
 **********************************************************************/
int GetWidth(int fontID, int fontSize) {
  return fontSize; // Dummy heuristic for char width
}

/**********************************************************************
 * find descent font
 **********************************************************************/
int GetDescent(int fontID, int fontSize) { return 0; }

/**********************************************************************
 * find ascent font
 **********************************************************************/
int GetAscent(int fontID, int fontSize) { return fontSize; }

/**********************************************************************
 * find fixed-width-ness of font
 **********************************************************************/
bool IsFixed(int fontID, int fontSize) { return false; }

/**********************************************************************
 * wait for the user to strike a modifier key
 **********************************************************************/
void AwaitKey(void) {}
/**********************************************************************
 * change or add some data to the current resource file
 **********************************************************************/
void ChangePResource(UPtr theData, int theLength, long theType, int theID,
                     UPtr theName) {
  /*
   * does the resource exist and reside in the topmost res file?
   */
  Zap1Resource(theType, (short)theID);

  AddPResource(theData, theLength, theType, theID, theName);
}

/**********************************************************************
 * add some data to the current resource file
 **********************************************************************/
void AddPResource(UPtr theData, int theLength, long theType, int theID,
                  UPtr theName) {
  Handle aHandle;

  /*
   * allocate the handle
   */
  aHandle = NuHandle((long)theLength);
  if (aHandle == nil)
    return;

  /*
   * copy the data
   */
  BMD(theData, *aHandle, (long)theLength);

  /*
   * add it
   */
  AddResource_(aHandle, theType, theID, theName);
}

/************************************************************************
 * ZapResourceLo - get rid of a resource.
 ************************************************************************/
OSErr ZapResourceLo(OSType type, short id, bool one) { return resNotFound; }

/**********************************************************************
 * Atoi - replacement for standard C routine
 **********************************************************************/
long Atoi(UPtr s) {
  long mul = 1;
  long n = 0;

  while (IsWhite(*s))
    s++;
  if (*s == '-') {
    mul = -1;
    s++;
  }
  if (*s == '+')
    s++;
  while (isdigit(*s)) {
    n = n * 10 + (*s - '0');
    s++;
  }
  return (n * mul);
}

/**********************************************************************
 * AToOSType - ASCII to OSType
 **********************************************************************/
OSType AToOSType(UPtr s) {
  long n = 0;
  short i;

  for (i = 0; i < 4; ++i) {
    n <<= 8;
    n += *s++;
  }
  return (n);
}

/**********************************************************************
 * ResourceCpy - copy a resource from one resource file to the other
 **********************************************************************/
int ResourceCpy(short toRef, short fromRef, long type, int id) { return noErr; }

/**********************************************************************
 * DrawTruncString - truncate and draw a string; restores string when done
 **********************************************************************/
void DrawTruncString(UPtr string, int len) {}

/**********************************************************************
 * CalcTrunc - figure out how much of a string we can print to fit
 * in a given width
 **********************************************************************/
int CalcTextTrunc(UPtr string, short length, short width, void *port) {
  return length; // Handled by GTK TextView/Label rendering
}

void WhiteRect(Rect *r) {}

int WannaSave(MyWindowPtr win) { return CANCEL_ITEM; }

#define PASSWORD_OK 0
#define PASSWORD_CANCEL 1

/* GTK4 password dialog — modal, blocks until user enters password or cancels.
 * Matches original Mac PASSWORD_DLOG: entry + "Save password" checkbox. */
typedef struct {
  GMainLoop *loop;
  int result;
  GtkWidget *entry;
  GtkWidget *save_check;
  UPtr word;
  int size;
} PwDialogData;

static void pw_on_ok(GtkWidget *btn, gpointer user_data) {
  (void)btn;
  PwDialogData *d = (PwDialogData *)user_data;
  const char *text = gtk_editable_get_text(GTK_EDITABLE(d->entry));
  if (text && d->word && d->size > 0) {
    int len = strlen(text);
    if (len >= d->size) len = d->size - 1;
    memcpy(d->word, text, len);
    d->word[len] = '\0';
  }
  d->result = PASSWORD_OK;
  g_main_loop_quit(d->loop);
}

static void pw_on_cancel(GtkWidget *btn, gpointer user_data) {
  (void)btn;
  PwDialogData *d = (PwDialogData *)user_data;
  d->result = PASSWORD_CANCEL;
  g_main_loop_quit(d->loop);
}

static gboolean pw_on_close(GtkWindow *win, gpointer user_data) {
  (void)win;
  PwDialogData *d = (PwDialogData *)user_data;
  d->result = PASSWORD_CANCEL;
  g_main_loop_quit(d->loop);
  return TRUE;
}

int GetPassword(PStr personality, PStr userName, PStr serverName, UPtr word,
                int size, short prompt) {
  (void)prompt;

  const char *pers_str = (const char *)personality;
  const char *user_str = (const char *)userName;
  const char *server_str = (const char *)serverName;
  if (!pers_str) pers_str = "";
  if (!user_str) user_str = "";
  if (!server_str) server_str = "";

  GtkWidget *dlg = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Enter Password");
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dlg), 380, -1);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(vbox, 20);
  gtk_widget_set_margin_end(vbox, 20);
  gtk_widget_set_margin_top(vbox, 16);
  gtk_widget_set_margin_bottom(vbox, 16);
  gtk_window_set_child(GTK_WINDOW(dlg), vbox);

  /* Info label: "Password for <personality> (<user> on <server>)" */
  char info[512];
  if (pers_str[0])
    snprintf(info, sizeof(info), "Password for %s\n(%s on %s)", pers_str, user_str, server_str);
  else
    snprintf(info, sizeof(info), "Password for %s on %s", user_str, server_str);
  GtkWidget *label = gtk_label_new(info);
  gtk_label_set_xalign(GTK_LABEL(label), 0);
  gtk_label_set_wrap(GTK_LABEL(label), TRUE);
  gtk_box_append(GTK_BOX(vbox), label);

  /* Password entry */
  GtkWidget *entry = gtk_password_entry_new();
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(entry), TRUE);
  gtk_box_append(GTK_BOX(vbox), entry);

  /* "Save password" checkbox — matches original Mac PASSWORD_SAVE item */
  GtkWidget *save_check = gtk_check_button_new_with_label("Save password");
  gboolean save_pref = prefs_get_bool(PREFS_GROUP_CHECKING_MAIL, "save_password", FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(save_check), save_pref);
  gtk_box_append(GTK_BOX(vbox), save_check);

  /* Buttons */
  GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
  gtk_widget_set_margin_top(btn_box, 8);
  GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
  GtkWidget *ok_btn = gtk_button_new_with_label("OK");
  gtk_widget_add_css_class(ok_btn, "suggested-action");
  gtk_box_append(GTK_BOX(btn_box), cancel_btn);
  gtk_box_append(GTK_BOX(btn_box), ok_btn);
  gtk_box_append(GTK_BOX(vbox), btn_box);

  PwDialogData data = {0};
  data.loop = g_main_loop_new(NULL, FALSE);
  data.result = PASSWORD_CANCEL;
  data.entry = entry;
  data.save_check = save_check;
  data.word = word;
  data.size = size;

  g_signal_connect(ok_btn, "clicked", G_CALLBACK(pw_on_ok), &data);
  g_signal_connect(cancel_btn, "clicked", G_CALLBACK(pw_on_cancel), &data);
  g_signal_connect(dlg, "close-request", G_CALLBACK(pw_on_close), &data);
  g_signal_connect_swapped(entry, "activate", G_CALLBACK(pw_on_ok), &data);

  gtk_window_present(GTK_WINDOW(dlg));
  g_main_loop_run(data.loop);
  g_main_loop_unref(data.loop);

  /* If OK, persist the save_password preference and optionally save the password */
  if (data.result == PASSWORD_OK) {
    gboolean want_save = gtk_check_button_get_active(GTK_CHECK_BUTTON(save_check));
    prefs_set_bool(PREFS_GROUP_CHECKING_MAIL, "save_password", want_save);
    if (want_save && word[0]) {
      /* Save password to INI so it persists across sessions */
      prefs_set_string(PREFS_GROUP_CHECKING_MAIL, "saved_password", (const char *)word);
    } else if (!want_save) {
      /* Clear any previously saved password */
      prefs_set_string(PREFS_GROUP_CHECKING_MAIL, "saved_password", "");
    }
  }

  gtk_window_destroy(GTK_WINDOW(dlg));

  return data.result;
}

bool PasswordFilter(void *dgPtr, void *event, short *item) { return false; }

/************************************************************************
 * GetPassStuff - collect the info needed for the password prompt
 * Ported from Mac: reads from INI prefs, fills C strings
 ************************************************************************/
void GetPassStuff(unsigned char *persName, unsigned char *uName, unsigned char *hName) {
  if (uName) {
    gchar *u = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_username", "");
    strncpy((char *)uName, u, 127);
    ((char *)uName)[127] = '\0';
    g_free(u);
  }
  if (hName) {
    gchar *h = prefs_get_string(PREFS_GROUP_CHECKING_MAIL, "pop_server", "");
    strncpy((char *)hName, h, 127);
    ((char *)hName)[127] = '\0';
    g_free(h);
  }
  if (persName) {
    strncpy((char *)persName, "Dominant", 63);
    ((char *)persName)[63] = '\0';
  }
}

/************************************************************************
 * CopyPassword - retrieve the password
 ************************************************************************/
void CopyPassword(UPtr password) {
  strcpy((char *)password, (const char *)*PwChars);
  HPurge(PwChars);
}

/************************************************************************
 * MyAppendMenu - see that a menu item gets appended to a menu.  Avoids
 * menu manager meta-characters.
 ************************************************************************/
void MyAppendMenu(MenuHandle menu, UPtr name) {}

RGBColor *GetItemColor(short menu, short item, RGBColor *color) {
  return color;
}

/************************************************************************
 * MyInsMenuItem - see that a menu item gets appended to a menu.	Avoids
 * menu manager meta-characters.
 ************************************************************************/
void MyInsMenuItem(MenuHandle menu, UPtr name, short afterItem) {}

void SetItemR(MenuHandle menu, short item, short id) {}

void MySetItem(MenuHandle menu, short item, PStr itemStr) {}

/************************************************************************
 * MyGetItem - get the text of a menu item.  Strip leading NULL, if any
 ************************************************************************/
/* Ported to gtk_menus.c */
#if 0
PStr MyGetItem(MenuHandle menu, short item, PStr name) {
  GetMenuItemText(menu, item, name);
  if (*name && !name[1]) {
    BMD(name + 2, name + 1, name[0] - 1);
    name[0]--;
  }
  return (name);
}
#endif

/************************************************************************
 * CopyMenuItem - copy a menu item from one menu to another
 ************************************************************************/
OSErr CopyMenuItem(MenuHandle fromMenu, short fromItem, MenuHandle toMenu,
                   short toItem) {
  return noErr;
}

/**********************************************************************
 * PStr2Handle - copy a PString to a handle
 **********************************************************************/
void **PStr2Handle(unsigned char *string) { return NULL; }

/************************************************************************
 * SetItemReducedIcon - set a reduced icon for a menu item
 ************************************************************************/
void SetItemReducedIcon(MenuHandle menu, short item, short iconid) {}

/* FindItemByName is implemented in gtk_menus.c */
extern short FindItemByName(MenuHandle menu, UPtr name);

/************************************************************************
 * FindMenuByName - find a named menu item
 ************************************************************************/
short FindMenuByName(UPtr name) { return 0; }

short MyUniqueID(ResType type) { return 0; }

/**********************************************************************
 * OnBatteries - are we running on batteries?
 **********************************************************************/
static bool OnBatteries9(void) { return false; }

//	In Wrappers.cp
extern bool OnBatteriesX(void);

bool OnBatteries(void) {
  static uLong ticks = 0;
  static bool val;

  //	Don't check too often
  if (TickCount() - ticks < 60)
    return val;
  ticks = TickCount();

  return val = HaveOSX() ? OnBatteriesX() : OnBatteries9();
}

/**********************************************************************
 * GestaltBits - return the bits for a Gestalt selector.  Note that this
 * routine cannot distinguish between an error and no bits, so if that's
 * important to you, use Gestalt directly.
 **********************************************************************/
uint32_t GestaltBits(uint32_t selector) { return 0; }

/************************************************************************
 * BinFindItemByName - find a named menu item, using binary search
 ************************************************************************/
short BinFindItemByName(MenuHandle menu, UPtr name) {
  /* Mac Menu searching is not portable; GTK handles menus differently. */
  return 0;
}

void *Event2Window(void *event) { return 0; }

void FixURLString(PStr url) {}

short CurrentModifiers(void) { return 0; }

void AttachHierMenu(short menu, short item, short hierId) {}

bool DirtyKey(long keyAndChar) { return false; }

UPtr LocalDateTimeStr(UPtr string) {
  *string = 0;
  return string;
}

PStr WeekDay(PStr string, long secs) {
  *string = 0;
  return string;
}

/************************************************************************
 * GMTDateTime - return the current seconds
 ************************************************************************/
uLong GMTDateTime(void) { return 0; }

uLong LocalDateTime(void) { return 0; }

PStr LocalDateTimeShortStr(PStr s) {
  *s = 0;
  return s;
}

short MenuWidth(MenuHandle mh) { return 0; }

int MyTrackDrag(DragReference drag, void *event, RgnHandle rgn) { return 0; }

/**********************************************************************
 * HasDragManager - is the pestilent drag manager installed?
 **********************************************************************/
bool HasDragManager(void) {
  long fxxkingGestalt;

  return (!(Gestalt('drag', &fxxkingGestalt) || !(fxxkingGestalt & 1)))
#if TARGET_RT_MAC_CFM
         && ((long)InstallTrackingHandler != kUnresolvedCFragSymbolAddress &&
             !PrefIsSet(PREF_NO_DRAG))
#endif
      ;
}

bool MyWaitMouseMoved(Point pt, bool honorControl) { return false; }

bool MyDragHas(void *drag, short item, OSType type) { return false; }

OSErr MyGetDragItemData(void *drag, short item, OSType type, Handle *data) {
  if (data)
    *data = nil;
  return noErr;
}

OSErr MySetDragItemFlavorData(void *drag, short item, OSType type, void *data,
                              long len) {
  return noErr;
}

short DragOrMods(void *drag) { return 0; }

short MyCountDragItems(void *drag) { return 0; }

short MyCountDragItemFlavors(void *drag, short item) { return 0; }

OSType MyGetDragItemFlavorType(void *drag, short item, short flavor) {
  return 0;
}

FlavorFlags MyGetDragItemFlavorFlags(DragReference drag, short item,
                                     short flavor) {
  return 0;
}

void MiniEventsLo(short mask, bool background) {}

#ifdef DEBUG
/**********************************************************************
 * Rude - be rude to memory
 **********************************************************************/
void Rude(void) {}
#endif
/************************************************************************
 * PlayNamedSound - play a sound with a given name
 ************************************************************************/
void PlayNamedSound(PStr name) {}

void PlaySystemSound(PStr name) {}

/************************************************************************
 * FindSystemSound - look in a particular location for a sound
 ************************************************************************/
OSErr FindSystemSound(OSType disk, OSType folder, PStr name, void *spec) {
  return fnfErr;
}

void PlaySoundIdle(void) {}

/************************************************************************
 * QueueSound - queue this system sound to play it later
 ************************************************************************/
void QueueSound(PStr name) {}

/************************************************************************
 * PlaySoundId - play a sound with a given resource id
 ************************************************************************/
void PlaySoundId(short id) {}

/************************************************************************
 * AddSoundsToMenu - add sound names to menu
 ************************************************************************/
void AddSoundsToMenu(MenuHandle mh) {}

void AddSoundsToMenuFrom(MenuHandle mh, short vRef, long dirID) {}

extern char checkKey;
/************************************************************************
 * MyMenuKey - fix MenuKey to ignore option key
 ************************************************************************/
long MyMenuKeyLo(void *event, bool enable) { return 0; }

/************************************************************************
 * AFPopUpMenuSelect - pop up menu in current font
 ************************************************************************/
long AFPopUpMenuSelect(MenuHandle mh, short top, short left, short item) {
  return 0;
}

/************************************************************************
 * RecountStrn - make sure an STR# resource really has the right number
 * of strings
 *
 * returns True if the resource had to be changed
 ************************************************************************/
bool RecountStrn(short resId) {
  /* GTK port: STR# resources not used; strings come from string_table_lookup */
  return false;
}

/************************************************************************
 * CountStrn - count the strings an STR# resource says it has
 ************************************************************************/
short CountStrn(short resId) {
  /* GTK port: string arrays not loaded this way */
  return 0;
}

/************************************************************************
 *
 ************************************************************************/
void NukeMenuItemByName(short menuId, UPtr itemName) {}

void NukeMenuItem(MenuHandle mh, short item) {}

/************************************************************************
 *
 ************************************************************************/
void RenameItem(short menuId, UPtr oldName, UPtr newName) {}

/************************************************************************
 *
 ************************************************************************/
/* HasSubmenu and SubmenuId are implemented in gtk_menus.c */
extern bool HasSubmenu(MenuHandle mh, short item);
extern short SubmenuId(MenuHandle mh, short item);
/************************************************************************
 * SetGreyControl - grey a control, if it isn't already
 ************************************************************************/
bool SetGreyControl(ControlHandle cntl, bool shdBeGrey) { return shdBeGrey; }

/************************************************************************
 * IsAUX - is A/UX running?
 ************************************************************************/
bool IsAUX(void) { return false; }

/************************************************************************
 * ZoneSecs - get the timezone offset, in seconds
 ************************************************************************/
long ZoneSecs(void) {
  /* Use standard C time APIs for timezone offset */
  time_t t = time(NULL);
  struct tm *local = localtime(&t);
  struct tm *gmt = gmtime(&t);
  long delta = (local->tm_hour - gmt->tm_hour) * 3600 +
               (local->tm_min - gmt->tm_min) * 60;
  return delta;
}

/**********************************************************************
 * AddMyResource - add resource, but set Eudora Settings, too
 **********************************************************************/
void AddMyResource(Handle h, OSType type, short id, ConstStr255Param name) {
  if (SettingsRefN)
    UseResFile(SettingsRefN);
  AddResource(h, type, id, name);
}

/************************************************************************
 * Provide the same benefit as a politician
 ************************************************************************/
void NOOP(void) {}

/************************************************************************
 * RoundDiv - Divide with rounding away from the origin
 ************************************************************************/
long RoundDiv(long quantity, long unit) {
  if (quantity < 0)
    quantity -= unit - 1;
  else
    quantity += unit - 1;
  return (quantity / unit);
}

/************************************************************************
 * TZName2Offset - interpret the time zone with a resource
 ************************************************************************/
long TZName2Offset(CStr zoneName) {
  /* GTK Port: system tzset/timezone should be used instead of 'zon#' res */
  return 0;
}

/************************************************************************
 * CenterRectIn - center one rect in another
 ************************************************************************/
void CenterRectIn(Rect *inner, Rect *outer) {
  OffsetRect(inner,
             (outer->left + outer->right - inner->left - inner->right) / 2,
             (outer->top + outer->bottom - inner->top - inner->bottom) / 2);
}

/************************************************************************
 * TopCenterRectIn - center one rect in (the bottom of) another
 ************************************************************************/
void TopCenterRectIn(Rect *inner, Rect *outer) {
  OffsetRect(inner,
             (outer->left + outer->right - inner->left - inner->right) / 2,
             outer->top - inner->top);
}

/************************************************************************
 * BottomCenterRectIn - center one rect in (the bottom of) another
 ************************************************************************/
void BottomCenterRectIn(Rect *inner, Rect *outer) {
  OffsetRect(inner,
             (outer->left + outer->right - inner->left - inner->right) / 2,
             outer->bottom - inner->bottom);
}

/************************************************************************
 * ThirdCenterRectIn - center one rect in (the top 1/3 of) another
 ************************************************************************/
void ThirdCenterRectIn(void *inner_, void *outer_) {
  Rect *inner = (Rect *)inner_;
  Rect *outer = (Rect *)outer_;
  OffsetRect(inner,
             (outer->left + outer->right - inner->left - inner->right) / 2,
             outer->top - inner->top +
                 (outer->bottom - outer->top - inner->bottom + inner->top) / 3);
}

/**********************************************************************
 * IsEnabled - is a menu item enabled
 **********************************************************************/
bool IsEnabled(short menu, short item) {
  MenuHandle mh = GetMHandle(menu);

  if (!mh)
    return (False);
  if (!IsMenuItemEnabled(mh, 0))
    return false;
  return IsMenuItemEnabled(mh, item);
}

/**********************************************************************
 *
 **********************************************************************/
void ShowDragRectHilite(DragReference drag, Rect *r, bool inside) {
  RgnHandle rgn = NewRgn();
  if (rgn) {
    RectRgn(rgn, r);
    ShowDragHilite(drag, rgn, inside);
    DisposeRgn(rgn);
  }
}

/**********************************************************************
 * CheckNone - make sure no items are marked in a menu
 **********************************************************************/
void CheckNone(MenuHandle mh) {
  short i;

  if (mh)
    for (i = CountMenuItems(mh); i; i--)
      if (!HasSubmenu(mh, i))
        SetItemMark(mh, i, noMark);
}

/**********************************************************************
 * ButtonFit - shrink button to min possibe size
 **********************************************************************/
void ButtonFit(ControlHandle button) {
  /* In the GTK port, button sizing is managed by GTK layout managers.
   * The Mac-specific hPitch/vPitch metrics and Control Manager calls
   * (GetBestControlRect, SizeControl, etc.) have no direct equivalent.
   * GTK handles preferred sizes via gtk_widget_set_size_request. */
  if (button) {
    GtkWidget *w = (GtkWidget *)button;
    gtk_widget_queue_resize(w);
  }
}

/**********************************************************************
 * return a string from an STR# resource
 **********************************************************************/
PStr GetRString(PStr theString, short theIndex) {
  SCPtr start, end;
  uLong ticks = TickCount();
  uLong oldest = ticks;
  short oldSpot;
  long n;
  unsigned char sizeStr[64];
  uLong curPersId = CurThreadGlobals ? (CurPers ? CurPers->persId : 0) : 0;
  bool dontReadCache = NoDominant || !StringCache || NoProxify;
  bool dontWriteCache =
      NoDominant || NoProxify || GrowZoning || (EjectBuckaroo && !StringCache);
  bool replaceOld = false;

  /*
   * find it in the cache
   */
  if (!dontReadCache) {
    n = HandleCount(StringCache);
    start = *StringCache;
    end = start + n;
    for (; start < end; start++) {
      if (theIndex == start->id && start->persId == curPersId) {
        g_strlcpy((char *)theString, (const char *)start->string, 256);
        start->used = ticks;
        return (ProxifyStr(theString, theIndex));
      } else if (oldest) {
        if (!start->id) {
          oldest = 0;
          oldSpot = start - *StringCache;
          replaceOld = true;
        } else if (oldest > start->used) {
          oldest = start->used;
          oldSpot = start - *StringCache;
          replaceOld = true;
        }
      }
    }
  }

  /*
   * not in the cache.  Grab it.
   */
  GetRStringLo(theString, theIndex, CurPers);

  /*
   * create cache
   */
  if (!StringCache && !dontWriteCache) {
    GetRStringLo(sizeStr, STRING_CACHE, nil);
    StringToNum(sizeStr, &n);
    StringCache = (SCHandle)NuHandleClear(n * sizeof(StringCacheEntry));
    oldSpot = 0;
  }

  /*
   * cache string
   */
  if (StringCache && !dontWriteCache &&
      replaceOld)
  {
    g_strlcpy((char *)(*StringCache)[oldSpot].string, (const char *)theString, 256);
    (*StringCache)[oldSpot].id = theIndex;
    (*StringCache)[oldSpot].used = ticks;
    (*StringCache)[oldSpot].persId = curPersId;
  }

  return (ProxifyStr(theString, theIndex));
}

/**********************************************************************
 * SCClear - clear the string cache
 **********************************************************************/
void SCClear(short theId) {
  short i;
  short n;

  if (StringCache) {
    n = HandleCount(StringCache);
    for (i = 0; i < n; i++)
      if (theId == -1 || (*StringCache)[i].id == theId) {
        (*StringCache)[i].id = 0;
        // break;
      }
  }
}

/**********************************************************************
 * return a string from an STR# resource
 **********************************************************************/
PStr GetRStringLo(PStr theString, int theIndex, PersHandle forPers) {
  /* GTK Port: look up from compiled-in string table - now returns C string */
  theString[0] = '\0';
  if (!NoDominant || CurPers == PersList) {
    const char *s = string_table_lookup((uint16_t)theIndex);
    if (s) {
      g_strlcpy((char *)theString, s, 256);
    }
  }
  return (theString);
}

/************************************************************************
 * FindSTRNIndex - find a string in a resource id
 ************************************************************************/
short FindSTRNIndex(short resId, PStr string) {
  /* GTK Port: Strings stored differently */
  return 0;
}

/**********************************************************************
 * FindSTRNIndexRes - Find a string in an STR# resource
 **********************************************************************/
short FindSTRNIndexRes(UHandle resource, PStr string) {
  /* GTK Port: Strings stored differently */
  return 0;
}

/************************************************************************
 * FindSTRNSubIndex - find a substring in a resource id
 ************************************************************************/
short FindSTRNSubIndex(short resId, PStr string) {
  /* GTK Port: Strings stored differently */
  return 0;
}

/**********************************************************************
 * FindSTRNSubIndexRes - Find a substring in an STR# resource
 **********************************************************************/
short FindSTRNSubIndexRes(UHandle resource, PStr string) {
  /* GTK Port: Strings stored differently */
  return 0;
}

/**********************************************************************
 * CountStrnRes - count strings in an STR# resource, given the resource
 **********************************************************************/
short CountStrnRes(UHandle resH) {
  /* GTK Port: Strings stored differently */
  return (0);
}

/**********************************************************************
 * Get a color out of a resource file, and darken for text
 **********************************************************************/
RGBColor *GetRTextColor(RGBColor *color, int index) {
  return (RGBColor *)DarkenColor((EUDORA_RGBColor *)GetRColor(color, index),
                                 GetRLong(TEXT_DARKER));
}

/**********************************************************************
 * Get a color out of a resource file
 **********************************************************************/
RGBColor *GetRColor(RGBColor *color, int index) {
  /* GTK Port: Colors stored differently */
  Zero(*color);
  return (nil);
}

/**********************************************************************
 * Color2String - convert a color to a string
 **********************************************************************/
PStr Color2String(PStr string, RGBColor *color) {
  ComposeString(string, (const unsigned char *)"%d,%d,%d", color->red, color->green, color->blue);
  return (string);
}

/**********************************************************************
 * Get a long out of a resource file
 **********************************************************************/
long GetRLong(int index) {
  unsigned char scratch[256];
  long aLong;

  if (GetRString(scratch, index) == nil)
    return (0L);
  else {
    StringToNum(scratch, &aLong);
    return (aLong);
  }
}

/**********************************************************************
 * Get an OSType out of a resource file
 **********************************************************************/
OSType GetROSType(int index) {
  unsigned char scratch[256];
  long len;
  OSType theType;
  char *src;
  char *dst;
  long i;

  if (GetRString(scratch, index) == nil)
    return ((OSType)0);
  len = strlen((const char *)scratch);
  if (len > 4)
    len = 4;
  else if (len < 4) {
    for (i = len, dst = ((char *)&theType) + 3; i < 4; i++)
      *dst-- = ' ';
  }
  for (i = 0, src = (char *)(void *)scratch, dst = (char *)(void *)&theType;
       i < len; i++)
    *dst++ = *src++;
  return theType;
}

/************************************************************************
 * RemoveChar - remove a char from some text
 ************************************************************************/
long RemoveChar(Byte c, UPtr text, long size) {
  UPtr from, to, limit;

  for (to = text, limit = text + size; to < limit && *to != c; to++)
    ;
  if (to < limit)
    for (from = to; from < limit; from++)
      if (*from != c)
        *to++ = *from;
  return (to - text);
}

/************************************************************************
 * RemoveCharHandle - remove a character from a handle
 ************************************************************************/
long RemoveCharHandle(Byte c, UHandle text) {
  long len = GetHandleSize((Handle)text);
  long newLen = RemoveChar(c, LDRef(text), len);

  UL(text);
  if (newLen < len)
    SetHandleBig((Handle)text, newLen);
  return (newLen);
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AddLf(Handle text) {
  UPtr spot, end;
  long len = GetHandleSize_(text);
  long newLen = len;

  end = *text + len;
  for (spot = *text; spot < end; spot++)
    if (*spot == '\015')
      newLen++;

  if (newLen > len) {
    SetHandleBig((Handle)text, newLen);
    if (MemError())
      return (MemError());
    end = *text + newLen;
    for (spot = *text + len - 1; spot > (UPtr)*text; spot--) {
      if (*spot == '\015')
        *--end = '\012';
      *--end = *spot;
    }
  }
  return (noErr);
}

/************************************************************************
 * GetRStr - get a string from an 'STR ' resource
 ************************************************************************/
UPtr GetRStr(UPtr string, short id) {
  /* GTK port: map to application resources */
  *string = 0;
  return (string);
}

/**********************************************************************
 * CountChars - count characters in a handle
 **********************************************************************/
long CountChars(Handle text, Byte c) {
  long count = CountCharsPtr(LDRef(text), GetHandleSize(text), c);
  UL(text);
  return (count);
}

/**********************************************************************
 * CountCharsPtr - count characters in a pointer
 **********************************************************************/
long CountCharsPtr(UPtr ptr, long size, Byte c) {
  short n __attribute__((unused)) = 0;
  UPtr end = ptr + size;

  while (ptr < end)
    if (*ptr++ == c)
      n++;

  return (n);
}

/**********************************************************************
 * HandleLineBreaks - figure out linebreaks in a block of text
 **********************************************************************/
OSErr HandleLinebreaks(Handle text, long ***breaks, short inWidth) {
  Fixed width;
  short n __attribute__((unused)) = 0;
  UPtr ptr;
  long len;
  long textOffset;
  long cum = 0;
  OSErr err;

  if ((*breaks = (long **)NuHTempBetter(0)) == nil)
    return (MemError());
  if (!text || !(len = GetHandleSize(text)))
    return (noErr);

  ptr = LDRef(text);
  while (len) {
    width = inWidth << 16;
    textOffset = 1;
    StyledLineBreak((const char *)ptr, len, 0, len, 0, &width, &textOffset);
    len -= textOffset;
    ptr += textOffset;
    cum += textOffset;
    if ((err = (OSErr)(intptr_t)PtrPlusHand_(&cum, *breaks, sizeof(cum))))
      return (err);
  }
  UL(text);

  return (noErr);
}

/************************************************************************
 * TransLitString - transliaterate with the default viewing table
 ************************************************************************/
void TransLitString(UPtr string) {
  /* GTK Port: Translation tables (taBL) not used directly */
}

/************************************************************************
 * TransLitRes - translit, fetching table from a resource
 ************************************************************************/
void TransLitRes(UPtr string, long len, short resId) {
  /* GTK Port: Translation tables (taBL) not used directly */
}

/************************************************************************
 * TransLit - transliterate some chars
 ************************************************************************/
void TransLit(UPtr string, long len, UPtr table) {
  /* GTK Port: Translation tables (taBL) not used directly */
}

/**********************************************************************
 * ScriptVar - return a script variable
 **********************************************************************/
long ScriptVar(short selector) {
  long result = GetScriptVariable(0, selector);

  // apple won't tell us what the small system font is yet
  if (!result && selector == smScriptSmallSysFondSize)
    result = 0x0001000A;

  return (result);
}

#define StackSpot(s, n)                                                        \
  ((UPtr) * s + sizeof(StackType_Util) + (n) * (*stack)->elSize)

/**********************************************************************
 * StackInit - initialize a stack
 *  First four bytes are the element size
 *  Second four bytes are the # of valid elements in the stack
 **********************************************************************/
OSErr StackInit(long size, StackHandle *stack) {
  *stack = (StackHandle)NuHTempBetter(sizeof(StackType_Util));
  if (!*stack)
    return (MemError());
  (**stack)->elSize = size;
  (**stack)->elCount = 0;
  return (noErr);
}

/**********************************************************************
 * StackPush - push an item onto a stack
 **********************************************************************/
OSErr StackPush(void *what, StackHandle stack) {
  if (!stack)
    return fnfErr;
  else {
    short nSpace =
        (GetHandleSize_(stack) - sizeof(StackType_Util)) / (*stack)->elSize;

    ASSERT(nSpace >= (*stack)->elCount);
    if (nSpace == (*stack)->elCount) {
      SetHandleBig_(stack, GetHandleSize_(stack) + 20 * (*stack)->elSize);
      if (MemError())
        return (MemError());
    }
    BMD(what, StackSpot(stack, (*stack)->elCount), (*stack)->elSize);
    (*stack)->elCount++;
    return (noErr);
  }
}

/**********************************************************************
 * StackQueue - queue an item onto a bottom of stack
 **********************************************************************/
OSErr StackQueue(void *what, StackHandle stack) {
  if (!stack)
    return fnfErr;
  else {
    short nSpace =
        (GetHandleSize_(stack) - sizeof(StackType_Util)) / (*stack)->elSize;

    ASSERT(nSpace >= (*stack)->elCount);
    if (nSpace == (*stack)->elCount) {
      SetHandleBig_(stack, GetHandleSize_(stack) + 20 * (*stack)->elSize);
      if (MemError())
        return (MemError());
    }
    BMD(StackSpot(stack, 0), StackSpot(stack, 1),
        (*stack)->elCount * (*stack)->elSize);
    BMD(what, StackSpot(stack, 0), (*stack)->elSize);
    (*stack)->elCount++;
    return (noErr);
  }
}

/**********************************************************************
 * StackPop - pop an item off a stack
 **********************************************************************/
OSErr StackPop(void *into, StackHandle stack) {
  if (!stack || !(*stack)->elCount)
    return (fnfErr);
  (*stack)->elCount--;
  if (into)
    BMD(StackSpot(stack, (*stack)->elCount), into, (*stack)->elSize);
  return (noErr);
}

/**********************************************************************
 * StackTop - fetch top stack item
 **********************************************************************/
OSErr StackTop(void *into, StackHandle stack) {
  if (!stack || !(*stack)->elCount)
    return (fnfErr);
  if (into)
    BMD(StackSpot(stack, (*stack)->elCount - 1), into, (*stack)->elSize);
  return (noErr);
}

/**********************************************************************
 * StackItem - fetch a stack item
 **********************************************************************/
OSErr StackItem(void *into, short item, StackHandle stack) {
  if (!stack || !(*stack)->elCount || item >= (*stack)->elCount)
    return (fnfErr);
  if (into)
    BMD(StackSpot(stack, item), into, (*stack)->elSize);
  return (noErr);
}

/**********************************************************************
 * StackCompact - get rid of waste space
 **********************************************************************/
void StackCompact(StackHandle stack) {
  if (!stack)
    return;
  SetHandleBig_(stack,
                (*stack)->elCount * (*stack)->elSize + sizeof(StackType_Util));
}

/**********************************************************************
 * StackStringFind - find an item in the stack
 **********************************************************************/
short StackStringFind(PStr find, StackHandle stack) {
  unsigned char s[256];
  short item;

  if (!stack)
    return -1;

  for (item = (*stack)->elCount; item > 0;) {
    item--;
    StackItem(s, item, stack);
    if (StringSame((const char *)s, (const char *)find))
      return item;
  }

  return -1;
}

#define AAElemSize(aa) ((*(aa))->dataSize + (*(aa))->keySize)
#define AAKeySpot(aa, indx)                                                    \
  ((UPtr)(*(aa)) + sizeof(AssocArray) + ((indx) - 1) * AAElemSize(aa))
#define AADataSpot(aa, indx) (AAKeySpot(aa, indx) + (*(aa))->keySize)
/************************************************************************
 * AANew - create an associative array
 ************************************************************************/
AAHandle AANew(short keySize, short dataSize) {
  AAHandle aa = NewZHTB(AssocArray);
  if (aa) {
    (*aa)->keySize = keySize;
    (*aa)->dataSize = dataSize;
  }
  return (aa);
}

/************************************************************************
 * AAAddItem - add an item to an associative array
 ************************************************************************/
OSErr AAAddItem(AAHandle aa, bool replace, PStr key, UPtr data) {
  short count = AACountItems(aa);
  short spot = AAFindKey(aa, key);
  unsigned char lwrKey[256];

  if (spot > 0) {
    /* already exists */
    if (!replace)
      return (1);
  } else {
    spot *= -1; /* spot now is the index of the item just after us */
    SetHandleBig_(aa, GetHandleSize_(aa) + AAElemSize(aa));
    if (MemError())
      return (MemError());
    if (spot && spot <= count) /* move old data */
      BMD(AAKeySpot(aa, spot), AAKeySpot(aa, spot) + AAElemSize(aa),
          AAElemSize(aa) * (count - spot + 1));
  }
  BMD(data, AADataSpot(aa, spot), (*aa)->dataSize);
  PCopy(lwrKey, key);
  MyLowerStr(lwrKey);
  BMD(lwrKey, AAKeySpot(aa, spot), (*aa)->keySize);
  return (noErr);
}

/************************************************************************
 * AAAddResItem - add an item to an associative array, using a resource for
 *a key
 ************************************************************************/
OSErr AAAddResItem(AAHandle aa, bool replace, short keyId, UPtr data) {
  unsigned char key[256];

  GetRString(key, keyId);
  return (AAAddItem(aa, replace, key, data));
}

/************************************************************************
 * AADeleteKey - delete an item by key
 ************************************************************************/
OSErr AADeleteKey(AAHandle aa, PStr key) {
  short spot = AAFindKey(aa, key);
  if (spot > 0) {
    short count = AACountItems(aa);
    if (spot < count)
      BMD(AAKeySpot(aa, spot + 1), AAKeySpot(aa, spot),
          AAElemSize(aa) * (count - spot));
    SetHandleBig_(aa, GetHandleSize_(aa) - AAElemSize(aa));
    return (noErr);
  } else
    return (1); /* not found */
}

/************************************************************************
 * AAFetchData - fetch data from an assoc array, by key
 ************************************************************************/
OSErr AAFetchData(AAHandle aa, PStr key, UPtr data) {
  short spot = AAFindKey(aa, key);
  if (spot > 0) {
    BMD(AADataSpot(aa, spot), data, (*aa)->dataSize);
    return (noErr);
  }
  return (1); /* not found */
}

/************************************************************************
 * AAFetchResData - fetch data from an assoc array, by a resource key
 ************************************************************************/
OSErr AAFetchResData(AAHandle aa, short keyId, UPtr data) {
  unsigned char key[256];

  return (AAFetchData(aa, GetRString(key, keyId), data));
}

/************************************************************************
 * AAFetchIndData - fetch data from an assoc array, by index
 ************************************************************************/
OSErr AAFetchIndData(AAHandle aa, short index, UPtr data) {
  BMD(AADataSpot(aa, index), data, (*aa)->dataSize);
  return (noErr);
}

/************************************************************************
 * AAFetchIndKey - fetch key from an assoc array, by index
 ************************************************************************/
OSErr AAFetchIndKey(AAHandle aa, short index, PStr key) {
  BMD(AAKeySpot(aa, index), key, (*aa)->keySize);
  return (noErr);
}

/************************************************************************
 * AAFindKey - find a key in an associative array
 *  returns:
 *		positive: index of found item
 *    negative: index of smallest item > current item
 ************************************************************************/
short AAFindKey(AAHandle aa, PStr key) {
  short count = AACountItems(aa);
  short first = 1;
  short last = count;
  short mid;
  short greater = count + 1;
  short result;
  unsigned char lwrKey[256];

  PCopy(lwrKey, key);
  MyLowerStr(lwrKey);

  LDRef(aa);

  while (first <= last) {
    mid = (first + last) / 2;
    result = StringComp(lwrKey, AAKeySpot(aa, mid));
    if (result == 0) {
      greater = -mid; /* found it! */
      break;
    } else if (result < 0) {
      greater = mid;
      last = mid - 1;
    } else
      first = mid + 1;
  }

  UL(aa);
  return (-greater);
}

/************************************************************************
 * AACountItems - count the items in an associative array
 ************************************************************************/
short AACountItems(AAHandle aa) {
  if (!aa || !*aa)
    return (-1);
  return ((GetHandleSize_(aa) - sizeof(AssocArray)) /
          ((*aa)->keySize + (*aa)->dataSize));
}

/**********************************************************************
 * AccuInit - initialize an accumulator
 **********************************************************************/
OSErr AccuInit(AccuPtr a) {
  a->offset = 0;
  a->size = 1 K;
  a->data = NuHTempBetter(a->size);
  return (a->err = MemError());
}

/**********************************************************************
 * AccuInitWithHandle - make an accumulator out of an existing handle
 **********************************************************************/
void AccuInitWithHandle(AccuPtr a, Handle h) {
  a->size = a->offset = GetHandleSize(h);
  a->data = h;
}

/************************************************************************
 * AccuWrite - write an accumulator to a file
 ************************************************************************/
OSErr AccuWrite(AccuPtr a, short refN) {
  long count = a->offset;
  OSErr err;

  if (!count)
    return (noErr);
  err = AWrite(refN, &count, LDRef(a->data));
  UL(a->data);
  return (err);
}

/************************************************************************
 * AccuFTell - tell the position of the file pointer, assuming that the
 *bytes in the accumulator had been written
 ************************************************************************/
long AccuFTell(AccuPtr a, short refN) {
  long spot;

  GetFPos(refN, &spot);
  return (spot + a->offset);
}

/************************************************************************
 * AccuFSeek - move the pointer back to a given spot
 ************************************************************************/
OSErr AccuFSeek(AccuPtr a, short refN, long spot) {
  long curSpot = AccuFTell(a, refN);
  if (curSpot - spot <= a->offset) {
    a->offset -= curSpot - spot;
    return (0);
  } else {
    a->offset = 0;
    return (SetFPos(refN, fsFromStart, spot));
  }
}

/**********************************************************************
 *
 **********************************************************************/
void AccuTrim(AccuPtr a) {
  OSErr err;

  if (!a->data && (err = AccuInit(a)))
    return;

#ifdef DEBUG
  if (RunType != Production) {
    if (a->size != GetHandleSize(a->data) || a->size < a->offset) {
      Dprintf("o %d s %d hs %d h %x", a->offset, a->size,
              GetHandleSize(a->data), a->data);
    }
  }
#endif

  SetHandleBig(a->data, a->offset);
  a->size = a->offset;
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuAddChar(AccuPtr a, Byte c) { return (AccuAddPtr(a, &c, 1)); }

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuAddLong(AccuPtr a, uLong longVal) {
  return (AccuAddPtr(a, &longVal, sizeof(longVal)));
}

/**********************************************************************
 * AccuAddPtr64 - add some data, base-64 encoded
 **********************************************************************/
OSErr AccuAddPtrB64(AccuPtr a, void *bytes, long len) {
  long newLen = len * 4 / 3 + 4;
  Handle encoded = NuHandle(newLen);
  OSErr err;

  if (!encoded)
    return MemError();

  Encode64DataPtr(LDRef(encoded), &newLen, bytes, len);
  ASSERT(newLen <= len * 4 / 3 + 4);

  err = AccuAddPtr(a, *encoded, newLen);

  ZapHandle(encoded);

  return err;
}

/**********************************************************************
 * AccuAddTrPtr - add to an accumulator, but translate first
 **********************************************************************/
OSErr AccuAddTrPtr(AccuPtr a, void *bytes, long len, UPtr from, UPtr to) {
  OSErr err;

  // translate
  TrLo(bytes, len, from, to);

  // add
  err = AccuAddPtr(a, bytes, len);

  // untranslate
  TrLo(bytes, len, to, from);

  return err;
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuAddPtr(AccuPtr a, void *bytes, long len) {
  OSErr err;

  if (!a->data && (err = AccuInit(a)))
    return (err);

#ifdef DEBUG
  if (RunType != Production) {
    if (a->size != GetHandleSize(a->data) || a->size < a->offset) {
      Dprintf("o %d s %d hs %d h %x", a->offset, a->size,
              GetHandleSize(a->data), a->data);
    }
  }
#endif

  if (a->offset + len > a->size) {
    a->size += len + 4 K;
    SetHandleBig_(a->data, a->size);
    if (MemError())
      return (a->err = MemError());
  }
  BMD(bytes, *a->data + a->offset, len);
  a->offset += len;
  return (noErr);
}

/**********************************************************************
 * AccuAddSortedLong - add a long to an accumulator, but keep it sorted
 **********************************************************************/
OSErr AccuAddSortedLong(AccuPtr a, long addVal) {
  OSErr err = AccuAddPtr(a, &addVal, sizeof(addVal));

  if (!err) {
    long *start = *a->data;                             // start of data
    long *spot = *a->data + a->offset - sizeof(addVal); // spot we added addVal
    long *newSpot = spot - 1; // spot addVal really belongs

    // if the value before us is bigger than we are, we'll need to move
    if (newSpot >= start && *newSpot > addVal) {
      // keep going until the value we're looking at is
      // not greater than the value we're putting in
      while (newSpot >= start && *newSpot > addVal)
        newSpot--;

      // Note: if we wanted to eliminate duplicates, here is the spot
      // if (newSpot>=start && *newSpot==addVal)
      // {
      // 	 a->offset -= sizeof(addVal);
      //	 return noErr;
      // }

      // newSpot now points one BEFORE the proper spot; increment
      newSpot++;

      // move everything above us to make room
      BMD(newSpot, newSpot + 1, sizeof(addVal) * (spot - newSpot));

      // and put us into place
      *newSpot = addVal;
    }
  }

  return err;
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuAddRes(AccuPtr a, short res) {
  unsigned char s[256];

  GetRString(s, res);
  return (AccuAddStr(a, s));
}

/**********************************************************************
 * AccuAddHandleToPtr - copy some data into a handle and add the handle to
 *the accumulator
 **********************************************************************/
OSErr AccuAddHandleToPtr(AccuPtr a, UPtr data, long size) {
  Handle h = NuDHTempBetter(data, size);
  OSErr err;
  if (h) {
    err = AccuAddPtr(a, (void *)&h, sizeof(h));
    if (err)
      ZapHandle(h);
  } else
    err = MemError();
  return (err);
}

/**********************************************************************
 * AccuAddTrHandle - add a translated handle
 **********************************************************************/
OSErr AccuAddTrHandle(AccuPtr a, Handle data, UPtr from, UPtr to) {
  OSErr err;

  Tr(data, from, to);
  err = AccuAddHandle(a, data);
  Tr(data, to, from);
  return err;
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuAddHandle(AccuPtr a, Handle data) {
  OSErr err;
  long len;

  if (!a->data && (err = AccuInit(a)))
    return (err);

  ASSERT(data);
  if (!data)
    return noErr;

#ifdef DEBUG
  if (RunType != Production) {
    if (a->size != GetHandleSize(a->data) || a->size < a->offset) {
      Dprintf("o %d s %d hs %d h %x", a->offset, a->size,
              GetHandleSize(a->data), a->data);
    }
  }
#endif

  len = GetHandleSize(data);

  if (a->offset + len > a->size) {
    a->size += len + 4 K;
    SetHandleBig_(a->data, a->size);
    if (MemError())
      return (a->err = MemError());
  }
  BMD(*data, *a->data + a->offset, len);
  a->offset += len;
  return (noErr);
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuAddFromHandle(AccuPtr a, Handle data, long offset, long len) {
  OSErr err;

  if (!a->data && (err = AccuInit(a)))
    return (err);

  if (len < 0)
    len = GetHandleSize(data);

  if (len < 0)
    return len;

#ifdef DEBUG
  if (RunType != Production) {
    if (a->size != GetHandleSize(a->data) || a->size < a->offset) {
      Dprintf("o %d s %d hs %d h %x", a->offset, a->size,
              GetHandleSize(a->data), a->data);
    }
  }
#endif

  if (a->offset + len > a->size) {
    a->size += len + 4 K;
    SetHandleBig_(a->data, a->size);
    if (MemError())
      return (a->err = MemError());
  }
  BMD(*data + offset, *a->data + a->offset, len);
  a->offset += len;
  return (noErr);
}

/************************************************************************
 * AccuFindPtr - find a pointer in an accumulator
 ************************************************************************/
long AccuFindPtr(AccuPtr a, UPtr stuff, short len) {
  UPtr spot;
  UPtr end;

  if (!a->data || !len)
    return -1;

  spot = *a->data;
  end = spot + a->offset - len;

  for (; spot <= end; spot += len)
    if (!memcmp(spot, stuff, len))
      return spot - (UPtr)*a->data;

  return -1;
}

/************************************************************************
 * AccuFindLong - find a long in an accumulator; returns an index
 ************************************************************************/
long AccuFindLong(AccuPtr a, uLong theLong) {
  uLong *spot;
  uLong *end;

  if (!a->data)
    return -1;

  spot = (uLong *)*a->data;
  end = (uLong *)spot + (a->offset - sizeof(uLong)) / sizeof(uLong);

  for (; spot <= end; spot++)
    if (*spot == theLong)
      return spot - (uLong *)*a->data;

  return -1;
}

/************************************************************************
 * DecodeB64Accu - decode a base64 accumulator
 ************************************************************************/
short DecodeB64Accu(AccuPtr a, bool isText) {
  Dec64 d64;
  Handle data = NuHandle((3 * a->offset) / 4 + 4);
  long len;
  long result;

  if (!data)
    return MemError();
  if (!a->offset)
    return noErr;

  Zero(d64);
  result = Decode64(LDRef(a->data), a->offset, LDRef(data), &len, &d64, isText);
  if ((d64.decoderState + d64.padCount) % 4)
    result++;

  if (!result) {
    a->offset = len;
    BMD(*data, *a->data, len);
  }

  ZapHandle(data);
  UL(a->data);

  return (result);
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuInsertChar(AccuPtr a, Byte c, long offset) {
  return (AccuInsertPtr(a, &c, 1, offset));
}

/**********************************************************************
 *
 **********************************************************************/
OSErr AccuInsertPtr(AccuPtr a, UPtr bytes, long len, long offset) {
  OSErr err;

  if (!a->data && (err = AccuInit(a)))
    return (err);

#ifdef DEBUG
  if (RunType != Production) {
    if (a->size != GetHandleSize(a->data) || a->size < a->offset) {
      Dprintf("o %d s %d hs %d h %x", a->offset, a->size,
              GetHandleSize(a->data), a->data);
    }
  }
#endif

  if (a->offset + len > a->size) {
    a->size += len + 4 K;
    SetHandleBig_(a->data, a->size);
    if (MemError())
      return (a->err = MemError());
  }
  BMD(*a->data + offset, *a->data + offset + len, a->offset - offset);
  BMD(bytes, *a->data + offset, len);
  a->offset += len;
  return (noErr);
}

/**********************************************************************
 * Strip 'n' bytes off of the end of an accumulator
 **********************************************************************/
OSErr AccuStrip(AccuPtr a, long num)

{
  OSErr theError;

  if (!a->data && (theError = AccuInit(a)))
    return (theError);

  a->offset = a->offset > num ? a->offset - num : 0;
  return (noErr);
}

/************************************************************************
 * Long2Hex - write a long in 8 hex bytes
 ************************************************************************/
PStr Long2Hex(PStr hex, long aLong) {
  Bytes2Hex((void *)&aLong, sizeof(aLong), (void *)hex);
  hex[8] = '\0';
  return (hex);
}

char *hexdig = "0123456789ABCDEF";
/************************************************************************
 * Bytes2Hex - encode some bytes in hex
 ************************************************************************/
UPtr Bytes2Hex(UPtr bytes, long size, UPtr hex) {
  Byte c;
  UPtr spot = hex;

  while (size--) {
    c = *bytes++;
    *spot++ = hexdig[(c >> 4) & 0xf];
    *spot++ = hexdig[c & 0xf];
  }

  return (hex);
}

/************************************************************************
 * IsHexDig - is a char a hex digit?
 ************************************************************************/
bool IsHexDig(Byte c) {
  if (islower(c))
    c = toupper(c);
  return (('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));
}

#define Hex2Nyb(c)                                                             \
  (c <= '9' ? c - '0' : (c >= 'a' ? c - 'a' + 10 : c - 'A' + 10))
/************************************************************************
 * Hex2Bytes - decode some bytes from hex
 ************************************************************************/
OSErr Hex2Bytes(UPtr hex, long size, UPtr bytes) {
  Byte hi, lo;

  while (size >= 2) {
    hi = Hex2Nyb(*hex);
    hex++;
    lo = Hex2Nyb(*hex);
    hex++;
    *bytes++ = (hi << 4) | lo;
    size -= 2;
  }
  return (noErr);
}

/************************************************************************
 * MyOSEventAvail - OSEventAvail with resource chain protection
 ************************************************************************/
#undef OSEventAvail
bool MyOSEventAvail(short mask, void *event) {
  EventRecord *event_rec = (EventRecord *)event;
  bool result;
  MightSwitch();
  result = EventAvail(mask, event_rec);
  AfterSwitch();
  return (result);
}
#define OSEventAvail MyOSEventAvail

/************************************************************************
 * SafeToAllocate - is it safe to allocate this much memory?
 ************************************************************************/
bool SafeToAllocate(long size) {
  static uLong allocated;

  if (size > 4 K || allocated + 50 K > LastContigSpace) {
    allocated = 0;
    PurgeSpace(&LastTotalSpace, &LastContigSpace);
  }
  allocated += size;
  if (LastContigSpace && size + 1 K K > LastContigSpace)
    return (False);
  if (!MemLastFailed)
    return (True);
  if (size > MemLastFailed)
    return (False);
  return (True);
}

/************************************************************************
 * NuHTempOK - New Handle, prefer my heap but temp mem ok
 ************************************************************************/
void *NuHTempOK(long size) {
  Handle h;
  RANDOM_FAILURE;
  if (!SafeToAllocate(size) || !(h = NuHandle(size)))
    h = NuHTempBetter(size);
  return (h);
}

/**********************************************************************
 * NuDHTempBetter - allocate a handle and copy data into it
 **********************************************************************/
void *NuDHTempBetter(void *data, long size) {
  Handle h;
  RANDOM_FAILURE;

  h = NuHTempBetter(size);
  if (h)
    BMD(data, *h, size);
  return (h);
}

/**********************************************************************
 * NuDHTempOK - allocate a handle and copy data into it
 **********************************************************************/
void *NuDHTempOK(void *data, long size) {
  Handle h;

  RANDOM_FAILURE;

  h = NuHTempOK(size);
  if (h)
    BMD(data, *h, size);
  return (h);
}

/************************************************************************
 * NuHTempBetter - New Handle, prefer temp mem
 ************************************************************************/
void *NuHTempBetter(long size) {
  Handle theMem;
  OSErr err;

  RANDOM_FAILURE;
#ifdef DEBUG
  if (BUG9)
    return (NuHandle(size));
#endif

  if (size > 1 K)
    CompactTempZone();
  theMem = TempNewHandleGlue(size, &err);
  if (!theMem)
    theMem = NuHandle(size);
  else if (size > 1 K)
    MoveHHi(theMem);
  return (theMem);
}

/************************************************************************
 * UpdateMDI -
 ************************************************************************/
void UpdateMDI(short resId, long type) {
  /* GTK port: UpdateMDI logic stubbed */
}

/**********************************************************************
 * ZeroHandle - clear the contents of a handle
 **********************************************************************/
void *TempNewHandleGlue(long size, OSErr *err) {
  return (TempNewHandle(size, err));
}

/**********************************************************************
 * ZeroHandle - clear the contents of a handle
 **********************************************************************/
void *ZeroHandle(void *hand) {
  Handle h = hand;
  long len;

  if (h && *h) {
    len = GetHandleSize(h);
    WriteZero(LDRef(h), len);
    UL(h);
  }
  return (h);
}

/************************************************************************
 * CompactTempZone - compact the temp memory zone
 ************************************************************************/
void CompactTempZone(void) {}

/************************************************************************
 * NewIOBHandle - New IO buffer, Handle
 ************************************************************************/
Handle NewIOBHandle(long min, long max) {
  Handle theMem;

  CompactTempZone();
  do {
    theMem = NuHTempOK(max);
    max /= 2;
  } while (!theMem && max >= min);
  if (theMem)
    MoveHHi(theMem);

  return (theMem);
}

/************************************************************************
 * GetTableCName - get the canonical name of a table
 ************************************************************************/
bool GetTableCName(short tid, PStr name) {
  /* GTK port: euTM resources not used */
  name[0] = '\0';
  return false;
}

/************************************************************************
 * GetTableID - get the id of a named table
 ************************************************************************/
bool GetTableID(PStr name, short *tid) {
  /* GTK port: euTM resources not used */
  *tid = 0;
  return false;
}

/************************************************************************
 * EventPending - is an event waiting for us?
 ************************************************************************/
bool EventPending(void) {
  EventRecord event;
  static uLong ticks;

  if (TickCount() - ticks <= 8)
    return (False);
  else {
    ticks = TickCount();
    return (OSEventAvail(mUpMask | mDownMask | keyDownMask | updateMask |
                             activMask | osMask,
                         &event));
  }
}
#ifdef DEBUG
#undef UseResFile
void MyUseResFile(short refN) {
  FSSpec oldSpec, newSpec;
  short oldRefN;

  if (BUG8) {
    oldRefN = CurResFile();
    if (oldRefN != refN) {
      Zero(oldSpec);
      Zero(newSpec);
      GetFileByRef(refN, &newSpec);
      GetFileByRef(oldRefN, &oldSpec);
      Dprintf("UseResFile �%p� -> �%p�;g", oldSpec.name, newSpec.name);
    }
  }
  UseResFile(refN);
}

/************************************************************************
 * RESCHK - verify that a resource is a resource
 ************************************************************************/
void RESCHK(OSType type, short resId);
void RESCHK(OSType type, short resId) {
  Handle resH = GetResource_(type, resId);
  short flags;

  if (resH) {
    flags = HGetState(resH);
    ASSERT(flags & 32);
  }
}
#endif

#define gestaltGatewayExternal 'VIGE'
/************************************************************************
 * IsVICOM - return true if VICOM Internet gateway is running
 ************************************************************************/
bool IsVICOM(void) {
  long interfaceVICOM = 0;

  if (Gestalt(gestaltGatewayExternal, &interfaceVICOM) == noErr)
    return (interfaceVICOM != 0);
  else
    return (false);
}

/************************************************************************
 * MyRemoveResource - fix bug in RemoveResource:
 *   it doesn't work if CurResFile is not set to the resource's resource file
 ************************************************************************/
#undef RemoveResource
OSErr MyRemoveResource(Handle h) {
  short useFile, saveFile = CurResFile();
  OSErr err = noErr;

  if (h) {
    useFile = HomeResFile(h);
    if (!(err = ResError())) {
      UseResFile(useFile);
      RemoveResource(h);
      err = ResError();
      UseResFile(saveFile);
    }
  }
  return err;
}

/************************************************************************
 * FinderDragVoodoo - hack around a Finder 8.1 (at least) bug
 ************************************************************************/
OSErr FinderDragVoodoo(DragReference drag) {
  // This magic incantation seems to avoid a bug in (at least) the 8.1
  // finder which seems to sometimes go boom if the first drag it sees is
  // non-TEXT, promised, sender-only, and not saved.
  return (AddDragItemFlavor(drag, 1L, 'xyzy', "", 0, flavorNotSaved));
}

/**********************************************************************
 * ShortCompare - compare two shorts, return 0 if equal, -1 if value1 <
 *value2, 1 if value1 2 value2
 **********************************************************************/
short ShortCompare(short value1, short value2) {
  if (value1 == value2)
    return 0;
  if (value1 < value2)
    return -1;
  return 1;
}

/************************************************************************
 * DateCompare - compare two dates
 *		return 0 if equal, -1 if date1 < date2, 1 if date1 2 date2
 *  Doesn't compare time portion of DateTimeRec
 ************************************************************************/
short DateCompare(DateTimeRec *date1, DateTimeRec *date2) {
  short result;

  if (!(result = ShortCompare(date1->year, date2->year)))
    if (!(result = ShortCompare(date1->month, date2->month)))
      result = ShortCompare(date1->day, date2->day);
  return result;
}

/************************************************************************
 * TimeCompare - compare two dates including time
 *		return 0 if equal, -1 if date1 < date2, 1 if date1 2 date2
 ************************************************************************/
short TimeCompare(DateTimeRec *date1, DateTimeRec *date2) {
  short result;

  if (!(result = DateCompare(date1, date2)))
    if (!(result = ShortCompare(date1->hour, date2->hour)))
      if (!(result = ShortCompare(date1->minute, date2->minute)))
        result = ShortCompare(date1->second, date2->second);
  return result;
}

//#define IsColorWin(win) \
//	(ThereIsColor && \
//	 (((GrafPtr)(win))->portBits.rowBytes & 0xC000) && \
//   ((**((CGrafPtr)(win))->portPixMap).pixelSize > 1))
Boolean IsColorWin(WindowPtr winWP)

{
  return gdk_display_get_default() != NULL;
}

/************************************************************************
 * GetOSVersion - get OS version in BCD format: 9.01 = 0x0901
 ************************************************************************/
short GetOSVersion(void) {
  static short sysVers;
  long result;

  if (!sysVers) {
    if (!Gestalt(gestaltSystemVersion, &result))
      sysVers = result;
  }
  return (sysVers);
}

/************************************************************************
 * HaveOSX - are we running Mac OS X or bettter
 ************************************************************************/
bool HaveOSX(void) { return GetOSVersion() >= 0x1000; }

/* Removed Mac OS 9 Speed Doubler NoSLGet blocks */

/**********************************************************************
 * TimeString - format seconds-since-epoch as a time string
 *
 * Mac original: used DateTimeUtils toolbox to format time according
 * to the current locale/script settings.
 *
 * GTK port: uses strftime with the locale's preferred time format.
 * The 'wantSeconds' parameter selects HH:MM:SS vs HH:MM.
 **********************************************************************/
void TimeString(long secs, bool wantSeconds, unsigned char *str, void *intlHandle) {
  (void)intlHandle; /* Mac intl resource handle — not used on POSIX */
  if (!str) return;
  str[0] = '\0';

  time_t t = (time_t)secs;
  struct tm *tm = localtime(&t);
  if (tm)
    strftime((char *)str, 64, wantSeconds ? "%H:%M:%S" : "%H:%M", tm);
}
