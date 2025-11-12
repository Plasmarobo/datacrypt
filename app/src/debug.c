#include "debug.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "serial.h"

#define DEBUG_DISPLAY (0)

static bool d_ = true;

static bool dready(int32_t status) {
    UNUSED(status);
    // signal
    d_ = true;
    return d_;
}

void dbgprintf(const char* fmt, ...) {
    char buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 128, fmt, args);
    va_end(args);
    vserial_printf(fmt, args);

#ifdef ENABLE_DEBUG_DISPLAY
    display_clear();
    draw_text(0, 0, buffer, strlen(buffer));
    if (d_) {
        d_ = false;
        display_show(DEBUG_DISPLAY, dready);
    }
#endif
}
