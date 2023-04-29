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
#include <search.h>
#include "includes/dictionary.h"

#define MS_BETWEEN_SAMPLES 100

static pid_t child_pid;
static int status;
static char* filename;
static unsigned long long load_addr = 0;
static dictionary* function_dict;
static double total_samples = 0;

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
            if (pc <= high_pc && pc >= low_pc &&
                dwarf_formstring(attr, &function, &error) == DW_DLV_OK) {
                char* copy = malloc(strlen(function) + 1);
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

// periodically pauses child and print program counter
void sample(pid_t child_pid) {
    struct user_regs_struct regs;
    struct timespec ts;
    ts.tv_sec = MS_BETWEEN_SAMPLES / 1000;
    ts.tv_nsec = (MS_BETWEEN_SAMPLES % 1000) * 1000000;
    while (true) {
        if (kill(child_pid, SIGSTOP) == -1) perror("kill");
        if (waitpid(child_pid, &status, 0) == -1) perror("wait");
        if (WIFEXITED(status))
            return;
        if (ptrace(PTRACE_GETREGS, child_pid, NULL, &regs) == -1)
            perror("GETREGS");
        printf("Child PC = %p\n", (void*)(regs.rip - load_addr));
        char* func = get_function_from_pc(regs.rip);
        if (func != NULL) {
            // printf("%d, %d\n", WIFSIGNALED(status), WTERMSIG(status));
            printf("Function name = %s\n\n", func);
            if (!dictionary_contains(function_dict, func)) {
                int one = 1;
                // char* copy = malloc(strlen(func)+1);
                // strcpy(copy, func);
                dictionary_set(function_dict, func, &one);
            } else {
                int increment_value =
                    *(int*)dictionary_get(function_dict, func) + 1;
                dictionary_set(function_dict, func, &increment_value);
            }
            total_samples++;
            free(func);
        } else {
            printf("Function name not found\n\n");
        }
        // if(waitpid(child_pid, &status, WNOHANG) == -1)
        //     perror("wait:");
        // printf("%d, %d\n", WIFSIGNALED(status), WTERMSIG(status));
        ptrace(PTRACE_CONT, child_pid, NULL, SIGCONT);
        nanosleep(&ts, &ts);
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("Usage: ./profiler <target exec> <args...>\n");
        exit(1);
    }
    // setup
    filename = argv[1];
    function_dict = string_to_int_dictionary_create();
    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        // Child process
        personality(ADDR_NO_RANDOMIZE);
        ptrace(PTRACE_TRACEME, NULL, NULL, NULL);
        execv(filename, argv + 1);
        exit(1);
    } else {
        // Parent process
        // Wait for child to stop at exec call
        wait(&status);
        initialise_load_address(child_pid);
        // Tell child to not stop at system calls?
        ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);
        // Continue through exec call
        if (ptrace(PTRACE_CONT, child_pid, NULL, SIGCONT) == -1) perror("CONT");
        sleep(1);
        sample(child_pid);
        vector* function_names = dictionary_keys(function_dict);
        for (int i = 0; i < (int)vector_size(function_names); i++) {
            char* func = vector_get(function_names, i);
            printf("%-10s %.2f%%\n", func,
                   (*(int*)dictionary_get(function_dict, func) / total_samples *
                    100));
        }
    }
    return 0;
}
