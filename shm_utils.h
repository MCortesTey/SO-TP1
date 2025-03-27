#ifndef SHM_UTILS_H
#define SHM_UTILS_H

#include <stddef.h>

void* create_shm(char *name, size_t size);

void* connect_shm(char * name, size_t size);
#endif // SHM_H