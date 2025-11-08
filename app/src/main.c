
#include <string.h>

#include "adc.h"
#include "hal.h"
#include "game.h"
#include "scheduler.h"
#include "rpc.h"
#include "filesystem.h"

int main() {
    // HW init
    hal_hw_init();
    // Task init
    scheduler_init();
    // Late HW init (task-dependent)
    hal_task_init();
    // HAL Init
    random_init();
    leds_init();
    display_init(game_init);
    serial_write("\r\nBOOT\r\n", 8, NULL);
    filesystem_init();
    rpc_init();
    // Start exec
    game_init(0);
    scheduler_freerun();
    return 0;
}
