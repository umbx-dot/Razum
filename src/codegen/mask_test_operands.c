#include "codegen/codegen.h"

bool mask_test_operands(const Expression *expression, const char **value, Operand *mask) {
    if (expression->kind != EXPRESSION_BINARY ||
        (expression->binary.operator != OP_EQUAL && expression->binary.operator != OP_NOT_EQUAL)) {
        return false;
    }
    const Expression *left = expression->binary.left;
    const Expression *right = expression->binary.right;
    if (right->kind != EXPRESSION_INT || right->int_literal.value != 0 ||
        left->kind != EXPRESSION_BINARY || left->binary.operator != OP_BIT_AND ||
        (left->type != TYPE_LONG && left->type != TYPE_ULONG)) {
        return false;
    }
    const Expression *bits = left->binary.left;
    const Expression *mask_expression = left->binary.right;
    if (bits->kind != EXPRESSION_NAME || bits->name.symbol->register_name == NULL ||
        mask_expression->kind != EXPRESSION_INT || !int_operand(mask_expression, mask)) {
        return false;
    }
    *value = bits->name.symbol->register_name;
    return true;
}
