#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "shm_utils.h"
#include "constants.h"
#include "shared_memory.h"

int main(int argc, char *argv[]  ){
    if ( argc != 3 ) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]); 

    game_sync *sync = connect_shm(SHM_GAME_SEMS_PATH, sizeof(game_sync));
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t));

    // Loop principal de la vista
    while (true) {
        // Esperar señal del master indicando que hay cambios para imprimir
        sem_wait(&sync->print_needed);

        // TODO: Implementar la lógica de impresión

        // Señalar al master que terminamos de imprimir
        sem_post(&sync->print_done);

        // Verificar si el juego ha terminado
        if (game_state->has_finished) {
            break;
        }
    }

    // Cleanup
    munmap(sync, sizeof(game_sync));
    munmap(game_state, sizeof(game_t));
    return EXIT_SUCCESS;
}