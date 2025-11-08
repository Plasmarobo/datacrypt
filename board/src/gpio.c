
#include "gpio.h"
#include "bsp.h"

gpio_t BAT_READ = {GPIO_PIN_0, GPIOA};
gpio_t LED_DATA = {GPIO_PIN_2, GPIOA};
gpio_t FLASH_DIN = {GPIO_PIN_3, GPIOA};
gpio_t FLASH_DOUT = {GPIO_PIN_4, GPIOA};
gpio_t LED_SCK = {GPIO_PIN_5, GPIOA};
gpio_t PWM_CH1 = {GPIO_PIN_6, GPIOA};
gpio_t PWM_CH2 = {GPIO_PIN_7, GPIOA};
gpio_t SUCCESS_SW = {GPIO_PIN_0, GPIOB};
gpio_t FAILURE_SW = {GPIO_PIN_1, GPIOB};
gpio_t INTERCEPT_SW = {GPIO_PIN_2, GPIOB};
gpio_t LOCK2_TGL = {GPIO_PIN_10, GPIOB};
gpio_t LOCK3_TGL = {GPIO_PIN_11, GPIOB};
gpio_t FLASH_CS = {GPIO_PIN_12, GPIOB};
gpio_t FLASH_SCK = {GPIO_PIN_13, GPIOB};
gpio_t WIN_SW = {GPIO_PIN_14, GPIOB};
gpio_t SHUFFLE_SW = {GPIO_PIN_15, GPIOB};
gpio_t TURN_SW = {GPIO_PIN_8, GPIOA};
gpio_t SOUND_TGL = {GPIO_PIN_6, GPIOC};
gpio_t MISC_SW = {GPIO_PIN_7, GPIOC};
gpio_t DISPLAY_SDA = {GPIO_PIN_10, GPIOA};
gpio_t HIDE_TGL = {GPIO_PIN_0, GPIOD};
gpio_t AUDIO_SD = {GPIO_PIN_2, GPIOD};
gpio_t TIMER_SW = {GPIO_PIN_3, GPIOB};
gpio_t MUXRST = {GPIO_PIN_5, GPIOB};
gpio_t DISPLAY_SCL = {GPIO_PIN_6, GPIOB};
gpio_t LOCK0_TGL = {GPIO_PIN_8, GPIOB};
gpio_t LOCK1_TGL = {GPIO_PIN_9, GPIOB};
gpio_t ANALOG_RNG = {GPIO_PIN_1, GPIOA};

#define SUCCESS_SW_EXTI_IRQn EXTI0_1_IRQn
#define FAILURE_SW_EXTI_IRQn EXTI0_1_IRQn
#define INTERCEPT_SW_EXTI_IRQn EXTI2_3_IRQn
#define LOCK2_TGL_EXTI_IRQn EXTI4_15_IRQn
#define LOCK3_TGL_EXTI_IRQn EXTI4_15_IRQn
#define TIMER_SW_EXTI_IRQn EXTI2_3_IRQn
#define LOCK0_TGL_EXTI_IRQn EXTI4_15_IRQn
#define LOCK1_TGL_EXTI_IRQn EXTI4_15_IRQn

static void gpio_event(gpio_t gpio)
{
    bool current = gpio_get(gpio);
    if (gpio.value != current)
    {
        gpio.value = current;
        if (NULL != gpio.cb)
        {
            gpio.cb(gpio.value ? 1 : 0);
        }
    }
}

void gpio_update(int32_t status)
{
    // reset IRQs
    uint8_t irq_flags = gpio_get_dirty();
    if ((0x01 << EXTI0_1_IRQn) & irq_flags)
    {
        gpio_event(SUCCESS_SW);
        gpio_event(FAILURE_SW);
        NVIC_EnableIRQ(EXTI0_1_IRQn);
        irq_flags &= ~EXTI0_1_IRQn;
    }
    if ((0x01 << EXTI2_3_IRQn) & irq_flags)
    {
        gpio_event(INTERCEPT_SW);
        gpio_event(TIMER_SW);
        NVIC_EnableIRQ(EXTI2_3_IRQn);
        irq_flags &= ~EXTI2_3_IRQn;
    }
    if ((0x01 << EXTI4_15_IRQn) & irq_flags)
    {
        gpio_event(LOCK0_TGL);
        gpio_event(LOCK1_TGL);
        gpio_event(LOCK2_TGL);
        gpio_event(LOCK3_TGL);
        NVIC_EnableIRQ(EXTI4_15_IRQn);
        irq_flags &= ~EXTI4_15_IRQn;
    }
    gpio_clear_dirty(irq_flags);
}

void gpio_set(gpio_t gpio, bool set)
{
    HAL_GPIO_WritePin(gpio.port, gpio.pin, set ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool gpio_get(gpio_t gpio)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)gpio.port, gpio.pin) == GPIO_PIN_SET;
}

void gpio_set_callback(gpio_t gpio, callback_t on_change)
{
    gpio.cb = on_change;
}

static uint8_t dirty;

void gpio_mark_dirty(uint8_t fields) { dirty |= fields; }

void gpio_clear_dirty(uint8_t fields) { dirty &= ~(fields); }

uint8_t gpio_get_dirty() { return dirty; }
