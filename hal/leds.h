#ifndef __LEDS_HAL_H_
#define __LEDS_HAL_H_

#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

// ========== LED write ==========
#define DISPLED_STATUS_OFFSET (0)
#define DISPLED_STATUS_LENGTH (4)
#define COUNTER_0_OFFSET (4)
#define COUNTER_1_OFFSET (8)
#define COUNTER_LENGTH (4)
#define TIMER_OFFSET (12)
#define TIMER_LENGTH (6)
#define LED_COUNT (18)

    typedef struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } color_t;

    void leds_init();

    // No callback, this should freerun
    void leds_write(void);
    bool leds_busy(void);
    void leds_tx_complete_handler(int32_t status);
    void leds_error_handler(int32_t status);

    void leds_set(uint32_t offset, uint8_t size, color_t *color);
    // convenience functions
    void set_displed(color_t color[DISPLED_STATUS_LENGTH]);
    void set_counter0(color_t color[COUNTER_LENGTH]);
    void set_counter1(color_t color[COUNTER_LENGTH]);
    void set_timer(color_t color[TIMER_LENGTH]);

#ifdef __cplusplus
}
#endif

#endif // __LEDS_HAL_H_