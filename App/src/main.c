
#include <string.h>

#include "bsp.h"
#include "scheduler.h"

static char buffer[32];

#define LEDGPIO(n, g) disp[n] = gpio_get(g) ? RED : GRN

const color_t RED = {255, 0, 0};
const color_t GRN = {0, 255, 0};

void test_leds(int32_t status) {
    static color_t disp[4];

    LEDGPIO(0, LOCK0_TGL);
    LEDGPIO(1, LOCK1_TGL);
    LEDGPIO(2, LOCK2_TGL);
    LEDGPIO(3, LOCK3_TGL);
    set_displed(disp);
    LEDGPIO(0, FAILURE_SW);
    LEDGPIO(1, INTERCEPT_SW);
    LEDGPIO(2, SUCCESS_SW);
    LEDGPIO(3, TURN_SW);
    set_counter0(disp);
    LEDGPIO(0, HIDE_TGL);
    LEDGPIO(1, TIMER_SW);
    LEDGPIO(2, WIN_SW);
    LEDGPIO(3, SHUFFLE_SW);
    set_counter1(disp);
    // Simple 6 bit timer
    static color_t time[6] = {0};
    static uint16_t timer = 0;

    set_timer(time);
    for (uint8_t t = 0; t < 6; ++t) {
        uint8_t value = (timer & (0x01 << t)) ? 255 : 0;
        time[t].r = time[t].b = time[t].g = value;
    }
    timer += 1;
    leds_write();
}

void write_text(int32_t status) {
    display_clear();
    snprintf(buffer, 32, "P %04x", random());
    display_set_text(1, 1, buffer, strlen(buffer));
    display_show(0, NULL);
    task_delayed(write_text, MILLIS(1000));
}

void delay_text(int32_t status) { task_delayed(write_text, MILLIS(2000)); }

int main() {
    scheduler_init();
    bsp_init();
    leds_init();
    display_init(delay_text);
    // flash_init(NULL);
    task_periodic(test_leds, MILLIS(200));
    serial_write("\r\nBOOT\r\n", 8, NULL);
    scheduler_freerun();
    return 0;
}
