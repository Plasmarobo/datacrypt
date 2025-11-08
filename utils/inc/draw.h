#ifndef __DRAW_H__
#define __DRAW_H__

#include "defs.h"

// Utility functions for drawing on the display
// Requires an implementation of display_pixel in HAL

void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void draw_char(uint8_t x, uint8_t y, unsigned char c, uint8_t sx,
               uint8_t sy);
void draw_text(uint8_t x, uint8_t y, const char *text, length_t length);

// Copy data into framebuffer
// Incoming image is a flat array that can be index as (x) + (h * y)
void draw_blit(uint8_t x, uint8_t y, const buffer_t img, uint8_t width,
               uint8_t height);
#endif // __DRAW_H__