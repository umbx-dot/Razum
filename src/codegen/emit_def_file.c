#include "codegen/codegen.h"

char *emit_def_file(const Program *program, const char *dll_name) {
    TextBuffer output = {0};
    text_line(&output, "LIBRARY \"%s\"", dll_name);
    text_line(&output, "EXPORTS");
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_exported) {
            text_line(&output, "    %s", function->name);
        }
    }
    for (int index = 0; index < program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_exported) {
            text_line(&output, "    %s DATA", variable->name);
        }
    }
    return output.data;
}
