#include "states.h"
#include "scheduler.h"
#include "events.h"

static void start_new(int32_t status);
static void resume(int32_t status);
static void settings(int32_t status);

typedef struct
{
    char *name;
    callback_t on_selected;
} menu_item_t;

static uint8_t selected_item;
static menu_item_t menu[] = {
    {"New Game", start_new},
    {"Resume", resume},
    {"Settings", settings},
};

static void start_new(int32_t status)
{
    game_new();
    set_state(STATEREF(game, word_select));
}

static void resume(int32_t status)
{
    set_state(STATEREF(game, load));
}

static void settings(int32_t status)
{
    set_state(STATEREF(game, settings));
}

static void select_next(int32_t status)
{
    selected_item += 1;
    if (selected_item >= sizeof(menu) / sizeof(menu_item_t))
    {
        selected_item = 0;
    }
}
static void select_prev(int32_t status)
{
    selected_item -= 1;
    if (selected_item < 0)
    {
        selected_item = (sizeof(menu) / sizeof(menu_item_t)) - 1;
    }
}

static void confirm_selection(int32_t status)
{
    if ((selected_item < sizeof(menu) / sizeof(menu_item_t)) &&
        (menu[selected_item].on_selected != NULL))
    {
        menu[selected_item].on_selected(0);
    }
    else
    {
        // Unknown error - return to root
        set_state(STATEREF(game, splash));
    }
}

STATE_ENTER(game, menu)
{
    event_clear_all(); // Remove any existing event handlers
    disp_clear_all();
    ON_LEFT(select_prev);
    ON_RIGHT(select_next);
    ON_ACCEPT(confirm_selection);
    selected_item = 0;
}

STATE_UPDATE(game, menu)
{
    for (uint8_t i = 0; i < sizeof(menu) / sizeof(menu_item_t); ++i)
    {
        display_clear();
        AWAIT(display_set_inverted(selected_item == i, default_future));
        draw_text(8, 8, menu[i].name, strlen(menu[i].name));
        AWAIT(display_show(i, default_future));
    }
}

STATE(game, menu, enter, update);