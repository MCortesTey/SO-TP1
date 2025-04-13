#include "game.h"
#include "constants.h"
#include "macros.h"
#include <sys/stat.h>
#include <sys/select.h>
#include "sems.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "valid_move.h"
#include <poll.h>

int rand_int(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

// src: https://stackoverflow.com/questions/36663845/check-if-unix-pipe-closed-without-writing-anything
bool is_pipe_closed(int fd) {
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLOUT,
    };

    if (poll(&pfd, 1, 1) < 0) {
        return false;
    }

    return pfd.revents & POLLERR;
}

game_sync * create_game_sync(){
    game_sync * game_sync_ptr = create_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP); // 0666
    if (game_sync_ptr == NULL) {
        fprintf(stderr, "Error al crear la memoria compartida de sincronización\n");
        return NULL;
    }

    init_shared_sem(&game_sync_ptr->print_needed,1);
    init_shared_sem(&game_sync_ptr->print_done,0);
    init_shared_sem(&game_sync_ptr->master_access_mutex,0);
    init_shared_sem(&game_sync_ptr->game_state_mutex,1);
    init_shared_sem(&game_sync_ptr->reader_count_mutex,1);
    game_sync_ptr->readers_count = 0;

    return game_sync_ptr;
}

game_t * create_game(unsigned short width, unsigned short height, unsigned int n_players, char * players[], unsigned int seed){
    game_t * game_t_ptr = create_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP); // 0644
    if (game_t_ptr == NULL) {
        fprintf(stderr, "Error al crear la memoria compartida del juego\n");
        return NULL;
    }

    game_t_ptr->board_width = width;
    game_t_ptr->board_height = height;
    game_t_ptr->player_number = n_players;
    game_t_ptr->has_finished = false;

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

    srand(seed);
    for(unsigned int i = 0; i < width * height; i++) {
        game_t_ptr->board_p[i] = rand_int(1, 9);
    }

    double angle_step = 2 * M_PI / n_players;
    double a = (width  * 0.75) / 2.0;
    double b = (height * 0.75) / 2.0;
    double center_x = width / 2.0;
    double center_y = height / 2.0;

    for(unsigned int i = 0; i < n_players; i++) {
        double angle = i * angle_step;
        int x_pos = (int)(center_x + a * cos(angle));
        int y_pos = (int)(center_y + b * sin(angle));
        game_t_ptr->players[i].x_coord = x_pos; 
        game_t_ptr->players[i].y_coord = y_pos;
        game_t_ptr->board_p[y_pos * width + x_pos] = -1 * i;
    }

    return game_t_ptr;
}

player_movement get_move(game_t * game, int pipes[MAX_PLAYER_NUMBER][2], int player_count, unsigned int timeout) {
    static int last_player = 0; 
    fd_set read_fds;
    int max_fd = -1;

    struct timeval tv = {timeout, 0};

    FD_ZERO(&read_fds);

    int current_player = last_player;
    for(int i = 0; i < player_count; i++) {
        if (!game->players[current_player].is_blocked && !is_pipe_closed(pipes[current_player][READ_END])) {
            FD_SET(pipes[current_player][READ_END], &read_fds);
            if (pipes[current_player][READ_END] > max_fd) {
                max_fd = pipes[current_player][READ_END];
            }
        }
        current_player = (current_player + 1) % player_count;
    }

    if (max_fd == -1) {
        return (player_movement){-1, NONE};
    }

    int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

    if (ready == -1) {
        IF_EXIT(true, "select")
    } else if (ready == 0) {
        return (player_movement){-1, NONE};
    } else {
        current_player = last_player;
        for(int i = 0; i < player_count; i++) {
            if (!game->players[current_player].is_blocked && 
                FD_ISSET(pipes[current_player][READ_END], &read_fds)) {
                
                unsigned char move;
                ssize_t bytes_read = read(pipes[current_player][READ_END], &move, sizeof(move));
                
                if (bytes_read == -1) {
                    IF_EXIT(true, "read")
                } else if (bytes_read == 0) {
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

static inline bool is_player_blocked(const int board[], int x, int y, int width, int height) {
    for (int i = UP; i <= UP_LEFT; i++) {
        int check_x = x + dx[i];
        int check_y = y + dy[i];
        if (is_valid_move(check_x, check_y, width, height, board)) {
            return false;
        }
    }
    return true;
}

bool process_move(game_t * game, player_movement player_mov, time_t* last_valid_mov_time, unsigned int timeout){
    if(player_mov.player_id == -1 || time(NULL) - *last_valid_mov_time > timeout) { 
        return true;
    }

    if(player_mov.move < 0 || player_mov.move > 7 || game->players[player_mov.player_id].is_blocked) { 
        game->players[player_mov.player_id].invalid_mov_requests++;
        return false;
    }

    int new_x = game->players[player_mov.player_id].x_coord + dx[player_mov.move];
    int new_y = game->players[player_mov.player_id].y_coord + dy[player_mov.move];

    if(new_x < 0 || new_x >= game->board_width || new_y < 0 || new_y >= game->board_height || game->board_p[new_y * game->board_width + new_x] <= 0) {
        game->players[player_mov.player_id].invalid_mov_requests++;
    } else {
        *last_valid_mov_time = time(NULL);
        game->players[player_mov.player_id].valid_mov_request++;
        game->players[player_mov.player_id].x_coord = new_x;
        game->players[player_mov.player_id].y_coord = new_y; 
        game->players[player_mov.player_id].score += game->board_p[new_y * game->board_width + new_x];
        game->board_p[new_y * game->board_width + new_x] = -1 * player_mov.player_id; 
    }

    bool player_blocked = is_player_blocked(game->board_p, game->players[player_mov.player_id].x_coord, game->players[player_mov.player_id].y_coord, game->board_width, game->board_height);

    if(player_blocked){
        game->players[player_mov.player_id].is_blocked = true;
        for(unsigned int i = 0; i < game->player_number; i++){
            if(!(game->players[i].is_blocked = is_player_blocked(game->board_p, game->players[i].x_coord, game->players[i].y_coord, game->board_width, game->board_height))){
                return false;
            }
        }
        return true;
    }
    return false;
}