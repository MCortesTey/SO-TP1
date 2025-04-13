// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <limits.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>
#include "constants.h"
#include "macros.h"
#include "shared_memory.h"
#include "child_manager.h"
#include "game.h"
#include "sems.h"

#define DESTROY_SEMS destroy_shared_sem(&sync->print_needed); \
                     destroy_shared_sem(&sync->print_done); \
                     destroy_shared_sem(&sync->master_access_mutex); \
                     destroy_shared_sem(&sync->game_state_mutex); \
                     destroy_shared_sem(&sync->reader_count_mutex); \

#define DESTROY_SHM destroy_shm(game, sizeof(game_t) + sizeof(int)*(width*height), SHM_GAME_PATH);\
                     destroy_shm(sync, sizeof(game_sync), SHM_GAME_SEMS_PATH);\
                     
#define FREE_PLAYER_NAMES \
    for(unsigned int i = 0; i < player_count; i++){ \
        free(players[i]); \
    } \
 
#define DESTROY_ALL DESTROY_SEMS\
                    DESTROY_SHM\
                    FREE_PLAYER_NAMES\

void parse_arguments(int argc, char const *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, unsigned int *timeout, unsigned int *seed, char **view_path, char **players, unsigned int *player_count) {
    
    *width = DEFAULT_WIDTH;
    *height = DEFAULT_HEIGHT;
    *delay = DEFAULT_DELAY;
    *timeout = DEFAULT_TIMEOUT;
    *seed = DEFAULT_SEED;
    *view_path = DEFAULT_VIEW;
    *player_count = 0;

    int c;
    while((c = getopt (argc, (char * const*)argv, "w:h:d:t:s:v:p:")) != -1){
        switch(c){
            case 'w':
                *width = atoi(optarg);
                break;
            case 'h':
                *height = atoi(optarg);
                break;
            case 'd':
                *delay = atoi(optarg);
                break;
            case 't':
                *timeout = atoi(optarg);
                break;
            case 's':
                *seed = atoi(optarg);
                break;
            case 'v':
                *view_path = optarg;
                break;
            case 'p': {
                int index = optind - 1; // se usa una variable auxiliar porque no se puede tocar optind

                while(index < argc && argv[index][0] != '-') {
                    IF_EXIT(index - (optind - 1) >= MAX_PLAYER_NUMBER, "Error: Too many players")
                    players[(*player_count)++] = strdup(argv[index++]); // guardo nombre completo para execv
                }
                break;
            }
            case '?': 
                fprintf(stderr, "Usage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    IF_EXIT(*player_count < MIN_PLAYER_NUMBER,"Error: At least one player must be specified using -p.")
    if(*height < DEFAULT_HEIGHT || *width < DEFAULT_WIDTH){
        fprintf(stderr, "Error: Minimal board dimensions: %dx%d. Given %dx%d\n", DEFAULT_WIDTH, DEFAULT_HEIGHT,*width, *height);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]){
    unsigned short width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;
    unsigned int seed = DEFAULT_SEED, delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, player_count = 0;
    char * view_path = DEFAULT_VIEW;
    char *players[MAX_PLAYERS];
    time_t last_valid_mov_time = time(NULL);

    parse_arguments(argc, argv, &width, &height, &delay, &timeout, &seed, &view_path, players, &player_count);

    fflush(stdout);
    if(view_path == NULL && delay != DEFAULT_DELAY){
        puts("Info: Delay parameter ignored since there is no view.");
        delay = DEFAULT_DELAY;
    }

    printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %d\n",
        width, height, delay, timeout, seed, view_path == NULL ? "-" : view_path, player_count);
    for(unsigned int i = 0; i < player_count; i++){
        printf("\t%s\n", players[i]);
    }

    sleep(3); // para que el usuario pueda leer lo que se imprimió antes.

    game_t * game = create_game(width, height, player_count, players, seed);
    game_sync* sync = create_game_sync();
    char board_dimensions[2][256];
    sprintf(board_dimensions[0],"%d",width);
    sprintf(board_dimensions[1],"%d",height);
    char * arguments[] = {view_path, board_dimensions[0], board_dimensions[1], NULL};


    pid_t view_pid;
    if(view_path != NULL){
        view_pid = new_child(view_path, arguments, -1, -1, -1, NULL); 
        if(view_pid == -1){
            perror("Error creating view process");
            DESTROY_ALL
            exit(EXIT_FAILURE);
        }
    }

    // jugadores
    int pipes[MAX_PLAYER_NUMBER][2];
    for(unsigned int i = 0; i < player_count; i++){
        if(pipe(pipes[i]) == -1){
            perror("pipe player");
            DESTROY_ALL
            exit(EXIT_FAILURE);
        }
    }

    for(unsigned int i = 0; i < player_count; i++){
        arguments[0] = players[i];
        game->players[i].pid = new_child(players[i], arguments, pipes[i][WRITE_END], -1, player_count, pipes); // write end
        if(game->players[i].pid == -1){
            perror("Error creating player process");
            DESTROY_ALL
            exit(EXIT_FAILURE);
        }
    }

    post_shared_sem(&sync->master_access_mutex); // arranca el juego

    while (!game->has_finished) {
        player_movement player_mov = get_move(game, pipes, player_count, timeout);

        wait_shared_sem(&sync->master_access_mutex);

        wait_shared_sem(&sync->game_state_mutex);

        post_shared_sem(&sync->master_access_mutex);

        game->has_finished = process_move(game, player_mov, &last_valid_mov_time, timeout);

        if(view_path != NULL){
            post_shared_sem(&sync->print_needed);
            wait_shared_sem(&sync->print_done);
        }

        usleep(delay*DELAY_MULTIPLIER);

        post_shared_sem(&sync->game_state_mutex);
    }

    while(view_path != NULL && view_pid > 0){
        int exit_status;
        if(wait_for_child(view_pid, &exit_status)){
            printf("View finished (%d)\n", exit_status);
            break;
        }
    }

    int remaining_players = player_count;
    while (remaining_players > 0) {
        for(unsigned int i = 0; i < player_count; i++) {
            int status;
            if (game->players[i].pid != 0 && wait_for_child(game->players[i].pid, &status)) {  
                printf("Player %s (%d) exited (%d) with a score of %d / %d / %d\n", players[i], i, status, game->players[i].score, game->players[i].valid_mov_request, game->players[i].invalid_mov_requests);
                game->players[i].pid = 0; 
                remaining_players--;
                close(pipes[i][READ_END]);
                close(pipes[i][WRITE_END]);
            }
        }
    }

    DESTROY_ALL

    return EXIT_SUCCESS;
}