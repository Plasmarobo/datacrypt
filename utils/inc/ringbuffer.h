#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "defs.h"

// Defines a static ringbuffer
// clang-format off
#define RINGBUFFER(name,type,size) \
static uint8_t CONCAT(name,_rb_buffer)[sizeof(type)*size]; \
static ringbuffer_t name = { \
false, \
CONCAT(name,_rb_buffer), \
CONCAT(name,_rb_buffer), \
CONCAT(name,_rb_buffer), \
size, \
sizeof(type) \
}
// clang-format on
// Ex: RINGBUFFER(serialrb, serial_message_t, 32);

typedef struct {
    bool full;
    uint8_t* head;
    uint8_t* tail;
    uint8_t* root;
    uint8_t capacity;
    uint8_t element_size;
} ringbuffer_t;

bool ringbuffer_full(ringbuffer_t* rb);
bool ringbuffer_empty(ringbuffer_t* rb);
bool ringbuffer_push(ringbuffer_t* rb, void* item);
bool ringbuffer_pop(ringbuffer_t* rb, void* item);
bool ringbuffer_peek(ringbuffer_t* rb, void* item);

#ifdef __cplusplus
}
#endif

#endif  // RINGBUFFER_H
