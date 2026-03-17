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
 * taskProgress.c – GTK4 Task Progress panel
 *
 * Replaces the original Mac QuickDraw/Cairo task progress window with
 * proper GTK4 widgets.  Can be embedded as a wazoo tab or shown in a
 * standalone window.
 *
 * Layout:
 *   ┌──────────────────────────────┐
 *   │ Next check: 12:30  Last: 12:25│  ← status bar
 *   ├──────────────────────────────┤
 *   │ ▶ Checking mail...  [■■■░░] │  ← active task rows
 *   │   POP: mail.example.com      │
 *   ├──────────────────────────────┤
 *   │ ✖ Error checking mail        │  ← error rows (red)
 *   │   Connection refused          │
 *   └──────────────────────────────┘
 */

#include "taskProgress.h"
#include "Globals.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "fileutil.h"
#include "mailbox.h"
#include "message.h"
#include "mydefs.h"
#include "portable-compat.h"
#include "progress.h"
#include "schizo.h"
#include "log.h"
#include "task_ldef.h"
#include "util.h"
#include "threading.h"
#include <gtk/gtk.h>
#include <time.h>

#ifndef ReallyDoAnAlert_declared
#define ReallyDoAnAlert_declared 1
int ReallyDoAnAlert(int templ, int which);
#endif

#define FILE_NUM 104

/* ── Task progress widget state ── */
static GtkWidget *tp_widget = NULL;       /* The embeddable widget (GtkBox) */
static GtkWidget *tp_task_list = NULL;    /* GtkListBox for active tasks */
static GtkWidget *tp_error_list = NULL;   /* GtkListBox for errors */
static GtkWidget *tp_next_check = NULL;   /* Label: next check time */
static GtkWidget *tp_last_check = NULL;   /* Label: last check time */
static GtkWidget *tp_status_label = NULL; /* Label: overall status */

ListHandle TaskListHandle = NULL;
taskErrHandle TaskErrorList = NULL;
static guint tp_idle_timer = 0;
static bool tp_last_check_running = false;
static bool tp_last_send_running = false;

/* ── Internal helpers ── */

/* Task kind → human-readable label */
static const char *task_kind_label(TaskKindEnum kind) {
  switch (kind) {
  case CheckingTask:       return "Checking Mail";
  case SendingTask:        return "Sending Mail";
  case IMAPResyncTask:     return "IMAP Resync";
  case IMAPFetchingTask:   return "IMAP Fetch";
  case IMAPDeleteTask:     return "IMAP Delete";
  case IMAPUndeleteTask:   return "IMAP Undelete";
  case IMAPTransferTask:   return "IMAP Transfer";
  case IMAPExpungeTask:
  case IMAPMultExpungeTask: return "IMAP Expunge";
  case IMAPAttachmentFetch: return "Fetching Attachment";
  case IMAPSearchTask:     return "IMAP Search";
  case IMAPMultResyncTask:
  case IMAPPollingTask:    return "IMAP Poll";
  case IMAPUploadTask:     return "IMAP Upload";
  case IMAPFilterTask:     return "IMAP Filter";
  case IMAPAlertTask:      return "IMAP Alert";
  default:                 return "Task";
  }
}

static void tp_update_status_impl(void);
static gboolean tp_update_status_idle(gpointer data) {
  (void)data;
  tp_update_status_impl();
  return G_SOURCE_REMOVE;
}
/* Update the status header labels — always on main thread */
static void tp_update_status(void) {
  if (InAThread()) {
    g_idle_add(tp_update_status_idle, NULL);
    return;
  }
  tp_update_status_impl();
}
static void tp_update_status_impl(void) {
  if (!tp_widget) return;

  /* Next check time */
  if (tp_next_check) {
    unsigned long checkTicks = PersCheckTicks();
    if (checkTicks && AutoCheckOK()) {
      gint64 now_us = g_get_monotonic_time();
      unsigned long ticks = (unsigned long)(now_us * 60 / 1000000);
      if (checkTicks > ticks) {
        time_t now = time(NULL);
        time_t next = now + (checkTicks - ticks) / 60;
        struct tm *tm = localtime(&next);
        if (tm) {
          char buf[64];
          strftime(buf, sizeof(buf), "Next: %H:%M", tm);
          gtk_label_set_text(GTK_LABEL(tp_next_check), buf);
        } else {
          gtk_label_set_text(GTK_LABEL(tp_next_check), "Next: —");
        }
      } else {
        gtk_label_set_text(GTK_LABEL(tp_next_check), "Next: Soon");
      }
    } else if (checkTicks) {
      gtk_label_set_text(GTK_LABEL(tp_next_check), "Next: Never");
    } else {
      gtk_label_set_text(GTK_LABEL(tp_next_check), "Next: Not scheduled");
    }
  }

  /* Last check time */
  if (tp_last_check) {
    if (CheckThreadRunning) {
      gtk_label_set_text(GTK_LABEL(tp_last_check), "Last: Checking...");
    } else if (LastCheckTime) {
      time_t t = (time_t)LastCheckTime;
      struct tm *tm = localtime(&t);
      if (tm) {
        char buf[64];
        strftime(buf, sizeof(buf), "Last: %H:%M", tm);
        gtk_label_set_text(GTK_LABEL(tp_last_check), buf);
      } else {
        gtk_label_set_text(GTK_LABEL(tp_last_check), "Last: —");
      }
    } else {
      gtk_label_set_text(GTK_LABEL(tp_last_check), "Last: Never");
    }
  }

  /* Overall status */
  if (tp_status_label) {
    if (CheckThreadRunning && SendThreadRunning)
      gtk_label_set_text(GTK_LABEL(tp_status_label), "Checking & Sending");
    else if (CheckThreadRunning)
      gtk_label_set_text(GTK_LABEL(tp_status_label), "Checking Mail...");
    else if (SendThreadRunning)
      gtk_label_set_text(GTK_LABEL(tp_status_label), "Sending Mail...");
    else {
      /* Count errors */
      int errs = 0;
      for (taskErrHandle e = TaskErrorList; e; e = e->next) errs++;
      if (errs > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d error%s", errs, errs > 1 ? "s" : "");
        gtk_label_set_text(GTK_LABEL(tp_status_label), buf);
      } else {
        gtk_label_set_text(GTK_LABEL(tp_status_label), "Idle");
      }
    }
  }
}

/* Update the standalone window title to reflect current activity */
static void tp_set_title(void) {
  if (!TaskProgressWindow || !TaskProgressWindow->window)
    return;
  if (!GTK_IS_WINDOW(TaskProgressWindow->window))
    return;

  if (CheckThreadRunning && SendThreadRunning)
    gtk_window_set_title(GTK_WINDOW(TaskProgressWindow->window),
                         "Tasks - Checking, Sending");
  else if (CheckThreadRunning)
    gtk_window_set_title(GTK_WINDOW(TaskProgressWindow->window),
                         "Tasks - Checking");
  else if (SendThreadRunning)
    gtk_window_set_title(GTK_WINDOW(TaskProgressWindow->window),
                         "Tasks - Sending");
  else
    gtk_window_set_title(GTK_WINDOW(TaskProgressWindow->window), "Tasks");
}

/* Periodic idle callback — refreshes status labels and title */
static gboolean tp_idle_cb(gpointer user_data) {
  (void)user_data;

  if (!tp_widget)
    return G_SOURCE_REMOVE;

  tp_update_status();
  tp_set_title();

  /* Detect state changes for auto-close */
  if (tp_last_check_running && !CheckThreadRunning)
    tp_last_check_running = false;
  if (tp_last_send_running && !SendThreadRunning)
    tp_last_send_running = false;

  /* Auto-close standalone window when idle and no errors */
  if (!CheckThreadRunning && !SendThreadRunning && !TaskErrorList &&
      !TaskDontAutoClose && TaskProgressWindow) {
    GtkWidget *notebook = gtk_widget_get_ancestor(tp_widget, GTK_TYPE_NOTEBOOK);
    if (!notebook && TaskProgressWindow->window &&
        GTK_IS_WINDOW(TaskProgressWindow->window)) {
      gtk_window_close(GTK_WINDOW(TaskProgressWindow->window));
    }
  }

  return G_SOURCE_CONTINUE;
}

/* Start the idle timer if not already running */
static void tp_ensure_idle_timer(void) {
  if (tp_idle_timer == 0)
    tp_idle_timer = g_timeout_add_seconds(2, tp_idle_cb, NULL);
}

/* Window close handler */
static void tp_on_close(GtkWidget *window, gpointer user_data) {
  (void)user_data;
  (void)window;

  if (TaskProgressWindow == ProgWindow)
    ProgWindow = NULL;
  TaskProgressWindow = NULL;
  gTaskProgressInitied = false;

  /* Stop idle timer if running */
  if (tp_idle_timer) {
    g_source_remove(tp_idle_timer);
    tp_idle_timer = 0;
  }

  /* Reset widget pointers — they're destroyed with the window */
  tp_widget = NULL;
  tp_task_list = NULL;
  tp_error_list = NULL;
  tp_next_check = NULL;
  tp_last_check = NULL;
  tp_status_label = NULL;
  TaskListHandle = NULL;
}

/* ── Create the embeddable task progress widget ── */

GtkWidget *create_task_progress_widget(void) {
  if (tp_widget) return tp_widget;

  tp_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* ── Status header ── */
  GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(status_box, 6);
  gtk_widget_set_margin_end(status_box, 6);
  gtk_widget_set_margin_top(status_box, 4);
  gtk_widget_set_margin_bottom(status_box, 4);

  tp_status_label = gtk_label_new("Idle");
  PangoAttrList *bold = pango_attr_list_new();
  pango_attr_list_insert(bold, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
  gtk_label_set_attributes(GTK_LABEL(tp_status_label), bold);
  pango_attr_list_unref(bold);
  gtk_widget_set_hexpand(tp_status_label, TRUE);
  gtk_label_set_xalign(GTK_LABEL(tp_status_label), 0);
  gtk_box_append(GTK_BOX(status_box), tp_status_label);

  gtk_box_append(GTK_BOX(tp_widget), status_box);

  /* Check time row */
  GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_set_margin_start(time_box, 6);
  gtk_widget_set_margin_end(time_box, 6);
  gtk_widget_set_margin_bottom(time_box, 4);

  tp_last_check = gtk_label_new("Last: Never");
  gtk_label_set_xalign(GTK_LABEL(tp_last_check), 0);
  gtk_widget_add_css_class(tp_last_check, "dim-label");
  gtk_box_append(GTK_BOX(time_box), tp_last_check);

  tp_next_check = gtk_label_new("Next: Not scheduled");
  gtk_label_set_xalign(GTK_LABEL(tp_next_check), 0);
  gtk_widget_add_css_class(tp_next_check, "dim-label");
  gtk_box_append(GTK_BOX(time_box), tp_next_check);

  gtk_box_append(GTK_BOX(tp_widget), time_box);

  gtk_box_append(GTK_BOX(tp_widget), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  /* ── Active tasks list ── */
  GtkWidget *tasks_label = gtk_label_new("Active Tasks");
  gtk_label_set_xalign(GTK_LABEL(tasks_label), 0);
  gtk_widget_set_margin_start(tasks_label, 6);
  gtk_widget_set_margin_top(tasks_label, 4);
  gtk_widget_add_css_class(tasks_label, "dim-label");
  gtk_box_append(GTK_BOX(tp_widget), tasks_label);

  tp_task_list = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(tp_task_list), GTK_SELECTION_NONE);
  gtk_list_box_set_placeholder(GTK_LIST_BOX(tp_task_list),
                               gtk_label_new("No active tasks"));

  GtkWidget *task_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(task_scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(task_scroll), tp_task_list);
  gtk_widget_set_vexpand(task_scroll, TRUE);
  gtk_box_append(GTK_BOX(tp_widget), task_scroll);

  gtk_box_append(GTK_BOX(tp_widget), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

  /* ── Errors list ── */
  GtkWidget *errors_label = gtk_label_new("Errors");
  gtk_label_set_xalign(GTK_LABEL(errors_label), 0);
  gtk_widget_set_margin_start(errors_label, 6);
  gtk_widget_set_margin_top(errors_label, 4);
  gtk_widget_add_css_class(errors_label, "dim-label");
  gtk_box_append(GTK_BOX(tp_widget), errors_label);

  tp_error_list = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(tp_error_list), GTK_SELECTION_NONE);
  gtk_list_box_set_placeholder(GTK_LIST_BOX(tp_error_list),
                               gtk_label_new("No errors"));

  GtkWidget *error_scroll = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(error_scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(error_scroll), tp_error_list);
  gtk_widget_set_vexpand(error_scroll, TRUE);
  gtk_box_append(GTK_BOX(tp_widget), error_scroll);

  /* Set TaskListHandle for legacy callers */
  TaskListHandle = tp_task_list;

  tp_update_status();
  return tp_widget;
}

/* ── Progress bar rect stubs (legacy API, now unused) ── */

void InitPrbl(ProgressBlock **prbl, short vert, ControlHandle *stopButton) {
  (void)prbl; (void)vert; (void)stopButton;
}

void InvalTPRect(Rect *invalRect) { (void)invalRect; }
void DrawTaskProgressBar(ControlHandle bar) { (void)bar; }

/* ── Open/close the tasks window ── */

void OpenTasksWin(void) {
  OpenTasksWinBehind(NULL);
  NewError = false;
}

void OpenTasksWinBehind(void *behind) {
  (void)behind;

  /* If widget already exists in wazoo, just show it */
  if (tp_widget && GTK_IS_WIDGET(tp_widget)) {
    /* Try to find and focus the wazoo tab */
    GtkWidget *notebook = gtk_widget_get_ancestor(tp_widget, GTK_TYPE_NOTEBOOK);
    if (notebook) {
      int page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), tp_widget);
      if (page >= 0)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
      return;
    }
  }

  /* Standalone window fallback */
  if (TaskProgressWindow) {
    if (GTK_IS_WIDGET(TaskProgressWindow->window))
      gtk_window_present(GTK_WINDOW(TaskProgressWindow->window));
    return;
  }

  MyWindowPtr win = (MyWindowPtr)g_malloc0(sizeof(MyWindow));
  TaskProgressWindow = win;

  GtkWidget *gtkWin = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(gtkWin), "Tasks");
  gtk_window_set_default_size(GTK_WINDOW(gtkWin), 350, 400);
  win->window = gtkWin;
  g_object_set_data(G_OBJECT(gtkWin), "my-window-ptr", win);

  g_signal_connect(gtkWin, "destroy", G_CALLBACK(tp_on_close), NULL);

  GtkWidget *content = create_task_progress_widget();
  gtk_window_set_child(GTK_WINDOW(gtkWin), content);
  gtk_window_present(GTK_WINDOW(gtkWin));

  TaskDontAutoClose = !PrefIsSet(PREF_TASK_PROGRESS_AUTO);
  gTaskProgressInitied = true;
  tp_ensure_idle_timer();
  tp_set_title();

  if (InAThread())
    ProgWindow = TaskProgressWindow;
}

void OpenTasksWinErrors(void) {
  OpenTasksWin();
  /* Scroll to errors section if present */
  if (tp_error_list && tp_widget) {
    GtkWidget *notebook = gtk_widget_get_ancestor(tp_widget, GTK_TYPE_NOTEBOOK);
    if (notebook) {
      int page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), tp_widget);
      if (page >= 0)
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page);
    }
  }
}

/* ── Stop button callback ── */

static void on_stop_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  threadDataHandle td = (threadDataHandle)user_data;
  if (td)
    SetThreadGlobalCommandPeriod(td->threadID, true);
}

/* ── Add/remove active tasks ── */

int AddProgressTask(threadDataHandle threadData) {
  if (!tp_task_list || !GTK_IS_WIDGET(tp_task_list))
    return -1;

  /* Create a row: title + subtitle + progress bar */
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_margin_start(vbox, 6);
  gtk_widget_set_margin_end(vbox, 6);
  gtk_widget_set_margin_top(vbox, 4);
  gtk_widget_set_margin_bottom(vbox, 4);

  /* Title label */
  GtkWidget *title = gtk_label_new("Working...");
  gtk_label_set_xalign(GTK_LABEL(title), 0);
  PangoAttrList *bold = pango_attr_list_new();
  pango_attr_list_insert(bold, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
  gtk_label_set_attributes(GTK_LABEL(title), bold);
  pango_attr_list_unref(bold);
  gtk_box_append(GTK_BOX(vbox), title);

  /* Subtitle label */
  GtkWidget *subtitle = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(subtitle), 0);
  gtk_widget_add_css_class(subtitle, "dim-label");
  gtk_box_append(GTK_BOX(vbox), subtitle);

  /* Message label */
  GtkWidget *message = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(message), 0);
  gtk_widget_add_css_class(message, "dim-label");
  gtk_box_append(GTK_BOX(vbox), message);

  /* Progress bar + stop button in a row */
  GtkWidget *prog_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  GtkWidget *pbar = gtk_progress_bar_new();
  gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(pbar), FALSE);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pbar));
  gtk_widget_set_hexpand(pbar, TRUE);
  gtk_widget_set_valign(pbar, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(prog_row), pbar);

  GtkWidget *stop_btn = gtk_button_new_with_label("Stop");
  gtk_widget_add_css_class(stop_btn, "destructive-action");
  g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_clicked), threadData);
  gtk_box_append(GTK_BOX(prog_row), stop_btn);

  gtk_box_append(GTK_BOX(vbox), prog_row);

  gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), vbox);

  /* Store the threadData handle so we can match for removal */
  g_object_set_data(G_OBJECT(row), "thread-data", threadData);
  g_object_set_data(G_OBJECT(row), "title-label", title);
  g_object_set_data(G_OBJECT(row), "subtitle-label", subtitle);
  g_object_set_data(G_OBJECT(row), "message-label", message);
  g_object_set_data(G_OBJECT(row), "progress-bar", pbar);

  gtk_list_box_append(GTK_LIST_BOX(tp_task_list), row);
  tp_update_status();
  tp_set_title();
  tp_ensure_idle_timer();
  return 0;
}

static gboolean remove_progress_task_idle(gpointer data) {
  threadDataHandle threadData = (threadDataHandle)data;
  if (!tp_task_list || !GTK_IS_WIDGET(tp_task_list))
    return G_SOURCE_REMOVE;

  GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(tp_task_list));
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    if (GTK_IS_LIST_BOX_ROW(child)) {
      void *d = g_object_get_data(G_OBJECT(child), "thread-data");
      if (d == (void *)threadData) {
        gtk_list_box_remove(GTK_LIST_BOX(tp_task_list), child);
        break;
      }
    }
    child = next;
  }
  tp_update_status();
  return G_SOURCE_REMOVE;
}

void RemoveProgressTask(threadDataHandle threadData) {
  if (InAThread()) {
    g_idle_add(remove_progress_task_idle, threadData);
    return;
  }
  if (!tp_task_list || !GTK_IS_WIDGET(tp_task_list))
    return;

  GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(tp_task_list));
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    if (GTK_IS_LIST_BOX_ROW(child)) {
      void *data = g_object_get_data(G_OBJECT(child), "thread-data");
      if (data == (void *)threadData) {
        gtk_list_box_remove(GTK_LIST_BOX(tp_task_list), child);
        break;
      }
    }
    child = next;
  }
  tp_update_status();
  tp_set_title();
}

/* --- Thread-safe wrappers for GTK progress updates --- */

typedef struct {
  int percent;
  int remaining;
} ProgressData;

static gboolean update_progress_idle(gpointer data) {
  ProgressData *pd = (ProgressData *)data;
  if (!tp_task_list || !GTK_IS_WIDGET(tp_task_list))
    goto done;

  GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(tp_task_list));
  while (child) {
    if (GTK_IS_LIST_BOX_ROW(child)) {
      GtkWidget *pbar = g_object_get_data(G_OBJECT(child), "progress-bar");
      if (pbar && GTK_IS_PROGRESS_BAR(pbar)) {
        if (pd->percent < 0)
          gtk_progress_bar_pulse(GTK_PROGRESS_BAR(pbar));
        else
          gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar),
                                        pd->percent / 100.0);
      }
      break;
    }
    child = gtk_widget_get_next_sibling(child);
  }
done:
  g_free(pd);
  return G_SOURCE_REMOVE;
}

void UpdateTaskProgress(int percent, int remaining) {
  ProgressData *pd = g_new(ProgressData, 1);
  pd->percent = percent;
  pd->remaining = remaining;
  g_idle_add(update_progress_idle, pd);
}

typedef struct {
  char *title;
  char *subtitle;
  char *message;
} MessageData;

static gboolean update_message_idle(gpointer data) {
  MessageData *md = (MessageData *)data;
  if (!tp_task_list || !GTK_IS_WIDGET(tp_task_list))
    goto done;

  GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(tp_task_list));
  while (child) {
    if (GTK_IS_LIST_BOX_ROW(child)) {
      if (md->title) {
        GtkWidget *lbl = g_object_get_data(G_OBJECT(child), "title-label");
        if (lbl && GTK_IS_LABEL(lbl))
          gtk_label_set_text(GTK_LABEL(lbl), md->title);
      }
      if (md->subtitle) {
        GtkWidget *lbl = g_object_get_data(G_OBJECT(child), "subtitle-label");
        if (lbl && GTK_IS_LABEL(lbl))
          gtk_label_set_text(GTK_LABEL(lbl), md->subtitle);
      }
      if (md->message) {
        GtkWidget *lbl = g_object_get_data(G_OBJECT(child), "message-label");
        if (lbl && GTK_IS_LABEL(lbl))
          gtk_label_set_text(GTK_LABEL(lbl), md->message);
      }
      break;
    }
    child = gtk_widget_get_next_sibling(child);
  }
  tp_update_status();
done:
  g_free(md->title);
  g_free(md->subtitle);
  g_free(md->message);
  g_free(md);
  return G_SOURCE_REMOVE;
}

void UpdateTaskMessage(const char *title_text, const char *subtitle_text, const char *message_text) {
  MessageData *md = g_new0(MessageData, 1);
  md->title = title_text ? g_strdup(title_text) : NULL;
  md->subtitle = subtitle_text ? g_strdup(subtitle_text) : NULL;
  md->message = message_text ? g_strdup(message_text) : NULL;
  g_idle_add(update_message_idle, md);
}

/* ── Filter task (batch delivery) ── */

/* batch delivery is always on — filter task stubs not needed */

/* ── Error management ── */

static gboolean add_task_errors_ui_idle(gpointer data) {
  taskErrHandle taskErrs = (taskErrHandle)data;

  if (tp_error_list && GTK_IS_WIDGET(tp_error_list)) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_start(vbox, 6);
    gtk_widget_set_margin_end(vbox, 6);
    gtk_widget_set_margin_top(vbox, 4);
    gtk_widget_set_margin_bottom(vbox, 4);

    /* Error title in red */
    char *markup = g_markup_printf_escaped(
        "<span color='#CC0000' weight='bold'>%s</span>",
        (const char *)taskErrs->taskDesc);
    GtkWidget *title_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_lbl), markup);
    g_free(markup);
    gtk_label_set_xalign(GTK_LABEL(title_lbl), 0);
    gtk_box_append(GTK_BOX(vbox), title_lbl);

    /* Error message */
    if (taskErrs->errMess[0]) {
      GtkWidget *err_lbl = gtk_label_new((const char *)taskErrs->errMess);
      gtk_label_set_xalign(GTK_LABEL(err_lbl), 0);
      gtk_label_set_wrap(GTK_LABEL(err_lbl), TRUE);
      gtk_box_append(GTK_BOX(vbox), err_lbl);
    }

    /* Explanation */
    if (taskErrs->errExplanation[0]) {
      GtkWidget *exp_lbl = gtk_label_new((const char *)taskErrs->errExplanation);
      gtk_label_set_xalign(GTK_LABEL(exp_lbl), 0);
      gtk_label_set_wrap(GTK_LABEL(exp_lbl), TRUE);
      gtk_widget_add_css_class(exp_lbl, "dim-label");
      gtk_box_append(GTK_BOX(vbox), exp_lbl);
    }

    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), vbox);
    g_object_set_data(G_OBJECT(row), "task-err-handle", taskErrs);
    gtk_list_box_prepend(GTK_LIST_BOX(tp_error_list), row);
  }

  tp_update_status();
  return G_SOURCE_REMOVE;
}

int AddTaskErrorsS(const char *error, const char *explanation,
                   TaskKindEnum taskKind, long persId) {
  int err = 0;
  taskErrHandle taskErrs;

  RemoveTaskErrors(taskKind, persId);
  ComposeLogS(LOG_ALRT, NULL, (unsigned char *)"%p %p", error, explanation);
  NewError = true;

  taskErrs = (taskErrHandle)g_malloc0(sizeof(taskErrData));
  if (!taskErrs) {
    CurThreadGlobals->tCommandPeriod = true;
    if (taskKind == CheckingTask) CheckThreadError = -108;
    else if (taskKind == SendingTask) SendThreadError = -108;
    return -108;
  }

  taskErrs->taskKind = taskKind;
  taskErrs->persId = persId;
  taskErrs->next = NULL;

  /* Store error/explanation as C strings */
  if (error)
    g_strlcpy((char *)taskErrs->errMess, error, sizeof(taskErrs->errMess));
  if (explanation)
    g_strlcpy((char *)taskErrs->errExplanation, explanation,
              sizeof(taskErrs->errExplanation));

  /* Build task description with personality name */
  const char *kindStr = task_kind_label(taskKind);
  PersHandle pers = FindPersById(persId);
  if (pers)
    snprintf((char *)taskErrs->taskDesc, sizeof(taskErrs->taskDesc),
             "%s for %s", kindStr, spec_name(pers));
  else
    g_strlcpy((char *)taskErrs->taskDesc, kindStr,
              sizeof(taskErrs->taskDesc));

  /* Add to error linked list */
  LL_Queue(TaskErrorList, taskErrs, (taskErrHandle));

  /* Add error row to UI — must happen on main thread */
  if (InAThread())
    g_idle_add(add_task_errors_ui_idle, taskErrs);
  else
    add_task_errors_ui_idle(taskErrs);

  return err;
}

int TPAddHelpButton(taskErrHandle taskErrs) {
  (void)taskErrs;
  return 0;
}

typedef struct {
  TaskKindEnum taskKind;
  long persId;
} RemoveTaskErrorsData;

static gboolean remove_task_errors_idle(gpointer data) {
  RemoveTaskErrorsData *rd = (RemoveTaskErrorsData *)data;
  taskErrHandle current, last, temp;

  for (last = current = TaskErrorList; current;) {
    temp = current;
    current = current->next;
    if ((temp->taskKind == rd->taskKind) &&
        ((temp->persId == rd->persId) || (rd->persId == -1))) {
      /* Unlink from list */
      if (temp == TaskErrorList)
        TaskErrorList = temp->next;
      else
        last->next = temp->next;

      /* Remove from UI */
      if (tp_error_list && GTK_IS_WIDGET(tp_error_list)) {
        GtkWidget *child = gtk_widget_get_first_child(GTK_WIDGET(tp_error_list));
        while (child) {
          GtkWidget *next = gtk_widget_get_next_sibling(child);
          if (GTK_IS_LIST_BOX_ROW(child)) {
            void *d = g_object_get_data(G_OBJECT(child), "task-err-handle");
            if (d == (void *)temp) {
              gtk_list_box_remove(GTK_LIST_BOX(tp_error_list), child);
              break;
            }
          }
          child = next;
        }
      }

      g_free(temp);
    } else {
      last = temp;
    }
  }

  if (!TaskErrorList) NewError = false;
  tp_update_status();
  g_free(rd);
  return G_SOURCE_REMOVE;
}

void RemoveTaskErrors(TaskKindEnum taskKind, long persId) {
  RemoveTaskErrorsData *rd = g_new(RemoveTaskErrorsData, 1);
  rd->taskKind = taskKind;
  rd->persId = persId;
  if (InAThread())
    g_idle_add(remove_task_errors_idle, rd);
  else
    remove_task_errors_idle(rd);
}

/* ── Stubs for legacy callbacks ── */

void TaskProgressRefresh(void) {
  tp_update_status(); /* tp_update_status already handles InAThread */
}

