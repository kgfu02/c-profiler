// Tester file for usign ptrace
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static unsigned int x;

double elapsed_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) +
                   (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;;
}

void A() {
    x++;
}

void B() {
    x++;
}

void C() {
    x++;
    x--;
    x++;
}

int main() {
    printf("Running test2.c...\n");
    // while(x<10000000) {
    //     A();
    //     B();
    //     C();
    //     if(x%(3*99999999)==0)
    //         printf(".\n");
    // }
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    clock_gettime(CLOCK_MONOTONIC, &end);
    while(elapsed_time(start, end)<20) {
        A();
        B();
        C();
        clock_gettime(CLOCK_MONOTONIC, &end);
    }
    printf("Ending test2.c...\n");
}