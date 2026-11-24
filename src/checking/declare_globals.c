#include "checking/checking.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"

static const char *global_label(const char *name) {
    size_t size = strlen(name) + 2;
    char *label = arena_alloc(size);
    snprintf(label, size, "$%s", name);
    return label;
}

void declare_globals(Program *program) {
    for (int index = 0; index < program->globals.count; index++) {
        GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        variable->symbol = arena_alloc(sizeof *variable->symbol);
        variable->symbol->kind = SYMBOL_GLOBAL;
        variable->symbol->type = variable->type;
        variable->symbol->shape = variable->shape;
        variable->symbol->size = 8;
        variable->symbol->label = global_label(variable->name);
        variable->symbol->is_const = variable->is_const;
        variable->symbol->global = variable;
    }
}
