#include "fsm.h"

void impl_fsm_set_state(state_machine_t* fsm, state_t* new_state) {
    if (NULL != fsm) {
        if ((NULL != fsm->current) && (NULL != fsm->current->exit)) {
            fsm->current->exit(NULL);
        }

        fsm->current = new_state;

        if ((NULL != fsm->current) && (NULL != fsm->current->enter)) {
            fsm->current->enter(NULL);
        }
    }
}

void fsm_update(state_machine_t* fsm) {
    if ((NULL != fsm) && (NULL != fsm->current) &&
        (NULL != fsm->current->update)) {
        fsm->current->update(NULL);
    }
}
