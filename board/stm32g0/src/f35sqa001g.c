#include <stdint.h>
#include <string.h>

#include "bsp.h"
#include "debug.h"
#include "flash.h"
#include "fsm.h"
#include "ringbuffer.h"
#include "scheduler.h"
#include "stack.h"
#include "stm32.h"

// Used to generate 8 dummy clocks
#define NOOP (0x00)
#define TIMING_BYTE (0x00)
#define CMD_SOFT_RESET (0xFF)
#define JEDEC_READ (0x9F)
// Can be used to poll registers/features, including busy
#define GET_FEATURE (0x0F)
// Can be used to set registers/features, such as WEL
#define SET_FEATURE (0x1F)
// Must be run before all program/erase operations
#define WRITE_ENABLE (0x06)
#define WRITE_DISABLE (0x04)
//
#define BLOCK_ERASE (0xD8)
// Loads data to be programmed into chip buffer, fills rest with 0xFF
#define PROGRAM_DATA_LOAD (0x02)
// Loads data to be programmed into chip buffer, does not alter rest
#define RANDOM_PROGRAM_DATA_LOAD (0x84)
// Commits program buffer to FLASH
#define PROGRAM_EXECUTE (0x10)

#define PAGE_READ_TO_CACHE (0x13)
#define READ_FROM_CACHE (0x03)
#define READ_FROM_CACHE_ALT (0x0B)
#define READ_FROM_CACHE_X2 (0x3B)
#define READ_FROM_CACHE_X4 (0x6B)

#define OOB (64)

#define FLASH_QUEUE_LENGTH (4)
#define FLASH_STATE_MAX_DEPTH (8)

#define FLASH_TIMEOUT_MS (75)
#define FLASH_POLL_US (100)

// 32 bit header indicating the invalid page table follows
#define TRST_US (200)
#define TRD_US (25)
#define TPROG_US (700)
#define TERASE_MS (10)
#define MAX_CMD_BYTES (8)
#define JEDEC_BYTES (3)

// We need to be able to read and write bbt with one op
#define MAX_TRANSACTION_BYTES (128 + 8)

#define PROTECTION_REGISTER (0xA0)
#define CONFIG_REGISTER (0xB0)
#define STATUS_REGISTER (0xC0)
#define ECC0_REGISTER (0x80)
#define ECC1_REGISTER (0x84)
#define ECC2_REGISTER (0x88)
#define ECC3_REGISTER (0x8C)

#define STATUS_OIP (0x01)
#define STATUS_WEL (0x02)
#define STATUS_ERASE_FAIL (0x04)
#define STATUS_PROG_FAIL (0x08)
#define STATUS_ECCS0 (0x10)
#define STATUS_ECCS1 (0x20)

#define JEDEC_OFFSET (2)
#define FEATURE_OFFSET (2)

#define INVALID_ADDRESS (0xFFFFFFFF)

#define PAGE_MASK (0x3F)
#define BLOCK_MASK (0xFFC0)


static callback_t flash_event_handlers[FLASH_EV_MAX];

typedef struct {
    buffer_t data;
    length_t size;
} flash_transaction_t;

static timespan_t poll_interval;
static uint8_t tx_rx_buffer[MAX_TRANSACTION_BYTES];
static bool spi_cs_hold_flag;
static uint8_t jedec_id[JEDEC_BYTES];

static flash_address_t block_page_address;
// Doubles as feature address
static uint16_t byte_address;
static uint8_t feature_value;
static uint8_t feature_address;
static bool flash_busy = false;
static flash_transaction_t current_transaction;
static callback_t operation_callback = NULL;
static task_handle_t timeout_task = NULL;
static task_handle_t poll_task = NULL;
static uint16_t bytes_written;

STACK(op_stack, callback_t, 8);
#ifdef TRACE_NAMES
STACK(name_stack, const char*, 16);
#endif

static void spi_start();
static void spi_finish();

static bool spi_write_read(buffer_t buffer, length_t tx_len, length_t rx_len,
                           callback_t oncomplete);
static bool spi_write(buffer_t buffer, length_t length, callback_t oncomplete);
static bool spi_read(buffer_t buffer, length_t length, callback_t oncomplete);

// static bool write(buffer_t buffer, length_t len, callback_t oncomplete);
// static bool read(buffer_t buffer, length_t len, callback_t oncomplete);
static void config_polling_op(timespan_t interval, timespan_t timeout);
static void status_poll(int32_t status);
static void check_status(int32_t status);
static void timeout_handler(int32_t status);
static void flash_write_cache(uint16_t byte_address, buffer_t source,
                              length_t length);
static void query_jedec(int32_t status);
static void flash_stream_data(int32_t status);
static void flash_flush_cache(int32_t status);
static void read_status(int32_t status);
static void op_handler_pop(int32_t status);

static void unlock_flash(int32_t status);
static bool lock_flash(callback_t notify);

#ifdef TRACE_NAMES
#define STRINGIFY_LITERAL(x) #x
#define STRINGIFY(x) STRINGIFY_LITERAL(x)
#define PUSH_OP(x) push_op(x, STRINGIFY(x))
static void push_op(callback_t op, const char* fnname) {
    stack_push(&name_stack, &fnname);
    if (fnname != NULL) {
        dbgprintf("PUSH: %s\r\n", fnname);
    }
#else
#define PUSH_OP(x) push_op(x)
static void push_op(callback_t op) {
#endif
    stack_push(&op_stack, &op);
}

// Sets CS
static void spi_start() { gpio_set(FLASH_CS, false); }

// Releases CS
static void spi_finish() { gpio_set(FLASH_CS, true); }

static bool spi_write_read(buffer_t buffer, length_t tx_len, length_t rx_len,
                           callback_t oncomplete) {
    memset(buffer + tx_len, 0, rx_len);
    operation_callback = oncomplete;
    gpio_set(FLASH_CS, false);
    HAL_SPI_TransmitReceive_DMA(&hspi2, buffer, buffer, tx_len + rx_len);
    return true;
}

static bool spi_write(buffer_t buffer, length_t len, callback_t oncomplete) {
    operation_callback = oncomplete;
    gpio_set(FLASH_CS, false);
    HAL_SPI_Transmit_DMA(&hspi2, buffer, len);
    return true;
}

static bool spi_read(buffer_t buffer, length_t len, callback_t oncomplete) {
    operation_callback = oncomplete;
    // Avoid sending possible reset command (0xFF)
    // zero is a safe dummy value
    memset(buffer, 0, len);
    gpio_set(FLASH_CS, false);
    HAL_SPI_Receive_DMA(&hspi2, buffer, len);
    return true;
}

static void set_write_enable_latch(callback_t oncomplete) {
    tx_rx_buffer[0] = WRITE_ENABLE;
    spi_write(tx_rx_buffer, 1, oncomplete);
}

// Load a page into the chip cache
static void populate_cache(int32_t status) {
    tx_rx_buffer[0] = PAGE_READ_TO_CACHE;
    tx_rx_buffer[1] = TIMING_BYTE;
    tx_rx_buffer[2] = (block_page_address >> 8) & 0xFF;
    tx_rx_buffer[3] = block_page_address & 0xFF;
    config_polling_op(MICROS(TRD_US / 2), MICROS(2 * TRD_US));
    spi_start();
    spi_write(tx_rx_buffer, 4, status_poll);
}

// Fetch data from chip
static void read_bytes(int32_t status) {
    spi_cs_hold_flag = false;  // Allow CS reset
    spi_read(current_transaction.data, current_transaction.size,
             op_handler_pop);
}

// Read cache into host memory buffer
static void read_cache(int32_t status) {
    tx_rx_buffer[0] = READ_FROM_CACHE;
    tx_rx_buffer[1] = (byte_address >> 8) & 0xFF;
    tx_rx_buffer[2] = byte_address & 0xFF;
    tx_rx_buffer[3] = TIMING_BYTE;
    spi_cs_hold_flag = true;
    spi_write(tx_rx_buffer, 4, read_bytes);
}

// Read register/feature
static void get_feature(int32_t status) {
    tx_rx_buffer[0] = GET_FEATURE;
    tx_rx_buffer[1] = feature_address;
    spi_write_read(tx_rx_buffer, 2, 1, op_handler_pop);
}

// Write register/feature to chip
static void set_feature(int32_t status) {
    tx_rx_buffer[0] = SET_FEATURE;
    tx_rx_buffer[1] = feature_address;
    tx_rx_buffer[2] = feature_value;
    spi_write(tx_rx_buffer, 3, op_handler_pop);
}

// Proceed to next op in stack
static void op_handler_pop(int32_t status) {
    callback_t handler = NULL;
    if (!stack_empty(&op_stack)) {
        stack_pop(&op_stack, &handler);
#ifdef TRACE_NAMES
        const char* name = NULL;
        stack_pop(&name_stack, &name);
        if (NULL != name) {
            dbgprintf("POP: %s %08x\r\n", name);
        } else {
            dbgprint("POPPED NULL\r\n");
        }
#endif
        if (NULL != handler) {
            if (NULL == task_immediate_signal(handler, status)) {
                dbgprintf("Scheduling failed\r\n");
            }
        }
    } else {
        if (flash_event_handlers[FLASH_EV_IDLE] != NULL) {
            (flash_event_handlers[FLASH_EV_IDLE])(status);
        }
    }
}

static void op_handler_clear(int32_t status) {
    callback_t handler;
    while(!stack_empty(&op_stack))
    {
        stack_pop(&op_stack, &handler);
        if (NULL != handler) {
            handler(status);
        }
    }
    unlock_flash(status);
}

// Poll chip for operation completion
static void status_poll(int32_t status) {
    if (FLASH_SUCCESS == status) {
        if (NULL == timeout_task) {
            timeout_task =
                task_delayed_unique(timeout_handler, MILLIS(FLASH_TIMEOUT_MS));
        }
        poll_task = task_delayed(read_status, poll_interval);
    } else {
        if (NULL != timeout_task) {
            task_abort(timeout_task);
            timeout_task = NULL;
        }
        op_handler_pop(status);
    }
}

static void flash_post_reset(int32_t status) {
    task_delayed(query_jedec, MICROS(TRST_US));
};

static void flash_reset(int32_t status) {
    // Wait for TRST then transition to startup scan
    flash_busy = true;
    tx_rx_buffer[0] = 0xFF;
    spi_write(tx_rx_buffer, 1, flash_post_reset);
};

// Warning: this conducts the actual block erase, check bbt first
static void block_erase(int32_t status) {
    if (FLASH_SUCCESS == status) {
        tx_rx_buffer[0] = BLOCK_ERASE;
        tx_rx_buffer[1] = TIMING_BYTE;
        tx_rx_buffer[2] = (block_page_address >> 8) & 0xFF;
        tx_rx_buffer[3] = block_page_address & 0xFF;
        config_polling_op(MILLIS(TERASE_MS / 2), MILLIS(2 * TERASE_MS));
        spi_write(tx_rx_buffer, 4, status_poll);
    } else {
        op_handler_pop(FLASH_ERR_BAD_STATE);
    }
};

static void flash_erase_block(flash_address_t address) {
    block_page_address = BLOCK_PAGE(address);
    set_write_enable_latch(block_erase);
};

static void flash_stream_data(int32_t status) {
    uint16_t bytes_to_write = current_transaction.size - bytes_written;
    if (bytes_to_write > MAX_TRANSACTION_BYTES) {
        bytes_to_write = MAX_TRANSACTION_BYTES;
        // We have more to stream, queue op
        PUSH_OP(flash_stream_data);
        spi_cs_hold_flag = true;
    } else {
        spi_cs_hold_flag = false;
    }
    memcpy(tx_rx_buffer, current_transaction.data + bytes_written,
           bytes_to_write);
    spi_write(tx_rx_buffer, bytes_to_write, op_handler_pop);
    bytes_written += bytes_to_write;
};

// Programs bytes in the write cache
// First call will set cache to 0xFF
// Page must have been previously erased
// Flash can only write down
static void flash_write_cache(uint16_t byte_address_, buffer_t source,
                              length_t length) {
    bytes_written = 0;
    tx_rx_buffer[0] = PROGRAM_DATA_LOAD;
    tx_rx_buffer[1] = (byte_address_ >> 8) & 0xFF;
    tx_rx_buffer[2] = byte_address_ & 0xFF;
    current_transaction.data = source;
    current_transaction.size = length;
    spi_cs_hold_flag = true;
    spi_write(tx_rx_buffer, 3, flash_stream_data);
};

static void flash_update_cache(uint16_t byte_address_, buffer_t source,
                               length_t length) {
    bytes_written = 0;
    tx_rx_buffer[0] = RANDOM_PROGRAM_DATA_LOAD;
    tx_rx_buffer[1] = (byte_address_ >> 8) & 0xFF;
    tx_rx_buffer[2] = byte_address_ & 0xFF;
    current_transaction.data = source;
    current_transaction.size = length;
    spi_cs_hold_flag = true;
    spi_write(tx_rx_buffer, 3, flash_stream_data);
}

// Flushes write-cache to flash
static void flash_program(int32_t status) {
    tx_rx_buffer[0] = PROGRAM_EXECUTE;
    tx_rx_buffer[1] = TIMING_BYTE;
    tx_rx_buffer[2] = (block_page_address >> 8) & 0xFF;
    tx_rx_buffer[3] = block_page_address & 0xFF;
    bytes_written = 0;
    config_polling_op(MICROS(TPROG_US / 2), MICROS(2 * TPROG_US));
    spi_write(tx_rx_buffer, 4, status_poll);
};

// Starts a flushing op
// needs to be called before write/erase operations
static void flash_flush_cache(int32_t status) {
    set_write_enable_latch(flash_program);
};

static void set_flash_ready(int32_t status) {
    unlock_flash(FLASH_SUCCESS);
    op_handler_pop(FLASH_SUCCESS);
}

static void disable_block_protection(int32_t status) {
    feature_address = PROTECTION_REGISTER;
    feature_value = 0x00;
    PUSH_OP(op_handler_pop);
    if (NULL == task_immediate_signal(set_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_register(int32_t status) {
    dbgprintf("REG: %02x\r\n", tx_rx_buffer[FEATURE_OFFSET]);
    op_handler_pop(FLASH_SUCCESS);
}

static void dump_status_reg(int32_t status) {
    feature_address = STATUS_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("Status ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_protection_reg(int32_t status) {
    feature_address = PROTECTION_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("Protection ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_configuration_reg(int32_t status) {
    feature_address = CONFIG_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("Configuration ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_ecc0_reg(int32_t status) {
    feature_address = ECC0_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("ECC0 ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_ecc1_reg(int32_t status) {
    feature_address = ECC1_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("ECC1 ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_ecc2_reg(int32_t status) {
    feature_address = ECC2_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("ECC2 ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void dump_ecc3_reg(int32_t status) {
    feature_address = ECC3_REGISTER;
    PUSH_OP(dump_register);
    dbgprint("ECC3 ");
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprint("Scheduling failed\r\n");
    }
}

static void verify_jedec(int32_t status) {
    memcpy(jedec_id, tx_rx_buffer + JEDEC_OFFSET, 3);
    dbgprintf("JEDEC ID: %02x %02x %02x\r\n", jedec_id[0], jedec_id[1],
              jedec_id[2]);
    if (jedec_id[0] == 0xCD && jedec_id[1] == 0x71 && jedec_id[2] == 0x71) {
        PUSH_OP(set_flash_ready);
        PUSH_OP(disable_block_protection);
        PUSH_OP(dump_ecc3_reg);
        PUSH_OP(dump_ecc2_reg);
        PUSH_OP(dump_ecc1_reg);
        PUSH_OP(dump_ecc0_reg);
        PUSH_OP(dump_status_reg);
        PUSH_OP(dump_configuration_reg);
        dump_protection_reg(FLASH_SUCCESS);
    } else {
        dbgprintf("Uknown JEDEC ID\r\n");
        op_handler_pop(FLASH_ERR_UNKNOWN_ID);
    }
}

static void query_jedec(int32_t status) {
    tx_rx_buffer[0] = JEDEC_READ;
    tx_rx_buffer[1] = TIMING_BYTE;
    tx_rx_buffer[2] = 0x00;
    tx_rx_buffer[3] = 0x00;
    tx_rx_buffer[4] = 0x00;
    spi_write_read(tx_rx_buffer, 2, 3, verify_jedec);
}

static void timeout_handler(int32_t status) {
    if (NULL != poll_task) {
        task_abort(poll_task);
        // Handle timeout
        op_handler_clear(FLASH_ERR_TIMEOUT);
        //op_handler_pop(FLASH_ERR_TIMEOUT);
        if (flash_event_handlers[FLASH_EV_TIMEOUT] != NULL) {
            (flash_event_handlers[FLASH_EV_TIMEOUT])(FLASH_ERR_TIMEOUT);
        }
    }
}

static void read_status(int32_t status) {
    feature_address = STATUS_REGISTER;
    PUSH_OP(check_status);
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        dbgprintf("Scheduling failed\r\n");
    }
}

static void config_polling_op(timespan_t interval, timespan_t timeout) {
    feature_value |= STATUS_OIP;
    poll_interval = interval;
}

static void check_status(int32_t status) {
    feature_value = tx_rx_buffer[FEATURE_OFFSET];
    if (feature_value & STATUS_OIP) {
        poll_task = task_delayed(read_status, poll_interval);
    } else {
        poll_task = NULL;
        if (NULL != timeout_task) {
            task_abort(timeout_task);
            timeout_task = NULL;
        }
        if ((feature_value & (STATUS_ECCS1))) {
            // Block has a stuck bit, mark bad
            // Note: error correction may be able to handle one bit of error...
            block_page_address &= BLOCK_MASK;
            // Note: ops should handle bad block without altering the
            // block_page_address
            op_handler_pop(FLASH_ERR_BAD_BLOCK);
            if (flash_event_handlers[FLASH_EV_BAD_BLOCK] != NULL) {
                (flash_event_handlers[FLASH_EV_BAD_BLOCK])(block_page_address);
            }
        } else if (feature_value & (STATUS_ERASE_FAIL | STATUS_PROG_FAIL)) {
            op_handler_pop(FLASH_ERR_FAILURE);
        } else {
            op_handler_pop(FLASH_SUCCESS);
        }
    }
}

static void unlock_flash(int32_t status) {
    op_handler_pop(status);
    flash_busy = false;
}

static bool lock_flash(callback_t notify) {
    if (flash_busy) {
        if (NULL != notify) {
            notify(FLASH_ERR_BUSY);
        }
        return false;
    }
    PUSH_OP(notify);
    PUSH_OP(unlock_flash);
    return true;
}

void flash_read(flash_address_t address,
                buffer_t dest, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        byte_address = BYTE(address);
        current_transaction.data = dest;
        current_transaction.size = size;
        block_page_address = BLOCK_PAGE(address);
        PUSH_OP(read_cache);
        populate_cache(FLASH_SUCCESS);
    }
}

// Setup a write with program data, will clear cache to 0xFF
void flash_write(flash_address_t address,
                 buffer_t data, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        block_page_address = BLOCK_PAGE(address);
        flash_write_cache(BYTE(address), data, size);
    }
}

// Setup a write with program data, random access, will not alter other cache
// bytes
void flash_update(flash_address_t address,
                  buffer_t data, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        block_page_address = BLOCK_PAGE(address);
        flash_update_cache(BYTE(address), data, size);
    }
}

void flash_commit(callback_t on_complete) {
    if (lock_flash(on_complete)) {
        flash_flush_cache(FLASH_SUCCESS);
    }
}

void flash_erase(flash_address_t addr, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        flash_erase_block(addr);
    }
}

void flash_init(callback_t on_init) {
    PUSH_OP(on_init);
    for (size_t i = 0; i < FLASH_EV_MAX; ++i) {
        flash_event_handlers[i] = NULL;
    }
    spi_finish();  // Resets the CS line
    flash_reset(FLASH_SUCCESS);
}

void flash_register_event_handler(flash_event_t event, callback_t handler) {
    if (event < FLASH_EV_MAX) {
        flash_event_handlers[event] = handler;
    }
}

void flash_op_complete_handler(int32_t status) {
    if (!spi_cs_hold_flag) {
        spi_finish();
    } else {
        spi_cs_hold_flag = false;
    }
    if (NULL != operation_callback) {
        if (NULL == task_immediate_signal(operation_callback, status)) {
            dbgprintf("Scheduling failed\r\n");
        }
        operation_callback = NULL;
    }
}
