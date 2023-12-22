#include "stack.h"

#include <stddef.h>
#include <string.h>

static bool stack_notfull(stack_t* st) {
    if (NULL == st || NULL == st->root) {
        // A non-existant stack is a full stack
        return false;
    }
    if (NULL == st->head) {
        // A null head is an empty stack
        return true;
    }
    return (((st->head - st->root) / st->element_size) + 1) < st->capacity;
}

bool stack_full(stack_t* st) { return !stack_notfull(st); }

bool stack_empty(stack_t* st) {
    if (NULL == st) {
        // A non-existant stack is an empty stack
        return true;
    }
    return (NULL == st->head);
}

bool stack_push(stack_t* st, void* item) {
    if (NULL == st || NULL == item) {
        return false;
    }
    if (stack_notfull(st)) {
        if (NULL == st->head) {
            st->head = st->root;
        } else {
            st->head += st->element_size;
        }
        memcpy(st->head, item, st->element_size);
        return true;
    }
    return false;
}

bool stack_pop(stack_t* st, void* item) {
    if (NULL == st) {
        return false;
    }
    if (NULL != st->head) {
        if (NULL != item) {
            memcpy(item, st->head, st->element_size);
        }
        if (st->head == st->root) {
            st->head = NULL;
        } else {
            st->head -= st->element_size;
        }
        return true;
    }
    return false;
}

bool stack_peek(stack_t* st, void* item) {
    if (NULL != st && NULL != st->head && NULL != item) {
        memcpy(item, st->head, st->element_size);
        return true;
    }
    return false;
}
