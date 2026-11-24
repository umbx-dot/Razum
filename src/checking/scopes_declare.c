#include "checking/checking.h"

#include <stdio.h>
#include <string.h>

#include "common/arena.h"
#include "common/diagnostics.h"

static int next_label_use(Scopes *scopes, const char *name) {
    for (int index = 0; index < scopes->label_uses.count; index++) {
        LabelUse *use = LIST_AT(scopes->label_uses, LabelUse, index);
        if (strcmp(use->name, name) == 0) {
            return use->uses++;
        }
    }
    LabelUse *use = arena_alloc(sizeof *use);
    use->name = name;
    use->uses = 1;
    list_push(&scopes->label_uses, use);
    return 0;
}

static const char *local_label(const char *name, int use) {
    size_t size = strlen(name) + 24;
    char *label = arena_alloc(size);
    if (use == 0) {
        snprintf(label, size, ".%s", name);
    } else {
        snprintf(label, size, ".%s@%d", name, use);
    }
    return label;
}

Symbol *scopes_declare(Scopes *scopes, const char *name, const Shape *shape, SymbolKind kind, int line, int column, int index) {
    for (int entry_index = scopes->entries.count - 1; entry_index >= 0; entry_index--) {
        ScopeEntry *entry = LIST_AT(scopes->entries, ScopeEntry, entry_index);
        if (entry->depth < scopes->depth) {
            break;
        }
        if (strcmp(entry->name, name) == 0) {
            compile_error(line, column, "'%s' is already declared in this block", name);
        }
    }
    Symbol *symbol = arena_alloc(sizeof *symbol);
    symbol->name = name;
    symbol->line = line;
    symbol->column = column;
    if (scopes->function != NULL) {
        list_push(&scopes->function->declared, symbol);
    }
    symbol->kind = kind;
    symbol->type = shape->kind;
    symbol->shape = shape;
    symbol->label = local_label(name, next_label_use(scopes, name));
    symbol->index = index;
    symbol->is_reference = kind == SYMBOL_PARAMETER && is_aggregate(shape->kind);
    symbol->size = is_aggregate(shape->kind) && !symbol->is_reference ? (shape_size(shape) + 7) / 8 * 8 : 8;
    if (symbol->is_reference && shape->kind == TYPE_ARRAY && shape->count < 0) {
        size_t size = strlen(symbol->label) + 8;
        char *length_label = arena_alloc(size);
        snprintf(length_label, size, "%s@length", symbol->label);
        symbol->length_label = length_label;
    }
    ScopeEntry *entry = arena_alloc(sizeof *entry);
    entry->name = name;
    entry->symbol = symbol;
    entry->depth = scopes->depth;
    list_push(&scopes->entries, entry);
    if (kind == SYMBOL_LOCAL) {
        list_push(&scopes->function->locals, symbol);
    }
    return symbol;
}
