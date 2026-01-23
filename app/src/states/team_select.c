#include "game.h"
#include "states.h"
#include "display.h"
#include "gpio.h"
#include "scheduler.h"
#include "events.h"
#include "effects.h"

static void try_confirm_team(int32_t status)
{
    UNUSED(status);
    bool all_locked = (IS_LOCKED(&LOCK0_TGL)) &&
                      (IS_LOCKED(&LOCK1_TGL)) &&
                      (IS_LOCKED(&LOCK2_TGL)) &&
                      (IS_LOCKED(&LOCK3_TGL));
    if (all_locked)
    {
        set_state(STATEREF(game, word_select));
    }
    else
    {
        flash_unlocked();
    }
}

STATE_ENTER(game, team_select)
{
    const char *name = game()->current_team == TEAM_A ? "Team A" : "Team B";
    disp_clear_all();
    ON_ACCEPT(try_confirm_team);
    disp_printf(DISP_A0, 0, 0, "%s\nONLY", name);
    disp_printf(DISP_A1, 0, 0, "Pass to\n%s", name);
    disp_printf(DISP_A2, 0, 0, "%s:\nLock\nall, then", name);
    disp_print(DISP_A3, 0, 0, "Acept to\ncontinue");
}

STATE_UPDATE(game, team_select)
{
    if (IS_LOCKED(&LOCK0_TGL))
    {

        display_select(DISP_B0, default_future);
        display_set_inverted(false, default_future);
        disp_print(DISP_B0, 0, 0, "Ready");
    }
    else
    {
        display_select(DISP_B0, default_future);
        display_set_inverted(true, default_future);
        disp_print(DISP_A0, 0, 0, "Lock");
    }
    if (IS_LOCKED(&LOCK1_TGL))
    {
        display_select(DISP_B1, default_future);
        display_set_inverted(false, default_future);
        disp_print(DISP_B1, 0, 0, "Ready");
    }
    else
    {
        display_select(DISP_B1, default_future);
        display_set_inverted(true, default_future);
        disp_print(DISP_A1, 0, 0, "Lock");
    }
    if (IS_LOCKED(&LOCK2_TGL))
    {
        display_select(DISP_B2, default_future);
        display_set_inverted(false, default_future);
        disp_print(DISP_B2, 0, 0, "Ready");
    }
    else
    {
        display_select(DISP_B2, default_future);
        display_set_inverted(true, default_future);
        disp_print(DISP_A2, 0, 0, "Lock");
    }
    if (IS_LOCKED(&LOCK3_TGL))
    {
        display_select(DISP_B3, default_future);
        display_set_inverted(false, default_future);
        disp_print(DISP_B3, 0, 0, "Ready");
    }
    else
    {
        display_select(DISP_B3, default_future);
        display_set_inverted(true, default_future);
        disp_print(DISP_A3, 0, 0, "Lock");
    }
}

STATE(game, team_select, enter, update);