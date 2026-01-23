#include "defs.h"
#include "gpio.h"

gpio_t BAT_READ = {0, NULL, 0, NULL};
gpio_t LED_DATA = {1, NULL, 0, NULL};
gpio_t FLASH_DIN = {2, NULL, 0, NULL};
gpio_t FLASH_DOUT = {3, NULL, 0, NULL};
gpio_t LED_SCK = {4, NULL, 0, NULL};
gpio_t PWM_CH1 = {5, NULL, 0, NULL};
gpio_t PWM_CH2 = {6, NULL, 0, NULL};
gpio_t SUCCESS_SW = {7, NULL, 0, NULL};
gpio_t FAILURE_SW = {8, NULL, 0, NULL};
gpio_t INTERCEPT_SW = {9, NULL, 0, NULL};
gpio_t LOCK2_TGL = {10, NULL, 0, NULL};
gpio_t LOCK3_TGL = {11, NULL, 0, NULL};
gpio_t FLASH_CS = {12, NULL, 0, NULL};
gpio_t FLASH_SCK = {13, NULL, 0, NULL};
gpio_t WIN_SW = {14, NULL, 0, NULL};
gpio_t SHUFFLE_SW = {15, NULL, 0, NULL};
gpio_t TURN_SW = {16, NULL, 0, NULL};
gpio_t SOUND_TGL = {17, NULL, 0, NULL};
gpio_t MISC_SW = {18, NULL, 0, NULL};
gpio_t DISPLAY_SDA = {19, NULL, 0, NULL};
gpio_t HIDE_TGL = {20, NULL, 0, NULL};
gpio_t AUDIO_SD = {21, NULL, 0, NULL};
gpio_t TIMER_SW = {22, NULL, 0, NULL};
gpio_t MUXRST = {23, NULL, 0, NULL};
gpio_t DISPLAY_SCL = {24, NULL, 0, NULL};
gpio_t LOCK0_TGL = {25, NULL, 0, NULL};
gpio_t LOCK1_TGL = {26, NULL, 0, NULL};
gpio_t ANALOG_RNG = {27, NULL, 0, NULL};

const uint32_t PRESSED = 0;

// Output device -> host
void gpio_set(gpio_t *gpio, bool set)
{
    gpio->value = set;
}

// Input device <- host
bool gpio_get(gpio_t *gpio)
{
    return gpio->value;
}

void gpio_set_callback(gpio_t *gpio, gpio_change_handler_t on_change)
{
    gpio->cb = on_change;
}

void gpio_host_set(gpio_t &gpio, bool value)
{
    if (gpio.value != value)
    {
        gpio.value = value;
        if (value == PRESSED && gpio.cb != NULL)
        {
            gpio.cb(&gpio, gpio.value ? 1 : 0);
        }
    }
}

uint32_t gpio_host_get(gpio_t &gpio)
{
    uint32_t state = gpio.value ? 1 : 0;
    return state;
}
// Unused
void gpio_change_handler(gpio_t gpio)
{
    UNUSED(gpio);
}
void gpio_update(int32_t status)
{
    UNUSED(status);
}
void gpio_mark_dirty(uint8_t fields)
{
    UNUSED(fields);
}
uint8_t gpio_get_dirty()
{
    return 0;
}
void gpio_clear_dirty(uint8_t fields)
{
    UNUSED(fields);
}