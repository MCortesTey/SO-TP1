// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

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

    close(fd); 

    return p;
}

void* connect_shm(char *name, size_t size, int flags) {
    IF_EXIT(!(flags == O_RDONLY || flags == O_RDWR), "Modo de apertura no permitido para la memoria compartida");

    // open mem
    int shm_fd = shm_open(name, flags, 0); // if shm already exists, open it
    IF_EXIT(shm_fd < 0, "Error al abrir la memoria compartida");

    // mep shm to size
    void* return_ptr = mmap(0, size, (flags == O_RDWR) ? (PROT_READ | PROT_WRITE) : PROT_READ, MAP_SHARED, shm_fd, 0);
    IF_EXIT(return_ptr == MAP_FAILED, "Error al mapear la memoria compartida");

    close(shm_fd);

    return return_ptr;
}

void destroy_shm(void* ptr, size_t size, const char* name) {
    IF_EXIT_NON_ZERO(munmap(ptr, size), "Error al desmapear la memoria compartida");
    IF_EXIT_NON_ZERO(shm_unlink(name), "Error al eliminar la memoria compartida");
}

void unmap_shm(void* ptr, size_t size) {
    if (ptr != MAP_FAILED) {
        IF_EXIT(munmap(ptr, size) == -1, "Error al desmapear la memoria compartida");
    }
}
    