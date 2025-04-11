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
        return EXIT_FAILURE;
    }
    if(pid == 0){ 
        if(write_fd != -1 && (dup2(write_fd, STDOUT_FILENO) == -1)){ 
            perror("dup2");
            return EXIT_FAILURE;
        }
        if(read_fd != -1 && (dup2(read_fd, STDIN_FILENO) == -1)) { 
            perror("dup2");
            return EXIT_FAILURE;
        } 

        if(write_fd != -1 && close(write_fd) != 0){ 
            perror("close");
            return EXIT_FAILURE;
        }
        if(read_fd != -1 && close(read_fd) != 0){ 
            perror("close");
            return EXIT_FAILURE;
        }

        if(access(executable_path, X_OK) == -1){
            perror("access");
            return EXIT_FAILURE;
        }

        if(args == NULL || args[0] == NULL){
            perror("execv");
            return EXIT_FAILURE;
        }

        for(int i = 0; i < fork_num && forks != NULL; i++){
            if(forks[i][READ_END] != read_fd && forks[i][READ_END] != write_fd) close(forks[i][READ_END]); // close unused read ends
            if(forks[i][WRITE_END] != read_fd && forks[i][WRITE_END] != write_fd) close(forks[i][WRITE_END]); // close unused write ends
        }
        execv(executable_path, args);
        IF_EXIT(true,"execv child") 
    } else if(pid > 0){ 
        // close unused pipe
        if(write_fd != -1) close(write_fd);
        if(read_fd != -1) close(read_fd);
    }
    return pid;
}

int wait_for_child(pid_t pid){
    int status;
    pid_t result;
    IF_EXIT((result = waitpid(pid, &status, WNOHANG)) == -1, "waitpid")
    return status;
}