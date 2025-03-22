#include "shm_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>           // Para O_* constantes
#include <sys/mman.h>       // Para shm_open y mmap
#include <sys/stat.h>       // Para mode constants
#include <semaphore.h>       // Para semaforos POSIX
#include <unistd.h>         // Para sleep
#include "shared_memory.h"
#include "constants.h"

// Estructura para el estado del tablero

//principal del jugador

int main(int argc, char *argv[]) {
    
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    if(width == 0){
        perror("atoi width");
        exit(EXIT_FAILURE);
    }

    int height = atoi(argv[2]);
    if(height == 0){
        perror("atoi height");
        exit(EXIT_FAILURE);
    }


    // Conexion a la memoria compartida
    game_t *game_state = connect_shm(SHM_GAME_PATH, sizeof(game_t));

    // Inicializar el estado del tablero
    //board->width = width;
    //board->height = height;
    // ... inicializar otros campos del tablero ...

    sem_t * sem = sem_open(SHM_GAME_SEMS_PATH, 0);  // Note el 0 en lugar de O_CREAT
    if (sem == SEM_FAILED) {
        perror("Error: No se pudo acceder al semaforo");
        munmap(game_state, sizeof(game_t));
        close(shm_fd);
        return EXIT_FAILURE;
    }

    // Bucle principal del jugador
    while (1) {
        // Esperar a que el semaforo este disponible
        sem_wait(sem);

        // Consultar el estado del tablero de forma sincronizada
        // ... codigo para consultar el estado del tablero ...

        // Enviar solicitudes de movimientos al master
        // ... codigo para enviar solicitudes de movimiento ...

        // Comprobar si el jugador esta bloqueado
        // ... codigo para verificar el estado de bloqueo ...

        // Liberar el semaforo
        sem_post(sem);

        // Esperar un tiempo antes de la siguiente consulta
        sleep(1);
    }

    // Desconectar de la memoria compartida
    munmap(game_state, sizeof(game_t));
    shm_unlink(SHM_GAME_PATH);
    sem_close(sem);
    sem_unlink(SEM_NAME); // game_sync no es un sem, es un struct de sems.

    return EXIT_SUCCESS;
}