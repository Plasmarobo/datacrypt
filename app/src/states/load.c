#include "game.h"
#include "states.h"
#include "debug.h"

static void on_load(int32_t status)
{
    if (status)
    {
        dbgprintf("Could not load game\n");
        set_state(STATEREF(game, menu));
    }
    else
    {
        set_state(STATEREF(game, turn));
    }
}

STATE_ENTER(game, load)
{
    game_load(on_load);
}

STATE(game, load, enter);