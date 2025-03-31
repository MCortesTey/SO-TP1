// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef MACROS_H
#define MACROS_H

#include <stdio.h> 
#include <stdlib.h>

#define IF_EXIT(condition,name) if((condition)) { perror(name); exit(EXIT_FAILURE); }
#define IS_NULL(ptr) ((ptr) == NULL)

#endif // MACROS_H