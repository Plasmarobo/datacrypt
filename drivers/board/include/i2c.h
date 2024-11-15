#ifndef I2C_H
#define I2C_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp.h"
// Wraps I2C Transactions

#define I2C_TIMEOUT_MS (500)
#define I2C_SUCCESS (0)
#define I2C_BUSY (-1)
#define I2C_ERR_TIMEOUT (-2)
#define I2C_ERR_NAK (-3)
#define I2C_ERR_PROTOCOL (-4)
#define I2C_ERR_UNKNOWN (-8)

void i2c_write(uint8_t address, length_t size, buffer_t data,
               callback_t oncomplete);
void i2c_read(uint8_t address, length_t max_size, buffer_t data,
              callback_t oncomplete);
void i2c_complete_handler(int32_t status);
int32_t i2c_get_status(void);
#endif  // I2C_H
