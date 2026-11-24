#include "codegen/codegen.h"

#include <inttypes.h>
#include <string.h>

static const char *const SCRATCH[] = {"r10", "r11", "r8", "r9"};

static bool emit_scaled_add(FunctionContext *context, Expression *expression, const char *destination) {
    const Expression *product = expression;
    int64_t offset = 0;
    if (expression->binary.operator == OP_ADD && expression->binary.right->kind == EXPRESSION_INT &&
        expression->binary.right->int_literal.value >= INT32_MIN && expression->binary.right->int_literal.value <= INT32_MAX) {
        product = expression->binary.left;
        offset = expression->binary.right->int_literal.value;
    }
    if (product->kind != EXPRESSION_BINARY || product->binary.operator != OP_MULTIPLY ||
        (product->type != TYPE_LONG && product->type != TYPE_ULONG)) {
        return false;
    }
    const Expression *base = product->binary.left;
    const Expression *factor = product->binary.right;
    if (base->kind == EXPRESSION_INT) {
        base = factor;
        factor = product->binary.left;
    }
    if (base->kind != EXPRESSION_NAME || base->name.symbol->register_name == NULL ||
        factor->kind != EXPRESSION_INT) {
        return false;
    }
    int64_t multiplier = factor->int_literal.value;
    if (multiplier != 3 && multiplier != 5 && multiplier != 9) {
        return false;
    }
    int64_t scale = multiplier - 1;
    const char *reg = base->name.symbol->register_name;
    emit(context, "lea %s, [%s + %s * %" PRId64 " %c %" PRId64 "]", destination, reg, reg, scale,
         offset < 0 ? '-' : '+', offset < 0 ? -offset : offset);
    return true;
}

static bool is_simple(const Expression *expression) {
    if (expression->kind == EXPRESSION_INT) {
        return expression->int_literal.value >= INT32_MIN && expression->int_literal.value <= INT32_MAX;
    }
    return expression->kind == EXPRESSION_NAME && !is_floating(expression->type) &&
           !is_aggregate(expression->type) && !is_memory_symbol(expression->name.symbol);
}

static bool fits(const Expression *expression, int depth) {
    if (is_simple(expression) || expression->kind == EXPRESSION_INT) {
        return true;
    }
    if (expression->kind != EXPRESSION_BINARY || (expression->type != TYPE_LONG && expression->type != TYPE_ULONG)) {
        return false;
    }
    const Expression *right = expression->binary.right;
    Operator op = expression->binary.operator;
    if (is_shift(op)) {
        return right->kind == EXPRESSION_INT && fits(expression->binary.left, depth);
    }
    return integer_arithmetic_instruction(op) != NULL && fits(expression->binary.left, depth) &&
           (is_simple(right) || (depth < (int)(sizeof SCRATCH / sizeof SCRATCH[0]) && fits(right, depth + 1)));
}

static void emit_tree(FunctionContext *context, Expression *expression, const char *destination, int depth) {
    Operand operand;
    if (int_operand(expression, &operand)) {
        if (strcmp(destination, operand.text) != 0) {
            emit(context, "mov %s, %s", destination, operand.text);
        }
        return;
    }
    if (expression->kind == EXPRESSION_INT) {
        emit(context, "mov %s, %" PRId64, destination, expression->int_literal.value);
        return;
    }
    if (emit_wide_offset(context, expression, destination) || emit_scaled_add(context, expression, destination)) {
        return;
    }
    Expression *right = expression->binary.right;
    Operator op = expression->binary.operator;
    emit_tree(context, expression->binary.left, destination, depth);
    if (is_shift(op)) {
        unsigned count = (unsigned)right->int_literal.value & 63;
        emit(context, "%s %s, %u", shift_instruction(op, expression->type), destination, count);
    } else if (int_operand(right, &operand)) {
        emit(context, "%s %s, %s", integer_arithmetic_instruction(op), destination, operand.text);
    } else {
        const char *scratch = SCRATCH[depth];
        emit_tree(context, right, scratch, depth + 1);
        emit(context, "%s %s, %s", integer_arithmetic_instruction(op), destination, scratch);
    }
}

bool emit_wide_tree(FunctionContext *context, Expression *expression, const char *destination) {
    if (!fits(expression, 0)) {
        return false;
    }
    emit_tree(context, expression, destination, 0);
    return true;
}
