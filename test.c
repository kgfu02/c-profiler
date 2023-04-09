// Tester file for usign ptrace
#include <stdio.h>
#include <unistd.h>
#include <time.h>


int main() {
    printf("Running test.c...\n");
    struct timespec start, end;
    double elapsed_time;

    clock_gettime(CLOCK_MONOTONIC, &start);
    sleep(5);
    printf("Done sleeping!\n");
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_time = (end.tv_sec - start.tv_sec) +
                   (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Elapsed time: %f seconds\n", elapsed_time);
    return 0;
}