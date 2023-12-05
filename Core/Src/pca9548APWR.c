#include "bsp.h"
#include "i2c.h"

// PCA9548APWR
// I2C Multiplexer
// Address 0x70 (7 bit) or 0xE0 (8 bit)
#define MUX_ADDRESS (0xE0)

typedef enum
{
    MUX_READY,
    MUX_BUSY,
} mux_state_t;

static uint8_t control_register = 0;
static uint8_t selected_display = 0;
static callback_t complete_handler = NULL;

static void display_selected_handler(int32_t result)
{
    if (result < 0)
    {
        // Error!
        return;
    }
    selected_display = control_register;
    if (complete_handler != NULL)
    {
        complete_handler(selected_display,0);
        complete_handler = NULL;
    }
}

uint8_t display_get_selected()
{
    return selected_display;
}

void display_select(uint8_t index, callback_t oncomplete)
{
    // Set control register to index
    i2c_write(MUX_ADDRESS, 1, &index, display_selected_handler);
}
