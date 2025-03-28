#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <wchar.h>
#include <locale.h>
#include "shm_utils.h"
#include "constants.h"
#include "shared_memory.h"

//colores en ANSI
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"
#define COLOR_PURPLE  "\x1b[95m"
#define COLOR_ORANGE  "\x1b[38;5;208m"

// Array de colores para los jugadores
const char* player_colors[] = {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_PURPLE,
    COLOR_ORANGE,
    COLOR_RED,   
    COLOR_GREEN
};

// Función auxiliar para limpiar la pantalla
void clear_screen() {
    printf("\033[2J\033[H");  // Secuencia ANSI para limpiar pantalla y mover cursor
}

// Función para imprimir el tablero
void print_board(game_t *game_state) {
    clear_screen();
    
    // Imprimir borde superior
    printf("┌");
    for(int x = 0; x < game_state->board_width; x++) {
        printf("─");
    }
    printf("┐\n");

    // Imprimir contenido del tablero
    for(int y = 0; y < game_state->board_height; y++) {
        printf("│");  // Borde izquierdo
        for(int x = 0; x < game_state->board_width; x++) {
            int cell = game_state->board_p[y * game_state->board_width + x];
            if(cell > 0) {
                printf("%d",cell);  // Celda vacía. Debería ser puntaje
            } else {
                //printf("%s",player_colors[-cell]);
                //wprintf(L"█\n");
                printf("%s%d%s", player_colors[-cell], -cell+1, COLOR_RESET);
                //printf("%s",COLOR_RESET);
            }
        }
        printf("│\n");  // Borde derecho
    }

    // Imprimir información de los jugadores con sus respectivos colores
    printf("\nJugadores:\n");
    for(unsigned int i = 0; i < game_state->player_number; i++) {
        player_t player = game_state->players[i];
        if(player.pid != 0) {  // Si el jugador está activo
            printf("%sJugador %d%s (%s): Score=%d, Pos=(%d,%d)%s\n", 
                player_colors[i],
                i + 1, 
                COLOR_RESET,
                player.name, 
                player.score, 
                player.x_coord, 
                player.y_coord,
                player.is_blocked ? " [BLOQUEADO]" : "");
        }
    }
    fflush(stdout);  // Asegurar que se imprima inmediatamente
}

int main(int argc, char *argv[] ){
    if ( argc != 3 ) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //int width = atoi(argv[1]);
    //int height = atoi(argv[2]); 

    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync), O_RDWR);
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t), O_RDONLY);

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