#include "scheduler.h"

#include <stddef.h>
#include <string.h>

#include "defs.h"

#define MAX_TASKS (64)
#define EXEC_DEPTH_LIMIT (4)

static timespan_t last_tick;
static uint8_t exec_depth;
static task_data_t tasks[MAX_TASKS];
static volatile callback_t future_waiting;
static volatile int32_t future_status;

static task_data_t* get_free_task() {
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if (TASK_FREE == tasks[i].type) {
            return &tasks[i];
        }
    }

    return NULL;
}

// Schedules a task for execution on the next exec cycle
task_handle_t task_immediate(callback_t handler) {
    return task_immediate_signal(handler, 0);
}
task_handle_t task_immediate_signal(callback_t handler, int32_t status) {
    enter_critical();
    task_data_t* slot = get_free_task();
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = 0;
        slot->type = TASK_IMMEDIATE;
        slot->elapsed = 0;
        slot->status = status;
    }
    exit_critical();
    return slot;
}

// Schedules a task for periodic execution
task_handle_t task_periodic(callback_t handler, timespan_t period) {
    return task_periodic_signal(handler, period, 0);
}

task_handle_t task_periodic_signal(callback_t handler, timespan_t period,
                                   int32_t status) {
    enter_critical();
    task_data_t* slot = get_free_task();
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = period;
        slot->type = TASK_PERIODIC;
        slot->elapsed = 0;
        slot->status = status;
    }
    exit_critical();
    return slot;
}

// Schedules a task for execution in the future
task_handle_t task_delayed(callback_t handler, timespan_t delay) {
    return task_delayed_signal(handler, delay, 0);
}

task_handle_t task_delayed_signal(callback_t handler, timespan_t delay,
                                  int32_t status) {
    enter_critical();
    task_data_t* slot = get_free_task();
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = delay;
        slot->type = TASK_DELAYED;
        slot->elapsed = 0;
        slot->status = status;
    }
    exit_critical();
    return slot;
}

// Searches for existing handler, overwrites if found
// If duplicates exist, only relpaces the first found
task_handle_t task_immediate_unique(callback_t handler) {
    enter_critical();
    task_data_t* slot = NULL;
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if ((handler == tasks[i].handler) && (TASK_FREE != tasks[i].type)) {
            // Overwrite
            slot = &tasks[i];
            break;
        } else if ((NULL == slot) && (TASK_FREE == tasks[i].type)) {
            slot = &tasks[i];
        }
    }
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = 0;
        slot->type = TASK_IMMEDIATE;
        slot->elapsed = 0;
        slot->status = 0;
    }
    exit_critical();
    return slot;
}

task_handle_t task_delayed_unique(callback_t handler, timespan_t delay) {
    return task_delayed_unique_signal(handler, delay, 0);
}

task_handle_t task_delayed_unique_signal(callback_t handler, timespan_t delay,
                                         int32_t status) {
    enter_critical();
    task_data_t* slot = NULL;
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if ((handler == tasks[i].handler) && (TASK_FREE != tasks[i].type)) {
            // Overwrite
            slot = &tasks[i];
            break;
        } else if ((NULL == slot) && (TASK_FREE == tasks[i].type)) {
            slot = &tasks[i];
        }
    }
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = delay;
        slot->type = TASK_DELAYED;
        slot->elapsed = 0;
        slot->status = status;
    }
    exit_critical();
    return slot;
}

void task_signal(task_handle_t task, int32_t status) {
    enter_critical();
    if (NULL != task) {
        task->status = status;
    }
    exit_critical();
}

void task_abort(task_handle_t task) {
    enter_critical();
    if (NULL != task) {
        task->type = TASK_FREE;
        task->handler = NULL;
        task->time = 0;
        task->elapsed = 0;
        task->status = 0;
    }
    exit_critical();
}

void scheduler_init() {
    exec_depth = 0;
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        tasks[i].handler = NULL;
        tasks[i].time = 0;
        tasks[i].elapsed = 0;
        tasks[i].type = TASK_FREE;
        tasks[i].status = 0;
    }
    last_tick = microseconds();
    future_waiting = NULL;
}

void scheduler_exec() {
    exec_depth++;
    if (exec_depth < EXEC_DEPTH_LIMIT) {
        timespan_t delta = 0;
        timespan_t now = microseconds();
        if (last_tick > now) {
            // Rollover,
            delta = now;
        } else {
            delta = now - last_tick;
        }
        // Minimum time slice is 1us
        last_tick = now;
        for (uint8_t i = 0; i < MAX_TASKS; ++i) {
            if ((TASK_DISABLED >= tasks[i].type) || !tasks[i].handler) {
                continue;
            }
            tasks[i].elapsed += delta;
            switch (tasks[i].type) {
                case TASK_IMMEDIATE:
                    // Invoke and free the task
                    tasks[i].type = TASK_PENDING_FREE;
                    tasks[i].handler(tasks[i].status);
                    break;
                case TASK_DELAYED:
                    if (tasks[i].elapsed >= tasks[i].time) {
                        // Invoke and free the task
                        tasks[i].type = TASK_PENDING_FREE;
                        tasks[i].handler(tasks[i].status);
                    }
                    break;
                case TASK_PERIODIC:
                    if (tasks[i].elapsed >= tasks[i].time) {
                        // Invoke, but do not free
                        tasks[i].elapsed = 0;
                        tasks[i].handler(tasks[i].status);
                    }
                    break;
                case TASK_DISABLED:      // intentional fallthrough
                case TASK_PENDING_FREE:  // intentional fallthrough
                case TASK_FREE:          // intentional fallthrough
                default:
                    break;
            }
        }
    }
    exec_depth--;
    // Free pending tasks
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].type == TASK_PENDING_FREE) {
            tasks[i].type = TASK_FREE;
        }
    }
}

void scheduler_freerun() {
    while (1) {
        scheduler_exec();
    }
}

// Handles resolving callback
static void future_resolve(int32_t status)
{
    future_waiting = NULL;
    future_status = status;
}

// Gives us a callback to pass to a nonblocking op
// Allows tasks to run while we wait, limited to 1 parallel future
callback_t future_get()
{
    if (NULL != future_waiting)
    {
        return NULL;
    }
    future_status = 0;
    future_waiting = &future_resolve;
    return future_waiting;
}

// Spins until the callback resolves, moves status back into blocking context
int32_t future_await(callback_t awaited_future, timespan_t timeout)
{
    if (NULL == awaited_future) {
        // Protect against nesting futures
        error_handler();
    }
    // We only support ONE future to bound the stack/scheduler depth
    timespan_t start = microseconds();
    while(NULL != future_waiting)
    {
        scheduler_exec();
        timespan_t delta = (microseconds() - start);
        if (delta > timeout) {
            future_resolve(FUTURE_TIMEOUT);
            break;
        }
    }
    return future_status;
}
