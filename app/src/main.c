
#include <string.h>

#include "adc.h"
#include "bsp.h"
#include "filesystem.h"
#include "flash.h"
#include "game.h"
#include "rpc.h"
#include "scheduler.h"

int main() {
    bsp_init();
    scheduler_init();
    adc_init(NULL);
    random_init();
    leds_init();
    display_init(game_init);
    flash_init(filesystem_init);
    rpc_init();
    dbgprint("\r\nBOOT Complete\r\n");
    scheduler_freerun();
    return 0;
}
