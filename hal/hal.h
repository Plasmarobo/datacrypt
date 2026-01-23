#ifndef __HAL_H_
#define __HAL_H_

#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Early hardware setup
    void hal_hw_init();
    // Late hardware setup (post-scheduler)
    void hal_task_init();

    void hal_tick();

    timespan_t microseconds();
    timespan_t milliseconds();

#ifdef __cplusplus
}
#endif

#endif // __HAL_H_