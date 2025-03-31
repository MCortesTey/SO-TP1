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
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <limits.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "shared_memory.h"
#include "shm_utils.h"
#include "constants.h"
#include "macros.h"

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL

enum pipeEnds{READ_END, WRITE_END};

#define MIN_PLAYER_NUMBER 1
#define MAX_PLAYER_NUMBER 9
#define MAX_NAME_LEN 16


void init_shared_sem(sem_t * sem, int initial_value){
    IF_EXIT_NON_ZERO(sem_init(sem,1,initial_value),"sem_init")
}

game_sync * createGameSync(){
    game_sync * game_sync_ptr = create_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP); // 0666
    IF_EXIT_NULL(game_sync_ptr,"Could not create game_sync")

    // setear semáforos iniciales
    init_shared_sem(&game_sync_ptr->print_needed,0);
    init_shared_sem(&game_sync_ptr->print_done,1);
    init_shared_sem(&game_sync_ptr->master_access_mutex,1);
    init_shared_sem(&game_sync_ptr->game_state_mutex,1);
    init_shared_sem(&game_sync_ptr->reader_count_mutex,1);
    game_sync_ptr->readers_count = 0;

    return game_sync_ptr;
}



game_t * createGame(int width, int height, int n_players, char * players[]){
    game_t * game_t_ptr = create_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP); // 0644
    IF_EXIT_NULL(game_t_ptr,"Could not create game_state")

    game_t_ptr->board_width = width;
    game_t_ptr->board_height = height;
    game_t_ptr->player_number = n_players;
    game_t_ptr->has_finished = false;

    // Inicializar el array de jugadores
    for(int i = 0; i < n_players; i++) {
        strncpy(game_t_ptr->players[i].name, players[i], MAX_NAME_LEN);
        game_t_ptr->players[i].score = 0;
        game_t_ptr->players[i].invalid_mov_requests = 0;
        game_t_ptr->players[i].valid_mov_request = 0;
        game_t_ptr->players[i].x_coord = 0;
        game_t_ptr->players[i].y_coord = 0;
        game_t_ptr->players[i].pid = 0;
        game_t_ptr->players[i].is_blocked = false;
    }



    return game_t_ptr;
}

int main(int argc, char const *argv[]){
    // PONELE
    extern char *optarg; // Declare optarg for getopt
    extern int optind, opterr, optopt; // Declare getopt-related variables
    // END PONELE

    int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT, delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, seed = DEFAULT_SEED;
    char * view_path = DEFAULT_VIEW;
    int player_count = 0;
    char *players[MAX_PLAYERS];
    int c;
    while((c = getopt (argc, (char * const*)argv, "w:h:d:t:s:v:p:")) != -1){
        switch(c){
            case 'w':
                width = atoi(optarg);
                IF_EXIT(width == 0, "atoi width")
                IF_EXIT(width < DEFAULT_WIDTH, "width menor que la mínima")
                break;
            case 'h':
                height = atoi(optarg);
                IF_EXIT(height == 0, "atoi height")
                IF_EXIT(height < DEFAULT_HEIGHT, "height menor que la mínima")
                break;
            case 'd':
                delay = atoi(optarg);
                IF_EXIT(delay == 0, "atoi delay")
                IF_EXIT(delay < 0, "delay menor que cero")
                break;
            case 't':
                timeout = atoi(optarg);
                IF_EXIT(timeout == 0, "atoi timeout")
                IF_EXIT(timeout < 0, "timeout menor que cero")
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'v':
                view_path = optarg;
                break;
            case 'p':
                // Guardamos el primer jugador que viene en optarg
                if (strlen(optarg) > MAX_NAME_LEN) {
                    fprintf(stderr, "Error: Player name too long (max %d characters).\n", MAX_NAME_LEN);
                    exit(EXIT_FAILURE);
                }
                
                players[player_count++] = strdup(optarg); 
                
                // Agarramos los jugadores que vienen después
                while (optind < argc && argv[optind][0] != '-') {
                    if (player_count >= MAX_PLAYERS) {
                        fprintf(stderr, "Error: Se alcanzó el límite de jugadores (%d).\n", MAX_PLAYERS);
                        exit(EXIT_FAILURE);
                    }
                    
                    // creo que esto no tiene sentido porque después el nombre se trunca.

                    // if (strlen(argv[optind]) > MAX_NAME_LEN) {
                    //     fprintf(stderr, "Error: Player name too long (max %d characters).\n", MAX_NAME_LEN);
                    //     exit(EXIT_FAILURE);
                    // }
                    players[player_count++] = strdup(argv[optind++]);
                }
                
                if (player_count < MIN_PLAYER_NUMBER) {
                    fprintf(stderr, "Error: Se requiere al menos un jugador.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case '?':
                if (optopt == 'w' || optopt == 'l' || optopt == 'd' || optopt == 't'
                    || optopt == 's' || optopt == 'v' || optopt == 'p')
                    fprintf(stderr, "La opción -%c requiere un argumento.\n", optopt);
                else
                    fprintf(stderr, "Opción desconocida `-%c'.\n", optopt);
                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    

    IF_EXIT(player_count < MIN_PLAYER_NUMBER,"Error: At least one player must be specified using -p.")
    IF_EXIT(player_count > MAX_PLAYER_NUMBER, "Error: max players = 9") // adaptar a lo que sea...

    game_t * game = createGame(width, height, player_count, players);
    game_sync* sync = createGameSync();

    char board_dimensions[2][256];
    sprintf(board_dimensions[0],"%d",height);
    sprintf(board_dimensions[1],"%d",width);
    
    // preparaciones para el fork ! 
    // pipe

    int pipes[MAX_PLAYER_NUMBER][2];

    // Crear pipes para jugadores
    for(int i = 0; i < player_count; i++){
        IF_EXIT_NON_ZERO(pipe(pipes[i]), "pipe")
    }

    // Crear pipe para view si existe
    if(view_path != NULL){
        IF_EXIT_NON_ZERO(pipe(pipes[player_count]), "pipe view")
    }

    for(int i=0; i < player_count; i++){
        pid_t pid = fork();
        IF_EXIT(pid < 0, "fork player")
        if(pid == 0){ // HIJO
            // cierro stdout 
            // dup pipe
            // cierro pipe que dupliqué
            // cierro los otros pipes porque no me sirven
            IF_EXIT_NON_ZERO(dup2(pipes[i][WRITE_END],STDOUT_FILENO),"dup2")


            // PENSAR SI QUIZÁS NO CONVIENE CERRAR SOLAMENTE PARA j!=i

            for(int j = 0; j < player_count; j++){ // close unused pipes
                IF_EXIT(close(pipes[j][READ_END]) != 0, "close")
                IF_EXIT(close(pipes[j][WRITE_END]) != 0, "close")
            }
            char *player_args[] = {players[i], board_dimensions[0], board_dimensions[1], NULL};
            execv(players[i], player_args);
            IF_EXIT(true,"execv player") // no debería llegar acá.
        } else if(pid > 0){ // PADRE
            // close unused pipes
            IF_EXIT_NON_ZERO(close(pipes[i][WRITE_END]),"close")

            game->players[i].pid = pid;
        }
    }

    //int view_pid;
    if(view_path != NULL){
        pid_t pid = fork();
        IF_EXIT(pid < 0, "fork view")
        if(pid == 0){ 
            // Redirigir stdout al pipe del view
            IF_EXIT_NON_ZERO(dup2(pipes[player_count][WRITE_END], STDOUT_FILENO), "dup2")

            // Cerrar todos los pipes que no se usan
            for(int j = 0; j < player_count + 1; j++){ 
                IF_EXIT_NON_ZERO(close(pipes[j][READ_END]), "close")
                IF_EXIT_NON_ZERO(close(pipes[j][WRITE_END]), "close")
            }

            char *view_args[] = {view_path, board_dimensions[0], board_dimensions[1], NULL};
            execv(view_path, view_args);
            IF_EXIT(true,"execv view")
            exit(EXIT_FAILURE);
        } else if(pid > 0){
            //view_pid = pid;
        }
    }
    

    // Bucle principal del master
    while (!game->has_finished) {

        // Leer movimientos de los jugadores
       


        // Señalar al view que hay cambios para imprimir
        sem_post(&sync->print_needed);
        
        // Esperar a que el view termine de imprimir
        sem_wait(&sync->print_done);

    }

    IF_EXIT_NON_ZERO(munmap(game,sizeof(game_t) + sizeof(int)*(width*height)), "munmap game")
    IF_EXIT_NON_ZERO(munmap(sync,sizeof(game_sync)), "munmap game")

    IF_EXIT_NON_ZERO(shm_unlink(SHM_GAME_PATH), "shm_unlink game")
    IF_EXIT_NON_ZERO(shm_unlink(SHM_GAME_SEMS_PATH), "shm_unlink sync")

    // libera espacio de los nombres y pipes
    for(int i = 0; i < player_count; i++){
        free(players[i]);
        IF_EXIT_NON_ZERO(close(pipes[i][READ_END]),"close")
        IF_EXIT_NON_ZERO(close(pipes[i][WRITE_END]),"close")
    }


    return 0;
}
