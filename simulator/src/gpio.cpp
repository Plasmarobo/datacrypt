#include "defs.h"
#include "gpio.h"

gpio_t BAT_READ = {0, NULL};
gpio_t LED_DATA = {1, NULL};
gpio_t FLASH_DIN = {2, NULL};
gpio_t FLASH_DOUT = {3, NULL};
gpio_t LED_SCK = {4, NULL};
gpio_t PWM_CH1 = {5, NULL};
gpio_t PWM_CH2 = {6, NULL};
gpio_t SUCCESS_SW = {7, NULL};
gpio_t FAILURE_SW = {8, NULL};
gpio_t INTERCEPT_SW = {9, NULL};
gpio_t LOCK2_TGL = {10, NULL};
gpio_t LOCK3_TGL = {11, NULL};
gpio_t FLASH_CS = {12, NULL};
gpio_t FLASH_SCK = {13, NULL};
gpio_t WIN_SW = {14, NULL};
gpio_t SHUFFLE_SW = {15, NULL};
gpio_t TURN_SW = {16, NULL};
gpio_t SOUND_TGL = {17, NULL};
gpio_t MISC_SW = {18, NULL};
gpio_t DISPLAY_SDA = {19, NULL};
gpio_t HIDE_TGL = {20, NULL};
gpio_t AUDIO_SD = {21, NULL};
gpio_t TIMER_SW = {22, NULL};
gpio_t MUXRST = {23, NULL};
gpio_t DISPLAY_SCL = {24, NULL};
gpio_t LOCK0_TGL = {25, NULL};
gpio_t LOCK1_TGL = {26, NULL};
gpio_t ANALOG_RNG = {27, NULL};

static uint32_t gpio_state;
static uint8_t dirty;

void gpio_set(gpio_t gpio, bool set)
{
    uint32_t mask = (1 << gpio.pin);
    if (set)
    {
        gpio_state |= mask;
    }
    else
    {
        gpio_state &= ~mask;
    }
}

bool gpio_get(gpio_t gpio)
{
    return (gpio_state & (1 << gpio.pin)) != 0;
}

void gpio_set_callback(gpio_t gpio, callback_t on_change)
{
    gpio.cb = on_change;
}

// Host hook functions
void _gpio_host_set(uint32_t state)
{
    gpio_state = state;
}
uint32_t _gpio_host_get()
{
    return gpio_state;
}

// Unused
void gpio_change_handler(gpio_t gpio) {}
void gpio_update(int32_t status) {}
void gpio_mark_dirty(uint8_t fields) {}
uint8_t gpio_get_dirty() {}
void gpio_clear_dirty(uint8_t fields) {}