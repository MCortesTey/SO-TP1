// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <wchar.h>
#include <locale.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "shm_utils.h"
#include "constants.h"
#include "shared_memory.h"

// limpiar pantalla
#define CLEAR_SCREEN printf("\033[2J\033[H")

// Colores de fondo para los jugadores
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[41m"    // Fondo rojo
#define COLOR_GREEN   "\x1b[42m"    // Fondo verde
#define COLOR_YELLOW  "\x1b[43m"        // Fondo amarillo
#define COLOR_BLUE    "\x1b[44m"        // Fondo azul
#define COLOR_MAGENTA "\x1b[45m"        // Fondo magenta
#define COLOR_CYAN    "\x1b[46m"        // Fondo cyan
#define COLOR_BROWN   "\x1b[48;5;130m"  // Fondo marrón
#define COLOR_WHITE   "\x1b[47m"        // Fondo blanco
#define COLOR_PURPLE  "\x1b[105m"       // Fondo púrpura brillante
#define COLOR_ORANGE  "\x1b[48;5;208m"  // Fondo naranja

// Colores de fondo brillantes para las cabezas de los jugadores
#define BRIGHT_RED     "\x1b[101m"   // Fondo rojo brillante
#define BRIGHT_GREEN   "\x1b[102m"   // Fondo verde brillante
#define BRIGHT_YELLOW  "\x1b[103m"   // Fondo amarillo brillante
#define BRIGHT_BLUE    "\x1b[104m"   // Fondo azul brillante
#define BRIGHT_MAGENTA "\x1b[105m"   // Fondo magenta brillante
#define BRIGHT_CYAN    "\x1b[106m"   // Fondo cyan brillante
#define BRIGHT_BROWN   "\x1b[48;5;173m"  // Fondo marrón brillante
#define BRIGHT_PURPLE  "\x1b[48;5;213m"  // Fondo púrpura más brillante
#define BRIGHT_ORANGE  "\x1b[48;5;214m"  // Fondo naranja brillante

// Color para los números (texto negro sobre fondo blanco)
#define SCORE_COLOR   "\x1b[30;47m"  // Texto negro sobre fondo blanco

// Array de colores para los jugadores
const char* player_colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_BROWN,
    COLOR_PURPLE,
    COLOR_ORANGE,
    COLOR_RED,   
    COLOR_GREEN
};

// Array de colores brillantes para las cabezas de los jugadores
const char* bright_player_colors[] = {
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_BROWN,
    BRIGHT_PURPLE,
    BRIGHT_ORANGE
};

// Colores de texto para la información de jugadores
#define TEXT_RED     "\x1b[31m"
#define TEXT_GREEN   "\x1b[32m"
#define TEXT_YELLOW  "\x1b[33m"
#define TEXT_BLUE    "\x1b[34m"
#define TEXT_MAGENTA "\x1b[35m"
#define TEXT_CYAN    "\x1b[36m"
#define TEXT_BROWN   "\x1b[38;5;130m"
#define TEXT_WHITE   "\x1b[37m"
#define TEXT_PURPLE  "\x1b[95m"
#define TEXT_ORANGE  "\x1b[38;5;208m"

// Array de colores de texto para la información de jugadores
const char* text_colors[] = {
    TEXT_RED,
    TEXT_GREEN,
    TEXT_YELLOW,
    TEXT_BLUE,
    TEXT_MAGENTA,
    TEXT_CYAN,
    TEXT_BROWN,
    TEXT_PURPLE,
    TEXT_ORANGE
};

// Función para imprimir el tablero
void print_board(game_t *game_state) {
    CLEAR_SCREEN;
    
    // Imprimir borde superior
    printf("┌");
    for(int x = 0; x < game_state->board_width * 3; x++) {
        printf("─");
    }
    printf("┐\n");

    // Imprimir contenido del tablero
    for(int y = 0; y < game_state->board_height; y++) {
        printf("│");  // Borde izquierdo
        for(int x = 0; x < game_state->board_width; x++) {
            int cell = game_state->board_p[y * game_state->board_width + x];
            if(cell > 0) {
                // Celda con puntaje - número negro sobre fondo blanco
                printf("%s%2d %s", SCORE_COLOR, cell, COLOR_RESET);
            } else {
                // Jugador - bloque de color sólido
                int player_idx = -1 * cell; 
                int is_head = 0;//Me fijo si es cabeza
                
                player_t player = game_state->players[player_idx];
                is_head = (x == player.x_coord && y == player.y_coord);
                
                printf("%s   %s", is_head ? bright_player_colors[player_idx] : player_colors[player_idx], COLOR_RESET);
            }
        }
        printf("│\n");  // Borde derecho
    }

    // Imprimir borde inferior
    printf("└");
    for(int x = 0; x < game_state->board_width * 3; x++) {
        printf("─");
    }
    printf("┘\n");

    // Imprimir información de los jugadores con sus respectivos colores
    printf("\nJugadores:\n");
    for(unsigned int i = 0; i < game_state->player_number; i++) {
        player_t player = game_state->players[i];
        if(player.pid != 0) {  // Si el jugador está activo
            printf("%sJugador %u%s (%s): Score=%u, Pos=(%d,%d)%s\n", 
                text_colors[i],
                i, 
                COLOR_RESET,
                player.name, 
                player.score, 
                player.x_coord, 
                player.y_coord,
                player.is_blocked ? " [BLOQUEADO]" : "");
        }
    }
    fflush(stdout);
}

int main(int argc, char *argv[] ){
    if ( argc != 3 ) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]); 

    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t) + sizeof(int)*(width*height), O_RDONLY);

    // Loop principal de la vista
    while (!game_state->has_finished) {
        // Esperar señal del master indicando que hay cambios para imprimir
        sem_wait(&sync->print_needed);
        
        // Imprimir el estado actual del tablero
        print_board(game_state);

        // Señalar al master que terminamos de imprimir
        sem_post(&sync->print_done);
    }

    // Cleanup
    munmap(sync, sizeof(game_sync));
    munmap(game_state, sizeof(game_t));
    return EXIT_SUCCESS;
}