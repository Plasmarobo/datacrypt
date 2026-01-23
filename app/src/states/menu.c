#include "states.h"
#include "scheduler.h"
#include "events.h"
#include <string.h>
#include "display.h"
#include "draw.h"
#include "game.h"

static void start_new(int32_t status);
static void resume(int32_t status);
static void settings(int32_t status);

typedef struct
{
    uint8_t index;
    char *name;
    callback_t on_selected;
} menu_item_t;

static int8_t selected_item;
static menu_item_t menu[] = {
    {DISP_A0, "New Game", start_new},
    {DISP_A1, "Resume", resume},
    {DISP_A2, "Settings", settings},
};

static void start_new(int32_t status)
{
    UNUSED(status);
    game_new();
    set_state(STATEREF(game, team_select));
}

static void resume(int32_t status)
{
    UNUSED(status);
    set_state(STATEREF(game, load));
}

static void settings(int32_t status)
{
    UNUSED(status);
    set_state(STATEREF(game, settings));
}

static void select_next(int32_t status)
{
    UNUSED(status);
    selected_item += 1;
    if (selected_item >= sizeof(menu) / sizeof(menu_item_t))
    {
        selected_item = 0;
    }
    dbgprintf("Selected item: %d\n", (int32_t)selected_item);
}
static void select_prev(int32_t status)
{
    UNUSED(status);
    selected_item -= 1;
    if (selected_item < 0)
    {
        selected_item = (sizeof(menu) / sizeof(menu_item_t)) - 1;
    }
    dbgprintf("Selected item: %d\n", (int32_t)selected_item);
}

static void confirm_selection(int32_t status)
{
    UNUSED(status);
    if ((selected_item < sizeof(menu) / sizeof(menu_item_t)) &&
        (menu[selected_item].on_selected != NULL))
    {
        dbgprintf("Confirming selection: %d\n", (int32_t)selected_item);
        menu[selected_item].on_selected(0);
    }
    else
    {
        // Unknown error - return to root
        set_state(STATEREF(game, splash));
        dbgprintf("Unknown error\n");
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
        menu_item_t *m = &menu[i];
        AWAIT(display_select(m->index, default_future));
        display_clear();
        AWAIT(display_set_inverted(selected_item == i, default_future));
        draw_text(8, 8, m->name, strlen(m->name));
        AWAIT(display_show(m->index, default_future));
    }
}

STATE(game, menu, enter, update);