#ifndef FSM_H
#define FSM_H

#include <stddef.h>
#include <stdint.h>

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*state_func_t)(void);

typedef struct {
    state_func_t enter;
    state_func_t update;
    state_func_t exit;
} state_t;

// clang-format off

#define STATE_ENTER(fsm,name) static void TRICAT(fsm,name,_enter)(void)
#define STATE_UPDATE(fsm,name) static void TRICAT(fsm,name,_update)(void)
#define STATE_EXIT(fsm,name) static void TRICAT(fsm,name,_exit)(void)

#define STATE_1_ARG(fsm,name,enter) \
static state_t CONCAT(fsm,name) = { \
    &TRICAT(fsm,name,_enter), \
    NULL, \
    NULL, \
}

#define STATE_2_ARG(fsm,name,enter,update) \
static state_t CONCAT(fsm,name) = { \
    &TRICAT(fsm,name,_enter), \
    &TRICAT(fsm,name,_update), \
    NULL, \
}

#define STATE_3_ARG(fsm,name,enter,update,exit) \
static state_t CONCAT(fsm,name) = { \
    &TRICAT(fsm,name,_enter), \
    &TRICAT(fsm,name,_update), \
    &TRICAT(fsm,name,_exit), \
}

#define STATE_EXTRACT_ARG(a1, a2, a3, a4, ...) a4
#define STATE_FLEX(...) \
STATE_EXTRACT_ARG(__VA_ARGS__, STATE_3_ARG, STATE_2_ARG, STATE_1_ARG, )

#define STATE(fsm, name, ...) STATE_FLEX(__VA_ARGS__)(fsm, name, __VA_ARGS__)
#define DECLARESTATE(fsm,name) static state_t CONCAT(fsm,name);

#define STATENAME(fsm,name) CONCAT(fsm,name)
#define STATEREF(fsm,name) (void*)&STATENAME(fsm,name)

// Stack-driven FSM pattern
#define STATESTACK(fsm,depth) STACK(CONCAT(fsm,_stack), state_t*,depth)
#define STACKNAME(fsm) CONCAT(fsm,_stack)
#define PUSHSTATE(fsm,name) { state_t* t_state = STATEREF(fsm,name); stack_push(&STACKNAME(fsm),&t_state); }
#define QUEUESTATE(fsm,name) {state_t* s = STATEREF(fsm,name); ringbuffer_push(&state_queue, &s);}
// clang-format on

/* *** Example ***
STATE_ENTER(f,IDLE)
{
    return 0;
};
STATE_EXIT(f,IDLE)
{
    return 0;
};
STATE_UPDATE(f,IDLE)
{
    return 0;
};
STATE(f,IDLE,enter,update,exit);
*/
// Generic implementation
void fsm_set_state(state_t *current, state_t *new);

// Run the current state update routine
void fsm_update(state_t *current);

#ifdef __cplusplus
}
#endif

#endif  // FSM_H
