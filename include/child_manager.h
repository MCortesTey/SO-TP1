// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef CHILD_MANAGER_H
#define CHILD_MANAGER_H

#include <sys/types.h>
#include <stdbool.h>

pid_t new_child(char *executable_path, char *args[], int write_fd, int read_fd, int fork_num, int forks[][2]);
bool wait_for_child(pid_t pid, int* status);

#endif // CHILD_MANAGER_H