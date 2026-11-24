#include "codegen/codegen.h"

#include <inttypes.h>

bool emit_wide_offset(FunctionContext *context, Expression *expression, const char *destination) {
    if (expression->kind != EXPRESSION_BINARY || (expression->type != TYPE_LONG && expression->type != TYPE_ULONG)) {
        return false;
    }
    Operator operation = expression->binary.operator;
    const Expression *left = expression->binary.left;
    const Expression *right = expression->binary.right;
    if ((operation != OP_ADD && operation != OP_SUBTRACT) || left->kind != EXPRESSION_NAME ||
        left->name.symbol->register_name == NULL || right->kind != EXPRESSION_INT ||
        right->int_literal.value < INT32_MIN || right->int_literal.value > INT32_MAX) {
        return false;
    }
    int64_t offset = operation == OP_ADD ? right->int_literal.value : -right->int_literal.value;
    if (offset > INT32_MAX) {
        return false;
    }
    emit(context, "lea %s, [%s %c %" PRId64 "]", destination, left->name.symbol->register_name,
         offset < 0 ? '-' : '+', offset < 0 ? -offset : offset);
    return true;
}
