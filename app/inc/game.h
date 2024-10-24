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
    // Main Menu:
    NEW_GAME,
    CONTINUE_GAME,
    // New Game
    WORD_SELECT_A,     // Team A - Lock in 4 words, flip PASS toggle
    PASS_DEVICE_A,      
    WORD_SELECT_B,     // Team B - Lock in 4 words, flip PASS toggle
    PASS_DEVICE_B,
    SEQUENCE_PHASE,  // Unhide, Show the sequence
    START_PHASE,     // Start the timer
    TIME_PHASE,      // Timer set, end on timer reset or timer expire
    SCORE_PHASE,     // Enter Miscommunicate, Intercept, Success
    CONFIRM_SCORE_PHASE, // Confirm user input, then PASS
    GAME_CHECK_PHASE, // Check if the game has been Won or Lost
    GAME_OVER,       // The win state - display stats, press continue
} game_state_t;

void game_init(int32_t status);
void game_update(int32_t status);

#endif  // APP_GAME_H
