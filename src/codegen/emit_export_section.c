#include "codegen/codegen.h"

#include <stdlib.h>
#include <string.h>

#include "common/arena.h"

static int compare_names(const void *left, const void *right) {
    const Function *a = *(const Function *const *)left;
    const Function *b = *(const Function *const *)right;
    return strcmp(a->name, b->name);
}

static int collect_exports(const Program *program, const Function ***exports) {
    *exports = arena_alloc(sizeof **exports * (size_t)(program->functions.count + 1));
    int count = 0;
    for (int index = 0; index < program->functions.count; index++) {
        const Function *function = LIST_AT(program->functions, Function, index);
        if (function->is_exported) {
            (*exports)[count++] = function;
        }
    }
    qsort(*exports, (size_t)count, sizeof **exports, compare_names);
    return count;
}

void emit_export_section(TextBuffer *output, const Program *program, const char *dll_name) {
    text_line(output, "");
    text_line(output, "    align 4, db 0");
    text_line(output, "pe@exports:");
    if (dll_name == NULL) {
        text_line(output, "pe@exports_end:");
        return;
    }
    const Function **exports = NULL;
    int count = collect_exports(program, &exports);
    text_line(output, "    dd 0, 0                                 ; flags, timestamp");
    text_line(output, "    dw 0, 0                                 ; version");
    text_line(output, "    dd pe@export_dll_name, 1, %d, %d          ; name, first ordinal, functions, names", count, count);
    text_line(output, "    dd pe@export_functions, pe@export_names, pe@export_ordinals");
    text_line(output, "pe@export_functions:");
    for (int index = 0; index < count; index++) {
        text_line(output, "    dd $%s", exports[index]->name);
    }
    text_line(output, "pe@export_names:");
    for (int index = 0; index < count; index++) {
        text_line(output, "    dd export@%s", exports[index]->name);
    }
    text_line(output, "pe@export_ordinals:");
    for (int index = 0; index < count; index++) {
        text_line(output, "    dw %d", index);
    }
    text_line(output, "pe@export_dll_name: db \"%s\", 0", dll_name);
    for (int index = 0; index < count; index++) {
        text_line(output, "export@%s: db \"%s\", 0", exports[index]->name, exports[index]->name);
    }
    text_line(output, "pe@exports_end:");
}
