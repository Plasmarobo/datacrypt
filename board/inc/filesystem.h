#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "defs.h"

#include <stddef.h>

// Flash chip has 1024 blocks of 64 * 2k bytes
// 1GiB = 128MB
// 64MB of hydrated word data
// 64MB of audio samples
#define WORD_DB_BASE_ADDRESS (0x00000400)
#define AUDIO_DB_BASE_ADDRESS (0x03D09400)

#define WORDS_DB ("/words.db")
#define AUDIO_DB ("/audio.db")

#define MINIMUM_OFFSET (WORD_DB_BASE_ADDRESS)

static uint32_t current_offset;

void filesystem_init();
void file_open(const char* path);

size_t file_read(buffer_t dest, size_t size);
void file_seek(int seekv);
void file_close();

#endif // FILESYSTEM_H
