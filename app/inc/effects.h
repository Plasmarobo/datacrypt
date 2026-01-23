#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include <stdbool.h>
#include "defs.h"

// Set which displays to enable blink
void set_blink_enable(uint8_t display, bool enable);
bool get_blink_enable(uint8_t display);

void flash_unlocked();

void effects_init();
void effects_update();

#endif // EFFECTS_H