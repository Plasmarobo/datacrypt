#include "display.h"
#include "font.h"
#include "draw.h"

void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    // Slow method
    for (uint16_t i = x; i < x + w; ++i)
    {
        for (uint16_t j = y; j < y + h; ++j)
        {
            display_pixel(i, j, 0x01);
        }
    }
}

void draw_char(uint8_t x, uint8_t y, unsigned char c, uint8_t sx,
               uint8_t sy)
{
    for (uint8_t i = 0; i < 5; ++i)
    {
        uint8_t line = font[c * 5 + i];
        for (uint8_t j = 0; j < 8; j++, line >>= 1)
        {
            if (line & 0x01)
            {
                if (sx == 1 && sy == 1)
                {
                    display_pixel(x + i, y + j, 0x01);
                }
                else
                {
                    draw_rect(x + i * sx, y + j * sy, sx, sy);
                }
            }
        }
    }
}

void draw_text(uint8_t x, uint8_t y, const char *text, length_t length)
{
    const uint8_t scale = 2;
    uint8_t x_offset = x;
    for (uint8_t i = 0; i < length; ++i)
    {
        // Font is 6x8 (padding included)
        if (text[i] == '\n')
        {
            y += 8 * scale;
            x_offset = x; // carriage return
            continue;
        }
        draw_char(x_offset, y, (uint8_t)text[i], scale, scale);
        x_offset += (6 * scale);
    }
}

// Copy data into framebuffer
// Incoming image is a flat array that can be index as (x) + (h * y)
void draw_blit(uint8_t x, uint8_t y, const buffer_t img, uint8_t width,
               uint8_t height)
{
    for (uint16_t j = 0; j < height; ++j)
    {
        for (uint16_t i = 0; i < width; ++i)
        {
            display_pixel(
                x + i, y + j,
                img[(j * (width / 8)) + (i / 8)] & (0x1 << (7 - (i % 8))));
        }
    }
}