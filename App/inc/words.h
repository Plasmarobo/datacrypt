#ifndef WORDS_H
#define WORDS_H

#include <stdint.h>

#define MAX_WORD_LENGTH (16)

typedef char word_t[MAX_WORD_LENGTH];

uint16_t word_count();
void get_word(uint16_t word, word_t* storage);

#endif  // WORDS_H
