#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "defs.h"
#include "images.h"
#include "scheduler.h"
#include "filesystem.h"
#include "states.h"
#include "gpio.h"
#include "display.h"
#include "leds.h"
#include "draw.h"
#include "random.h"
#include "effects.h"

#include "game.h"

#define LED_PERIOD_MS (33)
#define DISPLAY_PERIOD_MS (33)
#define INPUT_PERIOD_MS (100)

static task_handle_t timer_task;
static task_handle_t anim_task;
static const timespan_t ANIM_PERIOD_MS = 1;
static const timespan_t TIMER_PERIOD_MS = 10;
// One minute
static const timespan_t TIMER_DURATION_MS = 20000;
static timespan_t anim_duration_ms;
static timespan_t anim_value_ms;
static timespan_t timer_value_ms;
static timespan_t delta_ms;
static color_t timer_colors[6];
static color_t counter_1_colors[4];
static color_t counter_2_colors[4];
static color_t display_colors[4];
static const color_t GRN = {0, 255, 0};
static const color_t RED = {255, 0, 0};
static const color_t ORG = {252, 157, 30};

static game_state_t game_data;

static bool wordlist_cmp(const wordlist_t *lhs, const wordlist_t *rhs)
{
    for (uint8_t i = 0; i < SEQUENCE_LENGTH; ++i)
    {
        if (lhs->sequence[i] != rhs->sequence[i])
        {
            return false;
        }
    }
    for (uint8_t i = 0; i < WORDLIST_LENGTH; ++i)
    {
        if (strncmp(lhs->words[i], rhs->words[i], MAX_WORD_LENGTH) != 0)
        {
            return false;
        }
    }
    return true;
}

static bool team_cmp(const team_t *lhs, const team_t *rhs)
{
    return lhs->intercepts == rhs->intercepts &&
           lhs->failures == rhs->failures &&
           lhs->successes == rhs->successes &&
           wordlist_cmp(&lhs->words, &rhs->words);
}

static uint8_t normalize(uint16_t in, uint16_t limit)
{
    if (in > limit)
    {
        return 255;
    }
    return (uint8_t)((255 * in) / limit);
}

static uint8_t clamp(uint16_t in, uint16_t max, uint16_t min)
{
    if (in > max)
    {
        return 255;
    }

    if (in < min)
    {
        return 0;
    }

    return (255 * (in - min)) / (max - min);
}

static uint8_t blend_channel(uint16_t a, uint16_t b, uint8_t factor)
{
    a = a * (uint16_t)factor;
    b = b * (255 - (uint16_t)factor);
    uint16_t v = (a + b);
    return (uint8_t)(v >> 8);
}

static color_t blend(color_t a, color_t b, uint8_t factor)
{
    color_t out;
    out.r = blend_channel(a.r, b.r, factor);
    out.g = blend_channel(a.g, b.g, factor);
    out.b = blend_channel(a.b, b.b, factor);
    return out;
}
static callback_t on_blank = NULL;
static void blank_displays(int32_t status)
{
    UNUSED(status);
    static uint8_t display_to_blank = 0;
    if (display_to_blank < DISPLAY_MAX)
    {
        display_clear();
        display_show(display_to_blank, blank_displays);
        ++display_to_blank;
    }
    else
    {
        display_to_blank = 0;
        if (NULL != on_blank)
        {
            on_blank(0);
        }
    }
}

static callback_t timer_callback = NULL;

static void timer_handler(int32_t status)
{
    UNUSED(status);
    timer_value_ms += TIMER_PERIOD_MS;
    if (timer_value_ms > TIMER_DURATION_MS)
    {
        timer_value_ms = 0;
        if (NULL != timer_callback)
        {
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

static void start_timer(callback_t cb)
{
    timer_callback = cb;
    timer_value_ms = 0;
    timer_task = task_periodic(timer_handler, MILLIS(TIMER_PERIOD_MS));
}

static callback_t anim_callback = NULL;
static word_t current_word;
static uint8_t disp = 0;

static void random_word(int32_t status);

static void anim_handler(int32_t status)
{
    UNUSED(status);
    /*switch (gs) {
        default:
            break;
    }*/
    ++disp;
    if (disp < DISPLAY_MAX)
    {
        task_immediate(random_word);
    }
    else
    {
        disp = 0;
        task_delayed(random_word, MILLIS(ANIM_PERIOD_MS));
    }
}

static void random_word(int32_t status)
{
    UNUSED(status);
    bool skip = false;
    switch (disp)
    {
    case 0:
    case 1:
        skip = gpio_get(&LOCK0_TGL);
        break;
    case 2:
    case 3:
        skip = gpio_get(&LOCK1_TGL);
        break;
    case 4:
    case 5:
        skip = gpio_get(&LOCK2_TGL);
        break;
    case 6:
    case 7:
        skip = gpio_get(&LOCK3_TGL);
        break;
    default:
        break;
    }
    if (skip)
    {
        uint16_t rword = uniform(0, words_count());
        uint8_t rx = 0; // uniform(0, 8);
        uint8_t ry = 0; // uniform(0, 8);
        words_get(rword, &current_word);
        display_clear();
        draw_text(rx, ry, current_word, strlen(current_word));
        display_show(disp, anim_handler);
    }
    else
    {
        task_immediate(anim_handler);
    }
}

static void start_animation(callback_t cb, timespan_t duration_ms)
{
    anim_callback = cb;
    anim_duration_ms = duration_ms;
    anim_value_ms = 0;
    on_blank = anim_handler;
    blank_displays(0);
}

static void led_handler() { leds_write(); }

static void game_handler(int32_t status)
{
    UNUSED(status);
    update_state(0);
}

void game_init(int32_t status)
{
    UNUSED(status);
    effects_init();

    set_current_team(TEAM_A);
    set_state(STATEREF(game, splash));
    task_periodic(game_handler, MILLIS(100));
    task_periodic(led_handler, MILLIS(LED_PERIOD_MS));
}

team_t *get_current_team()
{
    team_t *_team = NULL;
    game_state_t *_game = game();
    if (_game != NULL)
    {
        _team = &(_game->teams[_game->current_team ? 0 : 1]);
    }
    return _team;
}

team_t *get_opposing_team()
{
    team_t *_team = NULL;
    game_state_t *_game = game();
    if (_game != NULL)
    {
        _team = &(_game->teams[_game->current_team ? 1 : 0]);
    }
    return _team;
}

void set_current_team(team_name_t team)
{
    game_state_t *_game = game();
    if (_game != NULL)
    {
        switch (team)
        {
        TEAM_A:
            _game->current_team = 0;
            break;
        TEAM_B:
            _game->current_team = 1;
            break;
        default:
            _game->current_team = 0;
            break;
        }
    }
}

void swap_current_team()
{
    game_state_t *_game = game();
    if (_game != NULL)
    {
        _game->current_team = !_game->current_team;
    }
}

void game_new()
{
    game_state_t *_game = game();
    _game->current_team = 0;
    _game->current_round = 0;
    memset(&_game->teams[0], 0, sizeof(team_t));
    memset(&_game->teams[1], 0, sizeof(team_t));
}

game_state_t *game()
{
    return &game_data;
}

void game_save(callback_t oncomplete)
{
    static uint32_t save_index = 0;
    // Write down current game
    game_state_t last_state;
    file_open(GAME_DB);
    // game_state has two semaphors, free and valid
    while (save_index < file_capacity())
    {
        file_read((buffer_t)&last_state, sizeof(game_state_t));
        if ((!last_state.free) && (last_state.valid))
        {
            // We have an in-use, still valid chunk
            // check for changes in current state
            if (last_state.current_team == game()->current_team &&
                last_state.current_round == game()->current_round &&
                team_cmp(&last_state.teams[0], &game()->teams[0]) &&
                team_cmp(&last_state.teams[1], &game()->teams[1]))
            {
                // No change since last save
                oncomplete(STATUS_OK);
                return;
            }
            // back off to last state
            int32_t seek_back = -((int32_t)sizeof(game_state_t));
            file_rseek(seek_back);
            // write-down valid flag
            last_state.valid = 0;
            file_write((buffer_t)&last_state, sizeof(game_state_t));
            break;
        }
        else if (last_state.free)
        {
            // No save data, continue to write-down (use current index)
            int32_t seek_back = -((int32_t)sizeof(game_state_t));
            file_rseek(seek_back);
        }
        else
        {
            // Save data, but it's no longer valid, keep looking
            save_index += sizeof(game_state_t);
        }
    }
    if (save_index >= file_capacity())
    {
        // erase block, will auto-seek to beginning
        file_erase(GAME_DB);
    }
    memcpy(&last_state, game(), sizeof(game_state_t));
    // Reset free flag
    // Set valid flag
    last_state.free = 0;
    last_state.valid = 1;
    file_write((buffer_t)&last_state, sizeof(game_state_t));
    oncomplete(STATUS_OK);
}

void game_load(callback_t oncomplete)
{
    static int32_t save_index = 0;
    // Write down current game
    game_state_t last_state;
    file_open(GAME_DB);
    // game_state has two semaphors, free and valid
    while (save_index < file_capacity())
    {
        file_read((buffer_t)&last_state, sizeof(game_state_t));
        if ((!last_state.free) && (last_state.valid))
        {
            // We have an in-use, still valid chunk
            memcpy(&game_data, &last_state, sizeof(game_state_t));
            oncomplete(STATUS_OK);
            return;
        }
        save_index += sizeof(game_state_t);
    }
    game_data.valid = 0;
    oncomplete(-1);
}

void disp_print(uint8_t display, uint8_t x, uint8_t y, const char *text)
{
    AWAIT(display_select(display, default_future));
    display_clear();
    draw_text(x, y, text, strlen(text));
    AWAIT(display_show(display, default_future));
}

void disp_printf(uint8_t display, uint8_t x, uint8_t y, const char *fmt, ...)
{
    char buffer[DISPLAY_MAX_STRING];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, DISPLAY_MAX_STRING, fmt, args);
    va_end(args);
    disp_print(display, x, y, buffer);
}

void disp_draw(uint8_t display, uint8_t x, uint8_t y, const buffer_t img)
{
    display_clear();
    AWAIT(display_select(display, default_future));
    draw_blit(x, y, img, 128, 32);
    AWAIT(display_show(display, default_future));
}

void disp_clear_all()
{
    display_clear();
    for (uint8_t i = 0; i < DISPLAY_MAX; ++i)
    {
        AWAIT(display_select(i, default_future));
        AWAIT(display_set_inverted(false, default_future));
        AWAIT(display_show(i, default_future));
    }
}