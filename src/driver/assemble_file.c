#include "driver/driver.h"

#include <stdlib.h>

#include "common/diagnostics.h"
#include "platform/process.h"

enum { PATH_SIZE = 4096 };

void assemble_file(const char *asm_path, const char *exe_path, bool object) {
    static char nasm[PATH_SIZE];
    const char *format = object ? (host_target() == TARGET_WINDOWS ? "win64" : "elf64") : "bin";
    const char *override = getenv("RZ_NASM");
    if (override != NULL && override[0] != '\0') {
        if (run_assembler(override, asm_path, exe_path, format) != 0) {
            fatal_error("%s could not assemble %s", override, asm_path);
        }
        return;
    }
    if (!find_program("nasm", nasm, sizeof nasm)) {
        fatal_error("nasm was not found on PATH; install NASM or set RZ_NASM to the full path of nasm.exe");
    }
    if (run_assembler(nasm, asm_path, exe_path, format) != 0) {
        fatal_error("nasm could not assemble %s; this is a bug in rzc, please report it with the .rz file", asm_path);
    }
}
