#ifndef __GPIO_HAL_H_
#define __GPIO_HAL_H_

#include <stdbool.h>
#include <stdint.h>
#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct gpio_t;
    typedef struct gpio_t gpio_t;
    typedef void (*gpio_change_handler_t)(gpio_t *gpio, int32_t value);

    typedef struct gpio_t
    {
        uint16_t pin;
        void *port;
        bool value;
        gpio_change_handler_t cb;
    } gpio_t;

    // Logical GPIOs
    extern gpio_t BAT_READ;
    extern gpio_t LED_DATA;
    extern gpio_t FLASH_DIN;
    extern gpio_t FLASH_DOUT;
    extern gpio_t LED_SCK;
    extern gpio_t PWM_CH1;
    extern gpio_t PWM_CH2;
    extern gpio_t SUCCESS_SW;
    extern gpio_t FAILURE_SW;
    extern gpio_t INTERCEPT_SW;
    extern gpio_t TIMER_SW;
    extern gpio_t LOCK2_TGL;
    extern gpio_t LOCK3_TGL;
    extern gpio_t FLASH_CS;
    extern gpio_t FLASH_SCK;
    extern gpio_t WIN_SW;
    extern gpio_t SHUFFLE_SW;
    extern gpio_t TURN_SW;
    extern gpio_t SOUND_TGL;
    extern gpio_t MISC_SW;
    extern gpio_t DISPLAY_SDA;
    extern gpio_t HIDE_TGL;
    extern gpio_t AUDIO_SD;
    extern gpio_t TIMER_SW;
    extern gpio_t MUXRST;
    extern gpio_t DISPLAY_SCL;
    extern gpio_t LOCK0_TGL;
    extern gpio_t LOCK1_TGL;
    extern gpio_t ANALOG_RNG;

    // GPIO aliases
#define LEFT_SW (SUCCESS_SW)
#define ACCEPT_SW (FAILURE_SW)
#define CANCEL_SW (INTERCEPT_SW)
#define RIGHT_SW (TIMER_SW)

#define IS_LOCKED(l) (gpio_get(l))

    void gpio_update(int32_t status);
    void gpio_mark_dirty(uint8_t fields);
    uint8_t gpio_get_dirty();
    void gpio_clear_dirty(uint8_t fields);
    void gpio_set(gpio_t *gpio, bool set);
    bool gpio_get(gpio_t *gpio);
    void gpio_set_callback(gpio_t *gpio, gpio_change_handler_t on_change);
    void gpio_change_handler(gpio_t *gpio);

#ifdef __cplusplus
}
#endif

#endif // __GPIO_HAL_H_