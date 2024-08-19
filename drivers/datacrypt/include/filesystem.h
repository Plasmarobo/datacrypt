#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "defs.h"

#include <stddef.h>

// Flash chip has 1024 blocks of 64 * 2k bytes
// Valid addresses start at 0x800
// Last valid address is
// 1GiB = 128MB
// 64MB of hydrated word data
// 64MB of audio samples

// Using byte wise offsets from zero
// Start at page 1 (page 0 reserved for system)
#define WORD_DB_BASE_ADDRESS (0x00000800)
// Start at page 32,768
#define AUDIO_DB_BASE_ADDRESS (0x3FFF800)
// Last address is page 63, block 1023, byte 2047
// Max block page fits exactly into 16 bits = 65535
#define MAXIMUM_ADDRESS (0x8000000)
#define MINIMUM_ADDRESS (WORD_DB_BASE_ADDRESS)

#define WORD_DB_SIZE (AUDIO_DB_BASE_ADDRESS - WORD_DB_BASE_ADDRESS)
#define AUDIO_DB_SIZE (MAXIMUM_ADDRESS - AUDIO_DB_BASE_ADDRESS)

#define WORDS_DB ("/words.db")
#define AUDIO_DB ("/audio.db")

void filesystem_init(int32_t status);
void file_open(const char* path);

size_t file_read(buffer_t dest, size_t size);
size_t file_write(buffer_t source, size_t size);
void file_seek(uint32_t seekv);
uint32_t file_tell();
void file_close();

#endif // FILESYSTEM_H
