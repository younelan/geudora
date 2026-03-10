#ifndef TASK_PROGRESS_H
#define TASK_PROGRESS_H

#include "progress.h"
#include "task_types.h"
#include "threading.h"

typedef struct taskErrData_ taskErrData, *taskErrPtr, **taskErrHandle;

struct taskErrData_ {
  TaskKindEnum taskKind;
  long persId;
  Str255 taskDesc, errMess, errExplanation;
  ControlHandle helpButton;
  taskErrHandle next;
};

OSErr AddFilterTask(void);
void RemoveFilterTask(void);
OSErr AddProgressTask(threadDataHandle threadData);
void RemoveProgressTask(threadDataHandle threadData);
OSErr AddTaskErrorsS(const char *error, const char *explanation,
                     TaskKindEnum taskKind, long persId);
void RemoveTaskErrors(TaskKindEnum taskKind, long persId);
void DrawTaskProgressBar(ControlHandle bar);
void InvalTPRect(Rect *invalRect);
void OpenTasksWin(void);
void OpenTasksWinBehind(void *behind);
void OpenTasksWinErrors(void);
void InitPrbl(ProgressBlock **prbl, short vert, ControlHandle *stopButton);
void TaskProgressRefresh(void);

#endif
