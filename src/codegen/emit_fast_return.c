#include "codegen/codegen.h"

#include <inttypes.h>

static Statement *first_statement(Statement *statement) {
    if (statement->kind == STATEMENT_BLOCK && statement->block.count > 0) {
        return LIST_AT(statement->block, Statement, 0);
    }
    return statement;
}

static bool is_parameter(const Expression *expression, const Symbol *parameter) {
    return expression->kind == EXPRESSION_NAME && expression->name.symbol == parameter;
}

void emit_fast_return(FunctionContext *context, TextBuffer *output) {
    if (context->tail_label == NULL) {
        return;
    }
    Function *function = context->function;
    Symbol *parameter = LIST_AT(function->parameters, Parameter, 0)->symbol;
    Statement *guard = first_statement(function->body);
    if (guard->kind != STATEMENT_IF || guard->if_statement.else_branch != NULL) {
        return;
    }
    Statement *early = guard->if_statement.then_branch;
    if (early->kind == STATEMENT_BLOCK && early->block.count == 1) {
        early = first_statement(early);
    }
    if (early->kind != STATEMENT_RETURN || early->return_value == NULL ||
        (!is_parameter(early->return_value, parameter) && early->return_value->kind != EXPRESSION_INT)) {
        return;
    }
    Expression *condition = guard->if_statement.condition;
    if (condition->kind != EXPRESSION_BINARY || condition->binary.operator < OP_EQUAL ||
        condition->binary.operator > OP_GREATER_EQUAL || !is_parameter(condition->binary.left, parameter) ||
        condition->binary.right->kind != EXPRESSION_INT ||
        condition->binary.right->int_literal.value < INT32_MIN || condition->binary.right->int_literal.value > INT32_MAX) {
        return;
    }
    FunctionContext entry = {0};
    entry.program = context->program;
    entry.function = function;
    const char *normal = new_label(&entry, "nonbase");
    const char *saved = parameter->register_name;
    parameter->register_name = "rcx";
    jump_if(&entry, condition, normal, false);
    emit_int(&entry, early->return_value);
    parameter->register_name = saved;
    emit(&entry, "ret");
    emit_label(&entry, normal);
    uint64_t table[16];
    int first;
    if (small_recurrence(function, guard, table, (int)(sizeof table / sizeof table[0]), &first)) {
        const char *recurse = new_label(&entry, "recursive");
        const char *values = new_label(&entry, "smallresults");
        emit(&entry, "cmp rcx, %d", first + (int)(sizeof table / sizeof table[0]));
        emit(&entry, "jge %s", recurse);
        emit(&entry, "lea rax, [rel %s]", values);
        emit(&entry, "mov rax, qword [rax + rcx*8 - %d]", first * 8);
        emit(&entry, "ret");
        emit_label(&entry, values);
        for (size_t index = 0; index < sizeof table / sizeof table[0]; index++) {
            emit(&entry, "dq %" PRIu64, table[index]);
        }
        emit_label(&entry, recurse);
    }
    text_append_bytes(output, entry.body.data, entry.body.length);
}
