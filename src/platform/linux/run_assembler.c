#include "platform/process.h"

#include <string.h>

int run_assembler(const char *program, const char *assembly, const char *output, const char *format) {
    bool elf = strcmp(format, "elf64") == 0;
    const char *arguments[] = {program, "-f", format, elf ? "/dev/stdin" : assembly, "-o", output,
                               elf ? "--reproducible" : NULL, NULL};
    return run_process_with_input(arguments, NULL, elf ? assembly : NULL);
}
