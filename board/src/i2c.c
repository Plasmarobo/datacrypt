#include "i2c.h"

#include "bsp.h"
#include "ringbuffer.h"
#include "scheduler.h"

#define I2C_QUEUE_LENGTH (4)
#define MAX_I2C_MESSAGE_LENGTH (128)

#define I2C_FLAG_READ (1)
#define I2C_FLAG_WRITE (0)

typedef struct {
    callback_t callback;
    uint8_t address;
    length_t length;
    uint8_t data[MAX_I2C_MESSAGE_LENGTH];
} i2c_transaction_t;

// Declares a static ringbuffer named i2c_queue
static i2c_transaction_t current_txn;
static int32_t i2c_status;
static task_handle_t timeout_task = NULL;

static void i2c_timeout_handler(int32_t status) {
    if (timeout_task != NULL) {
        HAL_I2C_Master_Abort_IT(&hi2c1, current_txn.address);
        timeout_task = NULL;
        if (NULL != current_txn.callback) {
            current_txn.callback(I2C_ERR_TIMEOUT);
        }
    }
}

void i2c_write(uint8_t address, length_t size, buffer_t data,
               callback_t oncomplete) {
    if (i2c_status != I2C_SUCCESS) {
        if (NULL != oncomplete) {
            oncomplete(i2c_status);
        }
        return;
    }
    i2c_status = I2C_BUSY;
    current_txn.callback = oncomplete;
    current_txn.address = (address << 1);
    current_txn.length = size;
    memcpy(current_txn.data, data, size);
    timeout_task =
        task_delayed_unique(i2c_timeout_handler, MILLIS(I2C_TIMEOUT_MS));
    HAL_I2C_Master_Transmit_DMA(&hi2c1, current_txn.address, current_txn.data,
                                current_txn.length);
};

void i2c_read(uint8_t address, length_t max_size, buffer_t data,
              callback_t oncomplete) {
    if (i2c_status != I2C_SUCCESS) {
        if (NULL != oncomplete) {
            oncomplete(i2c_status);
        }
        return;
    }
    current_txn.callback = oncomplete;
    current_txn.address = (address << 1);
    current_txn.length = max_size;
    timeout_task =
        task_delayed_unique(i2c_timeout_handler, MILLIS(I2C_TIMEOUT_MS));
    HAL_I2C_Master_Receive_DMA(&hi2c1, current_txn.address, current_txn.data,
                               current_txn.length);
}

void i2c_complete_handler(int32_t status) {
    i2c_status = status;
    if (NULL != timeout_task) {
        task_abort(timeout_task);
        timeout_task = NULL;
    }
    if (NULL != current_txn.callback) {
        current_txn.callback(i2c_status);
    }
}

int32_t i2c_get_status(void) { return i2c_status; }
