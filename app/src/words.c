#include "words.h"

#include "filesystem.h"

#include <string.h>
#include <stdint.h>

static uint32_t num_words = 0;

void words_init(void)
{
    file_open(WORDS_DB);
    file_read((buffer_t)&num_words, sizeof(uint32_t));
}

uint32_t words_count(void) { return num_words; }

void words_get(uint32_t index, word_t* storage) {
    // Words are 16 byte aligned structures
    if (index < num_words) {
        if (NULL != storage) {
            file_aseek((index * MAX_WORD_LENGTH) + sizeof(uint32_t));
            file_read((buffer_t)storage, MAX_WORD_LENGTH);
        }
    }
}
