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

enum MOVEMENTS {NONE = -1, UP, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT};

#ifdef FIRST_POSSIBLE

char generate_move(int width, int height, int board[][height], int x_pos, int y_pos){ 
    for(int i = UP; i <= UP_LEFT; i++){
        int x = x_pos;
        int y = y_pos;
        switch(i){
            case UP:
                y--;
                break;
            case UP_RIGHT:
                x++;
                y--;
                break;
            case RIGHT:
                x++;
                break;
            case DOWN_RIGHT:
                x++;
                y++;
                break;
            case DOWN:
                y++;
                break;
            case DOWN_LEFT:
                x--;
                y++;
                break;
            case LEFT:
                x--;
                break;
            case UP_LEFT:
                x--;
                y--;
                break;
        }
        if (x >= 0 && x < width && y >= 0 && y < height && board[x][y] > 0) {
            return i; // devuelve el movimiento
        }
    }
    return NONE; // si no hay movimientos válidos, devuelve NONE
}

#endif
#ifdef SPARSE
char generate_move(int width, int height, int board[][height], int x_pos, int y_pos) {
    int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
    int dy[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    int best_move = NONE, max_value = 0;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i], y = y_pos + dy[i];
        if (x >= 0 && x < width && y >= 0 && y < height && board[x][y] > max_value) {
            max_value = board[x][y];
            best_move = i;
        }
    }
    return best_move;
}

#endif

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    srand(getpid()); // provisorio, después ver como tunearlo para que sea más random aún

    int width = atoi(argv[1]);
    IF_EXIT(width <= 0, "ancho inválido")

    int height = atoi(argv[2]);
    IF_EXIT(height <= 0, "alto inválido")

    // Conectar a la memoria compartida del juego
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t), O_RDONLY);

    // Conectar a la memoria compartida de sincronización
    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);

    int player_id = -1; // el jugador va a identificarse a sí mismo!!
    pid_t my_pid = getpid();
    for (player_id = 0; player_id < game_state->player_number; player_id++){
        if(game_state->players[player_id].pid == my_pid){
            break;
        }
    }

    IF_EXIT(player_id < 0, "player could not identify itself")

    while (!game_state->has_finished || !game_state->players[player_id].is_blocked) {

        // lee estado de juego
        sem_wait(&sync->reader_count_mutex); // bloquea el reader count
        sync->readers_count++;  // incrementa el reader count
        if (sync->readers_count == 1) { 
            sem_wait(&sync->game_state_mutex); // si hay un solo lector bloquea el game state
        }

        sem_post(&sync->reader_count_mutex);

        // chequea si el jugador está bloqueado    
        // deberiamos pasar el id como arg cdo hacemos el execv de player para poder usarlo aca 
        // if (game_state->players[player_id].is_blocked) { 
        //     // Liberar semáforos
        //     sem_wait(&sync->reader_count_mutex);
        //     sync->readers_count--;
        //     if (sync->readers_count == 0) {
        //         sem_post(&sync->game_state_mutex); // libera el game state
        //     }
        //     sem_post(&sync->reader_count_mutex); 
            
        // }

        // TODO: genera movimiento
        char move = generate_move(game_state->board_width,game_state->board_height,(int (*)[height])game_state->board_p,game_state->players[player_id].x_coord,game_state->players[player_id].y_coord);
        IF_EXIT(write(STDOUT_FILENO, &move, sizeof(unsigned char)) == -1 , "write")



        // libera semáforos
        sem_wait(&sync->reader_count_mutex);
        sync->readers_count--;
        if (sync->readers_count == 0) {
            sem_post(&sync->game_state_mutex);
        }
        sem_post(&sync->reader_count_mutex);

        // TODO:espera respuesta del master por el pipe
        
        sleep(1);
    }

    // Limpieza
    munmap(game_state, sizeof(game_t));
    munmap(sync, sizeof(game_sync));

    return EXIT_SUCCESS;
}