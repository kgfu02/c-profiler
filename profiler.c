#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

int main()
{
    pid_t child_pid;
    int status;

    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        struct timespec start, end;
        double elapsed_time;

        clock_gettime(CLOCK_MONOTONIC, &start);
        printf("sleeping!\n");
        sleep(5);
        printf("Done sleeping!\n");
        clock_gettime(CLOCK_MONOTONIC, &end);

        elapsed_time = (end.tv_sec - start.tv_sec) +
                    (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
        printf("Elapsed time: %f seconds\n", elapsed_time);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        printf("Child process started, pid = %d\n", child_pid);
       //waitpid(child_pid, &status, 0); // Wait for child to stop
        ptrace(PTRACE_ATTACH, child_pid, NULL, NULL); // Pause child process
        printf("Child process paused\n");
        sleep(3);
        ptrace(PTRACE_DETACH, child_pid, NULL, NULL); // Detach from child process
        printf("Child process resumed\n");
        waitpid(child_pid, &status, 0); // Wait for child to exit
        printf("Child process exited with status %d\n", WEXITSTATUS(status));
        exit(EXIT_SUCCESS);
    }
}
