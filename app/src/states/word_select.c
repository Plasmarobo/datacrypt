#include "states.h"
#include "game.h"
#include "scheduler.h"
#include "events.h"
#include "fsm.h"
#include "gpio.h"

#include <stdint.h>
#include <string.h>

#define WORD_ROTATE_PERIOD_MS (2500)
#define WORD_POPUP_TIME (3000)
#define IS_LOCKED(l) (!gpio_get(l))

static wordlist_t working_list;
static task_handle_t word_task_handle;
static task_handle_t popup_handle;
static const char *message_override = NULL;

static void clear_popup(int32_t status)
{
    message_override = NULL;
}

static void popup(const char *msg)
{
    message_override = msg;
    task_delayed(clear_popup, MILLIS(WORD_POPUP_TIME));
}

static void try_confirm(int32_t status)
{
    bool all_locked = (IS_LOCKED(LOCK0_TGL)) &&
                      (IS_LOCKED(LOCK1_TGL)) &&
                      (IS_LOCKED(LOCK2_TGL)) &&
                      (IS_LOCKED(LOCK3_TGL));
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
        popup("Lock to \ncontinue");
    }
};
static void rotate_words(int32_t status)
{
    if (!IS_LOCKED(LOCK0_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[0]);
    }
    if (!IS_LOCKED(LOCK1_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[1]);
    }
    if (!IS_LOCKED(LOCK2_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[2]);
    }
    if (!IS_LOCKED(LOCK3_TGL))
    {
        words_get(random_int() % words_count(), &working_list.words[3]);
    }
};

STATE_ENTER(game, word_select)
{
    event_clear_all();
    disp_clear_all();
    ON_ACCEPT(try_confirm);
    rotate_words(0);
    word_task_handle = task_periodic(rotate_words, MILLIS(WORD_ROTATE_PERIOD_MS));
}

STATE_UPDATE(game, word_select)
{
    if (message_override && !IS_LOCKED(LOCK0_TGL))
    {
        disp_printf(0, 4, 4, "%s", message_override);
    }
    else
    {
        disp_print(0, 4, 4, working_list.words[0]);
    }
    if (message_override && !IS_LOCKED(LOCK1_TGL))
    {
        disp_printf(1, 4, 4, "%s", message_override);
    }
    else
    {
        disp_print(1, 4, 4, working_list.words[1]);
    }
    if (message_override && !IS_LOCKED(LOCK2_TGL))
    {
        disp_printf(2, 4, 4, "%s", message_override);
    }
    else
    {
        disp_print(2, 4, 4, working_list.words[2]);
    }
    if (message_override && !IS_LOCKED(LOCK3_TGL))
    {
        disp_printf(3, 4, 4, "%s", message_override);
    }
    else
    {
        disp_print(3, 4, 4, working_list.words[3]);
    }
}

STATE_EXIT(game, word_select)
{
    task_abort(word_task_handle);
    task_abort(popup_handle);
    memcpy((void *)&get_current_team()->words, (void *)&working_list, sizeof(wordlist_t));
}

STATE(game, word_select, enter, update, exit);