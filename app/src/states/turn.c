
#include "game.h"
#include "states.h"
#include "events.h"
#include "gpio.h"
#include "random.h"

#define INTERCEPTS_TO_WIN (3)
#define FAILURES_TO_LOSE (3)

static uint8_t intercepts_available;

static void change_sort(int32_t status)
{
    UNUSED(status);
    if (gpio_get(&LOCK0_TGL))
    {
        // Sort in word order
        disp_print(0, 2, 2, get_current_team()->words.words[0]);
        disp_print(1, 2, 2, get_current_team()->words.words[1]);
        disp_print(2, 2, 2, get_current_team()->words.words[2]);
        disp_print(3, 2, 2, get_current_team()->words.words[3]);
    }
    else
    {
        // Sort in sequence order
        for (uint8_t i = 0; i < SEQUENCE_LENGTH; ++i)
        {
            disp_print(i, 2, 2, get_current_team()->words.words[get_current_team()->words.sequence[i]]);
        }
    }
}

static void handle_intercept(int32_t status)
{
    UNUSED(status);
    if (intercepts_available > 0)
    {
        get_opposing_team()->intercepts += 1;
        --intercepts_available;
        if (get_opposing_team()->intercepts > INTERCEPTS_TO_WIN)
        {
            swap_current_team();
            set_state(STATEREF(game, victory));
        }
    }
}

static void handle_success(int32_t status)
{
    UNUSED(status);
    get_current_team()->successes += 1;
    set_state(STATEREF(game, pass));
}

static void handle_failure(int32_t status)
{
    UNUSED(status);
    get_current_team()->failures += 1;
    if (get_current_team()->failures > FAILURES_TO_LOSE)
    {
        swap_current_team();
        set_state(STATEREF(game, victory));
    }
    set_state(STATEREF(game, pass));
}

STATE_ENTER(game, turn)
{
    disp_clear_all();
    event_clear_all();
    // Generate a sequence
    intercepts_available = 1;
    uint8_t proposed_sequence[4] = {1, 2, 3, 4};
    int shuffle_passes = 64;
    for (int i = 0; i < shuffle_passes; i++)
    {
        int from = uniform(0, 3);
        int to = from;
        while (from == to)
        {
            to = uniform(0, 3);
        }
        uint8_t tmp = proposed_sequence[to];
        proposed_sequence[to] = proposed_sequence[from];
        proposed_sequence[from] = tmp;
    }
    get_current_team()->words.sequence[0] = proposed_sequence[0];
    get_current_team()->words.sequence[1] = proposed_sequence[1];
    get_current_team()->words.sequence[2] = proposed_sequence[2];
    change_sort(0);
    ON_LOCK0(change_sort);
    ON_LEFT(handle_intercept);
    ON_ACCEPT(handle_success);
    ON_CANCEL(handle_failure);
}

STATE(game, turn, enter);