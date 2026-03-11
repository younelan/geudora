#ifndef TASK_PROGRESS_H
#define TASK_PROGRESS_H

#include "progress.h"
#include "task_types.h"
#include "threading.h"
#include <gtk/gtk.h>

typedef struct taskErrData_ taskErrData, *taskErrPtr, **taskErrHandle;

struct taskErrData_ {
  TaskKindEnum taskKind;
  long persId;
  Str255 taskDesc, errMess, errExplanation;
  ControlHandle helpButton;
  taskErrHandle next;
};

/* Create the embeddable task progress widget for wazoo tab */
GtkWidget *create_task_progress_widget(void);

OSErr AddFilterTask(void);
void RemoveFilterTask(void);
OSErr AddProgressTask(threadDataHandle threadData);
void RemoveProgressTask(threadDataHandle threadData);
OSErr AddTaskErrorsS(const char *error, const char *explanation,
                     TaskKindEnum taskKind, long persId);
void RemoveTaskErrors(TaskKindEnum taskKind, long persId);
void UpdateTaskProgress(int percent, int remaining);
void UpdateTaskMessage(const char *title, const char *subtitle, const char *message);
void DrawTaskProgressBar(ControlHandle bar);
void InvalTPRect(Rect *invalRect);
void OpenTasksWin(void);
void OpenTasksWinBehind(void *behind);
void OpenTasksWinErrors(void);
void InitPrbl(ProgressBlock **prbl, short vert, ControlHandle *stopButton);
void TaskProgressRefresh(void);

#endif
