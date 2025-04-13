#ifndef VALID_MOVE_H
#define VALID_MOVE_H

#include <stdbool.h>

static inline bool is_valid_move(int x, int y, int width, int height, const int board[] ) {
    return (x >= 0 && x < width && y >= 0 && y < height && board[y * width + x] > 0);
}

#endif // VALID_MOVE_H