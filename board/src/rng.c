#include "bsp.h"
#include "debug.h"
#include "scheduler.h"

#define ENTROPIC_DEPTH (32)
#define ENTROPY_PERIOD_US (75)

volatile static uint32_t current_entropy = 0;
volatile static uint32_t seed = 0xDEADBEEF;
volatile static uint32_t pool = 0xDEADBEEF;

static void gather_entropy(int32_t status) {
    if (current_entropy < ENTROPIC_DEPTH) {
        ++current_entropy;
        uint16_t v = temp_read_raw() & 0x3;
        if (v == 0x00) {
            v = 0x00;
        } else if (v == 0x01) {
            v = 0x00;
        } else if (v == 0x10) {
            v = 0x01;
        } else {
            v = 0x01;
        }
        pool = (seed << 1) | (v);
        task_delayed_unique(gather_entropy, MICROS(ENTROPY_PERIOD_US));
    } else {
        seed = pool;
    }
}

void random_init() { gather_entropy(0); }

#define RANDOM_MAX (0x7FFFFFFF)

uint32_t random_int() {
    seed = (seed * 1103515245L + 12345L) % RANDOM_MAX;
    current_entropy = 0;
    task_delayed_unique(gather_entropy, MICROS(ENTROPY_PERIOD_US));
    return seed;
}

#define REJECTION_RANGE(r) ((uint32_t)((RANDOM_MAX + 1) / r))

uint32_t uniform(uint32_t min, uint32_t max) {
    uint32_t delta = max - min;
    uint32_t value;
    do {
        value = random_int();
    } while (value >= delta * REJECTION_RANGE(delta));
    value /= REJECTION_RANGE(delta);
    return min + value;
}
