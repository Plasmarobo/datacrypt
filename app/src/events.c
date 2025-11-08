#include "events.h"

#include <stdint.h>
#include <stddef.h>

#include "gpio.h"

static event_handler_t handlers[EVENT_MAX];

static void dispatch_lock0_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_LOCK0, value);
    }
}
static void dispatch_lock1_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_LOCK1, value);
    }
}
static void dispatch_lock2_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_LOCK2, value);
    }
}
static void dispatch_lock3_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_LOCK3, value);
    }
}
static void dispatch_left_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_LEFT, value);
    }
}
static void dispatch_accept_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_ACCEPT, value);
    }
}
static void dispatch_cancel_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_CANCEL, value);
    }
}
static void dispatch_right_sw(int32_t value)
{
    static int32_t previous_value = 0;
    int32_t change = value ^ previous_value;
    previous_value = value;
    if (change)
    {
        event_dispatch(EVENT_RIGHT, value);
    }
}

void event_clear_all()
{
    for (uint8_t i = 0; i < EVENT_MAX; ++i)
    {
        handlers[i] = NULL;
    }
};

void event_add_handler(event_t ev, event_handler_t handler)
{
    if (ev < EVENT_MAX)
    {
        handlers[ev] = handler;
    }
}

void event_dispatch(event_t ev, int32_t state)
{
    if (ev < EVENT_MAX && handlers[ev] != NULL)
    {
        (*handlers[ev])(state);
    }
}

void events_init()
{
    event_clear_all();
    // Link up appropriate GPIO handlers
    // A better way to do this would be to get a bitfield from the gpios
    // and scan through it, allocating one function for everything instead of a function pointer
    // per signal
    gpio_set_callback(LOCK0_TGL, dispatch_lock0_sw);
    gpio_set_callback(LOCK1_TGL, dispatch_lock1_sw);
    gpio_set_callback(LOCK2_TGL, dispatch_lock2_sw);
    gpio_set_callback(LOCK3_TGL, dispatch_lock3_sw);
    gpio_set_callback(LEFT_SW, dispatch_left_sw);
    gpio_set_callback(ACCEPT_SW, dispatch_accept_sw);
    gpio_set_callback(CANCEL_SW, dispatch_cancel_sw);
    gpio_set_callback(RIGHT_SW, dispatch_right_sw);
}

void events_update()
{
    // Noop currently, consider deferring execution
}