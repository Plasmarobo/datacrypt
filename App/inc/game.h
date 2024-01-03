#ifndef APP_GAME_H
#define APP_GAME_H

#include <stdint.h>

#include "words.h"

typedef enum {
    TEAM_A,
    TEAM_B,
} team_name_t;

#define SEQUENCE_LENGTH (3)

typedef struct {
    word_t a;
    word_t b;
    word_t c;
    word_t d;
    uint8_t sequence[SEQUENCE_LENGTH];
} wordlist_t;

typedef struct {
    team_name_t name;
    uint8_t intercepts;
    uint8_t failures;
    uint8_t successes;
    wordlist_t words;
} team_t;

typedef enum {
    SPLASH,
    MAIN_MENU,
    NEW_GAME,
    WORD_SELECT,     // Team A & B
    PASS_DEVICE,     // Hide toggle should be set
    SEQUENCE_PHASE,  // Show the sequence
    START_PHASE,     // Hide toggle unset
    TIME_PHASE,      // Timer set, end on timer reset or timer expire
    SCORE_PHASE,     // Enter Fail/Intercept, Success, then press end turn
    GAME_OVER,       // The win state
} game_state_t;

void game_init(int32_t status);
void game_update(int32_t status);

#endif  // APP_GAME_H
