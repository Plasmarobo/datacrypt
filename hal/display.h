#ifndef __DISPLAY_HAL_
#define __DISPLAY_HAL_

#include <stdbool.h>
#include <stdint.h>
#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

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
#define DISPLAY_MAX_STRING (128)
#define DISP(x, y) ((x) + (y * 4))

    void display_init(callback_t on_complete);
    void display_select(uint8_t index, callback_t on_complete);
    uint8_t display_get_selected();
    void display_set_inverted(bool inv, callback_t oncomplete);
    void display_clear(void);
    void display_show(uint8_t display, callback_t on_complete);

    // Key function to draw into hardware-dependent framebuffer
    void display_pixel(uint16_t x, uint16_t y, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // __DISPLAY_HAL