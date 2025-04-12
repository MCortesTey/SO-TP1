#ifndef GAME_H
#define GAME_H

#include "shared_memory.h"
#include "constants.h"

// GAME CONSTANTS

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL
#define DELAY_MULTIPLIER 1000

#define MIN_PLAYER_NUMBER 1
#define MAX_PLAYER_NUMBER 9
#define MAX_NAME_LEN 16

// fns for gama inicialization

/**
 * @brief creates struct for game sems
 * @return pointer to game_sync struct in shm
 */
game_sync * create_game_sync();

/**
 * @brief creates struct for game state
 * @param width width of the board
 * @param height height of the board
 * @param n_players number of players
 * @param players arr of player names of length n_players
 * @param seed seed for rand number generation
 * @return pointer to game_t struct in shm
 */
game_t * create_game(unsigned short width, unsigned short height, unsigned int n_players, char * players[], unsigned int seed);

// fns for game development

typedef struct {
    int player_id;
    enum MOVEMENTS move;
} player_movement;

player_movement get_move(game_t * game, int pipes[MAX_PLAYER_NUMBER][2], int player_count, unsigned int timeout);
bool process_move(game_t * game, player_movement player_mov, time_t* last_valid_mov_time);

#endif // GAME_H