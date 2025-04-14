// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "sems.h"
#include "macros.h"
#include <stdio.h>
#include <stdlib.h>

void init_shared_sem(sem_t * sem, int initial_value){
    IF_EXIT_NON_ZERO(sem_init(sem, 1, initial_value),"sem_init")
}

void destroy_shared_sem(sem_t * sem){
    IF_EXIT_NON_ZERO(sem_destroy(sem),"sem_destroy")
}

void wait_shared_sem(sem_t * sem){
    IF_EXIT_NON_ZERO(sem_wait(sem),"sem_wait")
}

void post_shared_sem(sem_t * sem){
    IF_EXIT_NON_ZERO(sem_post(sem),"sem_post")
}
