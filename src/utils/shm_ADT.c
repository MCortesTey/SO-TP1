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
    if (fd == -1) {
        perror("shm_open");
        return NULL;
    }

    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(name);
        return NULL;
    }

    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(name);
        return NULL;
    }

    close(fd); 
    return p;
}

void* connect_shm(char *name, size_t size, int flags) {
    if (!(flags == O_RDONLY || flags == O_RDWR)) {
        fprintf(stderr, "Modo de apertura no permitido para la memoria compartida\n");
        return NULL;
    }

    int shm_fd = shm_open(name, flags, 0);
    if (shm_fd < 0) {
        perror("Error al abrir la memoria compartida");
        return NULL;
    }

    void* return_ptr = mmap(0, size, (flags == O_RDWR) ? (PROT_READ | PROT_WRITE) : PROT_READ, MAP_SHARED, shm_fd, 0);
    if (return_ptr == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return return_ptr;
}

void destroy_shm(void* ptr, size_t size, const char* name) {
    if (ptr == NULL) {
        return;
    }

    if (munmap(ptr, size) != 0) {
        perror("Error al desmapear la memoria compartida");
    }
    if (shm_unlink(name) != 0) {
        perror("Error al eliminar la memoria compartida");
    }
}

void unmap_shm(void* ptr, size_t size) {
    if (ptr != MAP_FAILED && ptr != NULL) {
        if (munmap(ptr, size) == -1) {
            perror("Error al desmapear la memoria compartida");
        }
    }
}
    
    