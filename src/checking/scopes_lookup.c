#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

Symbol *scopes_find(const Scopes *scopes, const char *name) {
    for (int index = scopes->entries.count - 1; index >= 0; index--) {
        ScopeEntry *entry = LIST_AT(scopes->entries, ScopeEntry, index);
        if (strcmp(entry->name, name) == 0) {
            return entry->symbol;
        }
    }
    for (int index = 0; index < scopes->program->globals.count; index++) {
        GlobalVariable *variable = LIST_AT(scopes->program->globals, GlobalVariable, index);
        if (strcmp(variable->name, name) == 0) {
            return variable->symbol;
        }
    }
    return NULL;
}

Symbol *scopes_lookup(const Scopes *scopes, const char *name, int line, int column) {
    Symbol *symbol = scopes_find(scopes, name);
    if (symbol == NULL) {
        compile_error(line, column, "'%s' is not declared", name);
    }
    return symbol;
}
