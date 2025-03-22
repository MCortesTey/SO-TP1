#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "shared_memory.h"

int main(int argc, char *argv[]  ){
    if ( argc != 3 ) {
        fdprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]); 

    game_sync * sync = 

    
    game_t *game_state = connect_shm (SHM_GAME_PATH, sizeof(game_t) )
    
    sem_t * sem = sem_open( SEM_GAME_PATH ,0);
    if ( sem == SEM_FAILED) {
        perror("Error al acceder al semaforo");
        munmap(game_state, sizeof(game_t));
        close(sem);
        return EXIT_FAILURE;
    }







    return EXIT_SUCCESS;
}