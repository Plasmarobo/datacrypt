#include "bsp.h"
#include "defs.h"
#include "scheduler.h"

#include "stm32.h"

#include <stdint.h>

static callback_t serial_rxcomplete;
static callback_t serial_txcomplete;
static volatile bool serial_lock;

#define SERIAL_TIMEOUT_US (1000000)
#define MAX_SERIAL_PACKET (256)

static uint8_t serial_tx_buffer[MAX_SERIAL_PACKET];
static uint8_t serial_rx_buffer[MAX_SERIAL_PACKET];
static uint8_t* serial_read_ptr;
static uint8_t* serial_write_ptr;
static length_t notify_length;
static uint8_t* serial_dest;

void serial_init(void) {
    serial_read_ptr = serial_rx_buffer;
    serial_write_ptr = serial_rx_buffer;
    serial_lock = false;
    serial_rxcomplete = NULL;
    serial_txcomplete = NULL;
    serial_dest = NULL;
    while (HAL_UART_Receive_IT(&huart1, serial_write_ptr, 1) != HAL_OK) {
        // spin
    }
}

length_t serial_available_bytes(void)
{
    length_t bytes_in_buffer = 0;
    if (serial_write_ptr >= serial_read_ptr)
    {
        bytes_in_buffer = serial_write_ptr - serial_read_ptr;
    } else {
        bytes_in_buffer = (serial_write_ptr + MAX_SERIAL_PACKET) - serial_read_ptr;
    }
    return bytes_in_buffer;
}

void serial_flush_rx(void)
{
    serial_read_ptr = serial_write_ptr;
}

static void serial_deque(uint8_t* dest, uint32_t length)
{
    memcpy(dest, serial_read_ptr, length);
    serial_read_ptr += length;
    if (serial_read_ptr >= (serial_rx_buffer + MAX_SERIAL_PACKET)) { serial_read_ptr = serial_rx_buffer; }
}

static void serial_rx_audit(int32_t status)
{
    length_t bytes_in_buffer = serial_available_bytes();
    if (serial_rxcomplete && serial_dest && notify_length && (bytes_in_buffer >= notify_length))
    {
        serial_deque(serial_dest, notify_length);
        task_immediate_signal(serial_rxcomplete, status);
        serial_dest = NULL;
        serial_rxcomplete = NULL;
        notify_length = 0;
    }
}

void serial_read(buffer_t dest, length_t length, callback_t oncomplete) {
    if (serial_available_bytes() >= length)
    {
        serial_deque(dest, length);
        oncomplete(length);
    } else {
        serial_rxcomplete = oncomplete;
        notify_length = length;
        serial_dest = dest;
    }
}

void serial_write(const buffer_t data, length_t length, callback_t oncomplete) {
    // Poll ready
    while (true) {
        // Spinlock
        if (!serial_lock) {
            break;
        }
    }
    if (length <= MAX_SERIAL_PACKET) {
        memcpy(serial_tx_buffer, data, length);
        serial_lock = true;
        serial_txcomplete = oncomplete;
        while (HAL_UART_Transmit_IT(&huart1, serial_tx_buffer, length) !=
               HAL_OK) {
        }
    }
}

void serial_printhex(const buffer_t data, length_t length,
                     callback_t oncomplete) {
    timespan_t timer = microseconds();
    while (true) {
        if (!serial_lock) {
            break;
        }
        if ((microseconds() - timer) > SERIAL_TIMEOUT_US) {
            return;
        }
    }
    if ((length * 4) <= MAX_SERIAL_PACKET) {
        uint8_t* write_ptr = serial_tx_buffer;
        for (length_t i = 0; i < length; ++i) {
            snprintf(write_ptr, 4, "%02x ", data[i]);
            write_ptr += 4;
        }
        serial_lock = true;
        serial_txcomplete = oncomplete;
        if (HAL_OK !=
            HAL_UART_Transmit_IT(&huart1, serial_tx_buffer, length * 4)) {
        }
    }
}

void serial_print(const char* str) { serial_write(str, strlen(str), NULL); }

static char print_buffer[MAX_SERIAL_PACKET];

void serial_printf(const char* fmt, ...) {
    memset(print_buffer, 0, MAX_SERIAL_PACKET);
    va_list args;
    va_start(args, fmt);
    vsnprintf(print_buffer, MAX_SERIAL_PACKET, fmt, args);
    va_end(args);
    serial_write((const buffer_t)print_buffer, strlen(print_buffer), NULL);
}

void vserial_printf(const char* fmt, va_list args) {
    vsnprintf(print_buffer, MAX_SERIAL_PACKET, fmt, args);
    serial_write((const buffer_t)print_buffer, strlen(print_buffer), NULL);
}

void serial_tx_complete_handler(int32_t status) {
    // Should run in interrupt context
    if (NULL != serial_txcomplete) {
        task_immediate_signal(serial_txcomplete, status);
        serial_txcomplete = NULL;
    }
    serial_lock = false;
}

void serial_rx_complete_handler(int32_t status) {
    ++serial_write_ptr;
    if (serial_write_ptr == serial_read_ptr) {
        // clear buffer
        serial_write_ptr = serial_rx_buffer;
        serial_read_ptr = serial_rx_buffer;
        if (serial_rxcomplete)
        {
            task_immediate_signal(serial_rxcomplete,SERIAL_HW_ERROR);
            serial_rxcomplete = NULL;
        }
    } else if (serial_write_ptr >= (serial_rx_buffer + MAX_SERIAL_PACKET)) {
        serial_write_ptr = serial_rx_buffer;
    }
    while (HAL_UART_Receive_IT(&huart1, serial_write_ptr, 1) != HAL_OK) {
        // spin
    }
    task_immediate_unique(serial_rx_audit);
}

void serial_abort_tx(void) { HAL_UART_AbortTransmit_IT(&huart1); }
void serial_abort_rx(void) { HAL_UART_AbortReceive_IT(&huart1); }

void serial_abort_rx_notify(callback_t oncomplete) {
    serial_rxcomplete = oncomplete;
    serial_abort_rx();
}

void serial_tx_abort_handler(int32_t status) {
    if (NULL != serial_txcomplete) {
        task_immediate_signal(serial_txcomplete, status);
        serial_txcomplete = NULL;
    }
    serial_lock = false;
}

void serial_rx_abort_handler(int32_t status) {
    // clear buffer
    serial_write_ptr = serial_rx_buffer;
    serial_read_ptr = serial_rx_buffer;
    if (serial_rxcomplete) {
        task_immediate_signal(serial_rxcomplete, SERIAL_HW_ERROR);
        serial_rxcomplete = NULL;
    }
    while (HAL_UART_Receive_IT(&huart1, serial_write_ptr, 1) != HAL_OK) {
        // spin
        HAL_Delay(1);
    }
}
