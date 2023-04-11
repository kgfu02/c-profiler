#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <time.h>
#include <signal.h>
#include <syscall.h>

struct user_regs_struct regs;
struct timespec start;

double get_elapsed_time() {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) +
                   (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

void resume_until_exec(pid_t pid) {
    // Resume child until it reaches the exec call
    while (1) {
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        int status;
        waitpid(pid, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            // Check if the child has made the exec call
            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            if (regs.orig_rax == SYS_execve) {
                break;
            }
        }
    }
}

int main() {
    pid_t child_pid;
    int status;

    child_pid = fork();
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl("./test", "test", NULL);
        exit(1);
    } else {
        // Parent process
        printf("Child process started, pid = %d\n", child_pid);
        resume_until_exec(child_pid);
        ptrace(PTRACE_INTERRUPT, child_pid, NULL, NULL);
        printf("Child process paused...\n");
        sleep(5);
        ptrace(PTRACE_DETACH, child_pid, NULL,
               NULL);  // Detach from child process
        printf("Child process resumed...\n");
        waitpid(child_pid, &status, 0);  // Wait for child to exit
        printf("Child ran for %f secs.", get_elapsed_time());
        exit(EXIT_SUCCESS);
    }
}
