#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ARM toolchain doesn't support C23 yet
#ifndef __cplusplus
#include <assert.h>
#else
#define _Static_assert static_assert
#endif

#include "defs.h"

#define STACK(name, type, size)                                   \
    _Static_assert((sizeof(type) * size) > 0,                     \
                   "Stack size must be nonzero @ ");              \
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

#ifdef __cplusplus
}
#endif

#endif
