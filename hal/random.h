#ifndef __RANDOM_HAL_H_
#define __RANDOM_HAL_H_

#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // ========== Pseudo RNG ==========
    void random_init();
    // Produce a 16 bit prng number
    uint32_t random_int();
    uint32_t uniform(uint32_t min, uint32_t max);

#ifdef __cplusplus
}
#endif

#endif // __RANDOM_HAL_H_