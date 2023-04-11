// Tester file for usign ptrace
#include <stdio.h>
#include <unistd.h>
#include <time.h>


int main() {
    printf("Running test.c...\n");
    struct timespec start, end;
    double elapsed_time;

    clock_gettime(CLOCK_MONOTONIC, &start);
    printf("Sleeping!\n");
    sleep(1);
    printf("Done sleeping!\n");
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_time = (end.tv_sec - start.tv_sec) +
                   (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
    return 0;
}