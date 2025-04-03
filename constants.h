// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define SHM_GAME_PATH "/game_state"
#define SHM_GAME_SEMS_PATH "/game_sync"

enum MOVEMENTS {UP = 0, UP_RIGHT, RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT, NONE};
const int dx[] = { 0,  1, 1, 1, 0, -1, -1, -1};
const int dy[] = {-1, -1, 0, 1, 1,  1,  0, -1};

#endif // CONSTANTS_H