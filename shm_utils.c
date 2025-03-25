#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


void* create_shm(char * name, size_t size){ // void* es un puntero. mismo retorno que malloc
	int fd;
	fd = shm_open(name, O_RDWR | O_CREAT , 0666); // O_RDWR | O_CREAT --> quiero crear, con permiso de lectura y escritura.

	if(fd == -1){
		perror("shm_open");
		exit(EXIT_FAILURE);
	}
	
	if(ftruncate(fd, size) == -1){
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}

	void *p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0); // si saco el fd los demás parámetros no tiene relación!
	if(p == MAP_FAILED){
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	
	return p;
}

void* connect_shm(char * name, size_t size){
	int shm_fd;
    // Intentar abrir la memoria compartida existente
    shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("Error al abrir la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Mapear la memoria compartida
    void* return_ptr = mmap(0, sizeof(size), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (return_ptr == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        exit(EXIT_FAILURE);
    }
	return return_ptr;
}
