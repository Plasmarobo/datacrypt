
#include <string.h>

#include "adc.h"
#include "bsp.h"
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
    flash_init(NULL);
    rpc_init();
    serial_write("\r\nBOOT Complete\r\n", 17, NULL);
    scheduler_freerun();
    return 0;
}
