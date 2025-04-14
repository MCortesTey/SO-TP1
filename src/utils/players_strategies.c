// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "constants.h"
#include "players_strategies.h"
#include "valid_move.h"

#ifdef JASON
#include <sys/types.h>
#include <signal.h>
#define BEST_SCORE
#endif

#ifdef FIRST_POSSIBLE
unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){ 
    for(int i = UP; i <= UP_LEFT; i++){
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        if (is_valid_move(x,y,width,height,board)) {
            return i; 
        }
    }
    return NONE;
}
#endif

#ifdef BEST_SCORE
unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    int best_move = NONE;
    int max_score = -1;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        if (is_valid_move(x, y, width, height, board) && board[y * width + x] > max_score) {
            max_score = board[y * width + x];
            best_move = i;
        }
    }
    return best_move; 
}
#endif 

#ifdef RANDOM
unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    int valid_moves[8];
    int num_valid_moves = 0;

    for (int i = UP; i <= UP_LEFT; i++) {
        int x = x_pos + dx[i], y = y_pos + dy[i];
        if (is_valid_move(x, y, width, height, board)) {
            valid_moves[num_valid_moves++] = i;
        }
    }

    if (num_valid_moves > 0) {
        return valid_moves[rand() % num_valid_moves];
    }
    return NONE;
}
#endif

#ifdef CLOCK
unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    static int i = UP;
    int original_i = i;

    do {
        i = i % 8;  
        int x = x_pos + dx[i];
        int y = y_pos + dy[i];
        
        if (is_valid_move(x, y, width, height, board)) {
            int move = i;
            i = (i + 1) % 8;  
            return move;
        }
        i++;
    } while (i != original_i); 
    
    return NONE;
}
#endif

#ifdef KILLER
unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){

    int nearest_player = -1;
    int min_distance = width * height;

    for (int j = 0; j < width * height; j++) {
        if (board[j] < 0) { 
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

            if (abs(difx) > abs(dify)) {
                mov_dir = (difx > 0) ? RIGHT : LEFT;
            } else if (abs(difx) < abs(dify)) {
                mov_dir = (dify > 0) ? DOWN : UP;
            } else {
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
        for(int i = UP; i <= UP_LEFT; i++){
            int x = x_pos + dx[i];
            int y = y_pos + dy[i];
            if (is_valid_move(x,y,width,height,board)) {
                return i; 
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
        if (is_valid_move(nx, ny, width, height, board)) {
            break; 
        }
        j++;
    }
    return movements[j];
}
#endif

#ifdef ERROR
unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos){
    (void)width;    
    (void)height;
    (void)board;
    (void)x_pos;
    (void)y_pos;
    return NONE;
}
#endif