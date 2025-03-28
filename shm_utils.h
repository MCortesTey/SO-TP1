#ifndef SHM_UTILS_H
#define SHM_UTILS_H

#include <stddef.h>

void* create_shm(char *name, size_t size, int mode);

void* connect_shm(char * name, size_t size, int flags);
#endif // SHM_H