#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <limits.h>
#include "shared_memory.h"
#include "shm_utils.h"
#include "constants.h"

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL


#define MIN_PLAYER_NUMBER 1
#define MAX_PLAYER_NUMBER 9
#define MAX_NAME_LEN 16


void init_shared_sem(sem_t * sem, int initial_value){
    if(sem_init(sem,1,initial_value) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
}

game_sync * createGameSync(){
    game_sync * game_sync_ptr = create_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync));

    if(game_sync_ptr == NULL){ // no debería ocurrir esto
        perror("Could not create game_sync");
        exit(EXIT_FAILURE);
    }

    // setear semáforos iniciales
    init_shared_sem(&game_sync_ptr->print_needed,0);
    init_shared_sem(&game_sync_ptr->print_done,1);
    init_shared_sem(&game_sync_ptr->master_access_mutex,1);
    init_shared_sem(&game_sync_ptr->game_state_mutex,1);
    init_shared_sem(&game_sync_ptr->reader_count_mutex,1);
    game_sync_ptr->readers_count = 0;

    /**
     * 
     */

}

game_t * createGame(){
    game_t * game_t_ptr = create_shm(SHM_GAME_PATH, sizeof(game_t)); // crea memoria compartida

    if(game_t_ptr == NULL){ // no debería ocurrir esto
        perror("Could not create game_state");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]){
    // PONELE
    extern char *optarg; // Declare optarg for getopt
    extern int optind, opterr, optopt; // Declare getopt-related variables
    // END PONELE

    int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT, delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, seed = DEFAULT_SEED;
    char * view_path = DEFAULT_VIEW;
    size_t player_count = 0;
    char* players[MAX_PLAYERS];
    int c;
    while((c = getopt (argc, argv, "w:h:d:t:s:v:p:")) != -1){
        switch(c){
            case 'w':
                width = atoi(optarg);
                if(width == 0) {
                    perror("atoi width");
                    exit(EXIT_FAILURE);
                }
                if(width < DEFAULT_WIDTH){
                    perror("width menor que la mínima");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'h':
                height = atoi(optarg);
                if(height == 0) {
                    perror("atoi height");
                    exit(EXIT_FAILURE);
                }
                if(height < DEFAULT_HEIGHT){
                    perror("height menor que la mínima");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                delay = atoi(optarg);
                if(delay == 0) {
                    perror("atoi delay");
                    exit(EXIT_FAILURE);
                }
                if(delay < 0){
                    perror("delay menor que 0");
                    exit(EXIT_FAILURE);
                }
                break;
            case 't':
                timeout = atoi(optarg);
                if(timeout == 0) {
                    perror("atoi timeout");
                    exit(EXIT_FAILURE);
                }
                if(timeout < 0){
                    perror("timeout menor que 0");
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'v':
                view_path = optarg;
                break;
            case 'p':
                // TODO: agarrar valores de players usando optind y guardarlos. adaptar este bloque de código a lo nuestro
                players[player_count++] = optarg; 
                while (optind < argc && argv[optind][0] != '-') {
                    if (player_count < MAX_PLAYERS) {
                        players[player_count++] = argv[optind++];
                    } else {
                        fprintf(stderr, "Error: Se alcanzó el límite de jugadores (%d).\n", MAX_PLAYERS);
                        return 1;
                    }
                }
                break;
            case '?':
                if (optopt == 'w' || optopt == 'l' || optopt == 'd' || optopt == 't'
                    || optopt == 's' || optopt == 'v' || optopt == 'p')
                    fprintf(stderr, "La opción -%c requiere un argumento.\n", optopt);
                else
                    fprintf(stderr, "Opción desconocida `-%c'.\n", optopt);
                // sin break así baja al default !
            default:
                exit(EXIT_FAILURE);
        }
    }
    
    if(player_count < MIN_PLAYER_NUMBER || player_count > MAX_PLAYER_NUMBER){
        perror("Error: At least one player must be specified using -p.");
    }

    game_t * game = createGame();
    game_sync* sync = createGameSync();

    game->board_width = width;
    game->board_height = height;
    game->player_number = player_count;
    game->has_finished = false;

    char board_dimensions[2][256];
    // board_dimensions[0] = alto
    // board_dimensions[1] = ancho
    sprintf(board_dimensions[0],"%d",height);
    sprintf(board_dimensions[1],"%d",width);
    
    for(int i=0; i<player_count; i++){
        pid_t pid = fork();
        if(pid == 0){ 
            execv(players[i],(char * const *)board_dimensions);
            perror("execv player");
            exit(EXIT_FAILURE);
        } else if(pid > 0){
            game->players[i].pid = pid;
        } else {
            perror("fork player");
            exit(EXIT_FAILURE);
        }
    }

    int view_pid;

    if(view_path != NULL){
        pid_t pid = fork();
        if(pid == 0){ 
            // execve
            exit(EXIT_FAILURE);
        } else if(pid > 0){
            view_pid = pid;
        } else {
            perror("fork view");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
