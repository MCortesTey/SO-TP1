// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef SHM_UTILS_H
#define SHM_UTILS_H

#include <stddef.h>

void* create_shm(char *name, size_t size, int mode);

void* connect_shm(char * name, size_t size, int flags);
#endif // SHM_H