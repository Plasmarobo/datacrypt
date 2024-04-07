#include "bsp.h"
#include "fsm.h"
#include "ringbuffer.h"
#include "scheduler.h"
#include "stack.h"
#include "lfs.h"
#include "flash.h"

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
#define QUAD_PROGRAM_DATA_LOAD (0x34)
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

#define FLASH_TIMEOUT_MS (5)

// 32 bit header indicating the invalid page table follows
#define MAGIC_BYTES_INVALID_PAGE_TABLE (0x9A6E7ABE)
#define TRST_US (200)
#define TRD_US (25)
#define TPROG_US (700)
#define TERASE_MS (10)
#define MAX_CMD_BYTES (8)
#define JEDEC_BYTES (3)

#define PROTECTION_REGISTER (0xA0)
#define CONFIG_REGISTER (0xB0)
#define STATUS_REGISTER (0xC0)

#define STATUS_OIP (0x01)
#define STATUS_WEL (0x02)
#define STATUS_ERASE_FAIL (0x04)
#define STATUS_PROG_FAIL (0x08)
#define STATUS_ECCS0 (0x10)
#define STATUS_ECCS1 (0x20)

#define PAGE_MASK (0x1F)
#define BLOCK_MASK (0xFFE0)
#define BLOCK(x) ((x & BLOCK_MASK) >> 5)
#define PAGE(x) (x & PAGE_MASK)

#define BYTE_MASK (0x07FF)
#define ERASED_VALUE (0xFF)
#define BAD_BLOCK_VALUE (0x18)
#define PAGE_ADDRESS(block, page) \
    (((block << 5) & BLOCK_MASK) | (page & PAGE_MASK))
#define PAGE_OFFSET(offset) (offset / PAGE_SIZE)

#define OOB_BASE_ADDRESS (PAGE_SIZE)

typedef struct {
    buffer_t data;
    length_t size;
} flash_transaction_t;

static timespan_t poll_interval;
//
static uint8_t command_buffer[MAX_CMD_BYTES];
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

STACK(op_stack, callback_t, 8);

static bool write(buffer_t buffer, length_t len, callback_t oncomplete);
static bool read(buffer_t buffer, length_t len, callback_t oncomplete);
static void config_polling_op(timespan_t interval, timespan_t timeout);
static void status_poll(int32_t status);
static void check_status(int32_t status);
static void timeout_handler(int32_t status);
static void bad_block_scan_start(int32_t status);
static void flash_write_cache(uint16_t byte_address, buffer_t source,
                              length_t length);
static void query_jedec(int32_t status);
static void flash_flush_cache(int32_t status);
static void write_bbt(int32_t status);
static void read_status(int32_t status);
static void op_handler_pop(int32_t status);

// Enables re-using cache where possible
static void mark_cache_clean(int32_t status) {
    if (FLASH_SUCCESS == status) {
        cache_dirty = false;
    }
    op_handler_pop(status);
}

// Load a page into the chip cache
static void populate_cache(int32_t status) {
    command_buffer[0] = PAGE_READ_TO_CACHE;
    command_buffer[1] = TIMING_BYTE;
    command_buffer[2] = (block_page_address >> 8) & 0xFF;
    command_buffer[3] = block_page_address & 0xFF;
    config_polling_op(MICROS(TRD_US), FLASH_TIMEOUT_MS);
    write(command_buffer, 4, mark_cache_clean);
}

// Fetch data from chip
static void read_bytes(int32_t status) {
    read(&current_transaction.data, current_transaction.size, op_handler_pop);
}

// Read cache into memory buffer
static void read_cache(int32_t status) {
    command_buffer[0] = READ_FROM_CACHE;
    command_buffer[1] = (byte_address >> 8) & 0xFF;
    command_buffer[2] = byte_address & 0xFF;
    command_buffer[3] = TIMING_BYTE;
    write(command_buffer, 4, read_bytes);
}

// Fetch selected register/feature from chip
static void read_feature(int32_t status) {
    read(&feature_value, 1, op_handler_pop);
}

// Select register/feature (then read it)
static void get_feature(int32_t status) {
    command_buffer[0] = GET_FEATURE;
    command_buffer[1] = byte_address & 0xFF;
    write(command_buffer, 2, read_feature);
}

// Write register/feature to chip
static void set_feature(int32_t status) {
    command_buffer[0] = SET_FEATURE;
    command_buffer[1] = byte_address & 0xFF;
    command_buffer[2] = feature_value;
    write(command_buffer, 3, op_handler_pop);
}

// Proceed to next op in stack
static void op_handler_pop(int32_t status) {
    callback_t handler = NULL;
    stack_pop(&op_stack, &handler);
    if (NULL != handler) {
        handler(status);
    }
}

// Poll chip for operation completion
static void status_poll(int32_t status) {
    if (FLASH_SUCCESS == status) {
        if (NULL == timeout_task) {
            timeout_task =
                task_delayed_unique(timeout_handler, MILLIS(FLASH_TIMEOUT_MS));
        }
        poll_task = task_delayed(read_status, MILLIS(poll_interval));
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
    stack_push(&op_stack, &flash_flush_cache);
    stack_push(&op_stack, &mark_bbt_page_1);
    stack_push(&op_stack, &flash_flush_cache);
    bytes_written = 0;
    cache_dirty = false;
    byte_address = OOB_BASE_ADDRESS;
    flash_write_cache(block_page_address, &general_buffer, 1);
}

static void check_bbt_value(int32_t status)
{
    if (FLASH_SUCCESS == status)
    {
        if (BAD_BLOCK_VALUE == general_buffer)
        {
            //Block is bad!
            block_bad = true;
        }
        if (!(block_page_address & PAGE_MASK)) {
            // Check page 2
            block_page_address += 1;
            byte_address - OOB_BASE_ADDRESS;
            general_buffer = ERASED_VALUE;
            stack_push(&op_stack, &check_bbt_value);
            populate_cache(FLASH_SUCCESS);
        } else {
            op_handler_pop(status);
        }
    }
}

static void check_bad_block(int32_t status) {
    // Load first page
    block_bad = false;
    block_page_address = block_page_address & BLOCK_MASK;
    byte_address = OOB_BASE_ADDRESS; // First byte in OOB
    general_buffer = ERASED_VALUE;
    stack_push(&op_stack, &check_bbt_value);
    populate_cache(FLASH_SUCCESS);
}

static void flash_reset(int32_t status) {
    // Wait for TRST then transition to startup scan
    flash_busy = true;
    block_page_address = PAGE_ADDRESS(0, 0);
    // Factory bad block markers are in the first byte of spare
    byte_address = PAGE_SIZE;
    current_transaction.data = &general_buffer;
    current_transaction.size = 1;
    stack_push(&op_stack, &bad_block_scan_start);
    task_delayed(query_jedec, MICROS(TRST_US));
};

// Warning: this conducts the actual block erase, check bbt first
static void block_erase(int32_t status) {
    if (FLASH_SUCCESS == status)
    {
        command_buffer[0] = BLOCK_ERASE;
        command_buffer[1] = TIMING_BYTE;
        command_buffer[2] = (block_page_address >> 8) & 0xFF;
        command_buffer[3] = block_page_address & 0xFF;
        cache_dirty = true;
        config_polling_op(MILLIS(TERASE_MS), FLASH_TIMEOUT_MS);
        write(command_buffer, 4, status_poll);
    }
    else
    {
        op_handler_pop(FLASH_ERR_BAD_STATE);
    }
};

static void flash_erase_block(flash_page_address_t address)
{
    block_page_address = address & BLOCK_MASK;
    stack_push(&op_stack, &block_erase);
    stack_push(&op_stack, &check_bad_block);
    op_handler_pop(FLASH_SUCCESS);
};

static void flash_stream_data(int32_t status) {
    bytes_written += current_transaction.size;
    write(current_transaction.data, current_transaction.size, op_handler_pop);
};

// Programs bytes in the write cache
// First call will set cache to 0xFF
// Page must have been previously erased
// Flash can only write down
static void flash_write_cache(uint16_t byte_address, buffer_t source,
                              length_t length) {
    cache_dirty = true;
    if (0 == bytes_written) {
        command_buffer[0] = PROGRAM_DATA_LOAD;
    } else {
        command_buffer[0] = RANDOM_PROGRAM_DATA_LOAD;
    }
    command_buffer[1] = (byte_address >> 8) & 0xFF;
    command_buffer[2] = byte_address & 0xFF;
    current_transaction.data = source;
    current_transaction.size = length;
    write(command_buffer, 3, flash_stream_data);
};

// Flushes write-cache to flash
static void flash_program(int32_t status) {
    command_buffer[0] = PROGRAM_EXECUTE;
    command_buffer[1] = TIMING_BYTE;
    command_buffer[2] = (block_page_address >> 8) & 0xFF;
    command_buffer[3] = block_page_address & 0xFF;
    bytes_written = 0;
    cache_dirty = false;
    config_polling_op(MICROS(TPROG_US), FLASH_TIMEOUT_MS);
    write(command_buffer, 4, status_poll);
};

// Starts a flush op
static void flash_flush_cache(int32_t status) {
    command_buffer[0] = WRITE_ENABLE;
    write(command_buffer, 1, flash_program);
};

static void read_jedec(int32_t status) { read(jedec_id, 3, op_handler_pop); };

static void query_jedec(int32_t status) {
    command_buffer[0] = JEDEC_READ;
    command_buffer[1] = TIMING_BYTE;
    write(command_buffer, 2, read_jedec);
};

static bool write(buffer_t buffer, length_t len, callback_t oncomplete) {
    operation_callback = oncomplete;
    HAL_SPI_Transmit_DMA(&hspi2, buffer, len);
}

static bool read(buffer_t buffer, length_t len, callback_t oncomplete) {
    operation_callback = oncomplete;
    HAL_SPI_Receive_DMA(&hspi2, buffer, len);
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
    get_feature(check_status);
}

static void config_polling_op(timespan_t interval, timespan_t timeout) {
    feature_value |= STATUS_OIP;
    poll_interval = interval;
}

static void check_status(int32_t status) {
    if (feature_value & STATUS_OIP) {
        poll_task = task_delayed(read_status, MILLIS(poll_interval));
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
            // Note: ops should handle bad block without altering the block_page_address
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
    stack_push(&op_stack, &notify);
    stack_push(&op_stack, &unlock_flash);
    return true;
}

void flash_read(flash_page_address_t bp_addr, uint16_t byte_address_,
                buffer_t dest, length_t size, callback_t on_complete) {
    if (lock_flash(on_complete)) {
        current_transaction.data = dest;
        current_transaction.size = size;
        if ((cache_dirty) || (bp_addr != block_page_address)) {
            // Need to fetch the page from memory
            stack_push(&op_stack, &read_cache);
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
            // We are on the same page as previous operations, can use random access
            // (or it's the first write, and we can start our write op)
            flash_write_cache(byte_address_, data, size);
        } else {
            op_handler_pop(FLASH_ERR_CACHE_OVERWRITE);
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
    flash_reset(FLASH_SUCCESS);
}

void flash_tx_complete_handler(int32_t status) {
    if (NULL != operation_callback) {
        operation_callback(status);
    }
}

void flash_rx_complete_handler(int32_t status) {
    if (NULL != operation_callback) {
        operation_callback(status);
    }
}
