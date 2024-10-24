#include <stddef.h>

#include "bsp.h"
#include "i2c.h"
#include "scheduler.h"
// PCA9548APWR
// I2C Multiplexer
// Address 0x70 (7 bit) or 0xE0 (8 bit)
#define MUX_ADDRESS (0x70)
#define MUX_SETTLE_US (10)
typedef enum {
    MUX_READY,
    MUX_BUSY,
} mux_state_t;

static uint8_t retries = 0;
static uint8_t control_register = 0;
static uint8_t selected_display = 0;
static callback_t complete_handler = NULL;

static void write_reg(int32_t status);

static void mux_settle_handler(int32_t result) {
    if (0 == result)
    {
        selected_display = control_register;
    }
    else
    {
        selected_display = DISPLAY_MAX;
    }
    if (complete_handler != NULL) {
        complete_handler(result);
        complete_handler = NULL;
    }
}

static void display_selected_handler(int32_t result) {
    if (result != I2C_SUCCESS && retries <= 0) {
        // Error! Try again
        --retries;
        if (NULL == task_delayed(write_reg, MICROS(MUX_SETTLE_US)))
        {
            mux_settle_handler(-1);
        }
        return;
    }
    else
    {
        task_delayed_signal(mux_settle_handler, MICROS(MUX_SETTLE_US), result);
    }
}

static void write_reg(int32_t status) {
    i2c_write(MUX_ADDRESS, 1, &control_register, display_selected_handler);
}

uint8_t display_get_selected() { return selected_display; }

void display_mux_enable() { gpio_set(MUXRST, true); }

void display_select(uint8_t index, callback_t oncomplete) {
    // Set control register to index
    retries = 3;
    complete_handler = oncomplete;
    if (index > 0x80) {
        control_register = 0;
    } else {
        control_register = 0x01 << index;
    }
    write_reg(0);
}
