/* idle_scheduler.h — Centralized background task handler
 *
 * Runs periodically on the GTK main thread via g_timeout_add.
 * Dispatches to macmbx_mailer for mail work, handles UI notifications.
 */

#ifndef IDLE_SCHEDULER_H
#define IDLE_SCHEDULER_H

#include <stdbool.h>

/* Forward declaration — avoid pulling in macmbx_mailer.h everywhere */
typedef struct MacmbxMailer MacmbxMailer;

void IdleSchedulerStart(void);
void IdleSchedulerStop(void);

/* Set the mailer instance used for mail pipeline work.
 * Must be called before IdleSchedulerStart(). */
void idle_scheduler_set_mailer(MacmbxMailer *m);
MacmbxMailer *idle_scheduler_get_mailer(void);

/* Request check/send */
void idle_scheduler_request_send(void);
void idle_scheduler_request_check(void);

/* Offline mode — stops all IDLE sessions and blocks check/send.
 * Does not persist — caller is responsible for saving to prefs. */
void idle_scheduler_set_offline(bool offline);
bool idle_scheduler_is_offline(void);

#endif /* IDLE_SCHEDULER_H */
