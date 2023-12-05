#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

// What is the maximum slice to execute a thread or task
#define SCHEDULE_TIMESLICE_US (1)

typedef void (*task_handler_t)(int32_t);
// Schedules a task for execution on the next exec cycle
bool task_immediate(task_handler_t task);
// Schedules a task for periodic execution
bool task_periodic(task_handler_t task, timespan_t period);
// Schedules a task for execution in the future
bool task_delayed(task_handler_t task, timespan_t delay);
// Schedules or belays an existing task
void task_delayed_unique(task_handler_t task, timespan_t delay);

void scheduler_init();
void scheduler_exec();  // Will not return

#endif  // SCHEDULER_H
