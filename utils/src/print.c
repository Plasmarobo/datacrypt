

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hal.h"
#include "defs.h"

#include "print.h"

static char print_buffer[MAX_WRITE_LENGTH];

void generic_printf(write_function_t writer, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    generic_vprintf(writer, fmt, args);
    va_end(args);
}
void generic_vprintf(write_function_t writer, const char *fmt, va_list args)
{
    vsnprintf(print_buffer, MAX_WRITE_LENGTH, fmt, args);
    writer((buffer_t)print_buffer, strlen(print_buffer), NULL);
}
