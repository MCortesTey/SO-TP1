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
#include "shared_memory.h"
//#include "shm_utils.h"
#include "constants.h"
#include "macros.h"
#include "shm_ADT.h"

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

int rand_int(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

game_sync * create_game_sync(){
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



game_t * create_game(int width, int height, int n_players, char * players[], int seed){
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

    // Inicializar el tablero
    srand(seed);
    for(int i = 0; i < width * height; i++) {
        game_t_ptr->board_p[i] = rand_int(1, 9);
    }

    // Posicionar a los jugadores de manera elíptica
    double angle_step = 2 * M_PI / n_players;
    double a = (width - 1) / 2.0;  // semi-major axis
    double b = (height - 1) / 2.0; // semi-minor axis
    double center_x = a;
    double center_y = b;

    for(int i = 0; i < n_players; i++) {
        double angle = i * angle_step;
        int x_pos = (int)(center_x + a * cos(angle));
        int y_pos = (int)(center_y + b * sin(angle));
        game_t_ptr->players[i].x_coord = x_pos; // Coordenadas polares elípticas :)
        game_t_ptr->players[i].y_coord = y_pos;
        game_t_ptr->board_p[x_pos * width + y_pos] = -1*i; // Colocar a los jugadores en el tablero
    }


    return game_t_ptr;
}

void parse_arguments(int argc, char const *argv[], int *width, int *height, int *delay, int *timeout, int *seed, char **view_path, char **players, int *player_count) {
    // PONELE
    extern char *optarg; // Declare optarg for getopt
    extern int optind, opterr, optopt; // Declare getopt-related variables
    // END PONELE
    
    // Inicializar los valores por defecto
    *width = DEFAULT_WIDTH;
    *height = DEFAULT_HEIGHT;
    *delay = DEFAULT_DELAY;
    *timeout = DEFAULT_TIMEOUT;
    *seed = DEFAULT_SEED;
    *view_path = DEFAULT_VIEW;
    *player_count = 0;

    // Procesar argumentos
    int c;
    while((c = getopt (argc, (char * const*)argv, "w:h:d:t:s:v:p:")) != -1){
        switch(c){
            case 'w':
                *width = atoi(optarg);
                IF_EXIT(*width == 0, "atoi width")
                IF_EXIT(*width < DEFAULT_WIDTH, "width menor que la mínima")
                break;
            case 'h':
                *height = atoi(optarg);
                IF_EXIT(*height == 0, "atoi height")
                IF_EXIT(*height < DEFAULT_HEIGHT, "height menor que la mínima")
                break;
            case 'd':
                *delay = atoi(optarg);
                IF_EXIT(*delay == 0, "atoi delay")
                IF_EXIT(*delay < 0, "delay menor que cero")
                break;
            case 't':
                *timeout = atoi(optarg);
                IF_EXIT(*timeout == 0, "atoi timeout")
                IF_EXIT(*timeout < 0, "timeout menor que cero")
                break;
            case 's':
                *seed = atoi(optarg);
                break;
            case 'v':
                *view_path = optarg;
                break;
            case 'p': {
                // BORRAR ESTOS COMENTARIOS
                // optind es el índice del siguiente argumento a procesar
                // Si paso: ./master -p player1 
                // optind = 3 , optarg = player1 y argc = 3
                int index = optind - 1; // argv[index] = optarg

                while(index < argc && argv[index][0] != '-') {
                    IF_EXIT(strlen(argv[index]) > MAX_NAME_LEN, "Error: Player name too long")
                    IF_EXIT(index - (optind - 1) >= MAX_PLAYER_NUMBER, "Error: Too many players")

                    // Recuerde liberar el almacenamiento reservado con la llamada a strdup.
                    players[(*player_count)++] = strdup(argv[index++]); // strdup reserva espacio de almacenamiento para una copia de serie llamando a malloc
                }
                break;
            }
            case '?':
                fprintf(stderr, "Usage: ./ChompChamps_arm64 [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] -p player1 player2 ...\n");
                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    IF_EXIT(*player_count < MIN_PLAYER_NUMBER,"Error: At least one player must be specified using -p.")
}

typedef struct {
    int player_id;
    enum MOVEMENTS move;

} player_movement;



player_movement recibir_movimientos(game_t * game, int pipes[MAX_PLAYER_NUMBER][2], int player_count, int timeout) {
    static int last_player = 0;  // Para mantener política round robin
    fd_set read_fds;
    struct timeval tv;
    int max_fd = -1;

    // Configurar el timeout (es el timeout que necesita el select)
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // Inicializar el conjunto de descriptores
    FD_ZERO(&read_fds);

    // Agregar los pipes al conjunto, siguiendo round robin
    int current_player = last_player;
    for(int i = 0; i < player_count; i++) {
        if (!game->players[current_player].is_blocked) {
            FD_SET(pipes[current_player][READ_END], &read_fds);
            if (pipes[current_player][READ_END] > max_fd) {
                max_fd = pipes[current_player][READ_END];
            }
        }
        current_player = (current_player + 1) % player_count;
    }

    // Si no hay descriptores para monitorear, todos están bloqueados
    if (max_fd == -1) {
        return (player_movement){-1, NONE};
    }

    // Esperar por datos en los pipes
    int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
    
    if (ready == -1) {
        IF_EXIT(true, "select")
    } else if (ready == 0) {
        // Timeout
        return (player_movement){-1, NONE};
    }

    // Verificar qué pipe tiene datos, siguiendo round robin
    current_player = last_player;
    for(int i = 0; i < player_count; i++) {
        if (!game->players[current_player].is_blocked && 
            FD_ISSET(pipes[current_player][READ_END], &read_fds)) {
            
            unsigned char move;
            ssize_t bytes_read = read(pipes[current_player][READ_END], &move, sizeof(move));
            
            if (bytes_read == -1) {
                IF_EXIT(true, "read")
            } else if (bytes_read == 0) {
                // EOF detectado - marcar jugador como bloqueado
                game->players[current_player].is_blocked = true;
            } else {
                // Movimiento válido recibido
                last_player = (current_player + 1) % player_count;
                return (player_movement){current_player, move};
            }
        }
        current_player = (current_player + 1) % player_count;
    }

    // Si llegamos aquí, no se recibieron datos válidos
    return (player_movement){-1, NONE};
}

bool procesar_movimiento(game_t * game, player_movement pm){
    // Verificar si el movimiento es válido
    if(pm.player_id == -1) { // Timeout
        return true;
    }

    if(pm.move < 0 || pm.move > 7 || game->players[pm.player_id].is_blocked) { // Invalid movement
        game->players[pm.player_id].invalid_mov_requests++;
        return false;
    }
    int new_x = game->players[pm.player_id].x_coord + dx[pm.move];
    int new_y = game->players[pm.player_id].y_coord + dy[pm.move];

    if(new_x < 0 || new_x >= game->board_width || new_y < 0 || new_y >= game->board_height){
        game->players[pm.player_id].invalid_mov_requests++;
    } else {
        game->players[pm.player_id].valid_mov_request++;
        game->players[pm.player_id].x_coord = new_x;
        game->players[pm.player_id].y_coord = new_y;
        game->players[pm.player_id].score += game->board_p[new_x * game->board_width + new_y];
        game->board_p[new_x * game->board_width + new_y] = -1 * pm.player_id; // Colocar al jugador en el tablero
    }

    // Verificar si el jugador está bloqueado
    bool blocked = true;
    for(int i = UP; i <= UP_LEFT; i++){
        int check_x = game->players[pm.player_id].x_coord + dx[i];
        int check_y = game->players[pm.player_id].y_coord + dy[i];
        if(check_x >= 0 && check_x < game->board_width && check_y >= 0 && check_y < game->board_height && game->board_p[check_x * game->board_width + check_y] > 0){
            blocked = false;
            break;
        }
    }

    if(blocked){
        game->players[pm.player_id].is_blocked = true;
        bool all_blocked = true;
        for(unsigned int i = 0; i < game->player_number; i++){
            if(!game->players[i].is_blocked){
                all_blocked = false;
                break;
            }
        }
        if(all_blocked){
            return true;
        }
    }
    return false;
}

int main(int argc, char const *argv[]){
    int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT, delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, seed = DEFAULT_SEED;
    char * view_path = DEFAULT_VIEW;
    int player_count = 0;
    char *players[MAX_PLAYERS];

    parse_arguments(argc, argv, &width, &height, &delay, &timeout, &seed, &view_path, players, &player_count);

    game_t * game = create_game(width, height, player_count, players, seed);
    game_sync* sync = create_game_sync();

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
    if(!IS_NULL(view_path)){
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
            sleep(1);
            // close unused pipes
            IF_EXIT_NON_ZERO(close(pipes[i][WRITE_END]),"close")

            game->players[i].pid = pid;
        }
    }

    pid_t view_pid = 0;
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
            view_pid = pid;
        }
    }
    
    puts("aa");
    // Bucle principal del master
    while (!game->has_finished) {
        puts("bb");
        // Leer movimientos de los jugadores
        player_movement pm = recibir_movimientos(game,pipes, player_count, timeout);
        puts("aa");
        sem_wait(&sync->master_access_mutex);
        sem_wait(&sync->game_state_mutex);
        sem_post(&sync->master_access_mutex);

        // Mover
        game->has_finished = procesar_movimiento(game, pm);
        // Señalar al view que hay cambios para imprimir
        sem_post(&sync->print_needed);
        
        // Esperar a que el view termine de imprimir
        sem_wait(&sync->print_done);

        sem_post(&sync->game_state_mutex);
    }
    puts("cc");

    // wait a la vista
    if(view_path != NULL && view_pid > 0){
        IF_EXIT_NON_ZERO(waitpid(view_pid, NULL, 0), "waitpid view")
    }
    // wait a los jugadores
    int remaining_players = player_count;
    while (remaining_players > 0) {
        for(int i = 0; i < player_count; i++) {
            if (game->players[i].pid != 0) {  // Si el pid es 0, ya lo procesamos
                int status;
                pid_t result = waitpid(game->players[i].pid, &status, WNOHANG);
                IF_EXIT(result == -1, "waitpid player")
                
                if (result > 0) {  // El proceso terminó
                    printf("Player %s finished\n", game->players[i].name);
                    free(players[i]);
                    IF_EXIT_NON_ZERO(close(pipes[i][READ_END]),"close")
                    IF_EXIT_NON_ZERO(close(pipes[i][WRITE_END]),"close")
                    game->players[i].pid = 0;  // Marcar como procesado
                    remaining_players--;
                }
            }
        }
        if (remaining_players > 0) {
            usleep(1000);  // Pequeña pausa para no consumir CPU innecesariamente
        }
    }

    if(view_path != NULL) {
        IF_EXIT_NON_ZERO(close(pipes[player_count][READ_END]),"close")
        IF_EXIT_NON_ZERO(close(pipes[player_count][WRITE_END]),"close")
    }

    // Destroy semaphores
    sem_destroy(&sync->print_needed);
    sem_destroy(&sync->print_done);
    sem_destroy(&sync->master_access_mutex);
    sem_destroy(&sync->game_state_mutex);
    sem_destroy(&sync->reader_count_mutex);

    IF_EXIT_NON_ZERO(munmap(game,sizeof(game_t) + sizeof(int)*(width*height)), "munmap game")
    IF_EXIT_NON_ZERO(munmap(sync,sizeof(game_sync)), "munmap game")

    IF_EXIT_NON_ZERO(shm_unlink(SHM_GAME_PATH), "shm_unlink game")
    IF_EXIT_NON_ZERO(shm_unlink(SHM_GAME_SEMS_PATH), "shm_unlink sync")


    return 0;
}
