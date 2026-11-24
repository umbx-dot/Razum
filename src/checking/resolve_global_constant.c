#include "checking/checking.h"

#include "common/arena.h"
#include "common/diagnostics.h"

typedef struct {
    Program *program;
    GlobalVariable *variable;
} ConstantWork;

static void define(void *data) {
    ConstantWork *work = data;
    GlobalVariable *variable = work->variable;
    resolve_shape(work->program, variable->shape);
    variable->type = variable->shape->kind;
    variable->symbol->type = variable->type;
    Checker checker = {0};
    checker.program = work->program;
    checker.scopes.program = work->program;
    variable->symbol->constant = constant_value(&checker, variable->shape, variable->initial, variable->name);
}

static const Expression *stand_in(const GlobalVariable *variable) {
    Expression *zero = arena_alloc(sizeof *zero);
    zero->line = variable->line;
    zero->column = variable->column;
    if (variable->shape->kind == TYPE_STRING) {
        zero->kind = EXPRESSION_STRING;
        zero->string.bytes = "";
    } else if (is_floating(variable->shape->kind)) {
        zero->kind = EXPRESSION_DOUBLE;
    } else {
        zero->kind = EXPRESSION_INT;
        zero->int_literal.literal_type = is_integer(variable->shape->kind) || variable->shape->kind == TYPE_BOOL
                                             ? variable->shape->kind : TYPE_INT;
        zero->int_literal.from_constant = true;
    }
    zero->shape = variable->shape;
    return zero;
}

void resolve_global_constant(Program *program, GlobalVariable *variable) {
    if (variable->constant_state == 2) {
        return;
    }
    const char *previous_path = diagnostic_source_path;
    diagnostic_source_path = variable->source_path;
    if (variable->constant_state == 1) {
        compile_error(variable->line, variable->column, "const '%s' depends on its own value", variable->name);
    }
    variable->constant_state = 1;
    ConstantWork work = {program, variable};
    if (!diagnostic_try(define, &work)) {
        variable->symbol->constant = stand_in(variable);
    }
    variable->constant_state = 2;
    diagnostic_source_path = previous_path;
}
