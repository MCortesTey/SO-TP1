#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "shm_utils.h"
#include "constants.h"

int main(int argc, char *argv[]  ){
    if ( argc != 3 ) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]); 

    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t));
    if ((int)game_state == EXIT_FAILURE) {
        perror("Error al conectar a la memoria compartida del juego");
        return EXIT_FAILURE;
    }

    // No necesitamos inicializar los semáforos porque el master ya lo hizo con sem_init
    // Solo necesitamos acceder a los semáforos que ya existen en la memoria compartida
    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync));
    if ((int)sync == EXIT_FAILURE) {
        perror("Error al conectar a la memoria compartida de sincronización");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}