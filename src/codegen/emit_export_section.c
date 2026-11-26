#include "codegen/codegen.h"

#include <stdlib.h>
#include <string.h>

#include "common/arena.h"

static int compare_names(const void *left, const void *right) {
    return strcmp(*(const char *const *)left, *(const char *const *)right);
}

static int collect_exports(const Program *program, const char **exports) {
    int count = 0;
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_exported) {
            exports[count++] = function->name;
        }
    }
    for (int index = 0; index < program->globals.count; index++) {
        const GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        if (variable->is_exported) {
            exports[count++] = variable->name;
        }
    }
    qsort(exports, (size_t)count, sizeof *exports, compare_names);
    return count;
}

void emit_export_section(TextBuffer *output, const Program *program, const char *dll_name) {
    text_line(output, "");
    text_line(output, "    align 4, db 0");
    text_line(output, "pe@exports:");
    size_t capacity = (size_t)program->functions.count + (size_t)program->globals.count + 1;
    const char **exports = dll_name != NULL ? arena_alloc(sizeof *exports * capacity) : NULL;
    int count = dll_name != NULL ? collect_exports(program, exports) : 0;
    if (count == 0) {
        text_line(output, "pe@exports_end:");
        return;
    }
    text_line(output, "    dd 0, 0                                 ; flags, timestamp");
    text_line(output, "    dw 0, 0                                 ; version");
    text_line(output, "    dd pe@export_dll_name, 1, %d, %d          ; name, first ordinal, functions, names", count, count);
    text_line(output, "    dd pe@export_functions, pe@export_names, pe@export_ordinals");
    text_line(output, "pe@export_functions:");
    for (int index = 0; index < count; index++) {
        text_line(output, "    dd $%s", exports[index]);
    }
    text_line(output, "pe@export_names:");
    for (int index = 0; index < count; index++) {
        text_line(output, "    dd export@%s", exports[index]);
    }
    text_line(output, "pe@export_ordinals:");
    for (int index = 0; index < count; index++) {
        text_line(output, "    dw %d", index);
    }
    text_line(output, "pe@export_dll_name: db \"%s\", 0", dll_name);
    for (int index = 0; index < count; index++) {
        text_line(output, "export@%s: db \"%s\", 0", exports[index], exports[index]);
    }
    text_line(output, "pe@exports_end:");
}
