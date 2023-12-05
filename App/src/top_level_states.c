
#include "top_level_states.h"

#include "bsp.h"
#include "fsm.h"
#include "game.h"
#include "scheduler.h"

#define APP_UPDATE_PERIOD_MS (100)

static uint8_t selection = 0;

// From boot, run through these top level states
FSM(app_fsm);
STATE_UPDATE(init){};
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
        FSM_SET_STATE(app_fsm, game);
    } else if (selection & 0x01) {
        FSM_SET_STATE(app_fsm, test);
    }
}
STATE_ENTER(mode_select) {
    display_clear_all();

    gpio_set_callback(LOCK0_TGL, select_test);
    gpio_set_callback(LOCK1_TGL, select_game);
    gpio_set_callback(LOCK3_TGL, start)
};
STATE_UPDATE(mode_select) {
    // Selection hierarchy
    display_clear_all();
    if (selection & 0x02) {
        // Game selected
        display_set_inverted(0, true);
        display_set_text(0, 1, 1, "GAME", 4);
        display_set_inverted(1, false);
        display_set_text(1, 1, 1, "test", 4);
        display_set_text(3, 1, 1, "START", 5);
    } else if (selection & 0x01) {
        // Test selected
        display_set_inverted(0, false);
        display_set_text(0, 1, 1 "game", 4);
        display_set_inverted(1, true);
        display_set_text(1, 1, 1, "TEST", 4);
        display_set_text(3, 1, 1, "START", 5);
    } else {
        // Nothing selected
        display_set_inverted(0, false);
        display_set_inverted(1, false);
        display_set_text(0, 1, 1 "game", 4);
        display_set_text(1, 1, 1, "test", 4);
    }
};
STATE_EXIT(mode_select){

};
STATE_ENTER(test) {
    // Clear displays, clear leds
    display_clear_all();
    void set_disp([
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {255, 255, 255},
    ]);
    void set_counter0([
        {32, 0, 0},
        {64, 0, 0},
        {96, 0, 0},
        {128, 0, 0},
    ]);
    void set_counter1([
        {0, 32, 0},
        {0, 64, 0},
        {0, 96, 0},
        {0, 128, 0},
    ]);
    void set_timer([
        {255, 0, 0},
        {255, 32, 0},
        {255, 64, 0},
        {128, 128, 0},
        {0, 255, 0},
        {0, 255, 0},
    ]);
    for (uint8_t i = 0; i < DISPLAY_MAX; ++i) {
        display_set_text(i, 2, 2, "test pattern", 12);
    }
    FSM_SET_STATE(test, idle);
};
STATE_UPDATE(test){};
STATE_ENTER(game) {
    // Clear displays, clear leds
    // Clear gamestate
    FSM_SET_STATE(game, start);
};
STATE_UPDATE(game) { fsm_update(&game); };

REGISTER_STATE(app_fsm, init);
REGISTER_STATE(app_fsm, mode_select);
REGISTER_STATE(app_fsm, game);
REGISTER_STATE(app_fsm, test);

static void app_task(int32_t status) { fsm_update(&app_fsm); }

void app_init() {
    task_periodic(&app_task, MILLIS(APP_UPDATE_PERIOD_MS));
    FSM_SET_STATE(app_fsm, init);
}
