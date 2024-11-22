#include "game.h"

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "bsp.h"
#include "scheduler.h"
#include "words.h"
#include "images.h"

#define LED_PERIOD_MS (33)
#define DISPLAY_PERIOD_MS (100)
#define INPUT_PERIOD_MS (100)

#define TEAM_COUNT (2)

const char* MENU_NAME_TXT = "Menu";
const char* NEW_GAME_TXT = "New Game";
const char* CONTINUE_TXT = "Continue";
const char* GAME_INSTRUCTION_TXT = "Flip switch\npress SELECT";

static uint8_t current_team;
static uint8_t opposing_team;
static team_t teams[TEAM_COUNT];
static game_state_t gs;
static task_handle_t timer_task;
static task_handle_t anim_task;
static const timespan_t ANIM_PERIOD_MS = 1;
static const timespan_t TIMER_PERIOD_MS = 10;
// One minute
static const timespan_t TIMER_DURATION_MS = 20000;
static timespan_t anim_duration_ms;
static timespan_t anim_value_ms;
static timespan_t timer_value_ms;
static color_t timer_colors[6];
static color_t counter_1_colors[4];
static color_t counter_2_colors[4];
static color_t display_colors[4];
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

static void blank_displays(void) {
    future_t future;
    int32_t status;
    display_clear();
    for(uint8_t i = 0; i < DISPLAY_MAX; ++i)
    {
        WITH_FUTURE(display_show(i, future), DISPLAY_PERIOD_MS);
    }
}

static callback_t timer_callback = NULL;

static void timer_handler(int32_t status) {
    timer_value_ms += TIMER_PERIOD_MS;
    if (timer_value_ms > TIMER_DURATION_MS) {
        timer_value_ms = 0;
        gs = SCORE_PHASE;
        if (NULL != timer_callback) {
            timer_callback(0);
        }
        timer_callback = NULL;
        return;
    }
    color_t master_color;
    const color_t fade = {0, 0, 0};

    uint8_t v = (uint8_t)((255 * (TIMER_DURATION_MS - timer_value_ms) /
                           TIMER_DURATION_MS));

    master_color = blend(GRN, RED, v);
    // Convert to fixed point
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
}

static void start_timer(callback_t cb) {
    timer_callback = cb;
    timer_value_ms = 0;
    timer_task = task_periodic(timer_handler, MILLIS(TIMER_PERIOD_MS));
}

static callback_t anim_callback = NULL;
static word_t current_word;
static uint8_t disp = 0;

static void random_word(int32_t status);

static void anim_handler(int32_t status) {
    /*switch (gs) {
        default:
            break;
    }*/
    ++disp;
    if (disp < DISPLAY_MAX) {
        task_immediate(random_word);
    } else {
        disp = 0;
        task_delayed(random_word, MILLIS(ANIM_PERIOD_MS));
    }
}

static void random_word(int32_t status) {
    bool skip = false;
    switch (disp) {
        case 0:
        case 1:
            skip = gpio_get(LOCK0_TGL);
            break;
        case 2:
        case 3:
            skip = gpio_get(LOCK1_TGL);
            break;
        case 4:
        case 5:
            skip = gpio_get(LOCK2_TGL);
            break;
        case 6:
        case 7:
            skip = gpio_get(LOCK3_TGL);
            break;
        default:
            break;
    }
    if (skip) {
        uint16_t rword = uniform(0, words_count());
        uint8_t rx = 0;  // uniform(0, 8);
        uint8_t ry = 0;  // uniform(0, 8);
        words_get(rword, &current_word);
        display_clear();
        display_set_text(rx, ry, current_word, strlen(current_word));
        display_show(disp, anim_handler);
    } else {
        task_immediate(anim_handler);
    }
}

static void start_animation(callback_t cb, timespan_t duration_ms) {
    anim_callback = cb;
    anim_duration_ms = duration_ms;
    anim_value_ms = 0;
    blank_displays();
    anim_handler(0);
}

static void led_handler() { leds_write(); }

static void game_handler(int32_t _status) {
    future_t future;
    int32_t status = 0;
    char message[32];
    switch (gs) {
        case SPLASH:
            for(uint8_t i = 0; i < DISPLAY_MAX; ++i)
            {
                display_clear();
                snprintf(message, 32, "display %d", i);
                message[31] = '\0';
                display_set_text(1,1, message, strlen(message));
                WITH_FUTURE(display_show(i, future), MILLIS(DISPLAY_PERIOD_MS));
            }
            //gs = MAIN_MENU;
            //task_delayed(game_handler, 2000);
            break;
        case MAIN_MENU:
            blank_displays();
            status = 0xFFFFFFFF;
            display_set_text(1,1, MENU_NAME_TXT, strlen(MENU_NAME_TXT));
            WITH_FUTURE(display_show(0, future), DISPLAY_PERIOD_MS);
            display_set_text(1,1, NEW_GAME_TXT, strlen(NEW_GAME_TXT));
            WITH_FUTURE(display_show(1, future), DISPLAY_PERIOD_MS);
            display_set_text(1,1, CONTINUE_TXT, strlen(CONTINUE_TXT));
            WITH_FUTURE(display_show(2, future), DISPLAY_PERIOD_MS);
            display_set_text(1,1, GAME_INSTRUCTION_TXT, strlen(GAME_INSTRUCTION_TXT));
            WITH_FUTURE(display_show(4, future), DISPLAY_PERIOD_MS);
            break;
        case WORD_SELECT_A:
            break;
        case WORD_SELECT_B:
            break;
        case PASS_DEVICE_A:
            break;
        case PASS_DEVICE_B:
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
    gs = SPLASH;
    words_init();
    task_delayed(game_handler, MILLIS(2000));
    task_periodic(led_handler, MILLIS(LED_PERIOD_MS));
}

void game_update(int32_t status) {}
