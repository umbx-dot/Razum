#include "codegen/codegen.h"

const char *function_label_prefix(const ProgramContext *program) {
    return program->elf_object ? "rzfn@" : "$";
}
