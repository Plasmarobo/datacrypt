#include "game.h"
#include "states.h"
#include "events.h"

static void finish(int32_t status)
{
    set_state(STATEREF(game, menu));
}

STATE_ENTER(game, victory)
{
    disp_clear_all();
    event_clear_all();
    disp_printf(0, 0, 0, "WINNER: %s", game()->current_team ? "TEAM A" : "TEAM B");
    disp_printf(1, 0, 0, "WINS: %d, LOSS: %d\nINTERCEPTS: %d", get_current_team()->successes, get_current_team()->failures, get_current_team()->intercepts);
    disp_printf(2, 0, 0, "RUNNER-UP: %s", game()->current_team ? "TEAM B" : "TEAM A");
    disp_printf(3, 0, 0, "WINS: %d, LOSS: %d\nINTERCEPTS: %d", get_opposing_team()->successes, get_opposing_team()->failures, get_opposing_team()->intercepts);
    ON_ACCEPT(finish);
    ON_CANCEL(finish);
}

STATE(game, victory, enter);