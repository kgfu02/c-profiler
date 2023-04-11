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
        execl("./test", "test", NULL);
        exit(1);
    } else {
        // Parent process
        printf("Child process started, pid = %d\n", child_pid);
        ptrace(PTRACE_SEIZE, child_pid, NULL, NULL);
        sleep(1);
        printf("Child process paused\n");
        ptrace(PTRACE_INTERRUPT, child_pid, NULL, NULL);
        sleep(10);
        ptrace(PTRACE_DETACH, child_pid, NULL, NULL); // Detach from child process
        printf("Child process resumed\n");
        waitpid(child_pid, &status, 0); // Wait for child to exit
        printf("Child process exited with status %d\n", WEXITSTATUS(status));
        exit(EXIT_SUCCESS);
    }
}
