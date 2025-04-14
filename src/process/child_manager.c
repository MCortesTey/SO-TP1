// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "child_manager.h"
#include "macros.h"
#include "constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>



pid_t new_child(char *executable_path, char *args[], int write_fd, int read_fd, int fork_num, int forks[][2]){
    pid_t pid = fork();
    if(pid == -1){
        perror("fork");
        return -1;
    }
    if(pid == 0){ 
        if(write_fd != -1 && (dup2(write_fd, STDOUT_FILENO) == -1)) {
            perror("dup2");
            return -1;
        }

        if(read_fd != -1 && (dup2(read_fd, STDIN_FILENO) == -1)) {
            perror("dup2");
            return -1;
        }

        if(write_fd != -1 && close(write_fd) != 0){
            perror("close");
            return -1;
        }
        if (read_fd != -1 && close(read_fd) != 0) {
            perror("close");
            return -1;
        }
        
        if (access(executable_path, X_OK) == -1) {
            perror("access");
            return -1;
        }

        if (args == NULL || args[0] == NULL) {
            perror("args");
            return -1;
        }

        if (forks != NULL) {
            for (int i = 0; i < fork_num; i++) {
                if (forks[i][READ_END] != read_fd && forks[i][READ_END] != write_fd) {
                    close(forks[i][READ_END]); 
                }
                if (forks[i][WRITE_END] != read_fd && forks[i][WRITE_END] != write_fd) {
                    close(forks[i][WRITE_END]); 
                }
            }
        }
        execv(executable_path, args);
        perror("execv child");
        exit(EXIT_FAILURE);
    } else if(pid > 0){ 
        // close unused pipe
        if(write_fd != -1) close(write_fd);
        if(read_fd != -1) close(read_fd);
    }
    return pid;
}

bool wait_for_child(pid_t pid, int *status){
    pid_t result;
    if((result = waitpid(pid, status, WNOHANG)) == -1){
        perror("waitpid");
        return -1;
    }
    
    // result == 0 --> no terminó
    if(result == 0) {
        return false; 
    }

    if (WIFEXITED(*status)) { // Child terminated normally with exit()
        *status = WEXITSTATUS(*status);
        return true; 
    }

    return result;
}