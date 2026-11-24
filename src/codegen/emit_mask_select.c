#include "codegen/codegen.h"

static Statement *single_assignment(Statement *statement) {
    if (statement != NULL && statement->kind == STATEMENT_BLOCK && statement->block.count == 1) {
        statement = LIST_AT(statement->block, Statement, 0);
    }
    if (statement == NULL || statement->kind != STATEMENT_ASSIGNMENT || statement->assignment.operator != OP_ASSIGN ||
        statement->assignment.target->kind != EXPRESSION_NAME) {
        return NULL;
    }
    return statement;
}

static bool cheap(const Expression *expression, int *cost) {
    if (expression->kind == EXPRESSION_INT) {
        return true;
    }
    if (expression->kind == EXPRESSION_NAME) {
        return !is_floating(expression->type) && expression->name.symbol->register_name != NULL;
    }
    if (expression->kind != EXPRESSION_BINARY || (expression->type != TYPE_LONG && expression->type != TYPE_ULONG) ||
        ++*cost > 4) {
        return false;
    }
    Operator op = expression->binary.operator;
    const Expression *right = expression->binary.right;
    return (integer_arithmetic_instruction(op) != NULL || (is_shift(op) && right->kind == EXPRESSION_INT)) &&
           cheap(expression->binary.left, cost) && cheap(right, cost);
}

bool emit_mask_select(FunctionContext *context, Statement *statement) {
    Statement *yes = single_assignment(statement->if_statement.then_branch);
    Statement *no = single_assignment(statement->if_statement.else_branch);
    const Expression *condition = statement->if_statement.condition;
    const char *value;
    Operand mask;
    if (yes == NULL || no == NULL || !mask_test_operands(condition, &value, &mask)) {
        return false;
    }
    const Symbol *target = yes->assignment.target->name.symbol;
    if (target != no->assignment.target->name.symbol || target->register_name == NULL ||
        (target->type != TYPE_LONG && target->type != TYPE_ULONG)) {
        return false;
    }
    int cost = 0;
    if (!cheap(yes->assignment.value, &cost) || !cheap(no->assignment.value, &cost)) {
        return false;
    }
    emit_source_comment(context, yes->line);
    emit_wide_tree(context, yes->assignment.value, "rcx");
    emit_source_comment(context, no->line);
    emit_wide_tree(context, no->assignment.value, "rax");
    emit(context, "test %s, %s", value, mask.text);
    emit(context, "mov %s, rax", target->register_name);
    emit(context, "cmov%s %s, rcx", condition->binary.operator == OP_EQUAL ? "z" : "nz", target->register_name);
    return true;
}
