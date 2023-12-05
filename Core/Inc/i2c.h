#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#include "bsp.h"
// Wraps I2C Transactions

#define I2C_TIMEOUT_MS (10)

void i2c_write(uint8_t address, length_t size, buffer_t data,
               callback_t oncomplete);
void i2c_read(uint8_t address, length_t max_size, buffer_t data,
              callback_t oncomplete);
void i2c_complete_handler(void);

#endif  // I2C_H
