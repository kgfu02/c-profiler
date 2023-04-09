#if defined(__APPLE__) // changes definitions for mac
#define PTRACE_TRACEME PT_TRACE_ME 
#define PTRACE_PEEKUSER PT_READ_U
#define PTRACE_CONT PT_CONTINUE
#define PTRACE_INTERRUPT PT_INTERRUPT
#define PTRACE_KILL PT_KILL
#define PTRACE_ATTACH PT_ATTACHEXC
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    pid_t child;
    child = fork();
    if (child == 0) {
        if (ptrace(PTRACE_TRACEME, 0, NULL, 0) == -1) {
            perror("ptrace");
            return 1;
        }
        printf("Pausing child\n");
        execl("./test", "test", NULL);
        exit(1);
    } else {
        //ptrace(PTRACE_ATTACH, child, NULL, 0);
        waitpid(child, NULL, 0);
        ptrace(PTRACE_CONT, child, NULL, 0);
        waitpid(child, NULL, 0);
        ptrace(PTRACE_CONT, child, NULL, 0);
        // ptrace(PTRACE_KILL, child, (caddr_t)SIGINT, 0);
        // printf("Pausing child\n");
        // sleep(5);
        // printf("Resuming child\n");
        // ptrace(PTRACE_CONT, child, NULL, 0);
        //waitpid(child, NULL, 0);
    }
    return 0;
}