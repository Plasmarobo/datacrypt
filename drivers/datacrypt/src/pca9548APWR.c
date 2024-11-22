#include <stddef.h>

#include "bsp.h"
#include "i2c.h"
#include "scheduler.h"
// PCA9548APWR
// I2C Multiplexer
// Address 0x70 (7 bit) or 0xE0 (8 bit)
#define MUX_ADDRESS (0x70)
#define MUX_SETTLE_US (20)

static uint8_t control_register = 0;
static uint8_t selected_display = 0;

uint8_t display_get_selected() { return selected_display; }

void display_mux_enable() { gpio_set(MUXRST, true); }

void display_select(uint8_t index, callback_t oncomplete) {
    // Set control register to index
    if (index > 0x80) {
        control_register = 0;
    } else {
        control_register = 0x01 << index;
    }
    i2c_write(MUX_ADDRESS, 1, &control_register, oncomplete);
}

void display_select_settle(callback_t oncomplete)
{
    task_delayed(oncomplete, MICROS(MUX_SETTLE_US));
}
