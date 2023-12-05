#include "stack.h"

static bool stack_notfull(stack_t* st) {
    if (NULL == st) {
        return false;
    }
    if (NULL == st->head) {
        return false;
    }
    return ((st->head - st->root) / st->element_size) < st->capacity;
}

bool stack_full(stack_t* st) { return !stack_notfull(); }

bool stack_empty(stack_t* st) {
    if (NULL == st) {
        return false;
    }
    return (NULL == st->head);
}

bool stack_push(stack_t* st, void* item) {
    if (NULL == st || NULL == item) {
        return false;
    }
    if (stack_notfull()) {
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
    if (NULL == st || NULL == item) {
        return false;
    }
    if (NULL != st->head) {
        memcpy(item, st->head, st->element_size);
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
    if (NULL == st || NULL == item) {
        return false;
    }
    if (NULL != st->head) {
        memcpy(item, st->head, st->element_size);
        return true;
    }
    return false;
}
