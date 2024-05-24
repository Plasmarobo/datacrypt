#include "filesystem.h"

#include <stdint.h>

#include "bsp.h"
#include "flash.h"
#include "popcnt.h"
#include "scheduler.h"

#define FILESYSTEM_TIMEOUT_MS (500)
// We can tolerate one bit of error - at two the block is marked bad

typedef uint32_t bbt_t;
// Result of log2(block_count)
#define BBT_LOG2 (5)
//
#define BBT_MASK (0x1F)
// BBT lives at start of flash
#define BBT_BASE_ADDRESS (0)
#define BBT_WORDS (BLOCK_COUNT >> BBT_LOG2)
#define BBT_MAGIC (0xBADB10C0)

// Bit shift should match log2(sizeof(bbt_t))
static bbt_t bad_block_table[BBT_WORDS];

#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE)
#define INVALID_ADDRESS (BLOCK_COUNT * BLOCK_SIZE)

// Good blocks are marked with a 1 - (which means we do not need to erase before
// writing to the bbt, we will only ever write-down)
uint32_t translate_address(uint32_t input_address) {
    // Calc initial predicted block
    uint32_t target_block = input_address >> BBT_LOG2;
    uint32_t unset_bits = 0;
    uint32_t index;
    for (index = 0; index < (target_block / sizeof(bbt_t)) + unset_bits - 1;
         ++index) {
        // Calc bitmask, will be 0xFFFFFFFF until we reach the final block
        unset_bits += (8 * sizeof(bbt_t)) - pop_count(bad_block_table[index]);
    }
    // Check last word
    uint32_t additional_offset = 0;
    while (!(bad_block_table[index + 1] & (0x01 << additional_offset))) {
        additional_offset += 1;
        if (additional_offset > sizeof(bbt_t)) {
            index += 1;
            if (index >= BLOCK_COUNT) {
                // Critical error, we cannot find additional non-bad space
                return INVALID_ADDRESS;
            }
        }
    }
    unset_bits += additional_offset;
    uint32_t translated_address = (unset_bits * BLOCK_SIZE) + input_address;
    return (unset_bits * BLOCK_SIZE) + input_address;
}

static void rebuild_bbt(void) {
    callback_t future;
    uint32_t status;
    uint8_t mark_buf;
    for (uint32_t block = 0; block < BLOCK_COUNT; ++block) {
        // Check first page
        future = future_get();
        flash_read(PAGE_ADDRESS(block, 0), OOB_BASE_ADDRESS, &mark_buf, 1,
                   future);
        status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (ERASED_VALUE == mark_buf) {
            // Check second page
            future = future_get();
            flash_read(PAGE_ADDRESS(block, 1), OOB_BASE_ADDRESS, &mark_buf, 1,
                       future);
            status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        }
        // Check if either read produced a bad block value
        if (ERASED_VALUE != mark_buf) {
            // Write-down the specified bit
            bad_block_table[block >> BBT_LOG2] &= ~(0x01 << (block & BBT_MASK));
        }
    }
}

// Copy BBT from flash
void bbt_init(void) {
    callback_t future;
    uint32_t status;
    bbt_t buf = 0;
    for (uint16_t block = 0; block < BLOCK_COUNT; ++block) {
        future = future_get();
        flash_read(PAGE_ADDRESS(block, 0), 0, &buf, sizeof(bbt_t), future);
        status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (0xFFFFFFFF == buf) {
            // Flash is uninitialized - run scan to (re)build table
            // Initialize to all good
            for (uint8_t i = 0; i < BBT_WORDS; ++i) {
                bad_block_table[i] = 0xFFFFFFFF;
            }
            rebuild_bbt();
            return;
        } else if (BBT_MAGIC == buf) {
            // We have found our pattern, read in the bbt
            future = future_get();
            flash_read(PAGE_ADDRESS(0, 0), sizeof(bbt_t), &bad_block_table,
                       sizeof(bad_block_table), future);
            status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
            return;
        } else if (0 == buf) {
            // Flash has been written down, keep looking
            continue;
        } else {
            // Corrupted flash? TODO: Handle error
        }
    }
}

void bbt_mark_bad(uint32_t input_address) {
    uint32_t status;
    callback_t future;
    bbt_t write_buffer = 0;
    // Update the RAM table
    uint32_t block = (translate_address(input_address) >> BBT_LOG2);
    // Write down BBT space on marked bad block
    // Okay, we shouldn't be writing here, but we're gonna anyway
    future = future_get();
    flash_write(block, 0, &write_buffer, sizeof(bbt_t), future);
    status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d", status);
    }
    // Reset the bit corresponding to the block
    bad_block_table[block >> BBT_LOG2] &= ~(0x01 << (block & BBT_MASK));
    future = future_get();
    uint32_t bbt_address = translate_address(BBT_BASE_ADDRESS);
    write_buffer = BBT_MAGIC;
    future = future_get();
    flash_update(bbt_address, 0, &write_buffer, sizeof(bbt_t), future);
    status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d", status);
    }
    future = future_get();
    flash_update(bbt_address, sizeof(bbt_t), bad_block_table,
                 sizeof(bad_block_table), future);
    status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d", status);
    }
    // We need to commit the BBT when updated
    future = future_get();
    flash_commit(future);
    status = future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d", status);
    }
}

void filesystem_init(void) { bbt_init(); }

void file_open(const char* path) {
    if (strcmp(path, WORDS_DB) == 0) {
        current_offset = WORD_DB_BASE_ADDRESS;
    } else if (strcmp(path, AUDIO_DB) == 0) {
        current_offset = AUDIO_DB_BASE_ADDRESS;
    } else {
        current_offset = MINIMUM_OFFSET;
    }
}

size_t file_read(buffer_t dest, size_t size) {
    if (current_offset >= MINIMUM_OFFSET) {
        callback_t future = future_get();
        flash_read(current_offset >> 16, current_offset & 0xFFFF, dest, size,
                   future);
        future_await(future, MILLIS(FILESYSTEM_TIMEOUT_MS));
    }
}

void file_seek(int seekv) {
    uint32_t new_offset = seekv + current_offset;
    if (current_offset >= MINIMUM_OFFSET &&
        current_offset < AUDIO_DB_BASE_ADDRESS) {
        if ((new_offset < AUDIO_DB_BASE_ADDRESS) &&
            (new_offset >= MINIMUM_OFFSET)) {
            current_offset = new_offset;
        }
    } else  // (current_offset >= AUDIO_DB_BASE_ADDRESS)
    {
        if ((new_offset >= AUDIO_DB_BASE_ADDRESS)) {
            current_offset = new_offset;
        }
    }
}

void file_close() { current_offset = MINIMUM_OFFSET; }
