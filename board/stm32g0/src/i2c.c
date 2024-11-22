#include "i2c.h"

#include "bsp.h"
#include "ringbuffer.h"
#include "scheduler.h"
#include "stm32.h"

#define I2C_QUEUE_LENGTH (4)
// 1024 bytes of display memory + 1 byte preamble
#define MAX_I2C_MESSAGE_LENGTH (1024 + 1)

#define I2C_FLAG_READ (1)
#define I2C_FLAG_WRITE (0)

typedef struct {
    callback_t callback;
    uint8_t address;
    length_t length;
    uint8_t* data;
} i2c_transaction_t;

// Declares a static ringbuffer named i2c_queue
static i2c_transaction_t current_txn;
static callback_t user_callback;
static int32_t i2c_status;
static task_handle_t timeout_task = NULL;

static void i2c_timeout_handler(int32_t status) {
    if (timeout_task != NULL) {
        HAL_I2C_Master_Abort_IT(&hi2c1, current_txn.address);
        timeout_task = NULL;
        if (NULL != current_txn.callback) {
            callback_t cb = current_txn.callback;
            current_txn.callback = NULL;
            cb(I2C_ERR_TIMEOUT);
        }
        i2c_status = I2C_SUCCESS;
    }
}

static void i2c_continue_write(int32_t status) {
    timeout_task =
        task_delayed_unique(i2c_timeout_handler, MILLIS(I2C_TIMEOUT_MS));
    current_txn.length -= MAX_I2C_MESSAGE_LENGTH;
    current_txn.data += MAX_I2C_MESSAGE_LENGTH;
    length_t tx_len;
    if (current_txn.length < MAX_I2C_MESSAGE_LENGTH) {
        current_txn.callback = user_callback;
        tx_len = current_txn.length;
    } else {
        current_txn.callback = i2c_continue_write;
        tx_len = MAX_I2C_MESSAGE_LENGTH;
    }
    HAL_I2C_Master_Transmit_DMA(&hi2c1, current_txn.address, current_txn.data,
                                tx_len);
}

void i2c_write(uint8_t address, length_t size, buffer_t data,
               callback_t oncomplete) {
    if (i2c_status == I2C_SUCCESS)
    {
        i2c_status = I2C_BUSY;
        current_txn.address = (address << 1);
        timeout_task =
            task_delayed_unique(i2c_timeout_handler, MILLIS(I2C_TIMEOUT_MS));
        current_txn.length = size;
        current_txn.data = data;
        length_t tx_len;
        if (size <= MAX_I2C_MESSAGE_LENGTH) {
            user_callback = NULL;
            current_txn.callback = oncomplete;
            tx_len = size;
        } else {
            user_callback = oncomplete;
            current_txn.callback = i2c_continue_write;
            tx_len = MAX_I2C_MESSAGE_LENGTH;
        }
        HAL_I2C_Master_Transmit_DMA(&hi2c1, current_txn.address, current_txn.data,
                                    tx_len);
    } else {
        if (NULL != oncomplete) {
            oncomplete(I2C_BUSY);
        }
    }
};

void i2c_read(uint8_t address, length_t max_size, buffer_t data,
              callback_t oncomplete) {
    if (i2c_status == I2C_SUCCESS) {
        current_txn.callback = oncomplete;
        current_txn.address = (address << 1);
        current_txn.length = max_size;
        timeout_task =
            task_delayed_unique(i2c_timeout_handler, MILLIS(I2C_TIMEOUT_MS));
        HAL_I2C_Master_Receive_DMA(&hi2c1, current_txn.address, current_txn.data,
                                current_txn.length);
    } else {
        if (NULL != oncomplete) {
            oncomplete(i2c_status);
        }
    }
}

void i2c_complete_handler(int32_t status) {
    i2c_status = I2C_SUCCESS; // set to idle, status contains any real errors
    if (NULL != timeout_task) {
        task_abort(timeout_task);
        timeout_task = NULL;
    }
    if (NULL != current_txn.callback) {
        callback_t cb = current_txn.callback;
        current_txn.callback = NULL;
        cb(I2C_SUCCESS);
    }
}

int32_t i2c_get_status(void) { return i2c_status; }
