#include "states.h"
#include "images.h"
#include "scheduler.h"
#include "game.h"
#include "display.h"
#include "draw.h"

#define SPLASH_TIME_MS (2000)

static void load_menu(int32_t status)
{
    UNUSED(status);
    // Next state is menu
    set_state(STATEREF(game, menu));
}

STATE_ENTER(game, splash)
{
    // Display
    disp_clear_all();
    AWAIT(display_select(0, default_future));
    draw_blit(0, 0, (buffer_t)img_millibyte_alt_cropped, 128, 32);
    AWAIT(display_show(0, default_future));
    // can check default_status here if necessary
    task_delayed(load_menu, MILLIS(SPLASH_TIME_MS));
}

STATE(game, splash, enter);