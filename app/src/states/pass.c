
#include "game.h"
#include "states.h"
#include "events.h"
#include "gpio.h"

static void ready(int32_t status)
{
    if (gpio_get(LOCK3_TGL))
    {
        set_state(STATEREF(game, turn));
    }
    else
    {
        // Noop
    }
}

STATE_ENTER(game, pass)
{
    swap_current_team();
    event_clear_all();
    disp_clear_all();
    disp_print(0, 0, 0, "PASS TO");
    disp_printf(1, 0, 0, "%s,", game()->current_team ? "TEAM A" : "TEAM B");
    disp_print(2, 0, 0, "TOGGLE LOCK 3");
    disp_printf(3, 0, 0, "WHEN  %s READY", game()->current_team ? "TEAM A" : "TEAM B");
    ON_LOCK3(ready);
}

STATE(game, pass, enter);