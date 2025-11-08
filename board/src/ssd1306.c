#include <stdint.h>
#include <string.h>

#include "bsp.h"
#include "defs.h"
#include "font.h"
#include "fsm.h"
#include "i2c.h"
#include "images.h"
#include "display.h"
#include "ringbuffer.h"
#include "scheduler.h"
#include "draw.h"

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
#define DISPLAY_TASK_PERIOD_MS (100)
#define DISPLAY_STATE_STACK_DEPTH (8)
#define DISPLAY_COMMAND_BUFFER_DEPTH (33)
// Extra byte to auto-increment mem
#define DISPLAY_FRAMEBUFFER_DEPTH ((128 / 8) * 64)
#define PAGE_WIDTH (128)
#define FB_LOCKED (0x01)
#define HW_LOCKED (0x02)

static uint8_t data_buffer[1 + DISPLAY_FRAMEBUFFER_DEPTH];
static uint8_t* const framebuffer = data_buffer + 1;
static uint8_t command_buffer[DISPLAY_COMMAND_BUFFER_DEPTH];

static uint8_t selected_display;
static uint8_t display_pages;
static callback_t user_callback = NULL;
static callback_t callback_cache;
static state_t* current_state;

static void display_page(callback_t oncomplete);
static void display_command(length_t arg_len, callback_t oncomplete);
static void next_state(int32_t status);
static void update_state(int32_t status);

static uint8_t get_height() { return ((selected_display % 2) == 0) ? 64 : 32; }

LOCALSTATE(dfsm, idle);
LOCALSTATE(dfsm, init);
LOCALSTATE(dfsm, setup);
LOCALSTATE(dfsm, write_display);

// clang-format off
RINGBUFFER(state_queue, state_t*, DISPLAY_STATE_STACK_DEPTH);

static void init_next(int32_t status)
{
    QUEUESTATE(dfsm, init);
    next_state(0);
}

static STATE_ENTER(dfsm, init)
{
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
static STATE(dfsm,init,enter);

static STATE_ENTER(dfsm,setup)
{   
    uint8_t *cmd_ptr = command_buffer + 1;
    *cmd_ptr = CMD_SLEEP;
    ++cmd_ptr; *cmd_ptr = CMD_MULTIPLEX_RATIO;
    ++cmd_ptr; *cmd_ptr = get_height() - 1;
    ++cmd_ptr; *cmd_ptr = CMD_ADDR_MODE;
    ++cmd_ptr; *cmd_ptr = ADDR_MODE_HORZ;
    ++cmd_ptr; *cmd_ptr = CMD_SET_COLUMN;
    ++cmd_ptr; *cmd_ptr = 0;
    ++cmd_ptr; *cmd_ptr = 127;
    ++cmd_ptr; *cmd_ptr = CMD_SET_PAGE;
    ++cmd_ptr; *cmd_ptr = 0;
    ++cmd_ptr; *cmd_ptr = 7;
    ++cmd_ptr; *cmd_ptr = CMD_START_LINE_ADDR;
    ++cmd_ptr; *cmd_ptr = CMD_DISPLAY_OFFSET;
    ++cmd_ptr; *cmd_ptr = 0;
    ++cmd_ptr; *cmd_ptr = CMD_SEGMENT_MODE_ALT;
    ++cmd_ptr; *cmd_ptr = CMD_SCAN_DIRECTION_ALT;
    ++cmd_ptr; *cmd_ptr = CMD_HW_PIN_CONF;
    ++cmd_ptr; *cmd_ptr = ((selected_display % 2) == 0) ? 0x12 : 0x02;
    ++cmd_ptr; *cmd_ptr = CMD_SET_CONTRAST;
    ++cmd_ptr; *cmd_ptr = 0x7F;
    ++cmd_ptr; *cmd_ptr = CMD_DISPLAY_RAM;
    ++cmd_ptr; *cmd_ptr = CMD_NONINVERTED_MODE;
    ++cmd_ptr; *cmd_ptr = CMD_DIVIDER;
    ++cmd_ptr; *cmd_ptr = 0x80;
    ++cmd_ptr; *cmd_ptr = CMD_PRECHARGE_PERIOD;
    ++cmd_ptr; *cmd_ptr = 0xC2;
    ++cmd_ptr; *cmd_ptr = CMD_VCOMH_DESELECT;
    ++cmd_ptr; *cmd_ptr = 0x40;
    ++cmd_ptr; *cmd_ptr = CMD_ENABLE_CHARGE_PUMP;
    ++cmd_ptr; *cmd_ptr = 0x14;
    ++cmd_ptr; *cmd_ptr = CMD_WAKE;
    QUEUESTATE(dfsm, write_display);
    display_command(32, next_state);
};
static STATE(dfsm,setup,enter);
static STATE_ENTER(dfsm, write_display)
{
    display_page(update_state);
};
static STATE_UPDATE(dfsm, write_display)
{
    selected_display += 1;
    task_immediate(next_state);
}
static STATE(dfsm, write_display, enter, update);
static STATE_ENTER(dfsm, idle) {
    if (NULL != user_callback)
    {
        user_callback(0);
    }
};
static STATE(dfsm, idle, enter);
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
    callback_cache = on_init;
    selected_display = 0;
    display_pages = 0;
    current_state = NULL;
    display_mux_enable();
    display_clear();
    memset(framebuffer, 0xFF, 128);
    draw_blit(0, 0, img_millibyte_alt_cropped, 128, 32);
    QUEUESTATE(dfsm, init);
    task_delayed(next_state, MILLIS(MUX_ENABLE_DELAY_MS));
}

static struct {
    length_t size;
    callback_t on_complete;
    uint8_t* data;
    uint8_t retries;
} current_txn;

static void transaction_handler(int32_t status) {
    if ((I2C_SUCCESS == status) || (current_txn.retries == 0)) {
        if (NULL != current_txn.on_complete) {
            current_txn.on_complete(status);
        }
    } else {
        --current_txn.retries;
        i2c_write(DISPLAY_ADDRESS_CMD, current_txn.size, current_txn.data,
                  transaction_handler);
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

static void display_page(callback_t on_complete) {
    data_buffer[0] = DATA_HEADER;
    start_transaction(data_buffer, DISPLAY_FRAMEBUFFER_DEPTH + 1, on_complete);
}

void display_set_inverted(bool inv, callback_t oncomplete)
{
    command_buffer[1] = inv ? CMD_INVERTED_MODE : CMD_NONINVERTED_MODE;
    display_command(2, oncomplete);
}

void display_show(uint8_t display, callback_t oncomplete)
{
    user_callback = oncomplete;
    QUEUESTATE(dfsm, write_display);
    display_select(display, next_state);
}

void display_pixel(uint16_t x, uint16_t y, uint8_t value)
{
    const uint8_t fb_w = 128;
    if ((x < fb_w) || (y < get_height())) {
        if (value) {
            framebuffer[((y / 8) * fb_w) + x] |= (0x01 << (y & 7));
        } else {
            framebuffer[((y / 8) * fb_w) + x] &= ~(0x01 << (y & 7));
        }
    }
}

void display_clear() { memset(framebuffer, 0x00, DISPLAY_FRAMEBUFFER_DEPTH); }
