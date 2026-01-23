#ifndef _STATES_H_
#define _STATES_H_

#include <stdint.h>
#include "fsm.h"

EXPORTSTATE(game, splash); // boot splash screen
EXPORTSTATE(game, menu); // main menu
EXPORTSTATE(game, word_select); // team selects words
EXPORTSTATE(game, turn); // start of turn 
EXPORTSTATE(game, pass); // pass the device screen
EXPORTSTATE(game, victory); // game over
EXPORTSTATE(game, load); // read flash, enter pass state
EXPORTSTATE(game, settings); // Global settings (volume, etc)
EXPORTSTATE(game, team_select); // team select screen

// Clears a state without processing any enter/exit
void clear_state(state_t* next_state);
// Transitions normally to next state
void set_state(state_t* next_state);
// Runs the update routine 
void update_state(uint32_t delta);
// Get microseconds since last update
uint32_t get_delta();

#endif // _STATES_H_