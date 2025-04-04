// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef SHM_ADT_H
#define SHM_ADT_H

#include <stddef.h>
#include <stdint.h>
#include <semaphore.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_NAME_LEN 16
#define MAX_PLAYERS 9

typedef struct {
    char name[MAX_NAME_LEN];            // Nombre del jugador
    unsigned int score;                 // Puntaje
    unsigned int invalid_mov_requests;  // Cantidad de solicitudes de movimientos inválidas realizadas
    unsigned int valid_mov_request;     // Cantidad de solicitudes de movimientos válidas realizadas
    unsigned short x_coord, y_coord;    // Coordenadas x e y en el tablero
    pid_t pid;                          // Identificador de proceso
    bool is_blocked;                    // Indica si el jugador está bloqueado
} player_t;

typedef struct {
    unsigned short board_width;     // Ancho del tablero
    unsigned short board_height;    // Alto del tablero
    unsigned int player_number;     // Cantidad de jugadores
    player_t players[MAX_PLAYERS];  // Lista de jugadores
    bool has_finished;              // Indica si el juego se ha terminado
    int board_p[];                  // Puntero al comienzo del tablero. fila-0, fila-1, ..., fila-n-1. Flexible Array.
} game_t;

typedef struct {
    sem_t print_needed;         // Se usa para indicarle a la vista que hay cambios por imprimir
    sem_t print_done;           // Se usa para indicarle al master que la vista terminó de imprimir
    sem_t master_access_mutex;  // Mutex para evitar inanición del master al acceder al estado
    sem_t game_state_mutex;     // Mutex para el estado del juego
    sem_t reader_count_mutex;   // Mutex para la siguiente variable
    unsigned int readers_count; // Cantidad de jugadores leyendo el estado
} game_sync;


void* create_shm(char* name, size_t size, int mode);
void* connect_shm(char* name, size_t size, int flags);
void destroy_shm(void* ptr, size_t size, const char* name);
void unmap_shm(void* ptr, size_t size);

#endif // SHM_ADT_H 