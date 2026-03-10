/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to
the limitations in the disclaimer below) provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE. */

#include "utl.h"
#include <string.h>
#include <stdlib.h>
#include <pango/pango.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define FILE_NUM 42

/* GTK 4.20 removed gdk_display_get_primary_monitor, gdk_display_get_n_monitors,
   and gdk_display_get_monitor. Use the GListModel API instead. */
static inline GdkMonitor *_get_primary_monitor(GdkDisplay *d) {
    GListModel *ml = gdk_display_get_monitors(d);
    guint n = g_list_model_get_n_items(ml);
    return n > 0 ? GDK_MONITOR(g_list_model_get_object(ml, 0)) : NULL;
}
static inline guint _get_n_monitors(GdkDisplay *d) {
    return g_list_model_get_n_items(gdk_display_get_monitors(d));
}
static inline GdkMonitor *_get_monitor(GdkDisplay *d, guint i) {
    return GDK_MONITOR(g_list_model_get_object(gdk_display_get_monitors(d), i));
}
/*______________________________________________________________________

	 utl.c - Utilities.

	 Copyright 1988, 1989, 1990 Northwestern University.  Permission is granted
	 to use this code in your own projects, provided you give credit to both
	 John Norstad and Northwestern University in your about box or document.

	 This module exports miscellaneous reusable utility routines.
_____________________________________________________________________*/




/*______________________________________________________________________

	 Global Variables.
_____________________________________________________________________*/


static GdkCursor   **CursArray;     /* ptr to array of cursor pointers */
static short         NumCurs;       /* number of cursors to rotate */
static short         TickInterval;  /* number of ticks between rotations */
static short         CurCurs;       /* index of current cursor */
static gint64        LastTick;      /* microsecond timestamp at last rotation */

/*______________________________________________________________________

	 utl_CenterDlogRect - Center a dialog rectangle.

	 Entry: 	rect = rectangle.
					centerMain = true to center on main (menu bar) screen.
					centerMain = false to center on the screen containing
						 the maximum intersection with the frontmost window.

	 Exit:		rect = rectangle offset so that it is centered on
						 the specified screen, with twice as much space below
						 the rect as above.
_____________________________________________________________________*/


void utl_CenterDlogRect(GdkRectangle *rect, bool centerMain)
{
    GdkRectangle screenRect;
    GdkDisplay  *display;
    GdkMonitor  *monitor;
    int          nmonitors;
    int          dx, dy;

    display = gdk_display_get_default();

    if (centerMain) {
        monitor = _get_primary_monitor(display);
        gdk_monitor_get_geometry(monitor, &screenRect);
    } else {
        nmonitors = _get_n_monitors(display);
        monitor = NULL;
        if (nmonitors > 0) {
            long maxArea = 0;
            int i;
            for (i = 0; i < nmonitors; i++) {
                GdkRectangle mRect;
                GdkMonitor *m = _get_monitor(display, i);
                gdk_monitor_get_geometry(m, &mRect);
                GdkRectangle intersection;
                if (gdk_rectangle_intersect(rect, &mRect, &intersection)) {
                    long area = (long)intersection.width * (long)intersection.height;
                    if (area > maxArea) {
                        maxArea = area;
                        monitor = m;
                        screenRect = mRect;
                    }
                }
            }
            if (!monitor) {
                monitor = _get_primary_monitor(display);
                gdk_monitor_get_geometry(monitor, &screenRect);
            }
        } else {
            monitor = _get_primary_monitor(display);
            gdk_monitor_get_geometry(monitor, &screenRect);
        }
    }

    dx = (screenRect.x + screenRect.x + screenRect.width  - rect->x - rect->x - rect->width)  / 2 - rect->x;
    dy = (screenRect.y + screenRect.y + screenRect.height - rect->y - rect->y - rect->height + 7) / 3 - rect->y;
    rect->x += dx;
    rect->y += dy;
}

/*______________________________________________________________________

	 utl_CenterRect - Center a rectangle on the main screen.

	 Entry: 	rect = rectangle.

	 Exit:		rect = rectangle offset so that it is centered on
						 the main screen.
_____________________________________________________________________*/


void utl_CenterRect(GdkRectangle *rect)
{
    GdkDisplay   *display;
    GdkMonitor   *monitor;
    GdkRectangle  screenRect;
    int           dx, dy;

    display = gdk_display_get_default();
    monitor = _get_primary_monitor(display);
    gdk_monitor_get_geometry(monitor, &screenRect);

    dx = screenRect.x + (screenRect.width  - rect->width)  / 2 - rect->x;
    dy = screenRect.y + (screenRect.height - rect->height) / 2 - rect->y;
    rect->x += dx;
    rect->y += dy;
}

/*______________________________________________________________________

	 utl_CheckPack - Check to see if a package exists.

	 Exit:		function result = false (no Mac packages on GTK).
_____________________________________________________________________*/


bool utl_CheckPack(short packNum, bool preload)
{
    (void)packNum;
    (void)preload;
    return false;
}

/*______________________________________________________________________

	 utl_CopyPString - Copy Pascal String.

	 Entry: 	dest = destination C string buffer.
					source = source C string.
_____________________________________________________________________*/


void utl_CopyPString(char *dest, char *source)
{
    if (!dest || !source) return;
    strncpy(dest, source, 255);
    dest[255] = '\0';
}

/*______________________________________________________________________

	 utl_CouldDrag - Determine if a window could be dragged to a location.

	 Entry: 	theWindow = pointer to GTK window widget.
					windRect = window rectangle, in global coords.
					offset = pixel offset used in drag calls.
					titleBarHeight = height of the title bar in pixels.
					leftRimWidth = width of the left window rim in pixels.

	 Exit:		function result = true if the window could have been
						 dragged to the specified position.
_____________________________________________________________________*/


bool utl_CouldDrag(GtkWidget *theWindow, GdkRectangle *windRect, short offset,
                   short titleBarHeight, short leftRimWidth)
{
    GdkDisplay   *display;
    int           nmonitors, i;
    int           corner;
    bool          could = false;

    (void)theWindow;
    (void)leftRimWidth;

    display = gdk_display_get_default();
    nmonitors = _get_n_monitors(display);

    offset += 3;

    for (corner = 1; corner <= 4; corner++) {
        GdkRectangle r;
        switch (corner) {
            case 1:
                r.y = windRect->y - titleBarHeight;
                r.x = windRect->x;
                break;
            case 2:
                r.y = windRect->y - offset;
                r.x = windRect->x;
                break;
            case 3:
                r.y = windRect->y - titleBarHeight;
                r.x = windRect->x + windRect->width - offset;
                break;
            default:
                r.y = windRect->y - offset;
                r.x = windRect->x + windRect->width - offset;
                break;
        }
        r.width  = offset;
        r.height = offset;

        for (i = 0; i < nmonitors; i++) {
            GdkMonitor   *mon = _get_monitor(display, i);
            GdkRectangle  mRect;
            GdkRectangle  isect;
            gdk_monitor_get_geometry(mon, &mRect);
            if (gdk_rectangle_intersect(&r, &mRect, &isect)) {
                if (isect.width == r.width && isect.height == r.height) {
                    could = true;
                    break;
                }
            }
        }
        if (could) break;
    }
    return could;
}

/*______________________________________________________________________

	 utl_FixStdFile - Fix Standard File Package.

	 Not applicable on GTK.
_____________________________________________________________________*/


void utl_FixStdFile(void)
{
}

/*______________________________________________________________________

	 utl_FlashButton - Flash Dialog Button.

	 Entry: 	theDialog = pointer to dialog widget.
					itemNo = item number of button to flash (1-based child index).
_____________________________________________________________________*/


void utl_FlashButton(GtkWidget *theDialog, short itemNo)
{
    GtkWidget *child;
    GtkWidget *button = NULL;
    int        idx = 1;

    if (!theDialog) return;

    if (GTK_IS_DIALOG(theDialog)) {
        button = gtk_dialog_get_widget_for_response(GTK_DIALOG(theDialog), itemNo);
        if (!button) {
            GtkWidget *area = gtk_widget_get_first_child(theDialog);
            child = area ? gtk_widget_get_first_child(area) : NULL;
            for (; child != NULL; child = gtk_widget_get_next_sibling(child), idx++) {
                if (idx == itemNo) {
                    button = child;
                    break;
                }
            }
        }
    } else {
        child = gtk_widget_get_first_child(theDialog);
        for (; child != NULL; child = gtk_widget_get_next_sibling(child), idx++) {
            if (idx == itemNo) {
                button = child;
                break;
            }
        }
    }

    if (button) {
        gtk_widget_grab_focus(button);
    }
}

/*______________________________________________________________________

	 utl_FrameItem - Draw a frame around a dialog item.

	 Entry: 	theDialog = pointer to dialog window widget.
					itemNo = dialog item number (1-based child index).
_____________________________________________________________________*/


void utl_FrameItem(GtkWidget *theDialog, short itemNo)
{
    GtkWidget    *child;
    GtkWidget    *item = NULL;
    int           idx  = 1;

    if (!theDialog) return;

    if (GTK_IS_DIALOG(theDialog)) {
        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(theDialog));
        child = gtk_widget_get_first_child(content);
    } else {
        child = gtk_widget_get_first_child(theDialog);
    }

    for (; child != NULL; child = gtk_widget_get_next_sibling(child), idx++) {
        if (idx == itemNo) {
            item = child;
            break;
        }
    }

    if (!item) return;

    {
        GtkCssProvider *prov = gtk_css_provider_new();
        gtk_css_provider_load_from_string(prov, "* { border: 2px solid black; }");
        gtk_style_context_add_provider_for_display(gtk_widget_get_display(item),
                                                   GTK_STYLE_PROVIDER(prov),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(prov);
    }
}

/*______________________________________________________________________

	 utl_GetApplVol - Get the application volume reference number.

	 Exit:		function result = 0 (no Mac volumes on GTK).
_____________________________________________________________________*/


short utl_GetApplVol(void)
{
    return 0;
}

/*______________________________________________________________________

	 utl_GetFontNumber - Get Font Number.

	 Entry: 	fontName = font name (C string).

	 Exit:		function result = true if font description is valid.
					fontNum = set to 0 (Pango does not use numeric font IDs).
_____________________________________________________________________*/


bool utl_GetFontNumber(char *fontName, short *fontNum)
{
    PangoFontDescription *desc;

    if (!fontName || !fontNum) return false;

    desc = pango_font_description_from_string(fontName);
    if (!desc) {
        *fontNum = 0;
        return false;
    }
    *fontNum = 0;
    pango_font_description_free(desc);
    return true;
}

/*______________________________________________________________________

	 utl_GetMBarHeight - Get Menu Bar Height.

	 Exit:		function result = 0 (no Mac menu bar on GTK).
_____________________________________________________________________*/


short utl_GetMBarHeight(void)
{
    return 0;
}

/*______________________________________________________________________

	 utl_GetNewControl - Get New Control.

	 Entry: 	controlID = identifier (unused in GTK resource-less model).
					theWindow = pointer to parent window widget.

	 Exit:		function result = a new GtkButton widget added to the window.
_____________________________________________________________________*/


GtkWidget *utl_GetNewControl(short controlID, GtkWidget *theWindow)
{
    GtkWidget *button;

    (void)controlID;
    (void)theWindow;

    button = gtk_button_new();
    return button;
}

/*______________________________________________________________________

	 utl_GetNewDialog - Get New Dialog.

	 Entry: 	dialogID = identifier (unused).
					dStorage = unused storage pointer.
					behind = unused z-order hint.

	 Exit:		function result = a new GtkDialog widget.
_____________________________________________________________________*/


GtkWidget *utl_GetNewDialog(short dialogID, void *dStorage, GtkWidget *behind)
{
    GtkWidget *dialog;

    (void)dialogID;
    (void)dStorage;
    (void)behind;

    dialog = gtk_dialog_new();
    return dialog;
}

/*______________________________________________________________________

	 utl_GetNewWindow - Get New Window.

	 Entry: 	windowID = identifier (unused).
					wStorage = unused storage pointer.
					behind = unused z-order hint.

	 Exit:		function result = a new GtkWindow widget.
_____________________________________________________________________*/


GtkWidget *utl_GetNewWindow(short windowID, void *wStorage, GtkWidget *behind)
{
    GtkWidget *window;

    (void)windowID;
    (void)wStorage;
    (void)behind;

    window = gtk_window_new();
    return window;
}

/*______________________________________________________________________

	 utl_GetSysVol - Get the system volume reference number.

	 Exit:		function result = 0 (no Mac volumes on GTK).
_____________________________________________________________________*/


short utl_GetSysVol(void)
{
    return 0;
}

/*______________________________________________________________________

	 utl_GetWindGD - Get the GdkMonitor containing a window.

	 Entry: 	theWindow = pointer to GTK window widget.

	 Exit:		gd = handle to GdkMonitor containing the most window area.
					screenRect = bounding rectangle of that monitor.
					windRect = content rectangle of window, in screen coords.
					hasMB = true (GTK windows are always on a monitor).
_____________________________________________________________________*/


void utl_GetWindGD(GtkWidget *theWindow, GdkMonitor **gd,
                   GdkRectangle *screenRect, GdkRectangle *windRect,
                   bool *hasMB)
{
    GdkSurface   *gdkWin;
    GdkDisplay   *display;
    int           x, y, w, h;

    if (windRect) {
        x = y = w = h = 0;
        if (theWindow) {
            /* GTK4: compositor manages position; use widget size */
            x = 0;
            y = 0;
            w = gtk_widget_get_width(theWindow);
            h = gtk_widget_get_height(theWindow);
        }
        windRect->x      = x;
        windRect->y      = y;
        windRect->width  = w;
        windRect->height = h;
    }

    display = gdk_display_get_default();
    gdkWin  = theWindow ? gtk_native_get_surface(gtk_widget_get_native(theWindow)) : NULL;

    if (gdkWin) {
        GdkMonitor *mon = gdk_display_get_monitor_at_surface(display, gdkWin);
        if (mon) {
            if (gd) *gd = mon;
            if (screenRect) gdk_monitor_get_geometry(mon, screenRect);
            if (hasMB) *hasMB = true;
            return;
        }
    }

    {
        GdkMonitor *primary = _get_primary_monitor(display);
        if (gd) *gd = primary;
        if (screenRect) gdk_monitor_get_geometry(primary, screenRect);
        if (hasMB) *hasMB = true;
    }
}

/*______________________________________________________________________

	 utl_GetRectGDStuff - Get the GdkMonitor with the most overlap with a rect.

	 Entry: 	windRect = pointer to the rectangle (modified temporarily).
					titleBarHeight, leftRimWidth = decoration offsets.

	 Exit:		gd, screenRect, hasMB filled in.
_____________________________________________________________________*/


void utl_GetRectGDStuff(GdkMonitor **gd, GdkRectangle *screenRect,
                        GdkRectangle *windRect, short titleBarHeight,
                        short leftRimWidth, bool *hasMB)
{
    GdkDisplay   *display;
    int           nmonitors, i;
    GdkMonitor   *dominant = NULL;
    GdkRectangle  testRect;
    long          maxArea = 0;

    display   = gdk_display_get_default();
    nmonitors = _get_n_monitors(display);

    testRect        = *windRect;
    testRect.y     -= titleBarHeight;
    testRect.x     -= leftRimWidth;
    testRect.width += leftRimWidth;
    testRect.height += titleBarHeight;

    for (i = 0; i < nmonitors; i++) {
        GdkMonitor   *mon = _get_monitor(display, i);
        GdkRectangle  mRect;
        GdkRectangle  isect;
        gdk_monitor_get_geometry(mon, &mRect);
        if (gdk_rectangle_intersect(&testRect, &mRect, &isect)) {
            long area = (long)isect.width * (long)isect.height;
            if (area > maxArea) {
                maxArea  = area;
                dominant = mon;
            }
        }
    }

    if (!dominant) dominant = _get_primary_monitor(display);

    if (gd)         *gd = dominant;
    if (screenRect) gdk_monitor_get_geometry(dominant, screenRect);
    if (hasMB)      *hasMB = true;
}

/*______________________________________________________________________

	 utl_GetRectGD - Get the GdkMonitor containing a rect.

	 Entry: 	windRect = pointer to the rectangle.

	 Exit:		gd = handle to GdkMonitor with maximum intersection.
_____________________________________________________________________*/


void utl_GetRectGD(GdkRectangle *windRect, GdkMonitor **gd)
{
    GdkDisplay   *display;
    int           nmonitors, i;
    GdkMonitor   *dominant = NULL;
    long          maxArea  = 0;

    display   = gdk_display_get_default();
    nmonitors = _get_n_monitors(display);

    for (i = 0; i < nmonitors; i++) {
        GdkMonitor   *mon = _get_monitor(display, i);
        GdkRectangle  mRect;
        GdkRectangle  isect;
        gdk_monitor_get_geometry(mon, &mRect);
        if (gdk_rectangle_intersect(windRect, &mRect, &isect)) {
            long area = (long)isect.width * (long)isect.height;
            if (area > maxArea) {
                maxArea  = area;
                dominant = mon;
            }
        }
    }

    if (gd) *gd = dominant;
}

/*______________________________________________________________________

	 utl_GetIndGD - Get the nth GdkMonitor.

	 Entry: 	n = device index (1-based).

	 Exit:		gd = pointer to GdkMonitor, or NULL if out of range.
					screenRect = bounding rectangle of that monitor.
					hasMB = true.
_____________________________________________________________________*/


void utl_GetIndGD(short n, GdkMonitor **gd, GdkRectangle *screenRect,
                  bool *hasMB)
{
    GdkDisplay *display;
    int         nmonitors;
    GdkMonitor *mon = NULL;

    display   = gdk_display_get_default();
    nmonitors = _get_n_monitors(display);

    if (n >= 1 && n <= nmonitors) {
        mon = _get_monitor(display, n - 1);
    }

    if (!mon) {
        mon = _get_primary_monitor(display);
    }

    if (gd)         *gd = mon;
    if (screenRect) gdk_monitor_get_geometry(mon, screenRect);
    if (hasMB)      *hasMB = true;
}

/*______________________________________________________________________

	 utl_GetVolFilCnt - Get the number of files on a volume.

	 Exit:		function result = 0 (no Mac volumes on GTK).
_____________________________________________________________________*/


long utl_GetVolFilCnt(short volRefNum)
{
    (void)volRefNum;
    return 0;
}

/*______________________________________________________________________

	 utl_HaveColor - Determine if system has color display.

	 Exit:		function result = true (GTK always has color).
_____________________________________________________________________*/


bool utl_HaveColor(void)
{
    return true;
}

/*______________________________________________________________________

	 utl_InitSpinCursor - Initialize animated cursor.

	 Entry: 	cursArray = array of GdkCursor pointers.
					numCurs = number of cursors to rotate.
					tickInterval = interval between cursor rotations (in ticks, 1/60 s).
_____________________________________________________________________*/


void utl_InitSpinCursor(GdkCursor **cursArray, short numCurs,
                        short tickInterval)
{
    CursArray    = cursArray;
    CurCurs      = 0;
    NumCurs      = numCurs;
    TickInterval = tickInterval;
    LastTick     = g_get_monotonic_time();
}

/*______________________________________________________________________

	 utl_InvalGrow - Invalidate Grow Icon (resize grip area).

	 Entry: 	theWindow = pointer to GTK window widget.
_____________________________________________________________________*/


void utl_InvalGrow(GtkWidget *theWindow)
{
    if (!theWindow) return;

    gtk_widget_queue_draw(theWindow);
}

/*______________________________________________________________________

	 utl_IsDAWindow - Check to see if a window is a DA window.

	 Exit:		function result = false (no Mac DAs on GTK).
_____________________________________________________________________*/


bool utl_IsDAWindow(GtkWidget *theWindow)
{
    (void)theWindow;
    return false;
}

/*______________________________________________________________________

	 utl_PlugParams - Plug parameters into message.

	 Entry: 	line1 = input C string.
					p0, p1, p2, p3 = replacement parameters (C strings).

	 Exit:		line2 = output C string with ^0..^3 substituted.
_____________________________________________________________________*/


void utl_PlugParams(char *line1, char *line2, char *p0, char *p1,
                    char *p2, char *p3)
{
    const char *in;
    char       *out;
    const char *outEnd;
    const char *param;
    size_t      len;

    if (!line1 || !line2) return;

    in     = line1;
    out    = line2;
    outEnd = line2 + 4095;

    while (*in && out < outEnd) {
        if (*in == '^') {
            in++;
            if (!*in) break;
            switch (*in++) {
                case '0': param = p0; break;
                case '1': param = p1; break;
                case '2': param = p2; break;
                case '3': param = p3; break;
                default:  param = NULL; break;
            }
            if (!param) continue;
            len = strlen(param);
            if ((size_t)(outEnd - out) < len) len = (size_t)(outEnd - out);
            memcpy(out, param, len);
            out += len;
        } else {
            *out++ = *in++;
        }
    }
    *out = '\0';
}

/*______________________________________________________________________

	 utl_RestoreWindowPos - Restore Window Position.

	 Entry: 	theWindow = pointer to GTK window widget.
					userState = saved position/size rectangle.
					zoomed = true if window was in zoomed state when saved.
					offset = pixel offset used in drag calls.
					computeStdState = callback to compute standard (zoom) state.
					computeDefState = callback to compute default state.

	 Exit:		window position and size restored.
					userState = new user state.
_____________________________________________________________________*/


void utl_RestoreWindowPos(GtkWidget *theWindow, GdkRectangle *userState,
                          bool zoomed, short offset,
                          short titleBarHeight, short leftRimWidth,
                          utl_ComputeStdStatePtr computeStdState,
                          utl_ComputeDefStatePtr computeDefState)
{
    if (!theWindow || !userState) return;

    if (titleBarHeight > 0 &&
        !utl_CouldDrag(theWindow, userState, offset, titleBarHeight, leftRimWidth)) {
        GdkRectangle defState = *userState;
        int savedW = userState->width;
        int savedH = userState->height;
        (*computeDefState)(theWindow, &defState);
        if (!zoomed) {
            GdkRectangle testRect;
            testRect.x      = defState.x - leftRimWidth - 1;
            testRect.y      = defState.y - titleBarHeight - 1;
            testRect.width  = savedW + 2;
            testRect.height = savedH + titleBarHeight + 2;
            GdkDisplay   *display   = gdk_display_get_default();
            int           nmonitors = _get_n_monitors(display);
            bool          fits      = false;
            int           i;
            for (i = 0; i < nmonitors; i++) {
                GdkMonitor   *mon = _get_monitor(display, i);
                GdkRectangle  mRect;
                GdkRectangle  isect;
                gdk_monitor_get_geometry(mon, &mRect);
                if (gdk_rectangle_intersect(&testRect, &mRect, &isect)) {
                    if (isect.width == testRect.width &&
                        isect.height == testRect.height) {
                        fits = true;
                        break;
                    }
                }
            }
            if (fits) {
                defState.width  = savedW;
                defState.height = savedH;
            }
        }
        *userState = defState;
    }

    /* GTK4: position is compositor-managed; just set default size */
    gtk_window_set_default_size(GTK_WINDOW(theWindow), userState->width, userState->height);
    (*computeStdState)(theWindow);

    if (zoomed) {
        GdkRectangle stdState = *userState;
        (*computeDefState)(theWindow, &stdState);
        /* GTK4: position is compositor-managed; just set default size */
        gtk_window_set_default_size(GTK_WINDOW(theWindow), stdState.width, stdState.height);
    }
}

/*______________________________________________________________________

	 utl_Rom64 - Check to see if we have the old 64K ROM.

	 Exit:		function result = false (not applicable on GTK).
_____________________________________________________________________*/


bool utl_Rom64(void)
{
    return false;
}

/*______________________________________________________________________

	 utl_RFSanity - Check a Resource File's Sanity.

	 Entry: 	spec = file path string.

	 Exit:		*sane = true if the file passes basic header checks.
					function result = 0 on success, -1 on I/O error.
_____________________________________________________________________*/


int utl_RFSanity(const char *spec, bool *sane)
{
    FILE          *f;
    unsigned char  header[16];
    unsigned long  dataFWA, mapFWA, dataLen, mapLen;
    unsigned long  dataLWA, mapLWA;
    long           fileLen;
    unsigned char *map      = NULL;
    bool           mapOK    = false;
    int            rCode    = 0;

    if (!spec || !sane) return -1;

    f = fopen(spec, "rb");
    if (!f) {
        *sane = true;
        return 0;
    }

    if (fseek(f, 0, SEEK_END) != 0) { rCode = -1; goto done; }
    fileLen = ftell(f);
    if (fileLen < 0) { rCode = -1; goto done; }
    if (fileLen == 0) { *sane = true; goto done; }

    rewind(f);
    if (fread(header, 1, 16, f) != 16) { rCode = -1; goto done; }

    dataFWA = ((unsigned long)header[0]  << 24) | ((unsigned long)header[1]  << 16) |
              ((unsigned long)header[2]  <<  8) |  (unsigned long)header[3];
    mapFWA  = ((unsigned long)header[4]  << 24) | ((unsigned long)header[5]  << 16) |
              ((unsigned long)header[6]  <<  8) |  (unsigned long)header[7];
    dataLen = ((unsigned long)header[8]  << 24) | ((unsigned long)header[9]  << 16) |
              ((unsigned long)header[10] <<  8) |  (unsigned long)header[11];
    mapLen  = ((unsigned long)header[12] << 24) | ((unsigned long)header[13] << 16) |
              ((unsigned long)header[14] <<  8) |  (unsigned long)header[15];

    dataLWA = dataFWA + dataLen;
    mapLWA  = mapFWA  + mapLen;

    mapOK = (mapLen > 28);
    mapOK = mapOK && (dataFWA < 0x01000000);
    mapOK = mapOK && (mapFWA  < 0x01000000);
    mapOK = mapOK && (dataLWA <= (unsigned long)fileLen);
    mapOK = mapOK && (mapLWA  <= (unsigned long)fileLen);
    mapOK = mapOK && (dataLWA <= mapFWA || mapLWA <= dataFWA);

    if (mapOK && mapLen > 0) {
        map = (unsigned char *)malloc(mapLen);
        if (!map) { rCode = -1; goto done; }

        if (fseek(f, (long)mapFWA, SEEK_SET) != 0) { rCode = -1; goto done; }
        if (fread(map, 1, mapLen, f) != mapLen) { rCode = -1; goto done; }

        {
            unsigned short typeFWA, nameFWA;
            unsigned char *pType, *pTypeEnd, *psName, *pMapEnd;
            short nType;
            int t;

            typeFWA = (unsigned short)((map[24] << 8) | map[25]);
            nameFWA = (unsigned short)((map[26] << 8) | map[27]);

            mapOK = (typeFWA == 28) && (nameFWA >= typeFWA) &&
                    (nameFWA <= mapLen) && !(typeFWA & 1) && !(nameFWA & 1);

            if (mapOK) {
                pType   = map + typeFWA;
                psName  = map + nameFWA;
                pMapEnd = map + mapLen;
                nType   = (short)(((pType[0] << 8) | pType[1]) + 1);
                pType  += 2;
                pTypeEnd = pType + (nType << 3);
                mapOK = (pTypeEnd <= pMapEnd);

                for (t = 0; mapOK && t < nType; t++) {
                    unsigned char *entry = pType + t * 8;
                    short nRes;
                    unsigned short refFWA;
                    unsigned char *pRef, *pRefEnd;
                    int r;

                    nRes   = (short)(((entry[4] << 8) | entry[5]) + 1);
                    refFWA = (unsigned short)((entry[6] << 8) | entry[7]);
                    pRef   = map + typeFWA + refFWA;
                    pRefEnd = pRef + 12 * nRes;

                    if (!(pRef >= pTypeEnd && pRef < psName && !(refFWA & 1))) {
                        mapOK = false;
                        break;
                    }

                    for (r = 0; r < nRes && mapOK; r++) {
                        unsigned char *ref = pRef + r * 12;
                        unsigned short resNameFWA;
                        unsigned long  resDataFWA;

                        resNameFWA = (unsigned short)((ref[2] << 8) | ref[3]);
                        if (resNameFWA != 0xFFFF) {
                            unsigned char *pResName = psName + resNameFWA;
                            if (!(pResName + *pResName < pMapEnd)) {
                                mapOK = false;
                                break;
                            }
                        }
                        resDataFWA = (((unsigned long)ref[4] << 16) |
                                      ((unsigned long)ref[5] <<  8) |
                                       (unsigned long)ref[6]) & 0x00FFFFFF;
                        if (!(dataFWA + resDataFWA < dataLWA)) {
                            mapOK = false;
                        }
                    }
                }
            }
        }
    }

done:
    if (map) free(map);
    if (f) fclose(f);
    if (rCode == 0) *sane = mapOK;
    return rCode;
}

/*______________________________________________________________________

	 utl_SaveWindowPos - Save Window Position.

	 Entry: 	theWindow = pointer to GTK window widget.

	 Exit:		userState = current window position and size rectangle.
					zoomed = false (GTK does not expose zoom state directly).
_____________________________________________________________________*/


void utl_SaveWindowPos(GtkWidget *theWindow, GdkRectangle *userState, bool *zoomed)
{
    int x, y, w, h;

    if (!theWindow || !userState) return;

    /* GTK4: compositor manages position */
    x = 0;
    y = 0;
    w = gtk_widget_get_width(theWindow);
    h = gtk_widget_get_height(theWindow);

    userState->x      = x;
    userState->y      = y;
    userState->width  = w;
    userState->height = h;

    if (zoomed) *zoomed = false;
}

/*______________________________________________________________________

	 utl_ScaleFontSize - Scale Font Size.

	 Entry: 	fontNum = font number (unused, Pango fonts are named).
					fontSize = nominal font size.
					percent = percent change in size.
					laser = true if laser printer (bitmap rounding skipped).

	 Exit:		function result = scaled font size (integer points).
_____________________________________________________________________*/


short utl_ScaleFontSize(short fontNum, short fontSize, short percent,
                        bool laser)
{
    short nSize;

    (void)fontNum;

    nSize = (short)((int)fontSize * (int)percent / 100);
    if (!laser && nSize < 1) nSize = 1;
    return nSize;
}

/*______________________________________________________________________

	 utl_SpinCursor - Animate cursor.

	 Call periodically after utl_InitSpinCursor to advance the cursor frame.
_____________________________________________________________________*/


void utl_SpinCursor(void)
{
    gint64      now;
    gint64      intervalUs;
    GList      *windows, *wnode;

    if (!CursArray || NumCurs <= 0) return;

    now        = g_get_monotonic_time();
    intervalUs = (gint64)TickInterval * (G_GINT64_CONSTANT(1000000) / 60);

    if (now < LastTick + intervalUs) return;

    LastTick = now;
    CurCurs++;
    if (CurCurs >= NumCurs) CurCurs = 0;

    if (CursArray[CurCurs]) {
        windows = gtk_window_list_toplevels();
        for (wnode = windows; wnode; wnode = wnode->next) {
            GtkWidget *w = GTK_WIDGET(wnode->data);
            if (gtk_widget_get_visible(w)) {
                gtk_widget_set_cursor(w, CursArray[CurCurs]);
            }
        }
        g_list_free(windows);
    }
}

/*______________________________________________________________________

	 utl_StaggerWindow - Stagger a New Window.

	 Entry: 	theWindow = pointer to GTK window widget.
					windRect = desired window size rectangle.
					initialOffset = initial pixel offset from corner.
					offset = stagger offset for subsequent windows.
					titleBarHeight, leftRimWidth = decoration sizes.
					specificDevice = 1-based monitor index, or 0 for auto.

	 Exit:		pos set to the computed (x, y) window position.
_____________________________________________________________________*/

#define SIZE_9  540
#define OFF_9   3

void utl_StaggerWindow(GtkWidget *theWindow, GdkRectangle *windRect,
                       short initialOffset, short offset,
                       short titleBarHeight, short leftRimWidth,
                       gint *pos, short specificDevice)
{
    GdkDisplay   *display;
    GdkMonitor   *gd         = NULL;
    GdkRectangle  screenRect;
    bool          hasMB      = true;
    int           windWidth, windHeight;
    int           offsetDiv2;
    int           initX, initY;
    int           curX, curY;
    int           layer;
    bool          noPos;
    bool          is9;
    GList        *windows, *wnode;

    (void)theWindow;

    display = gdk_display_get_default();

    if (specificDevice) {
        utl_GetIndGD(specificDevice, &gd, &screenRect, &hasMB);
    }

    if (!gd) {
        gd = _get_primary_monitor(display);
        gdk_monitor_get_geometry(gd, &screenRect);
        hasMB = true;
    }

    windWidth  = windRect->width;
    windHeight = windRect->height;
    offsetDiv2 = (offset + 1) >> 1;

    screenRect.x      += 3;
    screenRect.y      += 3;
    screenRect.width  -= 6;
    screenRect.height -= 6;

    is9 = (screenRect.width <= SIZE_9);

    initX = screenRect.x + initialOffset + leftRimWidth;
    initY = screenRect.y + initialOffset + titleBarHeight;

    layer = 1;

    while (true) {
        curX  = initX;
        curY  = initY;
        noPos = true;

        while (true) {
            int      nOccupied = 0;
            bool     slotFull  = false;

            if (curX + windWidth  >= screenRect.x + screenRect.width ||
                curY + windHeight >= screenRect.y + screenRect.height) {
                break;
            }
            noPos = false;

            windows = gtk_window_list_toplevels();
            for (wnode = windows; wnode; wnode = wnode->next) {
                GtkWidget *w = GTK_WIDGET(wnode->data);
                if (!gtk_widget_get_visible(w)) continue;
                if (w == theWindow) continue;
                {
                    int wx, wy, dh, dv;
                    /* GTK4: compositor manages position; skip overlap detection */
                    wx = 0; wy = 0;
                    dh = curX - wx;
                    dv = curY - wy;
                    if (dh < 0) dh = -dh;
                    if (dv < 0) dv = -dv;
                    if (dh <= offsetDiv2 && dv <= offsetDiv2) {
                        nOccupied++;
                        if (nOccupied >= layer) {
                            slotFull = true;
                            break;
                        }
                    }
                }
            }
            g_list_free(windows);

            if (!slotFull) break;

            if (initialOffset) curX += is9 ? OFF_9 : offset;
            curY += offset;
        }

        if (!noPos) break;
        layer++;
        if (layer > 100) break;
    }

    if (pos) {
        pos[0] = curX;
        pos[1] = curY;
    }
}

/*______________________________________________________________________

	 utl_StopAlert - Present Stop Alert.

	 Entry: 	alertID = identifier (used as dialog title hint).
					filterProc = optional event filter (unused in GTK modal dialog).
					cancelItem = item number of cancel button, or 0 if none.

	 Exit:		function result = 1 if OK clicked, 2 if Cancel clicked.
_____________________________________________________________________*/

typedef struct { gint response; GMainLoop *loop; } StopAlertData;

static void on_stop_alert_response(GtkDialog *d, gint r, gpointer ud)
{
    StopAlertData *data = (StopAlertData *)ud;
    (void)d;
    data->response = r;
    g_main_loop_quit(data->loop);
}

short utl_StopAlert(short alertID, gboolean (*filterProc)(GtkWidget*, GdkEvent*, gpointer),
                    short cancelItem)
{
    GtkWidget     *dialog;
    short          ret = 1;
    StopAlertData  sad;

    (void)alertID;
    (void)filterProc;

    if (cancelItem) {
        dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK_CANCEL,
                                        "An error has occurred.");
    } else {
        dialog = gtk_message_dialog_new(NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
                                        "An error has occurred.");
    }

    sad.loop     = g_main_loop_new(NULL, FALSE);
    sad.response = GTK_RESPONSE_OK;

    g_signal_connect(dialog, "response", G_CALLBACK(on_stop_alert_response), &sad);
    gtk_widget_show(dialog);
    g_main_loop_run(sad.loop);
    g_main_loop_unref(sad.loop);
    gtk_window_destroy(GTK_WINDOW(dialog));

    if (sad.response == GTK_RESPONSE_OK) {
        ret = 1;
    } else {
        ret = (cancelItem > 0) ? cancelItem : 2;
    }
    return ret;
}

/*______________________________________________________________________

	 utl_VolIsMFS - Test for MFS volume.

	 Exit:		function result = false (not applicable on GTK).
_____________________________________________________________________*/


bool utl_VolIsMFS(short vRefNum)
{
    (void)vRefNum;
    return false;
}

/*----------------------------------------------------------------------------
	GetDropLocationDirectory

	Not applicable in GTK drag-and-drop model.

	Exit:	function result = -1 (error).
----------------------------------------------------------------------------*/

int GetDropLocationDirectory(GdkDrop *dropLocation, short *volumeID,
                             long *directoryID)
{
    (void)dropLocation;
    (void)volumeID;
    (void)directoryID;
    return -1;
}

/*----------------------------------------------------------------------------
	DragTargetWasTrash

	Check to see if the target of a drag and drop was the trash.

	Exit:	function result = false (not applicable in GTK).
----------------------------------------------------------------------------*/

bool DragTargetWasTrash(GdkDrop *dragRef)
{
    (void)dragRef;
    return false;
}
