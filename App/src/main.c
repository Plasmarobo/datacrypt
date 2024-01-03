
#include <string.h>

#include "adc.h"
#include "bsp.h"
#include "game.h"
#include "scheduler.h"

int main() {
    bsp_init();
    scheduler_init();
    adc_init(NULL);
    // random_init();
    leds_init();
    display_init(game_init);
    // flash_init(NULL);
    serial_write("\r\nBOOT\r\n", 8, NULL);
    scheduler_freerun();
    return 0;
}
