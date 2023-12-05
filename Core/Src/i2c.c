#include "i2c.h"
#include "scheduler.h"
#include "ringbuffer.h"
#include "bsp.h"

#define I2C_QUEUE_LENGTH (20)
#define MAX_I2C_MESSAGE_LENGTH (128)

#define I2C_FLAG_READ (1)
#define I2C_FLAG_WRITE (0)

#define ERR_I2C_QUEUE_FULL (-1)
#define ERR_I2C_TIMEOUT (-2)
#define ERR_I2C_UNKNOWN (-8)

typedef struct
{
    callback_t callback;
    uint8_t address;
    length_t length;
    uint8_t data[MAX_I2C_MESSAGE_LENGTH];
} i2c_transaction_t;

void i2c_exec_start(void);
void i2c_exec_finish(void);

// Declares a static ringbuffer named i2c_queue
RINGBUFFER(i2c_queue,i2c_transaction_t,I2C_QUEUE_LENGTH);
static bool i2c_busy = false; 

void i2c_write(uint8_t address, length_t size, buffer_t data, callback_t oncomplete)
{
    if (!ringbuffer_full(&i2c_queue))
    {
        i2c_transaction_t txn = {
            oncomplete,
            address | I2C_FLAG_WRITE,
            size,
            data
        };
        ringbuffer_push(&i2c_queue, &txn);
        i2c_exec_start();
    }
    else
    {
        if (NULL != oncomplete)
        {
            oncomplete(ERR_I2C_QUEUE_FULL);
        }
    }

}

void i2c_read(uint8_t address, length_t max_size, buffer_t data, callback_t oncomplete)
{
    if (!ringbuffer_full(&i2c_queue))
    {
        i2c_transaction_t txn = {
            oncomplete,
            address & I2C_FLAG_READ,
            max_size,
            data
        };
        ringbuffer_push(&i2c_queue, &txn);
        i2c_exec_start();
        
    }
    else
    {
        if (NULL != oncomplete)
        {
            oncomplete(ERR_I2C_QUEUE_FULL);
        }
    }
}

void i2c_exec_start(void)
{
    if (!ringbuffer_empty(&i2c_queue) && !i2c_busy)
    {
        i2c_transaction_t txn;
        ringbuffer_peek(&i2c_queue, &txn);
        i2c_busy = true;
        if (txn.address & I2C_FLAG_READ)
        {
            HAL_I2C_Master_Receive_DMA(I2C1, txn.address & 0xFE, txn.data, txn.length);
        }
        else
        {
            HAL_I2C_Master_Transmit_DMA(I2C1, txn.address & 0xFE, txn.data, txn.length);
        }
    }
}

void i2c_exec_finish(void)
{
    if (ringbuffer_empty(&i2c_queue) || !i2c_busy)
    {
        // No transaction
        return;
    }

    i2c_transaction_t txn;
    ringbuffer_pop(&i2c_queue, &txn);
    if (NULL != txn.callback)
    {
        txn.callback(0);
    }
    i2c_busy = false;
    // Kick off queued op if any
    i2c_exec_start();
}

void i2c_complete_handler(void)
{
    i2c_exec_finish();
}