#include "driver/driver.h"

#include <stdlib.h>

#include "common/diagnostics.h"
#include "platform/process.h"

enum { PATH_SIZE = 4096 };

static int assemble(const char *program, const char *assembly, const char *output) {
#ifdef _WIN32
    return run_assembler(program, assembly, output);
#else
    const char *arguments[] = {program, "-f", "bin", assembly, "-o", output, NULL};
    return run_process(arguments, NULL);
#endif
}

void assemble_file(const char *asm_path, const char *exe_path) {
    static char nasm[PATH_SIZE];
    const char *override = getenv("RZ_NASM");
    if (override != NULL && override[0] != '\0') {
        if (assemble(override, asm_path, exe_path) != 0) {
            fatal_error("%s could not assemble %s", override, asm_path);
        }
        return;
    }
    if (!find_program("nasm", nasm, sizeof nasm)) {
        fatal_error("nasm was not found on PATH; install NASM or set RZ_NASM to the full path of nasm.exe");
    }
    if (assemble(nasm, asm_path, exe_path) != 0) {
        fatal_error("nasm could not assemble %s; this is a bug in rzc, please report it with the .rz file", asm_path);
    }
}
