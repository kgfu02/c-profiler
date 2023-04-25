#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <time.h>
#include <signal.h>
#include <syscall.h>
#include <libdwarf/libdwarf.h>
#include <libdwarf/dwarf.h>
#include <sys/personality.h>
#include <string.h>

#define SAMPLE_HZ 2

struct user_regs_struct regs;
struct timespec start;
pid_t child_pid;
int status;
char* filename;
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

char* get_function_from_pc(unsigned long long pc) {
    pc -= load_addr;
    Dwarf_Debug dbg;
    Dwarf_Error error;
    int ret = dwarf_init_path(filename, NULL, 0, 0, DW_GROUPNUMBER_ANY, NULL,
                              NULL, &dbg, NULL, 0, NULL, NULL);
    Dwarf_Unsigned cu_header_length, next_cu_header_offset,
        next_cu_header_length;
    Dwarf_Half version_stamp, address_size, length_size, extension_size,
        header_cu_type;
    Dwarf_Off abbrev_offset;
    Dwarf_Sig8 signature;
    dwarf_next_cu_header_d(dbg, 1, &cu_header_length, &version_stamp,
                           &abbrev_offset, &address_size, &length_size,
                           &extension_size, &signature, &next_cu_header_offset,
                           &next_cu_header_length, &header_cu_type, &error);
    Dwarf_Die cu_die;
    Dwarf_Half tag;
    ret = dwarf_siblingof_b(dbg, NULL, 1, &cu_die,
                            &error);  // gets Compilation Unit DIE

    Dwarf_Die child_die;
    ret = dwarf_child(cu_die, &child_die, &error);
    dwarf_tag(child_die, &tag, NULL);
    while (ret == DW_DLV_OK) {
        if (tag == DW_TAG_subprogram) {
            Dwarf_Half form;
            enum Dwarf_Form_Class class;
            Dwarf_Addr low_pc = 0, high_pc = 0;
            dwarf_lowpc(child_die, &low_pc, &error);
            dwarf_highpc_b(child_die, &high_pc, &form, &class, &error);
            high_pc += low_pc;  // add to low_pc since high_pc is offset
            char* function;
            Dwarf_Attribute attr;
            dwarf_attr(child_die, DW_AT_name, &attr, &error);
            if (pc <= high_pc && pc >= low_pc && dwarf_formstring(attr, &function, &error) == DW_DLV_OK) {
                
                char* copy = malloc(strlen(function)+1);
                strcpy(copy, function);
                dwarf_dealloc(dbg, function, DW_DLA_STRING);
                dwarf_finish(dbg, &error);
                return copy;
            }
        }
        ret = dwarf_siblingof(dbg, child_die, &child_die, &error);
        dwarf_tag(child_die, &tag, NULL);
    }
    dwarf_finish(dbg, &error);
    return NULL;
}

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

int main(int argc, char** argv) {
    if(argc == 1) {
        printf("Usage: ./profiler <target exec>\n");
        exit(1);
    }
    filename = argv[1];
    // get_function_from_pc(0);
    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        personality(ADDR_NO_RANDOMIZE);
        ptrace(PTRACE_TRACEME);
        execl(filename, filename, NULL);
        exit(1);
    } else {
        // Parent process
        // Wait for child to stop at exec call
        wait(&status);
        initialise_load_address(child_pid);
        // Tell child to not stop at system calls
        ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);
        // Continue through exec call
        ptrace(PTRACE_CONT, child_pid);
        sleep(1);
        struct user_regs_struct regs;
        for (int i = 0; i < 10; i++) {
            kill(child_pid, SIGSTOP);
            waitpid(child_pid, &status, WUNTRACED);
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
            printf("Child PC = %p\n", (void*)(regs.rip-load_addr));
            char* func = get_function_from_pc(regs.rip);
            if(func != NULL) {
                printf("Function name = %s\n\n", func);
                free(func);
            } else {
                printf("Function name not found\n\n");
            }
            ptrace(PTRACE_CONT, child_pid);
            sleep(3);
        }
        wait(&status);
    }
    return 0;
}
