#include <stdint.h>

#include "bsp.h"
#include "defs.h"
#include "fsm.h"
#include "i2c.h"
#include "images.h"
#include "scheduler.h"

// Eight bit address 0111 10--,  bit 1 is data/cmd, bit 0 is r/w
#define DISPLAY_COMMAND_ADDRESS (0x78)
#define DISPLAY_DATA_ADDRESS (0x7A)

// One value byte
#define CMD_SET_CONTRAST (0x81)
// Normal display mode
#define CMD_DISPLAY_RAM (0xA4)
// Fill display white
#define CMD_DISPLAY_FILL (0xA5)
// Inverts bit to pixel mapping
#define CMD_NONINVERTED_MODE (0xA6)
#define CMD_INVERTED_MODE (0xA7)

#define CMD_SLEEP (0xAE)
#define CMD_WAKE (0xAF)

#define CMD_ADDR_MODE (0x20)
#define DISPLAY_MODE_HORZ (0x00)
#define CMD_COLUMN_START (0x21)
#define CMD_PAGE_START (0x22)
#define CMD_NOP (0xE3)

#define DISPLAY_INDEX_ADDR(x) (0x01 << x)
#define LINE_WIDTH_PX (128)
#define LINE_WIDTH_BYTES (LINE_WIDTH_PX / 8)

#define DISPLAY_TASK_PERIOD_MS (100)

#define FRAMEBUFFER(n, w, h)                     \
    static uint8_t CONCAT(n, data)[(w / 8) * h]; \
    framebuffers[n] = {w, h, &CONCAT(n, data), false, NULL}

// Locked for writing
const uint8_t FB_LOCKED = 0x01;
// Update requested on unlock
const uint8_t FB_UPDATE = 0x02;

typedef struct {
    uint8_t w;
    uint8_t h;
    uint8_t* data;
    uint8_t flags;
} framebuffer_t;

static framebuffer_t framebuffers[DISPLAY_MAX];

FRAMEBUFFER(0, 128, 64);
FRAMEBUFFER(1, 128, 64);
FRAMEBUFFER(2, 128, 64);
FRAMEBUFFER(3, 128, 64);
FRAMEBUFFER(4, 128, 32);
FRAMEBUFFER(5, 128, 32);
FRAMEBUFFER(6, 128, 32);
FRAMEBUFFER(7, 128, 32);

static uint8_t current_display = 0;
static uint8_t* boot_image = &img_millbyte_alt_cropped;

static void on_selected_init(int32_t a, uint32_t b);
static void display_command(uint8_t command, buffer_t arg, length_t arg_len);
static void display_data(uint8_t display);
static void display_task(int32_t status);
static void on_selected_task(int32_t a, uint32_t b);

STACK(pending_updates, uint8_t, DISPLAY_MAX);

// clang-format off
FSM(display_fsm);
STATE_ENTER(setcontrast)
{
    // From 1 to 256 = 0 to 255
    uint8_t contrast = 127;
    display_command(CMD_SET_CONTRAST, &contrast, 1);
};
STATE_ENTER(setmode)
{
    uint8_t argument = DISPLAY_MODE_HORZ;
    display_command(CMD_ADDR_MODE, &argument, 1);
};
STATE_ENTER(setcol)
{
    uint8_t argument = 0;
    display_command(CMD_COLUMN_START, &argument, 1);
};
STATE_ENTER(setpage)
{
    uint8_t argument = 0;
    display_command(CMD_PAGE_START, &argument, 1);
};
STATE_ENTER(bootmsg)
{
    memcpy(boot_image, framebuffers[current_display].data, LINE_WIDTH_BYTES);
    display_data();
};
STATE_ENTER(next) {
    current_display += 1;
    if (current_display >= DISPLAY_MAX) {
        // We're done, exit completely
        FSM_SET_STATE(display_fsm, running);
    } else {
        // Restart
        display_select(DISPLAY_INDEX_ADDR(current_display), &on_selected_init);
    }
};
STATE_ENTER(running)
{
    // START TASK
    task_periodic(display_task, MILLIS(DISPLAY_TASK_PERIOD_MS));
};
STATE_UPDATE(running)
{
    if (stack_empty(pending_updates))
    {
        for(uint8_t i = 0; i < DISPLAY_MAX; ++i)
        {
            stack_push(pending_updates, &i);
        }
        display_select(DISPLAY_INDEX_ADDR(stack_peek(pending_updates)), &on_selected_task);
    }
};
STATE_ENTER(error) {
    // handle error
};
REGISTER_STATE(display_fsm, setcontrast);
REGISTER_STATE(display_fsm, setmode);
REGISTER_STATE(display_fsm, setcol);
REGISTER_STATE(display_fsm, setpage);
REGISTER_STATE(display_fsm, bootmsg);
REGISTER_STATE(display_fsm, next);
REGISTER_STATE(display_fsm, running);
REGISTER_STATE(display_fsm, error);

// clang-format on

static void on_selected_init(int32_t a, uint32_t b) {
    FSM_SET_STATE(display_fsm, setcontrast);
}

static void on_command(int32_t a, uint32_t b) {
    state_t* current = FSM_GET_STATE(display_fsm);
    if (&setcontrast == current) {
        FSM_SET_STATE(display_fsm, setmode);
    } else if (&setmode == current) {
        FSM_SET_STATE(display_fsm, setcol);
    } else if (&setcol == current) {
        FSM_SET_STATE(display_fsm, setpage);
    } else if (&setpage == current) {
        FSM_SET_STATE(display_fsm, bootmsg);
    } else if (&bootmsg == current) {
        FSM_SET_STATE(display_fsm, next);
    } else if (&idle == current) {
        // noop
        // FSM_SET_STATE(display_fsm, idle);
    } else {
        FSM_SET_STATE(display_fsm, error);
    }
}

static void on_data(int32_t a, uint32_t b) {
    // Clear flags
    framebuffers[write_display].flags &= ~(FB_LOCKED | FB_UPDATE);
    state_t* current = FSM_GET_STATE(display_fsm);
    if (&setcontrast == current) {
        // noop
    } else if (&setmode == current) {
        // noop
    } else if (&setcol == current) {
        // noop
    } else if (&setpage == current) {
        // noop
    } else if (&bootmsg == current) {
        FSM_SET_STATE(display_fsm, next);
    } else if (&running == current) {
        // Check fsm for pending updates
        if (!stack_empty(pending_updates)) {
            display_select(DISPLAY_INDEX_ADDR(stack_peek(pending_updates)),
                           &on_selected_task);
        }
    } else {
        FSM_SET_STATE(display_fsm, error);
    }
}

static void on_selected_task() {
    // Display selected, blit the framebuffer
    if (stack_pop(pending_updates, &current_display)) {
        display_data(current_display);
    }
}

static void display_init() {
    // Setup addressing mode, blank screen
    for (uint8_t i = 0; i < 8; ++i) {
        display_select(DISPLAY_INDEX_ADDR(i));
    }
}

static void display_command(uint8_t cmd, buffer_t data, length_t data_size) {
    // Send
    i2c_write(DISPLAY_COMMAND_ADDRESS, data_size, data, on_command);
}

static uint8_t write_display = 0;

static void display_data(uint8_t display) {
    if (framebuffers[display].flags & FB_LOCKED) {
        framebuffers[display].flags |= FB_UPDATE;
    } else {
        write_display = display;
        framebuffers[display].flags |= FB_LOCKED;
        i2c_write(DISPLAY_DATA_ADDRESS,
                  (framebuffers[display].w / 8) * framebuffers[display].h,
                  framebuffers[display].data, on_data);
    }
}

static void display_task(int32_t status) { fsm_update(display_fsm); };

void display_set_text(uint8_t display, uint8_t x, uint8_t y, const char* text,
                      length_t length) {
    for (uint8_t i = 0; i < length; ++i) {
        // Font is 6x8 (padding included)
        display_blit(display, x + (6 * i), y, font[text[i]], 5, 8);
    }
}

void display_blit(uint8_t display, uint8_t x, uint8_t y, buffer_t img,
                  uint8_t width, uint8_t height) {
    if (framebuffers[display].flags & FB_LOCKED) {
        // Drop the framebuffer update
        return;
    }
    framebuffers[display].flags |= FB_LOCKED;
    // Clamp W and H to edges of display
    uint8_t cw = width;
    if (x + width > framebuffers[display].w) {
        cw = framebuffers[display].w - x;
    }
    if (y + height > framebuffers[display].h) {
        height = framebuffers[display].h - y;
    }
    // Check pixel-byte alignment
    uint8_t shift = x % 8;
    uint8_t* src = img;
    // Calculate offset into framebuffer
    for (uint16_t j = 0; j < height; ++j) {
        if (shift) {
            // uint16_t offset = (y * LINE_WIDTH_BYTES) + (x / 8);
            // fetch 2 bytes, shift, copy byte
            uint8_t* dest_ptr =
                framebuffers[display]
                    .data[((y + j) * LINE_WIDTH_BYTES) + (x / 8)];
            uint8_t window = (src[0] >> shift);
            for (uint16_t i = 0; i < (cw - 1); ++i) {
                dest_ptr[i] |= window;
                window = (src[i] << shift) | (src[i + 1] >> shift);
            }
            dest_ptr[cw - 1] |= src[cw - 1] << shift;
            src += width;
        } else {
            // Copy up to clamped width into FB
            memcpy(framebuffers[display].data[((j + y) * LINE_WIDTH_BYTES) + x],
                   src, cw);
            // Advance source by image pitch, not clamped width
            src += width;
        }
    }
    framebuffers[display].flags &= ~FB_LOCKED;
    if (framebuffers[display].flags & FB_UPDATE) {
        display_data(display);
    }
}
