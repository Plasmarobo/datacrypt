#include <stdint.h>
#include <string.h>

#include <debug.h>

#include "bsp.h"
#include "defs.h"
#include "font.h"
#include "fsm.h"
#include "i2c.h"
#include "images.h"
#include "ringbuffer.h"
#include "stack.h"
#include "scheduler.h"

// Eight bit address 0111 10--,  bit 1 is data/cmd, bit 0 is r/w
#define DISPLAY_ADDRESS_DATA (0x3D)
#define DISPLAY_ADDRESS_CMD (0x3C)

#define CMD_HEADER (0x00)
#define DATA_HEADER (0x40)

#define CMD_RESET (0xE4)

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

#define MUX_ENABLE_DELAY_MS (100)
#define DISPLAY_TASK_PERIOD_MS (10)
#define DISPLAY_STATE_STACK_DEPTH (8)
#define DISPLAY_USER_CALLBACK_DEPTH (4)
#define DISPLAY_COMMAND_BUFFER_DEPTH (33)
#define DISPLAY_HEIGHT (64)
#define DISPLAY_WIDTH (128)
// Extra byte to auto-increment mem
//#define DISPLAY_FRAMEBUFFER_DEPTH ((128 / 8) * 64)
#define DISPLAY_FRAMEBUFFER_DEPTH (DISPLAY_WIDTH * ((DISPLAY_HEIGHT + 7) / 8))
#define PAGE_WIDTH (128)
#define FB_LOCKED (0x01)
#define HW_LOCKED (0x02)

#define DISPLAY_RETRY_DELAY_MS (10)
#define DISPLAY_TIMEOUT_MS (100)
#define DISPLAY_DEFAULT_RETRIES (3)

#define RETRY_DELAY_MS (10)

typedef enum {
    INIT,
    SETUP_SELECT,
    SETUP_SETTLE,
    SETUP_COMMAND,
    SETUP_DATA,
    IDLE,
    MUX_SELECT,
    MUX_SETTLE,
    WRITE_COMMAND,
    WRITE_DATA,
} DISPLAY_STATES;

static int32_t disp_state;

static struct {
    length_t size;
    callback_t on_complete;
    uint8_t* data;
    uint8_t retries;
} current_txn;

static uint8_t data_buffer[1 + DISPLAY_FRAMEBUFFER_DEPTH];
static uint8_t* const framebuffer = data_buffer + 1;
static uint8_t command_buffer[DISPLAY_COMMAND_BUFFER_DEPTH];

static uint8_t selected_display;
static uint8_t init_displays;
static uint8_t display_pages;
static uint32_t retries = 0;
static callback_t user_callback;

static void display_data(callback_t oncomplete);
static void display_command(length_t arg_len, callback_t oncomplete);
static void display_handler(int32_t status);

static void draw_pixel(uint16_t x, uint16_t y, uint8_t value);
static void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static uint8_t get_height() { return ((selected_display % 2) == 0) ? 64 : 32; }
static void retry_write(int32_t status);

static void display_setup(callback_t on_complete) {
    uint8_t* cmd_ptr = command_buffer + 1;
    *cmd_ptr = CMD_SLEEP;
    ++cmd_ptr;
    *cmd_ptr = CMD_MULTIPLEX_RATIO;
    ++cmd_ptr;
    *cmd_ptr = get_height() - 1;
    ++cmd_ptr;
    *cmd_ptr = CMD_ADDR_MODE;
    ++cmd_ptr;
    *cmd_ptr = ADDR_MODE_HORZ;
    ++cmd_ptr;
    *cmd_ptr = CMD_SET_COLUMN;
    ++cmd_ptr;
    *cmd_ptr = 0;
    ++cmd_ptr;
    *cmd_ptr = 127;
    ++cmd_ptr;
    *cmd_ptr = CMD_SET_PAGE;
    ++cmd_ptr;
    *cmd_ptr = 0;
    ++cmd_ptr;
    *cmd_ptr = 7;
    ++cmd_ptr;
    *cmd_ptr = CMD_START_LINE_ADDR;
    ++cmd_ptr;
    *cmd_ptr = CMD_DISPLAY_OFFSET;
    ++cmd_ptr;
    *cmd_ptr = 0;
    ++cmd_ptr;
    *cmd_ptr = CMD_SEGMENT_MODE_ALT;
    ++cmd_ptr;
    *cmd_ptr = CMD_SCAN_DIRECTION_ALT;
    ++cmd_ptr;
    *cmd_ptr = CMD_HW_PIN_CONF;
    ++cmd_ptr;
    *cmd_ptr = ((selected_display % 2) == 0) ? 0x12 : 0x02;
    ++cmd_ptr;
    *cmd_ptr = CMD_SET_CONTRAST;
    ++cmd_ptr;
    *cmd_ptr = 0x7F;
    ++cmd_ptr;
    *cmd_ptr = CMD_DISPLAY_RAM;
    ++cmd_ptr;
    *cmd_ptr = CMD_NONINVERTED_MODE;
    ++cmd_ptr;
    *cmd_ptr = CMD_DIVIDER;
    ++cmd_ptr;
    *cmd_ptr = 0x80;
    ++cmd_ptr;
    *cmd_ptr = CMD_PRECHARGE_PERIOD;
    ++cmd_ptr;
    *cmd_ptr = 0xC2;
    ++cmd_ptr;
    *cmd_ptr = CMD_VCOMH_DESELECT;
    ++cmd_ptr;
    *cmd_ptr = 0x40;
    ++cmd_ptr;
    *cmd_ptr = CMD_ENABLE_CHARGE_PUMP;
    ++cmd_ptr;
    *cmd_ptr = 0x14;
    ++cmd_ptr;
    *cmd_ptr = CMD_WAKE;
    display_command(32, on_complete);
};

static void display_setup_write() {
    command_buffer[1] = CMD_SET_PAGE;
    command_buffer[2] = 0;
    command_buffer[3] = 0xFF;
    command_buffer[4] = CMD_SET_COLUMN;
    command_buffer[5] = 0;
    command_buffer[6] = 127;
    display_command(7, display_handler);
}

static void display_exec() {
    disp_state = MUX_SELECT;
    display_select(selected_display, display_handler);
}

void display_init(void) {
    // Copy bootmsg into framebuffer
    disp_state = INIT;
    selected_display = 0;
    display_pages = 0;
    user_callback = NULL;
    display_mux_enable();
    display_clear();
    //display_blit(0, 0, img_millibyte_alt_cropped, 128, 32);
    // start the driver
    init_displays = 0;
    display_handler(0);
}

static void transaction_handler(int32_t status) {
    if (I2C_SUCCESS == status) {
        
        if (current_txn.on_complete != NULL)
        {
            callback_t cb = current_txn.on_complete;
            current_txn.on_complete = NULL;
            cb(I2C_SUCCESS);
        }
    } else {
        task_delayed_unique(retry_write, MILLIS(RETRY_DELAY_MS));
    }
}

static void retry_write(int32_t status) {
    if (current_txn.retries > 0)
    {
        --current_txn.retries;
        i2c_write(DISPLAY_ADDRESS_CMD, current_txn.size, current_txn.data,
                transaction_handler);
    }
    else
    {
        if (current_txn.on_complete != NULL)
        {
            callback_t cb = current_txn.on_complete;
            current_txn.on_complete = NULL;
            cb(I2C_ERR_UNKNOWN);
        }
    }
}

static void start_transaction(uint8_t* data, length_t data_size,
                              callback_t on_complete) {
    current_txn.size = data_size;
    current_txn.data = data;
    current_txn.on_complete = on_complete;
    current_txn.retries = 3;
    i2c_write(DISPLAY_ADDRESS_CMD, current_txn.size, current_txn.data,
              transaction_handler);
}

static void display_command(length_t data_size, callback_t on_complete) {
    // Send
    command_buffer[0] = CMD_HEADER;
    start_transaction(command_buffer, data_size, on_complete);
}

static void display_data(callback_t on_complete) {
    data_buffer[0] = DATA_HEADER;
    start_transaction(data_buffer, DISPLAY_FRAMEBUFFER_DEPTH + 1, on_complete);
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
    if (disp_state == IDLE)
    {
        retries = DISPLAY_DEFAULT_RETRIES;
        user_callback = oncomplete;
        selected_display = display;
        display_exec(0);
    } else {
        if (NULL != oncomplete)
        {
            oncomplete(DISPLAY_ERR_BUSY);
        }
    }
    
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

// Called on result of a step
void display_handler(int32_t status)
{
    if (I2C_SUCCESS == status)
    {
        switch(disp_state)
        {
            case INIT:
                if (init_displays < DISPLAY_MAX)
                {
                    selected_display = init_displays;
                    disp_state = SETUP_SELECT;
                    display_select(selected_display, display_handler);
                } else {
                    disp_state = IDLE;
                }
                break;
            case SETUP_SELECT:
                disp_state = SETUP_SETTLE;
                display_select_settle(display_handler);
                break;
            case SETUP_SETTLE:
                disp_state = SETUP_COMMAND;
                display_setup(display_handler);
                break;
            case SETUP_COMMAND:
                disp_state = SETUP_DATA;
                display_data(display_handler);
                break;
            case SETUP_DATA:
                ++init_displays;
                disp_state = INIT;
                display_handler(0);
                break;
            case IDLE:
                break;
            case MUX_SELECT:
                disp_state = MUX_SETTLE;
                display_select_settle(display_handler);
                break;
            case MUX_SETTLE:
                disp_state = WRITE_COMMAND;
                display_setup_write(display_handler);
                break;
            case WRITE_COMMAND:
                disp_state = WRITE_DATA;
                display_data(display_handler);
                break;
            case WRITE_DATA:
                disp_state = IDLE;
                if (NULL != user_callback)
                {
                    user_callback(0);
                    user_callback = NULL;
                }
                break;
            default:
                break;
        }
    } else {
        task_delayed(display_handler, MILLIS(DISPLAY_RETRY_DELAY_MS));
    }
}
