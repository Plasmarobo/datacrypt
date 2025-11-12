#ifndef __PRINT_H__
#define __PRINT_H__

#include "defs.h"
#include <stdarg.h>

typedef void (*write_function_t)(const buffer_t, length_t, callback_t);
#define MAX_WRITE_LENGTH (256)

void generic_printf(write_function_t writer, const char *fmt, ...);
void generic_vprintf(write_function_t writer, const char *fmt, va_list args);

#endif // __PRINT_H__