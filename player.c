#include "shm_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>           // Para O_* constantes
#include <sys/mman.h>       // Para shm_open y mmap
#include <sys/stat.h>       // Para mode constants
#include <semaphore.h>       // Para semaforos POSIX
#include <unistd.h>         // Para sleep
#include <time.h>
#include "shared_memory.h"
#include "constants.h"
#include "macros.h"

// Estructura para el estado del tablero

// Principal del jugador

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    srand(getpid()); // provisorio, después ver como tunearlo para que sea más random aún

    int width = atoi(argv[1]);
    IF_EXIT(width <= 0, "ancho inválido")

    int height = atoi(argv[2]);
    IF_EXIT(height <= 0, "alto inválido")

    // Conectar a la memoria compartida del juego
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t));

    // Conectar a la memoria compartida de sincronización
    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync));

    // Bucle principal del jugador
    while (!game_state->has_finished) {
        
    }

    // Limpieza
    munmap(game_state, sizeof(game_t));
    munmap(sync, sizeof(game_sync));

    return EXIT_SUCCESS;
}