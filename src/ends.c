/*
 * Initialization and cleanup for gEudora
 * Portable C implementation for GTK4
 */

#include "ends.h"
#include "mailbox.h"
#include "toc.h"
#include "threading.h"
#include "StringDefs.h"
#include "StrnDefs.h"
#include "StringUtil.h"
#include "Globals.h"
#include <glib.h>
#include <stdlib.h>

/* Global send queue counter */
int SendQueue = 0;
bool SendImmediately = false;
bool SendThreadRunning = false;

void Initialize(void)
{
    g_print("Initializing gEudora\n");
}

void FigureOutFont(bool peteToo)
{
    (void)peteToo;
    g_print("Figuring out fonts\n");
}

void Cleanup(void)
{
    g_print("Cleaning up gEudora\n");
}

void BuildBoxMenus(void)
{
    g_print("Building box menus\n");
}

void RememberOpenWindows(void)
{
    g_print("Remembering open windows\n");
}

void RecallOpenWindows(void)
{
    g_print("Recalling open windows\n");
}

void SetSendQueue(void)
{
    /* Count messages in Out mailbox that are QUEUED or TIMED */
    TOCType *toc = GetOutTOC();
    int count = 0;
    if (toc) {
        g_print("SetSendQueue: Out TOC has %d messages\n", toc->count);
        for (int i = 0; i < toc->count; i++) {
            int state = toc->sums[i].state;
            g_print("  msg %d: state=%d subj='%s'\n", i, state, toc->sums[i].subj);
            if (state == QUEUED || state == TIMED)
                count++;
        }
    } else {
        g_print("SetSendQueue: GetOutTOC() returned NULL\n");
    }
    SendQueue = count;
    g_print("SetSendQueue: %d messages queued\n", count);
}

void GetBoxLines(void)
{
    int count = BoxLinesLimit - 1; /* 12 columns */
    unsigned char scratch[256];
    long width;
    int i;

    if (!BoxWidths) {
        /* Allocate Handle-style: pointer to pointer to array */
        short *data = (short *)calloc(count, sizeof(short));
        short **handle = (short **)malloc(sizeof(short *));
        if (!data || !handle) {
            free(data);
            free(handle);
            return;
        }
        *handle = data;
        BoxWidths = handle;
    }

    for (i = 0; i < count; i++) {
        if (*GetRString(scratch, BoxLinesStrn + i + 1)) {
            width = atol((const char *)scratch);
            (*BoxWidths)[i] = (short)width;
        } else {
            (*BoxWidths)[i] = 50; /* fallback default */
        }
    }
}

void SystemEudoraFolder(void)
{
    g_print("Getting system Eudora folder\n");
}

void BuildStationMenu(void)
{
    g_print("Building stationery menu\n");
}

void BuildPersMenu(void)
{
    g_print("Building personalities menu\n");
}

void BuildSigMenu(void)
{
    g_print("Building signatures menu\n");
}

void CleanTempFolder(void)
{
    g_print("Cleaning temp folder\n");
}

bool DateWarning(bool uiOK)
{
    (void)uiOK;
    return false;
}

void CreateTempBox(short which)
{
    (void)which;
    g_print("Creating temp box\n");
}
