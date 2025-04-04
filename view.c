// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <wchar.h>
#include <locale.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "constants.h"
#include "shm_ADT.h"


#define CLEAR_SCREEN printf("\033[2J\033[H")

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[41m"
#define COLOR_GREEN   "\x1b[42m"
#define COLOR_YELLOW  "\x1b[43m"
#define COLOR_BLUE    "\x1b[44m"
#define COLOR_MAGENTA "\x1b[45m"
#define COLOR_CYAN    "\x1b[46m"
#define COLOR_BROWN   "\x1b[48;5;130m"
#define COLOR_WHITE   "\x1b[47m"
#define COLOR_PURPLE  "\x1b[105m" 
#define COLOR_ORANGE  "\x1b[48;5;208m" 

#define BRIGHT_RED     "\x1b[101m"
#define BRIGHT_GREEN   "\x1b[102m"
#define BRIGHT_YELLOW  "\x1b[103m"
#define BRIGHT_BLUE    "\x1b[104m"
#define BRIGHT_MAGENTA "\x1b[105m" 
#define BRIGHT_CYAN    "\x1b[106m" 
#define BRIGHT_BROWN   "\x1b[48;5;173m"
#define BRIGHT_PURPLE  "\x1b[48;5;213m" 
#define BRIGHT_ORANGE  "\x1b[48;5;214m" 

#define SCORE_COLOR   "\x1b[30;47m"  

const char* player_colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_BROWN,
    COLOR_PURPLE,
    COLOR_ORANGE,
    COLOR_RED,   
    COLOR_GREEN
};

const char* bright_player_colors[] = {
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_BROWN,
    BRIGHT_PURPLE,
    BRIGHT_ORANGE
};

#define TEXT_RED     "\x1b[31m"
#define TEXT_GREEN   "\x1b[32m"
#define TEXT_YELLOW  "\x1b[33m"
#define TEXT_BLUE    "\x1b[34m"
#define TEXT_MAGENTA "\x1b[35m"
#define TEXT_CYAN    "\x1b[36m"
#define TEXT_BROWN   "\x1b[38;5;130m"
#define TEXT_WHITE   "\x1b[37m"
#define TEXT_PURPLE  "\x1b[95m"
#define TEXT_ORANGE  "\x1b[38;5;208m"

const char* text_colors[] = {
    TEXT_RED,
    TEXT_GREEN,
    TEXT_YELLOW,
    TEXT_BLUE,
    TEXT_MAGENTA,
    TEXT_CYAN,
    TEXT_BROWN,
    TEXT_PURPLE,
    TEXT_ORANGE
};

void print_board(game_t *game_state) {
    CLEAR_SCREEN;
    
    printf("┌");
    for(int x = 0; x < game_state->board_width * 3; x++) {
        printf("─");
    }
    printf("┐\n");

    for(int y = 0; y < game_state->board_height; y++) {
        printf("│"); 
        for(int x = 0; x < game_state->board_width; x++) {
            int cell = game_state->board_p[y * game_state->board_width + x];
            if(cell > 0) {
                printf("%s%2d %s", SCORE_COLOR, cell, COLOR_RESET);
            } else {
                int player_idx = -1 * cell; 
                int is_head = 0;
                
                player_t player = game_state->players[player_idx];
                is_head = (x == player.x_coord && y == player.y_coord);
                
                printf("%s   %s", is_head ? bright_player_colors[player_idx] : player_colors[player_idx], COLOR_RESET);
            }
        }
        printf("│\n"); 
    }

    printf("└");
    for(int x = 0; x < game_state->board_width * 3; x++) {
        printf("─");
    }
    printf("┘\n");

    printf("\nJugadores:\n");
    for(unsigned int i = 0; i < game_state->player_number; i++) {
        player_t player = game_state->players[i];
        if(player.pid != 0) {  
            printf("%sJugador %u%s (%s): Score=%u, Pos=(%d,%d)%s\n", 
                text_colors[i],
                i, 
                COLOR_RESET,
                player.name, 
                player.score, 
                player.x_coord, 
                player.y_coord,
                player.is_blocked ? " [BLOQUEADO]" : "");
        }
    }
    fflush(stdout);
}

int main(int argc, const char *argv[] ){
    if ( argc != 3 ) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]); 

    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), O_RDONLY);

    while (!game_state->has_finished) {
        sem_wait(&sync->print_needed);
        print_board(game_state);
        sem_post(&sync->print_done);
    }

    unmap_shm(sync, sizeof(game_sync));
    unmap_shm(game_state, sizeof(game_t));

    return EXIT_SUCCESS;
}