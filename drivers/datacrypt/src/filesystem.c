#include "filesystem.h"

#include <bsp.h>
#include <flash.h>
#include <popcnt.h>
#include <scheduler.h>
#include <stdint.h>

#define FILESYSTEM_TIMEOUT_MS (250)
// We can tolerate one bit of error - at two the block is marked bad

typedef struct {
    flash_page_address_t block_offset;
    uint16_t byte_offset;
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

#define BLOCK_SIZE (PAGES_PER_BLOCK * PAGE_SIZE)
#define INVALID_ADDRESS (0xFFFFFFFF)

// Good blocks are marked with a 1 - (which means we do not need to erase before
// writing to the bbt, we will only ever write-down)
flash_page_address_t translate_address(flash_page_address_t input_address) {
    // Calc initial predicted block
    uint32_t target_block = input_address >> BBT_LOG2;
    uint32_t unset_bits = 0;
    // Scan preceeding blocks to get offset
    for (uint32_t index = 0;
         index < ((target_block + unset_bits) / sizeof(bbt_t)); ++index) {
        // Calc bitmask, will be 0xFFFFFFFF until we reach the final block
        unset_bits += (8 * sizeof(bbt_t)) - pop_count32(bad_block_table[index]);
        // Check that we haven't advanced past the end of the BBT
        if ((target_block + unset_bits) >= BLOCK_COUNT) {
            return INVALID_ADDRESS;
        }
    }
    // Check first bits of target block
    uint32_t lastblock =
        bad_block_table[(target_block + unset_bits) / sizeof(bbt_t)];
    for (uint32_t bit = 0; bit <= ((target_block + unset_bits) % sizeof(bbt_t));
         ++bit) {
        // Check next bit
        if (!(lastblock & (0x01 << bit))) {
            unset_bits += 1;
            if (bit >= sizeof(bbt_t)) {
                if ((target_block + unset_bits >= BLOCK_COUNT)) {
                    return INVALID_ADDRESS;
                }
                // Start again at next dword
                bit = 0;
                lastblock = bad_block_table[(target_block + unset_bits)];
            }
        }
    }
    uint32_t translated_address = (unset_bits * BLOCK_SIZE) + input_address;
    return translated_address;
}

static void rebuild_bbt(void) {
    future_t future;
    uint32_t status;
    uint8_t mark_buf;
    for (uint32_t block = 0; block < BLOCK_COUNT; ++block) {
        dbgprintf("Block %04d/1024\r", block);
        // Check first page
        WITH_FUTURE(flash_read(PAGE_ADDRESS(block, 0), OOB_BASE_ADDRESS,
                               &mark_buf, 1, future),
                    MILLIS(FILESYSTEM_TIMEOUT_MS));
        if (ERASED_VALUE == mark_buf) {
            // Check second page
            WITH_FUTURE(flash_read(PAGE_ADDRESS(block, 1), OOB_BASE_ADDRESS,
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
            flash_read(PAGE_ADDRESS(block, 0), 0, &buf, sizeof(bbt_t), future),
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
            WITH_FUTURE(flash_write(translate_address(BBT_BASE_ADDRESS), 0,
                                    &header, sizeof(bbt_t), future),
                        MILLIS(FILESYSTEM_TIMEOUT_MS));
            if (status) {
                dbgprintf("FS:%d\r\n", status);
            }
            WITH_FUTURE(
                flash_update(translate_address(BBT_BASE_ADDRESS), sizeof(bbt_t),
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
            return;
        } else if (BBT_MAGIC == buf) {
            // We have found our pattern, read in the bbt
            WITH_FUTURE(
                flash_read(PAGE_ADDRESS(0, 0), sizeof(bbt_t), &bad_block_table,
                           sizeof(bad_block_table), future),
                MILLIS(FILESYSTEM_TIMEOUT_MS));
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
    WITH_FUTURE(flash_write(block, 0, &write_buffer, sizeof(bbt_t), future),
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
        flash_update(bbt_address, 0, &write_buffer, sizeof(bbt_t), future),
        MILLIS(FILESYSTEM_TIMEOUT_MS));
    if (status) {
        dbgprintf("FS:%d\r\n", status);
    }
    WITH_FUTURE(flash_update(bbt_address, sizeof(bbt_t), bad_block_table,
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

// returns bytes until page flip (for contiguous reads/write emulation)
static int32_t calculate_flash_address(uint32_t flat_address,
                                       flash_page_address_t* page_index,
                                       uint16_t* byte_index) {
    // Pitch excludes OOB bytes
    if (page_index == NULL || byte_index == NULL) {
        return -1;
    }
    const uint32_t PAGE_PITCH = 2048;
    *byte_index = flat_address % PAGE_PITCH;
    *page_index = flat_address / 2048;
    return (PAGE_PITCH - (*byte_index));
}

void filesystem_init(int32_t status) {
    current_file.block_offset = 0;
    current_file.byte_offset = 0;
    current_file.maximum_rw = 0;
    // flash_register_event_handler(FLASH_EV_IDLE, filesystem_start);
    flash_register_event_handler(FLASH_EV_BAD_BLOCK, bbt_mark_bad);
    // flash_register_event_handler(FLASH_EV_TIMEOUT, filesystem_abort);
    task_delayed(bbt_init, MILLIS(5));
}

void file_open(const char* path) {
    if (strcmp(path, WORDS_DB) == 0) {
        current_file.maximum_rw = calculate_flash_address(
            WORD_DB_BASE_ADDRESS, current_file.block_offset,
            &current_file.byte_offset);
        current_file.file_address = translate_address(WORD_DB_BASE_ADDRESS);
        // Calculate this
        current_file.file_size =
            translate_address(WORD_DB_BASE_ADDRESS + WORD_DB_SIZE) -
            translate_address(WORD_DB_BASE_ADDRESS);
    } else if (strcmp(path, AUDIO_DB) == 0) {
        current_file.maximum_rw = calculate_flash_address(
            AUDIO_DB_BASE_ADDRESS, &current_file.block_offset,
            &current_file.byte_offset);
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
    uint32_t status;
    uint32_t count = 0;
    while (size > current_file.maximum_rw) {
        // Split read until it fits within one page
        WITH_FUTURE(
            flash_read(current_file.block_offset, current_file.byte_offset,
                       dest + count, current_file.maximum_rw, future),
            MILLIS(FILESYSTEM_TIMEOUT_MS));
        size -= current_file.maximum_rw;
        count += current_file.maximum_rw;
        current_file.block_offset += 1;
        current_file.byte_offset = 0;
        current_file.maximum_rw = PAGE_SIZE;
    }
    WITH_FUTURE(flash_read(current_file.block_offset, current_file.byte_offset,
                           dest + count, size, future),
                MILLIS(FILESYSTEM_TIMEOUT_MS));
    current_file.maximum_rw -= size;
    count += size;
    if (current_file.maximum_rw == 0) {
        current_file.block_offset += 1;
        current_file.byte_offset = 0;
        current_file.maximum_rw = PAGE_SIZE;
    }
    return count;
}

size_t file_write(buffer_t source, size_t size) {
    future_t future;
    uint32_t status;
    uint32_t count = 0;
    while (size > current_file.maximum_rw) {
        WITH_FUTURE(
            flash_update(current_file.block_offset, current_file.byte_offset,
                         source + count, current_file.maximum_rw, future),
            MILLIS(FILESYSTEM_TIMEOUT_MS));
        size -= current_file.maximum_rw;
        count += current_file.maximum_rw;
        current_file.block_offset += 1;
        current_file.byte_offset = 0;
        current_file.maximum_rw = PAGE_SIZE;
        WITH_FUTURE(flash_commit(future), MILLIS(FILESYSTEM_TIMEOUT_MS));
    }
    WITH_FUTURE(
        flash_update(current_file.block_offset, current_file.byte_offset,
                     source + count, current_file.maximum_rw, future),
        MILLIS(FILESYSTEM_TIMEOUT_MS));
    current_file.maximum_rw -= size;
    count += size;
    if (current_file.maximum_rw = 0) {
        current_file.block_offset += 1;
        current_file.byte_offset = 0;
        current_file.maximum_rw = PAGE_SIZE;
    }
    return count;
}

void file_seek(uint32_t seekv) {
    // From base address
    current_file.maximum_rw = calculate_flash_address(
        current_file.file_address + seekv, current_file.block_offset,
        current_file.byte_offset);
}

uint32_t file_tell() {
    return (current_file.block_offset * 2048) + current_file.byte_offset;
}

void file_close() {
    current_file.block_offset = 0;
    current_file.byte_offset = 0;
    current_file.maximum_rw = 0;
    current_file.file_address = 0;
    current_file.file_size = 0;
}
