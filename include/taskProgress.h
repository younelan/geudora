#ifndef TASK_PROGRESS_H
#define TASK_PROGRESS_H

#include "progress.h"
#include "task_types.h"
#include "threading.h"
#include <gtk/gtk.h>

typedef struct taskErrData_ taskErrData, *taskErrPtr;
typedef taskErrData *taskErrHandle;

struct taskErrData_ {
  TaskKindEnum taskKind;
  long persId;
  char taskDesc[256], errMess[256], errExplanation[256];
  ControlHandle helpButton;
  taskErrHandle next;
};

/* Create the embeddable task progress widget for wazoo tab */
GtkWidget *create_task_progress_widget(void);

int AddFilterTask(void);
void RemoveFilterTask(void);
int AddProgressTask(threadDataHandle threadData);
void RemoveProgressTask(threadDataHandle threadData);
int AddTaskErrorsS(const char *error, const char *explanation,
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
int TPAddHelpButton(taskErrHandle taskErrs);

#endif
