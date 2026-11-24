#include "codegen/codegen.h"

#include <string.h>

static const char *const SCRATCH[] = {"xmm2", "xmm3", "xmm4", "xmm5"};

static bool is_leaf(const Expression *expression) {
    return expression->type == TYPE_DOUBLE &&
           (expression->kind == EXPRESSION_DOUBLE || expression->kind == EXPRESSION_NAME);
}

static bool fits(const Expression *expression, int depth) {
    if (is_leaf(expression)) {
        return true;
    }
    if (expression->type != TYPE_DOUBLE || expression->kind != EXPRESSION_BINARY ||
        expression->binary.operator < OP_ADD || expression->binary.operator > OP_DIVIDE) {
        return false;
    }
    const Expression *right = expression->binary.right;
    return fits(expression->binary.left, depth) &&
           (is_leaf(right) || (depth < (int)(sizeof SCRATCH / sizeof SCRATCH[0]) && fits(right, depth + 1)));
}

static void emit_tree(FunctionContext *context, Expression *expression, const char *target, int depth) {
    Operand operand;
    const char *cached = cached_double_product(context, expression);
    if (cached != NULL) {
        emit(context, "movapd %s, %s", target, cached);
        return;
    }
    if (double_operand(context->program, expression, &operand)) {
        if (strcmp(target, operand.text) != 0) {
            emit(context, "movsd %s, %s", target, operand.text);
        }
        return;
    }
    Expression *left = expression->binary.left;
    Expression *right = expression->binary.right;
    const char *instruction = double_arithmetic_instruction(expression->binary.operator);
    const char *left_cached = cached_double_product(context, left);
    if (context->program->zen3) {
        const char *left_register = left->kind == EXPRESSION_NAME ? left->name.symbol->register_name : left_cached;
        if (left_register != NULL) {
            const char *right_cached = cached_double_product(context, right);
            if (right_cached != NULL || double_operand(context->program, right, &operand)) {
                emit(context, "v%s %s, %s, %s", instruction, target, left_register,
                     right_cached != NULL ? right_cached : operand.text);
                return;
            }
        }
    }
    emit_tree(context, left, target, depth);
    cached = cached_double_product(context, right);
    if (cached != NULL) {
        emit(context, "%s %s, %s", instruction, target, cached);
    } else if (double_operand(context->program, right, &operand)) {
        emit(context, "%s %s, %s", instruction, target, operand.text);
    } else {
        const char *scratch = SCRATCH[depth];
        emit_tree(context, right, scratch, depth + 1);
        emit(context, "%s %s, %s", instruction, target, scratch);
    }
}

bool emit_double_tree(FunctionContext *context, Expression *expression, const char *target) {
    if (!double_tree_fits(expression)) {
        return false;
    }
    emit_tree(context, expression, target, 0);
    return true;
}

bool double_tree_fits(const Expression *expression) {
    return fits(expression, 0);
}

static bool reads_symbol(const Expression *expression, const Symbol *symbol) {
    if (expression->kind == EXPRESSION_NAME) {
        return expression->name.symbol == symbol;
    }
    return expression->kind == EXPRESSION_BINARY &&
           (reads_symbol(expression->binary.left, symbol) || reads_symbol(expression->binary.right, symbol));
}

bool emit_double_tree_to_symbol(FunctionContext *context, Expression *expression, const Symbol *target) {
    if (target->type != TYPE_DOUBLE || target->register_name == NULL ||
        !double_tree_fits(expression) || reads_symbol(expression, target)) {
        return false;
    }
    emit_tree(context, expression, target->register_name, 0);
    return true;
}
