#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "defs.h"

#include <stddef.h>

// Flash chip has 1024 blocks of 64 * 2k bytes
// Valid addresses start at 0x1000
// Last valid address is
// 1GiB = 128MB
// 64MB of hydrated word data
// 64MB of audio samples
#define MAXIMUM_ADDRESS (1024 * 64 * 2048)
#define PAGE_BOUNDARY (0x800)
#define BAD_BLOCK_BASE (0x00000000)
#define USER_DATA_BASE (2 * PAGE_BOUNDARY)
// Using byte wise offsets from zero
// Start at page 2 (page 0 reserved for system)
#define WORD_DB_BASE_ADDRESS (USER_DATA_BASE)
// Start at page 32,768
#define AUDIO_DB_BASE_ADDRESS (0x4000000)
// Last address is page 63, block 1023, byte 2047
// Max block page fits exactly into 16 bits = 65535
// Byte address is from 0 - 2047, fits into 11 bits
// Total address fits into 16 + 11 = 27 bits

#define MINIMUM_ADDRESS (WORD_DB_BASE_ADDRESS)

#define WORD_DB_SIZE (AUDIO_DB_BASE_ADDRESS - WORD_DB_BASE_ADDRESS)
#define AUDIO_DB_SIZE (MAXIMUM_ADDRESS - AUDIO_DB_BASE_ADDRESS)

#define WORDS_DB ("/words.db")
#define AUDIO_DB ("/audio.db")

typedef uint32_t filesystem_address_t;

void filesystem_start(int32_t status);
void filesystem_init(callback_t notify);
void file_open(const char* path);

size_t file_read(buffer_t dest, size_t size);
size_t file_write(buffer_t source, size_t size);
void file_seek(uint32_t seekv);
uint32_t file_tell();
void file_close();

#endif // FILESYSTEM_H
