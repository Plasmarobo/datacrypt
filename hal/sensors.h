#ifndef __SENSORS_HAL_H_
#define __SENSORS_HAL_H_

#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif
    // ========== Analog Channels ==========
    // External voltage divider
    uint16_t bat_read();
    // Floating analog pin
    uint16_t rng_read();
    // Internal temperature sensor (converted)
    uint16_t temp_read();
    // Internal temperature sensor (raw)
    uint16_t temp_read_raw();
    // Internal vbat
    uint16_t vbat_read();

#ifdef __cplusplus
}
#endif

#endif // __SENSORS_HAL_H_