// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "shm_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>           // Para O_* constantes
#include <sys/mman.h>       // Para shm_open y mmap
#include <sys/stat.h>       // Para mode constants
#include <semaphore.h>       // Para semaforos POSIX
#include <unistd.h>         // Para sleep
#include <time.h>
#include "shared_memory.h"
#include "constants.h"
#include "macros.h"

// Estructura para el estado del tablero

// Principal del jugador

enum MOVEMENTS {UP = 0, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT, NONE};

unsigned char generate_move(int width, int height, int board[], int x_pos, int y_pos);

const int dx[] = {0,   1, 1, 1, 0, -1, -1, -1};
const int dy[] = {-1, -1, 0, 1, 1,  1,   0, -1};

#ifdef FIRST_POSSIBLE

unsigned char generate_move(int width, int height, int board[], int x_pos, int y_pos){ 
    for(int i = UP; i <= UP_LEFT; i++){
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
            return i; // devuelve el movimiento
        }
    }
    return NONE; // si no hay movimientos válidos, devuelve NONE
}
#endif

#ifdef BEST_SCORE
unsigned char generate_move(int width, int height, int board[], int x_pos, int y_pos){
    int best_move = NONE;
    int max_score = -1;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i], y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (board[y * width + x] > max_score) {
                max_score = board[y * width + x];
                best_move = i;
            }
        }
    }
    return best_move;
}
#endif 

#ifdef RANDOM
unsigned char generate_move(int width, int height, int board[], int x_pos, int y_pos){
    int valid_moves[8];
    int num_valid_moves = 0;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i], y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
            valid_moves[num_valid_moves++] = i;
        }
    }

    if (num_valid_moves > 0) {
        return valid_moves[rand() % num_valid_moves]; // devuelve un movimiento aleatorio
    }
    return NONE; // si no hay movimientos válidos
}
#endif

#ifdef CLOCK
unsigned char generate_move(int width, int height, int board[], int x_pos, int y_pos){
    static int i = UP;
    int original_i = i;
    int x, y;
    
    do {
        i = i % 8;  // Mantener i entre 0 y 7
        x = x_pos + dx[i];
        y = y_pos + dy[i];
        
        if (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0) {
            int move = i;
            i = (i + 1) % 8;  // Preparar para la siguiente llamada
            return move;
        }
        
        i++;
    } while (i != original_i);  // Continuar hasta que hayamos verificado todas las direcciones
    
    return NONE;  // Solo si no encontramos ningún movimiento válido
}
#endif

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    

    //size_t mov_count = 0;

    int width = atoi(argv[1]);
    IF_EXIT(width <= 0, "ancho inválido")

    int height = atoi(argv[2]);
    IF_EXIT(height <= 0, "alto inválido")

    // Conectar a la memoria compartida del juego
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), O_RDONLY);

    // Conectar a la memoria compartida de sincronización
    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);

    srand(getpid()); // provisorio, después ver como tunearlo para que sea más random aún

    int player_id = 0; // el jugador va a identificarse a sí mismo!!
    pid_t my_pid = getpid();

    for (player_id = 0; player_id < game_state->player_number; player_id++){
        if(game_state->players[player_id].pid == my_pid){
            break;
        }
    }

    IF_EXIT(player_id == game_state->player_number, "player could not identify itself")
    //bool cut = false;

    int my_board[width*height];
    int my_x, my_y;

    unsigned char move = NONE;
    setvbuf(stdout, NULL, _IONBF, sizeof(move)); // Desactivar el buffering de stdout

    while (!game_state->has_finished && !game_state->players[player_id].is_blocked) {
        sem_wait(&sync->master_access_mutex); 
        sem_post(&sync->master_access_mutex);
        
        // lee estado de juego
        sem_wait(&sync->reader_count_mutex); // bloquea el reader count
        sync->readers_count++;  // incrementa el reader count
        if (sync->readers_count == 1) { 
            sem_wait(&sync->game_state_mutex); // si hay un solo lector bloquea el game state
        }
        
        sem_post(&sync->reader_count_mutex);

        memcpy(my_board, game_state->board_p, sizeof(int)*width*height);
        my_x = game_state->players[player_id].x_coord;
        my_y = game_state->players[player_id].y_coord;


        // libera semáforos
        sem_wait(&sync->reader_count_mutex);
        if (sync->readers_count-- == 1) {
            sem_post(&sync->game_state_mutex);
        }
        sem_post(&sync->reader_count_mutex);

        if(game_state->has_finished || game_state->players[player_id].is_blocked){
            break;
        }

        move = generate_move(width,height,my_board,my_x,my_y);

        if(move == NONE || game_state->has_finished || game_state->players[player_id].is_blocked){ // estoy bloqueado
            break;
        }

        //IF_EXIT(putchar(move) == EOF, "Error al escribir movimiento en stdout")
        IF_EXIT(write(STDOUT_FILENO, &move, sizeof(move)) == -1, "Error al escribir movimiento en stdout")
        //fflush(stdout);
         
        //while(!game_state->has_finished && !game_state->players[player_id].is_blocked && game_state->players[player_id].valid_mov_request + game_state->players[player_id].invalid_mov_requests != mov_count);
        usleep(2000000);
    }
    // Limpieza
    if (game_state != MAP_FAILED) {
        munmap(game_state, sizeof(game_t));
    }
    if (sync != MAP_FAILED) {
        munmap(sync, sizeof(game_sync));
    }
    return EXIT_SUCCESS;
    
}