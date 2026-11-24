#include "checking/checking.h"

#include <string.h>

#include "common/diagnostics.h"

static void require_literals(const Expression *initial) {
    if (initial->kind == EXPRESSION_LIST) {
        for (int index = 0; index < initial->list.count; index++) {
            require_literals(LIST_AT(initial->list, Expression, index));
        }
        return;
    }
    if (initial->kind == EXPRESSION_FUNCTION && initial->function_value->extern_dll != NULL) {
        compile_error(initial->line, initial->column,
                      "the address of extern function '%s' is only known when the program runs; set this global in main",
                      initial->function_value->name);
    }
    if (initial->kind != EXPRESSION_INT && initial->kind != EXPRESSION_DOUBLE && initial->kind != EXPRESSION_STRING &&
        initial->kind != EXPRESSION_FUNCTION) {
        compile_error(initial->line, initial->column,
                      "a global's initial value must be known when compiling: a literal such as 0, 1.5, 'a', true or \"text\", a const expression, or a { } list of them");
    }
}

static void fold_values(Expression *initial) {
    if (initial->kind == EXPRESSION_LIST) {
        for (int index = 0; index < initial->list.count; index++) {
            fold_values(LIST_AT(initial->list, Expression, index));
        }
        return;
    }
    Expression *folded = fold_constant(initial);
    if (folded != NULL) {
        *initial = *folded;
    }
}

static void check_initial(Program *program, GlobalVariable *variable) {
    Expression *initial = variable->initial;
    if (initial == NULL) {
        if (variable->type == TYPE_ARRAY && variable->shape->count < 0) {
            compile_error(variable->line, variable->column, "array '%s' needs a size or a { } list", variable->name);
        }
        return;
    }
    Checker checker = {0};
    checker.program = program;
    checker.scopes.program = program;
    check_initial_value(&checker, variable->shape, initial, variable->name);
    fold_values(initial);
    require_literals(initial);
}

typedef struct {
    Program *program;
    int index;
} GlobalWork;

static void check_global(void *data) {
    GlobalWork *work = data;
    Program *program = work->program;
    int index = work->index;
    {
        GlobalVariable *variable = LIST_AT(program->globals, GlobalVariable, index);
        diagnostic_source_path = variable->source_path;
        resolve_shape(program, variable->shape);
        variable->type = variable->shape->kind;
        variable->symbol->type = variable->type;
        if (variable->type == TYPE_VOID) {
            compile_error(variable->line, variable->column, "a variable cannot be void");
        }
        for (int earlier = 0; earlier < index; earlier++) {
            if (strcmp(LIST_AT(program->globals, GlobalVariable, earlier)->name, variable->name) == 0) {
                compile_error(variable->line, variable->column, "global '%s' is declared twice", variable->name);
            }
        }
        if (variable->is_const) {
            resolve_global_constant(program, variable);
        } else {
            check_initial(program, variable);
        }
    }
}

void check_globals(Program *program) {
    for (int index = 0; index < program->globals.count; index++) {
        GlobalWork work = {program, index};
        diagnostic_try(check_global, &work);
    }
}
