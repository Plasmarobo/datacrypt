#include "defs.h"
#include "leds.h"

#include <array>
#include <iostream>
#include <iterator>
#include <algorithm>

static std::array<color_t, LED_COUNT> _leds;

void leds_init()
{
    // Display element
}

// No callback, this should freerun
void leds_write(void)
{
    // Copy to display element
}
bool leds_busy(void)
{
    return false;
}
void leds_tx_complete_handler(int32_t status)
{
    UNUSED(status);
}
void leds_error_handler(int32_t status)
{
    std::cerr << "LED Error: " << status << std::endl;
}

void leds_set(uint32_t offset, uint8_t size, color_t *color)
{
    std::copy(color, color + size, _leds.begin() + offset);
}
// convenience functions
void set_displed(color_t color[DISPLED_STATUS_LENGTH])
{
    leds_set(DISPLED_STATUS_OFFSET, DISPLED_STATUS_LENGTH, color);
}
void set_counter0(color_t color[COUNTER_LENGTH])
{
    leds_set(COUNTER_0_OFFSET, COUNTER_LENGTH, color);
}
void set_counter1(color_t color[COUNTER_LENGTH])
{
    leds_set(COUNTER_1_OFFSET, COUNTER_LENGTH, color);
}
void set_timer(color_t color[TIMER_LENGTH])
{
    leds_set(TIMER_OFFSET, TIMER_LENGTH, color);
}

color_t *_host_led_get()
{
    return _leds.data();
}