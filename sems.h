#ifndef SEMS_H
#define SEMS_H

#include <semaphore.h>

void init_shared_sem(sem_t * sem, int initial_value);
void destroy_shared_sem(sem_t * sem);
void wait_shared_sem(sem_t * sem);
void post_shared_sem(sem_t * sem);

#endif // SEMS_H