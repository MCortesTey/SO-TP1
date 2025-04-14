// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef PLAYER_STRATEGIES_H
#define PLAYER_STRATEGIES_H

#include "constants.h"

unsigned char generate_move(int width, int height, const int board[], int x_pos, int y_pos);

#endif // PLAYER_STRATEGIES_H