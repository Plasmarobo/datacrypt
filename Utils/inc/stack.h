#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"

#define STACK(name, type, size)                                   \
    static uint8_t CONCAT(name, _st_buffer)[sizeof(type) * size]; \
    static stack_t name = {CONCAT(name, _st_buffer), NULL, sizeof(type), size};

typedef struct {
    uint8_t* root;
    uint8_t* head;
    length_t element_size;
    length_t capacity;
} stack_t;

bool stack_full(stack_t* st);
bool stack_empty(stack_t* st);
bool stack_push(stack_t* st, void* item);
bool stack_pop(stack_t* st, void* item);
bool stack_peek(stack_t* st, void* item);

#endif
