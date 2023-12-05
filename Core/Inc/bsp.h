/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "defs.h"
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

typedef struct {
    uint16_t pin;
    GPIO_TypeDef* port;
    bool value;
    callback_t cb;
} gpio_t;

gpio_t BAT_READ = {GPIO_PIN_0, GPIOA};
gpio_t LED_DATA = {GPIO_PIN_2, GPIOA};
gpio_t FLASH_DIN = {GPIO_PIN_3, GPIOA};
gpio_t FLASH_DOUT = {GPIO_PIN_4, GPIOA};
gpio_t LED_SCK = {GPIO_PIN_5, GPIOA};
gpio_t PWM_CH1 = {GPIO_PIN_6, GPIOA};
gpio_t PWM_CH2 = {GPIO_PIN_7, GPIOA};
gpio_t SUCCESS_SW = {GPIO_PIN_0, GPIOB};
#define SUCCESS_SW_EXTI_IRQn EXTI0_1_IRQn
gpio_t FAILURE_SW = {GPIO_PIN_1, GPIOB};
#define FAILURE_SW_EXTI_IRQn EXTI0_1_IRQn
gpio_t INTERCEPT_SW = {GPIO_PIN_2, GPIOB};
#define INTERCEPT_SW_EXTI_IRQn EXTI2_3_IRQn
gpio_t LOCK2_TGL = {GPIO_PIN_10, GPIOB};
#define LOCK2_TGL_EXTI_IRQn EXTI4_15_IRQn
gpio_t LOCK3_TGL = {GPIO_PIN_11, GPIOB};
#define LOCK3_TGL_EXTI_IRQn EXTI4_15_IRQn
gpio_t FLASH_CS = {GPIO_PIN_12, GPIOB};
gpio_t FLASH_SCK{GPIO_PIN_13, GPIOB};
gpio_t WIN_SW = {GPIO_PIN_14, GPIOB};
gpio_t SHUFFLE_SW = {GPIO_PIN_15, GPIOB};
gpio_t TURN_SW = {GPIO_PIN_8, GPIOA};
gpio_t SOUND_TGL = {GPIO_PIN_6, GPIOC};
gpio_t MISC_SW = {GPIO_PIN_7, GPIOC};
gpio_t DISPLAY_SDA = {GPIO_PIN_10, GPIOA};
gpio_t HIDE_TGL = {GPIO_PIN_0, GPIOD};
gpio_t AUDIO_SD = {GPIO_PIN_2, GPIOD};
gpio_t TIMER_SW = {GPIO_PIN_3, GPIOB};
#define TIMER_SW_EXTI_IRQn EXTI2_3_IRQn
gpio_t MUXRST = {GPIO_PIN_5, GPIOB};
gpio_t DISPLAY_SCL = {GPIO_PIN_6, GPIOB};
gpio_t LOCK0_TGL = {GPIO_PIN_8, GPIOB};
#define LOCK0_TGL_EXTI_IRQn EXTI4_15_IRQn
gpio_t LOCK1_TGL = {GPIO_PIN_9, GPIOB};
#define LOCK1_TGL_EXTI_IRQn EXTI4_15_IRQn
/* USER CODE BEGIN Private defines */

// ========== uS timer ==========
#define MILLIS(x) (x * 1000)
#define MICROS(x) (x)

// Correct rollover depends on the width of timespan_t
#define CORRECT_ROLLOVER(x) (0xFFFFFFFF - x)
typedef int32_t timespan_t;

timespan_t microseconds();
timespan_t milliseconds();

// ========== LED write ==========
#define DISPLED_STATUS_OFFSET (0)
#define DISPLED_STATUS_LENGTH (4)
#define COUNTER_0_OFFSET (4)
#define COUNTER_1_OFFSET (8)
#define COUNTER_LENGTH (4)
#define TIMER_OFFSET (12)
#define TIMER_LENGTH (6)

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

void leds_init();
void set_disp(color_t value[DISPLED_STATUS_LENGTH]);
void set_counter0(color_t value[COUNTER_LENGTH]);
void set_counter1(color_t value[COUNTER_LENGTH]);
void set_timer(color_t value[TIMER_LENGTH]);

// No callback, this should freerun
void leds_write(void);
bool leds_busy(void);
void leds_tx_complete_handler();
void leds_rx_complete_handler();

void leds_set(uint8_t offset, uint8_t size, color_t* color);
// convenience functions
inline void set_displed(color_t color[DISPLED_STATUS_LENGTH]) {
    set_led(DISPLED_STATUS_OFFSET, DISPLED_STATUS_LENGTH, color);
}
inline void set_counter0(color_t color[COUNTER_LENGTH]) {
    set_led(COUNTER_0_OFFSET, COUNTER_LENGTH, color);
}
inline void set_counter1(color_t color[COUNTER_LENGTH]) {
    set_led(COUNTER_1_OFFSET, COUNTER_LENGTH, color);
}
inline void set_timer(color_t color[TIMER_LENGTH]) {
    set_led(TIMER_OFFSET, TIMER_LENGTH, color);
}

// ========== Set audio ==========
void audio_init();
void audio_set_stream(buffer_t dma_buffer, length_t data_length,
                      callback_t on_complete);
void audio_stream_start(void);
void audio_stream_stop(void);
bool audio_busy(void);
void audio_shutdown(bool shutdown);

// ========== Set Display ==========
#define DISP0_INDEX (0x00)
#define DISP1_INDEX (0x01)
#define DISP2_INDEX (0x02)
#define DISP3_INDEX (0x04)
#define DISP4_INDEX (0x08)
#define DISP5_INDEX (0x10)
#define DISP6_INDEX (0x20)
#define DISP7_INDEX (0x40)
#define DISPLAY_MAX (8)

void display_init();
void display_select(uint8_t index);
uint8_t display_get_selected();
void display_set_text(uint8_t display, uint8_t x, uint8_t y, const char* text,
                      length_t length);
void display_blit(uint8_t display, uint8_t x, uint8_t y, buffer_t img,
                  uint8_t width, uint8_t height);

// ========== Read Switches ==========
void gpio_update(int32_t status);
void gpio_mark_dirty(uint8_t fields);
uint8_t gpio_get_dirty(uint8_t fields);
void gpio_clear_dirty(uint8_t fields);
void gpio_set(gpio_t gpio, bool set);
bool gpio_get(gpio_t gpio);
void gpio_set_callback(uint8_t gpio_index, callback_t on_change);
void gpio_change_handler(gpio_t gpio);

// ========== Read External Flash ==========
void flash_init();
void flash_read(uint32_t addr, buffer_t dest, length_t words,
                callback_t on_complete);
void flash_write(uint32_t addr, buffer_t dest, length_t words,
                 callback_t on_complete);
void flash_erase(uint32_t addr, callback_t on_complete);
void flash_tx_complete_handler();
void flash_rx_complete_handler();

// ========== BSP =======================
void bsp_init();
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
