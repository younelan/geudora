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

#include "print.h"
#define FILE_NUM 32
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions for dealing with printing
 * GTK4 port: Carbon Print Manager → GtkPrintOperation + Cairo/Pango
 *
 * Original architecture:
 *   PrintPreamble → PMBegin/PMPrintDialog/PMBeginDocument/PMBeginPage
 *   PrintMessage → loop calling PrintMessagePage
 *   PrintMessagePage → PMBeginPage/PETEPrintPage/PMEndPage
 *   PrintCleanup → PMEndDocument/PMEnd
 *
 * GTK4 architecture:
 *   GtkPrintOperation with begin-print, draw-page, done signals
 *   Cairo + Pango for all rendering (headers, footers, message text)
 *   Text extracted from GtkTextBuffer (GtkWidget * = GtkWidget*)
 ************************************************************************/

#include "Globals.h"
#include "MyRes.h"
#include "StringUtil.h"
#include "mailbox.h"
#include "macmbx.h"
#include "macmbx_mailer.h"
#include "message.h"
#include "../gEditCtrl/geditctrl.h"
/* imapdownload.h removed — crispy_imap handles IMAP */
#include "threading.h"
#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <string.h>

/* Module-level page setup — persists across print operations */
static GtkPageSetup *gPageSetup = NULL;
static GtkPrintSettings *gPrintSettings = NULL;

/* Functions and globals come from included headers:
 * CommandPeriod, FontID, FontSize etc. from Globals.h
 * GetWindowKind, GetMyWindowWindowPtr, CloseMyWindow, IsWindowVisible from mailbox.h
 * SetState from message.h
 * GetRLong, GetRString, GetPref, PrefIsSet from StringUtil.h / features.h */

/* Resource IDs used by print — from MyRes.h */
#ifndef PRINT_H_MAR
#define PRINT_H_MAR 500
#endif
#ifndef PRINT_LEFT_MAR
#define PRINT_LEFT_MAR 501
#endif
#ifndef PRINT_RIGHT_MAR
#define PRINT_RIGHT_MAR 502
#endif
#ifndef PRINT_H_SIZE
#define PRINT_H_SIZE 503
#endif
#ifndef PRINT_H_FONT
#define PRINT_H_FONT 504
#endif
#ifndef RETURN_PRINT_INTRO
#define RETURN_PRINT_INTRO 505
#endif
#ifndef PREF_PRINT_FONT
#define PREF_PRINT_FONT 506
#endif
#ifndef PREF_PRINT_FONT_SIZE
#define PREF_PRINT_FONT_SIZE 507
#endif
#ifndef PREF_PRINT_BLACK
#define PREF_PRINT_BLACK 508
#endif

#ifndef COMP_WIN
#define COMP_WIN 5
#endif

#ifndef ECANCELED
#define ECANCELED (-128)
#endif

/************************************************************************
 * EudoraPrintData — context passed through GtkPrintOperation signals
 * Replaces the imperative Carbon PM loop with signal-driven callbacks.
 *
 * Original had: PMPrintContext, gPageSetup, gPrintSettings globals,
 * gFirstPage, gLastPage, gCurrentPage tracking, gDontBeginPage flag.
 * All of that is now handled by GtkPrintOperation internally.
 ************************************************************************/
typedef struct {
    char *title;           /* message title for page header */
    char *returnAddr;      /* return address for page footer */
    char **lines;          /* text split into lines */
    int numLines;          /* total line count */
    int hdrLineCount;      /* number of message header lines (before blank line) */
    int linesPerPage;      /* lines fitting on one page */
    double lineHeight;     /* pixel height of one line */
    double hdrLineHeight;  /* pixel height of one header line */
    double marginTop;      /* top margin in points */
    double marginBottom;   /* bottom margin in points */
    double marginLeft;     /* left margin in points */
    double marginRight;    /* right margin in points */
    double headerHeight;   /* height of page header area */
    PangoFontDescription *bodyFont;    /* body text font */
    PangoFontDescription *msgHdrFont;  /* message header font (smaller) */
    PangoFontDescription *headerFont;  /* page header/footer font */
    PangoContext *pangoCtx;            /* pango context for layouts */
    bool isOutgoing;       /* outgoing message? */
    bool printBlack;       /* force black text */
    bool useMonospace;     /* fixed-width font (user toggled "Fixed") */
    char *export_pdf_path; /* PDF export path (NULL if printing directly) */
    GtkWidget *pte;        /* gEditCtrl widget for WYSIWYG rendering (may be NULL) */
    double totalContentH;  /* total styled content height (for pagination) */
    char *headerText;      /* message headers text (rendered separately) */
    int headerTextLines;   /* number of header lines */
    double headerBlockH;   /* rendered height of header block */
} EudoraPrintData;

/************************************************************************
 * GetTextFromPTE — extract text from a GtkWidget * (GtkWidget*)
 *
 * Original used PETEGetTextAndSelection / PETEPrintPage to render
 * PETE text directly to the printer port. In GTK4, GtkWidget * is a
 * GtkWidget* (GtkTextView from gEditCtrl). We extract the full text
 * from its GtkTextBuffer and render via Pango.
 ************************************************************************/
static char *GetTextFromPTE(GtkWidget *pte)
{
    if (!pte)
        return g_strdup("");

    /* gEditCtrl: use its native text extraction */
    gchar *text = geditctrl_get_text(pte);
    if (text)
        return text;

    /* Fallback for GtkTextView */
    if (GTK_IS_TEXT_VIEW(pte)) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
        if (buf) {
            GtkTextIter start, end;
            gtk_text_buffer_get_start_iter(buf, &start);
            gtk_text_buffer_get_end_iter(buf, &end);
            return gtk_text_buffer_get_text(buf, &start, &end, TRUE);
        }
    }

    return g_strdup("");
}

/************************************************************************
 * GetSelectedTextFromPTE — extract selected text from GtkWidget *
 *
 * Original used PETEPrintSelectionSetup to restrict printing to
 * the current selection. We just grab the selection text.
 ************************************************************************/
static char *GetSelectedTextFromPTE(GtkWidget *pte)
{
    if (!pte)
        return NULL;

    /* gEditCtrl selection */
    if (geditctrl_has_selection(pte))
        return geditctrl_get_selected_text(pte);

    /* Fallback for GtkTextView */
    if (GTK_IS_TEXT_VIEW(pte)) {
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
        if (buf) {
            GtkTextIter start, end;
            if (gtk_text_buffer_get_selection_bounds(buf, &start, &end))
                return gtk_text_buffer_get_text(buf, &start, &end, TRUE);
        }
    }

    return NULL;
}

/************************************************************************
 * CollectReturnAddr — collect the return address for page footers
 *
 * Original concatenated GetRString(RETURN_PRINT_INTRO) + GetReturnAddr.
 * Both used Pascal strings. Port uses C strings throughout.
 ************************************************************************/
static char *CollectReturnAddr(void)
{
    /* GetReturnAddr returns the user's configured return address. */
    unsigned char buf[256];
    GetReturnAddr(buf, false);
    if (buf[0])
        return g_strdup((char *)buf);
    return g_strdup("");
}

/************************************************************************
 * GetMessageTitle — get the title string for a message window
 *
 * Original: if isMessage, called MakeMessTitle (which formatted
 * "From: sender - Subject" with local date). Otherwise MyGetWTitle.
 * Both used Pascal Str255. Port uses C strings.
 ************************************************************************/
static char *GetMessageTitle(MyWindowPtr win)
{
    if (!win || !win->window)
        return g_strdup("Message");

    GtkWidget *winWP = win->window;

    /* Try window title first */
    if (GTK_IS_WINDOW(winWP)) {
        const char *title = gtk_window_get_title(GTK_WINDOW(winWP));
        if (title && *title)
            return g_strdup(title);
    }

    /* Try headerbar title widget (we use custom titlebar) */
    if (GTK_IS_WINDOW(winWP)) {
        GtkWidget *hb = gtk_window_get_titlebar(GTK_WINDOW(winWP));
        if (hb && GTK_IS_HEADER_BAR(hb)) {
            GtkWidget *tw = gtk_header_bar_get_title_widget(GTK_HEADER_BAR(hb));
            if (tw && GTK_IS_LABEL(tw)) {
                const char *label = gtk_label_get_text(GTK_LABEL(tw));
                if (label && *label)
                    return g_strdup(label);
            }
        }
    }

    /* Fall back to TOC summary info */
    extern void MakeMessTitle(char *, MacmbxTOC *, int, bool);
    MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
    if (messH && messH->tocH && messH->sumNum >= 0 &&
        messH->sumNum < messH->tocH->count) {
        char title[256];
        MakeMessTitle(title, messH->tocH, messH->sumNum, true);
        if (title[0])
            return g_strdup(title);
    }

    return g_strdup("Message");
}

/************************************************************************
 * SetupPrintFont — set up the font to use for printing
 *
 * Original read PREF_PRINT_FONT and PREF_PRINT_FONT_SIZE from prefs,
 * falling back to the current display font. Set FontID, FontSize,
 * FontWidth, FontLead, FontDescent, FontAscent, FontIsFixed globals.
 *
 * In GTK4, font metrics come from Pango at draw time. This function
 * is kept for compatibility — it updates the global font vars that
 * other code may read, but the actual print rendering uses Pango
 * font descriptions created in the begin-print callback.
 ************************************************************************/
void SetupPrintFont(void)
{
    /* In the GTK port, font handling is done via PangoFontDescription
       at print time. The globals are left alone since the print operation
       uses its own font descriptions. If other code calls SetupPrintFont
       expecting side effects on FontID etc., those globals remain as-is
       since we don't have the Mac font manager. */
}

/************************************************************************
 * MakePrintFont — create a Pango font description for printing
 *
 * Reads the print font preference. Falls back to "Monospace 11".
 * This replaces the Mac SetupPrintFont + TextFont + TextSize sequence.
 ************************************************************************/
static PangoFontDescription *MakePrintFont(void)
{
    /* TODO: read from Eudora preferences when pref system is fully ported.
       For now use a sensible default that matches the original's use of
       a fixed-width print font. */
    return pango_font_description_from_string("Monospace 11");
}

/************************************************************************
 * MakeHeaderFont — create a Pango font for headers/footers
 *
 * Original used GetRLong(PRINT_H_SIZE) for size and
 * GetFontID(GetRString(PRINT_H_FONT)) for the font face, with bold.
 ************************************************************************/
static PangoFontDescription *MakeHeaderFont(void)
{
    PangoFontDescription *font = pango_font_description_from_string("Sans Bold 10");
    return font;
}

/************************************************************************
 * begin_print — GtkPrintOperation "begin-print" signal handler
 *
 * This replaces PrintPreamble + the page counting logic.
 * Original: PMBegin, GetPrintSettings, PMPrintDialog, PMBeginDocument,
 * PMGetFirstPage/PMGetLastPage, PMBeginPage, PMGetGrafPtr, SetPort,
 * SetupPrintFont, TextFont, TextSize.
 *
 * GTK4: Calculate line height, lines per page, total pages. Set up
 * fonts and Pango context for draw-page callbacks.
 ************************************************************************/
static void begin_print(GtkPrintOperation *op, GtkPrintContext *context,
                        gpointer user_data)
{
    EudoraPrintData *pd = (EudoraPrintData *)user_data;
    gdouble pageHeight = gtk_print_context_get_height(context);
    gdouble pageWidth = gtk_print_context_get_width(context);
    PangoLayout *tmpLayout;
    PangoRectangle logical;
    int numPages;

    /* Margins — original used GetRLong(PRINT_H_MAR) for top/bottom header
       margin and GetRLong(PRINT_LEFT_MAR)/PRINT_RIGHT_MAR for sides.
       Typical values: header margin 36pt, left/right 36pt. */
    pd->marginTop = 36.0;
    pd->marginBottom = 36.0;
    pd->marginLeft = 36.0;
    pd->marginRight = 36.0;
    pd->headerHeight = 20.0;

    /* Create Pango context from the print context */
    PangoFontMap *fontmap = pango_cairo_font_map_get_default();
    pd->pangoCtx = pango_font_map_create_context(fontmap);

    /* Create fonts — use monospace if user toggled Fixed, else proportional */
    if (pd->useMonospace)
        pd->bodyFont = pango_font_description_from_string("Monospace 11");
    else
        pd->bodyFont = pango_font_description_from_string("Serif 11");
    pd->headerFont = MakeHeaderFont();
    /* Message headers: smaller sans-serif */
    pd->msgHdrFont = pango_font_description_from_string("Sans 9");

    /* Measure line height using body font */
    tmpLayout = pango_layout_new(pd->pangoCtx);
    pango_layout_set_font_description(tmpLayout, pd->bodyFont);
    pango_layout_set_width(tmpLayout,
        (int)((pageWidth - pd->marginLeft - pd->marginRight) * PANGO_SCALE));
    pango_layout_set_wrap(tmpLayout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_text(tmpLayout, "Xgy|", -1);
    pango_layout_get_extents(tmpLayout, NULL, &logical);
    pd->lineHeight = (double)logical.height / PANGO_SCALE;
    g_object_unref(tmpLayout);

    if (pd->lineHeight < 1.0)
        pd->lineHeight = 14.0;

    /* Usable content height = page minus margins minus header and footer */
    double contentHeight = pageHeight - pd->marginTop - pd->marginBottom
                           - pd->headerHeight * 2;

    pd->linesPerPage = (int)(contentHeight / pd->lineHeight);
    if (pd->linesPerPage < 1)
        pd->linesPerPage = 1;

    /* Measure header block height (rendered with smaller font) */
    double contentWidth = pageWidth - pd->marginLeft - pd->marginRight;
    pd->headerBlockH = 0;
    if (pd->headerText && pd->headerText[0]) {
        PangoLayout *hl = pango_layout_new(pd->pangoCtx);
        pango_layout_set_font_description(hl, pd->msgHdrFont);
        pango_layout_set_width(hl, (int)(contentWidth * PANGO_SCALE));
        pango_layout_set_wrap(hl, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_text(hl, pd->headerText, -1);
        PangoRectangle hlr;
        pango_layout_get_extents(hl, NULL, &hlr);
        pd->headerBlockH = (double)hlr.height / PANGO_SCALE + 8; /* +8 for padding */
        g_object_unref(hl);
    }

    /* Calculate total content height */
    double bodyContentH = 0;
    double pageContentH = pageHeight - pd->marginTop - pd->marginBottom
                          - pd->headerHeight * 2 - 8;

    if (pd->pte) {
        /* WYSIWYG: measure styled content */
        bodyContentH = geditctrl_measure_content(pd->pte, pd->pangoCtx,
                                                  (int)contentWidth);
    } else {
        /* Plain text fallback */
        PangoLayout *pageLayout = pango_layout_new(pd->pangoCtx);
        pango_layout_set_font_description(pageLayout, pd->bodyFont);
        pango_layout_set_width(pageLayout, (int)(contentWidth * PANGO_SCALE));
        pango_layout_set_wrap(pageLayout, PANGO_WRAP_WORD_CHAR);
        for (int li = 0; li < pd->numLines; li++) {
            pango_layout_set_text(pageLayout, pd->lines[li], -1);
            PangoRectangle lr;
            pango_layout_get_extents(pageLayout, NULL, &lr);
            double lh = (double)lr.height / PANGO_SCALE;
            if (lh < pd->lineHeight) lh = pd->lineHeight;
            bodyContentH += lh;
        }
        g_object_unref(pageLayout);
    }

    pd->totalContentH = pd->headerBlockH + bodyContentH;

    /* Calculate pages: first page has header block, subsequent pages don't */
    double firstPageContent = pageContentH - pd->headerBlockH;
    if (firstPageContent < 50) firstPageContent = 50;
    numPages = 1;
    double remaining = bodyContentH - firstPageContent;
    while (remaining > 0) {
        numPages++;
        remaining -= pageContentH;
    }
    if (numPages < 1) numPages = 1;

    gtk_print_operation_set_n_pages(op, numPages);
}

/************************************************************************
 * draw_page — GtkPrintOperation "draw-page" signal handler
 *
 * This replaces PrintMessagePage + PrintMessageHeader + PrintBottomHeader.
 *
 * Original PrintMessagePage:
 *   - Checked gCurrentPage vs gFirstPage/gLastPage range
 *   - Called PMBeginPage if not gDontBeginPage
 *   - Called PrintMessageHeader for top header (title + page number)
 *   - Called PETEPrintPage to render PETE text to printer port
 *   - Called PrintBottomHeader for bottom header (return addr + page number)
 *   - Called PMEndPage
 *
 * Original PrintMessageHeader:
 *   - Drew a horizontal line at the header boundary
 *   - Drew the title left-aligned, page number right-aligned
 *   - Used TextFace(bold), TextSize(PRINT_H_SIZE), TextFont(PRINT_H_FONT)
 *   - NumToString for page number, DrawString/DrawText for rendering
 *   - CalcTrunc to truncate long titles
 *
 * Original PrintBottomHeader:
 *   - Called CollectReturnAddr (RETURN_PRINT_INTRO + GetReturnAddr)
 *   - Called PrintMessageHeader with negative height for bottom placement
 *
 * GTK4 port renders everything with Cairo + Pango:
 ************************************************************************/
static void draw_page(GtkPrintOperation *op, GtkPrintContext *context,
                      gint pageNum, gpointer user_data)
{
    EudoraPrintData *pd = (EudoraPrintData *)user_data;
    cairo_t *cr = gtk_print_context_get_cairo_context(context);
    gdouble pageWidth = gtk_print_context_get_width(context);
    PangoLayout *layout;
    double y;
    int startLine, endLine, i;
    char pageStr[32];

    (void)op;

    /* ---- Top header ----
     * Original: PrintMessageHeader(title, pageNum, hMar, uRect->top+hMar,
     *           uRect->left, uRect->right)
     * Drew horizontal line, title on left, page number on right, bold font. */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    /* Horizontal line under header */
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, pd->marginLeft, pd->marginTop + pd->headerHeight);
    cairo_line_to(cr, pageWidth - pd->marginRight, pd->marginTop + pd->headerHeight);
    cairo_stroke(cr);

    /* Title text — left-aligned */
    layout = pango_layout_new(pd->pangoCtx);
    pango_layout_set_font_description(layout, pd->headerFont);

    /* Truncate title to fit, leaving room for page number */
    double maxTitleWidth = pageWidth - pd->marginLeft - pd->marginRight - 80;
    pango_layout_set_width(layout, (int)(maxTitleWidth * PANGO_SCALE));
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_text(layout, pd->title ? pd->title : "", -1);

    cairo_move_to(cr, pd->marginLeft, pd->marginTop + 2);
    pango_cairo_show_layout(cr, layout);

    /* Page number — right-aligned */
    snprintf(pageStr, sizeof(pageStr), "%d", pageNum + 1);
    pango_layout_set_width(layout, -1);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_text(layout, pageStr, -1);

    PangoRectangle pageRect;
    pango_layout_get_extents(layout, NULL, &pageRect);
    double pageNumWidth = (double)pageRect.width / PANGO_SCALE;
    cairo_move_to(cr, pageWidth - pd->marginRight - pageNumWidth,
                  pd->marginTop + 2);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);

    /* ---- Content area ---- */
    y = pd->marginTop + pd->headerHeight + 4;
    gdouble pageHeight = gtk_print_context_get_height(context);
    double maxY = pageHeight - pd->marginBottom - pd->headerHeight;
    double contentWidth = pageWidth - pd->marginLeft - pd->marginRight;
    double pageContentH = maxY - y;

    /* ---- Message headers (page 0 only, gray background, smaller font) ---- */
    if (pageNum == 0 && pd->headerText && pd->headerText[0]) {
        /* Gray background behind headers */
        cairo_save(cr);
        cairo_set_source_rgb(cr, 0.93, 0.93, 0.93);
        cairo_rectangle(cr, pd->marginLeft, y, contentWidth, pd->headerBlockH);
        cairo_fill(cr);
        cairo_restore(cr);

        /* Render header text with smaller font */
        layout = pango_layout_new(pd->pangoCtx);
        pango_layout_set_font_description(layout, pd->msgHdrFont);
        pango_layout_set_width(layout, (int)(contentWidth * PANGO_SCALE));
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_text(layout, pd->headerText, -1);
        cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
        cairo_move_to(cr, pd->marginLeft + 4, y + 4);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);

        y += pd->headerBlockH;
    }

    /* ---- Body: WYSIWYG or plain text ---- */
    double bodyStartY = (pageNum == 0) ? pd->headerBlockH : 0;
    double skipH = 0;
    if (pageNum == 0) {
        skipH = 0;
    } else {
        /* Calculate how much content to skip for this page */
        double firstPageBody = pageContentH - pd->headerBlockH;
        skipH = firstPageBody + (pageNum - 1) * pageContentH;
    }
    double availH = maxY - y;

    if (pd->pte) {
        /* WYSIWYG rendering via gEditCtrl */
        geditctrl_render_to_cairo(pd->pte, cr, pd->pangoCtx,
                                   pd->marginLeft, y,
                                   (int)contentWidth, skipH, availH);
    } else {
        /* Plain text fallback */
        layout = pango_layout_new(pd->pangoCtx);
        pango_layout_set_font_description(layout, pd->bodyFont);
        pango_layout_set_width(layout, (int)(contentWidth * PANGO_SCALE));
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

        /* Skip lines for previous pages */
        double skipped = 0;
        int curLine = 0;
        while (curLine < pd->numLines && skipped < skipH) {
            pango_layout_set_text(layout, pd->lines[curLine], -1);
            PangoRectangle lr;
            pango_layout_get_extents(layout, NULL, &lr);
            double lh = (double)lr.height / PANGO_SCALE;
            if (lh < pd->lineHeight) lh = pd->lineHeight;
            skipped += lh;
            curLine++;
        }

        /* Render visible lines */
        while (curLine < pd->numLines && y < maxY) {
            pango_layout_set_text(layout, pd->lines[curLine], -1);
            PangoRectangle lr;
            pango_layout_get_extents(layout, NULL, &lr);
            double lh = (double)lr.height / PANGO_SCALE;
            if (lh < pd->lineHeight) lh = pd->lineHeight;
            if (y + lh > maxY) break;
            cairo_set_source_rgb(cr, 0, 0, 0);
            cairo_move_to(cr, pd->marginLeft, y);
            pango_cairo_show_layout(cr, layout);
            y += lh;
            curLine++;
        }
        g_object_unref(layout);
    }

    /* ---- Bottom header / footer ----
     * Original: PrintBottomHeader(pageNum, uRect)
     *   → CollectReturnAddr(returnAddr)
     *   → PrintMessageHeader(returnAddr, pageNum, -hMar, uRect->bottom, ...)
     * Drew horizontal line above footer, return address left, page number right. */
    double footerY = pageHeight - pd->marginBottom;

    /* Horizontal line above footer */
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, pd->marginLeft, footerY - pd->headerHeight);
    cairo_line_to(cr, pageWidth - pd->marginRight, footerY - pd->headerHeight);
    cairo_stroke(cr);

    /* Return address — left-aligned in footer */
    layout = pango_layout_new(pd->pangoCtx);
    pango_layout_set_font_description(layout, pd->headerFont);

    if (pd->returnAddr && pd->returnAddr[0]) {
        pango_layout_set_width(layout, (int)(maxTitleWidth * PANGO_SCALE));
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
        pango_layout_set_text(layout, pd->returnAddr, -1);
        cairo_move_to(cr, pd->marginLeft, footerY - pd->headerHeight + 2);
        pango_cairo_show_layout(cr, layout);
    }

    /* Page number in footer — right-aligned */
    pango_layout_set_width(layout, -1);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_text(layout, pageStr, -1);
    pango_layout_get_extents(layout, NULL, &pageRect);
    pageNumWidth = (double)pageRect.width / PANGO_SCALE;
    cairo_move_to(cr, pageWidth - pd->marginRight - pageNumWidth,
                  footerY - pd->headerHeight + 2);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
}

/************************************************************************
 * print_done — GtkPrintOperation "done" signal handler
 *
 * This replaces PrintCleanup.
 * Original: PMEndDocument, PMSetError(0), PMEnd,
 *   PMDisposePageFormat, PMDisposePrintSettings, FigureOutFont,
 *   UseResFile, FloatingWinIdle.
 *
 * GTK4: Free our EudoraPrintData and all its allocations.
 ************************************************************************/
static void print_done(GtkPrintOperation *op, GtkPrintOperationResult result,
                       gpointer user_data)
{
    EudoraPrintData *pd = (EudoraPrintData *)user_data;

    (void)op;

    if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
        GError *error = NULL;
        gtk_print_operation_get_error(op, &error);
        if (error) {
            g_printerr("Eudora: Print error: %s\n", error->message);
            g_error_free(error);
        }
    }

    /* If we exported to PDF, open it in Preview.app for print/preview */
    if (pd->export_pdf_path && result == GTK_PRINT_OPERATION_RESULT_APPLY) {
        char *cmd = g_strdup_printf("open \"%s\"", pd->export_pdf_path);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
    }

    /* Free line array */
    if (pd->lines) {
        for (int i = 0; i < pd->numLines; i++)
            g_free(pd->lines[i]);
        g_free(pd->lines);
    }

    g_free(pd->title);
    g_free(pd->returnAddr);
    g_free(pd->headerText);
    g_free(pd->export_pdf_path);

    if (pd->bodyFont)
        pango_font_description_free(pd->bodyFont);
    if (pd->msgHdrFont)
        pango_font_description_free(pd->msgHdrFont);
    if (pd->headerFont)
        pango_font_description_free(pd->headerFont);
    if (pd->pangoCtx)
        g_object_unref(pd->pangoCtx);

    g_free(pd);
}

/************************************************************************
 * RunPrintOperation — execute a print operation for given text
 *
 * This is the core that replaces the PrintPreamble → PrintMessage →
 * PrintCleanup sequence. The original:
 *   1. PrintPreamble: PMBegin, GetPrintSettings (load/create page format
 *      and print settings), PMPrintDialog (show dialog unless 'now'),
 *      PMBeginDocument, PMBeginPage, PMGetGrafPtr, SetPort, GetURect,
 *      SetupPrintFont, TextFont, TextSize
 *   2. PrintMessage: PeteCalcOn, PETESetCallback, PETEPrintSetup,
 *      PETEChangeDocWidth, loop { PrintMessagePage }, PETEPrintCleanup
 *   3. PrintCleanup: PMEndDocument, PMEnd, PMDisposePageFormat,
 *      PMDisposePrintSettings, FigureOutFont, UseResFile
 *
 * GTK4 collapses all of that into: create GtkPrintOperation, connect
 * signals, gtk_print_operation_run. The "now" flag maps to using
 * ACTION_PRINT (skip dialog) vs ACTION_PRINT_DIALOG.
 ************************************************************************/
static int RunPrintOperation(const char *text, const char *headerText,
                             const char *title, GtkWidget *pte,
                             bool isOutgoing, bool useMonospace,
                             bool now, GtkWindow *parent)
{
    GtkPrintOperation *printOp;
    GtkPrintOperationResult result;
    EudoraPrintData *pd;
    GError *error = NULL;

    if (!text)
        return -1;

#ifdef __APPLE__
    /* macOS: GTK tries to launch "evince" for print preview via
       g_app_info_create_from_commandline, which is stubbed out on macOS.
       Tell GTK to use Preview.app instead. */
    {
        GtkSettings *settings = gtk_settings_get_default();
        if (settings)
            g_object_set(settings, "gtk-print-preview-command",
                         "open %f", NULL);
    }
#endif

    /* Allocate and populate print data */
    pd = g_new0(EudoraPrintData, 1);
    pd->title = g_strdup(title ? title : "Message");
    pd->returnAddr = CollectReturnAddr();
    pd->isOutgoing = isOutgoing;
    pd->useMonospace = useMonospace;
    pd->pte = pte;
    pd->headerText = g_strdup(headerText ? headerText : "");

    /* Split text into lines — original iterated through PETE paragraphs
       and lines via PETEPrintPage. We split on newlines. */
    pd->lines = g_strsplit(text, "\n", -1);
    pd->numLines = 0;
    pd->hdrLineCount = 0;
    if (pd->lines) {
        bool foundBlank = false;
        for (int i = 0; pd->lines[i] != NULL; i++) {
            /* First blank line separates headers from body */
            if (!foundBlank && pd->lines[i][0] == '\0') {
                pd->hdrLineCount = i;
                foundBlank = true;
            }
            pd->numLines++;
        }
        if (!foundBlank) pd->hdrLineCount = 0;
    }

    /* Create print operation */
    printOp = gtk_print_operation_new();
    gtk_print_operation_set_allow_async(printOp, FALSE);

    /* Connect the three core signals that replace the Carbon PM sequence */
    g_signal_connect(printOp, "begin-print", G_CALLBACK(begin_print), pd);
    g_signal_connect(printOp, "draw-page", G_CALLBACK(draw_page), pd);
    g_signal_connect(printOp, "done", G_CALLBACK(print_done), pd);

#ifdef __APPLE__
    /* macOS: GTK4's Unix print dialog crashes due to broken
       g_app_info_create_from_commandline stub. Export to PDF and
       open in Preview.app — user can print from there via Cmd+P.
       This gives both preview and print access to all printers. */
    GtkPrintOperationAction action = GTK_PRINT_OPERATION_ACTION_EXPORT;
    char *tmpdir = g_dir_make_tmp("eudora-print-XXXXXX", NULL);
    if (!tmpdir) tmpdir = g_strdup("/tmp");
    char *pdf_path = g_build_filename(tmpdir, "message.pdf", NULL);
    gtk_print_operation_set_export_filename(printOp, pdf_path);
    g_free(tmpdir);
    pd->export_pdf_path = pdf_path;
#else
    GtkPrintOperationAction action = now
        ? GTK_PRINT_OPERATION_ACTION_PRINT
        : GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG;
#endif

    /* Use saved page setup and print settings if available */
    if (gPageSetup)
        gtk_print_operation_set_default_page_setup(printOp, gPageSetup);
    if (gPrintSettings)
        gtk_print_operation_set_print_settings(printOp, gPrintSettings);

    result = gtk_print_operation_run(printOp, action, parent, &error);

    int err = 0;
    if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
        if (error) {
            g_printerr("Eudora: Print error: %s\n", error->message);
            g_error_free(error);
        }
        err = -1;
    } else if (result == GTK_PRINT_OPERATION_RESULT_CANCEL) {
        err = ECANCELED;
    }

    /* Save settings for next print */
    if (result == GTK_PRINT_OPERATION_RESULT_APPLY) {
        if (gPrintSettings)
            g_object_unref(gPrintSettings);
        gPrintSettings = g_object_ref(
            gtk_print_operation_get_print_settings(printOp));
    }

    g_object_unref(printOp);
    return err;
}

/************************************************************************
 * GetParentWindow — get the GtkWindow parent for print dialogs
 *
 * Original brought the document window to front of floating windows
 * and used it as the print manager's reference window.
 ************************************************************************/
static GtkWindow *GetParentWindow(MyWindowPtr win)
{
    if (win && win->window && GTK_IS_WINDOW(win->window))
        return GTK_WINDOW(win->window);
    return NULL;
}

/************************************************************************
 * PrintOneMessage - print out a given message
 *
 * Original:
 *   - Got GtkWidget * from MyWindowPtr via GetMyWindowWindowPtr
 *   - Checked for nickname scanning capability (NicknameWatcherFocusChange)
 *   - PushGWorld (save graphics state)
 *   - PrintPreamble (setup PM, show dialog unless 'now')
 *   - Checked if COMP_WIN (outgoing message)
 *   - PMGetGrafPtr/SetPort to printer port
 *   - PrintMessage (the core page loop)
 *   - PrintCleanup
 *   - PopGWorld (restore graphics state)
 *
 * GTK4: Extract text from the window's PTE editor widget, run print op.
 ************************************************************************/
int PrintOneMessage(MyWindowPtr win, bool select, bool now)
{
    char *text;
    char *title;
    bool isOut;
    int err;
    GtkWidget *pte;
    GtkWindow *parent;

    if (!win)
        return -1;

    pte = win->pte;
    parent = GetParentWindow(win);

    /* Original: isOut = GetWindowKind(winWP)==COMP_WIN */
    isOut = false;
    if (win->window)
        isOut = (GetWindowKind(win->window) == COMP_WIN);

    /* Build header text (separate from body for styled rendering) */
    char *headerText = NULL;
    MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
    if (messH && messH->tocH && messH->sumNum >= 0 &&
        messH->sumNum < messH->tocH->count) {
        MacmbxMsgSum *sum = &messH->tocH->msgs[messH->sumNum];
        GString *hdr = g_string_new("");

        if (MessFlagIsSet(messH, FLAG_SHOW_ALL)) {
            extern char *read_raw_headers(MacmbxTOC *, int, long *);
            long hdr_len = 0;
            char *raw = read_raw_headers(messH->tocH, messH->sumNum, &hdr_len);
            if (raw) {
                g_string_append_len(hdr, raw, hdr_len);
                g_free(raw);
            }
        } else {
            if (sum->from[0])
                g_string_append_printf(hdr, "%s: %s\n",
                    isOut ? "To" : "From", sum->from);
            if (sum->seconds) {
                time_t t = (time_t)sum->seconds;
                struct tm tm;
                localtime_r(&t, &tm);
                char datebuf[64];
                strftime(datebuf, sizeof(datebuf),
                         "%a, %b %d, %Y %I:%M %p", &tm);
                g_string_append_printf(hdr, "Date: %s\n", datebuf);
            }
            if (sum->subject[0])
                g_string_append_printf(hdr, "Subject: %s\n", sum->subject);
        }
        headerText = g_string_free(hdr, FALSE);
    }

    /* Get body text as fallback (used when pte WYSIWYG isn't available) */
    text = NULL;
    if (select)
        text = GetSelectedTextFromPTE(pte);
    if (!text)
        text = GetTextFromPTE(pte);

    /* Check if monospace/fixed-width mode is active */
    bool mono = false;
    if (messH && MessFlagIsSet(messH, FLAG_FIXED_WIDTH))
        mono = true;

    title = GetMessageTitle(win);

    err = RunPrintOperation(text, headerText, title, pte,
                            isOut, mono, now, parent);

    g_free(text);
    g_free(headerText);
    g_free(title);

    return err;
}

/************************************************************************
 * PrintSelectedMessages - print out selected messages from a TOC
 *
 * Original:
 *   - PushGWorld
 *   - PrintPreamble (one print job for all selected messages)
 *   - GetMailboxSpec to check if Out mailbox (outgoing messages)
 *   -  — unlock void **   - Loop through sums: if selected:
 *     - If IMAP, EnsureMsgDownloaded
 *     - GetAMessage to open it
 *     - PMGetGrafPtr/SetPort to printer port
 *     - PeteCalcOn
 *     - void *print-selection from preview pane (beginSel/endSel)
 *     - PrintMessage (the core page loop)
 *     - If window wasn't visible, close it; else restore selection
 *   - PrintCleanup
 *   - PopGWorld
 *
 * GTK4: Iterate selected messages, extract text from each, concatenate
 * with page breaks, run one print operation. Each message gets its own
 * header from its window title.
 *
 * Note: TOCHandle → MacmbxTOC* (no void *indirection)
 * Note: GtkWidget * → GtkWidget* (printMe parameter)
 ************************************************************************/
int PrintSelectedMessages(MacmbxTOC *tocH, bool select, bool now,
                          long beginSel, long endSel, GtkWidget *printMe)
{
    int sumNum;
    int err = 0, cumErr = 0;
    MyWindowPtr win;
    char *text = NULL;
    char *title = NULL;
    bool isOut;

    if (!tocH)
        return -1;

    /* Original checked if mailbox is "Out" to determine isOut.
       GetMailboxSpec(tocH,-1, spec); isOut = StringSame(outName, spec_name(spec));
       For now, default to false — comp windows self-identify. */
    isOut = false;

    /* Iterate through summaries — original: for(sumNum=0; !CommandPeriod && sumNum<tocH->count; sumNum++) */
    for (sumNum = 0; !CommandPeriod && sumNum < tocH->count; sumNum++) {
        if (!tocH->msgs[sumNum].selected)
            continue;

        /* Ensure message body is available (for IMAP headers-only mode) */
        {
          extern MacmbxMailer *idle_scheduler_get_mailer(void);
          MacmbxMailer *mailer = idle_scheduler_get_mailer();
          MacmbxTOC *mtoc = macmbx_toc_open(tocH->mbox_path);
          if (mailer && mtoc && sumNum < mtoc->count)
            macmbx_mailer_ensure_body(mailer, mtoc, sumNum);
        }

        /* Original: win = GetAMessage(tocH, sumNum, nil, nil, false);
           Opens the message window (or returns existing one). */
        win = GetAMessage(tocH, sumNum, NULL, NULL, false);
        if (!win) {
            err = -1;
            if (!cumErr) cumErr = err;
            continue;
        }

        /* Get the text to print */
        GtkWidget *pte = printMe ? printMe : win->pte;

        /* Original: if (!printMe && select && beginSel!=endSel)
               PeteGetTextAndSelection / PeteSelect to set selection */
        if (select && beginSel != endSel) {
            /* Extract just the selection range as text */
            GtkTextBuffer *buf = NULL;
            if (pte && GTK_IS_TEXT_VIEW(pte))
                buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pte));
            if (buf) {
                GtkTextIter start_iter, end_iter;
                gtk_text_buffer_get_iter_at_offset(buf, &start_iter, (int)beginSel);
                gtk_text_buffer_get_iter_at_offset(buf, &end_iter, (int)endSel);
                text = gtk_text_buffer_get_text(buf, &start_iter, &end_iter, TRUE);
            }
        }

        if (!text)
            text = GetTextFromPTE(pte);

        title = GetMessageTitle(win);

        /* Print this message */
        err = RunPrintOperation(text, NULL, title, win->pte,
                                isOut, false, now, GetParentWindow(win));

        g_free(text);
        text = NULL;
        g_free(title);
        title = NULL;

        /* Original: if (!IsWindowVisible(winWP)) CloseMyWindow(winWP);
           Close the message if it wasn't already open. */
        if (win->window && !IsWindowVisible(win->window))
            CloseMyWindow(win->window);

        if (err && err != ECANCELED)
            cumErr = err;

        /* Original: if (printMe) break;
           For preview pane printing, only print one copy. */
        if (printMe)
            break;

        /* Check for user cancel (CommandPeriod equivalent) */
        if (err == ECANCELED)
            break;
    }

    if (cumErr && cumErr != ECANCELED)
        g_printerr("Eudora: Some messages failed to print (error %d)\n", cumErr);

    return err ? err : cumErr;
}

/************************************************************************
 * PrintClosedMessage - print a message that is not currently open
 *
 * Original:
 *   - Checked if message was already open: opened = !tocH->msgs[sumNum].messH
 *   - GetAMessage to open it
 *   - PrintOneMessage
 *   - If we opened it, CloseMyWindow to close it again
 *
 * Note: messH is the MessHandle — if NULL, the message isn't open.
 * TOCHandle → MacmbxTOC* (no void *indirection).
 ************************************************************************/
int PrintClosedMessage(MacmbxTOC *tocH, short sumNum, bool now)
{
    bool opened;
    MyWindowPtr win;
    int err;

    if (!tocH || sumNum < 0 || sumNum >= tocH->count)
        return -1;

    /* Original: opened = !tocH->msgs[sumNum].messH;
       messH is non-NULL if the message is already open in a window. */
    opened = (tocH->msgs[sumNum].messH == NULL);

    /* Original: win = GetAMessage(tocH, sumNum, nil, nil, false); */
    win = GetAMessage(tocH, sumNum, NULL, NULL, false);
    if (!win)
        return -1;

    err = PrintOneMessage(win, false, now);

    /* If we had to open the message just for printing, close it now.
     * Original: if (opened) CloseMyWindow(GetMyWindowWindowPtr(win)); */
    if (opened && win->window)
        CloseMyWindow(win->window);

    return err;
}

/************************************************************************
 * DoPageSetup — carry on the Page Setup dialog with the user
 *
 * Original:
 *   - PMBegin (init print manager)
 *   - GetPrintSettings (load saved page format from resource file,
 *     or PMNewPageFormat + PMDefaultPageFormat)
 *   - PMPageSetupDialog (show dialog)
 *   - If accepted: PMFlattenPageFormat → save to resource file
 *   - PMDisposePageFormat, PMDisposePrintSettings
 *   - PMEnd
 *
 * GTK4: gtk_print_run_page_setup_dialog handles everything.
 * Settings persist via GtkPageSetup and GtkPrintSettings objects.
 ************************************************************************/

/* gPageSetup / gPrintSettings declared near top of file */

void DoPageSetup(GtkWindow *parent)
{
    GtkPageSetup *newSetup;

    if (!gPrintSettings)
        gPrintSettings = gtk_print_settings_new();

    /* gtk_print_run_page_setup_dialog replaces:
       PMBegin, GetPrintSettings, PMPageSetupDialog, PMFlattenPageFormat,
       SettingsHandle(save to resource), PMDisposePageFormat, PMEnd.
       The dialog is modal and returns the new page setup if accepted,
       or the old one if cancelled. */
    newSetup = gtk_print_run_page_setup_dialog(parent, gPageSetup, gPrintSettings);

    if (newSetup != gPageSetup) {
        if (gPageSetup)
            g_object_unref(gPageSetup);
        gPageSetup = newSetup;
    }

    /* Original saved the page setup to the Eudora settings resource file
       via SettingsHandle(PrintResType(), nil, PRINT_PAGE_SETUP, hPageSetup).
       TODO: persist GtkPageSetup to a config file when the prefs system
       is fully ported. For now the setup persists in memory for the
       lifetime of the application. */
}
