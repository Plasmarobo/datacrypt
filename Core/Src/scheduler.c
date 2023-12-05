#include "scheduler.h"

#include <stdbool.h>
#include <stddef.h>

#include "bsp.h"

typedef enum {
    TASK_FREE = 0,
    TASK_DISABLED,
    TASK_IMMEDIATE,
    TASK_DELAYED,
    TASK_PERIODIC,
    TASK_TYPE_MAX,
} task_type_t;

typedef struct {
    task_type_t type;
    timespan_t time;
    timespan_t elapsed;
    task_handler_t handler;
} task_data_t;

#define MAX_TASKS (64)

static timespan_t us_accumulator;
static task_data_t tasks[MAX_TASKS];

static task_data_t* get_free_task() {
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if (TASK_FREE == tasks[i].type) {
            return &tasks[i];
        }
    }
    return NULL;
}

// Schedules a task for execution on the next exec cycle
bool task_immediate(task_handler_t handler) {
    task_data_t* slot = get_free_task();
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = 0;
        slot->type = TASK_IMMEDIATE;
        slot->elapsed = 0;
    }
    return NULL != slot;
}

// Schedules a task for periodic execution
bool task_periodic(task_handler_t handler, timespan_t period) {
    task_data_t* slot = get_free_task();
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = period;
        slot->type = TASK_PERIODIC;
        slot->elapsed = 0;
    }
    return NULL != slot;
}

// Schedules a task for execution in the future
bool task_delayed(task_handler_t handler, timespan_t delay) {
    task_data_t* slot = get_free_task();
    if (NULL != slot) {
        slot->handler = handler;
        slot->time = delay;
        slot->type = TASK_DELAYED;
        slot->elapsed = 0;
    }
    return NULL != slot;
}

// Searches for existing handler, overwrites if found
// If duplicates exist, only relpaces the first found
bool task_delayed_unique(task_handler_t handler, timespan_t delay) {
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
    }
    return NULL != slot;
}

void scheduler_init() {
    us_accumulator = 0;
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        tasks[i].handler = NULL;
        tasks[i].time = 0;
        tasks[i].elapsed = 0;
        tasks[i].type = TASK_FREE;
    }
}

void scheduler_exec() {
    timespan_t last_tick = (timespan_t)microseconds();
    timespan_t delta = 0;
    while (1) {
        timespan_t now = (timespan_t)microseconds();
        if (last_tick > now) {
            // Rollover
            delta = now + CORRECT_ROLLOVER(last_tick);
        } else {
            delta = now - last_tick;
        }
        if (delta > 0) {
            last_tick = (timespan_t)microseconds();
            for (uint8_t i = 0; i < MAX_TASKS; ++i) {
                if ((TASK_FREE == tasks[i].type) || !tasks[i].handler) {
                    continue;
                }
                tasks[i].elapsed += delta;
                switch (tasks[i].type) {
                    case TASK_IMMEDIATE:
                        // Invoke and free the task
                        tasks[i].handler(0);
                        tasks[i].type = TASK_FREE;
                        break;
                    case TASK_DELAYED:
                        if (tasks[i].elapsed > tasks[i].time) {
                            // Invoke and free the task
                            tasks[i].handler(0);
                            tasks[i].type = TASK_FREE;
                        }
                        break;
                    case TASK_PERIODIC:
                        if (tasks[i].elapsed > tasks[i].time) {
                            // Invoke, but do not free
                            tasks[i].handler(0);
                            tasks[i].elapsed = 0;
                        }
                        break;
                    case TASK_DISABLED:  // intentional fallthrough
                    case TASK_FREE:      // intentional fallthrough
                    default:
                        break;
                }
            }
        }
    }
}
