
#include "top_level_states.h"

#include "bsp.h"
#include "fsm.h"
#include "game.h"
#include "scheduler.h"

#define APP_UPDATE_PERIOD_MS (100)

static uint8_t selection = 0;

// From boot, run through these top level states
static void select_test(int32_t value);
static void select_game(int32_t value);
static void start(int32_t value);

STATE_ENTER(app_fsm, mode_select) {
    display_clear_all();

    gpio_set_callback(LOCK0_TGL, &select_test);
    gpio_set_callback(LOCK1_TGL, &select_game);
    gpio_set_callback(LOCK3_TGL, &start);
};
STATE_UPDATE(app_fsm, mode_select) {
    // Selection hierarchy
    display_clear_all();
    if (selection & 0x02) {
        // Game selected
        display_set_inverted(true, NULL);
        display_set_text(1, 1, "GAME", 4);
        display_set_inverted(false, NULL);
        display_set_text(1, 1, "test", 4);
        display_set_text(1, 1, "START", 5);
    } else if (selection & 0x01) {
        // Test selected
        display_set_inverted(false, NULL);
        display_set_text(1, 1, "game", 4);
        display_set_inverted(true, NULL);
        display_set_text(1, 1, "TEST", 4);
        display_set_text(1, 1, "START", 5);
    } else {
        // Nothing selected
        display_set_inverted(false, NULL);
        display_set_inverted(false, NULL);
        display_set_text(1, 1, "game", 4);
        display_set_text(1, 1, "test", 4);
    }
};
STATE(app_fsm, mode_select, enter, update);
STATE_ENTER(app_fsm, test) {
    // Clear displays, clear leds
    display_clear_all();
    set_disp((color_t[]){
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 255},
    });
    set_counter0((color_t[]){
        {32, 0, 0},
        {64, 0, 0},
        {96, 0, 0},
        {128, 0, 0},
    });
    set_counter1((color_t[]){
        {0, 32, 0},
        {0, 64, 0},
        {0, 96, 0},
        {0, 128, 0},
    });
    set_timer((color_t[]){
        {255, 0, 0},
        {255, 32, 0},
        {255, 64, 0},
        {128, 128, 0},
        {0, 255, 0},
        {0, 255, 0},
    });
    for (uint8_t i = 0; i < DISPLAY_MAX; ++i) {
        display_set_text(2, 2, "test pattern", 12);
    }
};
STATE(app_fsm, test, enter);

static void select_test(int32_t value) {
    if (value) {
        selection |= 0x01;
    } else {
        selection &= ~0x01;
    }
};
static void select_game(int32_t value) {
    if (value) {
        selection |= 0x02;
    } else {
        selection &= ~0x02;
    }
};
static void start(int32_t value) {
    if (selection & 0x02) {
        // FSM_SET_STATE(app_fsm, game);
    } else if (selection & 0x01) {
        fsm_set_state(NULL, STATEREF(app_fsm, test));
    }
};

static void app_task(int32_t status) { fsm_update(STATEREF(app_fsm, test)); }

void app_init() { task_periodic(&app_task, MILLIS(APP_UPDATE_PERIOD_MS)); }
