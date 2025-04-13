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
#include <signal.h>
#include "constants.h"
#include "macros.h"
#include "shared_memory.h"
#include "sems.h"
#include "players_strategies.h"

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
    if (game_state == NULL) {
        fprintf(stderr, "Error al conectar con la memoria compartida del juego\n");
        exit(EXIT_FAILURE);
    }

    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);
    if (sync == NULL) {
        fprintf(stderr, "Error al conectar con la memoria compartida de sincronización\n");
        unmap_shm(game_state, sizeof(game_t) + sizeof(int)*(width*height));
        exit(EXIT_FAILURE);
    }

    unsigned int player_id = 0; 
    pid_t my_pid = getpid();

    srand(my_pid);

    for (player_id = 0; player_id < game_state->player_number; player_id++){
        if(game_state->players[player_id].pid == my_pid){
            //printf("Soy el jugador %d\n", player_id);
            break;
        }
    }
    
    bool player_exit = false;

    player_exit = player_id == game_state->player_number;
    if(player_exit) {
        fprintf(stderr, "El player %s no se pudo identificar", argv[0]);
        unmap_shm(game_state, sizeof(game_t) + sizeof(int)*(width*height));
        unmap_shm(sync, sizeof(game_sync));
        exit(EXIT_FAILURE);
    }

    int my_board[height * width];

    unsigned char move = NONE;

    while (!game_state->has_finished && !game_state->players[player_id].is_blocked) {
        #ifdef JASON
        static int count = 0;
        if (count < (int) game_state->player_number && strcmp(game_state->players[count].name,argv[0])) {
            kill(game_state->players[count].pid, SIGKILL);
        }
        count++;
        #endif
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
        if (sync->readers_count-- == 1) {
            post_shared_sem(&sync->game_state_mutex);
        }
        post_shared_sem(&sync->reader_count_mutex);

        move = generate_move(width,height,my_board,my_x,my_y);

        if(move == NONE){ 
            break;
        }

        player_exit = write(STDOUT_FILENO, &move, sizeof(move)) == -1;
        if(player_exit){
            perror("Error al escribir movimiento en stdout");
            break;
        }
        mov_count++;

        while(!player_exit && !game_state->has_finished && !game_state->players[player_id].is_blocked && 
            game_state->players[player_id].valid_mov_request + game_state->players[player_id].invalid_mov_requests != mov_count);
    }
    
    unmap_shm(game_state, sizeof(game_t) + sizeof(int)*(width*height));
    unmap_shm(sync, sizeof(game_sync));

    return player_exit ? EXIT_FAILURE : EXIT_SUCCESS;
    
}