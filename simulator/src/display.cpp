#include <stdint.h>
#include <iostream>

#include "defs.h"

#include "display.h"
#include "hal.h"
#include "simulator.h"

static uint8_t display_index = 0;
// Monochrome bitmap
static uint8_t framebuffer[(128 * 64) / 8];

static int get_height()
{
    if (display_index < 4)
    {
        return 64;
    }
    else
    {
        return 32;
    }
}

void display_init(callback_t on_complete)
{
    on_complete(0);
}

void display_select(uint8_t index, callback_t on_complete)
{
    display_index = index;
    on_complete(0);
}

uint8_t display_get_selected()
{
    return display_index;
}

void display_set_inverted(bool inv, callback_t oncomplete)
{
    UNUSED(inv);
    // Not implemented, should be a property of the display
    uint8_t buffer[128 * 64 / 8];
    DisplayView *displayView = SimDisplays::getDisplay(display_index);
    if (displayView != nullptr)
    {
        displayView->readDisplay(buffer, 128, get_height());
        for (int i = 0; i < 128 * 64 / 8; i++)
        {
            buffer[i] = ~buffer[i];
        }
        displayView->writeDisplay(buffer, 128, get_height());

        if (oncomplete)
        {
            oncomplete(0);
        }
    }
    else
    {
        std::cerr << "Display " << display_index << " is not initialized" << std::endl;
        if (oncomplete)
        {
            oncomplete(-1);
        }
    }
}

void display_clear()
{
    memset(framebuffer, 0x00, 128 * 64 / 8);
    DisplayView *displayView = SimDisplays::getDisplay(display_index);
    if (displayView != nullptr)
    {
        displayView->writeDisplay(framebuffer, 128, get_height());
    }
    else
    {
        std::cerr << "Display " << display_index << " is not initialized" << std::endl;
    }
}

void display_show(uint8_t display, callback_t on_complete)
{
    int result = 0;
    DisplayView *displayView = SimDisplays::getDisplay(display);
    if (displayView != nullptr)
    {
        displayView->writeDisplay(framebuffer, 128, get_height());
    }
    else
    {
        result = -1;
        std::cerr << "Display " << display << " is not initialized" << std::endl;
    }
    on_complete(result);
}

void display_pixel(uint16_t x, uint16_t y, uint8_t value)
{
    const uint8_t fb_w = 128;
    if ((x < fb_w) || (y < get_height()))
    {
        if (value)
        {
            framebuffer[((y / 8) * fb_w) + x] |= (0x01 << (y & 7));
        }
        else
        {
            framebuffer[((y / 8) * fb_w) + x] &= ~(0x01 << (y & 7));
        }
    }
}