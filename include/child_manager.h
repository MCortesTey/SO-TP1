#ifndef CHILD_MANAGER_H
#define CHILD_MANAGER_H

#include <sys/types.h>

pid_t new_child(char *executable_path, char *args[], int write_fd, int read_fd, int fork_num, int forks[][2]);
int wait_for_child(pid_t pid);

#endif // CHILD_MANAGER_H