#ifndef APP_GAME_H
#define APP_GAME_H

#include <stddef.h>
#include <stdint.h>

#include "words.h"
#include "defs.h"

typedef enum
{
    TEAM_A = 0,
    TEAM_B,
    TEAM_COUNT,
} team_name_t;

#define WORDLIST_LENGTH (4)
#define SEQUENCE_LENGTH (3)

typedef struct {
    word_t words[WORDLIST_LENGTH];
    uint8_t sequence[SEQUENCE_LENGTH];
} wordlist_t;

typedef struct
{
    uint8_t intercepts;
    uint8_t failures;
    uint8_t successes;
    wordlist_t words;
} team_t;

// Word alignment for flash
typedef struct __attribute__((packed, aligned(8)))
{
    struct
    { // bit flags
        uint8_t free : 1;
        uint8_t valid : 1; // write-down in flash, when offset 0 is written down, erase block
        uint8_t current_team : 1;
        uint8_t current_round : 6;
    };
    team_t teams[TEAM_COUNT];
} game_state_t;

void game_init(int32_t status);
void game_update(int32_t status);

team_t *get_current_team();
team_t *get_opposing_team();
void set_current_team(team_name_t name);
void swap_current_team();

void game_new();
game_state_t *game();
void game_save(callback_t oncomplete);
void game_load(callback_t oncomplete);

void disp_print(uint8_t display, uint8_t x, uint8_t y, const char *text);
void disp_printf(uint8_t display, uint8_t x, uint8_t y, const char *fmt, ...);
void disp_draw(uint8_t display, uint8_t x, uint8_t y, const buffer_t img);
void disp_clear_all();

#endif  // APP_GAME_H
