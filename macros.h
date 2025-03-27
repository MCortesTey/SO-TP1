#ifndef MACROS_H
#define MACROS_H

#include <stdio.h> 
#include <stdlib.h>

#define IF_EXIT(condition,name) if((condition)) { perror(name); exit(EXIT_FAILURE); }
#define IS_NULL(ptr) ((ptr) == NULL)

#endif // MACROS_H