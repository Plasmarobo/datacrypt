#include "debug.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"

#define DEBUG_DISPLAY (0)

#if defined(DEBUG_PRINT)
void dbgprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vserial_printf(fmt, args);
    va_end(args);
}

void dbgprint(const char* str) { serial_print(str); }
#else
void dbgprintf(const char* fmt, ...){}
void dbgprint(const char* str) {}
#endif
