#ifndef WORDS_H
#define WORDS_H

#include <stdint.h>

#define MAX_WORD_LENGTH (16)

typedef char word_t[MAX_WORD_LENGTH];

void words_init(void);
uint32_t words_count(void);
void words_get(uint32_t index, word_t* storage);

#endif  // WORDS_H
