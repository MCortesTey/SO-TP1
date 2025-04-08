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
#include "sems.h"



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

#ifdef KILLER
static inline unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    // search for nearest player
    int nearest_player = -1;
    int min_distance = width * height; // max possible distance

    for (int j = 0; j < width * height; j++) {
        if (board[j] < 0) { // player found
            int player_x = j % width;
            int player_y = j / width;
            int distance = abs(player_x - x_pos) + abs(player_y - y_pos);
            if (distance < min_distance) {
                min_distance = distance;
                nearest_player = j;
            }
        }
    }

    int mov_dir;

    if (nearest_player != -1){
        if(min_distance > 1){
            int player_x = nearest_player % width;
            int player_y = nearest_player / width;
            int difx = player_x - x_pos;
            int dify = player_y - y_pos;
            // move towards the nearest player
            if (abs(difx) > abs(dify)) {
                mov_dir = (difx > 0) ? RIGHT : LEFT;
            } else if (abs(difx) < abs(dify)) {
                mov_dir = (dify > 0) ? DOWN : UP;
            } else {
                // prioritize diagonal moves if distances are equal
                if (difx > 0 && dify > 0) {
                    mov_dir = DOWN_RIGHT;
                } else if (difx > 0 && dify < 0) {
                    mov_dir = UP_RIGHT;
                } else if (difx < 0 && dify > 0) {
                    mov_dir = DOWN_LEFT;
                } else {
                    mov_dir = UP_LEFT;
                }
            }
        } else {
            // search for adjacent player
            int player_to_kill = -1;
            for(mov_dir = UP; mov_dir <= UP_LEFT && player_to_kill == -1; mov_dir++){
                int x = x_pos + dx[mov_dir];
                int y = y_pos + dy[mov_dir];
                if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] < 0) {
                    player_to_kill = mov_dir;
                }
            }
        }
    } else {
        // no players found --> first valid move
        for(int i = UP; i <= UP_LEFT; i++){
            int x = x_pos + dx[i];
            int y = y_pos + dy[i];
            if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
                return i;  // return first valid mov
            }
        }
        return NONE;
    }
        
        
    int j=0;
    int movements[9];
    switch(mov_dir){
        case UP:
        memcpy(movements, (int[]){UP, UP_LEFT, UP_RIGHT, RIGHT, LEFT, DOWN_RIGHT, DOWN_LEFT, DOWN, NONE}, sizeof(movements));
        break;
        case UP_RIGHT:
        memcpy(movements, (int[]){UP_RIGHT, UP, RIGHT, DOWN_RIGHT, UP_LEFT, LEFT, DOWN, DOWN_LEFT, NONE}, sizeof(movements));
        break;
        case RIGHT:
        memcpy(movements, (int[]){RIGHT, UP_RIGHT, DOWN_RIGHT, DOWN, UP, DOWN_LEFT, UP_LEFT, LEFT, NONE}, sizeof(movements));
        break;
        case DOWN_RIGHT:
        memcpy(movements, (int[]){DOWN_RIGHT, RIGHT, DOWN, DOWN_LEFT, UP_RIGHT, UP, LEFT, UP_LEFT, NONE}, sizeof(movements));
        break;
        case DOWN:
        memcpy(movements, (int[]){DOWN, DOWN_RIGHT, DOWN_LEFT, RIGHT, LEFT, UP_LEFT, UP_RIGHT, UP, NONE}, sizeof(movements));
        break;
        case DOWN_LEFT:
        memcpy(movements, (int[]){DOWN_LEFT, DOWN, LEFT, UP_LEFT, DOWN_RIGHT, UP, RIGHT, UP_RIGHT, NONE}, sizeof(movements));
        break;
        case LEFT:
        memcpy(movements, (int[]){LEFT, DOWN_LEFT, UP_LEFT, UP, DOWN, UP_RIGHT, DOWN_RIGHT, RIGHT, NONE}, sizeof(movements));
        break;
        case UP_LEFT:
        memcpy(movements, (int[]){UP_LEFT, LEFT, UP, UP_RIGHT, DOWN_LEFT, DOWN, RIGHT, DOWN_RIGHT, NONE}, sizeof(movements));
        break;
        default:
        return NONE;
    }
    while (j < 8) {
        int nx = x_pos + dx[movements[j]];
        int ny = y_pos + dy[movements[j]];
        if (nx >= 0 && nx < width && ny >= 0 && ny < height && board[ny * width + nx] > 0) {
            break; // es un movimiento válido
        }
        j++;
    }
    return movements[j];

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

    int height = atoi(argv[2]);
    IF_EXIT(height <= 0, "alto inválido")

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

    int my_board[height * width];
    IF_EXIT_NULL(my_board, "Error al reservar memoria para el tablero")

    unsigned char move = NONE;

    while (!game_state->has_finished && !game_state->players[player_id].is_blocked) {
        int my_x, my_y;

        wait_shared_sem(&sync->master_access_mutex);
        //sem_wait(&sync->master_access_mutex); 

        post_shared_sem(&sync->master_access_mutex);
        //sem_post(&sync->master_access_mutex);
        
        wait_shared_sem(&sync->reader_count_mutex);
        //sem_wait(&sync->reader_count_mutex); 
        sync->readers_count++; 
        if (sync->readers_count == 1) { 
            wait_shared_sem(&sync->game_state_mutex);
            //sem_wait(&sync->game_state_mutex); 
        }
        
        
        post_shared_sem(&sync->reader_count_mutex);
        //sem_post(&sync->reader_count_mutex);

        memcpy(my_board, game_state->board_p, sizeof(int) * width * height); 
        my_x = game_state->players[player_id].x_coord;
        my_y = game_state->players[player_id].y_coord;

        wait_shared_sem(&sync->reader_count_mutex);
        //sem_wait(&sync->reader_count_mutex);
        if (sync->readers_count-- == 1) {
            post_shared_sem(&sync->game_state_mutex);
            //sem_post(&sync->game_state_mutex);
        }
        post_shared_sem(&sync->reader_count_mutex);
        //sem_post(&sync->reader_count_mutex);

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
    //free(my_board);
    return EXIT_SUCCESS;
    
}