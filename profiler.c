#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <time.h>
#include <signal.h>
#include <syscall.h>

#define SAMPLE_HZ 2

struct user_regs_struct regs;
struct timespec start;
pid_t child_pid;
int status;

double get_elapsed_time() {
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

// periodically pauses child and print program counter
void sample(pid_t pid) {
    int ms_between_samples = 1.0 / SAMPLE_HZ * 1000;
    struct timespec ts;
    ts.tv_sec = ms_between_samples / 1000;
    ts.tv_nsec = (ms_between_samples % 1000) * 1000000;
    for (int i = 0; i < 10; i++) {
        ptrace(PTRACE_INTERRUPT, pid, NULL, NULL);
        waitpid(pid, &status, 0);
        ptrace(PTRACE_GETREGS, pid, NULL, &regs);
        sleep(10);
        ptrace(PTRACE_CONT, pid, NULL, NULL);
        printf("Child PC: %llx\n", regs.rip);
        nanosleep(&ts, &ts);
    }
}

int main() {
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
     

    if (child_pid == 0) {
        // Child process
        ptrace(PTRACE_TRACEME);
        execl("test", "test", NULL);
        exit(1);
    } else {
        // Parent process
        // Wait for child to stop at exec call
        wait(&status);
        // Tell child to not stop at system calls
        ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);
        // Continue through exec call
        ptrace(PTRACE_CONT, child_pid);
        struct user_regs_struct regs;
        for (int i = 0; i < 10; i++) {
            kill(child_pid, SIGSTOP);
            waitpid(child_pid, &status, WUNTRACED);
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
            printf("Child PC = %p\n", (void*)regs.rip);
            ptrace(PTRACE_CONT, child_pid);
            sleep(3);
        }
        wait(&status);
    }
    return 0;
}
