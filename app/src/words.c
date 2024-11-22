#include "words.h"

#include "filesystem.h"

#include <string.h>
#include <stdint.h>

static uint32_t num_words = 0;

#define HEADER_ALIGNMENT (MAX_WORD_LENGTH)

void words_init(void)
{
    file_open(WORDS_DB);
    if (file_read((uint8_t*)&num_words, sizeof(uint32_t)) == 0)
    {
        num_words = 0;
    }
    if (num_words == 0xFFFFFFFF)
    {
        num_words = 0;
    }
}

uint32_t words_count(void) { return num_words; }

void words_get(uint32_t index, word_t* dest) {
    // Words are 16 byte aligned structures
    if (index < num_words) {
        if (NULL != dest) {
            // Offset by (MAX_WORD_LENGTH) 
            file_seek((index * MAX_WORD_LENGTH) + HEADER_ALIGNMENT);
            file_read((buffer_t)dest, MAX_WORD_LENGTH);
        }
    }
}
