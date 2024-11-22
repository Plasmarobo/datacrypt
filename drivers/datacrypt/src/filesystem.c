#include "filesystem.h"

#include <bsp.h>
#include <flash.h>
#include <popcnt.h>
#include <scheduler.h>
#include <stdint.h>

#define FILESYSTEM_TIMEOUT_MS (250)
// We can tolerate one bit of error - at two the block is marked bad

typedef struct {
    uint32_t file_offset;
    uint16_t maximum_rw;
    uint32_t file_size;
    uint32_t file_address;
} file_t;

typedef uint32_t bbt_t;
// Result of log2(block_count)
#define BBT_LOG2 (5)
//
#define BBT_MASK (0x1F)
// BBT lives at start of flash
#define BBT_BASE_ADDRESS (0)
#define BBT_DWORDS (BLOCK_COUNT >> BBT_LOG2)
#define BBT_WRITE_CHUNKS (FLASH_MAX)
#define BBT_MAGIC (0xBADB10CC)

// Bit shift should match log2(sizeof(bbt_t))
static bbt_t bad_block_table[BBT_DWORDS];
static file_t current_file;
static callback_t on_ready;

#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE)
#define INVALID_ADDRESS (0xFFFFFFFF)

// Good blocks are marked with a 1 - (which means we do not need to erase before
// writing to the bbt, we will only ever write-down)
uint32_t translate_address(uint32_t input_address) {
    // Calc initial predicted bit offset
    uint32_t target_block = input_address / BLOCK_SIZE;
    uint32_t preceeding_bad_blocks = 0;
    // Scan preceeding blocks to get offset
    for (uint32_t index = 0;
         index < ((target_block + preceeding_bad_blocks) / sizeof(bbt_t)); ++index) {
        // Calc bitmask, will be 0xFFFFFFFF until we reach the final block
        preceeding_bad_blocks += (8 * sizeof(bbt_t)) - pop_count32(bad_block_table[index]);
        // Check that we haven't advanced past the end of the BBT
        if ((target_block + preceeding_bad_blocks) >= BLOCK_COUNT) {
            return INVALID_ADDRESS;
        }
    }
    // Check first bits of target block
    uint32_t last_block =
        bad_block_table[(target_block + preceeding_bad_blocks) / sizeof(bbt_t)];
    for (uint32_t bit = 0; bit <= ((target_block + preceeding_bad_blocks) % sizeof(bbt_t));
         ++bit) {
        // Check next bit
        if (!(last_block & (0x01 << bit))) {
            preceeding_bad_blocks += 1;
            if (bit >= sizeof(bbt_t)) {
                if ((target_block + preceeding_bad_blocks >= BLOCK_COUNT)) {
                    return INVALID_ADDRESS;
                }
                // Start again at next dword
                bit = 0;
                last_block = bad_block_table[(target_block + preceeding_bad_blocks) / sizeof(bbt_t)];
            }
        }
    }
    uint32_t translated_address = (preceeding_bad_blocks * BLOCK_SIZE) + input_address;
    return translated_address;
}

static void rebuild_bbt(void) {
    future_t future;
    uint32_t status;
    uint8_t mark_buf;
    for (uint32_t block = 0; block < BLOCK_COUNT; ++block) {
        dbgprintf("Block %04d/1024\r", block);
        // Check first page
        WITH_FUTURE(flash_read(FLASH_ADDRESS(block, 0, OOB_BASE_ADDRESS),
                               &mark_buf, 1, future),
                    MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (ERASED_VALUE == mark_buf) {
            // Check second page
            WITH_FUTURE(flash_read(FLASH_ADDRESS(block, 1, OOB_BASE_ADDRESS),
                                   &mark_buf, 1, future),
                        MILLIS(FILESYSTEM_TIMEOUT_MS));
        }
        // Check if either read produced a bad block value
        if (ERASED_VALUE != mark_buf) {
            // Write-down the specified bit
            bad_block_table[block >> BBT_LOG2] &= ~(0x01 << (block & BBT_MASK));
            dbgprintf("\nBB:%d\r\n", block);
        }
    }
    // Our bbt is built
    dbgprintf("BBT: ");
    for (uint32_t i = 0; i < BBT_DWORDS; ++i) {
        dbgprintf("%08x", bad_block_table[i]);
    }
    dbgprint("\r\n\n");
}

// Copy BBT from flash
static void bbt_init(int32_t status) {
    future_t future;
    bbt_t buf = 0;
    for (uint16_t block = 0; block < BLOCK_COUNT; ++block) {
        WITH_FUTURE(
            flash_read(FLASH_ADDRESS(block, 0, 0), &buf, sizeof(bbt_t), future),
            MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (0xFFFFFFFF == buf) {
            // Flash is uninitialized - run scan to (re)build table
            // Initialize to all good
            for (uint8_t i = 0; i < BBT_DWORDS; ++i) {
                bad_block_table[i] = 0xFFFFFFFF;
            }
            rebuild_bbt();
            // write bbt to flash
            uint32_t address = 0;
            bbt_t header = BBT_MAGIC;
            WITH_FUTURE(flash_write(translate_address(BBT_BASE_ADDRESS),
                                    &header, sizeof(bbt_t), future),
                        MILLIS(FILESYSTEM_TIMEOUT_MS));
            if (status) {
                dbgprintf("FS:%d\r\n", status);
            }
            WITH_FUTURE(
                flash_update(translate_address(BBT_BASE_ADDRESS) + sizeof(bbt_t),
                             &bad_block_table, sizeof(bad_block_table), future),
                MILLIS(FILESYSTEM_TIMEOUT_MS));
            if (status) {
                dbgprintf("FS:%d\r\n", status);
            }
            WITH_FUTURE(flash_commit(future), MILLIS(FILESYSTEM_TIMEOUT_MS));
            if (status) {
                dbgprintf("FS:%d\r\n", status);
            } else {
                dbgprintf("BBT Saved\r\n");
            }
            if (on_ready != NULL) {
                on_ready(0);
                on_ready = NULL;
            }
            return;
        } else if (BBT_MAGIC == buf) {
            // We have found our pattern, read in the bbt
            WITH_FUTURE(
                flash_read(FLASH_ADDRESS(block, 0, 0) + sizeof(bbt_t), &bad_block_table,
                           sizeof(bad_block_table), future),
                MILLIS(FILESYSTEM_TIMEOUT_MS));
            if (on_ready != NULL)
            {
                on_ready(0);
                on_ready = NULL;
            }
            return;
        } else if (0 == buf) {
            // Flash has been written down, keep looking
            continue;
        } else {
            // Corrupted flash? TODO: Handle error
            continue;
        }
    }
}

void bbt_mark_bad(uint32_t input_address) {
    uint32_t status;
    future_t future;
    bbt_t write_buffer = 0;
    // Update the RAM table
    uint32_t block = (translate_address(input_address) >> BBT_LOG2);
    // Write down BBT space on marked bad block
    // Okay, we shouldn't be writing here, but we're gonna anyway
    WITH_FUTURE(flash_write(block, &write_buffer, sizeof(bbt_t), future),
                MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d\r\n", status);
    }
    WITH_FUTURE(flash_commit(future), MILLIS(FILESYSTEM_TIMEOUT_MS));
    // Reset the bit corresponding to the block
    bad_block_table[block >> BBT_LOG2] &= ~(0x01 << (block & BBT_MASK));
    uint32_t bbt_address = translate_address(BBT_BASE_ADDRESS);
    write_buffer = BBT_MAGIC;
    WITH_FUTURE(
        flash_update(bbt_address, &write_buffer, sizeof(bbt_t), future),
        MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d\r\n", status);
    }
    WITH_FUTURE(flash_update(bbt_address + sizeof(bbt_t), bad_block_table,
                             sizeof(bad_block_table), future),
                MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d\r\n", status);
    }
    // We need to commit the BBT when updated
    WITH_FUTURE(flash_commit(future), MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d\r\n", status);
    }
}

void filesystem_init(callback_t notify)
{
    on_ready = notify;
}

void filesystem_start(int32_t status) {
    current_file.file_offset = 0;
    // flash_register_event_handler(FLASH_EV_IDLE, filesystem_start);
    flash_register_event_handler(FLASH_EV_BAD_BLOCK, bbt_mark_bad);
    // flash_register_event_handler(FLASH_EV_TIMEOUT, filesystem_abort);
    task_delayed(bbt_init, MILLIS(5));
}

void file_open(const char* path) {
    if (strcmp(path, WORDS_DB) == 0) {
        current_file.file_address = translate_address(WORD_DB_BASE_ADDRESS);
        // Calculate this
        current_file.file_size =
            translate_address(WORD_DB_BASE_ADDRESS + WORD_DB_SIZE) -
            translate_address(WORD_DB_BASE_ADDRESS);
    } else if (strcmp(path, AUDIO_DB) == 0) {
        current_file.file_address = translate_address(AUDIO_DB_BASE_ADDRESS);
        // Calculate this
        current_file.file_size =
            translate_address(AUDIO_DB_SIZE + AUDIO_DB_BASE_ADDRESS) -
            translate_address(AUDIO_DB_BASE_ADDRESS);
    } else {
        file_close();
    }
}

size_t file_read(buffer_t dest, size_t size) {
    future_t future;
    uint32_t status = 0;
    uint32_t count = 0;

    while (size)
    {
        // Read up to page boundary
        uint32_t max_read = PAGE_BOUNDARY - (current_file.file_offset % PAGE_BOUNDARY);
        uint32_t read_length;
        if (size < max_read)
        {
            read_length = size;
        } else {
            read_length = max_read;
        }
        WITH_FUTURE(flash_read(current_file.file_address + current_file.file_offset, dest + count, read_length, future), MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (status)
        {
            // Handle error
            return 0;
        }
        count += read_length;
        size -= read_length;
        current_file.file_offset += read_length;
    }
    return count;
}

size_t file_write(buffer_t source, size_t size) {
    future_t future;
    uint32_t status;
    uint32_t count = 0;

    while (size)
    {
        uint32_t max_write = PAGE_BOUNDARY - (current_file.file_offset % PAGE_BOUNDARY);
        uint32_t write_length;
        if (size < max_write)
        {
            write_length = size;
        } else {
            write_length = max_write;
        }
        WITH_FUTURE(flash_update(current_file.file_address + current_file.file_offset, source + count, write_length, future), MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (status)
        {
            return 0;
        }
        count += write_length;
        size -= write_length;
        current_file.file_offset += write_length;
        WITH_FUTURE(flash_commit(future), MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (status)
        {
            return 0;
        }
    }
    return count;
}

void file_seek(uint32_t seekv) {
    // From base address
    current_file.file_offset = seekv;
}

uint32_t file_tell() {
    return current_file.file_offset;
}

void file_close() {
    current_file.file_offset = 0;
    current_file.file_address = 0;
    current_file.file_size = 0;
}
