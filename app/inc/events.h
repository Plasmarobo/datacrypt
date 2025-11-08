#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>

typedef enum {
    EVENT_NOOP = 0,
    EVENT_SWITCH,
    EVENT_BUTTON,
    EVENT_TIMER,
    EVENT_LOCK0,
    EVENT_LOCK1,
    EVENT_LOCK2,
    EVENT_LOCK3,
    EVENT_LEFT,
    EVENT_ACCEPT,
    EVENT_CANCEL,
    EVENT_RIGHT,
    EVENT_MAX
} event_t;

typedef void (*event_handler_t)(int32_t state);

// Shortcuts
#define ON_SWITCH(handler) event_add_handler(EVENT_SWITCH, handler)
#define ON_BUTTON(handler) event_add_handler(EVENT_BUTTON, handler)

#define ON_LOCK0(handler) event_add_handler(EVENT_LOCK0, handler)
#define ON_LOCK1(handler) event_add_handler(EVENT_LOCK1, handler)
#define ON_LOCK2(handler) event_add_handler(EVENT_LOCK2, handler)
#define ON_LOCK3(handler) event_add_handler(EVENT_LOCK3, handler)

#define ON_LEFT(handler) event_add_handler(EVENT_LEFT, handler)
#define ON_ACCEPT(handler) event_add_handler(EVENT_ACCEPT, handler)
#define ON_CANCEL(handler) event_add_handler(EVENT_CANCEL, handler)
#define ON_RIGHT(handler) event_add_handler(EVENT_RIGHT, handler)

// Events help to handle callbacks and logic in a more intuitive way: register and clear handlers
void event_clear_all(); // Unregister all event handlers
void event_add_handler(event_t ev, event_handler_t handler);
void event_dispatch(event_t ev, int32_t state);

void events_init();
void events_update();

#endif // EVENTS_H