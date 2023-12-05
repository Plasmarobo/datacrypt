#ifndef FSM_H
#define FSM_H

#include <stddef.h>
#include <stdint.h>

#include "defs.h"

typedef void (*state_func_t)(void *);

typedef struct state_t;

typedef struct {
    state_func_t enter;
    state_func_t update;
    state_func_t exit;
} state_t;

typedef struct {
    state_t *current;
} state_machine_t;

// clang-format off

// Starts an FSM
#define FSM(fsm) state_machine_t fsm = {NULL}

// Defines an enter state
#define STATE_ENTER(name) void CONCAT(name,_fn_enter)(void *ctx)

// Defines an exit state
#define STATE_EXIT(name) void CONCAT(name,_fn_exit)(void *ctx)

// Defines an update state
#define STATE_UPDATE(name) void CONCAT(name,_fn_update)(void *ctx)

// Constructs a state, populates function pointers
// with enter, exit, and update where they exist
// State can be referenced with fsm_name
#define REGISTER_STATE(fsm,name)                                   \
__attribute__((weak)) void CONCAT(name,_fn_enter)(void *ctx){};                   \
__attribute__((weak)) void CONCAT(name,_fn_exit)(void* ctx){};                    \
__attribute__((weak)) void CONCAT(name,_fn_update)(void* ctx){};                  \
state_t CONCAT(fsm,name) = {                                       \
    &CONCAT(name,_fn_enter),                                       \
    &CONCAT(name,_fn_exit),                                        \
    &CONCAT(name,_fn_update),                                      \
}                                                                 

#define FSM_SET_STATE(fsm, name) \
impl_fsm_set_state(&fsm, &CONCAT(fsm,name))

// clang-format on

/* *** Example ***
FSM(EXAMPLEFSM);
STATE_ENTER(IDLE)
{
    return 0;
};
STATE_EXIT(IDLE)
{
    return 0;
};
STATE_UPDATE(IDLE)
{
    return 0;
};
REGISTER_STATE(EXAMPLEFSM,IDLE);
// EXAMPLEFSM_IDLE should be 0 here
// We could set the state with fsm_set_state(EXAMPLEFSM, IDLE);
*/

// Set the state of an fsm, runs enter, exit then sets the current state

// Generic implementation
void impl_fsm_set_state(state_machine_t *fsm, state_t *new_state);

// Get the current state of an fsm
#define FSM_GET_STATE(fsm) (fsm).current

// Run the current state update routine
void fsm_update(state_machine_t *fsm);

#endif  // FSM_H
