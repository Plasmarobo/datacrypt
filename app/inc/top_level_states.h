#ifndef TOP_LEVEL_STATES_H
#define TOP_LEVEL_STATES_H

#include "fsm.h"

EXPORTSTATE(app_fsm, game);
EXPORTSTATE(app_fsm, mode_select);
EXPORTSTATE(app_fsm, test);

void app_init();

#endif  // TOP_LEVEL_STATES_H
