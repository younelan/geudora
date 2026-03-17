/*
 * Initialization and cleanup for gEudora
 * Portable C implementation for GTK4
 */

#include "ends.h"
#include "mailbox.h"
#include "schizo.h"
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
    /* Reset per-personality send queue counts */
    PersHandle pers;
    for (pers = PersList; pers; pers = pers->next)
        pers->sendQueue = 0;

    /* Count messages in Out mailbox that are QUEUED or TIMED */
    TOCType *toc = GetOutTOC();
    g_print("SetSendQueue: toc=%p count=%d\n", (void*)toc, toc ? toc->count : -1);
    int count = 0;
    if (toc) {
        for (int i = 0; i < toc->count; i++) {
            int state = toc->sums[i].state;
            if (state == QUEUED || state == TIMED) {
                count++;
                /* Credit to the owning personality */
                for (pers = PersList; pers; pers = pers->next) {
                    if (pers->persId == toc->sums[i].persId) {
                        pers->sendQueue++;
                        break;
                    }
                }
                /* If no matching pers found, credit the dominant (first) one */
                if (!pers && PersList)
                    PersList->sendQueue++;
            }
        }
    }
    SendQueue = count;
    g_print("SetSendQueue: SendQueue=%d\n", SendQueue);
}

void GetBoxLines(void)
{
    int count = BoxLinesLimit - 1; /* 12 columns */
    unsigned char scratch[256];
    long width;
    int i;

    if (!BoxWidths) {
        BoxWidths = (short *)calloc(count, sizeof(short));
        if (!BoxWidths) return;
    }

    for (i = 0; i < count; i++) {
        if (*GetRString(scratch, BoxLinesStrn + i + 1)) {
            width = atol((const char *)scratch);
            BoxWidths[i] = (short)width;
        } else {
            BoxWidths[i] = 50; /* fallback default */
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
