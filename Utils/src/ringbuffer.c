#include "ringbuffer.h"

#include <string.h>

bool ringbuffer_full(ringbuffer_t* rb)
{
    return (rb->head == rb->tail) && rb->full;
}

bool ringbuffer_empty(ringbuffer_t* rb)
{
    return (rb->head == rb->tail) && !rb->full;
}

bool ringbuffer_push(ringbuffer_t* rb, void* item)
{
    if (!ringbuffer_full(rb))
    {
        memcpy(rb->head, item, rb->element_size);
        void* new_ptr = rb->head + rb->element_size;
        if (new_ptr > (rb->root + (rb->capacity * rb->element_size)))
        {
            rb->head = rb->root;
        }
        else
        {
            rb->head = new_ptr;
        }
        if (rb->head == rb->tail)
        {
            rb->full = true;
        }
        return true;
    }
    return false;
}

bool ringbuffer_pop(ringbuffer_t* rb, void* item)
{
    if (!ringbuffer_empty(rb))
    {
        rb->full = false;
        memcpy(item, rb->tail, rb->element_size);
        void* new_ptr = rb->tail + rb->element_size;
        if (new_ptr > (rb->root + (rb->capacity * rb->element_size)))
        {
            rb->tail = rb->root;
        }
        else
        {
            rb->tail = new_ptr;
        }
        return true;
    }
    return false;
}