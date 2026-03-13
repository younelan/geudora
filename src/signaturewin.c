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

/*
 * signaturewin.c — Signature management for gEudora (GTK port).
 *
 * Original Mac version used Handle-based packed Pascal string lists,
 * Mac Controls, ViewList, DirIterate, and FSSpec. This port uses:
 *   - GPtrArray of char* for the signature name list
 *   - GDir for directory scanning
 *   - Plain filesystem paths via GLib
 *   - GTK4 widgets for the management window
 *
 * No FSSpec, no Handles, no Pascal strings, no Mac resource manager.
 */

#include "signaturewin.h"

#include <ctype.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>

#include "message.h"

/* Icon resource IDs — from MyRes.h enum */
#define SIG_SICN      270
#define ALT_SIG_SICN  272
#define N_SIG_SICN    273
#define PRO_ONLY_ICON 1115

/* Signature file names */
#define SIG_STANDARD_NAME "Standard"
#define SIG_ALTERNATE_NAME "Alternate"
#define SIG_FOLDER_NAME "Signatures"

/* Feature flag — tracks whether extra signatures have been seen */
static bool gHasExtraSignatures = false;

/*
 * The signature list: a GPtrArray of g_strdup'd C strings.
 * Index 0 = "Standard", index 1 = "Alternate", indices 2+ = extra sigs.
 * SignatureCount() returns the total count.
 * GetSignatureName(item, ...) uses 1-based indexing (matching original).
 */
static GPtrArray *gSigList = NULL;

/**********************************************************************
 * GetSigFolderPath - get the path to the signatures folder, creating
 * it if needed. Returns a newly-allocated string or NULL on error.
 **********************************************************************/
static char *GetSigFolderPath(void) {
  char *path =
      g_build_filename(g_get_user_config_dir(), "eudora", SIG_FOLDER_NAME, NULL);

  g_mkdir_with_parents(path, 0755);

  return path;
}

/**********************************************************************
 * BuildSigList - scan the signatures folder and build the name list.
 *
 * Always includes "Standard" (index 0) and "Alternate" (index 1),
 * then any extra text files found in the Signatures folder.
 **********************************************************************/
void BuildSigList(void) {
  char *folderPath;
  GDir *dir;
  const char *entry;

  /* Free old list */
  if (gSigList) {
    g_ptr_array_free(gSigList, TRUE);
    gSigList = NULL;
  }

  gSigList = g_ptr_array_new_with_free_func(g_free);

  g_ptr_array_add(gSigList, g_strdup(SIG_STANDARD_NAME));
  g_ptr_array_add(gSigList, g_strdup(SIG_ALTERNATE_NAME));

  folderPath = GetSigFolderPath();
  if (!folderPath)
    return;

  dir = g_dir_open(folderPath, 0, NULL);
  if (dir) {
    while ((entry = g_dir_read_name(dir)) != NULL) {
      if (entry[0] == '.')
        continue;

      /* Skip the standard and alternate signature files */
      if (strcmp(entry, SIG_STANDARD_NAME) == 0 ||
          strcmp(entry, SIG_ALTERNATE_NAME) == 0)
        continue;

      /* Only include regular files */
      char *fullPath = g_build_filename(folderPath, entry, NULL);
      if (g_file_test(fullPath, G_FILE_TEST_IS_REGULAR)) {
        gHasExtraSignatures = true;
        g_ptr_array_add(gSigList, g_strdup(entry));
      }
      g_free(fullPath);
    }
    g_dir_close(dir);
  }

  g_free(folderPath);
}

/**********************************************************************
 * SignatureCount - return number of signatures
 **********************************************************************/
short SignatureCount(void) {
  if (!gSigList)
    BuildSigList();
  return gSigList ? (short)gSigList->len : 0;
}

/**********************************************************************
 * GetSignatureName - return name of indicated signature (1-based index)
 **********************************************************************/
void GetSignatureName(short item, char *name, int maxLen) {
  short count = SignatureCount();

  if (item >= 1 && item <= count) {
    const char *sigName = g_ptr_array_index(gSigList, item - 1);
    g_strlcpy(name, sigName, maxLen);
  } else {
    name[0] = '\0';
  }
}

/**********************************************************************
 * GetSignatureIcon - return icon resID for menu of indicated signature
 **********************************************************************/
short GetSignatureIcon(short item) {
  switch (item) {
  case 1:
    return SIG_SICN;
  case 2:
    return ALT_SIG_SICN;
  default:
    return gHasExtraSignatures ? N_SIG_SICN : PRO_ONLY_ICON;
  }
}

/**********************************************************************
 * SigPath - get the filesystem path for a signature file.
 *
 * fid: 0 = standard, 1 = alternate, >1 = hash of lowered sig name.
 * Returns a newly-allocated path, or NULL on error.
 * Caller must g_free() the result.
 **********************************************************************/
char *SigPath(long fid) {
  char *folderPath;
  char name[256];

  if (fid < 0)
    return NULL;

  folderPath = GetSigFolderPath();
  if (!folderPath)
    return NULL;

  if (fid <= 1) {
    const char *sigName = (fid == 1) ? SIG_ALTERNATE_NAME : SIG_STANDARD_NAME;
    char *path = g_build_filename(folderPath, sigName, NULL);
    g_free(folderPath);
    return path;
  }

  /* Named signature: iterate through all sigs and find by hash */
  short count = SignatureCount();
  for (short i = 1; i <= count; i++) {
    GetSignatureName(i, name, sizeof(name));
    /* Lower-case for hash comparison (original did MyLowerStr) */
    for (char *p = name; *p; p++)
      *p = tolower((unsigned char)*p);
    if ((long)Hash(name) == fid) {
      /* Use original (non-lowered) name for path */
      GetSignatureName(i, name, sizeof(name));
      char *path = g_build_filename(folderPath, name, NULL);
      g_free(folderPath);
      return path;
    }
  }

  g_free(folderPath);
  return NULL;
}

/**********************************************************************
 * OpenSignaturesWin - open the signatures management window (GTK)
 **********************************************************************/
void OpenSignaturesWin(void) {
  GtkWidget *win, *box, *scrolled, *listbox, *btnBox;
  GtkWidget *btnRemove, *btnEdit;
  short i, count;

  BuildSigList();

  win = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(win), "Signatures");
  gtk_window_set_default_size(GTK_WINDOW(win), 300, 400);

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(box, 10);
  gtk_widget_set_margin_end(box, 10);
  gtk_widget_set_margin_top(box, 10);
  gtk_widget_set_margin_bottom(box, 10);
  gtk_window_set_child(GTK_WINDOW(win), box);

  scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled),
                                             250);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_box_append(GTK_BOX(box), scrolled);

  listbox = gtk_list_box_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), listbox);

  count = SignatureCount();
  for (i = 1; i <= count; i++) {
    char name[256];
    GetSignatureName(i, name, sizeof(name));
    GtkWidget *row = gtk_label_new(name);
    gtk_widget_set_halign(row, GTK_ALIGN_START);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
  }

  btnBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_append(GTK_BOX(box), btnBox);

  if (gHasExtraSignatures) {
    GtkWidget *btnNew = gtk_button_new_with_label("New");
    gtk_box_append(GTK_BOX(btnBox), btnNew);
  }

  btnRemove = gtk_button_new_with_label("Remove");
  gtk_box_append(GTK_BOX(btnBox), btnRemove);

  btnEdit = gtk_button_new_with_label("Edit");
  gtk_box_append(GTK_BOX(btnBox), btnEdit);

  gtk_window_present(GTK_WINDOW(win));
}
