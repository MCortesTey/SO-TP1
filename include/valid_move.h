// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef VALID_MOVE_H
#define VALID_MOVE_H

#include <stdbool.h>

static inline bool is_valid_move(int x, int y, int width, int height, const int board[] ) {
    return (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0);
}

#endif // VALID_MOVE_H