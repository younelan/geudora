/* idle_scheduler.h — Centralized background task handler
 *
 * Runs periodically on the GTK main thread via g_timeout_add.
 * Dispatches to macmbx_mailer for mail work, handles UI notifications.
 */

#ifndef IDLE_SCHEDULER_H
#define IDLE_SCHEDULER_H

/* Forward declaration — avoid pulling in macmbx_mailer.h everywhere */
typedef struct MacmbxMailer MacmbxMailer;

void IdleSchedulerStart(void);
void IdleSchedulerStop(void);

/* Set the mailer instance used for mail pipeline work.
 * Must be called before IdleSchedulerStart(). */
void idle_scheduler_set_mailer(MacmbxMailer *m);
MacmbxMailer *idle_scheduler_get_mailer(void);

#endif /* IDLE_SCHEDULER_H */
