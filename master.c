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
#include "shm_ADT.h"

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL
#define DELAY_MULTIPLIER 200

enum pipe_ends{READ_END, WRITE_END};

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

    // set intial values for sems
    init_shared_sem(&game_sync_ptr->print_needed,1);
    init_shared_sem(&game_sync_ptr->print_done,0);
    init_shared_sem(&game_sync_ptr->master_access_mutex,0);
    init_shared_sem(&game_sync_ptr->game_state_mutex,1);
    init_shared_sem(&game_sync_ptr->reader_count_mutex,1);
    game_sync_ptr->readers_count = 0;

    return game_sync_ptr;
}



game_t * create_game(unsigned int width, unsigned int height, unsigned int n_players, char * players[], unsigned int seed){
    game_t * game_t_ptr = create_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP); // 0644
    IF_EXIT_NULL(game_t_ptr,"Could not create game_state")

    game_t_ptr->board_width = width;
    game_t_ptr->board_height = height;
    game_t_ptr->player_number = n_players;
    game_t_ptr->has_finished = false;

    // init players
    for(unsigned int i = 0; i < n_players; i++) {
        strncpy(game_t_ptr->players[i].name, players[i], MAX_NAME_LEN - 1);
        game_t_ptr->players[i].score = 0;
        game_t_ptr->players[i].invalid_mov_requests = 0;
        game_t_ptr->players[i].valid_mov_request = 0;
        game_t_ptr->players[i].x_coord = 0;
        game_t_ptr->players[i].y_coord = 0;
        game_t_ptr->players[i].pid = 0;
        game_t_ptr->players[i].is_blocked = false;
    }

    // init board

    srand(seed);
    for(unsigned int i = 0; i < width * height; i++) {
        game_t_ptr->board_p[i] = rand_int(1, 9);
    }


    double angle_step = 2 * M_PI / n_players;
    double a = (width  * 0.75) / 2.0;  // semi-major axis
    double b = (height * 0.75) / 2.0; // semi-minor axis
    double center_x = width / 2.0;
    double center_y = height / 2.0;

    for(unsigned int i = 0; i < n_players; i++) {
        double angle = i * angle_step;
        int x_pos = (int)(center_x + a * cos(angle));
        int y_pos = (int)(center_y + b * sin(angle));
        game_t_ptr->players[i].x_coord = x_pos; 
        game_t_ptr->players[i].y_coord = y_pos;
        game_t_ptr->board_p[y_pos * width + x_pos] = -1*i;
    }


    return game_t_ptr;
}

void parse_arguments(int argc, char const *argv[], unsigned int *width, unsigned int *height, unsigned int *delay, unsigned int *timeout, unsigned int *seed, char **view_path, char **players, unsigned int *player_count) {
    
    // default values
    *width = DEFAULT_WIDTH;
    *height = DEFAULT_HEIGHT;
    *delay = DEFAULT_DELAY;
    *timeout = DEFAULT_TIMEOUT;
    *seed = DEFAULT_SEED;
    *view_path = DEFAULT_VIEW;
    *player_count = 0;

    // process args
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
                int index = optind - 1; // optind cannot be used!! aux var --> index

                while(index < argc && argv[index][0] != '-') {
                    IF_EXIT(index - (optind - 1) >= MAX_PLAYER_NUMBER, "Error: Too many players")
                    players[(*player_count)++] = strdup(argv[index++]); // player name (for execv)
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
}

typedef struct {
    int player_id;
    enum MOVEMENTS move;
} player_movement;

player_movement get_move(game_t * game, int pipes[MAX_PLAYER_NUMBER][2], int player_count, unsigned int timeout) {
    static int last_player = 0;  // round robin!
    fd_set read_fds;
    int max_fd = -1;

    // timeout using struct
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // init fd set
    FD_ZERO(&read_fds);

    // round robin for pipes
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

    // max_fd == 1 --> all blocked
    if (max_fd == -1) {
        return (player_movement){-1, NONE};
    }

    // wait for data on pipes
    int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
    if (ready == -1) {
        IF_EXIT(true, "select")
    } else if (ready == 0) {
        // timeout!
        return (player_movement){-1, NONE};
    } else {
        // verify data
        current_player = last_player;
        for(int i = 0; i < player_count; i++) {
            if (!game->players[current_player].is_blocked && 
                FD_ISSET(pipes[current_player][READ_END], &read_fds)) {
                
                unsigned char move;
                ssize_t bytes_read = read(pipes[current_player][READ_END], &move, sizeof(move));
                
                if (bytes_read == -1) {
                    IF_EXIT(true, "read")
                } else if (bytes_read == 0) {
                    // EOF == player finished
                    game->players[current_player].is_blocked = true;
                } else {
                    last_player = (current_player + 1) % player_count;
                    return (player_movement){current_player, move};
                }
            }
            current_player = (current_player + 1) % player_count;
        }
    }

    return (player_movement){-1, NONE};
}

bool process_move(game_t * game, player_movement player_mov){
    // player_id == -1 --> timeout
    if(player_mov.player_id == -1) { // Timeout
        return true;
    }

    if(player_mov.move < 0 || player_mov.move > 7 || game->players[player_mov.player_id].is_blocked) { // invalid mov 
        game->players[player_mov.player_id].invalid_mov_requests++;
        return false;
    }

    int new_x = game->players[player_mov.player_id].x_coord + dx[player_mov.move];
    int new_y = game->players[player_mov.player_id].y_coord + dy[player_mov.move];

    if(new_x < 0 || new_x >= game->board_width || new_y < 0 || new_y >= game->board_height || game->board_p[new_y * game->board_width + new_x] <= 0) { // Invalid move
        game->players[player_mov.player_id].invalid_mov_requests++;
    } else {
        game->players[player_mov.player_id].valid_mov_request++;
        game->players[player_mov.player_id].x_coord = new_x;
        game->players[player_mov.player_id].y_coord = new_y; 
        game->players[player_mov.player_id].score += game->board_p[new_y * game->board_width + new_x];
        game->board_p[new_y * game->board_width + new_x] = -1 * player_mov.player_id; 
    }

    bool is_player_blocked = true;
    for(int i = UP; i <= UP_LEFT; i++){
        int check_x = game->players[player_mov.player_id].x_coord + dx[i];
        int check_y = game->players[player_mov.player_id].y_coord + dy[i];
        if(check_x >= 0 && check_x < game->board_width && check_y >= 0 && check_y < game->board_height && game->board_p[check_y * game->board_width + check_x] > 0){
            is_player_blocked = false;
            break;
        }
    }

    if(is_player_blocked){
        game->players[player_mov.player_id].is_blocked = true;
        bool all_players_blocked = true;
        for(unsigned int i = 0; i < game->player_number; i++){
            if(!game->players[i].is_blocked){
                all_players_blocked = false;
                break;
            }
        }
        if(all_players_blocked){
            return true;
        }
    }
    return false;
}

int main(int argc, char const *argv[]){
    unsigned int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT, seed = DEFAULT_SEED, delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, player_count = 0;
    char * view_path = DEFAULT_VIEW;
    char *players[MAX_PLAYERS];

    parse_arguments(argc, argv, &width, &height, &delay, &timeout, &seed, &view_path, players, &player_count);

    game_t * game = create_game(width, height, player_count, players, seed);
    game_sync* sync = create_game_sync();
    char board_dimensions[2][256];
    sprintf(board_dimensions[0],"%d",height);
    sprintf(board_dimensions[1],"%d",width);
    
    // pipes for players
    int pipes[MAX_PLAYER_NUMBER][2];
    for(unsigned int i = 0; i < player_count; i++){
        IF_EXIT_NON_ZERO(pipe(pipes[i]), "pipe player")
    }

    pid_t view_pid = 0;
    if(view_path != NULL){
        pid_t pid = fork();
        IF_EXIT(pid < 0, "fork view")
        if(pid == 0){ 
            char *view_args[] = {view_path, board_dimensions[0], board_dimensions[1], NULL};
            execv(view_path, view_args);
            IF_EXIT(true,"execv view")
            exit(EXIT_FAILURE);
        } else if(pid > 0){
            view_pid = pid;
        }
    }

    for(unsigned int i = 0; i < player_count; i++){
        pid_t pid = fork();
        IF_EXIT(pid < 0, "fork player")
        if(pid == 0){ 
            IF_EXIT_NON_ZERO(close(STDOUT_FILENO), "close stdout")
            IF_EXIT(dup(pipes[i][WRITE_END]) == -1, "dup") 
            game->players[i].pid = getpid(); // set for player use
            char *player_args[] = {players[i], board_dimensions[0], board_dimensions[1], NULL};
            execv(players[i], player_args);
            IF_EXIT(true,"execv player") 
        } else if(pid > 0){ 
            // close unused pipe
            IF_EXIT_NON_ZERO(close(pipes[i][WRITE_END]),"close unused pipe")
        }
    }
    fflush(stdout);
    if(view_path == NULL && delay != DEFAULT_DELAY){
        puts("Info: Delay parameter ignored since there is no view.");
    }

    printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %d\n",
        width, height, delay, timeout, seed, view_path == NULL ? "-" : view_path, player_count);
    for(unsigned int i = 0; i < player_count; i++){
        printf("\t%s\n", players[i]);
    }

    sleep(3); 
    fflush(stdout);
    sem_post(&sync->master_access_mutex); // start game

    while (!game->has_finished) {

        player_movement player_mov = get_move(game, pipes, player_count, timeout);

        sem_wait(&sync->master_access_mutex);
        sem_wait(&sync->game_state_mutex);

        game->has_finished = process_move(game, player_mov);

        if(view_path != NULL){
            sem_wait(&sync->game_state_mutex);
            sem_post(&sync->print_done);
            usleep(delay*DELAY_MULTIPLIER); 
        }

        sem_post(&sync->game_state_mutex);
        sem_post(&sync->master_access_mutex);

        
    }

    // wait for view to exit
    if(view_path != NULL && view_pid > 0){
        int exit_status;
        waitpid(view_pid, &exit_status, 0);
        printf("View finished (%d)\n", exit_status);
    }

    // wait for players to exit
    int remaining_players = player_count;
    while (remaining_players > 0) {
        for(unsigned int i = 0; i < player_count; i++) {
            if (game->players[i].pid != 0) {  
                int status;
                pid_t result = waitpid(game->players[i].pid, &status, WNOHANG);
                IF_EXIT(result == -1, "waitpid player")
                
                if (result > 0) {  
                    printf("Player %s (%d) exited (%d) with a score of %d / %d / %d\n", players[i], i, status, game->players[i].score, game->players[i].valid_mov_request, game->players[i].invalid_mov_requests);
                    free(players[i]);
                    game->players[i].pid = 0; 
                    remaining_players--;
                }
            }
        }

    }

    sem_destroy(&sync->print_needed);
    sem_destroy(&sync->print_done);
    sem_destroy(&sync->master_access_mutex);
    sem_destroy(&sync->game_state_mutex);
    sem_destroy(&sync->reader_count_mutex);

    destroy_shm(game, sizeof(game_t) + sizeof(int)*(width*height), SHM_GAME_PATH);
    destroy_shm(sync, sizeof(game_sync), SHM_GAME_SEMS_PATH);

    return 0;
}
