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
        IF_EXIT(write_fd != -1 && (dup2(write_fd, STDOUT_FILENO) == -1), "dup2");
        IF_EXIT(read_fd != -1 && (dup2(read_fd, STDIN_FILENO) == -1), "dup2");
        
        IF_EXIT(write_fd != -1 && close(write_fd) != 0,"close")
        IF_EXIT(read_fd != -1 && close(read_fd) != 0,"close")
        
        IF_EXIT(access(executable_path, X_OK) == -1, "access");
        IF_EXIT(args == NULL || args[0] == NULL, "args");

        if (forks != NULL) {
            for (int i = 0; i < fork_num; i++) {
                if (forks[i][READ_END] != read_fd && forks[i][READ_END] != write_fd) {
                    close(forks[i][READ_END]); // close unused read ends
                }
                if (forks[i][WRITE_END] != read_fd && forks[i][WRITE_END] != write_fd) {
                    close(forks[i][WRITE_END]); // close unused write ends
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

int wait_for_child(pid_t pid){
    int status;
    pid_t result;
    if((result = waitpid(pid, &status, 0)) == -1){
        perror("waitpid");
        return -1;
    }

    // process the status with macros
    // src: https://users.pja.edu.pl/~jms/qnx/help/watcom/clibref/qnx/waitpid.html#:~:text=Returns%3A,errno%20is%20set%20to%20EINTR.
    if (WIFEXITED(status)) { // if the process exited...
        return WEXITSTATUS(status); 
    } 

    return status;
}