#ifndef __BSP_H
#define __BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"

void Error_Handler(void);

#define VERSION_STRING ("v0.0.1 alpha")

#define GPIO_SET (true)
#define GPIO_RESET (false)

typedef struct {
    uint16_t pin;
    void *port;
    bool value;
    callback_t cb;
} gpio_t;

extern gpio_t BAT_READ;
extern gpio_t LED_DATA;
extern gpio_t FLASH_DIN;
extern gpio_t FLASH_DOUT;
extern gpio_t LED_SCK;
extern gpio_t PWM_CH1;
extern gpio_t PWM_CH2;
extern gpio_t SUCCESS_SW;
#define SUCCESS_SW_EXTI_IRQn EXTI0_1_IRQn
extern gpio_t FAILURE_SW;
#define FAILURE_SW_EXTI_IRQn EXTI0_1_IRQn
extern gpio_t INTERCEPT_SW;
#define INTERCEPT_SW_EXTI_IRQn EXTI2_3_IRQn
extern gpio_t LOCK2_TGL;
#define LOCK2_TGL_EXTI_IRQn EXTI4_15_IRQn
extern gpio_t LOCK3_TGL;
#define LOCK3_TGL_EXTI_IRQn EXTI4_15_IRQn
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
#define TIMER_SW_EXTI_IRQn EXTI2_3_IRQn
extern gpio_t MUXRST;
extern gpio_t DISPLAY_SCL;
extern gpio_t LOCK0_TGL;
#define LOCK0_TGL_EXTI_IRQn EXTI4_15_IRQn
extern gpio_t LOCK1_TGL;
#define LOCK1_TGL_EXTI_IRQn EXTI4_15_IRQn
extern gpio_t ANALOG_RNG;
/* USER CODE BEGIN Private defines */

// ========== LED write ==========
#define DISPLED_STATUS_OFFSET (0)
#define DISPLED_STATUS_LENGTH (4)
#define COUNTER_0_OFFSET (4)
#define COUNTER_1_OFFSET (8)
#define COUNTER_LENGTH (4)
#define TIMER_OFFSET (12)
#define TIMER_LENGTH (6)

#define STATUS_SUCCESS (0)

typedef struct {
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

void display_init(void);
void display_mux_enable();
void display_select(uint8_t index, callback_t on_complete);
uint8_t display_get_selected();
void display_set_inverted(bool inv, callback_t oncomplete);
void display_set_text(uint8_t x, uint8_t y, const char *text, length_t length);
void display_blit(uint8_t x, uint8_t y, const buffer_t img, uint8_t width,
                  uint8_t height);
void display_clear(void);
void display_show(uint8_t display, callback_t on_complete);

// ========== Read Switches ==========
void gpio_update(int32_t status);
void gpio_mark_dirty(uint8_t fields);
uint8_t gpio_get_dirty();
void gpio_clear_dirty(uint8_t fields);
void gpio_set(gpio_t gpio, bool set);
bool gpio_get(gpio_t gpio);
void gpio_set_callback(gpio_t gpio, callback_t on_change);
void gpio_change_handler(gpio_t gpio);

// ========== Serial Comm ==========

#define SERIAL_USER_ABORT (-2)
#define SERIAL_HW_ERROR (-1)

void serial_init(void);
void serial_flush_rx(void);
length_t serial_available_bytes(void);
void serial_read(buffer_t dest, length_t length, callback_t oncomplete);
void serial_write(const buffer_t data, length_t length, callback_t oncomplete);
void serial_abort_tx(void);
void serial_abort_rx(void);
void serial_abort_rx_notify(callback_t oncomplete);
void serial_printhex(const buffer_t data, length_t length,
                     callback_t oncomplete);
void serial_print(const char *str);
void serial_printf(const char *fmt, ...);
void vserial_printf(const char *fmt, va_list args);
void serial_tx_complete_handler(int32_t status);
void serial_rx_complete_handler(int32_t status);
void serial_rx_abort_handler(int32_t status);
void serial_tx_abort_handler(int32_t status);

// ========== Pseudo RNG ==========
void random_init();
// Produce a 16 bit prng number
uint32_t random_int();
uint32_t uniform(uint32_t min, uint32_t max);

// ========== Flash ==========
void flash_op_complete_handler(int32_t status);

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

// ========== BSP =======================
void bsp_init();
void bsp_error();
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
