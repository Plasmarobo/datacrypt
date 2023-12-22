#include "fsm.h"

void fsm_set_state(state_t* current, state_t* new_state) {
    if (NULL != current && NULL != current->exit) {
        current->exit();
    }

    if ((NULL != new_state) && (NULL != new_state->enter)) {
        new_state->enter();
    }
}

void fsm_update(state_t* current) {
    if (NULL != current && NULL != current->update) {
        current->update();
    }
}
