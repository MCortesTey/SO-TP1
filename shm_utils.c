#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "macros.h"


void* create_shm(char * name, size_t size){ // void* es un puntero. mismo retorno que malloc
	int fd = shm_open(name, O_RDWR | O_CREAT , 0666); // O_RDWR | O_CREAT --> quiero crear, con permiso de lectura y escritura.

	IF_EXIT(fd == -1, "shm_open")
	
	IF_EXIT(ftruncate(fd, size) == -1,"ftruncate")

	void *p = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0); // si saco el fd los demás parámetros no tiene relación!
	
	IF_EXIT(p == MAP_FAILED,"mmap")
	
	return p;
}

void* connect_shm(char * name, size_t size){
    // Intentar abrir la memoria compartida existente
    int shm_fd = shm_open(name, O_RDWR, 0666);

	IF_EXIT(shm_fd < 0, "Error al abrir la memoria compartida")

    // Mapear la memoria compartida
    void* return_ptr = mmap(0, sizeof(size), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
	IF_EXIT(return_ptr == MAP_FAILED,"Error al mapear la memoria compartida")
	
	return return_ptr;
}
