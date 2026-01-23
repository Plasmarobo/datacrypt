#include "effects.h"

#include "hal.h"
#include "fsm.h"
#include "scheduler.h"
#include "display.h"
#include "draw.h"
#include "gpio.h"
#include "game.h"

#define FLASH_TIME (3000)

static uint8_t blink_enable;
static uint8_t blink_state;

static void unflash(int32_t status)
{
    UNUSED(status);
    set_blink_enable(DISP_B0, false);
    set_blink_enable(DISP_B1, false);
    set_blink_enable(DISP_B2, false);
    set_blink_enable(DISP_B3, false);
};

void flash_unlocked(s)
{
    if (IS_LOCKED(&LOCK0_TGL))
    {
        set_blink_enable(DISP_B0, true);
    }
    if (IS_LOCKED(&LOCK1_TGL))
    {
        set_blink_enable(DISP_B1, true);
    }
    if (IS_LOCKED(&LOCK2_TGL))
    {
        set_blink_enable(DISP_B2, true);
    }
    if (IS_LOCKED(&LOCK3_TGL))
    {
        set_blink_enable(DISP_B3, true);
    }
    task_delayed(unflash, MILLIS(FLASH_TIME));
};

static void blink_handler(int32_t status)
{
    UNUSED(status);
    for (uint8_t i = 0; i < DISPLAY_MAX; ++i)
    {
        uint8_t mask = 0x01 << i;
        if (blink_enable & mask)
        {
            // Read the bit
            bool invert = blink_state & mask;
            // XOR with current state to toggle
            blink_state ^= mask;
            AWAIT(display_select(i, default_future));
            AWAIT(display_set_inverted(invert, default_future));
            AWAIT(display_show(i, default_future));
        }
    }
}

void effects_init()
{
    blink_enable = 0;
    blink_state = 0;
    task_periodic(blink_handler, MILLIS(250));
}

void effects_update()
{
    // Noop
}

void set_blink_enable(uint8_t display, bool enable)
{
    if (display < DISPLAY_MAX)
    {
        uint8_t mask = 0x01 << display;
        if (enable)
        {
            blink_enable |= mask;
        }
        else
        {
            blink_enable &= ~mask;
        }
    }
}

bool get_blink_enable(uint8_t display)
{
    if (display < DISPLAY_MAX)
    {
        uint8_t mask = 0x01 << display;
        return (blink_enable & mask);
    }
    return false;
}
