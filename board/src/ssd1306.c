#include <stdint.h>
#include <string.h>

#include "bsp.h"
#include "defs.h"
#include "font.h"
#include "fsm.h"
#include "i2c.h"
#include "images.h"
#include "ringbuffer.h"
#include "scheduler.h"

// Eight bit address 0111 10--,  bit 1 is data/cmd, bit 0 is r/w
#define DISPLAY_ADDRESS_DATA (0x3D)
#define DISPLAY_ADDRESS_CMD (0x3C)

#define CMD_HEADER (0x00)
#define DATA_HEADER (0x40)

// One value byte
#define CMD_SET_CONTRAST (0x81)
#define CONTRAST_VALUE (0x80)

#define CMD_PRECHARGE_PERIOD (0xD9)
#define PRECHARGE_PERIOD (0x22)

#define CMD_VCOMH_DESELECT (0xDB)
#define VCOMH_DESELECT (0x20)
#define CMD_SCROLL_STOP (0x2E)
// Normal display mode
#define CMD_DISPLAY_RAM (0xA4)
// Fill display white
#define CMD_DISPLAY_FILL (0xA5)

#define CMD_DIVIDER (0xD5)

#define CMD_MULTIPLEX_RATIO (0xA8)
#define MULTIPLEX_RATIO (0x3F)

#define CMD_DISPLAY_OFFSET (0xD3)
#define CMD_START_LINE_ADDR (0x40)
#define CMD_ENABLE_CHARGE_PUMP (0x8D)

// Inverts bit to pixel mapping
#define CMD_NONINVERTED_MODE (0xA6)
#define CMD_INVERTED_MODE (0xA7)

#define CMD_SLEEP (0xAE)
#define CMD_WAKE (0xAF)

#define CMD_ADDR_MODE (0x20)
#define ADDR_MODE_HORZ (0x00)
#define ADDR_MODE_VERT (0x01)
#define CMD_SEGMENT_MODE (0xA0)
#define CMD_SEGMENT_MODE_ALT (0xA1)
#define CMD_SCAN_DIRECTION (0xC0)
#define CMD_SCAN_DIRECTION_ALT (0xC8)
#define CMD_HW_PIN_CONF (0xDA)
#define COM_PIN_32 (0x02)
#define COM_PIN_64 (0x12)

#define CMD_SET_COLUMN (0x21)
#define CMD_SET_PAGE (0x22)
#define CMD_NOP (0xE3)

#define DISPLAY_INDEX_ADDR(x) (0x01 << x)
#define LINE_WIDTH_PX (128)
#define LINE_WIDTH_BYTES (LINE_WIDTH_PX / 8)
#define CHARGE_PUMP_ENABLE (0x14)
#define CLK_DIVIDER (0x80)

#define MUX_ENABLE_DELAY_US (1)
#define DISPLAY_TASK_PERIOD_MS (100)
#define DISPLAY_STATE_STACK_DEPTH (8)
#define DISPLAY_COMMAND_BUFFER_DEPTH (32)
#define DISPLAY_PAGE_BUFFER_DEPTH (1 + 128)
#define DISPLAY_FRAMEBUFFER_DEPTH ((128 / 8) * 64)
#define PAGE_WIDTH (128)
#define FB_LOCKED (0x01)
#define HW_LOCKED (0x02)

static uint8_t framebuffer[DISPLAY_FRAMEBUFFER_DEPTH];
static uint8_t page_buffer[DISPLAY_PAGE_BUFFER_DEPTH];
static uint8_t command_buffer[DISPLAY_COMMAND_BUFFER_DEPTH];

static uint8_t selected_display;
static uint8_t display_pages;
static callback_t user_callback = NULL;
static callback_t callback_cache;
static state_t* current_state;
static uint8_t* page_ptr;

static void display_page(callback_t oncomplete);
static void display_command(length_t arg_len, callback_t oncomplete);
static void next_state(int32_t status);
static void update_state(int32_t status);
static void write_page(uint8_t page);
static void draw_pixel(uint16_t x, uint16_t y, uint8_t value);
static void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static uint8_t get_height() { return ((selected_display % 2) == 0) ? 64 : 32; }
static uint8_t get_pages() { return ((selected_display % 2) == 0) ? 7 : 3; }

DECLARESTATE(dfsm, idle);
DECLARESTATE(dfsm, init);
DECLARESTATE(dfsm, setup);
DECLARESTATE(dfsm, write_display);
DECLARESTATE(dfsm, write_page);
DECLARESTATE(dfsm, select_page);

// clang-format off
RINGBUFFER(state_queue, state_t*, DISPLAY_STATE_STACK_DEPTH);

static void init_next(int32_t status)
{
    QUEUESTATE(dfsm, init);
    next_state(0);
}

STATE_ENTER(dfsm, init)
{
    if (selected_display == 0)
    {
        if (user_callback != NULL)
        {
            callback_cache = user_callback;
        }
        display_mux_enable();
    }
    if (selected_display < DISPLAY_MAX)
    {
        user_callback = init_next;
        QUEUESTATE(dfsm, setup);
        display_select(selected_display, next_state);
    }
    else
    {
        if (callback_cache != NULL)
        {
            user_callback = callback_cache;
        }
        else
        {
            user_callback = NULL;
        }
        next_state(0);
    }
};
STATE(dfsm,init,enter);

STATE_ENTER(dfsm,setup)
{   
    command_buffer[1] = CMD_SLEEP;
    command_buffer[2] = CMD_MULTIPLEX_RATIO;
    command_buffer[3] = get_height() - 1;
    command_buffer[4] = CMD_ADDR_MODE;
    command_buffer[5] = ADDR_MODE_HORZ;
    command_buffer[6] = CMD_SET_COLUMN;
    command_buffer[7] = 0;
    command_buffer[8] = 127;
    command_buffer[9] = CMD_SET_PAGE;
    command_buffer[10] = 0;
    command_buffer[11] = get_pages();
    command_buffer[12] = CMD_START_LINE_ADDR;
    command_buffer[13] = CMD_DISPLAY_OFFSET;
    command_buffer[14] = 0;
    command_buffer[15] = CMD_SEGMENT_MODE_ALT;
    command_buffer[16] = CMD_SCAN_DIRECTION_ALT;
    command_buffer[17] = CMD_HW_PIN_CONF;
    command_buffer[18] = ((selected_display % 2) == 0) ? 0x12 : 0x02;
    command_buffer[19] = CMD_SET_CONTRAST;
    command_buffer[20] = 0x7F;
    command_buffer[21] = CMD_DISPLAY_RAM;
    command_buffer[22] = CMD_NONINVERTED_MODE;
    command_buffer[23] = CMD_DIVIDER;
    command_buffer[24] = 0x80;
    command_buffer[25] = CMD_PRECHARGE_PERIOD;
    command_buffer[26] = 0xC2;
    command_buffer[27] = CMD_VCOMH_DESELECT;
    command_buffer[28] = 0x40;
    command_buffer[29] = CMD_ENABLE_CHARGE_PUMP;
    command_buffer[30] = 0x14;
    command_buffer[31] = CMD_WAKE;
    QUEUESTATE(dfsm, write_display);
    display_command(32, next_state);
};
STATE(dfsm,setup,enter);
STATE_ENTER(dfsm, write_display)
{
    display_pages = get_pages();
    page_ptr = framebuffer;
    QUEUESTATE(dfsm, select_page);
    next_state(0);
};
STATE(dfsm, write_display, enter);
STATE_ENTER(dfsm, select_page)
{
    command_buffer[1] = CMD_SET_PAGE;
    command_buffer[2] = 0;
    command_buffer[3] = 0xFF;
    command_buffer[4] = CMD_SET_COLUMN;
    command_buffer[5] = 0;
    command_buffer[6] = 127;
    QUEUESTATE(dfsm, write_page);
    display_command(7, next_state);
};
STATE(dfsm, select_page, enter);
STATE_ENTER(dfsm, write_page)
{
    display_page(update_state);
};
STATE_UPDATE(dfsm, write_page)
{
    if (page_ptr < (framebuffer + DISPLAY_FRAMEBUFFER_DEPTH))
    {
        display_page(update_state);
    }
    else
    {
        selected_display += 1;
        task_immediate(next_state);
    }
}
STATE(dfsm, write_page, enter, update);
STATE_ENTER(dfsm, idle) {
    if (NULL != user_callback)
    {
        user_callback(0);
    }
};
STATE(dfsm, idle, enter);
// clang-format on

// Execute the current state without popping anything off the stack
static void next_state(int32_t status) {
    if (ringbuffer_empty(&state_queue)) {
        fsm_set_state(NULL, STATEREF(dfsm, idle));
        current_state = STATEREF(dfsm, idle);
    } else {
        state_t* new_state = NULL;
        ringbuffer_pop(&state_queue, &new_state);
        fsm_set_state(current_state, new_state);
        current_state = new_state;
    }
}

static void update_state(int32_t status) { fsm_update(current_state); }

uint8_t test_image[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void display_init(callback_t on_init) {
    // Copy bootmsg into framebuffer
    user_callback = on_init;
    callback_cache = NULL;
    selected_display = 0;
    display_pages = 0;
    current_state = NULL;
    display_clear();
    memset(framebuffer, 0xFF, 128);
    display_blit(0, 0, img_millibyte_alt_cropped, 128, 32);
    QUEUESTATE(dfsm, init);
    next_state(0);
}

static void display_command(length_t data_size, callback_t on_complete) {
    // Send
    command_buffer[0] = CMD_HEADER;
    if (i2c_get_status() == I2C_SUCCESS) {
        i2c_write(DISPLAY_ADDRESS_CMD, data_size, command_buffer, on_complete);
    }
}

static void display_page(callback_t on_complete) {
    page_buffer[0] = DATA_HEADER;
    uint16_t height = get_height();
    memcpy(page_buffer + 1, page_ptr, PAGE_WIDTH);
    page_ptr += PAGE_WIDTH;
    // Pages need to be converted to columnar data
    if (i2c_get_status() == I2C_SUCCESS) {
        i2c_write(DISPLAY_ADDRESS_CMD, DISPLAY_PAGE_BUFFER_DEPTH, page_buffer,
                  on_complete);
    }
}

static void display_complete(int32_t status) {
    if (NULL != user_callback) {
        user_callback(status);
        user_callback = NULL;
    }
}

static void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    // Slow method
    for (uint16_t i = x; i < x + w; ++i) {
        for (uint16_t j = y; j < y + h; ++j) {
            draw_pixel(i, j, 0x01);
        }
    }
}

void display_char(uint8_t x, uint8_t y, unsigned char c, uint8_t sx,
                  uint8_t sy) {
    for (uint8_t i = 0; i < 5; ++i) {
        uint8_t line = font[c * 5 + i];
        for (uint8_t j = 0; j < 8; j++, line >>= 1) {
            if (line & 0x01) {
                if (sx == 1 && sy == 1) {
                    draw_pixel(x + i, y + j, 0x01);
                } else {
                    draw_rect(x + i * sx, y + j * sy, sx, sy);
                }
            }
        }
    }
}

void display_set_text(uint8_t x, uint8_t y, const char* text, length_t length) {
    const uint8_t scale = 2;
    for (uint8_t i = 0; i < length; ++i) {
        // Font is 6x8 (padding included)
        display_char(x + (6 * i * scale), y, (uint8_t)text[i], scale, scale);
    }
}

void display_set_inverted(bool inv, callback_t oncomplete) {
    command_buffer[1] = inv ? CMD_INVERTED_MODE : CMD_NONINVERTED_MODE;
    display_command(2, oncomplete);
}

void display_show(uint8_t display, callback_t oncomplete) {
    user_callback = oncomplete;
    QUEUESTATE(dfsm, write_display);
    display_select(display, next_state);
}

static void draw_pixel(uint16_t x, uint16_t y, uint8_t value) {
    const uint8_t fb_w = 128;
    if ((x < fb_w) || (y < get_height())) {
        if (value) {
            framebuffer[((y / 8) * fb_w) + x] |= (0x01 << (y & 7));
        } else {
            framebuffer[((y / 8) * fb_w) + x] &= ~(0x01 << (y & 7));
        }
    }
}

// Copy data into framebuffer
// Incoming image is a flat array that can be index as (x) + (h * y)
void display_blit(uint8_t x, uint8_t y, const buffer_t img, uint8_t width,
                  uint8_t height) {
    // Clamp W and H to edges of display
    const uint8_t fb_w = 128;
    const uint8_t fb_h = get_height();
    uint8_t cw = width;
    uint8_t ch = height;
    if (x + width > fb_w) {
        cw = fb_w - x;
    }
    if (y + height > fb_h) {
        ch = fb_h - y;
    }
    for (uint16_t j = 0; j < ch; ++j) {
        for (uint16_t i = 0; i < cw; ++i) {
            draw_pixel(
                x + i, y + j,
                img[(j * (width / 8)) + (i / 8)] & (0x1 << (7 - (i % 8))));
        }
    }
}

void display_clear() { memset(framebuffer, 0x00, DISPLAY_FRAMEBUFFER_DEPTH); }
