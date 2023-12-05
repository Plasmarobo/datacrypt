#include "game.h"

#include "fsm.h"

#define GAME_STACK_DEPTH (4)

STACK(game_state, state_t*, GAME_STACK_DEPTH);

STATE_ENTER(start){

};

STATE_ENTER(setup){

};

STATE_ENTER(generate_wordlist){

};

STATE_UPDATE(generate_wordlist){

};

STATE_ENTER(team_turn){

};

STATE_UPDATE(team_turn){

};

STATE_ENTER(swap_team){

};

REGISTER_STATE(game, start);
REGISTER_STATE(game, setup);
REGISTER_STATE(game, generate_wordlist);
REGISTER_STATE(game, team_turn);
REGISTER_STATE(game, swap_team);
