#include "codegen/codegen.h"

void emit_coff_header(TextBuffer *output, const Program *program) {
    text_line(output, "bits 64");
    text_line(output, "default rel");
    text_line(output, "pe@base equ 0");
    text_line(output, "section .text align=16");
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_exported) {
            text_line(output, "global $%s", function->name);
        }
    }
    for (int index = 0; index < program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_exported) {
            text_line(output, "global $%s", variable->name);
        }
    }
}
