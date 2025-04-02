#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "shm_ADT.h"
#include "macros.h"


void* create_shm(char *name, size_t size, int mode) {
    int fd = shm_open(name, O_RDWR | O_CREAT, mode);
    IF_EXIT(fd == -1, "shm_open");

    IF_EXIT(ftruncate(fd, size) == -1, "ftruncate");

    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    IF_EXIT(p == MAP_FAILED, "mmap");

    close(fd);  // No necesitamos el descriptor después de mapear

    return p;
}

void* connect_shm(char *name, size_t size, int flags) {
    IF_EXIT(!(flags == O_RDONLY || flags == O_RDWR), "Modo de apertura no permitido para la memoria compartida");

    // Abrir la memoria
    int shm_fd = shm_open(name, flags, 0); // Si la memoria ya existe (no se usa O_CREAT), los permisos definidos en "int mode" no se modifican
    IF_EXIT(shm_fd < 0, "Error al abrir la memoria compartida");

    // Mapear la memoria compartida con el tamaño correcto
    void* return_ptr = mmap(0, size, (flags == O_RDWR) ? (PROT_READ | PROT_WRITE) : PROT_READ, MAP_SHARED, shm_fd, 0);
    IF_EXIT(return_ptr == MAP_FAILED, "Error al mapear la memoria compartida");

    close(shm_fd); // Cerrar el descriptor de archivo después del mmap

    return return_ptr;
}
