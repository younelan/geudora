/* idle_scheduler.h — Centralized background task handler
 *
 * Replaces Mac's FilterXferMessages() from filtthread.c.
 * Runs periodically on the GTK main thread via g_timeout_add.
 */

#ifndef IDLE_SCHEDULER_H
#define IDLE_SCHEDULER_H

void IdleSchedulerStart(void);
void IdleSchedulerStop(void);

#endif /* IDLE_SCHEDULER_H */
