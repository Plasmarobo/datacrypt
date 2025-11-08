#ifndef __SERIAL_HAL_H_
#define __SERIAL_HAL_H_

#include <stdarg.h>
#include "defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void serial_read(buffer_t dest, length_t length, callback_t oncomplete);
    void serial_write(const buffer_t data, length_t length, callback_t oncomplete);
    void serial_print(const char *str);
    void serial_printf(const char *fmt, ...);
    void vserial_printf(const char *fmt, va_list args);
    void serial_tx_complete_handler(int32_t status);
    void serial_rx_complete_handler(int32_t status);

#ifdef __cplusplus
}
#endif

#endif // __SERIAL_HAL_H_