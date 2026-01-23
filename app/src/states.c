#include "states.h"

// Current state of the game
static state_t* _current_state;
uint32_t _state_delta;

void clear_state(state_t* next_state)
{
    _current_state = next_state;
}

void set_state(state_t* next_state)
{
    fsm_set_state(&_current_state, next_state);
}

void update_state(uint32_t delta)
{
    _state_delta = delta;
    fsm_update(_current_state);
}

uint32_t get_delta()
{
    return _state_delta;
}
