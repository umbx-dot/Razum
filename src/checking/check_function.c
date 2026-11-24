#include "checking/checking.h"

#include "analysis/analysis.h"
#include "common/diagnostics.h"

static bool ends_with_asm(const Statement *body) {
    return body->block.count > 0 && LIST_AT(body->block, Statement, body->block.count - 1)->kind == STATEMENT_ASM;
}

void check_function(Checker *checker, Function *function) {
    if (function->extern_dll != NULL) {
        return;
    }
    diagnostic_source_path = function->source_path;
    checker->function = function;
    checker->loop_depth = 0;
    checker->switch_depth = 0;
    checker->scopes = (Scopes){0};
    checker->scopes.function = function;
    checker->scopes.program = checker->program;
    int position = 0;
    for (int index = 0; index < function->parameters.count; index++) {
        Parameter *parameter = LIST_AT(function->parameters, Parameter, index);
        parameter->symbol = scopes_declare(&checker->scopes, parameter->name, parameter->shape, SYMBOL_PARAMETER,
                                           parameter->line, parameter->column, position);
        position += parameter->symbol->length_label != NULL ? 2 : 1;
    }
    for (int index = 0; index < function->body->block.count; index++) {
        check_statement_safely(checker, LIST_AT(function->body->block, Statement, index));
    }
    if (function->return_type != TYPE_VOID && !always_returns(function->body) && !ends_with_asm(function->body)) {
        compile_error(function->line, function->column, "'%s' can reach its end without returning %s",
                      function->name, type_with_article(function->return_type));
    }
}
