// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>           
#include <sys/mman.h>       
#include <sys/stat.h>       
#include <semaphore.h>       
#include <unistd.h>         
#include <time.h>
#include "constants.h"
#include "macros.h"
#include "shm_ADT.h"



static inline unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos);

#ifdef FIRST_POSSIBLE

static inline unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){ 
    for(int i = UP; i <= UP_LEFT; i++){
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
            return i;  // return first valid mov
        }
    }
    return NONE;
}
#endif

#ifdef BEST_SCORE
static inline unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    int best_move = NONE;
    int max_score = -1;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (board[y * width + x] > max_score) {
                max_score = board[y * width + x];
                best_move = i;
            }
        }
    }
    return best_move; // return direction with best score
}
#endif 

#ifdef RANDOM
static inline unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    int valid_moves[8];
    int num_valid_moves = 0;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i], y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
            valid_moves[num_valid_moves++] = i;
        }
    }

    if (num_valid_moves > 0) {
        return valid_moves[rand() % num_valid_moves]; // return random move from possibles
    }
    return NONE;
}
#endif

#ifdef CLOCK
static inline unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    static int i = UP;
    int original_i = i;

    
    do {
        i = i % 8;  
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        
        if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
            int move = i;
            i = (i + 1) % 8;  
            return move;
        }
        
        i++;
    } while (i != original_i); // check for all directions
    
    return NONE;
}
#endif

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    size_t mov_count = 0;

    int width = atoi(argv[1]);
    IF_EXIT(width <= 0, "ancho inválido")
    fprintf(stderr,"width: %d\n", width);

    int height = atoi(argv[2]);
    IF_EXIT(height <= 0, "alto inválido")
    fprintf(stderr,"height: %d\n", height);

    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), O_RDONLY);
    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);

    unsigned int player_id = 0; 
    pid_t my_pid = getpid();

    srand(my_pid);

    for (player_id = 0; player_id < game_state->player_number; player_id++){
        if(game_state->players[player_id].pid == my_pid){
            printf("Soy el jugador %d\n", player_id);
            break;
        }
    }
    
    IF_EXIT(player_id == game_state->player_number, "player could not identify itself")

    int * my_board = malloc(sizeof(int) * width * height);
    IF_EXIT_NULL(my_board, "Error al reservar memoria para el tablero")

    unsigned char move = NONE;

    while (!game_state->has_finished && !game_state->players[player_id].is_blocked) {
        int my_x, my_y;
        sem_wait(&sync->master_access_mutex); 
        sem_post(&sync->master_access_mutex);
        
        sem_wait(&sync->reader_count_mutex); 
        sync->readers_count++; 
        if (sync->readers_count == 1) { 
            sem_wait(&sync->game_state_mutex); 
        }
        
        sem_post(&sync->reader_count_mutex);

        memcpy(my_board, game_state->board_p, sizeof(int) * width * height); 
        my_x = game_state->players[player_id].x_coord;
        my_y = game_state->players[player_id].y_coord;

        sem_wait(&sync->reader_count_mutex);
        if (sync->readers_count-- == 1) {
            sem_post(&sync->game_state_mutex);
        }
        sem_post(&sync->reader_count_mutex);

        if(game_state->has_finished || game_state->players[player_id].is_blocked){
            break;
        }

        move = generate_move(width,height,my_board,my_x,my_y);

        if(move == NONE || game_state->has_finished || game_state->players[player_id].is_blocked){ 
            break;
        }

        IF_EXIT(write(STDOUT_FILENO, &move, sizeof(move)) == -1, "Error al escribir movimiento en stdout")
        mov_count++;
        

        while(!game_state->has_finished && !game_state->players[player_id].is_blocked && 
            game_state->players[player_id].valid_mov_request + game_state->players[player_id].invalid_mov_requests != mov_count);
    }
    
    unmap_shm(game_state, sizeof(game_t) + sizeof(int)*(width*height));
    unmap_shm(sync, sizeof(game_sync));
    free(my_board);
    return EXIT_SUCCESS;
    
}