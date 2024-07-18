#include <stdint.h>

#include "bsp.h"
#include "flash.h"
#include "fsm.h"
#include "ringbuffer.h"
#include "scheduler.h"
#include "stack.h"

// Used to generate 8 dummy clocks
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
#define MAGIC_BYTES_INVALID_PAGE_TABLE (0x9A6E7ABE)
#define TRST_US (200)
#define TRD_US (25)
#define TPROG_US (700)
#define TERASE_MS (10)
#define MAX_CMD_BYTES (8)
#define JEDEC_BYTES (3)
#define MAX_TRANSACTION_BYTES (128 + 8)

#define PROTECTION_REGISTER (0xA0)
#define CONFIG_REGISTER (0xB0)
#define STATUS_REGISTER (0xC0)

#define STATUS_OIP (0x01)
#define STATUS_WEL (0x02)
#define STATUS_ERASE_FAIL (0x04)
#define STATUS_PROG_FAIL (0x08)
#define STATUS_ECCS0 (0x10)
#define STATUS_ECCS1 (0x20)

#define JEDEC_OFFSET (2)
#define FEATURE_OFFSET (2)

#define BYTE_MASK (0x07FF)

typedef struct {
    buffer_t data;
    length_t size;
} flash_transaction_t;

static timespan_t poll_interval;
static uint8_t tx_rx_buffer[MAX_TRANSACTION_BYTES];
static bool spi_cs_hold_flag;
static uint8_t jedec_id[JEDEC_BYTES];

static flash_page_address_t block_page_address;
// Doubles as feature address
static uint16_t byte_address;
static uint8_t feature_value;
static uint8_t general_buffer;
static bool flash_busy = false;
static bool cache_dirty = true;
static bool block_bad = true;
static flash_transaction_t current_transaction;
static callback_t user_callback = NULL;
static callback_t operation_callback = NULL;
static task_handle_t timeout_task = NULL;
static task_handle_t poll_task = NULL;
static uint16_t bytes_written;

STACK(op_stack, callback_t, 16);
STACK(name_stack, const char*, 16);

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
static void check_bad_block(int32_t status);
static void bad_block_scan_start(int32_t status);
static void flash_write_cache(uint16_t byte_address, buffer_t source,
                              length_t length);
static void query_jedec(int32_t status);
static void flash_stream_data(int32_t status);
static void flash_flush_cache(int32_t status);
static void read_status(int32_t status);
static void op_handler_pop(int32_t status);

#ifdef TRACE_NAMES
#define STRINGIFY_LITERAL(x) #x
#define STRINGIFY(x) STRINGIFY_LITERAL(x)
#define PUSH_OP(x) push_op(x, STRINGIFY(x))
static void push_op(callback_t op, const char* fnname) {
    stack_push(&name_stack, &fnname);
    if (fnname != NULL) {
        serial_printf("PUSH: %s\r\n", fnname);
    }
#else
#define PUSH_OP(x) push_op(x)
static void push_op(callback_t op) {
#endif
    stack_push(&op_stack, &op);
}

// Enables re-using cache where possible
static void mark_cache_clean(int32_t status) {
    if (FLASH_SUCCESS == status) {
        cache_dirty = false;
    }
    op_handler_pop(status);
}

// Sets CS
static void spi_start() { gpio_set(FLASH_CS, false); }

// Releases CS
static void spi_finish() { gpio_set(FLASH_CS, true); }

static bool spi_write_read(buffer_t buffer, length_t tx_len, length_t rx_len,
                           callback_t oncomplete) {
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
    config_polling_op(MICROS(TRD_US), MICROS(FLASH_POLL_US));
    PUSH_OP(mark_cache_clean);
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

// Write cache from host memory buffer
static void write_cache(int32_t status) {
    spi_cs_hold_flag = true;
    tx_rx_buffer[0] = RANDOM_PROGRAM_DATA_LOAD;
    tx_rx_buffer[1] = (byte_address >> 8) & 0xFF;
    tx_rx_buffer[2] = byte_address & 0xFF;
    spi_cs_hold_flag = true;
    spi_write(tx_rx_buffer, 3, flash_stream_data);
}

// Read register/feature
static void get_feature(int32_t status) {
    tx_rx_buffer[0] = GET_FEATURE;
    tx_rx_buffer[1] = byte_address & 0xFF;
    spi_write_read(tx_rx_buffer, 2, 1, op_handler_pop);
}

// Write register/feature to chip
static void set_feature(int32_t status) {
    tx_rx_buffer[0] = SET_FEATURE;
    tx_rx_buffer[1] = byte_address & 0xFF;
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
            serial_printf("POP: %s %08x\r\n", name);
        } else {
            serial_print("POPPED NULL\r\n");
        }
#endif
        if (NULL != handler) {
            if (NULL == task_immediate_signal(handler, status)) {
                serial_printf("Scheduling failed\r\n");
            }
        }
    } else {
        serial_print("Stack empty...\r\n");
    }
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
        op_handler_pop(status);
    }
}

static void mark_bbt_page_1(int32_t status) {
    general_buffer = BAD_BLOCK_VALUE;
    bytes_written = 0;
    cache_dirty = false;
    byte_address = OOB_BASE_ADDRESS;
    flash_write_cache(block_page_address + 1, &general_buffer, 1);
}

static void mark_bad_block(int32_t status) {
    general_buffer = BAD_BLOCK_VALUE;
    PUSH_OP(flash_flush_cache);
    PUSH_OP(mark_bbt_page_1);
    PUSH_OP(flash_flush_cache);
    bytes_written = 0;
    cache_dirty = false;
    byte_address = OOB_BASE_ADDRESS;
    flash_write_cache(block_page_address, &general_buffer, 1);
}

static void check_bbt_value(int32_t status) {
    if (FLASH_SUCCESS == status) {
        if (ERASED_VALUE != general_buffer) {
            // Block is bad!
            serial_printf("Bad Block at %04x (%d)\r\n", block_page_address,
                          BLOCK(block_page_address));
            block_bad = true;
        }
        if ((block_page_address & PAGE_MASK) == 0) {
            // Check page 2
            block_page_address += 1;
            task_immediate(check_bad_block);
        } else {
            serial_printf("Block %d done\r\n", BLOCK(block_page_address));
            if (BLOCK(block_page_address) >= (BLOCK_COUNT - 1)) {
                serial_print("BBT Complete\r\n");
                op_handler_pop(status);
            } else {
                block_page_address += PAGES_PER_BLOCK - 1;
                task_immediate(check_bad_block);
            }
        }
    } else {
        serial_printf("BBT Check failed %04x\r\n", status);
    }
}

static void check_bad_block(int32_t status) {
    // Load first page
    block_bad = false;
    general_buffer = ERASED_VALUE;
    byte_address = OOB_BASE_ADDRESS;
    current_transaction.data = &general_buffer;
    current_transaction.size = 1;
    PUSH_OP(check_bbt_value);
    PUSH_OP(read_cache);
    populate_cache(FLASH_SUCCESS);
}

static void bad_block_scan_start(int32_t status) {
    block_page_address = PAGE_ADDRESS(0, 0);
    // Factory bad block markers are in the first byte of spare
    check_bad_block(FLASH_SUCCESS);
}

static void flash_post_reset(int32_t status) {
    PUSH_OP(bad_block_scan_start);
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
        cache_dirty = true;
        config_polling_op(MILLIS(TERASE_MS), MICROS(FLASH_POLL_US));
        spi_write(tx_rx_buffer, 4, status_poll);
    } else {
        op_handler_pop(FLASH_ERR_BAD_STATE);
    }
};

static void flash_erase_block(flash_page_address_t address) {
    block_page_address = address & BLOCK_MASK;
    PUSH_OP(block_erase);
    set_write_enable_latch(check_bad_block);
};

static void flash_stream_data(int32_t status) {
    uint16_t bytes_to_write = current_transaction.size - bytes_written;
    if (bytes_to_write > MAX_TRANSACTION_BYTES) {
        bytes_to_write = MAX_TRANSACTION_BYTES;
        // We have more to stream, queue op
        PUSH_OP(flash_stream_data);
        spi_cs_hold_flag = true;
    }
    bytes_written += bytes_to_write;
    uint8_t* write_ptr = current_transaction.data + bytes_written;
    spi_write(write_ptr, bytes_to_write, op_handler_pop);
};

// Programs bytes in the write cache
// First call will set cache to 0xFF
// Page must have been previously erased
// Flash can only write down
static void flash_write_cache(uint16_t byte_address_, buffer_t source,
                              length_t length) {
    cache_dirty = true;
    if (0 == bytes_written) {
        tx_rx_buffer[0] = PROGRAM_DATA_LOAD;
    } else {
        tx_rx_buffer[0] = RANDOM_PROGRAM_DATA_LOAD;
    }
    tx_rx_buffer[1] = (byte_address_ >> 8) & 0xFF;
    tx_rx_buffer[2] = byte_address_ & 0xFF;
    current_transaction.data = source;
    current_transaction.size = length;
    spi_cs_hold_flag = true;
    spi_write(tx_rx_buffer, 3, flash_stream_data);
};

static void flash_modify_cache(uint16_t byte_addres_, buffer_t source,
                               length_t length) {
    cache_dirty = true;
    current_transaction.data = source;
    current_transaction.size = length;
    write_cache(FLASH_SUCCESS);
}

// Flushes write-cache to flash
static void flash_program(int32_t status) {
    tx_rx_buffer[0] = PROGRAM_EXECUTE;
    tx_rx_buffer[1] = TIMING_BYTE;
    tx_rx_buffer[2] = (block_page_address >> 8) & 0xFF;
    tx_rx_buffer[3] = block_page_address & 0xFF;
    bytes_written = 0;
    cache_dirty = false;
    config_polling_op(MICROS(TPROG_US), MICROS(FLASH_POLL_US));
    spi_write(tx_rx_buffer, 4, status_poll);
};

// Starts a flushing op
// needs to be called before write/erase operations
static void flash_flush_cache(int32_t status) {
    set_write_enable_latch(flash_program);
};

static void verify_jedec(int32_t status) {
    memcpy(jedec_id, tx_rx_buffer + JEDEC_OFFSET, 3);
    serial_printf("RX:");
    for (uint8_t i = 0; i < 3; ++i) {
        serial_printf(" 0x%02x", jedec_id[i]);
    }
    serial_print("\r\n");
    if (jedec_id[0] == 0xCD && jedec_id[1] == 0x71 && jedec_id[2] == 0x71) {
        op_handler_pop(FLASH_SUCCESS);
    } else {
        serial_printf("Uknown JEDEC ID\r\n");
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
        op_handler_pop(FLASH_ERR_TIMEOUT);
    }
}

static void read_status(int32_t status) {
    byte_address = STATUS_REGISTER;
    PUSH_OP(check_status);
    if (NULL == task_immediate_signal(get_feature, FLASH_SUCCESS)) {
        serial_printf("Scheduling failed\r\n");
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
            mark_bad_block(FLASH_SUCCESS);
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

void flash_read(flash_page_address_t bp_addr, uint16_t byte_address_,
                buffer_t dest, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        current_transaction.data = dest;
        current_transaction.size = size;
        if ((cache_dirty) || (bp_addr != block_page_address)) {
            // Need to fetch the page from memory
            PUSH_OP(read_cache);
            populate_cache(FLASH_SUCCESS);
        } else {
            // Our page is already in memory, we can optimize the read
            read_cache(FLASH_SUCCESS);
        }
    }
}

// Setup a write with program data
void flash_write(flash_page_address_t page, uint16_t byte_address_,
                 buffer_t data, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        if (block_page_address == page && cache_dirty) {
            // We are on the same page as previous operations, can use random
            // access (or it's the first write, and we can start our write op)
            flash_write_cache(byte_address_, data, size);
        } else {
            op_handler_pop(FLASH_ERR_CACHE_OVERWRITE);
        }
    }
}

// Perform a read-modify-write on a page - read in if it's the first op
void flash_update(flash_page_address_t page, uint16_t byte_address_,
                  buffer_t data, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        byte_address = byte_address_;
        current_transaction.size = size;
        current_transaction.data = data;
        if (block_page_address == page && cache_dirty) {
            write_cache(FLASH_SUCCESS);
        } else {
            block_page_address = page;
            PUSH_OP(write_cache);
            populate_cache(FLASH_SUCCESS);
        }
    }
}

void flash_commit(callback_t on_complete) {
    if (lock_flash(on_complete)) {
        flash_flush_cache(FLASH_SUCCESS);
    }
}

void flash_erase(uint32_t addr, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        flash_erase_block(addr);
    }
}

void flash_init(callback_t on_init) {
    user_callback = on_init;
    spi_finish();  // Resets the CS line
    flash_reset(FLASH_SUCCESS);
}

void flash_op_complete_handler(int32_t status) {
    if (!spi_cs_hold_flag) {
        spi_finish();
    } else {
        spi_cs_hold_flag = false;
    }
    if (NULL != operation_callback) {
        if (NULL == task_immediate_signal(operation_callback, status)) {
            serial_printf("Scheduling failed\r\n");
        }
        operation_callback = NULL;
    }
}
