// Tester file that repeatedly loops through 3 different functions and prints its current PC
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

__attribute__ ((__noinline__))
void * get_pc () { return __builtin_return_address(0); }

unsigned long long load_addr = 0;

void initialise_load_address(int pid) {
    // Assume position independent executable PIE
    // The load address is found in /proc/pid/maps
    char target_path[255];
    snprintf(target_path, 255, "/proc/%d/maps", pid);
    FILE* file = fopen(target_path, "r");
    char buffer[255];
    fgets(buffer, 255, file);
    *strchr(buffer, '-') = '\0';
    load_addr = strtol(buffer, NULL, 16);
}

void A() {
    printf("%p\n", get_pc());
    sleep(1);
}

void B() {
    printf("%p\n", get_pc());
    sleep(1);
}

void C() {
    for(int i = 0; i<100000000000000; i++) {
        if(i==10) {
            void* rip;
            asm volatile ("call next\n"
                  "next: pop %0" : "=r" (rip));
            printf("Program counter is: %p\n", rip-load_addr);
        }
        int a = 1;
        int b = 2;
        int c = 3;
        int d = 4;
        // write(STDOUT_FILENO, &a, 1);
    }
}

int main() {
    initialise_load_address(getpid());
    printf("Running test.c...\n");
    struct timespec start, end;
    double elapsed_time;
    char a = '\0';
    C();
    printf("Ending test.c...\n");
    return 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    //printf("Sleeping!\n");
    for(int i = 0; i<10000; i++) {
        printf("looped\n");
        printf("%p\n", get_pc());
        A();
        i++;
        B();
        i--;
        C();
    }
    sleep(5);
    //printf("Done sleeping!\n");
    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_time = (end.tv_sec - start.tv_sec) +
                   (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
    return 0;
}