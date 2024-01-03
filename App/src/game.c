#include "game.h"

#include <stdarg.h>
#include <stdint.h>

#include "bsp.h"
#include "scheduler.h"

#define TEAM_COUNT (2)

static uint8_t current_team;
static uint8_t opposing_team;
static team_t teams[TEAM_COUNT];
static game_state_t gs;
static task_handle_t timer_task;
static task_handle_t anim_task;
static const timespan_t ANIM_PERIOD_MS = 100;
static const timespan_t TIMER_PERIOD_MS = 100;
// One minute
static const timespan_t TIMER_DURATION_MS = 20000;
static timespan_t anim_duration_ms;
static timespan_t anim_value_ms;
static timespan_t timer_value_ms;
static color_t timer_colors[6];
static const color_t GRN = {0, 255, 0};
static const color_t RED = {255, 0, 0};
static const color_t ORG = {252, 157, 30};

static uint8_t normalize(uint16_t in, uint16_t limit) {
    if (in > limit) {
        return 255;
    }
    return (uint8_t)((255 * in) / limit);
}

static uint8_t clamp(uint16_t in, uint16_t max, uint16_t min) {
    if (in > max) {
        return 255;
    }

    if (in < min) {
        return 0;
    }

    return (255 * (in - min)) / (max - min);
}

static uint8_t blend_channel(uint16_t a, uint16_t b, uint8_t factor) {
    a = a * (uint16_t)factor;
    b = b * (255 - (uint16_t)factor);
    uint16_t v = (a + b);
    return (uint8_t)(v >> 8);
}

static color_t blend(color_t a, color_t b, uint8_t factor) {
    color_t out;
    out.r = blend_channel(a.r, b.r, factor);
    out.g = blend_channel(a.g, b.g, factor);
    out.b = blend_channel(a.b, b.b, factor);
    return out;
}

static void blank_displays(int32_t status) {
    static uint8_t display_to_blank = 0;
    if (display_to_blank < DISPLAY_MAX) {
        display_clear();
        display_show(display_to_blank, blank_displays);
        ++display_to_blank;
    } else {
        display_to_blank = 0;
    }
}

static callback_t timer_callback = NULL;

static void timer_handler(int32_t status) {
    timer_value_ms += TIMER_PERIOD_MS;
    if (timer_value_ms > TIMER_DURATION_MS) {
        gs = SCORE_PHASE;
        blank_displays(0);
        if (NULL != timer_callback) {
            timer_callback(0);
        }
        timer_callback = NULL;
        return;
    }
    color_t master_color;
    const color_t fade = {0, 0, 0};

    if (timer_value_ms < (TIMER_DURATION_MS / 3)) {
        master_color = GRN;
    } else if (timer_value_ms < (2 * TIMER_DURATION_MS / 3)) {
        master_color = ORG;
    } else {
        master_color = RED;
    }
    // Convert to fixed point
    uint8_t v = (uint8_t)((255 * (TIMER_DURATION_MS - timer_value_ms) /
                           TIMER_DURATION_MS));
    timer_colors[0] = blend(fade, master_color, clamp(255 - v, 255, 255 - 42));
    timer_colors[1] =
        blend(fade, master_color, clamp(255 - v, 255 - 42, 255 - 84));
    timer_colors[2] =
        blend(fade, master_color, clamp(255 - v, 255 - 84, 255 - 126));
    timer_colors[3] =
        blend(fade, master_color, clamp(255 - v, 255 - 126, 255 - 168));
    timer_colors[4] =
        blend(fade, master_color, clamp(255 - v, 255 - 168, 255 - 210));
    timer_colors[5] = blend(fade, master_color, clamp(255 - v, 255 - 210, 0));
    set_timer(timer_colors);
    leds_write();
}

static void start_timer(callback_t cb) {
    timer_callback = cb;
    timer_value_ms = 0;
    timer_task = task_periodic(timer_handler, MILLIS(TIMER_PERIOD_MS));
}

static callback_t anim_callback = NULL;
static word_t current_word;

static void random_word(int32_t status);

static void anim_handler(int32_t status) {
    /*switch (gs) {
        default:
            break;
    }*/
    task_delayed(random_word, MILLIS(ANIM_PERIOD_MS));
}

static void random_word(int32_t status) {
    uint16_t rword = uniform(0, word_count());
    uint8_t rx = uniform(0, 8);
    uint8_t ry = uniform(0, 8);
    uint8_t rd = uniform(0, 8);
    dbgprintf("RW %d RX %d RY %d RD %d\r\n", rword, rx, ry, rd);
    get_word(rword, &current_word);
    display_clear();
    display_set_text(rx, ry, current_word, strlen(current_word));
    display_show(rd, anim_handler);
}

static void start_animation(callback_t cb, timespan_t duration_ms) {
    anim_callback = cb;
    anim_duration_ms = duration_ms;
    anim_value_ms = 0;
    blank_displays(0);
    anim_task = task_delayed(anim_handler, MILLIS(ANIM_PERIOD_MS));
}

static void game_handler(int32_t status) {
    switch (gs) {
        case SPLASH:
            break;
        case MAIN_MENU:
            break;
        case WORD_SELECT:
            break;
        case PASS_DEVICE:
            break;
        case SEQUENCE_PHASE:
            break;
        case START_PHASE:
            break;
        case TIME_PHASE:
            start_timer(NULL);
            break;
        case SCORE_PHASE:
            start_timer(NULL);
            start_animation(NULL, 1000);
            break;
        case GAME_OVER:
            break;
        default:
            break;
    }
}

void game_init(int32_t status) {
    current_team = TEAM_A;
    opposing_team = TEAM_B;
    gs = SCORE_PHASE;
    task_delayed(game_handler, MILLIS(2000));
}

void game_update(int32_t status) {}
