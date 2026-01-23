#include "game.h"
#include "states.h"
#include "scheduler.h"
#include "events.h"
#include "fsm.h"
#include "gpio.h"
#include "random.h"

#include <stdint.h>
#include <string.h>

#define WORD_ROTATE_PERIOD_MS (500)

static wordlist_t working_list;
static task_handle_t word_task_handle;
static task_handle_t popup_handle;
static const char *messages[4] = {NULL};

static void try_confirm(int32_t status)
{
    UNUSED(status);
    bool all_locked = (IS_LOCKED(&LOCK0_TGL)) &&
                      (IS_LOCKED(&LOCK1_TGL)) &&
                      (IS_LOCKED(&LOCK2_TGL)) &&
                      (IS_LOCKED(&LOCK3_TGL));
    if (all_locked)
    {
        if (game()->current_team == TEAM_A)
        {
            set_current_team(TEAM_B);
            set_state(STATEREF(game, word_select));
        }
        else
        {
            set_state(STATEREF(game, pass));
        }
    }
    else
    {
        flash_unlocked();
    }
};
static void rotate_words(int32_t status)
{
    UNUSED(status);
    bool all_locked = (IS_LOCKED(&LOCK0_TGL)) &&
                      (IS_LOCKED(&LOCK1_TGL)) &&
                      (IS_LOCKED(&LOCK2_TGL)) &&
                      (IS_LOCKED(&LOCK3_TGL));
    if (all_locked)
    {
        messages[0] = "Accept to\ncontinue";
        messages[1] = "Accept to\ncontinue";
        messages[2] = "Accept to\ncontinue";
        messages[3] = "Accept to\ncontinue";
        return;
    }

    if (!IS_LOCKED(&LOCK0_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[0]);
        messages[0] = "Lock to\ncontinue";
    }
    else
    {
        messages[0] = "Locked!";
    }
    if (!IS_LOCKED(&LOCK1_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[1]);
        messages[1] = "Lock to\ncontinue";
    }
    else
    {
        messages[1] = "Locked!";
    }
    if (!IS_LOCKED(&LOCK2_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[2]);
        messages[2] = "Lock to\ncontinue";
    }
    else
    {
        messages[2] = "Locked!";
    }
    if (!IS_LOCKED(&LOCK3_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[3]);
        messages[3] = "Lock to\ncontinue";
    }
    else
    {
        messages[3] = "Locked!";
    }
};

STATE_ENTER(game, word_select)
{
    words_init();
    disp_clear_all();
    event_clear_all();
    ON_ACCEPT(try_confirm);
    rotate_words(0);
    // Check intiial lock states
    if (IS_LOCKED(&LOCK0_TGL))
    {
        messages[0] = "_";
        strncpy(working_list.words[0], "Unlock to\nstart", MAX_WORD_LENGTH);
    }
    else
    {
        messages[0] = "Lock to\ncontinue";
        set_blink_enable(DISP_B0, true);
    }

    if (IS_LOCKED(&LOCK1_TGL))
    {
        messages[1] = "_";
        strncpy(working_list.words[1], "Unlock to\nstart", MAX_WORD_LENGTH);
    }
    else
    {
        messages[1] = "Unlock to\nstart";
        set_blink_enable(DISP_B1, true);
    }

    if (IS_LOCKED(&LOCK2_TGL))
    {
        messages[2] = "_";
        strncpy(working_list.words[2], "Unlock to\nstart", MAX_WORD_LENGTH);
    }
    else
    {
        messages[2] = "Unlock to\nstart";
        set_blink_enable(DISP_B2, true);
    }

    if (IS_LOCKED(&LOCK3_TGL))
    {
        messages[3] = "_";
        strncpy(working_list.words[3], "Unlock to\nstart", MAX_WORD_LENGTH);
    }
    else
    {
        messages[3] = "Unlock to\nstart";
        set_blink_enable(DISP_B3, true);
    }
    word_task_handle = task_periodic(rotate_words, MILLIS(WORD_ROTATE_PERIOD_MS));
}

STATE_UPDATE(game, word_select)
{
    disp_print(DISP_A0, 4, 4, working_list.words[0]);
    disp_print(DISP_A1, 4, 4, working_list.words[1]);
    disp_print(DISP_A2, 4, 4, working_list.words[2]);
    disp_print(DISP_A3, 4, 4, working_list.words[3]);
    disp_print(DISP_B0, 0, 0, messages[0]);
    disp_print(DISP_B1, 0, 0, messages[1]);
    disp_print(DISP_B2, 0, 0, messages[2]);
    disp_print(DISP_B3, 0, 0, messages[3]);
}

STATE_EXIT(game, word_select)
{
    task_abort(word_task_handle);
    memcpy((void *)&get_current_team()->words, (void *)&working_list, sizeof(wordlist_t));
    if (game()->current_team == TEAM_A)
    {
        set_current_team(TEAM_B);
        set_state(STATEREF(game, team_select));
    }
    else
    {
        set_state(STATEREF(game, pass));
    }
}

STATE(game, word_select, enter, update, exit);