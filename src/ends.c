/*
 * Initialization and cleanup for gEudora
 * Portable C implementation for GTK4
 */

#include "ends.h"
#include <glib.h>

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
    g_print("Setting send queue\n");
}

void GetBoxLines(void)
{
    g_print("Getting box lines\n");
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

#ifdef THREADING_ON
void CreateTempBox(short which)
{
    (void)which;
    g_print("Creating temp box\n");
}
#endif
