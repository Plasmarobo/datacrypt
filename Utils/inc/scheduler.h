#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"

// What is the maximum slice to execute a thread or task
#define SCHEDULE_TIMESLICE_US (1)

typedef enum {
    TASK_FREE = 0,
    TASK_PENDING_FREE,
    TASK_DISABLED,
    TASK_IMMEDIATE,
    TASK_DELAYED,
    TASK_PERIODIC,
    TASK_TYPE_MAX,
} task_type_t;

typedef struct {
    uint32_t id;
    task_type_t type;
    timespan_t time;
    timespan_t elapsed;
    callback_t handler;
    int32_t status;
} task_data_t;
// Task handles should be used carefully, non-periodic tasks may be receclyed
typedef task_data_t* task_handle_t;

// Schedules a task for execution on the next exec cycle
task_handle_t task_immediate(callback_t task);
task_handle_t task_immediate_signal(callback_t task, int32_t status);
// Schedules a task for periodic execution
task_handle_t task_periodic(callback_t task, timespan_t period);
task_handle_t task_periodic_signal(callback_t task, timespan_t period,
                                   int32_t status);
// Schedules a task for execution in the future
task_handle_t task_delayed(callback_t task, timespan_t delay);
task_handle_t task_delayed_signal(callback_t task, timespan_t delay,
                                  int32_t status);
// Schedules or belays an existing task
task_handle_t task_delayed_unique(callback_t task, timespan_t delay);
task_handle_t task_delayed_unique_signal(callback_t task, timespan_t delay,
                                         int32_t status);

void task_signal(task_handle_t task, int32_t status);
void task_abort(task_handle_t task);

void scheduler_init();
void scheduler_exec();  // Will not return

#ifdef __cplusplus
}
#endif

#endif  // SCHEDULER_H
