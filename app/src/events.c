#include "events.h"

#include <stdint.h>
#include <stddef.h>

#include "gpio.h"

static event_handler_t handlers[EVENT_MAX];

const timespan_t DEBOUNCE_TIME_MS = 50;

static gpio_t *current_debounce = NULL;

static void debounce_event(int32_t status)
{
    UNUSED(status);
    if (current_debounce != NULL)
    {
        event_t ev = EVENT_NOOP;
        if (current_debounce == &LOCK0_TGL)
        {
            ev = EVENT_LOCK0;
        }
        else if (current_debounce == &LOCK1_TGL)
        {
            ev = EVENT_LOCK1;
        }
        else if (current_debounce == &LOCK2_TGL)
        {
            ev = EVENT_LOCK2;
        }
        else if (current_debounce == &LOCK3_TGL)
        {
            ev = EVENT_LOCK3;
        }
        else if (current_debounce == &LEFT_SW)
        {
            ev = EVENT_LEFT;
        }
        else if (current_debounce == &ACCEPT_SW)
        {
            ev = EVENT_ACCEPT;
        }
        else if (current_debounce == &CANCEL_SW)
        {
            ev = EVENT_CANCEL;
        }
        else if (current_debounce == &RIGHT_SW)
        {
            ev = EVENT_RIGHT;
        }
        event_dispatch(ev, current_debounce->value ? 1 : 0);
        current_debounce = NULL;
    }
}
static void debounce_handler(gpio_t *gpio, int32_t value)
{
    if (current_debounce == NULL)
    {
        current_debounce = gpio;
        task_delayed(debounce_event, MILLIS(DEBOUNCE_TIME_MS));
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
    gpio_set_callback(&LOCK0_TGL, debounce_handler);
    gpio_set_callback(&LOCK1_TGL, debounce_handler);
    gpio_set_callback(&LOCK2_TGL, debounce_handler);
    gpio_set_callback(&LOCK3_TGL, debounce_handler);
    gpio_set_callback(&LEFT_SW, debounce_handler);
    gpio_set_callback(&ACCEPT_SW, debounce_handler);
    gpio_set_callback(&CANCEL_SW, debounce_handler);
    gpio_set_callback(&RIGHT_SW, debounce_handler);
}

void events_update()
{
    // Noop currently, consider deferring execution
}